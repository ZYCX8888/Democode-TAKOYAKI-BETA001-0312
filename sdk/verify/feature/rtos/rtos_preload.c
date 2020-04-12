/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__linux__)
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_rgn.h"
#include "mi_sensor_datatype.h"
#include "cam_os_wrapper.h"
#include "sys_sys_isw_cli.h"
#include "mi_sensor.h"
#include "mi_vpe_impl.h"
#include "mi_vif_impl.h"
#include "mi_venc_impl.h"
#include "mi_sensor_impl.h"
#include "mi_rgn_internal.h"
#include "mi_divp.h"
#include "mi_divp_impl.h"
#include "mi_ai.h"
#include "CameraSetting.h"

#define MI_AUDIO_SAMPLE_PER_FRAME 1024

/*=============================================================*/
// Global Variable definition
/*=============================================================*/
typedef struct _AiChn_s {
    MI_AUDIO_DEV AiDevId;
    MI_AI_CHN AiChn;
    CamOsThread tid;
    MI_S8 szOutFilePath[128];
    MI_S32 S32StopFrameSize;
}_AiChn_t;

#if defined(__VER_MI_AIO__)
#define TTFF10SEC_PCM_16K_16BIT_1CH     (10*16000*2)

static MI_BOOL  blAiBootTest = FALSE;
static MI_S32 s32StopFrameCntsize = 64;
static MI_BOOL blAoTest = FALSE;
static MI_BOOL blExit = FALSE;

MI_S32 gTTFF10SecBuffAddr = 0;
MI_S32 gTTFF10SecBuffAddrLen = 0;
MI_BOOL gTTFF10SecDone = 0;
CamOsTsem_t ai_sem;
static _AiChn_t _astAiChn[MI_AUDIO_MAX_CHN_NUM];
#else
static _AiChn_t _astAiChn[2];
#endif

static MI_S32 s32ChanNum = 0;
static MI_AUDIO_DEV AiDevId = 0; // default set device 0
static MI_BOOL bEnableRes = FALSE;
static MI_BOOL bEnableVqe = FALSE;

extern MI_S32 MI_AI_Init(void);

#define BOOT_TEST 1
#if 1
#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        return 1;\
    }
#else
#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        CamOsPrintf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        CamOsPrintf("[%s %d]exec function pass\n", __FUNCTION__,__LINE__);\
    }
#endif
#define RGB_TO_CRYCB(r, g, b)                                                            \
    (((unsigned int)(( 0.439f * (r) - 0.368f * (g) - 0.071f * (b)) + 128.0f)) << 16) |    \
    (((unsigned int)(( 0.257f * (r) + 0.564f * (g) + 0.098f * (b)) + 16.0f)) << 8) |        \
    (((unsigned int)((-0.148f * (r) - 0.291f * (g) + 0.439f * (b)) + 128.0f)))

#define VPE_PORT1_OSD_HANDLE    100
#define VPE_PORT1_COVER_HANDLE  101
#define VPE_PORT1_COVER_1_HANDLE  102


typedef struct STUB_Resolution_s
{
    MI_U32 u32MaxWidth;
    MI_U32 u32MaxHeight;
    MI_U32 u32OutWidth;
    MI_U32 u32OutHeight;
} STUB_Resolution_t;


typedef struct STUB_VencRes_s
{
    MI_VENC_ModType_e eModType;
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    CamOsThread tid;
    MI_BOOL bThreadRunning;
} STUB_VencRes_t;

typedef enum
{
   STUB_SENSOR_TYPE_IMX323 = 0,
   STUB_SENSOR_TYPE_IMX291 = 1,
   STUB_SENSOR_TYPE_IMX307 = 2,
}STUB_SensorType_e;


#define STUB_VENC_CHN_NUM 1
#define STUB_VENC_RESOLUTION_NUM 3

STUB_VencRes_t _stStubVencRes[STUB_VENC_CHN_NUM];
STUB_Resolution_t _stVifResolution[1] =
{
    {1920, 1080, 1920, 1080},
};

