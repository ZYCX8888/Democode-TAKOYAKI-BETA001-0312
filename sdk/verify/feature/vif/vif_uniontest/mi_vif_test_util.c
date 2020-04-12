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
//#define _GNU_SOURCE


#include "mi_vif_test_util.h"
//#include "cam_os_wrapper.h"


#include "../i2c.h"


#if 0
void set_vif_dev_mask(MI_VIF_DEV ViDev, MI_VIF_DevAttr_t *pstDevAttr)
{
    switch(ViDev % 4)
    {
        case 0:
            pstDevAttr->au32CompMask[0] = 0xFF;

            if(E_MI_VIF_MODE_BT1120_STANDARD == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0x00FF0000;
            }
            else if(E_MI_VIF_MODE_BT1120_INTERLEAVED == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0x0;
            }

            break;

        case 1:
            pstDevAttr->au32CompMask[0] = 0xFF00;

            if(E_MI_VIF_MODE_BT1120_INTERLEAVED == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0x0;
            }

            break;

        case 2:
            pstDevAttr->au32CompMask[0] = 0xFF0000;

            if(E_MI_VIF_MODE_BT1120_STANDARD == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0xFF;
            }
            else if(E_MI_VIF_MODE_BT1120_INTERLEAVED == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0x0;
            }

            break;

        case 3:
            pstDevAttr->au32CompMask[0] = 0xFF000000;

            if(E_MI_VIF_MODE_BT1120_INTERLEAVED == pstDevAttr->eIntfMode)
            {
                pstDevAttr->au32CompMask[1] = 0x0;
            }

            break;

        default:
            assert(0);
    }
}
#endif


int my_write_yuv_file(FILE_HANDLE filehandle, MI_SYS_FrameData_t framedata)
{
    MI_U16 width, height;
    char *dst_buf;
    int i;

    //DBG_ENTER();

    if(!filehandle)
    {
        //printf("%s:%d filehandle is NULL,invalid parameter!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if(framedata.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV && framedata.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
    {
        printf("%s:%d ePixelFormat %d not support!\n", __FUNCTION__, __LINE__, framedata.ePixelFormat);
        return -1;
    }

    width = framedata.u16Width;
    height = framedata.u16Height;

    switch(framedata.ePixelFormat)
    {
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
            dst_buf = framedata.pVirAddr[0];

            for(i = 0; i < height; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width * 2, filehandle) != width * 2)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[0];
            }

            return 0;

        case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            dst_buf = framedata.pVirAddr[0];

            for(i = 0; i < height; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width, filehandle) != width)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[0];
            }

            dst_buf = framedata.pVirAddr[1];

            for(i = 0; i < height / 2; i++)
            {
                if(fwrite((char *)dst_buf, 1 , width, filehandle) != width)
                    goto ERR_RET;

                dst_buf += framedata.u32Stride[1];
            }

            return 0;

        default:
            DBG_ERR("%s:%d invalid format\n", __FUNCTION__, __LINE__);
            return -1;
    }

ERR_RET:
    DBG_ERR("fail\n");
    return -1;
}


MI_U32 vif_get_time()
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return 1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000;
}

#ifdef TEST_VIF_ENABLE_LIBADDA

static void vif_init_9931_AdRes(int dev, VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk , AD_RES_S* pAdRes)
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
    //printf("adda hardware reset successfully!!\n");

    return ;
}

long log = 0;

struct vif_video_info
{
    AD_VIDEO_IN_FORMAT_E eformat;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
};

static struct vif_video_info video_info[MI_VIF_MAX_PHYCHN_NUM];

