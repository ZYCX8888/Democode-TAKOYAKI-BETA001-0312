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
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "mi_venc.h"
#include "mi_sys.h"
#include "venc_common.h"
#include "env.h"

#define FPGA (0)
#define FPGA_DELAY_MS (1 * 1000)

#define IN_PORT_TIMEOUT (1000)
#define OUT_PORT_TIMEOUT IN_PORT_TIMEOUT

typedef enum {
    E_VENC_INPUT_FROM_NONE,
    E_VENC_INPUT_FROM_HEADER,
    E_VENC_INPUT_FROM_CODE,
    E_VENC_INPUT_FROM_FILE,
    E_VENC_INPUT_FROM_FILE_CACHE,
    E_VENC_INPUT_FROM_MAX
} VENC_InputType_e;

//==== configuration ====
#if BUILD_YUV_H == 288
    #include "NV12_cif_frames.h"
#elif BUILD_YUV_H == 240
    #include "NV12_qvga_10frame.h"
#endif

#if BUILD_YUV_H == 0
VENC_InputType_e eVencInputType = E_VENC_INPUT_FROM_NONE;
#else
#define VENC_NUM_OF_FRAMES (sizeof(gYUV) / ((BUILD_YUV_W * BUILD_YUV_H * 3) >> 1))
VENC_InputType_e eVencInputType = E_VENC_INPUT_FROM_HEADER;
#endif

const MI_U32 u32QueueLen = 10;
Chn_t _astChn[VENC_MAX_CHN];
MI_VENC_CHN s32CreateChnNum = 0;
MI_S32 s32FramesInFile;
MI_U32 u32CachedFsize = 0;
char *pFCache = NULL;
//MI_S32 s32InMsgLvl;

void _DumpSysBufInfo(MI_SYS_BufInfo_t *pstSysBufInfo, MI_SYS_BUF_HANDLE hBuf)
{
    printf("pstSysBufInfo EOS:%d USR:%d type:%d\n", pstSysBufInfo->bEndOfStream, pstSysBufInfo->bUsrBuf,
            pstSysBufInfo->eBufType);
    printf("pstSysBufInfo frame:pix %d res:%dP\n", pstSysBufInfo->stFrameData.ePixelFormat,
            pstSysBufInfo->stFrameData.u16Height);

    printf("pstSysBufInfo frame:addr %p %llX\n", pstSysBufInfo->stFrameData.pVirAddr[0],
            pstSysBufInfo->stFrameData.phyAddr[0]);
    printf("hHandle=%X\n", hBuf);
}

int _VENC_FillFrameData(MI_SYS_BufInfo_t *pstBufInfo, VENC_InputType_e eInputType, MI_U32 u32Frame)
{
    MI_U32 u32YSize;
    MI_U32 u32FrameSize;
    size_t size;
    MI_S32 YUV_W, YUV_H;
    MI_BOOL bErr;
    FILE *file;
    MI_S32 s32FrameInCurFile;
    MI_S32 s32MaxFrames;
    char *szFn;
    int ret = 0;

    if (pstBufInfo == NULL)
        return -1;

    YUV_W = get_cfg_int("VENC_GLOB_IN_W", &bErr);
    YUV_H = get_cfg_int("VENC_GLOB_IN_H", &bErr);
    u32YSize = YUV_W * YUV_H;
    u32FrameSize = (u32YSize >> 1) + u32YSize; //Y * 1.5
    //printf("type:%d size:%d\n", eVencInputType, u32YSize);
    //printf("sizeof YUV:%d, num:%d\n", sizeof(gYUV), VENC_NUM_OF_FRAMES);
    s32MaxFrames = get_cfg_int("VENC_GLOB_MAX_FRAMES", &bErr);

    switch (eInputType)
    {
    case E_VENC_INPUT_FROM_CODE:
        if((MI_S32)u32Frame >= s32MaxFrames - 1)
            pstBufInfo->bEndOfStream = TRUE;
        else
            pstBufInfo->bEndOfStream = FALSE;
        size = sprintf(pstBufInfo->stFrameData.pVirAddr[0], "INPUT FRAME %d", u32Frame);
        ret = (int)size;
        break;
    case E_VENC_INPUT_FROM_HEADER:
        s32FramesInFile = VENC_NUM_OF_FRAMES;
        s32FrameInCurFile = u32Frame % s32FramesInFile;
        if(((MI_S32)u32Frame) >= (MI_S32)(s32MaxFrames - 1))
        {
            u32Frame = s32MaxFrames - 1;
            pstBufInfo->bEndOfStream = TRUE;
        }
        else
        {
            pstBufInfo->bEndOfStream = FALSE;
        }
        //s32InMsgLvl = get_cfg_int("VENC_GLOB_IN_MSG_LEVEL", &bErr);
        if(VENC_GLOB_IN_MSG_LEVEL >= 1)
            printf("header[%d].eos=%d  ", u32Frame, pstBufInfo->bEndOfStream);
        if(VENC_GLOB_IN_MSG_LEVEL >= 2)
            printf("addr:%p %p %p\n", pstBufInfo->stFrameData.pVirAddr[0], pstBufInfo->stFrameData.pVirAddr[1],
                    pstBufInfo->stFrameData.pVirAddr[2]);
        //assume it's NV12.
        memcpy(pstBufInfo->stFrameData.pVirAddr[0], gYUV + s32FrameInCurFile * u32FrameSize, u32YSize);
        memcpy(pstBufInfo->stFrameData.pVirAddr[1], gYUV + s32FrameInCurFile * u32FrameSize + u32YSize, u32YSize >> 1);

        ret = u32FrameSize;
        break;
    case E_VENC_INPUT_FROM_FILE:
        s32FrameInCurFile = u32Frame;
        if(s32FrameInCurFile >= s32FramesInFile)
            s32FrameInCurFile = u32Frame % s32FramesInFile;

        s32MaxFrames = get_cfg_int("VENC_GLOB_MAX_FRAMES", &bErr);
        if(((MI_S32)u32Frame) >= (MI_S32)(s32MaxFrames - 1))
        {
            u32Frame = s32MaxFrames - 1;
            pstBufInfo->bEndOfStream = TRUE;
        }
        else
        {
            pstBufInfo->bEndOfStream = FALSE;
        }

        //s32InMsgLvl = get_cfg_int("VENC_GLOB_IN_MSG_LEVEL", &bErr);
        szFn = get_cfg_str("VENC_GLOB_IN_FILE", &bErr);
        file = fopen(szFn, "rb");
        if(file == NULL)
        {
            printf("Unable to open '%s'\n", szFn);
        }
        if(0 != fseek(file, s32FrameInCurFile * u32FrameSize, SEEK_SET))
        {
            pstBufInfo->bEndOfStream = TRUE;
        }
        else
        {
            if (u32YSize > fread(pstBufInfo->stFrameData.pVirAddr[0], 1, u32YSize, file))
                pstBufInfo->bEndOfStream = TRUE;
            if ((u32YSize >> 1) > fread(pstBufInfo->stFrameData.pVirAddr[1], 1, u32YSize >> 1, file))
                pstBufInfo->bEndOfStream = TRUE;
        }
        if(VENC_GLOB_IN_MSG_LEVEL >= 1)
            printf("frame[%d].eos=%d  ", u32Frame, pstBufInfo->bEndOfStream);
        if(VENC_GLOB_IN_MSG_LEVEL >= 2)
            printf("addr:%p %p %p\n", pstBufInfo->stFrameData.pVirAddr[0], pstBufInfo->stFrameData.pVirAddr[1],
                    pstBufInfo->stFrameData.pVirAddr[2]);
        fclose(file);

        ret = u32FrameSize;
        break;
    case E_VENC_INPUT_FROM_FILE_CACHE:
        if(pFCache == NULL)
        {
            printf("[ERR] No cached file!\n");
            return -3;
        }
        s32FrameInCurFile = u32Frame;
        if(s32FrameInCurFile >= s32FramesInFile)
            s32FrameInCurFile = u32Frame % s32FramesInFile;

        if(((MI_S32)u32Frame) >= (MI_S32)(s32MaxFrames - 1))
        {
            u32Frame = s32MaxFrames - 1;
            pstBufInfo->bEndOfStream = TRUE;
        }
        else
        {
            pstBufInfo->bEndOfStream = FALSE;
        }

        //s32InMsgLvl = get_cfg_int("VENC_GLOB_IN_MSG_LEVEL", &bErr);
        memcpy(pstBufInfo->stFrameData.pVirAddr[0], pFCache + s32FrameInCurFile * u32FrameSize, u32YSize);
        memcpy(pstBufInfo->stFrameData.pVirAddr[1], pFCache + s32FrameInCurFile * u32FrameSize + u32YSize, u32YSize >> 1);
        if(VENC_GLOB_IN_MSG_LEVEL >= 1)
            printf("$frame[%d].eos=%d  ", u32Frame, pstBufInfo->bEndOfStream);
        if(VENC_GLOB_IN_MSG_LEVEL >= 2)
            printf("addr:%p %p %p\n", pstBufInfo->stFrameData.pVirAddr[0], pstBufInfo->stFrameData.pVirAddr[1],
                    pstBufInfo->stFrameData.pVirAddr[2]);

        ret = u32FrameSize;
        break;
    default:
        return -2;
        break;
    }
    return ret;
}