STUB_Resolution_t _stVpeResolution[STUB_VENC_RESOLUTION_NUM] =
{
    {1920, 1080, 1920, 1080},
    {1280, 720, 1280, 720},
    {640, 360, 640, 360},
};


STUB_Resolution_t _stVencResolution[STUB_VENC_RESOLUTION_NUM] =
{
    {1920, 1080, 1920, 1080},
    {1920, 1080, 1280, 720},
    {1920, 1080, 640, 360},
};

MI_RGN_PaletteTable_t _gstPaletteTable =
{
    { //index0 ~ index15
         {255,   0,   0,   0}, {255, 255,   0,   0}, {255,   0, 255,   0}, {255,   0,   0, 255},
         {255, 255, 255,   0}, {255,   0, 112, 255}, {255,   0, 255, 255}, {255, 255, 255, 255},
         {255, 128,   0,   0}, {255, 128, 128,   0}, {255, 128,   0, 128}, {255,   0, 128,   0},
         {255,   0,   0, 0}, {255,   0, 128, 128}, {255, 128, 128, 128}, {255,  64,  64,  64},
         //index16 ~ index31
         {  0,   0,   0,   0}, {  0,   0,   0,  30}, {  0,   0, 255,  60}, {  0, 128,   0,  90},
         {255,   0,   0, 120}, {  0, 255, 255, 150}, {255, 255,   0, 180}, {  0, 255,   0, 210},
         {255,   0, 255, 240}, {192, 192, 192, 255}, {128, 128, 128,  10}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index32 ~ index47
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index48 ~ index63
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index64 ~ index79
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index80 ~ index95
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index96 ~ index111
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index112 ~ index127
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index128 ~ index143
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index144 ~ index159
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index160 ~ index175
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index176 ~ index191
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index192 ~ index207
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index208 ~ index223
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index224 ~ index239
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         // (index236 :192,160,224 defalut colorkey)
         {192, 160, 224, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index240 ~ index255
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {192, 160, 224, 255}
    }
};


static MI_VENC_ModType_e _aeModType[STUB_VENC_CHN_NUM] = {E_MI_VENC_MODTYPE_H264E};
static MI_U32 _u32ChnNum = 1;
static MI_U32 _u32RingLineCnt = 1088;
static MI_SYS_BindType_e _aeBindType[STUB_VENC_CHN_NUM] = {E_MI_SYS_BIND_TYPE_FRAME_BASE};
static MI_SYS_PixelFormat_e _eVpePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

static MI_U32 _u32VpePortIdInUse = 1;
MI_U32 _u32Bitrate[8] =
{
	2097152,
	1572864,
	1048576,
	768000,
	512000,
	409600,
	358400,
	307200
};

MI_U8 _u8framerate[4] =
{
	30,
	24,
	15,
    8
};

typedef struct _MaxFrameSize {
    MI_U32 frameSizeI;
    MI_U32 frameSizeP;
} MaxFrameSize;

static const MaxFrameSize _maxFrameSize[8] =
{
    {84 * 1024 * 8,  63 * 1024 * 8 },
    {63 * 1024 * 8,  47 * 1024 * 8 },
    {42 * 1024 * 8,  32 * 1024 * 8 },
    {32 * 1024 * 8,  24 * 1024 * 8 },
    {21 * 1024 * 8,  16 * 1024 * 8 },
    {21 * 1024 * 8,  16 * 1024 * 8 },
    {20 * 1024 * 8,  15 * 1024 * 8 },
    {20 * 1024 * 8,  15 * 1024 * 8 }
};

static MI_U32 _u32CropWidth = 0;
static MI_U32 _u32CropHeight = 0;

static STUB_SensorType_e _u32SensorType = STUB_SENSOR_TYPE_IMX323;

static MI_BOOL _bWriteFile = FALSE;

MI_VIF_DevAttr_t DEV_ATTR_DVP_BASE_venc =
{
    E_MI_VIF_MODE_DIGITAL_CAMERA,
    E_MI_VIF_WORK_MODE_RGB_REALTIME,
    E_MI_VIF_HDR_TYPE_OFF,
    E_MI_VIF_CLK_EDGE_DOUBLE,
    E_MI_VIF_INPUT_DATA_YUYV,
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_MIPI_BASE_venc =
{
    E_MI_VIF_MODE_MIPI,
    E_MI_VIF_WORK_MODE_RGB_REALTIME,
    E_MI_VIF_HDR_TYPE_OFF,
    E_MI_VIF_CLK_EDGE_DOUBLE,
    E_MI_VIF_INPUT_DATA_YUYV,
    FALSE
};

static void setVifDevAttr(MI_VIF_DevAttr_t *pVifDev, STUB_SensorType_e sensorType)
{
    switch(sensorType)
    {
    case STUB_SENSOR_TYPE_IMX323:
        memcpy(pVifDev, &DEV_ATTR_DVP_BASE_venc, sizeof(MI_VIF_DevAttr_t));
        break;
    case STUB_SENSOR_TYPE_IMX291:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_venc, sizeof(MI_VIF_DevAttr_t));
        break;
    case STUB_SENSOR_TYPE_IMX307:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_venc, sizeof(MI_VIF_DevAttr_t));
        break;
    default:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_venc, sizeof(MI_VIF_DevAttr_t));
        break;
    }
    pVifDev->eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    return;
}

