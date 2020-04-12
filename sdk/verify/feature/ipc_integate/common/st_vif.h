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
#ifndef _ST_VIF_H
#define _ST_VIF_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_vif.h"

typedef enum
{
    /* For Hi3531 or Hi3532 */
    SAMPLE_VI_MODE_1_D1 = 0,
    SAMPLE_VI_MODE_16_D1,
    SAMPLE_VI_MODE_16_960H,
    SAMPLE_VI_MODE_2_720P,
    SAMPLE_VI_MODE_1_1080P,
    /* For Hi3521 */
    SAMPLE_VI_MODE_8_D1,
    SAMPLE_VI_MODE_1_720P,
    SAMPLE_VI_MODE_16_Cif,
    SAMPLE_VI_MODE_16_2Cif,
    SAMPLE_VI_MODE_16_D1Cif,
    SAMPLE_VI_MODE_1_D1Cif,
    /*For Hi3520A*/
    SAMPLE_VI_MODE_4_D1,
    SAMPLE_VI_MODE_2_D1,
    SAMPLE_VI_MODE_8_2Cif,
    SAMPLE_VI_MODE_2X_DOUBLE_720P,
    SAMPLE_VI_MODE_2X_DOUBLE_1080P,
    SAMPLE_VI_MODE_2X_DOUBLE_4M,
    SAMPLE_VI_MODE_2X_DOUBLE_4K15,
    SAMPLE_VI_MODE_1120_2X_DOUBLE_1080P,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_VPE,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_VENC,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_FRAME,

    /*for MIPI*/
    SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME,

    /*for Parallel Sensor for FPGA test*/
    SAMPLE_VI_MODE_DIGITAL_CAM_1_1080P_REALTIME
} VIF_AD_WORK_MODE_E;

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
} ST_VIF_PortInfo_t;

//vif
MI_S32 ST_Vif_Init(void);
MI_S32 ST_Vif_Exit(void);
//vif dev
MI_S32 ST_Vif_CreateDev(MI_VIF_DEV VifDev, VIF_AD_WORK_MODE_E e_WorkMode);
MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev);

//vif channel not use
// MI_S32 ST_Vif_CreateChannel(MI_VIF_CHN VifChn);

//vif port
MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_t *pstPortInfoInfo);
MI_S32 ST_Vif_StartPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);
MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);



#endif //_ST_VPE_H
