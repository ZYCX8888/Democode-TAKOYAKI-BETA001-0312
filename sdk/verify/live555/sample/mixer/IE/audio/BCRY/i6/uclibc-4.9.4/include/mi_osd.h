/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef __MI_OSD_H__
#define __MI_OSD_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "mi_common.h"
#include "mi_video_type.h"

/*
 * user widget attr
 */
typedef void* UserData_t;       //用户自定义图形的数据

/**
 * 用户需要画自定义图形的callback函数类型
 * pUserData: 用户自定义的数据
 * dstYBuf::  提供给用户操作的Y buffer指针
 * dstUVBuf:  提供给用户操作的UV buffer指针
 * dstWidth:  画布的像素宽度
 * dstHeight: 画布的像素高度
 */
typedef void (*UserOsdCallback_t)(UserData_t pUserData, U8* dstYBuf, U8 *dstUVBuf, U32 dstWidth, U32 dstHeight);

/**
* font pixel type
*/
typedef enum _OsdFontSize_e
{
    FONT_SIZE_32,
    FONT_SIZE_64,
    FONT_SIZE_24,
    FONT_SIZE_16
} OsdFontSize_e;




typedef enum
{
    FB_PIXEL_ALPHA = 0,
    FB_CONST_ALPHA = 1,
} FbAlphaType_e;

/**
 * Alpha config
 */
typedef struct _FbAlphaConfig_t
{
    U8 enable;
    FbAlphaType_e alphaType;
    U8 alpha;
} FbAlphaConfig_t;

typedef struct
{
    /// RGBA888
    U8 u8B;
    U8 u8G;
    U8 u8R;
    U8 u8A;
} OsdPaletteEntry_t;

typedef struct _OsdPalette_t
{
    OsdPaletteEntry_t entry[256];
} OsdPalette_t;


typedef struct OsdInvertColor_s
{
    U32  nLumThresh;                //The threshold to decide whether invert the OSD's color or not.
    BOOL bInvColEn;                  //The switch of inverting color.
} OsdInvertColor_t;

typedef struct yuvWidgetAttr_s
{
    void* buf;
    U32 bufSize;
    Point_t* pPoint;
    U32 width;
    U32 height;
    BOOL bRle;
    BOOL bOutline;
} yuvWidgetAttr_t;



typedef struct textWidgetAttr_s
{
    char * string;
    Point_t* pPoint;
    OsdFontSize_e size;
    Color_t* pfColor;
    Color_t* pbColor;
    U32 space;
    BOOL bHard;
    BOOL bRle;
    BOOL bOutline ;
} textWidgetAttr_t;
typedef struct userWidgetAttr_s
{
    UserData_t pUserData;   //用户自定义的数据
    U64 userDataSize;       //用户自定义数据的大小
    UserOsdCallback_t callback; //用户画图回调函数
} userWidgetAttr_t;

/**
 * 贴yuv字符串到指定vi chn
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * string:  显示的字符串
 * point: 字符串显示的起始点
 * size:    字符串的大小
 * fColor:   字符串的颜色
 * bColor:   字符串的背景颜色
 * space:   字符间的像素间隔
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateTextWidget(MI_HANDLE *pHandle, VI_CHN viChn, const char * string, Point_t* pPoint, OsdFontSize_e size, Color_t* pfColor, Color_t* pbColor, U32 space , BOOL bHard);


/**
 * 贴yuv图像到指定vi chn
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * buf:     YUV图像buffer
 * bufSize: YUV图像buffer的大小
 * point: 字符串显示的起始点
 * width:   YUV图像的宽度
 * height:  YUV图像的高度
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateYuvWidget(MI_HANDLE *pHandle, VI_CHN viChn, void* buf, U32 bufSize, Point_t* pPoint, U32 width, U32 height);

/**
 * 贴yuv图像到指定vi chn
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * pAttr:  用于设置yuv widget的属性结构体
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateYuvWidgetEx(MI_HANDLE *pHandle, VI_CHN viChn, yuvWidgetAttr_t* pAttr);

/**
 * 向指定的vi chn贴矩形
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * rect: 矩形的大小
 * color:   矩形框的颜色
 * bFill:   矩形内部是否填充
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateRectWidget(MI_HANDLE *pHandle, VI_CHN viChn, Rect_t* pRect, Color_t* pColor, BOOL bFill);

/**
 * 向指定的vi chn贴矩形
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * rect: 矩形的大小
 * color:   矩形框的颜色
 * bFill:   矩形内部是否填充
 * bHard:   一般使用RECT_MODE_AUTO，如果需要没有隐私遮挡的用于视频分析的通道，请不要创建-1通道的矩形，同时指定mode为hard or soft。
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateRectWidgetEx(MI_HANDLE *pHandle, VI_CHN viChn, Rect_t* pRect, Color_t* pColor, BOOL bFill , BOOL bHard);


/**
 * 设定需要显示的图片buffer，图片的格式rgb888 argb8888 argb1555 argb4444
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * buf:  图片文件路径
 * point:  图片显示的起始点
 * pRoi:  显示图片的区域
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_OSD_CreateBitmapWidgetFromFile(MI_HANDLE *pHandle, VI_CHN viChn, const char* path, Point_t* pPoint, Rect_t *pRoi);

/**
 * 设定需要显示的图片buffer，图片的格式rgb888 argb8888 argb1555 argb4444
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * imageData:  图片数据
 * point:  图片显示的起始点
 * pRoi:  显示图片的区域
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_OSD_CreateBitmapWidget(MI_HANDLE *pHandle, VI_CHN viChn, ImageData_t* imageData, Point_t* pPoint, Rect_t *pRoi);

/**
 * 画自定义图案到指定vi chn
 * handle:  用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * pAttr：  用于设置user widget的属性结构体
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_CreateUserWidget(MI_HANDLE *pHandle, VI_CHN viChn, userWidgetAttr_t* pAttr);

/**
 * 更新字符串控件的属性
 * handle: 控件的句柄
 * viChn:   目标vi chn
 * string:  显示的字符串
 * point: 字符串显示的起始点
 * size:    字符串的大小
 * fColor:   字符串的颜色
 * bColor:   字符串的背景颜色
 * space:   字符间的像素间隔
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
*/
MI_RET MI_OSD_UpdateTextWidget(MI_HANDLE handle, S8 * string, Point_t* pPoint, OsdFontSize_e size, Color_t* pfColor, Color_t* pbColor, U32 space);


