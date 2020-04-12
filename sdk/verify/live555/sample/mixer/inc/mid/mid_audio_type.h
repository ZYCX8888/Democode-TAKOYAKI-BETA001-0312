/*
* mid_audio_type.h- Sigmastar
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
#ifndef __MI_AUDIO_TYPE_H__
#define __MI_AUDIO_TYPE_H__


#ifdef __cplusplus
extern "C"
{
#endif


#include "mid_common.h"
#include "mi_ai_datatype.h"
#include "mi_ao_datatype.h"
#include "mi_aio_datatype.h"


typedef MI_S32 AUDIO_DEV;
typedef MI_S32 AIO_CHN;
typedef MI_S32 AENC_CHN;
typedef MI_S32 ADEC_CHN;


#define    MIXER_AI_MAX_NUMBER        8
#define    MIXER_AO_MAX_NUMBER        1

//#define MI_AUDIO_SAMPLE_PER_FRAME   128
#define   MI_AUDIO_SAMPLE_PER_FRAME   1024

#define   MI_AUDIO_DEV_LINEOUT        0
#define   MI_AUDIO_DEV_I2S            1
#define   MI_AUDIO_DEV_ALL            2 //LineOut + I2s

/**
 *
 */
typedef enum _AudioSampleRate_e
{
    AUDIO_SAMPLE_RATE_8000   = 8000,
    AUDIO_SAMPLE_RATE_12000  = 12000,
    AUDIO_SAMPLE_RATE_11025  = 11025,
    AUDIO_SAMPLE_RATE_16000  = 16000,
    AUDIO_SAMPLE_RATE_22050  = 22050,
    AUDIO_SAMPLE_RATE_24000  = 24000,
    AUDIO_SAMPLE_RATE_32000  = 32000,
    AUDIO_SAMPLE_RATE_44100  = 44100,
    AUDIO_SAMPLE_RATE_48000  = 48000,
    AUDIO_SAMPLE_RATE_NUM,
} AudioSampleRate_e;

/**
 *
 */
typedef enum _AudioSoundMode_e
{
    AUDIO_SOUND_MODE_MONO      = 0,
    AUDIO_SOUND_MODE_STEREO,
    AUDIO_SOUND_MODE_NUM
} AudioSoundMode_e;


/**
 *
 */
typedef enum _AudioBitWidth_e
{
    AUDIO_BITWIDTH_4   = 4,
    AUDIO_BITWIDTH_8   = 8,
    AUDIO_BITWIDTH_16  = 16,
    AUDIO_BITWIDTH_24  = 24,
    AUDIO_BITWIDTH_NUM,
} AudioBitWidth_e;


/** ITU G726 (ADPCM) rate */
typedef enum
{
    G726Mode_Unused = 0,  /**< G726 Mode unused / unknown */
    G726Mode_16,          /**< 16 kbps */
    G726Mode_24,          /**< 24 kbps */
    G726Mode_32,          /**< 32 kbps, most common rate, also G721 */
    G726Mode_40,          /**< 40 kbps */
} Mode_e;

/** AMR band mode */
typedef enum _AMRBANDMODE
{
    AMRBandModeUnused = 0,          /**< AMRNB Mode unused / unknown */
    AMRBandModeNB0,                 /**< AMRNB Mode 0 =  4750 bps */
    AMRBandModeNB1,                 /**< AMRNB Mode 1 =  5150 bps */
    AMRBandModeNB2,                 /**< AMRNB Mode 2 =  5900 bps */
    AMRBandModeNB3,                 /**< AMRNB Mode 3 =  6700 bps */
    AMRBandModeNB4,                 /**< AMRNB Mode 4 =  7400 bps */
    AMRBandModeNB5,                 /**< AMRNB Mode 5 =  7950 bps */
    AMRBandModeNB6,                 /**< AMRNB Mode 6 = 10200 bps */
    AMRBandModeNB7,                 /**< AMRNB Mode 7 = 12200 bps */
    AMRBandModeWB0,                 /**< AMRWB Mode 0 =  6600 bps */
    AMRBandModeWB1,                 /**< AMRWB Mode 1 =  8850 bps */
    AMRBandModeWB2,                 /**< AMRWB Mode 2 = 12650 bps */
    AMRBandModeWB3,                 /**< AMRWB Mode 3 = 14250 bps */
    AMRBandModeWB4,                 /**< AMRWB Mode 4 = 15850 bps */
    AMRBandModeWB5,                 /**< AMRWB Mode 5 = 18250 bps */
    AMRBandModeWB6,                 /**< AMRWB Mode 6 = 19850 bps */
    AMRBandModeWB7,                 /**< AMRWB Mode 7 = 23050 bps */
    AMRBandModeWB8,                 /**< AMRWB Mode 8 = 23850 bps */
    AMRBandModeKhronosExtensions = 0x6F000000, /**< Reserved region for introducing Khronos Standard Extensions */
    AMRBandModeVendorStartUnused = 0x7F000000, /**< Reserved region for introducing Vendor Extensions */
    AMRBandModeMax = 0x7FFFFFFF
} AMRBandMode_e;

