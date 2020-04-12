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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_warp.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WARP_FUNC_EXEC(result)  do { \
    if (result == MI_WARP_OK)   \
    { \
        printf("%s %d is ok\n", __FUNCTION__, __LINE__);    \
    } \
    else \
    { \
        printf("\033[1m\033[42;31m%s %d is fail\033[0m\n", __FUNCTION__, __LINE__);    \
    } \
} while(0);

#define DBG_ERR(msg, ...) do{printf("[ST_WARP ERR] %s(%d):" msg,__FUNCTION__, __LINE__,##__VA_ARGS__);fflush(stdout); }while(0)
#define DBG_INFO(msg, ...) do{printf("[ST WARP INFO]: %s(%d):" msg,__FUNCTION__, __LINE__,##__VA_ARGS__);fflush(stdout);}while(0)
#define BUG_ON(cond) do{if(cond){DBG_ERR("[ST WARP BUG]: %s\n", #cond);while(1);/**((char*)(0))=0;*/}}while(0)


#define ST_BOUNDING_BOX_TABLE_MAX_NUM       1
#define ST_DISPLACEMENT_TABLE_MAX_NUM       1
#define ST_YUV_TEST_FILE_MAX_NUM            1

#define ST_THREAD_FLAG_STOP  0x01
#define ST_THREAD_FLAG_PAUSE 0x02

typedef enum
{
    THREAD_STATUS_STOP,
    THREAD_STATUS_PAUSE,
    THREAD_STATUS_MAX
}ST_WARP_ThreadStatus_e;

typedef struct
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_ChnPort_t stSysChnPort;
}ST_WARP_InjectHandle_t;

typedef struct
{
    MI_S32 s32Fd;
    MI_SYS_ChnPort_t stSysChnPort;
}ST_WARP_SinkHandle_t;

typedef struct
{
    MI_SYS_BUF_HANDLE bufHandle;
    MI_SYS_BufInfo_t stBufInfo;
}ST_WARP_FramePriv_t;

typedef struct ST_WARP_FramePlane_s
{
    MI_U8* pAddr;
    MI_PHY phy;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32Stride;
}ST_WARP_FramePlane_t;

typedef struct ST_WARP_FrameBuf_s
{
    MI_U64 u64Pts;
    MI_S32 s32Planenum;
    ST_WARP_FramePlane_t stPlane[3];
    void *pPriv;
}ST_WARP_FrameBuf_t;

typedef MI_S32 (*pfnGetInjectBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf);
typedef void (*pfnFinishInjectBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf, MI_BOOL bValidData);
typedef MI_S32 (*pfnWaitSinkBuf)(void *pHandle, MI_U32 u32Timeout);
typedef MI_S32 (*pfnGetSinkBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf);
typedef void (*pfnFinishSinkBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf);


//const char* _gszBbTablePath = "in/bb_undist.bin";
//const char* _gszDisplaceTablePath = "in/mi_3840x2160_disp_484x136_abs.bin";
//const char* _gszInjectYuvPath = "in/Mi_3840x2160_yuv420_nv12.yuv";
//const char* _gszSinkYuvPath = "out/Mi_3840x2160_yuv420_nv12.yuv";

const char* _gszBbTablePath = "warp/disp_abs_320x240_nv12/bb_320x240_nv12.bin";
const char* _gszDisplaceTablePath = "warp/disp_abs_320x240_nv12/mi_320x240_abs_disp.bin";
const char* _gszInjectYuvPath = "input/1.bin";
const char* _gszSinkYuvPath = "output/1.bin";

typedef struct ST_WARP_InjectInfo_s
{
    MI_BOOL bEnable;
    MI_S32 s32Fd;
    MI_S32 (*GetInjectBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf);
    void (*FinishInjectBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf, MI_BOOL bValidData);
    void *pHandle;
    MI_S32 s32FrmCnt;
}ST_WARP_InjectInfo_t;

typedef struct ST_WARP_SinkInfo_s
{
    MI_BOOL bEnable;
    MI_S32 s32Fd;
    MI_S32 (*WaitSinkBuf)(void *pHandle, MI_U32 u32Timeout); //0: ok; -1: fail and another wait; timeout: ms
    MI_S32 (*GetSinkBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf/*out*/);
    void (*FinishSinkBuf)(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf/*in*/);
    void *pHandle;
    MI_S32 s32FrmCnt;
}ST_WARP_SinkInfo_t;

typedef struct ST_WARP_Moudule_s
{
    MI_BOOL bInited;
    ST_WARP_InjectInfo_t stInjectInfo;
    ST_WARP_SinkInfo_t stSinkInfo;
    MI_U32 u32InjectFlag;
    pthread_t threadInject;
    pthread_mutex_t mtxInject;
    pthread_cond_t condInject;
    MI_U32 u32SinkFlag;
    pthread_t threadSink;
    pthread_mutex_t mtxSink;
    pthread_cond_t condSink;
}ST_WARP_Moudule_t;     // test 1 inject 1 sink first

static pthread_mutex_t _gmtxWarpModule = PTHREAD_MUTEX_INITIALIZER;
static ST_WARP_Moudule_t _gstWarpModule =
{
    .mtxInject=PTHREAD_MUTEX_INITIALIZER,
    .condInject=PTHREAD_COND_INITIALIZER,
    .mtxSink=PTHREAD_MUTEX_INITIALIZER,
    .condSink=PTHREAD_COND_INITIALIZER
};


