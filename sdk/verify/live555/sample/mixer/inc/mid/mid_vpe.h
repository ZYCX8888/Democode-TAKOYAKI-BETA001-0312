/*
* mid_vpe.h- Sigmastar
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
#ifndef _MID_VPE_H_
#define _MID_VPE_H_

#ifdef __cplusplus
extern "C"{
#endif    // __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_vpe.h"
#include "mi_vif.h"
#include "mid_common.h"
#if TARGET_CHIP_I6 ||TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#include "mi_sys_datatype.h"
#endif
enum{
    MIRROR_ON = true,
    MIRROR_OFF = false,
    FLIP_ON = true,
    FLIP_OFF = false,
};
typedef enum{
    LEVEL0 = 0,
    LEVEL1 = 1,
    LEVEL2 = 2,
    LEVEL3 = 3,
    LEVELMax = 4,
}MirrorFlipLevel;

struct mixer_rotation_t
{
    MI_BOOL bMirror;
    MI_BOOL bFlip;
};
typedef struct Mixer_VPE_ChannelInfo_s
{
    MI_U16 u16VpeMaxW;
    MI_U16 u16VpeMaxH;
    MI_U16 u16VpeCropW;
    MI_U16 u16VpeCropH;
    MI_S32 u32X;
    MI_S32 u32Y;
    MI_SYS_PixelFormat_e eFormat;
    MI_VPE_RunningMode_e eRunningMode;
    MI_BOOL bRotation;
    MI_VPE_HDRType_e eHDRtype;
    MI_VPE_3DNR_Level_e e3DNRLevel;
#if TARGET_CHIP_I6 ||TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    MI_SYS_Rotate_e eRotateType;
#endif
#if TARGET_CHIP_I6E
    MI_BOOL  bLdcEnable;
    MI_U32   u32ChnPortMode;
#endif
} Mixer_VPE_ChannelInfo_T;

typedef struct Mixer_VPE_PortInfo_s
{
    MI_SYS_ChnPort_t      VpeChnPort;
    MI_U16                u16OutputWidth;     // Width  of target image
    MI_U16                u16OutputHeight;    // Height of target image
    MI_U16                u16VpeOutputWidth;  // Width  of VPE output port
    MI_U16                u16VpeOutputHeight; // Height of VPE output port

    MI_SYS_PixelFormat_e  ePixelFormat;       // Pixel format of target image
    MI_SYS_CompressMode_e eCompressMode;      // Compression mode of the output
} Mixer_VPE_PortInfo_T;

//vpe channel
MI_S32 Mixer_Vpe_CreateChannel(MI_VPE_CHANNEL VpeChannel, Mixer_VPE_ChannelInfo_T *pstChannelInfo);
MI_S32 Mixer_Vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel);
MI_S32 Mixer_Vpe_SetChannelAttr(MI_VPE_CHANNEL VpeChannel, Mixer_VPE_ChannelInfo_T *pstChannelInfo);
MI_S32 Mixer_Vpe_StartChannel(MI_VPE_CHANNEL VpeChannel);
MI_S32 Mixer_Vpe_StopChannel(MI_VPE_CHANNEL VpeChannel);

//vpe port
MI_S32 Mixer_Vpe_StartPort(MI_VPE_CHANNEL VpeCh, Mixer_VPE_PortInfo_T *pstPortInfo);
MI_S32 Mixer_Vpe_StopPort(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort);
void  Mixer_GetVpeOutportSize(MI_VPE_PORT VpePortId, Size_t * spMaxSize);
MI_S8 GetMixerRotState(struct mixer_rotation_t *ptr, MI_SYS_Rotate_e eRotateType, MI_U32 nValue);

extern  MI_U8 GetMirrorFlipLevel();
#ifdef __cplusplus
}
#endif    // __cplusplus

#endif //_MID_VPE_H_
