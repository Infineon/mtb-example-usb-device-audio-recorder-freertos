/******************************************************************************
* File Name   : audio_in.h
*
* Description : This file contains the Audio In path routine declarations and
*               constants.
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
#ifndef AUDIO_IN_H
#define AUDIO_IN_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "cyhal.h"
#include "Global.h"


/******************************************************************************
* Externs
******************************************************************************/
extern cyhal_pdm_pcm_t pdm_pcm;
extern U8 mic_mute;


/******************************************************************************
* Audio In Functions
******************************************************************************/
void audio_in_init(void);
void audio_in_enable(void);
void audio_in_disable(void);
void audio_in_process(void *arg);
void audio_in_endpoint_callback(void *pUserContext, const U8 **ppNextBuffer, U32 *pNextPacketSize);
void audio_clock_init(void);


#if defined(__cplusplus)
}
#endif

#endif /* AUDIO_IN_H */

/* [] END OF FILE */
