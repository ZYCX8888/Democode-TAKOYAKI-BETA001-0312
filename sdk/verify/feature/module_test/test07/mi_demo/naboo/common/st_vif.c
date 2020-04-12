/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
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
        case 4:
            pclk_sel = VD_PORT_CKL_36M;
        default:
            break;
    }

    pAddaInitRes->iCount = 1;
    pAddaInitRes->iChipTypeNum = 1;

    pAddaInitRes->AdRes[0].iBusId = 0;
    pAddaInitRes->AdRes[0].iChipAddr = ChToI2c[dev];
    pAddaInitRes->AdRes[0].ucChnCount = 4; //max channels

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

int vif_dh9931_Init(int VifDev , int channel, int pclk, int edge)
{
    unsigned short regVal = 0;
    int i;
    int ChnnaleOffset = 0, ChannelRange = 0;

    pid_t pid;
    AD_DETECT_STATUS_S ad_status;
    ADS_RES_S AddaInitRes;
    MI_U32 count = 3;

    ST_INFO("dh9931 dev %d init chn:%d pclk %d begin!\n", VifDev, channel, pclk);
    while(count--)
    {
        vif_i2c_read(0x60 + 2 * VifDev, 0x0507, &regVal, I2C_FMT_A16D8);

        if ((regVal != 0x1c) && (0 == count))
        {
            ST_ERR("dev %d i2c status err %u\n", VifDev, regVal);
            //return -1;
        }
        usleep(1000*1000);
    }
    pid = vfork();

    if(pid == -1)
    {
        ST_ERR("create adda process fail!\n");
        return -1;
    }
    else if(pid == 0)
    {
        vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);

        Adda_I2COpsRegister(i2c_dummy_write, i2c_dummy_read);
        Adda_HWResetOpsRegister(reset_dummy);

        memset(&AddaInitRes, 0, sizeof(AddaInitRes));

        adda_config(VifDev, channel, edge, pclk , &AddaInitRes);
        Adda_Resource_Init(&AddaInitRes);
        Adda_Init();
        if(VifDev == 2)
        {
            vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x23, I2C_FMT_A16D8);
            //0x16~0x29 is ok, get middle value 0x23
        }
        else
            vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);

        ST_INFO("dh9931 dev %d init chn:%d pclk %d line:%d!\n", VifDev, channel, pclk, __LINE__);
        ChnnaleOffset = MI_VIF_MAX_WAY_NUM_PER_DEV * VifDev;
        if (pclk == 3)
        {
            ChannelRange = channel;
        }
        else if (pclk == 1)
        {
            ChannelRange = channel;
        }
        else if (2 == pclk)
        {
            if (4 == channel)
            {
                ChannelRange = 4;
            }
            else
            {
                ChannelRange = 1;
            }
        }
        else
        {
            ChannelRange = channel;
        }
        for(i = ChnnaleOffset; i < ChnnaleOffset + ChannelRange; i++)
        {
            count = 12;
            while (count--)
            {
                usleep(500*1000);
                Adda_GetVideoInStatus(i, &ad_status);
                ST_INFO("\nchn %d vfmt: %d, lost: %d, report fmt: %d, sigtype: %d\n", i,
                    ad_status.iVideoFormat, ad_status.iLostStatus, ad_status.iReportFormat, ad_status.iVideoSignalType);
                if (1 == ad_status.iLostStatus)
                {
                    ST_INFO("Video lost Chn(%d)!!!\n", i);
                    continue;
                }

                if (((AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat) || (AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat))
                    && (VIDEO_TYPE_CVBS == ad_status.iVideoSignalType))
                {
                    ST_INFO("Channel(%d) Detect video format CVBS(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else if ((AD_VIDEO_IN_HD_1080P_25HZ == ad_status.iVideoFormat) || (AD_VIDEO_IN_HD_1080P_30HZ == ad_status.iVideoFormat))
                {
                    ST_INFO("Channel(%d) Detect video format HDCVI format(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else if (AD_VIDEO_IN_HD_720P_25HZ == ad_status.iVideoFormat)
                {
                    ST_INFO("Channel(%d) Detect video format 720P(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else if ((AD_VIDEO_IN_HD_2560x1440_25HZ == ad_status.iVideoFormat) ||
                    (AD_VIDEO_IN_HD_2560x1440_30HZ == ad_status.iVideoFormat) ||
                    (AD_VIDEO_IN_HD_3840x2160_15HZ == ad_status.iVideoFormat) ||
                    (AD_VIDEO_IN_HD_3840x2160_12HZ == ad_status.iVideoFormat))
                    {
                        ST_INFO("Channel(%d) Detect video format 4M or 4K(%d)\n", i, ad_status.iVideoFormat);
                        goto detect_ok;
                    }
                else
                {
                    ST_WARN("Detect video format incorrect!!!\n");
                    continue;
                }
            }
  detect_ok:
            count = 0;
        }
        exit(0);
    }
    if(pid == wait(NULL))
    {
        ST_INFO("vfork =================== %d ======================\n", pid);
    }

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

    MI_S32 channel, pclk, edge;
    edge = VD_PORT_EDGE_UP;

    if(e_WorkMode == SAMPLE_VI_MODE_1_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_1MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 0;
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_4_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656D1_4MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 4;
        pclk = 0;
    }
    else if(e_WorkMode == SAMPLE_VI_MODE_1_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 2;
        pclk = 3;
        edge = VD_PORT_EDGE_DUAL;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 2;
        pclk = 3;
        edge = VD_PORT_EDGE_DUAL;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 2;
        pclk = 3;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_1_720P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2_D1)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT656FHD_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 2;
        pclk = 1;
        edge = VD_PORT_EDGE_DUAL;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_4M)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
        edge = VD_PORT_EDGE_DUAL;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_2X_DOUBLE_4K15)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
        edge = VD_PORT_EDGE_DUAL;
    }
    else if (e_WorkMode == SAMPLE_VI_MODE_1120_2X_DOUBLE_1080P)
    {
        memcpy(&stDevAttr, &DEV_ATTR_BT1120_DOUBLE_2MUX, sizeof(MI_VIF_DevAttr_t));
        channel = 1;
        pclk = 3;
        edge = VD_PORT_EDGE_DUAL;
    }

    ExecFunc(vif_dh9931_Init(VifDev, channel, pclk, edge), 0);

    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);
    return MI_SUCCESS;
}

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
    if (pstPortInfoInfo->u32IsInterlace)
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    }
    else
    {
        stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    }
    stChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    // stChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    stChnPortAttr.eFrameRate = pstPortInfoInfo->u32FrameRate;
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

    //MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 5);

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
