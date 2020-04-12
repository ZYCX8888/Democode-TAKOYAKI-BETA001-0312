/*
* public.h- Sigmastar
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
#ifndef __PUBLIC__H___
#define __PUBLIC__H___

#include <stdio.h>
#include <stdlib.h>

#include "mi_common_datatype.h"
#include "mi_sys.h"
#include "mi_common.h"
#include "mid_audio_type.h"



#define MAX_BUF_LEN 256

#define SEND_PACK_LEN_2K        (2048 * 1)
#define SEND_PACK_LEN_8K        (2048 * 8)
#define RECEIVE_PACK_LEN_2K     (2048 * 1)
#define RECEIVE_PACK_LEN_8K     (2048 * 8)

#define TYPE_ENCODE             0x10
#define TYPE_DECODE_SYNC        0x11
#define TYPE_DECODE_ASYNC       0x12
#define TYPE_ENCODE_AND_DECODE  0x13
#define TYPE_ENCODE_FD          0x18
#define TYPE_ENCODE_AND_PCM_AO  0x19

#define TYPE_PCM                0x14
#define TYPE_PCM_AO_SYNC        0x15
#define TYPE_PCM_AO_ASYNC       0x16
#define TYPE_PCM_AIAO_ASYNC     0x17
#define TYPE_PCM_PLAYBACK       0x1A

#define SOUND_MODE_OPTION            AUDIO_SOUND_MODE_MONO



typedef struct
{
    MI_S32 type;
    MediaType_e playload;
    AUDIO_DEV AudioDevId;
    MI_S32 aiochn;    // AI or AO
    MI_S32 acodecChn; // AENC or ADEC
    AudioSampleRate_e    sampleRate;
    AudioBitWidth_e      bitWidth;
    AudioSoundMode_e     soundMode;
    MI_S32 aivolume;
    MI_S32 aovolume;
} AudioParam_t;


typedef struct _Param
{
    HANDLE m_hVR;
    char m_pchCMSFile[MAX_BUF_LEN];
}Param;

typedef struct
{
    AudioFrame_t *pAudioFrame;
    MI_S32 AoChn;
    FILE *fp;
    MI_S32 total_size;
    MI_S32 pos;
} PcmUser_t;


typedef struct
{
    AudioStream_t *pAudioStream;
    FILE *fp;

    int total_size;
    int pos;
} User_t;


typedef struct _WaveHeader{         // HEADER BLOCK FOR WAVE FILES
    MI_S8  szRiff[0x4];             // Resource Interchange File Format
    MI_U32 dwWaveChunk;             // Size of waveform chunk (file size - 8)
    MI_S8  szWave[0x4];             // WAVE identifier
    MI_S8  szFmt[0x4];              // Chunk identifier wave format
    MI_U32 dwFormatSize;            // Size of format chunk (WAVEFORMAT)
    MI_U16 wFormatTag;              // WAVEFORMAT begin
    MI_U16 wChannels;
    MI_U32 dwSamplesPerSec;
    MI_U32 dwnAvgBytesPerSec;
    MI_U16 wBlockAlign;
    MI_U16 wBitsPerSample;             //WAVEFORMAT end
    MI_S8  szData[0x4];
    MI_U32 dwDataSize;
} WaveHeader;


//复制音频帧数据的专用数据结构
typedef struct
{
    MI_U8  *bufAddr;
    MI_U32  bufLen;
}AudioFrameData;

typedef struct Node
{
    AudioFrameData *data;
    Node *next;
}Node;

typedef struct
{
    Node *head;
    Node *tail;
}List;


extern int g_exitFlag_ao;
extern pthread_t pthreadMadp_Cy;
extern int recordAudio;
extern int nodesNum;
extern List aiDataList;
extern pthread_mutex_t aiDataListMutex;
extern AudioParam_t * theParam;

//尾插入链表
int pushBack(List *myList, AudioFrameData *p);
//pop首结点
AudioFrameData* popFront(List *myList);

//复制音频帧数据
AudioFrameData* copyAudioFrameData(AudioFrame_t *frame);
//释放复制过来的音频帧数据
void releaseAudioFrameData(AudioFrameData *p);


void insertAiDataIntoList(AudioFrame_t *frame);
AudioFrameData* getAiDataFromList();

int vd_filename_ensure(char* name, void *pParam);

#endif
