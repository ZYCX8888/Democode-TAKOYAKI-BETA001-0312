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

#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (1024*128)

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

int FrameMaxCount = 5;
void *ChnOutputPortGetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stDivpChn0InputPort0;
    stDivpChn0InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0InputPort0.u32DevId = 0;
    stDivpChn0InputPort0.u32ChnId = 0;
    stDivpChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stDivpChn0OutputPort0,2,5);

    fd =open("/mnt/test/divp_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn0OutputPort0,&stBufInfo,&hHandle))
            {
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(10 *1000);
            }
        }
    }
    if(fd > 0) close(fd);
    MI_SYS_SetChnOutputPortDepth(&stDivpChn0OutputPort0,0,5);

    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));
    int count = 0;
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    MI_SYS_ChnPort_t stVdecChn2InputPort0;
    stVdecChn2InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn2InputPort0.u32DevId = 0;
    stVdecChn2InputPort0.u32ChnId = 2;
    stVdecChn2InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVdecChn2OutputPort0;
    stVdecChn2OutputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn2OutputPort0.u32DevId = 0;
    stVdecChn2OutputPort0.u32ChnId = 2;
    stVdecChn2OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn0InputPort0;
    stDivpChn0InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0InputPort0.u32DevId = 0;
    stDivpChn0InputPort0.u32ChnId = 0;
    stDivpChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;
    MI_U8 au8Header[16] = {0};

    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

    MI_SYS_BindChnPort(&stVdecChn2OutputPort0,&stDivpChn0InputPort0,30,30);

    int fd = open("/mnt/test/h264.es", O_RDWR, 0666);
    if(fd > 0)
    {
        while(1)
        {
            int n = 0;
            memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
            stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
            MI_SYS_GetCurPts(&stBufConf.u64TargetPts);

            memset(au8Header, 0, 16);
            MI_U32 u32Pos = lseek(fd, 0L, SEEK_CUR);
            MI_U32 u32Len = read(fd, au8Header, 16);
            if(u32Len <= 0)
            {
                lseek(fd, 0, SEEK_SET);
                break;
            }
            MI_U32 u32FrameLen = MI_U32VALUE(au8Header, 4);
            if(u32FrameLen > VESFILE_READER_BATCH)
            {
                lseek(fd, 0, SEEK_SET);
                break;
            }

            stBufConf.stRawCfg.u32Size = u32FrameLen;

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVdecChn2InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                stBufInfo.bEndOfStream = FALSE;
                n = read(fd , stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize);
                stBufInfo.stRawData.u32ContentSize = n;
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if(count ++ >= FrameMaxCount)
                    break;
            }
            else
                printf("get buf fail\n");
            usleep(100 * 1000);
        }
    }
    if(fd > 0)
        close(fd);
    printf("test end\n");

    pthread_join(tid,NULL);
    return 0;
}
