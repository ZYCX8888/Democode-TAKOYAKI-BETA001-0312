/*
* main.cpp- Sigmastar
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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <error.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mid_common.h"

//#include "sample_comm.h"
#include "module_common.h"
#include "module_config.h"
#include "mid_audio_type.h"
#include "mid_AudioEncoder.h"
#include "module_cus3a.h"

#include "mid_vif.h"
#include "mid_utils.h"
#include "mi_venc.h"
#include "mi_sys.h"
#include "mi_ai.h"

#include "mid_VideoEncoder.h"
#include "mid_md.h"

#include "mid_system_config.h"

#if    TARGET_CHIP_I6E
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6B0
#include "mi_isp_datatype.h"
#endif

#include "PacketModule.h"
#include "Module_Console.h"
#include "Message_queue.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if TARGET_CHIP_I5
#define MIXER_FONT_PATH       "/config"
#define DEF_AUDIOPLAY_FILE    "/config/xiaopingguo.wav"
#elif TARGET_CHIP_I6
#define MIXER_FONT_PATH       "/customer"
#define DEF_AUDIOPLAY_FILE    "/customer/xiaopingguo.wav"
#elif TARGET_CHIP_I6E
#define MIXER_FONT_PATH       "/customer"
#define DEF_AUDIOPLAY_FILE    "/customer/xiaopingguo.wav"
#elif TARGET_CHIP_I6B0
#define MIXER_FONT_PATH       "/customer"
#define DEF_AUDIOPLAY_FILE    "/customer/xiaopingguo.wav"

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#if TARGET_CHIP_I5
const char *g_ChipType = "i5";    // i5
#elif TARGET_CHIP_I6
const char *g_ChipType = "i6";    // i6
#elif    TARGET_CHIP_I6E
const char *g_ChipType = "i6e";    // i6e
#elif    TARGET_CHIP_I6B0
const char *g_ChipType = "i6b0";    // i6b0
#endif

extern int g_IE_Open;
extern BOOL g_ShowFrameInterval;

MI_S32 gDebug_VideoStreamLog = FALSE;
MI_S32 gDebug_AudioStreamLog = FALSE;
MI_S32 gDebug_osdDrawRect = FALSE;
MI_S32 gDebug_saveAudioData[MIXER_AI_MAX_NUMBER] = {0};
MI_S32 gDebug_OsdTest = FALSE;
MI_S32 gDebug_osdColorInverse = FALSE;
MI_S32 g_s32OsdHandleCnt = 0; //create one osd per video
MI_S32 gDebug_ircutType = 0;
MI_S32 gDebug_osdPrivateMask = FALSE;

MI_S32 g_displayOsd = FALSE;
MI_S32 g_displayVideoInfo = FALSE;
MI_S32 g_SuperFrameEnable = 0;
#if MIXER_SED_ENABLE
static MI_S32 g_EnableSed = 0;
static MI_S32 g_SedType = 0;
static MI_S32 g_SedDebug = 0;
#endif

MI_S32 g_openUVCDevice = FALSE;

MI_S32 g_ircut = 0;
MI_S32 g_openIQServer = FALSE;
MI_S32 g_enableCus3A = FALSE;
MI_S32 g_CaliItem[SS_CALI_ITEM_MAX] = {0};
MI_U8 g_CaliDBPath[SS_CALI_ITEM_MAX][128] = { {0}, {0}, {0}, {0}, {0} };

MI_S8 g_ISPBinFilePath[256] = {0};
MI_S8 g_s8OsdFontPath[128] = {0};
MI_S32 g_IspKey = 0;
MI_S32 g_ieWidth  = RAW_W;
MI_S32 g_ieHeight = RAW_H;
#if TARGET_CHIP_I5
MI_S32 g_jpegViChn = 3;
MI_S32 g_ieVpePort = 2;
#elif TARGET_CHIP_I6
MI_S32 g_jpegViChn = 0;
MI_S32 g_ieVpePort = 2;
#elif    TARGET_CHIP_I6E
MI_S32 g_jpegViChn = 1;
MI_S32 g_ieVpePort = 2;
#elif    TARGET_CHIP_I6B0
MI_S32 g_jpegViChn = 1;
MI_S32 g_ieVpePort = 3;

#endif


MI_S32 g_openOnvif = FALSE;

BOOL g_bSetBitrate = FALSE;
BOOL g_bCusAEenable = FALSE;
BOOL g_bBiLinear  = FALSE;

//BOOL g_bAudioInWNR = FALSE;
//BOOL g_bAudioOutWNR = FALSE;

MI_S32 g_s32OsdFlicker = FALSE;

MI_S32 g_bUseTimestampAudio = FALSE;
MI_S32 g_bUseTimestampVideo = FALSE;
MI_S32 g_maskOsd = FALSE;

MI_S32 g_maskNum[MAX_VIDEO_NUMBER];
MI_S32 g_roi_num = 0;
MI_U32 g_roiParam[90];
MI_S8 g_uvcParam = 0;
MI_S32 g_modeParam = 0;
MI_S32 g_fdParam = 0;
MI_S32 g_mdParam = 0;
MI_S32 g_odParam = 0;
MI_S32 g_vgParam = 0;
MI_S32 g_hchdParam = 0;
MI_S32 g_dlaParam = 0;
MI_S32 g_ieLogParam = 0;
MI_S32 g_ieFrameInterval = 5;
MI_S32 g_ieOsdDisplayDisable = TRUE;
MI_S32 g_EnablePIP = 0;
MI_U32 g_rotation = 0; //0x00000000:disable; 0xFFxxAAAA:enable(AAAA=0:rotate0; 90:rotate90; 180:rotate180; 270:rotate270)

MI_S32 g_remoteIQServer = FALSE;

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
MI_S32 g_fixMMA = FALSE;
#endif

MI_U32 g_videoNumber = 2;
MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
MI_AI_VqeConfig_t g_stAiVqeCfg;
MI_AO_VqeConfig_t g_stAoVqeCfg;
//int  g_audioInEnableG7xxExt = 0;
int  g_audioOutEnableG7xxExt = 0;
MixerAudioInParam g_audioInParam[MIXER_AI_MAX_NUMBER];
MixerAudioOutParam g_audioOutParam[MIXER_AO_MAX_NUMBER];

int pwm_groud[11] = {0};
extern MI_S32 g_MD_InitDone;
#if MIXER_FD_ENABLE
extern MI_S32 g_FD_InitDone;
#endif


MI_U32 g_videoMaxWidth = 1920;
MI_U32 g_videoMaxHeight = 1080;

typedef struct _RecordStatus{
    MI_S32  ch;
    MI_S32 status;
}RecordStatus;
static RecordStatus  tmpRecStatus[MAX_VIDEO_NUMBER];

extern int MasktoRect(MI_VENC_CHN VeChn, MI_S32 s32MaskNum, MI_SYS_WindowRect_t *pstRect);
extern void mixer_osdInfoLinkListInit(void);

#if TARGET_CHIP_I6E
extern void mixerSetLdcStatus(MI_BOOL bEnable);
MI_BOOL mixerSetLdcBinPath(char *path);
#endif

void mixer_startup_display_help()
{
    printf("\n");
    printf("Usage: mixer [-n number] [-r resolution] [-b bitrate] [-t openFrameTimeStamp] [-f encoder frameRate] [-s sensor frameRate] [-p disposable] [-v videoEncoderType] [-o osd] [-q iqserver]\n\n");
    printf("\t-h Displays this help\n");
    printf("\t-a audio input/output  : Defalut g711a.open audio output with babycry and lound detection\n");
    printf("\t-A                     : [1_1_0_1_30_1_1] [AEC]_[AGC]_[GainInit(-80,0])]_[ANR]_[NrSmoothLevel(0,30)]_[HPF]_[EQ]\n");
    printf("\t-b bitrate             : bitrate. Default 2000000 support unit in human readable format k or K, m or M  (e.g., 100K 2M 6m) \n");
    printf("\t-B module buffer count : vif/vpe/venc/divp buffer count.  [0]:StreamIdx; [1,2]:vif usrDepth/BufCntQuota; [3,4]:vpe ...; [5,6]:venc ...; [7,8]:divp ...\n");
    printf("\t-C filepath            : load calibration data. default path is \"/data/cfg/calibration.db\"\n");
    printf("\t-e                     : [filepath] play audio file and open aec function\n");
    printf("\t-E                     : set bitrate encodeType, [0] Cbr [1] Vbr [2] Fixqp [3] Abr, Default Cbr\n");
    printf("\t-f video framerate     : set video StreamIdx framerate,for example  -f 20_20_20  Default 25\n");
    printf("\t-F encoder support  PixelFormat : select[0] is YUYV422 [11] is VN12(YUV420). Default 11\n");
    printf("\t-g gop                 : gop,  for example -g 50_50_50 \n");
    printf("\t-G virtual I interval  : virtualIInterval,  for example -G 10_10_10 \n");
    printf("\t-i filepath            : open ISP cusAE;  open select [1] close [0], for example -i 1_1_1, default close,\n");
    printf("\t-I isp cmd bin file    : key_filepath. For example -I 1_ispcmd.bin\n");
    printf("\t-j ircut type          : ircut control, [0] none , [1] auto switch , Default [0] \n");
    printf("\t-J roi config          : set roi , [venChn]_[roiIndex 0~7]_[Enable]_[bAbsQp[Absolutely qp:1,Relative qp:0] ]_[QP]_[RectLeft]_[Recttop]_[RectWidth]_[RectHeight]\n");
    printf("\t                         for example: -J 0_0_1_1_35_200_200_400_400 \n");
    printf("\t-k time stamp          : [0] use buffer time stamp video, [1] use buffer time stamp audio.\n");
    printf("\t-K mask                : enable Mask , use [0-4] , for example: -K 4_4 \n");
    printf("\t-l enable ldc          : enable ldc and set ldc config or bin file path, for example: -l ./Pattern/new_AMTK_FHD/cfg/new_AMTK_1P_cm_fhd.cfg\n");
    printf("\t                       : [1_420_0_1080_1080] crop 1080*1080\n");
    printf("\t-L set bind mode       : [0] framemode [1] realtime [2] ringmode [3]half ringmode, Default 0, doesn`t work I5!!\n");
    printf("\t-m angle               : rotation. 0, 90, 180, 270\n");
    printf("\t-M share mem size      : share mem size. unit in human readable format k or K, m or M   (e.g., 100K 2M 6m) \n");
    printf("\t-n number              : video number max = 6, Default 2\n");
    printf("\t-N Set 3DNR & HDR      : [0] enable and set 3DNR level(default level-3), [1] enable and set HDR type\n");
    printf("\t             3DNR level: Level 0: 0 set\n");
    printf("\t                         Level 1: 1 set,  8-bit\n");
    printf("\t                         Level 2: 1 set, 10-bit\n");
    printf("\t                         Level 3: 1 set, 12-bit\n");
    printf("\t                         Level 4: 2 set,  8-bit,  8-bit\n");
    printf("\t                         Level 5: 2 set,  8-bit, 10-bit\n");
    printf("\t                         Level 6: 2 set,  8-bit, 12-bit\n");
    printf("\t                         Level 7: 2 set, 12-bit, 12-bit(default if enable 3DNR)\n");
    printf("\t               HDR type: 1, E_MI_VPE_HDR_TYPE_VC(default if enable 3DNR@SC Sensor)\n");
    printf("\t                         2, E_MI_VPE_HDR_TYPE_DOL(default if enable 3DNR@Sony Sensor)\n");
    printf("\t                         3, E_MI_VPE_HDR_TYPE_EMBEDDED\n");
    printf("\t                         4, E_MI_VPE_HDR_TYPE_LI\n");
    printf("\t-o                     : open osd display,Default close\n");
    //printf("\t-p disposable          : [0] close, [1] open. Default close disposable\n");
    //printf("\t-P Enable PIP          : [0] x, [1] y, [2] w, [3] h. Default disable\n");
    printf("\t-q                     : open iqserver,Default close\n");
    printf("\t                       : when open iqserver, can not use -v 4, -x, -T, -c, -a\n");
    //printf("\t-Q set cQPOffset       : [-12,12]\n");
    printf("\t-r resolution          : [0] VGA        [1] 720P       [2] 1080P      [3]CIF         [4]4CIF        [5] 360p       [6] 1280*960   [7] D1\n");
    printf("\t                       : [8] 1056*1056  [9] 1920*1088  [10]2048*1536  [11]2688*1520  [12]2560*2048  [13]2592*1944  [14]3072*2160  [15]3840*2160\n");
    printf("\t                       : [16]2560*1440  [17]704*704    [18]1056*1056  [19]1440*1440  [20]1504*1504  [21]1536*1536  [22]1920*1920  [23]2048*2048\n");
    printf("\t                       : [24]2144*2144  [25]2688*1536  [26]2688*1920  [27]2688*1944  [28]2688*2048  [29]2688*2144  [30]2688*2160  Default: 1080P\n");
    printf("\t-R custom resolution   : width_height_width_height, customized resolution, for example\n");
    printf("\t                       : 1920_1080_720_720, video0  1920*1080, video1 720*720\n");
    printf("\t-s sensor framerate    : framerate. Default 25\n");
    printf("\t-S sensor resolution   : Select Sensor output resolution, work I5/I6E!!\n");
    printf("\t-t openFrameTimeStamp  : [0] not show frame interval(us), [else] show frame interval(us). Default [0] \n");
    //printf("\t-T seconds             : enable autotest after seconds \n");
    printf("\t-u open ircut          : [0] close,[1] open, default [0]\n");
    printf("\t-U uvc resolution      : [0] enable uvc only [1] open alsa capture dev & uvc [2] open alsa playback dev & uvc [3] open alsa dev & uvc\n");
    printf("\t-v video encoder type  : [0] h264, [1] h265,[2] mjpeg, [3] YUV420, [4] jpeg snapshot, Default [0] \n");
    printf("\t-V vi channel          : [0_1_3_4_2] video 0 use vi 0, video 1 use vi 1, video 2 use vi 3...\n");
    printf("\t                       : video encoder type [4] jpeg snapshot must use vi 2\n");
    printf("\t-x                     : enable fix MMA, Default disable\n");
#if TAGET_CHIP_I6B0
    printf("\t                         notice1:insmod mi_venc.ko max_width=xxxx max_height=xxxx\n");
    printf("\t                         notice2:sysConfig.json vif max resolution must set same as sensor real resolution\n");
#endif
    printf("\t-y                     : open remota IQServer, Default close\n");
    printf("\t-c ie video capture    : open ie funciton,param is fd,md,od,log,interval.\n");
    printf("\t                       : [1] open fd\n");
    printf("\t                       : [0_1] open md\n");
    printf("\t                       : [0_0_1] open od\n");
    printf("\t                       : [0_0_0_0_5_0_4_1] open vg\n");
    printf("\t                       : [0_0_0_1_0] open hchd\n");
    printf("\t                       : [0_1_0_0_5] yuv frame interval for ie, ie used yuv fps=25/interval, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_1] close ie osd display, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_0_2] vi channel for ie. default[1]. can not be the same with -V\n");
    printf("\t                       : [0_0_0_0_0_0_0_1] HD album mode operation, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_0_0_2] HD recognize mode, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_0_0_16] VD lenrn mode, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_0_0_32] VD detect mode, defalut[0]\n");
    printf("\t                       : [0_0_0_0_0_0_0_0_320_180] resolution of vi channel for ie. default[320x180]\n");
    printf("\t                       : [0_0_0_0_15_0_0_0_416_416_1] open dla process\n");
    printf("\t                       : [0_0_0_0_0_0_1] Default 0_0_0_0_0 close fd,md,od,close yuv data record\n");
    printf("\t                       : (ie only support one way 1080P now ./mixer -r 2 -n 1 -c 1_1_1_0 )\n");
    printf("\t                       : (use vi channel 4 to deal with ie ./mixer -n 3 -r 2_0_0 -v 0_1_1 -V 0_1_3 -c 0_1_0_0_0_0_4 -o)\n");
    printf("\t-w save stream         : [0] close, [1] open. Default close \n");
    //printf("\t-W test cycle buffer   : [0] close, [1] open. Default close \n");
    printf("\t\tset diffrent video options(support -r -v -b -f -s -p -w ), example for resolution: \n");
    printf("\t\t-r 0                 : video 0 is VGA, other video is default. \n");
    printf("\t\t-r 0_1               : video 0 is VGA, video 1 is 720P, other video is default. \n");
    printf("\t\t-r 0_1_2             : video 0 is VGA, video 1 is 720P. video 2 is 1080P \n");
    printf("\t\tvideo 3 encoder only support mjpeg now \n");
    printf("\t-z enable PIP          : [0] x, [1] y, [2] w, [3] h. Default disable, doesn`t work I6/I6B0\n");
#if MIXER_SED_ENABLE
    printf("\t-Z venc attach SED     : [0] close, [1] open, Default [0] \n");
#endif
    printf("\n");
    printf("\t-d level               : [0]|[1]|[2]set superframe work mode: 0 is none, 1 is discard, 2 is reencode.\n");
    //printf("\t                       : [2] save yuv or jpeg frame. \n");
    printf("\t                       : [4] printf audio stream log. \n");
    printf("\t                       : [8] printf video stream log. \n");
    printf("\t                       : [16 / 0x10]Enable display Audio infomation\n");
    printf("\t                       : [32 / 0x20]save audio data to file audio.wav \n");
    printf("\t                       : [64 / 0x40]test osd api \n");
    printf("\t                       : [128 / 0x80]open onvif\n");
    printf("\t                       : [256 / 0x100]Enable display ISP information,default disenable\n");
    printf("\t                       : [512 / 0x200]Enable video info display,default disenable\n");
    printf("\t                       : [1024 / 0x400]set osd flicker,default disenable\n");
    printf("\t                       : [2048 / 0x800]Enable display User0 information, default disenable\n");
    printf("\t                       : [4096 / 0x1000]Enable display User1 infomation,default disenable\n");
    printf("\t                       : [16384 / 0x4000]Osd color inverse open\n");
    printf("\t                       : [32768 / 0x8000] full osd number(create 8 osd per video)\n");
    printf("\t                       : [65535 / 0x10000] private mask. input privatemask to set private mask when running\n");
    printf("\t                       : default 0\n");
    printf("\n");
    printf("\tLocal Record:\n");
    printf("\tsetprop mxr.d.<stream_name> <path_to_file>   : set the file to save bit stream locally, must be set before enable\n");
    printf("\tsetprop mxr.r.<stream_name> 1                : start the local save\n");
    printf("\tsetprop mxr.r.<stream_name> 0                : sttop the local save and close the saving file\n");
    printf("\t                       \n");

    printf("\tsupport setprop to config param before lunch mixer:mixer.prop.vechn*.qp \n");
    printf("\t                                                   mixer.prop.watchdog \n");
    printf("\t                                                   mixer.prop.video.savepath\n");
    printf("\t                                                   mixer.osd.interval\n");
    printf("\t(such as set qp of ve chn 0 equal 26):setprop mixer.prop.vechn0.qp 26 \n");
    printf("\t(open watchdog test):setprop mixer.prop.watchdog 1 \n");
    printf("\t(set video default save path):setprop mixer.prop.video.savepath /usb \n");
    printf("\t(set osd flicker time interval ms):setprop mixer.osd.interval 500 \n");
    printf("\n");
}

void mixer_process_display_help(void)
{
    printf("\nwhen mixer is running:\n");
    printf("\t sensorfps       : set sensor framerate\n");
    printf("\t audiovqe        : set audiovqe mode\n");
    printf("\t videobitrate    : set bitrate and rc parameter\n");
    printf("\t superframesize  : set I/P-frame super frame bps\n");
    printf("\t ircut           : set ircut mode. [0] black  [1] white\n");
    printf("\t avideolog       : open or close audio/video stream log\n");
    printf("\t audiostream     : save audio stream data\n");
    printf("\t aed             : set sensitivity and operatingpoint of aed\n");
    printf("\t md              : set md parameter\n");
    printf("\t videofps        : set framerate\n");
    printf("\t gop             : set gop\n");
    printf("\t ltr             : set virtual I Interval\n");
    printf("\t isp             : set isp parameter\n");
    printf("\t idr             : request idr\n");
    printf("\t audio           : open or close audio\n");
    printf("\t videocodec      : set codec\n");
    printf("\t rot             : set video rotate, 0, 90, 180, 270.\n");
    printf("\t mask            : enable Mask\n");
    printf("\t slices          : set slices number\n");
    printf("\t video3dnr       : enable 3DNR pass number\n");
    printf("\t videoosd        : enable/disable osd and osd relative function\n");
    printf("\t privatemask     : osd private mask set\n");
    printf("\t videoon         : open or close video\n");
    printf("\t pwm             : set pwm mode\n");
    printf("\t roi             : set venc channel roi parameter\n");
    printf("\t res             : set video channel resolution\n");
    printf("\t snrres          : set sensor resolution\n");
    printf("\t showframeint    : showframeint\n");
    printf("\t shell           : set runtime system command\n");
    printf("\t superframemode  : set SuperFrame work mode: [0] is none, [1] is discard, [2] is reencode\n");
    printf("\t sclmirrorflip   : set mirror/flip state\n");
    printf("\t rec             : save stream,  [0] disable rec, [1] enable rec\n");
    printf("\t hdr             : switch mode [0]mode linaer,  [1]hdr VC, [2]hdr DOL, [3]hdr EMBEDDED, [4]hdr LI\n");
    printf("\t audioin         : set audio-in volume\n");
    printf("\t audioout        : set audio-out volume\n");
    printf("\t ie              : set ie mode\n");
    printf("\t ipu             : ipu fireware file,model file path,label file path\n");
    printf("\t cus3a           : Enable user space sigma3A [0] Disable, [1] Enable\n");
    printf("\t q/Q/exit        : mixer exit\n");
    printf("\t help            : help info\n");
    printf("\n");
}

void display_help()
{
    mixer_startup_display_help();
    mixer_process_display_help();

    exit(1);
}

MI_S32 OpenRecord()
{
       mixer_send_cmd(CMD_RECORD_ENABLE, NULL, 0);
    //MIXER_DBG("^^^^^^^^^^^^^^^^^^^^^^^\n");
    return 0;
}

MI_S32 CloseRecord()
{
    mixer_send_cmd(CMD_RECORD_DISABLE, NULL, 0);
    //MIXER_DBG("^$$$$$$$$$$$$$$^^^^^^^^^^^^\n");
    return 0;
}

MI_S32 getResolution(MI_S32 index, MI_U16 *width, MI_U16 *height)
{
    if(NULL == width || NULL == height)
    {
        return -1;
    }

    switch(index)
    {
        case 0:  { *width = 640;  *height = 480;  } break;
        case 1:  { *width = 1280; *height = 720;  } break;
        case 2:  { *width = 1920; *height = 1080; } break;
        case 3:  { *width = 352;  *height = 288;  } break;
        case 4:  { *width = 704;  *height = 576;  } break;
        case 5:  { *width = 640;  *height = 368;  } break;
        case 6:  { *width = 1280; *height = 960;  } break;
        case 7:  { *width = 720;  *height = 576;  } break;
        case 8:  { *width = 1056; *height = 1056; } break;
        case 9:  { *width = 1920; *height = 1088; } break;
        case 10: { *width = 2048; *height = 1536; } break;
        case 11: { *width = 2688; *height = 1520; } break;
        case 12: { *width = 2560; *height = 2048; } break;
        case 13: { *width = 2592; *height = 1944; } break;
        case 14: { *width = 3072; *height = 2160; } break;
        case 15: { *width = 3840; *height = 2160; } break;
        case 16: { *width = 2560; *height = 1440; } break;
        case 17: { *width = 704;  *height = 704;  } break;
        case 18: { *width = 1056; *height = 1056; } break;
        case 19: { *width = 1440; *height = 1440; } break;
        case 20: { *width = 1504; *height = 1504; } break;
        case 21: { *width = 1536; *height = 1536; } break;
        case 22: { *width = 1920; *height = 1920; } break;
        case 23: { *width = 2048; *height = 2048; } break;
        case 24: { *width = 2144; *height = 2144; } break;
        case 25: { *width = 2688; *height = 1536; } break;
        case 26: { *width = 2688; *height = 1920; } break;
        case 27: { *width = 2688; *height = 1944; } break;
        case 28: { *width = 2688; *height = 2048; } break;
        case 29: { *width = 2688; *height = 2144; } break;
        case 30: { *width = 2688; *height = 2160; } break;
        default: { *width = 1920; *height = 1080; } break;
    }

    return 0;
}


Mixer_EncoderType_e getVideoEncoderType(MI_S32 type)
{
    switch(type)
    {
        case 0:  return  VE_AVC;
        case 1:  return  VE_H265;
        case 2:  return  VE_MJPEG;
        case 3:  return  VE_YUV420;
        case 4:  return  VE_JPG;
        case 5:  return  VE_JPG_YUV422;
        default: return  VE_AVC;
    }
}

const char * getVideoEncoderTypeString(Mixer_EncoderType_e type)
{
    switch(type)
    {
        case VE_AVC:        return "h264";
        case VE_H265:       return "h265";
        case VE_MJPEG:      return "mjpeg";
        case VE_YUV420:     return "yuv420";
        case VE_JPG:        return "jpg";
        case VE_JPG_YUV422: return "YUV422";
        default:            return "h264";
    }
}

MediaType_e getAudioEncoderType(MI_S32 type)
{
    switch(type)
    {
        case 0:   return  MT_PCM;
        case 1:   return  MT_G711A;
        case 2:   return  MT_G711U;
        case 3:   return  MT_G711;
        case 4:   return  MT_AMR;
        case 5:   return  MT_AAC;
        case 6:   return  MT_G726;
        case 7:   return  MT_MP3;
        case 8:   return  MT_OGG;
        case 9:   return  MT_ADPCM;
        case 10:  return  MT_OPUS;
        case 11:  return  MT_G726_16;
        case 12:  return  MT_G726_24;
        case 13:  return  MT_G726_32;
        case 14:  return  MT_G726_40;
        default:  return  MT_G711A;
    }
}

MI_S32 mixerEnableIEModule(MI_S8 enable)
{
    if(1 == enable)
    {
        if(g_fdParam)
            mixer_send_cmd(CMD_IE_FD_OPEN, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));

        if(g_mdParam)
            mixer_send_cmd(CMD_IE_MD_OPEN, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));

        if(g_odParam)
            mixer_send_cmd(CMD_IE_OD_OPEN, (MI_S8 *)&g_odParam, sizeof(g_odParam));

        if(g_vgParam)
            mixer_send_cmd(CMD_IE_VG_OPEN, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));

        if(g_hchdParam)
            mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&g_hchdParam, sizeof(g_hchdParam));

        if(g_dlaParam)
            mixer_send_cmd(CMD_IE_DLA_OPEN, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));

    }
    else if(0 == enable)
    {
        if(g_fdParam)
            mixer_send_cmd(CMD_IE_FD_CLOSE, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));

        if(g_mdParam)
            mixer_send_cmd(CMD_IE_MD_CLOSE, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));

        if(g_odParam)
            mixer_send_cmd(CMD_IE_OD_CLOSE, (MI_S8 *)&g_odParam, sizeof(g_odParam));

        if(g_vgParam)
            mixer_send_cmd(CMD_IE_VG_CLOSE, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));

        if(g_hchdParam)
            mixer_send_cmd(CMD_IE_HCHD_CLOSE, (MI_S8 *)&g_hchdParam, sizeof(g_hchdParam));

        if(g_dlaParam)
            mixer_send_cmd(CMD_IE_DLA_CLOSE, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));

        if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_hchdParam || g_dlaParam)
            usleep(200000);
    }

    return 0;
}


MI_S32 mixerInputProcessLoop()
{
    g_ConsoleManager->RThreadProc();

    return 0;
}

void mixerExit(MI_S32 bCloseWatchDog)
{
    //MI_S32 fifo_data[256];

    if(g_bCusAEenable)
    {
        mixer_send_cmd(CMD_ISP_CLOSE_CUS3A, NULL, 0);
    }

    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
    {
        if(g_fdParam)
            mixer_send_cmd(CMD_IE_FD_CLOSE, NULL, 0);

        if(g_mdParam)
            mixer_send_cmd(CMD_IE_MD_CLOSE, NULL, 0);

        if(g_odParam)
            mixer_send_cmd(CMD_IE_OD_CLOSE, NULL, 0);

        if(g_vgParam)
            mixer_send_cmd(CMD_IE_VG_CLOSE, NULL, 0);

        if(g_hchdParam)
            mixer_send_cmd(CMD_IE_HCHD_CLOSE, NULL, 0);

        if(g_dlaParam)
            mixer_send_cmd(CMD_IE_DLA_CLOSE, NULL, 0);

        mixer_send_cmd(CMD_IE_CLOSE, NULL, 0);

        /*if(g_hdParam)
            mixer_send_cmd(CMD_IE_HD_CLOSE, NULL, 0);

        if(TRUE == g_audioOutParam[0].bFlag)
        {
            mixer_send_cmd(CMD_AUDIO_VD_CLOSE, (MI_S8 *)&g_vdParam, sizeof(g_vdParam));
            if(g_vdParam & 0x01)
                mixer_property_set("ie.vd.enable", "0");

            if(g_vdParam & 0x02)
                mixer_property_set("ie.vd.enable", "0");
        }*/
    }