int vif_set_vid_fmt(MI_VIF_DEV u32VifDev)
{
    AD_DETECT_STATUS_S ad_status;
    MI_U32 count;
    MI_VIF_CHN u32VifChn;
    struct vif_video_info* pInfo;

    DBG_ENTER();
    sleep(8);

    vif_i2c_write(0x60 + 2 * u32VifDev, 0x0802, 0x30, I2C_FMT_A16D8);

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        DBG_DEBUG("vi:%d\n", u32VifChn);

        pInfo = &video_info[u32VifChn];
        count = 2;

        while(count--)
        {
            DBG_DEBUG("vi:%d count:%d\n", u32VifChn, count);
            sleep(1);
            Adda_GetVideoInStatus(u32VifChn, &ad_status);

            switch(ad_status.iReportFormat)
            {
                case AD_VIDEO_IN_SD_NTSC:
                    pInfo->eformat = AD_VIDEO_IN_SD_NTSC;
                    pInfo->u32Width = 760;
                    pInfo->u32Height = 240;
                    pInfo->u32FrameRate = 60;
                    pInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
                    DBG_DEBUG("vi:%d ntsc success\n", u32VifChn);
                    count = 0;
                    break;;

                case AD_VIDEO_IN_SD_PAL:
                    pInfo->eformat = AD_VIDEO_IN_SD_PAL;
                    pInfo->u32Width = 720;
                    pInfo->u32Height = 288;
                    pInfo->u32FrameRate = 50;
                    pInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
                    DBG_DEBUG("vi:%d pal success\n", u32VifChn);
                    count = 0;
                    break;

                case AD_VIDEO_IN_HD_1080P_25HZ:
                    pInfo->eformat = AD_VIDEO_IN_HD_1080P_25HZ;
                    pInfo->u32Width = 1920;
                    pInfo->u32Height = 1080;
                    pInfo->u32FrameRate = 25;
                    pInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    DBG_DEBUG("vi:%d fhd success\n", u32VifChn);
                    count = 0;
                    break;

                case AD_VIDEO_IN_HD_1080P_30HZ:
                    pInfo->eformat = AD_VIDEO_IN_HD_1080P_30HZ;
                    pInfo->u32Width = 1920;
                    pInfo->u32Height = 1080;
                    pInfo->u32FrameRate = 30;
                    pInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    DBG_DEBUG("vi:%d fhd success\n", u32VifChn);
                    count = 0;
                    break;;

                //case  AD_VIDEO_IN_HD_720P_25HZ:
                default:
                    //printf("%s %d %d\n",__FUNCTION__,VifChn,ad_status.iReportFormat);
                    continue;
            }
        }
    }

    DBG_DEBUG("dev:%d success\n", u32VifDev);
    return 0;
}

void dump_dev_video_info(MI_VIF_DEV u32VifDev)
{
    MI_VIF_CHN u32VifChn;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;

    for(u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev ; u32VifChn < MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev + MI_VIF_MAX_WAY_NUM_PER_DEV ; u32VifChn ++)
    {
        if(vif_get_vid_fmt(u32VifChn , &u32Width, &u32Height, &u32FrameRate, &eScanMode))
        {
            DBG_DEBUG("pid:%d chn:%d vif_get_vid_fmt fail!\n", (int)getpid(), u32VifChn);
        }
        else
        {
            DBG_DEBUG("pid:%d vi:%d w:%u h:%u fps:%u scan:%u\n", (int)getpid(), u32VifChn, u32Width, u32Height, u32FrameRate, eScanMode);
        }
    }
}


