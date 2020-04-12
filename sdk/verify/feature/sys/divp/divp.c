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
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    pthread_t outTid;
    pthread_t inTid;
    MI_BOOL bInThreadRunning;
    MI_BOOL bOutThreadRunning;
    MI_S8 inputName[128];

    MI_U32 inputW;
    MI_U32 inputH;
    MI_U32 outputW;
    MI_U32 outputH;
} STUB_StreamRes_t;

#define STUB_VPE_OUTPUT_PORT_NUM 4
#define STUB_VENC_CHN_NUM 4
#define STUB_DIVP_CHN_NUM 2

static STUB_StreamRes_t _stStubStreamRes[STUB_VPE_OUTPUT_PORT_NUM];
static MI_U32 _u32StreamCount = 1;
static MI_BOOL _bChnEnable[32];

void test_saveSnapshot(char *buff, int buffLen, char *name)
{
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
        printf("u16Width = %u, u16Height = %u, u32BytesPerLine = %u, u32LineNum = %u. u32FrameDataSize = %u \n",
            pstBufInfo->stFrameData.u16Width, pstBufInfo->stFrameData.u16Height, u32BytesPerLine, u32LineNum, u32LineNum);
    }
    else
    {
        printf("######  error ######. bRet = %u\n", bRet);
        return bRet;
    }

    printf(" read input frame data : pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
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

    printf(" u32YSize = %u, u32UVSize = %u. bRet = %u\n", u32YSize, u32UVSize, bRet);

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
        printf("u16Width = %u, u16Height = %u, u32BytesPerLine = %u, u32LineNum = %u. u32FrameDataSize = %u \n",
            pstBufInfo->stFrameData.u16Width, pstBufInfo->stFrameData.u16Height, u32BytesPerLine, u32LineNum, u32FrameDataSize);
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

    printf("u32ReadSize = %d. bRet = %u\n", u32ReadSize, bRet);
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
        printf(" read input frame data : pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
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

    printf(" bRet = %u.\n", bRet);
    return bRet;
}

void *venc_channel_input_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 count = 0;
    MI_SYS_BufConf_t stInputBufConf;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHdl = 0;
    FILE *pInputFile = NULL;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = pstStubStreamRes->u32DevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32ChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32PortId;

    stInputBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInputBufConf.u32Flags = 0x80000000;   //0 or MI_SYS_MAP_VA=0x80000000
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

    //MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while(pstStubStreamRes->bInThreadRunning)
    {
        count++;
        hSysBuf = MI_HANDLE_NULL;
        usleep(1000 * 1000);

        if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stChnPort,&stInputBufConf,&stInputBufInfo,&hInputBufHdl, 1))
    {
        printf("@@ nChnId = %d, buffer phyaddr = 0x%llx, pVirAddr[0] = 0x%lx, pVirAddr[1] = 0x%lx, pVirAddr[2] = 0x%lx @@\n",
                stChnPort.u32ChnId, stInputBufInfo.stFrameData.phyAddr[0],stInputBufInfo.stFrameData.pVirAddr[0],
                stInputBufInfo.stFrameData.pVirAddr[1],stInputBufInfo.stFrameData.pVirAddr[2]);

        printf("@@ nChnId = %d, res: %u*%u\n", stChnPort.u32ChnId, stInputBufInfo.stFrameData.u16Width, stInputBufInfo.stFrameData.u16Height);

        ///get data from file.
        if(!mi_divp_GetDivpInputFrameData(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, pInputFile, &stInputBufInfo))
        {
            fclose(pInputFile);
            pInputFile = 0;
            printf(" read input frame data failed!.\n");
            return NULL;
        }

        //set buffer to divp
        if(MI_SUCCESS == MI_SYS_ChnInputPortPutBuf (hInputBufHdl,  &stInputBufInfo, false))
        {
            printf(" MI_SYS_ChnInputPortPutBuf OK.\n");
        }
        else
        {
            printf(" MI_SYS_ChnInputPortPutBuf fail.\n");
        }
    }
    else
    {
        printf("MI_SYS_ChnInputPortGetBuf fail.\n");
    }
		
    }

    if(NULL != pInputFile)
    {
        fclose(pInputFile);
    }
    return NULL;
}