MI_BOOL ST_WARP_OpenFile(const MI_S8 *ps8FileName, MI_BOOL bWrite, MI_S32 *ps32Fd)
{
    MI_S32 s32Fd = open(ps8FileName, bWrite?(O_WRONLY|O_CREAT|O_TRUNC):O_RDONLY);

    if (s32Fd < 0)
    {
        printf("Open file: %s failed.\n", ps8FileName);
        return FALSE;
    }

    *ps32Fd = s32Fd;
    return TRUE;
}

void ST_WARP_CloseFile(MI_S32 s32Fd)
{
    close(s32Fd);
}

MI_S32 _ST_WARP_ReadData(MI_S32 s32Fd, MI_U8 *pu8Buf, MI_S32 s32Len)
{
    MI_U8 *p = pu8Buf;
    MI_S32 s32Left = s32Len;

    while(s32Left > 0)
    {
        MI_S32 s32Ret = read(s32Fd, p, s32Left);

        if(s32Ret < 0)
        { //err
            if(errno == EINTR)
                continue;
            goto exit;
        }

        if(s32Ret == 0) //eof
        {
            goto exit;
        }
        p = p + s32Ret;
        s32Left -= s32Ret;
    }

exit:
    return (s32Len - s32Left);
}

MI_S32 _ST_WARP_WriteData(MI_S32 s32Fd, MI_U8 *pu8Buf, MI_S32 s32Len)
{
    MI_U8 *p = pu8Buf;
    MI_S32 s32Left = s32Len;

    while(s32Left > 0)
    {
        MI_S32 s32Ret = write(s32Fd, p, s32Left);
        if(s32Ret < 0)
        { //err
            if(errno == EINTR)
                continue;
            goto exit;
        }

        if(s32Ret == 0)
            goto exit;

        p = p + s32Ret;
        s32Left -= s32Ret;
    }

exit:
    return (s32Len - s32Left);
}

MI_S32 _ST_WARP_ReadToBuf(MI_S32 s32Fd, MI_U8 *pu8Buf, MI_S32 s32Width, MI_S32 s32Height, MI_S32 s32Stride)
{
    MI_U8 *p = pu8Buf;
    MI_S32 i;

    for(i = 0; i < s32Height; i++)
    {
        if(_ST_WARP_ReadData(s32Fd, p, s32Width) != s32Width)
            return -1;
        p += s32Stride;
    }

    return 0;
}

MI_S32 _ST_WARP_WriteFromBuf(MI_S32 s32Fd, MI_U8 *pu8Buf, MI_S32 s32Width, MI_S32 s32Height, MI_S32 s32Stride)
{
    MI_U8 *p = pu8Buf;
    int i;

    for(i = 0; i < s32Height; i++)
    {
        if(_ST_WARP_WriteData(s32Fd, p, s32Width) != s32Width)
            return -1;
        p += s32Stride;
    }

    return 0;
}

MI_S32 ST_WARP_ReadToFrame(MI_S32 s32Fd, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 i;

    for(i=0; i < pFrameBuf->s32Planenum; i++)
    {
        if(_ST_WARP_ReadToBuf(s32Fd, pFrameBuf->stPlane[i].pAddr, pFrameBuf->stPlane[i].u32Width
                             , pFrameBuf->stPlane[i].u32Height, pFrameBuf->stPlane[i].u32Stride) < 0)
        {
            printf("read to buf error\n");
            return -1;
        }
    }

    return 0;
}

static MI_S32 ST_WARP_WriteFromFrame(MI_S32 s32Fd, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 i;

    for(i = 0; i < pFrameBuf->s32Planenum; i++)
    {
        if(_ST_WARP_WriteFromBuf(s32Fd, pFrameBuf->stPlane[i].pAddr, pFrameBuf->stPlane[i].u32Width
                                , pFrameBuf->stPlane[i].u32Height, pFrameBuf->stPlane[i].u32Stride) < 0)
        {
            printf("write from buf error\n");
            return -1;
        }
    }

    return 0;
}

static MI_S32 ST_WARP_GetCurFilePos(MI_S32 s32Fd)
{
    off_t off_cur ;
    off_cur = lseek(s32Fd, 0, SEEK_CUR);
    return off_cur;
}


