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

static MI_BOOL gbDivpStop = FALSE;
static pthread_t gtid;

MI_S32 create_divp_channel(MI_DIVP_CHN DivpChn, MI_U16 u16InputWidth, MI_U16 u16InputHeight, MI_U16 u16OutputWidth, MI_U16 u16OutputHeight)
{
    MI_DIVP_ChnAttr_t stAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_SYS_ChnPort_t stDivpOutputPort;

    memset(&stDivpOutputPort, 0, sizeof(stDivpOutputPort));
    memset(&stAttr, 0, sizeof(stAttr));
    stDivpOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpOutputPort.u32DevId = 0;
    stDivpOutputPort.u32ChnId = 0;
    stDivpOutputPort.u32PortId = 0;

    stAttr.bHorMirror = false;
    stAttr.bVerMirror = false;
    if((260 <= u16InputHeight) && (300 >= u16InputHeight))//D1: 736x288 interlace
    {
        stAttr.eDiType = E_MI_DIVP_DI_TYPE_3D;
    }
    else
    {
        stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    }
    stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_MIDDLE;
    stAttr.stCropRect.u16X = 0;
    stAttr.stCropRect.u16Y = 0;
    stAttr.stCropRect.u16Width = u16InputWidth;
    stAttr.stCropRect.u16Height = u16InputHeight;
    stAttr.u32MaxWidth = u16InputWidth;
    stAttr.u32MaxHeight = u16InputHeight * 2;
    ExecFunc(MI_DIVP_CreateChn(DivpChn, &stAttr), MI_DISP_SUCCESS);

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
    stOutputPortAttr.u32Width = u16OutputWidth;
    stOutputPortAttr.u32Height = u16OutputHeight;
    ExecFunc(MI_DIVP_SetOutputPortAttr(DivpChn, &stOutputPortAttr), MI_DISP_SUCCESS);
    MI_SYS_SetChnOutputPortDepth(&stDivpOutputPort, 2, 5);
    ExecFunc(MI_DIVP_StartChn(DivpChn), MI_DISP_SUCCESS);
    return 0;
}

int destroy_divp_channel(MI_DIVP_CHN DivpChn)
{
    ExecFunc(MI_DIVP_StopChn(DivpChn), MI_DISP_SUCCESS);
    ExecFunc(MI_DIVP_DestroyChn(DivpChn), MI_DISP_SUCCESS);
    return 0;
}

