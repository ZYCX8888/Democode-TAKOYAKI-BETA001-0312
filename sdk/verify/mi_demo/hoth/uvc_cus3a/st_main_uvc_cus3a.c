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
#include "mi_vpe.h"
#include "mi_venc.h"
//#include "mi_alsa.h"
#include "mi_aio.h"
#include "mi_aio_datatype.h"
//#include "st_uvc.h" //infinity5
#include "st_common.h"
//#include "st_hdmi.h" //infinity5
//#include "st_disp.h" //infinity5
#include "st_vpe.h"
#include "st_vif.h"
//#include "st_fb.h" //infinity5
//#include "st_warp.h" //infinity5
//#include "st_sd.h" //infinity5
//#include "list.h" //infinity5
#include "mi_rgn.h"
//#include "tem.h" //infinity5
#include "mi_isp.h"
#include "mi_vpe.h"
#include "isp_cus3a_if.h"
#include "isp_3a_if.h"
#include "isp_sigma3a_ext.h"

#define UVC_SUPPORT_MMAP
//#define UVC_SUPPORT_LL
#ifndef UVC_SUPPORT_MMAP
    #define UVC_MEMORY_MODE UVC_MEMORY_USERPTR
#else
    #define UVC_MEMORY_MODE UVC_MEMORY_MMAP
#endif
//#define SUPPORT_WRITE_FILE
#define ENABLE_PUTES_TO_UVC 0
#define RD_OR_WR 1
#define ENABLE_DUMPCIF_PORT1 0

#define UVC_FUNCTION_EN (0) //infinity5


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
    E_UVC_TIMMING_320X240_NV12,
    E_UVC_TIMMING_1920X1080_YUV422_YUYV,
    E_UVC_TIMMING_1280X720_YUV422_YUYV,
    E_UVC_TIMMING_640X480_YUV422_YUYV,
    E_UVC_TIMMING_320X240_YUV422_YUYV,
} UVC_FormatTimming_e;

#ifdef SUPPORT_WRITE_FILE
    static MI_SYS_ChnPort_t gstChnPort;
#else
    #if (UVC_FUNCTION_EN)
        static MI_SYS_ChnPort_t gstChnPort;
    #endif
#endif

#if (UVC_FUNCTION_EN)
static struct pollfd gpfd[1] =
{
    {0, POLLIN | POLLERR},
};
#endif

typedef struct ST_ModuleState_s
{
    MI_BOOL bEnableVpe;
    MI_BOOL bEnableWarp;
    MI_BOOL bEnableCevaVx;
    MI_BOOL bEnableSd;
    MI_BOOL bEnableVenc;
} ST_ModuleState_t;

extern MI_U32 Mif_Syscfg_GetTime0();
static MI_U32 curtime[5];
static MI_U32 u32BufSize = 0;;
static pthread_mutex_t _gTime = PTHREAD_MUTEX_INITIALIZER;
#if (UVC_FUNCTION_EN)  //infinity5
    static MI_S32 _gs32Nv12ToMjpg = 0;
    static MI_SYS_ChnPort_t _gstVencSrcPort;
    static ST_ModuleState_t _gstModState;
#endif
static MI_S32 _gs32UseUac = 0;
#if (UVC_FUNCTION_EN)  //infinity5
    static pthread_t _gUacSendFrameThread;
    static MI_BOOL _gUacSendFrameWorkExit = false;
    static MI_BOOL bFrameOrRealMode = 0;
    static MI_BOOL bMJPG = 0;
#endif

#if (UVC_FUNCTION_EN) //infinity5
/* UVC_SUPPORT_USERPTR */
typedef struct userptr_mem_s
{
    bool inited;
    unsigned int num;
    pthread_mutex_t lock;
    struct
    {
        int busy;
        unsigned long start;
    } mem[];
} userptr_mem_t;
userptr_mem_t userptr_mem;
static int userptr_mem_init(unsigned int num, unsigned long size)
{
    if(userptr_mem.inited)
        return -EBUSY;

    int i;

    pthread_mutex_init(&(userptr_mem.lock), NULL);
    memset(&userptr_mem, 0x00, sizeof(userptr_mem_t));
    userptr_mem.num = num;

    pthread_mutex_lock(&(userptr_mem.lock));
    for(i = 0; i < num; i++)
    {
        userptr_mem.mem[i].start =  (unsigned long)malloc(size);
        userptr_mem.mem[i].busy = false;
    }
    userptr_mem.inited = true;
    pthread_mutex_unlock(&(userptr_mem.lock));
    return 0;
}
static int userptr_mem_exit()
{
    if(!userptr_mem.inited)
        return -EINVAL;

    int i;

    pthread_mutex_lock(&(userptr_mem.lock));
    for(i = 0; i < userptr_mem.num; i++)
    {
        free((void*)(userptr_mem.mem[i].start));
    }
    userptr_mem.inited = false;
    pthread_mutex_unlock(&(userptr_mem.lock));
    pthread_mutex_destroy(&(userptr_mem.lock));
    return 0;
}
static long userptr_mem_get_buf()
{
    if(!userptr_mem.inited)
        return -EINVAL;

    int i;
    unsigned long start = -EINVAL;

    pthread_mutex_lock(&(userptr_mem.lock));
    for(i = 0; i < userptr_mem.num; i++)
    {
        if(!userptr_mem.mem[i].busy)
        {
            userptr_mem.mem[i].busy = true;
            start = userptr_mem.mem[i].start;
            break;
        }
    }
    pthread_mutex_unlock(&(userptr_mem.lock));
    return start;
}
static int userptr_mem_buf_complete(unsigned long start)
{
    if(!userptr_mem.inited)
        return -EINVAL;

    int i, ret = -EINVAL;

    pthread_mutex_lock(&(userptr_mem.lock));
    for(i = 0; i < userptr_mem.num; i++)
    {
        if(start == userptr_mem.mem[i].start &&
                true == userptr_mem.mem[i].busy )
        {
            userptr_mem.mem[i].busy = false;
            ret = 0;
            break;
        }
    }
    pthread_mutex_unlock(&(userptr_mem.lock));
    return ret;
}
#endif

#ifdef SUPPORT_WRITE_FILE
static void * ST_DoWaitData(ST_TEM_BUFFER stBuffer)
{
    MI_SYS_ChnPort_t *pstChnPort = NULL;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32WriteSize = 0;
    int rval = 0;
    int fd = 0;
    int write_len = 0;
    void *pData = NULL;
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
    fd = *((int *)stBuffer.pTemBuffer);
    if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("GetBuf fail\n");
        pthread_mutex_unlock(&_gTime);
        return NULL;
    }
    curtime[2] =  Mif_Syscfg_GetTime0();
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
            u32WriteSize = stBufInfo.stRawData.u32ContentSize;
            write_len = write(fd, stBufInfo.stRawData.pVirAddr, u32WriteSize);
            ASSERT(write_len == u32WriteSize);
            u32BufSize = u32WriteSize;
            break;
        case E_MI_SYS_BUFDATA_FRAME:
            if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
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
            else if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422)
            {
                u32WriteSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write_len = write(fd, stBufInfo.stFrameData.pVirAddr[0], u32WriteSize);
                ASSERT(write_len == u32WriteSize);
                u32BufSize = u32WriteSize;
            }
            else
                ASSERT(0);
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
    stAttr.u32ThreadTimeoutMs = 20;
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
static void * DC_DoWaitData(ST_TEM_BUFFER stBuffer)
{
    int fd = 0;
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
    memset(&stVpeChnOutputPort0, 0, sizeof(MI_SYS_ChnPort_t));
    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 1;
    fd = *((int *)stBuffer.pTemBuffer);
    if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0, &stBufInfo, &hHandle))
    {
        // put frame
        //printf("putframe\n");
        test_vpe_PutOneFrame(fd,  stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], 352, 288);
        test_vpe_PutOneFrame(fd,  stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u32Stride[1], 352, 288 / 2);
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

#if (UVC_FUNCTION_EN)  //infinity5
static MI_U32 ST_DoGetData_userptr(MI_SYS_ChnPort_t * pstChnPort,
                                   unsigned long *pData, int *handle, bool *is_tail)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32RetSize = 0;
    MI_U16 i = 0;
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
    if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("Get buffer error!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    curtime[2] =  Mif_Syscfg_GetTime0();
    *handle = stBufHandle;
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
            *pData = (unsigned long)stBufInfo.stRawData.pVirAddr;
            u32RetSize = stBufInfo.stRawData.u32ContentSize;
            break;
        case E_MI_SYS_BUFDATA_FRAME:
            if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                *pData = (unsigned long)stBufInfo.stFrameData.pVirAddr[0];
                u32RetSize = stBufInfo.stFrameData.u16Height *
                             (stBufInfo.stFrameData.u32Stride[0] +
                              stBufInfo.stFrameData.u32Stride[1] / 2);
            }
            else if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
            {
                *pData = (unsigned long)stBufInfo.stFrameData.pVirAddr[0];
                u32RetSize =  stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            }
            else
                ASSERT(0);
            break;
        default:
            ASSERT(0);
    }
    curtime[3] =  Mif_Syscfg_GetTime0();
#ifdef UVC_SUPPORT_LL
    if(bMJPG)
    {
        if((unsigned int)(stBufInfo.u64SidebandMsg) == 2)
            *is_tail = 1;
        else
            *is_tail = 0;
    }
    else
    {
        *is_tail = 1;
    }
#else
    *is_tail = 1;
#endif
    u32BufSize = u32RetSize;

    return u32RetSize;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static int wd_fd;
static int wd_fd_size;
static int rd_fd;
static int rd_fd_size;

static MI_U64 u64preInitTime;
static MI_U32 ST_DoGetData_mmap(MI_SYS_ChnPort_t * pstChnPort, void *pData, bool *is_tail, MI_U64 *u64LLInitTime)
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
            memcpy(u8CopyData, stBufInfo.stRawData.pVirAddr, u32RetSize);
            write(wd_fd, stBufInfo.stRawData.pVirAddr, u32RetSize);
            write(wd_fd_size, &u32RetSize, sizeof(MI_U32));
        }
        break;
        case E_MI_SYS_BUFDATA_FRAME:
        {
            if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                u32RetSize = stBufInfo.stFrameData.u16Height * (stBufInfo.stFrameData.u32Stride[0] +  stBufInfo.stFrameData.u32Stride[1] / 2);
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
                write(wd_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);

                u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] / 2);
                write(wd_fd, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] / 2);
                write(wd_fd_size, &u32RetSize, sizeof(MI_U32));
            }
            else if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422)
            {
                u32RetSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], u32RetSize);
                write(wd_fd, stBufInfo.stRawData.pVirAddr, u32RetSize);
                write(wd_fd_size, &u32RetSize, sizeof(MI_U32));
            }
            else
                ASSERT(0);
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
    if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("Get buffer error!\n");
        pthread_mutex_unlock(&_gTime);
        return 0;
    }
    curtime[2] =  Mif_Syscfg_GetTime0();
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
            u32RetSize = stBufInfo.stRawData.u32ContentSize;
            memcpy(u8CopyData, stBufInfo.stRawData.pVirAddr, u32RetSize);
            break;
        case E_MI_SYS_BUFDATA_FRAME:
            if (stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                u32RetSize = stBufInfo.stFrameData.u16Height * (stBufInfo.stFrameData.u32Stride[0] +  stBufInfo.stFrameData.u32Stride[1] / 2);
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
                u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] / 2);
            }
            else if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
            {
                u32RetSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], u32RetSize);
            }
            else
                ASSERT(0);
            break;
        default:
            ASSERT(0);
    }
    curtime[3] =  Mif_Syscfg_GetTime0();
    MI_SYS_ChnOutputPortPutBuf(stBufHandle);
    curtime[4] =  Mif_Syscfg_GetTime0();
#ifdef UVC_SUPPORT_LL
    if(bMJPG)
    {
        if((unsigned int)(stBufInfo.u64SidebandMsg) == 2)
        {
            *is_tail = 1;
            *u64LLInitTime = u64preInitTime;
            u64preInitTime = 0;
        }
        else
        {
            *is_tail = 0;
            *u64LLInitTime = stBufInfo.u64SidebandMsg;
            u64preInitTime = *u64LLInitTime;
        }
    }
    else
    {
        *is_tail = 1;
        *u64LLInitTime = stBufInfo.u64SidebandMsg;
    }
#else
    *is_tail = 1;
    *u64LLInitTime = stBufInfo.u64SidebandMsg;
#endif
    u32BufSize = u32RetSize;
    pthread_mutex_unlock(&_gTime);
