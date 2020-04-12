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


static MixerVideoDeviceInfo_t stVideoDevInfo[MI_VIF_MAX_DEV_NUM];

static MixerVideoChnInfo_t stVideoChnInfo[MI_VIF_MAX_PHYCHN_NUM];




MI_VIF_DevAttr_t DEV_ATTR_BT656_D1_1MUX =
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

MI_VIF_DevAttr_t DEV_ATTR_BT656_D1_4MUX =
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

MI_VIF_DevAttr_t DEV_ATTR_BT656_720P_2MUX =
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

MI_VIF_DevAttr_t DEV_ATTR_BT656_FHD_1MUX =
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

MI_VIF_DevAttr_t DEV_ATTR_BT656_400M_1MUX =
{
    E_MI_VIF_MODE_BT656,
    E_MI_VIF_WORK_MODE_1MULTIPLEX,
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

int mixer_init_video_info(void)
{
    MI_VIF_DEV u32VifDev;
    MI_VIF_CHN u32VifChn;
    // MixerVideoChnInfo_t* pstMixerVideoChnInfo;

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        stVideoDevInfo[u32VifDev].u8Mode = E_MIXER_DEVICE_MODE_NONE;
    }

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        stVideoChnInfo[u32VifChn].eformat      = 255;
        stVideoChnInfo[u32VifChn].u16Width     = 0;
        stVideoChnInfo[u32VifChn].u16Height    = 0;
        stVideoChnInfo[u32VifChn].u32FrameRate[0] = 0;
        stVideoChnInfo[u32VifChn].u32FrameRate[1] = 0;
        stVideoChnInfo[u32VifChn].eScanMode    = E_MI_SYS_FRAME_SCAN_MODE_MAX;
        stVideoChnInfo[u32VifChn].eFrameRate[0] = E_MI_VIF_FRAMERATE_FULL;
        stVideoChnInfo[u32VifChn].eFrameRate[1] = E_MI_VIF_FRAMERATE_FULL;
        stVideoChnInfo[u32VifChn].ePixFormat =  E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVideoChnInfo[u32VifChn].u8Path[0] =   E_MIXER_CHN_PATH_NONE;
        stVideoChnInfo[u32VifChn].u8Path[1] =   E_MIXER_CHN_PATH_NONE;
        stVideoChnInfo[u32VifChn].eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
        stVideoChnInfo[u32VifChn].bDump[0] = FALSE;
        stVideoChnInfo[u32VifChn].bDump[1] = FALSE;
    }

    return 0;
}


MI_U8 mixer_device_get_mode(MI_VIF_DEV u32VifDev)
{
    return stVideoDevInfo[u32VifDev].u8Mode;
}

int mixer_device_create(MI_VIF_DEV u32VifDev)
{
    MI_S32 s32Ret;
    MI_VIF_DevAttr_t stDevAttr;
    VD_PORT_MUX_MODE_E enVdPortMuxMode = VD_PORT_MODE_UNKNOW;
    VD_PORT_EDGE_E enVdportEdge = VD_PORT_EDGE_UP;
    VD_PORT_CLK_E enVdportClk = VD_PORT_CKL_AUTO;
    MI_U8 u8DeviceMode;

    DBG_ENTER();

    u8DeviceMode = mixer_device_get_mode(u32VifDev);

    switch(u8DeviceMode)
    {
        case E_MIXER_DEVICE_MODE_NONE:
            {
                return 0;
            }
            break;

        case E_MIXER_DEVICE_MODE_4_D1:
            {
                memcpy(&stDevAttr, &DEV_ATTR_BT656_D1_4MUX, sizeof(MI_VIF_DevAttr_t));
                enVdPortMuxMode = VD_PORT_MUX_MODE_4MUX;
                enVdportEdge = VD_PORT_EDGE_UP;
                enVdportClk = VD_PORT_CKL_27M;
            }
            break;

        case E_MIXER_DEVICE_MODE_4_960H:
            {
            }
            break;

        case E_MIXER_DEVICE_MODE_2_720P:
            {
                memcpy(&stDevAttr, &DEV_ATTR_BT656_720P_2MUX, sizeof(MI_VIF_DevAttr_t));
                enVdPortMuxMode = VD_PORT_MUX_MODE_2MUX;
                enVdportEdge = VD_PORT_EDGE_UP;
                enVdportClk = VD_PORT_CKL_74_25M;
            }
            break;

        case E_MIXER_DEVICE_MODE_1_1080P:
            {
                memcpy(&stDevAttr, &DEV_ATTR_BT656_FHD_1MUX, sizeof(MI_VIF_DevAttr_t));
                enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
                enVdportEdge = VD_PORT_EDGE_UP;
                enVdportClk = VD_PORT_CKL_148_5M;
            }
            break;

        case E_MIXER_DEVICE_MODE_1_400M:
            {
                memcpy(&stDevAttr, &DEV_ATTR_BT656_400M_1MUX, sizeof(MI_VIF_DevAttr_t));
                enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
                enVdportEdge = VD_PORT_EDGE_DUAL;
                enVdportClk = VD_PORT_CKL_288M;
            }
            break;

        default:
            DBG_DEBUG("u8DeviceMode = %u\n", u8DeviceMode);
            return -1;
    }

    ExecFunc(mixer_ad_dh9931_init(u32VifDev, enVdPortMuxMode, enVdportEdge, enVdportClk), 0);

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

    DBG_ENTER();
    return 0;
}

