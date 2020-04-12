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
#ifndef __MI_ACODEC_H__
#define __MI_ACODEC_H__
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "mi_audio_type.h"

/**
 *  创建音频编码通道
 *  AeChn:              音频编码通道
 *  pstAttr:            音频设备属性
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AENC_CreateChn(AENC_CHN AeChn, AencChnAttr_t *pstAttr);

/**
 *  销毁音频编码通道
 *  AeChn:              音频编码通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AENC_DestroyChn(AENC_CHN AeChn);

/**
 *  开始获取音频编码数据流
 *  AeChn:              音频编码通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AENC_StartStream(AENC_CHN AeChn);

/**
 *  获取音频编码数据流
 *  AeChn:              音频编码通道
 *  pstStream :         音频数据流
 *  sMilliSec :         >0 等待sMillisec
 *            :         -1 默认等待INFINITE_WAIT
 *            :         0  不等待
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AENC_GetStream(AENC_CHN AeChn, AudioStream_t *pstStream, S32 sMilliSec);

/**
 *  释放音频编码数据流
 *  AeChn:              音频编码通道
 *  pstStream :         音频数据流
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AENC_ReleaseStream(AENC_CHN AeChn, AudioStream_t *pstStream);

/**
 *  获取音频编码通道号对应的设备文件句柄
 *  AeChn:              音频编码通道
 *  return:             FD句柄
 */
S32 MI_AENC_GetFd(AENC_CHN AeChn);

/**
 *  创建音频解码通道
 *  AdChn:              音频解码通道
 *  pstAttr:            音频设备属性
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_ADEC_CreateChn(ADEC_CHN AdChn, AdecChnAttr_t *pstAttr);

/**
 *  销毁音频解码通道
 *  AdChn:              音频解码通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_ADEC_DestroyChn(ADEC_CHN AdChn);

/**
 *  发送音频解码数据帧
 *  AdChn:              音频解码通道
 *  pstStream:          音频解码数据流
 *  sBlock:             -1,阻塞;　0　非阻塞
 *  pAdecCB             非阻塞回调函数
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_ADEC_SendStream(ADEC_CHN AdChn, AudioStream_t *pstStream, S32 sBlock, AdecCallback_t *pAdecCB);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif //__MI_ACODEC_H__

