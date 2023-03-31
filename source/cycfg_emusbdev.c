/*****************************************************************************
* File Name   : cycfg_emusbdev.c
*
* Description : This file contains the emUSB Audio class configurations.
*
* Note        : See README.md
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
#include "cycfg_emusbdev.h"
#include "audio.h"


/*****************************************************************************
* Global Variables
*****************************************************************************/
/* Device Descriptors Initialization*/
/* Information that is used during enumeration. */
const USB_DEVICE_INFO usb_deviceInfo =
{
    AUDIO_DEVICE_VENDOR_ID,       /* VendorId */
    AUDIO_DEVICE_PRODUCT_ID,      /* ProductId. Should be unique for this sample */
    "Infineon Technologies",      /* VendorName */
    "USB Audio Recorder",         /* ProductName */
    ""                            /* SerialNumber */
};

/*****************************************************************************
* Static Data
*****************************************************************************/
/* When changing these values check
*  https://wiki.segger.com/USB_Audio#Audio_class_issues_on_Windows
*
*  Also update MAX_AUDIO_IN_PACKET_SIZE_BYTES accordingly.
*/
static const USBD_AUDIO_FORMAT microphone_formats[] =
{
    {0, AUDIO_IN_NUM_CHANNELS, AUDIO_IN_SUB_FRAME_SIZE, AUDIO_IN_BIT_RESOLUTION, AUDIO_IN_SAMPLE_FREQ},
};

static USBD_AUDIO_UNITS microphone_units;

const USBD_AUDIO_IF_CONF audio_interfaces[] =
{
    /* Microphone config. */
    {
        0,                                  /* Flags */
        0x03,                               /* Controls */
        2,                                  /* TotalNrChannels */
        SEGGER_COUNTOF(microphone_formats), /* NumFormats */
        microphone_formats,                 /* paFormats */
        0x0003,                             /* bmChannelConfig (0x3: Left Front, Right Front) */
        USB_AUDIO_TERMTYPE_INPUT_MICROPHONE,/* TerminalType */
        &microphone_units                   /* pUnits */
    }
};

/* [] END OF FILE */
