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
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }


MI_U8 u8RetutnCnt = 0;
void *ChnOutputPortGetBuf(void *arg)
{
    int fd = -1;

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

    fd =open("/mnt/test/divp_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn0OutputPort0,&stBufInfo,&hHandle))
            {
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(u8RetutnCnt >= 4)
                    break;
            }
        }
    }
    MI_SYS_SetChnOutputPortDepth(&stDivpChn0OutputPort0,0,20);
    if(fd > 0) close(fd);
    u8RetutnCnt ++;
    return NULL;
}


void *Chn0InputPortGetBuf(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U8 au8Header[16] = {0};
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_ChnPort_t stVdecChn0InputPort0;
    stVdecChn0InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn0InputPort0.u32DevId = 0;
    stVdecChn0InputPort0.u32ChnId = 0;
    stVdecChn0InputPort0.u32PortId = 0;

    int fd = open("/mnt/test/h264.es", O_RDWR, 0666);
    if(fd > 0)
    {
         while(1)
         {
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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVdecChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                u32Len = read(fd, stBufInfo.stRawData.pVirAddr, u32FrameLen);
                stBufInfo.stRawData.u32ContentSize = u32Len;
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
            else
                printf("get buf fail\n");
            usleep(100 * 1000);
         }
    }
    if(fd > 0)
     close(fd);

    u8RetutnCnt ++;
    return NULL;
}


void *Chn1InputPortGetBuf(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U8 au8Header[16] = {0};
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_ChnPort_t stVdecChn1InputPort0;
    stVdecChn1InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn1InputPort0.u32DevId = 0;
    stVdecChn1InputPort0.u32ChnId = 1;
    stVdecChn1InputPort0.u32PortId = 0;

  int fd = open("/mnt/test/h264.es", O_RDWR, 0666);
    if(fd > 0)
    {
         while(1)
         {
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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVdecChn1InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                u32Len = read(fd, stBufInfo.stRawData.pVirAddr, u32FrameLen);
                stBufInfo.stRawData.u32ContentSize = u32Len;
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
             else
                 printf("get buf fail\n");
             usleep(100 * 1000);
         }
    }
    if(fd > 0)
     close(fd);
    u8RetutnCnt ++;
    return NULL;

}


void *Chn2InputPortGetBuf(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U8 au8Header[16] = {0};
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_ChnPort_t stVdecChn2InputPort0;
    stVdecChn2InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn2InputPort0.u32DevId = 0;
    stVdecChn2InputPort0.u32ChnId = 2;
    stVdecChn2InputPort0.u32PortId = 0;

  int fd = open("/mnt/test/h264.es", O_RDWR, 0666);
    if(fd > 0)
    {
         while(1)
         {
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
                u32Len = read(fd, stBufInfo.stRawData.pVirAddr, u32FrameLen);
                stBufInfo.stRawData.u32ContentSize = u32Len;
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
             else
                 printf("get buf fail\n");
             usleep(100 * 1000);
         }
    }
    if(fd > 0)
     close(fd);
    u8RetutnCnt ++;
    return NULL;


}

void *Chn3InputPortGetBuf(void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U8 au8Header[16] = {0};
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_ChnPort_t stVdecChn3InputPort0;
    stVdecChn3InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn3InputPort0.u32DevId = 0;
    stVdecChn3InputPort0.u32ChnId = 3;
    stVdecChn3InputPort0.u32PortId = 0;

    int fd = open("/mnt/test/h264.es", O_RDWR, 0666);
    if(fd > 0)
    {
         while(1)
         {
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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVdecChn3InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                u32Len = read(fd, stBufInfo.stRawData.pVirAddr, u32FrameLen);
                stBufInfo.stRawData.u32ContentSize = u32Len;
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
             else
                 printf("get buf fail\n");
             usleep(100 * 1000);
         }
    }
    if(fd > 0)
     close(fd);
    u8RetutnCnt ++;
    return NULL;


}



int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32ChnId = 0 , u32WorkChnNum = 4;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);


    MI_SYS_ChnPort_t stVdecChnOutputPort;
    stVdecChnOutputPort.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChnOutputPort.u32DevId = 0;
    stVdecChnOutputPort.u32ChnId = 0;
    stVdecChnOutputPort.u32PortId = 0;


    MI_SYS_ChnPort_t stDivpChn0InputPort0;
    stDivpChn0InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0InputPort0.u32DevId = 0;
    stDivpChn0InputPort0.u32ChnId = 0;
    stDivpChn0InputPort0.u32PortId = 0;


    pthread_t tid0 , tid1 ,tid2 ,tid3 ,tid4;
    pthread_create(&tid0, NULL, ChnOutputPortGetBuf, NULL);

    pthread_create(&tid1, NULL, Chn0InputPortGetBuf, NULL);
    pthread_create(&tid2, NULL, Chn1InputPortGetBuf, NULL);
    pthread_create(&tid3, NULL, Chn2InputPortGetBuf, NULL);
    pthread_create(&tid4, NULL, Chn3InputPortGetBuf, NULL);

    while(1)
    {
        for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
        {
            stVdecChnOutputPort.u32ChnId = u32ChnId;
            MI_SYS_BindChnPort(&stVdecChnOutputPort,&stDivpChn0InputPort0,30,30);
            sleep(1);
            MI_SYS_UnBindChnPort(&stVdecChnOutputPort,&stDivpChn0InputPort0);
        }
        if(u8RetutnCnt >= 5)
            break;
    }
    printf("test end\n");

    pthread_join(tid0,NULL);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_join(tid3,NULL);
    pthread_join(tid4,NULL);
    return 0;
}
