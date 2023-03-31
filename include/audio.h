/******************************************************************************
* File Name   : audio.h
*
* Description : This file contains the constants mapped to the USB descriptor.
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
#ifndef AUDIO_H
#define AUDIO_H

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************
* Constants from USB Audio Descriptor
******************************************************************************/
#define AUDIO_SAMPLING_RATE_16KHZ               (16000U)
#define AUDIO_SAMPLING_RATE_22KHZ               (22050U)
#define AUDIO_SAMPLING_RATE_32KHZ               (32000U)
#define AUDIO_SAMPLING_RATE_44KHZ               (44100U)

/* Initialization data for a single audio format */
#define AUDIO_IN_NUM_CHANNELS                   (2U)
#define AUDIO_IN_SUB_FRAME_SIZE                 (2U)   /* In bytes */
#define AUDIO_IN_BIT_RESOLUTION                 (16U)
#define AUDIO_IN_SAMPLE_FREQ                    AUDIO_SAMPLING_RATE_44KHZ

/* VendorID */
#define AUDIO_DEVICE_VENDOR_ID                  (0x058B)

/* ProductIDs */
#if (AUDIO_SAMPLING_RATE_16KHZ == AUDIO_IN_SAMPLE_FREQ)
#define AUDIO_DEVICE_PRODUCT_ID                 (0x0276)
#elif (AUDIO_SAMPLING_RATE_22KHZ == AUDIO_IN_SAMPLE_FREQ)
#define AUDIO_DEVICE_PRODUCT_ID                 (0x0277)
#elif (AUDIO_SAMPLING_RATE_32KHZ == AUDIO_IN_SAMPLE_FREQ)
#define AUDIO_DEVICE_PRODUCT_ID                 (0x0278)
#elif (AUDIO_SAMPLING_RATE_44KHZ == AUDIO_IN_SAMPLE_FREQ)
#define AUDIO_DEVICE_PRODUCT_ID                 (0x0279)
#else
#error "Sample rate not supported in this code example."
#endif


/*
 * Has to match the configured values in Microphone Configuration
 * For a sample rate of 44100, 16 bits per sample, 2 channels:
 * (44100 * ((16/8) * 2)) / 1000 = 176 bytes
 * Additional sample size is added to make sure we can send odd sized frames if necessary:
 * 176 bytes + ((16/8) * 2) = 180
 */

#define ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_BYTES   (((AUDIO_IN_BIT_RESOLUTION) / 8U) * (AUDIO_IN_SUB_FRAME_SIZE)) /* In bytes */

#define MAX_AUDIO_IN_PACKET_SIZE_BYTES          ((((AUDIO_IN_SAMPLE_FREQ) * (((AUDIO_IN_BIT_RESOLUTION) / 8U) * (AUDIO_IN_NUM_CHANNELS))) / 1000U) + (ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_BYTES)) /* In bytes */

#define ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_WORDS   ((ADDITIONAL_AUDIO_IN_SAMPLE_SIZE_BYTES) / (AUDIO_IN_SUB_FRAME_SIZE)) /* In words */

#define MAX_AUDIO_IN_PACKET_SIZE_WORDS          ((MAX_AUDIO_IN_PACKET_SIZE_BYTES) / (AUDIO_IN_SUB_FRAME_SIZE)) /* In words */


#if defined(__cplusplus)
}
#endif

#endif /* AUDIO_H */

/* [] END OF FILE */
