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
#ifndef _MI_UVC_H_
#define _MI_UVC_H_
#include "mi_uvc_datatype.h"

#ifdef  MI_INFINITY2_ENABLE
#else
#endif

#include "myvideo.h"

extern int pthread_setname_np(pthread_t __target_thread, 
        const char *__name);

MI_S32 MI_UVC_Init(char *uvc_name);
MI_S32 MI_UVC_Uninit(void);
MI_S32 MI_UVC_CreateDev(MI_UVC_CHANNEL Chn,MI_UVC_PORT PortId,const MI_UVC_ChnAttr_t* pstAttr);
MI_S32 MI_UVC_DestroyDev(MI_UVC_CHANNEL Chn);
MI_S32 MI_UVC_SetChnAttr(MI_UVC_CHANNEL Chn, const MI_UVC_ChnAttr_t* pstAttr);
MI_S32 MI_UVC_GetChnAttr(MI_UVC_CHANNEL Chn, const MI_UVC_ChnAttr_t* pstAttr);
MI_S32 MI_UVC_StartDev(void);
MI_S32 MI_UVC_StopDev(void);

#endif //_MI_UVC_H_

