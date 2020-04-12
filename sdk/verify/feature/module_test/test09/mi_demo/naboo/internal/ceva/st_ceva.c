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
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "st_common.h"
#include "st_ceva.h"
#include "i2c.h"

// include cdnn header
#include "CDNNCommonInterface.h"
#include "CDNNUserInterface.h"
#include "mi_ive.h"
#include "mem_teller.h"
#include "yolo.h"
#include "dummy_data.h"         // rect result


#define DEBUG_ON    1
#if DEBUG_ON
#define DEBUG_PRINT     printf
#else
#define DEBUG_PRINT
#endif

//#define DUMP_OUTPUTPORTBUF_TO_FILE
#define FAKE_CDNN_FRAMEWORK

#define FRAME_WIDTH         608
#define FRAME_HEIGHT        352
#define BOX_MAX_NUM         32

#define SRC_NAME_0  "src_img_0"
#define SRC_NAME_1  "src_img_1"
#define DST_NAME_0  "dst_img_0"

typedef void* CDNN_HANDLE;

typedef struct
{
    MI_S32 s32Fd;
    MI_SYS_ChnPort_t stSinkChnPort;

}ST_SinkHandle_t;

typedef struct
{
    MI_U16 u16LeftTopX;
    MI_U16 u16LeftTopY;
    MI_U16 u16RightBottomX;
    MI_U16 u16RightBottomY;
}ST_DisplayRect_t;

typedef struct
{
    CDNN_HANDLE hCdnnHandle;
    MI_IVE_HANDLE hIveHandle;
    mem_teller stTeller;
    MI_IVE_SrcImage_t stSrcImage;
    MI_IVE_DstImage_t stDstImage;
    BBox aBox[BOX_MAX_NUM];
    ST_DisplayRect_t stDisplayRect[BOX_MAX_NUM];
}ST_CDNN_Algo_t;

typedef struct
{
    MI_BOOL bUsrGet;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    pthread_mutex_t mtxChn;
    MI_U16 u16PreviewWidth;
    MI_U16 u16PreviewHeight;
    MI_U16 u16CdnnWidth;
    MI_U16 u16CdnnHeight;
}ST_CDNN_FrameInfo_t;

const char _gszDeviceName[] = "/dev/ceva_linkdrv_xm6_0";        // fixed
const char _gszImagePath[] = "image.bin";                       // dsp_bootup_load
float _gafAnchors[18] = {9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5, 9.5,9.5};
YoloOutputParam _gYoloParam = {
        .img_height = FRAME_HEIGHT,
        .img_width = FRAME_WIDTH,
        .height = FRAME_HEIGHT/32,
        .width = FRAME_WIDTH/32,
        .n_box = 9,
        .n_class = 1,
        .prob_thresh = 0.4,
        .nms_thresh = 0.5,
        .anchors = _gafAnchors
    };

ST_CDNN_Algo_t _gstCdnnAlgo;
ST_SinkHandle_t _gastCvSinkHandle[MI_VPE_MAX_CHANNEL_NUM];
struct pollfd _gaFdSet[MI_VPE_MAX_CHANNEL_NUM];
ST_CDNN_FrameInfo_t _gastFrameInfo[MI_VPE_MAX_CHANNEL_NUM];

MI_U32 _gu32ActiveFdCnt = 0;
pthread_mutex_t _gmtxFdSet;

pthread_t _gpGetBufThread;
MI_BOOL _gbGetBufThreadExit = FALSE;
MI_BOOL _gbGetBufThreadPause = TRUE;

pthread_t _gpCalcThread;
MI_BOOL _gbCalcThreadExit = FALSE;
MI_BOOL _gbCalcThreadPause = TRUE;



