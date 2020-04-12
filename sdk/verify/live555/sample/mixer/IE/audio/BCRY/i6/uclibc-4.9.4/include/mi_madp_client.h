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
#ifndef __MI_VDF_H__
#define __MI_VDF_H__

#include <stdint.h>
#include "mi_osd.h"
#include "mi_audio_type.h"
#include "mi_madp_public.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define MAIN_WINDOW_VICHN            0
#define VICHN0_RESOLUTION_WIDTH      1920
#define VICHN0_RESOLUTION_HEIGHT     1080

#define AI_MAX_CHANNEL_NUM           4
#define AO_MAX_CHANNEL_NUM           1
#define MAX_SUB_DEVICE               20

#define VENC_CHANNEL_NUM             10
#define VENC_MAX_CHANNEL_NUM         VENC_CHANNEL_NUM

#define MD_APP_DISPLAY_NAME          "MD1 Demo"
#define OD_APP_DISPLAY_NAME          "OD Demo"
#define HD_APP_DISPLAY_NAME          "BD Demo"
#define FD_APP_DISPLAY_NAME          "FD Demo"
#define FDFR_APP_DISPLAY_NAME        "FDFR Demo"
#define VG_APP_DISPLAY_NAME          "VG Demo"
#define AI_APP_DISPLAY_NAME          "AI Demo"
#define AO_APP_DISPLAY_NAME          "AO Demo"
#define MD_MODE1_APP_DISPLAY_NAME    "MD2 Demo"

typedef enum MI_MADP_IE_MODE_E
{
    MADP_IE_MODE_MD = 0,            //FULL_MD mode
    MADP_IE_MODE_OD,                //OD
    MADP_IE_MODE_FD,                //FD
    MADP_IE_MODE_FDFR,              //FDFR
    MADP_IE_MODE_VG,                //VG
    MADP_IE_MODE_HD,                //HD
    MADP_IE_MODE_AI,                //AI
    MADP_IE_MODE_AO,                //AO
    MADP_IE_MODE_APP,               //APP
    MADP_IE_MODE_MD2,            //REGION_MD mode
    MADP_IE_MODE_MAX
} MADP_IE_MODE_E;


/* 定义幻数 */
#define MSYS_IOCTL_MAGIC             'S'

/* 定义命令 */
#define IOCTL_MSYS_GET_RIU_MAP       _IO(MSYS_IOCTL_MAGIC, 0x15)
#define IOCTL_MSYS_GET_UDID          _IO(MSYS_IOCTL_MAGIC, 0x32)

typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long udid;
    unsigned int VerChk_Size;
} __attribute__((__packed__)) MSYS_UDID_INFO;


typedef struct _text_Widget_Attr_s
{
    char  string[64];
    Point_t stPoint;
    OsdFontSize_e size;
    Color_t stfColor;
    Color_t stbColor;
    Color_t steColor;
    BOOL bHard;
} TEXT_WIDGET_ATTR_T;

typedef struct _rect_Widget_Attr_s
{
    Rect_t stRect;
    Color_t stColor;
    BOOL bFill;
    BOOL bHard;
} RECT_WIDGET_ATTR_T;

typedef struct _madp_image_t
{
    U32 u32MsgType;
    U8  u8ImageAttr[124];
    U8  *pu8ImageData;
} __attribute__((__packed__)) MADP_IMAGE_T;


typedef struct
{
    S32 type;
    MediaType_e playload;
    AUDIO_DEV AudioDevId;
    S32 aiochn;    // AI or AO
    S32 acodecChn; // AENC or ADEC
    AudioSampleRate_e    sampleRate;
    AudioBitWidth_e      bitWidth;
    AudioSoundMode_e     soundMode;
    S32 volume;
} AudioParam_t;

typedef struct WAVE_FORMAT
{
    S16 wFormatTag;
    S16 wChannels;
    U32 dwSamplesPerSec;
    U32 dwAvgBytesPerSec;
    S16 wBlockAlign;
    S16 wBitsPerSample;
} WaveFormat_t;


int MI_MADP_GetUUID(char *dev, MSYS_UDID_INFO *pstUuidInfo);
int MI_MADP_GetThdSN(char *dev, int version);
int MI_MADP_ReadUsrKey(char *dev, int version);
int MI_MADP_WriteUsrKey(char *dev, int version);
int MI_MADP_GetKey(char *key_buf, uint8_t buf_len, uint16_t app_id);
int MI_MADP_SaveKey(char *key_buf, uint8_t buf_len, uint16_t app_id);