MI_S32 ST_WARP_GetInjectBufYuv422(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 s32Ret = -1;
    ST_WARP_InjectHandle_t *pInject = (ST_WARP_InjectHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv;
    pInject->stBufConf.u64TargetPts = pFrameBuf->u64Pts;
    pFrameBuf->pPriv = NULL;
    pPriv = malloc(sizeof(ST_WARP_FramePriv_t));

    if(!pPriv)
        goto exit;

    memset(pPriv, 0, sizeof(ST_WARP_FramePriv_t));
    if(MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&pInject->stSysChnPort,
                                             &pInject->stBufConf,
                                             &pPriv->stBufInfo,
                                             &pPriv->bufHandle, 0))
    {
        //DBG_INFO("get input port buf fail\n");
        goto free_priv;
    }

    BUG_ON(pPriv->stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME);
    BUG_ON(pPriv->stBufInfo.stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    pFrameBuf->pPriv = pPriv;
    pFrameBuf->s32Planenum = 1;
    pFrameBuf->stPlane[0].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[0];
    pFrameBuf->stPlane[0].u32Width = pPriv->stBufInfo.stFrameData.u16Width*2;
    pFrameBuf->stPlane[0].u32Height = pPriv->stBufInfo.stFrameData.u16Height;
    pFrameBuf->stPlane[0].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[0];
    pFrameBuf->stPlane[0].phy = pPriv->stBufInfo.stFrameData.phyAddr[0];
    s32Ret = 0;
    goto exit;
free_priv:
    free(pPriv);
exit:
    return s32Ret;
}

int ST_WARP_GetInjectBufYuv420(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 s32Ret = -1;
    ST_WARP_InjectHandle_t *pstInject = (ST_WARP_InjectHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv;
    pstInject->stBufConf.u64TargetPts = pFrameBuf->u64Pts;
    pFrameBuf->pPriv = NULL;
    pPriv = malloc(sizeof(ST_WARP_FramePriv_t));

    if(!pPriv)
        goto exit;

    memset(pPriv, 0, sizeof(ST_WARP_FramePriv_t));
    if(MI_SUCCESS!=MI_SYS_ChnInputPortGetBuf(&pstInject->stSysChnPort,
                                             &pstInject->stBufConf,
                                             &pPriv->stBufInfo,
                                             &pPriv->bufHandle, 0))
    {
        //DBG_INFO("get input port buf fail\n");
        goto free_priv;
    }

    BUG_ON(pPriv->stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME);
    BUG_ON(pPriv->stBufInfo.stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420);
    pFrameBuf->pPriv = pPriv;
    pFrameBuf->s32Planenum = 2;
    pFrameBuf->stPlane[0].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[0];
    pFrameBuf->stPlane[0].u32Width = pPriv->stBufInfo.stFrameData.u16Width;
    pFrameBuf->stPlane[0].u32Height = pPriv->stBufInfo.stFrameData.u16Height;
    pFrameBuf->stPlane[0].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[0];
    pFrameBuf->stPlane[0].phy = pPriv->stBufInfo.stFrameData.phyAddr[0];

    pFrameBuf->stPlane[1].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[1];
    pFrameBuf->stPlane[1].u32Width = pPriv->stBufInfo.stFrameData.u16Width;
    pFrameBuf->stPlane[1].u32Height = pPriv->stBufInfo.stFrameData.u16Height/2;
    pFrameBuf->stPlane[1].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[1];
    pFrameBuf->stPlane[1].phy = pPriv->stBufInfo.stFrameData.phyAddr[1];
    s32Ret = 0;
    goto exit;
free_priv:
    free(pPriv);
exit:
    return s32Ret;
}

MI_S32 ST_WARP_GetInjectBuf(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 s32Ret = 0;
    ST_WARP_InjectHandle_t *pstInject = (ST_WARP_InjectHandle_t *)pHandle;
    BUG_ON(pstInject->stBufConf.eBufType!=E_MI_SYS_BUFDATA_FRAME);

    if(E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == pstInject->stBufConf.stFrameCfg.eFormat)
    {
        s32Ret = ST_WARP_GetInjectBufYuv422(pHandle, pFrameBuf);
    }
    else if(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == pstInject->stBufConf.stFrameCfg.eFormat)
    {
        s32Ret = ST_WARP_GetInjectBufYuv420(pHandle, pFrameBuf);
    }

    return s32Ret;
}

void ST_WARP_FinishInjectBufYuv422(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf, MI_BOOL bValidData)
{
    ST_WARP_InjectHandle_t *pstInject = (ST_WARP_InjectHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv = pFrameBuf->pPriv;

    if(!pPriv)
        return;

    MI_SYS_ChnInputPortPutBuf(pPriv->bufHandle, &pPriv->stBufInfo, !bValidData);
    free(pPriv);
}

void ST_WARP_FinishInjectBufYuv420(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf, MI_BOOL bValidData)
{
    ST_WARP_InjectHandle_t *pstInject = (ST_WARP_InjectHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv = pFrameBuf->pPriv;

    if(!pPriv)
        return;

    MI_SYS_ChnInputPortPutBuf(pPriv->bufHandle, &pPriv->stBufInfo, !bValidData);
    free(pPriv);
}

void ST_WARP_FinishInjectBuf(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf, MI_BOOL bValidData)
{
    ST_WARP_InjectHandle_t *pstInject = (ST_WARP_InjectHandle_t*)pHandle;
    BUG_ON(pstInject->stBufConf.eBufType != E_MI_SYS_BUFDATA_FRAME);

    if(pstInject->stBufConf.stFrameCfg.eFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        ST_WARP_FinishInjectBufYuv422(pHandle, pFrameBuf, bValidData);
    }
    else if(pstInject->stBufConf.stFrameCfg.eFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        ST_WARP_FinishInjectBufYuv420(pHandle, pFrameBuf, bValidData);
    }
}

MI_S32 ST_WARP_WaitSinkBuf(void *pHandle, MI_U32 u32Timeout)
{
    ST_WARP_SinkHandle_t *pstSink=(ST_WARP_SinkHandle_t *)pHandle;
    struct pollfd pfd[1] = {
            {pstSink->s32Fd, POLLIN|POLLERR},
        };
    MI_S32 s32Rval = poll(pfd, 1, u32Timeout);

    if(s32Rval>0 && (pfd[0].revents & POLLIN))
    {
        printf("Get data !!!\n");
        return 0;
    }
    else
        return -1;
}

MI_S32 ST_WARP_GetSinkBuf(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf)
{
    MI_S32 s32Ret = -1;
    ST_WARP_SinkHandle_t *pstSink = (ST_WARP_SinkHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv;
    pFrameBuf->pPriv = NULL;
    pPriv = malloc(sizeof(ST_WARP_FramePriv_t));

    if(!pPriv)
        goto exit;

    memset(pPriv, 0, sizeof(ST_WARP_FramePriv_t));

    if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&pstSink->stSysChnPort, &pPriv->stBufInfo, &pPriv->bufHandle))
    {
        goto free_priv;
    }

    BUG_ON(pPriv->stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME);
    pFrameBuf->pPriv = pPriv;

    printf("[%d] getSinlBuf\n", __LINE__);

    if(pPriv->stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
    {
        pFrameBuf->s32Planenum = 1;
        pFrameBuf->stPlane[0].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[0];
        pFrameBuf->stPlane[0].u32Width = pPriv->stBufInfo.stFrameData.u16Width*2;
        pFrameBuf->stPlane[0].u32Height = pPriv->stBufInfo.stFrameData.u16Height;
        pFrameBuf->stPlane[0].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[0];
        pFrameBuf->stPlane[0].phy = pPriv->stBufInfo.stFrameData.phyAddr[0];
    }
    else if(pPriv->stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        printf("[%d] get nv12 data, w:%d h:%d\n", __LINE__, pPriv->stBufInfo.stFrameData.u16Width, pPriv->stBufInfo.stFrameData.u16Height);
        pFrameBuf->s32Planenum = 2;
        pFrameBuf->stPlane[0].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[0];
        pFrameBuf->stPlane[0].u32Width = pPriv->stBufInfo.stFrameData.u16Width;
        pFrameBuf->stPlane[0].u32Height = pPriv->stBufInfo.stFrameData.u16Height;
        pFrameBuf->stPlane[0].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[0];
        pFrameBuf->stPlane[0].phy = pPriv->stBufInfo.stFrameData.phyAddr[0];

        pFrameBuf->stPlane[1].pAddr = pPriv->stBufInfo.stFrameData.pVirAddr[1];
        pFrameBuf->stPlane[1].u32Width = pPriv->stBufInfo.stFrameData.u16Width;
        pFrameBuf->stPlane[1].u32Height = pPriv->stBufInfo.stFrameData.u16Height/2;
        pFrameBuf->stPlane[1].u32Stride = pPriv->stBufInfo.stFrameData.u32Stride[1];
        pFrameBuf->stPlane[1].phy = pPriv->stBufInfo.stFrameData.phyAddr[1];
    }
    else
    {
        BUG_ON(1);
    }

    s32Ret=0;
    goto exit;
free_priv:
    free(pPriv);
exit:
    return s32Ret;
}

void ST_WARP_FinishSinkBuf(void *pHandle, ST_WARP_FrameBuf_t *pFrameBuf)
{
    ST_WARP_SinkHandle_t *pstSink = (ST_WARP_SinkHandle_t *)pHandle;
    ST_WARP_FramePriv_t *pPriv = pFrameBuf->pPriv;

    if(!pPriv)
        return;

    MI_SYS_ChnOutputPortPutBuf(pPriv->bufHandle);
    free(pPriv);
}

static void _ST_WARP_InjectProcess(ST_WARP_Moudule_t *pstMod)
{
    MI_S32 i;
    MI_S32 s32Ret;
    ST_WARP_InjectInfo_t *pstInjectInfo;
    ST_WARP_FrameBuf_t stFrameBuf;
    MI_BOOL bValidData = FALSE;
    MI_S32 s32FilePos = 0;
    pstInjectInfo = &pstMod->stInjectInfo;
    pthread_mutex_lock(&pstMod->mtxInject);

    if(!pstInjectInfo->bEnable)
    {
        printf("Not enable !\n");
    }

    s32Ret = pstInjectInfo->GetInjectBuf(pstInjectInfo->pHandle, &stFrameBuf);
    if(s32Ret < 0)
    {
        goto continue_inject;
    }

    s32FilePos = ST_WARP_GetCurFilePos(pstInjectInfo->s32Fd);
    s32Ret = ST_WARP_ReadToFrame(pstInjectInfo->s32Fd, &stFrameBuf);
    if(s32Ret < 0)
    {
        goto finish_inj_buf;
    }

    bValidData = TRUE;
    pstInjectInfo->s32FrmCnt++;
    printf("Get  frame cnt %d plane %d! Wid %d Height %d stride %d phy %llx\n", pstInjectInfo->s32FrmCnt, stFrameBuf.s32Planenum,
        stFrameBuf.stPlane[0].u32Width, stFrameBuf.stPlane[0].u32Height, stFrameBuf.stPlane[0].u32Stride, stFrameBuf.stPlane[0].phy);
    printf("plane %d! Wid %d Height %d stride %d phy %llx\n", stFrameBuf.s32Planenum, stFrameBuf.stPlane[1].u32Width, stFrameBuf.stPlane[1].u32Height,
        stFrameBuf.stPlane[1].u32Stride, stFrameBuf.stPlane[1].phy);

finish_inj_buf:
    pstInjectInfo->FinishInjectBuf(pstInjectInfo->pHandle, &stFrameBuf, bValidData);

continue_inject:
    pthread_mutex_unlock(&pstMod->mtxInject);

    usleep(1000);
}

static void* _ST_WARP_InjectThreadWork(void *pData)
{
    ST_WARP_Moudule_t *pstMod = (ST_WARP_Moudule_t*)pData;
    prctl(PR_SET_NAME, "inject_thread");

    while(1)
    {
loop_again:
        if(pstMod->u32InjectFlag & ST_THREAD_FLAG_STOP)
        {
            DBG_INFO("inject thread exit\n");
            break;
        }

        if(pstMod->u32InjectFlag & ST_THREAD_FLAG_PAUSE)
        {
            DBG_INFO("inject thread pause\n");
            pthread_mutex_lock(&pstMod->mtxInject);

            while(pstMod->u32InjectFlag & ST_THREAD_FLAG_PAUSE)
            {
                pthread_cond_wait(&pstMod->condInject, &pstMod->mtxInject);
            }

            pthread_mutex_unlock(&pstMod->mtxInject);
            DBG_INFO("inject thread resume\n");
            goto loop_again;
        }
        _ST_WARP_InjectProcess(pstMod);
    }
    return NULL;
}

static void _ST_WARP_SinkProcess(ST_WARP_Moudule_t *pstMod)
{
    ST_WARP_SinkInfo_t *pstSinkInfo = &pstMod->stSinkInfo;
    MI_S32 s32Ret;
    ST_WARP_FrameBuf_t stFrameBuf;
    pthread_mutex_lock(&pstMod->mtxSink);

    if(!pstSinkInfo->bEnable)
        goto exit;

    s32Ret = pstSinkInfo->WaitSinkBuf(pstSinkInfo->pHandle, 100);

    if(s32Ret < 0)
        goto exit;

    s32Ret = pstSinkInfo->GetSinkBuf(pstSinkInfo->pHandle, &stFrameBuf);

    if(s32Ret < 0)
        goto exit;

    s32Ret = ST_WARP_WriteFromFrame(pstSinkInfo->s32Fd, &stFrameBuf);
    if(s32Ret < 0)
    {
        DBG_ERR("write to frame err\n");
    }

    pstSinkInfo->FinishSinkBuf(pstSinkInfo->pHandle, &stFrameBuf);
    pstSinkInfo->s32FrmCnt++;
exit:
    pthread_mutex_unlock(&pstMod->mtxSink);
    usleep(1000);
}

static void* _ST_WARP_SinkThreadWork(void *pData)
{
    ST_WARP_Moudule_t *pstMod = (ST_WARP_Moudule_t*)pData;
    prctl(PR_SET_NAME,"sink_thread");

    while(1)
    {
loop_again:
        if(pstMod->u32SinkFlag & ST_THREAD_FLAG_STOP)
        {
            DBG_INFO("sink thread exit\n");
            break;
        }

        if(pstMod->u32SinkFlag & ST_THREAD_FLAG_PAUSE)
        {
            DBG_INFO("sink thread pause\n");
            pthread_mutex_lock(&pstMod->mtxSink);

            while(pstMod->u32SinkFlag & ST_THREAD_FLAG_PAUSE)
            {
                pthread_cond_wait(&pstMod->condSink, &pstMod->mtxSink);
            }

            pthread_mutex_unlock(&pstMod->mtxSink);
            DBG_INFO("sink thread resume\n");
            goto loop_again;
        }

        _ST_WARP_SinkProcess(pstMod);
    }
    return NULL;
}

static void _ST_WARP_PauseInjectThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxInject);
    _gstWarpModule.u32InjectFlag |= ST_THREAD_FLAG_PAUSE;
    pthread_mutex_unlock(&_gstWarpModule.mtxInject);
}

static void _ST_WARP_PauseSinkThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxSink);
    _gstWarpModule.u32SinkFlag |= ST_THREAD_FLAG_PAUSE;
    pthread_mutex_unlock(&_gstWarpModule.mtxSink);
}
static void _ST_WARP_ResumeInjectThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxInject);
    _gstWarpModule.u32InjectFlag &= ~ST_THREAD_FLAG_PAUSE;
    pthread_cond_signal(&_gstWarpModule.condInject);
    pthread_mutex_unlock(&_gstWarpModule.mtxInject);
}
static void _ST_WARP_ResumeSinkThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxSink);
    _gstWarpModule.u32SinkFlag &= ~ST_THREAD_FLAG_PAUSE;
    pthread_cond_signal(&_gstWarpModule.condSink);
    pthread_mutex_unlock(&_gstWarpModule.mtxSink);
}

