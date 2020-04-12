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

#include "mixer.h"
#include <math.h>

uint8_t dh9931_write_reg(uint8_t i2c_addr, uint16_t reg_addr, uint8_t val, uint8_t bus_id)
{
    mixer_i2c_write(i2c_addr, reg_addr, val, I2C_FMT_A16D8);
    return 0;
}

uint8_t dh9931_read_reg(uint8_t i2c_addr, uint16_t reg_addr, uint8_t bus_id)
{
    uint16_t reg_val = 0;
    mixer_i2c_read(i2c_addr, reg_addr, &reg_val, I2C_FMT_A16D8);
    return (uint8_t)(reg_val & 0xFF);
}

void dh9931_reset(void)
{
    printf("adda hardware reset successfully!!\n");

    return ;
}


void dh9931_init_AdRes(int dev, VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk , AD_RES_S* pAdRes)
{
    pAdRes->iBusId = 0;
    pAdRes->iChipAddr = 0x60 + dev * 2;
    pAdRes->ucChnCount = 4;
    pAdRes->ucChipIndex = dev;
    pAdRes->stChnMap[0].ucLogicChn = dev * 4 + 0;
    pAdRes->stChnMap[0].ucPhyChn = 0;
    pAdRes->stChnMap[1].ucLogicChn = dev * 4 + 1;
    pAdRes->stChnMap[1].ucPhyChn = 1;
    pAdRes->stChnMap[2].ucLogicChn = dev * 4 + 2;
    pAdRes->stChnMap[2].ucPhyChn = 2;
    pAdRes->stChnMap[3].ucLogicChn = dev * 4 + 3;
    pAdRes->stChnMap[3].ucPhyChn = 3;
    pAdRes->enEqMode = EQ_MODE_IN;/*使用内部EQ*/
    pAdRes->stVdPortDev.ucCount = 1; /*One port config*/
    pAdRes->stVdPortDev.stVdPort[0].enVdportClk = enVdportClk;
    pAdRes->stVdPortDev.stVdPort[0].enVdportEdge = enVdportEdge;
    pAdRes->stVdPortDev.stVdPort[0].enVdPortMuxMode = enVdPortMuxMode;/*2路复用模式*/
    pAdRes->stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
    pAdRes->stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/
    pAdRes->enDriverPower = DRIVER_POWER_18V;/*BT656的逻辑电平*/
}

MI_S32 dh9931_vifchn_to_logicchn(MI_VIF_CHN u32VifChn)
{
    MI_VIF_DEV u32VifDev = u32VifChn/MI_VIF_MAX_WAY_NUM_PER_DEV;
    MI_VIF_WorkMode_e  eWorkMode = mixer_device_get_workmode(u32VifDev);

    switch (eWorkMode)
    {
        case E_MI_VIF_WORK_MODE_1MULTIPLEX:
        {
            if(u32VifChn%4)
                return -1;
            else
                return u32VifChn;
        }
        case E_MI_VIF_WORK_MODE_2MULTIPLEX:
        {
            if(u32VifChn%2)
                return -1;
            else
                return u32VifDev*4 + (u32VifChn%4)/2;
        }
        default:
            return u32VifChn;
    }
    return -1;
}

void dh9931_correct_data_latch_point(MI_VIF_DEV u32VifDev)
{
#if 0
    MI_U8 u8Mode = mixer_device_get_mode(u32VifDev);

    if (u8Mode == E_MIXER_DEVICE_MODE_2_720P)
    {
        if(u32VifDev == 0)
        {
             mixer_i2c_write(0x60 + 2 * u32VifDev, 0x0802, 0x4f, I2C_FMT_A16D8);
             return;
        }
    }
#endif
    mixer_i2c_write(0x60 + 2 * u32VifDev, 0x0802, 0x30, I2C_FMT_A16D8);
}