MI_RET MI_MADP_SetChnAttr(VI_CHN ViChn, ViChnAttr_t *pstAttr);
MI_RET MI_MADP_GetChnAttr(VI_CHN ViChn, ViChnAttr_t *pstAttr);
MI_RET MI_MADP_SetDevAttr(VI_CHN ViChn, ViDevAtrr_t *pstAttr);
MI_RET MI_MADP_GetDevAttr(VI_CHN ViChn, ViDevAtrr_t *pstAttr);
MI_RET MI_MADP_EnableChn(VI_CHN ViChn);
MI_RET MI_MADP_DisableChn(VI_CHN ViChn);
S32 MI_MADP_GetViChnNumber(void);
MI_RET MI_MADP_GetFrameTimeOutForSandBox(VI_CHN ViChn, VideoFrameInfo_t *pstFrameInfo, S32 u32MilliSec, U8 release);
MI_RET MI_MADP_Video_Exit(VI_CHN ViChn);

MI_RET MI_MADP_Regester_YUV_Callback(void *p_Fn);
MI_RET MI_MADP_DeRegester_YUV_Callback(void *p_Fn);
MI_RET MI_MADP_Set_YUV_FrameInterval(int frameInterval);
MI_RET MI_MADP_Start_ReceiveYUV(VI_CHN ViChn, U8 release);
MI_RET MI_MADP_Stop_ReceiveYUV(void);


MI_RET MI_MADP_Init(void);
MI_RET MI_MADP_DeInit(void);


#define OD_DIV_W 3
#define OD_DIV_H 3
#define MD_REGION_NUM 8
#define MASK_REGION_NUM 8

#define MAX_OD_RECT_NUMBER 9
#define MAX_FD_RECT_NUMBER 20
#define MAX_FR_RECT_NUMBER 20

#define MAX_VG_LINE_NUMBER 2


MI_RET MI_MADP_GetResolution(VI_CHN ViChn, Size_t *pstSize);
MI_RET MI_MADP_Osd_Init(void);
MI_RET MI_MADP_Osd_DeInit(void);
MI_RET MI_MADP_OSD_Exit(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div);
MI_RET MI_MADP_CreateOsdRectWidget(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div, Rect_t *rect, Color_t *color, BOOL bFill, BOOL bHard, BOOL bVisible);
MI_RET MI_MADP_CreateOsdRectWidgetEx(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div, MADP_OSD_T * pstMadpOsd, BOOL bVisible);
MI_RET MI_MADP_CreateOsdTextWidget(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 handle, Point_t *point, Color_t *color, BOOL bFill, BOOL bHard, BOOL bVisible);
MI_RET MI_MADP_DestroyOsdWidget(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div);
MI_RET MI_MADP_DestroyOsdWidgetEx(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div);
MI_RET MI_MADP_UpdateOsdRectWidget(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div, Rect_t* rect, Color_t *color, BOOL bFill, BOOL bHard, BOOL bVisible);
MI_RET MI_MADP_UpdateOsdRectWidgetEx(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div, U32 *pu32MDResult);
MI_RET MI_MADP_UpdateOsdTextWidget(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 handle, textWidgetAttr_t* pTextAttr, BOOL visible);
MI_RET MI_MADP_SetOsdWidgetVisible(VI_CHN ViChn, MADP_IE_MODE_E OsdType, U32 W_Div, U32 H_Div, BOOL visible);



#define TYPE_ENCODE             0x10
#define TYPE_DECODE_SYNC        0x11
#define TYPE_DECODE_ASYNC       0x12
#define TYPE_ENCODE_AND_DECODE  0x13
#define TYPE_PCM                0x14
#define TYPE_PCM_AO_SYNC        0x15
#define TYPE_PCM_AO_ASYNC       0x16
#define TYPE_PCM_AIAO_ASYNC     0x17
#define TYPE_ENCODE_FD          0x18
#define TYPE_ENCODE_AND_PCM_AO  0x19
#define SOUND_MODE_OPTION       AUDIO_SOUND_MODE_MONO

MI_RET MI_MADP_Audio_Init(void);
MI_RET MI_MADP_Audio_DeInit(void);

MI_RET MI_MADP_AO_Init(AudioParam_t *pstAudiopara);
MI_RET MI_MADP_AO_DeInit(void);
MI_RET MI_MADP_AO_SetDevAttr(AUDIO_DEV AudioDevId, const AioDevAttr_t *pstAttr);
MI_RET MI_MADP_AO_GetDevAttr(AUDIO_DEV AudioDevId, AioDevAttr_t *pstAttr);
MI_RET MI_MADP_AO_SetChnAttr(AIO_CHN AoChn, const AioChnAttr_t* pstChnParam);
MI_RET MI_MADP_AO_GetChnAttr(AIO_CHN AoChn, AioChnAttr_t* pstChnParam);
MI_RET MI_MADP_AO_EnableChn(AUDIO_DEV AudioDevId , AIO_CHN AoChn);
MI_RET MI_MADP_AO_DisableChn(AUDIO_DEV AudioDevId, AIO_CHN AoChn);
MI_RET MI_MADP_AO_Enable(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AO_Disable(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AO_SetVolume(AUDIO_DEV AudioDevId, S32 sVolume);
MI_RET MI_MADP_AO_GetVolume(AUDIO_DEV AudioDevId, S32* sVolume);
MI_RET MI_MADP_AO_SetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);
MI_RET MI_MADP_AO_GetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);
MI_RET MI_MADP_AO_Start(AUDIO_DEV AudioDevId, AIO_CHN VoChn);
MI_RET MI_MADP_AO_Thread_Exit(AUDIO_DEV AudioDevId, AIO_CHN VoChn);
MI_RET MI_MADP_AO_SendFrame(AUDIO_DEV AudioDevId, S32 AoChn, AudioFrame_t *pFrame, S32 sBlock, AdecCallback_t *pAoCB);


