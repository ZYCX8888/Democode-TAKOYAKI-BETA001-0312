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
#ifndef _MID_SYS_H_
#define _MID_SYS_H_

#include "mid_common.h"

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

typedef enum
{
    VIDEO_PLAY_LOAD_NONE = 0x0,
    VIDEO_PLAY_LOAD_H264,
    VIDEO_PLAY_LOAD_H265,
    VIDEO_PLAY_LOAD_JPEG,
    VIDEO_PLAY_LOAD_MJPEG,
    VIDEO_PLAY_LOAD_YUV
}VIDEO_PLAY_LOAD_E;

typedef enum
{
    VIDEO_FRAME_TYPE_NONE = 0x0,
    VIDEO_FRAME_TYPE_I,
    VIDEO_FRAME_TYPE_P,
    VIDEO_FRAME_TYPE_B,
    VIDEO_FRAME_TYPE_YUV,
    VIDEO_FRAME_TYPE_JPEG
}VIDEO_FRAME_TYPE_E;

typedef enum
{
    AUDIO_PLAY_LOAD_NONE = 0x0,
    AUDIO_PLAY_LOAD_G711A,
    AUDIO_PLAY_LOAD_G711U,
    AUDIO_PLAY_LOAD_G726,
    AUDIO_PLAY_LOAD_AAC,
    AUDIO_PLAY_LOAD_AMR,
    AUDIO_PLAY_LOAD_PCM,
    AUDIO_PLAY_LOAD_ADPCM,
    AUDIO_PLAY_LOAD_OPUS,
}AUDIO_PLAY_LOAD_E;

typedef enum{
    VIDEO_MAIN_STREAM,
    VIDEO_SUB_STREAM,
    VIDEO_THRD_STREAM,
    VIDEO_SNAP_STREAM,
    VIDEO_YUV_STREAM,
    AUDIO_STREAM,
    AV_STREAM_MAX
}STREAM_TYPE;

#pragma pack (4)
typedef struct
{
    //时间戳(pts)
    MI_U64 nPts;

    //帧数据
    HANDLE pPacketAddr;

    //帧数据长度
    MI_U32 nLen;

    MI_U8  Ch;
    STREAM_TYPE  StreamType;
    //视频编码类型
    VIDEO_PLAY_LOAD_E nVideoPlayLoad;
    //视频帧类型(I帧/P帧/JPEG/YUV)
    VIDEO_FRAME_TYPE_E nFrameType;
    //音频编码类型
    AUDIO_PLAY_LOAD_E nAudioPlayLoad;
}FrameInf_t;
#pragma pack ()


typedef struct _mma_bufinfo
{
    MI_ModuleId_e eModule;
    MI_U32 u32Devid;
    MI_S32 s32ChnId;
    MI_U8  u8MMAHeapName[MI_MAX_MMA_HEAP_LENGTH];
    MI_U32 u32PrivateHeapSize;
}USER_MMA_INFO;


#endif

