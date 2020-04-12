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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_divp.h"

#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }


typedef struct STUB_StreamRes_s
{
    MI_VENC_ModType_e eModType;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;
    pthread_t input_tid;
    pthread_t output_tid;
    MI_BOOL bInThreadRunning;
    MI_BOOL bOutThreadRunning;
    MI_S8 inputName[128];

    MI_U32 inputW;
    MI_U32 inputH;
    MI_U32 outputW;
    MI_U32 outputH;
} STUB_StreamRes_t;

#define STUB_VENC_CHN_NUM 4
#define STUB_DIVP_CHN_NUM 2

static STUB_StreamRes_t _stStubStreamRes[STUB_VENC_CHN_NUM];
static MI_U32 _u32StreamCount = 1;
static MI_U32 _u32FrameRate = 25;
static MI_VENC_ModType_e _eModType = E_MI_VENC_MODTYPE_H264E;
static MI_BOOL _bChnEnable[STUB_VENC_CHN_NUM];

void test_saveSnapshot(char *buff, int buffLen, char *name)
{
    return;
    char filename[64];
    struct timeval timestamp;
    gettimeofday(&timestamp, 0);
    printf("saveSnapshot%d_%08d.%s buffLen=%d\n", (int)timestamp.tv_sec, (int)timestamp.tv_usec, name, buffLen);
    sprintf(filename, "saveSnapshot%d_%08d.%s", (int)timestamp.tv_sec, (int)timestamp.tv_usec, name);
    FILE *pfile = fopen(filename, "wb");
    if(NULL == pfile)
    {
        printf("error: fopen %s failed\n", filename);
        return;
    }
    fwrite(buff, 1,  buffLen, pfile);
    fflush(pfile);
    fclose(pfile);
    sync();
}

MI_BOOL mi_divp_GetDivpInputFrameData420(FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = FALSE;
    MI_U32 u32LineNum = 0;
    MI_U32 u32BytesPerLine = 0;
    MI_U32 u32Index = 0;
    MI_U32 u32FrameDataSize = 0;
    MI_U32 u32YSize = 0;
    MI_U32 u32UVSize = 0;

    if (pInputFile == NULL)
    {
        printf("create file error.\n");
        return bRet;
    }

    if(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height * 3 / 2;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;

    }
    else
    {
        printf("######  error ######. bRet = %u\n", bRet);
        return bRet;
    }

    //printf(" read input frame data : pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
    for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
    {
        u32YSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
    }

    for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
    {
        u32UVSize += fread(pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], 1, u32BytesPerLine, pInputFile);
    }

    if(u32YSize + u32UVSize == u32FrameDataSize)
    {
        bRet = TRUE;
    }
    else if(u32YSize + u32UVSize < u32FrameDataSize)
    {
        fseek(pInputFile, 0, SEEK_SET);
        u32YSize = 0;
        u32UVSize = 0;

        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
        {
            u32YSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
        }

        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
        {
            u32UVSize += fread(pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], 1, u32BytesPerLine, pInputFile);
        }

        if(u32YSize + u32UVSize == u32FrameDataSize)
        {
            bRet = TRUE;
        }
        else
        {
            printf(" read file error. u32YSize = %u, u32UVSize = %u. \n", u32YSize, u32UVSize);
            bRet = FALSE;
        }
    }

    //printf(" u32YSize = %u, u32UVSize = %u. bRet = %u\n", u32YSize, u32UVSize, bRet);
    return bRet;
}