void *venc_channel_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 count = 0;
    MI_SYS_BufConf_t stInputBufConf;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHdl = 0;
    FILE *pInputFile = NULL;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = pstStubStreamRes->u32DevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32ChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32PortId;

    stInputBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInputBufConf.u32Flags = 0x80000000;   //0 or MI_SYS_MAP_VA=0x80000000
    stInputBufConf.u64TargetPts = 0x12340000;
    stInputBufConf.stFrameCfg.u16Width = pstStubStreamRes->inputW;
    stInputBufConf.stFrameCfg.u16Height = pstStubStreamRes->inputH;
    stInputBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stInputBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while(pstStubStreamRes->bOutThreadRunning)
    {
        count++;
        hSysBuf = MI_HANDLE_NULL;
        usleep(30 * 1000);
		
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        while(1)
        {
            if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
            {
                usleep(1000);
                continue;
            }
            else
            {
                break;
            }
        }

        //printf("vpe eModId = %u, u32DevId = %u, u32ChnId = %u,  u32PortId = %u, addr = %p, len = %u\n",
	//		 stChnPort.eModId, stChnPort.u32DevId, stChnPort.u32ChnId, stChnPort.u32PortId, stBufInfo.stRawData.pVirAddr, );

        printf("vpe getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p) size %u\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
            stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
            stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2], stBufInfo.stFrameData.u32BufSize);

        if(count % 10 == 0)
        {
            test_saveSnapshot(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, "yuv");
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
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
    printf("\n s32Ret = %d", s32Ret);
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

static MI_S32 STUB_BaseModuleInit(void)
{
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;

    MI_S32 s32Ret = MI_SUCCESS;
    MI_DIVP_CHN u32ChnId = 0;
    MI_DIVP_ChnAttr_t stAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_SYS_BufConf_t stInputBufConf;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHdl = 0;
    FILE *pInputFile = NULL;
    char ch;
    MI_U32 i = 0;
	
    MI_SYS_Init();

    for(i = 0; i < _u32StreamCount; i++)
    {
    memset(&stAttr, 0, sizeof(stAttr));
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    memset(&stInputBufConf, 0, sizeof(stInputBufConf));
    memset(&stInputBufInfo, 0, sizeof(stInputBufInfo));

    u32ChnId = i;
    stAttr.bHorMirror = false;
    stAttr.bVerMirror = false;
    stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stAttr.stCropRect.u16X = 0;
    stAttr.stCropRect.u16Y = 0;
    stAttr.stCropRect.u16Width = _stStubStreamRes[i].inputW;
    stAttr.stCropRect.u16Height = _stStubStreamRes[i].inputH;
    stAttr.u32MaxWidth = _stStubStreamRes[i].inputW;
    stAttr.u32MaxHeight = _stStubStreamRes[i].inputH;

    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width = _stStubStreamRes[i].outputW;
    stOutputPortAttr.u32Height = _stStubStreamRes[i].outputH;

    stInputBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInputBufConf.u32Flags = 0x80000000;   //0 or MI_SYS_MAP_VA=0x80000000
    stInputBufConf.u64TargetPts = 0x12340000;
    stInputBufConf.stFrameCfg.u16Width = _stStubStreamRes[i].inputW;
    stInputBufConf.stFrameCfg.u16Height = _stStubStreamRes[i].inputH;
    stInputBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stInputBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    mi_divp_CreateChannel(u32ChnId, _stStubStreamRes[i].inputW, _stStubStreamRes[0].inputH, E_MI_DIVP_DI_TYPE_OFF);
    mi_divp_test_SetChnAttr(u32ChnId, &stAttr);
    mi_divp_test_SetOutputPortAttr(u32ChnId, &stOutputPortAttr);
    

    /************************************************
    Step7:  create thread to get venc output buffer
    *************************************************/
    u32DivpDevId = 0;
    u32DivpChnId = i;
    u32DivpPortId = 0;
    //for(u32VencChnId=0; u32VencChnId<1; ++u32VencChnId)
    {
        _stStubStreamRes[i].u32DevId = u32DivpDevId;
        _stStubStreamRes[i].u32ChnId = u32DivpChnId;
        _stStubStreamRes[i].u32PortId = u32DivpPortId;
        _stStubStreamRes[i].bOutThreadRunning = TRUE;
         pthread_create(&_stStubStreamRes[i].outTid, NULL, venc_channel_func, &_stStubStreamRes[i]);

         _stStubStreamRes[i].bInThreadRunning = TRUE;
         pthread_create(&_stStubStreamRes[i].inTid, NULL, venc_channel_input_func, &_stStubStreamRes[i]);
    }
    }

    printf("press q to exit\n");
    while((ch = getchar()) != 'q')
    {
    }
    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_U32 u32DivpChnId = 0, i = 0;
    for(i = 0; i < _u32StreamCount; i++)
    {
    _stStubStreamRes[i].bInThreadRunning = FALSE;
    _stStubStreamRes[i].bOutThreadRunning = FALSE;
    pthread_join(_stStubStreamRes[i].inTid, NULL);
    pthread_join(_stStubStreamRes[i].outTid, NULL);
    _stStubStreamRes[i].inTid = 0;
    _stStubStreamRes[i].outTid = 0;

    STCHECKRESULT(MI_DIVP_StopChn(u32DivpChnId));
    STCHECKRESULT(MI_DIVP_DestroyChn(u32DivpChnId));
    }

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

void display_help()
{

    printf("\n");
    printf("Usage: prog [-n channel count] [-f filename] [-i input resolution] [-o output resolution]\n\n");
    printf("\t\t-h Displays this help\n");
    printf("\t\t-n                                : channel count\n");
    printf("\t\t-f                                : input yuv file name. ex: -f test.yuv\n");
    printf("\t\t-i input yuv resolution    : width_height_width_height, customized resolution, for example\n");
    printf("\t\t                                  : 1920_1080_720_720, divp0  1920*1080, divp1 720*720\n");
    printf("\t\t-o out yuv resolution    : width_height_width_height, customized resolution, for example\n");
    printf("\t\t                                  : 1280_720_640_480, divp0  1280*720, divp1 640*480\n");
    printf("\t\t                                  : for example\n");
    printf("\t\t                                  : ./prog -n 3 -f test.yuv_test.yuv_test.yuv -i 1920_1080_1920_1080_1920_1080 -o 1280_720_640_480_704_576\n");
}
MI_S32 main(int argc, char **argv)
{
    MI_S32 result, i, width, height;
    char *str = NULL;

    _u32StreamCount = 1;
    for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM; i++)
    {
        _stStubStreamRes[i].inputW = 1920;
        _stStubStreamRes[i].inputH = 1080;
        _stStubStreamRes[i].outputW = 1280;
        _stStubStreamRes[i].outputH = 720;
        strcpy(_stStubStreamRes[i].inputName, "test.yuv");
    }
    while((result = getopt(argc, argv, "f:i:n:o:h")) != -1)
    {
        switch(result)
        {
        case 'f':
            {
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    strcpy(_stStubStreamRes[i].inputName, optarg);
                    str = strtok(optarg, "_");
                }
            }
            break;
        case 'i':
            {
                str = strtok(optarg, "_");

                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
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

                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
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
        default:
            display_help();
            break;
        }
    }

    for(i = 0; i < _u32StreamCount; i++)
    {
        printf("input: %u*%u, output: %u*%u, inputfile: %s\n", _stStubStreamRes[i].inputW, _stStubStreamRes[i].inputH,
			_stStubStreamRes[i].outputW,  _stStubStreamRes[i].outputH, _stStubStreamRes[i].inputName);
    }

    STCHECKRESULT(STUB_BaseModuleInit());

    STCHECKRESULT(STUB_BaseModuleDeinit());
    return 0;
}

