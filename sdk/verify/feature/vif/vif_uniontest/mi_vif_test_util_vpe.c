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


static MI_BOOL vpe_channel_port_stop[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
static pthread_t vpe_channel_port_tid[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
MI_BOOL bDelayStart = FALSE;

int create_vpe_channel(MI_VPE_CHANNEL VpeChannel, MI_SYS_WindowRect_t *pstCropWin)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;

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


int config_vpe_outport(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort , MI_SYS_WindowRect_t *pstDispWin , MI_SYS_PixelFormat_e  ePixelFormat)
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

int destroy_vpe_channel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VPE_StopChannel(VpeChannel), MI_VPE_OK);

    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);

    return 0;
}

void* vpe_channel_func(void* p)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_SYS_ChnPort_t stChnPort;
    FILE_HANDLE hYuvFile = 0;
    MI_VPE_PortMode_t stPortMode;
    char aName[128];
    MI_U32 u32Arg = (MI_U32)p;
    MI_BOOL bDump = (u32Arg >> 16) & 0x1;
    MI_VPE_CHANNEL VpeChn = (u32Arg >> 8) & 0xFF;
    MI_VPE_PORT VpePort = u32Arg & 0xFF;
    MI_S32 s32Fd;
    MI_U32 u32Frame = 0;

    DBG_DEBUG("VpeChn = %d VpePort = %d bDump = %d\n", VpeChn, VpePort ,bDump);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VpeChn;
    stChnPort.u32PortId = VpePort;

    MI_VPE_GetPortMode(VpeChn, VpePort, &stPortMode);

    memset(aName, 0x0, sizeof(aName));

    if(stPortMode.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        sprintf(aName, "/mnt/vpe_chn_[%d_%d]_[%d_%d]_yuyv.yuv", VpeChn, VpePort, stPortMode.u16Width, stPortMode.u16Height);
    }
    else if(stPortMode.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        sprintf(aName, "/mnt/vpe_chn_[%d_%d]_[%d_%d]_yuv420.yuv", VpeChn, VpePort, stPortMode.u16Width, stPortMode.u16Height);
    }
    else
    {
        DBG_INFO("err\n");
        //return -1;
    }

    DBG_DEBUG("%s\n", aName);

    if(bDump)
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
            //DBG_ERR("vpe chn:%d port%d  get buffer time out!\n", VpeChn, VpePort);
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
            DBG_INFO("[%d %d] %u va [%p] [%p]\n", VpeChn, VpePort, vif_get_time(), stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        }

        //DBG_INFO("[%d %d] GetBuf %#llx\n", VpeChn, VpePort, stBufInfo.u64Pts);
        //DBG_INFO("w:%d h:%d pmt:%d \n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.ePixelFormat);
        //DBG_INFO("va [%p] [%p]\n", stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        //DBG_INFO("[%d %d] App GetBuf pa [%#llx] [%#llx]\n", VpeChn, VpePort, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.phyAddr[1]);
        //DBG_INFO("stride [%u] [%u]\n", stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]);
        my_write_yuv_file(hYuvFile, stBufInfo.stFrameData);

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


int start_vpe_channel(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    DBG_ENTER();

    if(MI_VPE_OK != MI_VPE_StartChannel(VpeChn))
    {
        DBG_ERR("MI_VPE_StartChannel fail\n");
        return -1;
    }

    return 0;
}

int stop_vpe_channel(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    DBG_ENTER();

    if(MI_VPE_OK != MI_VPE_StopChannel(VpeChn))
    {
        DBG_ERR("MI_VPE_StopChannel fail\n");
        return -1;
    }

    return 0;
}

int start_vpe_thread(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort, MI_U32 bDump)
{
    pthread_t tid;
    pthread_create(&tid, NULL, vpe_channel_func, (void*)((bDump << 16) | (VpeChn << 8) | VpePort));
    vpe_channel_port_tid[VpeChn][VpePort] = tid;
    return 0;
}

int stop_vpe_thread(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    vpe_channel_port_stop[VpeChn][VpePort] = TRUE;
    pthread_join(vpe_channel_port_tid[VpeChn][VpePort], NULL);
    return 0;
}

