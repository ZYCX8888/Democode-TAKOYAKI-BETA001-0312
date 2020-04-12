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
#ifndef _ST_DIVP_H
#define _ST_DIVP_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_divp.h"
#define ST_DIVP_PIP_CHN 8

//divp module
MI_S32 ST_Divp_Init();

//divp channel
MI_S32 ST_Divp_CreatChannel(MI_DIVP_CHN DivpChn, MI_SYS_Rotate_e eRoate, MI_SYS_WindowRect_t *pstCropWin);
MI_S32 ST_Divp_SetOutputAttr(MI_DIVP_CHN DivpChn, MI_SYS_PixelFormat_e eOutPixel, MI_SYS_WindowRect_t *pstOutWin);
MI_S32 ST_Divp_StartChn(MI_DIVP_CHN DivpChn);

MI_S32 ST_Divp_StopChn(MI_DIVP_CHN DivpChn);
MI_S32 ST_Divp_DestroyChn(MI_DIVP_CHN DivpChn);


#endif//_ST_DIVP_H
