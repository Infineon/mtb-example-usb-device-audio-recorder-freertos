/******************************************************************************
* File Name   : audio_app.c
*
* Description : This file contains the implementation of adding audio interface
*               and main audio process task.
*
* Note        : See README.md
*
*******************************************************************************
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
******************************************************************************/
#include "audio_app.h"
#include "audio_in.h"
#include "audio.h"
#include "cybsp.h"
#include "cycfg_emusbdev.h"
#include "cy_retarget_io.h"

#include "rtos.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Polling interval for the endpoint */
#define EP_IN_INTERVAL               (8U)

#define ONE_BYTE                     (1)
#define THREE_BYTES                  (3)
#define DELAY_TICKS                  (50U)


/*******************************************************************************
* Global Variables
********************************************************************************/
/* RTOS task handles */
TaskHandle_t rtos_audio_app_task;
TaskHandle_t rtos_audio_in_task;


/*******************************************************************************
* Static data
*******************************************************************************/
static USBD_AUDIO_HANDLE handle;
static USBD_AUDIO_INIT_DATA init_data;
static USBD_AUDIO_IF_CONF* microphone_config = (USBD_AUDIO_IF_CONF *) &audio_interfaces[0];
static uint8_t current_mic_format_index;
static volatile bool usb_suspend_flag = false;


/********************************************************************************
 * Function Name: vApplicationTickHook
 ********************************************************************************
 * Summary:
 *  Tick hook function called at every tick (1ms). It checks for the activity in 
 *  the USB bus.
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
void vApplicationTickHook(void) 
{
    /* Supervisor of suspend conditions on the bus */
#if defined (COMPONENT_CAT1A)
    USB_DRIVER_Cypress_PSoC6_SysTick();
#endif /* COMPONENT_CAT1A */

    if (USBD_GetState() & USB_STAT_SUSPENDED)
    {
        /* Suspend condition on USB bus is detected */
        usb_suspend_flag = true;
    }
    else
    {
        /* Clear suspend conditions */
        usb_suspend_flag = false;
    }
}

