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
#include <sys/time.h>
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_sensor.h"
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

#define STUB_VPE_OUTPUT_PORT_NUM 3
#define STUB_VENC_CHN_NUM 4
#define STUB_DIVP_CHN_NUM 2

typedef enum
{
   STUB_SENSOR_TYPE_IMX323 = 0,
   STUB_SENSOR_TYPE_IMX291 = 1,
   STUB_SENSOR_TYPE_IMX307 = 2,
}STUB_SensorType_e;

typedef enum
{
   STUB_YUV_TYPE_420 = 0,
   STUB_YUV_TYPE_422 = 1,
}STUB_YuvType_e;

typedef struct STUB_VencRes_s
{
    MI_S32 s32ModuleId;
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    pthread_t tid;
    MI_BOOL bRunning;
    MI_U32 u32ViChn;
} STUB_StreamRes_t;

typedef struct
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32CropX;
    MI_U32 u32CropY;
    MI_U32 u32CropWidth;
    MI_U32 u32CropHeight;
    MI_S32 s32ModeId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    MI_BOOL bMirror;
    MI_BOOL bFlip;
    MI_SYS_PixelFormat_e ePixFormat;
    STUB_StreamRes_t stThreadInfo;
    MI_U32 u32ViChn;
}VPE_Param_t;

typedef struct
{
    MI_U32 u32SensorWidth;
    MI_U32 u32SensorHeight;
    STUB_SensorType_e u32SensorType;

    MI_U32 u32StreamCnt;
    VPE_Param_t stVpeParam[STUB_VPE_OUTPUT_PORT_NUM];
}STUB_Param_t;

typedef struct
{
    MI_U64 TStart;
    MI_U32 totalSize;
    MI_S32 gopCount;
    MI_S32 bitrate;
    float fps;
    MI_S32 gop;
}VideoInfoCalculator_t;

static MI_U64 getTimeUs()
{
    struct timeval tv;
    MI_U64 ret;
    gettimeofday(&tv, NULL);
    ret = (MI_U64)tv.tv_sec * 1000000 + tv.tv_usec;
    return ret;
}

void IncFrmCnt(VideoInfoCalculator_t * pCal, MI_S32 frameType)
{
    MI_U64 TEnd = 0;
    if(1 == frameType)
    {
        TEnd = getTimeUs();
        pCal->fps = (float)pCal->gopCount * 1000000 / (TEnd - pCal->TStart);
        pCal->gopCount = 0;
        pCal->TStart = getTimeUs();
        pCal->totalSize = 0;
    }

    pCal->gopCount++;
}

float GetFps(VideoInfoCalculator_t * pCal)
 {
        return pCal->fps;
 }

static MI_BOOL g_showFps = FALSE;
static MI_BOOL g_saveFile = TRUE;
STUB_Param_t g_stSubParam;
MI_SYS_PixelFormat_e g_vifRawType = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX;
MI_SYS_PixelFormat_e g_vpeRawType = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX;
MI_BOOL g_forceRawType = FALSE;
static MI_BOOL g_bSnap = FALSE;
static MI_U32 g_u32SnapPort  = 0;
MI_SYS_Rotate_e g_eRotate = E_MI_SYS_ROTATE_NONE;
MI_S32 g_3dnr = 2;
MI_S32 g_hdr = 0;
MI_S32 g_mirror = 0;

void test_saveSnapshot(char *buff, int buffLen, char *name)
{
    char filename[128];
    //char filenameto[256];
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
    printf("snap ok\n");
    //sprintf(filenameto, "cat /tmp/%s > /dev/mtd2", filename);
    //system(filenameto);
}

