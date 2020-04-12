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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }


void *ChnOutputPortGetBuf(void *arg)
{
    int fd = -1;

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVencChn0InputPort0;
    stVencChn0InputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0InputPort0.u32DevId = 0;
    stVencChn0InputPort0.u32ChnId = 0;
    stVencChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVencChn0OutputPort0;
    stVencChn0OutputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0OutputPort0.u32DevId = 0;
    stVencChn0OutputPort0.u32ChnId = 0;
    stVencChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stVencChn0OutputPort0,5,10);

    fd =open("/mnt/test/rawdata.es",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVencChn0OutputPort0,&stBufInfo,&hHandle))
            {
                write(fd,stBufInfo.stRawData.pVirAddr,stBufInfo.stRawData.u32ContentSize);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(stBufInfo.bEndOfStream)
                    break;
            }
        }
    }
    if(fd > 0) close(fd);

    MI_SYS_SetChnOutputPortDepth(&stVencChn0OutputPort0,0,10);

    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    MI_SYS_ChnPort_t stVpeChn0InputPort0;
    stVpeChn0InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0InputPort0.u32DevId = 0;
    stVpeChn0InputPort0.u32ChnId = 0;
    stVpeChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVencChn0InputPort0;
    stVencChn0InputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0InputPort0.u32DevId = 0;
    stVencChn0InputPort0.u32ChnId = 0;
    stVencChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVencChn0OutputPort0;
    stVencChn0OutputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0OutputPort0.u32DevId = 0;
    stVencChn0OutputPort0.u32ChnId = 0;
    stVencChn0OutputPort0.u32PortId = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

    MI_SYS_BindChnPort(&stVpeChn0OutputPort0,&stVencChn0InputPort0,30,30);

    int fd = open("/mnt/test/1920_1080_yuv422.yuv", O_RDWR, 0666);
    if(fd > 0)
    {
        while(1)
        {
            int n = 0;

            memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
            MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
            stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
            stBufConf.stFrameCfg.u16Width = 1920;
            stBufConf.stFrameCfg.u16Height = 1080;

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVpeChn0InputPort0,&stBufConf,&stBufInfo,&hHandle ,-1))
            {
                stBufInfo.bEndOfStream = FALSE;
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                n = read(fd , stBufInfo.stFrameData.pVirAddr[0], u32Size);
                if(n < u32Size)
                {
                    stBufInfo.bEndOfStream = TRUE;
                    MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                    break;
                }
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
            else
                printf("get buf fail\n");
        }
    }
    if(fd > 0)
        close(fd);
    printf("test end\n");

    pthread_join(tid,NULL);
    return 0;
}
