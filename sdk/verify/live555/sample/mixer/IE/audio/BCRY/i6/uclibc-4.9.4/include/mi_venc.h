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
#ifndef __MI_VENC_H__
#define __MI_VENC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "mi_venc_type.h"

/**
 *  创建编码通道
 *  VeChn:          VENC 通道号
 *  pstAttr:        VENC 通道属性指针
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_CreateChn(VENC_CHN VeChn, const VencChnAttr_t *pstAttr);

/**
 *  销毁编码通道
 *  VeChn:          VENC 通道号
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_DestroyChn(VENC_CHN VeChn);

/**
 *  开启编码通道接收输入图像
 *  VeChn:          VENC 通道号
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_StartRecvPic(VENC_CHN VeChn);

/**
 *  停止编码通道接收输入图像
 *  VeChn:          VENC 通道号
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_StopRecvPic(VENC_CHN VeChn);


/**
 *  获取编码通道对应的文件句柄
 *  VeChn:          VENC 通道号
 *  return:         文件句柄
 */
S32 MI_VENC_GetFd(VENC_CHN VeChn);

/**
 *  获取编码的码流
 *  VeChn:          VENC 通道号
 *  pstStream:      一帧图像数据
 *  bBlockFlag:     是否阻塞
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_GetStream(VENC_CHN VeChn, VencStream_t *pstStream, BOOL bBlockFlag);

/**
 *  获取编码的码流
 *  VeChn:          VENC 通道号
 *  pstStream:      一帧图像数据
 *  milliSec:       等待时间，单位是ms。-1为阻塞
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_GetStreamTimeOut(VENC_CHN VeChn, VencStream_t *pstStream, S32 milliSec);

/**
 *  释放码流缓存
 *  VeChn:          VENC 通道号
 *  pstStream:      一帧图像数据
 *  return:         MI_RET_SUCCESS成功，MI_RET_FAIL失败
 */
MI_RET MI_VENC_ReleaseStream(VENC_CHN VeChn, VencStream_t *pstStream);

MI_RET MI_VENC_SendFrame(VENC_CHN VeChn, VideoFrameInfo_t *pFrame, S32 mSec);

MI_RET MI_VENC_SendFrameEx(VENC_CHN VeChn, VideoFrameInfoEx_t *pFrame, S32 mSec);

MI_RET MI_VENC_SetChnAttr(VENC_CHN VeChn, const VencChnAttr_t *pstAttr);

MI_RET MI_VENC_GetChnAttr(VENC_CHN VeChn, VencChnAttr_t *pstAttr);

MI_RET MI_VENC_RequestIDR(VENC_CHN VeChn);

MI_RET MI_VENC_SetRoiCfg(VENC_CHN VeChn, const VencRoi_t *vencRoiCfg);

MI_RET MI_VENC_GetRoiCfg(VENC_CHN VeChn, U32 index, VencRoi_t *vencRoiCfg);

MI_RET MI_VENC_SetFrameRate(VENC_CHN VeChn, const VencFramerate_t *frameRate);

MI_RET MI_VENC_GetFrameRate(VENC_CHN VeChn, VencFramerate_t *frameRate);

/**
 *  设置分辨率。可以动态更改分辨率，只支持h264/h265
 *  VeChn:          VENC 通道号
 *  size:           分辨率
 *  return:         MI_RET_SUCCESS
 *                  MI_RET_FAIL
 *                  MI_RET_BAD_PARAMETER
 */
MI_RET MI_VENC_SetResolution(VENC_CHN VeChn, const Size_t *size);

/**
 *  获取分辨率。只支持h264/h265
 *  VeChn:          VENC 通道号
 *  size:           分辨率
 *  return:         MI_RET_SUCCESS
 *                  MI_RET_FAIL
 *                  MI_RET_BAD_PARAMETER
 */
MI_RET MI_VENC_GetResolution(VENC_CHN VeChn, Size_t *size);

MI_RET MI_VENC_SetH264SliceSplit(VENC_CHN VeChn, const VencH264SliceSplit_t *sliceSplit);

MI_RET MI_VENC_GetH264SliceSplit(VENC_CHN VeChn, VencH264SliceSplit_t *sliceSplit);

MI_RET MI_VENC_SetH264InterPred(VENC_CHN VeChn, const VencH264InterPred_t *interPred);

MI_RET MI_VENC_GetH264InterPred(VENC_CHN VeChn, VencH264InterPred_t *interPred);

MI_RET MI_VENC_SetH264IntraPred(VENC_CHN VeChn, const VencH264IntraPred_t *intraPred);

MI_RET MI_VENC_GetH264IntraPred(VENC_CHN VeChn, VencH264IntraPred_t *intraPred);

MI_RET MI_VENC_SetH264Gop(VENC_CHN VeChn, const VencGop_t *h264Gop);

MI_RET MI_VENC_GetH264Gop(VENC_CHN VeChn, VencGop_t *h264Gop);

MI_RET MI_VENC_SetH264Entropy(VENC_CHN VeChn, const VencH264Entropy_t *h264Entropy);

MI_RET MI_VENC_GetH264Entropy(VENC_CHN VeChn, VencH264Entropy_t *h264Entropy);

MI_RET MI_VENC_SetH264Poc(VENC_CHN VeChn, const VencH264Poc_t *h264Poc);

MI_RET MI_VENC_GetH264Poc(VENC_CHN VeChn, VencH264Poc_t *h264Poc);