static void _ST_WARP_StopInjectThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxInject);
    _gstWarpModule.u32InjectFlag |= ST_THREAD_FLAG_STOP;
    _gstWarpModule.u32InjectFlag &= ~ST_THREAD_FLAG_PAUSE;
    pthread_cond_signal(&_gstWarpModule.condInject);
    pthread_mutex_unlock(&_gstWarpModule.mtxInject);
    pthread_join(_gstWarpModule.threadInject, NULL);
}

static void _ST_WARP_StopSinkThread(void)
{
    pthread_mutex_lock(&_gstWarpModule.mtxSink);
    _gstWarpModule.u32SinkFlag |= ST_THREAD_FLAG_STOP;
    _gstWarpModule.u32SinkFlag &= ~ST_THREAD_FLAG_PAUSE;
    pthread_cond_signal(&_gstWarpModule.condSink);
    pthread_mutex_unlock(&_gstWarpModule.mtxSink);
    pthread_join(_gstWarpModule.threadSink, NULL);
}

int ST_WARP_Module_Init(void)
{
    MI_S32 s32Ret = -1;
    pthread_mutex_lock(&_gmtxWarpModule);

    if(_gstWarpModule.bInited)
        goto exit;

    memset(&_gstWarpModule.stInjectInfo,0, sizeof(ST_WARP_InjectInfo_t));
    memset(&_gstWarpModule.stSinkInfo, 0, sizeof(ST_WARP_SinkInfo_t));
    _gstWarpModule.u32InjectFlag = ST_THREAD_FLAG_PAUSE;
    _gstWarpModule.u32SinkFlag = ST_THREAD_FLAG_PAUSE;

    if(pthread_create(&_gstWarpModule.threadInject, NULL, _ST_WARP_InjectThreadWork, &_gstWarpModule) < 0)
    {
        _gstWarpModule.threadInject = 0;
        goto exit;
    }

    if(pthread_create(&_gstWarpModule.threadSink, NULL, _ST_WARP_SinkThreadWork, &_gstWarpModule) < 0)
    {
        _gstWarpModule.threadSink = 0;
        goto stop_inject_thread;
    }

    _gstWarpModule.bInited = TRUE;
    s32Ret = 0;
    goto exit;

stop_inject_thread:
    if(_gstWarpModule.threadInject)
    {
        _ST_WARP_StopInjectThread();
    }

exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return s32Ret;
}

