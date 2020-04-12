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

#include "AddaSysApi.h"
#include "i2c.h"


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

static void adda_config(int dev, int max_chs, int edge, int pclk , ADS_RES_S* pAddaInitRes)
{
    unsigned int ChToI2c[] = {0x60, 0x62, 0x64, 0x66}; //channel to device i2c slave address
    unsigned int tmux_mode = 1;

    switch(max_chs)
    {
        case 1:
            tmux_mode = VD_PORT_MUX_MODE_1MUX;
            break;

        case 2:
            tmux_mode = VD_PORT_MUX_MODE_2MUX;
            break;

        case 4:
            tmux_mode = VD_PORT_MUX_MODE_4MUX;
            break;

        default:
            break;
    }

    int pclk_sel = 0;

    switch(pclk)
    {
        case 0:
            pclk_sel = VD_PORT_CKL_AUTO;
            break;
        case 1:
            pclk_sel = VD_PORT_CKL_27M;
            break;
        case 2:
            pclk_sel = VD_PORT_CKL_74_25M;
            break;
        case 3:
            pclk_sel = VD_PORT_CKL_148_5M;
            break;
        default:
            break;
    }

    pAddaInitRes->iCount = 1;
    pAddaInitRes->iChipTypeNum = 1;

    pAddaInitRes->AdRes[0].iBusId = 0;
    pAddaInitRes->AdRes[0].iChipAddr = ChToI2c[dev];
    pAddaInitRes->AdRes[0].ucChnCount = 4; //max channels

    /*通道映射配置，逻辑通道0 ~3 对应第一片物理通道0,1,2,3*/
    pAddaInitRes->AdRes[0].stChnMap[0].ucLogicChn = dev*4;
    pAddaInitRes->AdRes[0].stChnMap[0].ucPhyChn = 0;
    pAddaInitRes->AdRes[0].stChnMap[1].ucLogicChn = dev*4 + 1;
    pAddaInitRes->AdRes[0].stChnMap[1].ucPhyChn = 1;
    pAddaInitRes->AdRes[0].stChnMap[2].ucLogicChn = dev*4 + 2;
    pAddaInitRes->AdRes[0].stChnMap[2].ucPhyChn = 2;
    pAddaInitRes->AdRes[0].stChnMap[3].ucLogicChn = dev*4 + 3;
    pAddaInitRes->AdRes[0].stChnMap[3].ucPhyChn = 3;

    pAddaInitRes->AdRes[0].enEqMode = EQ_MODE_IN;/*使用内部EQ*/

    pAddaInitRes->AdRes[0].stVdPortDev.ucCount = 1; /*One port config*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdportClk = pclk_sel;
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdportEdge = (edge == 0) ? VD_PORT_EDGE_UP : VD_PORT_EDGE_DUAL;
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdPortMuxMode = tmux_mode; /*复用模式*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/
    pAddaInitRes->AdRes[0].enDriverPower = DRIVER_POWER_18V;/*BT656的逻辑电平*/

}

uint8_t i2c_dummy_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t val, uint8_t bus_id)
{
    vif_i2c_write(i2c_addr, reg_addr, val, I2C_FMT_A16D8);
    return 0;
}

uint8_t i2c_dummy_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t bus_id)
{
    uint16_t reg_val = 0;
    vif_i2c_read(i2c_addr, reg_addr, &reg_val, I2C_FMT_A16D8);
    return (uint8_t)(reg_val & 0xFF);
}

void reset_dummy(void)
{
    printf("adda hardware reset successfully!!\n");

    return ;
}