/**
 *
 */
typedef struct _AudioFade_t
{
    BOOL         fade;
} AudioFade_t;

/**
 *
 */
typedef struct _AudioMute_t
{
    BOOL mute;             /**< Mute setting for this port */
} AudioMute_t;





/**
 *
 */
typedef struct _AudioFrame_t
{
    MI_U8  *bufAddr;
    AudioBitWidth_e   bitWidth;     /*audio frame bitwidth*/
    AudioSoundMode_e     soundMode;    /*audio frame mono or stereo mode*/
    MI_U64  timeStamp;                /*audio frame timestamp*/
    MI_U32  seq;                      /*audio frame seq*/
    MI_U32  bufLen;                      /*data lenth per channel in frame*/
    void *privateData;
} AudioFrame_t;


/**
 *
 */
typedef struct _AudioStream_t
{
    MI_U8 *stream;         /* the virtual address of stream */
    MI_U32 phyAddr;      /* the physics address of stream */
    MI_U32 len;          /* stream lenth, by bytes */
    MI_U64 timeStamp;    /* frame time stamp*/
    MI_U32 seq;          /* frame seq,if stream is not a valid frame,nSeq is 0*/
} AudioStream_t;


/**
 *
 */

typedef enum
{
    CHANNEL_LF,
    CHANNEL_RF,
    CHANNEL_MAX
} ChannelType_e;

typedef struct _AudioAttr_t
{
    AudioSampleRate_e    SampleRate;   /* sample rate */
    AudioBitWidth_e      bitWidth;     /* bitwidth */
    AudioSoundMode_e     soundMode;    /* mono or stereo */
    MI_U32                  bufferLen;    /* audio buffer size */
    ChannelType_e        sndmode;      /*Ĭ???????;?????????*/
    BOOL                 abstimemode;  /*audioʱ?????Ǿ??Ի???????*/
} AioDevAttr_t;

typedef struct _AioAttr_t
{
    AudioSampleRate_e    SampleRate;
    AudioBitWidth_e      bitWidth;
    AudioSoundMode_e     soundMode;
} AioAttr_t; //for dev


typedef enum _ChnType_e
{
    AI_CHANNEL = 0,
    AO_CHANNEL,
    AENC_CHANNEL,
    ADEC_CHANNEL,

    VI_CHANNEL,
    VENC_CHANNEL,

    CHANNEL_NUM,
} ChnType_e;


typedef enum
{
    TUNNEL_MODE = 0,
    USER_MODE,
    SHARE_MODE,
    NOT_TUNNEL_MODE,
    INVALID_MODE
} ChannelMode_e;

typedef struct _Chn_t
{
    ChnType_e chnType;
    MI_U32    deviceID;
    MI_U32    chnID;
} Chn_t;

typedef struct _AioChn_t
{
    Chn_t  chn;
    ChannelMode_e mode;  //ͨ??ģʽ???Ƿ?tunnel
    BOOL bEnable;        //?Ƿ?ʹ?ܸ?ͨ??
    MI_U32    bufferLen;
    AioAttr_t chnAttr;
} AioChn_t;  //for channel