static void setVifChnAttr(MI_VIF_ChnPortAttr_t *pVifChnPortAttr, STUB_SensorType_e sensorType)
{
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    switch(sensorType)
    {
    case STUB_SENSOR_TYPE_IMX323:
        pVifChnPortAttr->stCapRect.u16X = 108;
        pVifChnPortAttr->stCapRect.u16Y = 28;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    case STUB_SENSOR_TYPE_IMX291:
        pVifChnPortAttr->stCapRect.u16X = 0;
        pVifChnPortAttr->stCapRect.u16Y = 0;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    case STUB_SENSOR_TYPE_IMX307:
        pVifChnPortAttr->stCapRect.u16X = 0;
        pVifChnPortAttr->stCapRect.u16Y = 0;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    default:
        pVifChnPortAttr->stCapRect.u16X = 108;
        pVifChnPortAttr->stCapRect.u16Y = 28;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    }

    _stVifResolution[0].u32MaxWidth= stSnrPlane0Info.stCapRect.u16Width;
    _stVifResolution[0].u32MaxHeight = stSnrPlane0Info.stCapRect.u16Height;
    _stVifResolution[0].u32OutWidth = stSnrPlane0Info.stCapRect.u16Width;
    _stVifResolution[0].u32OutHeight = stSnrPlane0Info.stCapRect.u16Height;

    pVifChnPortAttr->stCapRect.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    pVifChnPortAttr->stCapRect.u16Height = stSnrPlane0Info.stCapRect.u16Height;
    pVifChnPortAttr->stDestSize.u16Width  = stSnrPlane0Info.stCapRect.u16Width;
    pVifChnPortAttr->stDestSize.u16Height = stSnrPlane0Info.stCapRect.u16Height;

    pVifChnPortAttr->eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    pVifChnPortAttr->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    //E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pVifChnPortAttr->eFrameRate = E_MI_VIF_FRAMERATE_FULL;
}

static MI_SYS_PixelFormat_e getVpeRawType_venc(STUB_SensorType_e sensorType)
{
    MI_SYS_PixelFormat_e eVpeRawType = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_NUM;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);
    eVpeRawType =  (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    return eVpeRawType;
}