MI_BOOL mi_divp_GetDivpInputFrameDataYuv422(FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = FALSE;
    MI_U32 u32ReadSize = 0;
    MI_U32 u32LineNum = 0;
    MI_U32 u32BytesPerLine = 0;
    MI_U32 u32Index = 0;
    MI_U32 u32FrameDataSize = 0;

    if (pInputFile == NULL)
    {
        printf("create file error.\n");
        return bRet;
    }

    if(E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width * 2;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;
    }
    else
    {
        printf("######  error ######. ePixelFormat = %u\n", pstBufInfo->stFrameData.ePixelFormat);
        return bRet;
    }

    for (u32Index = 0; u32Index < u32LineNum; u32Index ++)
    {
        u32ReadSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
    }

    if(u32ReadSize == u32FrameDataSize)
    {
        bRet = TRUE;
    }
    else if(u32ReadSize < u32FrameDataSize)
    {
        fseek(pInputFile, 0, SEEK_SET);
        u32ReadSize = 0;

        for (u32Index = 0; u32Index < u32LineNum; u32Index ++)
        {
            u32ReadSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
        }

        if(u32ReadSize == u32FrameDataSize)
        {
            bRet = TRUE;
        }
        else
        {
            printf("read file error. u32ReadSize = %u. \n", u32ReadSize);
            bRet = FALSE;
        }
    }

    //printf("u32ReadSize = %d. bRet = %u\n", u32ReadSize, bRet);
    return bRet;
}

MI_BOOL mi_divp_GetDivpInputFrameData(MI_SYS_PixelFormat_e ePixelFormat, FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = TRUE;
    if((NULL == pInputFile) || (NULL == pstBufInfo))
    {
        bRet = FALSE;
        printf(" read input frame data failed! pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
    }
    else
    {
        //printf(" read input frame data : pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
        if(ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
        {
            if(!mi_divp_GetDivpInputFrameData420(pInputFile, pstBufInfo))
            {
                bRet = FALSE;
                printf(" read input frame data failed!.\n");
            }
        }
        else if(ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
        {
            if(!mi_divp_GetDivpInputFrameDataYuv422(pInputFile, pstBufInfo))
            {
                bRet = FALSE;
                printf(" read input frame data failed!.\n");
            }
        }
    }

    return bRet;
}

MI_S32 mi_divp_test_CreateChn(MI_DIVP_CHN DivpChn, MI_U32 u32MaxWidth, MI_U32 u32MaxHeight, MI_DIVP_DiType_e eDiType)
{
    MI_S32 s32Ret = -1;
    MI_DIVP_ChnAttr_t stAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    stAttr.bHorMirror = false;
    stAttr.bVerMirror = false;
    stAttr.eDiType = eDiType;
    stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stAttr.stCropRect.u16X = 0;
    stAttr.stCropRect.u16Y = 0;
    stAttr.stCropRect.u16Width = u32MaxWidth;
    stAttr.stCropRect.u16Height = u32MaxHeight;
    stAttr.u32MaxWidth = u32MaxWidth;
    stAttr.u32MaxHeight = u32MaxHeight;

    s32Ret = MI_DIVP_CreateChn(DivpChn, &stAttr);
    return s32Ret;
}

MI_S32 mi_divp_test_SetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_SetChnAttr(DivpChn, pstAttr));

    return s32Ret;
}

MI_S32 mi_divp_test_SetOutputPortAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t* pstOutputPortAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DivpChn, pstOutputPortAttr));

    return s32Ret;
}

MI_S32 mi_divp_test_StartChn(MI_DIVP_CHN DivpChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_StartChn(DivpChn));
    return s32Ret;
}

MI_S32 mi_divp_CreateChannel(MI_DIVP_CHN DivpChn, MI_U16 u16Width, MI_U16 u16Height, MI_DIVP_DiType_e eDiType)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    mi_divp_test_CreateChn(DivpChn, u16Width, u16Height, eDiType);
    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;//E_MI_SYS_PIXEL_FRAME_YUV_MST_420;//E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;//
    stOutputPortAttr.u32Width = u16Width;
    stOutputPortAttr.u32Height = u16Height;
    mi_divp_test_SetOutputPortAttr(DivpChn, &stOutputPortAttr);
    mi_divp_test_StartChn(DivpChn);

    //MI_SYS_SetChnOutputPortDepth(&gstDivpOutputPort[DivpChn], 2, 5);
    return s32Ret;
}

