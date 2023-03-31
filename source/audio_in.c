/*****************************************************************************
* File Name    : audio_in.c
*
* Description  : This file contains the Audio In path configuration and
*                processing code.
*
* Note         : See README.md
*
******************************************************************************
* Copyright 2022-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*****************************************************************************/
#include "audio_in.h"
#include "audio.h"
#include "cycfg_emusbdev.h"
#include "cy_retarget_io.h"
#include "cyhal.h"
#include "cybsp.h"

#include "rtos.h"


/*****************************************************************************
* Macros
*****************************************************************************/
/* PDM/PCM Pins */
#ifndef CYBSP_PDM_DATA
    #define CYBSP_PDM_DATA          CYBSP_A5
#endif
#ifndef CYBSP_PDM_CLK
    #define CYBSP_PDM_CLK           CYBSP_A4
#endif

/* Decimation Rate of the PDM/PCM block */
#define DECIMATION_RATE             (64U)

/* Audio Subsystem Clock. Typical values depends on the desired sample rate:
 * 8KHz / 16 KHz / 32 KHz / 48 KHz    : 24.576 MHz
 * 22.05 KHz / 44.1 KHz               : 22.579 MHz 
 */
#if ((AUDIO_SAMPLING_RATE_22KHZ == AUDIO_IN_SAMPLE_FREQ) || (AUDIO_SAMPLING_RATE_44KHZ == AUDIO_IN_SAMPLE_FREQ))
#define AUDIO_SYS_CLOCK_HZ                  (22579200U)
#else 
#define AUDIO_SYS_CLOCK_HZ                  (24576000U) 
#endif /* ((AUDIO_SAMPLING_RATE_22KHZ == AUDIO_IN_SAMPLE_FREQ) || (AUDIO_SAMPLING_RATE_44KHZ == AUDIO_IN_SAMPLE_FREQ)) */


/*****************************************************************************
* Global Variables
*****************************************************************************/
/* PCM buffer data (16-bits) */
uint16_t audio_in_pcm_buffer_ping[(MAX_AUDIO_IN_PACKET_SIZE_WORDS)];
uint16_t audio_in_pcm_buffer_pong[(MAX_AUDIO_IN_PACKET_SIZE_WORDS)];

/* Audio IN flags */
volatile bool audio_in_start_recording = false;
volatile bool audio_in_is_recording    = false;

/* Mic mute status */
U8 mic_mute;

/* HAL object */
cyhal_pdm_pcm_t pdm_pcm;
static cyhal_clock_t audio_clock;

/* HAL Config for pdm_pcm */
const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate     = AUDIO_IN_SAMPLE_FREQ,
    .decimation_rate = DECIMATION_RATE,
    .mode            = CYHAL_PDM_PCM_MODE_STEREO,
    .word_length     = AUDIO_IN_BIT_RESOLUTION,  /* bits */
    .left_gain       = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
    .right_gain      = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
};


/*****************************************************************************
* Static const data
*****************************************************************************/
const unsigned char silent_frame[MAX_AUDIO_IN_PACKET_SIZE_BYTES] = {0};

/*****************************************************************************
* Function Name: audio_in_init
******************************************************************************
* Summary:
*  Initialize the PDM/PCM block and create the "Audio In Task" which will
*  process the Audio IN endpoint transactions.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_init(void)
{
    BaseType_t rtos_task_status;

    /* Initialize the PDM PCM block */
    cyhal_pdm_pcm_init(&pdm_pcm, CYBSP_PDM_DATA, CYBSP_PDM_CLK, &audio_clock, &pdm_pcm_cfg);

    /* Create the AUDIO Write RTOS task */
    rtos_task_status = xTaskCreate(audio_in_process, "Audio In Task", AUDIO_TASK_STACK_DEPTH, NULL,
            AUDIO_WRITE_TASK_PRIORITY, &rtos_audio_in_task);
    if (pdPASS != rtos_task_status)
    {
        CY_ASSERT(0);
    }
}

/*****************************************************************************
* Function Name: audio_in_enable
******************************************************************************
* Summary:
*  Start a recording session.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_enable(void)
{
    audio_in_start_recording = true;
    /* Turn ON the kit LED to indicate start of a recording session */
    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
}

/*****************************************************************************
* Function Name: audio_in_disable
******************************************************************************
* Summary:
*  Stop a recording session.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_disable(void)
{
    audio_in_is_recording = false;
    /* Turn OFF the kit LED to indicate the end of the recording session */
    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
}