static MI_S32 STUB_GetVencConfig(MI_VENC_ModType_e eModType, MI_VENC_ChnAttr_t *pstVencChnAttr, MI_U32 u32ChnId)
{
    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicWidth = _stVencResolution[0].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32PicHeight = _stVencResolution[0].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicWidth = _stVencResolution[0].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32MaxPicHeight = _stVencResolution[0].u32MaxHeight;
            pstVencChnAttr->stVeAttr.stAttrH264e.bByFrame = TRUE;
            pstVencChnAttr->stVeAttr.stAttrH264e.u32Profile = 1;

            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32BitRate = _u32Bitrate[0];
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32Gop = 60;
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = _u8framerate[0];
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            pstVencChnAttr->stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        }
        break;
        case E_MI_VENC_MODTYPE_H265E:
        {
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicWidth = _stVencResolution[u32ChnId].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32PicHeight = _stVencResolution[u32ChnId].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicWidth = _stVencResolution[u32ChnId].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrH265e.u32MaxPicHeight = _stVencResolution[u32ChnId].u32MaxHeight;
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
            pstVencChnAttr->stVeAttr.eType = eModType;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicWidth = _stVencResolution[u32ChnId].u32MaxWidth;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32MaxPicHeight = _stVencResolution[u32ChnId].u32MaxHeight;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicWidth = _stVencResolution[u32ChnId].u32OutWidth;
            pstVencChnAttr->stVeAttr.stAttrJpeg.u32PicHeight = _stVencResolution[u32ChnId].u32OutHeight;
            pstVencChnAttr->stVeAttr.stAttrJpeg.bByFrame = TRUE;
            pstVencChnAttr->stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        }
        break;
        default:
            CamOsPrintf("unsupport eModType[%u]\n", eModType);
            return E_MI_ERR_FAILED;
    }

    return MI_SUCCESS;
}
#ifdef CONFIG_SIGMASTAR_CHIP_I6E
extern MI_S32 MI_MipiTx_IMPL_Init(void);
#endif
extern void MI_VENC_IMPL_Init(void);
extern MI_S32 mi_rgn_DrvInit(void);
extern MI_S32 mi_rgn_Init(void);
extern MI_S32 mi_divp_Init(void);

void *rtos_vpe_func(void* p)
{
    CamOsThread tid = *(CamOsThread *)p;
	MI_SYS_ChnPort_t stChnPort;
	MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_S32 s32Ret;

	stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 2;


	while (1)
	{
        CamOsUsSleep(1000);
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);

        if(MI_SUCCESS != s32Ret)
        {
			//CamOsPrintf("MI_SYS_ChnOutputPortGetBuf err, %x\n", s32Ret);
			continue;
        }

        MI_SYS_ChnOutputPortPutBuf(stBufHandle);
	MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 4);
		break;
	}
	CamOsThreadStop(tid);
	return NULL;
}

