#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "st_vif.h"
#include "st_common.h"

//#define UVC_SUPPORT_LL

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

MI_VIF_DevAttr_t DEV_ATTR_MIPI_HDR =
{
    E_MI_VIF_MODE_MIPI,
    E_MI_VIF_WORK_MODE_RGB_FRAMEMODE,
    E_MI_VIF_HDR_TYPE_DOL,
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

MI_S32 ST_Vif_EnableDev(MI_VIF_DEV VifDev, VIF_WORK_MODE_E enWorkMode)
{
    MI_VIF_DevAttr_t stDevAttr;
    MI_S32 s32Ret = MI_SUCCESS;

    //memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
    memcpy(&stDevAttr, &DEV_ATTR_MIPI_HDR, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;//E_MI_VIF_WORK_MODE_RGB_REALTIME;
    if (enWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_FRAME)
    {
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    }
    else if (enWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME)
    {
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    }
    stDevAttr.bMirror = FALSE;
    stDevAttr.bFlip = FALSE;

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);

    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev)
{
    ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn,
                        MI_VIF_PORT VifPort, ST_VIF_PortInfo_T *pstPortInfoInfo)
{
    MI_SYS_ChnPort_t stChnPort;
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

    stChnPortAttr.eFrameRate = pstPortInfoInfo->s32FrameRate;
#ifdef UVC_SUPPORT_LL
	stChnPortAttr.u32FrameModeLineCount = 10;
#endif
    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr), MI_SUCCESS);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = VifDev;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 6), MI_SUCCESS);

    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);

    return MI_SUCCESS;
}