int init_vif_vpe(MI_VIF_CHN u32VifChn , MI_VPE_CHANNEL u32VpeChn , MI_VIF_FrameRate_e eFrameRate)
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};

    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("vi:%d vpe:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32VpeChn, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, eFrameRate), 0);
    stVpeInWin.u16Width = ALIGN_UP(u32Width, 32);
    stVpeInWin.u16Height = u32Height;
    ExecFunc(create_vpe_channel(u32VpeChn,  &stVpeInWin), 0);
    stVpeOutWin.u16Width  = 1920 / 4;
    stVpeOutWin.u16Height = 1080 / 4;
    ExecFunc(config_vpe_outport(u32VpeChn, 0, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);
    u32FrameRate = vif_cal_fps(u32FrameRate, eFrameRate);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, 50,
                         E_MI_MODULE_ID_VPE, 0, u32VpeChn, 0, 50), 0);
    ExecFunc(start_vpe_channel(u32VpeChn, 0), 0);

    return 0;
}

int deinit_vif_vpe(MI_VIF_CHN u32VifChn, MI_VPE_CHANNEL u32VpeChn)
{
    DBG_INFO("%d %d\n", u32VifChn, u32VpeChn);

    ExecFunc(stop_vpe_channel(u32VpeChn, 0), 0);
    ExecFunc(unbind_vif_vpe(u32VifChn, 0, u32VpeChn, 0), 0);
    ExecFunc(destroy_vif_channel(u32VifChn, 0), 0);
    ExecFunc(destroy_vpe_channel(u32VifChn, 0), 0);
    return 0;
}

int stop_vpe_1_1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    MI_VPE_CHANNEL u32VpeChn = u32VifChn;

    ExecFunc(stop_vpe_thread(u32VpeChn, 0), 0);
    ExecFunc(deinit_vif_vpe(u32VifChn, u32VpeChn), 0);
    ExecFunc(destroy_vif_dev(u32VifDev), 0);
    return 0;
}

int stop_vpe_1_4(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        u32VpeChn = u32VifChn;
        ExecFunc(stop_vpe_thread(u32VpeChn, 0), 0);
        ExecFunc(deinit_vif_vpe(u32VifChn, u32VpeChn), 0);
    }

    ExecFunc(destroy_vif_dev(u32VifDev), 0);

    return 0;
}

int stop_vpe_4_1(void)
{
    return 0;
}

int stop_vpe_4_4(void)
{
    return 0;
}

int start_vpe_1_1D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    MI_VPE_CHANNEL u32VpeChn = u32VifChn;
    MI_U32 bDump = 1;

    DBG_ENTER();
    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL), 0);
    ExecFunc(start_vpe_thread(u32VpeChn, 0 , bDump), 0);
    return 0;
}

int start_vpe_1_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 bDump = 0;

    DBG_ENTER();

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0);

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        ///if(u32VifChn == 1)
        //continue;

        //if(u32VifChn == 2)
        //continue;

        //if(u32VifChn == 3)
        //continue;

        u32VpeChn = u32VifChn;

        if(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL), 0)
            continue;

        ExecFunc(start_vpe_thread(u32VpeChn, 0, bDump), 0);
    }

    return 0;
}


int start_vpe_4_1D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn ;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 bDump = 0;
    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
        u32VpeChn = u32VifChn;

        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);

        if(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL))
            continue;

        if(bDump)
        {
            ExecFunc(start_vpe_thread(u32VpeChn, 0, bDump), 0);
        }
    }

    return 0;
}

int start_vpe_4_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn ;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 bDump = 0;
    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0);

        for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
        {
            u32VpeChn = u32VifChn;

            if(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL))
                continue;

            if(bDump)
            {
                ExecFunc(start_vpe_thread(u32VpeChn, 0, bDump), 0);
            }
        }
    }

    return 0;
}


// 20180111 掉帧  TimeMax: 659360 太高
// cat /proc/mi_modules/mi_vif/mi_vif0 | grep u32FrameRate

int start_vpe_1_1FHD(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 bDump = 1;

    DBG_ENTER();

    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32VpeChn = u32VifChn;
    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0);
    ExecFunc(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL), 0);

    if(bDump)
    {
        ExecFunc(start_vpe_thread(u32VpeChn, 0, bDump), 0);
    }

    return 0;
}

// 20180111 掉帧  4路的时�?通道 0 4 无数�?//                2路的时�?通道 4 无数�?  ,通道0 fps 不稳

// cat /proc/mi_modules/mi_vif/mi_vif0 | grep u32FrameRate

int start_vpe_4_1FHD(void)
{
    MI_VIF_DEV u32VifDev;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_U32 bDump = 0;

    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < 2 ; u32VifDev++)
    {
        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
        u32VpeChn = u32VifChn;

        if(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0)
            continue;

        if(init_vif_vpe(u32VifChn, u32VpeChn, E_MI_VIF_FRAMERATE_FULL), 0)
            continue;

        if(bDump)
        {
            ExecFunc(start_vpe_thread(u32VpeChn, 0, bDump), 0);
        }
    }

    return 0;
}