MI_RET MI_VENC_SetH264Dblk(VENC_CHN VeChn, const VencH264Dblk_t *h264Dblk);

MI_RET MI_VENC_GetH264Dblk(VENC_CHN VeChn, VencH264Dblk_t *h264Dblk);

MI_RET MI_VENC_SetH264Disposable(VENC_CHN VeChn, const VencDisposable_t *h264Disposable);

MI_RET MI_VENC_GetH264Disposable(VENC_CHN VeChn, VencDisposable_t *h264Disposable);

MI_RET MI_VENC_InsertUserData(VENC_CHN VeChn, U8 *pData, U32 len);

MI_RET MI_VENC_SetH265SliceSplit(VENC_CHN VeChn, const VencH265SliceSplit_t *sliceSplit);

MI_RET MI_VENC_GetH265SliceSplit(VENC_CHN VeChn, VencH265SliceSplit_t *sliceSplit);

MI_RET MI_VENC_SetH265PredUnit(VENC_CHN VeChn, const VencH265PredUnit_t *predUnit);

MI_RET MI_VENC_GetH265PredUnit(VENC_CHN VeChn, VencH265PredUnit_t *predUnit);

MI_RET MI_VENC_SetH265Trans(VENC_CHN VeChn, const VencH265Trans_t *trans);

MI_RET MI_VENC_GetH265Trans(VENC_CHN VeChn, VencH265Trans_t *trans);

MI_RET MI_VENC_SetH265Entropy(VENC_CHN VeChn, const VencH265Entropy_t *entropy);

MI_RET MI_VENC_GetH265Entropy(VENC_CHN VeChn, VencH265Entropy_t *entropy);

MI_RET MI_VENC_SetH265Dblk(VENC_CHN VeChn, const VencH265Dblk_t *h265Dblk);

MI_RET MI_VENC_GetH265Dblk(VENC_CHN VeChn, VencH265Dblk_t *h265Dblk);

MI_RET MI_VENC_SetH265Sao(VENC_CHN VeChn, const VencH265Sao_t *sao);

MI_RET MI_VENC_GetH265Sao(VENC_CHN VeChn, VencH265Sao_t *sao);

MI_RET MI_VENC_SetH265Gop(VENC_CHN VeChn, const VencGop_t *gop);

MI_RET MI_VENC_GetH265Gop(VENC_CHN VeChn, VencGop_t *gop);

MI_RET MI_VENC_SetH265Disposable(VENC_CHN VeChn, const VencDisposable_t *h265Disposable);

MI_RET MI_VENC_GetH265Disposable(VENC_CHN VeChn, VencDisposable_t *h265Disposable);

MI_RET MI_VENC_SetRcParam(VENC_CHN VeChn, const VencRcParam_t *rcParam);

MI_RET MI_VENC_GetRcParam(VENC_CHN VeChn, VencRcParam_t *rcParam);

MI_RET MI_VENC_SetRcParamEx(VENC_CHN VeChn, const VencRcParamEx_t *rcParam);

MI_RET MI_VENC_GetRcParamEx(VENC_CHN VeChn, VencRcParamEx_t *rcParam);

MI_RET MI_VENC_SetMaxStreamCnt(VENC_CHN VeChn, U32 u32MaxStrmCnt);

MI_RET MI_VENC_GetMaxStreamCnt(VENC_CHN VeChn, U32 *pu32MaxStrmCnt);

MI_RET MI_VENC_SetJpegParam(VENC_CHN VeChn, const VencParamJpeg_t *jpegParam);

MI_RET MI_VENC_GetJpegParam(VENC_CHN VeChn, VencParamJpeg_t *jpegParam);

MI_RET MI_VENC_SetMjpegParam(VENC_CHN VeChn, const VencParamJpeg_t *jpegParam);

MI_RET MI_VENC_GetMjpegParam(VENC_CHN VeChn, VencParamJpeg_t *jpegParam);

MI_RET MI_VENC_SetColor2Grey(VENC_CHN VeChn, const VencColor2Grey_t *color2Grey);

MI_RET MI_VENC_GetColor2Grey(VENC_CHN VeChn, VencColor2Grey_t *color2Grey);

MI_RET MI_VENC_SetISPDebugInfo(VENC_CHN VeChn);

MI_RET MI_VENC_SetMjpegBypassStatus(VENC_CHN VeChn, MI_JPE_BYPASS_MODE eBypassStatus);

MI_RET MI_VENC_SetVui(VENC_CHN VeChn, const VencVui_t *vui);

MI_RET MI_VENC_GetVui(VENC_CHN VeChn, VencVui_t *vui);

MI_RET MI_VENC_SetRefParamEx(VENC_CHN VeChn, const VencParamRefEx_t *param);

MI_RET MI_VENC_GetRefParamEx(VENC_CHN VeChn, VencParamRefEx_t *param);

MI_RET MI_VENC_SetRefParam(VENC_CHN VeChn, const VencParamRef_t *param);

MI_RET MI_VENC_GetRefParam(VENC_CHN VeChn, VencParamRef_t *param);

//MI_RET MI_VENC_SetLtrParam(VENC_CHN VeChn, const VencParamLtr_t *param);

//MI_RET MI_VENC_GetLtrParam(VENC_CHN VeChn, VencParamLtr_t *param);


BOOL MI_VENC_GetFlushStatus(VENC_CHN VeChn);

#ifdef __cplusplus
}
#endif

#endif //__MI_VENC_H__

