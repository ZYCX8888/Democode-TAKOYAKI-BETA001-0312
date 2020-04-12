/*
* mid_utils.h- Sigmastar
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

/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_osd.h
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/27
* Description: mixer osd middleware header file
*
*
* History:
*
*    1. Date  :        2018/6/27
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#ifndef _MID_UTILS_H_
#define _MID_UTILS_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "mid_common.h"
#include "mi_rgn.h"
#include "List.h"
#if TARGET_CHIP_I6E
#define MAX_RGN_NUMBER_PER_CHN                       16
#elif TARGET_CHIP_I6B0
#define MAX_RGN_NUMBER_PER_CHN                       16
#else
#define MAX_RGN_NUMBER_PER_CHN                       8
#endif
#define MAX_COVER_NUMBER_PER_CHN                     4
#define MIXER_RGN_OSD_MAX_NUM                        MI_RGN_MAX_HANDLE


#define MI_RGN_COLOR_KEY_VALUE                        0x2323

#define HZ_8P_BIN_SIZE                                65424
#define HZ_12P_BIN_SIZE                               196272
#define HZ_16P_BIN_SIZE                               261696
typedef unsigned short RGBA4444;

#define ARGB1555_RED                                  0x7c00
#define ARGB1555_GREEN                                0x03e0
#define ARGB1555_BLUE                                 0x001f


#ifndef RGB2PIXEL1555
#define RGB2PIXEL1555(a,r,g,b)    (((a & 0x80) << 8) | ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3))
#endif

typedef enum _OsdFontSize_e
{
    FONT_SIZE_8,
    FONT_SIZE_12,
    FONT_SIZE_16,
    FONT_SIZE_24,
    FONT_SIZE_32,
    FONT_SIZE_36,
    FONT_SIZE_40,
    FONT_SIZE_48,
    FONT_SIZE_56,
    FONT_SIZE_60,
    FONT_SIZE_64,
    FONT_SIZE_72,
    FONT_SIZE_80,
    FONT_SIZE_84,
    FONT_SIZE_96
} OsdFontSize_e;

struct MixerOsdTextWidgetOrder_t
{
    Point_t stPointTime;
    Point_t stPointFps;
    Point_t stPointBitRate;
    Point_t stPointResolution;
    Point_t stPointGop;
    Point_t stPointTemp;
    Point_t stPointTotalGain;
    Point_t stPointIspExpo;
    Point_t stPointIspWb;
    Point_t stPointIspExpoInfo;
    Point_t stPointUser;
    Point_t stPointUser1;
    OsdFontSize_e size[MAX_VIDEO_NUMBER+1];
    MI_RGN_PixelFormat_e pmt;
};

typedef struct YuvRunData_s
{
    MI_U32* yuv_y_run_start;
    MI_U32* yuv_y_run_len;
    MI_U32* yuv_y_run_num;

    MI_U32* yuv_uv_run_start;
    MI_U32* yuv_uv_run_len;
    MI_U32* yuv_uv_run_num;

    MI_U32 yuv_size;
} YuvRunData_t;

struct OsdTextWidget_t
{
    MI_S8 * string;
    Point_t point;
    OsdFontSize_e size;
    Color_t fColor;
    Color_t bColor;
    Color_t eColor;
    MI_U32 space;
    BOOL bOutline;
    ImageData_t imageData;
    YuvRunData_t yuvRunData;
};

typedef enum
{
    E_OSD_WIDGET_TYPE_RECT = 0,
    E_OSD_WIDGET_TYPE_TEXT,
    E_OSD_WIDGET_TYPE_COVER,
    E_OSD_WIDGET_TYPE_BITMAP,
    E_OSD_WIDGET_TYPE_MAX
} EN_OSD_WIDGET_TYPE;
typedef enum
{
  SHOW_LEVEL0,
  SHOW_LEVEL1,
  SHOW_LEVEL2,
  SHOW_LEVEL_MAX
}OSD_SHOW_LEVEL;
typedef struct
{
    MI_VENC_CHN           s32VencChn;
    MI_S32                s32Idx;
    MI_U32                u32Color;
    MI_U16                u16LumaThreshold;
    MI_BOOL               bShow;
    MI_BOOL               bOsdColorInverse;
    MI_RGN_PixelFormat_e  eRgnpixelFormat;
    EN_OSD_WIDGET_TYPE    eOsdWidgetType;
    MI_SYS_WindowRect_t   stRect;
    MI_RGN_ChnPort_t      stRgnChnPort;
    pthread_mutex_t       *pstMutexMixerOsdRun;
} ST_MIXER_RGN_WIDGET_ATTR;

typedef struct _TextWidgetAttr_s
{
    const char * string;
    Point_t* pPoint;
    OsdFontSize_e size;
    MI_RGN_PixelFormat_e pmt;
    Color_t* pfColor;
    Color_t* pbColor;
    MI_U8 u32Color;
    MI_U32 space;
    BOOL bHard;
    BOOL bRle;
    BOOL bOutline;
} TextWidgetAttr_t;

typedef struct _RectWidgetAttr_s
{
    MI_SYS_WindowRect_t *pstRect;
    MI_S32 s32RectCnt;
    MI_U8 u8BorderWidth;
    MI_U8 u32Color;
    MI_RGN_PixelFormat_e pmt;
    Color_t* pfColor;
    Color_t* pbColor;
    BOOL bFill;
    BOOL bHard;
    BOOL bOutline;
} RectWidgetAttr_t;


typedef struct monoWidgetAttr_s
{
    void* buf;
    MI_U32 width;
    MI_U32 height;
    Point_t* pPoint;
    Color_t* pfColor;
    Color_t* pbColor;
} monoWidgetAttr_t;

typedef struct gray2WidgetAttr_s
{
    void* buf;
    MI_U32 width;
    MI_U32 height;
    Point_t* pPoint;
    Color_t *pfColor;
    Color_t *pbColor;
    Color_t *peColor;
} gray2WidgetAttr_t;

typedef struct
{
    MI_U16 u16LeftTopX;
    MI_U16 u16LeftTopY;
    MI_U16 u16RightBottomX;
    MI_U16 u16RightBottomY;
}Mixer_Rect_t;


typedef struct
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_CanvasInfo_t stCanvasInfo;
} RGN_Thread_Args_t;

typedef struct MI_Font_s
{
    MI_U32 nFontSize;
    MI_U8* pData;
} MI_Font_t;

typedef enum
{
    HZ_DOT_8,
    HZ_DOT_12,
    HZ_DOT_16,
    HZ_DOT_NUM
} MI_FontDot_e;



typedef  MI_RGN_PaletteTable_t        OsdPalette_t;




#include "mi_sys_datatype.h"
#include "mi_rgn_datatype.h"

typedef struct
{
    MI_U32 u16X;
    MI_U32 u16Y;
}DrawPoint_t;
typedef struct
{
    MI_U32 u16Width;
    MI_U32 u16Height;
}DrawSize_t;

typedef struct
{
    MI_RGN_PixelFormat_e ePixelFmt;
    MI_U32 u32Color;
}DrawRgnColor_t;
typedef struct
{
    MI_RGN_HANDLE  handle;
    MI_VENC_CHN  channel;
    const char *pstr;
    DrawPoint_t  point;
    DrawSize_t   size;
    MI_RGN_PixelFormat_e format;
}st_Osd_Attr;

typedef struct
{
    MI_VENC_CHN  channel;
    MI_U8 oIndex;
    const char *pstr;
}st_Osd_Info;

typedef struct
{
    MI_VENC_CHN  channel;
    MI_U8 oIndex;
    MI_U8 alpha;
}st_osd_alpha;

typedef struct
{
    MI_BOOL enable;
    union
    {
        MI_U16 format;
        MI_U16 scope;
    }m1;

    union
    {
        MI_U16 x;
        MI_U16 ch;
        MI_U16 index;
    }m2;

    union
    {
        MI_U16 y;
        MI_U16 index;
    }m3;
}st_osd_format;

typedef struct
{
    MI_VENC_CHN  channel;
    MI_U8 iItem;
}st_osd_destroyhandle;

typedef struct
{
    MI_VENC_CHN  channel;
    MI_U16 index;
    DrawPoint_t point;
}st_osd_movebych;

typedef struct
{
    MI_VENC_CHN  channel;
    DrawPoint_t point;
    DrawSize_t size;
    MI_U8 colorindex;
}st_cover_attr;
typedef struct
{
  DrawSize_t size;
  DrawPoint_t point;
}osdSizePoint_t;
typedef struct
{
    MI_VENC_CHN  channel;
    MI_U8 index;
}osdChnIndex_t;
typedef struct
{
    MI_RGN_HANDLE  handle;
    DrawPoint_t point;
    DrawSize_t size;
    MI_U8 colorindex;
}st_cover_handle;

typedef struct _I2_To_I8_WidgetAttr_s
{
    Point_t pPoint;
    void* buf;
    MI_U32 buf_length;
    DrawSize_t size;
} I2ToI8WidgetAttr_t;
typedef enum
{
  BY_CHANNEL,
  BY_INDEX,
  BY_TYPE,
  BY_HANDLE,
  BY_MAX
}PRINT_e;
typedef enum
{
  GET_CHANNEL,
  GET_INDEX,
  GET_TYPE,
  GET_HANDLE,
  GET_X_Y,
  GET_FORMAT,
  GET_MAX
}GetOsdATTR_e;
typedef struct
{
  DrawPoint_t point;
  DrawSize_t size;
  MI_U8 osdIndex;
  MI_U8 channel;
}bitmap_t;
typedef struct
{
   DrawPoint_t point;
   DrawSize_t size;
   MI_U8 osdIndex;
   MI_VENC_CHN channel;
}osd_t;
typedef struct
{
   DrawPoint_t point;
   DrawSize_t size;
   MI_S32 osdIndex;
   MI_VENC_CHN channel;
   const char *osdInfo;
}osdRectTxt_t;
typedef struct
{
    EN_OSD_WIDGET_TYPE    osdType;
    OSD_SHOW_LEVEL ShowLevel;
    MI_U8 alpha;
    MI_U8 currentOdNum;
    MI_U8 osdIndex;
    MI_VENC_CHN channel;
    MI_RGN_HANDLE u32RgnHandle;
    MI_RGN_PixelFormat_e format;
    DrawPoint_t point;
    DrawSize_t size;
}osdInfo_t;
typedef struct
{
  osdInfo_t osdInfo;
  struct list_head osd_info_list;
}osd_Link_List_t;
void DrawPoint(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stPt, DrawRgnColor_t stColor);
void DrawLine(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stStartPt, DrawPoint_t stEndPt, MI_U8 u8BorderWidth, DrawRgnColor_t stColor);
void DrawRect(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stLeftTopPt, DrawPoint_t stRightBottomPt, MI_U8 u8BorderWidth, DrawRgnColor_t stColor);
void DrawRect2(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t *pstLeftTopPt, DrawPoint_t *pstRightBottomPt, MI_U8 u8BorderWidth, DrawRgnColor_t stColor);

void FillRect(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stLeftTopPt, DrawPoint_t stRightBottomPt, DrawRgnColor_t stColor);
void DrawCircular(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stCenterPt, MI_U32 u32Radius, MI_U8 u8BorderWidth, DrawRgnColor_t stColor);
void FillCircular(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stCenterPt, MI_U32 u32Radius, DrawRgnColor_t stColor);


MI_U32 mid_Font_Strlen(const MI_S8 *phzstr);
MI_U32 mid_Font_Strcmp(MI_S8* pStr1, MI_S8* pStr2);
MI_U32 mid_Font_GetSizeByType(OsdFontSize_e eFontSize);
MI_U32 mid_Font_GetCharQW(const MI_S8 *str, MI_U32 *p_qu, MI_U32 *p_wei);
void mid_YuvDilate(MI_U8* pYbuffer, MI_U16* pUvbuffer, MI_U32 stride, MI_U32 width, MI_U32 height, YUVColor_t* pYuvColor);

MI_Font_t* mid_Font_GetHandle(OsdFontSize_e eFontSize, MI_U32* pMultiple);

MI_S32 mid_Font_DrawText(ImageData_t* pImage, const MI_S8 *pStr, MI_U32 idx_start, OsdFontSize_e eFontSize,
                              MI_U32 nSpace, Color_t* pfColor, Color_t* pbColor , BOOL bOutline);

//void mid_ImageGenRLE(ImageData_t* pImageData, ImageRunData_t* pRunData);
//void mid_BmpDilate(MI_U16* pbuffer , MI_U32 width , MI_U32 height , MI_U32 stride , MI_U16 rgbColor);
MI_S32 mid_Font_DrawYUV420(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pYuv,
                                        MI_U32 nIndex, YUVColor_t* pfColor, YUVColor_t* pbColor , BOOL bOutline);
MI_S32 mid_Font_DrawRGB4444(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                          MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline);
MI_S32 mid_Font_DrawRGB1555(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                          MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline);

//MI_S32 MI_Bmp2RGB4444(RGBA4444 *dstData, U8* srcData , U32 srcWidth , U32 srcHeight , PixelFormat_e fmt);
MI_S32 MI_BmpReadFromMem(MI_U8 *buffer , ImageData_t* pBmp);
void Argb2Yuv(Color_t* pRgbColor , YUVColor_t* pYuvColor);
MI_U32 mid_GetPixelBitCount(MI_RGN_PixelFormat_e pixelConfig);

size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);


void preInitOsd(void);
int  initOsdModule(char *param, int paramLen);
int  uninitOsdModule();
int initOsdIeModule(char *param, int paramLen);
int uninitOsdIeModule();
MI_S32 initOsdMask(MI_VENC_CHN VeChn, MI_S32 s32Idx);
MI_S32 uninitOsdMask(MI_VENC_CHN VeChn, MI_S32 s32Idx);

void initOsdVideoInfo();
void uninitOsdVideoInfo();

MI_S32 enableOsdColorInverse();
MI_S32 disableOsdColorInverse();
MI_S32 initOsdFull();
MI_S32 uninitOsdFull();
MI_S32 resetOsdResolution(MI_VENC_CHN VeChn, MI_SYS_WindowSize_t *pstSize);
MI_S32 restartOsdResolution(MI_VENC_CHN VeChn, MI_SYS_WindowSize_t *pstSize);


MI_S32 openOsdModule(char *param, int paramLen);
void setOsdPrivateMask(char *param, int paramLen);

MI_S32 createOsdRectWidget(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRgnAttr, MI_RGN_ChnPort_t *pstChnPort, MI_RGN_ChnPortParam_t *pstChnPortParam);
MI_S32 updateOsdRectWidget(MI_RGN_HANDLE hHandle, RectWidgetAttr_t *pstRectWidgetAttr, BOOL visible);
MI_S32 createOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRgnAttr, MI_RGN_ChnPort_t *pstChnPort, MI_RGN_ChnPortParam_t *pstChnPortParam);
MI_S32 updateOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, TextWidgetAttr_t* pstTextWidgetAttr, BOOL visible);

MI_S32 cleanOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, MI_SYS_WindowRect_t *pstRect, MI_U32 index);
MI_S32 configRgnWidgetAttr(MI_VENC_CHN s32VencChn,
                                   MI_S32 s32Idx,
                                   MI_SYS_WindowRect_t *pstRect,
                                   MI_RGN_ChnPort_t *pstRgnChnPort,
                                   ST_MIXER_RGN_WIDGET_ATTR *pstMixerRgnWidgetAttr,
                                   MI_RGN_PixelFormat_e eRgnFormat);
MI_RGN_HANDLE calcOsdHandle(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType);
MI_RGN_HANDLE getOsdHandle(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType);
MI_RGN_HANDLE * getOsdHandleAddr(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType);
MI_RGN_HANDLE getOsdChnHandleFirstNull(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx);
MI_S32 createOsdWidget(MI_RGN_HANDLE *pu32RgnHandle, ST_MIXER_RGN_WIDGET_ATTR *pstMixerRgnWidgetAttr);
MI_S32 destroyOsdWidget(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE u32OsdHandle);
MI_S32 configOsdRgnChnPort(MI_VENC_CHN s32VencChn, MI_RGN_ChnPort_t *pstRgnChnPort);
MI_S32 changeOsdAlpha(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_U8 u8Alpha);

MI_S32 changeOsdPositionByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect);
MI_S32 changeOsdPositionAndSize(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_SYS_WindowRect_t *pstRect);
MI_S32 changeOsdPositionAndSizeByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect);

MI_S32 changeCoverPositionByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color);
MI_S32 changeCoverPositionAndSize(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color);
MI_S32 changeCoverPositionAndSizeByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color);

MI_S32 getOsdChnHandleCount(MI_VENC_CHN s32VencChn);
MI_RGN_HANDLE getOsdChnMaxHandle(MI_VENC_CHN s32VencChn);
MI_RGN_HANDLE getOsdChnHandleNumber(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx,EN_OSD_WIDGET_TYPE osdType);

MI_S32 getCoverHandleCount(MI_VENC_CHN s32VencChn);
MI_RGN_HANDLE getCoverMaxHandleNumber();
MI_RGN_HANDLE getCoverChnHandleNumber(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx);
MI_RGN_HANDLE getCoverChnHandleFirstNull(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx);

MI_S32 updateOsdInfomation(MI_RGN_HANDLE u32OsdHandle, const MI_S8 *ps8String,osdSizePoint_t *osdSizePoint);
MI_S32 testOsdFormatBitmap(MI_S32      format,MI_U32 *idex,MI_U32 startX, MI_U32 startY);
MI_S32 destoryOsdHandle(MI_RGN_HANDLE OsdHandle, MI_U32 osdChn,MI_U32 idex,EN_OSD_WIDGET_TYPE eOsdHandleType);
void getOsdInfoLinkList(struct list_head *osdInfo);
void midOsdInfoInit(void);
void midOsdInfoUpdate(osdInfo_t *osdInfo);
void midOsdInfoDel(osdInfo_t *osdInfo);
void midFindOsdInfoPrint(osd_Link_List_t *osdInfo,PRINT_e way);
void midManageOsdInfoLinkList(osdInfo_t *osdInfo,BOOL State,osdSizePoint_t *osdSizePoint,const char *pstring);
MI_U8 GetOdUseVdfChannelValue(void);
void SetOdUseVdfChannelValue(MI_U8 value);

MI_RGN_HANDLE midFindOsdHandleByOsdAttr(osdInfo_t *osdInfo);
#if TARGET_CHIP_I6B0
MI_BOOL checkVencInitOsdFull(MI_S32 vencChn1,MI_S32 *vencChn2);
#endif

#ifdef __cplusplus
}
#endif

#endif //_MID_UTILS_H_
