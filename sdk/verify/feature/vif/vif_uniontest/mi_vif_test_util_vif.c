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

MI_VIF_DevAttr_t DEV_ATTR_BT656D1_1MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_1MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656D1_4MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_4MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656FHD_1MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_1MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

static MI_BOOL vif_channel_port_stop[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
static pthread_t vif_channel_port_tid[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];

int create_vif_dev(MI_VIF_DEV u32VifDev , MI_U32 u32Mode)
{
    MI_S32 s32Ret;
    MI_VIF_DevAttr_t stDevAttr;
    VD_PORT_MUX_MODE_E enVdPortMuxMode = VD_PORT_MODE_UNKNOW;
    VD_PORT_EDGE_E enVdportEdge = VD_PORT_EDGE_UP;
    VD_PORT_CLK_E enVdportClk = VD_PORT_CKL_AUTO;

    DBG_ENTER();

    if(u32Mode == SAMPLE_VI_MODE_1_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_1MUX, sizeof(MI_VIF_DevAttr_t));
        //set_vif_dev_mask(u32VifDev,&stDevAttr);
        enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
        enVdportEdge = VD_PORT_EDGE_UP;
        enVdportClk = VD_PORT_CKL_27M;
    }
    else if(u32Mode == SAMPLE_VI_MODE_4_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_4MUX, sizeof(MI_VIF_DevAttr_t));
        enVdPortMuxMode = VD_PORT_MUX_MODE_4MUX;
        enVdportEdge = VD_PORT_EDGE_UP;
        enVdportClk = VD_PORT_CKL_27M;
    }
    else if(u32Mode == SAMPLE_VI_MODE_1_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
        enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
        enVdportEdge = VD_PORT_EDGE_UP;
        enVdportClk = VD_PORT_CKL_148_5M;
    }
    else
    {
        return -1;
    }

#ifdef TEST_VIF_ENABLE_LIBADDA
    ExecFunc(vif_dh9931_Init(u32VifDev, enVdPortMuxMode, enVdportEdge, enVdportClk), 0);
#else
    enVdPortMuxMode = enVdPortMuxMode;
    enVdportEdge =  enVdportEdge;
    enVdportClk = enVdportClk;
    DBG_INFO("use system cmd for dh9931\n");
    {
        char system_cmd[50] = {0};
        //sprintf(system_cmd, "AHDInit -b 1 -c %d -d %d -e 0 -k %d &", channel, u32VifDev, pclk);
        system(system_cmd);
        system("sleep 10");
        system("busybox pgrep AHDInit | busybox xargs kill -9");
    }
#endif

    s32Ret = MI_VIF_SetDevAttr(u32VifDev, &stDevAttr);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Set dev attributes failed with error code %#x!\n", s32Ret);
        return -1;
    }

    s32Ret = MI_VIF_EnableDev(u32VifDev);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Enable dev failed with error code %#x!\n", s32Ret);
        return -1;
    }

    DBG_ENTER();
    return 0;
}

int destroy_vif_dev(MI_VIF_DEV u32VifDev)
{
    MI_S32 s32Ret;
    s32Ret = MI_VIF_DisableDev(u32VifDev);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Disable dev failed with error code %#x!\n", s32Ret);
        return -1;
    }

    return 0;
}

int create_vif_channel(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort, MI_U32 u32Width, MI_U32 u32Height, MI_SYS_FrameScanMode_e eScanMode, MI_SYS_PixelFormat_e ePixFormat, MI_VIF_FrameRate_e eFrameRate)
{
    MI_S32 s32Ret;
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    MI_SYS_ChnPort_t stChnPort;

    stChnPortAttr.stCapRect.u16X = 0;
    stChnPortAttr.stCapRect.u16Y = 0;
    stChnPortAttr.stCapRect.u16Width = u32Width;
    stChnPortAttr.stCapRect.u16Height = u32Height;
    stChnPortAttr.stDestSize.u16Width = u32Width;
    stChnPortAttr.stDestSize.u16Height = u32Height;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    stChnPortAttr.eScanMode = eScanMode;
    stChnPortAttr.ePixFormat = ePixFormat;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = eFrameRate;

    s32Ret = MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Set chn attributes failed with error code %#x!\n", s32Ret);
        return -1;
    }

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3);

    //DBG_INFO("\n");
    s32Ret = MI_VIF_EnableChnPort(VifChn, VifPort);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Enable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }

    DBG_ENTER();
    return 0;
}

