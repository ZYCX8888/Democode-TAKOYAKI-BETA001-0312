/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_AudioEncoder.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/22
* Description: Audio MiddleWare source file
*
*
* History:
*
*    1. Date  :        2018/6/22
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <iostream>
using namespace std;

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <malloc.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <math.h>

#include <vector>
#include <assert.h>

#include "mi_ai.h"
#include "mi_ao.h"
#include "mi_sys.h"
#include "mi_aio_datatype.h"

#include "mid_sys.h"
#include "mid_common.h"
#include "module_common.h"
#include "module_config.h"
#include "mid_audio_type.h"
#include "PacketModule.h"
#include "List.h"

#include "public.h"
#include "mid_AudioEncoder.h"
#include "g711.h"
#include "g726.h"

#include "PacketModule.h"

#define DEFAULT_AUDIO_STREAM_BUF_SIZE    "192000"
#define AUDIO_BUFFER_SIZE                (9 * 1000)  //160 * 1000
#define CONFIG_AUDIO_THREAD_NUMBER       2
#define WAV_G711A                        0x06
#define WAV_G711U                        0x07
#define WAV_G726                         0x45
#define WAV_PCM                          0x01
#define G726_16                          0x02
#define G726_24                          0x03
#define G726_32                          0x04
#define G726_40                          0x05
#define MIXER_AO_ADEC_G726_UNIT          (60)
#define MIXER_AO_ADEC_G726_UNIT_MAX      (960)


//extern int  g_audioInEnableG7xxExt;
extern int  g_audioOutEnableG7xxExt;
extern int  gDebug_AudioStreamLog;
extern int  gDebug_saveAudioData[MIXER_AI_MAX_NUMBER];
extern int  g_bUseTimestampAudio;
extern MI_AI_VqeConfig_t g_stAiVqeCfg;
extern MI_AO_VqeConfig_t g_stAoVqeCfg;

extern int AudioDetectToRECT(BOOL bBabyCry, BOOL bLoudSound, short LoudSounddB);


BOOL g_audioOut_exit = FALSE;

pthread_t g_pthreadId_SPL;
pthread_t g_pthreadId_Aout;
extern MixerAudioOutParam g_stAudioOutParam[MIXER_AO_MAX_NUMBER];


//Initialize the class's static member variables
MI_AudioEncoder* MI_AudioEncoder::g_pAudioEncoderArray[MIXER_AI_MAX_NUMBER] = { 0 };
int MI_AudioEncoder::g_audioEncoderNumber = 0;
BOOL MI_AudioEncoder::g_audio_exit = FALSE;
int MI_AudioEncoder::g_audio_thread_exit = 0;
static MI_U32 g_u32AudioInInit = 0;
static MI_U32 g_u32AudioOnInit = 0;

MI_U32 MI_AudioEncoder::g_s32AudioInNum  = 0;

int MI_AudioEncoder::fNumChannels = 1;
int MI_AudioEncoder::fBitsPerSample = 16;
int MI_AudioEncoder::fSampleFrequency = 8000;

#if MIXER_AED_ENABLE
AED_HANDLE g_aedHandle = NULL;
AedParams g_aedParams;
#endif //#if MIXER_AED_ENABLE

#if MIXER_LSD_ENABLE
LSD_HANDLE g_lsdHandle = NULL;
LSD_PARAMS g_lsdParams;
#endif //#if MIXER_LSD_ENABLE

int g_pointLength;
pthread_t g_playThread;

MI_AudioConsumer::MI_AudioConsumer()
{
    ConsumerCount = 0x0;

    INIT_LIST_HEAD(&mConsumerList);
}

MI_AudioConsumer::~MI_AudioConsumer()
{
    AudioConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;
    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, AudioConsumer, ConsumerList);
        if(NULL != tmp)
        {
            list_del(&tmp->ConsumerList);
            tmp->tChannel = NULL;
            free(tmp);
            tmp = NULL;
        }
    }
    m_Mutex.OnUnLock();
    INIT_LIST_HEAD(&mConsumerList);
    ConsumerCount = 0x0;
}

MI_BOOL MI_AudioConsumer::AddConsumer(AudioChannelFrame &tframe)
{
    AudioConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;
    m_Mutex.OnLock();

    //
    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, AudioConsumer, ConsumerList);

        if(NULL != tmp && NULL != tmp->tChannel && tmp->tChannel == &tframe)
        {
            MIXER_ERR("%p has already register in AudioConsumer\n", &tframe);
            m_Mutex.OnUnLock();
            return TRUE;
        }
    }

    tmp = (AudioConsumer *)malloc(sizeof(AudioConsumer));
    if(NULL == tmp)
    {
        MIXER_ERR("can not malloc AudioConsumer\n");
        m_Mutex.OnUnLock();
        return FALSE;
    }
    INIT_LIST_HEAD(&tmp->ConsumerList);
    tmp->tChannel = &tframe;
    //MIXER_DBG("tChannel is %p\n", tmp->tChannel);
    list_add_tail(&tmp->ConsumerList, &mConsumerList);

    ConsumerCount++;
    m_Mutex.OnUnLock();
    return TRUE;
}

MI_BOOL MI_AudioConsumer::DelConsumer(AudioChannelFrame &tframe)
{
    AudioConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, AudioConsumer, ConsumerList);

        if(NULL != tmp && NULL != tmp->tChannel && tmp->tChannel == &tframe)
        {
            //MIXER_DBG("fine it\n");

            list_del(&tmp->ConsumerList);
            tmp->tChannel = NULL;
            free(tmp);
            tmp = NULL;
            ConsumerCount--;
            break;
        }
    }
    m_Mutex.OnUnLock();

    return TRUE;
}

MI_U32 MI_AudioConsumer::GetConsumerCount()
{
    MI_U32 tmp=0x0;
    m_Mutex.OnLock();
    tmp = ConsumerCount;
    m_Mutex.OnUnLock();

    return tmp;
}

MI_U32 MI_AudioConsumer::Consumer(const FrameInf_t &pFrameInf)
{
    AudioConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, AudioConsumer, ConsumerList);
        //MIXER_DBG("tmp is %p\n", tmp);
        if(NULL != tmp && NULL != tmp->tChannel)
        {
            //MIXER_DBG("tChannel111 is %p\n", tmp->tChannel);
            tmp->tChannel->OnData(pFrameInf);
        }
    }
    m_Mutex.OnUnLock();

    return TRUE;
}

float AVERAGE_BABYCRY_RUN(int a)
{
    static unsigned int num = 0;
    static float avg = 0;

    if((a > 0) && (a < 2000))
    {
        if(num == 0)
            avg = 0;

        num++;
        avg = avg + (a - avg) / num;
    }

    return avg;
}

MI_AudioEncoder* MI_AudioEncoder::createNew(MI_U32 streamId)
{
    MI_AudioEncoder *pAudioEncoder = NULL;

    do {
        if(g_s32AudioInNum && (g_s32AudioInNum > streamId))
        {
            if(g_audioEncoderNumber >= MIXER_AI_MAX_NUMBER)
            {
                printf("MI_AudioEncoder createNew failed, audio encoder is full\n");
                return NULL;
            }
        }

        pAudioEncoder = new MI_AudioEncoder(streamId);

        g_pAudioEncoderArray[g_audioEncoderNumber] = pAudioEncoder;
        g_audioEncoderNumber++;

        return pAudioEncoder;
    }while(0);

    return NULL;
}

MI_AudioEncoder::MI_AudioEncoder(const int streamId)
	: m_AiMediaType(MT_PCM)
	, m_s32VolumeInDb(0)
	, m_AencChn(0)
	, m_pthreadId(0)
	, m_pthreadId_SPL(0)
	, m_pthreadId_Aout(0)
	, m_u32UserFrameDepth(0)
	, m_u32BufQueueDepth(0)
	, m_bAudioInVqe(FALSE)
	, m_eAudioInReSampleRate(E_MI_AUDIO_SAMPLE_RATE_16000)
{
		memset(&m_AiChnPort, 0, sizeof(MI_SYS_ChnPort_t));
		memset(&m_stAiDevAttr, 0, sizeof(MI_AUDIO_Attr_t));
		memset(&m_stAiVqeCfg, 0, sizeof(MI_AI_VqeConfig_t));
		memset(&m_stAencCfg, 0, sizeof(MI_AI_AencConfig_t));
		memset(&m_s8FilePath, 0, sizeof(m_s8FilePath));
}

MI_AudioEncoder::~MI_AudioEncoder()
{
    if(0 < g_audioEncoderNumber)
    {
        g_audioEncoderNumber--;
    }
}