/*******************************************************************************
* Function Name: audio_control_callback
********************************************************************************
* Summary:
*  Callback called in ISR context.
*  Receives audio class control commands and sends appropriate responses
*  where necessary.
*
* Parameters:
*  pUserContext: User context which is passed to the callback.
*  Event: Audio event ID.
*  Unit: ID of the feature unit. In case of USB_AUDIO_PLAYBACK_*
*        and USB_AUDIO_RECORD_*: 0.
*  ControlSelector: ID of the control. In case of USB_AUDIO_PLAYBACK_* and
*                   USB_AUDIO_RECORD_*: 0.
*  pBuffer: In case of GET events: pointer to a buffer into which the
*           callback should write the reply. In case of SET events: pointer
*           to a buffer containing the command value. In case of
*           USB_AUDIO_PLAYBACK_* and USB_AUDIO_RECORD_*: NULL.
*  NumBytes: In case of GET events: requested size of the reply in bytes.
*            In case of SET events: number of bytes in pBuffer. In case
*            of USB_AUDIO_PLAYBACK_* and USB_AUDIO_RECORD_*: 0.
*  InterfaceNo: The number of the USB interface for which the event was issued.
*  AltSetting: The alternative setting number of the USB interface for
*              which the event was issued.
*
* Return:
*  int: =0 Audio command was handled by the callback, ≠ 0 Audio command was
*       not handled by the callback. The stack will STALL the request.
*
*******************************************************************************/
static int audio_control_callback(void *pUserContext,
                                  U8   Event,
                                  U8   Unit,
                                  U8   ControlSelector,
                                  U8   *pBuffer,
                                  U32  NumBytes,
                                  U8   InterfaceNo,
                                  U8   AltSetting)
{
    int retVal;

    CY_UNUSED_PARAMETER(pUserContext);
    CY_UNUSED_PARAMETER(InterfaceNo);

    retVal = 0;
    switch (Event)
    {
        case USB_AUDIO_RECORD_START:
            /* Host enabled reception */
            audio_in_enable();
            break;

        case USB_AUDIO_RECORD_STOP:
            /* Host disabled reception. Some hosts do not always send this! */
            audio_in_disable();
            break;

        case USB_AUDIO_PLAYBACK_START:
        case USB_AUDIO_PLAYBACK_STOP:
            break;

        case USB_AUDIO_SET_CUR:
            switch (ControlSelector)
            {
                case USB_AUDIO_MUTE_CONTROL:
                    if (ONE_BYTE == NumBytes) 
                    {
                        if (Unit == microphone_config->pUnits->FeatureUnitID) 
                        {
                            mic_mute = *pBuffer;
                        }
                    }
                    break;

                case USB_AUDIO_VOLUME_CONTROL:
                    break;

                case USB_AUDIO_SAMPLING_FREQ_CONTROL:
                    if (THREE_BYTES == NumBytes)
                    {
                        if (Unit == microphone_config->pUnits->FeatureUnitID)
                        {
                            if ((AltSetting > 0) && (AltSetting < microphone_config->NumFormats))
                            {
                                current_mic_format_index = AltSetting-1;
                            }
                        }
                    }
                    break;

                default:
                    retVal = 1;
                    break;
            }
            break;

        case USB_AUDIO_GET_CUR:
            switch (ControlSelector)
            {
                case USB_AUDIO_MUTE_CONTROL:
                    pBuffer[0] = 0;
                    break;

                case USB_AUDIO_VOLUME_CONTROL:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;

                case USB_AUDIO_SAMPLING_FREQ_CONTROL:
                    if (Unit == microphone_config->pUnits->FeatureUnitID)
                    {
                        pBuffer[0] = microphone_config->paFormats[current_mic_format_index].SamFreq & 0xff;
                        pBuffer[1] = (microphone_config->paFormats[current_mic_format_index].SamFreq >> 8) & 0xff;
                        pBuffer[2] = (microphone_config->paFormats[current_mic_format_index].SamFreq >> 16) & 0xff;
                    }
                    break;

                default:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;
            }           
            break;

        case USB_AUDIO_SET_MIN:
        case USB_AUDIO_SET_MAX:
        case USB_AUDIO_SET_RES:
            break;

        case USB_AUDIO_GET_MIN:
            switch (ControlSelector)
            {
                case USB_AUDIO_VOLUME_CONTROL:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0xf1;
                    break;

                default:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;
            }
            break;

        case USB_AUDIO_GET_MAX:
            switch (ControlSelector)
            {
                case USB_AUDIO_VOLUME_CONTROL:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;

                default:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;
            }
            break;

        case USB_AUDIO_GET_RES:
            switch (ControlSelector)
            {
                case USB_AUDIO_VOLUME_CONTROL:
                    pBuffer[0] = 0;
                    pBuffer[1] = 1;
                    break;

                default:
                    pBuffer[0] = 0;
                    pBuffer[1] = 0;
                    break;
            }
            break;
        
         default:
            retVal = 1;
            break;
    }

    return retVal;
}

