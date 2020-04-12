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

#include "NV12_cif_frames.h"

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


typedef struct STUB_VencRes_s
{
    MI_VENC_ModType_e eModType;
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    pthread_t input_tid;
    pthread_t output_tid;
    MI_BOOL bInThreadRunning;
    MI_BOOL bOutThreadRunning;
} STUB_VencRes_t;

#define STUB_VENC_CHN_NUM 4
#define STUB_VENC_PICTURE_WIDTH 352
#define STUB_VENC_PICTURE_HEIGHT 288
#define STUB_VENC_NUM_OF_FRAMES (sizeof(gYUV) / ((STUB_VENC_PICTURE_WIDTH * STUB_VENC_PICTURE_HEIGHT * 3) >> 1))


static STUB_VencRes_t _stStubVencRes[STUB_VENC_CHN_NUM];
static MI_U32 _u32MaxSendFrames = 100;
static MI_U32 _u32FrameRate = 30;
static MI_U32 _u32ChnNum = 1;

void *venc_send_frame(void* p)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32YuvWidth, u32YuvHeight, u32YSize, u32FrameSize;
    MI_U32 u32HasSendFrames;
    MI_U32 u32FrameIndexInHeaderFile;

    STUB_VencRes_t *pstStubVencRes = (STUB_VencRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = pstStubVencRes->u32DevId;
    stChnPort.u32ChnId = pstStubVencRes->u32ChnId;
    stChnPort.u32PortId = pstStubVencRes->u32PortId;

    u32YuvWidth = STUB_VENC_PICTURE_WIDTH;
    u32YuvHeight = STUB_VENC_PICTURE_HEIGHT;
    u32YSize = u32YuvWidth * u32YuvHeight;
    u32FrameSize = (u32YSize >> 1) + u32YSize; //Y * 1.5

    MI_SYS_BufConf_t stBufConf;
    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = u32YuvWidth;
    stBufConf.stFrameCfg.u16Height = u32YuvHeight;

    u32HasSendFrames = 0;
    while (u32HasSendFrames < _u32MaxSendFrames && (pstStubVencRes->bInThreadRunning == TRUE))
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf, 1000);
        if(MI_SUCCESS == s32Ret)
        {
            u32FrameIndexInHeaderFile = u32HasSendFrames % STUB_VENC_NUM_OF_FRAMES;
            if(u32HasSendFrames >= (_u32MaxSendFrames - 1))
            {
                u32HasSendFrames = _u32MaxSendFrames - 1;
                stBufInfo.bEndOfStream = TRUE;
            }
            else
            {
                stBufInfo.bEndOfStream = FALSE;
            }

            memcpy(stBufInfo.stFrameData.pVirAddr[0], gYUV + u32FrameIndexInHeaderFile * u32FrameSize, u32YSize);
            memcpy(stBufInfo.stFrameData.pVirAddr[1], gYUV + u32FrameIndexInHeaderFile * u32FrameSize + u32YSize, u32YSize >> 1);

            s32Ret = MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, FALSE);
            if (s32Ret != MI_SUCCESS)
            {
                printf("[ERR]%X Fail to put input port.\n", s32Ret);
            }

            ++u32HasSendFrames;
            printf("channelId[%u] u32HasSendFrames[%u]\n", stChnPort.u32ChnId, u32HasSendFrames);
            usleep(1000/_u32FrameRate *1000);
        }
        else
        {
            usleep(10*1000);
        }
    }

    return NULL;
}

void *venc_handle_output(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_U32 ret = MI_SUCCESS;
    MI_U32 u32GetFramesCount = 0;

    STUB_VencRes_t *pstStubVencRes = (STUB_VencRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = pstStubVencRes->u32DevId;
    stChnPort.u32ChnId = pstStubVencRes->u32ChnId;
    stChnPort.u32PortId = pstStubVencRes->u32PortId;

    while(pstStubVencRes->bOutThreadRunning)
    {
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack, 0, sizeof(stPack));
        stStream.pstPack = &stPack;
        stStream.u32PackCount = 1;
        ret = MI_VENC_Query(stChnPort.u32ChnId, &stStat);
        if(ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            usleep(5*1000);
            continue;
        }

        ret = MI_VENC_GetStream(stChnPort.u32ChnId, &stStream, FALSE);
        if(MI_SUCCESS == ret)
        {
            MI_VENC_ReleaseStream(stChnPort.u32ChnId, &stStream);

            ++u32GetFramesCount;
            printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);
        }
    }

    return NULL;
}