int ST_WARP_Module_Deinit(void)
{
    pthread_mutex_lock(&_gmtxWarpModule);
    if(!_gstWarpModule.bInited)
        goto exit;

    if(_gstWarpModule.threadInject)
    {
        _ST_WARP_StopInjectThread();
    }

    if(_gstWarpModule.threadSink)
    {
        _ST_WARP_StopSinkThread();
    }

    _gstWarpModule.bInited = FALSE;
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
}

int ST_WARP_EnableInject(MI_S32 s32Fd, pfnGetInjectBuf pfnGetBuf, pfnFinishInjectBuf pfnFinishBuf, void *pHandle)
{
    MI_S32 s32Ret = -1;
    ST_WARP_InjectInfo_t *pstInjectInfo;
    pthread_mutex_lock(&_gmtxWarpModule);

    if(!_gstWarpModule.bInited)
        goto exit;

    pthread_mutex_lock(&_gstWarpModule.mtxInject);
    pstInjectInfo = &_gstWarpModule.stInjectInfo;

    if(pstInjectInfo->bEnable)
        goto exit_inj;

    pstInjectInfo->s32Fd = s32Fd;
    pstInjectInfo->GetInjectBuf = pfnGetBuf;
    pstInjectInfo->FinishInjectBuf = pfnFinishBuf;
    pstInjectInfo->pHandle = pHandle;
    pstInjectInfo->s32FrmCnt = 0;
    pstInjectInfo->bEnable = TRUE;
    s32Ret = 0;
exit_inj:
    pthread_mutex_unlock(&_gstWarpModule.mtxInject);
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);

    return s32Ret;
}