int dsp_boot_up(const char *image_name, void *working_buffer)
{
#ifdef FAKE_CDNN_FRAMEWORK
    return 0;
#else
    struct dsp_image image;
    int img_handle, dsp_handle, size, ret = 0;

    // check file size
    struct stat st;
    if (stat(image_name, &st))  // samba方式使用会异常
    {
        printf("stat fail : %s, %s\n", image_name, strerror(errno));
        return -1;
    }

    if (st.st_size == 0)
    {
        printf("image size is 0\n");
        return -1;
    }

    // allocate temporary buffer for image
    image.size = st.st_size;
    image.data = malloc(image.size);
    if (image.data == NULL)
    {
        printf("can't allocate buffer for image\n");
        return -1;
    }

    ///////////////////////////////////////////////////////////
    // read image

    img_handle = open(image_name, O_RDONLY);
    if (img_handle <= 0)
    {
        printf("Can't open image %s (%d: %s)\n", image_name, errno, strerror(errno));
        ret = -1;
        goto RETURN_0;
    }

    size = read(img_handle, image.data, image.size);
    if (size != image.size)
    {
        printf("only read %d but size is %d\n", size, image.size);
        ret = -1;
        goto RETURN_1;
    }

    ///////////////////////////////////////////////////////////
    // boot up DSP

    dsp_handle = open(_gszDeviceName, O_RDWR);
    if (dsp_handle < 0)
    {
        printf("can't open device %s\n", _gszDeviceName);
        ret = -1;
        goto RETURN_1;
    }

    ret = ioctl(dsp_handle, IOC_CEVADRV_BOOT_UP, &image);
    if (ret != 0)
    {
        printf("DSP does not boot up\n");
        ret = -1;
        goto RETURN_2;
    }

    printf("DSP boot up\n");

RETURN_2:
    close(dsp_handle);

RETURN_1:
    close(img_handle);

RETURN_0:
    free(image.data);

    return ret;
#endif
}

void* cdnn_create(const char *model_name)
{
#if 0
    int status = 0;
    void *cdnn_handle = NULL;

    cdnnBringUpInfo_st cdnnBringUpInfo = { 0 };
    cdnnTargetInfo_st cdnnTargetInfo;

    cdnnTargetInfo.eDeviceType = E_CDNN_DEVICE_USER_XM6;
    cdnnBringUpInfo.NumberOfTargets = 1;
    cdnnBringUpInfo.pTargetsInformation = &cdnnTargetInfo;
    status = CDNNCreate(&cdnn_handle, &cdnnBringUpInfo);
    if (status)
    {
        printf("Failed to call CDNNCreate(), ret is %d\n", status);
        return NULL;
    }

    // we do not create a real network so far because driver is not ready

    return cdnn_handle;
#endif
    return 0;
}

void cdnn_release(void *cdnn_handle)
{
}

int ive_buffer_create(mem_teller *teller, MI_IVE_SrcImage_t *src, MI_IVE_DstImage_t *dst)
{
    int ret = 0;

    memset(src, 0, sizeof(MI_IVE_Image_t));
    src->eType = E_MI_IVE_IMAGE_TYPE_YUV420SP;
    src->u16Width  = FRAME_WIDTH;
    src->u16Height = FRAME_HEIGHT;
    src->azu16Stride[0] = FRAME_WIDTH;
    src->azu16Stride[1] = FRAME_WIDTH;
    src->azu16Stride[2] = FRAME_WIDTH;

    ret = mem_teller_alloc(teller, &src->aphyPhyAddr[0], FRAME_WIDTH*FRAME_HEIGHT, SRC_NAME_0);
    if (ret != 0) goto RETURN_0;

    ret = mem_teller_mmap(teller, src->aphyPhyAddr[0], &src->apu8VirAddr[0], FRAME_WIDTH*FRAME_HEIGHT);
    if (ret != 0) goto RETURN_1;

    ret = mem_teller_alloc(teller, &src->aphyPhyAddr[1], FRAME_WIDTH*FRAME_HEIGHT/2, SRC_NAME_1);
    if (ret != 0) goto RETURN_2;

    ret = mem_teller_mmap(teller, src->aphyPhyAddr[1], &src->apu8VirAddr[1], FRAME_WIDTH*FRAME_HEIGHT/2);
    if (ret != 0) goto RETURN_3;

    memset(dst, 0, sizeof(MI_IVE_Image_t));
    dst->eType = E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE;
    dst->u16Width  = FRAME_WIDTH;
    dst->u16Height = FRAME_HEIGHT;
    dst->azu16Stride[0] = FRAME_WIDTH;
    dst->azu16Stride[1] = FRAME_WIDTH;
    dst->azu16Stride[2] = FRAME_WIDTH;

    ret = mem_teller_alloc(teller, &dst->aphyPhyAddr[0], FRAME_WIDTH*FRAME_HEIGHT*3, DST_NAME_0);
    if (ret != 0) goto RETURN_4;

    ret = mem_teller_mmap(teller, dst->aphyPhyAddr[0], &dst->apu8VirAddr[0], FRAME_WIDTH*FRAME_HEIGHT*3);
    if (ret != 0) goto RETURN_5;

    return ret;

RETURN_5:
    mem_teller_free(teller, dst->aphyPhyAddr[0], DST_NAME_0);

RETURN_4:
    mem_teller_unmmap(teller, src->apu8VirAddr[1], FRAME_WIDTH*FRAME_HEIGHT/2);

RETURN_3:
    mem_teller_free(teller, src->aphyPhyAddr[1], SRC_NAME_1);

RETURN_2:
    mem_teller_unmmap(teller, src->apu8VirAddr[0], FRAME_WIDTH*FRAME_HEIGHT);

RETURN_1:
    mem_teller_free(teller, src->aphyPhyAddr[0], SRC_NAME_0);

RETURN_0:
    return ret;
}