#if MIXER_SED_ENABLE
    if(g_EnableSed)
    {
        mixer_send_cmd(CMD_SED_CLOSE, NULL, 0);
    }
#endif
    //close record
    CloseRecord();
    //end

    // close before video module
    if(g_displayVideoInfo && g_displayOsd)
    {
        mixer_send_cmd(CMD_OSD_CLOSE_DISPLAY_VIDEO_INFO, NULL, 0);
    }

    // live555 should close before video and audio close ,because live555 used video&audio global variable
    mixer_send_cmd(CMD_SYSTEM_LIVE555_CLOSE, NULL, 0);


    if(TRUE == g_audioOutParam[0].bFlag)
    {
        mixer_send_cmd(CMD_AUDIO_STOPPLAY, NULL, 0);
        g_audioOutParam[0].bFlag = FALSE;
    }

    for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
    {
        if(TRUE == g_audioInParam[i].bFlag)
        {
          mixer_send_cmd(CMD_AUDIO_CLOSE, NULL, 0);
          break;
        }
    }

    if(g_openUVCDevice)
    {
        printf("Close the UVC Device\n");
        mixer_send_cmd(CMD_UVC_CLOSE, (MI_S8 *)&g_uvcParam, sizeof(g_uvcParam));
    }

    if(g_displayOsd)
    {
        if(!g_ieOsdDisplayDisable)
        {
            mixer_send_cmd(CMD_OSD_IE_CLOSE, NULL, 0);
        }

        if(gDebug_osdPrivateMask)
        {
            mixer_send_cmd(CMD_OSD_PRIVATEMASK_CLOSE, NULL, 0);
        }
        mixer_send_cmd(CMD_OSD_MASK_CLOSE, NULL, 0);
        mixer_send_cmd(CMD_OSD_CLOSE, NULL, 0);
    }

    mixer_send_cmd(CMD_VIDEO_CLOSE, NULL, 0);

    if(g_openOnvif == TRUE)
    {
        mixer_send_cmd(CMD_SYSTEM_ONVIF_CLOSE, NULL, 0);
    }

    if(g_ircut == TRUE)
    {
        mixer_send_cmd(CMD_SYSTEM_IRCUT_CLOSE, NULL, 0);
    }

    if(g_openIQServer)
    {
        mixer_send_cmd(CMD_SYSTEM_IQSERVER_CLOSE , NULL, 0);
    }

    if(bCloseWatchDog)
    {
        mixer_send_cmd(CMD_SYSTEM_WATCHDOG_CLOSE , NULL, 0);
    }

    mixer_send_cmd(CMD_VIF_UNINIT, NULL, 0);
    //add this mixer will not exit normally
    //mixer_wait_cmdresult(CMD_VIF_UNINIT, fifo_data, sizeof(fifo_data));

    mixer_send_cmd(CMD_SYSTEM_UNINIT, NULL, 0);

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    if(TRUE == g_fixMMA)
    {
        mixer_send_cmd(CMD_VIDEO_MMA_FREE, (MI_S8*)&g_videoNumber, sizeof(g_videoNumber));
    }
#endif
    g_CMsgManager->UnInit();
    g_PacketModule->stop();
    g_PacketModule->unInit();

}

void signalStop(MI_S32 signo)
{
    printf("\nsignalStop(signal code: %d) !!!\n", signo);
    mixerExit(FALSE);
    printf("mixer app exit\n");
    _exit(0);
}

inline void mixer_set_audioType(void)
{
    char  *str = strtok(optarg, "_");
    int index = 0;
    MI_U32 AudioInEnable = 0;
    MI_S32 s32Value = 0;

    if(NULL == str)
    {
        printf("%s %d str is NULL\n", __func__, __LINE__);
        return;
    }
    AudioInEnable = mixerStr2Int(str);
    str = strtok(NULL, "_");

    if(0 != AudioInEnable) //set Audio In param
    {
        index = g_audioInParam[0].u8AudioInNum;
        if((MIXER_AI_MAX_NUMBER <= index) || (index < 0))
        {
            printf("%s:%d set AudioIn index(=%d) error, and exit(1)!\n", __func__, __LINE__, index);
            exit(1);
        }

        for(MI_S32 i = 0; i < 10 && str; i++)
        {
            switch(i)
            {
                case 0:
                    s32Value = mixerStr2Int(str);
                    if((0 == index) && (0 <= s32Value) && (s32Value < 3))
                    {
                        g_audioInParam[index].stAudioInChnPort.u32DevId = s32Value;
                    }
                    else
                    {
                        g_audioInParam[index].stAudioInChnPort.u32DevId = g_audioInParam[0].stAudioInChnPort.u32DevId;
                    }

                    break;

                case 1:
                    s32Value = mixerStr2Int(str);
                    if((0 <= s32Value) && (s32Value < MIXER_AI_MAX_NUMBER) && (s32Value == index))
                    {
                        g_audioInParam[index].stAudioInChnPort.u32ChnId = s32Value;
                    }
                    else
                    {
                        g_audioInParam[index].stAudioInChnPort.u32ChnId = index;
                        printf("%s:%d set AudioIn-%d ChnId(=%d) error, and re-set ChnId=%d!\n", __func__, __LINE__, index, s32Value,index);
                    }
                    g_audioInParam[index].stAudioInChnPort.u32PortId = 0;
                    break;

                case 2:
                    s32Value = mixerStr2Int(str);
                    if((0 == index) && (0 <= s32Value) && (s32Value <= 1))
                    {
                        g_audioInParam[index].stAudioAttr.eBitwidth = (MI_AUDIO_BitWidth_e)0;//only support 16bit
                    }
                    else if(0 < index)
                    {
                        g_audioInParam[index].stAudioAttr.eBitwidth = g_audioInParam[0].stAudioAttr.eBitwidth;
                    }
                    break;

                case 3:
                    s32Value = mixerStr2Int(str);
                    if((0 <= s32Value) && (s32Value < MT_NUM))
                    {
                        g_audioInParam[index].AiMediaType = getAudioEncoderType(s32Value);
                    }
                    else
                    {
                        g_audioInParam[index].AiMediaType = getAudioEncoderType(1); //MT_G711A
                        printf("%s:%d set AudioIn-%d Aenc Type(=%d) error, and re-set Aenc Type=G711A!\n", __func__, __LINE__, index, s32Value);
                    }
                    break;

                case 4:
                    s32Value = mixerStr2Int(str);
                    if((0 <= s32Value) && (s32Value <= 21))
                    {
                        g_audioInParam[index].s32VolumeInDb = s32Value;
                    }
                    else
                    {
                        g_audioInParam[index].s32VolumeInDb = 21;
                        printf("%s:%d Set AudioIn-%d VolumeInDb in [0 21], you set value=%d, reset VolumeInDb=%dDb\n",
                                            __func__, __LINE__, index, s32Value, g_audioInParam[index].s32VolumeInDb);
                    }
                    break;

                case 5:
                    s32Value = mixerStr2Int(str);
                    if((0 == index) && \
                      ((E_MI_AUDIO_SAMPLE_RATE_8000 == s32Value) || \
                        (E_MI_AUDIO_SAMPLE_RATE_16000 == s32Value) || \
                        (E_MI_AUDIO_SAMPLE_RATE_32000 == s32Value) || \
                        (E_MI_AUDIO_SAMPLE_RATE_48000 == s32Value)))
                    {
                        g_audioInParam[index].stAudioAttr.eSamplerate = (MI_AUDIO_SampleRate_e)s32Value;
                    }
                    else if(0 < index)
                    {
                        g_audioInParam[index].stAudioAttr.eSamplerate = g_audioInParam[0].stAudioAttr.eSamplerate;
                        printf("%s:%d set AudioIn-%d SampleRate(=%d) error, and re-set SampleRate=8000!\n", __func__, __LINE__,index, s32Value);
                    }
                    break;

                case 6:
                    s32Value = mixerStr2Int(str);
                    if(0 != s32Value)
                    {
                        g_audioInParam[index].bAudioInVqe = TRUE;
                        g_stAiVqeCfg.u32ChnNum = 1;
                    }
                    else
                    {
                        g_audioInParam[index].bAudioInVqe = FALSE;
                    }
                    break;
/*
                case 7:
                    s32Value = mixerStr2Int(str);
                    if(0 != s32Value)
                    {
                        g_audioInEnableG7xxExt = TRUE;
                    }
                    else
                    {
                        g_audioInEnableG7xxExt = FALSE;
                    }
                    break;
*/
                case 7:
                    s32Value = mixerStr2Int(str);
                    if((E_MI_AUDIO_SAMPLE_RATE_8000 == s32Value) ||\
                    (E_MI_AUDIO_SAMPLE_RATE_16000 == s32Value) || \
                    (E_MI_AUDIO_SAMPLE_RATE_32000 == s32Value) || \
                    (E_MI_AUDIO_SAMPLE_RATE_48000 == s32Value))
                    {
                        g_audioInParam[index].eAudioInReSampleRate = (MI_AUDIO_SampleRate_e)s32Value;
                    }
                    else
                    {
                        g_audioInParam[index].eAudioInReSampleRate = E_MI_AUDIO_SAMPLE_RATE_INVALID;
                        printf("%s:%d set AudioIn-%d AudioInReSampleRate(=%d) error, and disable ReSampleRate!\n", __func__, __LINE__,index, s32Value);
                    }
                    break;

                case 8:
                    s32Value = mixerStr2Int(str);
                    if((0 == index) && \
                        ((E_MI_AUDIO_SOUND_MODE_MONO == s32Value) || \
                        (E_MI_AUDIO_SOUND_MODE_STEREO == s32Value) || \
                        (E_MI_AUDIO_SOUND_MODE_QUEUE == s32Value)))
                    {
                        g_audioInParam[index].stAudioAttr.eSoundmode = (MI_AUDIO_SoundMode_e)s32Value;
                    }
                    else if(0 < index)
                    {
                        g_audioInParam[index].stAudioAttr.eSoundmode = g_audioInParam[0].stAudioAttr.eSoundmode;
                        printf("%s:%d set AudioIn-%d eSoundmode(=%d) error, and re-set eSoundmode=E_MI_AUDIO_SOUND_MODE_MONO!\n", __func__, __LINE__,index, s32Value);
                    }
                    break;
            }

            str = strtok(NULL, "_");
        }

        g_audioInParam[0].u8AudioInNum++;
        g_audioInParam[0].stAudioAttr.u32ChnCnt = g_audioInParam[0].u8AudioInNum;
        if(MIXER_AI_MAX_NUMBER > index)
        {
         g_audioInParam[index].bFlag = TRUE;
        }
    }
    else // set Audio Out param
    {
        index = g_audioOutParam[0].s32AudioOutNum;
        printf("%s:%d index=%d, g_audioOutParam[0].s32AudioOutNum=%d!\n", __func__, __LINE__, index, g_audioOutParam[0].s32AudioOutNum);

        if((MIXER_AO_MAX_NUMBER <= index) || (index < 0))
        {
            printf("%s:%d set AudioOut index(=%d) error, and exit(1)!\n", __func__, __LINE__, index);
            exit(1);
        }

        for(MI_S32 i = 0; i < 7 && str; i++)
        {
            switch(i)
            {
                case 0 :
                    s32Value = mixerStr2Int(str);
                    if((0 <= s32Value) && (s32Value < MI_AUDIO_DEV_ALL))
                    {
                        g_audioOutParam[index].stAudioOutChnPort.u32DevId = s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].stAudioOutChnPort.u32DevId = g_audioOutParam[0].stAudioOutChnPort.u32DevId;
                        printf("%s:%d set AudioOut-%d DevId(=%d) error, and re-set DevId=0!\n", __func__, __LINE__, index, s32Value);
                    }
                    break;

                case 1 :
                    s32Value = mixerStr2Int(str);
                    if((0 <= s32Value) && (s32Value < MIXER_AO_MAX_NUMBER))
                    {
                        g_audioOutParam[index].stAudioOutChnPort.u32ChnId = s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].stAudioOutChnPort.u32ChnId = 0;
                        printf("%s:%d set AudioOut-%d ChnId(=%d) error, and re-set ChnId=0!\n", __func__, __LINE__, index, s32Value);
                    }

                    g_audioOutParam[index].stAudioOutChnPort.u32PortId = 0;
                    break;

                case 2 :
                    s32Value = mixerStr2Int(str);
                    if(0 != s32Value)
                    {
                        g_audioOutParam[index].u8AudioOutRes = TRUE;
                    }
                    else
                    {
                        g_audioOutParam[index].u8AudioOutRes = FALSE;
                    }
                    break;

                case 3 :
                    s32Value = mixerStr2Int(str);
                    if(0 != s32Value)
                    {
                        g_audioOutParam[index].u8AudioOutVqe = TRUE;
                    }
                    else
                    {
                        g_audioOutParam[index].u8AudioOutVqe = FALSE;
                    }
                    break;

                case 4 :
                    if('-' == (char)*str)
                    {
                        s32Value = mixerStr2Int(str + 1);
                        s32Value = (~s32Value) + 1;
                    }
                    else
                    {
                        s32Value = mixerStr2Int(str);
                    }
#if TARGET_CHIP_I5
                    if((-64 <= s32Value) && (s32Value <= 30))
                    {
                        g_audioOutParam[index].s32VolumeOutDb = s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].s32VolumeOutDb = 0;
                        printf("%s:%d VolumeOutDb in [-64 30], you set value=%d, reset VolumeOutDb=%dDb\n",
                                      __func__, __LINE__, s32Value, g_audioOutParam[index].s32VolumeOutDb);
                    }
#elif TARGET_CHIP_I6
                    if((-60 <= s32Value) && (s32Value <= 10))
                    {
                        g_audioOutParam[index].s32VolumeOutDb = s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].s32VolumeOutDb = -10;
                        printf("%s:%d VolumeOutDb in [-60 10], you set value=%d, reset VolumeOutDb=%dDb\n",
                                      __func__, __LINE__, s32Value, g_audioOutParam[index].s32VolumeOutDb);
                    }
#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                    if((-60 <= s32Value) && (s32Value <= 30))
                    {
                        g_audioOutParam[index].s32VolumeOutDb = s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].s32VolumeOutDb = 0;
                        printf("%s:%d VolumeOutDb in [-60 30], you set value=%d, reset VolumeOutDb=%dDb\n",
                                      __func__, __LINE__, s32Value, g_audioOutParam[index].s32VolumeOutDb);
                    }

#endif
                    break;

                case 5 :
                    s32Value = mixerStr2Int(str);
                    if((E_MI_AUDIO_SAMPLE_RATE_8000 == s32Value) || \
                    (E_MI_AUDIO_SAMPLE_RATE_16000 == s32Value) || \
                    (E_MI_AUDIO_SAMPLE_RATE_32000 == s32Value) || \
                    (E_MI_AUDIO_SAMPLE_RATE_48000 == s32Value))
                    {
                        g_audioOutParam[index].eAudioOutReSampleRate = (MI_AUDIO_SampleRate_e)s32Value;
                    }
                    else
                    {
                        g_audioOutParam[index].eAudioOutReSampleRate = E_MI_AUDIO_SAMPLE_RATE_8000;
                        printf("%s:%d set AudioOut-%d AudioOutReSampleRate(=%d) error, and re-set SampleRate=8000!\n", __func__, __LINE__,index, s32Value);
                    }
                    break;

                case 6:
                    s32Value = mixerStr2Int(str);
                    if(0 != s32Value)
                    {
                        g_audioOutEnableG7xxExt = TRUE;
                    }
                    else
                    {
                        g_audioOutEnableG7xxExt = FALSE;
                    }
                    break;
            }

            str = strtok(NULL, "_");
        }

        g_audioOutParam[0].s32AudioOutNum++;
        if((MIXER_AO_MAX_NUMBER > index) && (0 <= index))
        {
          g_audioOutParam[index].bFlag = TRUE;
        }
    }
}


inline void mixer_set_audioFunc(void)
{
    char *str = strtok(optarg, "_");
    MI_S32 tmp[40] = {0x0};
    MI_U32 i = 0x0;

    memset(tmp, 0, sizeof(tmp));

    for( i = 0; i<20 && str; i++)
    {
        tmp[i] = mixerStr2Int(str);
        str = strtok(NULL, "_");
    }

    if(1 == tmp[0])
    {
        g_stAiVqeCfg.bAecOpen = tmp[1] ? TRUE : FALSE;
        g_stAiVqeCfg.bAgcOpen = tmp[2] ? TRUE : FALSE;
        tmp[3] = tmp[3] < (-80) ? (-80) : tmp[3];
        g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = tmp[3] > 0 ? 0 : tmp[3];
        g_stAiVqeCfg.bAnrOpen = tmp[4] ? TRUE : FALSE;
        tmp[5] = tmp[5] < 0 ? 0 : tmp[5];
        g_stAiVqeCfg.stAnrCfg.u32NrSmoothLevel = tmp[5] > 30 ? 30 : tmp[5];
        g_stAiVqeCfg.bHpfOpen = tmp[6] ? TRUE : FALSE;
        g_stAiVqeCfg.bEqOpen = tmp[7] ? TRUE :FALSE;
//        g_bAudioInWNR = tmp[6] ? TRUE : FALSE;
//        tmp[7] = (tmp[7] < AUDIO_WNROff ? AUDIO_WNROff : tmp[7]);
        //g_VqeConfig.WnrCfg.WnrType = (WNRType_e)(tmp[7] > AUDIO_WNRPlusNF ? AUDIO_WNRPlusNF : tmp[7]);
    }
    else
    {
        //g_bAudioOutAEC = tmp[1] ? TRUE : FALSE;
        g_stAoVqeCfg.bAgcOpen = tmp[1] ? TRUE : FALSE;
        tmp[2] = tmp[2] < (-80) ? (-80) : tmp[2];
        g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = tmp[2] > 0 ? 0 : tmp[2];
        g_stAoVqeCfg.bAnrOpen = tmp[3] ? TRUE : FALSE;
        tmp[4] = tmp[4] < 0 ? 0 : tmp[4];
        g_stAoVqeCfg.stAnrCfg.u32NrSmoothLevel = tmp[4] > 30 ? 30 : tmp[4];
        g_stAoVqeCfg.bHpfOpen = tmp[5] ? TRUE : FALSE;
        g_stAoVqeCfg.bEqOpen = tmp[6] ? TRUE : FALSE;
//        g_bAudioOutWNR = tmp[6] ? TRUE : FALSE;
//        tmp[7] = (tmp[7] < AUDIO_WNROff ? AUDIO_WNROff : tmp[7]);
        //g_VqeConfig.WnrCfg.WnrType = (WNRType_e)(tmp[7] > AUDIO_WNRPlusNF ? AUDIO_WNRPlusNF : tmp[7]);
    }
}