#endif
    return u32RetSize;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_EnableVpe(ST_VPE_PortInfo_t *pVpePortInfo)
{

    ASSERT(pVpePortInfo);
    pVpePortInfo->DepVpeChannel = 0;
    pVpePortInfo->eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    STCHECKRESULT(ST_Vpe_CreatePort(MAIN_VENC_PORT, pVpePortInfo)); //default support port2 --->>> venc
#if (ENABLE_DUMPCIF_PORT1 == 1)
    ST_VPE_PortInfo_t stPortInfo;
    MI_SYS_ChnPort_t stChnPort;
    memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_t));
    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
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
    _gstModState.bEnableVpe = TRUE;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_DisableVpe(void)
{
    STCHECKRESULT(ST_Vpe_StopPort(0, MAIN_VENC_PORT)); //default support port2 --->>> venc
    _gstModState.bEnableVpe = FALSE;
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_EnableWarp(ST_Warp_Timming_e eWarpTimming)
{

    MI_SYS_ChnPort_t stChnPort;
    STCHECKRESULT(ST_Warp_Init());
    STCHECKRESULT(ST_Warp_SetChnBin(0, eWarpTimming));
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
    printf("####FUNC %s\n", __FUNCTION__);
    _gstModState.bEnableWarp = TRUE;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_DisableWarp(void)
{
    STCHECKRESULT(ST_Warp_DestroyChannel(0)); //default support port2 --->>> venc
    STCHECKRESULT(ST_Warp_Exit());
    printf("####FUNC %s\n", __FUNCTION__);
    _gstModState.bEnableWarp = FALSE;
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_EnableSd(ST_SD_PortInfo_t *pstPortInfo)
{
    MI_SYS_ChnPort_t stChnPort;
    ST_SD_ChannelInfo_t stSDChannelInfo;

    stSDChannelInfo.u32X = 0;
    stSDChannelInfo.u32Y = 0;
    stSDChannelInfo.u16SDCropW = 1920;
    stSDChannelInfo.u16SDCropH = 1080;
    stSDChannelInfo.u16SDMaxW = 1920;
    stSDChannelInfo.u16SDMaxH = 1080;
    STCHECKRESULT(ST_SD_CreateChannel(0, &stSDChannelInfo));

    ASSERT(pstPortInfo);
    pstPortInfo->DepSDChannel = 0;
    pstPortInfo->eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    pstPortInfo->ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    STCHECKRESULT(ST_SD_CreatePort(pstPortInfo));

    _gstModState.bEnableSd = TRUE;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_DisableSd(void)
{

    STCHECKRESULT(ST_SD_StopPort(0, 0));
    STCHECKRESULT(ST_SD_DestroyChannel(0));
    _gstModState.bEnableSd = FALSE;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5

static MI_S32 UVC_EnableVenc(MI_VENC_ChnAttr_t *pstChnAttr)
{
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnPort;
    MI_VENC_ParamJpeg_t stJpegPara;
    ASSERT(pstChnAttr);
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    s32Ret = MI_VENC_CreateChn(0, pstChnAttr);
    if(pstChnAttr->stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        memset(&stJpegPara, 0, sizeof(MI_VENC_ParamJpeg_t));
        MI_VENC_GetJpegParam(0, &stJpegPara);
        stJpegPara.u32Qfactor = 30;
        MI_VENC_SetJpegParam(0, &stJpegPara);
    }
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
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 5, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }
    s32Ret = MI_VENC_StartRecvPic(0);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }

    printf("####FUNC %s\n", __FUNCTION__);
    _gstModState.bEnableVenc = TRUE;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_DisableVenc(void)
{
    MI_S32 s32Ret = 0;
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
    _gstModState.bEnableVenc = FALSE;
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 _UVC_IsSupportNv12ToMjpg(void)
{
    MI_S32 s32Choose = 0;
    printf("Which flow do you want ?\n 0: YUY2 to MJPG, 1: NV12 to MJPG\n");
    scanf("%d", &s32Choose);
    printf("You select support %s to MJPG\n", s32Choose ? "YUY2" : "NV12");
    return s32Choose;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 _UVC_IsSupportAudio(void)
{
    MI_S32 s32Choose = 0;
    printf("Support audio with ALSA ?\n 0: NO, 1: YES\n");
    scanf("%d", &s32Choose);
    if ((s32Choose != 0) && (s32Choose != 1))
    {
        printf("You select choose %d, set to default 0\n", s32Choose);
        s32Choose = 0;
    }
    printf("You select %s audio\n", s32Choose ? "support" : "not support");
    return s32Choose;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static void UVC_EnableWindow(UVC_FormatTimming_e eFormatTimming)
{

    ST_SD_PortInfo_t stSdPortInfo;
    ST_VPE_PortInfo_t stVpePortInfo;
    ST_Sys_BindInfo_t stBindInfo;
    MI_S32 s32Ret = 0;
    MI_U32 u32FrameRate = 30;
    MI_BOOL bByframe = FALSE;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_VPE_PortMode_t stVpeMode;
    MI_SD_OuputPortAttr_t stSdOutAttr;

    printf("####FUNC %s\n", __FUNCTION__);
    printf("Timming! %d\n", eFormatTimming);
    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stSdOutAttr, 0, sizeof(MI_SD_OuputPortAttr_t));
    memset(&_gstVencSrcPort, 0, sizeof(MI_SYS_ChnPort_t));

    _gstVencSrcPort.eModId = E_MI_MODULE_ID_WARP;
    _gstVencSrcPort.u32DevId = 0;
    _gstVencSrcPort.u32ChnId = 0;
    _gstVencSrcPort.u32PortId = 0;

#ifdef UVC_SUPPORT_LL
    bByframe = FALSE;
#else
    bByframe = TRUE;
#endif
    switch(eFormatTimming)
    {
        case E_UVC_TIMMING_4K2K_JPG:
        {
            stVpePortInfo.u16OutputWidth = 3840;
            stVpePortInfo.u16OutputHeight = 2160;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 2160;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_JPG:
        {
            stVpePortInfo.u16OutputWidth = 2560;
            stVpePortInfo.u16OutputHeight = 1440;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 1440;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_JPG:
        {
            stVpePortInfo.u16OutputWidth = 1920;
            stVpePortInfo.u16OutputHeight = 1080;
            if (!_gs32Nv12ToMjpg)
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_1920_1080_NV16);
                stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
                stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
                stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                UVC_EnableSd(&stSdPortInfo);
                _gstVencSrcPort.eModId = E_MI_MODULE_ID_SD;
            }
            else
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_1920_1080_NV12);
            }
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 1080;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_JPG:
        {
            stVpePortInfo.u16OutputWidth = 1280;
            stVpePortInfo.u16OutputHeight = 720;
            if (!_gs32Nv12ToMjpg)
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_1280_720_NV16);
                stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
                stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
                stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                UVC_EnableSd(&stSdPortInfo);
                _gstVencSrcPort.eModId = E_MI_MODULE_ID_SD;
            }
            else
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_1280_720_NV12);
            }
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 720;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_JPG:
        {
            stVpePortInfo.u16OutputWidth = 640;
            stVpePortInfo.u16OutputHeight = 480;
            if (!_gs32Nv12ToMjpg)
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_640_480_NV16);
                stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
                stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
                stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                UVC_EnableSd(&stSdPortInfo);
                _gstVencSrcPort.eModId = E_MI_MODULE_ID_SD;
            }
            else
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_640_480_NV12);
            }
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 480;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_JPG:
        {
            stVpePortInfo.u16OutputWidth = 320;
            stVpePortInfo.u16OutputHeight = 240;
            if (!_gs32Nv12ToMjpg)
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_320_240_NV16);
                stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
                stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
                stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                UVC_EnableSd(&stSdPortInfo);
                _gstVencSrcPort.eModId = E_MI_MODULE_ID_SD;
            }
            else
            {
                stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                UVC_EnableVpe(&stVpePortInfo);
                UVC_EnableWarp(E_WARP_320_240_NV12);
            }
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 240;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = bByframe;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_4K2K_H264:
        {
            stVpePortInfo.u16OutputWidth = 3840;
            stVpePortInfo.u16OutputHeight = 2160;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 4 * 1024 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_H264:
        {
            stVpePortInfo.u16OutputWidth = 2560;
            stVpePortInfo.u16OutputHeight = 1440;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 2 * 1024 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_H264:
        {
            stVpePortInfo.u16OutputWidth = 1920;
            stVpePortInfo.u16OutputHeight = 1080;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1 * 1024 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_H264:
        {
            stVpePortInfo.u16OutputWidth = 1280;
            stVpePortInfo.u16OutputHeight = 720;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 720;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1 * 1024 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_H264:
        {
            stVpePortInfo.u16OutputWidth = 640;
            stVpePortInfo.u16OutputHeight = 480;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 480;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 500 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_H264:
        {
            stVpePortInfo.u16OutputWidth = 320;
            stVpePortInfo.u16OutputHeight = 240;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 240;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 500 * 1024;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_4K2K_H265:
        {
            stVpePortInfo.u16OutputWidth = 3840;
            stVpePortInfo.u16OutputHeight = 2160;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_3840_2160_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 3840;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 2160;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 2 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_2560X1440P_H265:
        {
            stVpePortInfo.u16OutputWidth = 2560;
            stVpePortInfo.u16OutputHeight = 1440;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_2560_1440_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 2560;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 1440;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 2 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080P_H265:
        {
            stVpePortInfo.u16OutputWidth = 1920;
            stVpePortInfo.u16OutputHeight = 1080;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1920;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 1080;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1280X720P_H265:
        {
            stVpePortInfo.u16OutputWidth = 1280;
            stVpePortInfo.u16OutputHeight = 720;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 720;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1280;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 720;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_640X480P_H265:
        {
            stVpePortInfo.u16OutputWidth = 640;
            stVpePortInfo.u16OutputHeight = 480;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 640;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 480;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 640;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 480;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 500 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_320X240P_H265:
        {
            stVpePortInfo.u16OutputWidth = 320;
            stVpePortInfo.u16OutputHeight = 240;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 320;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 240;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 320;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 240;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 500 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            UVC_EnableVenc(&stChnAttr);
        }
        break;
        case E_UVC_TIMMING_1920X1080_NV12:
        {
            stVpePortInfo.u16OutputWidth = 1920;
            stVpePortInfo.u16OutputHeight = 1080;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV12);
        }
        break;
        case E_UVC_TIMMING_1280X720_NV12:
        {
            stVpePortInfo.u16OutputWidth = 1280;
            stVpePortInfo.u16OutputHeight = 720;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV12);
        }
        break;
        case E_UVC_TIMMING_640X480_NV12:
        {
            stVpePortInfo.u16OutputWidth = 640;
            stVpePortInfo.u16OutputHeight = 480;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV12);
        }
        break;
        case E_UVC_TIMMING_320X240_NV12:
        {
            stVpePortInfo.u16OutputWidth = 320;
            stVpePortInfo.u16OutputHeight = 240;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV12);
        }
        break;
        case E_UVC_TIMMING_320X240_YUV422_YUYV:
        {
            stVpePortInfo.u16OutputWidth = 320;
            stVpePortInfo.u16OutputHeight = 240;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_320_240_NV16);
            stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
            stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
            stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            UVC_EnableSd(&stSdPortInfo);
        }
        break;
        case E_UVC_TIMMING_640X480_YUV422_YUYV:
        {
            stVpePortInfo.u16OutputWidth = 640;
            stVpePortInfo.u16OutputHeight = 480;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_640_480_NV16);
            stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
            stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
            stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            UVC_EnableSd(&stSdPortInfo);
        }
        break;
        case E_UVC_TIMMING_1280X720_YUV422_YUYV:
        {
            stVpePortInfo.u16OutputWidth = 1280;
            stVpePortInfo.u16OutputHeight = 720;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1280_720_NV16);
            stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
            stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
            stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            UVC_EnableSd(&stSdPortInfo);
        }
        break;
        case E_UVC_TIMMING_1920X1080_YUV422_YUYV:
        {
            stVpePortInfo.u16OutputWidth = 1920;
            stVpePortInfo.u16OutputHeight = 1080;
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_422;
            UVC_EnableVpe(&stVpePortInfo);
            UVC_EnableWarp(E_WARP_1920_1080_NV16);
            stSdPortInfo.u16OutputWidth = stVpePortInfo.u16OutputWidth;
            stSdPortInfo.u16OutputHeight = stVpePortInfo.u16OutputHeight;
            stSdPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            UVC_EnableSd(&stSdPortInfo);
        }
        break;
        default:
            printf("Format not support!\n");
            return;
    }
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static void UVC_DisableWindow(void)
{
    printf("####FUNC %s\n", __FUNCTION__);
    if (_gstModState.bEnableVenc)
    {
        UVC_DisableVenc();
    }
    if (_gstModState.bEnableSd)
    {
        UVC_DisableSd();
    }
    if(_gstModState.bEnableWarp)
    {
        UVC_DisableWarp();
    }
    if(_gstModState.bEnableVpe)
    {
        UVC_DisableVpe();
    }
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_EnableBindRelation(void)
{
    ST_Sys_BindInfo_t stBindInfo;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;

    ASSERT(_gstModState.bEnableVpe);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    if(_gstModState.bEnableWarp)
    {
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
#ifdef UVC_SUPPORT_LL
        if(bMJPG)
        {
            if(bFrameOrRealMode == 0)
                MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 19);
            else
                MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 13);
        }

#endif
    }
    if (_gstModState.bEnableCevaVx)
    {
    }
    else if (_gstModState.bEnableSd)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_WARP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = 0;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SD;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
        //MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort,&stBindInfo.stDstChnPort,TRUE,3);
#endif

    }
    if (_gstModState.bEnableVenc)
    {
        s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
        }
        stBindInfo.stSrcChnPort = _gstVencSrcPort;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
        if(bMJPG)
            MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 3);
#endif

    }
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_DisableBindRelation(void)
{
    ST_Sys_BindInfo_t stBindInfo;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));

    if (_gstModState.bEnableVenc)
    {
        s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
        }
        stBindInfo.stSrcChnPort = _gstVencSrcPort;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