void *divp_send_frame(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stInputBufConf;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHdl = 0;
    MI_U32 u32HasSendFrames = 0;
    FILE *pInputFile = NULL;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = pstStubStreamRes->u32DivpDevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32DivpChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32DivpPortId;

    stInputBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInputBufConf.u32Flags = MI_SYS_MAP_VA;
    stInputBufConf.u64TargetPts = 0x12340000;
    stInputBufConf.stFrameCfg.u16Width = pstStubStreamRes->inputW;
    stInputBufConf.stFrameCfg.u16Height = pstStubStreamRes->inputH;
    stInputBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stInputBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    ///read input video stream
    pInputFile =fopen(pstStubStreamRes->inputName, "rb");
    if (pInputFile == NULL)
    {
        printf("open input file error. pchFileName = test.yuv\n");
        return NULL;
    }
    else
    {
        printf("input file name :  test.yuv.\n");
    }

    while(pstStubStreamRes->bInThreadRunning)
    {
        hInputBufHdl = MI_HANDLE_NULL;
        if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stChnPort,&stInputBufConf,&stInputBufInfo,&hInputBufHdl, 1000))
        {
            if(!mi_divp_GetDivpInputFrameData(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, pInputFile, &stInputBufInfo))
            {
                fclose(pInputFile);
                pInputFile = 0;
                printf(" read input frame data failed!.\n");
                return NULL;
            }

            if(MI_SUCCESS == MI_SYS_ChnInputPortPutBuf (hInputBufHdl,  &stInputBufInfo, false))
            {
                //printf(" MI_SYS_ChnInputPortPutBuf OK.\n");
            }
            else
            {
                //printf(" MI_SYS_ChnInputPortPutBuf fail.\n");
            }

            ++u32HasSendFrames;
            printf("u32HasSendFrames[%u]\n", u32HasSendFrames);
            usleep(1000/_u32FrameRate *1000);
        }
        else
        {
            usleep(10*1000);
        }

    }

    if(NULL != pInputFile)
    {
        fclose(pInputFile);
    }
    return NULL;
}

void *venc_handle_output(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32GetFramesCount;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = pstStubStreamRes->u32VencDevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32VencChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32VencPortId;

    u32GetFramesCount = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while(pstStubStreamRes->bOutThreadRunning)
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            usleep(10*1000);
            continue;
        }
        else
        {
            if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
            {
                continue;
            }

            ++u32GetFramesCount;
            printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);
            usleep(1000/_u32FrameRate *1000);
        }
    }

    return NULL;
}

