#ifndef _ST_STREAM_H_
#define _ST_STREAM_H_

#include <semaphore.h>

#include "st_common.h"

#define MAXVIDEOPACKNUM 128
#define MAXNALUSIZE (128*1024)

#define MIN_AO_VOLUME           -60
#define MAX_AO_VOLUME           30
#define MIN_ADJUST_AO_VOLUME    -10
#define MAX_ADJUST_AO_VOLUME    7

#ifdef  __cplusplus
extern "C"
{
#endif

typedef enum
{
    E_H264_IFrame = 0,
    E_H264_PFrame,
    E_H264_BFrame,
    E_JPEG,
} Stream_FrameType_e;

typedef struct st_Tmp_VideoBuf_s
{
    MI_U32 timestamp; //timestamp of recv pack slice
    MI_U32 frameno; //sequence of recv pack slice
    MI_U16 TotalPackage;
    MI_U8 CurrPackage[MAXVIDEOPACKNUM];
    MI_S32 Len;
    MI_U8 isFull;
    MI_U8 frame_flag;
    MI_U8 *buffer;
} Tmp_VideoBuf_T;

//VideoStream
typedef struct st_VideoBufInfo_s
{
    MI_S32 s32FrameType; //Stream_FrameType_e
    MI_S32 s32FrameNo;
    MI_U32 u32TimeStamp;
    MI_U32 CurrPackseq[MAXVIDEOPACKNUM];
    MI_U16 seq_no;
    MI_U8 VideoBuff[128*1024]; //nalu
    MI_U8 CurrPackage[MAXVIDEOPACKNUM];

    MI_S32 s32NaluPackSize;
    MI_S32 TotalPackage;
    MI_S32 Mark;
} VideoBufInfo_T;

typedef struct st_StreamSync_s 
{
    pthread_mutex_t video_rec_lock;
    pthread_mutex_t video_dec_lock;
    MI_U32 s32Count;
    MI_S32 s32DecodeFlag;
    MI_S32 s32VdecReady;
} StreamSync_T;

typedef struct st_TimeStamp_s{
    unsigned int OldCurrVideo;     //上一次当前视频时间
    unsigned int CurrVideo;
    unsigned int OldCurrAudio;     //上一次当前音频时间
    unsigned int CurrAudio;
} st_TimeStamp_T;

MI_S32 StopPlayAudioFile(void);
MI_S32 StartPlayAudioFile(const char *WavAudioFile, MI_S32 s32AoVolume);

MI_S32 Stream_StartPlayVideo(void);
MI_S32 Stream_StopPlayVideo(void);

MI_S32 Stream_StartSendVideo(unsigned long IPaddr);
MI_S32 Stream_StopSendVideo();

MI_S32 Stream_StartSendAudio(unsigned long IPaddr);
MI_S32 Stream_StopSendAudio();
MI_S32 Stream_StartRecvAudio(MI_S32 s32VolValue, MI_BOOL bMute);
MI_S32 Stream_StopRecvAudio();


#ifdef  __cplusplus
}
#endif

#endif //_ST_STREAM_H_