typedef struct _AioChnAttr_t
{
    //AudioAttr_t AudioAttr;
    Chn_t chn;
    ChannelMode_e mode;//ͨ??ģʽ???Ƿ?tunnel
    BOOL bEnable;//?Ƿ?ʹ?ܸ?ͨ??
    //U32  bufferLen;?Ժ???Ҫ????chn????
} AioChnAttr_t;



/**
 *
 */
typedef void (*pfnAdecStreamDoneCB)(void *pParam);
typedef struct _AdecCallback
{
    pfnAdecStreamDoneCB pCBFunc;
    void *pParam;
} AdecCallback_t;



typedef enum _MediaType_e
{
    MT_PCM          = 0,
    MT_G711A,
    MT_G711U,
    MT_G711,
    MT_AMR,
    MT_AAC,
    MT_G726,
    MT_MP3,
    MT_OGG,
    MT_ADPCM,
    MT_OPUS,
    MT_G726_16,
    MT_G726_24,
    MT_G726_32,
    MT_G726_40,
    MT_NUM
} MediaType_e;

typedef enum _AUDIO_WNRTYPE
{
    AUDIO_WNROff = 0,    /**< WNR is disabled */
    AUDIO_WNRNormal,     /**< WNR normal operation - Hpf enabled*/
    AUDIO_WNRPlusNF,     /**< WNR plus NF operation- Hpf + Noise Filter */
} WNRType_e;

typedef struct _AGC_CONFIG_T
{
    MI_S16 AgcGain;//-80 - 0
} AGC_CONFIG;

typedef struct _WNR_CONFIG_T
{
    WNRType_e WnrType;
} WNR_CONFIG;

typedef struct _ANR_CONFIG_T
{
    MI_U16 nLevel;//0-30
} ANR_CONFIG;

typedef struct _EQ_CONFIG_T
{
    MI_S16 nEqTable[257];
} EQ_CONFIG;
typedef struct AI_VQE_CONFIG_T
{
    AGC_CONFIG AgcCfg;
    WNR_CONFIG WnrCfg;
    ANR_CONFIG AnrCfg;
    EQ_CONFIG  EqCfg;
} AI_VQE_CONFIG;

typedef struct AI_ADCCONFIG_T
{
    MI_U16 nMode;           /**< ADC source mode */
    MI_S16 nGain;           /**< ADC gain  */
    MI_S16 nPreGain;        /**< MIC pregain, not required for ADC setting  */
} AiAdcConfig_t;

/**
 *  Audio encoder channel atrribute
 */
typedef struct _AencAttr_t
{
    AudioSampleRate_e         SampleRate;   /* sample rate */
    AudioBitWidth_e     bitWidth;           /* bitwidth */
    AudioSoundMode_e         soundMode;    /* mono or stereo */
} AencAttr_t;

typedef struct _G726Attr_t
{
    AudioSampleRate_e         SampleRate;   /* sample rate */
    AudioBitWidth_e     bitWidth;           /* bitwidth */
    AudioSoundMode_e         soundMode;    /* mono or stereo */
    Mode_e                     mode;
} G726Attr_t;

typedef struct _AMRAttr_t
{
    AudioSampleRate_e         SampleRate;   /* sample rate */
    AudioBitWidth_e     bitWidth;           /* bitwidth */
    AudioSoundMode_e         soundMode;    /* mono or stereo */
    AMRBandMode_e               mode;
} AMRAttr_t;



typedef enum _AUDIO_OPUSFRAMEDUR
{
    AUDIO_OPUSFrameDur2_5_MS = 0,
    AUDIO_OPUSFrameDur5_MS,
    AUDIO_OPUSFrameDur10_MS,
    AUDIO_OPUSFrameDur20_MS,
    AUDIO_OPUSFrameDur40_MS,
    AUDIO_OPUSFrameDur60_MS,
    AUDIO_OPUSFrameDurMax = 0x7FFFFFFF
} Audio_OpusframeDur;

typedef enum _AUDIO_OPUSVBRTYPE
{
    AUDIO_OPUSVbrOff = 0,
    AUDIO_OPUSVbrOn,
    AUDIO_OPUSVbrConstrained,
    AUDIO_OPUSVbrMax = 0x7FFFFFFF
} Audio_OpusVbrtype;

