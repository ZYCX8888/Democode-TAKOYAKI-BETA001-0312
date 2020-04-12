/*
* mid_video_type.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __MI_VIDEO_TYPE_H__
#define __MI_VIDEO_TYPE_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include "mid_common.h"


typedef MI_S32 VI_CHN;
typedef MI_S32 VENC_CHN;

#define SUPERBFRMBITSTHR          2
#define SUPERIFRMBITMOL           10
#define SUPERPFRMBITMOL           5
#define SUPERIPFRMBITDEN          10

#define RESOLUTION_5M             4500000
#define IQSERVER_5M_SENSOR_FPS    13
#define RESOLUTION_4M             3800000
#define IQSERVER_4M_SENSOR_FPS    20

typedef enum _PixelFormat_e
{
    COLOR_FormatUnused,
    COLOR_FormatYCbYCr,                 //yuv422
    COLOR_FormatSstarSensor16bitRaw,    //
    COLOR_FormatSstarSensor16bitYC,
    COLOR_FormatSstarSensor16bitSTS,    //do not support
    COLOR_FormatYUV420SemiPlanar,
    COLOR_FormatYUV420Planar,           //yuv420
    COLOR_Format16bitBGR565,
    COLOR_Format16bitARGB4444,
    COLOR_Format16bitARGB1555,
    COLOR_Format24bitRGB888,
    COLOR_Format32bitABGR8888,
    COLOR_FormatL8,
    COLOR_FormatMONO,
    COLOR_FormatGRAY2,
    COLOR_FormatMax,
} PixelFormat_e;


typedef struct _VideoFrame_t
{
    MI_U8           *bufAddr;
    MI_U32          bufLen;
    MI_U64          pts;
    MI_U32          data1;
    MI_U32          data2;
    MI_S32          reserve[1];
} VideoFrame_t;

typedef struct _VideoFrameInfo_t
{
    VideoFrame_t vFrame;
    MI_S32       reserve[1];
} VideoFrameInfo_t;

typedef struct _VideoFrameExtInfo_t
{
    MI_U32         yOffset;
    MI_U32         uvOffset;
    MI_U32         width;
    MI_U32         height;
    MI_U32         pitch;
    PixelFormat_e  pixFormat;
    MI_U32         seqNum;          //not support now
} VideoFrameExtInfo_t;

typedef struct _VideoFrameInfoEx_t
{
    VideoFrameInfo_t      basicInfo;
    VideoFrameExtInfo_t   extInfo;
    MI_S32                reserve[1];
    MI_S32                version;
} VideoFrameInfoEx_t;

typedef struct ImageData_s
{
    MI_RGN_PixelFormat_e pmt;
    MI_U16 width;
    MI_U16 height;
    MI_U8* buffer;
} ImageData_t;

typedef struct _MixerVifInfo
{
    MI_SYS_ChnPort_t stVifChnPort;
    MI_U8  HdrType;
    MI_U8  level3DNR;
    MI_U8  vifframeRate;
    MI_U16 SensorWidth;
    MI_U16 SensorHeight;
    MI_U32  sensorFrameRate;

}MixerVifInfo;

typedef struct _MixerVideoParam
{
    MixerVifInfo  stVifInfo;
    MI_SYS_ChnPort_t stVpeChnPort;
    MI_U16 width;
    MI_U16 height;
    MI_U16 MaxWidth;
    MI_U16 MaxHeight;
    MI_U32 bitrate;
    MI_U32 gop;

    MI_U8  encoderType;

    MI_U8 FromPort;
    MI_U8  vpeframeRate;
    MI_U8  vpeBufUsrDepth;
    MI_U8  vpeBufCntQuota;

    MI_U32  vencframeRate;
    MI_U8  vencBufUsrDepth;
    MI_U8  vencBufCntQuota;

    MI_U8  divpBufUsrDepth;
    MI_U8  divpBufCntQuota;

    MI_U8  maxQp;
    MI_U8  minQp;
    MI_U8  maxIQp;
    MI_U8  minIQp;
    MI_U8  maxPQp;
    MI_U8  minPQp;

    MI_U8  u8MaxQfactor;
    MI_U8  u8MinQfactor;

    MI_S8  IPQPDelta;
    MI_S8  s8Qfactor;
    MI_S8  s8DivpEnable;
    MI_S8  pipCfg;
    MI_U16 pipRectX;
    MI_U16 pipRectY;
    MI_U16 pipRectW;
    MI_U16 pipRectH;

    MI_U8  virtualIEnable;
    MI_U8  u8RateCtlType;
    MI_U8  u8ChangePos;
    MI_U16 u16VpeOutWidth;
    MI_U16 u16VpeOutHeight;
    MI_S32 virtualIInterval;

    ViChnStatus viChnStatus;
#if MIXER_SED_ENABLE
    MI_U8 u8SedEnable;
#endif
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    Mixer_Venc_Bind_Mode_E eBindMode;
    MI_SYS_PixelFormat_e ePixelFormat;
#endif
} MixerVideoParam;

#ifdef __cplusplus
}
#endif

#endif /*__MI_VIDEO_TYPE_H__*/
