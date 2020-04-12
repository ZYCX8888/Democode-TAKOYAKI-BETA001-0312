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
#include <stdio.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_vdec_datatype.h"
#include "st_common.h"

MI_S32 ST_Vdec_CreateChannel(MI_S32 s32VdecChannel, MI_VDEC_ChnAttr_t *PstVdecChnAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;

    STCHECKRESULT(MI_VDEC_CreateChn(s32VdecChannel, PstVdecChnAttr));
    STCHECKRESULT(MI_VDEC_StartChn(s32VdecChannel));

    return MI_SUCCESS;
}