inline void mixer_set_videoBitrate(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;
     MI_S32 tmp  = 0x0;

#if LOAD_MIXER_CFG
    ModuleVencInfo_s *pVencInfo_t = GetVencInfo();
#endif

    for( i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
               tmp = mixerStr2Int(str);
#if LOAD_MIXER_CFG
        if(tmp > (MI_S32)pVencInfo_t->MaxBitrate[i])
        {
            printf("please check you input param![bitrate must <= %d]\n", pVencInfo_t->MaxBitrate[i]);
            //continue;
        }
        else
        {
         if(str[strlen(str) - 1] == 'k' || str[strlen(str) - 1] == 'K')
         {
           if(tmp*1000 > (MI_S32)pVencInfo_t->MaxBitrate[i]){
             printf("please check you input param![bitrate must <= %d]\n",pVencInfo_t->MaxBitrate[i]);
              //continue;
            }
         }
         if(str[strlen(str) - 1] == 'm' || str[strlen(str) - 1] == 'M')
         {
            if(tmp*1000000 > (MI_S32)pVencInfo_t->MaxBitrate[i]){
            printf("please check you input param![bitrate must <= %d]\n",pVencInfo_t->MaxBitrate[i]);
            //continue;
            }
         }
        }
#endif

        if(tmp > 0)
        {
            g_bSetBitrate = TRUE;
            g_videoParam[i].bitrate = tmp;
        }

        if(str[strlen(str) - 1] == 'k' || str[strlen(str) - 1] == 'K')
        {
            g_videoParam[i].bitrate *= 1000;
        }
        else if(str[strlen(str) - 1] == 'm' || str[strlen(str) - 1] == 'M')
        {
            g_videoParam[i].bitrate *= 1000000;
        }

        printf("Set VideoStream-%d bitrate %d\n", i, g_videoParam[i].bitrate);
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_ModuleBufCnt(void)
{
    char *str = strtok(optarg, "_");
    MI_S32 s32StreamIdx = -1;

    if(NULL == str)
    {
        printf("%s %d str is NULL\n", __func__, __LINE__);
        return;
    }

    s32StreamIdx = mixerStr2Int(str);
    str = strtok(NULL, "_");

    if((NULL == str) || (s32StreamIdx < 0) || (s32StreamIdx >= MAX_VIDEO_NUMBER))
    {
         printf("The input streamIdx(%d) is out of range(%d)\n", s32StreamIdx, MAX_VIDEO_NUMBER);
         return;
    }

    for(MI_S32 i = 0; i < 3 && str; i++)
    {
        switch(i)
        {
            case 0 : //vpe
                if(str)
                {
                    g_videoParam[s32StreamIdx].vpeBufUsrDepth = (MI_U8)mixerStr2Int(str);
                    str = strtok(NULL, "_");
                }
                if(str)
                {
                    g_videoParam[s32StreamIdx].vpeBufCntQuota = (MI_U8)mixerStr2Int(str);
                }
                break;

            case 1 : //venc
                if(str)
                {
                    g_videoParam[s32StreamIdx].vencBufUsrDepth = (MI_U8)mixerStr2Int(str);
                    str = strtok(NULL, "_");
                }
                if(str)
                {
                    g_videoParam[s32StreamIdx].vencBufCntQuota = (MI_U8)mixerStr2Int(str);
                }
                break;

            case 2 : //divp
                if(str)
                {
                    g_videoParam[s32StreamIdx].divpBufUsrDepth = (MI_U8)mixerStr2Int(str);
                    str = strtok(NULL, "_");
                }
                if(str)
                {
                    g_videoParam[s32StreamIdx].divpBufCntQuota = (MI_U8)mixerStr2Int(str);
                }
                break;

            default:
                return;
        }
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_IeParam(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

    g_ieOsdDisplayDisable = FALSE;

    for( i = 0; i < 11 && str; i++)
    {
        printf("%d\n", i);

        switch(i)
        {
            case 0 :
                g_fdParam = mixerStr2Int(str);
                break;

            case 1 :
                g_mdParam = mixerStr2Int(str);
                break;

            case 2 :
                g_odParam = mixerStr2Int(str);
                break;

            case 3 :
                g_hchdParam = mixerStr2Int(str);
                break;

            case 4 :
                g_ieFrameInterval = mixerStr2Int(str);
                break;

            case 5 :
                g_ieOsdDisplayDisable = mixerStr2Int(str);
                break;

            case 6:
                //g_ieVpePort = mixerStr2Int(str);
                printf("fix g_ieVpePort=%d, can not change!\n", g_ieVpePort);
                break;

            case 7 :
                g_vgParam = mixerStr2Int(str);
                printf("g_vgParam=%d\n", g_vgParam);
                break;

            case 8:

                {
                 g_ieWidth = mixerStr2Int(str);
                 printf("IE width=%d\n", g_ieWidth);
                }
                break;

            case 9:

                {
                  g_ieHeight = mixerStr2Int(str);
                  printf("IE height=%d\n", g_ieHeight);
                }
                 break;

            case 10:
                g_dlaParam = atoi(str);
                printf("g_dlaParam=%d\n", g_dlaParam);
                break;
        }

        str = strtok(NULL, "_");
    }

    if(g_fdParam || g_mdParam || g_odParam || g_vgParam || g_dlaParam)
    {
        g_ieLogParam = 1;
    }
}

inline void mixer_set_CaliDBPath(void)
{
   // MI_S32 len = strlen(optarg);
    char *pstr = optarg;
    char *pstrNxt = optarg;
    MI_U32 i = 0x0;

    memset(g_CaliItem, 0x0, sizeof(g_CaliItem));
    memset(g_CaliDBPath, 0x0, sizeof(g_CaliDBPath));

    for(i = 0; i < SS_CALI_ITEM_MAX; i++)
    {
        if(pstrNxt != NULL)
        {
            pstr = strtok(pstrNxt, ",");
            pstrNxt = strtok(NULL, "\0");

            if(NULL == pstr)
            {
                printf("%s %d pstr is NULL\n", __func__, __LINE__);
                return;
            }
            pstr = strtok(pstr, "_");

			if(NULL == pstr)
            {
                printf("%s %d pstr is NULL\n", __func__, __LINE__);
                return;
            }
            g_CaliItem[i] = mixerStr2Int(pstr);
            printf("%s:%d g_CaliItem[%d]: %d\n", __func__, __LINE__, i, g_CaliItem[i]);

            pstr = strtok(NULL, "\0");
            strncpy((char *)g_CaliDBPath[i], pstr, 128);
            g_CaliDBPath[i][127]='\0';
            printf("%s:%d g_CaliDBPath[%d]: %s\n", __func__, __LINE__, i, g_CaliDBPath[i]);

            if(g_CaliItem[i] >= SS_CALI_ITEM_MAX)
            {
                printf("[Load cali bin error]: Invalid format, please set the arg by: -C item_path,item_path,item_path,... \n");
                printf("example: mixer -C 1_/customer/obc_cali.data,0_/customer/awb_cali.data,3_/customer/alsc_cali.data \n");
            }
        }
    }
}

inline void mixer_set_OsdDisplay(void)
{
    MI_U32 len = 0;
    char *str = strtok(optarg, "_");
    MI_S32 tmp = 0;

    if(NULL == str)
    {
        printf("%s %d str is NULL\n", __func__, __LINE__);
        return;
    }

    tmp = mixerStr2Int(str);

    if(tmp & 0x03)
    {
        g_SuperFrameEnable = (tmp & 0x03);

        if(E_MI_VENC_SUPERFRM_DISCARD == g_SuperFrameEnable)
        {
            printf("\n  g_SuperFrameEnable = %d(E_MI_VENC_SUPERFRM_DISCARD)\n", g_SuperFrameEnable);
        }
        else if(E_MI_VENC_SUPERFRM_REENCODE == g_SuperFrameEnable)
        {
            printf("\n  g_SuperFrameEnable = %d(E_MI_VENC_SUPERFRM_REENCODE)\n", g_SuperFrameEnable);
#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
            MIXER_ERR("DO not support SUPERFRM_REENCODE , set def SUPERFRM_NONE\n");
            g_SuperFrameEnable = 0;
#endif
        }
    }

    if((tmp & 0x04) != 0)
    {
        gDebug_AudioStreamLog = TRUE;
        printf("\n  gDebug_AudioStreamLog open1\n");
    }

    if((tmp & 0x08) != 0)
    {
        gDebug_VideoStreamLog = TRUE;
        printf("\n  gDebug_VideoStreamLog Open!\n");
    }

    if((tmp & 0x10) != 0)
    {
        g_s32OsdHandleCnt = g_s32OsdHandleCnt | 0x08;
        printf("\n  Enable display Audio infomation\n");
    }

    if((tmp & 0x20) != 0)
    {
        gDebug_saveAudioData[0] = 0xFF;
        printf("\n  gDebug_saveAudioData open:\n");
    }

    if((tmp & 0x40) != 0)
    {
        g_displayOsd = TRUE;
        gDebug_OsdTest = TRUE;
        printf("\n  gDebug_osdTest open:\n");
    }

    if((tmp & 0x80) != 0)
    {
        g_openOnvif = TRUE;
        printf("\n  g_openOnvif open:\n");
    }

    if((tmp & 0x100) != 0)
    {
        g_s32OsdHandleCnt = g_s32OsdHandleCnt | 0x04;
        printf("\n  Enable display ISP infomation\n");
    }

    if((tmp & 0x200) != 0)
    {
        g_displayVideoInfo = TRUE;
        g_s32OsdHandleCnt = g_s32OsdHandleCnt | 0x02;
        printf("\n  Enable display Video infomation\n");
        g_displayOsd = TRUE;
    }

    if((tmp & 0x400) != 0)
    {
        g_displayOsd = TRUE;
        g_s32OsdFlicker = TRUE;
        printf("\n  g_s32OsdFlicker open\n");
    }

    if((tmp & 0x800) != 0)
    {
        g_s32OsdHandleCnt = g_s32OsdHandleCnt | 0x10;
        printf("\n  Enable display User0 infomation\n");
    }

    if((tmp & 0x1000) != 0)
    {
        g_s32OsdHandleCnt = g_s32OsdHandleCnt | 0x20;
        printf("\n  Enable display User1 infomation\n");
    }

    if((tmp & 0x4000) != 0)
    {
        g_displayOsd = TRUE;
        gDebug_osdColorInverse = TRUE;
        printf("\n  gDebug_osdColorInverse open\n");
    }

    if((tmp & 0x8000) != 0)
    {
        g_s32OsdHandleCnt = 8; //create 8 osd per video
        printf("\n  g_s32OsdHandleCnt = 8\n");
    }

    if((tmp & 0x10000) != 0)
    {
        gDebug_osdPrivateMask = TRUE;
        printf("\n  gDebug_osdPrivateMask open\n");
    }


    str = strtok(NULL, "_");
    if(NULL != str)
    {
        len = strlen(str);
        memset((char*)g_s8OsdFontPath, 0x00, sizeof(g_s8OsdFontPath));
        memcpy((char*)g_s8OsdFontPath, str, MIN(sizeof(g_s8OsdFontPath), len));
    }
}

inline void mixer_set_audioPath(void)
{
    MI_S32 audioIdx = 0;
    char  *str = strtok(optarg, "_");

    if(NULL == str)
    {
        printf("%s %d str is NULL\n", __func__, __LINE__);
        return;
    }

    audioIdx = mixerStr2Int(str);
    str = strtok(NULL, "\0");
    if(NULL == str) return;

    if((0 > audioIdx) || (audioIdx >= MIXER_AO_MAX_NUMBER))
    {
        printf("%s:%d set AudioOut Index error(%d)\n", __func__, __LINE__, audioIdx);
        exit(0);
    }

    memset(g_audioOutParam[audioIdx].s8AudioPath, 0x00, sizeof(g_audioOutParam[audioIdx].s8AudioPath));
    memcpy((char*)g_audioOutParam[audioIdx].s8AudioPath, str, strlen(str));
}

inline void mixer_set_video_RateCtlType(void)
{
    MI_S32 tmp = 0;
    char *str = strtok(optarg, "_");

    for(int i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        tmp = mixerStr2Int(str);
        if((RATECTLTYPE_CBR <= tmp) && (tmp < RATECTLTYPE_MAX))
        {
            g_videoParam[i].u8RateCtlType = tmp;
        }
        else
        {
            g_videoParam[i].u8RateCtlType = RATECTLTYPE_CBR;
        }
         if(g_videoParam[i].encoderType == VE_JPG || VE_MJPEG == g_videoParam[i].encoderType)
        {
          if(RATECTLTYPE_FIXQP != g_videoParam[i].u8RateCtlType)
          {
            printf("you set encodeType don't support m_rateCtlType=%d",g_videoParam[i].u8RateCtlType);
            g_videoParam[i].u8RateCtlType = RATECTLTYPE_FIXQP;
          }
        }
        printf("vencChn%d RateCtlType=%d \n", i, g_videoParam[i].u8RateCtlType);
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoFrameRate(void)
{
    MI_S32 tmp = 0;
    char *str = strtok(optarg, "_");

#if LOAD_MIXER_CFG
    //ModuleVencInfo_s *pVencInfo_t = GetVencInfo();
#endif

    for(int i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        //tmp = mixerStr2Int(str);
        tmp = mixerStrtok2Int(str, "/");

#if LOAD_MIXER_CFG
    #if 0
        if(tmp > pVencInfo_t->Fps[i])
        {
        tmp = pVencInfo_t->Fps[i];
        MIXER_DBG("please check you input param:%d![must[%d-%d]\n",tmp, pVencInfo_t->Minfps[i],pVencInfo_t->Fps[i]);

     }
     else if(tmp < pVencInfo_t->Minfps[i])
        {
        tmp = pVencInfo_t->Minfps[i];

          MIXER_DBG("please check you input param:%d![must[%d-%d]\n",tmp, pVencInfo_t->Minfps[i],pVencInfo_t->Fps[i]);
          //continue;
        }
     #endif
#endif
        if(0 < tmp)
        {
            g_videoParam[i].vencframeRate = tmp;
        }
        else
        {
            g_videoParam[i].vencframeRate = MIXER_DEFAULT_FPS;
        }

     if( g_videoParam[i].vencframeRate & 0xffff0000)
     {
         if((g_videoParam[i].vencframeRate & 0x0000ffff) != 0x0 )
         {
        g_videoParam[i].gop = (((g_videoParam[i].vencframeRate & 0x0000ffff))/ \
                         ((g_videoParam[i].vencframeRate & 0xffff0000) >> 16) + 1) * 2;
         }
        else
            g_videoParam[i].gop = MIXER_DEFAULT_FPS * 2;
     }
     else
     {
        g_videoParam[i].gop  = g_videoParam[i].vencframeRate * 2;
     }
        MIXER_DBG("vencChn%d frameRate=%d, gop:%d\n", i, g_videoParam[i].vencframeRate, g_videoParam[i].gop);
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoGop(void)
{
    printf("%s \n", optarg);
    MI_S32 tmp = 0;
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

    for( i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        tmp = mixerStr2Int(str);
        if(0 < tmp)
        {
            g_videoParam[i].gop = (unsigned int)tmp;
        }
        else
        {
            g_videoParam[i].gop = g_videoParam[i].vencframeRate * 2;
        }

        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoLTR(void)
{
    printf("%s \n", optarg);
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

    for( i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_videoParam[i].virtualIInterval = mixerStr2Int(str);
        if(*(str + strlen(str) - 1) == 'I')
        {
            g_videoParam[i].virtualIEnable = TRUE;
        }
        else if(*(str + strlen(str) - 1) == 'P')
        {
            g_videoParam[i].virtualIEnable = FALSE;
        }
        else
        {
            //default ref I
            g_videoParam[i].virtualIEnable = TRUE;
        }

        str = strtok(NULL, "_");
    }
}

inline void mixer_set_IspBinFilePath(void)
{
    MI_S32 len = 0;
    char *ptr = strtok(optarg, "_");


    if(NULL == ptr)
    {
        return;
    }
#if 1
    g_IspKey = mixerStr2Int(ptr);

    ptr = strtok(NULL, "\0");
    len = strlen(ptr);

    if(len > 0)
    {
        strcpy((char *)g_ISPBinFilePath, ptr);
    }
    else
    {
        printf("[Load bin error]: Invalid format, please set the arg by: -I key_path\n");
    }
#else

    len = strlen(ptr);

    if(len > 0)
    {
        strcpy((char *)g_ISPBinFilePath, ptr);
    }
    else
    {
        printf("[Load bin error]: Invalid format, please set the arg by: -I path_key\n");
    }
    ptr = strtok(NULL, "\0");
    g_IspKey = mixerStr2Int(ptr);


#endif

}

inline void mixer_set_VideoChnRoiConfig(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i  = 0x0;

    for( i = 0;str != NULL;i++)
    {
        g_roiParam[i] = atoi(str);
        str = strtok(NULL, "_");
        g_roi_num++;
    }
    g_roi_num = g_roi_num/9;
}


inline void mixer_set_AudioVideoTimestamp(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

    for( i = 0; i < 2 && str; i++)
    {
        if(0 == i)
        {
            g_bUseTimestampVideo = mixerStr2Int(str);
        }
        else if(1 == i)
        {
            g_bUseTimestampAudio = mixerStr2Int(str);
        }

        str = strtok(NULL, "_");
    }
}

inline void  mixer_set_VideoMaskOSD(void)
{
    char *str = strtok(optarg, "_");
    g_maskOsd = TRUE;
    MI_U32 i = 0x0;

    for(i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_maskNum[i] = atoi(str);
        str = strtok(NULL, "_");
    }
/*
    for(i = 0x0;i < MAX_VIDEO_NUMBER;i++)
    {
        g_maskNum[i] = 0;
    }
*/
}

#if TARGET_CHIP_I6E
inline void mixer_set_Ldc()
{
    MI_S32 len = 0;
    char *ptr = NULL;

    ptr = strtok(optarg, "\0");
    if(ptr == NULL)
    {
        printf("error: ptr is NULL!!!\n");
        return ;
    }

    len = strlen(ptr);

    if(len > 0)
    {
        printf("ldc cfg path:%s \n", ptr);
        mixerSetLdcBinPath(ptr);
        mixerSetLdcStatus(TRUE);
    }
    else
    {
        printf("error: Invalid ldc library cfg path!\n");
    }
}
#endif

inline void mixer_set_VideoRotation(MI_U32 *protation)
{
    MI_S32 tmp = mixerStr2Int(optarg);

    tmp %= 360;

    if((0 == tmp) || (90 == tmp) || (180 == tmp) || (270 == tmp))
    {
        *protation = 0xFF000000 + tmp;
    }
    else
    {
        *protation = 0xFF000000;
    }

    printf("\n  set Rotate %d\n", tmp);
}

inline void mixer_set_videoShareSize(void)
{
    MIXER_DBG("this firmware don't support setting shareSize\n");
}

inline void mixer_set_videoNumber(MI_U32 *pvideoNumber)
{
    MI_S32 tmp = mixerStr2Int(optarg);

    if(tmp > 0 && tmp <= MAX_VIDEO_NUMBER)
    {
        *pvideoNumber = tmp;
    }
    else
    {
        MIXER_ERR("video num is err. %d\n", tmp);
    }
}

inline void mixer_set_videoHdr3DNR(void)
{
    char *str = strtok(optarg, "_");
    MI_S32 u32Value = 0;
    MI_U32 i = 0x0;
/*
Level 0: 0 set;
Level 1: 1 set,  8-bit;
Level 2: 1 set, 10-bit;
Level 3: 1 set, 12-bit;
Level 4: 2 set,  8-bit,  8-bit;
Level 5: 2 set,  8-bit, 10-bit;
Level 6: 2 set,  8-bit, 12-bit;
Level 7: 2 set, 12-bit, 12-bit;
*/


    for(i = 0; i < 2 && str; i++)
    {
        switch(i)
        {
            case 0:
                u32Value = mixerStr2Int(str);
                if((E_MI_VPE_3DNR_LEVEL_OFF <= u32Value) && (u32Value < E_MI_VPE_3DNR_LEVEL_NUM))
                {
                    g_videoParam[0].stVifInfo.level3DNR = (char)u32Value;
                }
                else
                {
                    g_videoParam[0].stVifInfo.level3DNR = 3;
                }
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                if(E_MI_VPE_3DNR_LEVEL2 < g_videoParam[0].stVifInfo.level3DNR)
                {
                    g_videoParam[0].stVifInfo.level3DNR = E_MI_VPE_3DNR_LEVEL2;
                    printf("%s:%d for Infinity6 3DNR level only support %d(E_MI_VPE_3DNR_LEVEL2)\n", __func__, __LINE__, E_MI_VPE_3DNR_LEVEL2);
                }
#endif
                break;

            case 1:
                u32Value = mixerStr2Int(str);
                if((E_MI_VPE_HDR_TYPE_OFF < u32Value) && (u32Value < E_MI_VPE_HDR_TYPE_MAX))
                {
                    g_videoParam[0].stVifInfo.HdrType = (char)u32Value;
                    mixer_setHdrValue(TRUE);
                }
                else if(E_MI_VPE_HDR_TYPE_OFF == u32Value)
                {
                  mixer_setHdrValue(FALSE);
                }
                else
                {
                    printf("%s:%d HdrType must in [0, 5]\n", __func__, __LINE__);
                }
                break;
        }

        str = strtok(NULL, "_");
    }


    for( i = 1; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_videoParam[i].stVifInfo.HdrType = g_videoParam[0].stVifInfo.HdrType;
        g_videoParam[i].stVifInfo.level3DNR  = g_videoParam[0].stVifInfo.level3DNR;
    }

    if((mixer_GetHdrValue()) && (g_videoParam[0].stVifInfo.level3DNR))
    {
        printf("\n  HDR enable(HDR_type=%d), 3DNR enable(3DNR_level=%d)\n", g_videoParam[0].stVifInfo.HdrType, g_videoParam[0].stVifInfo.level3DNR);
    }
    else if((mixer_GetHdrValue()) && (0 == g_videoParam[0].stVifInfo.level3DNR))
    {
        printf("\n  HDR enable(HDR_type=%d), 3DNR disable\n", g_videoParam[0].stVifInfo.HdrType);
    }
    else if((FALSE == mixer_GetHdrValue()) && (g_videoParam[0].stVifInfo.level3DNR))
    {
        printf("\n  HDR disable, 3DNR enable(3DNR_level=%d)\n", g_videoParam[0].stVifInfo.level3DNR);
    }
    else if((FALSE == mixer_GetHdrValue()) && (0 == g_videoParam[0].stVifInfo.level3DNR))
    {
        printf("\n  HDR disable, 3DNR disable\n");
    }
}

inline void mixer_set_videoPIP(void)
{
    MI_S32 s32Value = 0;
    char *str = strtok(optarg, "_");


    for(MI_S32 i = 0; i < 4 && str; i++)
    {
        switch(i)
        {
            case 0 :
                s32Value = mixerStr2Int(str);
                if((0 <= s32Value) && (s32Value < (MI_S32)g_videoParam[0].width))
                {
                    g_videoParam[0].pipRectX = ALIGN_2xUP(s32Value);
                }
                else
                {
                    printf("%s:%d set PIP start X(=%d) error!\n", __func__, __LINE__, s32Value);
                }
                break;

            case 1 :
                s32Value = mixerStr2Int(str);
                if((0 <= s32Value) && (s32Value < (MI_S32)g_videoParam[0].height))
                {
                    g_videoParam[0].pipRectY = ALIGN_2xUP(s32Value);
                }
                else
                {
                    printf("%s:%d set PIP start Y(=%d) error!\n", __func__, __LINE__, s32Value);
                }
                break;

            case 2 :
                s32Value = mixerStr2Int(str);
                if((0 <= s32Value) && (s32Value <= (MI_S32)g_videoParam[0].width))
                {
                    g_videoParam[0].pipRectW = ALIGN_2xUP(s32Value);
                }
                else
                {
                    printf("%s:%d set PIP width(=%d) error!\n", __func__, __LINE__, s32Value);
                }
                break;

            case 3 :
                s32Value = mixerStr2Int(str);
                if((0 <= s32Value) && (s32Value < (MI_S32)g_videoParam[0].height))
                {
                    g_videoParam[0].pipRectH = ALIGN_2xUP(s32Value);
                }
                else
                {
                    printf("%s:%d set PIP height(=%d) error!\n", __func__, __LINE__, s32Value);
                }
                break;
            }

        str = strtok(NULL, "_");
    }

    if((g_videoParam[0].width  <= (g_videoParam[0].pipRectX + g_videoParam[0].pipRectW)) ||
       (g_videoParam[0].height <= (g_videoParam[0].pipRectY + g_videoParam[0].pipRectH)))
    {
        if((640 <= g_videoParam[0].width) && (480 <= g_videoParam[0].height))
        {
            printf("Set PIP param error, and re-set PIP param: (X:0 Y:0 Width:640 Hight:480)\n\n");
            g_videoParam[0].pipRectX = ALIGN_2xUP(0);
            g_videoParam[0].pipRectY = ALIGN_2xUP(0);
            g_videoParam[0].pipRectW = ALIGN_2xUP(640);
            g_videoParam[0].pipRectH = ALIGN_2xUP(480);
        }
        else
        {
            printf("Set PIP param error, and re-set PIP param: (X:0 Y:0 Width:320 Hight:240)\n\n");
            g_videoParam[0].pipRectX = ALIGN_2xUP(0);
            g_videoParam[0].pipRectY = ALIGN_2xUP(0);
            g_videoParam[0].pipRectW = ALIGN_2xUP(320);
            g_videoParam[0].pipRectH = ALIGN_2xUP(240);
        }
    }

    g_videoParam[0].pipCfg = 1;
    g_EnablePIP = 1;
}

inline void mixer_select_videoResolution(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

#if LOAD_MIXER_CFG
    ModuleVencInfo_s *pVencInfo_t = GetVencInfo();
#endif

    for( i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        if(getResolution(mixerStr2Int(str), &g_videoParam[i].width, &g_videoParam[i].height))
        {
            printf("%s:%d mixer set video%d resolution(%d * %d) error!\n", __func__, __LINE__,
                          i, g_videoParam[i].width, g_videoParam[i].height);
        }
#if LOAD_MIXER_CFG
       if(g_videoParam[i].width > pVencInfo_t->StreamWidth[i] || g_videoParam[i].height > pVencInfo_t->StreamHeight[i])
        {
           printf("please check chn[%d] you input param! streamWidth[must <= %d] streamHeight[must <= %d]\n",i,pVencInfo_t->StreamWidth[i],pVencInfo_t->StreamHeight[i]);

        }
#else
        if(g_videoParam[i].stVpeChnPort.u32PortId == 2 && (g_videoParam[i].width > 2688/* || g_videoParam[i].height > 1520*/))
        {
            printf("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n",g_videoParam[i].width,  g_videoParam[i].height);
            g_videoParam[i].width  = 2688;
            g_videoParam[i].height = 1520;
        }

        if(g_videoParam[i].stVpeChnPort.u32PortId == 1 && (g_videoParam[i].width > 2688/* ||  g_videoParam[i].height > 1520*/))
        {
            printf("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n",g_videoParam[i].width,  g_videoParam[i].height);
            g_videoParam[i].width = 2688;
            g_videoParam[i].height = 1520;
        }
#endif
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoResolution(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 width=0x0, height=0x0;

#if LOAD_MIXER_CFG
    ModuleVencInfo_s *pVencInfo_t = GetVencInfo();
#endif

    for(MI_S32 i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        width = mixerStr2Int(str);
        str = strtok(NULL, "_");
        height = mixerStr2Int(str);
        str = strtok(NULL, "_");
		if((0 == (g_rotation & 0xFFFF)) || (180 == (g_rotation & 0xFFFF)))
		{
	        if(2 == g_videoParam[i].stVpeChnPort.u32PortId && (width > VENC_ROTA0_MAX_W || height > VENC_ROTA0_MAX_H))
	        {
	            MIXER_DBG("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n", width, height);
	            break;
	        }
	        else if(1 == g_videoParam[i].stVpeChnPort.u32PortId && (width > VENC_ROTA0_MAX_W || height > VENC_ROTA0_MAX_H))
	        {
	            MIXER_DBG("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n", width, height);
	            break;
	        }
		}
		else
		{
		    if(2 == g_videoParam[i].stVpeChnPort.u32PortId  && (width > VENC_ROTA0_MAX_H || height > VENC_ROTA0_MAX_W))
	        {
	            MIXER_DBG("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n", width, height);
	            break;
	        }
	        else if(1 == g_videoParam[i].stVpeChnPort.u32PortId && (width > VENC_ROTA0_MAX_H || height > VENC_ROTA0_MAX_W))
	        {
	            MIXER_DBG("Over vpe portid maxsize 2688*2160, requst size:[%d:%d]\n", width, height);
	            break;
	        }
		}
#if LOAD_MIXER_CFG
     if((0 == (g_rotation & 0xFFFF)) || (180 == (g_rotation & 0xFFFF)))
     {
        if(width > pVencInfo_t->StreamMaxWidth[i] || height > pVencInfo_t->StreamMaxHeight[i])
        {
           MIXER_DBG("please check param! [width:%d must <= streamWidth:%d, height:%d must <= streamHeight:%d\n",\
                   width, pVencInfo_t->StreamWidth[i],\
                   height, pVencInfo_t->StreamHeight[i]);
        }
     }
	 else //rota90/270
	 {
	    if(width > pVencInfo_t->StreamMaxHeight[i] || height > pVencInfo_t->StreamMaxWidth[i])
        {
           MIXER_DBG("please check param! [width:%d must <= streamWidth:%d, height:%d must <= streamHeight:%d\n",\
                   width, pVencInfo_t->StreamWidth[i],\
                   height, pVencInfo_t->StreamHeight[i]);
        }
	 }
#endif
     /*
      I6e/I6b0          264/265  width >= 256  height >= 128
      I5/I6           264/265    width >= 192  height >= 128
      I5/I6/I6e/I6b0  Jpeg                  width >= 64  height >= 64
      ��rotate������撣行��獄�嚗�idth >= 256 height >= 256
      */
      if(width < 256)
      {
         MIXER_WARN("i:%d, width:%d < 256; set 256\n", i, width);
         width = 256;
      }
      if(height < 256)
      {
         MIXER_WARN("i:%d, height:%d < 256; set 256\n", i, height);
         height = 256;
      }

     if((90 == (g_rotation & 0xFFFF)) || (270 == (g_rotation & 0xFFFF)))
     {
         g_videoParam[i].width  = height;
         g_videoParam[i].height = width;
     }
     else
     {
        g_videoParam[i].width  = width;
        g_videoParam[i].height = height;
     }
     MIXER_DBG("after: i:%d, width:%d, height:%d\n", i, g_videoParam[i].width, g_videoParam[i].height);
    }

}

inline void mixer_set_sensorFrameRate(void)
{
    char *str = strtok(optarg, "_");
    float tmp = MIXER_DEFAULT_FPS;
    MI_U32 i = 0x0;

    if(str)
	{
        tmp = atof(str);
	}
    else
	{
        printf("please check you input param! str == NULL\n");
        tmp = MIXER_DEFAULT_FPS;
	}


#if LOAD_MIXER_CFG
    ModuleSensorInfo_s * pSensorInfo_t = GetSensorInfo();

   if(tmp < pSensorInfo_t->ResIndex[pSensorInfo_t->workResIndex].minFps || tmp > pSensorInfo_t->ResIndex[pSensorInfo_t->workResIndex].maxFps)
    {
       printf("please check you input param! [Sensor frameRate must in[%d-%d]\n",pSensorInfo_t->ResIndex[pSensorInfo_t->workResIndex].minFps,pSensorInfo_t->ResIndex[pSensorInfo_t->workResIndex].maxFps);
    }
#endif

    if(tmp < 1 || tmp > MIXER_MAX_FPS)
    {
        g_videoParam[0].stVifInfo.sensorFrameRate = MIXER_DEFAULT_FPS;
    }
    else
    {
        g_videoParam[0].stVifInfo.vifframeRate = tmp;
        g_videoParam[0].vpeframeRate = tmp;
        g_videoParam[0].stVifInfo.sensorFrameRate = tmp;
    }

    printf("sensor framerate %d\n", g_videoParam[0].stVifInfo.sensorFrameRate);

    for(i = 1; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_videoParam[i].stVifInfo.sensorFrameRate = g_videoParam[0].stVifInfo.sensorFrameRate;
        g_videoParam[i].stVifInfo.vifframeRate = g_videoParam[0].stVifInfo.vifframeRate;
        g_videoParam[i].vpeframeRate = g_videoParam[0].vpeframeRate;
    }
}

inline void mixer_enable_ShowFrameInterval(void)
{
    MI_S32 tmp = mixerStr2Int(optarg);

    if(0 == tmp)
    {
        g_ShowFrameInterval = FALSE;
    }
    else
    {
        g_ShowFrameInterval = TRUE;
    }
}

inline void mixer_set_videoEncoderType(void)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;

    for(i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_videoParam[i].encoderType = getVideoEncoderType(mixerStr2Int(str));
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoViChnNum(MI_S32 *pviChnParam, MI_S32 *pviParamNum)
{
    char *str = strtok(optarg, "_");
    MI_U32 i = 0x0;
    for(i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        pviChnParam[i] = mixerStr2Int(str);
        (*pviParamNum)++;
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_videoSaveStreamFlag(void)
{
    char *str = strtok(optarg, "_");
    //MI_S32 param[2]={0x0};

    for(MI_S32 i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        MI_S32 tmp = mixerStr2Int(str);

    tmp = !!tmp;
    //param[0] = i;
    //param[1] = tmp;

    tmpRecStatus[i].ch = i;
    tmpRecStatus[i].status = tmp;
    //mixer_send_cmd(CMD_RECORD_SETMODE, (MI_S8 *)param, sizeof(param));
        str = strtok(NULL, "_");
    }
}

inline void mixer_set_ircut(void)
{
    MI_S32 opt = mixerStr2Int(optarg);

    g_ircut = opt > 0 ? opt : 0;
}

inline void mixer_set_UvcResolution(void)
{
    MI_S32 opt = mixerStr2Int(optarg);

    printf("mixer : UVC Function Open");
    printf("mixer : default chnnel 0 resolution: 1920_1080");
    g_videoParam[0].width  = 1920;
    g_videoParam[0].height = 1080;
    g_openUVCDevice = TRUE;

    g_uvcParam = opt;
}

inline void mixer_set_ISPcusAe(MI_S32 * nCus3aispParam )
{
    char *str = strtok(optarg, "_");
    for(int i = 0; i < 3 && str; i++)
    {
        nCus3aispParam[i] = mixerStr2Int(str);
        str = strtok(NULL, "_");
    }

    if(nCus3aispParam[0] || nCus3aispParam[1] || nCus3aispParam[2])
    {
        //g_bCusAEenable = TRUE;
    }
    else
    {
        printf("ISPcus3A enable fail !!\n");
    }
}

inline void Mixer_Flush(void)
{
    char c;
    while ((c = ((char)getchar())) != '\n'&&c!=EOF);
}

inline void mixer_set_videoSed(void)
{
#if MIXER_SED_ENABLE
    g_EnableSed = 0;
    char* str = strtok(optarg, "_");
    for(MI_S32 i = 0; i < MAX_VIDEO_NUMBER && str; i++)
    {
        g_videoParam[i].u8SedEnable = (MI_U8)atoi(str);
        g_SedType = (MI_S32)atoi(str);
        MIXER_ERR("gSedType = %d\n", g_SedType);
        if(g_videoParam[i].u8SedEnable >= 0)
        {
            g_EnableSed = 1;
            g_videoParam[i].u8SedEnable = 1;
        }
        str = strtok(NULL, "_");
    }
#else
    printf("not support videSed!!\n");
#endif

}

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
inline void mixer_set_videoBindMode(void)
{
    char *str = strtok(optarg, "_");
    MI_S32 tmp;

    for(MI_S32 i = 0; str && (i < MAX_VIDEO_NUMBER); i++)
    {
        tmp = mixerStr2Int(str);
        switch (tmp)
        {
            case 0:  g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_FRAME;    break;
            case 1:  g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_REALTIME; break;
            case 2:  g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_HW_RING;  break;
#if TARGET_CHIP_I6B0
            case 3:     g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_HW_HALF_RING;  break;
#endif
            default: g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_FRAME;    break;
        }
#if TARGET_CHIP_I6E
        if(VE_JPG == g_videoParam[i].encoderType || VE_MJPEG == g_videoParam[i].encoderType)
        {
            if((0 != tmp)&&(1 != tmp)&&(0 == g_videoParam[i].FromPort))
            {
                MIXER_WARN("I6E port0 no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_REALTIME);
                g_videoParam[i].eBindMode= Mixer_Venc_Bind_Mode_REALTIME;
            }
        }
        else
        {
            if((0 != tmp)&&(0 != g_videoParam[i].FromPort))
            {
                MIXER_WARN("I6E port1-2 no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_FRAME);
                g_videoParam[i].eBindMode= Mixer_Venc_Bind_Mode_FRAME;
            }
        }
#elif TARGET_CHIP_I6B0
        if(VE_JPG == g_videoParam[i].encoderType || VE_MJPEG == g_videoParam[i].encoderType)
        {
          if((0 != tmp)&&(1 != tmp)&&(0 == g_videoParam[i].FromPort))
          {
             MIXER_WARN("I6B0 port0 no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_REALTIME);
             g_videoParam[i].eBindMode= Mixer_Venc_Bind_Mode_REALTIME;
          }
        }
        if((2==tmp||3==tmp) && (g_videoParam[i].FromPort!=1))
        {
            MIXER_WARN("I6B0 port%d no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].FromPort,g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_FRAME);
        }
#elif TARGET_CHIP_I6
            if(VE_JPG == g_videoParam[i].encoderType || VE_MJPEG == g_videoParam[i].encoderType)
            {
              if((0 != tmp)&&(1 != tmp)&&(0 == g_videoParam[i].FromPort))
              {
                 MIXER_WARN("I6B0 port0 no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_REALTIME);
                 g_videoParam[i].eBindMode= Mixer_Venc_Bind_Mode_REALTIME;
              }
            }
            if((2==tmp) && (g_videoParam[i].FromPort!=1))
            {
                MIXER_WARN("I6B0 port%d no support this bindMoudle[%d] set def bindMoudle[%d]\n",g_videoParam[i].FromPort,g_videoParam[i].eBindMode,Mixer_Venc_Bind_Mode_FRAME);
            }

#endif
        str = strtok(NULL, "_");
        MIXER_INFO("video %d, venc bind mode  = %d\n", i, g_videoParam[i].eBindMode);


    }
}
#endif

#if TARGET_CHIP_I5
MI_S32 mixerSetViChn(MixerVideoParam *pDstParam, MI_S32 videoNum, MI_S32 *viParam, MI_S32 viParamLen, MI_S32 ieViChn)
{
    MI_U8 u8JpgEnable = 0;
    MI_U8 u8VpePort3Enable = 0;
    MI_U8 u8VpePort1Enable = 0;
    MI_U32 u32MaxWidth = 0;
    MI_U8 u8MaxWidthIndex = 0;
    MI_U8 u8VpeChn0Port2 = 2;
    MI_S32 i = 0, j = 0, videoStreamNum = 0;
    MI_S32 maxVideoStream = MAX_VIDEO_STREAM, maxImageStream = 1;
    MI_S32 u32Size4KNum = 0;

    if(viParamLen > 0 && videoNum != viParamLen)
    {
        printf("    -V must set the same vi channel count with -n\n");
        printf("    for example -n 3 -V 0_3_4\n");
        return -1;
    }

    if(viParamLen > 0)
    {
        for(i = 0; i < viParamLen; i++)
        {
            for(j = i + 1; j < viParamLen; j++)
            {
                if(viParam[i] == viParam[j])
                {
                    printf("-V set video%d and video%d share the same vpe port%d!\n", i, j, viParam[i]);
                    if(g_displayOsd)
                    {
                        MI_S32 s32DivpEnable = 1;
                        pDstParam[i].s8DivpEnable = (char)s32DivpEnable;
                        pDstParam[j].s8DivpEnable = (char)s32DivpEnable;
                    }
                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            pDstParam[i].stVpeChnPort.u32PortId = viParam[i];
            if((g_rotation & 0xFF000000))
            {
                pDstParam[i].stVpeChnPort.u32ChnId = 1;
            }

            if((TRUE == g_bBiLinear) && ((pDstParam[i].width <= 720) || (pDstParam[i].height <= 576)))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

            if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

            pDstParam[i].viChnStatus = VI_ENABLE;
            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }
    else
    {
        if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
        {
            videoStreamNum++;
        }

        for(i = 0; i < videoNum; i++)
        {
            //step1:check jpg stream
            if(VE_JPG == pDstParam[i].encoderType)
            {
                if(0 == u8JpgEnable)
                {
                    pDstParam[i].stVpeChnPort.u32PortId = g_jpegViChn;
                    u8JpgEnable = 1;
                }
                else
                {
                    printf("[err] %s you have set multiple JPG stream!\n", __func__);
                    //exit(1);
                }
            }
            //step2:check 4k stream
            else if((ALIGN_2xUP(1920) < pDstParam[i].width) || (ALIGN_2xUP(1080) < pDstParam[i].height))
            {
                if(VE_JPG != pDstParam[i].encoderType)
                {
                    u32Size4KNum ++;
                }

                if(u32Size4KNum > 1)
                {
                    printf("[err] %s you have set multiple 4K stream!\n", __func__);
                    exit(1);
                }
            }
            //step3:check max size stream
            if(VE_JPG != pDstParam[i].encoderType)
            {
                if(u32MaxWidth < pDstParam[i].width)
                {
                    u32MaxWidth = pDstParam[i].width;
                    u8MaxWidthIndex = i;
                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            if(0 == u8JpgEnable)
            {
                if(u8MaxWidthIndex == i)
                {
                    pDstParam[i].stVpeChnPort.u32PortId = 0;
                    videoStreamNum++;
                }
                else if(!u8VpePort1Enable )
                {
                    pDstParam[i].stVpeChnPort.u32PortId = 1;
                    videoStreamNum++;
                    u8VpePort1Enable = 1;
                }
                else if(!u8VpePort3Enable )
                {
                    pDstParam[i].stVpeChnPort.u32PortId = 3;
                    videoStreamNum++;
                    u8VpePort3Enable = 1;
                }
                else
                {
                    pDstParam[i].stVpeChnPort.u32PortId = u8VpeChn0Port2;
                    videoStreamNum++;
                }
            }
            else
            {
                if(u8MaxWidthIndex == i)
                {
                    pDstParam[i].stVpeChnPort.u32PortId = 0;
                    videoStreamNum++;
                }
                else if(VE_JPG == pDstParam[i].encoderType && !u8VpePort3Enable)
                {
                    pDstParam[i].stVpeChnPort.u32PortId = g_jpegViChn;
                    videoStreamNum++;
                    u8VpePort3Enable = 1;
                }
                else if(!u8VpePort1Enable )
                {
                    pDstParam[i].stVpeChnPort.u32PortId = 1;
                    videoStreamNum++;
                    u8VpePort1Enable = 1;
                }
                else
                {
                    pDstParam[i].stVpeChnPort.u32PortId = u8VpeChn0Port2;
                    videoStreamNum++;
                }
            }

            if((g_rotation & 0xFF000000))
            {
                pDstParam[i].stVpeChnPort.u32ChnId = 1;
            }

            if(((pDstParam[i].width <= 720) || (pDstParam[i].height <= 576)) &&
               (TRUE == g_bBiLinear) && (VE_TYPE_MAX != g_videoParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

           if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
           {
               pDstParam[i].s8DivpEnable = 1;
           }

            pDstParam[i].viChnStatus = VI_ENABLE;
            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }

        if(videoStreamNum > maxVideoStream)
        {
            printf("only support %d route video stream, and %d route image stream \n", maxVideoStream, maxImageStream);
            return -1;
        }
    }

    return 0;
}

#elif TARGET_CHIP_I6
MI_S32 mixerSetViChn(MixerVideoParam *pDstParam, MI_S32 videoNum, MI_S32 *viParam, MI_S32 viParamLen, MI_S32 ieViChn)
{
    MI_S32 i = 0, j = 0, ieStreamNum = 0, jpegEnable = 0, curViChn = 1;
    MI_S32 maxVideoStream = MAX_VIDEO_STREAM;
    ModuleVencInfo_s *VencInfo_t = GetVencInfo();
    if(viParamLen > 0 && videoNum != viParamLen)
    {
        printf("    -V must set the same vi channel count with -n\n");
        printf("    for example -n 3 -V 0_3_4\n");
        return -1;
    }

    printf("%s viParamLen = %d\n", __func__, viParamLen);
    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_hchdParam || g_dlaParam)
    {
        ieStreamNum = 1;
    }

    if((ieStreamNum + videoNum) > maxVideoStream)
    {
        printf("only support more than %d route video stream\n", maxVideoStream);
        return -1;
    }

    if(viParamLen > 0)
    {
        for(i = 0; i < viParamLen; i++)
        {
            for(j = i + 1; j < viParamLen; j++)
            {
                if(viParam[i] == viParam[j])
                {
                    printf("-V set video%d and video%d share the same vpe port%d!\n", i, j, viParam[i]);
                    if(g_displayOsd)
                    {
                        MI_S32 s32DivpEnable = 1;
                        pDstParam[i].s8DivpEnable = (char)s32DivpEnable;
                        pDstParam[j].s8DivpEnable = (char)s32DivpEnable;
                    }
                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            pDstParam[i].stVpeChnPort.u32PortId = viParam[i];
            pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].stVpeChnPort.u32PortId);

            if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }
    else
    {
        for(i = 0; i < videoNum; i++)
        {
            if((0 == jpegEnable) && ((VE_JPG == pDstParam[i].encoderType) || (VE_MJPEG == pDstParam[i].encoderType)))
            {
                jpegEnable = 1;
                pDstParam[i].stVpeChnPort.u32PortId = g_jpegViChn;
                if(Mixer_Venc_Bind_Mode_NUM == pDstParam[i].eBindMode)
                {
                    pDstParam[i].eBindMode = Mixer_Venc_Bind_Mode_REALTIME;
                }
            }
            else
            {
                pDstParam[i].stVpeChnPort.u32PortId = curViChn;
                if((1 == curViChn) && (Mixer_Venc_Bind_Mode_NUM == pDstParam[i].eBindMode))
                {
                    pDstParam[i].eBindMode = Mixer_Venc_Bind_Mode_HW_RING;
                }
                else if((2 <= curViChn) && (Mixer_Venc_Bind_Mode_NUM == pDstParam[i].eBindMode))
                {
                    pDstParam[i].eBindMode = Mixer_Venc_Bind_Mode_FRAME;

                    //if(g_displayOsd)            //same vpe port support over two stream need divp
                    {
                        pDstParam[i].s8DivpEnable = 1;
                    }
                }

                curViChn++;
                if(2 < curViChn)    //reserved vi2 because it without divp
                {
                    //if(g_displayOsd)
                    {
                        pDstParam[i].s8DivpEnable = 1;
                    }
                }
            }
            VencInfo_t->BindModuleType[i] = pDstParam[i].eBindMode;//肮祭饜离恅璃陓洘
            if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

            pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].stVpeChnPort.u32PortId);
            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }

    return 0;
}

inline void mixer_set_videoPixelFormat(void)
{
    char *str = strtok(optarg, "_");

    for(MI_S32 i = 0; str && (i < MAX_VIDEO_NUMBER); i++)
    {
        g_videoParam[i].ePixelFormat = (MI_SYS_PixelFormat_e)mixerStr2Int(str);
        str = strtok(NULL, "_");
    }
}

void mixer_Set_BindMode(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    for(MI_S32 i = 0; i < videoNum; i++)
    {
        if(E_MI_SYS_PIXEL_FRAME_FORMAT_MAX == g_videoParam[i].ePixelFormat)
        {
            if(Mixer_Venc_Bind_Mode_REALTIME == g_videoParam[i].eBindMode)
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            }
            else
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            }
        }
    }
}

MI_S32 mixer_Check_FrameRate(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    MI_U32 den = 0, num = 0;

    for(MI_S32 i = 0; i < videoNum; i++)
    {
        den = g_videoParam[i].vencframeRate & 0xffff0000 >> 16;
        num = g_videoParam[i].vencframeRate & 0x0000ffff;

        if((g_videoParam[i].encoderType == VE_JPG) && (g_videoParam[i].eBindMode == Mixer_Venc_Bind_Mode_REALTIME))
        {
            /*In I6,IMI JPEG most framerate is half of sensor framerate*/
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num * 2)) || \
               ((den == 0) && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate < num * 2)))
            {

                printf("%s IMI JPEG most framerate is half of sensor framerate\n",__func__);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate >> 1;
                printf("%s set jpeg framerate %d\n",__func__, g_videoParam[i].vencframeRate);
            }
        }
        else
        {
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num)) || \
                ((den == 0) && (((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate) < num)))
            {
                printf("%s video %d framerate must samller than sensor framerate\n",__func__,i);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
                printf("%s set video %d framerate %d\n", __func__, i, g_videoParam[i].vencframeRate);
            }
        }
    }

    return 0;
}
#elif  TARGET_CHIP_I6E
MI_S32 mixerSetViChn(MixerVideoParam *pDstParam, MI_S32 videoNum, MI_S32 *viParam, MI_S32 viParamLen, MI_S32 ieViChn)
{
    MI_S32 i = 0, j = 0;//ieStreamNum = 0;
    MI_S32 maxVideoStream = MAX_VIDEO_STREAM;

    if(viParamLen > 0 && videoNum != viParamLen)
    {
        printf("    -V must set the same vi channel count with -n\n");
        printf("    for example -n 3 -V 0_3_4\n");
        return -1;
    }

    printf("%s viParamLen = %d\n", __func__, viParamLen);
/*
    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_hchdParam || g_dlaParam)
    {
        ieStreamNum = 2;
    }
*/

    if(videoNum > maxVideoStream)
    {
        printf("only support more than %d route video stream\n", maxVideoStream);
        return -1;
    }

    if(viParamLen > 0)
    {
        for(i = 0; i < viParamLen; i++)
        {
            for(j = i + 1; j < viParamLen; j++)
            {
                if(viParam[i] == viParam[j])
                {
                    printf("-V set video%d and video%d share the same vpe port%d!\n", i, j, viParam[i]);
                    if(g_displayOsd)
                    {
                        MI_S32 s32DivpEnable = 1;
                        pDstParam[i].s8DivpEnable = (char)s32DivpEnable;
                        pDstParam[j].s8DivpEnable = (char)s32DivpEnable;
                    }
                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            pDstParam[i].stVpeChnPort.u32PortId = viParam[i];
            pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].stVpeChnPort.u32PortId);

            if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }

            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }
    else
    {
        for(i = 0; i < videoNum; i++)
        {
            if(VPE_MAX_PORT <= pDstParam[i].FromPort)
            {
              pDstParam[i].s8DivpEnable = 1;
              pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].FromPort);//VPE_MAX_PORT-1;
            }
            if((g_displayOsd || gDebug_osdColorInverse) && (pDstParam[i].stVpeChnPort.u32PortId == 2) && (VE_TYPE_MAX != pDstParam[i].encoderType))
            {
                pDstParam[i].s8DivpEnable = 1;
            }
            printf("%s:%d VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n", __func__, __LINE__, i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }

    return 0;
}

inline void mixer_set_videoPixelFormat(void)
{
    char *str = strtok(optarg, "_");

    for(MI_S32 i = 0; str && (i < MAX_VIDEO_NUMBER); i++)
    {
        g_videoParam[i].ePixelFormat = (MI_SYS_PixelFormat_e)mixerStr2Int(str);
        str = strtok(NULL, "_");
    }
}
void mixer_Set_BindMode(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    for(MI_S32 i = 0; i < videoNum; i++)
    {
        if(E_MI_SYS_PIXEL_FRAME_FORMAT_MAX == g_videoParam[i].ePixelFormat)
        {
            if(Mixer_Venc_Bind_Mode_REALTIME == g_videoParam[i].eBindMode)
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            }
            else
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            }
        }
    }
}

MI_S32 mixer_Check_FrameRate(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    MI_U32 den = 0, num = 0;

    for(MI_S32 i = 0; i < videoNum; i++)
    {
        den = g_videoParam[i].vencframeRate & 0xffff0000 >> 16;
        num = g_videoParam[i].vencframeRate & 0x0000ffff;

        if((g_videoParam[i].encoderType == VE_JPG) && (g_videoParam[i].eBindMode == Mixer_Venc_Bind_Mode_REALTIME))
        {
            /*In I6,IMI JPEG most framerate is half of sensor framerate*/
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num * 2)) || \
               ((den == 0) && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate < num * 2)))
            {

                printf("%s IMI JPEG most framerate is half of sensor framerate\n",__func__);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate >> 1;
                printf("%s set jpeg framerate %d\n",__func__, g_videoParam[i].vencframeRate);
            }
        }
        else
        {
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num)) || \
                ((den == 0) && (((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate) < num)))
            {
                printf("%s video %d framerate:%d must samller than sensor framerate(%d)\n",__func__,i, num, g_videoParam[0].stVifInfo.sensorFrameRate);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
                printf("%s set video %d framerate %d\n", __func__, i, g_videoParam[i].vencframeRate);
            }
        }
    }

    return 0;
}

#elif TARGET_CHIP_I6B0
MI_S32 mixerSetViChn(MixerVideoParam *pDstParam, MI_S32 videoNum, MI_S32 *viParam, MI_S32 viParamLen, MI_S32 ieViChn)
{
    MI_S32 i = 0, j = 0,ieStreamNum = 0;
    MI_S32 maxVideoStream = MAX_VIDEO_STREAM;
    MI_BOOL needFrameDivp = FALSE,needREALDivp = FALSE;

    if(viParamLen > 0 && videoNum != viParamLen)
    {
        printf("    -V must set the same vi channel count with -n\n");
        printf("    for example -n 3 -V 0_3_4\n");
        return -1;
    }

    printf("%s viParamLen = %d\n", __func__, viParamLen);

    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_hchdParam)
    {
        ieStreamNum = 1;
    }


    if((videoNum+ieStreamNum) > maxVideoStream)
    {
        printf("only support more than %d route video stream\n", maxVideoStream);
        return -1;
    }

    if(viParamLen > 0)
    {
        for(i = 0; i < viParamLen; i++)
        {
            for(j = i + 1; j < viParamLen; j++)
            {
                if(viParam[i] == viParam[j])
                {
                    printf("-V set video%d and video%d share the same vpe port%d!\n", i, j, viParam[i]);

                    if(pDstParam[i].width != pDstParam[j].width || pDstParam[i].height != pDstParam[j].height)
                    {
                        if(viParam[i] == VPE_REALMODE_SUB_PORT)
                        {
                            MIXER_ERR("real mode divp cannon carry different resolution output.\n");
                            return -1;
                        }
                        else
                        {
                            MI_S32 s32DivpEnable = 1;
                            pDstParam[i].s8DivpEnable = (char)s32DivpEnable;
                            pDstParam[j].s8DivpEnable = (char)s32DivpEnable;
                            needFrameDivp = TRUE;
                            MIXER_DBG("different resolution from same vpe port ,need frame mode divp.\n");
                        }

                    }

                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            pDstParam[i].FromPort = viParam[i];
            pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].FromPort);
            if(VPE_MAX_PORT <= pDstParam[i].FromPort)
            {
              pDstParam[i].s8DivpEnable = 1;
              needFrameDivp = TRUE;
            }
            if(pDstParam[i].stVpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
            {
                pDstParam[i].s8DivpEnable = 1;
                needREALDivp = TRUE;
            }
            MIXER_DBG("VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n",i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }

    }
    else
    {
        for(i = 0; i < videoNum; i++)
        {
            for(j = i + 1; j < videoNum; j++)
            {
                if(pDstParam[i].FromPort == pDstParam[j].FromPort)
                {
                    printf("-V set video%d and video%d share the same vpe port%d!\n", i, j, pDstParam[i].FromPort);

                    if(pDstParam[i].width != pDstParam[j].width || pDstParam[i].height != pDstParam[j].height)
                    {
                        if(pDstParam[i].FromPort == VPE_REALMODE_SUB_PORT)
                        {
                            MIXER_ERR("real mode divp cannon carry different resolution output.\n");
                            return -1;
                        }
                        else
                        {
                            MI_S32 s32DivpEnable = 1;
                            pDstParam[i].s8DivpEnable = (char)s32DivpEnable;
                            pDstParam[j].s8DivpEnable = (char)s32DivpEnable;
                            needFrameDivp = TRUE;
                            MIXER_DBG("different resolution from same vpe port ,need frame mode divp.\n");
                        }

                    }

                }
            }
        }

        for(i = 0; i < videoNum; i++)
        {
            pDstParam[i].stVpeChnPort.u32PortId = Mixer_coverVi2Vpe(pDstParam[i].FromPort);//VPE_MAX_PORT-1;
            if(VPE_MAX_PORT <= pDstParam[i].FromPort)
            {
              pDstParam[i].s8DivpEnable = 1;
              needFrameDivp = TRUE;
            }
            if(pDstParam[i].stVpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
            {
                pDstParam[i].s8DivpEnable = 1;
                needREALDivp = TRUE;
            }
            MIXER_DBG("VideoNo.=%d, VpeChnId=%d, VpePortId=%d, DivpEnable=%d, VencType=%d, width=%4d, height=%4d\n",i,
                    (0 == (g_rotation & 0xFF000000))? pDstParam[i].stVpeChnPort.u32ChnId : (pDstParam[i].stVpeChnPort.u32ChnId + 1),
                    pDstParam[i].stVpeChnPort.u32PortId, pDstParam[i].s8DivpEnable, pDstParam[i].encoderType, pDstParam[i].width, pDstParam[i].height);
        }
    }

    //protect -V FOR REAL DIVP
    if(needREALDivp==TRUE && ieStreamNum==1)
    {
        MIXER_ERR("not support IE and real divp at the same time,because VDF use real DIVP!\n");
        return -1;
    }
    if(needFrameDivp==TRUE && needREALDivp==TRUE)
    {
        MIXER_ERR("real mode divp cannon exist with frame mode divp.\n");
        return -1;
    }
    return 0;
}

inline void mixer_set_videoPixelFormat(void)
{
    char *str = strtok(optarg, "_");

    for(MI_S32 i = 0; str && (i < MAX_VIDEO_NUMBER); i++)
    {
        g_videoParam[i].ePixelFormat = (MI_SYS_PixelFormat_e)mixerStr2Int(str);
        str = strtok(NULL, "_");
    }
}
void mixer_Set_BindMode(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    for(MI_S32 i = 0; i < videoNum; i++)
    {
        if(g_videoParam[i].FromPort == VPE_REALMODE_SUB_PORT)
        {
            g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_FRAME;// venc bind frame mode with real time mode DIVP.
        }
    }
    for(MI_S32 i = 0; i < videoNum; i++)
    {
        if(E_MI_SYS_PIXEL_FRAME_FORMAT_MAX == g_videoParam[i].ePixelFormat)
        {
            if(Mixer_Venc_Bind_Mode_REALTIME == g_videoParam[i].eBindMode)
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            }
            else
            {
                g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            }
        }
    }
}

MI_S32 mixer_Check_FrameRate(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    MI_U32 den = 0, num = 0;

    for(MI_S32 i = 0; i < videoNum; i++)
    {
        den = g_videoParam[i].vencframeRate & 0xffff0000 >> 16;
        num = g_videoParam[i].vencframeRate & 0x0000ffff;

        if((g_videoParam[i].encoderType == VE_JPG) && (g_videoParam[i].eBindMode == Mixer_Venc_Bind_Mode_REALTIME))
        {
            /*In I6,IMI JPEG most framerate is half of sensor framerate*/
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num * 2)) || \
               ((den == 0) && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate < num * 2)))
            {

                printf("%s IMI JPEG most framerate is half of sensor framerate\n",__func__);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate >> 1;
                printf("%s set jpeg framerate %d\n",__func__, g_videoParam[i].vencframeRate);
            }
        }
        else
        {
            if((den && ((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate * den < num)) || \
                ((den == 0) && (((MI_U32)g_videoParam[0].stVifInfo.sensorFrameRate) < num)))
            {
                printf("%s video %d framerate:%d must samller than sensor framerate(%d)\n",__func__,i, num, g_videoParam[0].stVifInfo.sensorFrameRate);
                g_videoParam[i].vencframeRate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
                printf("%s set video %d framerate %d\n", __func__, i, g_videoParam[i].vencframeRate);
            }
        }
    }

    return 0;
}

#endif

#if 0
inline void mixer_init_videoAudioParam(void)
{
    MI_S16 s16CompressionRatioInput[5]  = {-80, -60, -40, -25, 0};
    MI_S16 s16CompressionRatioOutput[5] = {-80, -30, -15, -10, -3};
    MI_U32 U32AecSupfreq[6] = {4, 6, 36, 49, 50, 51};
    MI_U32 U32AecSupIntensity[7] = {5, 4, 4, 5, 10, 10, 10};
    MI_S32 i = 0;

    memset(&g_videoParam, 0x00, sizeof(MixerVideoParam) * MAX_VIDEO_NUMBER);

    for(i = 0; i < MAX_VIDEO_NUMBER; i++)
    {
        //
    tmpRecStatus[i].ch = -1;
    tmpRecStatus[i].status = -1;
    //end

        g_videoParam[i].vifBufUsrDepth = 0;
        g_videoParam[i].vifBufCntQuota = 0;
        g_videoParam[i].vpeBufUsrDepth = 0;    // user no need set buffer
        g_videoParam[i].vpeBufCntQuota = 4;
        g_videoParam[i].vencBufUsrDepth = 4;
        g_videoParam[i].vencBufCntQuota = 4;
        g_videoParam[i].divpBufUsrDepth = 0;
        g_videoParam[i].divpBufCntQuota = 3;

        g_videoParam[i].vifframeRate = 30;
        g_videoParam[i].vpeframeRate = 30;
        g_videoParam[i].vencframeRate = (char)MIXER_DEFAULT_FPS;
        g_videoParam[i].sensorFrameRate = (float)MIXER_DEFAULT_FPS;
        g_videoParam[i].gop = g_videoParam[i].vencframeRate * 2;
        g_videoParam[i].maxQp = 48;
        g_videoParam[i].minQp = 12;
        g_videoParam[i].maxIQp = 48;  //[10 48]
        g_videoParam[i].minIQp = 25;  //(10 48)
        g_videoParam[i].maxPQp = 48;  //[10 48)
        g_videoParam[i].minPQp = 25;  //(10 48]
        g_videoParam[i].u8MaxQfactor = 90;
        g_videoParam[i].u8MinQfactor = 20;
        g_videoParam[i].s8Qfactor = 50;
        g_videoParam[i].IPQPDelta = 0;
        g_videoParam[i].u8ChangePos = 50;  //[50 100]
        g_videoParam[i].encoderType = VE_TYPE_MAX;
        g_videoParam[i].u8RateCtlType = RATECTLTYPE_MAX;
        g_videoParam[i].saveStreamFlag = FALSE;
        g_videoParam[i].virtualIInterval = 0;
        g_videoParam[i].virtualIEnable = FALSE;
        g_videoParam[i].viChnStatus = VI_DISABLE;
        g_videoParam[i].s8DivpEnable = 0;

        g_videoParam[i].stVifChnPort.eModId = E_MI_MODULE_ID_VIF;
        g_videoParam[i].stVifChnPort.u32DevId = 0;
        g_videoParam[i].stVifChnPort.u32ChnId = 0;
        g_videoParam[i].stVifChnPort.u32PortId = 0;
        g_videoParam[i].stVpeChnPort.eModId = E_MI_MODULE_ID_VPE;
        g_videoParam[i].stVpeChnPort.u32DevId = 0;
        g_videoParam[i].stVpeChnPort.u32ChnId = 0;
        g_videoParam[i].u16VpeOutWidth  = 0;
        g_videoParam[i].u16VpeOutHeight = 0;

#if TARGET_CHIP_I6
        g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_NUM;
        g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX;
#elif TARGET_CHIP_I6E
    g_videoParam[i].eBindMode = Mixer_Venc_Bind_Mode_NUM;
       g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX;
#endif

        if(i == 0)
        {
            g_videoParam[i].width  = 1920;
            g_videoParam[i].height = 1080;
            g_videoParam[i].bitrate = 2000000;
#if TARGET_CHIP_I5
            g_videoParam[0].level3DNR  = 3; //enable 3DNR : 1-pass, 12-bit
#elif TARGET_CHIP_I6
            g_videoParam[0].level3DNR  = 2; //enable 3DNR : 1-pass, 12-bit
#elif TARGET_CHIP_I6E
            g_videoParam[0].level3DNR  = 2; //enable 3DNR : 1-pass, 12-bit
#endif
            g_videoParam[0].HdrType = 0;    //disable HDR
        }
        else
        {
#if TARGET_CHIP_I5
            g_videoParam[i].width = 1280;
            g_videoParam[i].height = 720;
#elif TARGET_CHIP_I6
            g_videoParam[i].width = 704;
            g_videoParam[i].height = 576;
#elif TARGET_CHIP_I6E
            g_videoParam[i].width = 704;
            g_videoParam[i].height = 576;
#endif
            g_videoParam[i].bitrate = 1000000;
#if TARGET_CHIP_I5
            g_videoParam[i].level3DNR  = 3; //enable 3DNR : 1-pass, 12-bit
#elif TARGET_CHIP_I6
            g_videoParam[i].level3DNR  = 2; //enable 3DNR : 1-pass, 12-bit
#elif TARGET_CHIP_I6E
            g_videoParam[i].level3DNR  = 2; //enable 3DNR : 1-pass, 12-bit
#endif
            g_videoParam[i].HdrType = 0;    //disable HDR
        }
    }

    memset(&g_stAiVqeCfg, 0x00, sizeof(MI_AI_VqeConfig_t));
    memset(&g_audioInParam, 0x00, sizeof(MixerAudioInParam) * MIXER_AI_MAX_NUMBER);
    for(i = 0; i < MIXER_AI_MAX_NUMBER; i++)
    {
        g_audioInParam[i].stAudioInChnPort.eModId    = E_MI_MODULE_ID_AI;
        g_audioInParam[i].stAudioInChnPort.u32DevId  = 0; //0:AMIC[2chn]; 1:DMIC[4chn]; 2:I2S[8chn]; default:0
        g_audioInParam[i].stAudioInChnPort.u32ChnId  = 0;
        g_audioInParam[i].stAudioInChnPort.u32PortId = 0;

        g_audioInParam[i].AiMediaType = MT_G711A;
        g_audioInParam[i].u8AudioInNum = 0;
        g_audioInParam[i].s32VolumeInDb = 5;
        g_audioInParam[i].u8UserFrameDepth = 9;
        g_audioInParam[i].u8BufQueueDepth  = 10;
        g_audioInParam[i].eAudioInReSampleRate = E_MI_AUDIO_SAMPLE_RATE_INVALID;

        g_audioInParam[i].stAudioAttr.eBitwidth   = E_MI_AUDIO_BIT_WIDTH_16;
        g_audioInParam[i].stAudioAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
        g_audioInParam[i].stAudioAttr.eWorkmode   = E_MI_AUDIO_MODE_I2S_MASTER;
        g_audioInParam[i].stAudioAttr.eSoundmode  = E_MI_AUDIO_SOUND_MODE_MONO;
        g_audioInParam[i].stAudioAttr.u32FrmNum   = 16;  //must >=5
        g_audioInParam[i].stAudioAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
        g_audioInParam[i].stAudioAttr.u32ChnCnt   = 0;

        if(0 == i)
        {
            g_stAiVqeCfg.bHpfOpen = FALSE;
            g_stAiVqeCfg.bAnrOpen = FALSE;
            g_stAiVqeCfg.bAgcOpen = FALSE;
            g_stAiVqeCfg.bEqOpen  = FALSE;
            g_stAiVqeCfg.bAecOpen = FALSE;

            g_stAiVqeCfg.s32FrameSample = 128;
            g_stAiVqeCfg.s32WorkSampleRate = g_audioInParam[i].stAudioAttr.eSamplerate;
            g_stAiVqeCfg.u32ChnNum = 0;

            //AiVqeCfg.Hpf
            g_stAiVqeCfg.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAiVqeCfg.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_150;

            //AiVqeCfg.aec
            g_stAiVqeCfg.stAecCfg.bComfortNoiseEnable = FALSE;
            g_stAiVqeCfg.stAecCfg.s16DelaySample = 0;
            memcpy(g_stAiVqeCfg.stAecCfg.u32AecSupfreq, U32AecSupfreq, sizeof(U32AecSupfreq));
            memcpy(g_stAiVqeCfg.stAecCfg.u32AecSupIntensity,U32AecSupIntensity,sizeof(U32AecSupIntensity));

            //AiVqeCfg.Anr
            g_stAiVqeCfg.stAnrCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_MUSIC;//2;
            g_stAiVqeCfg.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_MID;
            g_stAiVqeCfg.stAnrCfg.u32NrIntensity = 15;            //[0,30]
            g_stAiVqeCfg.stAnrCfg.u32NrSmoothLevel = 10;

            //AiVqeCfg.Agc
            g_stAiVqeCfg.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAiVqeCfg.stAgcCfg.s32NoiseGateDb = -60;           //[-80, 0], NoiseGateDb disable when value = -80
            g_stAiVqeCfg.stAgcCfg.s32TargetLevelDb = -10;           //[-80, 0]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = 0;  //[-20, 30]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0, 30]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMin = 0;  //[-20, 30]
            g_stAiVqeCfg.stAgcCfg.u32AttackTime = 1;
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_output,s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));
            g_stAiVqeCfg.stAgcCfg.u32DropGainMax = 12;            //[0,60]
            g_stAiVqeCfg.stAgcCfg.u32NoiseGateAttenuationDb = 0; //[0,100]
            g_stAiVqeCfg.stAgcCfg.u32ReleaseTime = 3;

            //AiVqeCfg.Eq
            g_stAiVqeCfg.stEqCfg.eMode = (MI_AUDIO_AlgorithmMode_e)E_MI_AUDIO_ALGORITHM_MODE_USER;
            for (MI_U32 j = 0; j < sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb) / sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb[0]); j++)
            {
                g_stAiVqeCfg.stEqCfg.s16EqGainDb[j] = 5;
            }


            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_output,s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));

            for(MI_U32 j = 0; j < sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb) / sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb[0]); j++)
            {
                g_stAiVqeCfg.stEqCfg.s16EqGainDb[j] = 5;
            }
        }
    }

    memset(&g_stAoVqeCfg, 0x00, sizeof(MI_AO_VqeConfig_t));
    memset(&g_audioOutParam, 0x00, sizeof(MixerAudioOutParam) * MIXER_AO_MAX_NUMBER);
    for(i = 0; i < MIXER_AO_MAX_NUMBER; i++)
    {
        g_audioOutParam[i].stAudioOutChnPort.eModId    = E_MI_MODULE_ID_AO;
        g_audioOutParam[i].stAudioOutChnPort.u32DevId  = -1;
        g_audioOutParam[i].stAudioOutChnPort.u32ChnId  = 0;
        g_audioOutParam[i].stAudioOutChnPort.u32PortId = 0;

        g_audioOutParam[i].AoMediaType = MT_G711A;

        g_audioOutParam[i].s32AudioOutNum = 0;
        g_audioOutParam[i].s32VolumeOutDb = 5;
        g_audioOutParam[i].u8UserFrameDepth = 12;
        g_audioOutParam[i].u8BufQueueDepth  = 13;
        memset(g_audioOutParam[i].s8AudioPath, 0x00, sizeof(g_audioOutParam[i].s8AudioPath));
        memcpy(g_audioOutParam[i].s8AudioPath, DEF_AUDIOPLAY_FILE, strlen(DEF_AUDIOPLAY_FILE));

        g_audioOutParam[i].stAudioAttr.eBitwidth   = E_MI_AUDIO_BIT_WIDTH_16;
        g_audioOutParam[i].stAudioAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
        g_audioOutParam[i].stAudioAttr.eWorkmode   = E_MI_AUDIO_MODE_I2S_MASTER;
        g_audioOutParam[i].stAudioAttr.eSoundmode  = E_MI_AUDIO_SOUND_MODE_MONO;
        g_audioOutParam[i].stAudioAttr.u32FrmNum   = 16;  //must >=5
        g_audioOutParam[i].stAudioAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
        g_audioOutParam[i].stAudioAttr.u32ChnCnt   = 0;

        if(0 == i)
        {
            g_stAoVqeCfg.bHpfOpen = FALSE;
            g_stAoVqeCfg.bAnrOpen = TRUE;
            g_stAoVqeCfg.bAgcOpen = FALSE;
            g_stAoVqeCfg.bEqOpen  = FALSE;

            g_stAoVqeCfg.s32FrameSample = 128;
            g_stAoVqeCfg.s32WorkSampleRate = g_audioOutParam[i].stAudioAttr.eSamplerate;

            //AoVqeCfg.Hpf
            g_stAoVqeCfg.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER; //1;
            g_stAoVqeCfg.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_120;

            //AoVqeCfg.Anr
            g_stAoVqeCfg.stAnrCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAoVqeCfg.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_LOW;
            g_stAoVqeCfg.stAnrCfg.u32NrIntensity   = 10;          //[0,30]
            g_stAoVqeCfg.stAnrCfg.u32NrSmoothLevel = 10;

            //AoVqeCfg.Agc
            g_stAoVqeCfg.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAoVqeCfg.stAgcCfg.s32NoiseGateDb = -50;           //[-80, 0], NoiseGateDb disable when value = -80
            g_stAoVqeCfg.stAgcCfg.s32TargetLevelDb = -15;         //[-80, 0]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = 1;  //[-20,30]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0,  30]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMin = -10; //[-20,30]
            g_stAoVqeCfg.stAgcCfg.u32AttackTime = 1;

            g_stAoVqeCfg.stAgcCfg.u32DropGainMax = 60;            //[0,60]
            g_stAoVqeCfg.stAgcCfg.u32NoiseGateAttenuationDb = 0;  //[0,100]
            g_stAoVqeCfg.stAgcCfg.u32ReleaseTime = 1;

            //AoVqeCfg.Eq
            g_stAoVqeCfg.stEqCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
        }
    }
}
#else
inline void mixer_init_videoAudioParam(void)
{
    MI_S16 s16CompressionRatioInput[5]  = {-80, -60, -40, -25, 0};
    MI_S16 s16CompressionRatioOutput[5] = {-80, -30, -15, -10, -3};
    MI_U32 U32AecSupfreq[6] = {4, 6, 36, 49, 50, 51};
    MI_U32 U32AecSupIntensity[7] = {5, 4, 4, 5, 10, 10, 10};

    ModuleVencInfo_s *VencInfo_t = GetVencInfo();
    ModuleVpeInfo_s  *VpeBaseInfo_t = GetVpeBaseInfo();
    ModuleVifInfo_s  *VifInfo_t = GetVifInfo();
    ModuleSensorInfo_s      *SensorInfo_t = GetSensorInfo();

    MI_U8 i = 0x0, j = 0x0;
    memset(&g_videoParam, 0x00, sizeof(MixerVideoParam) * MAX_VIDEO_NUMBER);

    for(i = 0; i < MAX_VIDEO_NUMBER; i++, j++)
    {
        //init this channel's rec state
        tmpRecStatus[i].ch = -1;
        tmpRecStatus[i].status = -1;
        //end


        //init vif
        if(0x0 == SensorInfo_t->ResIndex[SensorInfo_t->workResIndex].maxFps)
        {
            MIXER_WARN("sensor max_fps is zero. err\n");
            SensorInfo_t->ResIndex[SensorInfo_t->workResIndex].maxFps = 30;
        }
        MIXER_DBG("sensor max_fps is %d\n", SensorInfo_t->ResIndex[SensorInfo_t->workResIndex].maxFps);
        g_videoParam[i].stVifInfo.sensorFrameRate = SensorInfo_t->ResIndex[SensorInfo_t->workResIndex].maxFps;

        g_videoParam[i].stVifInfo.vifframeRate = VifInfo_t->MaxFps;
        g_videoParam[i].stVifInfo.SensorWidth = VifInfo_t->MaxWidth;
        g_videoParam[i].stVifInfo.SensorHeight = VifInfo_t->MaxHeight;
        g_videoParam[i].stVifInfo.stVifChnPort.eModId = E_MI_MODULE_ID_VIF;
        g_videoParam[i].stVifInfo.stVifChnPort.u32DevId = 0;
        g_videoParam[i].stVifInfo.stVifChnPort.u32ChnId = 0;
        g_videoParam[i].stVifInfo.stVifChnPort.u32PortId = 0;

        if(0x0 == i)
            g_videoParam[0].stVifInfo.HdrType = VifInfo_t->IsHDR;    //disable HDR

    //init vpe

        g_videoParam[i].vpeframeRate = 30;
        g_videoParam[i].vpeBufUsrDepth = 0;    // user no need set buffer

        g_videoParam[i].stVpeChnPort.eModId = E_MI_MODULE_ID_VPE;
        g_videoParam[i].stVpeChnPort.u32DevId = 0;
        g_videoParam[i].stVpeChnPort.u32ChnId = 0;

        j = Mixer_coverVi2Vpe(j);
#if 0
        if( j >= MAX_VPE_PORT_NUMBER-1)
            j = MAX_VPE_PORT_NUMBER-1;
#endif
        g_videoParam[i].vpeBufCntQuota = VpeBaseInfo_t->Outdepth[j];
        g_videoParam[i].FromPort = g_videoParam[i].stVpeChnPort.u32PortId = VencInfo_t->FromPort[i];

        /*if(MAX_VPE_PORT_NUMBER != 0x0 && g_videoParam[i].stVpeChnPort.u32PortId >= MAX_VPE_PORT_NUMBER)
            g_videoParam[i].stVpeChnPort.u32PortId = MAX_VPE_PORT_NUMBER -1 ;*/

        g_videoParam[i].stVpeChnPort.u32PortId = (MI_U32)Mixer_coverVi2Vpe((int)g_videoParam[i].stVpeChnPort.u32PortId);

        g_videoParam[i].u16VpeOutWidth  = VencInfo_t->StreamWidth[i];
        g_videoParam[i].u16VpeOutHeight = VencInfo_t->StreamHeight[i];
        MIXER_DBG("i:%d, init cw:%d, ch:%d.\n", i, VencInfo_t->StreamWidth[i], VencInfo_t->StreamHeight[i]);

        if(i == 0)
        {
            g_videoParam[0].stVifInfo.level3DNR  = VpeBaseInfo_t->_3dnrLevel; //enable 3DNR : 1-pass, 12-bit
        }

    //init divp
        g_videoParam[i].divpBufUsrDepth = 0;
        g_videoParam[i].divpBufCntQuota = 3;

    //init venc
        g_videoParam[i].vencBufUsrDepth = 4;
        g_videoParam[i].vencBufCntQuota = 4;
        g_videoParam[i].vencframeRate = VencInfo_t->Fps[i];
        g_videoParam[i].width  = VencInfo_t->StreamWidth[i];
        g_videoParam[i].height = VencInfo_t->StreamHeight[i];
        g_videoParam[i].MaxWidth  = VencInfo_t->StreamMaxWidth[i];
        g_videoParam[i].MaxHeight = VencInfo_t->StreamMaxHeight[i];

        g_videoParam[i].bitrate = VencInfo_t->Bitrate[i];

        g_videoParam[i].gop = g_videoParam[i].vencframeRate * 2;

    #if TARGET_CHIP_I6 || TARGET_CHIP_I5
        g_videoParam[i].maxQp = 48;
        g_videoParam[i].minQp = 12;
        g_videoParam[i].maxIQp = 48;  //[10 48]
        g_videoParam[i].minIQp = 25;  //(10 48)
        g_videoParam[i].maxPQp = 48;  //[10 48)
        g_videoParam[i].minPQp = 25;  //(10 48]
        #else
    g_videoParam[i].maxQp = 48;
    g_videoParam[i].minQp = 15;
    g_videoParam[i].maxIQp = 48;  //[10 48]
        g_videoParam[i].minIQp = 15;  //(10 48)
        g_videoParam[i].maxPQp = 48;  //[10 48)
        g_videoParam[i].minPQp = 15;  //(10 48]
    #endif

        g_videoParam[i].u8MaxQfactor = 90;
        g_videoParam[i].u8MinQfactor = 20;
        g_videoParam[i].s8Qfactor = 50;
        g_videoParam[i].IPQPDelta = 0;
        g_videoParam[i].u8ChangePos = 50;  //[50 100]
        g_videoParam[i].encoderType = VencInfo_t->EncodeType[i];
        g_videoParam[i].u8RateCtlType = RATECTLTYPE_MAX;
        g_videoParam[i].virtualIInterval = 0;
        g_videoParam[i].virtualIEnable = FALSE;
        g_videoParam[i].viChnStatus = VI_DISABLE;
        g_videoParam[i].s8DivpEnable = 0;

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        g_videoParam[i].eBindMode = (Mixer_Venc_Bind_Mode_E)VencInfo_t->BindModuleType[i];
        g_videoParam[i].ePixelFormat = E_MI_SYS_PIXEL_FRAME_FORMAT_MAX;
#endif
#if MIXER_SED_ENABLE
        g_videoParam[i].u8SedEnable = 0;
#endif

    }

    memset(&g_stAiVqeCfg, 0x00, sizeof(MI_AI_VqeConfig_t));
    memset(&g_audioInParam, 0x00, sizeof(MixerAudioInParam) * MIXER_AI_MAX_NUMBER);
    for(i = 0; i < MIXER_AI_MAX_NUMBER; i++)
    {
        g_audioInParam[i].stAudioInChnPort.eModId    = E_MI_MODULE_ID_AI;
        g_audioInParam[i].stAudioInChnPort.u32DevId  = 0; //0:AMIC[2chn]; 1:DMIC[4chn]; 2:I2S[8chn]; default:0
        g_audioInParam[i].stAudioInChnPort.u32ChnId  = 0;
        g_audioInParam[i].stAudioInChnPort.u32PortId = 0;

        g_audioInParam[i].bFlag = FALSE;
        g_audioInParam[i].AiMediaType = MT_G711A;
        g_audioInParam[i].u8AudioInNum = 0;
        g_audioInParam[i].s32VolumeInDb = 5;
        g_audioInParam[i].u8UserFrameDepth = 9;
        g_audioInParam[i].u8BufQueueDepth  = 10;
        g_audioInParam[i].eAudioInReSampleRate = E_MI_AUDIO_SAMPLE_RATE_INVALID;

        g_audioInParam[i].stAudioAttr.eBitwidth   = E_MI_AUDIO_BIT_WIDTH_16;
        g_audioInParam[i].stAudioAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
        g_audioInParam[i].stAudioAttr.eWorkmode   = E_MI_AUDIO_MODE_I2S_MASTER;
        g_audioInParam[i].stAudioAttr.eSoundmode  = E_MI_AUDIO_SOUND_MODE_MONO;
        g_audioInParam[i].stAudioAttr.u32FrmNum   = 16;  //must >=5
        g_audioInParam[i].stAudioAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
        g_audioInParam[i].stAudioAttr.u32ChnCnt   = 0;

        if(0 == i)
        {
            g_stAiVqeCfg.bHpfOpen = FALSE;
            g_stAiVqeCfg.bAnrOpen = FALSE;
            g_stAiVqeCfg.bAgcOpen = FALSE;
            g_stAiVqeCfg.bEqOpen  = FALSE;
            g_stAiVqeCfg.bAecOpen = FALSE;

            g_stAiVqeCfg.s32FrameSample = 128;
            g_stAiVqeCfg.s32WorkSampleRate = g_audioInParam[i].stAudioAttr.eSamplerate;
            g_stAiVqeCfg.u32ChnNum = 0;

            //AiVqeCfg.Hpf
            g_stAiVqeCfg.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAiVqeCfg.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_150;

            //AiVqeCfg.aec
            g_stAiVqeCfg.stAecCfg.bComfortNoiseEnable = FALSE;
            g_stAiVqeCfg.stAecCfg.s16DelaySample = 0;
            memcpy(g_stAiVqeCfg.stAecCfg.u32AecSupfreq, U32AecSupfreq, sizeof(U32AecSupfreq));
            memcpy(g_stAiVqeCfg.stAecCfg.u32AecSupIntensity,U32AecSupIntensity,sizeof(U32AecSupIntensity));

            //AiVqeCfg.Anr
            g_stAiVqeCfg.stAnrCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_MUSIC;//2;
            g_stAiVqeCfg.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_MID;
            g_stAiVqeCfg.stAnrCfg.u32NrIntensity = 15;            //[0,30]
            g_stAiVqeCfg.stAnrCfg.u32NrSmoothLevel = 10;

            //AiVqeCfg.Agc
            g_stAiVqeCfg.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAiVqeCfg.stAgcCfg.s32NoiseGateDb = -60;           //[-80, 0], NoiseGateDb disable when value = -80
            g_stAiVqeCfg.stAgcCfg.s32TargetLevelDb = -10;           //[-80, 0]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = 0;  //[-20, 30]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0, 30]
            g_stAiVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMin = 0;  //[-20, 30]
            g_stAiVqeCfg.stAgcCfg.u32AttackTime = 1;
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_output,s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));
            g_stAiVqeCfg.stAgcCfg.u32DropGainMax = 12;            //[0,60]
            g_stAiVqeCfg.stAgcCfg.u32NoiseGateAttenuationDb = 0; //[0,100]
            g_stAiVqeCfg.stAgcCfg.u32ReleaseTime = 3;

            //AiVqeCfg.Eq
            g_stAiVqeCfg.stEqCfg.eMode = (MI_AUDIO_AlgorithmMode_e)E_MI_AUDIO_ALGORITHM_MODE_USER;
            for (MI_U32 j = 0; j < sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb) / sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb[0]); j++)
            {
                g_stAiVqeCfg.stEqCfg.s16EqGainDb[j] = 5;
            }


            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
            memcpy(g_stAiVqeCfg.stAgcCfg.s16Compression_ratio_output,s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));

            for(MI_U32 j = 0; j < sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb) / sizeof(g_stAiVqeCfg.stEqCfg.s16EqGainDb[0]); j++)
            {
                g_stAiVqeCfg.stEqCfg.s16EqGainDb[j] = 5;
            }
        }
    }

    memset(&g_stAoVqeCfg, 0x00, sizeof(MI_AO_VqeConfig_t));
    memset(&g_audioOutParam, 0x00, sizeof(MixerAudioOutParam) * MIXER_AO_MAX_NUMBER);
    for(i = 0; i < MIXER_AO_MAX_NUMBER; i++)
    {
        g_audioOutParam[i].stAudioOutChnPort.eModId    = E_MI_MODULE_ID_AO;
        g_audioOutParam[i].stAudioOutChnPort.u32DevId  = 0;
        g_audioOutParam[i].stAudioOutChnPort.u32ChnId  = 0;
        g_audioOutParam[i].stAudioOutChnPort.u32PortId = 0;

        g_audioOutParam[i].AoMediaType = MT_G711A;

        g_audioOutParam[i].bFlag = FALSE;
        g_audioOutParam[i].s32AudioOutNum = 0;
        g_audioOutParam[i].s32VolumeOutDb = 5;
        g_audioOutParam[i].u8UserFrameDepth = 12;
        g_audioOutParam[i].u8BufQueueDepth  = 13;
        memset(g_audioOutParam[i].s8AudioPath, 0x00, sizeof(g_audioOutParam[i].s8AudioPath));
        memcpy(g_audioOutParam[i].s8AudioPath, DEF_AUDIOPLAY_FILE, strlen(DEF_AUDIOPLAY_FILE));

        g_audioOutParam[i].stAudioAttr.eBitwidth   = E_MI_AUDIO_BIT_WIDTH_16;
        g_audioOutParam[i].stAudioAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
        g_audioOutParam[i].stAudioAttr.eWorkmode   = E_MI_AUDIO_MODE_I2S_MASTER;
        g_audioOutParam[i].stAudioAttr.eSoundmode  = E_MI_AUDIO_SOUND_MODE_MONO;
        g_audioOutParam[i].stAudioAttr.u32FrmNum   = 16;  //must >=5
        g_audioOutParam[i].stAudioAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
        g_audioOutParam[i].stAudioAttr.u32ChnCnt   = 0;

        if(0 == i)
        {
            g_stAoVqeCfg.bHpfOpen = FALSE;
            g_stAoVqeCfg.bAnrOpen = TRUE;
            g_stAoVqeCfg.bAgcOpen = FALSE;
            g_stAoVqeCfg.bEqOpen  = FALSE;

            g_stAoVqeCfg.s32FrameSample = 128;
            g_stAoVqeCfg.s32WorkSampleRate = g_audioOutParam[i].stAudioAttr.eSamplerate;

            //AoVqeCfg.Hpf
            g_stAoVqeCfg.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER; //1;
            g_stAoVqeCfg.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_120;

            //AoVqeCfg.Anr
            g_stAoVqeCfg.stAnrCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAoVqeCfg.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_LOW;
            g_stAoVqeCfg.stAnrCfg.u32NrIntensity   = 10;          //[0,30]
            g_stAoVqeCfg.stAnrCfg.u32NrSmoothLevel = 10;

            //AoVqeCfg.Agc
            g_stAoVqeCfg.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;//1;
            g_stAoVqeCfg.stAgcCfg.s32NoiseGateDb = -50;           //[-80, 0], NoiseGateDb disable when value = -80
            g_stAoVqeCfg.stAgcCfg.s32TargetLevelDb = -15;         //[-80, 0]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainInit = 1;  //[-20,30]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0,  30]
            g_stAoVqeCfg.stAgcCfg.stAgcGainInfo.s32GainMin = -10; //[-20,30]
            g_stAoVqeCfg.stAgcCfg.u32AttackTime = 1;

            g_stAoVqeCfg.stAgcCfg.u32DropGainMax = 60;            //[0,60]
            g_stAoVqeCfg.stAgcCfg.u32NoiseGateAttenuationDb = 0;  //[0,100]
            g_stAoVqeCfg.stAgcCfg.u32ReleaseTime = 1;

            //AoVqeCfg.Eq
            g_stAoVqeCfg.stEqCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
        }
    }
}
#endif