void ive_buffer_release(mem_teller *teller, MI_IVE_SrcImage_t *src, MI_IVE_DstImage_t *dst)
{
    mem_teller_unmmap(teller, dst->apu8VirAddr[0], FRAME_WIDTH*FRAME_HEIGHT*3);
    mem_teller_free(teller, dst->aphyPhyAddr[0], DST_NAME_0);
    mem_teller_unmmap(teller, src->apu8VirAddr[1], FRAME_WIDTH*FRAME_HEIGHT/2);
    mem_teller_free(teller, src->aphyPhyAddr[1], SRC_NAME_1);
    mem_teller_unmmap(teller, src->apu8VirAddr[0], FRAME_WIDTH*FRAME_HEIGHT);
    mem_teller_free(teller, src->aphyPhyAddr[0], SRC_NAME_0);
}

void _ST_CEVA_GetChnPortFormFd(MI_S32 s32Fd, MI_SYS_ChnPort_t *pstChnPort)
{
    MI_S32 i = 0;

    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        if (s32Fd == _gastCvSinkHandle[i].s32Fd)
        {
            *pstChnPort = _gastCvSinkHandle[i].stSinkChnPort;
            break;
        }
    }

//    DEBUG_PRINT("ChnPort info: devId=%d chnId=%d portId=%d\n", pstChnPort->u32DevId, pstChnPort->u32ChnId,
//                 pstChnPort->u32PortId);
}

void *_ST_CEVA_CdnnGetBufThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32TimeOutMs = 20;
    void *pFrameDataBuf[2];
    MI_U32 u32BufSize = 0;
    MI_S32 s32Ret;
    MI_S32 i, S32TotalBox;

#ifdef DUMP_OUTPUTPORTBUF_TO_FILE
    FILE *paFile[MI_VPE_MAX_CHANNEL_NUM] = NULL;
    char szaFilePath[MI_VPE_MAX_CHANNEL_NUM][16];
    MI_S32 s32SampleForTest = 200;

    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        memset(szaFilePath[i], 0, 16);
        sprintf(szaFilePath[i], "ceva_sub.yuv_%d", i);

        paFile[i] = fopen(szaFilePath[i], "ab+");
        if (!paFile[i])
        {
            DEBUG_PRINT("Ceva_Thread: Create ceva_sub %d file error\n", i);
        }
    }