int dh9931_probe_video_info(MI_VIF_DEV u32VifDev)
{
#ifdef MIXER_ENABLE_LIBADDA

    AD_DETECT_STATUS_S ad_status;
    MI_U32 count;
    MI_VIF_CHN u32VifChn;
    int32 iChannel;

    DBG_ENTER();
    sleep(8);

    dh9931_correct_data_latch_point(u32VifDev);

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        DBG_DEBUG("vi:%d\n", u32VifChn);

#if 0
        if(E_MIXER_DEVICE_MODE_2_720P == mixer_device_get_mode(u32VifDev))
        {
            mixer_chn_set_video_info_by_fmt(u32VifDev*4 + 0,AD_VIDEO_IN_HD_720P_25HZ);
            mixer_chn_set_video_info_by_fmt(u32VifDev*4 + 2,AD_VIDEO_IN_HD_720P_25HZ);
            continue;
        }
#endif
        count = 3;
        while(count--)
        {
            DBG_DEBUG("vi:%d count:%d\n", u32VifChn, count);
            sleep(1);
            iChannel = dh9931_vifchn_to_logicchn(u32VifChn);
            DBG_DEBUG("iChannel:%d\n", iChannel);
            if(iChannel < 0)
                continue;
            Adda_GetVideoInStatus(dh9931_vifchn_to_logicchn(u32VifChn), &ad_status);
            switch(ad_status.iReportFormat)
            {
                case AD_VIDEO_IN_SD_NTSC:
                case AD_VIDEO_IN_SD_PAL:
                case AD_VIDEO_IN_HD_720P_25HZ:
                case AD_VIDEO_IN_HD_1080P_25HZ:
                case AD_VIDEO_IN_HD_1080P_30HZ:
                case AD_VIDEO_IN_HD_2560x1440_25HZ:
                    mixer_chn_set_video_info_by_fmt(u32VifChn,ad_status.iReportFormat);
                    count = 0;
                    break;;
                default:
                    continue;
            }
        }
    }
#endif

    DBG_DEBUG("dev:%d success\n", u32VifDev);
    return 0;
}

int mixer_ad_dh9931_init(MI_VIF_DEV u32VifDev , VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk)
{
    unsigned short regVal = 0;
    //struct init_9931_param  init_9931_thread_param;
    pid_t pid;
    mixer_i2c_read(0x60 + 2 * u32VifDev, 0x0507, &regVal, I2C_FMT_A16D8);

    if(regVal != 0x1c)
    {
        DBG_ERR("dev %d i2c status err %u\n", u32VifDev, regVal);
        return -1;
    }

    DBG_INFO("dh9931 dev %d init mux_mode:%d edge:%d pclk:%d line:%d!\n", u32VifDev, enVdPortMuxMode, enVdportEdge, enVdportClk, __LINE__);

    pid = vfork();

    if(pid == -1)   //错误返回
    {
        DBG_ERR("create adda process fail!\n");
        return -1;
    }
    else if(pid == 0)
    {
#ifdef MIXER_ENABLE_LIBADDA
        ADS_RES_S AddaInitRes;
        DBG_DEBUG("pid:%d dh9931 dev %d init mux_mode:%d edge:%d pclk:%d line:%d!\n", (int)getpid(), u32VifDev, enVdPortMuxMode, enVdportEdge, enVdportClk, __LINE__);
        dh9931_correct_data_latch_point(u32VifDev);
        Adda_I2COpsRegister(dh9931_write_reg, dh9931_read_reg);
        Adda_HWResetOpsRegister(dh9931_reset);
        memset(&AddaInitRes, 0, sizeof(AddaInitRes));
        AddaInitRes.iCount = 1;
        AddaInitRes.iChipTypeNum = 1;
        dh9931_init_AdRes(u32VifDev, enVdPortMuxMode, enVdportEdge, enVdportClk , &AddaInitRes.AdRes[0]);
        Adda_Resource_Init(&AddaInitRes);
        Adda_Init();
#endif
        dh9931_probe_video_info(u32VifDev);
        DBG_DEBUG("\n");
        exit(0);
    }

    if(pid == wait(NULL))
    {
        DBG_DEBUG("%d \n", pid);
    }

    //dump_dev_video_info(device);
    //sleep(1000);
    return 0;
}