void *venc_channel_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 count = 0;
    //char filename[128];
    //FILE *pfile = NULL;
    VideoInfoCalculator_t stCal;
    MI_S32 frameType = 0;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = pstStubStreamRes->s32ModuleId;
    stChnPort.u32DevId = pstStubStreamRes->u32DevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32ChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32PortId;
    printf("%s u32PortId = %u\n", __func__, stChnPort.u32PortId);

    memset(&stCal, 0, sizeof(VideoInfoCalculator_t));

    //sprintf(filename, "vpe%d.yuv", stChnPort.u32ChnId);
    //pfile = fopen(filename, "wb");
    //if(NULL == pfile)
    //{
    //    printf("error: fopen venc.es failed\n");
    //}

    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while(pstStubStreamRes->bRunning)
    {
        hSysBuf = MI_HANDLE_NULL;
        usleep(10 * 1000);
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }
        count++;

        if(g_showFps)
        {
            frameType = (0 == count % 30) ? 1 : 0;
            IncFrmCnt(&stCal, frameType);
            if(frameType)
            {
                printf("port %u,  fps = %f\n", stChnPort.u32PortId, GetFps(&stCal));
            }
        }

        //printf("vpe eModId = %u, u32DevId = %u, u32ChnId = %u,  u32PortId = %u, addr = %p, len = %u\n",
	//		 stChnPort.eModId, stChnPort.u32DevId, stChnPort.u32ChnId, stChnPort.u32PortId, stBufInfo.stRawData.pVirAddr, );

        //printf("vpe getbuf sucess 3, u32PortId = %d, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p) size %u\n", stChnPort.u32PortId, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
       //     stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
       //     stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2], stBufInfo.stFrameData.u32BufSize);

	// printf("vpe getbuf sucess 31 count = %d, u32PortId = %d, size(%dx%d), stride(%d, %d, %d), Pixel %d, phyaddr(%llu, %llu, %llu) size %u\n", count, stChnPort.u32PortId, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
       //     stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
       //     stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.phyAddr[1], stBufInfo.stFrameData.phyAddr[2], stBufInfo.stFrameData.u32BufSize);

       // if(count == 1 && g_saveFile)
        //if(1 == stChnPort.u32PortId)
        //{
            //test_saveSnapshot(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, "yuv");
       //     if(pfile)
      //      {
      //          printf("write len = %u\n",stBufInfo.stFrameData.u32BufSize);
      //          fwrite(stBufInfo.stFrameData.pVirAddr[0], 1,  stBufInfo.stFrameData.u32BufSize, pfile);
      //          fflush(pfile);
      //      }
      //  }

         if(g_bSnap &&  pstStubStreamRes->u32ViChn == g_u32SnapPort)
         {
             g_bSnap = FALSE;
             test_saveSnapshot(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, "yuv");
         }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

static void setVifDevAttr(MI_VIF_DevAttr_t *pVifDev, STUB_SensorType_e sensorType)
{
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    //sensor init
    MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE);
    MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }

    MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,0);
    MI_SNR_Enable(E_MI_SNR_PAD_ID_0);

    MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info);

    pVifDev->eIntfMode = stPad0Info.eIntfMode;
    pVifDev->eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    pVifDev->eHDRType = E_MI_VIF_HDR_TYPE_OFF;

    if(pVifDev->eIntfMode == E_MI_VIF_MODE_BT656)
        pVifDev->eClkEdge = stPad0Info.unIntfAttr.stBt656Attr.eClkEdge;
    else
        pVifDev->eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;

    if(pVifDev->eIntfMode == E_MI_VIF_MODE_MIPI)
        pVifDev->eDataSeq =stPad0Info.unIntfAttr.stMipiAttr.eDataYUVOrder;
    else
        pVifDev->eDataSeq = E_MI_VIF_INPUT_DATA_YUYV;

    if(pVifDev->eIntfMode == E_MI_VIF_MODE_BT656)
        memcpy(&pVifDev->stSyncAttr, &stPad0Info.unIntfAttr.stBt656Attr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));

    pVifDev->eBitOrder = E_MI_VIF_BITORDER_NORMAL;
	
    return;
}