int vif_dh9931_Init(int device , VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk)
{
    unsigned short regVal = 0;
    //struct init_9931_param  init_9931_thread_param;
    pid_t pid;
    vif_i2c_read(0x60 + 2 * device, 0x0507, &regVal, I2C_FMT_A16D8);

    if(regVal != 0x1c)
    {
        DBG_ERR("dev %d i2c status err %u\n", device, regVal);
        return -1;
    }

    DBG_INFO("dh9931 dev %d init mux_mode:%d edge:%d pclk:%d line:%d!\n", device, enVdPortMuxMode, enVdportEdge, enVdportClk, __LINE__);

    //int (*pthread_create)(pthread_t *, const pthread_attr_t *, void *(*) (void *), void *);

    //real_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
    //printf("%p %p\n",real_pthread_create,pthread_create);
    pid = vfork();

    if(pid == -1)   //错误返回
    {
        DBG_ERR("create adda process fail!\n");
        return -1;
    }
    else if(pid == 0)
    {
        ADS_RES_S AddaInitRes;
        DBG_DEBUG("pid:%d dh9931 dev %d init mux_mode:%d edge:%d pclk:%d line:%d!\n", (int)getpid(), device, enVdPortMuxMode, enVdportEdge, enVdportClk, __LINE__);
        vif_i2c_write(0x60 + 2 * device, 0x0802, 0x30, I2C_FMT_A16D8);
        Adda_I2COpsRegister(i2c_dummy_write, i2c_dummy_read);
        Adda_HWResetOpsRegister(reset_dummy);
        memset(&AddaInitRes, 0, sizeof(AddaInitRes));
        AddaInitRes.iCount = 1;
        AddaInitRes.iChipTypeNum = 1;
        vif_init_9931_AdRes(device, enVdPortMuxMode, enVdportEdge, enVdportClk , &AddaInitRes.AdRes[0]);
        Adda_Resource_Init(&AddaInitRes);
        Adda_Init();
        vif_set_vid_fmt(device);
        //dump_dev_video_info(device);
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
#endif

int vif_init_video_info(void)
{
#ifdef TEST_VIF_ENABLE_LIBADDA
    MI_VIF_CHN u32VifChn;

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        video_info[u32VifChn].eformat = 255;
        video_info[u32VifChn].u32Width = 0;
        video_info[u32VifChn].u32Height = 0;
        video_info[u32VifChn].u32FrameRate = 0;
        video_info[u32VifChn].eScanMode = E_MI_SYS_FRAME_SCAN_MODE_MAX;
    }
#endif
    return 0;
}

int vif_get_vid_fmt(MI_VIF_CHN u32VifChn , MI_U32* pu32Width, MI_U32* pu32Height , MI_U32* pu32FrameRate , MI_SYS_FrameScanMode_e* peScanMode)
{
#ifdef TEST_VIF_ENABLE_LIBADDA
    struct vif_video_info* pInfo = &video_info[u32VifChn];
    DBG_DEBUG("%d\n", u32VifChn);

    if(pInfo->eformat == 255)
    {
        DBG_DEBUG("\n");
        *pu32Width = 0;
        *pu32Height = 0;
        *pu32FrameRate = 0;
        *peScanMode = 0;
        return -1;
    }
    else
    {
        DBG_DEBUG("\n");
        *pu32Width = pInfo->u32Width;
        *pu32Height = pInfo->u32Height;
        *pu32FrameRate = pInfo->u32FrameRate;
        *peScanMode = pInfo->eScanMode;
        return 0;
    }

#else
    return -1;
#endif
}

int vif_init_hdmi(void)
{
    MI_HDMI_InitParam_t stHdmiInitParam;
    MI_HDMI_Attr_t stHdmiAttr;
    MI_HDMI_DeviceId_e eHdmiDevId = E_MI_HDMI_ID_0;

    stHdmiInitParam.pCallBackArgs = NULL;
    stHdmiInitParam.pfnHdmiEventCallback = NULL;
    ExecFunc(MI_HDMI_Init(&stHdmiInitParam), MI_SUCCESS);
    ExecFunc(MI_HDMI_Open(eHdmiDevId), MI_SUCCESS);
    ExecFunc(MI_HDMI_GetAttr(eHdmiDevId, &stHdmiAttr), MI_SUCCESS);
    stHdmiAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stHdmiAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stHdmiAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stHdmiAttr.stAudioAttr.bEnableAudio = TRUE;
    stHdmiAttr.stAudioAttr.bIsMultiChannel = 0;
    stHdmiAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stHdmiAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stHdmiAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stHdmiAttr.stVideoAttr.bEnableVideo = TRUE;
    stHdmiAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stHdmiAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stHdmiAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
    stHdmiAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    ExecFunc(MI_HDMI_SetAttr(eHdmiDevId, &stHdmiAttr), MI_SUCCESS);
    ExecFunc(MI_HDMI_Start(eHdmiDevId), MI_SUCCESS);
    return 0;
}

int vif_deInit_hdmi(void)
{
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_Stop(eHdmi);
    MI_HDMI_Close(eHdmi);
    MI_HDMI_DeInit();
    return MI_SUCCESS;
}

MI_U32 vif_cal_fps(MI_U32 u32FrameRate, MI_VIF_FrameRate_e eFrameRate)
{
    switch(eFrameRate)
    {
        case E_MI_VIF_FRAMERATE_FULL:
            return u32FrameRate;

        case E_MI_VIF_FRAMERATE_HALF:
            return u32FrameRate / 2;

        case E_MI_VIF_FRAMERATE_QUARTER:
            return u32FrameRate / 4;

        case E_MI_VIF_FRAMERATE_OCTANT:
            return u32FrameRate / 8;

        case E_MI_VIF_FRAMERATE_THREE_QUARTERS:
            return u32FrameRate * 3 / 4;

        default:
            return 0;
    }
}