#ifdef UVC_SUPPORT_LL
        MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, FALSE, 0);
#endif
    }
    if (_gstModState.bEnableCevaVx)
    {
    }
    else if (_gstModState.bEnableSd)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_WARP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = 0;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_SD;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
        //MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort,&stBindInfo.stDstChnPort,FALSE,0);
#endif

    }
    if(_gstModState.bEnableWarp)
    {
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

#ifdef UVC_SUPPORT_LL
        MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, FALSE, 0);
#endif

    }
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_Init(void *uvc)
{
    return MI_SUCCESS;
}

static MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_UP_FinishBUffer(int handle)
{
#if 1   //for user alloc buf
    userptr_mem_buf_complete(handle);
#else
    MI_SYS_ChnOutputPortPutBuf(handle);
#endif
    curtime[4] =  Mif_Syscfg_GetTime0();
    pthread_mutex_unlock(&_gTime);
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_UP_FillBuffer(void *uvc, ST_UVC_BufInfo_t *bufInfo, int *handle)
{
#ifndef SUPPORT_WRITE_FILE
#if 1   //for user alloc buf
    MI_U64 u64LLInitTime;
    *handle = userptr_mem_get_buf();
    if(*handle == 0)
        return -1;

    bufInfo->b.start = *handle;
    bufInfo->length = ST_DoGetData_mmap(&gstChnPort, (void*)(bufInfo->b.start), &(bufInfo->is_tail), &u64LLInitTime);
#else
    bufInfo->length = ST_DoGetData_userptr(&gstChnPort,
                                           &bufInfo->b.start, handle, &(bufInfo->is_tail));
#endif
#endif
    if(bufInfo->length == 0)
        return -1;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_MM_FillBuffer(void *uvc, ST_UVC_BufInfo_t *bufInfo)
{
#ifndef SUPPORT_WRITE_FILE
    MI_U64 u64LLInitTime = 0;
    bufInfo->length = ST_DoGetData_mmap(&gstChnPort, bufInfo->b.buf, &(bufInfo->is_tail), &u64LLInitTime);
    if(u64LLInitTime)
    {
        bufInfo->tv.tv_sec = u64LLInitTime / 1000000;
        bufInfo->tv.tv_usec = u64LLInitTime % 1000000;
    }
    //gettimeofday(&bufInfo->tv,NULL);
#endif
    if(bufInfo->length == 0)
        return -1;

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 UVC_StartCapture(void *uvc, Stream_Params_t format)
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
    rd_fd_size = open("./venc_data_size", O_RDONLY);
    if (wd_fd_size < 0)
    {
        perror("open");
        return;
    }
#endif
#endif
    printf("Fmt %s width %d height %d\n",
           uvc_get_format(format.fcc), format.width, format.height);
#ifndef UVC_SUPPORT_MMAP
    userptr_mem_init(((ST_UVC_Device_t*)uvc)->ChnAttr.setting.nbuf,
                     format.maxframesize);
#endif
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
            bMJPG = TRUE;
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
        case V4L2_PIX_FMT_YUYV:
        {
            if (format.width == 320 && format.height == 240)
            {
                UVC_EnableWindow(E_UVC_TIMMING_320X240_YUV422_YUYV);
            }
            else if (format.width == 640 && format.height == 480)
            {
                UVC_EnableWindow(E_UVC_TIMMING_640X480_YUV422_YUYV);
            }
            else if (format.width == 1280 && format.height == 720)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1280X720_YUV422_YUYV);
            }
            else if (format.width == 1920 && format.height == 1080)
            {
                UVC_EnableWindow(E_UVC_TIMMING_1920X1080_YUV422_YUYV);
            }
            else
            {
                printf("Yuv 422 not support format! width %d height %d\n", format.width, format.height);
                return -1;
            }
            gstChnPort.eModId = E_MI_MODULE_ID_SD;
            gstChnPort.u32DevId = 0;
            gstChnPort.u32ChnId = 0;
            gstChnPort.u32PortId = 0;
        }
        break;
        default:
            printf("not support format!\n");
            return -1;
    }
    UVC_EnableBindRelation();
    if(MI_SUCCESS != MI_SYS_GetFd(&gstChnPort, (MI_S32 *)&gpfd[0].fd))
    {
        printf("MI_SYS_GetFd fail\n");
    }
#ifdef SUPPORT_WRITE_FILE
    ST_TemCreate();
#endif

    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
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
    UVC_DisableBindRelation();
    UVC_DisableWindow();
    bMJPG = FALSE;
#ifndef UVC_SUPPORT_MMAP
    userptr_mem_exit();
#endif
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
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UvcInit()
{
    ST_UVC_Setting_t pstSet = {4, UVC_MEMORY_MODE, USB_ISOC_MODE};
    ST_UVC_OPS_t fops = { UVC_Init,
                          UVC_Deinit,
#ifdef UVC_SUPPORT_MMAP
                          .m = UVC_MM_FillBuffer,
#else
                          .u = {UVC_UP_FillBuffer, UVC_UP_FinishBUffer},
#endif
                          UVC_StartCapture,
                          UVC_StopCapture
                        };
    ST_UVC_ChnAttr_t pstAttr = {pstSet, fops};
    STCHECKRESULT(ST_UVC_Init("/dev/video0"));
    STCHECKRESULT(ST_UVC_CreateDev(&pstAttr));
    STCHECKRESULT(ST_UVC_StartDev());
    return MI_SUCCESS;
}
#endif

static MI_S32 St_UvcDeinit()
{
#if (UVC_FUNCTION_EN)  //infinity5
    STCHECKRESULT(ST_UVC_StopDev());
    STCHECKRESULT(ST_UVC_DestroyDev(0));
    STCHECKRESULT(ST_UVC_Uninit());
#endif
    return MI_SUCCESS;
}