/**
 * 更新YUV控件的属性
 * handle: 控件的句柄
 * viChn:   目标vi chn
 * buf:     YUV图像buffer
 * bufSize: YUV图像buffer的大小
 * point: 字符串显示的起始点
 * width:   YUV图像的宽度
 * height:  YUV图像的高度
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_UpdateYuvWidget(MI_HANDLE handle, void* buf, U32 bufSize, Point_t* pPoint, U32 width, U32 height);

/**
 * 向指定的vi chn贴矩形
 * handle: 控件的句柄
 * viChn:   目标vi chn
 * rect: 矩形的大小
 * color:   矩形框的颜色
 * bFill:   矩形内部是否填充
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_UpdateRectWidget(MI_HANDLE handle, Rect_t* pRect, Color_t* pColor, BOOL bFill);

MI_RET MI_OSD_UpdateBitmapWidgetFromFile(MI_HANDLE handle, const char* path, Point_t* pPoint, Rect_t *pRoi);

/**
 * 更新需要显示的图片buffer，图片的格式rgb888 argb8888 argb1555 argb4444
 * handle: 用于获取返回的控件句柄指针
 * viChn:   目标vi chn
 * imageData:  图片数据
 * point:  图片显示的起始点
 * pRoi:  显示图片的区域
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_UpdateBitmapWidget(MI_HANDLE handle, ImageData_t* pImageData, Point_t* pPoint, Rect_t *pRoi);

/**
 * 更新自定义图案到指定vi chn
 * handle:  用于获取返回的控件句柄指针
 * pAttr：  用于设置user widget的属性结构体
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_UpdateUserWidget(MI_HANDLE handle, userWidgetAttr_t* pAttr);
/***********************************OSD API define  *****************************************/

/**
 * 销毁控件
 * handle: 控件句柄指针
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_DestroyWidget(MI_HANDLE handle);

/**
 * 更新控件的显示属性
 * handle: 控件的句柄
 * viChn:   目标vi chn
 * visible:  控件是否可见
 * return: MI_RET_SUCCESS成功，MI_RET_FAIL失败
*/
MI_RET MI_OSD_SetWidgetVisible(MI_HANDLE handle, BOOL visible);


/**
 * 设定osd的alpha值
 * alphaConfig: alpha设定
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_SetAlphaConfig(VI_CHN viChn, FbAlphaConfig_t* alphaConfig);

/**
 * 获取osd的alpha值
 * viChn:   目标vi chn
 * alphaConfig: alpha设定
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_GetAlphaConfig(VI_CHN viChn, FbAlphaConfig_t *alphaConfig);


/**
 * 设定osd的color key值
 * viChn:   目标vi chn
 * index:     color key在调色板中的索引值
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_SetColorKeyConfig(VI_CHN viChn, U8 index);

/**
 * 获取osd的color key值
 * viChn:   目标vi chn
 * index:     color key在调色板中的索引值
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_GetColorKeyConfig(VI_CHN viChn, U8 *index);

/**
 * 设定osd调色板
 * viChn:   目标vi chn
 * palette:     调色板
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_SetPalette(VI_CHN viChn, OsdPalette_t* palette);

/**
 * 获取osd的调色板
 * viChn:   目标vi chn
 * palette:     调色板的地址
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_GetPalette(VI_CHN viChn, OsdPalette_t *palette);

/**
 * 获取osd的反色设置? * viChn:   目标vi chn
 * palette:     反色设置
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_SetInvertColor(VI_CHN viChn, OsdInvertColor_t *invertColor);


/**
 * 设定osd的反色设置
 * viChn:   目标vi chn
 * palette:     反色设置
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_OSD_GetInvertColor(VI_CHN viChn, OsdInvertColor_t *invertColor);

/**
 * 设置通道OSD 画板的最大值，如果不设置，默认整个画面都可以? * 比如如果只需要在画面的上方区域画OSD,nHeight就可以设置小一些
 * viChn:   目标vi chn
 * nWidth:     宽度边界
 * nHeight:     高度边界
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_OSD_SetCanvasSize(VI_CHN ViChn , U32 nWidth , U32 Height);


/**
 * 使得ViChn 通道的Widget 显示到画面上
 * viChn:   目标vi chn
 * palette:     调色板的地址
 * return:  MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_OSD_UpdateCanvas(VI_CHN ViChn);


#ifdef __cplusplus
}
#endif


#endif //__MI_OSD_H__