static void setVifChnAttr(MI_VIF_ChnPortAttr_t *pVifChnPortAttr, STUB_SensorType_e sensorType)
{
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    pVifChnPortAttr->stCapRect.u16X = stSnrPlane0Info.stCapRect.u16X;
    pVifChnPortAttr->stCapRect.u16Y = stSnrPlane0Info.stCapRect.u16Y;
    pVifChnPortAttr->stCapRect.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    pVifChnPortAttr->stCapRect.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    pVifChnPortAttr->stDestSize.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    pVifChnPortAttr->stDestSize.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
}

static MI_SYS_PixelFormat_e getVpeRawType(STUB_SensorType_e sensorType)
{
    MI_SYS_PixelFormat_e eVpeRawType = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_NUM;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);
    eVpeRawType =  (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    return eVpeRawType;
}

static MI_U32 getVpePortFromViChn(MI_U32 viChn)
{
    MI_U32 vpePort = 0;
    switch(viChn)
    {
    case 0:
	 vpePort = 0;
        break;
    case 1:
	 vpePort = 1;
        break;
    case 20:
	 vpePort = 3;
        break;
    default:
	 vpePort = 2;
        break;
    }
    return vpePort;
}

static MI_U32 getDivpChnFromViChn(MI_U32 viChn)
{
    MI_U32 divpChn = 0;
    switch(viChn)
    {
    case 0:
	 divpChn = -1;
        break;
    case 1:
	 divpChn = -1;
        break;
    case 2:
	 divpChn = -1;
        break;
    case 20:
	 divpChn = 0;
        break;
    default:
	 divpChn = viChn - 3;
        break;
    }
    return divpChn;
}

static MI_SYS_BindType_e getBindTypeFromViChn(MI_U32 viChn)
{
    MI_SYS_BindType_e bindType = 0;
    switch(viChn)
    {
    case 0:
	 bindType = 0;
        break;
    case 1:
	 bindType = 0;
        break;
    case 2:
	 bindType = 0;
        break;
    case 20:
	 bindType = E_MI_SYS_BIND_TYPE_REALTIME;
        break;
    default:
	 bindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        break;
    }
    return bindType;
}

