/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_vpe.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: vpe module source file
*
*
*
* History:
*
*    1. Date  :        2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mid_vpe.h"
#include "module_common.h"
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#endif
#include "mid_utils.h"

extern BOOL g_bBiLinear;

static const struct mixer_rotation_t    mixerRotation[4][4]=
{
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
    {{0, 1}, {1, 1}, {0, 0}, {1, 0}},
    {{1, 1}, {0, 1}, {1, 0}, {0, 0}},
    {{1, 0}, {0, 0}, {1, 1}, {0, 1}},
};
MI_S8 GetMixerRotState(struct mixer_rotation_t *ptr, MI_SYS_Rotate_e eRotateType, MI_U32 nValue)
{
    if(NULL == ptr || E_MI_SYS_ROTATE_NUM <= eRotateType || nValue >= sizeof(mixerRotation[eRotateType])/sizeof(mixerRotation[eRotateType][0]))
        return -1;

    ptr->bMirror = mixerRotation[eRotateType][nValue].bMirror;
    ptr->bFlip =  mixerRotation[eRotateType][nValue].bFlip;
    return 0x0;
}
MI_S32 Mixer_Vpe_CreateChannel(MI_VPE_CHANNEL VpeChannel, Mixer_VPE_ChannelInfo_T *pstChannelInfo)
{


    MI_VPE_ChannelPara_t stVpeChnParam;
    MI_VPE_ChannelAttr_t stChannelVpeAttr;


    memset(&stVpeChnParam, 0x00, sizeof(MI_VPE_ChannelPara_t));

    if(E_MI_VPE_HDR_TYPE_OFF != pstChannelInfo->eHDRtype)
    {
        stVpeChnParam.eHDRType = pstChannelInfo->eHDRtype;
        //ExecFunc(MI_VPE_SetChannelParam(VpeChannel, &stVpeChnParam), MI_VPE_OK);
    }

    if((E_MI_VPE_RUN_REALTIME_MODE == pstChannelInfo->eRunningMode) ||
       (E_MI_VPE_RUN_REALTIME_TOP_MODE == pstChannelInfo->eRunningMode))
    {
        stVpeChnParam.e3DNRLevel = pstChannelInfo->e3DNRLevel;
        stVpeChnParam.bWdrEn = pstChannelInfo->e3DNRLevel ? TRUE : FALSE;
        //ExecFunc(MI_VPE_SetChannelParam(VpeChannel, &stVpeChnParam), MI_VPE_OK);
    }

    ExecFunc(MI_VPE_SetChannelParam(VpeChannel, &stVpeChnParam), MI_VPE_OK);

    memset(&stChannelVpeAttr, 0x00, sizeof(MI_VPE_ChannelAttr_t));
    stChannelVpeAttr.u16MaxW = pstChannelInfo->u16VpeMaxW;
    stChannelVpeAttr.u16MaxH = pstChannelInfo->u16VpeMaxH;
    stChannelVpeAttr.bNrEn = FALSE;
    stChannelVpeAttr.bEsEn = FALSE;
    stChannelVpeAttr.bEdgeEn = FALSE;
    stChannelVpeAttr.bUvInvert = FALSE;
    stChannelVpeAttr.bContrastEn = FALSE;
    stChannelVpeAttr.ePixFmt  = pstChannelInfo->eFormat;
    stChannelVpeAttr.eRunningMode = pstChannelInfo->eRunningMode;
    stChannelVpeAttr.bRotation = FALSE; //set rotation end of videoOpen() 
    stChannelVpeAttr.eSensorBindId = E_MI_VPE_SENSOR0;
#if TARGET_CHIP_I6E
    stChannelVpeAttr.bEnLdc = pstChannelInfo->bLdcEnable;
    stChannelVpeAttr.u32ChnPortMode = pstChannelInfo->u32ChnPortMode;
    MIXER_DBG("u32ChnPortMode:0x%x\n", stChannelVpeAttr.u32ChnPortMode);
#endif
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);



    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);


#if TARGET_CHIP_I5
    stChannelVpeAttr.bNrEn = TRUE;
    stChannelVpeAttr.bContrastEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);
#endif

#if TARGET_CHIP_I5
    MI_SYS_WindowRect_t stCropWinGet;
     MI_SYS_WindowRect_t stCropWin;
    stChannelVpeAttr.bNrEn = TRUE;
    stChannelVpeAttr.bContrastEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);


    memset(&stCropWin, 0x00, sizeof(MI_SYS_WindowRect_t));
    stCropWin.u16X = pstChannelInfo->u32X;
    stCropWin.u16Y = pstChannelInfo->u32Y;
    stCropWin.u16Width  = pstChannelInfo->u16VpeCropW;
    stCropWin.u16Height = pstChannelInfo->u16VpeCropH;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);

    memset(&stCropWinGet, 0x00, sizeof(MI_SYS_WindowRect_t));
    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWinGet), MI_VPE_OK);
    MIXER_INFO("Set CropWin(x:%d y:%d w:%d h:%d), Get CropWin(x:%d y:%d w:%d h:%d)\n",
                stCropWin.u16X, stCropWin.u16Y, stCropWin.u16Width, stCropWin.u16Height,
                stCropWinGet.u16X, stCropWinGet.u16Y, stCropWinGet.u16Width, stCropWinGet.u16Height);
#endif

    return MI_SUCCESS;
}