typedef struct VENC_FrameEndParam_s
{
    MI_U32 u32FrameSeq;
    int iIdrCnt;
} VENC_FrameEndParam_t;

//operation after frame filling and putting are done
int _VENC_DoInputFrameEnd(VENC_FrameEndParam_t *pstParam)
{
    MI_VENC_CHN VencChn;
    MI_S32 s32Ret;
    int iRet = 0;
    //ExecFunc(pstParam, NULL);
    if(pstParam == NULL)
    {
        DBG_ERR("[%4d]exec function failed\n", __LINE__);
        return 1;
    }
    //---- post frame putting step ----
    if(iRequestIdr > 0)
    {
        pstParam->iIdrCnt++;
        if(pstParam->iIdrCnt >= iRequestIdr)
        {
            printf("Request IDR\n");
            for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
            {
                s32Ret = MI_VENC_RequestIdr(VencChn, TRUE);
                if (s32Ret != MI_SUCCESS)
                {
                    printf("[ERR]%X Fail to request IDR to ch%2d.\n", s32Ret, VencChn);
                    return 2;
                }
            }
            pstParam->iIdrCnt = 0;
        }
    }

    if(iEnableIdr >= 0 && iEnableIdr <= 1)
    {
        if(iEnableIdrCnt == iEnableIdrFrame)
        {
            for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
            {
                printf("EnableIdr(%d) at frame#%d\n", iEnableIdr, iEnableIdrFrame);
                s32Ret = MI_VENC_EnableIdr(VencChn, iEnableIdr);
                if (s32Ret != MI_SUCCESS)
                {
                    printf("[ERR]%X Fail to enable IDR to ch%2d.\n", s32Ret, VencChn);
                    return 3;
                }
            }
        }
        iEnableIdrCnt++;
    }

    if(VENC_CHANGE_CHN_ATTR > 0 && pstParam->u32FrameSeq == (MI_U32)VENC_CHANGE_CHN_ATTR)
    {
        MI_VENC_ChnAttr_t stChnAttr;
        MI_S32 s32Ret;
        int cnt;
        DBG_INFO("---- Change Channel Attribute at frame#%d\n", pstParam->u32FrameSeq);

        for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
        {
            MI_U32 *pu32PicHeight = NULL;
            MI_U32 *pu32IQp = NULL, *pu32PQp = NULL;
            MI_U32 *pu32Gop = NULL;
            MI_U32 *pu32FpsN = NULL, *pu32FpsM = NULL;
            MI_U32 *pu32Bitrate = NULL;

            ExecFunc(MI_VENC_GetChnAttr(VencChn, &stChnAttr), MI_SUCCESS);
            switch (stChnAttr.stVeAttr.eType)
            {
                case E_MI_VENC_MODTYPE_H265E:
                    pu32PicHeight = &stChnAttr.stVeAttr.stAttrH265e.u32PicHeight;
                    if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H265FIXQP)
                    {
                        pu32IQp = &stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp;
                        pu32PQp = &stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp;
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen;
                    }
                    else if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H265CBR)
                    {
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen;
                        pu32Bitrate = &stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate;
                    }
                    else if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H265VBR)
                    {
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen;
                        pu32Bitrate = &stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate;
                    }
                    break;
                case E_MI_VENC_MODTYPE_H264E:
                    pu32PicHeight = &stChnAttr.stVeAttr.stAttrH264e.u32PicHeight;
                    if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H264FIXQP)
                    {
                        pu32IQp = &stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp;
                        pu32PQp = &stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp;
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen;
                    }
                    else if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H264CBR)
                    {
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen;
                        pu32Bitrate = &stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate;
                    }
                    else if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H264VBR)
                    {
                        pu32Gop = &stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop;
                        pu32FpsN = &stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum;
                        pu32FpsM = &stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen;
                        pu32Bitrate = &stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate;
                    }
                    break;
                case E_MI_VENC_MODTYPE_JPEGE:
                    pu32PicHeight = &stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight;
                    break;
                default:
                    break;
            }

            if(pu32PicHeight)
            {
                *pu32PicHeight += VENC_ChangeHeight;
            }
            if(pu32IQp != NULL && pu32PQp != NULL)
            {
                *pu32IQp += VENC_ChangeQp;
                *pu32PQp += VENC_ChangeQp;
            }
            if(pu32Gop != NULL)
            {
                *pu32Gop += VENC_ChangeGop;
            }
            if(pu32FpsN != NULL)
            {
                *pu32FpsN = VENC_ChangeToFpsN;
            }
            if(pu32FpsM != NULL)
            {
                *pu32FpsM = VENC_ChangeToFpsM;
            }
            if(pu32Bitrate != NULL)
            {
                *pu32Bitrate *= VENC_ChangeBitrate;
            }

            s32Ret = MI_ERR_VENC_UNDEFINED;
            //                ExecFunc(MI_VENC_StopRecvPic(VencChn), MI_SUCCESS);
            cnt = 0;
            do
            {
                s32Ret = MI_VENC_SetChnAttr(VencChn, &stChnAttr);
                if(s32Ret == MI_SUCCESS)
                    break;
                sleep_ms(33);
                cnt++;
            } while(s32Ret == MI_ERR_VENC_NOT_PERM && cnt < 30);
            if(cnt == 30)
            {
                DBG_ERR("SetChnAttr timeout\n");
                return -4;
            }
            if(s32Ret != MI_SUCCESS)
            {
                DBG_ERR("s32Ret=%X\n", s32Ret);
            }
            ExecFunc(s32Ret, MI_SUCCESS);
            //                ExecFunc(MI_VENC_StartRecvPic(VencChn), MI_SUCCESS);
        }
    }
    return iRet;
}