void mixer_printf_video_audio_info(void)
{
    MI_U32 i = 0 ;
     printf("\n");

     printf("\n  video valid output number:%d\n", g_videoNumber);
     printf("    ie FD param     %d\n", g_fdParam);
     printf("    ie MD param     %d\n", g_mdParam);
     printf("    ie OD param     %d\n", g_odParam);
     printf("    ie VG param     %d\n", g_vgParam);
     printf("    ie DLA param    %d\n", g_dlaParam);
     printf("    ie log param    %d\n", g_ieLogParam);

     if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_dlaParam)
     {
         printf("    ie vpe port     %d\n", g_ieVpePort);
     }
     else
     {
         printf("    ie vpe port     -1\n");
     }

     for(i=0x0; i < g_videoNumber; i++)
     {
         printf("\n  video %d/%d param:\n", i + 1, g_videoNumber);
         printf("    resolution             %d*%d\n", g_videoParam[i].width, g_videoParam[i].height);
         printf("    bitrate                %d\n", g_videoParam[i].bitrate);
         if(0 == (g_videoParam[i].vencframeRate & 0xffff0000))
         {
             printf("    VencframeRate          %d\n", g_videoParam[i].vencframeRate);
         }
         else
         {
             printf("    VencframeRate          %f\n", (g_videoParam[i].vencframeRate & 0xffff) * 1.0 /
             ((g_videoParam[i].vencframeRate >> 16) & 0xffff));
         }
         printf("    sensor frameRate       %d\n", g_videoParam[i].stVifInfo.sensorFrameRate);
         printf("    videoType              %s\n", getVideoEncoderTypeString((Mixer_EncoderType_e)g_videoParam[i].encoderType));
         printf("    vif channel ID         %d   \n", g_videoParam[i].stVpeChnPort.u32ChnId);
         printf("    vif port ID            %d   \n", g_videoParam[i].stVpeChnPort.u32PortId);
         printf("    vpe channel ID         %d   \n", g_videoParam[i].stVpeChnPort.u32ChnId);
         printf("    vpe port ID            %d   \n", g_videoParam[i].stVpeChnPort.u32PortId);
     }


     //print AudioIn param
     printf("\n");

     for(i = 0 ; i < g_audioInParam[0].u8AudioInNum; i++)
     {
         printf("\n  AudioIn %d/%d param:\n", i + 1, g_audioInParam[0].u8AudioInNum);
         if(E_MI_MODULE_ID_AI == g_audioInParam[i].stAudioInChnPort.eModId)
         {
             printf("    AIChnPort.eModId       %d (%s)\n", g_audioInParam[i].stAudioInChnPort.eModId, "E_MI_MODULE_ID_AI");

             switch(g_audioInParam[i].stAudioInChnPort.u32DevId)
             {
                 case 0: printf("    AIChnPort.DevID        %d (%s)\n", g_audioInParam[i].stAudioInChnPort.u32DevId, "AMIC"); break;
                 case 1: printf("    AIChnPort.DevID        %d (%s)\n", g_audioInParam[i].stAudioInChnPort.u32DevId, "DMIC"); break;
                 case 2: printf("    AIChnPort.DevID        %d (%s)\n", g_audioInParam[i].stAudioInChnPort.u32DevId, "I2S"); break;
                 default:printf("    AIChnPort.DevID        %d (Set wrong param)\n", g_audioInParam[i].stAudioInChnPort.u32DevId);
             }
             printf("    AIChnPort.ChnID        %d\n", g_audioInParam[i].stAudioInChnPort.u32ChnId);
             printf("    AIChnPort.PortID       %d\n", g_audioInParam[i].stAudioInChnPort.u32PortId);

             switch(g_audioInParam[i].AiMediaType)
             {
                 case 0:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_PCM"); break;
                 case 1:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G711A"); break;
                 case 2:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G711U"); break;
                 case 3:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G711"); break;
                 case 4:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_AMR"); break;
                 case 5:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_AAC"); break;
                 case 6:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G726"); break;
                 case 7:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_MP3"); break;
                 case 8:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_OGG"); break;
                 case 9:  printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_ADPCM"); break;
                 case 10: printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_OPUS"); break;
                 case 11: printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G726_16"); break;
                 case 12: printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G726_24"); break;
                 case 13: printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G726_32"); break;
                 case 14: printf("    AIChnPort.MediaType    %d (%s)\n", g_audioInParam[i].AiMediaType, "MT_G726_40"); break;
                 default: printf("    AIChnPort.MediaType    %d (Set wrong param)\n", g_audioInParam[i].AiMediaType);
             }

             printf("    AIChnPort.VolumeInDb   %d\n", g_audioInParam[i].s32VolumeInDb);
             printf("    AIChnPort.EnableVqe    %d\n", g_audioInParam[i].bAudioInVqe);
             printf("    UserFrameDepth         %d\n", g_audioInParam[i].u8UserFrameDepth);
             printf("    BufQueueDepth          %d\n", g_audioInParam[i].u8BufQueueDepth);

             switch(g_audioInParam[i].stAudioAttr.eBitwidth)
             {
                 case 0: printf("    Bitwidth               %d (%s)\n", g_audioInParam[i].stAudioAttr.eBitwidth, "16bit"); break;
                 case 1: printf("    Bitwidth               %d (%s)\n", g_audioInParam[i].stAudioAttr.eBitwidth, "24bit"); break;
                 default:printf("    Bitwidth               %d (Set wrong param)\n", g_audioInParam[i].stAudioAttr.eBitwidth);
             }

             switch(g_audioInParam[i].stAudioAttr.eSamplerate)
             {
                 case 8000:   printf("    Samplerate             %d\n", g_audioInParam[i].stAudioAttr.eSamplerate); break;
                 case 16000:  printf("    Samplerate             %d\n", g_audioInParam[i].stAudioAttr.eSamplerate); break;
                 case 32000:  printf("    Samplerate             %d\n", g_audioInParam[i].stAudioAttr.eSamplerate); break;
                 case 48000:  printf("    Samplerate             %d\n", g_audioInParam[i].stAudioAttr.eSamplerate); break;
                 default:     printf("    Samplerate             %d (Set wrong param)\n", g_audioInParam[i].stAudioAttr.eSamplerate);
             }

             switch(g_audioInParam[i].stAudioAttr.eWorkmode)
             {
                 case 0: printf("    Workmode               %d (%s)\n", g_audioInParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_I2S_MASTER"); break;
                 case 1: printf("    Workmode               %d (%s)\n", g_audioInParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_I2S_SLAVE"); break;
                 case 2: printf("    Workmode               %d (%s)\n", g_audioInParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_TDM_MASTER"); break;
                 default:printf("    Workmode               %d (Set wrong param)\n", g_audioInParam[i].stAudioAttr.eWorkmode);
             }

             switch(g_audioInParam[i].stAudioAttr.eSoundmode)
             {
                 case 0: printf("    Soundmode              %d (%s)\n", g_audioInParam[i].stAudioAttr.eSoundmode, "E_MI_AUDIO_SOUND_MODE_MONO"); break;
                 case 1: printf("    Soundmode              %d (%s)\n", g_audioInParam[i].stAudioAttr.eSoundmode, "E_MI_AUDIO_SOUND_MODE_STEREO"); break;
                 default:printf("    Soundmode              %d (Set wrong param)\n", g_audioInParam[i].stAudioAttr.eSoundmode);
             }

             printf("    FrmNum                 %d\n", g_audioInParam[i].stAudioAttr.u32FrmNum);
             printf("    PtNumPerFrm            %d\n", g_audioInParam[i].stAudioAttr.u32PtNumPerFrm);

             printf("    VqeCfg.FrameSample     %d\n", g_stAiVqeCfg.s32FrameSample);
             printf("    VqeCfg.WorkSampleRate  %d\n", g_stAiVqeCfg.s32WorkSampleRate);

             printf("    VqeCfg.HpfCfg.UsrMode  %d\n", g_stAiVqeCfg.stHpfCfg.eMode);
             printf("    VqeCfg.HpfCfg.HpfFreq  %d\n", g_stAiVqeCfg.stHpfCfg.eHpfFreq);

             printf("    VqeCfg.AnrCfg.UsrMode  %d\n", g_stAiVqeCfg.stAnrCfg.eMode);

             switch(g_audioInParam[i].stAudioAttr.eSoundmode)
             {
                 case 0: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAiVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_LOW"); break;
                 case 1: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAiVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_MID"); break;
                 case 2: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAiVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_HIGH"); break;
                 default:printf("    VqeCfg.AnrCfg.NrSpeed  %d (Set wrong param)\n", g_stAiVqeCfg.stAnrCfg.eNrSpeed);
             }

             printf("    VqeCfg.AnrCfg.NrIntensity    %d\n", g_stAiVqeCfg.stAnrCfg.u32NrIntensity);
             printf("    VqeCfg.AnrCfg.NrSmoothLevel  %d\n", g_stAiVqeCfg.stAnrCfg.u32NrSmoothLevel);
         }
         else
         {
             printf("    Set wrong AudioInChnPort[%d].eModId = %d\n", i, g_audioInParam[i].stAudioInChnPort.eModId);
         }

         printf("\n");
    }

     //start print AudioOut param
     for(i = 0 ; i < g_audioOutParam[0].s32AudioOutNum; i++)
     {
         printf("\n  AudioOut %d/%d param:\n", i + 1, g_audioOutParam[0].s32AudioOutNum);
         if(E_MI_MODULE_ID_AO == g_audioOutParam[i].stAudioOutChnPort.eModId)
         {
             printf("    AoChnPort.eModId       %d (%s)\n", g_audioOutParam[i].stAudioOutChnPort.eModId, "E_MI_MODULE_ID_AO");

             switch(g_audioOutParam[i].stAudioOutChnPort.u32DevId)
             {
                 case 0: printf("    AoChnPort.DevID        %d (%s)\n", g_audioOutParam[i].stAudioOutChnPort.u32DevId, "AMIC"); break;
                 case 1: printf("    AoChnPort.DevID        %d (%s)\n", g_audioOutParam[i].stAudioOutChnPort.u32DevId, "DMIC"); break;
                 case 2: printf("    AoChnPort.DevID        %d (%s)\n", g_audioOutParam[i].stAudioOutChnPort.u32DevId, "I2S"); break;
                 default:printf("    AoChnPort.DevID        %d (Set wrong param)\n", g_audioOutParam[i].stAudioOutChnPort.u32DevId);
             }
             printf("    AoChnPort.ChnID        %d\n", g_audioOutParam[i].stAudioOutChnPort.u32ChnId);
             printf("    AoChnPort.PortID       %d\n", g_audioOutParam[i].stAudioOutChnPort.u32PortId);

             switch(g_audioOutParam[i].AoMediaType)
             {
                 case 0:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_PCM"); break;
                 case 1:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G711A"); break;
                 case 2:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G711U"); break;
                 case 3:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G711"); break;
                 case 4:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_AMR"); break;
                 case 5:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_AAC"); break;
                 case 6:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G726"); break;
                 case 7:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_MP3"); break;
                 case 8:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_OGG"); break;
                 case 9:  printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_ADPCM"); break;
                 case 10: printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_OPUS"); break;
                 case 11: printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G726_16"); break;
                 case 12: printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G726_24"); break;
                 case 13: printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G726_32"); break;
                 case 14: printf("    AOChnPort.MediaType    %d (%s)\n", g_audioOutParam[i].AoMediaType, "MT_G726_40"); break;
                 default: printf("    AOChnPort.MediaType    %d (Set wrong param)\n", g_audioOutParam[i].AoMediaType);
             }

             printf("    AOChnPort.EnableRes    %d\n", g_audioOutParam[i].u8AudioOutRes);
             printf("    AOChnPort.EnableVqe    %d\n", g_audioOutParam[i].u8AudioOutVqe);
             printf("    AOChnPort.VolumeOutDb  %d\n", g_audioOutParam[i].s32VolumeOutDb);

             printf("    UserFrameDepth         %d\n", g_audioOutParam[i].u8UserFrameDepth);
             printf("    BufQueueDepth          %d\n", g_audioOutParam[i].u8BufQueueDepth);

             switch(g_audioOutParam[i].stAudioAttr.eBitwidth)
             {
                 case 0: printf("    Bitwidth               %d (%s)\n", g_audioOutParam[i].stAudioAttr.eBitwidth, "16bit"); break;
                 case 1: printf("    Bitwidth               %d (%s)\n", g_audioOutParam[i].stAudioAttr.eBitwidth, "24bit"); break;
                 default:printf("    Bitwidth               %d (Set wrong param)\n", g_audioOutParam[i].stAudioAttr.eBitwidth);
             }

             switch(g_audioOutParam[i].stAudioAttr.eSamplerate)
             {
                 case 8000:   printf("    Samplerate             %d\n", g_audioOutParam[i].stAudioAttr.eSamplerate); break;
                 case 16000:  printf("    Samplerate             %d\n", g_audioOutParam[i].stAudioAttr.eSamplerate); break;
                 case 32000:  printf("    Samplerate             %d\n", g_audioOutParam[i].stAudioAttr.eSamplerate); break;
                 case 48000:  printf("    Samplerate             %d\n", g_audioOutParam[i].stAudioAttr.eSamplerate); break;
                 default:     printf("    Samplerate             %d (Set wrong param)\n", g_audioOutParam[i].stAudioAttr.eSamplerate);
             }

             switch(g_audioOutParam[i].stAudioAttr.eWorkmode)
             {
                 case 0: printf("    Workmode               %d (%s)\n", g_audioOutParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_I2S_MASTER"); break;
                 case 1: printf("    Workmode               %d (%s)\n", g_audioOutParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_I2S_SLAVE"); break;
                 case 2: printf("    Workmode               %d (%s)\n", g_audioOutParam[i].stAudioAttr.eWorkmode, "E_MI_AUDIO_MODE_TDM_MASTER"); break;
                 default:printf("    Workmode               %d (Set wrong param)\n", g_audioOutParam[i].stAudioAttr.eWorkmode);
             }

             switch(g_audioOutParam[i].stAudioAttr.eSoundmode)
             {
                 case 0: printf("    Soundmode              %d (%s)\n", g_audioOutParam[i].stAudioAttr.eSoundmode, "E_MI_AUDIO_SOUND_MODE_MONO"); break;
                 case 1: printf("    Soundmode              %d (%s)\n", g_audioOutParam[i].stAudioAttr.eSoundmode, "E_MI_AUDIO_SOUND_MODE_STEREO"); break;
                 default:printf("    Soundmode              %d (Set wrong param)\n", g_audioOutParam[i].stAudioAttr.eSoundmode);
             }

             printf("    FrmNum                 %d\n", g_audioOutParam[i].stAudioAttr.u32FrmNum);
             printf("    PtNumPerFrm            %d\n", g_audioOutParam[i].stAudioAttr.u32PtNumPerFrm);

             printf("    VqeCfg.FrameSample     %d\n", g_stAoVqeCfg.s32FrameSample);
             printf("    VqeCfg.WorkSampleRate  %d\n", g_stAoVqeCfg.s32WorkSampleRate);

             printf("    VqeCfg.HpfCfg.UsrMode  %d\n", g_stAoVqeCfg.stHpfCfg.eMode);
             printf("    VqeCfg.HpfCfg.HpfFreq  %d\n", g_stAoVqeCfg.stHpfCfg.eHpfFreq);

             printf("    VqeCfg.AnrCfg.UsrMode  %d\n", g_stAoVqeCfg.stAnrCfg.eMode);

             switch(g_audioOutParam[i].stAudioAttr.eSoundmode)
             {
                 case 0: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAoVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_LOW"); break;
                 case 1: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAoVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_MID"); break;
                 case 2: printf("    VqeCfg.AnrCfg.NrSpeed  %d (%s)\n", g_stAoVqeCfg.stAnrCfg.eNrSpeed, "E_MI_AUDIO_NR_SPEED_HIGH"); break;
                 default:printf("    VqeCfg.AnrCfg.NrSpeed  %d (Set wrong param)\n", g_stAoVqeCfg.stAnrCfg.eNrSpeed);
             }

             printf("    VqeCfg.AnrCfg.NrIntensity    %d\n", g_stAoVqeCfg.stAnrCfg.u32NrIntensity);
             printf("    VqeCfg.AnrCfg.NrSmoothLevel  %d\n", g_stAoVqeCfg.stAnrCfg.u32NrSmoothLevel);
         }
         else
         {
             printf("    Set wrong AudioOutChnPort[%d].eModId = %d\n", i, g_audioOutParam[i].stAudioOutChnPort.eModId);
         }

         printf("\n");
    }
}

void mixerBitrateCheck(MI_U8 i,MI_U32 sTWidth, MI_U32 sTHeight,MI_U32 maxBitRate,MI_U32 minBitRate,MI_U32 maxBitRate1,MI_U32 minBitRate1,MI_U8 currentFps,MI_U32 *currentBitRate)
{
   MI_U32 tmpWidth = 0;
   MI_U32 tmpHeight = 0;
   if(5 <= currentFps && 30 >= currentFps)
   {
     if((90 == (g_rotation & 0xFFFF)) || (270 == (g_rotation & 0xFFFF)))
      {
        tmpWidth = g_videoParam[i].height;
       tmpHeight = g_videoParam[i].width;
       if((sTHeight == tmpHeight) && (sTWidth == tmpWidth))
       {
          if(*currentBitRate < minBitRate)
          {
            *currentBitRate = minBitRate;
            return;
          }
          else if(*currentBitRate > maxBitRate)
          {
             *currentBitRate = maxBitRate;
              return;
           }
       }
     }
     else
     {
       if((sTWidth == g_videoParam[i].width) && (sTHeight == g_videoParam[i].height))
       {
        if(*currentBitRate < minBitRate)
         {
            *currentBitRate = minBitRate;
            return;
         }
         else if(*currentBitRate > maxBitRate)
         {
              *currentBitRate = maxBitRate;
              return;
         }
       }
     }

   }
   else if(1 <= currentFps && 5 > currentFps)
   {
        if((90 == (g_rotation & 0xFFFF)) || (270 == (g_rotation & 0xFFFF)))
      {
        tmpWidth = g_videoParam[i].height;
       tmpHeight = g_videoParam[i].width;
       if((sTHeight == tmpHeight) && (sTWidth == tmpWidth))
       {
            if(*currentBitRate < minBitRate1)
         {
          *currentBitRate = minBitRate1;
           return;
            }
         else if(*currentBitRate > maxBitRate1)
         {
             *currentBitRate = maxBitRate1;
           return;
         }
       }
     }
     else
     {
      if((sTWidth == g_videoParam[i].width) && (sTHeight == g_videoParam[i].height))
      {
        if(*currentBitRate < minBitRate1)
           {
          *currentBitRate = minBitRate1;
           return;
           }
        else if(*currentBitRate > maxBitRate1)
           {
             *currentBitRate = maxBitRate1;
           return;
        }
      }
     }
   }
}

MI_S32 mixerCheckParam(MixerVideoParam *pVideoParam, MI_S32 videoNum)
{
    MI_U32 i = 0;
    for(i = 0; i < g_videoNumber; i++)
    {
        if((g_videoMaxWidth < g_videoParam[i].width) && (g_videoMaxHeight < g_videoParam[i].height))
        {
            g_videoMaxWidth  = g_videoParam[i].width;
            g_videoMaxHeight = g_videoParam[i].height;
        }

        if(VE_TYPE_MAX == g_videoParam[i].encoderType)
        {
            g_videoParam[i].encoderType = VE_AVC;
        }

        if((VE_JPG == g_videoParam[i].encoderType) || (VE_MJPEG == g_videoParam[i].encoderType))
        {
            if(RATECTLTYPE_MAX == g_videoParam[i].u8RateCtlType)
            {
                g_videoParam[i].u8RateCtlType = RATECTLTYPE_FIXQP;
            }
        }
        else
        {
            if(RATECTLTYPE_MAX == g_videoParam[i].u8RateCtlType)
            {
                g_videoParam[i].u8RateCtlType = RATECTLTYPE_CBR;
            }
        }


        //only the sensor frame rate take effect
        //for(MI_S32 i = 0; i < g_videoNumber; i++)
        {
            //set IPQPDelta
            if(g_videoParam[i].encoderType == VE_AVC)
            {
                g_videoParam[i].IPQPDelta = -2;
            }
            else if(g_videoParam[i].encoderType == VE_H265)
            {
                g_videoParam[i].IPQPDelta = 2;
            }

            printf("%s: i = %d\n", __func__, i);
        }
#if (!TARGET_CHIP_I6E) && (!TARGET_CHIP_I6B0)
      if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
      {
          printf("g_videoParam[%d].height = %d,g_videoParam[%d].width=%d\n",i,g_videoParam[i].height,i,g_videoParam[i].width);
          MI_U32 tmp_width = g_videoParam[i].height;
          MI_U32 tmp_height = g_videoParam[i].width;
           if((2048 <= tmp_height && 3840 >= tmp_height)&&(1536 <= tmp_width && 2160 >= tmp_width))
          {
            mixerBitrateCheck(i,2160,3840,12000000,3000000,4000000,1000000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,2160,3072,10000000,3000000,3000000,768000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1944,2952,8000000, 2000000,4000000,1000000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1536,2560,6000000, 2000000,2000000,768000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1440,2560,6000000, 2000000,2000000,500000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1536,2048,5000000, 1500000,2000000,500000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);

          }
           else if(((640 <= tmp_height) && (1920 >= tmp_height))&&((480 <= tmp_width) && (1088 >= tmp_width)))
          {
            mixerBitrateCheck(i,1088,1920,4000000, 1000000,1500000,300000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1080,1920,4000000, 1000000,1500000,300000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,736,1280, 2000000, 768000,1000000,250000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,720,1280, 2000000, 768000,1000000,250000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,576,720,  1500000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,576,704, 1500000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,480,640, 1000000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
          }
      }
      else
      {
         if((2048 <= g_videoParam[i].width && 3840 >= g_videoParam[i].width)&&(1536 <= g_videoParam[i].height && 2160 >= g_videoParam[i].height))
          {
            mixerBitrateCheck(i,3840,2160,12000000,3000000,4000000,1000000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,3072,2160,10000000,3000000,3000000,768000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,2952,1944,8000000, 2000000,4000000,1000000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,2560,1536,6000000, 2000000,2000000,768000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,2560,1440,6000000, 2000000,2000000,500000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,2048,1536,5000000, 1500000,2000000,500000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);

          }
          else if((640 <= g_videoParam[i].width && 1920 >= g_videoParam[i].width)||(480 <= g_videoParam[i].height && 1088 >= g_videoParam[i].height))
          {
               mixerBitrateCheck(i,1920,1088,4000000, 1000000,1500000,300000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1920,1080,4000000, 1000000,1500000,300000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1280,736, 2000000, 768000,1000000,250000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,1280,720, 2000000, 768000,1000000,250000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,720,576,  1500000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,704,576,  1500000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
            mixerBitrateCheck(i,640,480,  1000000, 500000,768000,200000,g_videoParam[i].vencframeRate,&g_videoParam[i].bitrate);
          }
      }
      printf("==check after==g_videoParam[%d].bitrate==%d====================!\n",i,g_videoParam[i].bitrate);
#endif
    }

#if TARGET_CHIP_I5 || TARGET_CHIP_I6
    if((3840 <= g_videoMaxWidth) && (2160 <= g_videoMaxHeight))
    {
        if((mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 15))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 15;
            printf("%s:%d enable HDR mode, and Sensor fps must less than 15fps@3840*2160!\n", __func__, __LINE__);
        }
        else if((FALSE == mixer_GetHdrValue()) &&  (1 == g_EnablePIP))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 16;
            printf("%s:%d enable PIP, and Sensor fps must less than 15fps@3840*2160!\n", __func__, __LINE__);
        }
        else if((FALSE == mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 20))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 20;
            printf("%s:%d Sensor fps must less than 20fps@3840*2160!\n", __func__, __LINE__);
        }

        g_bBiLinear = TRUE;
    }
    else if((2592 == g_videoMaxWidth) && (1944 == g_videoMaxHeight))
    {
        if((FALSE == mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 30))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 30;
            printf("%s:%d Sensor fps must less than 30fps!\n", __func__, __LINE__);
        }
        else if((mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 25))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 25;
            printf("%s:%d enable HDR mode, and Sensor fps must less than 25fps@5MP!\n", __func__, __LINE__);
        }

        g_bBiLinear = TRUE;
    }
    else if((1920 < g_videoMaxWidth) && (1080 < g_videoMaxHeight))
    {
        if((FALSE == mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 30))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 30;
            printf("%s:%d Sensor fps must less than 30fps!\n", __func__, __LINE__);
        }
        else if((mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 30))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 30;
            printf("%s:%d enable HDR mode, and Sensor fps must less than 30fps!\n", __func__, __LINE__);
        }
    }
    else/* if((1920 >= g_videoMaxWidth) && (1080 >= g_videoMaxHeight))*/
    {
        if((FALSE == mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 60))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 60;
            printf("%s:%d enable HDR mode, and Sensor fps must less than 60fps!\n", __func__, __LINE__);
        }
        else if((mixer_GetHdrValue()) &&  (g_videoParam[0].stVifInfo.sensorFrameRate > 30))
        {
            g_videoParam[0].stVifInfo.sensorFrameRate = 30;
            printf("%s:%d enable HDR mode, and Sensor fps must less than 30fps!\n", __func__, __LINE__);
        }
    }
