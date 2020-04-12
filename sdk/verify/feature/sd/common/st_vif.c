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
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "st_vif.h"
#include "st_common.h"

//#define UVC_SUPPORT_LL


MI_VIF_DevAttr_t DEV_ATTR_BT656D1_1MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_1MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656D1_4MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_4MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656FHD_1MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_1MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656FHD_2MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_2MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_SINGLE_UP,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};
MI_VIF_DevAttr_t DEV_ATTR_MIPI_BASE =
{
    E_MI_VIF_MODE_MIPI,
    E_MI_VIF_WORK_MODE_RGB_REALTIME,
    E_MI_VIF_HDR_TYPE_OFF,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0},
    E_MI_VIF_CLK_EDGE_DOUBLE,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        E_MI_VIF_VSYNC_PULSE,
        E_MI_VIF_VSYNC_NEG_LOW,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            /*hsync_hfb  hsync_act  hsync_hhb*/
            0,            1920,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            1080,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT656FHD_DOUBLE_2MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_2MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_DOUBLE,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};

MI_VIF_DevAttr_t DEV_ATTR_BT1120_DOUBLE_2MUX =
{
    E_MI_VIF_MODE_BT1120_STANDARD,
    E_MI_VIF_WORK_MODE_2MULTIPLEX,
    E_MI_VIF_HDR_TYPE_OFF,
    {0xFF000000, 0x0},
    E_MI_VIF_CLK_EDGE_DOUBLE,
    { -1, -1, -1, -1},
    E_MI_VIF_INPUT_DATA_YUYV,
    {
        E_MI_VIF_VSYNC_FIELD,
        E_MI_VIF_VSYNC_NEG_HIGH,
        E_MI_VIF_HSYNC_VALID_SINGNAL,
        E_MI_VIF_HSYNC_NEG_HIGH,
        E_MI_VIF_VSYNC_VALID_SINGAL,
        E_MI_VIF_VSYNC_VALID_NEG_HIGH,
        {
            0,            0,        0,
            0,            0,        0,
            0,            0,        0
        }
    },
    FALSE
};


MI_S32 ST_Vif_CreateDev(MI_VIF_DEV VifDev, VIF_AD_WORK_MODE_E e_WorkMode)
{
    MI_VIF_DevAttr_t stDevAttr;
    
    if(e_WorkMode == SAMPLE_VI_MODE_1_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_1MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_4_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_4MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_1_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_1_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_4M)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_4K15)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_1120_2X_DOUBLE_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VPE ||
             e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VENC)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_FRAME)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    }

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_t *pstPortInfoInfo)
{
    MI_VIF_ChnPortAttr_t stChnPortAttr;

    memset(&stChnPortAttr, 0, sizeof(MI_VIF_ChnPortAttr_t));
    stChnPortAttr.stCapRect.u16X = pstPortInfoInfo->u32RectX;
    stChnPortAttr.stCapRect.u16Y = pstPortInfoInfo->u32RectY;
    stChnPortAttr.stCapRect.u16Width = pstPortInfoInfo->u32RectWidth;
    stChnPortAttr.stCapRect.u16Height = pstPortInfoInfo->u32RectHeight;
    stChnPortAttr.stDestSize.u16Width = pstPortInfoInfo->u32DestWidth;
    stChnPortAttr.stDestSize.u16Height = pstPortInfoInfo->u32DestHeight;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    if (pstPortInfoInfo->u32IsInterlace)
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    }
    else
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
    stChnPortAttr.ePixFormat = pstPortInfoInfo->ePixFormat;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = pstPortInfoInfo->s32FrameRate;
#ifdef UVC_SUPPORT_LL
	stChnPortAttr.u32FrameModeLineCount = 10;
#endif
    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_StartPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 6);

    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev)
{
    ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}