#endif

    DEBUG_PRINT("_ST_CEVA_CdnnGetBufThread\n");

    while (1)
    {
#ifdef DUMP_OUTPUTPORTBUF_TO_FILE
        if (s32SampleForTest-- <= 0)
        {
            _gbGetBufThreadExit = TRUE;
        }
#endif

        if(_gbGetBufThreadExit)
        {
            DEBUG_PRINT("Ceva_Thread: Ceva work stop\n");
#ifdef DUMP_OUTPUTPORTBUF_TO_FILE
            for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
            {
                if (paFile[i])
                {
                    fclose(paFile[i]);
                    paFile[i] = NULL;
                }
            }
#endif
            break;
        }

        if (_gbGetBufThreadPause)
        {
            usleep(20000);
            continue;
        }

        // recv date

        pthread_mutex_lock(&_gmtxFdSet);
        s32Ret = -1;
        if (_gu32ActiveFdCnt > 0)
        {
//            DEBUG_PRINT("active fd cnt=%d\n", _gu32ActiveFdCnt);

            s32Ret = poll(_gaFdSet, _gu32ActiveFdCnt, s32TimeOutMs);
//            DEBUG_PRINT("poll ret=%d _gu32ActiveFdCnt=%d\n", s32Ret, _gu32ActiveFdCnt);
            if (s32Ret > 0)
            {
                for (i = 0; i < _gu32ActiveFdCnt; i++)
                {
                    if (_gaFdSet[i].revents & POLLIN)
                    {
                        _ST_CEVA_GetChnPortFormFd(_gaFdSet[i].fd, &stChnPort);
                        break;
                    }
                }
            }
        }

        pthread_mutex_unlock(&_gmtxFdSet);

        if (s32Ret > 0)
        {
            // get data
            pthread_mutex_lock(&_gastFrameInfo[stChnPort.u32ChnId].mtxChn);

            if (_gastFrameInfo[stChnPort.u32ChnId].bUsrGet)
            {
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                _gastFrameInfo[stChnPort.u32ChnId].bUsrGet = FALSE;
            }

            s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle);

            DEBUG_PRINT("getBufThread: MI_SYS_ChnOutputPortGetBuf ret=%d\n", s32Ret);

            if (MI_SUCCESS ==  s32Ret)
            {
                _gastFrameInfo[stChnPort.u32ChnId].stBufInfo = stBufInfo;
                _gastFrameInfo[stChnPort.u32ChnId].hHandle = hHandle;
                _gastFrameInfo[stChnPort.u32ChnId].bUsrGet = TRUE;
            }
            pthread_mutex_unlock(&_gastFrameInfo[stChnPort.u32ChnId].mtxChn);

            // dump buf
#ifdef DUMP_OUTPUTPORTBUF_TO_FILE
            if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == stBufInfo.stFrameData.ePixelFormat)
            {
                u32BufSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u16Width;
                MI_SYS_Mmap(stBufInfo.stFrameData.phyAddr[0], u32BufSize, &pFrameDataBuf[0], FALSE);
                MI_SYS_Mmap(stBufInfo.stFrameData.phyAddr[1], u32BufSize, &pFrameDataBuf[1], FALSE);

                DEBUG_PRINT("chn=%d, hgt=%d, stride0=%d, size=%d fp=%p vir0=%p phy0=%llx vir1=%p phy1=%llx\n"           ,
                            stChnPort.u32ChnId, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0],
                            u32BufSize, paFile[stChnPort.u32ChnId], stBufInfo.stFrameData.pVirAddr[0],
                            stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.phyAddr[1]);

                if (paFile[stChnPort.u32ChnId] && u32BufSize && pFrameDataBuf[0] && pFrameDataBuf[1])
                {
                    fwrite(pFrameDataBuf[0], u32BufSize, 1, paFile[stChnPort.u32ChnId]);
                    fwrite(pFrameDataBuf[1], u32BufSize/2, 1, paFile[stChnPort.u32ChnId]);
                }
            }
            else if (E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == stBufInfo.stFrameData.ePixelFormat)   // 422
            {
                u32BufSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                s32Ret = MI_SYS_Mmap(stBufInfo.stFrameData.phyAddr[0], u32BufSize, &pFrameDataBuf[0], FALSE);
                DEBUG_PRINT("chn=%d, hgt=%d, stride0=%d, size=%d fp=%p vir0=%p phy0=%lld pWriteBuf0=%p ret=%d\n",
                            stChnPort.u32ChnId, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0],
                            u32BufSize, paFile[stChnPort.u32ChnId], stBufInfo.stFrameData.pVirAddr[0],
                            stBufInfo.stFrameData.phyAddr[0], pFrameDataBuf[0], s32Ret);

                if (paFile[stChnPort.u32ChnId] && u32BufSize && pFrameDataBuf[0] && (MI_SUCCESS==s32Ret))
                {
                    s32Ret = fwrite(pFrameDataBuf[0], u32BufSize, 1, paFile[stChnPort.u32ChnId]);
                    // DEBUG_PRINT("write %d bytes\n", s32Ret);
                }
            }
            else
            {
                DEBUG_PRINT("Ceva_Thread: Frame format not support\n");
            }
#endif
        }

        usleep(200000);
    }