int destroy_vif_channel(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort)
{
    MI_S32 s32Ret;

    //DBG_ENTER();

    s32Ret = MI_VIF_DisableChnPort(VifChn, VifPort);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Disable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }

    return 0;
}
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

void* vif_channel_func(void* p)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_SYS_ChnPort_t stChnPort;
    FILE_HANDLE hYuvFile = NULL;
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    char aName[128];
    MI_U32 u32Arg = (MI_U32)p;
    MI_BOOL bDump = (u32Arg >> 16) & 0x1;
    MI_VIF_CHN VifChn = (u32Arg >> 8) & 0xFF;
    MI_VIF_PORT VifPort = u32Arg & 0xFF;
    MI_S32 s32Fd;
    MI_U32 u32Frame = 0;
    DBG_ENTER();
    DBG_DEBUG("VifChn = %d VifPort = %d bDump = %d\n", VifChn, VifPort, bDump);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_VIF_GetChnPortAttr(VifChn, VifPort, &stChnPortAttr);

    memset(aName, 0x0, sizeof(aName));

    if(stChnPortAttr.ePixFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        sprintf(aName, "/mnt/vif_chn_[%d_%d]_[%d_%d]_yuyv.yuv", VifChn, VifPort, ALIGN_UP(stChnPortAttr.stDestSize.u16Width, 32), stChnPortAttr.stDestSize.u16Height);
    }
    else if(stChnPortAttr.ePixFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        sprintf(aName, "/mnt/vif_chn_[%d_%d]_[%d_%d]_yuv420.yuv", VifChn, VifPort, ALIGN_UP(stChnPortAttr.stDestSize.u16Width, 32), stChnPortAttr.stDestSize.u16Height);
    }
    else
    {
        DBG_INFO("err\n");
        //return -1;
    }

    DBG_DEBUG("%s\n", aName);

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

    while(!vif_channel_port_stop[VifChn][VifPort])
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        //DBG_INFO("[%d %d] GetBuf begin\n",VifChn,VifPort);

        int rval = poll(pfd, 1, 200);

        if(rval < 0)
        {
            DBG_ERR("vif chn:%d port%d  poll error!\n", VifChn, VifPort);
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
            DBG_ERR("[%d %d] GetBuf fail\n", VifChn, VifPort);
            continue;
        }

        u32Frame ++;

        if(u32Frame % 100 == 0)
        {
            DBG_INFO("%u %d %d va [%p] [%p]\n", vif_get_time(), VifChn, VifPort, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
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

int start_vif_channel(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, MI_U32 bDump)
{
    pthread_t tid;
    DBG_DEBUG("VifChn = %d VifPort = %d bDump = %d\n", VifChn, VifPort, bDump);
    pthread_create(&tid, NULL, vif_channel_func, (void*)((bDump << 16) | (VifChn << 8) | VifPort));
    vif_channel_port_tid[VifChn][VifPort] = tid;
    return 0;
}

int stop_vif_channel(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    //DBG_ENTER();
    vif_channel_port_stop[VifChn][VifPort] = TRUE;
    return pthread_join(vif_channel_port_tid[VifChn][VifPort], NULL);
}

int stop_vif_1_1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;

    DBG_ENTER();

    ExecFunc(stop_vif_channel(u32VifChn, 0), 0);
    ExecFunc(stop_vif_channel(u32VifChn, 1), 0);
    ExecFunc(destroy_vif_channel(u32VifChn, 0), 0);
    ExecFunc(destroy_vif_channel(u32VifChn, 1), 0);
    ExecFunc(destroy_vif_dev(u32VifDev), 0);

    return 0;
}


int stop_vif_1_4(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = 0;

    DBG_ENTER();

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        ExecFunc(stop_vif_channel(u32VifChn, 0), 0);
        ExecFunc(stop_vif_channel(u32VifChn, 1), 0);
        ExecFunc(destroy_vif_channel(u32VifChn, 0), 0);
        ExecFunc(destroy_vif_channel(u32VifChn, 1), 0);
    }

    ExecFunc(destroy_vif_dev(u32VifDev), 0);
    return 0;
}

