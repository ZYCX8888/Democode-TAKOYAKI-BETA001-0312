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
#ifndef __MI_VI_H__
#define __MI_VI_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "mi_vi_type.h"

/**
 *  设置 VI 设备属性
 *  pstAttr:        VI 设备属性指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_SetDevAttr(const ViDevAtrr_t *pstAttr);

/**
 *  获取 VI 设备属性
 *  pstAttr:        VI 设备属性指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetDevAttr(ViDevAtrr_t *pstAttr);

/**
 *  设置 VI 通道属性
 *  ViChn:          VI 通道号
 *  pstAttr:        VI 通道属性指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_SetChnAttr(VI_CHN ViChn, const ViChnAttr_t *pstAttr);

/**
 *  获取 VI 通道属性
 *  ViChn:          VI 通道号
 *  pstAttr:        VI 通道属性指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetChnAttr(VI_CHN ViChn, ViChnAttr_t *pstAttr);


/**
 *  启用 VI 通道
 *  ViChn:          VI 通道号
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_EnableChn(VI_CHN ViChn);

/**
 *  禁用 VI 通道
 *  ViChn:          VI 通道号
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_DisableChn(VI_CHN ViChn);

/**
 *  获取 VI 采集的图像
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetFrame(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo);

/**
 *  获取 VI 采集的图像
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  u32MilliSec:    超时时间
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetFrameTimeOut(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo, U32 u32MilliSec);

/**
 *  释放 VI 图像数据所占的缓存
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_ReleaseFrame(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo);

/**
 *  获取 VI 采集的图像
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetFrameEx(VI_CHN ViChn, VideoFrameInfoEx_t *pstFrameInfo);

/**
 *  获取 VI 采集的图像
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  u32MilliSec:    超时时间
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetFrameTimeOutEx(VI_CHN ViChn, VideoFrameInfoEx_t *pstFrameInfo, U32 u32MilliSec);

/**
 *  释放 VI 图像数据所占的缓存
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_ReleaseFrameEx(VI_CHN ViChn, VideoFrameInfoEx_t *pstFrameInfo);

/**
 *  释放多个VI 图像数据所占的缓存，如果MI缓冲中还有yuv数据，也会释放掉，只留下最新的restFrameNum帧数据
 *  ViChn:          VI 通道号
 *  pstFrameInfo:   VI 帧信息结构指针
 *  restFrameNum:   需要在缓冲中留多少帧
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_ReleaseMutiFrame(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo, U32 restFrameNum);

/**
 *
 */
MI_RET MI_VI_SetSensorFrameRate(F32 frameRate);
MI_RET MI_VI_GetSensorFrameRate(F32 *frameRate);

/*
* 触发vi 1/2的取流。
*  ViChn:		   VI 通道号
*  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失?*/
MI_RET MI_VI_FlashTrigger(VI_CHN ViChn);

/**
 *  设置vi图像最大深度
 *  ViChn:          VI 通道号
 *  depth:          深度。为0时无效，会使用默认值
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_SetFrameDepth(VI_CHN ViChn, U32 depth);

/**
 *  获取vi图像最大深度
 *  ViChn:          VI 通道号
 *  depth:          深度。
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_GetFrameDepth(VI_CHN ViChn, U32 *pDepth);

MI_RET MI_VI_SetRotate(S32 Rotation);

/*输入裁剪区域*/
MI_RET MI_ISP_SetInputCrop(U32 nChannel, Rect_t CropRect);
MI_RET MI_ISP_GetInputCrop(U32 nChannel, Rect_t *pCropRect);
/*数字缩放*/
MI_RET MI_ISP_SetDigitalZoom(U32 nChannel, S32 Width, S32 Height);
MI_RET MI_ISP_GetDigitalZoom(U32 nChannel, S32 *pWidth, S32 *pHeight);
/*排除区域*/
MI_RET MI_ISP_SetExclusionRect(U32 nChannel, Rect_t ExclusionRect);
MI_RET MI_ISP_GetExclusionRect(U32 nChannel, Rect_t *pExclusionRect);

/**
 * 设置crop通道区域。
 *  ViChn:          VI 通道号
 *  pRect:          区域
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VI_SetChnCrop(VI_CHN ViChn, Rect_t *pRect);
MI_RET MI_VI_GetFrameTimeOutForSandBox(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo, S32 milliSec);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MI_VI_H__ */