static MI_S32 STUB_GetVencConfig(MI_VENC_ModType_e eModType, MI_VENC_ChnAttr_t *pstVencChnAttr, MI_U32 u32ChnId)
{
    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicWidth = _stStubStreamRes[u32ChnId].outputW;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicHeight = _stStubStreamRes[u32ChnId].outputH;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicWidth = _stStubStreamRes[u32ChnId].outputW;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicHeight = _stStubStreamRes[u32ChnId].outputH;
            pstVencChnAttr->stVeAttr.stAttrH264e.bByFrame = TRUE;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32Gop = 30;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32IQp = 25;
            pstVencChnAttr->stRcAttr.stAttrH264FixQp.u32PQp = 25;
        }
        break;
        case E_MI_VENC_MODTYPE_H265E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicWidth = _stStubStreamRes[u32ChnId].outputW;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicHeight = _stStubStreamRes[u32ChnId].outputH;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicWidth = _stStubStreamRes[u32ChnId].outputW;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicHeight = _stStubStreamRes[u32ChnId].outputH;
            pstVencChnAttr->stVeAttr.stAttrH265e.bByFrame = TRUE;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32Gop = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32IQp = 25;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32PQp = 25;
        }
        break;
        default:
            printf("unsupport eModType[%u]\n", eModType);
            return E_MI_ERR_FAILED;
    }

    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleInit(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;

    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;

    MI_U32 i = 0;
    char ch;

    MI_SYS_Init();

    u32VencDevId = 0;
    u32VencPortId = 0;
    u32DivpDevId = 0;
    u32DivpPortId = 0;
    for(i = 0; i < _u32StreamCount; i++)
    {
        memset(&stDivpChnAttr, 0, sizeof(stDivpChnAttr));
        memset(&stDivpOutputPortAttr, 0, sizeof(stDivpOutputPortAttr));

        stDivpChnAttr.bHorMirror = false;
        stDivpChnAttr.bVerMirror = false;
        stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X = 0;
        stDivpChnAttr.stCropRect.u16Y = 0;
        stDivpChnAttr.stCropRect.u16Width = _stStubStreamRes[i].inputW;
        stDivpChnAttr.stCropRect.u16Height = _stStubStreamRes[i].inputH;
        stDivpChnAttr.u32MaxWidth = _stStubStreamRes[i].inputW;
        stDivpChnAttr.u32MaxHeight = _stStubStreamRes[i].inputH;
        stDivpOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stDivpOutputPortAttr.u32Width = _stStubStreamRes[i].outputW;
        stDivpOutputPortAttr.u32Height = _stStubStreamRes[i].outputH;
        STCHECKRESULT(mi_divp_CreateChannel(i, _stStubStreamRes[i].inputW, _stStubStreamRes[i].inputH, E_MI_DIVP_DI_TYPE_OFF));
        STCHECKRESULT(mi_divp_test_SetChnAttr(i, &stDivpChnAttr));
        STCHECKRESULT(mi_divp_test_SetOutputPortAttr(i, &stDivpOutputPortAttr));

        memset(&stVencChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
        STUB_GetVencConfig(_eModType, &stVencChnAttr, i);
        STCHECKRESULT(MI_VENC_CreateChn(i, &stVencChnAttr));
        STCHECKRESULT(MI_VENC_StartRecvPic(i));

        //Bind DIVP(0, i, 0)->VENC(0, i, 0)
        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stSrcChnPort.u32DevId = u32DivpDevId;
        stSrcChnPort.u32ChnId = i;
        stSrcChnPort.u32PortId = u32DivpPortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = i;
        stDstChnPort.u32PortId = u32VencPortId;
        u32SrcFrmrate = 30;
        u32DstFrmrate = 30;
        eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));


        /************************************************
        Step3:  create thread to send/get frame thread
        *************************************************/
        _stStubStreamRes[i].eModType = E_MI_VENC_MODTYPE_H264E;
        _stStubStreamRes[i].u32DivpDevId = u32DivpDevId;
        _stStubStreamRes[i].u32DivpChnId = i;
        _stStubStreamRes[i].u32DivpPortId = u32DivpPortId;
        _stStubStreamRes[i].u32VencDevId = u32VencDevId;
        _stStubStreamRes[i].u32VencChnId = i;
        _stStubStreamRes[i].u32VencPortId = u32VencPortId;

        _stStubStreamRes[i].bInThreadRunning = TRUE;
        _stStubStreamRes[i].bOutThreadRunning = TRUE;
        pthread_create(&_stStubStreamRes[i].input_tid, NULL, divp_send_frame, &_stStubStreamRes[i]);
        pthread_create(&_stStubStreamRes[i].output_tid, NULL, venc_handle_output, &_stStubStreamRes[i]);
    }

    printf("press q to exit\n");
    while((ch = getchar()) != 'q')
    {
        printf("press q to exit\n");
        usleep(1*1000*1000);
    }

    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;

    MI_U32 u3ChnId;

    u32DivpDevId = 0;
    u32DivpPortId = 0;
    u32VencDevId = 0;
    u32VencPortId = 0;
    for(u3ChnId = 0; u3ChnId < _u32StreamCount; u3ChnId++)
    {
        _stStubStreamRes[u3ChnId].bInThreadRunning = FALSE;
        _stStubStreamRes[u3ChnId].bOutThreadRunning = FALSE;
        pthread_join(_stStubStreamRes[u3ChnId].input_tid, NULL);
        pthread_join(_stStubStreamRes[u3ChnId].output_tid, NULL);
        _stStubStreamRes[u3ChnId].input_tid= 0;
        _stStubStreamRes[u3ChnId].output_tid = 0;

        memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stSrcChnPort.u32DevId = u32DivpDevId;
        stSrcChnPort.u32ChnId = u3ChnId;
        stSrcChnPort.u32PortId = u32DivpPortId;
        stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stDstChnPort.u32DevId = u32VencDevId;
        stDstChnPort.u32ChnId = u3ChnId;
        stDstChnPort.u32PortId = u32VencPortId;
        STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

        STCHECKRESULT(MI_VENC_StopRecvPic(u3ChnId));
        STCHECKRESULT(MI_VENC_DestroyChn(u3ChnId));

        STCHECKRESULT(MI_DIVP_StopChn(u3ChnId));
        STCHECKRESULT(MI_DIVP_DestroyChn(u3ChnId));
    }

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

