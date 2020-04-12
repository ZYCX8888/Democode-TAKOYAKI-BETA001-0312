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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mi_vpe_test.h"
#ifndef ENABLE_WITH_DISP_TEST
#define ENABLE_WITH_DISP_TEST
#endif
#include "mi_vif.h"
#include "AddaSysApi.h"
#include "i2c.h"
#define DBG_INFO(msg, ...)
static ADS_RES_S AddaInitRes;
static long device = 0;
static long channel = 0;
static long pclk = 0;

static void adda_config(int dev, int max_chs, int edge, int pclk)
{
    unsigned int ChToI2c[] = {0x60, 0x62, 0x64, 0x66}; //channel to device i2c slave address
    memset(&AddaInitRes, 0, sizeof(AddaInitRes));
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

        //case 2:
        //  pclk_sel = VD_PORT_CKL_37_125M;
        //break;
        case 2:
            pclk_sel = VD_PORT_CKL_74_25M;
            break;

        case 3:
            pclk_sel = VD_PORT_CKL_148_5M;
            break;

        default:
            break;
    }

    AddaInitRes.iCount = 1;
    AddaInitRes.iChipTypeNum = 1;

    AddaInitRes.AdRes[0].iBusId = 0;
    AddaInitRes.AdRes[0].iChipAddr = ChToI2c[dev];
    AddaInitRes.AdRes[0].ucChnCount = max_chs; //max channels

    /*通道映射配置，逻辑通道0 ~3 对应第一片物理通道0,1,2,3*/
    AddaInitRes.AdRes[0].stChnMap[0].ucLogicChn = 0;
    AddaInitRes.AdRes[0].stChnMap[0].ucPhyChn = 0;
    AddaInitRes.AdRes[0].stChnMap[1].ucLogicChn = 1;
    AddaInitRes.AdRes[0].stChnMap[1].ucPhyChn = 1;
    AddaInitRes.AdRes[0].stChnMap[2].ucLogicChn = 2;
    AddaInitRes.AdRes[0].stChnMap[2].ucPhyChn = 2;
    AddaInitRes.AdRes[0].stChnMap[3].ucLogicChn = 3;
    AddaInitRes.AdRes[0].stChnMap[3].ucPhyChn = 3;

    AddaInitRes.AdRes[0].enEqMode = EQ_MODE_IN;/*使用内部EQ*/


    /*配置每个VO口的时序模式*/
    //AddaInitRes.AdRes[0].stVdPortDev.ucCount=1;/*4个VO口均启用*/
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdportClk = VD_PORT_CKL_27M;
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdportEdge = VD_PORT_EDGE_DUAL;
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortMuxMode = VD_PORT_MUX_MODE_2MUX;/*2路复用模式*/
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/

    AddaInitRes.AdRes[0].stVdPortDev.ucCount = 1; /*One port config*/
    //AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdportClk = VD_PORT_CKL_148_5M;
    AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdportClk = pclk_sel;
    AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdportEdge = (edge == 0) ? VD_PORT_EDGE_UP : VD_PORT_EDGE_DUAL;
    AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortMuxMode = tmux_mode;/*2路复用模式*/
    AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
    AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/
    AddaInitRes.AdRes[0].enDriverPower = DRIVER_POWER_18V;/*BT656的逻辑电平*/

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

#ifdef TEST_VIF_ENABLE_LIBADDA
long log = 0;

int vif_dh9931_Init(int device , int channel, int pclk)
{
    unsigned short regVal = 0;
    int count = 0;

    DBG_ENTER();

    printf("device=%u ", device);
    printf("max channels=%u ", channel);
    printf("log=%u ", log);
    printf("pclk=%u \n", pclk);

    vif_i2c_read(0x60 + 2 * device, 0x0507, &regVal, I2C_FMT_A16D8);

    if(regVal != 0x1c)
    {
        printf("ahd i2c status err\n");
        return -1;
    }

    Adda_I2COpsRegister(i2c_dummy_write, i2c_dummy_read);
    Adda_HWResetOpsRegister(reset_dummy);
    adda_config(device, channel, 0, pclk);
    Adda_Resource_Init(&AddaInitRes);
    Adda_Init();

    do
    {
        AD_DETECT_STATUS_S ad_status;

        Adda_GetVideoInStatus(0, &ad_status);
        printf("%d %d %d %d vfmt: %d, lost: %d, report fmt: %d, sigtype: %d\n", device, channel, pclk, count,
               ad_status.iVideoFormat, ad_status.iLostStatus, ad_status.iReportFormat, ad_status.iVideoSignalType);

        if(ad_status.iVideoFormat != AD_VIDEO_IN_HD_720P_25HZ)
            break;

        sleep(1);

        if(++count == 10)
        {
            if(log)
            {
                int ch = 0;

                for(ch = 0 ; ch < channel ; ch ++)
                    Adda_DumpReg(ch);
            }

            return -1;
        }
    }
    while(1);

    system("sleep 6");
    DBG_INFO("ahd %d init success,chn:%d pclk锛?d!\n", device, channel, pclk);
    return 0;
}
#endif