void* divp_channel_func(void* p)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_SYS_ChnPort_t stChnPort;
    FILE_HANDLE hYuvFile = NULL;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    char aName[128];
    MI_U32 u32Arg = (MI_U32)p;
    MI_BOOL bDump = (u32Arg >> 16) & 0x1;
    MI_DIVP_CHN DivpChn = (u32Arg >> 8) & 0xFF;
    MI_U16 u16divpPort = u32Arg & 0xFF;
    MI_S32 s32Fd;
    MI_U32 u32Frame = 0;
    DBG_INFO("DivpChn = %d u16divpPort = %d bDump = %d\n", DivpChn, u16divpPort, bDump);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stOutputPortAttr, 0x0, sizeof(MI_DIVP_OutputPortAttr_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = DivpChn;
    stChnPort.u32PortId = u16divpPort;
    MI_DIVP_GetOutputPortAttr(0, &stOutputPortAttr);
    //MI_VIF_GetChnPortAttr(VifChn, VifPort, &stChnPortAttr);

    memset(aName, 0x0, sizeof(aName));

    if(stOutputPortAttr.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        sprintf(aName, "/mnt/divp_chn_[%d_%d]_[%d_%d]_yuv422.yuv", DivpChn, u16divpPort,stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
    }
    else if(stOutputPortAttr.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        sprintf(aName, "/mnt/divp_chn_[%d_%d]_[%d_%d]_yuv420.yuv", DivpChn, u16divpPort, stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
    }
    else if(stOutputPortAttr.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_MST_420)
    {
        sprintf(aName, "/mnt/divp_chn_[%d_%d]_[%d_%d]_mst420.yuv", DivpChn, u16divpPort, stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
    }
    else
    {
        DBG_INFO("err\n");
        //return -1;
    }

    DBG_INFO("%s\n", aName);

    if(TRUE == bDump)
    {
        hYuvFile = open_yuv_file(aName, 1);
    }

    if(MI_SUCCESS != MI_SYS_GetFd(&stChnPort, &s32Fd))
    {
        DBG_ERR("MI_SYS_GetFd fail\n");
    }

    struct pollfd pfd[1] =
    {
        {s32Fd, POLLIN | POLLERR},
    };

    while(gbDivpStop)
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        //DBG_INFO("[%d %d] GetBuf begin\n",VifChn,VifPort);

        int rval = poll(pfd, 1, 200);

        if(rval < 0)
        {
            DBG_ERR("DivpChn:%d port%d  poll error!\n", DivpChn, u16divpPort);
            continue;
        }

        if(rval == 0)
        {
            //DBG_ERR("vif chn:%d port%d  get buffer time out!\n", VifChn, VifPort);
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
            DBG_ERR("[%d %d] GetBuf fail\n", DivpChn, u16divpPort);
            continue;
        }

        u32Frame ++;

        if(u32Frame % 100 == 0)
        {
            DBG_INFO("%u %d %d va [%p] [%p]\n", vif_get_time(), DivpChn, u16divpPort, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        }

        //DBG_INFO("[%d %d] GetBuf %#llx\n", VifChn, VifPort, stBufInfo.u64Pts);
        //DBG_INFO("w:%d h:%d pmt:%d \n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.ePixelFormat);
        //DBG_INFO("va [%p] [%p]\n", stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        //DBG_INFO("[%d %d] App GetBuf pa [%#llx] [%#llx]\n", VifChn, VifPort, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.phyAddr[1]);
        //DBG_INFO("stride [%u] [%u]\n", stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]);


        my_write_yuv_file(hYuvFile, stBufInfo.stFrameData);

        if(MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            //DBG_INFO("[%d %d] PutBuf fail\n",VifChn,VifPort);
            continue;
        }

        //DBG_INFO("[%d %d] PutBuf end\n",VifChn,VifPort);
    }

    if(MI_SUCCESS != MI_SYS_CloseFd(s32Fd))
    {
        DBG_ERR("MI_SYS_CloseFd fail\n");
    }

    if(hYuvFile)
        close_yuv_file(hYuvFile);

    DBG_ENTER();
    return NULL;
}

int start_divp_channelthread(MI_DIVP_CHN DivpChn, MI_U16 u16divpPort, MI_U32 bDump)
{
    //DBG_INFO("VifChn = %d VifPort = %d bDump = %d\n",VifChn,VifPort,bDump);
    pthread_create(&gtid, NULL, divp_channel_func, (void*)((bDump << 16) | (DivpChn << 8) | u16divpPort));
    return 0;
}

int stop_divp_channelThread(MI_DIVP_CHN DivpChn, MI_U16 u16divpPort)
{
    //DBG_ENTER();
    gbDivpStop = TRUE;
    return pthread_join(gtid, NULL);
}


int init_vif_divp_disp(MI_VIF_CHN u32VifChn , MI_DIVP_CHN u32DivpChn,MI_U32 u32DispInPort)
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;

    ExecFunc(vif_get_vid_fmt(u32VifChn,&u32Width,&u32Height,&u32FrameRate,&eScanMode),0);
    DBG_INFO("vi:%d divp:%d disp:%u w:%u h:%u fps:%u scan:%u\n",u32VifChn, u32DivpChn, u32DispInPort,u32Width,u32Height,u32FrameRate,eScanMode);

    ExecFunc(create_vif_channel(u32VifChn,0,u32Width,u32Height,eScanMode,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_VIF_FRAMERATE_FULL), 0);

    u32Width = ALIGN_UP(u32Width,32);
    ExecFunc(create_divp_channel(u32DivpChn, u32Width, u32Height, 1920/4,1080/4), MI_SUCCESS);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, u32FrameRate, E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate), MI_SUCCESS);

    ExecFunc(config_disp_port(u32DispInPort, 1920/4, 1080/4), MI_SUCCESS);
    ExecFunc(bind_module(E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate/2, E_MI_MODULE_ID_DISP, 0, 0, u32DispInPort, u32FrameRate/2), MI_SUCCESS);

    return 0;
}

