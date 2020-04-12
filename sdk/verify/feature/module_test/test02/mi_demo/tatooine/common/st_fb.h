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
#ifndef _ST_FB_H
#define _ST_FB_H


MI_S32 ST_Fb_Init();
MI_S32 ST_Fb_SetColorKey();
MI_S32 ST_Fb_FillRect(const MI_SYS_WindowRect_t *pRect, MI_U32 u32ColorVal);
MI_S32 ST_Fb_DeInit();
MI_S32 ST_Fb_GetColorKey(MI_U32 *pu32ColorKeyVal);
MI_S32 ST_Fb_InitMouse(MI_S32 s32MousePicW, MI_S32 s32MousePicH, MI_S32 s32BytePerPixel, MI_U8 *pu8MouseFile);
MI_S32 ST_Fb_MouseSet(MI_U32 u32X, MI_U32 u32Y);
void ST_FB_Show(MI_BOOL bShown);

#endif//_ST_FB_H