MI_RET MI_MADP_AI_Init(AudioParam_t *pstAudiopara);
MI_RET MI_MADP_AI_DeInit(void);
MI_RET MI_MADP_AI_SetDevAttr(AUDIO_DEV AudioDevId, const AioDevAttr_t *pstAttr);
MI_RET MI_MADP_AI_GetDevAttr(AUDIO_DEV AudioDevId, AioDevAttr_t *pstAttr);
MI_RET MI_MADP_AI_SetChnAttr(AIO_CHN AiChn, const AioChnAttr_t* pstChnParam);
MI_RET MI_MADP_AI_GetChnAttr(AIO_CHN AiChn, AioChnAttr_t* pstChnParam);
MI_RET MI_MADP_AI_EnableChn(AUDIO_DEV AudioDevId, AIO_CHN AiChn);
MI_RET MI_MADP_AI_DisableChn(AUDIO_DEV AudioDevId, AIO_CHN AiChn);
MI_RET MI_MADP_AI_Enable(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_Disable(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_SetVolume(AUDIO_DEV AudioDevId, S32 sVolume);
MI_RET MI_MADP_AI_GetVolume(AUDIO_DEV AudioDevId, S32 *sVolume);
MI_RET MI_MADP_AI_SetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);
MI_RET MI_MADP_AI_GetMute(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);
MI_RET MI_MADP_AI_StartFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn);
MI_RET MI_MADP_AI_GetFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioFrame_t *pFrame, S32 sMilliSec);
MI_RET MI_MADP_AI_ReleaseFrame(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioFrame_t *pstFrm);
MI_RET MI_MADP_AI_Thread_Exit(AUDIO_DEV AudioDevId, AIO_CHN AiChn);
MI_RET MI_MADP_AI_EnableReSmp(AUDIO_DEV AudioDevId, AIO_CHN AiChn, AudioSampleRate_e enOutSampleRate);
MI_RET MI_MADP_AI_DisableReSmp(AUDIO_DEV AudioDevId, AIO_CHN AiChn);
MI_RET MI_MADP_AI_EnableAEC(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_DisableAEC(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_EnableANR(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_DisableANR(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_EnableWNR(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_DisableWNR(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_EnableAGC(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_DisableAGC(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_EnableEQ(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_DisableEQ(AUDIO_DEV AudioDevId);
MI_RET MI_MADP_AI_SetVqeAttr(AUDIO_DEV AudioDevId, AI_VQE_CONFIG *pVqeConfig);
MI_RET MI_MADP_AI_GetVqeAttr(AUDIO_DEV AudioDevId, AI_VQE_CONFIG *pVqeConfig);
MI_RET MI_MADP_AI_GetRTVolume(AUDIO_DEV AudioDevId, U16 *pRtVolume);
MI_RET MI_MADP_AI_SetADC(AUDIO_DEV AudioDevId, AudioMute_t *ptMute);
S32 MI_MADP_AI_GetFd(AIO_CHN AiChn);


#define MADP_INIPARSER_FILE_NODE_GET
#ifdef MADP_INIPARSER_FILE_NODE_GET

#define MIXER_CFG_FILE          "mixer.ini"
#define __INIPARSE__            "INIPARSE"
#define BufferSize              127
#define MIXER_MD_SECTION_NAME   "MD_CFG"
#define MIXER_VG_SECTION_NAME   "VG_CFG"

typedef enum _line_status_
{
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
} line_status;

typedef struct _db_file_node
{
    char *key;
    int value;
    struct _db_file_node *next;
} DB_FILE_NODE;

typedef struct _db_file
{
    char sectionName[20];
    int dbFileNum;
    DB_FILE_NODE *fileNode;
} DB_FILE;

DB_FILE * db_file_init(const char *fname, const char *section);
int db_file_free(DB_FILE * db_file);
int DB_GetKeyValue(const DB_FILE * db_file, const char * key);
line_status iniparser_line(char * input_line, char * section, char * key, char * value);
char * strstrip(char * s);
#endif


#ifdef __cplusplus
}
#endif

#endif /* __MI_VDF_H__ */
