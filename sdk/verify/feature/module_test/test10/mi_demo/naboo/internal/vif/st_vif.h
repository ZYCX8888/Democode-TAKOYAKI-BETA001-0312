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
#ifndef _ST_VIF_H_
#define _ST_VIF_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_vif.h"

typedef enum
{
    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_VPE,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_VENC,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_FRAME,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME
} VIF_WORK_MODE_E;

typedef struct ST_VIF_PortInfo_s
{
	MI_U32 u32RectX;
	MI_U32 u32RectY;
	MI_U32 u32RectWidth;
	MI_U32 u32RectHeight;
	MI_U32 u32DestWidth;
	MI_U32 u32DestHeight;
    MI_U32 u32IsInterlace;
    MI_S32 s32FrameRate;
    MI_SYS_PixelFormat_e ePixFormat;
} ST_VIF_PortInfo_T;

MI_S32 ST_Vif_EnableDev(MI_VIF_DEV VifDev, VIF_WORK_MODE_E enWorkMode);
MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev);
MI_S32 ST_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn,
                        MI_VIF_PORT VifPort, ST_VIF_PortInfo_T *pstPortInfoInfo);
MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);


#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_VIF_H_
