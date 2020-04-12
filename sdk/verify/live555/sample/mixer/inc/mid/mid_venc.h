/*
* mid_venc.h- Sigmastar
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
#ifndef _MID_VENC_H_
#define _MID_VENC_H_

#ifdef __cplusplus
extern "C"{
#endif    // __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_venc.h"

MI_S32 Mixer_Venc_CreateChannel(MI_VENC_CHN VencChn, MI_VENC_ChnAttr_t *pstAttr);
MI_S32 Mixer_Venc_DestoryChannel(MI_VENC_CHN VencChn);

MI_S32 Mixer_Venc_StartChannel(MI_VENC_CHN VencChn);
MI_S32 Mixer_Venc_StopChannel(MI_VENC_CHN VencChn);

#ifdef __cplusplus
}
#endif    // __cplusplus

#endif //_MID_VENC_H_