/*****************************************************************************
* Function Name: audio_in_process
******************************************************************************
* Summary:
*  Wrapper task for USBD_AUDIO_Write_Task (audio in endpoint).
*
* Parameters:
*  arg
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_process(void *arg)
{
    CY_UNUSED_PARAMETER(arg);

    USBD_AUDIO_Write_Task();

    for (;;)
    {
    }
}

/*****************************************************************************
* Function Name: audio_in_endpoint_callback
******************************************************************************
* Summary:
*  Callback called in the context of USBD_AUDIO_Write_Task.
*  Handles data sent to the host (IN direction).
*
* Parameters:
*  pUserContext: User context which is passed to the callback.
*  ppNextBuffer: Buffer containing audio samples which should match the
*                configuration from microphone USBD_AUDIO_IF_CONF.
*  pNextPacketSize: Size of the next buffer.
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_endpoint_callback(void *pUserContext,
                                const U8 **ppNextBuffer,
                                U32 *pNextPacketSize)
{
    unsigned int sample_size;
    size_t audio_in_count;
    static uint16_t *audio_in_pcm_buffer = NULL;

    CY_UNUSED_PARAMETER(pUserContext);

    /*
     * Packet size minus the additional audio frames reserved in MAX_AUDIO_IN_PACKET_SIZE_BYTES.
     * The application can periodically increase SampleSize to counterbalance differences between the
     * regular sample size and the actual byte rate.
     */
    sample_size = ((MAX_AUDIO_IN_PACKET_SIZE_BYTES) - (ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_BYTES));

    
    if (audio_in_start_recording)
    {
        audio_in_start_recording = false;
        audio_in_is_recording = true;

        /* Clear Audio In buffer */
        memset(audio_in_pcm_buffer_ping, 0, (MAX_AUDIO_IN_PACKET_SIZE_BYTES));

        audio_in_pcm_buffer = audio_in_pcm_buffer_ping;

        /* Clear PDM/PCM RX FIFO */
        cyhal_pdm_pcm_clear(&pdm_pcm);

        /* Start PDM/PCM */
        cyhal_pdm_pcm_start(&pdm_pcm);

        /* Start a transfer to the Audio IN endpoint */
        *ppNextBuffer = (uint8_t *) audio_in_pcm_buffer;
        *pNextPacketSize = sample_size;
    }
    else if (audio_in_is_recording) /* Check if should keep recording */
    {
        if (audio_in_pcm_buffer == audio_in_pcm_buffer_ping)
        {
            audio_in_pcm_buffer = audio_in_pcm_buffer_pong;
        }
        else
        {
            audio_in_pcm_buffer = audio_in_pcm_buffer_ping;
        }

        /* Setup the number of bytes to transfer based on the current FIFO level */
        if (Cy_PDM_PCM_GetNumInFifo(pdm_pcm.base) > (MAX_AUDIO_IN_PACKET_SIZE_WORDS))
        {
            audio_in_count = (MAX_AUDIO_IN_PACKET_SIZE_WORDS);
        }
        else
        {
            audio_in_count = ((MAX_AUDIO_IN_PACKET_SIZE_WORDS) - (ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_WORDS));
        }

        /* Read all the data in the PDM/PCM buffer */
        cyhal_pdm_pcm_read(&pdm_pcm, (void *) audio_in_pcm_buffer, &audio_in_count);

        if (1U == mic_mute)
        {
            /* Send silent frames in case of mute */
            *ppNextBuffer = silent_frame;
        }
        else
        {
            /* Send captured audio samples to the Audio IN endpoint */
            *ppNextBuffer = (uint8_t *) audio_in_pcm_buffer;
        }
        *pNextPacketSize = audio_in_count * (AUDIO_IN_SUB_FRAME_SIZE);
    }
}

/*******************************************************************************
* Function Name: audio_clock_init
********************************************************************************
* Summary:
*  Initializes clock for audio subsystem.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_clock_init(void)
{
    cy_rslt_t result;
    cyhal_clock_t clock_pll;

    /* Initialize, take ownership of PLL0/PLL */
    result = cyhal_clock_reserve(&clock_pll, &CYHAL_CLOCK_PLL[0]);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Set the PLL0/PLL frequency to AUDIO_SYS_CLOCK_HZ based on AUDIO_IN_SAMPLE_FREQ */
    result = cyhal_clock_set_frequency(&clock_pll, AUDIO_SYS_CLOCK_HZ, NULL);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* If the PLL0/PLL clock is not already enabled, enable it */
    if (!cyhal_clock_is_enabled(&clock_pll))
    {
        result = cyhal_clock_set_enabled(&clock_pll, true, true);
        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
    }

    /* Initialize, take ownership of CLK_HF1 */
    result = cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Source the audio subsystem clock (CLK_HF1) from PLL0/PLL */
    result = cyhal_clock_set_source(&audio_clock, &clock_pll);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Set the divider for audio subsystem clock (CLK_HF1) */
    result = cyhal_clock_set_divider(&audio_clock, 1);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* If the audio subsystem clock (CLK_HF1) is not already enabled, enable it */
    if (!cyhal_clock_is_enabled(&audio_clock))
    {
        result = cyhal_clock_set_enabled(&audio_clock, true, true);
        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
    }
}

/* [] END OF FILE */
