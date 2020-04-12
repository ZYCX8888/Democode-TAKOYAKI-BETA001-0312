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
#include "mi_vif_test_util.h"


MI_S32 create_disp_channel(MI_DISP_CHN DispChn, MI_SYS_PixelFormat_e ePixFormat)
{
    MI_S32 s32Ret = 0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_LAYER DispLayer = 0;
    //MI_SYS_ChnPort_t stDispChnInputPort;
    //MI_U32 u32PortId = 0;

    ExecFunc(MI_DISP_GetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60; // E_MI_DISP_OUTPUT_1080P60 E_MI_DISP_OUTPUT_1080P60
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
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
    stLayerAttr.ePixFormat = ePixFormat;
    ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    DBG_INFO("Get Video Layer Size [%d, %d] !!!\n", stLayerAttr.stVidLayerSize.u16Width, stLayerAttr.stVidLayerSize.u16Height);
    DBG_INFO("Get Video Layer DispWin [%d, %d, %d, %d] !!!\n", stLayerAttr.stVidLayerDispWin.u16X, stLayerAttr.stVidLayerDispWin.u16Y, stLayerAttr.stVidLayerDispWin.u16Width, stLayerAttr.stVidLayerDispWin.u16Height);
    DBG_INFO("Get Video Layer format [%d] !!!\n", stLayerAttr.ePixFormat);
    ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_DISP_SUCCESS);

    return s32Ret;
}

int destroy_disp_channel(MI_DIVP_CHN DivpChn)
{
    ExecFunc(MI_DISP_DisableInputPort(0, 0), MI_SUCCESS);
    ExecFunc(MI_DISP_DisableVideoLayer(0), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(0, 0), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(0), MI_SUCCESS);
    ExecFunc(MI_HDMI_DeInit(), MI_SUCCESS);
    return 0;
}

int config_disp_port(MI_U32 u32InputPort , MI_U16 u16Width , MI_U16 u16Height)
{
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_DISP_LAYER DispLayer = 0;

    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width = u16Width;
    stInputPortAttr.stDispWin.u16Height = u16Height;
    stInputPortAttr.stDispWin.u16X = (u32InputPort % 4) * u16Width;
    stInputPortAttr.stDispWin.u16Y = (u32InputPort / 4) * u16Height;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32InputPort), MI_SUCCESS);
    return 0;
}

int init_vif_vpe_disp(MI_VIF_CHN u32VifChn, MI_VPE_CHANNEL u32VpeChn, MI_U32 u32InputPort, MI_VIF_FrameRate_e eFrameRate)
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};

    //vif
    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("vi:%d vpe:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32VpeChn, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, eFrameRate), 0);

    // vpe
    stVpeInWin.u16Width = ALIGN_UP(u32Width, 32);
    stVpeInWin.u16Height = u32Height;
    ExecFunc(create_vpe_channel(u32VpeChn,  &stVpeInWin), 0);
    stVpeOutWin.u16Width  = 1920 / 4;
    stVpeOutWin.u16Height = 1080 / 4;
    ExecFunc(config_vpe_outport(u32VpeChn, 0, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    u32FrameRate = vif_cal_fps(u32FrameRate, eFrameRate);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, u32FrameRate,
                         E_MI_MODULE_ID_VPE, 0, u32VpeChn, 0, u32FrameRate), 0);
    ExecFunc(start_vpe_channel(u32VpeChn, 0), 0);

    // disp
    ExecFunc(config_disp_port(u32InputPort, 1920 / 4, 1080 / 4), 0);
    ExecFunc(bind_module(E_MI_MODULE_ID_VPE, 0, u32VpeChn, 0, u32FrameRate, E_MI_MODULE_ID_DISP, 0, 0, u32InputPort, u32FrameRate), 0);

    return 0;
}

int stop_test_1_1(void)
{
    ExecFunc(unbind_module(E_MI_MODULE_ID_VPE, 0, 0, 0,  E_MI_MODULE_ID_DISP, 0, 0, 0), 0);
    ExecFunc(destroy_disp_channel(0), 0);
    ExecFunc(deinit_vif_vpe(0, 0), 0);
    ExecFunc(destroy_vif_dev(0), 0);
    DBG_ENTER();
    return 0;
}

int stop_disp_4_1(void)
{
    DBG_ENTER();
    MI_U32 i = 0;

    for(i = 0 ; i < 4 ; i ++)
    {
        ExecFunc(unbind_module(E_MI_MODULE_ID_VPE, 0, i, 0,
                               E_MI_MODULE_ID_DISP, 0, 0, i), 0);

        ExecFunc(deinit_vif_vpe(i, i), 0);
    }

    ExecFunc(destroy_disp_channel(0), 0);
    ExecFunc(destroy_vif_dev(0), 0);
    DBG_ENTER();
    return 0;
}

int start_disp_1_1D1(void)
{
    DBG_ENTER();
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;

    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32InputPort = u32VpeChn = u32VifChn;
    ExecFunc(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL), 0);

    DBG_ENTER();
    return 0;
}

int start_disp_1_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;

    DBG_ENTER();

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0);
    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        u32InputPort = u32VpeChn = u32VifChn;

        if(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL))
            continue;
    }

    DBG_ENTER();
    return 0;
}

int start_disp_4_1D1(void)
{
    DBG_ENTER();
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn ;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;

    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0,  E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);

        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
        u32InputPort = u32VpeChn = u32VifChn;

        if(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL))
            continue;
    }

    DBG_ENTER();
    return 0;
}

int start_disp_4_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;

    DBG_ENTER();

    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0);

        for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
        {
            u32InputPort = u32VpeChn = u32VifChn;

            if(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL))
                continue;
        }
    }

    DBG_ENTER();
    return 0;
}

int start_disp_1_1FHD(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;

    DBG_ENTER();

    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0,  E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0);
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;

    u32InputPort = u32VpeChn = u32VifChn;
    ExecFunc(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL), 0);

    DBG_ENTER();
    return 0;
}

int start_disp_4_1FHD(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn ;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 u32InputPort;
    DBG_ENTER();

    ExecFunc(vif_init_hdmi(), 0);
    ExecFunc(create_disp_channel(0,  E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        if(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0)
            continue;

        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
        u32InputPort = u32VpeChn = u32VifChn;

        if(init_vif_vpe_disp(u32VifChn, u32VpeChn, u32InputPort, E_MI_VIF_FRAMERATE_FULL))
            continue;
    }

    DBG_ENTER();
    return 0;
}
