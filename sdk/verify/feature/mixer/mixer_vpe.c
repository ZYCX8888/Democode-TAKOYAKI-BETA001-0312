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

int mixer_chn_vpe_start(MI_VPE_CHANNEL VpeChn)
{
    DBG_INFO("vpe ch:%d\n",VpeChn);

    if(MI_VPE_OK != MI_VPE_StartChannel(VpeChn))
    {
        DBG_ERR("MI_VPE_StartChannel fail\n");
        return -1;
    }

    return 0;
}

int mixer_chn_vpe_stop(MI_VPE_CHANNEL VpeChn)
{
    DBG_INFO("vpe ch:%d\n",VpeChn);

    if(MI_VPE_OK != MI_VPE_StopChannel(VpeChn))
    {
        DBG_ERR("MI_VPE_StopChannel fail\n");
        return -1;
    }

    return 0;
}

int mixer_chn_vpe_create(MI_VPE_CHANNEL VpeChannel, MI_SYS_WindowRect_t *pstCropWin)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;

    DBG_INFO("vpe ch:%d\n",VpeChannel);

    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn = FALSE;
    stChannelVpssAttr.bEdgeEn = FALSE;
    stChannelVpssAttr.bEsEn = FALSE;
    stChannelVpssAttr.bContrastEn = FALSE;
    stChannelVpssAttr.bUvInvert = FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = FALSE;
    stChannelVpssAttr.bNrEn = FALSE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

#if 0
    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = pstCropWin->u16X;
    stCropWin.u16Y = pstCropWin->u16Y;
    stCropWin.u16Width = pstCropWin->u16Width;
    stCropWin.u16Height = pstCropWin->u16Height;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
#endif

    return 0;
}

int mixer_chn_vpe_destroy(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    DBG_INFO("vpe ch:%d\n",VpeChannel);

    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);

    return 0;
}

int mixer_chn_vpe_config_port(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort , MI_SYS_WindowRect_t *pstDispWin , MI_SYS_PixelFormat_e  ePixelFormat)
{
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_ChnPort_t stVpeChnOutputPort;

    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = ePixelFormat;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height = pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    //ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    //printf("%s %d %d\n",__FUNCTION__,__LINE__,stVpeMode.ePixelFormat);

    stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort.u32DevId = 0;
    stVpeChnOutputPort.u32ChnId = VpeChannel;
    stVpeChnOutputPort.u32PortId = VpePort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, 1, 3), 0);

    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
    return MI_VPE_OK;
}
