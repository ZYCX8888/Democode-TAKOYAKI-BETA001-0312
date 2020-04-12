/*
* iqclient.h- Sigmastar
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
#ifndef _IQCLIENT_H
#define _IQCLIENT_H

#include "mi_isp_type.h"
#include "iqserver_cali_func.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef void* IPC_ISP_Handle;
int IPC_ISP_Init(IPC_ISP_Handle *phnd);
int IPC_ISP_Deinit(IPC_ISP_Handle hnd);
#if 0
int IPC_ISP_SetExposureAttr(IPC_ISP_Handle hnd, ExpoAttr_t ExpAttr);
int IPC_ISP_GetExposureAttr(IPC_ISP_Handle hnd, ExpoAttr_t *ExpAttr);
int IPC_ISP_SetWhiteBalanceAttr(IPC_ISP_Handle hnd, WBAttr_t WBAttr);
int IPC_ISP_GetWhiteBalanceAttr(IPC_ISP_Handle hnd, WBAttr_t *pWBAttr);
int IPC_ISP_SetWhiteBalanceAttrEX(IPC_ISP_Handle hnd, WBAttrEX_t WBAttr_ex);
int IPC_ISP_GetWhiteBalanceAttrEX(IPC_ISP_Handle hnd, WBAttrEX_t *pWBAttr_ex);
int IPC_ISP_SetBlackLevel(IPC_ISP_Handle hnd, OBCAttr_t BlackLevel);
int IPC_ISP_GetBlackLevel(IPC_ISP_Handle hnd, OBCAttr_t *pBlackLevel);
int IPC_ISP_SetDP(IPC_ISP_Handle hnd, DynamicDPAttr_t DefectPixel);
int IPC_ISP_GetDP(IPC_ISP_Handle hnd, DynamicDPAttr_t *pDefectPixel);
int IPC_ISP_SetCrosstalk(IPC_ISP_Handle hnd, CrosstalkAttr_t Crosstalk);
int IPC_ISP_GetCrosstalk(IPC_ISP_Handle hnd, CrosstalkAttr_t *pCrosstalk);
int IPC_ISP_SetAntiFalseColor(IPC_ISP_Handle hnd, FalseColorAttr_t FalseColor);
int IPC_ISP_GetAntiFalseColor(IPC_ISP_Handle hnd, FalseColorAttr_t *pFalseColor);
int IPC_ISP_SetCCM(IPC_ISP_Handle hnd, RGBMatrixAttr_t RGBMatrix);
int IPC_ISP_GetCCM(IPC_ISP_Handle hnd, RGBMatrixAttr_t *pRGBMatrix);
int IPC_ISP_SetSaturation(IPC_ISP_Handle hnd, S32 sSaturation);
int IPC_ISP_GetSaturation(IPC_ISP_Handle hnd, S32 *sSaturation);
int IPC_ISP_SetColorEnhancement(IPC_ISP_Handle hnd, BOOL bColorEnhance, U16 nCustomizedU, U16 nCustomizedV);
int IPC_ISP_GetColorEnhancement(IPC_ISP_Handle hnd, BOOL *bColorEnhance, U16 *nCustomizedU, U16 *nCustomizedV);
int IPC_ISP_SetDRC(IPC_ISP_Handle hnd, DRCAttr_t DRCAttr);
int IPC_ISP_GetDRC(IPC_ISP_Handle hnd, DRCAttr_t *DRCAttr);
int IPC_ISP_SetDefog(IPC_ISP_Handle hnd, Defog_t DefogAttr);
int IPC_ISP_GetDefog(IPC_ISP_Handle hnd, Defog_t *DefogAttr);
int IPC_ISP_SetNR3DAttr(IPC_ISP_Handle hnd, S32 NR3DAttr);
int IPC_ISP_GetNR3DAttr(IPC_ISP_Handle hnd, S32 *NR3DAttr);
int IPC_ISP_SetNR2DAttr(IPC_ISP_Handle hnd, NR2DAttr_t NR2DAttr);
int IPC_ISP_GetNR2DAttr(IPC_ISP_Handle hnd, NR2DAttr_t *NR2DAttr);
int IPC_ISP_SetSharpness(IPC_ISP_Handle hnd, Sharpness_t SharpnessAttr);
int IPC_ISP_GetSharpness(IPC_ISP_Handle hnd, Sharpness_t *SharpnessAttr);
int IPC_ISP_SetLightness(IPC_ISP_Handle hnd, S32 LightnessAttr);
int IPC_ISP_GetLightness(IPC_ISP_Handle hnd, S32 *LightnessAttr);
int IPC_ISP_SetBrightness(IPC_ISP_Handle hnd, U32 BrightnessAttr);
int IPC_ISP_GetBrightness(IPC_ISP_Handle hnd, U32 *BrightnessAttr);
int IPC_ISP_SetGamma(IPC_ISP_Handle hnd, Gamma_Lut_Type GammaAttr);
int IPC_ISP_GetGamma(IPC_ISP_Handle hnd, Gamma_Lut_Type *GammaAttr);
int IPC_ISP_SetContrast(IPC_ISP_Handle hnd, S32 ContrastAttr);
int IPC_ISP_GetContrast(IPC_ISP_Handle hnd, S32 *ContrastAttr);
int IPC_ISP_SetACM(IPC_ISP_Handle hnd, ACM_Attr_t ACMAttr);
int IPC_ISP_GetACM(IPC_ISP_Handle hnd, ACM_Attr_t *ACMAttr);
int IPC_ISP_QueryWBInfo(IPC_ISP_Handle hnd, ISP_AWB_INFO_t *pWBInfo);
int IPC_ISP_QueryExposureInfo(IPC_ISP_Handle hnd, ISP_EXP_INFO_t *pExpInfo);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
