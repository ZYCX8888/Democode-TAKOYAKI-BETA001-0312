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
#ifndef __ST_CEVA_H__
#define __ST_CEVA_H__

#include "mi_sys_datatype.h"
#include "st_vpe.h"

typedef struct
{
    ST_VPE_PortInfo_t stPortInfo;
    MI_SYS_ChnPort_t stChnPort;
}ST_CEVA_PortInfo_t;

typedef struct
{
    MI_U16 u16SrcWidth;
    MI_U16 u16SrcHeight;
    MI_U16 u16DstWidth;
    MI_U16 u16DstHeight;
}ST_CEVA_ResolutionMap_t;


MI_S32 ST_CEVA_Init(void);
MI_S32 ST_CEVA_Deinit();
MI_S32 ST_CEVA_Start();
MI_S32 ST_CEVA_Stop();
MI_S32 ST_CEVA_RegisterChn(ST_CEVA_PortInfo_t *pstCdnnPort, ST_CEVA_ResolutionMap_t *pstResolutionMap);
MI_S32 ST_CEVA_UnRegisterChn(MI_SYS_ChnPort_t stCdnnPort);


#endif

