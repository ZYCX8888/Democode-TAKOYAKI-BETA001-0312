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
#ifndef _ST_VDISP_H
#define _ST_VDISP_H

#include "st_common.h"
#include "mi_vdisp_datatype.h"

#define MI_VDISP_DEV_0 0
MI_S32 ST_Vdisp_Init(void);
MI_S32 ST_Vdisp_Exit(void);

MI_S32 ST_Vdisp_SetInputPortAttr(MI_VDISP_DEV DevId, MI_S32 s32InputPort, ST_Rect_t *pstRect);
MI_S32 ST_Vdisp_EnableInputPort(MI_VDISP_DEV DevId,MI_VDISP_PORT PortId);
MI_S32 ST_Vdisp_DisableInputPort(MI_VDISP_DEV DevId,MI_VDISP_PORT PortId);

MI_S32 ST_Vdisp_SetOutputPortAttr(MI_VDISP_DEV DevId, MI_S32 s32OutputPort,
    ST_Rect_t *pstRect, MI_S32 s32FrmRate, MI_S32 s32FrmDepth);
MI_S32 ST_Vdisp_StartDevice(MI_VDISP_DEV DevId);
MI_S32 ST_Vdisp_StopDevice(MI_VDISP_DEV DevId);

#endif//_ST_VDISP_H