static MI_S32 STUB_BaseModuleInit_vif_vpe_venc(void)
{
    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;
    MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;

    MI_VPE_ChannelAttr_t stVpeChnAttr;
    MI_VPE_PortMode_t stVpePortMode;
    MI_U32 u32VpeDevId;
    MI_U32 u32VpeChnId;

    MI_VENC_ChnAttr_t stVencChnAttr;
    MI_U32 u32VencDevId;
    MI_U32 u32VencChnId;
    MI_U32 u32VencPortId;

    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
    MI_SYS_BindType_e eBindType;

    MI_SYS_WindowRect_t stVpeCrop;
 //   MI_RGN_Attr_t stRgnAttr;
 //   MI_RGN_ChnPort_t stAttachChnPort;
//    MI_RGN_CanvasInfo_t stCanvasInfo;
 //   MI_RGN_ChnPortParam_t stChnPortParam;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
	MI_VENC_SuperFrameCfg_t stSuperFrameCfg = { E_MI_VENC_SUPERFRM_REENCODE, 0, 0, 0 };
    CameraBootSetting_t* pCameraBootSetting = CameraBootSettingGetHandle();


    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(MI_SYS_Init());

    MI_VIF_IMPL_Init();
    MI_VPE_IMPL_Init();
    MI_VENC_IMPL_Init();
    #ifdef CONFIG_SIGMASTAR_CHIP_I6E
    MI_MipiTx_IMPL_Init();
    #endif
  //  mi_rgn_DrvInit();
  //  mi_rgn_Init();

    MI_SNR_SetPlaneMode(0,0);
	MI_SNR_SetFps(0,pCameraBootSetting->u8SensorFrameRate);
	MI_SNR_Enable(0);
    /************************************************
    Step2:  init VIF
    *************************************************/
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    setVifDevAttr(&stVifDevAttr, _u32SensorType);
    stVifDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    STCHECKRESULT(MI_VIF_SetDevAttr(u32VifDevId, &stVifDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(u32VifDevId));

    memset(&stVifChnPortAttr, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    setVifChnAttr(&stVifChnPortAttr, _u32SensorType);
    STCHECKRESULT(MI_VIF_SetChnPortAttr(u32VifChnId, u32VifPortId, &stVifChnPortAttr));
    STCHECKRESULT(MI_VIF_EnableChnPort(u32VifChnId, u32VifPortId));
    /************************************************
    Step3:  init VPE
    *************************************************/
    u32VpeDevId = 0;
    u32VpeChnId = 0;
    memset(&stVpeChnAttr, 0x0, sizeof(MI_VPE_ChannelAttr_t));
    stVpeChnAttr.u16MaxW = _stVifResolution[0].u32OutWidth;
    stVpeChnAttr.u16MaxH = _stVifResolution[0].u32OutHeight;
    stVpeChnAttr.bNrEn = FALSE;
    stVpeChnAttr.bEsEn = FALSE;
    stVpeChnAttr.bEdgeEn = FALSE;
    stVpeChnAttr.bUvInvert = FALSE;
    stVpeChnAttr.bContrastEn = FALSE;
    stVpeChnAttr.ePixFmt  = getVpeRawType_venc(_u32SensorType);
    stVpeChnAttr.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
    stVpeChnAttr.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChnAttr.eSensorBindId = E_MI_VPE_SENSOR0;
    stVpeChnAttr.tIspInitPara.u16Fps = pCameraBootSetting->u8SensorFrameRate;
    stVpeChnAttr.tIspInitPara.u16Flicker = pCameraBootSetting->u8AntiFlicker;
    stVpeChnAttr.tIspInitPara.u32Shutter = pCameraBootSetting->u32shutter;
    stVpeChnAttr.tIspInitPara.u32SensorGainX1024 = pCameraBootSetting->u32SensorGain;
    stVpeChnAttr.tIspInitPara.u32DigitalGain = pCameraBootSetting->u32DigitalGain;
    STCHECKRESULT(MI_VPE_CreateChannel(u32VpeChnId, &stVpeChnAttr));
    STCHECKRESULT(MI_VPE_StartChannel(u32VpeChnId));

	/************************************************
    Step5:  Bind VIF->VPE
    *************************************************/
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
    u32SrcFrmrate = pCameraBootSetting->u8SensorFrameRate;
    u32DstFrmrate = pCameraBootSetting->u8SensorFrameRate;
    eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, eBindType, 0));



    memset(&stVpePortMode, 0x00, sizeof(stVpePortMode));
    stVpePortMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stVpePortMode.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVpePortMode.u16Width  = 640;
    stVpePortMode.u16Height = 320;
    STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, 2, &stVpePortMode));
	STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, 2));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = u32VpeDevId;
    stChnPort.u32ChnId = u32VpeChnId;
    stChnPort.u32PortId = 2;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4));

	{
	    CamOsThreadAttrb_t threadAttr = {.nPriority = 16,.szName = "vpe_func",.nStackSize = 1024};
	    CamOsThread tid;
	    CamOsThreadCreate(&tid, &threadAttr, rtos_vpe_func, &tid);
	    CamOsThreadWakeUp(tid);
	}

    memset(&stVpePortMode, 0x00, sizeof(stVpePortMode));
    stVpePortMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stVpePortMode.ePixelFormat  = _eVpePixelFormat;
    stVpePortMode.u16Width  = _stVpeResolution[0].u32OutWidth;
    stVpePortMode.u16Height = _stVpeResolution[0].u32OutHeight;

    //CamOsPrintf("[%s][%d] u32VpeChnId[%u] u32VpePortId[%u] MI_VPE_SetPortMode\n", __FUNCTION__, __LINE__, u32VpeChnId, _u32VpePortIdInUse);
    STCHECKRESULT(MI_VPE_SetPortMode(u32VpeChnId, _u32VpePortIdInUse, &stVpePortMode));
    STCHECKRESULT(MI_VPE_EnablePort(u32VpeChnId, _u32VpePortIdInUse));

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = u32VpeDevId;
    stChnPort.u32ChnId = u32VpeChnId;
    stChnPort.u32PortId = _u32VpePortIdInUse;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 4));

    if(_u32CropWidth > 0 && _u32CropHeight > 0)
    {
        memset(&stVpeCrop, 0x0, sizeof(MI_SYS_WindowRect_t));
        stVpeCrop.u16X = 0;
        stVpeCrop.u16Y = 0;
        stVpeCrop.u16Width = _u32CropWidth;
        stVpeCrop.u16Height = _u32CropHeight;
        MI_VPE_SetPortCrop(u32VpeChnId, _u32VpePortIdInUse, &stVpeCrop);
        CamOsPrintf("vpe port[%u] crop u16Width = %u, u16Height = %u\n", _u32VpePortIdInUse, stVpeCrop.u16Width, stVpeCrop.u16Height);
    }

    /************************************************
    Step4:  init VENC
    *************************************************/
    u32VencDevId = 0;
    u32VencChnId = 0;
    u32VencPortId = 0;
    memset(&stVencChnAttr, 0x0, sizeof(MI_VENC_ChnAttr_t));
    STUB_GetVencConfig(_aeModType[u32VencChnId], &stVencChnAttr, u32VencChnId);
    //CamOsPrintf("mxh = %d, maxw = %d, pich = %d,picw  =%d, mode = %d\n", stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight, stVencChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth, stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight, stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth, stVencChnAttr.stRcAttr.eRcMode);
    STCHECKRESULT(MI_VENC_CreateChn(u32VencChnId, &stVencChnAttr));
    if(u32VencChnId == 0)
    {
        STCHECKRESULT(MI_VENC_SetMaxStreamCnt(u32VencChnId, 5 * _u8framerate[0]));
    }

	stSuperFrameCfg.u32SuperIFrmBitsThr = _maxFrameSize[0].frameSizeI;
    stSuperFrameCfg.u32SuperPFrmBitsThr = _maxFrameSize[0].frameSizeP;
    STCHECKRESULT(MI_VENC_SetSuperFrameCfg(u32VencChnId, &stSuperFrameCfg));



    /************************************************
    Step6:  Bind VPE->VENC
    *************************************************/
    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId = u32VpeDevId;
    stSrcChnPort.u32ChnId = u32VpeChnId;
    stSrcChnPort.u32PortId = _u32VpePortIdInUse;


    stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    STCHECKRESULT(MI_VENC_GetChnDevid(u32VpeChnId, &u32VencDevId));
    stDstChnPort.u32DevId = u32VencDevId;
    stDstChnPort.u32ChnId = u32VencChnId;
    stDstChnPort.u32PortId = u32VencPortId;
    u32SrcFrmrate = pCameraBootSetting->u8SensorFrameRate;
    u32DstFrmrate = pCameraBootSetting->u8EncFrameRate;
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, _aeBindType[0], _u32RingLineCnt));
    STCHECKRESULT(MI_VENC_StartRecvPic(u32VencChnId));

    /************************************************
    Step7:  Bind VPE->DIVP for YUV420: 240x160@5fps
    *************************************************/
	mi_divp_Init();
    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0;
    stDivpChnAttr.stCropRect.u16Y       = 0;
    stDivpChnAttr.stCropRect.u16Width   = 0;
    stDivpChnAttr.stCropRect.u16Height  = 0;
    stDivpChnAttr.u32MaxWidth           = 256;
    stDivpChnAttr.u32MaxHeight          = 160;
    STCHECKRESULT(MI_DIVP_CreateChn(0, &stDivpChnAttr));
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = 256;
    stOutputPortAttr.u32Height          = 160;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(0, &stOutputPortAttr));
    STCHECKRESULT(MI_DIVP_StartChn(0));

    memset(&stSrcChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDstChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32PortId = 2;

    stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32PortId = 0;
    u32SrcFrmrate = pCameraBootSetting->u8SensorFrameRate;
    u32DstFrmrate = 5;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stDstChnPort, 25, 25));
    STCHECKRESULT(MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate, E_MI_SYS_BIND_TYPE_FRAME_BASE, 0));