#if (UVC_FUNCTION_EN)  //infinity5
static void *St_UacSendFrameWork()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1, UacDevId = ALSA_CAPTURE_DEV;
    MI_AI_CHN AiChn = 0;
    MI_ALSA_CHN UacChn   = 0;
    MI_AUDIO_Frame_t stAiFrm;
    MI_ALSA_Frame_t dtUacFrm;
    while(false == _gUacSendFrameWorkExit)
    {
        s32Ret = MI_AI_GetFrame(AiDevId, AiChn, &stAiFrm, NULL, 16);//256 / 16000 = 16ms
        if (s32Ret != MI_SUCCESS)
        {
            continue;
        }
        dtUacFrm.u32Len = stAiFrm.u32Len;
        dtUacFrm.pu8Addr = stAiFrm.apVirAddr[0];
        dtUacFrm.u64PTS = stAiFrm.u64TimeStamp;
        MI_ALSA_SendFrame(UacChn, &dtUacFrm, 0);
        MI_AI_ReleaseFrame(AiDevId,  AiChn, &stAiFrm, NULL);
    }
    return NULL;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UacCaptureInit(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1, UacDevId = ALSA_CAPTURE_DEV;
    MI_AUDIO_Attr_t stAiAttr;
    MI_AI_CHN AiChn = 0;
    MI_U32 UacChn   = 0;
    MI_SYS_ChnPort_t stAiPort;
    MI_U32 u32FrameDepth = 12;
    MI_U32 u32BuffQueue = 13;

    /*ALSA Init*/
    ExecFunc(MI_ALSA_OpenDevice(UacDevId), MI_SUCCESS);
    /*AI PARAM*/
    memset(&stAiAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAiAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAiAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAiAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAiAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAiAttr.u32ChnCnt = 2;
    stAiAttr.u32FrmNum = 16;
    stAiAttr.u32PtNumPerFrm = 256;

    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChn(AiDevId, AiChn), MI_SUCCESS);
    /*AI Depth*/
    memset(&stAiPort, 0, sizeof(MI_SYS_ChnPort_t));
    stAiPort.eModId = E_MI_MODULE_ID_AI;
    stAiPort.u32ChnId = AiChn;
    stAiPort.u32DevId = AiDevId;
    stAiPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAiPort, u32FrameDepth, u32BuffQueue), MI_SUCCESS);

    /* Get A Buf And Send To Uac Capture Device */
    _gUacSendFrameWorkExit = false;
    ExecFunc(MI_ALSA_StartDev(UacDevId), MI_SUCCESS);
    ExecFunc(pthread_create(&_gUacSendFrameThread, NULL, St_UacSendFrameWork, NULL), MI_SUCCESS);
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UacCaptureDeinit(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1, UacDevId = ALSA_CAPTURE_DEV;
    MI_AI_CHN AiChn = 0;

    _gUacSendFrameWorkExit = true;
    ExecFunc(pthread_join(_gUacSendFrameThread, NULL), MI_SUCCESS);
    ExecFunc(MI_ALSA_StopDev(UacDevId), MI_SUCCESS);
    ExecFunc(MI_ALSA_CloseDevice(UacDevId), MI_SUCCESS);
    ExecFunc(MI_AI_DisableChn(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UacPlaybackInit(void)
{
    MI_AUDIO_Attr_t stAoAttr;
    ST_Sys_BindInfo_t stBindInfo;

    /************************************************
    Step1:  init AO
    *************************************************/
    memset(&stAoAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAoAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAoAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAoAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAoAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoAttr.u32ChnCnt = 1;
    stAoAttr.u32FrmNum = 6;
    stAoAttr.u32PtNumPerFrm = 800;
    ExecFunc(MI_AO_SetPubAttr(0, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_GetPubAttr(0, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(0), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(0, 0), MI_SUCCESS);

    /************************************************
    Step2:  init ALSA
    *************************************************/
    ExecFunc(MI_ALSA_OpenDevice(ALSA_PLAYBACK_DEV), MI_SUCCESS);

    /************************************************
    Step4:  bind ALSA playback to A0
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ALSA;
    stBindInfo.stSrcChnPort.u32DevId = ALSA_PLAYBACK_DEV;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stSrcChnPort, 3, 6), MI_SUCCESS);

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_AO;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    /************************************************
    Step4:  start ALSA playback DEV
    *************************************************/
    ExecFunc(MI_ALSA_StartDev(ALSA_PLAYBACK_DEV), MI_SUCCESS);
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UacPlaybackDeinit(void)
{
    ST_Sys_BindInfo_t stBindInfo;

    ExecFunc(MI_ALSA_StopDev(ALSA_PLAYBACK_DEV), MI_SUCCESS);

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_ALSA;
    stBindInfo.stSrcChnPort.u32DevId = ALSA_PLAYBACK_DEV;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_AO;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    ExecFunc(MI_AO_DisableChn(0, 0), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(0), MI_SUCCESS);

    ExecFunc(MI_ALSA_CloseDevice(ALSA_PLAYBACK_DEV), MI_SUCCESS);
    return MI_SUCCESS;
}
#endif

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_UacInit(MI_S32 s32CaptureFlag, MI_S32 s32PlaybackFlag)
{

    MI_S32 s32Ret = MI_SUCCESS;
    /************************************************
    Step1:  init ALSA
    *************************************************/
    if (s32CaptureFlag == 1)
    {
        s32Ret = St_UacCaptureInit();
    }

    if (s32PlaybackFlag == 1)
    {
        s32Ret = St_UacPlaybackInit();
    }
    return s32Ret;
}
#endif

static MI_S32 St_UacDeinit(MI_S32 s32CaptureFlag, MI_S32 s32PlaybackFlag)
{
#if (UVC_FUNCTION_EN)  //infinity5
    if (s32CaptureFlag == 1)
    {
        St_UacCaptureDeinit();
    }

    if (s32PlaybackFlag == 1)
    {
        St_UacPlaybackDeinit();
    }
#endif
    return MI_SUCCESS;
}

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_DisplayInit(void)
{
    ST_VPE_PortInfo_t stPortInfo;
    ST_Sys_BindInfo_t stBindInfo;
    ST_DispChnInfo_t stDispChnInfo;
    memset(&stPortInfo, 0x0, sizeof(ST_VPE_PortInfo_t));
    memset(&stDispChnInfo, 0x0, sizeof(ST_DispChnInfo_t));
    stPortInfo.DepVpeChannel = 0;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stPortInfo.u16OutputWidth = 1920;
    stPortInfo.u16OutputHeight = 1080;
    STCHECKRESULT(ST_Vpe_CreatePort(DISP_PORT, &stPortInfo)); //default support port0 --->>> vdisp
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, E_MI_DISP_OUTPUT_1080P60)); //Dispout timing
    stDispChnInfo.InputPortNum = 1;
    stDispChnInfo.stInputPortAttr[0].u32Port = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width = 1920;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height = 1080;
    STCHECKRESULT(ST_Disp_ChnInit(0, &stDispChnInfo));
    /************************************************
    Step8:  Bind VPE->VDISP
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;

    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
    if(bFrameOrRealMode == 0)
        MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 15);
    else
        MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 10);
#endif

    STCHECKRESULT(ST_Hdmi_Init());
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, E_MI_HDMI_TIMING_1080_60P)); //Hdmi timing
    return MI_SUCCESS;
}
#endif

static MI_S32 St_DisplayDeinit(void)
{
#if (UVC_FUNCTION_EN)  //infinity5
    ST_VPE_PortInfo_t stPortInfo;
    ST_Sys_BindInfo_t stBindInfo;
    ST_DispChnInfo_t stDispChnInfo;
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
    MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, FALSE, 0);
#endif

    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, ST_DISP_LAYER0, 1));
    STCHECKRESULT(ST_Vpe_StopPort(0, DISP_PORT));
#endif
    return MI_SUCCESS;
}

#if (UVC_FUNCTION_EN)  //infinity5
static MI_S32 St_BaseModuleInit(MI_S32 s32RunMode)
{
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    MI_SYS_ChnPort_t stChnPort;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_Sys_BindInfo_t stBindInfo;
    VIF_AD_WORK_MODE_E eVifWorkMode;
    MI_VPE_RunningMode_e eVpeRunningMode;
    MI_SYS_PixelFormat_e ePixelFormat;
    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVpeChannelInfo, 0x0, sizeof(ST_VPE_ChannelInfo_t));
    STCHECKRESULT(ST_Sys_Init());
    switch (s32RunMode)
    {
        case 0:
        {
            eVifWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME;
            eVpeRunningMode = E_MI_VPE_RUNNING_MODE_REALTIME_MODE;
            ePixelFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
            stVpeChannelInfo.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
        }
        break;
        case 1:
        {
            eVifWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_FRAME;
            eVpeRunningMode = E_MI_VPE_RUNNING_MODE_FRAMEBUF_CAM_MODE;
            ePixelFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
            stVpeChannelInfo.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
        }
        break;
        case 2:
        {
            eVifWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME_HDR;
            eVpeRunningMode = E_MI_VPE_RUNNING_MODE_REALTIME_MODE;
            ePixelFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
            stVpeChannelInfo.eHDRType = E_MI_VPE_HDR_TYPE_DOL;
        }
        break;
        default:
        {
            printf("vif workmode %d error \n", s32RunMode);
            return 1;
        }
    }
    STCHECKRESULT(ST_Vif_CreateDev(0, eVifWorkMode));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 3840;
    stVifPortInfoInfo.u32RectHeight = 2160;
    stVifPortInfoInfo.u32DestWidth = 3840;
    stVifPortInfoInfo.u32DestHeight = 2160;
    stVifPortInfoInfo.ePixFormat = ePixelFormat;
#ifdef UVC_SUPPORT_LL
    if(eVifWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_FRAME)
    {
        stVifPortInfoInfo.u32FrameModeLineCount = 10;
    }
#endif
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
    stVpeChannelInfo.eRunningMode = eVpeRunningMode;
    stVpeChannelInfo.eFormat = ePixelFormat;
    stVpeChannelInfo.eBindSensorID = E_MI_VPE_SENSOR0;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));
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
#ifdef UVC_SUPPORT_LL
    if(eVifWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_FRAME)
    {
        MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, TRUE, 8);
    }
#endif
    return MI_SUCCESS;
}
#endif

static MI_S32 St_BaseModuleDeinit(void)
{
#if (UVC_FUNCTION_EN)  //infinity5
    ST_Sys_BindInfo_t stBindInfo;
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
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
#ifdef UVC_SUPPORT_LL
    MI_SYS_BindChnPort_SWLowLatency(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, FALSE, 0);
#endif

    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));
    STCHECKRESULT(ST_Sys_Exit());
#endif
    return MI_SUCCESS;
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
        pData = pDataFrame + line_offset * i;
        yuvSize = line_size;
        do
        {
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
        }
        while (yuvSize > 0);
    }
    return 0;
}
#endif

unsigned int poll_isp(int fd, int timeout)
{
    struct pollfd pfd;
    int ret = 0;
    unsigned int flag = 0;

    pfd.fd = fd;
    pfd.events = POLLIN/* | POLLRDNORM*/;

    ret = poll(&pfd, 1, timeout);
    if(ret > 0)
    {
        read(fd, (void*)&flag, sizeof(flag));
    }
    return flag;
}

static void test_mi_api()
{
    FILE *fd;
    MI_ISP_AE_HW_STATISTICS_t *pAe;
    MI_ISP_AWB_HW_STATISTICS_t *pAwb;
    MI_ISP_HISTO_HW_STATISTICS_t *pHisto;
    Cus3AEnable_t *pCus3AEnable;
    CusAEInitParam_t *pAeInitParam;
    CusAEInfo_t *pAeInfo;
    CusAEResult_t *pAeResult;
    CusAWBInfo_t *pAwbInfo;
    CusAWBResult_t *pAwbResult;

    /*AE avg statistics*/
    pAe = malloc(sizeof(MI_ISP_AE_HW_STATISTICS_t));
    MI_ISP_AE_GetAeHwAvgStats(0, pAe);
    fd = fopen("ae.bin", "w+b");
    fwrite(pAe->nAvg, sizeof(pAe->nAvg), 1, fd);
    fclose(fd);
    printf("Save ae data to file ae.bin\n");
    free(pAe);

    /*AWB avg statistics*/
    pAwb = malloc(sizeof(MI_ISP_AWB_HW_STATISTICS_t));
    MI_ISP_AWB_GetAwbHwAvgStats(0, pAwb);
    fd = fopen("awb.bin", "w+b");
    fwrite(pAwb->nAvg, sizeof(pAwb->nAvg), 1, fd);
    fclose(fd);
    printf("Save awb data to file ae.bin\n");
    free(pAwb);

    /*Histo0 avg statistics*/
    pHisto = malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto0HwStats(0, pHisto);
    fd = fopen("histo0.bin", "w+b");
    fwrite(pHisto->nHisto, sizeof(pHisto->nHisto), 1, fd);
    fclose(fd);
    printf("Save histo0 data to file ae.bin\n");
    free(pHisto);

    /*Histo1 avg statistics*/
    pHisto = malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto1HwStats(0, pHisto);
    fd = fopen("histo1.bin", "w+b");
    fwrite(pHisto->nHisto, sizeof(pHisto->nHisto), 1, fd);
    fclose(fd);
    printf("Save histo1 data to file ae.bin\n");
    free(pHisto);

    /*Check CUS3A*/
    pCus3AEnable = malloc(sizeof(Cus3AEnable_t));
    pCus3AEnable->bAE = 1;
    pCus3AEnable->bAWB = 1;
    pCus3AEnable->bAF = 0;
    MI_ISP_CUS3A_Enable(0, pCus3AEnable);
    free(pCus3AEnable);

    /*Check AE init param*/
    pAeInitParam = malloc(sizeof(CusAEInitParam_t));
    memset(pAeInitParam, 0, sizeof(CusAEInitParam_t));
    MI_ISP_CUS3A_GetAeInitStatus(0, pAeInitParam);
    printf("AeInitParam ,shutter=%d,shutter_step=%d,sensor_gain_min=%d,sensor_gain_max=%d\n",
           pAeInitParam->shutter,
           pAeInitParam->shutter_step,
           pAeInitParam->sensor_gain_min,
           pAeInitParam->sensor_gain_max
          );
    free(pAeInitParam);

    /*Check AE param*/
    pAeInfo = malloc(sizeof(CusAEInfo_t));
    memset(pAeInfo, 0, sizeof(CusAEInfo_t));
    MI_ISP_CUS3A_GetAeStatus(0, pAeInfo);
    printf("AeInitParam ,Shutter=%d,SensirGain=%d,IspGain=%d\n",
           pAeInfo->Shutter,
           pAeInfo->SensorGain,
           pAeInfo->IspGain
          );
    free(pAeInfo);

    /*Check AWB param*/
    pAwbInfo = malloc(sizeof(CusAWBInfo_t));
    memset(pAwbInfo, 0, sizeof(CusAWBInfo_t));
    MI_ISP_CUS3A_GetAwbStatus(0, pAwbInfo);
    printf("AwbInitParam ,Rgain=%d,Ggain=%d,Bgain=%d\n",
           pAwbInfo->CurRGain,
           pAwbInfo->CurGGain,
           pAwbInfo->CurBGain
          );
    free(pAwbInfo);

    /*Set AWB param*/
    pAwbResult = malloc(sizeof(CusAWBResult_t));
    memset(pAwbResult, 0, sizeof(CusAWBResult_t));
    pAwbResult->Size = sizeof(CusAWBResult_t);
    pAwbResult->R_gain = 4096;
    pAwbResult->G_gain = 1024;
    pAwbResult->B_gain = 1024;
    MI_ISP_CUS3A_SetAwbParam(0, pAwbResult);
    free(pAwbResult);

#if 1
    int isp_fe = open("/dev/isp_fe", O_RDWR);
    int n = 0;
    /*Check AE init param*/
    pAeResult = malloc(sizeof(CusAEResult_t));
    for(n = 0; n < 120; ++n)
    {
        unsigned int status = poll_isp(isp_fe, 500);
        //printf("ISP CH%d FE\n", status);
        memset(pAeResult, 0, sizeof(CusAEResult_t));
        pAeResult->Size = sizeof(CusAEResult_t);
        pAeResult->Change = 1;
        pAeResult->u4BVx16384 = 16384;
        pAeResult->HdrRatio = 10;
        pAeResult->ShutterHdrShort = 300;
        pAeResult->Shutter = (300 * n) % 30000;
        pAeResult->IspGain = 2048;
        pAeResult->SensorGain = 1024;
        pAeResult->SensorGainHdrShort = 4096;
        pAeResult->IspGainHdrShort = 1024;
        pAeResult->AvgY = 128;
        MI_ISP_CUS3A_SetAeParam(0, pAeResult);
    }
    free(pAeResult);
    close(isp_fe);
#endif
    pCus3AEnable = malloc(sizeof(Cus3AEnable_t));
    pCus3AEnable->bAE = 0;
    pCus3AEnable->bAWB = 0;
    pCus3AEnable->bAF = 0;
    MI_ISP_CUS3A_Enable(0, pCus3AEnable);
    free(pCus3AEnable);

    /*Unregister api agent*/
    //MI_ISP_RegisterIspApiAgent(0, NULL, NULL);
}

//*****************************************************************************************************/
// Customer 3A
//*****************************************************************************************************/
#define USING_NEW_AE_ALGO    (1)    // 0: using simple AE algorithm, 1: using complex AE algorithm
#define USING_NEW_AWB_ALGO    (1)    // 0: using simple AE algorithm, 1: using complex AE algorithm
#define AE_RUN_WITH_USE_CASE_1    (0)  // Implement ae algorithm function only in ae_run() callback function, don't need to create new thread
#define AE_RUN_WITH_USE_CASE_2    (1)  // Create new thread to run ae algorithm, get AvgY and get result, then send result to ae_run() callback function
#define AE_RUN_WITH_USE_CASE_3    (2)  // Create new thread to run ae algorithm, get AvgY, get result and set AE result in same thread, don't need to register ae_run() callback function
#define AE_RUN_WITH_USE_CASE    (AE_RUN_WITH_USE_CASE_1)

#define ENABLE_DOAE_MSG        (1)
#define ENABLE_DOAWB_MSG    (0)
#define ENABLE_DOAF_MSG        (0)

#define SHUTTER_GAIN_DELAY_TEST1   (0)
#define SHUTTER_GAIN_DELAY_TEST2   (0)
#define SHUTTER_TEST                               (0)
#define GAIN_TEST                                      (0)
#define AE_EXAMPLE                                   (1)

typedef struct
{
    AeHandle    hAe;
    AeOutput_t  tAeOutput;
    AEDEBUG_t   tAeDbg;
} CusAeInfo_t;

typedef struct
{
    AwbHandle   hAwb;
    AwbOutput_t tAwbOutput;
    AWBDEBUG_t  tAwbDbg;
} CusAwbInfo_t;

typedef struct
{
    CusAeInfo_t tCusAeInfo;
    CusAwbInfo_t tCusAwbInfo;
} Cus3A_Priv_Data_t;

Cus3A_Priv_Data_t g_tCus3APrivData[MS_CAM_AF_MAX_WIN_NUM];
int ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    AeInitParam_t tAeInit;
    AeInput_t tAeInput;

    if(pdata == NULL)
    {
        printf("ae_init error!!! NULL Pointer!!!\n");
        return -1;
    }
    printf("[ae_init] isp_gain_max = 0x%x, sgl_min = 0x%x, sgl_max = 0x%x, sgs_min  0x%x, sgs_max = 0x%x\n",
           (int)init_state->isp_gain_max,
           (int)init_state->sensor_gain_min,
           (int)init_state->sensor_gain_max,
           (int)init_state->sensor_gainHDRShort_min,
           (int)init_state->sensor_gainHDRShort_max
          );
    printf("[ae_init], shl_min = 0x%x, shl_max = 0x%x, shl_step = 0x%x, shs_min = 0x%x, shs_max = 0x%x, shs_step = 0x%x\n",
           (int)init_state->shutter_min,
           (int)init_state->shutter_max,
           (int)init_state->shutter_step,
           (int)init_state->shutterHDRShort_min,
           (int)init_state->shutterHDRShort_max,
           (int)init_state->shutterHDRShort_step
          );
#if (USING_NEW_AE_ALGO)  // new AE algo

    pAe = &pstCus3a->tCusAeInfo;
    tAeInit.FNx10           = init_state->FNx10;
    tAeInit.Size            = sizeof(AeInitParam_t);
    tAeInit.fps             = init_state->fps;
    tAeInit.isp_gain        = init_state->isp_gain;
    tAeInit.isp_gain_max    = init_state->isp_gain_max;

    tAeInit.sensor_gain     = init_state->sensor_gain;
    tAeInit.sensor_gain_min = 1 * 1024;          // 1024 * 1
    tAeInit.sensor_gain_max = init_state->sensor_gain_max;          // 1024 * 177
    tAeInit.sensor_id[0]    = init_state->sensor_id[0];
    tAeInit.shutter         = init_state->shutter;
    tAeInit.shutter_min     = init_state->shutter_min;    // 14814 * 2  //conversion driver's ns to AE's us
    tAeInit.shutter_max     = init_state->shutter_max;    // 1000000000 / 3   //conversion driver's ns to AE's us
    tAeInit.shutter_step    = init_state->shutter_step;     // 14814   //conversion driver's ns to AE's ns

    // Init AE DEBUG Setting.
    pAe->tAeDbg.AE_DEBUG_ENABLE_AE = 1;
    pAe->tAeDbg.AE_DEBUG_LEVEL = 0;
    tAeInput.tAEDebug = &pAe->tAeDbg;

    // Init. AE Output parameters.
    pAe->tAeOutput.FNx10         = 18;
    pAe->tAeOutput.CurYx10       = 100;
    pAe->tAeOutput.Shutter       = 300;
    pAe->tAeOutput.SensorGain    = 1024;
    pAe->tAeOutput.IspGain       = 1024;
    pAe->tAeOutput.ShutterHDR    = 30;
    pAe->tAeOutput.SensorGainHDR = 1024;
    pAe->tAeOutput.IspGainHDR    = 1024;

    pstCus3a->tCusAeInfo.hAe = AeInit(NULL, &tAeInit, &tAeInput, &pAe->tAeOutput);

#endif
    return 0;
}

void ae_release(void* pdata)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;

    if(pdata == NULL)
    {
        printf("ae_release error!!! NULL Pointer!!!\n");
        return;
    }

#if (USING_NEW_AE_ALGO)  // new AE algo
    AeRelease(pstCus3a->tCusAeInfo.hAe);
#endif
    printf("************* ae_release *************\n");
}

#if (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_1)
void ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    AeInput_t  tAeInput;
    AeInput_t *ptAeInput;
    static int period = 3;
    static unsigned int fcount = 0;

#if (USING_NEW_AE_ALGO)  // new AE algo

    if(pdata == NULL)
    {
        printf("ae_run error!!! NULL Pointer!!!\n");
        return;
    }

    pAe = &pstCus3a->tCusAeInfo;
    result->Size              = sizeof(ISP_AE_RESULT);
    result->Change             = 0;
    result->u4BVx16384         = pAe->tAeOutput.i4BVx16384;
    result->HdrRatio           = 0;      /*TODO: ??*/
    result->Shutter            = pAe->tAeOutput.Shutter;
    result->SensorGain         = pAe->tAeOutput.SensorGain;
    result->IspGain            = pAe->tAeOutput.IspGain;
    result->ShutterHdrShort    = pAe->tAeOutput.ShutterHDR;
    result->SensorGainHdrShort = pAe->tAeOutput.SensorGainHDR;
    result->IspGainHdrShort    = pAe->tAeOutput.IspGainHDR;
    result->AvgY               = pAe->tAeOutput.CurYx10 / 10;

    if(++fcount % period == 0)
    {
        ptAeInput = &tAeInput;
        ptAeInput->AvgBlkX        = info->AvgBlkX;
        ptAeInput->AvgBlkY        = info->AvgBlkY;
        ptAeInput->FNx10          = 18;
        ptAeInput->PreCurYx10     = 100;
        ptAeInput->Shutter        = info->Shutter;
        ptAeInput->SensorGain     = info->SensorGain;
        ptAeInput->IspGain        = info->IspGain;
        ptAeInput->ShutterHDR     = info->ShutterHDRShort;
        ptAeInput->SensorGainHDR  = info->SensorGainHDRShort;
        ptAeInput->IspGainHDR     = info->IspGainHDRShort;

        ptAeInput->Size           = sizeof(AeInput_t);
        ptAeInput->avgs           = (ISPAESample_t *)info->avgs;
        ptAeInput->avgs1          = NULL;
        ptAeInput->hist1          = (ISPHistX_t *)info->hist1;
        ptAeInput->hist2          = (ISPHistX_t *)info->hist2;
        ptAeInput->tAEDebug       = &pAe->tAeDbg;

        /*Customer AE*/
        DoAe(pAe->hAe, ptAeInput, &pAe->tAeOutput);

        result->Size         = sizeof(ISP_AE_RESULT);
        result->Change       = pAe->tAeOutput.Change;
        result->u4BVx16384   = pAe->tAeOutput.i4BVx16384;
        result->HdrRatio     = 0;      /*TODO: ??*/
        result->ShutterHdrShort = pAe->tAeOutput.ShutterHDR;
        result->Shutter      = pAe->tAeOutput.Shutter;
        result->IspGain      = pAe->tAeOutput.IspGain;
        result->SensorGain   = pAe->tAeOutput.SensorGain;
        result->SensorGainHdrShort = pAe->tAeOutput.SensorGainHDR;
        result->IspGainHdrShort = pAe->tAeOutput.IspGainHDR;
        result->AvgY         = pAe->tAeOutput.CurYx10 / 10;

#if (ENABLE_DOAE_MSG)
        printf("DoAE--> Change:%02u, AvgY:%06u, u4BVx16384:%06d, IspGain:%06u, sensor gain:%06u, Shutter:%06u\n",
               (int)result->Change,
               (int)result->AvgY,
               (int)result->u4BVx16384,
               (int)result->IspGain,
               (int)result->SensorGain,
               (int)result->Shutter);
#endif
    }

#else  // #if (USING_NEW_AE_ALGO == 0)

    // Only one can be chosen (the following three define)
#define shutter_gain_delay_test 0
#define shutter_test 0
#define gain_test 0
#define AE_sample 1

#if (shutter_test) || (gain_test)
#endif
    unsigned int max = info->AvgBlkY * info->AvgBlkX;
    unsigned int avg = 0;
    unsigned int n;
#if gain_test
    static int tmp = 0;
    static int tmp1 = 0;
#endif

    result->Change              = 0;
    result->u4BVx16384          = 16384;
    result->HdrRatio            = 10; //infinity5 //TBD //10 * 1024;   //user define hdr exposure ratio
    result->IspGain             = 1024;
    result->SensorGain          = 4096;
    result->Shutter             = 20000;
    result->IspGainHdrShort     = 1024;
    result->SensorGainHdrShort  = 1024;
    result->ShutterHdrShort     = 1000;
    //result->Size         = sizeof(CusAEResult_t);

    for(n = 0; n < max; ++n)
    {
        avg += info->avgs[n].y;
    }
    avg /= max;

    result->AvgY         = avg;

#if shutter_gain_delay_test // shutter gain delay test
{
    static int tmp = 0;

    printf("AvgLum = 0x%X \n", avg);
    if(++fcount % 60 == 0)
    {
        if ((++tmp % 2) == 0)
        {
            result->Shutter = 30000;
            result->SensorGain = 4096;
            result->IspGain             = 1024;
        }
        else
        {
            result->Shutter = 10000;
            result->SensorGain = 4096;
            result->IspGain             = 1024;
        }
        result->Change = 1;
        printf("===== Shutter = %d\n", result->Shutter);
    }
}
#endif

#if shutter_test // shutter test under constant sensor gain
    int Shutter_Step = 100; //per frame
    int Shutter_Max = 33333;
    int Shutter_Min = 150;
    int Gain_Constant = 10240;

    result->SensorGain = Gain_Constant;
    result->Shutter = info->Shutter;

    if(++fcount % AE_period == 0)
    {
        if (tmp == 0)
        {
            result->Shutter = info->Shutter + Shutter_Step * AE_period;
            //printf("[shutter-up] result->Shutter = %d \n", result->SensorGain);
        }
        else
        {
            result->Shutter = info->Shutter - Shutter_Step * AE_period;
            //printf("[shutter-down] result->Shutter = %d \n", result->SensorGain);
        }
        if (result->Shutter >= Shutter_Max)
        {
            result->Shutter = Shutter_Max;
            tmp = 1;
        }
        if (result->Shutter <= Shutter_Min)
        {
            result->Shutter = Shutter_Min;
            tmp = 0;
        }
    }
#if (ENABLE_DOAE_MSG)
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("tmp = %d, Shutter: %d -> %d \n", tmp, info->Shutter, result->Shutter);
#endif
#endif

#if gain_test // gain test under constant shutter
    int Gain_Step = 1024; //per frame
    int Gain_Max = 1024 * 100;
    int Gain_Min = 1024 * 2;
    int Shutter_Constant = 20000;

    result->SensorGain = info->SensorGain;
    result->Shutter = Shutter_Constant;

    if(++fcount % AE_period == 0)
    {
        if (tmp1 == 0)
        {
            result->SensorGain = info->SensorGain + Gain_Step * AE_period;
            //printf("[gain-up] result->SensorGain = %d \n", result->SensorGain);
        }
        else
        {
            result->SensorGain = info->SensorGain - Gain_Step * AE_period;
            //printf("[gain-down] result->SensorGain = %d \n", result->SensorGain);
        }
        if (result->SensorGain >= Gain_Max)
        {
            result->SensorGain = Gain_Max;
            tmp1 = 1;
        }
        if (result->SensorGain <= Gain_Min)
        {
            result->SensorGain = Gain_Min;
            tmp1 = 0;
        }
    }
#if (ENABLE_DOAE_MSG)
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("tmp = %d, SensorGain: %d -> %d \n", tmp, info->SensorGain, result->SensorGain);
#endif
#endif

#if AE_sample
    int y_lower = 0x28;
    int y_upper = 0x38;
    int change_ratio = 10; // percentage
    int Gain_Min = 1024 * 2;
    int Gain_Max = 1024 * 1000;
    int Shutter_Min = 150;
    int Shutter_Max = 33333;

    result->SensorGain = info->SensorGain;
    result->Shutter = info->Shutter;

    if(avg < y_lower)
    {
        if (info->Shutter < Shutter_Max)
        {
            result->Shutter = info->Shutter + (info->Shutter * change_ratio / 100);
            if (result->Shutter > Shutter_Max) result->Shutter = Shutter_Max;
        }
        else
        {
            result->SensorGain = info->SensorGain + (info->SensorGain * change_ratio / 100);
            if (result->SensorGain > Gain_Max) result->SensorGain = Gain_Max;
        }
        result->Change = 1;
    }
    else if(avg > y_upper)
    {
        if (info->SensorGain > Gain_Min)
        {
            result->SensorGain = info->SensorGain - (info->SensorGain * change_ratio / 100);
            if (result->SensorGain < Gain_Min) result->SensorGain = Gain_Min;
        }
        else
        {
            result->Shutter = info->Shutter - (info->Shutter * change_ratio / 100);
            if (result->Shutter < Shutter_Min) result->Shutter = Shutter_Min;
        }
        result->Change = 1;
    }

#if 0 //infinity5 //TBD
    //hdr demo code
    result->SensorGainHdrShort = result->SensorGain;
    result->ShutterHdrShort = result->Shutter * 1024 / result->HdrRatio;
#endif

#if (ENABLE_DOAE_MSG)
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("SensorGain: %d -> %d \n", (int)info->SensorGain, (int)result->SensorGain);
    printf("Shutter: %d -> %d \n", (int)info->Shutter, (int)result->Shutter);
#endif

#endif

#endif
}


int ae_ctrl(void *pdata, ISP_AE_CTRL_CMD cmd, void* param)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    const MI_CUS3A_CtrlCmd_t *pstCus3aCtrlCmd = (MI_CUS3A_CtrlCmd_t *)param;

    if(pdata == NULL)
    {
        printf("ae_ctrl pdata error!!! NULL Pointer!!!\n");
        return -1;
    }

    if(param == NULL)
    {
        printf("ae_ctrl param error!!! NULL Pointer!!!\n");
        return -1;
    }

    pAe = &pstCus3a->tCusAeInfo;

    printf("[ae_ctrl] CtrlID = %d, Dir = %d\n", (int)cmd, pstCus3aCtrlCmd->u32Dir);
    if(pstCus3aCtrlCmd->u32Dir)
    {
        DrvAlgo_IF_ApiGet(cmd, pAe->hAe, NULL, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (const void*)&pstCus3aCtrlCmd->pData);
    }
    else
    {
        DrvAlgo_IF_ApiSet(cmd, pAe->hAe, NULL, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (const void*)&pstCus3aCtrlCmd->pData);
    }

    return 0;
}

#elif ((AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2) || (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_3))

static u32 g_ThreadRun = 0;
pthread_t g_Cus3aThread;

void* Cus3aThreadProcRoutine(void* data);
static int Cus3aThreadProcAE(void);

pthread_mutex_t g_ThreadLock = PTHREAD_MUTEX_INITIALIZER;

int Cus3aThreadInitialization(void)
{
    int r;
    pthread_attr_t attr;

    g_ThreadRun = 1;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    r = pthread_create(&g_Cus3aThread, NULL, Cus3aThreadProcRoutine, (void*)NULL);
    pthread_attr_destroy (&attr);
    if(r == 0)
        return 0;
    else
    {
        printf("Failed to create Cus3A thread. err=%d", r);
        return r;
    }
}

void Cus3aThreadRelease(void)
{
    if(g_ThreadRun == 1)
    {
        g_ThreadRun = 0;
        pthread_join(g_Cus3aThread, NULL);
    }
    printf("===== Cus3aThreadRelease() =====\n");
}

void* Cus3aThreadProcRoutine(void* data)
{
    int fd0 = 0, fd1 = 0;

    Cus3AOpenIspFrameSync(&fd0, &fd1);
    printf("fd0 = 0x%x, fd1 = 0x%x\n", fd0, fd1);
    while(g_ThreadRun)
    {
        unsigned int status = Cus3AWaitIspFrameSync(fd0, fd1, 500);
        pthread_mutex_lock(&g_ThreadLock);
        if(status != 0)
        {
            Cus3aThreadProcAE();
        }
        pthread_mutex_unlock(&g_ThreadLock);
    }
    Cus3ACloseIspFrameSync(fd0, fd1);
    return 0;
}

int Cus3aDoAE(ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    unsigned int max = info->AvgBlkY * info->AvgBlkX;
    unsigned int avg = 0;
    unsigned int n;

    result->Change              = 0;
    result->u4BVx16384          = 16384;
    result->HdrRatio            = 10;
    result->IspGain             = 1024;
    result->SensorGain          = 4096;
    result->Shutter             = 20000;
    result->IspGainHdrShort     = 1024;
    result->SensorGainHdrShort  = 1024;
    result->ShutterHdrShort     = 2000;
    //result->Size         = sizeof(CusAEResult_t);

    for(n = 0; n < max; ++n)
    {
        avg += info->avgs[n].y;
    }
    avg /= max;

    result->AvgY         = avg;
    //printf("avg = 0x%X \n", avg);

#if SHUTTER_GAIN_DELAY_TEST1 // shutter gain delay test
    {
        static unsigned int fcount = 0;
        static int tmp = 0;

        printf("AvgLum = 0x%X \n", avg);
        if(++fcount % 60 == 0)
        {
            if ((++tmp % 2) == 0)
            {
                result->Shutter = 30000;
                result->SensorGain = 4096;
                result->IspGain             = 1024;
            }
            else
            {
                result->Shutter = 10000;
                result->SensorGain = 4096;
                result->IspGain             = 1024;
            }
            result->Change = 1;
            printf("===== Shutter = %ld\n", result->Shutter);
        }
    }
#endif

#if AE_EXAMPLE  // DoAE sample code
    {
        int y_lower = 0x28;
        int y_upper = 0x38;
        int change_ratio = 10; // percentage
        int Gain_Min = 1024 * 2;
        int Gain_Max = 1024 * 1000;
        int Shutter_Min = 150;
        int Shutter_Max = 33333;

        result->SensorGain = info->SensorGain;
        result->Shutter = info->Shutter;

        if(avg < y_lower)
        {
            if (info->Shutter < Shutter_Max)
            {
                result->Shutter = info->Shutter + (info->Shutter * change_ratio / 100);
                if (result->Shutter > Shutter_Max) result->Shutter = Shutter_Max;
            }
            else
            {
                result->SensorGain = info->SensorGain + (info->SensorGain * change_ratio / 100);
                if (result->SensorGain > Gain_Max) result->SensorGain = Gain_Max;
            }
            result->Change = 1;
        }
        else if(avg > y_upper)
        {
            if (info->SensorGain > Gain_Min)
            {
                result->SensorGain = info->SensorGain - (info->SensorGain * change_ratio / 100);
                if (result->SensorGain < Gain_Min) result->SensorGain = Gain_Min;
            }
            else
            {
                result->Shutter = info->Shutter - (info->Shutter * change_ratio / 100);
                if (result->Shutter < Shutter_Min) result->Shutter = Shutter_Min;
            }
            result->Change = 1;
        }

#if (ENABLE_DOAE_MSG)
        printf("AvgY = %ld, Shutter = %ld(%ld), Gain = %ld(%ld)\n", result->AvgY, result->Shutter, info->Shutter, result->SensorGain, info->SensorGain);
#endif
    }
#endif
    return 0;
}

static int Cus3aThreadProcAE(void)
{
    MI_ISP_AE_HW_STATISTICS_t *pAvg = NULL;
    MI_S32 sret = MI_ISP_OK;
    CusAEInfo_t tCusAeInfo;
    CusAEResult_t tCusAeResult;
    int nCh = 0;
    ISP_AE_INFO tAeInfo;
    ISP_AE_RESULT tAeResult;

    /*AE avg statistics*/
    pAvg = malloc(sizeof(MI_ISP_AE_HW_STATISTICS_t));
    sret = MI_ISP_AE_GetAeHwAvgStats(nCh, pAvg);
    if(sret != MI_ISP_OK)
    {
        printf("%s,%d error!\n", __FUNCTION__, __LINE__);
        goto Cus3aThreadProcAE_EXIT;
    }

    /*Check AE param*/
    memset(&tCusAeInfo, 0, sizeof(CusAEInfo_t));
    sret = MI_ISP_CUS3A_GetAeStatus(nCh, &tCusAeInfo);
    if(sret != MI_ISP_OK)
    {
        printf("%s,%d error!\n", __FUNCTION__, __LINE__);
        goto Cus3aThreadProcAE_EXIT;
    }

    tAeInfo.Size        = sizeof(ISP_AE_INFO);
    tAeInfo.hist1       = NULL;
    tAeInfo.hist2       = NULL;
    tAeInfo.AvgBlkX     = tCusAeInfo.AvgBlkX;
    tAeInfo.AvgBlkY     = tCusAeInfo.AvgBlkY;
    tAeInfo.avgs        = (ISP_AE_SAMPLE*)pAvg->nAvg;
    tAeInfo.Shutter     = tCusAeInfo.Shutter;
    tAeInfo.SensorGain  = tCusAeInfo.SensorGain;
    tAeInfo.IspGain     = tCusAeInfo.IspGain;
    tAeInfo.ShutterHDRShort = tCusAeInfo.ShutterHDRShort;
    tAeInfo.SensorGainHDRShort = tCusAeInfo.SensorGainHDRShort;
    tAeInfo.IspGainHDRShort = tCusAeInfo.IspGainHDRShort;

    memset(&tAeResult, 0, sizeof(ISP_AE_RESULT));
    tAeResult.Size = sizeof(ISP_AE_RESULT);

    Cus3aDoAE(&tAeInfo, &tAeResult);

    if(tAeResult.Change)
    {
        memset(&tCusAeResult, 0, sizeof(CusAEResult_t));
        tCusAeResult.Size         = sizeof(CusAEResult_t);
        tCusAeResult.Change       = tAeResult.Change;
        tCusAeResult.u4BVx16384   = tAeResult.u4BVx16384;
        tCusAeResult.HdrRatio     = tAeResult.HdrRatio;
        tCusAeResult.ShutterHdrShort = tAeResult.ShutterHdrShort;
        tCusAeResult.Shutter      = tAeResult.Shutter;
        tCusAeResult.IspGain      = tAeResult.IspGain;
        tCusAeResult.SensorGain   = tAeResult.SensorGain;
        tCusAeResult.SensorGainHdrShort = tAeResult.SensorGainHdrShort;
        tCusAeResult.IspGainHdrShort = tAeResult.IspGainHdrShort;
        tCusAeResult.AvgY         = tAeResult.AvgY;

        sret = MI_ISP_CUS3A_SetAeParam(nCh, &tCusAeResult);
        if(sret != MI_ISP_OK)
        {
            printf("%s,%d error!\n", __FUNCTION__, __LINE__);
            goto Cus3aThreadProcAE_EXIT;
        }
    }

Cus3aThreadProcAE_EXIT:

    free(pAvg);
    return 0;
}

#if (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2)
void ae_run_with_frame_sync(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    unsigned int max = info->AvgBlkY * info->AvgBlkX;
    unsigned int avg = 0;
    unsigned int n;

    result->Change              = 0;
    result->u4BVx16384          = 16384;
    result->HdrRatio            = 10; //infinity5 //TBD //10 * 1024;   //user define hdr exposure ratio
    result->IspGain             = 1024;
    result->SensorGain          = 4096;
    result->Shutter             = 20000;
    result->IspGainHdrShort     = 1024;
    result->SensorGainHdrShort  = 1024;
    result->ShutterHdrShort     = 2000;
    //result->Size         = sizeof(CusAEResult_t);

    for(n = 0; n < max; ++n)
    {
        avg += info->avgs[n].y;
    }
    avg /= max;

    result->AvgY         = avg;

#if SHUTTER_GAIN_DELAY_TEST2 // shutter gain delay test
    {
        static unsigned int fcount = 0;
        static int tmp = 0;

        printf("AvgLum = 0x%X \n", avg);
        if(++fcount % 60 == 0)
        {
            if ((++tmp % 2) == 0)
            {
                result->Shutter = 30000;
                result->SensorGain = 4096;
                result->IspGain             = 1024;
            }
            else
            {
                result->Shutter = 10000;
                result->SensorGain = 4096;
                result->IspGain             = 1024;
            }
            result->Change = 1;
            printf("===== Shutter = %ld\n", result->Shutter);
        }
    }
#endif

}
#endif  // end of #if (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2)

#endif  // end of #if ((AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2) || (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_3))


int awb_init(void *pdata)
{
    printf("************ awb_init **********\n");
    return 0;
}

void awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result)
{
    static u32 count = 0;
    int avg_r = 0;
    int avg_g = 0;
    int avg_b = 0;
    int tar_rgain = 1024;
    int tar_bgain = 1024;
    int x = 0;
    int y = 0;

    result->R_gain = info->CurRGain;
    result->G_gain = info->CurGGain;
    result->B_gain = info->CurBGain;
    result->Change = 0;
    result->ColorTmp = 6000;

    if (++count % 4 == 0)
    {
        //center area YR/G/B avg
        for (y = 30; y < 60; ++y)
        {
            for (x = 32; x < 96; ++x)
            {
                avg_r += info->avgs[info->AvgBlkX * y + x].r;
                avg_g += info->avgs[info->AvgBlkX * y + x].g;
                avg_b += info->avgs[info->AvgBlkX * y + x].b;
            }
        }
        avg_r /= 30 * 64;
        avg_g /= 30 * 64;
        avg_b /= 30 * 64;

        if (avg_r < 1)
            avg_r = 1;
        if (avg_g < 1)
            avg_g = 1;
        if (avg_b < 1)
            avg_b = 1;

#if (ENABLE_DOAWB_MSG)
        printf("AVG R / G / B = %d, %d, %d \n", avg_r, avg_g, avg_b);
#endif

        // calculate Rgain, Bgain
        tar_rgain = avg_g * 1024 / avg_r;
        tar_bgain = avg_g * 1024 / avg_b;

        if (tar_rgain > info->CurRGain)
        {
            if (tar_rgain - info->CurRGain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain + (tar_rgain - info->CurRGain) / 10;
        }
        else
        {
            if (info->CurRGain - tar_rgain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain - (info->CurRGain - tar_rgain) / 10;
        }

        if (tar_bgain > info->CurBGain)
        {
            if (tar_bgain - info->CurBGain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain + (tar_bgain - info->CurBGain) / 10;
        }
        else
        {
            if (info->CurBGain - tar_bgain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain - (info->CurBGain - tar_bgain) / 10;
        }

        result->Change = 1;
        result->G_gain = 1024;

#if (ENABLE_DOAWB_MSG)
        printf("[current] r=%d, g=%d, b=%d \n", (int)info->CurRGain, (int)info->CurGGain, (int)info->CurBGain);
        printf("[result] r=%d, g=%d, b=%d \n", (int)result->R_gain, (int)result->G_gain, (int)result->B_gain);
#endif
    }
}

int awb_ctrl(void *pdata, ISP_AWB_CTRL_CMD cmd, void* param)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAwbInfo_t *pAwb;
    const MI_CUS3A_CtrlCmd_t *pstCus3aCtrlCmd = (MI_CUS3A_CtrlCmd_t *)param;

    if(pdata == NULL)
    {
        printf("awb_ctrl pdata error!!! NULL Pointer!!!\n");
        return -1;
    }

    if(param == NULL)
    {
        printf("ae_ctrl param error!!! NULL Pointer!!!\n");
        return -1;
    }

    pAwb = &pstCus3a->tCusAwbInfo;

    printf("[awb_ctrl] CtrlID = %d, Dir = %d\n", (int)cmd, pstCus3aCtrlCmd->u32Dir);
    if(pstCus3aCtrlCmd->u32Dir)
    {
        DrvAlgo_IF_ApiGet(cmd, NULL, pAwb->hAwb, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (const void*)&pstCus3aCtrlCmd->pData);
    }
    else
    {
        DrvAlgo_IF_ApiSet(cmd, NULL, pAwb->hAwb, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (const void*)&pstCus3aCtrlCmd->pData);
    }

    return 0;
}

void awb_release(void *pdata)
{
    printf("************ awb_release **********\n");
}

int af_init(void *pdata, ISP_AF_INIT_PARAM *param)
{
    MI_U32 u32ch = 0;
    MI_U8 u8win_idx = 16;
    CusAFRoiMode_t taf_roimode;

    printf("************ af_init **********\n");

#if 1
    //Init Normal mode setting
    taf_roimode.mode = AF_ROI_MODE_NORMAL;
    taf_roimode.u32_vertical_block_number = 1;
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {
        { 0, {   0,    0,  255,  255}},
        { 1, { 256,    0,  511,  255}},
        { 2, { 512,    0,  767,  255}},
        { 3, { 768,    0, 1023,  255}},
        { 4, {   0,  256,  255,  511}},
        { 5, { 256,  256,  511,  511}},
        { 6, { 512,  256,  767,  511}},
        { 7, { 768,  256, 1023,  511}},
        { 8, {   0,  512,  255,  767}},
        { 9, { 256,  512,  511,  767}},
        {10, { 512,  512,  767,  767}},
        {11, { 768,  512, 1023,  767}},
        {12, {   0,  768,  255, 1023}},
        {13, { 256,  768,  511, 1023}},
        {14, { 512,  768,  767, 1023}},
        {15, { 768,  768, 1023, 1023}}
    };
    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }

#else
    //Init Matrix mode setting
    taf_roimode.mode = AF_ROI_MODE_MATRIX;
    taf_roimode.u32_vertical_block_number = 16; //16xN, N=16
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {
#if 1
        //full image, equal divide to 16x16
        {0, {0, 0, 63, 63}},
        {1, {64, 64, 127, 127}},
        {2, {128, 128, 191, 191}},
        {3, {192, 192, 255, 255}},
        {4, {256, 256, 319, 319}},
        {5, {320, 320, 383, 383}},
        {6, {384, 384, 447, 447}},
        {7, {448, 448, 511, 511}},
        {8, {512, 512, 575, 575}},
        {9, {576, 576, 639, 639}},
        {10, {640, 640, 703, 703}},
        {11, {704, 704, 767, 767}},
        {12, {768, 768, 831, 831}},
        {13, {832, 832, 895, 895}},
        {14, {896, 896, 959, 959}},
        {15, {960, 960, 1023, 1023}}
#else
        //use two row only => 16x2 win
        {0, {0, 0, 63, 63}},
        {1, {64, 64, 127, 127}},
        {2, {128, 0, 191, 2}},      //win2 v_str, v_end doesn't use, set to (0, 2)
        {3, {192, 0, 255, 2}},
        {4, {256, 0, 319, 2}},
        {5, {320, 0, 383, 2}},
        {6, {384, 0, 447, 2}},
        {7, {448, 0, 511, 2}},
        {8, {512, 0, 575, 2}},
        {9, {576, 0, 639, 2}},
        {10, {640, 0, 703, 2}},
        {11, {704, 0, 767, 2}},
        {12, {768, 0, 831, 2}},
        {13, {832, 0, 895, 2}},
        {14, {896, 0, 959, 2}},
        {15, {960, 0, 1023, 2}}
#endif
    };

    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }
#endif

    static CusAFFilter_t affilter =
    {
        //filter setting with sign value
        //{63, -126, 63, -109, 48, 0, 320, 0, 1023},
        //{63, -126, 63, 65, 55, 0, 320, 0, 1023}

        //convert to hw format (sign bit with msb)
        63, 126 + 1024, 63, 109 + 128, 48, 0, 320, 0, 1023,
        63, 126 + 1024, 63, 65, 55, 0, 320, 0, 1023,

    };

    MI_ISP_CUS3A_SetAFFilter(0, &affilter);
    return 0;
}

void af_release(void *pdata)
{
    printf("************ af_release **********\n");
}

void af_run(void *pdata, const ISP_AF_INFO *af_info, ISP_AF_RESULT *result)
{
#if (ENABLE_DOAF_MSG)
    int i = 0, x = 0;

    printf("\n\n");

    //print row0 16wins
    x = 0;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }

    //print row15 16wins
    x = 15;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }
