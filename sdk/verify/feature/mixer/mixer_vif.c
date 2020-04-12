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

static MI_BOOL vif_channel_port_stop[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
static pthread_t vif_channel_port_tid[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];

void* mix_chn_vif_func(void* p)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_SYS_ChnPort_t stChnPort;
    FILE_HANDLE hYuvFile = NULL;
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    char aName[128];
    MI_U32 u32Arg = (MI_U32)p;
    MI_VIF_CHN VifChn = (u32Arg >> 8) & 0xFF;
    MI_VIF_PORT VifPort = u32Arg & 0xFF;
    MI_S32 s32Fd;
    MI_U32 u32Frame = 0;
    DBG_ENTER();
    DBG_DEBUG("VifChn = %d VifPort = %d \n", VifChn, VifPort);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_VIF_GetChnPortAttr(VifChn, VifPort, &stChnPortAttr);

    memset(aName, 0x0, sizeof(aName));

    if(stChnPortAttr.ePixFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        sprintf(aName, "/vendor/vif_chn_[%d_%d]_[%d_%d]_yuyv.yuv", VifChn, VifPort, ALIGN_UP(stChnPortAttr.stDestSize.u16Width, 32), stChnPortAttr.stDestSize.u16Height);
    }
    else if(stChnPortAttr.ePixFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        sprintf(aName, "/vendor/vif_chn_[%d_%d]_[%d_%d]_yuv420.yuv", VifChn, VifPort, ALIGN_UP(stChnPortAttr.stDestSize.u16Width, 32), stChnPortAttr.stDestSize.u16Height);
    }
    else
    {
        DBG_INFO("err\n");
        //return -1;
    }

    DBG_DEBUG("%s\n", aName);

    hYuvFile = open_yuv_file(aName, 1);

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
            DBG_INFO("[%u %d] %d va [%p] [%p]\n", mixer_util_get_time(), VifChn, VifPort, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        }

        //DBG_INFO("[%d %d] GetBuf %#llx\n", VifChn, VifPort, stBufInfo.u64Pts);
        //DBG_INFO("w:%d h:%d pmt:%d \n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.ePixelFormat);
        //DBG_INFO("va [%p] [%p]\n", stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1]);
        //DBG_INFO("[%d %d] App GetBuf pa [%#llx] [%#llx]\n", VifChn, VifPort, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.phyAddr[1]);
        //DBG_INFO("stride [%u] [%u]\n", stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]);


        mixer_write_yuv_file(hYuvFile, stBufInfo.stFrameData);

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

int mixer_chn_vif_start(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    pthread_t tid;
    DBG_DEBUG("VifChn = %d VifPort = %d\n", VifChn, VifPort);
    pthread_create(&tid, NULL, mix_chn_vif_func, (void*)((VifChn << 8) | VifPort));
    vif_channel_port_tid[VifChn][VifPort] = tid;
    return 0;
}

int mixer_chn_vif_stop(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    //DBG_ENTER();
    vif_channel_port_stop[VifChn][VifPort] = TRUE;
    return pthread_join(vif_channel_port_tid[VifChn][VifPort], NULL);
}


int mixer_chn_vif_create(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort, MixerVideoChnInfo_t* pstChnInfo)
{
    MI_S32 s32Ret;
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    MI_SYS_ChnPort_t stChnPort;

    stChnPortAttr.stCapRect.u16X = 0;
    stChnPortAttr.stCapRect.u16Y = 0;

    if(VifPort == 0)
    {
        stChnPortAttr.stCapRect.u16Width   = pstChnInfo->u16Width;
        stChnPortAttr.stCapRect.u16Height  = pstChnInfo->u16Height;
        stChnPortAttr.stDestSize.u16Width  = pstChnInfo->u16Width;
        stChnPortAttr.stDestSize.u16Height = pstChnInfo->u16Height;
    }
    else
    {
        stChnPortAttr.stCapRect.u16Width   = pstChnInfo->u16Width / 2;
        stChnPortAttr.stCapRect.u16Height  = pstChnInfo->u16Height / 2;
        stChnPortAttr.stDestSize.u16Width  = pstChnInfo->u16Width / 2;
        stChnPortAttr.stDestSize.u16Height = pstChnInfo->u16Height / 2;
    }

    stChnPortAttr.eCapSel = pstChnInfo->eCapSel;
    stChnPortAttr.eScanMode = pstChnInfo->eScanMode;
    stChnPortAttr.ePixFormat = pstChnInfo->ePixFormat;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = pstChnInfo->eFrameRate[VifPort];

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

    DBG_EXIT_OK();
    return 0;
}

int mixer_chn_vif_destroy(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort)
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