/*
    // rgn init
    STCHECKRESULT(MI_RGN_Init(&_gstPaletteTable));

    memset(&stAttachChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stAttachChnPort.eModId = E_MI_RGN_MODID_VPE;
    stAttachChnPort.s32DevId = 0;
    stAttachChnPort.s32ChnId = 0;
    stAttachChnPort.s32OutputPortId = 1;


    // create cover, handle 101
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    STCHECKRESULT(MI_RGN_Create(VPE_PORT1_COVER_HANDLE, &stRgnAttr));
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = tMiBootAttr.u16Regions_x[0];
    stChnPortParam.stPoint.u32Y = tMiBootAttr.u16Regions_y[0];
    stChnPortParam.unPara.stCoverChnPort.u32Layer = 101;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = tMiBootAttr.u16Regions_w[0];
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = tMiBootAttr.u16Regions_h[0];
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 0);
    STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_COVER_HANDLE, &stAttachChnPort, &stChnPortParam));

    // create cover, handle 102
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    STCHECKRESULT(MI_RGN_Create(VPE_PORT1_COVER_1_HANDLE, &stRgnAttr));
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = tMiBootAttr.u16Regions_x[1];
    stChnPortParam.stPoint.u32Y = tMiBootAttr.u16Regions_y[1];
    stChnPortParam.unPara.stCoverChnPort.u32Layer = 102;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = tMiBootAttr.u16Regions_w[1];
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = tMiBootAttr.u16Regions_h[1];
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 0);
    STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_COVER_1_HANDLE, &stAttachChnPort, &stChnPortParam));
*/
    return MI_SUCCESS;
}