int vif_dh9931_Init(int VifDev , int channel, int pclk)
{
    unsigned short regVal = 0;
    int i;
    int ChnnaleOffset = 0, ChannelRange = 0;

    pid_t pid;
    AD_DETECT_STATUS_S ad_status;
    ADS_RES_S AddaInitRes;
    MI_U32 count = 2;
    MI_U32 score = 0;

    ST_INFO("dh9931 dev %d init chn:%d pclk %d begin!\n", VifDev, channel, pclk);

    vif_i2c_read(0x60 + 2 * VifDev, 0x0507, &regVal, I2C_FMT_A16D8);

    if(regVal != 0x1c)
    {
        ST_ERR("dev %d i2c status err %u\n", VifDev, regVal);
        return -1;
    }

    ST_INFO("dh9931 dev %d init chn:%d pclk %d line:%d!\n", VifDev, channel, pclk, __LINE__);
    pid = vfork();

    if(pid == -1)   //閿欒杩斿洖
    {
        ST_ERR("create adda process fail!\n");
        return -1;
    }
    else if(pid == 0)
    {
        vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);
        //vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x4f, I2C_FMT_A16D8);

        Adda_I2COpsRegister(i2c_dummy_write, i2c_dummy_read);
        Adda_HWResetOpsRegister(reset_dummy);

        memset(&AddaInitRes, 0, sizeof(AddaInitRes));
        adda_config(VifDev, channel, 0, pclk , &AddaInitRes);
        Adda_Resource_Init(&AddaInitRes);
        Adda_Init();
        sleep(8);
        vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);
        //vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x4f, I2C_FMT_A16D8);

        ST_INFO("dh9931 dev %d init chn:%d pclk %d line:%d!\n", VifDev, channel, pclk, __LINE__);
        ChnnaleOffset = MI_VIF_MAX_WAY_NUM_PER_DEV * VifDev;
        if (pclk == 3)
        {
            ChannelRange = 1;
        }
        else if (pclk == 1)
        {
            if (1 == channel)
            {
                ChannelRange = 1;
            }
            else
            {
                ChannelRange = 4;
            }
        }
        else if (2 == pclk)
        {
            if (4 == channel)
            {
                ChannelRange = 4;
            }
        }
        for(i = ChnnaleOffset; i < ChnnaleOffset + ChannelRange; i++)
        {
            count = 2;
            while (count--)
            {
                sleep(1);
                Adda_GetVideoInStatus(i, &ad_status);
                ST_INFO("chn %d vfmt: %d, lost: %d, report fmt: %d, sigtype: %d\n", i,
                    ad_status.iVideoFormat, ad_status.iLostStatus, ad_status.iReportFormat, ad_status.iVideoSignalType);
                if (1 == ad_status.iLostStatus)
                {
                    ST_INFO("Video lost Chn(%d)!!!\n", i);
                    continue;
                }
                switch (pclk)
                {
                    case 1:
                    {
                        if (((AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat) || (AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat))
                            && (VIDEO_TYPE_CVBS == ad_status.iVideoSignalType))
                        {
                            ST_INFO("Channel(%d) Detect video format CVBS(%d)\n", i, ad_status.iVideoFormat);
                            goto detect_ok;
                        }
                    }
                    case 3:
                    if ((AD_VIDEO_IN_HD_1080P_25HZ == ad_status.iVideoFormat) && (VIDEO_TYPE_HDCVI == ad_status.iVideoSignalType))
                    {
                        ST_INFO("Channel(%d) Detect video format HDCVI format(%d)\n", i, ad_status.iVideoFormat);
                        goto detect_ok;
                    }
                    case 2:
                    {
                        if (AD_VIDEO_IN_HD_720P_25HZ == ad_status.iVideoFormat)
                        {
                            ST_INFO("Channel(%d) Detect video format 720P(%d)\n", i, ad_status.iVideoFormat);
                            goto detect_ok;
                        }
                    }
                    defalut:
                    {
                        ST_WARN("Detect video format incorrect!!!\n");
                        continue;
                    }
                }
            }
        }
        exit(0);
    }
detect_ok:
    if(pid == wait(NULL))
    {
        ST_INFO("vfork =================== %d ======================\n", pid);
    }

    if(score == channel)
        ST_INFO("dh9931 dev %d init chn:%d score:%d pclk %d success!\n", VifDev, channel , score, pclk);
    else
        ST_INFO("dh9931 dev %d init chn:%d score:%d pclk %d fail!\n", VifDev, channel , score, pclk);

    return 0;
}

MI_S32 ST_Vif_Init(void)
{
    printf("ST Vif Init!!!\n");
    return MI_SUCCESS;
}

MI_S32 ST_Vif_Exit(void)
{
    printf("ST Vif Exit!!!\n");
    return MI_SUCCESS;
}

MI_S32 ST_Vif_CreateDev(MI_VIF_DEV VifDev, VIF_AD_WORK_MODE_E e_WorkMode)
{
    MI_VIF_DevAttr_t stDevAttr;
    MI_U32 u32Flag = 0;

    MI_S32 channel, pclk;

    if(e_WorkMode == SAMPLE_VI_MODE_1_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_1MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 1;
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_4_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_4MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 4;
        pclk = 1;
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_1_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_4_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 4;
        pclk = 2;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VPE ||
             e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VENC)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
        u32Flag = 1;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_FRAME)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
        u32Flag = 1;
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME)
    {
        memcpy(&stDevAttr, &DEV_ATTR_MIPI_BASE, sizeof(MI_VIF_DevAttr_t));
        u32Flag = 1;
        stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    }

    if (u32Flag == 0)
    {
        ExecFunc(vif_dh9931_Init(VifDev, channel, pclk), 0);
    }

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}
// MI_S32 ST_Vif_CreateChannel(MI_VIF_CHN VifChn)
// {
// 	return MI_SUCCESS;
// }

MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_t *pstPortInfoInfo)
{
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    stChnPortAttr.stCapRect.u16X = pstPortInfoInfo->u32RectX;
    stChnPortAttr.stCapRect.u16Y = pstPortInfoInfo->u32RectY;
    stChnPortAttr.stCapRect.u16Width = pstPortInfoInfo->u32RectWidth;
    stChnPortAttr.stCapRect.u16Height = pstPortInfoInfo->u32RectHeight;
    stChnPortAttr.stDestSize.u16Width = pstPortInfoInfo->u32DestWidth;
    stChnPortAttr.stDestSize.u16Height = pstPortInfoInfo->u32DestHeight;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    stChnPortAttr.u32FrameModeLineCount = 0;
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

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 3, 5);

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