int deinit_vif_divp_disp(MI_VIF_CHN u32VifChn , MI_VPE_CHANNEL u32DivpChn,MI_U32 u32DispInPort)
{
    ExecFunc(unbind_module(E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0,  E_MI_MODULE_ID_DISP, 0, 0, u32DispInPort), MI_SUCCESS);
    ExecFunc(unbind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0,  E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0), MI_SUCCESS);
    ExecFunc(destroy_divp_channel(u32DivpChn), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(u32VifChn, 0), MI_SUCCESS);
    return 0;
}


static MI_S32 _create_disp_channel(MI_DISP_CHN DispChn, MI_SYS_PixelFormat_e ePixFormat, MI_U16 u16Width, MI_U16 u16Height)
{
    MI_S32 s32Ret = 0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_LAYER DispLayer = 0;

    ExecFunc(MI_DISP_GetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60; // E_MI_DISP_OUTPUT_1080P60 E_MI_DISP_OUTPUT_1080P60
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Enable(DispDev), MI_DISP_SUCCESS);

    ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);

    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    stLayerAttr.stVidLayerSize.u16Width = u16Width;
    stLayerAttr.stVidLayerSize.u16Height = u16Height;
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

int start_divp_1_1D1_no_hvsp(void)
{
    MI_VIF_DEV u32VifDev = 1;
    MI_VIF_CHN u32VifChn = 0;
    MI_DIVP_CHN u32DivpChn = 0;
    MI_U32 u32DispInPort = 0;
    MI_U32 u32Width = 0;
    MI_U32 u32Height = 0;
    MI_U32 u32FrameRate = 0;
    MI_SYS_FrameScanMode_e eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32DispInPort = u32DivpChn = u32VifChn;
    MI_U32 u32DivpOutputWidth = 0;
    MI_U32 u32DivpOutputHeight = 0;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_DISP_LAYER DispLayer = 0;

    DBG_ENTER();
    DBG_INFO("\n @@@@@@@@@@@@ start_divp_1_1D1_no_hvsp start @@@@@@@@@@@@ \n\n");

    //vif
    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(vif_get_vid_fmt(u32VifChn,&u32Width,&u32Height,&u32FrameRate,&eScanMode),MI_SUCCESS);
    ExecFunc(create_vif_channel(u32VifChn,0,u32Width,u32Height,eScanMode,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    DBG_INFO("\n\n\n\n====> vi:%d divp:%d disp:%u w:%u h:%u fps:%u scan:%u <====\n\n\n\n",u32VifChn, u32DivpChn, u32DispInPort,u32Width,u32Height,u32FrameRate,eScanMode);
    u32Width = ALIGN_UP(u32Width,32);
    u32DivpOutputWidth = u32Width;
    u32DivpOutputHeight = u32Height;
    if(E_MI_SYS_FRAME_SCAN_MODE_INTERLACE == eScanMode)
    {
        u32DivpOutputHeight *= 2;
    }

    // disp
    ExecFunc(_create_disp_channel(0, E_MI_SYS_PIXEL_FRAME_YUV_MST_420, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    ExecFunc(vif_init_hdmi(), MI_SUCCESS);
    //divp disp
    //ExecFunc(init_vif_divp_disp(u32VifChn,u32DivpChn,u32DispInPort), 0);
    ExecFunc(create_divp_channel(u32DivpChn, u32Width, u32Height, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    //ExecFunc(config_disp_port(u32DispInPort, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32DispInPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width = u32DivpOutputWidth;
    stInputPortAttr.stDispWin.u16Height = u32DivpOutputHeight;
    stInputPortAttr.stDispWin.u16X = 0;
    stInputPortAttr.stDispWin.u16Y = 0;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32DispInPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32DispInPort), MI_SUCCESS);

    ExecFunc(bind_module(E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate, E_MI_MODULE_ID_DISP, 0, 0, u32DispInPort, u32FrameRate), MI_SUCCESS);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, u32FrameRate, E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate), MI_SUCCESS);

    DBG_INFO("\n @@@@@@@@@@@@ start_divp_1_1D1_no_hvsp end @@@@@@@@@@@@ \n\n");
    DBG_ENTER();
    return 0;
}

int start_divp_1_1D1_hvsp(void)
{
    MI_VIF_DEV u32VifDev = 1;
    MI_VIF_CHN u32VifChn = 0;
    MI_DIVP_CHN u32DivpChn = 0;
    MI_U32 u32DispInPort = 0;
    MI_U32 u32Width = 0;
    MI_U32 u32Height = 0;
    MI_U32 u32FrameRate = 0;
    MI_SYS_FrameScanMode_e eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32DispInPort = u32DivpChn = u32VifChn;
    MI_U32 u32DivpOutputWidth = 0;
    MI_U32 u32DivpOutputHeight = 0;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_DISP_LAYER DispLayer = 0;

    DBG_ENTER();
    DBG_INFO("\n @@@@@@@@@@@@ start_divp_1_1D1_hvsp start @@@@@@@@@@@@ \n\n");

    //vif
    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(vif_get_vid_fmt(u32VifChn,&u32Width,&u32Height,&u32FrameRate,&eScanMode),MI_SUCCESS);
    ExecFunc(create_vif_channel(u32VifChn,0,u32Width,u32Height,eScanMode,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    DBG_INFO("\n\n\n\n ====> vi:%d divp:%d disp:%u w:%u h:%u fps:%u scan:%u <====\n\n\n\n",u32VifChn, u32DivpChn, u32DispInPort,u32Width,u32Height,u32FrameRate,eScanMode);
    u32Width = ALIGN_UP(u32Width,32);

    u32DivpOutputWidth = 640;
    u32DivpOutputHeight = 480;
    // disp
    ExecFunc(_create_disp_channel(0, E_MI_SYS_PIXEL_FRAME_YUV_MST_420, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    ExecFunc(vif_init_hdmi(), MI_SUCCESS);
    //divp disp
    //ExecFunc(init_vif_divp_disp(u32VifChn,u32DivpChn,u32DispInPort), 0);
    ExecFunc(create_divp_channel(u32DivpChn, u32Width, u32Height, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    //ExecFunc(config_disp_port(u32DispInPort, u32DivpOutputWidth, u32DivpOutputHeight), MI_SUCCESS);
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32DispInPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width = u32DivpOutputWidth;
    stInputPortAttr.stDispWin.u16Height = u32DivpOutputHeight;
    stInputPortAttr.stDispWin.u16X = 0;
    stInputPortAttr.stDispWin.u16Y = 0;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32DispInPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32DispInPort), MI_SUCCESS);

    ExecFunc(bind_module(E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate, E_MI_MODULE_ID_DISP, 0, 0, u32DispInPort, u32FrameRate), MI_SUCCESS);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, u32FrameRate, E_MI_MODULE_ID_DIVP, 0, u32DivpChn, 0, u32FrameRate), MI_SUCCESS);

    DBG_INFO("\n @@@@@@@@@@@@ start_divp_1_1D1_hvsp end @@@@@@@@@@@@ \n\n");
    DBG_ENTER();
    return 0;
}

int stop_divp_1_1D1(void)
{
    DBG_INFO("\n @@@@@@@@@@@@ stop_test_divp_1 start @@@@@@@@@@@@ \n\n");
    MI_VIF_DEV u32VifDev = 1;
    MI_VIF_CHN u32VifChn ;
    MI_DIVP_CHN u32DivpChn;
    MI_U32 u32DispInPort;

    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32DispInPort = u32DivpChn = u32VifChn;

    ExecFunc(deinit_vif_divp_disp(u32VifChn,u32DivpChn,u32DispInPort), MI_SUCCESS);
    ExecFunc(destroy_disp_channel(0), MI_SUCCESS);
    ExecFunc(destroy_vif_dev(u32VifDev), MI_SUCCESS);
    ExecFunc(vif_deInit_hdmi(), MI_SUCCESS);

    DBG_INFO("\n @@@@@@@@@@@@ stop_test_divp_1 end @@@@@@@@@@@@ \n\n");
    DBG_ENTER();
    return 0;
}


