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
#ifndef _ST_HDMI_H
#define _ST_HDMI_H

#include "mi_hdmi.h"

/* call Init & Start */
MI_S32 ST_Hdmi_Init(void);
MI_S32 ST_Hdmi_DeInit(MI_HDMI_DeviceId_e eHdmi);
MI_S32 ST_Hdmi_Start(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_TimingType_e eTimingType);
MI_S32 ST_Hdmi_SetAttr(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_TimingType_e eTimingType);

MI_S32 ST_Hdmi_GetEdid(MI_HDMI_DeviceId_e eHdmi, MI_U8 *pu8Data, MI_U8 *u8Len);
MI_S32 ST_Hdmi_GetSinkInfo(MI_HDMI_DeviceId_e eHdmi);

#endif //_ST_HDMI_H
