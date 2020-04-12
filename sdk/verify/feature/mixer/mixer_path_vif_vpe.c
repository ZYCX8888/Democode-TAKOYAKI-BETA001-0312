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


static MI_BOOL vpe_channel_port_stop[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
static pthread_t vpe_channel_port_tid[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];


void* mixer_chn_vpe_chn_func(void* p)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_SYS_ChnPort_t stChnPort;
    FILE_HANDLE hYuvFile = 0;
    MI_VPE_PortMode_t stPortMode;
    char aName[128];
    MI_U32 u32Arg = (MI_U32)p;
    MI_VPE_CHANNEL VpeChn = (u32Arg >> 8) & 0xFF;
    MI_VPE_PORT VpePort = u32Arg & 0xFF;
    MI_S32 s32Fd;
    MI_U32 u32Frame = 0;

    DBG_INFO("VpeChn = %d VpePort = %d\n", VpeChn, VpePort);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VpeChn;
    stChnPort.u32PortId = VpePort;

    MI_VPE_GetPortMode(VpeChn, VpePort, &stPortMode);

    memset(aName, 0x0, sizeof(aName));

    if(stPortMode.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        sprintf(aName, "/vendor/vpe_chn_[%d_%d]_[%d_%d]_yuyv.yuv", VpeChn, VpePort, stPortMode.u16Width, stPortMode.u16Height);
    }
    else if(stPortMode.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        sprintf(aName, "/vendor/vpe_chn_[%d_%d]_[%d_%d]_yuv420.yuv", VpeChn, VpePort, stPortMode.u16Width, stPortMode.u16Height);
    }
    else
    {
        DBG_INFO("err\n");
        //return -1;
    }

    DBG_INFO("%s\n", aName);

    hYuvFile = open_yuv_file(aName, 1);

    if(MI_SUCCESS != MI_SYS_GetFd(&stChnPort, &s32Fd))
    {
        DBG_ERR("MI_SYS_GetFd fail\n");
    }

    struct pollfd pfd[1] =
    {
        {s32Fd, POLLIN | POLLERR},
    };

    while(!vpe_channel_port_stop[VpeChn][VpePort])
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        //DBG_INFO("[%d %d] GetBuf begin\n",VpeChn,VpePort);

        int rval = poll(pfd, 1, 200);

        if(rval < 0)
        {
            DBG_ERR("vpe chn:%d port%d  poll error!\n", VpeChn, VpePort);
            continue;
        }
        if(rval == 0)
        {
            DBG_ERR("vpe chn:%d port%d  get buffer time out!\n", VpeChn, VpePort);
            continue;
        }
        if((pfd[0].revents & POLLIN) == 0)
        {
            int key_value;
            read(s32Fd, &key_value, sizeof(key_value));
            DBG_ERR("Key value is '%d'\n", key_value);
        }
        if(pfd[0].revents & POLLERR)
        {
            DBG_ERR("Device error\n");
        }

        if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            DBG_ERR("[%d %d] GetBuf fail\n", VpeChn, VpePort);
            continue;
        }

        u32Frame ++;

        if(u32Frame % 25 == 0)
        {
            DBG_INFO("[%d %d] %u va [%p]\n", VpeChn, VpePort, mixer_util_get_time(), stBufInfo.stFrameData.pVirAddr[0]);
        }

        //DBG_INFO("[%d %d] GetBuf %#llx\n", VpeChn, VpePort, stBufInfo.u64Pts);
        //DBG_INFO("w:%d h:%d pmt:%d \n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.ePixelFormat);
        //DBG_INFO("va [%p] [%p]\n", stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        //DBG_INFO("[%d %d] App GetBuf pa [%#llx] [%#llx]\n", VpeChn, VpePort, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.phyAddr[1]);
        //DBG_INFO("stride [%u] [%u]\n", stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]);
        mixer_write_yuv_file(hYuvFile, stBufInfo.stFrameData);

        if(MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            //DBG_INFO("[%d %d] PutBuf fail\n",VpeChn,VpePort);
            continue;
        }

        //DBG_INFO("[%d %d] PutBuf end\n",VpeChn,VpePort);
    }

    DBG_ENTER();

    if(MI_SUCCESS != MI_SYS_CloseFd(s32Fd))
    {
        DBG_ERR("MI_SYS_CloseFd fail\n");
    }

    if(hYuvFile)
        close_yuv_file(hYuvFile);

    return NULL;
}


int mixer_chn_start_vif_vpe_thread(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    pthread_t tid;
    pthread_create(&tid, NULL, mixer_chn_vpe_chn_func, (void*)((VpeChn << 8) | VpePort));
    vpe_channel_port_tid[VpeChn][VpePort] = tid;
    return 0;
}

int mixer_chn_stop_vif_vpe_thread(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    vpe_channel_port_stop[VpeChn][VpePort] = TRUE;
    pthread_join(vpe_channel_port_tid[VpeChn][VpePort], NULL);
    return 0;
}

int mixer_path_create_vif_vpe(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MI_VPE_CHANNEL u32VpeChn;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};
    MI_U32 u32FrameRate;
    MixerVideoChnInfo_t stChnInfo;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    DBG_ENTER();

    mixer_chn_get_video_info(u32VifChn,&stChnInfo);
    if(stChnInfo.eformat == 255)
        return -1;

    DBG_ENTER();

    ExecFunc(mixer_chn_vif_create(u32VifChn, u32VifPort, &stChnInfo), 0);
    stVpeInWin.u16Width = ALIGN_UP(stChnInfo.u16Width, 32);
    stVpeInWin.u16Height = stChnInfo.u16Height;
    u32VpeChn = u32VifChn*2 + u32VifPort;
    ExecFunc(mixer_chn_vpe_create(u32VpeChn,  &stVpeInWin), 0);
    stVpeOutWin.u16Width  = 1920 / 4;
    stVpeOutWin.u16Height = 1080 / 4;
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

    if(stChnInfo.bDump[u32VifPort])
    {
        ExecFunc(mixer_chn_start_vif_vpe_thread(u32VpeChn,3), 0);
    }

    return 0;
}

int mixer_path_destroy_vif_vpe(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MI_VPE_CHANNEL u32VpeChn;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    DBG_INFO("%d %d\n", u32VifChn, u32VifChn);

    u32VpeChn = u32VifChn*2 + u32VifPort;
    ExecFunc(mixer_chn_vpe_stop(u32VpeChn), 0);

    stSrcChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VifChn;
    stSrcChnPort.u32PortId = u32VifPort;
    stDstChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32ChnId  = u32VpeChn;
    stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort,&stDstChnPort),0);
    ExecFunc(mixer_chn_vpe_destroy(u32VpeChn,0), 0);
    ExecFunc(mixer_chn_vif_destroy(u32VifChn,0), 0);
    return 0;
}
