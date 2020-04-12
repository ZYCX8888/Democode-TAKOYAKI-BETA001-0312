/*
* mid_vif.h- Sigmastar
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
#ifndef _MID_VIF_H_
#define _MID_VIF_H_

#ifdef __cplusplus
extern "C"{
#endif    // __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mid_common.h"
#include "mi_sensor.h"
#include "mi_sys.h"
#include "mi_vif.h"



typedef struct Mixer_VIF_PortInfo_s
{
    MI_U32 u32RectX;
    MI_U32 u32RectY;
    MI_U32 u32RectWidth;
    MI_U32 u32RectHeight;
    MI_U32 u32DestWidth;
    MI_U32 u32DestHeight;
    MI_U32 u32IsInterlace;
    MI_S32 s32FrameRate;
    MI_SYS_PixelFormat_e ePixFormat;
} Mixer_VIF_PortInfo_T;

MI_S32 Mixer_Vif_EnableDev(MI_VIF_DEV VifDev, MI_VIF_HDRType_e eHdrType, MI_SNR_PADInfo_t *pstSnrPadInfo);
MI_S32 Mixer_Vif_DisableDev(MI_VIF_DEV VifDev);
MI_S32 Mixer_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);
MI_S32 Mixer_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, Mixer_VIF_PortInfo_T *pstPortInfoInfo);
MI_S32 Mixer_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);

MI_U8 Mixer_vifDevNumberGet(void);
void Mixer_vifDevNumberSet(MI_U8 num);
MI_U8 Mixer_GetSensorNum(void);
void Mixer_SetSensorNum(MI_U8 num);
BOOL  Mixer_GetSensor1GetRawDataThrState();
void Mixer_SetSensor1GetRawDataThrState(BOOL state);

#ifdef __cplusplus
}
#endif    // __cplusplus

#endif //_MID_VIF_H_