#endif

    if(g_openIQServer)
    {
        //-c
        if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
        {
            printf("when use -q, do not support -c\n");
            return -1;
        }

        //audio
        if(TRUE == g_audioOutParam[0].bFlag)
        {
            printf("when use -q, do not support audio -a.\n");
            return -1;
        }

        // -q only support -v 0 or -v 1
        for(i = 0; i < g_videoNumber; i++)
        {
            if(pVideoParam[i].encoderType != VE_AVC && pVideoParam[i].encoderType != VE_H265)
            {
                printf("when use -q, only support -v 0 or -v 1.\n");
                return -1;
            }
        }
    }
#if MIXER_SED_ENABLE
    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
    {
        if(g_EnableSed)
        {
            printf("now mixer is no support : sed and vdf is conflict because they use divp chn 0 \n");
            return -1;
        }
    }
#endif

    return 0;
}
void MixerCheckVPEoutport()
{
#if TARGET_CHIP_I6B0
    MI_U16 vpeport2_width=0,vpeport2_height=0;
    MI_BOOL bHalf_ring_mode=FALSE;
#endif
    MI_U32 i = 0x0, j = 0x0;
    for(i = 0; i < g_videoNumber; i++)
    {

        if(g_videoParam[i].width > g_videoParam[i].MaxWidth)
        {
            g_videoParam[i].width = g_videoParam[i].MaxWidth;
        }
        if(g_videoParam[i].height > g_videoParam[i].MaxHeight)
        {
            g_videoParam[i].height = g_videoParam[i].MaxHeight;
        }

        g_videoParam[i].u16VpeOutWidth = g_videoParam[i].width;
        g_videoParam[i].u16VpeOutHeight = g_videoParam[i].height;

        for(j = i + 1; j < g_videoNumber; j++)
        {
            if(g_videoParam[i].stVpeChnPort.u32PortId == g_videoParam[j].stVpeChnPort.u32PortId)
            {
                if(g_videoParam[i].width  < g_videoParam[j].width)
                {
                    g_videoParam[i].u16VpeOutWidth    = g_videoParam[j].width;
                }
                else
                {
                    g_videoParam[j].u16VpeOutWidth    = g_videoParam[i].width;
                }

                if(g_videoParam[i].height < g_videoParam[j].height)
                {
                    g_videoParam[i].u16VpeOutHeight = g_videoParam[j].height;
                }
                else
                {
                    g_videoParam[j].u16VpeOutHeight = g_videoParam[i].height;
                }
            }
         //MIXER_DBG("vpe_outw:%d, vpe_outh:%d\n", g_videoParam[i].u16VpeOutWidth, g_videoParam[i].u16VpeOutHeight);
        }
    }
    for(i = 0; i < g_videoNumber-1; i++)
    {
        j = g_videoNumber-1;
        if(g_videoParam[i].stVpeChnPort.u32PortId == g_videoParam[j].stVpeChnPort.u32PortId)
        {
            if(g_videoParam[i].width > g_videoParam[j].width)
            {
                g_videoParam[j].u16VpeOutWidth    = g_videoParam[i].width;
            }

            if(g_videoParam[i].height > g_videoParam[j].height)
            {
                g_videoParam[j].u16VpeOutHeight = g_videoParam[i].height;
            }
        }
    }
#if TARGET_CHIP_I6B0
    //for I6B0,-V 2_4_5 -r 4_0_7 IS allowed.
    for(i = 0; i < g_videoNumber; i++)
    {
        if(g_videoParam[i].FromPort == 2)
        {
            vpeport2_width=g_videoParam[i].width;
            vpeport2_height=g_videoParam[i].height;
            break;
        }
    }
    if(vpeport2_width!=0 && vpeport2_height!=0)
    {
        for(i = 0; i < g_videoNumber; i++)
        {
            if(g_videoParam[i].stVpeChnPort.u32PortId == 2)
            {
                g_videoParam[i].u16VpeOutWidth=vpeport2_width;
                g_videoParam[i].u16VpeOutHeight=vpeport2_height;
            }
        }
    }
    //workaround sdk bug:half ring mode ,port2 need depth=4,to fix frame rate no enough problem,mantis=1687163
    for(i = 0; i < g_videoNumber; i++)
    {
	if(g_videoParam[i].stVpeChnPort.u32PortId == 1 && g_videoParam[i].eBindMode==Mixer_Venc_Bind_Mode_HW_HALF_RING)
	{
	    bHalf_ring_mode=TRUE;
	    MIXER_DBG("HALF RING MODE,vpe port2 need SET vpeBufCntQuota=4\n");
	    break;
        }
    }
    if(bHalf_ring_mode==TRUE)
    {
	for(i = 0; i < g_videoNumber; i++)
	{
	    if(g_videoParam[i].stVpeChnPort.u32PortId == VPE_SUB_PORT)
		g_videoParam[i].vpeBufCntQuota=4;
	}
    }
#endif
}