MI_S32 test_vpe_InitVif(MI_VIF_DEV u32VifDev, MI_VIF_CHN VifChn, MI_VIF_CHN VifPort, MI_SYS_WindowRect_t *pstCapWin, MI_SYS_PixelFormat_e ePixFmt, MI_VIF_FrameRate_e eFrameRate)
{
    MI_S32 s32Ret = 0;
    MI_VIF_ChnPortAttr_t stChnPortAttr;
    MI_SYS_ChnPort_t stChnPort;
    MI_VIF_DevAttr_t stDevAttr =
    {
        E_MI_VIF_MODE_BT656,
        E_MI_VIF_WORK_MODE_1MULTIPLEX,
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

vif_i2c_init();


#ifdef TEST_VIF_ENABLE_LIBADDA
    ExecFunc(vif_dh9931_Init(u32VifDev, channel, pclk), 0);
#else
    DBG_INFO("use system cmd for dh9931\n");
    char system_cmd[50] = {0};
    sprintf(system_cmd, "AHDInit -b 1 -c %d -d %d -e 0 -k %d &", channel, u32VifDev, pclk);
    system(system_cmd);
    system("sleep 10");
    system("busybox pgrep AHDInit | busybox xargs kill -9");
#endif

    s32Ret = MI_VIF_SetDevAttr(u32VifDev, &stDevAttr);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Set dev attributes failed with error code %#x!\n", s32Ret);
        return -1;
    }

    s32Ret = MI_VIF_EnableDev(u32VifDev);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Enable dev failed with error code %#x!\n", s32Ret);
        return -1;
    }

    stChnPortAttr.stCapRect.u16X = pstCapWin->u16X;
    stChnPortAttr.stCapRect.u16Y = pstCapWin->u16Y;
    stChnPortAttr.stCapRect.u16Width = pstCapWin->u16Width;
    stChnPortAttr.stCapRect.u16Height =pstCapWin->u16Height;
    stChnPortAttr.stDestSize.u16Width =pstCapWin->u16Width;
    stChnPortAttr.stDestSize.u16Height =pstCapWin->u16Height;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
    stChnPortAttr.ePixFormat = ePixFmt;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = eFrameRate;

    s32Ret = MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Set chn attributes failed with error code %#x!\n", s32Ret);
        return -1;
    }

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3);

    //DBG_INFO("\n");
    s32Ret = MI_VIF_EnableChnPort(VifChn, VifPort);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Enable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }
}

MI_S32 test_vpe_DeinitVif(MI_VIF_DEV u32VifDev, MI_VIF_CHN VifChn, MI_VIF_PORT VifPort)
{
    MI_S32 s32Ret;
    s32Ret = MI_VIF_DisableChnPort(VifChn, VifPort);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Disable chn failed with error code %#x!\n", s32Ret);
        return -1;
    }

    s32Ret = MI_VIF_DisableDev(u32VifDev);

    if(s32Ret != MI_SUCCESS)
    {
        printf("Disable dev failed with error code %#x!\n", s32Ret);
        return -1;
    }

    return 0;
}

MI_S32 test_vpeBinderFromVif(MI_U32 VifOutputPort, MI_U32 VpeInputChannel)
{
    //Bind VPE to DISP
     MI_SYS_ChnPort_t stSrcChnPort;
     MI_SYS_ChnPort_t stDstChnPort;
     MI_U32 u32SrcFrmrate;
     MI_U32 u32DstFrmrate;

     stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
     stSrcChnPort.u32DevId = 0;
     stSrcChnPort.u32ChnId = 0;
     stSrcChnPort.u32PortId = VifOutputPort;

     stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
     stDstChnPort.u32DevId = 0;
     stDstChnPort.u32ChnId = VpeInputChannel;
     stDstChnPort.u32PortId = 0;

     u32SrcFrmrate = 50;
     u32DstFrmrate = 50;

     ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
     return 0;
}

MI_S32 test_vpeUnBinderFromVif(MI_U32 VifOutputPort, MI_U32 VpeInputChannel)
{
    //Bind VPE to DISP
     MI_SYS_ChnPort_t stSrcChnPort;
     MI_SYS_ChnPort_t stDstChnPort;
     MI_U32 u32SrcFrmrate;
     MI_U32 u32DstFrmrate;

     stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
     stSrcChnPort.u32DevId = 0;
     stSrcChnPort.u32ChnId = 0;
     stSrcChnPort.u32PortId = VifOutputPort;

     stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
     stDstChnPort.u32DevId = 0;
     stDstChnPort.u32ChnId = VpeInputChannel;
     stDstChnPort.u32PortId = 0;

     ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
     return 0;
}