void add_wave_header(MI_U32 u32AencIdx, WavHeader_t* tWavHead, MI_S32 raw_len)
{
    MI_AudioEncoder* pstAudioEncoder = NULL;
    pstAudioEncoder = MI_AudioEncoder::g_pAudioEncoderArray[u32AencIdx];
    MI_AUDIO_SampleRate_e eSamplerate; /*sample rate*/

    tWavHead->riff[0] = 'R';
    tWavHead->riff[1] = 'I';
    tWavHead->riff[2] = 'F';
    tWavHead->riff[3] = 'F';

    tWavHead->wave[0] = 'W';
    tWavHead->wave[1] = 'A';
    tWavHead->wave[2] = 'V';
    tWavHead->wave[3] = 'E';

    tWavHead->fmt_chunk_marker[0] = 'f';
    tWavHead->fmt_chunk_marker[1] = 'm';
    tWavHead->fmt_chunk_marker[2] = 't';
    tWavHead->fmt_chunk_marker[3] = 0x20;
    tWavHead->length_of_fmt = 0x10;

    if(E_MI_AUDIO_SAMPLE_RATE_INVALID != MI_AudioEncoder::g_pAudioEncoderArray[u32AencIdx]->m_eAudioInReSampleRate)
    {
        eSamplerate = MI_AudioEncoder::g_pAudioEncoderArray[u32AencIdx]->m_eAudioInReSampleRate;
    }
    else
    {
        eSamplerate = pstAudioEncoder->m_stAiDevAttr.eSamplerate;
    }

    if(MT_PCM == pstAudioEncoder->m_AiMediaType)
    {
        if(E_MI_AUDIO_SOUND_MODE_MONO == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
        {
            tWavHead->channels = 0x01;
        }
        else
        {
            tWavHead->channels = 0x02;
        }

        tWavHead->bits_per_sample  = 16;
        tWavHead->sample_rate = eSamplerate;
        tWavHead->format_type = 0x01;
        tWavHead->byterate = (tWavHead->bits_per_sample  * tWavHead->sample_rate * tWavHead->channels) / 8;
        tWavHead->block_align = (tWavHead->bits_per_sample  * tWavHead->channels) / 8;
    }
    else
    {
        if(MT_G711A == pstAudioEncoder->m_AiMediaType)
        {
            tWavHead->format_type = 0x06;
        }

        if(MT_G711U == pstAudioEncoder->m_AiMediaType)
        {
            tWavHead->format_type = 0x07;
        }

        if((MT_G711A == pstAudioEncoder->m_AiMediaType) || (MT_G711U == pstAudioEncoder->m_AiMediaType))
        {
#if TARGET_CHIP_I5
            if((MI_U32)AUDIO_SOUND_MODE_MONO == (MI_U32)pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6
            if(E_MI_AUDIO_SOUND_MODE_MONO == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6E
            if(E_MI_AUDIO_SOUND_MODE_MONO == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6B0
            if(E_MI_AUDIO_SOUND_MODE_MONO == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#endif
            {
                tWavHead->channels = 0x01;
            }
            else
            {
                tWavHead->channels = 0x02;
            }

            tWavHead->bits_per_sample   = 8;  //bitWidth;g711encodeåºæ¥æ?bitï¼è¿ééè¦åæ­?            tWavHead->sample_rate  = eSamplerate;
            tWavHead->sample_rate  = eSamplerate;
            tWavHead->byterate = (tWavHead->bits_per_sample  * tWavHead->sample_rate * tWavHead->channels) / 8;
            tWavHead->block_align = (tWavHead->bits_per_sample  * tWavHead->channels) / 8;
        }
/*
        if((MT_G726_16 == pstAudioEncoder->m_AiMediaType) || (MT_G726_24 == pstAudioEncoder->m_AiMediaType) ||
           (MT_G726_32 == pstAudioEncoder->m_AiMediaType) || (MT_G726_40 == pstAudioEncoder->m_AiMediaType))
        {
            tWavHead->format_type = 0x45;
            tWavHead->channels  = pstAudioEncoder->m_stAiDevAttr.u32ChnCnt;

#if TARGET_CHIP_I5
            if (E_MI_AUDIO_G726_MODE_40 == pstAudioEncoder->m_stAencCfg.stAencG726Cfg.eG726Mode)
#elif TARGET_CHIP_I6
            if (MT_G726_40 ==  pstAudioEncoder->m_AiMediaType)
#endif
            {
                tWavHead->block_align = 5;
                tWavHead->bits_per_sample = 5;
            }
#if TARGET_CHIP_I5
            else if (E_MI_AUDIO_G726_MODE_32 == pstAudioEncoder->m_stAencCfg.stAencG726Cfg.eG726Mode)
#elif TARGET_CHIP_I6
            else if (MT_G726_32 == pstAudioEncoder->m_AiMediaType)
#endif
            {
                tWavHead->block_align = 4;
                tWavHead->bits_per_sample = 4;
            }
#if TARGET_CHIP_I5
            else if (E_MI_AUDIO_G726_MODE_24 == pstAudioEncoder->m_stAencCfg.stAencG726Cfg.eG726Mode)
#elif TARGET_CHIP_I6
            else if (MT_G726_24 == pstAudioEncoder->m_AiMediaType)
#endif
            {
                tWavHead->block_align = 3;
                tWavHead->bits_per_sample = 3;
            }
#if TARGET_CHIP_I5
            else if (E_MI_AUDIO_G726_MODE_16 == pstAudioEncoder->m_stAencCfg.stAencG726Cfg.eG726Mode)
#elif TARGET_CHIP_I6
            else if (MT_G726_16 == pstAudioEncoder->m_AiMediaType)
#endif
            {
                tWavHead->block_align = 2;
                tWavHead->bits_per_sample = 2;
            }

            tWavHead->sample_rate  = eSamplerate;
            tWavHead->byterate = (tWavHead->bits_per_sample  * tWavHead->sample_rate * tWavHead->channels) / 8;
        }
*/
       if(MT_ADPCM == pstAudioEncoder->m_AiMediaType)
       {
#if TARGET_CHIP_I5
           if((MI_U32)AUDIO_SOUND_MODE_MONO  == (MI_U32)pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6
           if(E_MI_AUDIO_SOUND_MODE_MONO  == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6E
           if(E_MI_AUDIO_SOUND_MODE_MONO  == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#elif TARGET_CHIP_I6B0
            if(E_MI_AUDIO_SOUND_MODE_MONO  == pstAudioEncoder->m_stAiDevAttr.eSoundmode)
#endif
           {
               tWavHead->channels = 0x01;
           }
           else
           {
               tWavHead->channels = 0x02;
           }

           tWavHead->format_type = 0x11;
           tWavHead->bits_per_sample = 4;
           tWavHead->sample_rate = eSamplerate;
           tWavHead->byterate = (tWavHead->bits_per_sample  * tWavHead->sample_rate * tWavHead->channels) / 8;
           tWavHead->block_align = 1024 ;
       }
    }

    tWavHead->data_chunk_header[0] = 'd';
    tWavHead->data_chunk_header[1] = 'a';
    tWavHead->data_chunk_header[2] = 't';
    tWavHead->data_chunk_header[3] = 'a';
    tWavHead->data_size = raw_len;
    tWavHead->ChunkSize = raw_len + sizeof(WavHeader_t) - 8;

    return ;
}


#if MIXER_LSD_ENABLE
void LsdInit()
{
    /* LSD parameter setting */
   g_lsdParams.sample_rate= 8000;
   g_lsdParams.channel = 2;

    /* LSD init */
   g_lsdHandle = MI_LSD_Init(&g_lsdParams, &g_pointLength);
   MI_LSD_SetThreshold    (g_lsdHandle, -15);
}

void LsdUninit()
{
    MI_LSD_Uninit(g_lsdHandle);
}
#endif

#if MIXER_AED_ENABLE
void AedInit()
{
    /* AED parameter setting */
    g_aedParams.sample_rate = 8000;
    g_aedParams.enable_nr = 1;
    g_aedParams.channel = 2;

    /* AED init */
    g_aedHandle = MI_AED_Init(&g_aedParams, &g_pointLength);
    MI_AED_SetSensitivity(g_aedHandle, AED_SEN_MID);
    MI_AED_SetOperatingPoint(g_aedHandle, 0);
    MI_AED_SetLsdThreshold(g_aedHandle, -15);
}

void AedUninit()
{
    MI_AED_Uninit(g_aedHandle);
}

void AedSetSensitivity(AedSensitivity sen)
{
    if(g_aedHandle != NULL)
    {
        printf("MI_AED_SetSensitivity %d\n", sen);
        MI_AED_SetSensitivity(g_aedHandle, sen);
    }
    else
    {
        printf("aed not init\n");
    }
}

void AedSetOperatingPoint(S32 point)
{
    if(g_aedHandle != NULL)
    {
        printf("MI_AED_SetOperatingPoint %d\n", point);
        MI_AED_SetOperatingPoint(g_aedHandle, point);
    }
    else
    {
        printf("aed not init\n");
    }
}
#endif

void* Audio_Task(void * argv)
{
    MI_S32 eError = MI_SUCCESS;
    FILE *outfile = NULL;
    MI_S32 audioIdx = 0;
    MI_S8 s8OutFilePath[128];
    MI_AUDIO_Frame_t stAiChFrame;
    MI_AUDIO_AecFrame_t stAecFrm;
    MI_AudioEncoder* pstAudioEncoder = NULL;
    MI_U32 audioDataLen = 0;
    MI_AUDIO_SampleRate_e eSamplerate; /*sample rate*/
    MI_U8 eBitwidth;        /*bitwidth, 0:16bit; 1:24bit*/
    MI_U8 eSoundmode[16];   /*mono or stereo*/
    struct timeval curTimestamp/*, startTimeStame*/;
    FrameInf_t  FrameAudio;
    CPacket *pPacket = NULL;


    audioIdx = (MI_S32)argv;
    pstAudioEncoder = MI_AudioEncoder::g_pAudioEncoderArray[audioIdx];
    eSamplerate = pstAudioEncoder->m_stAiDevAttr.eSamplerate;
    eBitwidth   = (pstAudioEncoder->m_stAiDevAttr.eBitwidth == 0) ? 16 : 24;

    memset((char *)eSoundmode, 0x00, sizeof(eSoundmode));
    if(pstAudioEncoder->m_stAiDevAttr.eSoundmode == 0)
    {
        memcpy((char *)eSoundmode, "Mono", strlen("Mono")+1);
    }
    else
    {
        memcpy((char *)eSoundmode, "Stereo", strlen("Stereo")+1);
    }

    memset((char *)&stAiChFrame, 0x00, sizeof(MI_AUDIO_Frame_t));

    printf("Audio_Task%d+  pstAudioEncoder->m_AiMediaType=%d\n", pstAudioEncoder->m_AiChnPort.u32ChnId,pstAudioEncoder->m_AiMediaType);

    do
    {
        if((NULL == outfile) && (gDebug_saveAudioData[audioIdx] & (audioIdx + 1)))
        {
            memset((char *)s8OutFilePath, 0x00, sizeof(s8OutFilePath));

            switch(pstAudioEncoder->m_AiMediaType)
            {
                case MT_PCM:
            sprintf((char *)s8OutFilePath, "pcm_Chn%d_%d_%d_%s_%dRES.wav",   \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G711A:
            sprintf((char *)s8OutFilePath, "g711a_Chn%d_%d_%d_%s_%dRES.wav", \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode, \
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G711U:
            sprintf((char *)s8OutFilePath, "g711u_Chn%d_%d_%d_%s_%dRES.wav", \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char*)eSoundmode, \
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate);\
            break;

                case MT_G711:   \
            sprintf((char *)s8OutFilePath, "g711_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate,\
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_AMR:
            sprintf((char *)s8OutFilePath, "amr_Chn%d_%d_%d_%s_%dRES.amr",   \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_AAC:
            sprintf((char *)s8OutFilePath, "aac_Chn%d_%d_%d_%s_%dRES.aac",   \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G726:
            sprintf((char *)s8OutFilePath, "g726_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *) eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;
                case MT_MP3:
            sprintf((char *)s8OutFilePath, "mp3_Chn%d_%d_%d_%s_%dRES.mp3",   \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode, \
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_OGG:
            sprintf((char *)s8OutFilePath, "ogg_Chn%d_%d_%d_%s_%dRES.ogg",   \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_ADPCM:
            sprintf((char *)s8OutFilePath, "adpcm_Chn%d_%d_%d_%s_%dRES.wav", \
                audioIdx, \
                eSamplerate,\
                eBitwidth,\
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate);\
            break;

                case MT_OPUS:
            sprintf((char *)s8OutFilePath, "opus_Chn%d_%d_%d_%s_%dRES.opus", \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G726_16:
            sprintf((char *)s8OutFilePath, "g726(16)_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate);\
            break;

                case MT_G726_24:
            sprintf((char *)s8OutFilePath, "g726(24)_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G726_32:
            sprintf((char *)s8OutFilePath, "g726(32)_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate); \
            break;

                case MT_G726_40:
            sprintf((char *)s8OutFilePath, "g726(40)_Chn%d_%d_%d_%s_%dRES.wav",  \
                audioIdx, \
                eSamplerate, \
                eBitwidth, \
                (char *)eSoundmode,\
                MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_eAudioInReSampleRate);\
            break;

                default:
            MIXER_DBG("%s:%d the input Aenc Type(%d) is not support!", \
                __func__, \
                __LINE__, \
                pstAudioEncoder->m_AiMediaType); \
            return NULL;
            }

            if(0 == access((char *)s8OutFilePath, 0))
            {
                if(remove((char *)s8OutFilePath))
                {
                    printf("%s:%d remove \"%s\" fail!", __func__, __LINE__, s8OutFilePath);
                }
            }

            if(NULL != (outfile = fopen((char *)s8OutFilePath, "wb")))
            {
                WavHeader_t stWavHead;

                audioDataLen = 0;
                add_wave_header(audioIdx, &stWavHead, audioDataLen);
                fwrite(&stWavHead, sizeof(char), sizeof(WavHeader_t), outfile);
            }

            printf("%s:%d Start record AudioIn-%d, filename: %s\n", __func__, __LINE__, (audioIdx + 1), s8OutFilePath);
        }
        eError = MI_AI_GetFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, &stAecFrm, -1);
        if(MI_SUCCESS != eError)
        {
            MIXER_DBG( "MI_AI_GetFrame err=%d\n", eError);
            usleep(1000 * 10);
            continue;
        }

        if(gDebug_AudioStreamLog)
        {
            printf("MI_AI_GetFrame(%d, %d) read=%d\n", pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, stAiChFrame.u32Len);
            printf("get output data for process : len = %d\n", stAiChFrame.u32Len);
        }


        if(g_bUseTimestampAudio & (audioIdx + 1))
        {
            gettimeofday(&curTimestamp, NULL);
        }
        else
        {
            curTimestamp.tv_sec  = stAiChFrame.u64TimeStamp / 1000000LL;
            curTimestamp.tv_usec = stAiChFrame.u64TimeStamp % 1000000LL;
        }

        if(g_bUseTimestampAudio & (audioIdx + 1))
        {
            printf("audio chn = 0, s: %ld, us: %ld\n", curTimestamp.tv_sec, curTimestamp.tv_usec);
        }

        {
            MI_U32 u32AudioInDataSize = 0;

            if(MT_PCM == pstAudioEncoder->m_AiMediaType)
            {
               u32AudioInDataSize = stAiChFrame.u32Len;
            }
            else
            {
                u32AudioInDataSize = stAiChFrame.u32Len/2;
            }

            //PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());
            PacketModule* pobjPacketModule = g_PacketModule;
            switch(pstAudioEncoder->m_AiMediaType)
            {
                case MT_G711:
                case MT_G711U:
                    {
                        pPacket = (CPacket *)pobjPacketModule->MallocPacket(u32AudioInDataSize);
                        if(NULL == pPacket)
                        {
                            MIXER_ERR("can not alloc pPacket, size(%d)\n", u32AudioInDataSize);
                            MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, NULL);
                            continue;
                        }
/*
                        if(g_audioInEnableG7xxExt)
                        {
                            G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 1);
                        }
                        else
                        {
                            memcpy(pPacket->GetBuffer(), (short *)stAiChFrame.apVirAddr[0], u32AudioInDataSize);
                        }
*/
                        G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 1);
                        FrameAudio.nAudioPlayLoad = AUDIO_PLAY_LOAD_G711U;
                        FrameAudio.nLen = u32AudioInDataSize;
                        FrameAudio.Ch = audioIdx;
                        FrameAudio.StreamType = AUDIO_STREAM;
                        FrameAudio.nPts = stAiChFrame.u64TimeStamp;
                        FrameAudio.pPacketAddr = (void*)pPacket;
                    }
                    break;

                case MT_G711A:
                    {
                        pPacket = (CPacket *)pobjPacketModule->MallocPacket(u32AudioInDataSize);
                        if(NULL == pPacket)
                        {
                            MIXER_ERR("can not alloc pPacket, size(%d)\n", u32AudioInDataSize);
                            MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, NULL);
                            continue;
                        }
/*
                        if(g_audioInEnableG7xxExt)
                        {
                            G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 0);
                        }
                        else
                        {
                            memcpy(pPacket->GetBuffer(), (short *)stAiChFrame.apVirAddr[0], u32AudioInDataSize);
                        }
*/
                        G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 0);
                        FrameAudio.nAudioPlayLoad = AUDIO_PLAY_LOAD_G711A;
                        FrameAudio.nLen = u32AudioInDataSize;
                        FrameAudio.Ch = audioIdx;
                        FrameAudio.StreamType = AUDIO_STREAM;
                        FrameAudio.nPts = stAiChFrame.u64TimeStamp;
                        FrameAudio.pPacketAddr = (void*)pPacket;
                    }
                    break;

                case MT_G726:
                    {
                        pPacket = (CPacket *)pobjPacketModule->MallocPacket(u32AudioInDataSize);
                        if(NULL == pPacket)
                        {
                            MIXER_ERR("can not alloc pPacket, size(%d)\n", u32AudioInDataSize);
                            MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, NULL);
                            continue;
                        }
/*
                        if(g_audioInEnableG7xxExt)
                        {
                            G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 0);
                        }
                        else
                        {
                            memcpy(pPacket->GetBuffer(), (short *)stAiChFrame.apVirAddr[0], u32AudioInDataSize);
                        }
*/
                        G711Encoder((short *)stAiChFrame.apVirAddr[0], pPacket->GetBuffer(), u32AudioInDataSize, 0);
                        FrameAudio.nAudioPlayLoad = AUDIO_PLAY_LOAD_G726;
                        FrameAudio.nLen = u32AudioInDataSize;
                        FrameAudio.Ch = audioIdx;
                        FrameAudio.StreamType = AUDIO_STREAM;
                        FrameAudio.nPts = stAiChFrame.u64TimeStamp;
                        FrameAudio.pPacketAddr = (void*)pPacket;
                    }
                    break;

                case MT_PCM:
                default:
                    {
                        u32AudioInDataSize = stAiChFrame.u32Len;
                        pPacket = (CPacket *)pobjPacketModule->MallocPacket(u32AudioInDataSize);
                        if(NULL == pPacket)
                        {
                            MIXER_ERR("can not alloc pPacket, size(%d)\n", u32AudioInDataSize);
                            MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, NULL);
                            continue;
                        }

                        memcpy(pPacket->GetBuffer(), stAiChFrame.apVirAddr[0], stAiChFrame.u32Len);
                        FrameAudio.nAudioPlayLoad = AUDIO_PLAY_LOAD_PCM;
                        FrameAudio.nLen = stAiChFrame.u32Len;
                        FrameAudio.Ch = audioIdx;
                        FrameAudio.StreamType = AUDIO_STREAM;
                        FrameAudio.nPts = stAiChFrame.u64TimeStamp;
                        FrameAudio.pPacketAddr = (void*)pPacket;
                    }
                    break;
            }

            if((NULL != outfile) && (0 != (gDebug_saveAudioData[audioIdx] & (audioIdx + 1))))
            {
                fwrite(pPacket->GetBuffer(), sizeof(char), u32AudioInDataSize, outfile);
                audioDataLen += u32AudioInDataSize;
            }
            else if((NULL != outfile) && (0 == (gDebug_saveAudioData[audioIdx] & (audioIdx + 1))))
            {
                WavHeader_t stWavHead;

                add_wave_header(audioIdx, &stWavHead, audioDataLen);
                fseek(outfile, 0, SEEK_SET);
                fwrite(&stWavHead, sizeof(char), sizeof(WavHeader_t), outfile);
                fclose(outfile);
                outfile = NULL;
                printf("%s:%d Stop record AudioIn-%d, filename:%s\n", __func__, __LINE__, (audioIdx + 1), s8OutFilePath);
            }

            pPacket->AddRef();
            pstAudioEncoder->GetLiveChannelConsumer().Consumer(FrameAudio);
            pPacket->Release();
        }

        //release audio frame
        MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame,  NULL);
        usleep(1000);
    }while(MI_AudioEncoder::g_audio_exit == FALSE);


    if(outfile && (gDebug_saveAudioData[audioIdx] & (audioIdx + 1)))
    {
        WavHeader_t stWavHead;

        add_wave_header(audioIdx, &stWavHead, audioDataLen);
        fseek(outfile, 0, SEEK_SET);
        fwrite(&stWavHead, sizeof(char), sizeof(WavHeader_t), outfile);
        fclose(outfile);
        outfile = NULL;
        printf("%s:%d Stop record AudioIn-%d, filename:%s\n", __func__, __LINE__, (audioIdx + 1), s8OutFilePath);
    }

    printf("Audio_Task%d-\n", pstAudioEncoder->m_AiChnPort.u32ChnId);

    MI_AudioEncoder::g_audio_thread_exit++;
    pthread_exit(NULL);
    return NULL;
}


void *AudioSPL_Task(void * argv)
{
    MI_S32 eError = MI_SUCCESS;
    MI_AUDIO_Frame_t stAiChFrame;
    MI_AUDIO_AecFrame_t stAecFrame;
    MI_S32 audioIdx = 0;
    FILE *outfile = NULL;
    FILE *outfile_alg = NULL;
    MI_S8 s8OutFilePath[128];
    WavHeader_t tWavHead;
    MI_U32 g_rawlen = 0x0;
    MI_AudioEncoder* pstAudioEncoder = NULL;


    audioIdx = (MI_S32)argv;
    pstAudioEncoder = MI_AudioEncoder::g_pAudioEncoderArray[audioIdx];


    memset(&stAiChFrame, 0x00, sizeof(MI_AUDIO_Frame_t));

    //  unsigned int T0=0,T1=0;
    if(gDebug_saveAudioData[audioIdx] & (audioIdx + 1))
    {
        memset((char *)s8OutFilePath, 0x00, sizeof(s8OutFilePath));
        sprintf((char *)s8OutFilePath, "audio_dev%dchn%d.pcm", pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId);
        outfile = fopen((char *)s8OutFilePath, "wb");

        memset((char *)s8OutFilePath, 0x00, sizeof(s8OutFilePath));
        sprintf((char *)s8OutFilePath, "audio_alg_dev%dchn%d.pcm", pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId);
        outfile_alg = fopen((char *)s8OutFilePath, "wb");

#if TARGET_CHIP_I6
        memset(&tWavHead, 0x00, sizeof(WaveHeader));

        if((MT_PCM == pstAudioEncoder->m_AiMediaType) || \
           (MT_G711A == pstAudioEncoder->m_AiMediaType) ||\
           (MT_G711U == pstAudioEncoder->m_AiMediaType))
        {
            add_wave_header(0x0, &tWavHead, g_rawlen);
            fseek(outfile, 0, SEEK_SET);
            fwrite(&tWavHead, 1, sizeof(WaveHeader), outfile);
        }
#endif
    }

    printf("AudioSPL_Task%d+\n", pstAudioEncoder->m_AiChnPort.u32ChnId);

    do {
        memset(&stAiChFrame, 0x00, sizeof(MI_AUDIO_Frame_t));
        memset(&stAecFrame, 0x00, sizeof(MI_AUDIO_AecFrame_t));
        eError = MI_AI_GetFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, &stAecFrame, -1);
        if(MI_SUCCESS != eError)
        {
            MIXER_DBG( "MI_AI_GetFrame err=%d\n", eError);
            continue;
        }

        if((gDebug_saveAudioData[audioIdx] & (audioIdx + 1)) && outfile)
        {
            fwrite(stAiChFrame.apVirAddr[0], sizeof(char), stAiChFrame.u32Len, outfile);
        }

        if(gDebug_AudioStreamLog)
        {
            printf("MI_AI_GetFrame1 read=%d\n", stAiChFrame.u32Len);
        }

#if MIXER_AED_ENABLE || MIXER_LSD_ENABLE
        if(g_aedHandle || g_lsdHandle)
        {
            int ret = 0;
            int aed_result = 0;
            int lsd_result = 0;
            short lsd_dBResult;

#if MIXER_LSD_ENABLE
            MI_LSD_GetdBResult(g_lsdHandle,  (S16 *)pFrame.apVirAddr[0], &lsd_dBResult);
            MI_LSD_Run(g_lsdHandle, &lsd_dBResult);
            ret = MI_LSD_GetResult(g_lsdHandle, (short*)(&lsd_result));
            if (ret < 0)
            {
                printf("Error occured in LSD, ret=%d\n",ret);
            }
            if (lsd_result)
            {
                if(lsd_dBResult!=0)
                    printf("LSD  loud sound detected!@#$*^?-~@/#&#~$^*!?-=~*%%#, %ddB\n",lsd_dBResult);
            }
#endif

#if MIXER_AED_ENABLE
            if(1)
            {
                /* Run LSD process */
                ret = MI_AED_RunLsd(g_aedHandle , (S16 *)pFrame.bufAddr, agc_gain);
                lsd_result = MI_AED_GetLsdResult(g_aedHandle);

                if(ret < 0)
                {
                    printf("IaaAed_RunLsd failed ret =%d\n", ret);
                }
                else if(lsd_result)
                {
                    printf("Aed  loud sound detected!@#$*^?-~@/#&#~$^*!?-=~*%%#\n");
                }
            }

            /* Run AED process */
            //T0=(long)BABYCRY_OsCounterGetUs();
            ret = MI_AED_Run(g_aedHandle, (S16 *)pFrame.bufAddr);
            //T1=(long)BABYCRY_OsCounterGetUs();
            aed_result = MI_AED_GetResult(g_aedHandle);

            if(ret < 0)
            {
                printf("Aed IaaAed_GetResult failed ret =%d\n", ret);
            }
            else if(aed_result)
            {
                printf("Aed baby is crying! >___________________________< \n");
            }
#endif
            if((gDebug_saveAudioData[audioIdx] & (audioIdx + 1)) && outfile_alg)
            {
                fwrite(stAiChFrame.apVirAddr[0], sizeof(char), stAiChFrame.u32Len, outfile_alg);
            }

            if(aed_result || lsd_result)
            {
                AudioDetectToRECT((BOOL)aed_result, (BOOL)lsd_result, lsd_dBResult);
            }
        }
#endif

        MI_AI_ReleaseFrame(pstAudioEncoder->m_AiChnPort.u32DevId, pstAudioEncoder->m_AiChnPort.u32ChnId, &stAiChFrame, NULL);

        /* Calculate AED_Run time and show to UART every 1s */
        static struct timeval timestamp = {0};
        static struct timeval lastTimestamp = {0};
        //static long T_Diff;
        //static float T_Diff_Avg;
        //T_Diff = (long)(T1-T0);
        //T_Diff_Avg = AVERAGE_BABYCRY_RUN(T1-T0);
        gettimeofday(&timestamp, 0);

        if(lastTimestamp.tv_sec > timestamp.tv_sec - 1)
        {
            continue ;
        }

        lastTimestamp = timestamp;
        //printf(" thBABYCRY:  time dur MI_AED_Run: %lu us, AVG is %.2f us\n",T_Diff, T_Diff_Avg);

        //usleep(u32SleepTime);
    }while(MI_AudioEncoder::g_audio_exit == FALSE);

    memset(&tWavHead, 0x00, sizeof(WavHeader_t));

    if((gDebug_saveAudioData[audioIdx] & (audioIdx + 1)) && outfile)
    {
        if((MT_PCM == pstAudioEncoder->m_AiMediaType)   || \
           (MT_G711A == pstAudioEncoder->m_AiMediaType) || \
           (MT_G711U == pstAudioEncoder->m_AiMediaType))
        {
            add_wave_header(0x0, &tWavHead, g_rawlen);
            fseek(outfile, 0, SEEK_SET);
            fwrite(&tWavHead, 1, sizeof(WavHeader_t), outfile);
        }

        fclose(outfile);
    }

    if((gDebug_saveAudioData[audioIdx] & (audioIdx + 1)) && outfile_alg)
    {
        fclose(outfile_alg);
    }

    MIXER_DBG("AudioSPL_Task%d-\n", pstAudioEncoder->m_AiChnPort.u32ChnId);
    MI_AudioEncoder::g_audio_thread_exit++;

    return NULL;
}

static MI_S32 mid_AdecInit(MixerAudioOutParam * pstAudioOutParam, WavHeader_t *pstWavHeader)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 audioIdx = 0;
    MI_AO_CHN AoChnId = 0;
    MI_AUDIO_DEV AoDevId[MI_AUDIO_DEV_ALL];
    MI_AUDIO_SampleRate_e eInSampleRate = E_MI_AUDIO_SAMPLE_RATE_8000;
    MI_AUDIO_Attr_t stAudioAttrGet;

    MI_AO_AdecConfig_t stAdecConfig;

    eInSampleRate = (MI_AUDIO_SampleRate_e)pstAudioOutParam->stWavHeader.sample_rate;

    MIXER_DBG("mid_AdecInit+\n");

    //for (audioIdx = 0; audioIdx < MI_AUDIO_DEV_ALL; audioIdx ++)
    {
        AoDevId[audioIdx] = pstAudioOutParam->stAudioOutChnPort.u32DevId;
        if (AoDevId[audioIdx] == -1)
        {
            MIXER_ERR("%s:%d get AoDevId[%d]=%d, and return!\n", __func__, __LINE__, audioIdx, AoDevId[audioIdx]);
            return -1; //continue;
        }

        /* makesure Ao device is disable */
        memset(&stAudioAttrGet, 0x00, sizeof(MI_AUDIO_Attr_t));

        if (MI_SUCCESS == MI_AO_GetPubAttr(AoDevId[audioIdx], &stAudioAttrGet))
/*        {
            for(MI_U32 u32AoChnId = 0; u32AoChnId < MIXER_AO_MAX_NUMBER; u32AoChnId++)
            {
                ExecFunc(MI_AO_DisableChn(AoDevId[audioIdx], u32AoChnId), MI_SUCCESS);
            }

            ExecFunc(MI_AO_Disable(AoDevId[audioIdx]), MI_SUCCESS);
        }
*/
        AoChnId = pstAudioOutParam->stAudioOutChnPort.u32ChnId;
        MIXER_DBG("u32FrmNum[%d]=%u\n", AoChnId, pstAudioOutParam->stAudioAttr.u32FrmNum);
        if (MI_SUCCESS != (s32Ret = MI_AO_SetPubAttr(AoDevId[audioIdx], &pstAudioOutParam->stAudioAttr)))
        {
            MIXER_ERR("%s %d, AoDev:%d, 0x%X\n", __func__, __LINE__, AoDevId[audioIdx], s32Ret);
            return 1;
        }

        /* enable ao device */
        ExecFunc(MI_AO_Enable(AoDevId[audioIdx]), MI_SUCCESS);

        /* enable ao channel of device*/
        ExecFunc(MI_AO_EnableChn(AoDevId[audioIdx], AoChnId), MI_SUCCESS);

#if TARGET_CHIP_I5
        /* Clear ao channel buff of device*/
        ExecFunc(MI_AO_ClearChnBuf(AoDevId[audioIdx], AoChnId), MI_SUCCESS);
#endif

        // if test resample, enable Resample
        if(pstAudioOutParam->u8AudioOutRes)
        {
            ExecFunc(MI_AO_EnableReSmp(AoDevId[audioIdx], AoChnId, eInSampleRate), MI_SUCCESS);
        }

        /* if test VQE: set attribute of AO VQE  */
        if(pstAudioOutParam->u8AudioOutVqe)
        {
            g_stAoVqeCfg.s32WorkSampleRate = pstAudioOutParam->stAudioAttr.eSamplerate;
            ExecFunc(MI_AO_SetVqeAttr(AoDevId[audioIdx], AoChnId, &g_stAoVqeCfg), MI_SUCCESS);
            ExecFunc(MI_AO_EnableVqe(AoDevId[audioIdx], AoChnId), MI_SUCCESS);
        }

       if((1 < pstWavHeader->format_type) && (0 == g_audioOutEnableG7xxExt))
        {
            if (pstWavHeader->format_type == WAV_G711A || pstWavHeader->format_type == WAV_G711U)
            {
                stAdecConfig.eAdecType = pstWavHeader->format_type == WAV_G711A ? E_MI_AUDIO_ADEC_TYPE_G711A : E_MI_AUDIO_ADEC_TYPE_G711U;
                stAdecConfig.stAdecG711Cfg.eSamplerate = (MI_AUDIO_SampleRate_e)pstWavHeader->sample_rate;
                stAdecConfig.stAdecG711Cfg.eSoundmode = pstWavHeader->channels == 1 ? E_MI_AUDIO_SOUND_MODE_MONO : E_MI_AUDIO_SOUND_MODE_STEREO;
            }
            else if (pstWavHeader->format_type == WAV_G726)
            {
                stAdecConfig.eAdecType = E_MI_AUDIO_ADEC_TYPE_G726;
                stAdecConfig.stAdecG726Cfg.eSamplerate = (MI_AUDIO_SampleRate_e)pstWavHeader->sample_rate;
                stAdecConfig.stAdecG726Cfg.eSoundmode = pstWavHeader->channels == 1 ? E_MI_AUDIO_SOUND_MODE_MONO : E_MI_AUDIO_SOUND_MODE_STEREO;
                switch(pstWavHeader->bits_per_sample)
                {
                    case 2:  stAdecConfig.stAdecG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_16; break;
                    case 3:  stAdecConfig.stAdecG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_24; break;
                    case 4:  stAdecConfig.stAdecG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_32; break;
                    case 5:  stAdecConfig.stAdecG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_40; break;
                    default: stAdecConfig.stAdecG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_16; break;
                }
            }

            if(g_u32AudioOnInit == 0)
            {
                ExecFunc(MI_AO_SetAdecAttr(AoDevId[audioIdx], AoChnId, &stAdecConfig), MI_SUCCESS);
                g_u32AudioOnInit = 1;
            }
            ExecFunc(MI_AO_EnableAdec(AoDevId[audioIdx], AoChnId), MI_SUCCESS);
        }

        /* if test AO Volume */
        ExecFunc(MI_AO_SetVolume(AoDevId[audioIdx], pstAudioOutParam->s32VolumeOutDb), MI_SUCCESS);

#if TARGET_CHIP_I5
    MI_SYS_ChnPort_t stAoChn0OutputPort0;

        // test get output port buffer
        stAoChn0OutputPort0.eModId    = E_MI_MODULE_ID_AO;
        stAoChn0OutputPort0.u32DevId  = AoDevId[audioIdx];
        stAoChn0OutputPort0.u32ChnId  = AoChnId;
        stAoChn0OutputPort0.u32PortId = 0;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0,
                                              pstAudioOutParam->u8BufQueueDepth,
                                              pstAudioOutParam->u8BufQueueDepth), MI_SUCCESS);
#endif
    }

    MIXER_DBG("mid_AdecInit-\n");

    return MI_SUCCESS;
}

static MI_S32 mid_AdecDeinit()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 audioIdx = 0;
    MI_AO_CHN AoChnId;
    MI_AUDIO_DEV AoDevId = 0x0;


    for (audioIdx = 0; audioIdx < (MI_S32)g_stAudioOutParam[0].s32AudioOutNum; audioIdx++)
    {
        AoDevId = g_stAudioOutParam[audioIdx].stAudioOutChnPort.u32DevId;
        AoChnId = g_stAudioOutParam[audioIdx].stAudioOutChnPort.u32ChnId;
        printf("%s:%d Stop AudioPlay(AoDevID=%d, AoChnId=%d)\n", __func__,__LINE__,AoDevId, AoChnId);

        /* Disable Resample */
        if(g_stAudioOutParam[audioIdx].u8AudioOutRes)
        {
            ExecFunc(MI_AO_DisableReSmp(AoDevId, AoChnId), MI_SUCCESS);
        }

        /* Disable VQE */
        if(g_stAudioOutParam[audioIdx].u8AudioOutVqe)
        {
            ExecFunc(MI_AO_DisableVqe(AoDevId, AoChnId), MI_SUCCESS);
        }

        /* disable ao channel of */
        ExecFunc(MI_AO_DisableChn(AoDevId, AoChnId), MI_SUCCESS);
    }

    /* disable ao device */
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);

    return s32Ret;
}


void adec_sendBufferCB(void *pParam)
{
    MI_S32 nRet = MI_SUCCESS;
    User_t *pUser = (User_t *)pParam;
    MI_S32 nRead = 0;

    if((pUser->total_size - pUser->pos) > 0)
    {
        nRead = fread(pUser->pAudioStream->stream, 1, SEND_PACK_LEN, pUser->fp);

        if(ferror(pUser->fp))
        {
            goto fail;
        }

        pUser->pos += nRead;
        pUser->pAudioStream->len = nRead;
        //nRet = MI_ADEC_SendStream(0, pUser->pAudioStream, 0, &tAdecCB);

        if(nRet == E_MI_ERR_FAILED)
        {
            MIXER_ERR("MI_ADEC_SendStream FAIL");
            goto fail;
        }
    }
    else
    {
    fail:
        FREEIF(pUser->pAudioStream->stream);
        FREEIF(pUser->pAudioStream);
        fclose(pUser->fp);
        FREEIF(pUser);
    }
    return ;
}


void *adec_thread_sendStreamAsync(void* argv)
{
    FILE* fp = NULL;
    MI_S32 audioIdx = 0;
    MI_AUDIO_DEV AoDevId = -1;
    MI_AO_CHN AoChnId = -1;
    MI_U32 s32ReadLen = 0;
    unsigned int WavHeaderLen = 0;
    MI_U8 *pu8AoutBuf = NULL;
    MI_U32 u32AoutBufSize = MIXER_AO_ADEC_G726_UNIT_MAX * 2 * 2;
    MI_AUDIO_Frame_t stAudioFrame;
    MI_S32 s32RetSendStatus = MI_SUCCESS;
    WavHeader_t stWavHeaderInput;
    g726_state_t stG726State;
    g726_state_t *pstG726State = NULL;


    audioIdx = (MI_S32)argv;
    AoDevId = g_stAudioOutParam[audioIdx].stAudioOutChnPort.u32DevId;
    AoChnId = g_stAudioOutParam[audioIdx].stAudioOutChnPort.u32ChnId;
    printf("%s:%d AoDevID=%d, AoChnId=%d\n", __func__, __LINE__, AoDevId, AoChnId);

    if(NULL == (fp = fopen((char *)g_stAudioOutParam[audioIdx].s8AudioPath, "r")))
    {
        printf("Open input file path:%s fail \n", g_stAudioOutParam[audioIdx].s8AudioPath);
        goto adec_thread_sendStreamAsync_Exit;
    }

    pu8AoutBuf = (MI_U8 *)malloc(u32AoutBufSize);
    if(NULL == pu8AoutBuf)
    {
        printf("%s:%d malloc buf for pu8AoutBuf fail \n", __func__, __LINE__);
        goto adec_thread_sendStreamAsync_Exit;
    }

    memset(pu8AoutBuf, 0x00, u32AoutBufSize);
    sprintf((char *)pu8AoutBuf, "Aout_Task%d", AoChnId);
    prctl(PR_SET_NAME, pu8AoutBuf);
    memset(pu8AoutBuf, 0x00, u32AoutBufSize);


    // skip wave header
    WavHeaderLen = fread((char *)&stWavHeaderInput, 1, sizeof(WavHeader_t), fp);

    memset(&stAudioFrame, 0x00, sizeof(MI_AUDIO_Frame_t));
    //read data and send to AO module
    stAudioFrame.eBitwidth  = (24 == g_stAudioOutParam[audioIdx].stWavHeader.bits_per_sample) ? \
                                     E_MI_AUDIO_BIT_WIDTH_24 : E_MI_AUDIO_BIT_WIDTH_16;
    stAudioFrame.eSoundmode = (2 == g_stAudioOutParam[audioIdx].stWavHeader.channels) ? \
                                    E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;

    stAudioFrame.apVirAddr[1] = NULL;

    if(g_audioOutEnableG7xxExt)
    {
        if (g_stAudioOutParam[audioIdx].stWavHeader.format_type == 0x45)
        {
            stAudioFrame.apVirAddr[0] = (void *)malloc(u32AoutBufSize * 8);
            if(NULL == stAudioFrame.apVirAddr[0])
            {
                printf("%s:%d malloc buf for AudioOut fail \n", __FUNCTION__, __LINE__);
                goto adec_thread_sendStreamAsync_Exit;
            }

            memset(&stG726State, 0x00, sizeof(g726_state_t));

            pstG726State = g726_init(&stG726State, g_stAudioOutParam[audioIdx].stWavHeader.byterate);
            if(NULL == pstG726State)
            {
                printf("%s:%d call g726_init() fail \n",  __FUNCTION__, __LINE__);
                goto adec_thread_sendStreamAsync_Exit;
            }
        }
        else
        {
            stAudioFrame.apVirAddr[0] = (void * )malloc(u32AoutBufSize * 2);
            if(NULL == stAudioFrame.apVirAddr[0])
            {
                printf("%s:%d malloc buf for AudioOut fail \n",  __FUNCTION__, __LINE__);
                goto adec_thread_sendStreamAsync_Exit;
            }
        }
    }


    while(g_audioOut_exit == FALSE)
    {
        s32ReadLen = fread(pu8AoutBuf, 1, u32AoutBufSize, fp);
        if (s32ReadLen == 0)
        {
            fseek(fp, WavHeaderLen, SEEK_SET);
            usleep(1000 * 1000 * 3);
            continue;
        }

        if(s32ReadLen != u32AoutBufSize)
        {
            //printf("error memset set.\n");
            memset(pu8AoutBuf + s32ReadLen, 0x00, u32AoutBufSize - s32ReadLen);
        }

        //printf("s32ReadLen[%d].\n", s32ReadLen);
        do {
            if(g_audioOutEnableG7xxExt)
            {
                stAudioFrame.u32Len = s32ReadLen * 2;

                switch(g_stAudioOutParam[audioIdx].stWavHeader.format_type)
                {
                    case 0x06: G711Decoder((short *)stAudioFrame.apVirAddr[0], pu8AoutBuf, s32ReadLen, 0); break;
                    case 0x07: G711Decoder((short *)stAudioFrame.apVirAddr[0], pu8AoutBuf, s32ReadLen, 1); break;
                    case 0x45: { stAudioFrame.u32Len = g726_decode(pstG726State, (short *)stAudioFrame.apVirAddr[0], pu8AoutBuf, s32ReadLen); } break;
                    case 0x01: //PCM data
                    default:  { memcpy(stAudioFrame.apVirAddr[0], pu8AoutBuf, s32ReadLen); stAudioFrame.u32Len = s32ReadLen; } break;
                }
            }
            else
            {
                stAudioFrame.u32Len = s32ReadLen;
                stAudioFrame.apVirAddr[0] = pu8AoutBuf;
            }

            s32RetSendStatus = MI_AO_SendFrame(AoDevId, AoChnId, &stAudioFrame, -1);

            //u32SleepTime = (g_stAudioOutParam[audioIdx].stAudioAttr.u32PtNumPerFrm * 1000) / g_stAudioOutParam[audioIdx].stAudioAttr.eSamplerate;
            //u32SleepTime = (u32SleepTime > 100) ? ((u32SleepTime / 100) * 100000) :
            //               ((u32SleepTime > 20) ? ((u32SleepTime / 20) * 10000) : u32SleepTime * 1000);
            usleep(1000/*u32SleepTime*/);
            //printf("u32SleepTime[%d].\n", u32SleepTime);
            //printf("send buffer s32RetSendStatus=0x%x.\n", s32RetSendStatus);
        } while((s32RetSendStatus == MI_AO_ERR_NOBUF) && (g_audioOut_exit == FALSE));

        if(MI_SUCCESS != s32RetSendStatus)
        {
            printf("[Warning]: MI_AO_SendFrame fail, error code is 0x%x: \n", s32RetSendStatus);
            sleep(1);
        }
    }

    if(MI_SUCCESS == s32RetSendStatus)
    {
        printf("[Warning]: MI_AO_SendFrame done!\n");
    }

adec_thread_sendStreamAsync_Exit:
    if(NULL != fp)
    {
        fclose(fp);
    }

    while(g_audioOut_exit == FALSE)
    {
        usleep(1000 * 20);
    }

    if(g_audioOutEnableG7xxExt)
    {
        if(NULL != stAudioFrame.apVirAddr[0])
        {
            free(stAudioFrame.apVirAddr[0]);
            stAudioFrame.apVirAddr[0] = NULL;
        }
    }

    if(NULL != pu8AoutBuf)
    {
        free(pu8AoutBuf);
        pu8AoutBuf = NULL;
    }

    mid_AdecDeinit();
    g_audioOut_exit = FALSE;

    pthread_exit(NULL);
    return NULL;
}

MI_S32 SetAIVolume(MI_S32 audioIdx, MI_S32 volume)
{

    if((0 <= volume) && (volume <= 21))
    {
        ExecFunc(MI_AI_SetVqeVolume(MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_AiChnPort.u32DevId,
                                    MI_AudioEncoder::g_pAudioEncoderArray[audioIdx]->m_AiChnPort.u32ChnId,
                                    volume), MI_SUCCESS);
    }
    else
    {
        printf("%s:%d Set AudioIn-%d VolumeInDb in [0 21], you set value=%d is out of range\n", __func__, __LINE__, audioIdx, volume);
        return -1;
    }
    return MI_SUCCESS;
}

MI_S32 SetAOVolume(MI_S32 s32AudioInIdx,MI_S32 volume)
{
#if TARGET_CHIP_I5
        if((-60 <= volume) && (volume <= 10))
        {
            ExecFunc(MI_AO_SetVolume(g_stAudioOutParam[s32AudioInIdx].stAudioOutChnPort.u32DevId, volume), MI_SUCCESS);
        }
        else
        {
            printf("%s:%d VolumeOutDb in [-60 10], you set value=%d is out of range!\n", __func__, __LINE__, volume);
            return -1;
        }
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        if((-60 <= volume) && (volume <= 30))
        {
            ExecFunc(MI_AO_SetVolume(g_stAudioOutParam[s32AudioInIdx].stAudioOutChnPort.u32DevId, volume), MI_SUCCESS);
        }
        else
        {
            printf("%s:%d VolumeOutDb in [-60 30], you set value=%d is out of range!\n", __func__, __LINE__, volume);
            return -1;
        }
#endif

    return MI_SUCCESS;

}

MI_S32 SetVQEStatus(MI_S8 setmodule, MI_S8 * ps8AudioVqeData,MI_U32 bufLen)
{
    MI_S32 audioIdx = 0;
    MI_U32 len = 0;
    // set Audio In VQE
    if(1 == setmodule)
    {
        MI_AI_CHN AiChnId;
        MI_AUDIO_DEV AiDevId[] = {MI_AUDIO_DEV_ALL};
        MI_AI_VqeConfig_t stVqeConfigAi = {0};
        MI_AI_VqeConfig_t stGetVqeConfig;
        MI_AudioEncoder* pstAudioEncoder = NULL;

        pstAudioEncoder = MI_AudioEncoder::g_pAudioEncoderArray[audioIdx];

        AiDevId[audioIdx] = pstAudioEncoder->m_AiChnPort.u32DevId;
        if (AiDevId[audioIdx] == -1)
        {
            return -1;
        }

        AiDevId[audioIdx] = pstAudioEncoder->m_AiChnPort.u32DevId;
        AiChnId = pstAudioEncoder->m_AiChnPort.u32ChnId;
        printf("SetVQEStatus:%d\n", setmodule);

           len = sizeof(MI_AI_VqeConfig_t) > (bufLen) ? (bufLen) : sizeof(MI_AI_VqeConfig_t);
        memcpy(&stVqeConfigAi, (MI_S8*)ps8AudioVqeData, len);

        if(pstAudioEncoder->m_bAudioInVqe)
        {
            if(stVqeConfigAi.bHpfOpen == TRUE)
                pstAudioEncoder->m_stAiVqeCfg.bHpfOpen = TRUE;
            else
                pstAudioEncoder->m_stAiVqeCfg.bHpfOpen = FALSE;

            if(stVqeConfigAi.bAnrOpen == TRUE)
                pstAudioEncoder->m_stAiVqeCfg.bAnrOpen = TRUE;
            else
                pstAudioEncoder->m_stAiVqeCfg.bAnrOpen = FALSE;

            if(stVqeConfigAi.bAgcOpen == TRUE)
                pstAudioEncoder->m_stAiVqeCfg.bAgcOpen = TRUE;
            else
                pstAudioEncoder->m_stAiVqeCfg.bAgcOpen = FALSE;

            if(stVqeConfigAi.bEqOpen == TRUE)
                pstAudioEncoder->m_stAiVqeCfg.bEqOpen  = TRUE;
            else
                pstAudioEncoder->m_stAiVqeCfg.bEqOpen  = FALSE;

            if(stVqeConfigAi.bAecOpen == TRUE)
                pstAudioEncoder->m_stAiVqeCfg.bAecOpen = TRUE;
            else
                pstAudioEncoder->m_stAiVqeCfg.bAecOpen = FALSE;

            ExecFunc(MI_AI_DisableVqe(AiDevId[audioIdx], AiChnId), MI_SUCCESS);
            ExecFunc(MI_AI_SetVqeAttr(AiDevId[audioIdx], AiChnId, 0, 0, &pstAudioEncoder->m_stAiVqeCfg), MI_SUCCESS);
            ExecFunc(MI_AI_GetVqeAttr(AiDevId[audioIdx], AiChnId, &stGetVqeConfig), MI_SUCCESS);
            ExecFunc(MI_AI_EnableVqe(AiDevId[audioIdx], AiChnId), MI_SUCCESS);
        }
    }
    else if(setmodule == 0)  // set Audio Out VQE
    {
        MI_AO_CHN AoChnId = 0;
        MI_AO_VqeConfig_t stVqeConfigAo = {0};
        MI_AUDIO_DEV AoDevId[] = {MI_AUDIO_DEV_ALL};
        AoDevId[audioIdx] = 0;
        if (AoDevId[audioIdx] == -1)
        {
            return -1;
        }

        len =  sizeof(MI_AO_VqeConfig_t) > (bufLen) ? (bufLen):sizeof(MI_AO_VqeConfig_t);
        memcpy(&stVqeConfigAo, ps8AudioVqeData, len);

         if(g_stAudioOutParam[audioIdx].u8AudioOutVqe)
        {
            if(stVqeConfigAo.bHpfOpen == TRUE )
                g_stAoVqeCfg.bHpfOpen = TRUE;
            else
                g_stAoVqeCfg.bHpfOpen = FALSE;

            if(stVqeConfigAo.bAnrOpen == TRUE)
                g_stAoVqeCfg.bAnrOpen = TRUE;
            else
                g_stAoVqeCfg.bAnrOpen = FALSE;

            if(stVqeConfigAo.bAgcOpen == TRUE)
                g_stAoVqeCfg.bAgcOpen = TRUE;
            else
                g_stAoVqeCfg.bAgcOpen = FALSE;

            if(stVqeConfigAo.bEqOpen == TRUE)
                g_stAoVqeCfg.bEqOpen  = TRUE;
            else
                g_stAoVqeCfg.bEqOpen  = FALSE;

            ExecFunc(MI_AO_DisableVqe(AoDevId[audioIdx], AoChnId), MI_SUCCESS);
            ExecFunc(MI_AO_SetVqeAttr(AoDevId[audioIdx], AoChnId, &g_stAoVqeCfg), MI_SUCCESS);
            ExecFunc(MI_AO_EnableVqe(AoDevId[audioIdx], AoChnId), MI_SUCCESS);
        }
    }
    return MI_SUCCESS;
}
void startPlayMedia(MixerAudioOutParam * pstAudioOutParam)
{
    int fdRd = -1;
    MI_S32 s32RdSize;
    MI_S32 audioIdx = 0;
    WavHeader_t stWavHeaderInput;

    printf("pstAudioOutParam->s8AudioPath===%s===\n",pstAudioOutParam->s8AudioPath);
    if(pstAudioOutParam->s8AudioPath[0])
    {
        fdRd = open((const char *)pstAudioOutParam->s8AudioPath, O_RDONLY, 0666);
        if(fdRd < 0)
        {
            printf("Open input file path:%s fail \n", pstAudioOutParam->s8AudioPath);
            return ;
        }
    }
    else
    {
        printf("The audio file name is NULL and return!\n");
        return ;
    }

    // read input wav file
    s32RdSize = read(fdRd, &stWavHeaderInput, sizeof(WavHeader_t));
    if(s32RdSize != sizeof(WavHeader_t))
    {
        printf("Read input file %s size %d fail1\n", pstAudioOutParam->s8AudioPath,  s32RdSize);
        close(fdRd);
        return ;
    }

    close(fdRd);

    memcpy(&pstAudioOutParam->stWavHeader, &stWavHeaderInput, sizeof(WavHeader_t));
    printf("\n%s:%d Ready to play music: %s\n", __func__, __LINE__, pstAudioOutParam->s8AudioPath);
    printf("WAV channel          %d\n", pstAudioOutParam->stWavHeader.channels);
    printf("WAV byterate         %d\n", pstAudioOutParam->stWavHeader.byterate);
    printf("WAV samplerate       %d\n", pstAudioOutParam->stWavHeader.sample_rate);
    printf("WAV bits per sample  %d\n", pstAudioOutParam->stWavHeader.bits_per_sample);

    switch(pstAudioOutParam->stWavHeader.format_type)
    {
        // PCM
        case 0x01: printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_PCM"); break;
        // G711A
        case 0x06: printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_G711A"); break;
        // G711U
        case 0x07: printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_G711U"); break;
        //case MT_G711:  printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_G711"); break;
        //case MT_AMR:   printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_AMR"); break;
        //case MT_AAC:   printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_AAC"); break;
        // G726
        case 0x45: printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_G726"); break;
        //case MT_MP3:   printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_MP3"); break;
        //case MT_OGG:   printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_OGG"); break;
        //case MT_ADPCM: printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_ADPCM"); break;
        //case MT_OPUS:  printf("WAV format type      %d (%s)\n\n", pstAudioOutParam->stWavHeader.format_type, "MT_OPUS"); break;
        default:  printf("WAV format type      %d (Set wrong param)\n\n", pstAudioOutParam->stWavHeader.format_type); return;
    }


    pstAudioOutParam->stAudioAttr.eSamplerate = (MI_AUDIO_SampleRate_e)pstAudioOutParam->stWavHeader.sample_rate;
    if(pstAudioOutParam->u8AudioOutRes)
    {
        pstAudioOutParam->stAudioAttr.eSamplerate = (MI_AUDIO_SampleRate_e)pstAudioOutParam->eAudioOutReSampleRate;
    }
    pstAudioOutParam->stAudioAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16; //only support bit_width_16
    pstAudioOutParam->stAudioAttr.u32ChnCnt = pstAudioOutParam->stWavHeader.channels;
    if(pstAudioOutParam->stAudioAttr.u32ChnCnt == 2)
    {
        pstAudioOutParam->stAudioAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    }
    else if(pstAudioOutParam->stAudioAttr.u32ChnCnt == 1)
    {
        pstAudioOutParam->stAudioAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    }

    //memset(m_s8FilePath, 0x00, sizeof(m_s8FilePath));
    //snprintf(m_s8FilePath, sizeof(m_s8FilePath) - 1, "%s", audioFileName);

    int ret = 0;

    ret = mid_AdecInit(pstAudioOutParam, &pstAudioOutParam->stWavHeader);
    if(ret != MI_SUCCESS)
    {
        MIXER_WARN("mid_AdecInit failed! Maybe music is already play!\n");
        return ;
    }

    // create a thread to send stream to ao
    g_audioOut_exit = FALSE;
    pthread_create(&g_pthreadId_Aout, NULL, adec_thread_sendStreamAsync, (void *)((MI_S32)audioIdx));
}

void StopPlayMedia()
{
    g_audioOut_exit = TRUE;

    usleep(1000*10);
}

void MI_AudioEncoder::initAudioEncoder(MixerAudioInParam * pstAudioParam, MI_S32 audioIdx)
{
    //MI_AUDIO_Attr_t stGetAudioAttr;
    MI_AI_VqeConfig_t stGetVqeConfig;
    MI_U32 AudioInDevId;
    MI_U32 AudioInChnId;
    MI_U32 AudioInPortId;
    memset(&m_stAiDevAttr,0x00,sizeof(MI_AUDIO_Attr_t));
    m_AiMediaType = MT_NUM;
    m_s32VolumeInDb = 0x00;
    m_eAudioInReSampleRate = E_MI_AUDIO_SAMPLE_RATE_INVALID;
    m_bAudioInVqe = 0x00;
    m_u32BufQueueDepth = 0x00;
    m_AencChn = 0x00;
    m_pthreadId_Aout = (pthread_t)(-1);
    m_pthreadId_SPL = (pthread_t)(-1);
    m_pthreadId = (pthread_t)(-1);
    memset(&m_stAencCfg,0x00,sizeof(MI_AI_AencConfig_t));
    memset(&m_stAiVqeCfg,0x00,sizeof(MI_AI_VqeConfig_t));
    if(NULL == pstAudioParam)
    {
        printf("%s input mixer param is NULL!\n", __func__);
        return;
    }


    if(g_s32AudioInNum && (g_s32AudioInNum > (MI_U32)audioIdx))
    {
        m_AencChn = audioIdx;
        m_s32VolumeInDb  = pstAudioParam->s32VolumeInDb;

        memset(&m_stAiDevAttr, 0x00, sizeof(MI_AUDIO_Attr_t));
        memcpy(&m_stAiDevAttr, &pstAudioParam->stAudioAttr, sizeof(MI_AUDIO_Attr_t));

        memset(&m_AiChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
        memcpy(&m_AiChnPort, &pstAudioParam->stAudioInChnPort, sizeof(MI_SYS_ChnPort_t));

        m_AiMediaType = pstAudioParam->AiMediaType;
        m_bAudioInVqe = pstAudioParam->bAudioInVqe;
        m_u32BufQueueDepth  = pstAudioParam->u8BufQueueDepth;
        m_u32UserFrameDepth = pstAudioParam->u8UserFrameDepth;
        m_eAudioInReSampleRate  = pstAudioParam->eAudioInReSampleRate;

        AudioInDevId  = m_AiChnPort.u32DevId;
        AudioInChnId  = m_AiChnPort.u32ChnId;
        AudioInPortId = m_AiChnPort.u32PortId;

        if(m_bAudioInVqe)
        {
            memset(&m_stAiVqeCfg, 0x00, sizeof(MI_AI_VqeConfig_t));
            memcpy(&m_stAiVqeCfg, &g_stAiVqeCfg, sizeof(MI_AI_VqeConfig_t));
        }

        printf("start AudioEncoder: DevId=%d ChnID=%d PortID=%d\n", AudioInDevId, AudioInChnId, AudioInPortId);

        printf("Ai Media channel     %d\n", m_stAiDevAttr.u32ChnCnt);
        printf("Ai Media samplerate  %d\n", m_stAiDevAttr.eSamplerate);
        printf("Ai Media VolumeInDb  %d\n", m_s32VolumeInDb);

        switch(m_stAiDevAttr.eBitwidth)
        {
            case E_MI_AUDIO_BIT_WIDTH_16: printf("Ai Media BitWidth    %d(16bit)\n", m_stAiDevAttr.eBitwidth); break;
            case E_MI_AUDIO_BIT_WIDTH_24: printf("Ai Media BitWidth    %d(24bit)\n", m_stAiDevAttr.eBitwidth); break;
            default: printf("Ai Media BitWidth    %d(set error, and reset it to 16bit)\n", m_stAiDevAttr.eBitwidth); break;
        }

        switch(m_stAiDevAttr.eSoundmode)
        {
            case E_MI_AUDIO_SOUND_MODE_MONO:   printf("Ai Media Soundmode   %d (MONO)\n",  m_stAiDevAttr.eSoundmode); break;
            case E_MI_AUDIO_SOUND_MODE_STEREO: printf("Ai Media Soundmode   %d (STERO)\n", m_stAiDevAttr.eSoundmode); break;
            default: printf("Ai Media Soundmode   %d (Set wrong Soundmode)\n", m_stAiDevAttr.eSoundmode); break;
        }

        switch(m_AiMediaType)
        {
            case MT_PCM:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_PCM");
            break;

            case MT_G711A:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n",\
                m_AiMediaType,\
                "MT_G711A"); \
            break;

            case MT_G711U:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G711U"); \
            break;

            case MT_G711:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G711"); \
            break;

            case MT_AMR:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_AMR"); \
            break;

            case MT_AAC:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_AAC"); \
            break;

            case MT_G726:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G726"); \
            break;

            case MT_MP3:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType,\
                "MT_MP3");\
            break;

            case MT_OGG:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_OGG"); \
            break;

            case MT_ADPCM:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_ADPCM"); \
            break;

            case MT_OPUS:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_OPUS"); \
            break;

            case MT_G726_16:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G726_16"); \
            break;

            case MT_G726_24:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G726_24"); \
            break;

            case MT_G726_32:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G726_32"); \
            break;

            case MT_G726_40:
            MIXER_DBG("Ai MediaType         %d (%s)\n\n", \
                m_AiMediaType, \
                "MT_G726_40"); \
            break;

            default:
            MIXER_DBG("Ai MediaType         %d (Set wrong AiMediaType)\n\n", \
                m_AiMediaType);\
            break;
        }


        if(0 == g_u32AudioInInit)
        {
            /* set ai public attr*/
            ExecFuncReturnNothing(MI_AI_SetPubAttr(AudioInDevId, &m_stAiDevAttr), MI_SUCCESS);

            /* enable ai device */
            ExecFuncReturnNothing(MI_AI_Enable(AudioInDevId), MI_SUCCESS);

            g_u32AudioInInit = 1;
        }
/*
        // get ai device
        ExecFuncReturnNothing(MI_AI_GetPubAttr(AudioInDevId, &stGetAudioAttr), MI_SUCCESS);
*/
        if(0 <= m_s32VolumeInDb || m_s32VolumeInDb <= 21)
        {
            ExecFuncReturnNothing(MI_AI_SetVqeVolume(AudioInDevId, AudioInChnId, m_s32VolumeInDb), MI_SUCCESS);
        }

        ExecFuncReturnNothing(MI_SYS_SetChnOutputPortDepth(&m_AiChnPort, m_u32UserFrameDepth, m_u32BufQueueDepth), MI_SUCCESS);

        /* enable ai channel of device*/
        ExecFuncReturnNothing(MI_AI_EnableChn(AudioInDevId, AudioInChnId), MI_SUCCESS);

        if(E_MI_AUDIO_SAMPLE_RATE_INVALID != m_eAudioInReSampleRate)
        {
            ExecFuncReturnNothing(MI_AI_EnableReSmp(AudioInDevId, AudioInChnId, m_eAudioInReSampleRate), MI_SUCCESS);
        }
/*
        if((MT_NUM != m_AiMediaType) && (MT_G711A <= m_AiMediaType)) //(0 == g_audioInEnableG7xxExt)
        {
            //set ai Aenc
            memset(&m_stAencCfg, 0x00, sizeof(MI_AI_AencConfig_t));
            switch(m_AiMediaType)
            {
                case MT_G711A:
                    m_stAencCfg.eAencType = E_MI_AUDIO_AENC_TYPE_G711A;
                    m_stAencCfg.stAencG711Cfg.eSamplerate = m_stAiDevAttr.eSamplerate;
                    m_stAencCfg.stAencG711Cfg.eSoundmode  = m_stAiDevAttr.eSoundmode;
                    break;

                case MT_G711U:
                    m_stAencCfg.eAencType = E_MI_AUDIO_AENC_TYPE_G711U;
                    m_stAencCfg.stAencG711Cfg.eSamplerate = m_stAiDevAttr.eSamplerate;
                    m_stAencCfg.stAencG711Cfg.eSoundmode  = m_stAiDevAttr.eSoundmode;
                    break;

                case MT_G726:
                    m_stAencCfg.eAencType = E_MI_AUDIO_AENC_TYPE_G726;
                    m_stAencCfg.stAencG726Cfg.eSamplerate = m_stAiDevAttr.eSamplerate;
                    m_stAencCfg.stAencG726Cfg.eSoundmode = m_stAiDevAttr.eSoundmode;
                    m_stAencCfg.stAencG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_32;
                    switch(m_stAiDevAttr.eBitwidth)
                    {
                           default:
                        case E_MI_AUDIO_BIT_WIDTH_16:  m_stAencCfg.stAencG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_16; break;
                        case E_MI_AUDIO_BIT_WIDTH_24:  m_stAencCfg.stAencG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_24; break;
            #if TARGET_CHIP_I5
                        case 4:  m_stAencCfg.stAencG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_32; break;
                        case 5:  m_stAencCfg.stAencG726Cfg.eG726Mode = E_MI_AUDIO_G726_MODE_40; break;
            #endif
                    }
                    break;

                default:
                    MIXER_ERR("Set the wrong AImediaType(%d)\n", pstAudioParam->AiMediaType);
                    return;
            }

            ExecFuncReturnNothing(MI_AI_SetAencAttr(AudioInDevId, AudioInChnId, &m_stAencCfg), MI_SUCCESS);
            ExecFuncReturnNothing(MI_AI_EnableAenc(AudioInDevId, AudioInChnId), MI_SUCCESS);
        }
*/
        /* if test VQe: set attribute of AO VQE  */
        if(m_bAudioInVqe)
        {
            // need to check algorithm configure
            m_stAiVqeCfg.s32WorkSampleRate = m_stAiDevAttr.eSamplerate;
            ExecFuncReturnNothing(MI_AI_SetVqeAttr(AudioInDevId, AudioInChnId, 0, 0, &m_stAiVqeCfg), MI_SUCCESS);
            ExecFuncReturnNothing(MI_AI_GetVqeAttr(AudioInDevId, AudioInChnId, &stGetVqeConfig), MI_SUCCESS);
            ExecFuncReturnNothing(MI_AI_EnableVqe(AudioInDevId, AudioInChnId), MI_SUCCESS);
        }

        if(m_stAiDevAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)
        {
            MI_AudioEncoder::fNumChannels = 2;
        }
        else
        {
            MI_AudioEncoder::fNumChannels = 1;
        }

        if(E_MI_AUDIO_SAMPLE_RATE_INVALID != m_eAudioInReSampleRate)
        {
            MI_AudioEncoder::fSampleFrequency = m_eAudioInReSampleRate;
        }
        else
        {
            MI_AudioEncoder::fSampleFrequency = m_stAiDevAttr.eSamplerate;
        }
        switch(m_stAiDevAttr.eBitwidth)
        {
            case E_MI_AUDIO_BIT_WIDTH_16: MI_AudioEncoder::fBitsPerSample = 16; break;
            case E_MI_AUDIO_BIT_WIDTH_24: MI_AudioEncoder::fBitsPerSample = 24; break;
            default:
                MI_AudioEncoder::fBitsPerSample = 16;
                printf("Set Ai Bitwidth=%d(Not Support), and reset it to 16!\n", m_stAiDevAttr.eBitwidth);
                break;
        }
    }
#if MIXER_AED_ENABLE
    if(g_bAudioAED) AedInit();
#endif

#if MIXER_LSD_ENABLE
    if(g_bAudioLSD) LsdInit();
#endif
}

void MI_AudioEncoder::uninitAudioEncoder()
{
    MI_S32 s32AudioNum = 0;

    s32AudioNum = MI_AudioEncoder::g_audioEncoderNumber;
    if(s32AudioNum > MIXER_AI_MAX_NUMBER || s32AudioNum < 0)
    {
        s32AudioNum = 0;
        return;
    }

    if(g_s32AudioInNum)
    {
        MIXER_DBG("\nStop AI device Channel Number: %d \n", s32AudioNum);

        // wait for getting frame data threshold return
        for(MI_S32 u32AencIdx = 0; u32AencIdx < s32AudioNum; u32AencIdx++)
        {
            MI_U32 AiPortId = g_pAudioEncoderArray[u32AencIdx]->m_AiChnPort.u32PortId;
            MI_AI_CHN AiChnId = g_pAudioEncoderArray[u32AencIdx]->m_AiChnPort.u32ChnId;
            MI_AUDIO_DEV AiDevId = g_pAudioEncoderArray[u32AencIdx]->m_AiChnPort.u32DevId;


            MIXER_DBG("stopAudioEncoder: DevId=%d ChnID=%d PortID=%d\n", AiDevId, AiChnId, AiPortId);

            MIXER_DBG("Ai MediaType SampleRate   %d\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eSamplerate);

            switch(g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eSoundmode)
            {
                case E_MI_AUDIO_SOUND_MODE_MONO:
                    MIXER_DBG("Ai MediaType SoundMode    %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eSoundmode, "MONO");
                    break;
                case E_MI_AUDIO_SOUND_MODE_STEREO:
                    MIXER_DBG("Ai MediaType SoundMode    %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eSoundmode, "STERO");
                    break;
                default:
                    MIXER_DBG("Ai MediaType SoundMode    %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eSoundmode, "Not Support");
                    break;
            }

            switch(g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eBitwidth)
            {
                case E_MI_AUDIO_BIT_WIDTH_16:
                    MIXER_DBG("Ai MediaType BitWidth     %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eBitwidth, "16bit");
                    break;
                case E_MI_AUDIO_BIT_WIDTH_24:
                    MIXER_DBG("Ai MediaType BitWidth     %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eBitwidth, "24bit");
                    break;
                default:
                    MIXER_DBG("Ai MediaType BitWidth     %d (%s)\n", g_pAudioEncoderArray[u32AencIdx]->m_stAiDevAttr.eBitwidth, "Not Support");
                    break;
            }

            switch(g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType)
            {
                case MT_PCM:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_PCM");\
            break;

                case MT_G711A:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n",\
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G711A"); \
            break;

                case MT_G711U:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G711U"); \
            break;

                case MT_G711:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G711"); \
            break;

                case MT_AMR:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_AMR"); \
            break;

                case MT_AAC:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_AAC"); \
            break;

                case MT_G726:    \
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G726"); \
            break;

                case MT_MP3:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_MP3"); \
            break;

                case MT_OGG:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_OGG"); \
            break;

                case MT_ADPCM:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_ADPCM");\
            break;

                case MT_OPUS:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_OPUS"); \
            break;

                case MT_G726_16:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G726_16"); \
            break;

                case MT_G726_24: \
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G726_24"); \
            break;

                case MT_G726_32:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G726_32"); \
            break;

                case MT_G726_40:
            MIXER_DBG("Ai MediaType              %d (%s)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType, \
                "MT_G726_40"); \
            break;

                default:
            MIXER_DBG("Ai MediaType              %d (Set wrong param)\n\n", \
                g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType);\
            break;
            }

            /* Disable Resample */
            if(E_MI_AUDIO_SAMPLE_RATE_INVALID != g_pAudioEncoderArray[u32AencIdx]->m_eAudioInReSampleRate)
            {
               ExecFuncReturnNothing(MI_AI_DisableReSmp(AiDevId, AiChnId), MI_SUCCESS);
            }

            /* Disable VQE */
            if(g_pAudioEncoderArray[u32AencIdx]->m_bAudioInVqe)
            {
                ExecFuncReturnNothing(MI_AI_DisableVqe(AiDevId, AiChnId), MI_SUCCESS);
            }
/*
        if((MT_NUM != g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType) && (MT_G711A <= g_pAudioEncoderArray[u32AencIdx]->m_AiMediaType))
        {
            ExecFuncReturnNothing(MI_AI_DisableAenc(AiDevId, AiChnId), MI_SUCCESS);
        }
*/

        /* disable ai channel of device */
        ExecFuncReturnNothing(MI_AI_DisableChn(AiDevId, AiChnId), MI_SUCCESS);

        }

        /* disable ai device */
    ExecFuncReturnNothing(MI_AI_Disable(g_pAudioEncoderArray[0]->m_AiChnPort.u32DevId), MI_SUCCESS);
    }

    g_audioEncoderNumber = 0;
    g_u32AudioInInit = 0;

    MIXER_DBG("uninitAudioEncoder uninitAudioEncoder-\n");
    MIXER_DBG("uninitAudioEncoder -\n");
#if MIXER_AED_ENABLE
    if(g_bAudioAED) AedUninit();
#endif

#if MIXER_AED_ENABLE
    if(g_bAudioLSD) LsdUninit();
#endif
}

void MI_AudioEncoder::startAudioEncoder(MI_S32 audioIdx)
{
    MI_U8 s8OutFilePath[128];

    MIXER_DBG("\nStart capture audio buffer.\n");

    //for(MI_S32 audioIdx = 0; audioIdx < MI_AudioEncoder::g_audioEncoderNumber; audioIdx++)
    {
        pthread_create(&m_pthreadId, NULL, Audio_Task, (void *)((MI_S32)audioIdx));
        memset((char *)s8OutFilePath, 0x00, sizeof(s8OutFilePath));
        sprintf((char *)s8OutFilePath, "Audio_Task%d", g_pAudioEncoderArray[audioIdx]->m_AiChnPort.u32ChnId);
        pthread_setname_np(m_pthreadId, (const char *)s8OutFilePath);
    }
}

void MI_AudioEncoder::stopAudioEncoder()
{
    g_audio_exit = TRUE;
    MIXER_DBG("MI_AudioEncoder stopAudioEncoder +\n");
    MIXER_DBG("%s:%d g_audioEncoderNumber=%d, g_audio_thread_exit=%d\n", __func__, __LINE__, g_audioEncoderNumber, g_audio_thread_exit);

    while(g_audio_thread_exit < MI_AudioEncoder::g_audioEncoderNumber)
    {
        sleep(1);
        MIXER_DBG("%s:%d g_audio_thread_exit=%d\n", __func__, __LINE__, g_audio_thread_exit);
    }

    MIXER_DBG("MI_AudioEncoder stopAudioEncoder -\n");
}