/*******************************************************************************
* Function Name: add_audio
********************************************************************************
* Summary:
*  Add a USB Audio interface to the USB stack.
*
* Parameters:
*  None
*
* Return:
*  USBD_AUDIO_HANDLE
*
*******************************************************************************/
static USBD_AUDIO_HANDLE add_audio(void)
{
    USB_ADD_EP_INFO       EPIn;
    USBD_AUDIO_HANDLE     handle;

    memset(&EPIn, 0x0, sizeof(EPIn));
    memset(&init_data, 0x0, sizeof(init_data));

    EPIn.MaxPacketSize               = MAX_AUDIO_IN_PACKET_SIZE_BYTES;       /* Max packet size for IN endpoint (in bytes) */
    EPIn.Interval                    = EP_IN_INTERVAL;                       /* Interval of 1 ms (8 * 125us) */
    EPIn.Flags                       = USB_ADD_EP_FLAG_USE_ISO_SYNC_TYPES;   /* Optional parameters */
    EPIn.InDir                       = USB_DIR_IN;                           /* IN direction (Device to Host) */
    EPIn.TransferType                = USB_TRANSFER_TYPE_ISO;                /* Endpoint type - Isochronous. */
    EPIn.ISO_Type                    = USB_ISO_SYNC_TYPE_ASYNCHRONOUS;       /* Async for isochronous endpoints */

    init_data.EPIn                   = USBD_AddEPEx(&EPIn, NULL, 0);
    init_data.EPOut                  = 0U;
    init_data.OutPacketSize          = 0U;
    init_data.pfOnOut                = NULL;
    init_data.pfOnIn                 = audio_in_endpoint_callback;
    init_data.pfOnControl            = audio_control_callback;
    init_data.pControlUserContext    = NULL;
    init_data.NumInterfaces          = SEGGER_COUNTOF(audio_interfaces);
    init_data.paInterfaces           = audio_interfaces;
    init_data.pOutUserContext        = NULL;
    init_data.pInUserContext         = NULL;

    handle = USBD_AUDIO_Add(&init_data);

    return handle;
}

/*******************************************************************************
* Function Name: audio_app_init
********************************************************************************
* Summary:
*  Invokes "audio_clock_init" to initialize the audio subsystem clock and create 
*  the RTOS task "Audio App Task".
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_app_init(void)
{
    BaseType_t rtos_task_status;

    /* Initialize the audio clock based on audio sample rate */
    audio_clock_init();

    /* Create the AUDIO APP RTOS task */
    rtos_task_status = xTaskCreate(audio_app_task, "Audio App Task", AUDIO_TASK_STACK_DEPTH, NULL,
                       AUDIO_APP_TASK_PRIORITY, &rtos_audio_app_task);
    if (pdPASS != rtos_task_status)
    {
        CY_ASSERT(0);
    }
}

/*******************************************************************************
* Function Name: audio_app_task
********************************************************************************
* Summary:
*  Main audio task. Initializes the USB communication and the audio application.
*  In the main loop, checks USB device connectivity and based on that start/stop
*  providing audio data to the host.
*
* Parameters:
*  arg
*
* Return:
*  None
*
*******************************************************************************/
void audio_app_task(void *arg)
{
    volatile bool usb_suspended = false;
    volatile bool usb_connected = false;

    CY_UNUSED_PARAMETER(arg);

    USBD_Init();

    handle = add_audio();

    USBD_SetDeviceInfo(&usb_deviceInfo);

    USBD_AUDIO_Set_Timeouts(handle, 0, WRITE_TIMEOUT);

    /* Init the audio IN application */
    audio_in_init();

    USBD_Start();

    /* Make device appear on the bus. This function call is blocking,
     * toggle the kit user LED until device gets enumerated.
     */
    while (USB_STAT_CONFIGURED != (USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)))
    {
        cyhal_gpio_toggle(CYBSP_USER_LED);
        USB_OS_Delay(50);
    }
   
    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);

    /* Start providing audio data to the host */
    USBD_AUDIO_Start_Play(handle, NULL);

    for (;;)
    {
        /* Check if suspend condition is detected on the bus */
        if (usb_suspend_flag)
        {
            if (!usb_suspended)
            {
                usb_suspended = true;
                usb_connected = false;

                /* Stop providing audio data to the host */
                USBD_AUDIO_Stop_Play(handle);

                printf("APP_LOG: USB Audio Device Disconnected\r\n");
            }
        }
        else /* USB device connected */
        {
            if (!usb_connected)
            {
                usb_connected = true;
                usb_suspended = false;

                /* Start providing audio data to the host */
                USBD_AUDIO_Start_Play(handle, NULL);

                printf("APP_LOG: USB Audio Device Connected\r\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DELAY_TICKS));
    }
}

/* [] END OF FILE */