#ifdef DUMP_OUTPUTPORTBUF_TO_FILE
    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        if (paFile[i])
        {
            fclose(paFile[i]);
            paFile[i] = NULL;
        }
    }

    DEBUG_PRINT("Close dump file\n");
#endif

    return NULL;
}


void *_ST_CEVA_CdnnCalcThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_S32 S32TotalBox;
    MI_U32 i = 0;
    MI_U32 u32ActiveCnt = 0;
    MI_BOOL bDataValid = TRUE;
    MI_S32 s32CurFd = 0;
    MI_S32 s32FdIndex = 0;
    MI_S32 s32CurChn = 0;

    DEBUG_PRINT("_ST_CEVA_CdnnCalcThread\n");

    while (1)
    {
        if (_gbCalcThreadExit)
        {
            break;
        }

        if (_gbCalcThreadPause)
        {
            usleep(20000);
            continue;
        }

        pthread_mutex_lock(&_gmtxFdSet);
        u32ActiveCnt = _gu32ActiveFdCnt;

        if (_gu32ActiveFdCnt > 0)
        {
            if (s32FdIndex >= _gu32ActiveFdCnt)
                s32FdIndex = 0;

            s32CurFd = _gaFdSet[s32FdIndex].fd;
//             DEBUG_PRINT("CalcThread: curFd=%d\n", s32CurFd);
        }
        else
        {
            usleep(10000);
            pthread_mutex_unlock(&_gmtxFdSet);
            continue;
        }

        pthread_mutex_unlock(&_gmtxFdSet);

        _ST_CEVA_GetChnPortFormFd(s32CurFd, &stChnPort);
        s32CurChn = stChnPort.u32ChnId;
//        DEBUG_PRINT("CalcThread: curChn=%d\n", s32CurChn);

        pthread_mutex_lock(&_gastFrameInfo[s32CurChn].mtxChn);
        if (_gastFrameInfo[s32CurChn].bUsrGet)
        {
            DEBUG_PRINT("CalcThread: get buff curChn=%d\n", s32CurChn);
            bDataValid = TRUE;

            // 1. transfer YUV420/YUV422 to YUV444 by IVE
            if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == _gastFrameInfo[s32CurChn].stBufInfo.stFrameData.ePixelFormat)
            {
                // 1. convert YUYV to 444
            }
            else if (E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == _gastFrameInfo[s32CurChn].stBufInfo.stFrameData.ePixelFormat)   // 422
            {
                // 1. convert YUYV to 444
            }
            else
            {
                DEBUG_PRINT("Ceva_Thread: Frame format not support\n");
                bDataValid = FALSE;
            }

            if (bDataValid)
            {
                // 2. calculate to generator a result

                // 3. parse result to x-y of cdnn port
                S32TotalBox = GetBBox((float *)cdnn_result, _gYoloParam, _gstCdnnAlgo.aBox, BOX_MAX_NUM);

                if (S32TotalBox > BOX_MAX_NUM)
                {
                    DEBUG_PRINT("detect %d but only %d gotten\n", S32TotalBox, BOX_MAX_NUM);
                    S32TotalBox = BOX_MAX_NUM;
                }

                // 4. convert x-y of cdnn port to preview port's
                memset(_gstCdnnAlgo.stDisplayRect, 0, sizeof(_gstCdnnAlgo.stDisplayRect));

                for (i=0; i<S32TotalBox; i++)
                {
                    DEBUG_PRINT("prob = %.2f%%, top-left = (%4d, %4d), bottom-right = (%4d, %4d)\n",
                                _gstCdnnAlgo.aBox[i].prob*100, _gstCdnnAlgo.aBox[i].x_min, _gstCdnnAlgo.aBox[i].y_min,
                                _gstCdnnAlgo.aBox[i].x_max, _gstCdnnAlgo.aBox[i].y_max);

                    _gstCdnnAlgo.stDisplayRect[i].u16LeftTopX = _gstCdnnAlgo.aBox[i].x_min * _gastFrameInfo[s32CurChn].u16PreviewWidth
                                                                      / _gastFrameInfo[s32CurChn].u16CdnnWidth;
                    _gstCdnnAlgo.stDisplayRect[i].u16LeftTopY = _gstCdnnAlgo.aBox[i].y_min * _gastFrameInfo[s32CurChn].u16PreviewHeight
                                                                      / _gastFrameInfo[s32CurChn].u16CdnnHeight;
                    _gstCdnnAlgo.stDisplayRect[i].u16RightBottomX = _gstCdnnAlgo.aBox[i].x_max * _gastFrameInfo[s32CurChn].u16PreviewWidth
                                                                           / _gastFrameInfo[s32CurChn].u16CdnnWidth;
                    _gstCdnnAlgo.stDisplayRect[i].u16RightBottomY = _gstCdnnAlgo.aBox[i].y_max * _gastFrameInfo[s32CurChn].u16PreviewHeight
                                                                           / _gastFrameInfo[s32CurChn].u16CdnnHeight;

                    DEBUG_PRINT("%2d: top-left = (%4d, %4d), bottom-right = (%4d, %4d)\n", i, _gstCdnnAlgo.stDisplayRect[i].u16LeftTopX
                                , _gstCdnnAlgo.stDisplayRect[i].u16LeftTopY, _gstCdnnAlgo.stDisplayRect[i].u16RightBottomX,
                                _gstCdnnAlgo.stDisplayRect[i].u16RightBottomY);
                }

                // 5. create rgn to draw rect


            }

            MI_SYS_ChnOutputPortPutBuf(_gastFrameInfo[s32CurChn].hHandle);
            _gastFrameInfo[s32CurChn].bUsrGet = FALSE;
        }

        pthread_mutex_unlock(&_gastFrameInfo[s32CurChn].mtxChn);

        s32FdIndex++;
    }

    return NULL;
}