#if TARGET_CHIP_I6
void MixerCfgSyncUserParam(MI_U8 videoNum)
{
  ModuleVencInfo_s *VencInfo_t = GetVencInfo();
  for(MI_U8 chn=0; chn<videoNum; chn++)
  {
   if((g_videoParam[chn].encoderType != VencInfo_t->EncodeType[chn])&&\
       g_videoParam[chn].eBindMode == VencInfo_t->BindModuleType[chn])//扢离bindtype祥扢离encode奀剒猁統杅潰脤
   {
     if((VE_AVC == g_videoParam[chn].encoderType)||(VE_H265 == g_videoParam[chn].encoderType))
     {
        if((Mixer_Venc_Bind_Mode_FRAME != g_videoParam[chn].eBindMode)&&(Mixer_Venc_Bind_Mode_HW_RING != g_videoParam[chn].eBindMode))
          {
            g_videoParam[chn].eBindMode = Mixer_Venc_Bind_Mode_NUM;
          }
        VencInfo_t->EncodeType[chn] = g_videoParam[chn].encoderType;
     }
     else if((VE_MJPEG == g_videoParam[chn].encoderType)||(VE_JPG == g_videoParam[chn].encoderType))
     {
        if((Mixer_Venc_Bind_Mode_REALTIME != g_videoParam[chn].eBindMode))
           {
          g_videoParam[chn].eBindMode = Mixer_Venc_Bind_Mode_NUM;
           }
        VencInfo_t->EncodeType[chn] = g_videoParam[chn].encoderType;
     }
     else
     {
       g_videoParam[chn].eBindMode = Mixer_Venc_Bind_Mode_NUM;
     }
   }
   else if((g_videoParam[chn].encoderType == VencInfo_t->EncodeType[chn])&&\
       g_videoParam[chn].eBindMode != VencInfo_t->BindModuleType[chn])
    {
        if((VE_AVC == g_videoParam[chn].encoderType)||(VE_H265 == g_videoParam[chn].encoderType))
        {
           if((Mixer_Venc_Bind_Mode_FRAME != g_videoParam[chn].eBindMode)&&(Mixer_Venc_Bind_Mode_HW_RING != g_videoParam[chn].eBindMode))
           {
              g_videoParam[chn].eBindMode = Mixer_Venc_Bind_Mode_NUM;
           }
           VencInfo_t->BindModuleType[chn] = g_videoParam[chn].eBindMode;
        }
        else if((VE_MJPEG == g_videoParam[chn].encoderType)||(VE_JPG == g_videoParam[chn].encoderType))
        {
           if((Mixer_Venc_Bind_Mode_REALTIME != g_videoParam[chn].eBindMode))
           {
              g_videoParam[chn].eBindMode = Mixer_Venc_Bind_Mode_NUM;
           }
           VencInfo_t->BindModuleType[chn] = g_videoParam[chn].eBindMode;
        }
        else
        {
          VencInfo_t->EncodeType[chn] = g_videoParam[chn].encoderType;
        }
      }
    else if(g_videoParam[chn].width*g_videoParam[chn].height > VencInfo_t->StreamHeight[chn]*VencInfo_t->StreamWidth[chn])
       {
         VencInfo_t->StreamWidth[chn] = g_videoParam[chn].width;
      VencInfo_t->StreamHeight[chn] = g_videoParam[chn].height;
      if((VencInfo_t->StreamMaxHeight[chn] < g_videoParam[chn].MaxHeight) ||(VencInfo_t->StreamMaxWidth[chn] < g_videoParam[chn].MaxWidth))
      {
        VencInfo_t->StreamMaxHeight[chn] = g_videoParam[chn].MaxHeight;
        VencInfo_t->StreamMaxWidth[chn] = g_videoParam[chn].MaxWidth;
      }
       }
  }
}
#endif
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
void mixer_set_sensorNum(MI_U8 num)
{
  Mixer_SetSensorNum(num);
}
#endif