#endif
}

int af_ctrl(void *pdata, ISP_AF_CTRL_CMD cmd, void* param)
{
    return 0;
}

#define BUFFER_SIZE_OUT_16BITS(w) ((((w + 7) / 8) * 8) * 2) //unsigned char
#define BUFFER_SIZE_IN_12BITS(w) ((((w + 31) / 32) * 32) * 12 / 8) // (12bits/ pixel) -> (12 / 8) (bytes / pixel)
#define BUFFER_SIZE_IN_10BITS(w) ((((w + 63) / 64) * 64) * 10 / 8)  // (10bits/ pixel) -> (10 / 8) (bytes / pixel)
#define BUFFER_SIZE_IN_8BITS(w) ((((w + 15) / 16) * 16))   // (8bits/ pixel) -> (8 / 8) (bytes / pixel)

void Convert12BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 4) & 0x0ff0) + (((unsigned short)pInbuf[1] << 12) & 0xf000);
    pOutbuf[1] = (((unsigned short)pInbuf[1]) & 0x00f0) + (((unsigned short)pInbuf[2] << 8) & 0xff00);
    pOutbuf[2] = (((unsigned short)pInbuf[3] << 4) & 0x0ff0) + (((unsigned short)pInbuf[4] << 12) & 0xf000);
    pOutbuf[3] = (((unsigned short)pInbuf[4]) & 0x00f0) + (((unsigned short)pInbuf[5] << 8) & 0xff00);
    *pu32InSize = 6;
    *pu32OutSize = 4;
}