MI_S32 ST_WARP_DisableInject(void)
{
    MI_S32 s32Ret = -1;
    ST_WARP_InjectInfo_t *pstInjectInfo;

    pthread_mutex_lock(&_gmtxWarpModule);

    if(!_gstWarpModule.bInited)
        goto exit;

    pthread_mutex_lock(&_gstWarpModule.mtxInject);
    pstInjectInfo = &_gstWarpModule.stInjectInfo;

    if(!pstInjectInfo->bEnable)
        goto exit_inj;

    pstInjectInfo->bEnable = FALSE;
    s32Ret = 0;
exit_inj:
    pthread_mutex_unlock(&_gstWarpModule.mtxInject);
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return s32Ret;
}

MI_S32 ST_WARP_EnableSink(MI_S32 s32Fd, pfnWaitSinkBuf pfnWaitBuf, pfnGetSinkBuf pfnGetBuf
                            , pfnFinishSinkBuf pfnFinishBuf, void *pHandle)
{
    MI_S32 s32Ret = -1;
    ST_WARP_SinkInfo_t *pstSinkInfo;
    pthread_mutex_lock(&_gmtxWarpModule);

    if(!_gstWarpModule.bInited)
        goto exit;

    pthread_mutex_lock(&_gstWarpModule.mtxSink);
    pstSinkInfo = &_gstWarpModule.stSinkInfo;

    if(pstSinkInfo->bEnable)
        goto exit_sink;

    pstSinkInfo->s32Fd = s32Fd;
    pstSinkInfo->GetSinkBuf = pfnGetBuf;
    pstSinkInfo->FinishSinkBuf = pfnFinishBuf;
    pstSinkInfo->WaitSinkBuf = pfnWaitBuf;
    pstSinkInfo->pHandle = pHandle;
    pstSinkInfo->s32FrmCnt = 0;
    pstSinkInfo->bEnable = TRUE;
    s32Ret = 0;
exit_sink:
    pthread_mutex_unlock(&_gstWarpModule.mtxSink);
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return s32Ret;
}