// initial fd list
MI_S32 ST_CEVA_Init()
{
    MI_S32 i = 0;

    _gbGetBufThreadExit = FALSE;
    _gbCalcThreadExit = FALSE;
    _gbGetBufThreadPause = TRUE;
    _gbCalcThreadPause = TRUE;

    memset(&_gstCdnnAlgo, 0 ,sizeof(ST_CDNN_Algo_t));
    memset(_gastCvSinkHandle, 0, sizeof(_gastCvSinkHandle));
    memset(_gastFrameInfo, 0, sizeof(_gastFrameInfo));
    memset(_gaFdSet, 0, sizeof(_gaFdSet));

    pthread_mutex_init(&_gmtxFdSet, NULL);

    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        pthread_mutex_init(&_gastFrameInfo[i].mtxChn, NULL);
        _gastFrameInfo[i].bUsrGet = FALSE;
        _gastFrameInfo[i].hHandle = 0;
        memset(&_gastFrameInfo[i].stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        _gastFrameInfo[i].u16PreviewWidth = 0;
        _gastFrameInfo[i].u16PreviewHeight = 0;
        _gastFrameInfo[i].u16CdnnWidth = 0;
        _gastFrameInfo[i].u16CdnnHeight = 0;

        _gaFdSet[i].events = POLLIN|POLLERR;
    }

    // 1. ive init
    // 1.1 create buf handle
    DEBUG_PRINT("1.1 create buf handle\n");
    if (mem_teller_create(&_gstCdnnAlgo.stTeller) != 0)
    {
        DEBUG_PRINT("can't create memory teller\n");
        goto INITIAL_FAILED;
    }

    // 1.2 create frame buffer
    DEBUG_PRINT("1.2 create frame buffer\n");
    if (ive_buffer_create(&_gstCdnnAlgo.stTeller, &_gstCdnnAlgo.stSrcImage, &_gstCdnnAlgo.stDstImage) != 0)
    {
        printf("can't create ive buffers\n");
        goto DESTROY_BUFHANDLE;
    }

    // 1.3 create ive handle
    DEBUG_PRINT("1.3 create ive handle\n");
    if (MI_IVE_Create(_gstCdnnAlgo.hIveHandle) != 0)
    {
        printf("Could not create IVE handle %d\n", _gstCdnnAlgo.hIveHandle);
        goto DESTROY_IVE_BUF;
    }

    // 2. boot up dsp
    DEBUG_PRINT("2. boot up dsp\n");
    if (dsp_boot_up("image.bin", NULL) != 0)
    {
        printf("can't boot up dsp\n");
        goto DESTROY_IVE_HANDLE;
    }

    // 3. cdnn init
    /* not ready yet...
    DEBUG_PRINT("3. cdnn init\n");
    cdnn_handle = cdnn_create(....);
    if (cdnn_handle == NULL)
    {
        printf("can't create dsp\n");
        goto DESTROY_IVE_HANDLE;
    }
    */
#if 1
    // 4. create cdnn workThread
    DEBUG_PRINT("4.1 create cdnn getBufThread\n");
    pthread_create(&_gpGetBufThread, NULL, _ST_CEVA_CdnnGetBufThread, NULL);  // save to buf list
    if (!_gpGetBufThread)
    {
        goto DESTROY_CDNN_HANDLE;
    }

    DEBUG_PRINT("4.2 create cdnn calcThread\n");
    pthread_create(&_gpCalcThread, NULL, _ST_CEVA_CdnnCalcThread, NULL);    // proc thread
    if (!_gpCalcThread)
    {
        goto DESTROY_GETBUF_THREAD;
    }

    return 0;

DESTROY_GETBUF_THREAD:
    pthread_join(_gpGetBufThread, NULL);
#endif

DESTROY_CDNN_HANDLE:
    cdnn_release(_gstCdnnAlgo.hCdnnHandle);

DESTROY_IVE_HANDLE:
    MI_IVE_Destroy(_gstCdnnAlgo.hIveHandle);

DESTROY_IVE_BUF:
    ive_buffer_release(&_gstCdnnAlgo.stTeller, &_gstCdnnAlgo.stSrcImage, &_gstCdnnAlgo.stDstImage);

DESTROY_BUFHANDLE:
    mem_teller_release(&_gstCdnnAlgo.stTeller);

INITIAL_FAILED:
    return -1;
}

