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

int mixer_path_create_vif_vpe_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MI_U32 u32FrameRate;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};
    MI_VPE_CHANNEL u32VpeChn;
    MI_DISP_INPUTPORT LayerInputPort;
    MixerVideoChnInfo_t stChnInfo;
    MI_DISP_LAYER s32DispLayer;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_DISP_DEV DispDev;
    MI_VIF_DEV u32VifDev = u32VifChn/MI_VIF_MAX_WAY_NUM_PER_DEV;
    MI_VIF_WorkMode_e eWorkMode = mixer_device_get_workmode(u32VifDev);

    u32VpeChn = u32VifChn*2 + u32VifPort;
	DispDev = s32DispLayer = u32VifPort;

    LayerInputPort = u32VifChn;

    mixer_chn_get_video_info(u32VifChn,&stChnInfo);
    if(stChnInfo.eformat == 255)
        return -1;

    DBG_INFO("%d %d\n", u32VifChn, u32VifChn);

    ExecFunc(mixer_chn_vif_create(u32VifChn, u32VifPort, &stChnInfo), 0);
    stVpeInWin.u16Width = ALIGN_UP(stChnInfo.u16Width, 32);

    stVpeInWin.u16Height = stChnInfo.u16Height;
    ExecFunc(mixer_chn_vpe_create(u32VpeChn,  &stVpeInWin), 0);

    if(eWorkMode == E_MI_VIF_WORK_MODE_1MULTIPLEX)
    {
        stVpeOutWin.u16Width  = 1920/2;
        stVpeOutWin.u16Height = 1080/2;
    }
    else if(eWorkMode == E_MI_VIF_WORK_MODE_2MULTIPLEX)
    {
        stVpeOutWin.u16Width  = 1920/2;
        stVpeOutWin.u16Height = 1080/4;
    }
    else
    {
        stVpeOutWin.u16Width  = 1920/4;
        stVpeOutWin.u16Height = 1080/4;
    }
    ExecFunc(mixer_chn_vpe_config_port(u32VpeChn, 3, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);
    u32FrameRate = mixer_chn_get_video_fps(u32VifChn, 0);

    stSrcChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VifChn;
    stSrcChnPort.u32PortId = u32VifPort;
    stDstChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32ChnId  = u32VpeChn;
    stDstChnPort.u32PortId = 0;

    ExecFunc(mixer_bind_module(&stSrcChnPort,&stDstChnPort, u32FrameRate, u32FrameRate), 0);
    ExecFunc(mixer_chn_vpe_start(u32VpeChn), 0);

    // disp
    ExecFunc(mixer_chn_disp_config_port(s32DispLayer,LayerInputPort,stVpeOutWin.u16Width,stVpeOutWin.u16Height), 0);

    stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VpeChn;
    stSrcChnPort.u32PortId = 3;
    stDstChnPort.eModId    = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32DevId  = DispDev;
    stDstChnPort.u32ChnId  = 0;
    stDstChnPort.u32PortId = LayerInputPort;

    ExecFunc(mixer_bind_module(&stSrcChnPort, &stDstChnPort, u32FrameRate, u32FrameRate), 0);

    return 0;
}

int mixer_path_destroy_vif_vpe_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MI_VPE_CHANNEL u32VpeChn;
    MI_DISP_INPUTPORT LayerInputPort;
    MI_DISP_LAYER s32DispLayer;
    MI_DISP_DEV DispDev;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;


    u32VpeChn = u32VifChn*2 + u32VifPort;
    DispDev = s32DispLayer = u32VifPort;
    LayerInputPort = u32VifChn;

    DBG_INFO("%d %d\n", u32VifChn, u32VifChn);

    MI_DISP_DisableInputPort(s32DispLayer, LayerInputPort);
    mixer_chn_vpe_stop(u32VpeChn);

    // unbind vpe disp
    stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VpeChn;
    stSrcChnPort.u32PortId = 0;
    stDstChnPort.eModId    = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32DevId  = DispDev;
    stDstChnPort.u32ChnId  = 0;
    stDstChnPort.u32PortId = LayerInputPort;
    MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort);

    // unbind vif vpe
    stSrcChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VifChn;
    stSrcChnPort.u32PortId = u32VifPort;
    stDstChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32ChnId  = u32VpeChn;
    stDstChnPort.u32PortId = 0;
    MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort);

    mixer_chn_vpe_destroy(u32VpeChn,0);
    mixer_chn_vif_destroy(u32VifChn,u32VifPort);
    return 0;
}