MI_S32 ST_WARP_DisableSink(void)
{
    MI_S32 s32Ret = -1;
    ST_WARP_SinkInfo_t *pstSinkInfo;
    pthread_mutex_lock(&_gmtxWarpModule);

    if(!_gstWarpModule.bInited)
        goto exit;

    pthread_mutex_lock(&_gstWarpModule.mtxSink);
    pstSinkInfo = &_gstWarpModule.stSinkInfo;

    if(!pstSinkInfo->bEnable)
        goto exit_sink;

    pstSinkInfo->bEnable = FALSE;
    s32Ret = 0;
exit_sink:
    pthread_mutex_unlock(&_gstWarpModule.mtxSink);
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return s32Ret;
}

int ST_WARP_Start(void)
{
    pthread_mutex_lock(&_gmtxWarpModule);
    if(!_gstWarpModule.bInited)
        goto exit;

    if(_gstWarpModule.threadInject)
    {
        _ST_WARP_ResumeInjectThread();
    }

    if(_gstWarpModule.threadSink){
        _ST_WARP_ResumeSinkThread();
    }
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return 0;
}

int ST_WARP_Stop(void)
{
    pthread_mutex_lock(&_gmtxWarpModule);

    if(!_gstWarpModule.bInited)
        goto exit;

    if(_gstWarpModule.threadInject)
    {
        _ST_WARP_PauseInjectThread();
    }

    if(_gstWarpModule.threadSink)
    {
        _ST_WARP_PauseSinkThread();
    }
exit:
    pthread_mutex_unlock(&_gmtxWarpModule);
    return 0;
}


// cmd: [0:porg]        1 inject 1sink
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret;
    MI_S32 s32InjectFd;     // inject file handle
    MI_S32 s32SinkFd;       // sink file handle
    MI_SYS_ChnPort_t stWarpChnInputPort;
    MI_SYS_ChnPort_t stWarpChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    ST_WARP_InjectHandle_t stInjectHandle;
    ST_WARP_SinkHandle_t stSinkHandle;
    MI_SYS_ChnPort_t ChnPort;
    time_t stTime = 0;

    char szInputPath[32];
    char szOutputPath[32];
    memset(szInputPath, 0, sizeof(szInputPath));
    memset(szOutputPath, 0, sizeof(szOutputPath));

    if (argc < 2)
    {
        printf("please input src file index\n");
        return -1;
    }

    sprintf(szInputPath, "input/%d.bin", atoi(argv[1]));
    sprintf(szOutputPath, "output/%d.bin", atoi(argv[1]));

    printf("inputFile:%s, outputFile:%s\n", szInputPath, szOutputPath);

    printf("Warp test\n");

    // 1. open injectFile & sinkFile
    //if (!ST_WARP_OpenFile(_gszInjectYuvPath, FALSE, &s32InjectFd))
    if (!ST_WARP_OpenFile(szInputPath, FALSE, &s32InjectFd))
    {
        printf("Open inject file failed\n");
        return -1;
    }

    //if (!ST_WARP_OpenFile(_gszSinkYuvPath, TRUE, &s32SinkFd))
    if (!ST_WARP_OpenFile(szOutputPath, TRUE, &s32SinkFd))
    {
        printf("Open sink file failed\n");
        ST_WARP_CloseFile(s32InjectFd);
        return -1;
    }

    // init warp test thread
    if(ST_WARP_Module_Init() < 0)
    {
        DBG_ERR("warp_test_init fail\n");
        goto close_file;
    }

    s32Ret = MI_SYS_Init();
    if(MI_SUCCESS != s32Ret)
    {
        DBG_ERR("MI_SYS_Init fail\n");
        goto exit_deinit;
    }

    // 1. create dev&chn
    s32Ret = MI_WARP_CreateDevice(0);
    if (MI_WARP_OK != s32Ret)
    {
        DBG_ERR("MI_WARP_CreateDevice failed\n");
        goto exit_sys;
    }
    s32Ret = MI_WARP_StartDev(0);
    if (MI_WARP_OK != s32Ret)
    {
        DBG_ERR("MI_WARP_StartDev failed\n");
        goto exit_sys;
    }
    WARP_FUNC_EXEC(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, _gszBbTablePath));
    WARP_FUNC_EXEC(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, _gszDisplaceTablePath));

    s32Ret = MI_WARP_CreateChannel(0, 0);
    if (MI_WARP_OK != s32Ret)
    {
        DBG_ERR("MI_WARP_CreateChannel failed\n");
        goto destroy_device;
    }
    printf("create dev=0, chn=0\n");

    // set outputport depth
    ChnPort.eModId = E_MI_MODULE_ID_WARP;
    ChnPort.u32DevId = 0;
    ChnPort.u32ChnId = 0;
    ChnPort.u32PortId = 0;
    s32Ret = MI_SYS_SetChnOutputPortDepth(&ChnPort, 5, 20);

    stInjectHandle.stSysChnPort.eModId = E_MI_MODULE_ID_WARP;
    stInjectHandle.stSysChnPort.u32DevId = 0;
    stInjectHandle.stSysChnPort.u32ChnId = 0;
    stInjectHandle.stSysChnPort.u32PortId = 0;
    stInjectHandle.stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInjectHandle.stBufConf.u32Flags = 0;
    stInjectHandle.stBufConf.u64TargetPts = time(&stTime);
    stInjectHandle.stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420; //E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stInjectHandle.stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stInjectHandle.stBufConf.stFrameCfg.u16Width = 320; //3840;
    stInjectHandle.stBufConf.stFrameCfg.u16Height = 240; //2160;
    s32Ret = ST_WARP_EnableInject(s32InjectFd, ST_WARP_GetInjectBuf, ST_WARP_FinishInjectBuf, &stInjectHandle);

    if(s32Ret < 0)
    {
        DBG_ERR("ST_WARP_EnableInject fail\n");
        goto destroy_channel;
    }

    stSinkHandle.stSysChnPort.eModId = E_MI_MODULE_ID_WARP;
    stSinkHandle.stSysChnPort.u32DevId = 0;
    stSinkHandle.stSysChnPort.u32ChnId = 0;
    stSinkHandle.stSysChnPort.u32PortId = 0;
    s32Ret = MI_SYS_GetFd(&stSinkHandle.stSysChnPort, &stSinkHandle.s32Fd);

    if(MI_SUCCESS != s32Ret || stSinkHandle.s32Fd<0)
    {
        DBG_ERR("MI_SYS_GetFd fail\n");
        goto disable_inject;
    }

    s32Ret = ST_WARP_EnableSink(s32SinkFd, ST_WARP_WaitSinkBuf, ST_WARP_GetSinkBuf, ST_WARP_FinishSinkBuf, &stSinkHandle);

    if(s32Ret < 0)
    {
        DBG_ERR("ST_WARP_EnableSink fail\n");
        goto close_fd;
    }

    s32Ret = ST_WARP_Start();
    if(s32Ret < 0)
    {
        DBG_ERR("vdisp_test_start fail\n");
        goto disable_sink;
    }

    // 3. run warp flow
    s32Ret = MI_WARP_StartDev(0);
    if (MI_WARP_OK != s32Ret)
    {
        DBG_ERR("MI_WARP_StartDev dev 0 failed\n");
        goto stop_warp_test;
    }

    // wait for quit command, if true jion thread
    while(1)
    {
        MI_S8 ch = getchar();
        if(ch == 'q')
        {
            break;
        }
    }

