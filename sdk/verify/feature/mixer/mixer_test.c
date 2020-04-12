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

int mixer_test_0(void)
{
    MI_VIF_CHN u32VifChn = 0 ;
    MI_VIF_PORT u32VifPort = 0;
    MI_VPE_CHANNEL u32VpeChn = 0;
    MI_U32 u32FrameRate;
    MI_S32 s32Ret;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    DBG_ENTER();

    s32Ret = MI_VIF_DisableChnPort(u32VifChn, u32VifPort);
    if(s32Ret != MI_SUCCESS)
    {
        printf("Disable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }

    stSrcChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VifChn;
    stSrcChnPort.u32PortId = u32VifPort;
    stDstChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32ChnId  = u32VpeChn;
    stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort,&stDstChnPort),0);

    sleep(5);

    s32Ret = MI_VIF_EnableChnPort(u32VifChn, u32VifPort);
    if(s32Ret != MI_SUCCESS)
    {
        printf("Enable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }

    u32FrameRate = mixer_chn_get_video_fps(u32VifChn, 0);
    ExecFunc(mixer_bind_module(&stSrcChnPort,&stDstChnPort, u32FrameRate, u32FrameRate), 0);
    return 0;
}