static MI_S32 STUB_BaseModuleInit(void)
{
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;

    MI_VPE_ChannelAttr_t stVpeChnAttr;
    MI_VPE_PortMode_t stVpeMode;
    MI_VPE_ChannelPara_t stVpePara;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;
    MI_U32 u32VpePortId;

    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_U32 i = 0;

    MI_S32 s32DivpChnId;

    MI_SYS_BindType_e eBindType;
    //MI_U32 u32RingLineCnt;

    MI_SYS_WindowRect_t stCrop;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(MI_SYS_Init());

    /************************************************
    Step2:  init VIF
    *************************************************/
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    setVifDevAttr(&stVifDevAttr, g_stSubParam.u32SensorType);
    stVifDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    STCHECKRESULT(MI_VIF_SetDevAttr(u32VifDevId, &stVifDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(u32VifDevId));

    memset(&stVifChnPortAttr, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    setVifChnAttr(&stVifChnPortAttr, g_stSubParam.u32SensorType);
    if(g_forceRawType)
    {
        stVifChnPortAttr.ePixFormat = g_vifRawType;
        printf("force set vif raw type to be %d\n", stVifChnPortAttr.ePixFormat);
    }
    //stVifChnPortAttr.stCapRect.u16X = 108;
    //stVifChnPortAttr.stCapRect.u16Y = 28;
    //stVifChnPortAttr.stCapRect.u16Width  = 1920;
    //stVifChnPortAttr.stCapRect.u16Height = 1080;
    //stVifChnPortAttr.stDestSize.u16Width  = 1920;
    //stVifChnPortAttr.stDestSize.u16Height = 1080;
    //stVifChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    //stVifChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;

    printf("capRect: %u, %u, %u, %u, dest: %u, %u, ePixFormat: %d\n", stVifChnPortAttr.stCapRect.u16X, stVifChnPortAttr.stCapRect.u16Y,
		stVifChnPortAttr.stCapRect.u16Width,  stVifChnPortAttr.stCapRect.u16Height, stVifChnPortAttr.stDestSize.u16Width, stVifChnPortAttr.stDestSize.u16Height,
		stVifChnPortAttr.ePixFormat);

    //stVifChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GB;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    //stVifChnPortAttr.bMirror = FALSE;
    //stVifChnPortAttr.bFlip   = FALSE;
    //stVifChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    STCHECKRESULT(MI_VIF_SetChnPortAttr(u32VifChnId, u32VifPortId, &stVifChnPortAttr));
    STCHECKRESULT(MI_VIF_EnableChnPort(u32VifChnId, u32VifPortId));
    

    u32VpeDevId = 0;
    u32VpeChnId = 0;

    printf("MI_VPE_SetChannelRotation %d\n", g_eRotate);
    MI_VPE_SetChannelRotation(u32VpeChnId, g_eRotate);
    memset(&stVpeChnAttr, 0x0, sizeof(MI_VPE_ChannelAttr_t));
    stVpeChnAttr.u16MaxW = stVifChnPortAttr.stDestSize.u16Width;
    stVpeChnAttr.u16MaxH = stVifChnPortAttr.stDestSize.u16Height;
    stVpeChnAttr.bNrEn = FALSE;
    stVpeChnAttr.bEsEn = FALSE;
    stVpeChnAttr.bEdgeEn = FALSE;
    stVpeChnAttr.bUvInvert = FALSE;
    stVpeChnAttr.bContrastEn = FALSE;
    stVpeChnAttr.ePixFmt  = getVpeRawType(g_stSubParam.u32SensorType);
    if(g_forceRawType)
    {
        stVpeChnAttr.ePixFmt = g_vpeRawType;
        printf("force set vpe raw type to be %d\n", stVpeChnAttr.ePixFmt);
    }

    printf("vpe raw type %d\n", stVpeChnAttr.ePixFmt);

    stVpeChnAttr.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
    stVpeChnAttr.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    STCHECKRESULT(MI_VPE_CreateChannel(u32VpeChnId, &stVpeChnAttr));

    memset(&stVpePara, 0x0, sizeof(MI_VPE_ChannelPara_t));
    stVpePara.e3DNRLevel = g_3dnr;
    stVpePara.bMirror = g_mirror & 0x1;
    stVpePara.bFlip = g_mirror & 0x2;
    printf("vpe 3dnr %d, bMirror %d, bFlip %d\n", stVpePara.e3DNRLevel, stVpePara.bMirror, stVpePara.bFlip);
    MI_VPE_SetChannelParam(u32VpeChnId, &stVpePara);
    STCHECKRESULT(MI_VPE_StartChannel(u32VpeChnId));

    for(i=0; i<g_stSubParam.u32StreamCnt; ++i)
    {
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        u32VpePortId = getVpePortFromViChn(g_stSubParam.stVpeParam[i].u32ViChn);
        stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpeMode.ePixelFormat  = g_stSubParam.stVpeParam[i].ePixFormat;
        stVpeMode.u16Width = g_stSubParam.stVpeParam[i].u32Width;
        stVpeMode.u16Height = g_stSubParam.stVpeParam[i].u32Height;
        stVpeMode.bMirror = g_stSubParam.stVpeParam[i].bMirror;
        stVpeMode.bFlip = g_stSubParam.stVpeParam[i].bFlip;
        STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, u32VpePortId, &stVpeMode));
        STCHECKRESULT(MI_VPE_GetPortMode(u32VpeChnId, u32VpePortId, &stVpeMode));
        printf("Get VpeMode(port:%d w:%d h:%d PixelFormat:%d CompressMode:%d, mirror %d, flip %d)\n",
                u32VpePortId, stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat, stVpeMode.eCompressMode,
                stVpeMode.bMirror, stVpeMode.bFlip);

        if(g_stSubParam.stVpeParam[i].u32CropWidth > 0 && g_stSubParam.stVpeParam[i].u32CropHeight > 0)
        {
            memset(&stCrop, 0x00, sizeof(stCrop));
            stCrop.u16X = g_stSubParam.stVpeParam[i].u32CropX;
            stCrop.u16Y = g_stSubParam.stVpeParam[i].u32CropY;
            stCrop.u16Width = g_stSubParam.stVpeParam[i].u32CropWidth;
            stCrop.u16Height = g_stSubParam.stVpeParam[i].u32CropHeight;
            printf("MI_VPE_SetPortCrop(port:%d x: %u,  y: %u, w:%u, h:%u)\n",
                u32VpePortId, stCrop.u16X, stCrop.u16Y, stCrop.u16Width, stCrop.u16Height);
            MI_VPE_SetPortCrop(u32VpeChnId, u32VpePortId, &stCrop);
        }

        g_stSubParam.stVpeParam[i].s32ModeId = E_MI_MODULE_ID_VPE;
        g_stSubParam.stVpeParam[i].u32ChnId = u32VpeChnId;
        g_stSubParam.stVpeParam[i].u32PortId = u32VpePortId;

        s32DivpChnId = getDivpChnFromViChn(g_stSubParam.stVpeParam[i].u32ViChn);
        if(s32DivpChnId >= 0)
        {
            MI_DIVP_ChnAttr_t stAttr;
            MI_DIVP_OutputPortAttr_t stOutputPortAttr;
            memset(&stAttr, 0, sizeof(stAttr));
            memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
            stAttr.bHorMirror = FALSE;
            stAttr.bVerMirror = FALSE;
            stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
            stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
            stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
            stAttr.stCropRect.u16X = 0;
            stAttr.stCropRect.u16Y = 0;
            stAttr.stCropRect.u16Width = g_stSubParam.stVpeParam[i].u32Width;
            stAttr.stCropRect.u16Height = g_stSubParam.stVpeParam[i].u32Height;
            stAttr.u32MaxWidth = g_stSubParam.stVpeParam[i].u32Width;
            stAttr.u32MaxHeight = g_stSubParam.stVpeParam[i].u32Height;
            MI_DIVP_CreateChn(s32DivpChnId, &stAttr);

            stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
            stOutputPortAttr.u32Width = g_stSubParam.stVpeParam[i].u32Width;
            stOutputPortAttr.u32Height = g_stSubParam.stVpeParam[i].u32Height;
            MI_DIVP_SetOutputPortAttr(s32DivpChnId, &stOutputPortAttr);
            MI_DIVP_StartChn(s32DivpChnId);

            g_stSubParam.stVpeParam[i].s32ModeId = E_MI_MODULE_ID_DIVP;
            g_stSubParam.stVpeParam[i].u32ChnId = s32DivpChnId;
            g_stSubParam.stVpeParam[i].u32PortId = 0;

            //Bind VIF(0, 0, 0)->VPE(0, 0, 0)
            memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stSrcChnPort.u32DevId = 0;
            stSrcChnPort.u32ChnId = u32VpeChnId;
            stSrcChnPort.u32PortId = u32VpePortId;
            stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stDstChnPort.u32DevId = 0;
            stDstChnPort.u32ChnId = s32DivpChnId;
            stDstChnPort.u32PortId = 0;
            u32SrcFrmrate = 30;
            u32DstFrmrate = 30;
            eBindType = getBindTypeFromViChn(g_stSubParam.stVpeParam[i].u32ViChn);;
            STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));
        }

        STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, u32VpePortId));
        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = g_stSubParam.stVpeParam[i].s32ModeId;
        stChnPort.u32DevId = 0;
        stChnPort.u32ChnId = g_stSubParam.stVpeParam[i].u32ChnId;
        stChnPort.u32PortId = g_stSubParam.stVpeParam[i].u32PortId;
        STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 5));
    }


    //Bind VIF(0, 0, 0)->VPE(0, 0, 0)
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    u32VpePortId = 0;
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId = u32VifDevId;
    stSrcChnPort.u32ChnId = u32VifChnId;
    stSrcChnPort.u32PortId = u32VifPortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId = u32VpeDevId;
    stDstChnPort.u32ChnId = u32VpeChnId;
    stDstChnPort.u32PortId = u32VpePortId;
    u32SrcFrmrate = 30;
    u32DstFrmrate = 30;
    eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));
    //STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate));


    u32VpeDevId = 0;
    u32VpeChnId = 0;
    for(i=0; i<g_stSubParam.u32StreamCnt; ++i)
    {
        
        g_stSubParam.stVpeParam[i].stThreadInfo.s32ModuleId = g_stSubParam.stVpeParam[i].s32ModeId;
        g_stSubParam.stVpeParam[i].stThreadInfo.u32DevId = 0;
        g_stSubParam.stVpeParam[i].stThreadInfo.u32ChnId = g_stSubParam.stVpeParam[i].u32ChnId;
        g_stSubParam.stVpeParam[i].stThreadInfo.u32PortId = g_stSubParam.stVpeParam[i].u32PortId;
        g_stSubParam.stVpeParam[i].stThreadInfo.u32ViChn = g_stSubParam.stVpeParam[i].u32ViChn;
        g_stSubParam.stVpeParam[i].stThreadInfo.bRunning = TRUE;
        pthread_create(& g_stSubParam.stVpeParam[i].stThreadInfo.tid, NULL, venc_channel_func, &g_stSubParam.stVpeParam[i].stThreadInfo);
    }

    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;
    MI_U32 i = 0;
    MI_S32 s32DivpChnId;

    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    u32VpeDevId = 0;
    u32VpeChnId = 0;

    for(i=0; i<g_stSubParam.u32StreamCnt; ++i)
    {
        g_stSubParam.stVpeParam[i].stThreadInfo.bRunning = FALSE;
	 pthread_join(g_stSubParam.stVpeParam[i].stThreadInfo.tid, NULL);
        g_stSubParam.stVpeParam[i].stThreadInfo.tid = 0;

        s32DivpChnId = getDivpChnFromViChn(g_stSubParam.stVpeParam[i].u32ViChn);
        if(s32DivpChnId > 0)
        {
            MI_DIVP_StopChn(s32DivpChnId);
		
            memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stSrcChnPort.u32DevId = u32VpeDevId;
            stSrcChnPort.u32ChnId = u32VpeChnId;
            stSrcChnPort.u32PortId = getVpePortFromViChn(g_stSubParam.stVpeParam[i].u32ViChn);
            stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stDstChnPort.u32DevId = 0;
            stDstChnPort.u32ChnId = s32DivpChnId;
            stDstChnPort.u32PortId = 0;
            STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));
        }

        STCHECKRESULT(MI_VPE_DisablePort(u32VpeChnId, getVpePortFromViChn(g_stSubParam.stVpeParam[i].u32ViChn)));
    }

    STCHECKRESULT(MI_VPE_StopChannel(u32VpeChnId));

    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId = u32VifDevId;
    stSrcChnPort.u32ChnId = u32VifChnId;
    stSrcChnPort.u32PortId = u32VifPortId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId = u32VpeDevId;
    stDstChnPort.u32ChnId = u32VpeChnId;
    stDstChnPort.u32PortId = 0;
    STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

    STCHECKRESULT(MI_VPE_DestroyChannel(u32VpeChnId));

    STCHECKRESULT(MI_VIF_DisableChnPort(u32VifChnId, u32VifPortId));
    STCHECKRESULT(MI_VIF_DisableDev(u32VifDevId));

    STCHECKRESULT(MI_SYS_Exit());
    return MI_SUCCESS;
}