MI_VIF_WorkMode_e mixer_device_get_workmode(MI_VIF_DEV u32VifDev)
{
    MI_U8 u8Mode  = stVideoDevInfo[u32VifDev].u8Mode;

    switch(u8Mode)
    {
        case E_MIXER_DEVICE_MODE_1_1080P:
            return E_MI_VIF_WORK_MODE_1MULTIPLEX;

        case E_MIXER_DEVICE_MODE_2_720P:
            return E_MI_VIF_WORK_MODE_2MULTIPLEX;

        case E_MIXER_DEVICE_MODE_4_D1:
        case E_MIXER_DEVICE_MODE_4_960H:
            return E_MI_VIF_WORK_MODE_4MULTIPLEX;

        default:
            DBG_EXIT_ERR("dev:%d err_mode:0x%x\n", u32VifDev, u8Mode);
            return -1;
    }
}

int mixer_device_set_mode(MI_VIF_DEV u32VifDev, MI_U8 u8DevicesMode)
{
    if(u8DevicesMode >= E_MIXER_DEVICE_MODE_MAX)
    {
        DBG_EXIT_ERR("dev:%d err_mode:0x%x\n", u32VifDev, u8DevicesMode);
        return -1;
    }

    stVideoDevInfo[u32VifDev].u8Mode = u8DevicesMode;

    switch(u8DevicesMode)
    {
        case E_MIXER_DEVICE_MODE_4_D1:
            DBG_DEBUG("dev:%d %s\n", u32VifDev, "4_D1");
            break;

        case E_MIXER_DEVICE_MODE_4_960H:
            DBG_DEBUG("dev:%d %s\n", u32VifDev, "4_960H");
            break;

        case E_MIXER_DEVICE_MODE_2_720P:
            DBG_DEBUG("dev:%d %s\n", u32VifDev, "2_720P");
            break;

        case E_MIXER_DEVICE_MODE_1_1080P:
            DBG_DEBUG("dev:%d %s\n", u32VifDev, "1_1080P");
            break;

        case E_MIXER_DEVICE_MODE_1_400M:
            DBG_DEBUG("dev:%d %s\n", u32VifDev, "1_400M");
            break;

        default:
            break;
    }

    return 0;
}

int mixer_device_set_mode_all(char * ps8Mode)
{
    MI_VIF_DEV u32VifDev;
    DBG_DEBUG("%s\n", ps8Mode);

    if(strlen(ps8Mode) > MI_VIF_MAX_DEV_NUM)
    {
        return -1;
    }

    for(u32VifDev = 0 ; u32VifDev < strlen(ps8Mode)  ; u32VifDev ++)
    {
        mixer_device_set_mode(u32VifDev, ps8Mode[u32VifDev] - '0');
    }

    DBG_EXIT_OK();
    return 0;
}



int mixer_device_create_all(void)
{
    MI_VIF_DEV u32VifDev;

    for(u32VifDev = 0 ; u32VifDev < MI_VIF_MAX_DEV_NUM ; u32VifDev++)
    {
        mixer_device_create(u32VifDev);
    }

    mixer_chn_dump_video_info();
    return 0;
}

int mixer_device_get_layout(MI_VIF_DEV u32VifDev,MI_SYS_WindowSize_t stWinSize)
{
    MI_VIF_WorkMode_e eWorkMode = mixer_device_get_workmode(u32VifDev);

    if(eWorkMode == E_MI_VIF_WORK_MODE_1MULTIPLEX)
    {
        stWinSize.u16Width  = 1;
        stWinSize.u16Height = 1;
    }
    else if(eWorkMode == E_MI_VIF_WORK_MODE_2MULTIPLEX)
    {
        stWinSize.u16Width  = 1;
        stWinSize.u16Height = 2;
    }
    else if(eWorkMode == E_MI_VIF_WORK_MODE_4MULTIPLEX)
    {
        stWinSize.u16Width  = 2;
        stWinSize.u16Height = 2;
    }
    return 0;
}

