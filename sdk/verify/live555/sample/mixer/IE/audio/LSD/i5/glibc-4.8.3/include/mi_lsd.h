/*
* mi_lsd.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
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