void initDefaultParam(void)
{
    MI_S32 i;
    g_stSubParam.u32SensorType = 0;
    g_stSubParam.u32SensorWidth = 1920;
    g_stSubParam.u32SensorHeight = 1080;
    g_stSubParam.u32StreamCnt = 1;

    g_stSubParam.stVpeParam[0].u32Width = 1920;
    g_stSubParam.stVpeParam[0].u32Height = 1080;

     for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM; i++)
     {
         g_stSubParam.stVpeParam[i].u32PortId = i;
         g_stSubParam.stVpeParam[i].ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
         g_stSubParam.stVpeParam[i].u32ViChn = i;
     }
}

void display_help()
{
    printf("\n");
    printf("Usage: prog [-f frame rate] [-n num of channel][-m mod type]\n\n");
    printf("\t\t-c  : set vpe port crop window. eg: -c 1280_736\n");
    printf("\t\t-n  : num of channel. eg: -n 2\n");
    printf("\t\t-r  : set resolution of each channel. eg: -n 2 -r 1280_736_352_288\n");
    printf("\t\t-h  : displays this help\n");
}

MI_S32 main(int argc, char **argv)
{
    char ch;
    MI_S32 s32Result;
    char *str = NULL;
    MI_S32 i;

    memset(&g_stSubParam, 0, sizeof(STUB_Param_t));
    initDefaultParam();
    
    while((s32Result = getopt(argc, argv, "b:c:e:f:m:M:n:r:T:u:V:h")) != -1)
    {
        switch(s32Result)
        {
            case 'b':
            {
                str = strtok(optarg, "_");
                g_showFps = atoi(str);
                printf("g_showFps = %d \n", g_showFps);
            }
            break;
            case 'c':
            {
                printf("test crop\n");
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    g_stSubParam.stVpeParam[i].u32CropX = atoi(str);

                    str = strtok(NULL, "_");
	             if(str)
	             {
	                 g_stSubParam.stVpeParam[i].u32CropY = atoi(str);
	             }

                    str = strtok(NULL, "_");
                    if(str)
	             {
	                 g_stSubParam.stVpeParam[i].u32CropWidth= atoi(str);
	             }

                    str = strtok(NULL, "_");
                    if(str)
	             {
	                 g_stSubParam.stVpeParam[i].u32CropHeight = atoi(str);
	             }

                    str = strtok(NULL, "_");

                    printf("port[%d], crop(%u, %u, %u, %u)\n", i, g_stSubParam.stVpeParam[i].u32CropX, g_stSubParam.stVpeParam[i].u32CropY,
						g_stSubParam.stVpeParam[i].u32CropWidth, g_stSubParam.stVpeParam[i].u32CropHeight);
                }
            }
            break;
            case 'e':
            {
                str = strtok(optarg, "_");
                g_3dnr = atoi(str);
                printf("3dnr = %d\n", g_3dnr);
            }
            break;
            case 'f':
            {
                MI_S32 tmp = 0;
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    tmp = atoi(str);
                    g_stSubParam.stVpeParam[i].bMirror = tmp & 0x1;
                    g_stSubParam.stVpeParam[i].bFlip = (tmp & 0x2) >> 1;


                    str = strtok(NULL, "_");

                    printf("port[%d], mirror/flip(%d, %d)\n", i, g_stSubParam.stVpeParam[i].bMirror, g_stSubParam.stVpeParam[i].bFlip);
                }
            }
            break;
            case 'm':
            {
                MI_S32 tmp = 0;
                str = strtok(optarg, "_");
                tmp = atoi(str);
			
                if(90 == tmp)
                 {
                     g_eRotate = E_MI_SYS_ROTATE_90;
                 }
                 else if(180 == tmp)
                 {
                     g_eRotate = E_MI_SYS_ROTATE_180;
                 }
                 else if(270 == tmp)
                 {
                     g_eRotate = E_MI_SYS_ROTATE_270;
                 }
                 else
                 {
                     g_eRotate = E_MI_SYS_ROTATE_NONE;
                 }
                 printf("rotate = %d\n", g_eRotate);
            }
            break;
            case 'M':
            {
                str = strtok(optarg, "_");
                g_mirror = atoi(str);
            }
             break;
            case 'n':
            {
                str = strtok(optarg, "_");
                g_stSubParam.u32StreamCnt = atoi(str);
                if(g_stSubParam.u32StreamCnt > STUB_VPE_OUTPUT_PORT_NUM)
                {
                    printf("warning, the max channel num is[%u]\n", STUB_VPE_OUTPUT_PORT_NUM);
                    g_stSubParam.u32StreamCnt = STUB_VPE_OUTPUT_PORT_NUM;
                }
                printf("_u32ChnNum[%u]\n", g_stSubParam.u32StreamCnt);
            }
            break;
            case 'r':
            {
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    g_stSubParam.stVpeParam[i].u32Width= atoi(str);

                    str = strtok(NULL, "_");
	             if(str)
	             {
	                 g_stSubParam.stVpeParam[i].u32Height = atoi(str);
	             }

                    str = strtok(NULL, "_");

                    printf("port[%d], size(%u, %u)\n", i, g_stSubParam.stVpeParam[i].u32Width, g_stSubParam.stVpeParam[i].u32Height);
                }
            }
            break;
            case 'R':
            {
                str = strtok(optarg, "_");
                if(NULL != str)
                {
                    g_stSubParam.u32SensorWidth = atoi(str);
                }
                str = strtok(NULL, "_");
                if(NULL != str)
                {
                    g_stSubParam.u32SensorHeight = atoi(str);
                }
                printf("SensorWidth = %u, SensorHeight = %u\n", g_stSubParam.u32SensorWidth, g_stSubParam.u32SensorHeight);
            }
            break;
            case 's':
            {
                str = strtok(optarg, "_");
                g_stSubParam.u32SensorType = atoi(str);
                printf("sensor type %u\n", g_stSubParam.u32SensorType);
            }
            break;
            case 't':
            {
                str = strtok(optarg, "_");
                g_vifRawType = atoi(str);
                str = strtok(NULL, "_");
                g_vpeRawType = atoi(str);
                g_forceRawType = TRUE;
                printf("g_vifRawType %d, g_vpeRawType = %d\n", g_vpeRawType, g_forceRawType);
            }
            break;
            case 'T':
            {
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    g_stSubParam.stVpeParam[i].ePixFormat= atoi(str);

                    str = strtok(NULL, "_");

                    printf("port[%d], ePixFormat: %d\n", i, g_stSubParam.stVpeParam[i].ePixFormat);
                }
            }
            case 'u':
            {
                str = strtok(optarg, "_");
                g_saveFile = atoi(str);
                printf("g_saveFile %d\n", g_saveFile);
            }
            break;
            case 'V':
            {
                str = strtok(optarg, "_");
                for(i = 0; i < STUB_VPE_OUTPUT_PORT_NUM && str; i++)
                {
                    g_stSubParam.stVpeParam[i].u32ViChn= atoi(str);

                    str = strtok(NULL, "_");

                    printf("port[%d], u32ViChn: %d\n", i, g_stSubParam.stVpeParam[i].u32ViChn);
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
        if('k' == ch)
        {
            printf("Please input port:");
            scanf("%u", &g_u32SnapPort);
            printf("snap port: %u\n", g_u32SnapPort);
            g_bSnap = TRUE;
        }
        printf("press q to exit\n");
        usleep(1*1000*1000);
    }
    printf("exit\n");

    STCHECKRESULT(STUB_BaseModuleDeinit());
    return 0;
}

