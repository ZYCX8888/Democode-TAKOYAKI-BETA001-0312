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
#ifndef _MIXER_VIDEO_INFO_H_
#define _MIXER_VIDEO_INFO_H_

#include "mixer_common.h"

int mixer_init_video_info(void);

enum
{
    E_MIXER_DEVICE_MODE_NONE      = 3,
    E_MIXER_DEVICE_MODE_4_D1      = 5,
    E_MIXER_DEVICE_MODE_4_960H = 6,
    E_MIXER_DEVICE_MODE_2_720P = 7,
    E_MIXER_DEVICE_MODE_1_1080P = 8,
    E_MIXER_DEVICE_MODE_1_400M = 9,
    E_MIXER_DEVICE_MODE_MAX = 0xA
};

enum
{
    E_MIXER_CHN_PATH_NONE = '1',
    E_MIXER_CHN_PATH_VIF = 'B',
    E_MIXER_CHN_PATH_VIF_VPE = 'C',
    E_MIXER_CHN_PATH_VIF_VPE_DISP = 'D',
    E_MIXER_CHN_PATH_VIF_DIVP_DISP = 'E',
    E_MIXER_CHN_PATH_VIF_VPE_VENC = 'F',

    E_MIXER_CHN_PATH_DUMP = 'H',
    E_MIXER_CHN_PATH_VIF_DUMP = E_MIXER_CHN_PATH_DUMP,
    E_MIXER_CHN_PATH_VIF_VPE_DUMP = 'I',
    E_MIXER_CHN_PATH_VIF_VPE_VENC_DUMP = 'J',

    //E_MIXER_CHN_PATH_VIF_VPE_VENC = 'F',//4,5,j are bigger in ASCII
        E_MIXER_CHN_PATH_VIF_VPE_VENC_H264 = '4',
        E_MIXER_CHN_PATH_VIF_VPE_VENC_H265 = '5',
        E_MIXER_CHN_PATH_VIF_VPE_VENC_JPEG = 'j',
    E_MIXER_CHN_PATH_MAX
};

typedef struct MixerVideoChnInfo_s
{
    AD_VIDEO_IN_FORMAT_E eformat;
    MI_U16 u16Width;
    MI_U16 u16Height;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SYS_FieldType_e eCapSel;
    MI_U32 u32FrameRate[2];
    MI_VIF_FrameRate_e eFrameRate[2];
    MI_U8 u8Path[2];
    MI_BOOL bDump[2];
} MixerVideoChnInfo_t;

typedef struct MixerVideoDeviceInfo_s
{
    MI_U8 u8Mode;
} MixerVideoDeviceInfo_t;

MI_U32 mixer_chn_get_video_fps(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort);

int mixer_device_create_all(void);
int mixer_device_set_mode_all(char * ps8Mode);
MI_U8 mixer_device_get_mode(MI_VIF_DEV u32VifDev);
MI_VIF_WorkMode_e mixer_device_get_workmode(MI_VIF_DEV u32VifDev);



int mixer_chn_set_video_info_by_fmt(MI_VIF_CHN u32VifChn , AD_VIDEO_IN_FORMAT_E eFmt);
int mixer_chn_get_video_info(MI_VIF_CHN u32VifChn , MixerVideoChnInfo_t*  pstChnInfo);
int mixer_chn_dump_video_info(void);
MI_U8 mixer_chn_get_path(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort);
int mixer_chn_set_path(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort, MI_U8 u8Path);
int mixer_chn_set_framerate(MI_VIF_CHN u32VifChn ,MI_VIF_PORT u32VifPort, MI_VIF_FrameRate_e eFrameRate);

int mixer_chn_vif_create(MI_VIF_CHN u32VifChn,    MI_VIF_PORT u32VifPort, MixerVideoChnInfo_t* pstChnInfo);
int mixer_chn_vif_destroy(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort);
int mixer_chn_vif_start(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);
int mixer_chn_vif_stop(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);


int mixer_chn_vpe_create(MI_VPE_CHANNEL u32VpeChn, MI_SYS_WindowRect_t *pstCropWin);
int mixer_chn_vpe_destroy(MI_VPE_CHANNEL u32VpeChn, MI_VPE_PORT u32VpePort);
int mixer_chn_vpe_config_port(MI_VPE_CHANNEL u32VpeChn, MI_VPE_PORT u32VpePort , MI_SYS_WindowRect_t *pstDispWin , MI_SYS_PixelFormat_e  ePixelFormat);
int mixer_chn_vpe_start(MI_VPE_CHANNEL u32VpeChn);
int mixer_chn_vpe_stop(MI_VPE_CHANNEL u32VpeChn);

int mixer_chn_disp_config_port(MI_DISP_LAYER DispLayer, MI_DISP_INPUTPORT u32InputPort , MI_U16 u16Width , MI_U16 u16Height);

void mixer_path_set_display(MI_BOOL bDisplay);
MI_BOOL mixer_path_get_display(void);


int mixer_path_set_all(char * ps8Path);
int mixer_path_set_all_eframerate(char * ps8Path);

int mixer_path_create_all(void);
int mixer_path_destroy_all(void);

int mixer_path_create_vif(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);
int mixer_path_destroy_vif(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);

int mixer_path_create_vif_vpe(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);
int mixer_path_destroy_vif_vpe(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);

int mixer_path_create_vif_vpe_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);
int mixer_path_destroy_vif_vpe_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);

int mixer_path_create_vif_divp_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);
int mixer_path_destroy_vif_divp_disp(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);

int mixer_path_create_vif_vpe_venc(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);
int mixer_path_destroy_vif_vpe_venc(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort);

#endif // _MIXER_VIDEO_INFO_H_
