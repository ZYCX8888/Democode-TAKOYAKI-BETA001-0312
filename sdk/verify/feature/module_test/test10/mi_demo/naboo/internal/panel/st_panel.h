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
#ifndef __ST_PANEL_H__
#define __ST_PANEL_H__

#include "mi_panel.h"
#include "mi_panel_datatype.h"

MI_S32 ST_Panel_Init(MI_PANEL_LinkType_e enLinkType);

MI_S32 ST_Panel_DeInit();

MI_PANEL_ParamConfig_t *ST_Panel_GetParam(MI_PANEL_LinkType_e enLinkType);

#endif