typedef struct _OPUSAttr_t
{
    AudioSampleRate_e         SampleRate;   /* sample rate */
    AudioBitWidth_e     bitWidth;           /* bitwidth */
    AudioSoundMode_e         soundMode;    /* mono or stereo */
    MI_U32                  nBitRate;
    Audio_OpusframeDur   eFrameDuration;
    Audio_OpusVbrtype   eVbrType;
} OPUSAttr_t;

typedef struct _AencDevChnAttr_t
{
    MediaType_e           type;                    /*audio type ()*/
    union
    {
        AencAttr_t g711cfg;
        AencAttr_t AACcfg;
        AencAttr_t adpcmcfg;
        G726Attr_t g726cfg;
        AMRAttr_t  amrcfg;
        OPUSAttr_t opuscfg;
    } CodecType;
    MI_U32 oneframemode;//0:default,1:oneframe
} AencDevAttr_t;

typedef struct _AencChnAttr_t
{
    AencDevAttr_t aencdevattr;
    Chn_t chn;
    ChannelMode_e mode;//ͨ??ģʽ???Ƿ?tunnel
    BOOL bEnable;//?Ƿ?ʹ?ܸ?ͨ??
} AencChnAttr_t;


/**
 *  Audio decoder channel attibute
 */
typedef struct _AdecChnAttr_t
{
    AencDevAttr_t adecdevattr;
    Chn_t chn;
    ChannelMode_e mode;//ͨ??ģʽ???Ƿ?tunnel
    BOOL bEnable;//?Ƿ?ʹ?ܸ?ͨ??
} AdecChnAttr_t;


// WAVE file header format
typedef struct _WavHeader_s
{
    MI_U8   riff[4];                // RIFF string
    MI_U32  ChunkSize;              // overall size of file in bytes
    MI_U8   wave[4];                // WAVE string
    MI_U8   fmt_chunk_marker[4];    // fmt string with trailing null char
    MI_U32  length_of_fmt;          // length of the format data
    MI_U16  format_type;            // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    MI_U16  channels;               // no.of channels
    MI_U32  sample_rate;            // sampling rate (blocks per second)
    MI_U32  byterate;               // SampleRate * NumChannels * BitsPerSample/8
    MI_U16  block_align;            // NumChannels * BitsPerSample/8
    MI_U16  bits_per_sample;        // bits per sample, 8- 8bits, 16- 16 bits etc
    MI_U8   data_chunk_header [4];  // DATA string or FLLR string
    MI_U32  data_size;              // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
}WavHeader_t;


typedef struct _MixerAudioInParam
{
    // u32DevId ==> 0:AMIC[2chn]; 1:DMIC[4chn]; 2:I2S[8chn]; default:0
    MI_SYS_ChnPort_t stAudioInChnPort;
    MediaType_e AiMediaType;
    MI_S32 s32VolumeInDb;

    MI_U8 u8UserFrameDepth;
    MI_U8 u8BufQueueDepth;
    MI_U8 u8AudioInNum;
    MI_U8 bAudioInVqe;

    MI_AUDIO_Attr_t stAudioAttr;
    //MI_AI_VqeConfig_t stAiVqeCfg;
    MI_AUDIO_SampleRate_e eAudioInReSampleRate;
    BOOL bFlag;
}MixerAudioInParam;

typedef struct _MixerAudioOutParam
{
    // u32DevId ==> 0:AMIC[2chn]; 1:DMIC[4chn]; 2:I2S[8chn]; default:0
    MI_SYS_ChnPort_t stAudioOutChnPort;
    WavHeader_t stWavHeader;
    MediaType_e AoMediaType;
    MI_S32 s32VolumeOutDb;
    MI_U32 s32AudioOutNum;

    MI_AUDIO_Attr_t stAudioAttr;
    //MI_AO_VqeConfig_t stAoVqeCfg;
    MI_AUDIO_SampleRate_e eAudioOutReSampleRate;

    MI_U8  u8UserFrameDepth;
    MI_U8  u8BufQueueDepth;
    MI_U8  u8AudioOutRes;
    MI_U8  u8AudioOutVqe;

    MI_S8 s8AudioPath[64];
    BOOL bFlag;
}MixerAudioOutParam;


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__MI_AUDIO_TYPE_H__

