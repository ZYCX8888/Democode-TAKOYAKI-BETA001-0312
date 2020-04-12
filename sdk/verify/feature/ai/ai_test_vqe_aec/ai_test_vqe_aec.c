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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mi_common_datatype.h"
#include "mi_sys.h"
#include "mi_ai.h"
#include "mi_ao.h"

#if 0
#define ExecFunc(func, _ret_) \
    printf("%d Start test: %s\n", __LINE__, #func);\
    if (func != _ret_)\
    {\
        printf("AI_TEST [%d] %s exec function failed\n",__LINE__, #func);\
        return 1;\
    }\
    else\
    {\
        printf("AI_TEST [%d] %s  exec function pass\n", __LINE__, #func);\
    }\
    printf("%d End test: %s\n", __LINE__, #func);
#else
#define ExecFunc(func, _ret_) func
#endif

#define MI_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, eWidth)          \
    switch(eWidth) \
    {   \
        case E_MI_AUDIO_BIT_WIDTH_16:   \
            u32BitWidthByte = 2;    \
        break;  \
        case E_MI_AUDIO_BIT_WIDTH_24:   \
            u32BitWidthByte = 4;    \
            break;  \
        default:    \
            u32BitWidthByte = 0; \
            printf("BitWidth is illegal = %u.\n", eWidth); \
            break; \
    }

//#define MI_AUDIO_SAMPLE_PER_FRAME 128
#define MI_AUDIO_SAMPLE_PER_FRAME 1024

/*=============================================================*/
// Global Variable definition
/*=============================================================*/
// WAVE file header format
typedef struct _WavHeader_s{
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
}_WavHeader_t;


typedef struct _AiChn_s {
    MI_AUDIO_DEV AiDevId;
    MI_AI_CHN AiChn;
    pthread_t tid;
    MI_S8 szOutFilePath[128];
    MI_U8 _szRecData[16000*2*90]; // 90s , 16k
    MI_AUDIO_SampleRate_e eAttrSampleRate;
    MI_AUDIO_BitWidth_e eBitWidth;
}_AiChn_t;

typedef struct _AoChn_s {
    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    int fdRd;
    pthread_t tid;
}_AoChn_t;

typedef struct _AOSendParam_s
{
    pthread_t pt;
    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    MI_AUDIO_Attr_t stAudioAttr;
    int fdRd;
    char szFilePath[64];
} _AOSendParam_t;

static _AiChn_t _astAiChn[MI_AUDIO_MAX_CHN_NUM];
static _AoChn_t _gstAochn;
static _AOSendParam_t _gstAoSendParam;

static MI_S32 s32StopFrameCntsize = 64;
static MI_U32 _gu32SkipTime = 0; //ms second
static MI_U32 _gu32RecTime = 10; //second

static void _mi_ao_deinterleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size)
{
    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    MI_U32 u32MaxSize = 0;
    MI_S32 s32LeftIdx = 0;
    MI_S32 s32RightIdx = 0;

    u32MaxSize = u32Size/4;

    for(s32Iidx = 0; s32Iidx < u32MaxSize; s32Iidx++)
    {
        pu8LBuf[s32LeftIdx++] = pu8LRBuf[s32Jidx++];
        pu8LBuf[s32LeftIdx++] = pu8LRBuf[s32Jidx++];
        pu8RBuf[s32RightIdx++] = pu8LRBuf[s32Jidx++];
        pu8RBuf[s32RightIdx++] = pu8LRBuf[s32Jidx++];
    }
}

void* _mi_ai_GetChnPortBuf(void* data)
{
    _AiChn_t* pstAiChn = (_AiChn_t*)data;
    MI_AUDIO_Frame_t stAiChFrame;
    MI_AUDIO_AecFrame_t stRefFrame;
    MI_S32 s32Ret;
    int fdWr = -1;
    MI_U32 u32FrmeLen = 0;
    MI_U32 u32Wptr = 0;
    MI_U32 u32BufLen;
    MI_U32 u32SmpRate = (MI_U32)pstAiChn->eAttrSampleRate;
    MI_U32 u32BitWidthByte;
    MI_AUDIO_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, pstAiChn->eBitWidth);
    MI_U32 u32RecLen = _gu32RecTime * u32SmpRate * u32BitWidthByte;
    MI_U32 u32SkipLen = u32SmpRate * _gu32SkipTime/1000 * u32BitWidthByte;
    MI_U32 u32RdAllLen = 0;
    //MI_U32 u32WrLen = (u32RecLen -u32SkipLen);
    MI_U32 u32FrmCnt = 0;

    u32BufLen = sizeof( pstAiChn->_szRecData);
    memset(pstAiChn->_szRecData, 0, u32BufLen);

    sprintf(pstAiChn->szOutFilePath, "/tmp/ai_test_chan%d.pcm", pstAiChn->AiChn);
    fdWr = open(pstAiChn->szOutFilePath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);


    if( (fdWr>0) )
   {
        while(1)
        {

            s32Ret = MI_AI_GetFrame(pstAiChn->AiDevId, pstAiChn->AiChn,  &stAiChFrame,  &stRefFrame, 0);

            if(s32Ret == MI_SUCCESS)
            {
                u32FrmCnt ++;
                if(u32RdAllLen >= u32SkipLen)
                {
                    write(fdWr, stRefFrame.stRefFrame.apVirAddr[0], stRefFrame.stRefFrame.u32Len);

                    u32FrmeLen = stRefFrame.stRefFrame.u32Len;

                    if(u32Wptr >= u32RecLen)
                    {
                        printf("Device %d write is full, u32RecLen:%d, u32Wptr:%d \n ", pstAiChn->AiDevId, u32RecLen, u32Wptr);

                    }
                    else if(u32Wptr + u32FrmeLen < u32RecLen )
                    {
                        memcpy(pstAiChn->_szRecData+u32Wptr ,stRefFrame.stRefFrame.apVirAddr[0], u32FrmeLen);
                        u32Wptr += u32FrmeLen;
                    }
                    else // u32Wptr + u32FrmeLen >= u32RecLen
                    {
                        memcpy(pstAiChn->_szRecData + u32Wptr, stRefFrame.stRefFrame.apVirAddr[0], u32RecLen - u32Wptr);
                        u32Wptr  = u32RecLen;
                    }
                    u32RdAllLen += u32FrmeLen;
                }
                else // u32RdAllLen < u32SkipLen
                {
                    u32FrmeLen = stRefFrame.stRefFrame.u32Len;
                    u32RdAllLen += u32FrmeLen;
                }
                MI_AI_ReleaseFrame(pstAiChn->AiDevId, pstAiChn->AiChn,  &stAiChFrame,  &stRefFrame);

                if(u32Wptr >= u32RecLen)
                    break;

            }
            else
            {
                //printf("get frame fail \n");
            }
            usleep(1);
        }

   }
   else
   {
        printf("Open output file  fail: fdr %d, file path:%s \n", fdWr, pstAiChn->szOutFilePath);
   }

    if(fdWr>0)
           close(fdWr);

    // printf("User get frame cnt: %d !!!!\n", u32FrmCnt);  //for debug

    return NULL;

}

void *_mi_ao_SendStream(void* data)
{
    _AOSendParam_t *pstSendParam = (_AOSendParam_t *)data;
    MI_S32 s32ReadLen = 0;
    MI_U8 au8TempBuf[2048 * 2 * 2];
    MI_U8 au8LBuf[2048*2];
    MI_U8 au8RBuf[2048*2];

    MI_AUDIO_Attr_t *pstAudioAttr = &pstSendParam->stAudioAttr;
    MI_AUDIO_Frame_t stAudioFrame;
    MI_S32 s32RetSendStatus = MI_SUCCESS;
    MI_U8 u8ThreadName[30];

    memset(u8ThreadName, 0, sizeof(u8ThreadName));
    sprintf(u8ThreadName, "SendStreamThread_%d", pstSendParam->AoDevId);
    //prctl(PR_SET_NAME, u8ThreadName);

    while(s32ReadLen = read(pstSendParam->fdRd, &au8TempBuf, (pstAudioAttr->u32PtNumPerFrm) * 2 * (pstAudioAttr->u32ChnCnt)))
    {

        if(s32ReadLen != (pstAudioAttr->u32PtNumPerFrm)*2*(pstAudioAttr->u32ChnCnt))
        {
            memset(au8TempBuf + s32ReadLen, 0,  (pstAudioAttr->u32PtNumPerFrm)*2*(pstAudioAttr->u32ChnCnt) - s32ReadLen);
            s32ReadLen = (pstAudioAttr->u32PtNumPerFrm)*2*(pstAudioAttr->u32ChnCnt);
        }

        //read data and send to AO module
        stAudioFrame.eBitwidth = pstAudioAttr->eBitwidth;
        stAudioFrame.eSoundmode = pstAudioAttr->eSoundmode;

        if(pstAudioAttr->eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
        {
            stAudioFrame.u32Len = s32ReadLen;
            stAudioFrame.apVirAddr[0] = au8TempBuf;
            stAudioFrame.apVirAddr[1] = NULL;
        }
        else  // stereo mode
        {
            stAudioFrame.u32Len = s32ReadLen / 2;
            _mi_ao_deinterleave(au8TempBuf, au8LBuf, au8RBuf, s32ReadLen);
            stAudioFrame.apVirAddr[0] = au8LBuf;
            stAudioFrame.apVirAddr[1] = au8RBuf;
        }

        do
        {
            s32RetSendStatus = MI_AO_SendFrame(pstSendParam->AoDevId, pstSendParam->AoChn, &stAudioFrame, 1);
            usleep(((pstAudioAttr->u32PtNumPerFrm * 1000) / pstAudioAttr->eSamplerate - 10) * 1000 );

        } while(s32RetSendStatus == MI_AO_ERR_NOBUF);

        if(s32RetSendStatus != MI_SUCCESS)
            printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n", s32RetSendStatus);

        if(s32ReadLen != (pstAudioAttr->u32PtNumPerFrm) * 2 * (pstAudioAttr->u32ChnCnt))
            break;
    }

    close(pstSendParam->fdRd);

    return NULL;
}

static void _mi_ai_test_help()
{
    printf("\n");
    printf("Please enter: ai_test_vqe_aec [-d AI device][-z frame size] [-t time] [-s skip time] [-f SampleRate][-i AO input file path] \n");
    printf("device: AI device number [1:Mic ]\n");
    printf("AO/AI audio frame size, [128, 256..2048]");
    printf("time: test time \n");
    printf("skip time, <= 10000 ms \n");
    printf("SampleRate:[8k, 16k]\n");
    printf("AO input file path \n");
}


int main(int argc, char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;

    MI_AUDIO_DEV AiDevId = 1;
    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_U32 u32AudioFrameSize = 1024;
    MI_AUDIO_SampleRate_e eAttrSampleRate = E_MI_AUDIO_SAMPLE_RATE_16000;
    MI_BOOL bEnableVqe = TRUE;
    MI_AI_VqeConfig_t stSetVqeConfig;
    MI_AI_VqeConfig_t stGetVqeConfig;
    MI_BOOL bVqeAecEnable = FALSE;
    MI_U32 u32AecSupfreq[6] = {20,40,60,80,100,120};
    MI_U32 u32AecSupIntensity[7] = {4,4,4,4,6,6,6};

    MI_S32 s32Opt = 0;
    MI_S32 s32Idx;
    MI_S32 s32ChanNum = 2;
    MI_U32 u32TotalFrmCnt = 0;

    MI_S32 s32RdSize;

    // AO device
    MI_AUDIO_DEV AoDevId = 0;
    MI_AUDIO_Attr_t stAoSetAttr;
    MI_AUDIO_Attr_t stAoGetAttr;
    MI_AO_CHN AoChn;

    // file read/write
    int fdRd =-1;

    MI_S8* ps8InputFilePath = NULL;
    _WavHeader_t stWavHeaderInput;
    MI_AUDIO_SaveFileInfo_t stSaveFileInfo;

    // parsing command line
    while ((s32Opt = getopt(argc, argv, "d:i:f:s:t:z:a")) != -1)
    {
        switch(s32Opt)
        {
            case 'f':
                eAttrSampleRate = (MI_AUDIO_SampleRate_e)atoi(optarg);
                printf("AI device sample rate is %d \n", eAttrSampleRate);
                break;

            case 'z':
                u32AudioFrameSize = atoi(optarg);
                if(u32AudioFrameSize >2048)
                {
                    u32AudioFrameSize = 1024;
                    printf("u32AudioFrameSize is [128, 256...2048] \n");
                }
                break;

            case 't':
                _gu32RecTime = atoi(optarg);
                break;

            case 's':
                _gu32SkipTime = atoi(optarg);
                if(_gu32SkipTime >10000)
                {
                    _gu32SkipTime = 10000;
										printf("[Warning!!!] The max skip time = 10000 ms\n");
                }
                break;

            case 'd':
                AiDevId = atoi(optarg);
                printf("Enable AI device %d !\n", AiDevId);
                break;

            case 'i':
                ps8InputFilePath = optarg;
                printf("Input file path: %s \n", ps8InputFilePath);
                break;

            case 'a':
                bVqeAecEnable = TRUE;
                break;

            case '?':
                if(optopt == 'f')
                    printf("Missing AI/AO device sample rate , please -f [eAttrSampleRate: 8k, 16k] \n");
                else if(optopt == 't')
                    printf("Missing record time, please [-t time] \n");
                else if(optopt == 'd')
                    printf("Missing AI device ID, please [-d AI Device ID] \n");
                else if(optopt == 'i')
                    printf("Missing AO input file path, please [-i input file path] \n");
                else if(optopt == 's')
                    printf("Missing skip time of start record, please [-s skip time] \n");
                else
                    printf("Invalid option received\n");

            default:
                _mi_ai_test_help();
                return 1;
        }

    }

    //Open AO input file
    /* open file  to read data and write data */
    fdRd = open(ps8InputFilePath, O_RDONLY, 0666);
    if(fdRd <= 0)
    {
        printf("Open input file path:%s fail \n", ps8InputFilePath);
        return 1;
    }

    // skip wav head
    s32RdSize = read(fdRd, &stWavHeaderInput, sizeof(_WavHeader_t));

    //MI_SYS init
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);

    // AI
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eSamplerate = eAttrSampleRate;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 16;
    stSetAttr.u32PtNumPerFrm = u32AudioFrameSize;
    stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.u32ChnCnt = 2;

    /* set ai public attr*/
    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stSetAttr), MI_SUCCESS);

    /* get ai device*/
    ExecFunc(MI_AI_GetPubAttr(AiDevId, &stGetAttr), MI_SUCCESS);


    //====  AO ===========================//
    AoChn = 0;
    stAoSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAoSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoSetAttr.u32FrmNum = 6;
    stAoSetAttr.u32PtNumPerFrm = u32AudioFrameSize;
    stAoSetAttr.u32ChnCnt = stWavHeaderInput.channels;

    if(stAoSetAttr.u32ChnCnt == 2)
        stAoSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    else if(stAoSetAttr.u32ChnCnt == 1)
        stAoSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;

     stAoSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;

    /* set ao public attr*/
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stAoSetAttr), MI_SUCCESS);

    /* get ao device*/
    ExecFunc(MI_AO_GetPubAttr(AoDevId, &stAoGetAttr), MI_SUCCESS);

    /* enable ao device */
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);

    /* enable ao channel of device*/
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);

    // set Ao send param
    _gstAoSendParam.fdRd = fdRd;
    _gstAoSendParam.AoDevId = AoDevId;
    _gstAoSendParam.AoChn = AoChn;
    memcpy(&_gstAoSendParam.stAudioAttr, &stAoSetAttr, sizeof(MI_AUDIO_Attr_t));

    MI_SYS_ChnPort_t stAoChn0OutputPort0;
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0,34,35), MI_SUCCESS);

    // ============================//

    if(bEnableVqe)
    {
        memset(&stSetVqeConfig, 0, sizeof(MI_AI_VqeConfig_t));
        stSetVqeConfig.bHpfOpen = FALSE;
        stSetVqeConfig.bAnrOpen = FALSE;
        stSetVqeConfig.bAgcOpen = FALSE;
        stSetVqeConfig.bEqOpen = FALSE;
         stSetVqeConfig.bAecOpen = bVqeAecEnable;

        stSetVqeConfig.s32FrameSample = 128;
        stSetVqeConfig.s32WorkSampleRate = stSetAttr.eSamplerate;

        //Hpf
        stSetVqeConfig.stHpfCfg.bUsrMode = 1;
        stSetVqeConfig.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_120;

        //Anr
        stSetVqeConfig.stAnrCfg.bUsrMode= 1;
        stSetVqeConfig.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_LOW;
        stSetVqeConfig.stAnrCfg.u32NrIntensity = 10;            //[0,30]
        stSetVqeConfig.stAnrCfg.u32NrSmoothLevel = 10;

        //Agc
        stSetVqeConfig.stAgcCfg.bUsrMode = 1;
        stSetVqeConfig.stAgcCfg.s32NoiseGateDb = -50;           //[-80, 0], NoiseGateDb disable when value = -80
        stSetVqeConfig.stAgcCfg.s32TargetLevelDb =   -15;       //[-80, 0]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit = 1;  //[-20, 30]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax =  20; //[0, 30]
        stSetVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin = -10; //[-20, 30]
        stSetVqeConfig.stAgcCfg.u32AttackTime = 1;
        stSetVqeConfig.stAgcCfg.u32CompressionRatio = 1;        //[1, 10]
        stSetVqeConfig.stAgcCfg.u32DropGainMax = 60;            //[0,60]
        stSetVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb = 0;  //[0,100]
        stSetVqeConfig.stAgcCfg.u32ReleaseTime = 1;

        //Eq
        stSetVqeConfig.stEqCfg.bUsrMode = 1;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain100Hz = 5;     //[-50,20]
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain200Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain250Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain350Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain500Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain800Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain1200Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain2500Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain4000Hz = 5;
        stSetVqeConfig.stEqCfg.stEqGain.s16EqGain8000Hz = 5;

        //AEC
        stSetVqeConfig.stAecCfg.bComfortNoiseEnable = FALSE;
        memcpy(stSetVqeConfig.stAecCfg.u32AecSupfreq, u32AecSupfreq, sizeof(u32AecSupfreq));
        memcpy(stSetVqeConfig.stAecCfg.u32AecSupIntensity,  u32AecSupIntensity, sizeof(u32AecSupIntensity));
    }

     //set AI output port buffer depth ???
    MI_SYS_ChnPort_t astAiChnOutputPort0[MI_AUDIO_MAX_CHN_NUM];


    // creat AO send data thread
    // if you want to AO playback first
    //pthread_create(&_gstAoSendParam.pt, NULL, _mi_ao_SendStream, &_gstAoSendParam);
    //usleep(10000);

     /* enable ai device */
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);

    // dump VQE data
    stSaveFileInfo.bCfg = TRUE;
    sprintf(stSaveFileInfo.szFilePath , "/tmp");

    // creat thread to get frame data to save file
    //for(s32Idx = 0; s32Idx < s32ChanNum; s32Idx++)
    for(s32Idx = 0; s32Idx < 1; s32Idx++) // for aec , process mic 0
    {
        _astAiChn[s32Idx].AiDevId = AiDevId;
        _astAiChn[s32Idx].AiChn= s32Idx;
        _astAiChn[s32Idx].eAttrSampleRate = eAttrSampleRate;
        _astAiChn[s32Idx].eBitWidth= stSetAttr.eBitwidth;

        memset(&astAiChnOutputPort0[s32Idx], 0x0, sizeof(MI_SYS_ChnPort_t));
        astAiChnOutputPort0[s32Idx].eModId = E_MI_MODULE_ID_AI;
        astAiChnOutputPort0[s32Idx].u32DevId = _astAiChn[s32Idx].AiDevId;
        astAiChnOutputPort0[s32Idx].u32ChnId = _astAiChn[s32Idx].AiChn;
        astAiChnOutputPort0[s32Idx].u32PortId = 0;
        ExecFunc(MI_SYS_SetChnOutputPortDepth(&astAiChnOutputPort0[s32Idx],12,13), MI_SUCCESS);

        /* enable ai channel of device*/
        ExecFunc(MI_AI_EnableChn(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn), MI_SUCCESS);


        /* if test VQe: set attribute of AO VQE  */
        if(bEnableVqe)
        {
            // need to check algorithm configure
            ExecFunc(MI_AI_SetVqeAttr(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, 0, 0, &stSetVqeConfig), MI_SUCCESS);
            ExecFunc(MI_AI_GetVqeAttr(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, &stGetVqeConfig), MI_SUCCESS);
            ExecFunc(MI_AI_EnableVqe(_astAiChn[s32Idx].AiDevId,_astAiChn[s32Idx].AiChn), MI_SUCCESS);
        }

        MI_AI_SaveFile(_astAiChn[s32Idx].AiDevId, _astAiChn[s32Idx].AiChn, &stSaveFileInfo);

        printf("create thread to get channel %d data\n", _astAiChn[s32Idx].AiChn);
        pthread_create(&_astAiChn[s32Idx].tid, NULL, _mi_ai_GetChnPortBuf, &_astAiChn[s32Idx]);
    }

    // creat AO send data thread
    pthread_create(&_gstAoSendParam.pt, NULL, _mi_ao_SendStream, &_gstAoSendParam);

    // wait for getting frame data threshold return
    //for(s32Idx = 0; s32Idx < s32ChanNum; s32Idx++)
    for(s32Idx = 0; s32Idx < 1; s32Idx++) // for aec , process mic 0
    {
        pthread_join(_astAiChn[s32Idx].tid,NULL);

        /* Disable VQE */
        if(bEnableVqe)
        {
            ExecFunc(MI_AI_DisableVqe(_astAiChn[s32Idx].AiDevId,_astAiChn[s32Idx].AiChn), MI_SUCCESS);
        }

        /* disable ai channel of device */
        ExecFunc(MI_AI_DisableChn(AiDevId, _astAiChn[s32Idx].AiChn), MI_SUCCESS);
        //MI_SYS_SetChnOutputPortDepth(&astAiChnOutputPort0[s32Idx],0,10);
    }


    // wait for _mi_ao_SendStream end
    pthread_join(_gstAoSendParam.pt, NULL);

    /* disable ao channel of */
    ExecFunc(MI_AO_DisableChn(AoDevId, AoChn), MI_SUCCESS);

    /* disable ao device */
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);



    /* disable ai device */
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);



    return 0;
}