MI_S32 Test_vif_vpe_venc(CLI_t *pCli, char *p)
{

	_u32ChnNum = 1;
	_stVifResolution[0].u32OutWidth = 1920;
	_stVifResolution[0].u32OutHeight = 1080;
	_aeModType[0] = 2;
	_aeBindType[0] = 1;
	_u32SensorType = 1;
	_bWriteFile = 1;
    STCHECKRESULT(STUB_BaseModuleInit_vif_vpe_venc());
    return 0;
}

int Rtk_Ai_Autorun_BindingOnly(CLI_t *pCli, char *p)
{
    CamOsPrintf("Func %s\n", __FUNCTION__);
    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_SampleRate_e eOutSampleRate = E_MI_AUDIO_SAMPLE_RATE_16000;
    MI_AI_VqeConfig_t stSetVqeConfig;
    MI_AI_VqeConfig_t stGetVqeConfig;
    MI_S32 s32Idx;
    CamOsThreadAttrb_t threadAttr;
    //set output port buffer depth ???
    MI_SYS_ChnPort_t astAiChnOutputPort0[2];
    MI_U32 i;
    MI_S16 s16CompressionRatioInput[5] = {-70, -60, -30, 0, 0};
    MI_S16 s16CompressionRatioOutput[5] = {-70, -45, -18, 0, 0};


    memset(&threadAttr, 0x0, sizeof(CamOsThreadAttrb_t));
    memset(&stSetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    memset(&stGetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    memset(&stSetVqeConfig, 0, sizeof(MI_AI_VqeConfig_t));
    memset(&stGetVqeConfig, 0, sizeof(MI_AI_VqeConfig_t));

    //UartSendTrace("Rtk_Ai_Autorun_BindingOnly ++\n");

    s32ChanNum = 1;//REF:: array->_astAiChn[2]

    STCHECKRESULT(MI_SYS_Init());
    STCHECKRESULT(MI_AI_Init());

    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 16;
    stSetAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.u32ChnCnt = s32ChanNum;

    /* set ai public attr*/
    STCHECKRESULT(MI_AI_SetPubAttr(AiDevId, &stSetAttr));

    /* get ai device*/
    STCHECKRESULT(MI_AI_GetPubAttr(AiDevId, &stGetAttr));

    /* enable ai device */
    STCHECKRESULT(MI_AI_Enable(AiDevId));

    if(bEnableVqe)
    {
        memset(&stSetVqeConfig, 0, sizeof(MI_AI_VqeConfig_t));
        stSetVqeConfig.bHpfOpen = FALSE;
        stSetVqeConfig.bAnrOpen = FALSE;
        stSetVqeConfig.bAgcOpen = FALSE;
        stSetVqeConfig.bEqOpen = FALSE;

        stSetVqeConfig.s32FrameSample = 128;
        stSetVqeConfig.s32WorkSampleRate = stSetAttr.eSamplerate;

        //Hpf
        stSetVqeConfig.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
        stSetVqeConfig.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_150;

        //Anr
        stSetVqeConfig.stAnrCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_MUSIC;
        stSetVqeConfig.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_MID;
        stSetVqeConfig.stAnrCfg.u32NrIntensity = 15;            //[0,30]
        stSetVqeConfig.stAnrCfg.u32NrSmoothLevel = 10;          //[0, 10]

        //Agc
        stSetVqeConfig.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
        stSetVqeConfig.stAgcCfg.s32NoiseGateDb = -50;           //[-80, 0], NoiseGateDb disable when value = -80
        stSetVqeConfig.stAgcCfg.s32TargetLevelDb =   0;       //[-80, 0]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit = 1;  //[-20, 30]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0, 30]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin = -10; //[-20, 30]
        stSetVqeConfig.stAgcCfg.u32AttackTime = 1;              //[1, 20]
        memcpy(stSetVqeConfig.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
        memcpy(stSetVqeConfig.stAgcCfg.s16Compression_ratio_output, s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));
        stSetVqeConfig.stAgcCfg.u32DropGainMax = 60;            //[0, 60]
        stSetVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb = 0;  //[0, 100]
        stSetVqeConfig.stAgcCfg.u32ReleaseTime = 10;             //[1, 20]
        //Eq
        stSetVqeConfig.stEqCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
        for (i = 0; i < sizeof(stSetVqeConfig.stEqCfg.s16EqGainDb) / sizeof(stSetVqeConfig.stEqCfg.s16EqGainDb[0]); i++)
        {
            if (i <= 60)
            {
                stSetVqeConfig.stEqCfg.s16EqGainDb[i] = 5;
            }
            else
            {
                    stSetVqeConfig.stEqCfg.s16EqGainDb[i] = 0;
            }
        }
    }

    // creat thread to get frame data to save file
    for(s32Idx = 0; s32Idx < s32ChanNum; s32Idx++)
    {
        _astAiChn[s32Idx].AiDevId = AiDevId;
        _astAiChn[s32Idx].AiChn= s32Idx;

        memset(&astAiChnOutputPort0[s32Idx], 0x0, sizeof(MI_SYS_ChnPort_t));
        astAiChnOutputPort0[s32Idx].eModId = E_MI_MODULE_ID_AI;
        astAiChnOutputPort0[s32Idx].u32DevId = _astAiChn[s32Idx].AiDevId;
        astAiChnOutputPort0[s32Idx].u32ChnId = _astAiChn[s32Idx].AiChn;
        astAiChnOutputPort0[s32Idx].u32PortId = 0;
      STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&astAiChnOutputPort0[s32Idx],200,200));


        /* enable ai channel of device*/
        STCHECKRESULT(MI_AI_EnableChn(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn));

        if(bEnableRes)
        {
            STCHECKRESULT(MI_AI_EnableReSmp(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, eOutSampleRate));
        }

        /* if test VQe: set attribute of AO VQE  */
        if(bEnableVqe)
        {
            // need to check algorithm configure
            STCHECKRESULT(MI_AI_SetVqeAttr(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, 0, 0, &stSetVqeConfig));
            STCHECKRESULT(MI_AI_GetVqeAttr(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, &stGetVqeConfig));
            STCHECKRESULT(MI_AI_EnableVqe(_astAiChn[s32Idx].AiDevId,_astAiChn[s32Idx].AiChn));
        }
    }
    return 0;
}