void display_help()
{

    printf("\n");
    printf("Usage: prog [-f filename] [-i input resolution] [-o output resolution][-n num of channel][-r frame rate]\n\n");
    printf("\t\t-h Displays this help\n");
    printf("\t\t-f                                : input yuv file name. ex: -f test.yuv\n");
    printf("\t\t-i input yuv resolution  : width_height_width_height, customized resolution, for example\n");
    printf("\t\t                                   : 1920_1080_720_720, divp0  1920*1080, divp1 720*720\n");
    printf("\t\t-o out yuv resolution    : width_height_width_height, customized resolution, for example\n");
    printf("\t\t                                   : 1280_720_640_480, divp0  1280*720, divp1 640*480\n");
    printf("\t\t-n  num of channel\n");
    printf("\t\t-r  frame rate\n");
    printf("\t\t-m : mod type. ex: 2:h264 3:h265 4:jpeg\n");

}
MI_S32 main(int argc, char **argv)
{
    MI_S32 result, i, width, height;
    char *str = NULL;

    _u32StreamCount = 1;
    for(i = 0; i < STUB_VENC_CHN_NUM; i++)
    {
        _stStubStreamRes[i].inputW = 1920;
        _stStubStreamRes[i].inputH = 1080;
        _stStubStreamRes[i].outputW = 1280;
        _stStubStreamRes[i].outputH = 720;
        strcpy(_stStubStreamRes[i].inputName, "test.yuv");
    }
    while((result = getopt(argc, argv, "f:r:i:o:n:m:h")) != -1)
    {
        switch(result)
        {
            case 'f':
            {
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VENC_CHN_NUM && str; i++)
                {
                    strcpy(_stStubStreamRes[i].inputName, optarg);
                    str = strtok(optarg, "_");
                }
            }
            break;
            case 'r':
            {
                str = strtok(optarg, "_");
                _u32FrameRate = atoi(str);
                printf("_u32FrameRate[%u]\n", _u32FrameRate);
            }
            break;
            case 'i':
            {
                str = strtok(optarg, "_");

                for(i = 0; i < STUB_VENC_CHN_NUM && str; i++)
                {
                    width = atoi(str);
                    str = strtok(NULL, "_");
                    height = atoi(str);
                    str = strtok(NULL, "_");
                    _stStubStreamRes[i].inputW = width;
                    _stStubStreamRes[i].inputH = height;
                }
            }
            break;
            case 'o':
            {
                str = strtok(optarg, "_");

                for(i = 0; i < STUB_VENC_CHN_NUM && str; i++)
                {
                    width = atoi(str);
                    str = strtok(NULL, "_");
                    height = atoi(str);
                    str = strtok(NULL, "_");
                    _stStubStreamRes[i].outputW = width;
                    _stStubStreamRes[i].outputH = height;
                }
            }
            break;
            case 'n':
            {
                str = strtok(optarg, "_");
                _u32StreamCount = atoi(str);
            }
            break;
            case 'm':
            {
                str = strtok(optarg, "_");
                _eModType= atoi(str);
                printf("_eModType[%u]\n", _eModType);
            }
            break;
            default:
                display_help();
                break;
        }
    }

    printf("_u32StreamCount[%u]\n", _u32StreamCount);
    for(i = 0; i < _u32StreamCount; i++)
    {
        printf("input: %u*%u, output: %u*%u, inputfile: %s\n", _stStubStreamRes[i].inputW, _stStubStreamRes[i].inputH,
               _stStubStreamRes[i].outputW,  _stStubStreamRes[i].outputH, _stStubStreamRes[i].inputName);
    }

    STCHECKRESULT(STUB_BaseModuleInit());
    STCHECKRESULT(STUB_BaseModuleDeinit());
    return 0;
}