// deinit fd list
MI_S32 ST_CEVA_Deinit()
{
    MI_S32 i = 0;

    if (_gpGetBufThread)
    {
        _gbGetBufThreadExit = TRUE;
        pthread_join(_gpGetBufThread, NULL);
    }

    if (_gpCalcThread)
    {
        _gbCalcThreadExit = TRUE;
        pthread_join(_gpCalcThread, NULL);
    }

    _gpGetBufThread = NULL;
    _gpCalcThread = NULL;

    // release cdnn
    cdnn_release(_gstCdnnAlgo.hCdnnHandle);

    // destroy ive handle
    MI_IVE_Destroy(_gstCdnnAlgo.hIveHandle);

    // free ive buf
    ive_buffer_release(&_gstCdnnAlgo.stTeller, &_gstCdnnAlgo.stSrcImage, &_gstCdnnAlgo.stDstImage);

    // release buf handle
    mem_teller_release(&_gstCdnnAlgo.stTeller);

    // param destroy
    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        pthread_mutex_destroy(&_gastFrameInfo[i].mtxChn);
        _gastFrameInfo[i].bUsrGet = FALSE;
        _gastFrameInfo[i].hHandle = 0;
        memset(&_gastFrameInfo[i].stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        _gastFrameInfo[i].u16PreviewWidth = 0;
        _gastFrameInfo[i].u16PreviewHeight = 0;
        _gastFrameInfo[i].u16CdnnWidth = 0;
        _gastFrameInfo[i].u16CdnnHeight = 0;
    }

    pthread_mutex_destroy(&_gmtxFdSet);

    memset(&_gstCdnnAlgo, 0 ,sizeof(ST_CDNN_Algo_t));
    memset(_gastCvSinkHandle, 0, sizeof(_gastCvSinkHandle));
    memset(_gaFdSet, 0, sizeof(_gaFdSet));

    DEBUG_PRINT("ST_CEVA_DeInit\n");

    return 0;
}

MI_S32 ST_CEVA_Start()
{
    if (_gpGetBufThread)
    {
        _gbGetBufThreadPause = FALSE;
    }

    if (_gpCalcThread)
    {
        _gbCalcThreadPause = FALSE;
    }

    return 0;
}

MI_S32 ST_CEVA_Stop()
{
    if (_gpGetBufThread)
    {
        _gbGetBufThreadPause = TRUE;
    }

    if (_gpCalcThread)
    {
        _gbCalcThreadPause = TRUE;
    }

    return 0;
}