#if defined (MIXER_VERSION)
#define TO_STR_NATIVE(e) #e
#define TO_STR_PROXY(m, e) m(e)
#define MACRO_TO_STRING(e) TO_STR_PROXY(TO_STR_NATIVE, e)
static char *mixer_module_version = (char*)MACRO_TO_STRING(MIXER_VERSION)"\n";
#endif

MI_S32 main(int argc, char** argv)
{
    MI_S32 viChnParam[MAX_VIDEO_NUMBER];
    MI_S32 viParamNum = 0;
    MI_S32 result;
    MI_S32 vifparam[6];
    MI_S32 nCus3aispParam[3] = {0};
    MI_U32 i = 0x0, j = 0x0;
    MI_S8  configPath[64] ="/tmp/sysConfig.json";

    MIXER_DBG("version info: %s.\n", mixer_module_version);

    printf("========== Make Time: %s ==========\n", MAKE_TIME);
#if TARGET_CHIP_I5
    printf("========== This mixer run on platform: %s ==========\n", "SSC326D, SSC328Q, SSC329Q");
#elif TARGET_CHIP_I6
    printf("========== This mixer run on platform: %s ==========\n", "SSC325XX, SSC327XX");
#elif TARGET_CHIP_I6E
    printf("========== This mixer run on platform: %s ==========\n", "SSC336XX, SSC338XX");
#elif TARGET_CHIP_I6B0
    printf("========== This mixer run on platform: %s ==========\n", "SSC335XX, SSC337XX");

#endif


    system("cat /sys/devices/soc0/revision");
    system("cat /sys/devices/soc0/family");
    system("cat /sys/devices/soc0/machine");
    system("cat /proc/mi_modules/mi_sys/module_version_file");

    memset(g_ISPBinFilePath, 0x00, sizeof(g_ISPBinFilePath));
    sprintf((char *)g_ISPBinFilePath, "date -s \"%s\"", MAKE_TIME);
    system((char *)g_ISPBinFilePath);
    memset(g_ISPBinFilePath, 0x00, sizeof(g_ISPBinFilePath));

    memset((char*)g_s8OsdFontPath, 0x00, sizeof(g_s8OsdFontPath));
    memcpy((char*)g_s8OsdFontPath, MIXER_FONT_PATH, MIN(sizeof(g_s8OsdFontPath), strlen(MIXER_FONT_PATH)));

    g_PacketModule->init();
    g_PacketModule->start();

    g_ShowFrameInterval = FALSE;
#if LOAD_MIXER_CFG
    Get_AllCustomerConfigInfo((MI_S8 *)configPath);
#endif

    mixer_init_videoAudioParam();

    while((result = getopt(argc, argv, "a:A:b:B:c:C:d:D:e:E:f:F:g:G:i:I:j:J:k:K:l:L:m:M:n:N:p:P:Q:r:R:s:S:t:T:U:u:v:V:w:W:X:Y:z:Z:Uhoqxy")) != -1)
    {
        switch(result)
        {
            case 'a': mixer_set_audioType(); break;
            case 'A': mixer_set_audioFunc(); break;
            case 'b': mixer_set_videoBitrate(); break;
            case 'B': mixer_set_ModuleBufCnt(); break;
            case 'c': mixer_set_IeParam(); break;
            case 'C': mixer_set_CaliDBPath(); break;
            case 'd': mixer_set_OsdDisplay(); break;
            case 'e': mixer_set_audioPath(); break;
            case 'E': mixer_set_video_RateCtlType(); break;
            case 'f': mixer_set_videoFrameRate(); break;
#if TARGET_CHIP_I6
            case 'F': mixer_set_videoPixelFormat(); break;
#endif
            case 'g': mixer_set_videoGop(); break;
            case 'G': mixer_set_videoLTR(); break;
            case 'h': display_help(); return 0;
            case 'i': mixer_set_ISPcusAe(nCus3aispParam); break;
            case 'I': mixer_set_IspBinFilePath(); break;
            case 'j': { gDebug_ircutType = mixerStr2Int(optarg); } break;
            case 'J': mixer_set_VideoChnRoiConfig(); break;
            case 'k': mixer_set_AudioVideoTimestamp(); break;
            case 'K': mixer_set_VideoMaskOSD(); break;
#if TARGET_CHIP_I6E
            case 'l': mixer_set_Ldc();break;
#endif

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
            case 'L': mixer_set_videoBindMode(); break;
#endif
            case 'm': mixer_set_VideoRotation(&g_rotation); break;
            case 'M': mixer_set_videoShareSize(); break;
            case 'n': mixer_set_videoNumber(&g_videoNumber); break;
            case 'N': mixer_set_videoHdr3DNR(); break;
            case 'o': { g_displayOsd = TRUE; g_s32OsdHandleCnt = g_s32OsdHandleCnt|0x01;} break; //Enable display Timestamp infomation
            case 'p': break; //Lowercase "p"
            case 'P': break; //Uppercase "P"
            case 'q': { g_openIQServer = TRUE; } break;
            case 'Q': break;
            case 'r': mixer_select_videoResolution(); break;
            case 'R': mixer_set_videoResolution(); break;
            case 's': mixer_set_sensorFrameRate(); break;
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
            case 'S': mixer_set_sensorNum(mixerStr2Int(optarg));break;
#endif
            case 't': mixer_enable_ShowFrameInterval(); break;
            case 'T': break;
            case 'u': mixer_set_ircut(); break;
            case 'U': mixer_set_UvcResolution(); break;
            case 'v': mixer_set_videoEncoderType(); break;
#if (TARGET_CHIP_I6 || TARGET_CHIP_I5 || TARGET_CHIP_I6B0)
            case 'V': mixer_set_videoViChnNum(viChnParam, &viParamNum); break;
#endif
            case 'w': mixer_set_videoSaveStreamFlag(); break;
            case 'W': break;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
            case 'x': { g_fixMMA = TRUE; } break;
#endif
//            case 'X': {memcpy(configPath,optarg,64);} break;
            case 'y': { g_remoteIQServer = TRUE; } break;
            case 'Y': { g_enableCus3A = TRUE; } break;
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
            case 'z': mixer_set_videoPIP(); break;
#endif
            case 'Z':mixer_set_videoSed();break;
            default:  display_help();
        }
    }
	if(MAX_VIDEO_NUMBER < g_videoNumber)
    {
    	display_help();
        printf("\nwhen -n, video num max = %d -c\n", MAX_VIDEO_NUMBER);
        return -1;
    }
    if(0 != mixerCheckParam(g_videoParam, g_videoNumber))
    {
        display_help();
        return 0;
    }
#if TARGET_CHIP_I6
    MixerCfgSyncUserParam(g_videoNumber);
#endif
    if(0 != mixerSetViChn(g_videoParam, g_videoNumber, viChnParam, viParamNum, g_ieVpePort))
    {
        display_help();
        exit(1);
    }
    MixerCheckVPEoutport();
    for(i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
    {
        g_audioInParam[i].u8AudioInNum = g_audioInParam[0].u8AudioInNum;
        memcpy(&g_audioInParam[i].stAudioAttr, &g_audioInParam[0].stAudioAttr, sizeof(g_audioInParam[0].stAudioAttr));
    }
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    mixer_Set_BindMode(g_videoParam, g_videoNumber);
    mixer_Check_FrameRate(g_videoParam, g_videoNumber);
#endif

    //reset vi 0 bitrate
    if(FALSE == g_bSetBitrate && (g_videoParam[0].width <= 640 && g_videoParam[0].height <= 640))
    {
        g_videoParam[0].bitrate = 1000000;
    }


    if((sizeof(MixerVideoParam) * g_videoNumber) < MIXER_MAX_MSG_BUFFER_SIZE)
    {
        printf("%s:%d sizeof(MixerVideoParam)=%d, Video Para length=%d\n", __func__, __LINE__, sizeof(MixerVideoParam), sizeof(MixerVideoParam)*g_videoNumber);
    }
    else
    {
        printf("%s:%d Video Para length(%d) is larger than %d and exit!\n", __func__, __LINE__, sizeof(MixerVideoParam)*g_videoNumber, MIXER_MAX_MSG_BUFFER_SIZE);
        exit(1);
    }
    mixer_printf_video_audio_info();
    struct sigaction sigAction;
    sigAction.sa_handler = signalStop;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGHUP,  &sigAction, NULL);  //-1
    sigaction(SIGINT,  &sigAction, NULL);  //-2
    sigaction(SIGQUIT, &sigAction, NULL);  //-3
    sigaction(SIGKILL, &sigAction, NULL);  //-9
    sigaction(SIGTERM, &sigAction, NULL);  //-15


    if(g_CMsgManager->Init())
    {
        printf("%s:%d mixer initial message queue error!\n", __func__, __LINE__);
        exit(1);
    }
   mixer_osdInfoLinkListInit();

    //system init
    mixer_send_cmd(CMD_SYSTEM_INIT, NULL, 0);
    mixer_send_cmd(CMD_VIDEO_SET_ROTATE_ENABLE, (MI_S8 *)&g_rotation, sizeof(g_rotation));

     if(g_displayOsd || gDebug_osdColorInverse)
     {
         mixer_send_cmd(CMD_OSD_PRE_INIT, NULL, 0);
     }

    vifparam[0] = (MI_S32)1;   //vif dev num
    vifparam[1] = (MI_S32)g_videoParam[0].stVifInfo.sensorFrameRate;
    vifparam[2] = (MI_S32)g_videoMaxWidth;
    vifparam[3] = (MI_S32)g_videoMaxHeight;
    vifparam[4] = (MI_S32)g_videoParam[0].stVifInfo.HdrType;
    vifparam[5] = (MI_S32)g_videoParam[0].stVifInfo.level3DNR;
    mixer_send_cmd(CMD_VIF_INIT, (MI_S8 *)vifparam, sizeof(vifparam));

    //system core back trace
    mixer_send_cmd(CMD_SYSTEM_CORE_BACKTRACE, NULL, 0);

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    if(TRUE == g_fixMMA)
    {
        mixer_send_cmd(CMD_VIDEO_MMA_ALLOC, (MI_S8*)&g_videoNumber, sizeof(g_videoNumber));
    }
#endif



    mixer_send_cmd(CMD_VIDEO_OPEN, (MI_S8 *)&g_videoParam, sizeof(MixerVideoParam) * g_videoNumber);

    if(g_roi_num)
    {
        MI_U32 param[9];
        j = 0;
        do
        {
            for(i = 0;i < 9;i++)
            {
                param[i] = g_roiParam[j];
                j++;
            }
            mixer_send_cmd(CMD_VIDEO_SET_ROICFG, (MI_S8 *) param, sizeof(Param));
            g_roi_num--;
        }while(g_roi_num);
    }


    for(i = 0; i < g_videoNumber; i++)
    {
        if(g_videoParam[i].gop > 0)
        {
            MI_S32 param[2];
            param[0] = i;
            param[1] = g_videoParam[i].gop;
            if(g_videoParam[i].encoderType == VE_AVC || g_videoParam[i].encoderType == VE_H265)
            {
                mixer_send_cmd(CMD_VIDEO_SET_GOP , (MI_S8 *) param, sizeof(param) * 2);
            }
        }
    }

    if(TRUE == g_audioInParam[0].bFlag)
    {
        if(sizeof(MixerAudioInParam) < MIXER_MAX_MSG_BUFFER_SIZE)
        {
            printf("%s:%d sizeof(MixerAudioInParam)=%d, Audio Para limit length=%d\n", __func__, __LINE__, sizeof(MixerAudioInParam), MIXER_MAX_MSG_BUFFER_SIZE);
        }
        else
        {
            printf("%s:%d AudioIn Para length(%d) is larger than %d and exit!\n", __func__, __LINE__, sizeof(MixerAudioInParam), MIXER_MAX_MSG_BUFFER_SIZE);
            exit(1);
        }

        mixer_send_cmd(CMD_AUDIO_OPEN, (MI_S8 *)&g_audioInParam, sizeof(MixerAudioInParam) * g_audioInParam[0].u8AudioInNum);
    }

    if(TRUE == g_audioOutParam[0].bFlag)
    {
        if(sizeof(MixerAudioOutParam) < MIXER_MAX_MSG_BUFFER_SIZE)
        {
            printf("%s:%d sizeof(MixerAudioOutParam)=%d, Audio Para limit length=%d\n", __func__, __LINE__, sizeof(MixerAudioOutParam), MIXER_MAX_MSG_BUFFER_SIZE);
        }
        else
        {
            printf("%s:%d AudioOut Para length(%d) is larger than %d and exit!\n", __func__, __LINE__, sizeof(MixerAudioOutParam), MIXER_MAX_MSG_BUFFER_SIZE);
            exit(1);
        }

        mixer_send_cmd(CMD_AUDIO_PLAY_MEDIA, (MI_S8 *)&g_audioOutParam, sizeof(MixerAudioOutParam) * g_audioOutParam[0].s32AudioOutNum);
    }

    if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam || g_dlaParam || g_hchdParam)
    {
        MI_S32 param[3];
        param[0] = g_ieVpePort;
        param[1] = g_ieWidth;
        param[2] = g_ieHeight;

        if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
        {
            param[1] = g_ieHeight;
            param[2] = g_ieWidth;
        }

        printf("CMD_IE_OPEN\n");
        mixer_send_cmd(CMD_IE_OPEN, (MI_S8 *)param, sizeof(param));

        if(g_fdParam)
        {
            g_modeParam |= (1 << 2);
            mixer_send_cmd(CMD_IE_FD_OPEN, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));
        }
        if(g_mdParam)
        {
            g_modeParam |= (1 << 0);
            mixer_send_cmd(CMD_IE_MD_OPEN, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));
        }
        if(g_odParam)
        {
            g_modeParam |= (1 << 1);
            mixer_send_cmd(CMD_IE_OD_OPEN, (MI_S8 *)&g_odParam, sizeof(g_odParam));
        }
        if(g_vgParam)
        {
            g_modeParam |= (1 << 5);
            mixer_send_cmd(CMD_IE_VG_OPEN, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));
        }
        if(g_hchdParam)
        {
            if(g_hchdParam == 1)
                g_modeParam |= (1 << 3);
            else
                g_modeParam |= (1 << 4);
            mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&g_hchdParam, sizeof(g_hchdParam));
        }
        if(g_dlaParam)
        {
            g_modeParam |= (1 << 6);
            mixer_send_cmd(CMD_IE_DLA_OPEN, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));
        }
        mixer_send_cmd(CMD_IE_SET_FRAME_INTERVAL, (MI_S8 *)&g_ieFrameInterval, sizeof(g_ieFrameInterval));
        //mixer_send_cmd(CMD_IE_SET_YUV_SAVE, (MI_S8 *)&g_ieLogParam, sizeof(g_ieLogParam));
    }