int mixer_chn_get_video_info(MI_VIF_CHN u32VifChn , MixerVideoChnInfo_t*  pstChnInfo)
{
    //DBG_DEBUG("%d\n", u32VifChn);
    memcpy(pstChnInfo, &stVideoChnInfo[u32VifChn], sizeof(MixerVideoChnInfo_t));
    return 0;
}

MI_U32 mixer_chn_get_video_fps(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort)
{
    MI_U32 u32FrameRate = stVideoChnInfo[u32VifChn].u32FrameRate[u32VifPort];

    switch(stVideoChnInfo[u32VifChn].eFrameRate[u32VifPort])
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

int mixer_chn_set_video_info_by_fmt(MI_VIF_CHN u32VifChn , AD_VIDEO_IN_FORMAT_E eFmt)
{
    MixerVideoChnInfo_t* pChnInfo = &stVideoChnInfo[u32VifChn];
    DBG_INFO("chn:%d fmt:%d\n", u32VifChn, eFmt);

    switch(eFmt)
    {
        case AD_VIDEO_IN_SD_NTSC:
            {
                pChnInfo->eformat = AD_VIDEO_IN_SD_NTSC;
                pChnInfo->u16Width = 760;
                pChnInfo->u16Height = 240;
                pChnInfo->u32FrameRate[0] = 60;
                pChnInfo->u32FrameRate[1] = 60;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
                DBG_DEBUG("vi:%d 760x240_25 success\n", u32VifChn);
            }
            break;

        case AD_VIDEO_IN_SD_PAL:
            {
                pChnInfo->eformat = AD_VIDEO_IN_SD_PAL;
                pChnInfo->u16Width = 720;
                pChnInfo->u16Height = 288;
                pChnInfo->u32FrameRate[0] = 50;
                pChnInfo->u32FrameRate[1] = 50;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_INTERLACE;
                DBG_DEBUG("vi:%d 720x288_50 success\n", u32VifChn);
            }
            break;

        case AD_VIDEO_IN_HD_720P_25HZ:
            {
                pChnInfo->eformat = AD_VIDEO_IN_HD_720P_25HZ;
                pChnInfo->u16Width = 1280;
                pChnInfo->u16Height = 720;
                pChnInfo->u32FrameRate[0] = 25;
                pChnInfo->u32FrameRate[1] = 25;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                DBG_DEBUG("vi:%d 1280x720_25 success\n", u32VifChn);
            }
            break;

        case AD_VIDEO_IN_HD_1080P_25HZ:
            {
                pChnInfo->eformat = AD_VIDEO_IN_HD_1080P_25HZ;
                pChnInfo->u16Width = 1920;
                pChnInfo->u16Height = 1080;
                pChnInfo->u32FrameRate[0] = 25;
                pChnInfo->u32FrameRate[1] = 25;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                DBG_DEBUG("vi:%d 1920x1080_25 success\n", u32VifChn);
            }
            break;

        case AD_VIDEO_IN_HD_1080P_30HZ:
            {
                pChnInfo->eformat = AD_VIDEO_IN_HD_1080P_30HZ;
                pChnInfo->u16Width = 1920;
                pChnInfo->u16Height = 1080;
                pChnInfo->u32FrameRate[0] = 30;
                pChnInfo->u32FrameRate[1] = 30;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                DBG_DEBUG("vi:%d 1920x1080_30 success\n", u32VifChn);
            }
            break;

        case AD_VIDEO_IN_HD_2560x1440_25HZ:
            {
                pChnInfo->eformat = AD_VIDEO_IN_HD_2560x1440_25HZ;
                pChnInfo->u16Width = 2560;
                pChnInfo->u16Height = 1440;
                pChnInfo->u32FrameRate[0] = 25;
                pChnInfo->u32FrameRate[1] = 25;
                pChnInfo->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                DBG_DEBUG("vi:%d 2560x1440_25 success\n", u32VifChn);
            }
            break;

        default:
            return -1;
    }

    return 0;
}


int mixer_chn_set_framerate(MI_VIF_CHN u32VifChn ,MI_VIF_PORT u32VifPort, MI_VIF_FrameRate_e eFrameRate)
{
    stVideoChnInfo[u32VifChn].eFrameRate[u32VifPort]     = eFrameRate;
    return 0;
}

#if 0
int mixer_chn_set_video_info(MI_VIF_CHN u32VifChn , MixerVideoChnInfo_t*  pstChnInfo)
{

    stVideoChnInfo[u32VifChn].eformat         = pstChnInfo->eformat;
    stVideoChnInfo[u32VifChn].u16Width        = pstChnInfo->u16Width ;
    stVideoChnInfo[u32VifChn].u16Height       = pstChnInfo->u16Height;
    stVideoChnInfo[u32VifChn].u32FrameRate = pstChnInfo->u32FrameRate;
    stVideoChnInfo[u32VifChn].eScanMode       = pstChnInfo->eScanMode;
    return 0;
}
#endif
int mixer_chn_dump_video_info(void)
{
    MI_VIF_CHN u32VifChn;
    MixerVideoChnInfo_t stChnInfo;
    MI_VIF_PORT u32VifPort;

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        for(u32VifPort = 0 ; u32VifPort < MI_VIF_MAX_CHN_OUTPORT ; u32VifPort ++)
        {
            mixer_chn_get_video_info(u32VifChn, &stChnInfo);

            if(stChnInfo.eformat == 255)
                continue;

            if(u32VifPort == 0)
            {
                DBG_DEBUG("vi:%d port:%d w:%u h:%u scan:%u fps:%u path:%u\n",u32VifChn, u32VifPort,stChnInfo.u16Width, stChnInfo.u16Height,stChnInfo.eScanMode,stChnInfo.u32FrameRate[0],stChnInfo.u8Path[0]);
            }
            else
            {
                DBG_DEBUG("vi:%d port:%d w:%u h:%u scan:%u fps:%u path:%u\n",u32VifChn, u32VifPort,stChnInfo.u16Width/2, stChnInfo.u16Height/2,stChnInfo.eScanMode,stChnInfo.u32FrameRate[1],stChnInfo.u8Path[1]);
            }

        }
    }

    return 0;
}