// regitster chn, add port to listen fd list
MI_S32 ST_CEVA_RegisterChn(ST_CEVA_PortInfo_t *pstCdnnPort, ST_CEVA_ResolutionMap_t *pstResolutionMap)
{
    MI_S32 s32Fd = 0;
    MI_S32 i = 0;
    MI_S32 s32Ret;

    MI_SYS_SetChnOutputPortDepth(&pstCdnnPort->stChnPort, 3, 5);

    s32Ret = MI_SYS_GetFd(&pstCdnnPort->stChnPort, &s32Fd);
    if (MI_SUCCESS != s32Ret)
    {
        DEBUG_PRINT("get fd error, devId=%d, chn=%d, port=%d\n", pstCdnnPort->stChnPort.u32DevId, pstCdnnPort->stChnPort.u32ChnId,
                    pstCdnnPort->stChnPort.u32PortId);
        return -1;
    }

    DEBUG_PRINT("register port: chnId=%d portId=%d fd=%d\n", pstCdnnPort->stChnPort.u32ChnId, pstCdnnPort->stChnPort.u32PortId, s32Fd);

    _gastFrameInfo[pstCdnnPort->stChnPort.u32ChnId].u16PreviewWidth = pstResolutionMap->u16SrcWidth;
    _gastFrameInfo[pstCdnnPort->stChnPort.u32ChnId].u16PreviewHeight = pstResolutionMap->u16SrcHeight;
    _gastFrameInfo[pstCdnnPort->stChnPort.u32ChnId].u16CdnnWidth = pstResolutionMap->u16DstWidth;
    _gastFrameInfo[pstCdnnPort->stChnPort.u32ChnId].u16CdnnHeight = pstResolutionMap->u16DstHeight;

    _gastCvSinkHandle[pstCdnnPort->stChnPort.u32ChnId].s32Fd = s32Fd;
    _gastCvSinkHandle[pstCdnnPort->stChnPort.u32ChnId].stSinkChnPort = pstCdnnPort->stChnPort;

    pthread_mutex_lock(&_gmtxFdSet);
    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        if (_gaFdSet[i].fd == s32Fd)
        {
            DEBUG_PRINT("Port has been registered, devId=%d, chnId=%d, portId=%d\n", pstCdnnPort->stChnPort.u32DevId,
                         pstCdnnPort->stChnPort.u32ChnId, pstCdnnPort->stChnPort.u32PortId);
            pthread_mutex_unlock(&_gmtxFdSet);
            return MI_SUCCESS;
        }
        else if (_gaFdSet[i].fd <= 0)
        {
            _gaFdSet[i].fd = s32Fd;
            _gaFdSet[i].events = POLLIN|POLLERR;
            break;
        }
    }

    _gu32ActiveFdCnt++;
    pthread_mutex_unlock(&_gmtxFdSet);

    DEBUG_PRINT("Dump fdset:");
    for (i = 0; i < MI_VPE_MAX_CHANNEL_NUM; i++)
    {
        DEBUG_PRINT("%d ", _gaFdSet[i].fd);
    }
    DEBUG_PRINT("\n");

    return 0;
}


// unregister chn, remove port from fd list and resort fd list
MI_S32 ST_CEVA_UnRegisterChn(MI_SYS_ChnPort_t stCdnnPort)
{
    MI_S32 s32Fd = 0;
    MI_S32 i = 0;
    MI_S32 s32Ret;

    s32Ret = MI_SYS_GetFd(&stCdnnPort, &s32Fd);
    if (MI_SUCCESS != s32Ret)
    {
        DEBUG_PRINT("get fd error, devId=%d, chn=%d, port=%d\n", stCdnnPort.u32DevId,          stCdnnPort.u32ChnId,
                    stCdnnPort.u32PortId);
        return s32Ret;
    }

    pthread_mutex_lock(&_gmtxFdSet);
    for (i = 0; i < _gu32ActiveFdCnt; i++)
    {
        if (_gaFdSet[i].fd = s32Fd)
        {
            _gaFdSet[i].fd = 0;
            i--;
            break;
        }
    }

    for(;i < _gu32ActiveFdCnt; i++)
    {
        if (i == _gu32ActiveFdCnt - 1)
            _gaFdSet[i].fd = 0;
        else
            _gaFdSet[i].fd = _gaFdSet[i+1].fd;
    }

    if (_gu32ActiveFdCnt > 0)
        _gu32ActiveFdCnt--;

    pthread_mutex_unlock(&_gmtxFdSet);

    return 0;
}