#if MIXER_SED_ENABLE
    if(g_EnableSed)
    {
        MI_S32 param[3]={0, 0, 0};
        param[0] = g_SedType;
        param[1] = g_SedDebug;
        mixer_send_cmd(CMD_SED_OPEN, (MI_S8 *)param, sizeof(param));
        MIXER_ERR("CMD_SED_OPEN:open sed\n");
    }
#endif
    //open onvif
    if(g_openOnvif)
    {
        mixer_send_cmd(CMD_SYSTEM_ONVIF_OPEN, NULL, 0);
    }

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        MI_S32 param;
        param = TRUE;

        if(gDebug_osdColorInverse)
        {
            mixer_send_cmd(CMD_OSD_COLOR_INVERSE_OPEN , NULL, 0);
        }

        if((0 != g_mdParam) || (0 != g_odParam) || (0 != g_fdParam) || (0 != g_vgParam) || (0 != g_hchdParam) || (0 != g_dlaParam))
        {
            param = 1;
            mixer_send_cmd(CMD_OSD_IE_OPEN, (MI_S8 *)&param, sizeof(param));
        }

        mixer_send_cmd(CMD_OSD_FULL_OPEN, (MI_S8 *)&g_s32OsdHandleCnt, sizeof(g_s32OsdHandleCnt));
        mixer_send_cmd(CMD_OSD_OPEN, (MI_S8 *)&g_videoNumber, sizeof(g_videoNumber));

        if(g_displayOsd)
        {
            if(g_displayVideoInfo)
            {
                mixer_send_cmd(CMD_OSD_OPEN_DISPLAY_VIDEO_INFO, NULL, 0);
            }

            if(g_s32OsdFlicker)
            {
                mixer_send_cmd(CMD_OSD_FLICKER, (MI_S8 *)&g_s32OsdFlicker, sizeof(g_s32OsdFlicker));
            }

            if(gDebug_osdPrivateMask)
            {
                mixer_send_cmd(CMD_OSD_PRIVATEMASK_OPEN , NULL, 0);
            }
            if(g_maskOsd)
            {
                usleep(2000000);  //do mask initial first!
                MI_S32 param = 0;
                for(i = 0; (i < g_videoNumber)&&(i < MAX_VIDEO_NUMBER); i++)
                {
                    printf("Create OSD MASK on Video Chn[%d], OSD MASK Number = %d\n", i, *(g_maskNum + i));
                    param = g_maskNum[i];
                    MasktoRect(i, param, NULL);
                }
            }
        }
    }

    if(g_ircut > 0)
    {
        MI_S32 param[3];
        param[0] = (MI_S32)(g_videoParam[0].stVifInfo.sensorFrameRate * 1000.0);
        param[1] = g_ircut;
        param[2] = gDebug_ircutType;
        mixer_send_cmd(CMD_SYSTEM_IRCUT_OPEN, (MI_S8 *)param, sizeof(param));
    }

    if(g_openUVCDevice)
    {
        printf("Open The UVC Device \n");
        mixer_send_cmd(CMD_UVC_INIT, &g_uvcParam, sizeof(g_uvcParam));
    }

    mixer_send_cmd(CMD_SYSTEM_LIVE555_OPEN, NULL, 0);
   // mixer_send_cmd(CMD_SYSTEM_LIVE555_SET_FRAMERATE, (MI_S8 *) & (g_videoParam[0].frameRate), sizeof(g_videoParam[0].frameRate));

   for(i = 0; (i < g_videoNumber) && (i < MAX_VIDEO_NUMBER) && (RATECTLTYPE_MAX != g_videoParam[i].u8RateCtlType); i++)
   {
       //bitrate control
       MI_S32 param[10], paramLen = 0;

       memset(param, 0x00, sizeof(param));
       param[0] = i;
       param[1] = (MI_S32)g_videoParam[param[0]].u8RateCtlType;

       if(RATECTLTYPE_CBR == param[1]) // cbr
       {
           if((VE_JPG == g_videoParam[param[0]].encoderType) || (VE_MJPEG == g_videoParam[param[0]].encoderType))
           {
               param[2] = g_videoParam[param[0]].bitrate;
               param[3] = g_videoParam[param[0]].u8MaxQfactor;
               param[4] = g_videoParam[param[0]].u8MinQfactor;
               paramLen = 5;
           }
           else
           {
               param[2] = g_videoParam[param[0]].bitrate;
               param[3] = g_videoParam[param[0]].maxQp;
               param[4] = g_videoParam[param[0]].minQp;
               param[5] = g_videoParam[param[0]].maxIQp;
               param[6] = g_videoParam[param[0]].minIQp;
               param[7] = g_videoParam[param[0]].IPQPDelta;
               paramLen = 8;
           }
       }
       else if(RATECTLTYPE_VBR == param[1]) // vbr
       {
           param[2] = g_videoParam[param[0]].bitrate;
           param[3] = g_videoParam[param[0]].maxQp;
           param[4] = g_videoParam[param[0]].minQp;
           param[5] = g_videoParam[param[0]].maxIQp;
           param[6] = g_videoParam[param[0]].minIQp;
           param[7] = g_videoParam[param[0]].IPQPDelta;
           param[8] = g_videoParam[param[0]].u8ChangePos;
           paramLen = 9;
       }
       else if(RATECTLTYPE_FIXQP == param[1]) // fixqp
       {
           if((VE_JPG == g_videoParam[param[0]].encoderType) || (VE_MJPEG == g_videoParam[param[0]].encoderType))
           {
               param[3] = g_videoParam[param[0]].s8Qfactor;
               paramLen = 3;
           }
           else
           {
               param[2] = g_videoParam[param[0]].maxQp;
               param[3] = g_videoParam[param[0]].minQp;
               paramLen = 4;
           }
       }
      mixer_send_cmd(CMD_VIDEO_SET_BITRATE_CONTROL, (MI_S8 *)param, sizeof(MI_S32) * paramLen);
   }


    if(g_openIQServer)
    {
        int param[1] = {0};

        if(FALSE == g_remoteIQServer)
        {
            param[0] = 0;   // use local IQ Server
        }
        else
        {
            param[0] = 1;   // use remote Server
        }

        mixer_send_cmd(CMD_SYSTEM_IQSERVER_OPEN, (MI_S8 *)param, sizeof(param));
    }

    if(nCus3aispParam[0] || nCus3aispParam[1] || nCus3aispParam[2])
    {
        //MI_S32 fifo_data[256];
        MI_S32 param[3] = {0};
        param[0] = (MI_BOOL)!!nCus3aispParam[0];
        param[1] = (MI_BOOL)!!nCus3aispParam[1];
        param[2] = (MI_BOOL)!!nCus3aispParam[2];
        mixer_send_cmd(CMD_ISP_OPEN_CUS3A, (MI_S8 *) &param, sizeof(param));
    }

    //open recording
    OpenRecord();
    for(MI_S32 i = 0; i < MAX_VIDEO_NUMBER; i++)
    {
        MI_S32 param[2]={0x0};

        if(0 <=  tmpRecStatus[i].ch && tmpRecStatus[i].ch < MAX_VIDEO_NUMBER)
        {
            param[0] = tmpRecStatus[i].ch;
            param[1] = tmpRecStatus[i].status;
            mixer_send_cmd(CMD_RECORD_SETMODE, (MI_S8 *)param, sizeof(param));
        }

    }
    //end

    usleep(1000 * 1000);
    //load isp cmd bin file
    if('\0' != g_ISPBinFilePath[0])
    {
        int param[2];
        param[0] = (int)g_ISPBinFilePath;
        param[1] = g_IspKey;
        mixer_send_cmd(CMD_ISP_LOAD_CMDBIN , (MI_S8 *)param, sizeof(param));
    }

    //load calibration data file (must be put after load isp cmd bin file)
    for(i = 0; i < SS_CALI_ITEM_MAX; i++)
    {
        if('\0' != g_CaliDBPath[i][0])
        {
            int param[3];
            param[0] = (int)g_CaliDBPath[i];
            param[1] = g_CaliItem[i];
            param[2] = 128; //i5
            mixer_send_cmd(CMD_ISP_LOAD_CALI_DATA, (MI_S8 *)param, sizeof(param));
        }
    }

#if 0
	IPU_InitInfo_S stIPUInitInfo;
	memset(&stIPUInitInfo,0x00,sizeof(stIPUInitInfo));
	stIPUInitInfo.enModelType = Model_Type_Hc;
	sprintf(stIPUInitInfo.szIPUfirmware,"%s","ipu_firmware.bin.20191230.shrink");
	sprintf(stIPUInitInfo.szModelFile,"%s","hc_fixed.tflite_sgsimg.img");
    mixer_send_cmd(CMD_IE_DLA_SETINITINFO, (MI_S8 *)&stIPUInitInfo, sizeof(IPU_InitInfo_S));

#endif
    mixerInputProcessLoop();

    // start exit mixer:
    sync();
    mixerExit(TRUE);
    printf("mixer app exit\n");

    return 0;
}