int mixer_chn_set_path(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort, MI_U8 u8Path)
{
    MI_BOOL bDump;
    if(u8Path <= E_MIXER_CHN_PATH_NONE || u8Path >= E_MIXER_CHN_PATH_MAX)
    {
        if(u8Path != '0')
        {
            DBG_ERR("chn:%d port:%d err %d\n", u32VifChn, u32VifPort, u8Path);
            DBG_INFO("Expect [%d-%d]\n", E_MIXER_CHN_PATH_NONE + 1, E_MIXER_CHN_PATH_MAX - 1);
        }
        return -1;
    }

    switch(u8Path)
    {
        case E_MIXER_CHN_PATH_VIF:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif");
            break;

        case E_MIXER_CHN_PATH_VIF_DUMP:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-dump");
            break;

        case E_MIXER_CHN_PATH_VIF_VPE:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe");
            break;

        case E_MIXER_CHN_PATH_VIF_VPE_DUMP:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-dump");
            break;

        case E_MIXER_CHN_PATH_VIF_VPE_DISP:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-disp");
            break;

        case E_MIXER_CHN_PATH_VIF_DIVP_DISP:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-divp-disp");
            break;

        case E_MIXER_CHN_PATH_VIF_VPE_VENC:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-venc");
            break;
        case E_MIXER_CHN_PATH_VIF_VPE_VENC_H264:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-H264e");
            break;
        case E_MIXER_CHN_PATH_VIF_VPE_VENC_H265:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-H265e");
            break;
        case E_MIXER_CHN_PATH_VIF_VPE_VENC_JPEG:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-Jpege");
            break;

        case E_MIXER_CHN_PATH_VIF_VPE_VENC_DUMP:
            DBG_DEBUG("chn:%d port:%d %s\n", u32VifChn, u32VifPort, "vif-vpe-venc-dump");
            break;

        default:
            break;
    }

    bDump = FALSE;

    if(u8Path == E_MIXER_CHN_PATH_VIF_DUMP)
    {
        u8Path = E_MIXER_CHN_PATH_VIF;
        bDump = TRUE;
    }
    else if(u8Path == E_MIXER_CHN_PATH_VIF_VPE_DUMP)
    {
        u8Path = E_MIXER_CHN_PATH_VIF_VPE;
        bDump = TRUE;
    }
    else if(u8Path == E_MIXER_CHN_PATH_VIF_VPE_VENC_DUMP)
    {
        u8Path = E_MIXER_CHN_PATH_VIF_VPE_VENC;
        bDump = TRUE;
    }

    if(u8Path == E_MIXER_CHN_PATH_VIF_VPE_DISP || u8Path == E_MIXER_CHN_PATH_VIF_DIVP_DISP)
    {
        mixer_path_set_display(TRUE);
    }

    stVideoChnInfo[u32VifChn].u8Path[u32VifPort] = u8Path;
    stVideoChnInfo[u32VifChn].bDump[u32VifPort]    = bDump;
    return 0;

}

MI_U8 mixer_chn_get_path(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort)
{
    return stVideoChnInfo[u32VifChn].u8Path[u32VifPort];
}

