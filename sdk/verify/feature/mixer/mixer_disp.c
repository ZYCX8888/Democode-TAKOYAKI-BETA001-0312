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
#include "mixer.h"

static int mixer_disp_create(MI_DISP_Interface_e eInf)
{
    MI_S32 s32Ret = 0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_LAYER DispLayer = 0;

    DispDev = DispLayer = 1;
    if(eInf == E_MI_DISP_INTF_HDMI)
        DispDev = DispLayer = 0;

    ExecFunc(MI_DISP_GetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
    stPubAttr.eIntfType = eInf;
    ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Enable(DispDev), MI_DISP_SUCCESS);

    ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);

    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    stLayerAttr.stVidLayerSize.u16Width = 1920;
    stLayerAttr.stVidLayerSize.u16Height = 1080;
    stLayerAttr.stVidLayerDispWin.u16X = 0;
    stLayerAttr.stVidLayerDispWin.u16Y = 0;
    stLayerAttr.stVidLayerDispWin.u16Width  = 1920;
    stLayerAttr.stVidLayerDispWin.u16Height = 1080;
    stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    DBG_INFO("Get Video Layer Size [%d, %d] !!!\n", stLayerAttr.stVidLayerSize.u16Width, stLayerAttr.stVidLayerSize.u16Height);
    DBG_INFO("Get Video Layer DispWin [%d, %d, %d, %d] !!!\n", stLayerAttr.stVidLayerDispWin.u16X, stLayerAttr.stVidLayerDispWin.u16Y, stLayerAttr.stVidLayerDispWin.u16Width, stLayerAttr.stVidLayerDispWin.u16Height);
    DBG_INFO("Get Video Layer format [%d] !!!\n", stLayerAttr.ePixFormat);
    ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_DISP_SUCCESS);

    return s32Ret;
}

static int mixer_disp_destroy(MI_DISP_Interface_e eInf)
{
    MI_DISP_DEV DispDev;
    MI_DISP_LAYER DispLayer;

    DispDev = DispLayer = 1;
    if(eInf == E_MI_DISP_INTF_HDMI)
        DispDev = DispLayer = 0;

    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_SUCCESS);

    return 0;
}


int mixer_init_disp(void)
{
    ExecFunc(mixer_disp_create(E_MI_DISP_INTF_VGA),0);
    ExecFunc(mixer_disp_create(E_MI_DISP_INTF_HDMI),0);
    return 0;
}

int mixer_deinit_disp(void)
{
    ExecFunc(mixer_disp_destroy(E_MI_DISP_INTF_VGA),0);
    ExecFunc(mixer_disp_destroy(E_MI_DISP_INTF_HDMI),0);
    return 0;
}

int mixer_chn_disp_config_port(MI_DISP_LAYER DispLayer,MI_DISP_INPUTPORT u32InputPort , MI_U16 u16Width , MI_U16 u16Height)
{
    MI_VIF_DEV u32VifDev = u32InputPort/MI_VIF_MAX_WAY_NUM_PER_DEV;
    MI_DISP_InputPortAttr_t stInputPortAttr;

    DBG_INFO("DispLayer:%d InputPort:%d Width:%d Height:%d\n",DispLayer,u32InputPort,u16Width,u16Height);
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width  = u16Width;
    stInputPortAttr.stDispWin.u16Height = u16Height;
    stInputPortAttr.stDispWin.u16X =  1920/2*(u32VifDev%2) + (u32InputPort%4)%2*u16Width;
    stInputPortAttr.stDispWin.u16Y =  1080/2*(u32VifDev/2) + (u32InputPort%4)/2*u16Height;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32InputPort), MI_SUCCESS);
    return 0;
}
