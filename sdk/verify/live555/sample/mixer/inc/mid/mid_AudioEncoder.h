/*
* mid_AudioEncoder.h- Sigmastar
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
#ifndef _MI_AUDIO_ENCODER_H_
#define _MI_AUDIO_ENCODER_H_

#include "List.h"
#include "mid_common.h"
#include "mid_audio_type.h"
#include "mi_ai.h"
#include "mi_ao.h"
#include "mi_sys.h"
#include "mid_sys.h"
#include "cycle_buffer.h"
#include "AudioChannelFrame.h"

#if MIXER_AED_ENABLE
#include "mi_aed.h"
#include "mi_lsd.h"
#endif

#include "ms_notify.h"
#include "public.h"


#define SEND_PACK_LEN                0x400 * 4
#define MI_AUDIO_SAMPLE_PER_FRAME    1024

#define AUDIO_VQE_SIZE_MAX           (sizeof(MI_AI_VqeConfig_t)>sizeof(MI_AO_VqeConfig_t))?sizeof(MI_AI_VqeConfig_t):sizeof(MI_AO_VqeConfig_t)

#ifndef AUDIO_DEV
#define AUDIO_DEV    int
#endif

typedef struct _AudioConsumer
{
    struct list_head ConsumerList;
    AudioChannelFrame *tChannel;
}AudioConsumer;

class MI_AudioConsumer
{

private:
    MI_U32 ConsumerCount;
    MyMutex        m_Mutex;
    struct list_head mConsumerList;
    //deoConsumer mTmpVideoConsumerList;

public:
        MI_AudioConsumer();
        ~MI_AudioConsumer();
        MI_BOOL AddConsumer(AudioChannelFrame &tframe);
        MI_BOOL DelConsumer(AudioChannelFrame &tframe);
        MI_U32 GetConsumerCount();
        MI_U32 Consumer(const FrameInf_t &pFrameInf);
};

class MI_AudioEncoder
{
public:
    static MI_AudioEncoder* createNew(MI_U32 streamId);

    void startAudioEncoder(MI_S32 audioIdx);
    void initAudioEncoder(MixerAudioInParam * pstAudioParam, MI_S32 audioIdx);

    static void stopAudioEncoder();
    static void uninitAudioEncoder();

    ~MI_AudioEncoder(void);

    MI_AudioConsumer &GetLiveChannelConsumer(){ return mLiveChannelConsumer;}

public:
    static BOOL g_audio_exit;
    static int g_audio_thread_exit;
    static MI_AudioEncoder* g_pAudioEncoderArray[MIXER_AI_MAX_NUMBER];
    static int g_audioEncoderNumber;

    static int fNumChannels;
    static int fBitsPerSample;
    static int fSampleFrequency;
    static MI_U32 g_s32AudioInNum;

    MI_SYS_ChnPort_t m_AiChnPort;
    MI_AUDIO_Attr_t  m_stAiDevAttr;

    MediaType_e m_AiMediaType;
    MI_S32      m_s32VolumeInDb;
    AUDIO_DEV   m_AencChn;
    pthread_t   m_pthreadId;
    pthread_t   m_pthreadId_SPL;
    pthread_t   m_pthreadId_Aout;

    MI_U32 m_u32UserFrameDepth;
    MI_U32 m_u32BufQueueDepth;
    MI_S8  m_s8FilePath[128];

    MI_BOOL m_bAudioInVqe;
    MI_AI_VqeConfig_t m_stAiVqeCfg;
    MI_AI_AencConfig_t m_stAencCfg;
    MI_AUDIO_SampleRate_e m_eAudioInReSampleRate;

private:
    MI_AudioEncoder(const int streamId);

protected:
    MI_AudioConsumer mLiveChannelConsumer;
};


#if MIXER_AED_ENABLE
typedef enum {
    AED_SEN_LOW,
    AED_SEN_MID,
    AED_SEN_HIGH
} AedSensitivity;

void AedSetOperatingPoint(MI_S32 point);
void AedSetSensitivity(AedSensitivity sen);
#endif //#if MIXER_AED_ENABLE

MI_S32 SetAIVolume(MI_S32 audioIdx, MI_S32 volume);
MI_S32 SetAOVolume(MI_S32 s32AudioInIdx,MI_S32 volume);
MI_S32 SetVQEStatus(MI_S8 setmodule, MI_S8 * ps8AudioVqeData,MI_U32 bufLen);

#endif //_MI_AUDIO_ENCODER_H_