int stop_vif_4_1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = 0;

    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
        ExecFunc(stop_vif_channel(u32VifChn, 0), 0);
        ExecFunc(stop_vif_channel(u32VifChn, 1), 0);
        ExecFunc(destroy_vif_channel(u32VifChn, 0), 0);
        ExecFunc(destroy_vif_channel(u32VifChn, 1), 0);
        ExecFunc(destroy_vif_dev(u32VifDev), 0);
    }

    return 0;
}

int stop_vif_4_4(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = 0;
    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
        {
            ExecFunc(stop_vif_channel(u32VifChn, 0), 0);
            ExecFunc(stop_vif_channel(u32VifChn, 1), 0);
            ExecFunc(destroy_vif_channel(u32VifChn, 0), 0);
            ExecFunc(destroy_vif_channel(u32VifChn, 1), 0);
        }

        ExecFunc(destroy_vif_dev(u32VifDev), 0);
    }

    return 0;
}

// 20180110 u32VifDev = 0  fail
// 20180110 u32VifDev = 1  pass
// 20180110 u32VifDev = 2  fail
// 20180110 u32VifDev = 3  fail
int start_vif_1_1D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 1;

    DBG_ENTER();

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
    ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
    ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
    ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0);

    return 0;
}


// 20180110 u32VifDev = 0  pass
// 20180110 u32VifDev = 1  fail
// 20180110 u32VifDev = 2  fail
// 20180110 u32VifDev = 3  fail
int start_vif_1_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 1;

    DBG_ENTER();

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0);

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        if(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode))
            continue;

        DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);

        ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
        ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
        ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
        ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0);
    }

    return 0;
}

int start_vif_4_1D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 1;

    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_D1), 0);

        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;

        if(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode))
            continue;

        DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);

        ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height , eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
        ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
        ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
        ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0)
    }

    return 0;
}

int start_vif_4_4D1(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn = 0;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 0;

    DBG_ENTER();

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_4_D1), 0)

        for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
        {
            if(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode))
                continue;

            DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);
            ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_QUARTER), 0);
            ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
            ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_QUARTER), 0);
            ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0);
        }
    }

    return 0;
}

// 20180110 u32VifDev = 0  pass
// 20180110 u32VifDev = 1  fail
// 20180110 u32VifDev = 2  pass
// 20180110 u32VifDev = 3  pass
int start_vif_1_1FHD(void)
{
    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 1;

    DBG_ENTER();

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0);
    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);
    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);

    if(bDump)
    {
        ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
    }

    ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);

    if(bDump)
    {
        ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0);
    }

    return 0;
}


int start_vif_4_1FHD(void)
{
    MI_VIF_DEV u32VifDev;
    MI_VIF_CHN u32VifChn;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_U32 bDump = 1;

    DBG_ENTER();

    // cat /proc/mi_modules/mi_vif/mi_vif0 | grep u32FrameRate

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0);
    }

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;

        if(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode))
            continue;

        DBG_INFO("vi:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);

        ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height , eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_QUARTER), 0);

        if(bDump)
        {
            ExecFunc(start_vif_channel(u32VifChn, 0 , bDump), 0);
        }

        ExecFunc(create_vif_channel(u32VifChn, 1, u32Width / 2, u32Height  / 2, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_QUARTER), 0);

        if(bDump)
        {
            ExecFunc(start_vif_channel(u32VifChn, 1 , bDump), 0);
        }
    }

    return 0;
}
