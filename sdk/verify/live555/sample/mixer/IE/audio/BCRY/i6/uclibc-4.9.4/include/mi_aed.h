/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef MI_AED_H_
#define MI_AED_H_

#include "mi_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

//==============================================================================
//
//                              ENUMERATIONS
//
//==============================================================================

typedef enum {
    AED_SEN_LOW,
    AED_SEN_MID,
    AED_SEN_HIGH
} AedSensitivity;


//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================


typedef struct {
    unsigned int sample_rate;
    unsigned int channel;
    int enable_nr;
} AedParams;

typedef void* AED_HANDLE;

AED_HANDLE MI_AED_Init(AedParams *aed_params, S32 *point_length);
void MI_AED_Uninit(AED_HANDLE aedHandle);
MI_RET MI_AED_Run(AED_HANDLE aedHandle, S16 *audio_input);
MI_RET MI_AED_GetResult(AED_HANDLE aedHandle);
MI_RET MI_AED_SetSensitivity(AED_HANDLE aedHandle, AedSensitivity sensitivity);
MI_RET MI_AED_SetOperatingPoint(AED_HANDLE aedHandle, S32 operating_point);
MI_RET MI_AED_SetVadThreshold(AED_HANDLE aedHandle, S32 threshold_db);
MI_RET MI_AED_RunLsd(AED_HANDLE aedHandle, S16 *audio_input, S32 agc_gain);
MI_RET MI_AED_SetLsdThreshold(AED_HANDLE aedHandle, S32 threshold_db);
MI_RET MI_AED_GetLsdResult(AED_HANDLE aedHandle);

#ifdef __cplusplus
}
#endif

#endif /* MI_AED_H_ */
