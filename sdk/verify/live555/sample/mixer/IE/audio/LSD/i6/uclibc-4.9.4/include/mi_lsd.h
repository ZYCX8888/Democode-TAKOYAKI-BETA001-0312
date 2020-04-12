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
#ifndef MI_LSD_H_
#define MI_LSD_H_

#include "mi_common.h"

#ifdef __cplusplus
extern "C"
{
#endif
//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================


typedef struct {
    U32 sample_rate;
    U32 channel;
} LSD_PARAMS;

typedef void* LSD_HANDLE;

LSD_HANDLE MI_LSD_Init(LSD_PARAMS *lsd_params, S32 *point_length);
MI_RET MI_LSD_Uninit(LSD_HANDLE lsd_handle);
MI_RET MI_LSD_Run(LSD_HANDLE lsd_handle, S16 *lsd_db_result);
MI_RET MI_LSD_SetThreshold(LSD_HANDLE lsd_handle, S32 threshold_db);
MI_RET MI_LSD_GetResult(LSD_HANDLE lsd_handle, S16 *lsd_result);
MI_RET MI_LSD_GetdBResult(LSD_HANDLE lsd_handle, S16 *audio_input, S16 *lsd_db_result);
#ifdef __cplusplus
}
#endif

#endif /* MI_LSD_H_ */
