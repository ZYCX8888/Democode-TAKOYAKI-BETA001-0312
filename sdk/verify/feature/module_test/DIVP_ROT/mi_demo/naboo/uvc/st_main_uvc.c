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
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include "mi_sys.h"
#include "mi_venc.h"
#include "st_uvc.h"
#include "st_common.h"
#include "st_hdmi.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"
#include "st_fb.h"
#include "st_warp.h"
#include "st_ceva.h"
#include "list.h"
#include "mi_rgn.h"
#include "mi_divp.h"
#include "i2c.h"
#include "tem.h"
#define SUPPORT_VIDEO_ENCODE
#define SUPPORT_UVC
#define UVC_SUPPORT_MMAP
//#define UVC_SUPPORT_LL
#define SUPPORT_WARP
#ifdef UVC_SUPPORT_USERPTR
#define UVC_MEMORY_MODE UVC_MEMORY_USERPTR
//yuv420 orz...
#else
#define UVC_MEMORY_MODE UVC_MEMORY_MMAP
#endif
//#define SUPPORT_CDNN
//#define SUPPORT_WRITE_FILE
#define ENABLE_PUTES_TO_UVC 0
#define RD_OR_WR 1
#define ENABLE_DUMPCIF_PORT1 0
typedef enum
{
    E_UVC_TIMMING_4K2K_JPG,
    E_UVC_TIMMING_2560X1440P_JPG,
    E_UVC_TIMMING_1920X1080P_JPG,
    E_UVC_TIMMING_1280X720P_JPG,
    E_UVC_TIMMING_640X480P_JPG,
    E_UVC_TIMMING_320X240P_JPG,
    E_UVC_TIMMING_4K2K_H264,
    E_UVC_TIMMING_2560X1440P_H264,
    E_UVC_TIMMING_1920X1080P_H264,
    E_UVC_TIMMING_1280X720P_H264,
    E_UVC_TIMMING_640X480P_H264,
    E_UVC_TIMMING_320X240P_H264,
    E_UVC_TIMMING_4K2K_H265,
    E_UVC_TIMMING_2560X1440P_H265,
    E_UVC_TIMMING_1920X1080P_H265,
    E_UVC_TIMMING_1280X720P_H265,
    E_UVC_TIMMING_640X480P_H265,
    E_UVC_TIMMING_320X240P_H265,
    E_UVC_TIMMING_1920X1080_NV12,
    E_UVC_TIMMING_1280X720_NV12,
    E_UVC_TIMMING_640X480_NV12,
    E_UVC_TIMMING_320X240_NV12
}UVC_FormatTimming_e;
static MI_SYS_ChnPort_t gstChnPort;
static struct pollfd gpfd[1] =
{
    {0, POLLIN | POLLERR},
};
extern MI_U32 Mif_Syscfg_GetTime0();
static MI_U32 curtime[5];
static MI_U32 u32BufSize = 0;;
static pthread_mutex_t _gTime = PTHREAD_MUTEX_INITIALIZER;
#ifdef SUPPORT_WRITE_FILE
static void * ST_DoWaitData(ST_TEM_BUFFER stBuffer, pthread_mutex_t *pMutex)
{
    MI_SYS_ChnPort_t *pstChnPort = NULL;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32WriteSize = 0;
    int rval = 0;
    int fd= 0;
    int write_len= 0;
    pthread_mutex_lock(&_gTime);
    curtime[0] =  Mif_Syscfg_GetTime0();
    pstChnPort = &gstChnPort;
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    rval = poll(gpfd, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if(rval == 0)
    {
        printf("get fd time out!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if((gpfd[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    curtime[1] =  Mif_Syscfg_GetTime0();
    if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("GetBuf fail\n");
        pthread_mutex_unlock(&_gTime);
        return NULL;
    }
    curtime[2] =  Mif_Syscfg_GetTime0();
    fd = *((int *)stBuffer.pTemBuffer);
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            u32WriteSize = stBufInfo.stRawData.u32ContentSize;
            write_len = write(fd, stBufInfo.stRawData.pVirAddr, u32WriteSize);
            ASSERT(write_len == u32WriteSize);
            u32BufSize = u32WriteSize;
        }
        break;
        case E_MI_SYS_BUFDATA_FRAME:
        {
            u32WriteSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write_len = write(fd, stBufInfo.stFrameData.pVirAddr[0], u32WriteSize);
            ASSERT(write_len == u32WriteSize);
            u32BufSize = u32WriteSize;
            u32WriteSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] / 2;
            write_len = write(fd, stBufInfo.stFrameData.pVirAddr[0], u32WriteSize);
            ASSERT(write_len == u32WriteSize);
            u32BufSize += u32WriteSize;
        }
        break;
        default:
            ASSERT(0);
    }
    curtime[3] =  Mif_Syscfg_GetTime0();
    MI_SYS_ChnOutputPortPutBuf(stBufHandle);
    curtime[4] =  Mif_Syscfg_GetTime0();
    pthread_mutex_unlock(&_gTime);
    return NULL;
}
static void ST_TemDestroy(void)
{
    int fd;
    TemGetBuffer("Wait data", &fd);
    TemClose("Wait data");
    close(fd);
}
static void ST_TemCreate(void)
{
    ST_TEM_ATTR stAttr;
    pthread_attr_t m_SigMonThreadAttr;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    ST_TEM_USER_DATA stUserData;
    int fd = 0;
    fd = open("./venc_data", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0)
    {
        perror("open");
        return;
    }
    PTH_RET_CHK(pthread_attr_init(&m_SigMonThreadAttr));
    memset(&stAttr, 0, sizeof(ST_TEM_ATTR));
    stAttr.fpThreadDoSignal = NULL;
    stAttr.fpThreadWaitTimeOut = ST_DoWaitData;
    stAttr.thread_attr = m_SigMonThreadAttr;
    stAttr.u32ThreadTimeoutMs = 33;
    stAttr.bSignalResetTimer = 0;
    stAttr.stTemBuf.pTemBuffer = (void *)&fd;
    stAttr.stTemBuf.u32TemBufferSize = sizeof(int);
    TemOpen("Wait data", stAttr);
    TemStartMonitor("Wait data");
}
#endif
#if (ENABLE_DUMPCIF_PORT1 == 1)
static struct pollfd gpfd1[1] =
{
    {0, POLLIN | POLLERR},
};
static void * DC_DoWaitData(ST_TEM_BUFFER stBuffer, pthread_mutex_t *pMutex)
{
    int fd= 0;
    int rval = 0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    pthread_mutex_lock(&_gTime);
    rval = poll(gpfd1, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if(rval == 0)
    {
        //printf("get fd time out!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if((gpfd1[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&hHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    memset(&stVpeChnOutputPort0,0,sizeof(MI_SYS_ChnPort_t));
    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 1;
    fd = *((int *)stBuffer.pTemBuffer);
    if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
    {
    	// put frame
    	//printf("putframe\n");
    	test_vpe_PutOneFrame(fd,  stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], 352, 288);
    	test_vpe_PutOneFrame(fd,  stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u32Stride[1], 352, 288/2);
    	MI_SYS_ChnOutputPortPutBuf(hHandle);
    }
    pthread_mutex_unlock(&_gTime);
    return NULL;
}
static void DC_TemDestroy(void)
{
    int fd;
    TemGetBuffer("DC Wait data", &fd);
    TemClose("DC Wait data");
    close(fd);
}
static void DC_TemCreate(void)
{
    ST_TEM_ATTR stAttr;
    pthread_attr_t m_SigMonThreadAttr;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    ST_TEM_USER_DATA stUserData;
    char szFn[64];
    sprintf(szFn, "/mnt/yuv/out_CIF.yuv");
    int fd = open(szFn, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0)
    {
        perror("open");
        return;
    }
    PTH_RET_CHK(pthread_attr_init(&m_SigMonThreadAttr));
    memset(&stAttr, 0, sizeof(ST_TEM_ATTR));
    stAttr.fpThreadDoSignal = NULL;
    stAttr.fpThreadWaitTimeOut = DC_DoWaitData;
    stAttr.thread_attr = m_SigMonThreadAttr;
    stAttr.u32ThreadTimeoutMs = 33;
    stAttr.bSignalResetTimer = 0;
    stAttr.stTemBuf.pTemBuffer = (void *)&fd;
    stAttr.stTemBuf.u32TemBufferSize = sizeof(int);
    TemOpen("DC Wait data", stAttr);
    TemStartMonitor("DC Wait data");
}
#endif
static int wd_fd;
static int wd_fd_size;
static int rd_fd;
static int rd_fd_size;
static MI_U32 ST_DoGetData_mmap(MI_SYS_ChnPort_t * pstChnPort, void *pData,bool *is_tail)
{
#if (ENABLE_PUTES_TO_UVC == 1)
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32RetSize = 0;
    MI_U16 i = 0;
    MI_U8 *u8CopyData = pData;
    int rval = 0;
    ASSERT(pstChnPort);
    ASSERT(pData);
#if (RD_OR_WR == 0)
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    rval = poll(gpfd, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        return 0;
    }
    if(rval == 0)
    {
        printf("get fd time out!\n");
        return 0;
    }
    if((gpfd[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        return 0;
    }
    if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("Get buffer error!\n");
        return 0;
    }
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            u32RetSize = stBufInfo.stRawData.u32ContentSize;
            memcpy(pData, stBufInfo.stRawData.pVirAddr, u32RetSize);
            write(wd_fd, stBufInfo.stRawData.pVirAddr, u32RetSize);
            write(wd_fd_size, &u32RetSize, sizeof(MI_U32));
        }
        break;
        case E_MI_SYS_BUFDATA_FRAME:
        {
            u32RetSize = stBufInfo.stFrameData.u16Height * (stBufInfo.stFrameData.u32Stride[0] +  stBufInfo.stFrameData.u32Stride[1] / 2);
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
            u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
        }
        break;
        default:
            ASSERT(0);
    }
    MI_SYS_ChnOutputPortPutBuf(stBufHandle);
#else
    if (0 == read(rd_fd_size, &u32RetSize, 4))
    {
        lseek(rd_fd_size, 0, SEEK_SET);
        lseek(rd_fd, 0, SEEK_SET);
        read(rd_fd_size, &u32RetSize, 4);
    }
    printf("Get size is %x\n", u32RetSize);
    read(rd_fd, pData, u32RetSize);
#endif
#else
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32RetSize = 0;
    MI_U16 i = 0;
    MI_U8 *u8CopyData = pData;
    int rval = 0;
    ASSERT(pstChnPort);
    ASSERT(pData);
    pthread_mutex_lock(&_gTime);
    curtime[0] =  Mif_Syscfg_GetTime0();
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    rval = poll(gpfd, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if(rval == 0)
    {
        printf("get fd time out!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    if((gpfd[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    curtime[1] =  Mif_Syscfg_GetTime0();
    if (gstChnPort.eModId == E_MI_MODULE_ID_WARP)
    {
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
        {
            printf("Get buffer error!\n");
            pthread_mutex_unlock(&_gTime);
            return 0;
        }
        curtime[2] =  Mif_Syscfg_GetTime0();
        u32RetSize = stBufInfo.stFrameData.u16Height * (stBufInfo.stFrameData.u32Stride[0] +  stBufInfo.stFrameData.u32Stride[1] / 2);
        memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
        u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
        memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
        curtime[3] =  Mif_Syscfg_GetTime0();
        MI_SYS_ChnOutputPortPutBuf(stBufHandle);
        curtime[4] =  Mif_Syscfg_GetTime0();
    }
    else //raw data
    {
        stBufInfo.stRawData.pVirAddr = pData;
        if (MI_SUCCESS != MI_SYS_ChnOutputPortCopyToUsr(pstChnPort, &stBufInfo, &stBufHandle))
        {
            printf("Get buffer error!\n");
            pthread_mutex_unlock(&_gTime);
            return 0;
        }
        curtime[2] =  Mif_Syscfg_GetTime0();
        u32RetSize = stBufInfo.stRawData.u32ContentSize;
#ifdef UVC_SUPPORT_LL
        if((unsigned int)(stBufInfo.u64SidebandMsg) ==2)
            *is_tail = 1;
        else
            *is_tail = 0;
#else
        *is_tail = 1;
#endif
    }
    u32BufSize = u32RetSize;
    pthread_mutex_unlock(&_gTime);
#endif
    return u32RetSize;
}
static MI_S32 UVC_EnableVpe(ST_VPE_PortInfo_t *pVpePortInfo)
{
    ASSERT(pVpePortInfo);
    pVpePortInfo->DepVpeChannel = 0;
    pVpePortInfo->eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    pVpePortInfo->ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    STCHECKRESULT(ST_Vpe_CreatePort(MAIN_VENC_PORT, pVpePortInfo)); //default support port2 --->>> venc
#if (ENABLE_DUMPCIF_PORT1 == 1)
    ST_VPE_PortInfo_t stPortInfo;
    MI_SYS_ChnPort_t stChnPort;
    memset(&stPortInfo,0,sizeof(ST_VPE_PortInfo_t));
    memset(&stChnPort,0,sizeof(MI_SYS_ChnPort_t));
    stPortInfo.u16OutputWidth = 352;
    stPortInfo.u16OutputHeight = 288;
    stPortInfo.DepVpeChannel = 0;
    stPortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    STCHECKRESULT(ST_Vpe_CreatePort(1, &stPortInfo)); //default support port2 --->>> venc
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = stPortInfo.DepVpeChannel;
    stChnPort.u32PortId = 1;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5), 0);
    if(MI_SUCCESS != MI_SYS_GetFd(&stChnPort, (MI_S32 *)&gpfd1[0].fd))
    {
        printf("MI_SYS_GetFd fail\n");
    }
#endif
    return MI_SUCCESS;
}
static MI_S32 UVC_DisableVpe(void)
{
    STCHECKRESULT(ST_Vpe_StopPort(0, MAIN_VENC_PORT)); //default support port2 --->>> venc
    return MI_SUCCESS;
}
#ifdef SUPPORT_CDNN
static MI_S32 UVC_EnableCeva(ST_CEVA_PortInfo_t *pstCvPortInfo, ST_CEVA_ResolutionMap_t *pstCvMap)
{
    ASSERT(pstCvPortInfo);
    ASSERT(pstCvMap);
    pstCvPortInfo->stPortInfo.u16OutputWidth = 608;
    pstCvPortInfo->stPortInfo.u16OutputHeight = 352;
    pstCvPortInfo->stPortInfo.DepVpeChannel = 0;
    pstCvPortInfo->stPortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    pstCvPortInfo->stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    pstCvPortInfo->stChnPort.eModId = E_MI_MODULE_ID_VPE;
    pstCvPortInfo->stChnPort.u32DevId = 0;
    pstCvPortInfo->stChnPort.u32ChnId = 0;
    pstCvPortInfo->stChnPort.u32PortId = VDISP_PORT;
    pstCvMap->u16SrcWidth = 1920;
    pstCvMap->u16SrcHeight = 1080;
    pstCvMap->u16DstWidth = pstCvPortInfo->stPortInfo.u16OutputWidth;
    pstCvMap->u16DstHeight = pstCvPortInfo->stPortInfo.u16OutputHeight;
    STCHECKRESULT(ST_Vpe_CreatePort(VDISP_PORT, &pstCvPortInfo->stPortInfo)); //default support port1 --->>> cdnn
    STCHECKRESULT(ST_CEVA_Init());
    STCHECKRESULT(ST_CEVA_RegisterChn(pstCvPortInfo, pstCvMap));
    STCHECKRESULT(ST_CEVA_Start());
    return MI_SUCCESS;
}
static MI_S32 UVC_DisableCeva(void)
{
    MI_SYS_ChnPort_t stChnPort;
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = VDISP_PORT;
    STCHECKRESULT(ST_CEVA_Stop());
    STCHECKRESULT(ST_CEVA_UnRegisterChn(stChnPort));
    STCHECKRESULT(ST_CEVA_Deinit());
    STCHECKRESULT(ST_Vpe_StopPort(0, VDISP_PORT)); //default support port1 --->>> cdnn
}
#endif
static MI_S32 UVC_EnableWarp(ST_Warp_Timming_e eWarpTimming)
{
    MI_SYS_ChnPort_t stChnPort;
    ST_Sys_BindInfo_t stBindInfo;
    STCHECKRESULT(ST_Warp_Init(eWarpTimming));
    /************************************************
    Step6:  init Warp
    *************************************************/
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_WARP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 4);
    STCHECKRESULT(ST_Warp_CreateChannel(0)); //default support port2 --->>> venc
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_WARP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    printf("####FUNC %s\n", __FUNCTION__);
    return MI_SUCCESS;
}
static MI_S32 UVC_DisableWarp(void)
{
    ST_Sys_BindInfo_t stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_WARP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Warp_DestroyChannel(0)); //default support port2 --->>> venc
    STCHECKRESULT(ST_Warp_Exit());
    printf("####FUNC %s\n", __FUNCTION__);
    return MI_SUCCESS;
}
static MI_S32 UVC_EnableVenc(MI_VENC_ChnAttr_t *pstChnAttr)
{
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnPort;
    ST_Sys_BindInfo_t stBindInfo;
    ASSERT(pstChnAttr);
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    s32Ret = MI_VENC_CreateChn(0, pstChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    ST_DBG("u32DevId:%d\n", u32DevId);
    stChnPort.u32DevId = u32DevId;
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    s32Ret = MI_VENC_StartRecvPic(0);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_WARP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    printf("####FUNC %s\n", __FUNCTION__);
    return MI_SUCCESS;
}
static MI_S32 UVC_DisableVenc(void)
{
    ST_Sys_BindInfo_t stBindInfo;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_WARP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    s32Ret = MI_VENC_StopRecvPic(0);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    s32Ret = MI_VENC_DestroyChn(0);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    printf("####FUNC %s\n", __FUNCTION__);
    return MI_SUCCESS;
}
static void UVC_EnableWindow(UVC_FormatTimming_e eFormatTimming)
{
    ST_VPE_PortInfo_t stPortInfo;
    ST_CEVA_PortInfo_t stCvPortInfo;
    ST_CEVA_ResolutionMap_t stCvMap;
    ST_Sys_BindInfo_t stBindInfo;
    MI_S32 s32Ret = 0;
    MI_U32 u32FrameRate = 30;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_VPE_PortMode_t stVpeMode;
    MI_VENC_ParamJpeg_t stJpegPara;
    printf("####FUNC %s\n", __FUNCTION__);
    printf("Timming! %d\n", eFormatTimming);
    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stJpegPara, 0, sizeof(MI_VENC_ParamJpeg_t));
#ifdef UVC_SUPPORT_LL
    stChnAttr.stVeAttr.stAttrJpeg.bLowLatencyMode = 1;
#endif
    switch(eFormatTimming)
    {
        case E_UVC_TIMMING_4K2K_JPG:
        {
            stPortInfo.u16OutputWidth = 3840;
            stPortInfo.u16OutputHeight = 2160;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 2160;
            MI_VENC_GetJpegParam(0, &stJpegPara);
            stJpegPara.u32Qfactor = 50;
            MI_VENC_SetJpegParam(0, &stJpegPara);
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_JPG:
        {
            stPortInfo.u16OutputWidth = 2560;
            stPortInfo.u16OutputHeight = 1440;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 1440;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_JPG:
        {
            stPortInfo.u16OutputWidth = 1920;
            stPortInfo.u16OutputHeight = 1080;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 1080;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_JPG:
        {
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 720;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_JPG:
        {
            stPortInfo.u16OutputWidth = 640;
            stPortInfo.u16OutputHeight = 480;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 480;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_JPG:
        {
            stPortInfo.u16OutputWidth = 320;
            stPortInfo.u16OutputHeight = 240;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 240;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_4K2K_H264:
        {
            stPortInfo.u16OutputWidth = 3840;
            stPortInfo.u16OutputHeight = 2160;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 2160;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_H264:
        {
            stPortInfo.u16OutputWidth = 2560;
            stPortInfo.u16OutputHeight = 1440;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1440;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_H264:
        {
            stPortInfo.u16OutputWidth = 1920;
            stPortInfo.u16OutputHeight = 1080;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1080;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
           
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_H264:
        {
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 720;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_H264:
        {
            stPortInfo.u16OutputWidth = 640;
            stPortInfo.u16OutputHeight = 480;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 480;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_H264:
        {
            stPortInfo.u16OutputWidth = 320;
            stPortInfo.u16OutputHeight = 240;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 240;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_4K2K_H265:
        {
            stPortInfo.u16OutputWidth = 3840;
            stPortInfo.u16OutputHeight = 2160;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 2160;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_H265:
        {
            stPortInfo.u16OutputWidth = 2560;
            stPortInfo.u16OutputHeight = 1440;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 1440;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_H265:
        {
            stPortInfo.u16OutputWidth = 1920;
            stPortInfo.u16OutputHeight = 1080;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 1080;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_H265:
        {
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 720;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_H265:
        {
            stPortInfo.u16OutputWidth = 640;
            stPortInfo.u16OutputHeight = 480;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 480;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_H265:
        {
            stPortInfo.u16OutputWidth = 320;
            stPortInfo.u16OutputHeight = 240;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 240;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate=1024*1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=45;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080_NV12:
        {
            stPortInfo.u16OutputWidth = 1920;
            stPortInfo.u16OutputHeight = 1080;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
        }
        break;
        case E_UVC_TIMMING_1280X720_NV12:
        {
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
        }
        break;
        case E_UVC_TIMMING_640X480_NV12:
        {
            stPortInfo.u16OutputWidth = 640;
            stPortInfo.u16OutputHeight = 480;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
        }
        break;
        case E_UVC_TIMMING_320X240_NV12:
        {
            stPortInfo.u16OutputWidth = 320;
            stPortInfo.u16OutputHeight = 240;
            UVC_EnableVpe(&stPortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
        }
        break;
        default:
            printf("Format not support!\n");
            return;
    }
#ifdef SUPPORT_CDNN
    UVC_EnableCeva(&stCvPortInfo, &stCvMap);
#endif
}
static void UVC_DisableWindow(MI_BOOL bDisableVenc)
{
    printf("####FUNC %s\n", __FUNCTION__);
    if (bDisableVenc)
    {
        UVC_DisableVenc();
    }
#ifdef SUPPORT_CDNN
    UVC_DisableCeva();
#endif
    UVC_DisableWarp();
    UVC_DisableVpe();
}
static MI_S32 UVC_Init(void *uvc)
{
    return MI_SUCCESS;
}
static MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}
static MI_S32 UVC_UP_FinishBUffer(int handle)
{
    return MI_SUCCESS;
}
static MI_S32 UVC_UP_FillBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
        //todo
    if(bufInfo->length ==0)
        return -1;
    return MI_SUCCESS;
}
static MI_S32 UVC_MM_FillBuffer(void *uvc,ST_UVC_BufInfo_t *bufInfo)
{
#ifndef SUPPORT_WRITE_FILE
    bufInfo->length = ST_DoGetData_mmap(&gstChnPort, bufInfo->b.buf,&(bufInfo->is_tail));
#endif
    if(bufInfo->length ==0)
        return -1;
    return MI_SUCCESS;
}
static MI_S32 UVC_StartCapture(void *uvc,Stream_Params_t format)
{
    memset(&gstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
#if (ENABLE_PUTES_TO_UVC == 1)
#if (RD_OR_WR == 0)
    wd_fd = open("./venc_data", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (wd_fd < 0)
    {
        perror("open");
        return;
    }
    wd_fd_size = open("./venc_data_size", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (wd_fd_size < 0)
    {
        perror("open");
        return;
    }
#else
    rd_fd = open("./venc_data", O_RDONLY);
    if (wd_fd < 0)
    {
        perror("open");
        return;
    }
    rd_fd_size = open("./venc_data_size",O_RDONLY);
    if (wd_fd_size < 0)
    {
        perror("open");
        return;
    }
#endif
#endif
    printf("Fmt %s width %d height %d\n", 
           uvc_get_format(format.fcc), format.width, format.height);
    switch(format.fcc)
    {
        case V4L2_PIX_FMT_NV12:
        {
            if (format.width == 1920 && format.height == 1080)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1920X1080_NV12);
            }
            else if (format.width == 1280 && format.height == 720)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1280X720_NV12);
            }
            else if (format.width == 640 && format.height == 480)
            {
                UVC_EnableWindow(E_UVC_TIMMING_640X480_NV12);
            }
            else if (format.width == 320 && format.height == 240)
            {
                UVC_EnableWindow(E_UVC_TIMMING_320X240_NV12);
            }
            else
            {
                printf("NV12 not support format! width %d height %d\n", format.width, format.height);
                return -1;
            }
            gstChnPort.eModId = E_MI_MODULE_ID_WARP;
            gstChnPort.u32DevId = 0;
            gstChnPort.u32ChnId = 0;
            gstChnPort.u32PortId = 0;
        }
        break;
        case V4L2_PIX_FMT_H264:
        {
            if (format.width == 3840 && format.height == 2160)
            {
                UVC_EnableWindow(E_UVC_TIMMING_4K2K_H264);
            }
            else if (format.width == 2560 && format.height == 1440)
            {
                UVC_EnableWindow(E_UVC_TIMMING_2560X1440P_H264);
            }
            else if (format.width == 1920 && format.height == 1080)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1920X1080P_H264);
            }
            else if (format.width == 1280 && format.height == 720)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1280X720P_H264);
            }
            else if (format.width == 640 && format.height == 480)
            {
                UVC_EnableWindow(E_UVC_TIMMING_640X480P_H264);
            }
            else if (format.width == 320 && format.height == 240)
            {
                UVC_EnableWindow(E_UVC_TIMMING_320X240P_H264);
            }
            else
            {
                printf("H264 not support format! width %d height %d\n", format.width, format.height);
                return -1;
            }
            gstChnPort.eModId = E_MI_MODULE_ID_VENC;
            MI_VENC_GetChnDevid(0, &gstChnPort.u32DevId);
            gstChnPort.u32ChnId = 0;
            gstChnPort.u32PortId = 0;
        }
        break;
        case V4L2_PIX_FMT_MJPEG:
        {
            if (format.width == 3840 && format.height == 2160)
            {
                UVC_EnableWindow(E_UVC_TIMMING_4K2K_JPG);
            }
            else if (format.width == 2560 && format.height == 1440)
            {
                UVC_EnableWindow(E_UVC_TIMMING_2560X1440P_JPG);
            }
            else if (format.width == 1920 && format.height == 1080)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1920X1080P_JPG);
            }
            else if (format.width == 1280 && format.height == 720)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1280X720P_JPG);
            }
            else if (format.width == 640 && format.height == 480)
            {
                UVC_EnableWindow(E_UVC_TIMMING_640X480P_JPG);
            }
            else if (format.width == 320 && format.height == 240)
            {
                UVC_EnableWindow(E_UVC_TIMMING_320X240P_JPG);
            }
            else
            {
                printf("Mjpeg not support format! width %d height %d\n", format.width, format.height);
                return -1;
            }
            gstChnPort.eModId = E_MI_MODULE_ID_VENC;
            MI_VENC_GetChnDevid(0, &gstChnPort.u32DevId);
            gstChnPort.u32ChnId = 0;
            gstChnPort.u32PortId = 0;
        }
        break;
        case V4L2_PIX_FMT_H265:
        {
            if (format.width == 3840 && format.height == 2160)
            {
                UVC_EnableWindow(E_UVC_TIMMING_4K2K_H265);
            }
            else if (format.width == 2560 && format.height == 1440)
            {
                UVC_EnableWindow(E_UVC_TIMMING_2560X1440P_H265);
            }
            else if (format.width == 1920 && format.height == 1080)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1920X1080P_H265);
            }
            else if (format.width == 1280 && format.height == 720)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1280X720P_H265);
            }
            else if (format.width == 640 && format.height == 480)
            {
                UVC_EnableWindow(E_UVC_TIMMING_640X480P_H265);
            }
            else if (format.width == 320 && format.height == 240)
            {
                UVC_EnableWindow(E_UVC_TIMMING_320X240P_H265);
            }
            else
            {
                printf("H265 not support format! width %d height %d\n", format.width, format.height);
                return -1;
            }
            gstChnPort.eModId = E_MI_MODULE_ID_VENC;
            MI_VENC_GetChnDevid(0, &gstChnPort.u32DevId);
            gstChnPort.u32ChnId = 0;
            gstChnPort.u32PortId = 0;
        }
        break;
        default:
            printf("not support format!\n");
            return -1;
    }
    if(MI_SUCCESS != MI_SYS_GetFd(&gstChnPort, (MI_S32 *)&gpfd[0].fd))
    {
        printf("MI_SYS_GetFd fail\n");
    }
#ifdef SUPPORT_WRITE_FILE
    ST_TemCreate();
#endif
    return MI_SUCCESS;
}
static MI_S32 UVC_StopCapture(void *uvc)
{
#ifdef SUPPORT_WRITE_FILE
    ST_TemDestroy();
#endif
#if(ENABLE_DUMPCIF_PORT1 == 1)
    DC_TemDestroy();
#endif
    if(MI_SUCCESS != MI_SYS_CloseFd(gpfd[0].fd))
    {
        printf("MI_SYS_CloseFd fail\n");
    }
    if (gstChnPort.eModId == E_MI_MODULE_ID_WARP)
    {
        UVC_DisableWindow(FALSE);
    }
    else if (gstChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        UVC_DisableWindow(TRUE);
    }
    else
        printf("Not support!\n");
#if (ENABLE_PUTES_TO_UVC == 1)
#if (RD_OR_WR == 0)
    close(wd_fd);
    close(wd_fd_size);
#else
    close(rd_fd);
    close(rd_fd_size);
#endif
#endif
    return MI_SUCCESS;
}
static MI_S32 St_UvcInit()
{
    ST_UVC_Setting_t pstSet={4,UVC_MEMORY_MODE,USB_BULK_MODE};
    ST_UVC_OPS_t fops = { UVC_Init ,
                          UVC_Deinit,
#ifdef UVC_SUPPORT_MMAP
                          UVC_MM_FillBuffer,
#else
                          UVC_UP_FillBuffer,
                          UVC_UP_FinishBUffer,
#endif
                          UVC_StartCapture,
                          UVC_StopCapture};
    ST_UVC_ChnAttr_t pstAttr ={pstSet,fops};
    STCHECKRESULT(ST_UVC_Init("/dev/video0"));
    STCHECKRESULT(ST_UVC_CreateDev(&pstAttr));
    STCHECKRESULT(ST_UVC_StartDev());
    return MI_SUCCESS;
}
static MI_S32 St_UvcDeinit()
{
    STCHECKRESULT(ST_UVC_StopDev());
    STCHECKRESULT(ST_UVC_DestroyDev(0));
    STCHECKRESULT(ST_UVC_Uninit());
    return MI_SUCCESS;
}

static MI_S32 st_BindModule(    MI_ModuleId_e eSrcModId, MI_U32 u32SrcChnId, MI_U32 u32SrcPortId,
                               MI_ModuleId_e eDstModId, MI_U32 u32DstChnId, MI_U32 u32DstPortId, MI_BOOL bBind)
{
    ST_Sys_BindInfo_t stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = eSrcModId;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = u32SrcChnId;
    stBindInfo.stSrcChnPort.u32PortId = u32SrcPortId;
    stBindInfo.stDstChnPort.eModId = eDstModId;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = u32DstChnId;
    stBindInfo.stDstChnPort.u32PortId = u32DstPortId;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;

    if(bBind == TRUE)
    {
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
    else
    {
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    return MI_SUCCESS;
}

static MI_S32 St_DisplayInit(void)
{
    ST_VPE_PortInfo_t stPortInfo;
    ST_VPE_PortInfo_t stPor3tInfo;
    ST_Sys_BindInfo_t stBindInfo;
    ST_Sys_BindInfo_t stBindInfo1;
    ST_Sys_BindInfo_t stBindInfo2;
    ST_Sys_BindInfo_t stBindInfo3;
    ST_Sys_BindInfo_t stBindInfo4;
    ST_DispChnInfo_t stDispChnInfo;
    memset(&stPortInfo, 0x0, sizeof(ST_VPE_PortInfo_t));
    memset(&stPor3tInfo, 0x0, sizeof(ST_VPE_PortInfo_t));
    memset(&stDispChnInfo, 0x0, sizeof(ST_DispChnInfo_t));
    stPortInfo.DepVpeChannel = 0;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stPortInfo.u16OutputWidth = 1920;
    stPortInfo.u16OutputHeight = 1080;
    STCHECKRESULT(ST_Vpe_CreatePort(0, &stPortInfo)); //default support port0 --->>> vdisp

    stPor3tInfo.DepVpeChannel = 0;
    stPor3tInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stPor3tInfo.u16OutputWidth = 1920;
    stPor3tInfo.u16OutputHeight = 1080;
    STCHECKRESULT(ST_Vpe_CreatePort(3, &stPor3tInfo)); //default support port0 --->>> vdisp
    
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, E_MI_DISP_OUTPUT_3840x2160_30)); //Dispout timing
    stDispChnInfo.InputPortNum = 2;
    stDispChnInfo.stInputPortAttr[0].u32Port = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width = 1088;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height = 1920;

    stDispChnInfo.stInputPortAttr[1].u32Port = 1;
    stDispChnInfo.stInputPortAttr[1].stAttr.stDispWin.u16X = 1088;
    stDispChnInfo.stInputPortAttr[1].stAttr.stDispWin.u16Y = 0;
    stDispChnInfo.stInputPortAttr[1].stAttr.stDispWin.u16Width = 1920;
    stDispChnInfo.stInputPortAttr[1].stAttr.stDispWin.u16Height = 1080;
    STCHECKRESULT(ST_Disp_ChnInit(0, &stDispChnInfo));

    
    /************************************************
    Step8:  Bind VPE->VDISP
    *************************************************/

    st_BindModule(E_MI_MODULE_ID_VPE, 0, 0, E_MI_MODULE_ID_DIVP, 0, 0, TRUE);
    st_BindModule(E_MI_MODULE_ID_DIVP, 0, 0, E_MI_MODULE_ID_DIVP, 1, 0, TRUE);
    st_BindModule(E_MI_MODULE_ID_DIVP, 1, 0, E_MI_MODULE_ID_DISP, 0, 0, TRUE);

    st_BindModule(E_MI_MODULE_ID_VPE, 0, 3, E_MI_MODULE_ID_DISP, 0, 1, TRUE);
    
    STCHECKRESULT(ST_Hdmi_Init());
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, E_MI_HDMI_TIMING_4K2K_30P)); //Hdmi timing
    return MI_SUCCESS;
}
static MI_S32 St_DisplayDeinit(void)
{
    ST_VPE_PortInfo_t stPortInfo;
    ST_DispChnInfo_t stDispChnInfo;
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));

    st_BindModule(E_MI_MODULE_ID_VPE, 0, 0, E_MI_MODULE_ID_DIVP, 0, 0, FALSE);
    st_BindModule(E_MI_MODULE_ID_DIVP, 0, 0, E_MI_MODULE_ID_DIVP, 1, 0, FALSE);
    st_BindModule(E_MI_MODULE_ID_DIVP, 1, 0, E_MI_MODULE_ID_DISP, 0, 0, FALSE);

    st_BindModule(E_MI_MODULE_ID_VPE, 0, 3, E_MI_MODULE_ID_DISP, 0, 1, FALSE);
    
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, ST_DISP_LAYER0, 2));
    STCHECKRESULT(ST_Vpe_StopPort(0, 0));
    return MI_SUCCESS;
}
static MI_S32 St_BaseModuleInit(MI_BOOL bFramOrReal)
{
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    MI_SYS_ChnPort_t stChnPort;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_Sys_BindInfo_t stBindInfo;
    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVpeChannelInfo, 0x0, sizeof(ST_VPE_ChannelInfo_t));
    STCHECKRESULT(ST_Sys_Init());
    STCHECKRESULT(ST_Vif_CreateDev(0, bFramOrReal?SAMPLE_VI_MODE_MIPI_1_1080P_FRAME:SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 3840;
    stVifPortInfoInfo.u32RectHeight = 2160;
    stVifPortInfoInfo.u32DestWidth = 3840;
    stVifPortInfoInfo.u32DestHeight = 2160;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 3);
    STCHECKRESULT(ST_Vif_StartPort(0, 0));
    /************************************************
    Step4:  init VPE
    *************************************************/
    stVpeChannelInfo.u16VpeMaxW = 3840;
    stVpeChannelInfo.u16VpeMaxH = 2160;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 3840;
    stVpeChannelInfo.u16VpeCropH = 2160;
    stVpeChannelInfo.eRunningMode = bFramOrReal?E_MI_VPE_RUN_CAM_MODE:E_MI_VPE_RUN_REALTIME_MODE;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));

    MI_SYS_WindowRect_t stCropRect;
    memset(&stCropRect, 0, sizeof(stCropRect));
    stCropRect.u16Width = 0;
    stCropRect.u16Height = 0;
    STCHECKRESULT(ST_Divp_CreatChannel(0, E_MI_SYS_ROTATE_90, &stCropRect));

    MI_SYS_WindowRect_t stOutputWin;
    memset(&stOutputWin, 0, sizeof(stOutputWin));
    stOutputWin.u16Width = 1080;
    stOutputWin.u16Height = 1920;
    STCHECKRESULT(ST_Divp_SetOutputAttr(0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, &stOutputWin));
    STCHECKRESULT(ST_Divp_StartChn(0));

    MI_SYS_WindowRect_t stCropRect1;
    memset(&stCropRect1, 0, sizeof(stCropRect1));
    stCropRect1.u16Width = 0;
    stCropRect1.u16Height = 0;
    STCHECKRESULT(ST_Divp_CreatChannel(1, E_MI_SYS_ROTATE_NONE, &stCropRect1));
    STCHECKRESULT(ST_Divp_SetOutputAttr(1, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV, &stOutputWin));
    STCHECKRESULT(ST_Divp_StartChn(1));
    
    /************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    return MI_SUCCESS;
}
static MI_S32 St_BaseModuleDeinit(void)
{
    STCHECKRESULT(ST_Divp_StopChn(0));
    STCHECKRESULT(ST_Divp_DestroyChn(0));

    STCHECKRESULT(ST_Divp_StopChn(1));
    STCHECKRESULT(ST_Divp_DestroyChn(1));

    STCHECKRESULT(ST_Divp_StopChn(2));
    STCHECKRESULT(ST_Divp_DestroyChn(2));
    
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));
    STCHECKRESULT(ST_Sys_Exit());
    return MI_SUCCESS;
}
static void St_FbInit(void)
{
    ST_Fb_Init(E_MI_FB_COLOR_FMT_ARGB1555);
    sleep(1);

    ST_FB_ChangeResolution(1080, 1920);

#if 0
    MI_SYS_WindowRect_t Rect;

    Rect.u16X = 100;
    Rect.u16Y = 100;
    Rect.u16Width = 200;
    Rect.u16Height = 200;
    printf("1 R:%X,G:%X,B:%X\n", PIXEL8888RED(ARGB888_RED), PIXEL8888GREEN(ARGB888_RED), PIXEL8888BLUE(ARGB888_RED));
    ST_Fb_FillRect(&Rect, ARGB888_RED);
#endif

    ST_FB_Show(TRUE);

    ST_Fb_SetColorKey(ARGB888_BLUE);

    MI_FB_GlobalAlpha_t stAlphaInfo;

    memset(&stAlphaInfo, 0, sizeof(MI_FB_GlobalAlpha_t));

    ST_FB_GetAlphaInfo(&stAlphaInfo);

    printf("FBIOGET_GLOBAL_ALPHA alpha info: alpha blend enable=%d,Multialpha enable=%d,Global Alpha=%d,u8Alpha0=%d,u8Alpha1=%d\n",
            stAlphaInfo.bAlphaEnable,stAlphaInfo.bAlphaChannel,stAlphaInfo.u8GlobalAlpha,stAlphaInfo.u8Alpha0,stAlphaInfo.u8Alpha1);

    stAlphaInfo.bAlphaEnable = TRUE;
    stAlphaInfo.bAlphaChannel= TRUE;
    stAlphaInfo.u8GlobalAlpha = 0x70;
    ST_FB_SetAlphaInfo(&stAlphaInfo);

    //ST_FB_ShowBMP("background.bmp");
    ST_FB_ShowBMP(0, 0, "background.bmp");
}

static void do_fix_mantis(int mantis)
{
    printf("To fix mantis 1574080!\n");
    return;
}
#if (ENABLE_DUMPCIF_PORT1 == 1)
MI_S32 test_vpe_PutOneFrame(int dstFd,  char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;
    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for (i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset*i;
        yuvSize = line_size;
        do {
            if (yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }
            size = write(dstFd, pData, size);
            if (size == 0)
            {
                break;
            }
            else if (size < 0)
            {
                break;
            }
            pData += size;
            yuvSize -= size;
        } while (yuvSize > 0);
    }
    return 0;
}
#endif
MI_S32 main(int argc, char **argv)
{
    MI_S32 s32Mode = 0;
    char cmd = 0;
    int i = 0;
    if (signal(SIGINT, do_fix_mantis) != 0)
    {
        perror("signal");
        return -1;
    }
    if (signal(SIGTSTP, do_fix_mantis) != 0)
    {
        perror("signal");
        return -1;
    }
    printf("Which mode do you want ?\n 0 for real time mode 1 for frame mode\n");
    scanf("%d", &s32Mode);
    printf("You select %s mode\n", s32Mode?"frame":"real time");
    St_BaseModuleInit(s32Mode);
    St_DisplayInit();
    St_FbInit();
    St_UvcInit();
#if(ENABLE_DUMPCIF_PORT1 == 1)
    DC_TemCreate();
#endif
    while(1)
    {
        printf("Type \"q\" to exit\n");
        cmd = getchar();
        if (cmd == 'q')
            break;
        pthread_mutex_lock(&_gTime);
        for (i = 0; i < 5; i++)
        {
            printf("time [%d] = %d\n", i, curtime[i]);
        }
        printf("Buf size is %d\n", u32BufSize);
        pthread_mutex_unlock(&_gTime);
    }
    St_UvcDeinit();
    printf("St_UvcDeinit deinit\n");
    St_DisplayDeinit();
    printf("St_DisplayDeinit deinit\n");
    St_BaseModuleDeinit();
    return 0;
}
