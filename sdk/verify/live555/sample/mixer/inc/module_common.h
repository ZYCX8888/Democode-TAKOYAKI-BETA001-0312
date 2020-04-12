/*
* module_common.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _MODULE_COMMON_
#define _MODULE_COMMON_

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "mid_common.h"
#include "mid_audio_type.h"
#include "mid_venc_type.h"
#if TARGET_CHIP_I5
#include "mi_isp_pretzel.h"
#include "mi_isp_pretzel_datatype.h"
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#endif
#include "mi_sensor.h"

#if TARGET_CHIP_I6E
#include "mi_vdisp_datatype.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern MI_DIVP_CHN gDivpChn4Vdf;

#define MIXER_MAX_MSG_BUFFER_SIZE           (MAX_VIDEO_NUMBER-2) *512+256

#define MIXER_DIVP_CHNID_FOR_VDF            gDivpChn4Vdf
#define MIXER_DIVP_CHNID_FOR_VDISP            0x02

#if TARGET_CHIP_I5
#define MIXER_DIVP_CHNID_FOR_DLA            0x03
#endif

#define MIXER_VDISP_DEVID_FOR_PIP           0x00
#define MIXER_VDISP_CHNID_FOR_PIP           0x00
#define MIXER_VDISP_PORTID_FOR_PIP          0x00
#if MIXER_SED_ENABLE
#define MIXER_DIVP_CHNID_FOR_SED             MIXER_DIVP_CHNID_FOR_VDF //vdf and sed will conflict,they can not use in same time.
#endif

#if TARGET_CHIP_I5
#define MIXER_CHANGE_SENSOR_RES_GATE        5000000
#define MIXER_VDISP_PORTID_FOR_VDISP        0x10
#elif TARGET_CHIP_I6
#define MIXER_CHANGE_SENSOR_RES_GATE        3000000
#define MIXER_VDISP_PORTID_FOR_VDISP        0x0
#elif TARGET_CHIP_I6E
#define MIXER_CHANGE_SENSOR_RES_GATE        3000000
#define MIXER_VDISP_CHNID_FOR_VDISP         VDISP_OVERLAYINPUTCHNID
#define MIXER_VDISP_PORTID_FOR_VDISP        0x0
#elif TARGET_CHIP_I6B0
#define MIXER_CHANGE_SENSOR_RES_GATE        3000000
#else
#define MIXER_VDISP_PORTID_FOR_VDISP       0x0
#endif

#define MIXER_LDC_MAX_VIEWNUM 4
#define MIXER_LDC_CFG_BIN_PATH_LENGTH 128

typedef enum _MixerCmdIdtype
{
    CMD_VIDEO       = 0x1000 ,
    CMD_AUDIO       = 0x2000 ,
    CMD_OSD         = 0x3000 ,
    CMD_ISP         = 0x4000 ,
    CMD_IE          = 0x5000 ,
    CMD_SYSTEM      = 0x6000 ,
    CMD_VIF         = 0x7000 ,
    CMD_VPE         = 0x8000 ,
    CMD_VENC        = 0x9000 ,
    CMD_SYS         = 0xA000 ,
    CMD_REC         = 0xB000,
#if MIXER_SED_ENABLE
    CMD_SED            = 0xC000,
#endif
    CMD_MOTOR  = 0xD000,
    CMD_UVC         = 0xE000 ,
    MIXER_CMD_TYPE  = 0xF000 ,
} MixerCmdType;

typedef enum _MixerCmdId
{
    CMD_VIDEO_OPEN                          = 0x01 | CMD_VIDEO,
    CMD_VIDEO_CLOSE                         = 0x02 | CMD_VIDEO,
    CMD_VIDEO_SET_ENCODER                   = 0x03 | CMD_VIDEO,
    CMD_VIDEO_SET_RESOLUTION                = 0x04 | CMD_VIDEO,
    CMD_VIDEO_SET_FRAMERATE                 = 0x05 | CMD_VIDEO,
    CMD_VIDEO_SET_SUPERFRAME                = 0x06 | CMD_VIDEO,
    CMD_VIDEO_SET_BITRATE_CONTROL           = 0x07 | CMD_VIDEO,
    CMD_VIDEO_SET_IMAGE_QUALITY             = 0x08 | CMD_VIDEO,
    CMD_VIDEO_SET_QP                        = 0x09 | CMD_VIDEO,
    CMD_VIDEO_SET_SAO                       = 0x0a | CMD_VIDEO,
    CMD_VIDEO_SET_GOP                       = 0x0b | CMD_VIDEO,
    CMD_VIDEO_START_FRAME_CAPTUR            = 0x0c | CMD_VIDEO,
    CMD_VIDEO_STOP_FRAME_CAPTUR             = 0x0d | CMD_VIDEO,
    CMD_VIDEO_SET_SUPERFRAME_MODE           = 0x0e | CMD_VIDEO,
    CMD_VIDEO_REQUEST_IDR                   = 0x11 | CMD_VIDEO,
    CMD_VIDEO_CROP                          = 0x12 | CMD_VIDEO,
    CMD_VIDEO_SET_BITRATE_CONTROL_CREATE    = 0x13 | CMD_VIDEO,
    CMD_VIDEO_SET_VIRTUAL_IINTERVAL         = 0x14 | CMD_VIDEO,

    CMD_VIDEO_SET_STATUS                    = 0x21 | CMD_VIDEO,
    CMD_VIDEO_SET_ROTATE                    = 0x22 | CMD_VIDEO,
    CMD_VIDEO_SET_SAVE_TASK_STATUS            = 0x23 | CMD_VIDEO,
    CMD_VIDEO_CHANGE_CODEC                     = 0x24 | CMD_VIDEO,
    CMD_VIDEO_SET_ROTATE_ENABLE             = 0x25 | CMD_VIDEO,
    CMD_VIDEO_SET_CROP_STATUS               = 0x26 | CMD_VIDEO,
    CMD_VIDEO_CHANGE_LTR_PARAM              = 0x27 | CMD_VIDEO,
    CMD_VIDEO_SET_ROICFG                    = 0x28 | CMD_VIDEO,
    CMD_VIDEO_SET_3DNR                      = 0x2A | CMD_VIDEO,
    CMD_VIDEO_SET_MIRROR_FLIP               = 0x2B | CMD_VIDEO,
    CMD_VIDEO_SWITCH_HDR_LINAER_MODE        = 0x2C | CMD_VIDEO,
    CMD_VIDEO_MMA_ALLOC                     = 0x2D | CMD_VIDEO,
    CMD_VIDEO_MMA_FREE                      = 0x2E | CMD_VIDEO,
    CMD_VIDEO_SET_DIVP_BIND_TYPE    = 0x2F | CMD_VIDEO,

    CMD_VIDEO_GET_ENCODER                   = 0x103 | CMD_VIDEO,
    CMD_VIDEO_GET_RESOLUTION                = 0x104 | CMD_VIDEO,
    CMD_VIDEO_GET_FRAMERATE                 = 0x105 | CMD_VIDEO,
    CMD_VIDEO_GET_BITRATE                   = 0x106 | CMD_VIDEO,
    CMD_VIDEO_GET_BITRATE_CONTROL           = 0x107 | CMD_VIDEO,
    CMD_VIDEO_GET_IMAGE_QUALITY             = 0x108 | CMD_VIDEO,
    CMD_VIDEO_GET_QP                        = 0x109 | CMD_VIDEO,
    CMD_VIDEO_GET_SAO                       = 0x10a | CMD_VIDEO,
    CMD_VIDEO_GET_GOP                       = 0x10b | CMD_VIDEO,

    CMD_VIDEO_GET_SENSOR_RESOLUTION         = 0x200 | CMD_VIDEO,
    CMD_VIDEO_SET_SENSOR_RESOLUTION         = 0x201 | CMD_VIDEO,
    CMD_VIDEO_SET_SENSOR_FRAMERATE          = 0x202 | CMD_VIDEO,

    CMD_AUDIO_OPEN                          = 0x01 | CMD_AUDIO,
    CMD_AUDIO_CLOSE                         = 0x02 | CMD_AUDIO,
    CMD_AUDIO_SET_SAMPLES                   = 0x03 | CMD_AUDIO,
    CMD_AUDIO_SET_BITWIDE                   = 0x04 | CMD_AUDIO,
    CMD_AUDIO_SET_ENCODER                   = 0x05 | CMD_AUDIO,
    CMD_AUDIO_PLAY_MEDIA                    = 0x06 | CMD_AUDIO,
    CMD_AUDIO_SET_AED_SENSITIVITY           = 0x07 | CMD_AUDIO,
    CMD_AUDIO_SET_AED_OPERATIONGPOINT       = 0x08 | CMD_AUDIO,
    CMD_AUDIO_STOPPLAY                      = 0x09 | CMD_AUDIO,
    CMD_AUDIO_SET_AIVOLUME                  = 0x0a | CMD_AUDIO,
    CMD_AUDIO_SET_AOVOLUME                  = 0x0b | CMD_AUDIO,
    CDM_AUDIO_SETVQEMODULE                  = 0x0c | CMD_AUDIO,
    CMD_AUDIO_GET_SAMPLES                   = 0x103 | CMD_AUDIO,
    CMD_AUDIO_GET_BITWIDE                   = 0x104 | CMD_AUDIO,
    CMD_AUDIO_GET_ENCODER                   = 0x105 | CMD_AUDIO,
    CMD_AUDIO_VD_OPEN                       = 0x200 | CMD_AUDIO,
    CMD_AUDIO_VD_CLOSE                      = 0x201 | CMD_AUDIO,


    CMD_OSD_OPEN                            = 0x01 | CMD_OSD,
    CMD_OSD_CLOSE                           = 0x02 | CMD_OSD,
    CMD_OSD_SET_OD                          = 0x03 | CMD_OSD,
    CMD_OSD_GET_OD                          = 0x103 | CMD_OSD,
    CMD_OSD_OPEN_DISPLAY_VIDEO_INFO         = 0x04 | CMD_OSD,
    CMD_OSD_CLOSE_DISPLAY_VIDEO_INFO        = 0x05 | CMD_OSD,
    CMD_OSD_FLICKER                         = 0x06 | CMD_OSD,
    CMD_OSD_IE_OPEN                         = 0x07 | CMD_OSD,
    CMD_OSD_IE_CLOSE                        = 0x08 | CMD_OSD,
    CMD_OSD_MASK_OPEN                       = 0x09 | CMD_OSD,
    CMD_OSD_MASK_CLOSE                      = 0x0A | CMD_OSD,
    CMD_OSD_COLOR_INVERSE_OPEN              = 0x0D | CMD_OSD,
    CMD_OSD_COLOR_INVERSE_CLOSE             = 0x0E | CMD_OSD,
    CMD_OSD_FULL_OPEN                       = 0x0F | CMD_OSD,
    CMD_OSD_PRIVATEMASK_OPEN                = 0x10 | CMD_OSD,
    CMD_OSD_PRIVATEMASK_CLOSE               = 0x11 | CMD_OSD,
    CMD_OSD_PRIVATEMASK_SET                 = 0x12 | CMD_OSD,
    CMD_OSD_PRIVATEMASK_SET_FROM_VI         = 0x13 | CMD_OSD,
    CMD_OSD_RESET_RESOLUTION                = 0x14 | CMD_OSD,
    CMD_OSD_RESTART_RESOLUTION              = 0x15 | CMD_OSD,
    CMD_OSD_PRE_INIT              = 0x16 | CMD_OSD,


    CMD_ISP_OPEN                            = 0x01 | CMD_ISP,
    CMD_ISP_CLOSE                           = 0x02 | CMD_ISP,
    CMD_ISP_SET_SATURATION                  = 0x03 | CMD_ISP,
    CMD_ISP_SET_SHARPENESS                  = 0x04 | CMD_ISP,
    CMD_ISP_SET_LIGHTSENSITIVITY            = 0x05 | CMD_ISP,
    CMD_ISP_SET_SCENE                       = 0x06 | CMD_ISP,
    CMD_ISP_SET_EXPOSURE_MODE               = 0x07 | CMD_ISP,
    CMD_ISP_SET_IRCUT                       = 0x08 | CMD_ISP,
    CMD_ISP_SET_ROTATION                    = 0x09 | CMD_ISP,
    CMD_ISP_SET_AUTO_WB                     = 0x0a | CMD_ISP,
    CMD_ISP_SET_NIGHT_OPTIMIZE              = 0x0b | CMD_ISP,
    CMD_ISP_SET_NIGHT_SELF_ADAPTION         = 0x0c | CMD_ISP,
    CMD_ISP_SET_WIDE_DYNAMIC                = 0x0d | CMD_ISP,
    CMD_ISP_SET_DISPLAY_MIRROR              = 0x0e | CMD_ISP,
    CMD_ISP_SET_DISPLAY_REVERSAL            = 0x0f | CMD_ISP,
    CMD_ISP_SET_STAR                        = 0x10 | CMD_ISP,
    CMD_ISP_SET_DEFOGGING                   = 0x11 | CMD_ISP,
    CMD_ISP_SET_CONTRAST                    = 0x12 | CMD_ISP,
    CMD_ISP_SET_BRIGHTNESS                  = 0x13 | CMD_ISP,

    CMD_ISP_GET_SATURATION                  = 0x103 | CMD_ISP,
    CMD_ISP_GET_SHARPENESS                  = 0x104 | CMD_ISP,
    CMD_ISP_GET_LIGHTSENSITIVITY            = 0x105 | CMD_ISP,
    CMD_ISP_GET_SCENE                       = 0x106 | CMD_ISP,
    CMD_ISP_GET_EXPOSURE_MODE               = 0x107 | CMD_ISP,
    CMD_ISP_GET_IRCUT                       = 0x108 | CMD_ISP,
    CMD_ISP_GET_ROTATION                    = 0x109 | CMD_ISP,
    CMD_ISP_GET_AUTO_WB                     = 0x10a | CMD_ISP,
    CMD_ISP_GET_NIGHT_OPTIMIZE              = 0x10b | CMD_ISP,
    CMD_ISP_GET_NIGHT_SELF_ADAPTION         = 0x10c | CMD_ISP,
    CMD_ISP_GET_WIDE_DYNAMIC                = 0x10d | CMD_ISP,
    CMD_ISP_GET_DISPLAY_MIRROR              = 0x10e | CMD_ISP,
    CMD_ISP_GET_DISPLAY_REVERSAL            = 0x10f | CMD_ISP,
    CMD_ISP_GET_STAR                        = 0x110 | CMD_ISP,
    CMD_ISP_GET_DEFOGGING                   = 0x111 | CMD_ISP,
    CMD_ISP_GET_CONTRAST                    = 0x112 | CMD_ISP,
    CMD_ISP_GET_BRIGHTNESS                  = 0x113 | CMD_ISP,
    CMD_ISP_GET_INPUTCROP                   = 0x114 | CMD_ISP,
    CMD_ISP_GET_AFWIN                       = 0x115 | CMD_ISP,
    CMD_ISP_SET_AFWIN                       = 0x116 | CMD_ISP,
    CMD_ISP_QUERY_AFINFO                    = 0x117 | CMD_ISP,
    CMD_ISP_LOAD_CALI_DATA                  = 0x118 | CMD_ISP,
    CMD_ISP_LOAD_CMDBIN                     = 0x119 | CMD_ISP,
    CMD_ISP_OPEN_CUS3A                      = 0x11a | CMD_ISP,
    CMD_ISP_CLOSE_CUS3A                     = 0x11b | CMD_ISP,
    CMD_ISP_GET_AESTATS                     = 0x11c | CMD_ISP,
    CMD_ISP_SET_AEWINBLK                    = 0x11d | CMD_ISP,
    CMD_ISP_GET_AEHIST                      = 0x11e | CMD_ISP,
    CMD_ISP_GET_AWBSTATS                    = 0x11f | CMD_ISP,
    CMD_ISP_SET_AWBSAMPLING                 = 0x120 | CMD_ISP,
    CMD_ISP_GET_AFSTATS                     = 0x121 | CMD_ISP,
    //CMD_ISP_GET_AFWIN                       = 0x122 | CMD_ISP,
    //CMD_ISP_SET_AFWIN                       = 0x123 | CMD_ISP,
    CMD_ISP_GET_AFFILTER                    = 0x124 | CMD_ISP,
    CMD_ISP_SET_AFFILTER                    = 0x125 | CMD_ISP,
    CMD_ISP_GET_AFFILTERSQ                  = 0x126 | CMD_ISP,
    CMD_ISP_SET_AFFILTERSQ                  = 0x127 | CMD_ISP,
    CMD_ISP_GET_AFROIMODE                   = 0x128 | CMD_ISP,
    CMD_ISP_SET_AFROIMODE                   = 0x129 | CMD_ISP,
    CMD_ISP_GET_IMAGERES                    = 0x12a | CMD_ISP,
    CMD_ISP_GET_ISPIMAGE                    = 0x12b | CMD_ISP,
    CMD_ISP_TEST_CUS3A_API                  = 0x12c | CMD_ISP,
    CMD_ISP_CUS3A_INTERFACE_UT              = 0x12d | CMD_ISP,

    CMD_IE_OPEN                             = 0x01 | CMD_IE,
    CMD_IE_CLOSE                            = 0x02 | CMD_IE,
    CMD_IE_FD_OPEN                          = 0x03 | CMD_IE,
    CMD_IE_FD_CLOSE                         = 0x04 | CMD_IE,
    CMD_IE_MD_OPEN                          = 0x05 | CMD_IE,
    CMD_IE_MD_CLOSE                         = 0x06 | CMD_IE,
    CMD_IE_OD_OPEN                          = 0x07 | CMD_IE,
    CMD_IE_OD_CLOSE                         = 0x08 | CMD_IE,
    CMD_IE_SET_FRAME_INTERVAL               = 0x09 | CMD_IE,
    CMD_IE_SET_YUV_SAVE                     = 0x0a | CMD_IE,
    CMD_IE_VG_OPEN                          = 0x0b | CMD_IE,
    CMD_IE_VG_CLOSE                         = 0x0c | CMD_IE,
    CMD_IE_MD_CHANGE                        = 0x0d | CMD_IE,
    CMD_IE_FD_CHANGE                        = 0x0e | CMD_IE,
    CMD_IE_HD_OPEN                          = 0x10 | CMD_IE,
    CMD_IE_HD_CLOSE                         = 0x11 | CMD_IE,
    CMD_IE_HD_ACTION                        = 0x12 | CMD_IE,
    CMD_IE_HCHD_OPEN                        = 0x15 | CMD_IE,
    CMD_IE_HCHD_CLOSE                       = 0x16 | CMD_IE,
    CMD_IE_DLA_OPEN                         = 0x17 | CMD_IE,
    CMD_IE_DLA_CLOSE                        = 0x18 | CMD_IE,
    CMD_IE_DLA_SETPARAM                     = 0x19 | CMD_IE,
    CMD_IE_DLA_SETINITINFO                  = 0x20 | CMD_IE,

    CMD_SYSTEM_WATCHDOG_OPEN               = 0x01 | CMD_SYSTEM,
    CMD_SYSTEM_WATCHDOG_CLOSE              = 0x02 | CMD_SYSTEM,
    CMD_SYSTEM_IQSERVER_OPEN               = 0x03 | CMD_SYSTEM,
    CMD_SYSTEM_IQSERVER_CLOSE              = 0x04 | CMD_SYSTEM,
    CMD_SYSTEM_IRCUT_OPEN                  = 0x05 | CMD_SYSTEM,
    CMD_SYSTEM_IRCUT_CLOSE                 = 0x06 | CMD_SYSTEM,
    CMD_SYSTEM_ONVIF_OPEN                  = 0x07 | CMD_SYSTEM,
    CMD_SYSTEM_ONVIF_CLOSE                 = 0x08 | CMD_SYSTEM,
    CMD_SYSTEM_LIVE555_OPEN                = 0x09 | CMD_SYSTEM,
    CMD_SYSTEM_LIVE555_CLOSE               = 0x0a | CMD_SYSTEM,
    CMD_SYSTEM_LIVE555_SET_FRAMERATE       = 0x0b | CMD_SYSTEM,
    CMD_SYSTEM_LIVE555_SET_ENCODETYPE      = 0x0c | CMD_SYSTEM,
    CMD_SYSTEM_CUS3A_OPEN                  = 0x0d | CMD_SYSTEM,
    CMD_SYSTEM_CUS3A_CLOSE                 = 0x0e | CMD_SYSTEM,
    CMD_SYSTEM_LIVE555_STOP_SUBMEDIASESSION= 0x0f | CMD_SYSTEM,
    CMD_SYSTEM_EXIT_QUEUE                  = 0x10 | CMD_SYSTEM,

    CMD_SYSTEM_INIT                        = 0x11 | CMD_SYSTEM,
    CMD_SYSTEM_UNINIT                      = 0x12 | CMD_SYSTEM,
    CMD_SYSTEM_CORE_BACKTRACE              = 0x13 | CMD_SYSTEM,
    CMD_SYSTEM_IRCUT_WHITE                 = 0x14 | CMD_SYSTEM,
    CMD_SYSTEM_IRCUT_BLACK                 = 0x15 | CMD_SYSTEM,
    CMD_SYSTEM_SETCHNOUTPUTPORTDEPTH       = 0x16 | CMD_SYSTEM,

    CMD_SYSTEM_PWM_MOTO_GROUP              = 0x17 | CMD_SYSTEM,
    CMD_SYSTEM_PWM_MOTO_PARAM              = 0x18 | CMD_SYSTEM,
    CMD_SYSTEM_PWM_MOTO_ENABLE             = 0x19 | CMD_SYSTEM,
    CMD_SYSTEM_PWM_MOTO_HSTOP              = 0x1a | CMD_SYSTEM,

    CMD_UVC_INIT                           = 0x01 | CMD_UVC,
    CMD_UVC_CLOSE                          = 0x02 | CMD_UVC,

    CMD_VIF_INIT                           = 0x01 | CMD_VIF,
    CMD_VIF_UNINIT                         = 0x02 | CMD_VIF,
    CMD_VIF_ENABLEDEV                      = 0x03 | CMD_VIF,
    CMD_VIF_DISABLEDEV                     = 0x04 | CMD_VIF,
    CMD_VIF_SETCHNPORTATTR                 = 0x05 | CMD_VIF,
    CMD_VIF_GETCHNPORTATTR                 = 0x06 | CMD_VIF,
    CMD_VIF_ENABLECHNPORT                  = 0x07 | CMD_VIF,
    CMD_VIF_DISABLECHNPORT                 = 0x08 | CMD_VIF,
    CMD_VIF_QUERY                          = 0x09 | CMD_VIF,

    CMD_VPE_CREATECHANNEL                  = 0x01 | CMD_VPE,
    CMD_VPE_DESTROYCHANNEL                 = 0x02 | CMD_VPE,
    CMD_VPE_SETCHANNELATTR                 = 0x03 | CMD_VPE,
    CMD_VPE_GETCHANNELATTR                 = 0x04 | CMD_VPE,
    CMD_VPE_STARTCHANNEL                   = 0x05 | CMD_VPE,
    CMD_VPE_STOPCHANNEL                    = 0x06 | CMD_VPE,
    CMD_VPE_SETCHANNELPARAM                = 0x07 | CMD_VPE,
    CMD_VPE_GETCHANNELPARAM                = 0x08 | CMD_VPE,
    CMD_VPE_SETCHANNELCROP                 = 0x09 | CMD_VPE,
    CMD_VPE_GETCHANNELCROP                 = 0x0A | CMD_VPE,
    CMD_VPE_GETCHANNELREGIONLUMA           = 0x0B | CMD_VPE,
    CMD_VPE_SETCHANNELROTATION             = 0x0C | CMD_VPE,
    CMD_VPE_GETCHANNELROTATION             = 0x0D | CMD_VPE,
    CMD_VPE_ENABLEPORT                     = 0x0E | CMD_VPE,
    CMD_VPE_DISABLEPORT                    = 0x0F | CMD_VPE,
    CMD_VPE_SETPORTMODE                    = 0x10 | CMD_VPE,
    CMD_VPE_GETPORTMODE                    = 0x11 | CMD_VPE,

    CMD_VENC_GETCHNDEVID                   = 0x01 | CMD_VENC,
    CMD_VENC_SETMODPARAM                   = 0x02 | CMD_VENC,
    CMD_VENC_GETMODPARAM                   = 0x03 | CMD_VENC,
    CMD_VENC_CREATECHN                     = 0x04 | CMD_VENC,
    CMD_VENC_DESTROYCHN                    = 0x05 | CMD_VENC,
    CMD_VENC_RESETCHN                      = 0x06 | CMD_VENC,
    CMD_VENC_STARTRECVPIC                  = 0x07 | CMD_VENC,
    CMD_VENC_STARTRECVPICEX                = 0x08 | CMD_VENC,
    CMD_VENC_STOPTRECVPIC                  = 0x09 | CMD_VENC,
    CMD_VENC_QUERY                         = 0x0A | CMD_VENC,
    CMD_VENC_SETCHNATTR                    = 0x0B | CMD_VENC,
    CMD_VENC_GETCHNATTR                    = 0x0C | CMD_VENC,
    CMD_VENC_GETSTREAM                     = 0x0D | CMD_VENC,
    CMD_VENC_RELEASESTREAM                 = 0x0E | CMD_VENC,
    CMD_VENC_INSERTUSERDATA                = 0x0F | CMD_VENC,
    CMD_VENC_SETMAXSTREAMCNT               = 0x10 | CMD_VENC,
    CMD_VENC_GETMAXSTREAMCNT               = 0x11 | CMD_VENC,
    CMD_VENC_REQUESTIDR                    = 0x12 | CMD_VENC,
    CMD_VENC_ENABLEIDR                     = 0x13 | CMD_VENC,
    CMD_VENC_SETH264IDRPICID               = 0x14 | CMD_VENC,
    CMD_VENC_GETH264IDRPICID               = 0x15 | CMD_VENC,
    CMD_VENC_GETFD                         = 0x16 | CMD_VENC,
    CMD_VENC_CLOSEFD                       = 0x17 | CMD_VENC,
    CMD_VENC_SETROICFG                     = 0x18 | CMD_VENC,
    CMD_VENC_GETROICFG                     = 0x19 | CMD_VENC,
    CMD_VENC_SETROIBGFRAMERATE             = 0x1A | CMD_VENC,
    CMD_VENC_GETROIBGFRAMERATE             = 0x1B | CMD_VENC,
    CMD_VENC_SETH264SLICESPLIT             = 0x1C | CMD_VENC,
    CMD_VENC_GETH264SLICESPLIT             = 0x1D | CMD_VENC,
    CMD_VENC_SETH264INTERPRED              = 0x1E | CMD_VENC,
    CMD_VENC_GETH264INTERPRED              = 0x1F | CMD_VENC,

    CMD_RECORD_ENABLE                      = 0x01 | CMD_REC,
    CMD_RECORD_DISABLE                     = 0x02 | CMD_REC,
    CMD_RECORD_SETMODE                     = 0x03 | CMD_REC,
    CMD_RECORD_GETMODE                     = 0x04 | CMD_REC,
#if MIXER_SED_ENABLE
    CMD_SED_OPEN                            = 0x01 | CMD_SED,
    CMD_SED_CLOSE                            = 0x02 | CMD_SED,
#endif
    CMD_MOTOR_INIT   = 0x01 | CMD_MOTOR,
    CMD_MOTOR_UNINIT   = 0x02 | CMD_MOTOR,
    CMD_MOTOR_CONTROL  = 0x03 | CMD_MOTOR,
    CMD_MOTOR_DelayMs  = 0x04 | CMD_MOTOR,
    CMD_MOTOR_GetPos  = 0x05 | CMD_MOTOR,
} MixerCmdId;


typedef struct VideoParam_s
{
    MixerCmdId id;
    MI_SNR_PAD_ID_e sensorPad;
    MI_U32 u32SnrRes;
    MI_U32 u32SnrFps;
    MI_U32 u32VideoWidth;
    MI_U32 u32VideoHeight;
    MI_VIF_HDRType_e eHdrType;
    MI_VPE_3DNR_Level_e e3DNRLevel;
} VideoParam_t;

int isp_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int audio_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int video_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int osd_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int ie_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int vif_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int vpe_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int venc_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int system_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int rec_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen);

int uvc_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen);
int mixer_send_cmd(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen);
int mixer_return_cmd_result(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen);
int mixer_wait_cmdresult(MixerCmdId cmdId, void *param, MI_U32 paramLen);
#if MIXER_SED_ENABLE
int sed_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen);
#endif
int motor_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen);

class FpsCalculator
{
private:
    MI_U64 TStart;
    unsigned int count;
    unsigned int interval;
    float fps;
    float fps_avg;
private:

    MI_U64 getTimeUs()
    {
        struct timeval tv;
        MI_U64 ret;
        gettimeofday(&tv, NULL);
        ret = (MI_U64)tv.tv_sec * 1000000 + tv.tv_usec;
        return ret;
    }

public:
    FpsCalculator()
        : TStart(0),
          count(0),
          interval(0),
          fps(0),
          fps_avg(0)
    {}
    virtual ~FpsCalculator() {}
    void IncFrmCnt()
    {
        if(++count >= 30)
        {
            MI_U64 TEnd = getTimeUs();
            //LOG_INFO(DBG_MODULE_SAMPLE,"TEnd=%d\r\n",(int)TEnd);
            fps = (float)count * 1000000 / (TEnd - TStart);

            if(fps >= 0.5)
            {
                if(interval == 0)
                    fps_avg = 0;

                interval++;
                fps_avg = fps_avg + (fps - fps_avg) / interval;
            }

            count = 0;
        }

        if(count == 0)
        {
            TStart = getTimeUs();
        }
    }
    float GetFps()
    {
        return fps;
    }
    float GetFps_Avg()
    {
        return fps_avg;
    }
};


class VideoInfoCalculator
{
private:
    MI_U64 TStart;
    MI_U32 totalSize;
    MI_U32 gopCount;
    MI_U32 bitrate;
    float  fps;
    MI_U32 gop;

    MI_U64 getTimeUs()
    {
        struct timeval tv;
        MI_U64 ret;
        gettimeofday(&tv, NULL);
        ret = (MI_U64)tv.tv_sec * 1000000 + tv.tv_usec;
        return ret;
    }

public:
    VideoInfoCalculator()
        : TStart(0),
          totalSize(0),
          gopCount(0),
          bitrate(0),
          fps(0),
          gop(0)
    {}
    virtual ~VideoInfoCalculator() {}
    void CalculatorframeInfo()
    {
        MI_U64 TEnd = getTimeUs();
        MI_U64 tmp=0x0;
        bitrate = (TEnd == TStart ? 0 : (MI_U32)((MI_U64)totalSize * 8 * 1000000L / (TEnd - TStart)));
        tmp = (TEnd == TStart ? 0 : ((MI_U64)gopCount * 1000000000L / (TEnd - TStart)));
        fps = ((float)((MI_U32)tmp)) /1000.0;
        gopCount  = 0;
        totalSize = 0;
        TStart = getTimeUs();
    }

    void CalculatorJPGframeInfo(MI_U64 curBufferTime)
    {
        MI_U64 TEnd = curBufferTime;
        MI_U64 tmp = 0x0;
        bitrate = (TEnd - TStart)==0? 0 : (MI_U32)((MI_U64)totalSize * 8 * 1000000L / (TEnd - TStart));
        tmp = (TEnd - TStart)==0? 0 : ((MI_U64)gopCount * 1000000000L / (TEnd - TStart));
        fps = ((float)((MI_U32)tmp)) /1000.0;
        gopCount  = 0;
        totalSize = 0;
        TStart = curBufferTime;
    }
    void IncFrmCnt(int frameSize, int frameType, Mixer_EncoderType_e type,MI_U64 curBufferTime)
    {
        if(VE_JPG == type || VE_MJPEG == type)
        {
            gop = 1;
            CalculatorJPGframeInfo(curBufferTime);
        }
        else if((frameType == E_MI_VENC_H264E_NALU_ISLICE) && VE_AVC == type)
        {
            gop = gopCount;
            CalculatorframeInfo();
        }
        else if((frameType == E_MI_VENC_H265E_NALU_ISLICE) && VE_H265 == type)
        {
            gop = gopCount;
            CalculatorframeInfo();
        }
        gopCount++;
        totalSize += frameSize;
    }

    MI_U32 GetBitrate()
    {
        return bitrate;
    }

    float GetFps()
    {
        return fps;
    }

    MI_U32 GetGop()
    {
        return gop;
    }
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //_MODULE_COMMON_