//Put NV12 data into all channels
int _VENC_PutFrames(void)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    //MI_U32 u32FrameSet = 0; //1 set of frame would send to all channels
    MI_VENC_CHN VencChn = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_ChnPort_t stVencInChnPort;
    MI_S32 YUV_W, YUV_H;
    MI_U32 u32Nv12Size;
    MI_BOOL bErr;
    char *szFn;
    FILE *file;
    MI_S32 s32InFps;
    MI_S32 s32DelayMs;
    VENC_FrameEndParam_t stParam;
    int ret;

#if FPGA
    s32DelayMs = FPGA_DELAY_MS;//for FPGA
#else
    s32DelayMs = 100;
#endif

    memset(&stParam , 0 , sizeof(stParam));
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    stVencInChnPort.eModId = E_MI_MODULE_ID_VENC;
    stVencInChnPort.u32DevId = 0;
    stVencInChnPort.u32ChnId = 0;
    stVencInChnPort.u32PortId = 0;

    // ==== Put frame data loop ====
    stParam.u32FrameSeq = 0;
    s32InFps = get_cfg_int("VENC_GLOB_IN_FPS", &bErr);
    if(FALSE == bErr && s32InFps != 0)
    {
        s32DelayMs = 1000 / s32InFps;
    }

    YUV_W = get_cfg_int("VENC_GLOB_IN_W", &bErr);
    YUV_H = get_cfg_int("VENC_GLOB_IN_H", &bErr);
    u32Nv12Size = (YUV_W * YUV_H) * 3 / 2;
    //printf("Still %d channels.\n", u32CreateChnNum);
    szFn = get_cfg_str("VENC_GLOB_IN_FILE", &bErr);
    file = fopen(szFn, "rb");
    if(file)
    {
        unsigned int size;
        int iCacheMb;
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        s32FramesInFile = size / u32Nv12Size;
        eVencInputType = E_VENC_INPUT_FROM_FILE;
        iCacheMb = get_cfg_int("VENC_GLOB_IN_FILE_CACHE_MB", &bErr);

        if(iCacheMb > 0)
        {
            u32CachedFsize = iCacheMb * 1024 * 1024;
            if(size > u32CachedFsize)
            {
                printf("Not enough cache size in config, File size:%d\n",
                        size / 1024 / 1024);
            }
            else
            {
                pFCache = malloc(u32CachedFsize);
                if(pFCache == NULL)
                {
                    printf("Not enough size for file cache.\n");
                }
                else
                {
                    fread(pFCache, size, 1, file);
                    eVencInputType = E_VENC_INPUT_FROM_FILE_CACHE;
                    printf("\n\n$$$$ Use cached file as input $$$$\n\n");
                }
            }
        }
        if(size % u32Nv12Size != 0)
        {
            printf("\n\n[WARN] The file size is not the exact multiple of YUV size.\n"
                    "Check YUV size config again!\n\n\n");
        }
        printf("%d frames in the file.\n", s32FramesInFile);
        fclose(file);
    }
    else
    {
        s32FramesInFile = 0;
        u32CachedFsize = 0;
        printf("Input file %s is set but unable to be read. Use built-in YUV.\n", szFn);
    }

    while (1)
    {
        MI_SYS_BufConf_t stBufConf;
        MI_U64 u64Time;

        memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
        MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = YUV_W;
        stBufConf.stFrameCfg.u16Height = YUV_H;

        for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
        {
            //stVencInChnPort.u32DevId = _astChn[VencChn].u32DevId;
            //ExecFunc(MI_VENC_GetChnDevid(VencChn, &stVencInChnPort.u32DevId), MI_SUCCESS);
            s32Ret = MI_VENC_GetChnDevid(VencChn, &stVencInChnPort.u32DevId);
            if(MI_SUCCESS != s32Ret)
            {
                DBG_ERR("Get Device ID:%X\n :%d", s32Ret, stVencInChnPort.u32DevId);
                break;
            }
            stVencInChnPort.u32ChnId = VencChn;
            trace_venc_channel("before GetInput", VencChn);
            s32Ret = MI_SYS_ChnInputPortGetBuf(&stVencInChnPort, &stBufConf, &stBufInfo, &_astChn[VencChn].hBuf, IN_PORT_TIMEOUT);
            trace_venc_channel("after GetInput", VencChn);
            if(MI_SUCCESS == s32Ret)
            {
                VENC_InputType_e eInputType;

                eInputType = eVencInputType;
                //if(stVencInChnPort.u32DevId == E_VENC_DEV_DUMMY)
                if(_astChn[VencChn].eModType == E_MI_VENC_MODTYPE_MAX)
                {//special process for dummy test
                    eInputType = E_VENC_INPUT_FROM_CODE;
                }
                else if(file)
                {
                    eInputType = E_VENC_INPUT_FROM_FILE;
                }
                //printf("INPUT FRAME Y.VA %p(%d)\n", stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height);
                ret = _VENC_FillFrameData(&stBufInfo, eInputType, stParam.u32FrameSeq);

                //print debug messages
                if (stParam.u32FrameSeq == 0 && VENC_GLOB_IN_MSG_LEVEL > 0)
                {
                    MI_SYS_ChnPort_t *pstPort = &stVencInChnPort;
                    _DumpSysBufInfo(&stBufInfo, _astChn[VencChn].hBuf);
                    printf("MOD:%d DEV:%d, CHN:%d\n", pstPort->eModId, pstPort->u32DevId, pstPort->u32ChnId);
                }
            }
            else
            {
                MI_SYS_ChnPort_t *pstPort = &stVencInChnPort;
                printf("[ERR]%X Unable to get input port\n", s32Ret);//refer MI_ErrCode_e
                printf("MOD:%d DEV:%d, CHN:%d buf:%X\n", pstPort->eModId, pstPort->u32DevId, pstPort->u32ChnId,
                        _astChn[VencChn].hBuf);
                _DumpSysBufInfo(&stBufInfo, _astChn[VencChn].hBuf);
                _astChn[VencChn].hBuf = NULL;
            }
        }
        if(ret == 0)
            break;
        for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
        {
            //put input buffer
            if (_astChn[VencChn].hBuf)
            {
                if(VENC_GLOB_IN_MSG_LEVEL >= 2)
                    printf(">> Put buf[%d] %d bytes >>ch%d\n", stParam.u32FrameSeq, ret, VencChn);
                else if (VENC_GLOB_IN_MSG_LEVEL > 0)
                    printf("\n");
                trace_venc_channel("before PutInput", VencChn);
                s32Ret = MI_SYS_ChnInputPortPutBuf(_astChn[VencChn].hBuf ,&stBufInfo , FALSE);
                trace_venc_channel("After PutInput", VencChn);
                if (s32Ret != MI_SUCCESS)
                {
                    printf("[ERR]%X Fail to put input port.\n", s32Ret);
                    break;
                }
            }
        }
        if(s32Ret != 0)
            break;
        stParam.u32FrameSeq++;
        ret = _VENC_DoInputFrameEnd(&stParam);

        if (stBufInfo.bEndOfStream || (ret != 0))
        {
            break;
        }

        //sleep between input frames
        while(1){
            MI_SYS_GetCurPts(&u64Time);
            if((u64Time - stBufConf.u64TargetPts) >= (s32DelayMs * 1000))
                break;
            sleep_ms(1);
        } //sleep_ms(s32DelayMs);

        //printf("sleep codec:%d\n", stChnAttr.stVeAttr.eType);
    }
    printf("Done putting frames.\n");
    if(pFCache)
    {
        free(pFCache);
        pFCache = NULL;
    }
    return ret;
}