static MI_S32 STUB_GetVencConfig(MI_VENC_ModType_e eModType, MI_VENC_ChnAttr_t *pstVencChnAttr)
{
    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicHeight = STUB_VENC_PICTURE_HEIGHT;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicHeight = STUB_VENC_PICTURE_HEIGHT;
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
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicHeight = STUB_VENC_PICTURE_HEIGHT;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicHeight = STUB_VENC_PICTURE_HEIGHT;
            pstVencChnAttr->stVeAttr.stAttrH265e.bByFrame = TRUE;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32Gop = 30;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32IQp = 25;
            pstVencChnAttr->stRcAttr.stAttrH265FixQp.u32PQp = 25;
        }
        break;
        case E_MI_VENC_MODTYPE_JPEGE:
        {
            pstVencChnAttr->stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicHeight = STUB_VENC_PICTURE_HEIGHT;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicWidth = STUB_VENC_PICTURE_WIDTH;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicHeight = STUB_VENC_PICTURE_HEIGHT;
            pstVencChnAttr->stVeAttr.stAttrJpeg.bByFrame = TRUE;
            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
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
    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_SYS_ChnPort_t stChnPort;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(MI_SYS_Init());


    /************************************************
    Step2:  init VENC
    *************************************************/
    u32VencPortId = 0;
    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; ++u32VencChnId)
    {
        memset(&stVencChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
        STUB_GetVencConfig(_stStubVencRes[u32VencChnId].eModType, &stVencChnAttr);
        STCHECKRESULT(MI_VENC_CreateChn(u32VencChnId, &stVencChnAttr));
        STCHECKRESULT(MI_VENC_StartRecvPic(u32VencChnId));
    }

    /************************************************
    Step3:  create thread to send/get frame thread
    *************************************************/
    u32VencPortId = 0;
    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; ++u32VencChnId)
    {
        STCHECKRESULT(MI_VENC_GetChnDevid(u32VencChnId, &u32VencDevId));
        _stStubVencRes[u32VencChnId].u32DevId = u32VencDevId;
        _stStubVencRes[u32VencChnId].u32ChnId = u32VencChnId;
        _stStubVencRes[u32VencChnId].u32PortId = u32VencPortId;
        _stStubVencRes[u32VencChnId].bInThreadRunning = TRUE;
        _stStubVencRes[u32VencChnId].bOutThreadRunning = TRUE;
        pthread_create(&_stStubVencRes[u32VencChnId].input_tid, NULL, venc_send_frame, &_stStubVencRes[u32VencChnId]);
        pthread_create(&_stStubVencRes[u32VencChnId].output_tid, NULL, venc_handle_output, &_stStubVencRes[u32VencChnId]);
    }

    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    u32VencDevId = 0;
    u32VencPortId = 0;
    for(u32VencChnId=0; u32VencChnId<_u32ChnNum; ++u32VencChnId)
    {
        _stStubVencRes[u32VencChnId].bInThreadRunning = FALSE;
        _stStubVencRes[u32VencChnId].bOutThreadRunning = FALSE;
        pthread_join(_stStubVencRes[u32VencChnId].input_tid, NULL);
        pthread_join(_stStubVencRes[u32VencChnId].output_tid, NULL);

        STCHECKRESULT(MI_VENC_StopRecvPic(u32VencChnId));
        STCHECKRESULT(MI_VENC_DestroyChn(u32VencChnId));
    }

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

void display_help()
{
    printf("\n");
    printf("Usage: prog [-f frame rate] [-n num of channel] [-i num of input frame]\n\n");
    printf("\t\t-h  : displays this help\n");
    printf("\t\t-r   : frame rate\n");
    printf("\t\t-n  : num of channel\n");
    printf("\t\t-i   : num of input frame\n");
    printf("\t\t-m : mod type. ex: 2:h264 3:h265 4:jpeg\n");
}

MI_S32 main(int argc, char **argv)
{
    MI_S32 s32Result;
    MI_U32 i;
    char *str = NULL;
    char ch = '\0';
    for(i = 0; i < STUB_VENC_CHN_NUM; i++)
    {
        _stStubVencRes[i].eModType= E_MI_VENC_MODTYPE_H264E;
    }
    while((s32Result = getopt(argc, argv, "r:i:n:m:h")) != -1)
    {
        switch(s32Result)
        {
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
                _u32MaxSendFrames = atoi(str);
                printf("_u32MaxSendFrames[%u]\n", _u32MaxSendFrames);
            }
            break;
            case 'n':
            {
                str = strtok(optarg, "_");
                _u32ChnNum = atoi(str);
                if(_u32ChnNum > STUB_VENC_CHN_NUM)
                {
                    printf("warning, the max channel num is[%u]\n", STUB_VENC_CHN_NUM);
                    _u32ChnNum = STUB_VENC_CHN_NUM;
                }
                printf("_u32ChnNum[%u]\n", _u32ChnNum);
            }
            break;
            case 'm':
            {
                str = strtok(optarg, "_");

                for(i = 0; i < STUB_VENC_CHN_NUM && str; i++)
                {
                    _stStubVencRes[i].eModType = atoi(str);
                    str = strtok(NULL, "_");
                    printf("channel[%u] modType[%u]\n", i, _stStubVencRes[i].eModType);
                }
            }
            break;
            default:
                display_help();
                break;
        }
    }

    STCHECKRESULT(STUB_BaseModuleInit());

    printf("press q to exit\n");
    while((ch = getchar()) != 'q')
    {
        printf("press q to exit\n");
        usleep(1*1000*1000);
    }

    STCHECKRESULT(STUB_BaseModuleDeinit());
    return 0;
}

