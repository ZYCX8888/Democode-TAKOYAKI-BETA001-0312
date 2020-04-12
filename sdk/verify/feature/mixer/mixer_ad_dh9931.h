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
#ifndef _MIXER_AD_DH9931_H_
#define _MIXER_AD_DH9931_H_

#include "mixer_common.h"

int mixer_ad_dh9931_init(MI_VIF_DEV u32VifDev , VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk);

#endif // _MIXER_AD_DH9931_H_