void Convert10BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 6) & 0x3fc0) + (((unsigned short)pInbuf[1] << 14) & 0xc000);
    pOutbuf[1] = (((unsigned short)pInbuf[1] << 4) & 0x0fc0) + (((unsigned short)pInbuf[2] << 12) & 0xf000);
    pOutbuf[2] = (((unsigned short)pInbuf[2] << 2) & 0x03c0) + (((unsigned short)pInbuf[3] << 10) & 0xfc00);
    pOutbuf[3] = (((unsigned short)pInbuf[3]) & 0x00c0) + (((unsigned short)pInbuf[4] << 8) & 0xff00);
    *pu32InSize = 5;
    *pu32OutSize = 4;
}

void Convert8BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 8) & 0xff00);
    pOutbuf[1] = (((unsigned short)pInbuf[1] << 8) & 0xff00);
    pOutbuf[2] = (((unsigned short)pInbuf[2] << 8) & 0xff00);
    pOutbuf[3] = (((unsigned short)pInbuf[3] << 8) & 0xff00);
    *pu32InSize = 4;
    *pu32OutSize = 4;
}

void ConvertOneLineTo16Bits(unsigned char *pbufin, unsigned short *pbufout, unsigned long ulinbufsize, unsigned long uloutbufsize, unsigned short usbitwidth)
{
    unsigned long u32InSize, u32OutSize;
    unsigned long InCnt = 0;
    unsigned long OutCnt = 0;

    memset((void *)pbufout, 0x0, uloutbufsize);

    u32InSize = 0;
    u32OutSize = 0;
    while ((u32InSize < ulinbufsize) && (u32OutSize < uloutbufsize))
    {
        if (usbitwidth == 12)
        {
            Convert12BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        else if (usbitwidth == 10)
        {
            Convert10BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        else if (usbitwidth == 8)
        {
            Convert8BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        pbufin += InCnt;
        pbufout += OutCnt;
        u32InSize += InCnt;
        u32OutSize += OutCnt;
    }
}

void ConvertRawImageTo16Bits(void *pBufin, void *pBufout, unsigned long u32Width, unsigned long u32Height, unsigned long u32BitDepth)
{
    unsigned char *pbufin;
    unsigned short *pbufout;
    unsigned long buf_size_in, buf_size_out;
    unsigned long linecnt = 0;
    unsigned short bitwidth = 0;

    if (u32BitDepth == 0)  // 8-bits
    {
        buf_size_in = BUFFER_SIZE_IN_8BITS(u32Width);
        bitwidth = 8;
    }
    else if (u32BitDepth == 1)  // 10 bits
    {
        buf_size_in = BUFFER_SIZE_IN_10BITS(u32Width);
        bitwidth = 10;
    }
    else if (u32BitDepth == 3)  // 12-bits
    {
        buf_size_in = BUFFER_SIZE_IN_12BITS(u32Width);
        bitwidth = 12;
    }
    else
    {
        printf("Error! Don't need to convert image!!!u32BitDepth = %lu\n", u32BitDepth);
        return;
    }

    buf_size_out = (BUFFER_SIZE_OUT_16BITS(u32Width) >> 1);  // by unsigned short

    linecnt = 0;
    pbufin = (unsigned char *)pBufin;
    pbufout = (unsigned short *)pBufout;
    while (linecnt < u32Height)
    {
        ConvertOneLineTo16Bits(pbufin, pbufout, buf_size_in, buf_size_out, bitwidth);
        linecnt++;
        pbufin += buf_size_in;
        pbufout += buf_size_out;
    }
}

MI_S32 main(int argc, char **argv)
{
#if (UVC_FUNCTION_EN)
    MI_S32 s32Mode = 0;
#endif
    char cmd = 0;
    int i = 0;
    u32 nCh = 0;
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
#if (UVC_FUNCTION_EN)  //infinity5
    printf("Which mode do you want ?\n 0 for real time mode 1 for frame mode 2 for HDR real time mode\n");
    scanf("%d", &s32Mode);
    printf("You select %s mode\n", s32Mode ? "frame" : "real time");
    bFrameOrRealMode = s32Mode;
    _gs32Nv12ToMjpg = _UVC_IsSupportNv12ToMjpg();
    _gs32UseUac = _UVC_IsSupportAudio();
    memset(&_gstModState, 0, sizeof(ST_ModuleState_t));
    St_BaseModuleInit(s32Mode);
    St_DisplayInit();
    St_UvcInit();
    St_UacInit(_gs32UseUac, _gs32UseUac);
#endif
#if(ENABLE_DUMPCIF_PORT1 == 1)
    DC_TemCreate();
#endif
    while(1)
    {
        printf("Type \"q\" to exit,\n");
        printf("\"i\" to test mi api,\n");
        printf("\"c\" to start CusAE,\n");
        printf("\"d\" to dump ISP out YUV image\n");
        printf("\"D\" to dump ISP in Bayer raw image\n");
        printf("\"H\" to dump HDR P1 Raw image\n");
        printf("\"j\" pause AE\n");
        printf("\"k\" resume AE\n");
        printf("Type \"r\" to dump IR image\n");

        cmd = getchar();
        if (cmd == 'q')
        {
#if ((AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2) || (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_3))
            Cus3aThreadRelease();
#endif
            /*Register api agent*/
            MI_ISP_RegisterIspApiAgent(0, NULL, NULL);

            CUS3A_RegInterface(0, NULL, NULL, NULL);
            CUS3A_Release();
            break;
        }
        else if(cmd == 'i')
        {
            test_mi_api();
        }
        else if(cmd == 'j')
        {
            MI_ISP_AE_SetState(0,SS_ISP_STATE_PAUSE);
        }
        else if(cmd == 'k')
        {
            MI_ISP_AE_SetState(0,SS_ISP_STATE_NORMAL);
        }
        else if(cmd == 'c')
        {
            ISP_AE_INTERFACE tAeIf;
            ISP_AWB_INTERFACE tAwbIf;
            ISP_AF_INTERFACE tAfIf;

            CUS3A_Init();

            /*AE*/
            tAeIf.ctrl = ae_ctrl;
            tAeIf.pdata = &g_tCus3APrivData[nCh];
            tAeIf.init = ae_init;
            tAeIf.release = ae_release;
#if (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_1)
            tAeIf.run = ae_run;
#elif (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2)
            tAeIf.run = ae_run_with_frame_sync;
#elif (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_3)
            tAeIf.run = NULL;
#endif

            /*AWB*/
            tAwbIf.ctrl = NULL;
            tAwbIf.pdata = NULL;
            tAwbIf.init = awb_init;
            tAwbIf.release = awb_release;
            tAwbIf.run = awb_run;

            /*AF*/
            tAfIf.pdata = NULL;
            tAfIf.init = af_init;
            tAfIf.release = af_release;
            tAfIf.run = af_run;
            tAfIf.ctrl = af_ctrl;
            CUS3A_RegInterface(nCh, &tAeIf, &tAwbIf, &tAfIf);

            /*Register api agent*/
            MI_ISP_RegisterIspApiAgent(0, Cus3A_SetIspApiData, Cus3A_GetIspApiData);

#if ((AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_2) || (AE_RUN_WITH_USE_CASE == AE_RUN_WITH_USE_CASE_3))
            Cus3aThreadInitialization();
#endif
        }
        else if((cmd == 'd') || (cmd == 'D') || (cmd == 'H') || (cmd == 'r'))
        {
            void *pImgAddrSrc, *pImgAddrDst;
            u32  u32BufSizeSrc, u32BufSizeDst;
            u32 u32PhysAddrSrc = 0, u32PhysAddrDst = 0;
            u32 u32MiuAddrSrc = 0, u32MiuAddrDst = 0;
            MI_U32 u32Channel;
            MI_U32 u32isp_out_image_count = 0, u32timeout = 0;
            CusImageResolution_t timage_resolution;
            CusISPOutImage_t tisp_out_image;
            CameraRawStoreNode_e eNode;
            CusHdrRawImage_t thdr_raw_image;

            if(cmd == 'd')
            {
                eNode = eRawStoreNode_ISPOUT;
            }
            else if(cmd == 'D')
            {
                eNode = eRawStoreNode_P0TAIL;
            }
            else if(cmd == 'H')
            {
                eNode = eRawStoreNode_P1HEAD;
            }
            else if(cmd == 'r')
            {
                eNode = eRawStoreNode_RGBIR_IR_ONLY;
            }

            //Get width and height from isp api.
            u32Channel = 0;
            timage_resolution.u32Node = (MI_U32)eNode;
            MI_ISP_CUS3A_GetImageResolution(u32Channel, &timage_resolution);
            printf("Node:%d, width=%d, height=%d, depth=%d\n", eNode, timage_resolution.u32image_width, timage_resolution.u32image_height, timage_resolution.u32PixelDepth);
            /*
            // data precision
            {
                ISP_DATAPRECISION_8 = 0,
                ISP_DATAPRECISION_10 = 1,
                ISP_DATAPRECISION_16 = 2,
                ISP_DATAPRECISION_12 = 3,
            }
            */
            u32BufSizeSrc = (eNode == eRawStoreNode_RGBIR_IR_ONLY) ?
                            timage_resolution.u32image_width * timage_resolution.u32image_height / 4 :
                            timage_resolution.u32image_width * timage_resolution.u32image_height * 2;

            do
            {
                if(u32BufSizeSrc == 0)
                {
                    printf("Error! Buffer size is 0.\n");
                    break;
                }

                pImgAddrSrc = (void *)pAllocDmaBuffer("ISP_OUT_SRC", u32BufSizeSrc, &u32PhysAddrSrc, &u32MiuAddrSrc, FALSE);
                if(pImgAddrSrc == NULL)
                {
                    printf("Error! Allocate buffer Error!!!\n");
                    break;
                }

                if(eNode == eRawStoreNode_P1HEAD)
                {
                    thdr_raw_image.u32enable = 1;
                    thdr_raw_image.u32image_width = timage_resolution.u32image_width;
                    thdr_raw_image.u32image_height = timage_resolution.u32image_height;
                    thdr_raw_image.u32physical_address = u32MiuAddrSrc;
                    thdr_raw_image.u32Node = (MI_U32)eNode;
                    thdr_raw_image.u32PixelDepth = (MI_U32)timage_resolution.u32PixelDepth;
                    MI_ISP_CUS3A_CaptureHdrRawImage(u32Channel, &thdr_raw_image);

                    u32timeout = 100;
                    do
                    {
                        usleep(1000 * 10);
                        MI_ISP_CUS3A_GetISPOutImageCount(u32Channel, &u32isp_out_image_count);
                    }
                    while((u32isp_out_image_count < 2) && (--u32timeout > 0));
                    printf("HDR raw image count:%d, time:%d ms.\n", u32isp_out_image_count, (100 - u32timeout) * 10);

                    thdr_raw_image.u32enable = 0;
                    MI_ISP_CUS3A_CaptureHdrRawImage(u32Channel, &thdr_raw_image);

                    if(thdr_raw_image.u32PixelDepth != 2)  // not  ISP_DATAPRECISION_16
                    {
                        u32BufSizeDst = thdr_raw_image.u32image_width * thdr_raw_image.u32image_height * 2;
                        pImgAddrDst = (void *)pAllocDmaBuffer("ISP_OUT_DST", u32BufSizeDst, &u32PhysAddrDst, &u32MiuAddrDst, FALSE);
                        if(pImgAddrDst == NULL)
                        {
                            printf("Error! Allocate buffer Error!!!\n");
                            break;
                        }
                        ConvertRawImageTo16Bits(pImgAddrSrc, pImgAddrDst, thdr_raw_image.u32image_width, thdr_raw_image.u32image_height, thdr_raw_image.u32PixelDepth);
                    }
                    else  // for ISP_DATAPRECISION_16
                    {
                        pImgAddrDst = pImgAddrSrc;
                        u32BufSizeDst = u32BufSizeSrc;
                    }
                }
                else
                {
                    tisp_out_image.u32enable = 1;
                    tisp_out_image.u32image_width = timage_resolution.u32image_width;
                    tisp_out_image.u32image_height = timage_resolution.u32image_height;
                    tisp_out_image.u32physical_address = u32MiuAddrSrc;
                    tisp_out_image.u32Node = (MI_U32)eNode;
                    MI_ISP_CUS3A_EnableISPOutImage(u32Channel, &tisp_out_image);

                    u32timeout = 100;
                    do
                    {
                        usleep(1000 * 10);
                        MI_ISP_CUS3A_GetISPOutImageCount(u32Channel, &u32isp_out_image_count);
                    }
                    while((u32isp_out_image_count < 2) && (--u32timeout > 0));
                    printf("ISP out image count:%d, time:%d ms.\n", u32isp_out_image_count, (100 - u32timeout) * 10);

                    tisp_out_image.u32enable = 0;
                    MI_ISP_CUS3A_EnableISPOutImage(u32Channel, &tisp_out_image);
                    pImgAddrDst = pImgAddrSrc;
                    u32BufSizeDst = u32BufSizeSrc;
                }

                //Save image file.
                {
                    char filename[64];
                    //struct timeval timestamp;

                    //gettimeofday(&timestamp, 0);
                    //printf("saveSnapshot%d_%08d.%s buffLen=%d\n", (int)timestamp.tv_sec, (int)timestamp.tv_usec, name, buffLen);

                    memset(filename, 0x00, sizeof(filename));
                    if(eNode == eRawStoreNode_ISPOUT)
                        sprintf(filename, "dump_ispout_yuv422.raw");
                    else if(eNode == eRawStoreNode_RGBIR_IR_ONLY)
                        sprintf(filename, "dump_RGBIR_IR_only_raw8.raw");
                    else if(eNode == eRawStoreNode_P1HEAD)
                    {
                        switch(thdr_raw_image.u32PixelDepth)
                        {
                            case 0: // ISP_DATAPRECISION_8
                                sprintf(filename, "dump_hdr_p1_raw8_to_raw16.raw");
                                break;
                            case 1: // ISP_DATAPRECISION_10
                                sprintf(filename, "dump_hdr_p1_raw10_to_raw16.raw");
                                break;
                            case 2: // ISP_DATAPRECISION_16
                                sprintf(filename, "dump_hdr_p1_raw16.raw");
                                break;
                            case 3: // ISP_DATAPRECISION_12
                                sprintf(filename, "dump_hdr_p1_raw12_to_raw16.raw");
                                break;
                            default:
                                printf("data precision error!! fmt: %d\n", thdr_raw_image.u32PixelDepth);
                                break;
                        }
                    }
                    else
                        sprintf(filename, "dump_ispin_p0_raw16.raw");

                    FILE *pfile = fopen(filename, "wb");
                    if(NULL == pfile)
                    {
                        printf("error: fopen %s failed\n", filename);
                        return -1;
                    }

                    fwrite(pImgAddrDst, 1, u32BufSizeDst, pfile);

                    fflush(pfile);
                    fclose(pfile);
                    sync();
                }

                FreeDmaBuffer("ISP_OUT_SRC", u32MiuAddrSrc, pImgAddrSrc, u32BufSizeSrc);
                if((eNode == eRawStoreNode_P1HEAD) && (thdr_raw_image.u32PixelDepth != 2))
                {
                    FreeDmaBuffer("ISP_OUT_DST", u32MiuAddrDst, pImgAddrDst, u32BufSizeDst);
                }
            }
            while(0);
        }
        pthread_mutex_lock(&_gTime);
        for (i = 0; i < 5; i++)
        {
            printf("time [%d] = %d\n", i, curtime[i]);
        }
        printf("Buf size is %d\n", u32BufSize);
        pthread_mutex_unlock(&_gTime);
    }
    St_UacDeinit(_gs32UseUac, _gs32UseUac);
    St_UvcDeinit();
    printf("St_UvcDeinit deinit\n");
    St_DisplayDeinit();
    printf("St_DisplayDeinit deinit\n");
    St_BaseModuleDeinit();
    return 0;
}