MI_S32 Mixer_Vpe_StartChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_StartChannel(VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vpe_StopChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_StopChannel(VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vpe_StartPort(MI_VPE_CHANNEL VpeCh, Mixer_VPE_PortInfo_T *pstPortInfo)
{
    MI_VPE_PortMode_t stVpeMode;
    MI_VPE_PortMode_t stVpeModeGet;

    memset(&stVpeMode, 0x00, sizeof(stVpeMode));
    stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stVpeMode.ePixelFormat  = pstPortInfo->ePixelFormat;

#if TARGET_CHIP_I5
    if((TRUE == g_bBiLinear) && ((pstPortInfo->u16OutputWidth <= 720) || (pstPortInfo->u16OutputHeight <= 576)))
    {
        //stVpeMode.u16Width  = 1280; //ALIGN_2xUP(m_width);
        //stVpeMode.u16Height = 720;  //ALIGN_2xUP(m_height);
        if(1280 < pstPortInfo->u16VpeOutputWidth)
        {
            stVpeMode.u16Width  = pstPortInfo->u16VpeOutputWidth;
            stVpeMode.u16Height = pstPortInfo->u16VpeOutputHeight;
        }
        else
        {
            stVpeMode.u16Width  = 1280;
            stVpeMode.u16Height = 720;
        }
    }
    else
    {
        if(pstPortInfo->u16OutputWidth < pstPortInfo->u16VpeOutputWidth)
        {
            stVpeMode.u16Width    = pstPortInfo->u16VpeOutputWidth;
            stVpeMode.u16Height = pstPortInfo->u16VpeOutputHeight;
        }
        else
        {
            stVpeMode.u16Width    = pstPortInfo->u16OutputWidth;
            stVpeMode.u16Height = pstPortInfo->u16OutputHeight;
        }
    }
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
     stVpeMode.u16Width  = pstPortInfo->u16VpeOutputWidth;
     stVpeMode.u16Height = pstPortInfo->u16VpeOutputHeight;
#endif

#if TARGET_CHIP_I5
        stVpeMode.u16Width  = ALIGN_8xUP(stVpeMode.u16Width);
            stVpeMode.u16Height = ALIGN_2xUP(stVpeMode.u16Height);
#elif TARGET_CHIP_I6
        stVpeMode.u16Width  = ALIGN_32xUP(stVpeMode.u16Width);
            stVpeMode.u16Height = ALIGN_2xUP(stVpeMode.u16Height);
#elif TARGET_CHIP_I6E
        stVpeMode.u16Width    = ALIGN_8xUP(stVpeMode.u16Width);
        stVpeMode.u16Height = ALIGN_2xUP(stVpeMode.u16Height);
#elif TARGET_CHIP_I6B0
        stVpeMode.u16Width    = ALIGN_8xUP(stVpeMode.u16Width);
        stVpeMode.u16Height = ALIGN_2xUP(stVpeMode.u16Height);
#endif

    ExecFunc(MI_VPE_SetPortMode(pstPortInfo->VpeChnPort.u32ChnId, pstPortInfo->VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);

    memset(&stVpeModeGet, 0x00, sizeof(stVpeModeGet));
    ExecFunc(MI_VPE_GetPortMode(pstPortInfo->VpeChnPort.u32ChnId, pstPortInfo->VpeChnPort.u32PortId, &stVpeModeGet), MI_VPE_OK);
    MIXER_DBG("Set VpeMode(w:%d h:%d PixelFormat:%d CompressMode:%d), Get VpeMode(w:%d h:%d PixelFormat:%d CompressMode:%d)\n",
                stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat, stVpeMode.eCompressMode,
                stVpeModeGet.u16Width, stVpeModeGet.u16Height, stVpeModeGet.ePixelFormat, stVpeModeGet.eCompressMode);

#if TARGET_CHIP_I6
    MI_SYS_WindowRect_t stPortCrop;

    if(0 == pstPortInfo->VpeChnPort.u32PortId)
    {
        stPortCrop.u16X = 0;
        stPortCrop.u16Y = 0;
        stPortCrop.u16Width  = pstPortInfo->u16OutputWidth;
        stPortCrop.u16Height = pstPortInfo->u16OutputHeight;
        ExecFunc(MI_VPE_SetPortCrop(pstPortInfo->VpeChnPort.u32ChnId, pstPortInfo->VpeChnPort.u32PortId, &stPortCrop), MI_VPE_OK);
    }
#endif

    ExecFunc(MI_VPE_EnablePort(pstPortInfo->VpeChnPort.u32ChnId, pstPortInfo->VpeChnPort.u32PortId), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 Mixer_Vpe_StopPort(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    return MI_SUCCESS;
}

void  Mixer_GetVpeOutportSize(MI_VPE_PORT VpePortId, Size_t * spMaxSize)
{
    MI_U32 u32Maxwidth  = 0;
    MI_U32 u32Maxheight = 0;

     if(NULL == spMaxSize)
        {
            MIXER_ERR("mixer input point param is null!\n");
            return ;
        }

    if(VpePortId == 2)
    {
        u32Maxwidth  = 1920;
        u32Maxheight = 1080;
    }
    else if(VpePortId == 1)
    {
        u32Maxwidth  = 1280;
        u32Maxheight = 720;
    }
    else
    {
        u32Maxwidth  = 3840;
        u32Maxheight = 2160;
    }

    spMaxSize->width  = u32Maxwidth;
    spMaxSize->height = u32Maxheight;
}

