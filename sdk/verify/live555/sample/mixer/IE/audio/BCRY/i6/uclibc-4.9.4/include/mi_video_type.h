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
#ifndef __MI_VIDEO_TYPE_H__
#define __MI_VIDEO_TYPE_H__


#ifdef __cplusplus
extern "C"
{
#endif

#include "mi_common.h"


typedef S32 VI_CHN;
typedef S32 VENC_CHN;


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

    COLOR_FormatMax,
} PixelFormat_e;


typedef struct _VideoFrame_t
{
    U8           *bufAddr;
    U32          bufLen;
    U64          pts;
    U32          data1;
    U32          data2;
    U8         *phys;
} VideoFrame_t;

typedef struct _VideoFrameInfo_t
{
    VideoFrame_t vFrame;
    S32          reserve[1];
} VideoFrameInfo_t;

typedef struct _VideoFrameExtInfo_t
{
	U32            yOffset;
	U32            uvOffset;
	U32            width;
	U32            height;
	U32            pitch;
	PixelFormat_e  pixFormat;
	U32            seqNum;          //not support now
} VideoFrameExtInfo_t;

typedef struct _VideoFrameInfoEx_t
{
    VideoFrameInfo_t      basicInfo;
	VideoFrameExtInfo_t   extInfo;
    S32                   reserve[1];
	S32                   version;
} VideoFrameInfoEx_t;

typedef struct ImageData_s
{
    PixelFormat_e pmt;
    U16 width;
    U16 height;
    U8* buffer;
} ImageData_t;

#ifdef __cplusplus
}
#endif

#endif /*__MI_VIDEO_TYPE_H__*/