stop_warp_dev:
    WARP_FUNC_EXEC(MI_WARP_StopDev(0));
stop_warp_test:
    ST_WARP_Stop();
    DBG_INFO("ST_WARP_Stop\n");
disable_sink:
    ST_WARP_DisableSink();
    DBG_INFO("ST_WARP_DisableSink\n");
close_fd:
    MI_SYS_CloseFd(stSinkHandle.s32Fd);
    DBG_INFO("MI_SYS_CloseFd\n");
disable_inject:
    ST_WARP_DisableInject();
    DBG_INFO("ST_WARP_DisableInject\n");
destroy_channel:
    MI_WARP_DestroyChannel(0, 0);
    DBG_INFO("MI_WARP_DestroyChannel\n");
destroy_device:
    MI_WARP_DestroyDevice(0);
    DBG_INFO("MI_WARP_DestroyDevice\n");
exit_sys:
    MI_SYS_Exit();
    DBG_INFO("MI_SYS_Exit\n");
exit_deinit:
    ST_WARP_Stop();
    DBG_INFO("ST_WARP_Stop\n");
close_file:
    if(s32InjectFd >= 0)
    {
        ST_WARP_CloseFile(s32InjectFd);
        DBG_INFO("ST_WARP_CloseFile: close inject file\n");
    }

    if(s32SinkFd >= 0)
    {
        ST_WARP_CloseFile(s32SinkFd);
        DBG_INFO("ST_WARP_CloseFile: close sink file\n");
    }
exit:
    return s32Ret;
}




/*

#define MAX_TEST 4
int main(int argc, const char *argv[])
{
    int ret;
    int test_index = 1;
    if(argc>=2)
    {
        ret = sscanf(argv[1], "%d", &test_index);
        if(ret <= 0)
            test_index = 1;
    }

    switch (test_index)
    {
        case 1:
            DBG_INFO("Start Test 001:\n");
            ret = main_001(argc, argv);
            DBG_INFO("End Test 001\n");
            break;
        case 2:
            DBG_INFO("Start Test 002:\n");
            ret = main_002(argc, argv);
            DBG_INFO("End Test 002\n");
            break;
        case 3:
            DBG_INFO("Start Test 003:\n");
            ret = main_003(argc, argv);
            DBG_INFO("End Test 003\n");
            break;
        case 4:
            DBG_INFO("Start Test 004:\n");
            ret = main_004(argc, argv);
            DBG_INFO("End Test 004\n");
            break;
        default:
            DBG_INFO("Start Test 001:\n");
            ret = main_001(argc, argv);
            DBG_INFO("End Test 001\n");
            break;
    }

    return ret;
}

*/
