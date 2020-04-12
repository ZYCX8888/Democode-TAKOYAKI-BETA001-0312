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
#ifndef __MI_AIO_H__
#define __MI_AIO_H__
#ifdef __cplusplus
extern "C"
{
#endif

#include "mi_common.h"
#include "mi_audio_type.h"

/*this is used just for single audio case,no video,no osd,no isp etc..*/
MI_RET MI_Audio_Init();
MI_RET MI_Audio_Exit();
/*end*/
/**
 *  设置AI设备参数
 *  AudioDevId:         音频设备编号
 *  pstAttr:                音频设备属性
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_SetDevAttr(AUDIO_DEV AudioDevId, const AioDevAttr_t *pstAttr);

/**
 *  获取AI设备参数
 *  AudioDevId:         音频设备编号
 *  pstAttr:                音频设备属性
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetDevAttr(AUDIO_DEV AudioDevId, AioDevAttr_t *pstAttr);


/**
 *  设置AI通道参数
 *  AiChn:                 音频通道
 *  pstChnParam        音频通道属性
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_AI_SetChnAttr(AIO_CHN AiChn, const AioChnAttr_t* pstChnParam);

/**
 * 获取通道参数
 *  AiChn:            音频通道号
 *  pstChnParam   音频通道属性
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetChnAttr(AIO_CHN AiChn, AioChnAttr_t* pstChnParam);


/**
 *  启用AI设备
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_Enable(AUDIO_DEV AudioDevId);

/**
 *  禁用AI设备
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_Disable(AUDIO_DEV AudioDevId);

/**
 *  启用AI通道
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableChn(AUDIO_DEV AudioDevId, AIO_CHN AiChn);

/**
 *  禁用AI通道
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableChn(AUDIO_DEV AudioDevId, AIO_CHN AiChn);

/**
 *  获取音频编码通道号对应的设备文件句柄
 *  AiChn:              音频编码通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetFd(AIO_CHN AiChn);

/**
 *  设置AI设备声音
 *  AudioDevId:         音频设备编号
 *  sVolume:            音量
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_SetVolume(AUDIO_DEV AudioDevId, S32 sVolume);

/**
 *  获取AI设备声音
 *  AudioDevId:         音频设备编号
 *  sVolume:            音量
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetVolume(AUDIO_DEV AudioDevId, S32 *sVolume);

/**
*  启用AI重采样
*  AudioDevId:         音频设备编号
*  AiChn:              音频通道号
*  enOutSampleRate:    采样率
*  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
*/

MI_RET MI_AI_EnableReSmp(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioSampleRate_e enOutSampleRate);

/**
 *  设置AI静音状态
 *  AudioDevId:         音频设备编号
 *  ptMute:             禁用结构体
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_SetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);

/**
 *  获取AI静音状态
 *  AudioDevId:         音频设备编号
 *  ptMute:             禁用结构体
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);

/**
 *  禁用AI重采样
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableReSmp(AUDIO_DEV AudioDevId, AIO_CHN AiChn);


/**
 *  开启音频码流
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_AI_StartFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn);


/**
 *  获取AI数据帧
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  pstFrm:             音频数据帧
 *  sMilliSec :         >0 等待sMillisec
 *            :         -1 默认等待INFINITE_WAIT
 *            :         0  不等待
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioFrame_t *pstFrm, S32 sMilliSec);

/**
 *  释放AI数据帧
 *  AudioDevId:         音频设备编号
 *  AiChn:              音频通道号
 *  pstFrm:             音频数据帧
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_ReleaseFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioFrame_t *pstFrm);

/**
 *  启用消回声
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableAEC(AUDIO_DEV AudioDevId);

/**
 *  禁用消回声
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableAEC(AUDIO_DEV AudioDevId);

/**
 *  启用噪声消除
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableANR(AUDIO_DEV AudioDevId);

/**
 *  禁用噪声消除
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableANR(AUDIO_DEV AudioDevId);

/**
 *  启用自动增益
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableAGC(AUDIO_DEV AudioDevId);

/**
 *  禁用自动增益
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableAGC(AUDIO_DEV AudioDevId);

/**
 *  启用降风噪
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableWNR(AUDIO_DEV AudioDevId);

/**
 *  禁用降风噪
 *  AudioDevId:         音频设备编号
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableWNR(AUDIO_DEV AudioDevId);

/**
 *  启用均衡
 *  AudioDevId:         音频设备编号
 *  return:                MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_EnableEQ(AUDIO_DEV AudioDevId);

/**
 *  禁用均衡
 *  AudioDevId:         音频设备编号
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_DisableEQ(AUDIO_DEV AudioDevId);

/**
 *  设置AI声音增强属性
 *  AudioDevId:         音频设备编号
 *  AI_VQE_CONFIG:  声音增强结构体
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_SetVqeAttr(AUDIO_DEV AudioDevId, AI_VQE_CONFIG *pVqeConfig);

/**
 *  获取AI声音增强属性
 *  AudioDevId:         音频设备编号
 *  pVqeConfig:         声音增强结构体
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetVqeAttr(AUDIO_DEV AudioDevId, AI_VQE_CONFIG *pVqeConfig);

/**
 *  获取AI 实时音量
 *  AudioDevId:         音频设备编号
 *  pRtVolume:          实时音量
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetRTVolume(AUDIO_DEV AudioDevId, U16 *pRtVolume);

/**
 *  设置AI ADC
 *  AudioDevId:         音频设备编号
 *  pAdc:                  adc控制
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_SetADC(AUDIO_DEV AudioDevId, AiAdcConfig_t *pAdc);

/**
 *  获取AI ADC设置
 *  AudioDevId:         音频设备编号
 *  pAdc:                  adc参数
 *  return:                 MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AI_GetADC(AUDIO_DEV AudioDevId, AiAdcConfig_t *pAdc);

/**
 *  设置AO设备参数
 *  AudioDevId:         音频设备编号
 *  pstAttr:            音频设备参数
 *  return:
 */
MI_RET MI_AO_SetDevAttr(AUDIO_DEV AudioDevId, const AioDevAttr_t *pstAttr);

/**
 *  获取AO设备参数
 *  AudioDevId:         音频设备编号
 *  pstAttr:            音频设备参数
 *  return:
 */
MI_RET MI_AO_GetDevAttr(AUDIO_DEV AudioDevId, AioDevAttr_t *pstAttr);

/**
 *  设置AO通道参数
 *  AoChn:         音频设备编号
 *  pstChnParam   音频通道属性
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_SetChnAttr(AIO_CHN AoChn, const AioChnAttr_t* pstChnParam);

/**
 * 获取AO通道参数
 *  AoChn:            音频通道号
 *  pstChnParam   音频通道属性
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */

MI_RET MI_AO_GetChnAttr(AIO_CHN AoChn, AioChnAttr_t* pstChnParam);


/**
 *  启用AO设备
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_Enable(AUDIO_DEV AudioDevId);

/**
 *  禁用AO设备
 *  AudioDevId:         音频设备编号
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_Disable(AUDIO_DEV AudioDevId);

/**
 *  启用AO通道
 *  AudioDevId:         音频设备编号
 *  AoChn:              音频通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_EnableChn(AUDIO_DEV AudioDevId, AIO_CHN AoChn);

/**
 *  禁用AO通道
 *  AudioDevId:         音频设备编号
 *  AoChn:              音频通道
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_DisableChn(AUDIO_DEV AudioDevId, AIO_CHN AoChn);

/**
 *  设置AO音量
 *  AudioDevId:         音频设备编号
 *  sVolume:        音量
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_SetVolume(AUDIO_DEV AudioDevId, S32 sVolume);

/**
 *  获取AO音量
 *  AudioDevId:         音频设备编号
 *  sVolume:       音量数据地址
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_GetVolume(AUDIO_DEV AudioDevId, S32 *sVolume);

/**
 *  设置AO静音状态
 *  AudioDevId:         音频设备编号
 *  ptMute:             禁用结构体
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_SetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);

/**
 *  获取AO静音状态
 *  AudioDevId:         音频设备编号
 *  ptMute:             禁用结构体
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_GetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);


/**
 *  发送音频到AO
 *  AudioDevId:         音频设备编号
 *  AoChn:                音频通道号
 *  pFrame:               音频数据帧
 *  sBlock:                -1,阻塞;　0　非阻塞
 *  pAoCB:                非阻塞回调函数
 *  return:             MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_AO_SendFrame(AUDIO_DEV AudioDevId, AIO_CHN AoChn, AudioFrame_t *pFrame, S32 sBlock, AdecCallback_t *pAoCB);


#ifdef __cplusplus
}
#endif //__cplusplus
#endif // __MI_AIO_H__