//before or after STREAM_ON
int _VENC_ConfigH265(int iChn)
{
    MI_BOOL bErr;
    int iVal;

    //Intra
    iVal = get_cfg_int("VENC_INTRA", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1) //enable deblocking filter control
    {
        MI_VENC_ParamH265IntraPred_t stIntra;
        memset(&stIntra, 0, sizeof(stIntra));
        iVal = get_cfg_int("u32Intra32x32Penalty", &bErr);
        if(bErr == FALSE)
            stIntra.u32Intra32x32Penalty = iVal;
        iVal = get_cfg_int("u32Intra16x16Penalty", &bErr);
        if(bErr == FALSE)
            stIntra.u32Intra16x16Penalty = iVal;
        iVal = get_cfg_int("u32Intra8x8Penalty", &bErr);
        if(bErr == FALSE)
            stIntra.u32Intra8x8Penalty = iVal;
        ExecFunc(MI_VENC_SetH265IntraPred(iChn, &stIntra), MI_SUCCESS);
    }

    //Inter
    iVal = get_cfg_int("VENC_INTER", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1)
    {
        MI_VENC_ParamH265InterPred_t stInter;
        memset(&stInter, 0, sizeof(stInter));
        iVal = get_cfg_int("nDmv_X", &bErr);
        if(bErr == FALSE)
            stInter.u32HWSize = iVal;
        iVal = get_cfg_int("nDmv_Y", &bErr);
        if(bErr == FALSE)
            stInter.u32VWSize = iVal;
        iVal = get_cfg_int("bInter8x8PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter8x8PredEn = iVal;
        iVal = get_cfg_int("bInter8x16PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter8x16PredEn = iVal;
        iVal = get_cfg_int("bInter16x8PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter16x8PredEn = iVal;
        iVal = get_cfg_int("bInter16x16PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter16x16PredEn = iVal;
        ExecFunc(MI_VENC_SetH265InterPred(iChn, &stInter), MI_SUCCESS);
    }

    //trans
    iVal = get_cfg_int("qpoffset", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH265Trans_t stTrans;
        stTrans.u32InterTransMode = 0;
        stTrans.u32IntraTransMode = 0;
        stTrans.s32ChromaQpIndexOffset = iVal;
        ExecFunc(MI_VENC_SetH265Trans(iChn, &stTrans), MI_SUCCESS);
    }

    //deblocking
    iVal = get_cfg_int("DEBLOCK_FILTER_CONTROL", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1) //enable deblocking filter control
    {
        MI_VENC_ParamH265Dblk_t stDeblk;
        stDeblk.disable_deblocking_filter_idc = get_cfg_int("disable_deblocking_filter_idc", &bErr);
        ExecFunc(bErr, FALSE);
        stDeblk.slice_tc_offset_div2 = get_cfg_int("slice_tc_offset_div2", &bErr);
        ExecFunc(bErr, FALSE);
        stDeblk.slice_beta_offset_div2 = get_cfg_int("slice_beta_offset_div2", &bErr);
        ExecFunc(bErr, FALSE);
        ExecFunc(MI_VENC_SetH265Dblk(iChn, &stDeblk), MI_SUCCESS);
    }

    //VUI
    iVal = get_cfg_int("VENC_GLOB_VUI", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1)
    {
        MI_VENC_ParamH265Vui_t stH265Vui;
        ExecFunc(MI_VENC_GetH265Vui(iChn, &stH265Vui), MI_SUCCESS);
        stH265Vui.stVuiVideoSignal.u8VideoSignalTypePresentFlag = 1;
        stH265Vui.stVuiVideoSignal.u8VideoFullRangeFlag = 1;
        stH265Vui.stVuiVideoSignal.u8VideoFormat = 5;
        stH265Vui.stVuiVideoSignal.u8ColourDescriptionPresentFlag = 1;
        stH265Vui.stVuiTimeInfo.u8TimingInfoPresentFlag = 1;
        stH265Vui.stVuiAspectRatio.u16SarHeight = 12;
        stH265Vui.stVuiAspectRatio.u16SarHeight = 11;

        ExecFunc(MI_VENC_SetH265Vui(iChn, &stH265Vui), MI_SUCCESS);
    }
    return MI_SUCCESS;
}

int _VENC_ConfigH264(int iChn)
{
    MI_BOOL bErr;
    MI_S32 s32Ret;

    int iVal;

    //Intra
    iVal = get_cfg_int("VENC_INTRA", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1) //enable deblocking filter control
    {
        MI_VENC_ParamH264IntraPred_t stIntra;
        memset(&stIntra, 0, sizeof(stIntra));
        iVal = get_cfg_int("bConstrainedIntraPredFlag", &bErr);
        if(bErr == FALSE)
            stIntra.bConstrainedIntraPredFlag = iVal;
        iVal = get_cfg_int("u32Intra16x16Penalty", &bErr);
        if(bErr == FALSE)
            stIntra.u32Intra16x16Penalty = iVal;
        iVal = get_cfg_int("u32Intra4x4Penalty", &bErr);
        if(bErr == FALSE)
            stIntra.u32Intra4x4Penalty = iVal;
        iVal = get_cfg_int("bIntraPlanarPenalty", &bErr);
        if(bErr == FALSE)
            stIntra.bIntraPlanarPenalty = iVal;
        ExecFunc(MI_VENC_SetH264IntraPred(iChn, &stIntra), MI_SUCCESS);
    }

    //Inter
    iVal = get_cfg_int("VENC_INTER", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1) //enable deblocking filter control
    {
        MI_VENC_ParamH264InterPred_t stInter;
        memset(&stInter, 0, sizeof(stInter));
        iVal = get_cfg_int("nDmv_X", &bErr);
        if(bErr == FALSE)
            stInter.u32HWSize = iVal;
        iVal = get_cfg_int("nDmv_Y", &bErr);
        if(bErr == FALSE)
            stInter.u32VWSize = iVal;
        iVal = get_cfg_int("bInter8x8PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter8x8PredEn = iVal;
        iVal = get_cfg_int("bInter8x16PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter8x16PredEn = iVal;
        iVal = get_cfg_int("bInter16x8PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter16x8PredEn = iVal;
        iVal = get_cfg_int("bInter16x16PredEn", &bErr);
        if(bErr == FALSE)
            stInter.bInter16x16PredEn = iVal;
        ExecFunc(MI_VENC_SetH264InterPred(iChn, &stInter), MI_SUCCESS);
    }

    //trans
    iVal = get_cfg_int("qpoffset", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH264Trans_t stTrans;
        stTrans.u32InterTransMode = 0;
        stTrans.u32IntraTransMode = 0;
        stTrans.s32ChromaQpIndexOffset = iVal;
        ExecFunc(MI_VENC_SetH264Trans(iChn, &stTrans), MI_SUCCESS);
    }

    //entropy
    iVal = get_cfg_int("Cabac", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH264Entropy_t stEntropy;
        stEntropy.u32EntropyEncModeI = (MI_U32)iVal;
        stEntropy.u32EntropyEncModeP = (MI_U32)iVal;
        s32Ret = MI_VENC_SetH264Entropy(iChn, &stEntropy);
        if (s32Ret != MI_SUCCESS)
        {
            DBG_ERR("Set Entropy :%d ERR(%X)\n", stEntropy.u32EntropyEncModeI, s32Ret);
            goto error;
        }
    }

    //deblocking
    iVal = get_cfg_int("DEBLOCK_FILTER_CONTROL", &bErr);
    ExecFunc(bErr, FALSE);
    if (iVal == 1) //enable deblocking filter control
    {
        MI_VENC_ParamH264Dblk_t stDeblk;
        stDeblk.disable_deblocking_filter_idc = get_cfg_int("disable_deblocking_filter_idc", &bErr);
        ExecFunc(bErr, FALSE);
        stDeblk.slice_alpha_c0_offset_div2 = get_cfg_int("slice_alpha_c0_offset_div2", &bErr);
        ExecFunc(bErr, FALSE);
        stDeblk.slice_beta_offset_div2 = get_cfg_int("slice_beta_offset_div2", &bErr);
        ExecFunc(bErr, FALSE);
        ExecFunc(MI_VENC_SetH264Dblk(iChn, &stDeblk), MI_SUCCESS);
    }

    //VUI
    iVal = get_cfg_int("VENC_GLOB_VUI", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH264Vui_t stH264Vui;
        ExecFunc(MI_VENC_GetH264Vui(iChn, &stH264Vui), MI_SUCCESS);
        stH264Vui.stVuiVideoSignal.u8VideoSignalTypePresentFlag = 1;
        stH264Vui.stVuiVideoSignal.u8VideoFullRangeFlag = 1;
        stH264Vui.stVuiVideoSignal.u8VideoFormat = 5;
        stH264Vui.stVuiVideoSignal.u8ColourDescriptionPresentFlag = 1;
        stH264Vui.stVuiTimeInfo.u8TimingInfoPresentFlag = 1;
        stH264Vui.stVuiAspectRatio.u16SarHeight = 12;
        stH264Vui.stVuiAspectRatio.u16SarHeight = 11;

        ExecFunc(MI_VENC_SetH264Vui(iChn, &stH264Vui), MI_SUCCESS);
    }
error:
    return MI_SUCCESS;
}

#define CamOsPrintf printf
void print_hex(char* title, void* buf, int num) {
    int i;
    char *data = (char *) buf;

    CamOsPrintf("%s\nOffset(h)  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                "----------------------------------------------------------",
                title);
    for (i = 0; i < num; i++) {
        if (i % 16 == 0)
            CamOsPrintf("\n%08X   ", i);
            CamOsPrintf("%02X ", data[i]);
    }
    CamOsPrintf("\n");
}

int _VENC_ConfigJpeg(int iChn)
{
    MI_S32 s32Ret;
    MI_VENC_ParamJpeg_t stParamJpeg;
    MI_BOOL bErr;
    int iVal;

    iVal = get_cfg_int("Qfactor", &bErr);
    if(bErr == FALSE && iVal > 0 && iVal <= 100)
    {
        memset(&stParamJpeg, 0, sizeof(stParamJpeg));
        s32Ret = MI_VENC_GetJpegParam(iChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
        DBG_INFO("Get Qf:%d to %d\n", stParamJpeg.u32Qfactor, iVal);

        stParamJpeg.u32Qfactor = (MI_U32)iVal;
        s32Ret = MI_VENC_SetJpegParam(iChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
    }
    //print_hex("Y table", stParamJpeg.au8YQt, 64);
    return MI_SUCCESS;
}

int _VENC_Config(int iChn, MI_VENC_ModType_e eModType)
{
    MI_BOOL bErr;
    int iApplyCfg;
    char *szUserData;
    size_t iUserDataLen;
    MI_S32 s32Ret;

    szUserData = get_cfg_str("VENC_GLOB_USER_DATA", &bErr);
    ExecFunc(bErr, FALSE);
    iUserDataLen = strlen(szUserData);
    if(iUserDataLen != 0)
    {
        s32Ret = MI_VENC_InsertUserData(iChn, (MI_U8*)szUserData, iUserDataLen + 1);
        if (s32Ret != MI_SUCCESS)
        {
            printf("[ERR]%X Fail to Insert user data.\n", s32Ret);
        }
        iUserDataLen = 0;
    }

    iApplyCfg = get_cfg_int("VENC_Crop", &bErr);
    if(iApplyCfg == 1)
    {
        MI_VENC_CropCfg_t stCrop, stGetCrop;
        stCrop.bEnable = iApplyCfg;
        stCrop.stRect.u32Left = get_cfg_int("VENC_Crop_X", &bErr);
        ExecFunc(bErr, FALSE);
        stCrop.stRect.u32Top = get_cfg_int("VENC_Crop_Y", &bErr);
        ExecFunc(bErr, FALSE);
        stCrop.stRect.u32Width = get_cfg_int("VENC_Crop_W", &bErr);
        ExecFunc(bErr, FALSE);
        stCrop.stRect.u32Height = get_cfg_int("VENC_Crop_H", &bErr);
        ExecFunc(bErr, FALSE);
        ExecFunc(MI_VENC_SetCrop(iChn, &stCrop), MI_SUCCESS);
        ExecFunc(MI_VENC_GetCrop(iChn, &stGetCrop), MI_SUCCESS);
        ExecFunc(memcmp(&stCrop, &stGetCrop, sizeof(stCrop)), 0);
    }

    iApplyCfg = get_cfg_int("VENC_REF_EN", &bErr);
    if(iApplyCfg == 1)
    {
        MI_VENC_ParamRef_t stParamRef;
        stParamRef.u32Base = get_cfg_int("VENC_REF_Base", &bErr);
        ExecFunc(bErr, FALSE);
        stParamRef.u32Enhance = get_cfg_int("VENC_REF_Enhance", &bErr);
        ExecFunc(bErr, FALSE);
        stParamRef.bEnablePred = get_cfg_int("VENC_REF_EnablePred", &bErr);
        ExecFunc(bErr, FALSE);
        ExecFunc(MI_VENC_SetRefParam(iChn, &stParamRef), MI_SUCCESS);
    }

    iApplyCfg = get_cfg_int("VENC_SUPER_FRAME_MODE", &bErr);
    if(iApplyCfg >= 0 && iApplyCfg < (int)E_MI_VENC_SUPERFRM_MAX)
    {
        MI_VENC_SuperFrameCfg_t stSuperFrame, stGetSuperFrame;
        memset(&stSuperFrame, 0, sizeof(stSuperFrame));
        stSuperFrame.eSuperFrmMode = (MI_VENC_SuperFrmMode_e) iApplyCfg;
        stSuperFrame.u32SuperIFrmBitsThr = (MI_U32)get_cfg_int("SuperI", &bErr);
        ExecFunc(bErr, FALSE);
        stSuperFrame.u32SuperPFrmBitsThr = (MI_U32)get_cfg_int("SuperP", &bErr);
        ExecFunc(bErr, FALSE);
        stSuperFrame.u32SuperBFrmBitsThr = 2;//magic debug parameter
        ExecFunc(MI_VENC_SetSuperFrameCfg(iChn, &stSuperFrame), MI_SUCCESS);
        ExecFunc(MI_VENC_GetSuperFrameCfg(iChn, &stGetSuperFrame), MI_SUCCESS);
        ExecFunc(memcmp(&stSuperFrame, &stGetSuperFrame, sizeof(stSuperFrame)), 0);

        iApplyCfg = get_cfg_int("RcPrio", &bErr);
        if(iApplyCfg >= 0 && iApplyCfg < (int)E_MI_VENC_RC_PRIORITY_MAX)
        {
            MI_VENC_RcPriority_e ePrio = (MI_VENC_RcPriority_e) iApplyCfg;
            MI_VENC_RcPriority_e eGetPrio;
            ExecFunc(MI_VENC_SetRcPriority(iChn, &ePrio), MI_SUCCESS);
            ExecFunc(MI_VENC_GetRcPriority(iChn, &eGetPrio), MI_SUCCESS);
            ExecFunc(memcmp(&ePrio, &eGetPrio, sizeof(ePrio)), 0);
        }
    }

    iApplyCfg = get_cfg_int("VENC_FRAME_LOST", &bErr);
    if(iApplyCfg >= 0 && iApplyCfg <= 2)
    {
        MI_VENC_ParamFrameLost_t stFrameLost, stGetFrameLost;
        memset(&stFrameLost, 0, sizeof(stFrameLost));
        stFrameLost.bFrmLostOpen = (MI_VENC_SuperFrmMode_e) iApplyCfg;
        stFrameLost.u32EncFrmGaps = (MI_U32)get_cfg_int("FrmLostGap", &bErr);
        ExecFunc(bErr, FALSE);
        stFrameLost.u32FrmLostBpsThr = (MI_U32)get_cfg_int("FrmLostBps", &bErr);
        ExecFunc(bErr, FALSE);
        ExecFunc(MI_VENC_SetFrameLostStrategy(iChn, &stFrameLost), MI_SUCCESS);
        ExecFunc(MI_VENC_GetFrameLostStrategy(iChn, &stGetFrameLost), MI_SUCCESS);
        ExecFunc(memcmp(&stFrameLost, &stGetFrameLost, sizeof(stFrameLost)), 0);
    }

    iApplyCfg = get_cfg_int("VENC_GLOB_APPLY_CFG", &bErr);
    if(iApplyCfg == 0)
        return MI_SUCCESS;

    if(eModType == E_MI_VENC_MODTYPE_H265E)
    {
        return _VENC_ConfigH265(iChn);
    }
    else if(eModType == E_MI_VENC_MODTYPE_H264E)
    {
        return _VENC_ConfigH264(iChn);
    }
    else if(eModType == E_MI_VENC_MODTYPE_JPEGE)
    {
        return _VENC_ConfigJpeg(iChn);
    }
    return MI_SUCCESS;
}

int _VENC_SetH265Param(int iChn)
{
    MI_BOOL bErr;
    int iVal;
   //slice
    iVal = get_cfg_int("nRows", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH265SliceSplit_t stSlice;
        stSlice.bSplitEnable = iVal > 0 ? TRUE : FALSE;
        stSlice.u32SliceRowCount = iVal;
        ExecFunc(MI_VENC_SetH265SliceSplit(iChn, &stSlice), MI_SUCCESS);
    }
    return MI_SUCCESS;
}

int _VENC_SetH264Param(int iChn)
{
    MI_BOOL bErr;
    int iVal;

   //slice
    iVal = get_cfg_int("nRows", &bErr);
    ExecFunc(bErr, FALSE);
    {
        MI_VENC_ParamH264SliceSplit_t stSlice;
        stSlice.bSplitEnable = iVal > 0 ? TRUE : FALSE;;
        stSlice.u32SliceRowCount = iVal;
        ExecFunc(MI_VENC_SetH264SliceSplit(iChn, &stSlice), MI_SUCCESS);
    }
    return MI_SUCCESS;
}

//run time parameters after
int _VENC_SetParam(int iChn, MI_VENC_ModType_e eModType)
{
    MI_BOOL bErr;
    MI_VENC_RoiCfg_t stRoi;
    MI_BOOL bRoiAssigned = FALSE;

    int iApplyCfg;

    iApplyCfg = get_cfg_int("VENC_GLOB_APPLY_CFG", &bErr);
    if(iApplyCfg == 0)
        return MI_SUCCESS;

    if(eModType == E_MI_VENC_MODTYPE_H265E)
    {
        ExecFunc(_VENC_SetH265Param(iChn), 0);
    }
    if(eModType == E_MI_VENC_MODTYPE_H264E)
    {
        ExecFunc(_VENC_SetH264Param(iChn), 0);
    }

    //common, run-time
    iApplyCfg = get_cfg_int("VENC_ROI0_EN", &bErr);
    if(iApplyCfg == 1 && bErr == FALSE)
    {
        bRoiAssigned = TRUE;
        memset(&stRoi, 0, sizeof(stRoi));
        stRoi.bAbsQp = FALSE;
        stRoi.bEnable = TRUE;
        stRoi.u32Index = 0;
        stRoi.stRect.u32Left   = (MI_U32)get_cfg_int("VENC_ROI0_X", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Top    = (MI_U32)get_cfg_int("VENC_ROI0_Y", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Width  = (MI_U32)get_cfg_int("VENC_ROI0_W", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Height = (MI_U32)get_cfg_int("VENC_ROI0_H", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.s32Qp            = (MI_S32)get_cfg_int("VENC_ROI0_Qp", &bErr);
        ExecFunc(MI_VENC_SetRoiCfg(iChn, &stRoi), MI_SUCCESS);
        ExecFunc(MI_VENC_GetRoiCfg(iChn, 0, &stRoi), MI_SUCCESS);
        DBG_INFO("get rect back %d,%d,%dx%d\n", stRoi.stRect.u32Left, stRoi.stRect.u32Top, stRoi.stRect.u32Width,
                 stRoi.stRect.u32Height);
    }
    iApplyCfg = get_cfg_int("VENC_ROI1_EN", &bErr);
    if(iApplyCfg == 1 && bErr == FALSE)
    {
        bRoiAssigned = TRUE;
        memset(&stRoi, 0, sizeof(stRoi));
        stRoi.bAbsQp = FALSE;
        stRoi.bEnable = TRUE;
        stRoi.u32Index = 1;
        stRoi.stRect.u32Left   = (MI_U32)get_cfg_int("VENC_ROI1_X", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Top    = (MI_U32)get_cfg_int("VENC_ROI1_Y", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Width  = (MI_U32)get_cfg_int("VENC_ROI1_W", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.stRect.u32Height = (MI_U32)get_cfg_int("VENC_ROI1_H", &bErr);
        ExecFunc(bErr, FALSE);
        stRoi.s32Qp            = (MI_S32)get_cfg_int("VENC_ROI1_Qp", &bErr);
        ExecFunc(MI_VENC_SetRoiCfg(iChn, &stRoi), MI_SUCCESS);
    }
    if(bRoiAssigned)
    {
        int iSrcRate = 0, iDstRate = 0;
        iSrcRate = get_cfg_int("VENC_ROI_SRC_RATE", &bErr);
        if(bErr == FALSE)
            iDstRate = get_cfg_int("VENC_ROI_DST_RATE", &bErr);
        if(bErr == FALSE && iSrcRate > 0 && iDstRate > 0)
        {
            MI_VENC_RoiBgFrameRate_t stRoiBgFrameRate, stGetRoiBgFrameRate;
            stRoiBgFrameRate.s32SrcFrmRate = (MI_U32)iSrcRate;
            stRoiBgFrameRate.s32DstFrmRate = (MI_U32)iDstRate;
            ExecFunc(MI_VENC_SetRoiBgFrameRate(iChn, &stRoiBgFrameRate), MI_SUCCESS);
            ExecFunc(MI_VENC_GetRoiBgFrameRate(iChn, &stGetRoiBgFrameRate), MI_SUCCESS);
            ExecFunc(memcmp(&stRoiBgFrameRate, &stGetRoiBgFrameRate, sizeof(stRoiBgFrameRate)), 0);
        }
    }
    return MI_SUCCESS;
}

int _VENC_GetRcConfig(int iChn, MI_VENC_ModType_e eModType, VENC_Rc_t *pstRc)
{
    int iApplyCfg;
    MI_BOOL bErr;
    char *szRcType;
    char szQp[64];

    if(pstRc == NULL)
        return -1;
    if(eModType == E_MI_VENC_MODTYPE_MAX)
    {
        pstRc->eRcMode = E_MI_VENC_RC_MODE_MAX;
        return MI_SUCCESS;
    }
    memset(pstRc, 0, sizeof(VENC_Rc_t));

    pstRc->u32Gop = get_cfg_int("VENC_GOP", &bErr);
    iApplyCfg = get_cfg_int("VENC_RC", &bErr);

    if(iApplyCfg == 0)//not by cfg system, use default value
    {
        szRcType = "FixQp";
    }
    else
    {
        szRcType = get_cfg_str("RcType", &bErr);
        if(bErr)
        {
            DBG_ERR("Can not find RcType config.\n");
            return -2;
        }
    }
    pstRc->u32SrcFrmRateNum = 30;
    pstRc->u32SrcFrmRateDen = 1;

    if(strcmp("Cbr", szRcType) == 0)
    {
        pstRc->u32Bitrate = get_cfg_int("Bitrate", &bErr);
        ExecFunc(bErr, FALSE);
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            break;
        default:
            return -2;
            break;
        }
    }
    else if(strcmp("Vbr", szRcType) == 0)
    {
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264VBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            break;
        default:
            return -2;
            break;
        }
        pstRc->u32Bitrate = get_cfg_int("Bitrate", &bErr);
        ExecFunc(bErr, FALSE);
        pstRc->u32VbrMinQp = get_cfg_int("VbrMinQp", &bErr);
        ExecFunc(bErr, FALSE);
        pstRc->u32VbrMaxQp = get_cfg_int("VbrMaxQp", &bErr);
        ExecFunc(bErr, FALSE);
    }
    else if(strcmp("FixQp", szRcType) == 0)
    {
        switch (eModType)
        {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            break;
        case E_MI_VENC_MODTYPE_JPEGE:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            break;
        default:
            return -2;
            break;
        }
        if(iApplyCfg)
        {
            snprintf(szQp, sizeof(szQp)-1, "VENC_CH%02d_QP", iChn);
            pstRc->u32FixQp = get_cfg_int(szQp, &bErr) + iChn;
            if (bErr)
            {
                pstRc->u32FixQp = DEF_QP + iChn;
            }
        }
        else
        {
            pstRc->u32FixQp = /*venc_state.rc.u8QpI*/ DEF_QP + iChn;
        }
    }
    else if(strcmp("Abr", szRcType) == 0)
    {
        pstRc->u32AvrMaxBitrate = get_cfg_int("AbrMaxBitrate", &bErr);
        ExecFunc(bErr, FALSE);
    }
    else
    {
        DBG_ERR("Unknown rcType %s.\n", szRcType);
        return -3;
    }

    return MI_SUCCESS;
}

void _VENC_PrintUsage(void)
{
    printf("\nvenc NUMBER_OF_CHANNEL [CH1_FORMAT [CH2_FORMAT] ...]\n");
    printf("Available FORMAT: h265 h264 jpeg dummy\n");
    printf("Examples:\n"
           "    venc 1 h264\n"
           "    venc 2 h264 h264\n");
    printf("\n---- CONFIG ---- Use 'export CFG=VALUE' to set the value.\n");
    //dump_cfg();
    env_dump_cfg_2_col();
}

int _VENC_AssignCommonGlobal(void)
{
    MI_BOOL bErr;
    VENC_GLOB_TRACE = get_cfg_int("VENC_GLOB_TRACE", &bErr);
    VENC_GLOB_TRACE_QUERY = get_cfg_int("VENC_GLOB_TRACE_QUERY", &bErr);
    VENC_GLOB_IN_MSG_LEVEL = get_cfg_int("VENC_GLOB_IN_MSG_LEVEL", &bErr);
    VENC_GLOB_OUT_MSG_LEVEL = get_cfg_int("VENC_GLOB_OUT_MSG_LEVEL", &bErr);

    //These variables are used in frame-end operations
    iRequestIdr = get_cfg_int("Idr", &bErr);
    ExecFunc(bErr, FALSE);
    iEnableIdr = get_cfg_int("EnableIdr", &bErr);
    ExecFunc(bErr, FALSE);
    iEnableIdrFrame = get_cfg_int("EnableIdrFrame", &bErr);
    ExecFunc(bErr, FALSE);
    VENC_CHANGE_CHN_ATTR = get_cfg_int("VENC_CHANGE_CHN_ATTR", &bErr);
    ExecFunc(bErr, FALSE);
    {
        VENC_ChangeHeight = get_cfg_int("VENC_ChangeHeight", &bErr);
        ExecFunc(bErr, FALSE);
        VENC_ChangeQp = get_cfg_int("VENC_ChangeQp", &bErr);
        ExecFunc(bErr, FALSE);
        VENC_ChangeGop = get_cfg_int("VENC_ChangeGop", &bErr);
        ExecFunc(bErr, FALSE);
        VENC_ChangeToFpsN = get_cfg_int("VENC_ChangeToFpsN", &bErr);
        ExecFunc(bErr, FALSE);
        VENC_ChangeToFpsM = get_cfg_int("VENC_ChangeToFpsM", &bErr);
        ExecFunc(bErr, FALSE);
        VENC_ChangeBitrate = get_cfg_int("VENC_ChangeBitrate", &bErr);
        ExecFunc(bErr, FALSE);
    }
    return 0;
}

int main(int argc, const char *argv[])
{
    MI_BOOL bErr;
    MI_S32 YUV_W, YUV_H;
    MI_U32 u32Cnt;
    int iRetMain = 0;

    ///create
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    VENC_Rc_t stRc;
    MI_VENC_ModType_e eModType;

    if (argc > 1)
    {
        s32CreateChnNum = atoi(argv[1]);
    }
    else
    {
        s32CreateChnNum = 0;
    }

    if (s32CreateChnNum > VENC_MAX_CHN)
    {
        printf("Max supported Channels: %d\n", VENC_MAX_CHN);
        _VENC_PrintUsage();
        return -1;
    }
    if (s32CreateChnNum == 0)
    {
        printf("The first argument should be NUMBER_OF_CHANNELS.\n");
        _VENC_PrintUsage();
        return -1;
    }

    if (argc < (2 + s32CreateChnNum))
    {
        printf("Request %d channels but assigned %d formats only.\n", s32CreateChnNum, argc - 2);
        printf("The number of channel types are not enough.\n");
        _VENC_PrintUsage();
        return -2;
    }


    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    (void)MI_VENC_GetMaxStreamCnt(0, &u32Cnt);//dummy function to connect module
    //sleep_ms(10);//wait for mi_sys_init to be done
    get_cfg_from_env();
    _VENC_AssignCommonGlobal();
    printf("\n\n==== MI_VENC %d Channels Feature Test! ====\n"__DATE__" "__TIME__"\n\n", s32CreateChnNum);

    YUV_W = get_cfg_int("VENC_GLOB_IN_W", &bErr);
    YUV_H = get_cfg_int("VENC_GLOB_IN_H", &bErr);

    for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
    {
        memset(&stChnAttr, 0x0, sizeof(stChnAttr));

        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_MAX;
        if (argc > (2 + VencChn))
        {
            if (0 == strcmp(argv[VencChn + 2], "h265"))
            {
                eModType = E_MI_VENC_MODTYPE_H265E;
            }
            else if (0 == strcmp(argv[VencChn + 2], "h264"))
            {
                eModType = E_MI_VENC_MODTYPE_H264E;
            }
            else if (0 == strcmp(argv[VencChn + 2], "jpeg"))
            {
                eModType = E_MI_VENC_MODTYPE_JPEGE;
            }
            else if (0 == strcmp(argv[VencChn + 2], "dummy"))
            {
                eModType = E_MI_VENC_MODTYPE_MAX;
            }
            else
            {
                DBG_ERR("Unknown type ch%d: %s\n", VencChn, argv[VencChn + 2]);
                return -4;
            }
            _astChn[VencChn].eModType = eModType;
        }

        ExecFunc(_VENC_GetRcConfig(VencChn, eModType, &stRc), MI_SUCCESS);
        ExecFunc(create_venc_channel(eModType, VencChn, YUV_W, YUV_H, &stRc), MI_SUCCESS);
        ExecFunc(_VENC_Config(VencChn, eModType), MI_SUCCESS);
        ExecFunc(MI_VENC_StartRecvPic(VencChn), MI_SUCCESS);
        ExecFunc(_VENC_SetParam(VencChn, eModType), MI_SUCCESS);
    }

    //Check if all channels are ready
    for (VencChn = 0; VencChn < s32CreateChnNum;)
    {
        MI_S32 s32Ret;
        MI_U32 u32Devid;
        s32Ret = MI_VENC_GetChnDevid(VencChn, &u32Devid);
        printf("CH%2d d%d\n", VencChn, u32Devid);
        fflush(stdout);
        if(s32Ret != MI_SUCCESS || u32Devid == ((MI_U32)-1))
        {
            sleep_ms(30);
            continue;
        }
        ++VencChn;
    }

    printf("Creating %d frame-getting threads.\n", s32CreateChnNum);
    for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
    {
        _astChn[VencChn].u32ChnId = VencChn;
        pthread_create(&_astChn[VencChn].tid, NULL, venc_channel_func, &_astChn[VencChn]);
    }
    printf("All threads created\n");

    if(0!=_VENC_PutFrames())
    {
        gbPanic = TRUE;
        iRetMain = -4;
    }

    {
        int iAutoStop;
        iAutoStop = get_cfg_int("VENC_GLOB_AUTO_STOP", &bErr);
        if(bErr || (iAutoStop == 0))
        {
            printf("\nPress any [Enter] to end this program.\n");
            (void)getchar();
        }
    }

    for (VencChn = 0; VencChn < s32CreateChnNum; ++VencChn)
    {
        pthread_join(_astChn[VencChn].tid,NULL);
        ExecFunc(destroy_venc_channel(VencChn), MI_SUCCESS);
    }

    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    return iRetMain;
}
