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
#include <signal.h>
#include <sys/prctl.h>

#include "mi_common_datatype.h"
#include "mi_sys.h"
#include "mi_ao.h"

#include "st_hdmi.h"
#include "st_common.h"
#include "mi_disp_datatype.h"

#define ExecFunc(func, _ret_) \
    printf("%d Start test: %s\n", __LINE__, #func);\
    if (func != _ret_)\
    {\
        printf("AO_TEST [%d] %s exec function failed\n",__LINE__, #func);\
        return 1;\
    }\
    else\
    {\
        printf("AO_TEST [%d] %s  exec function pass\n", __LINE__, #func);\
    }\
    printf("%d End test: %s\n", __LINE__, #func);

//#define MI_AUDIO_SAMPLE_PER_FRAME 128
#define MI_AUDIO_SAMPLE_PER_FRAME 768

#define MI_AUDIO_DEV_LINEOUT        0
#define MI_AUDIO_DEV_I2S            1
#define MI_AUDIO_DEV_HDMI           2

#define MI_MAX_AUDIO_DEV            2

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

typedef struct _AoOutChn_s {
    pthread_t pt;
    MI_AUDIO_DEV AoDevId;
    int fd;
    MI_AO_CHN AoChn;
    MI_BOOL bEndOfStream;
    MI_U32 u32InputFileSize;
    MI_U32 u32WriteTotalSize;
    MI_U32 u32InSampleRate;
    MI_U32 u32OutSampleRate;
    _WavHeader_t stWavHeader;
}_AoOutChn_t;

typedef struct
{
    pthread_t pt;
    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    MI_AUDIO_Attr_t stAudioAttr;
    char szFilePath[64];
} AO_SEND_Param_S;

MI_BOOL g_bExitFlag = 0;

static void _mi_ao_test_help(char *argv[])
{
    printf("\n");
    printf("Please enter: feature_ao -i xxx -r zzz\n");
    printf("xxx: input file path \n");
    printf("zzz: output sample rate, resample rate \n");
    printf("such as, %s -i input.wav -r 16000\n", argv[0]);
}

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
static void _mi_ao_interleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 *u32Addr = NULL;
    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    MI_U32 u32MaxSize = 0;

    u32Addr = (MI_U32 *)pu8LRBuf;
    u32MaxSize = u32Size / 2;
    for(s32Iidx = 0; s32Iidx < u32MaxSize; s32Iidx++)
    {
        u32Addr[s32Iidx] = (MI_U32)pu8LBuf[s32Jidx] | (((MI_U32)pu8RBuf[s32Jidx]) << 8) | (((MI_U32)pu8LBuf[s32Jidx + 1]) << 16) | (((MI_U32)pu8RBuf[s32Jidx + 1]) << 24);
        s32Jidx += 2;
    }

    return s32Ret;
}


void *_mi_ao_GetOutPortBuf(void* data)
{
#if 1
    _AoOutChn_t* pstAoOutChn = (_AoOutChn_t*)data;
    MI_AUDIO_Attr_t stGetAttr;
    MI_U64 u64NeedWriteSize = 0;
    MI_S16 s16SrcUpDownStatus = 0;
    MI_U32 u32WriteSize = 0;
    MI_U32 u32EndCnt = 0;
    MI_S32 s32Idx;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U8 u8ThreadName[30];
    MI_SYS_ChnPort_t stAoChn0OutputPort0;

    memset(u8ThreadName, 0, sizeof(u8ThreadName));
    sprintf(u8ThreadName, "GetOutPortBufThead_%d", pstAoOutChn->AoDevId);
    prctl(PR_SET_NAME, u8ThreadName);
    /* get ao device*/
    memset(&stGetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    ExecFunc(MI_AO_GetPubAttr(pstAoOutChn->AoDevId, &stGetAttr), MI_SUCCESS);
    u64NeedWriteSize = (pstAoOutChn->u32InputFileSize * (pstAoOutChn->u32OutSampleRate) + pstAoOutChn->u32InSampleRate -1) /(pstAoOutChn->u32InSampleRate);
    //printf("u64NeedWriteSize is : %d, \n",u64NeedWriteSize);

    // get data of sending by AO output port
    while(!g_bExitFlag)
    {
        memset(&stAoChn0OutputPort0, 0, sizeof(MI_SYS_ChnPort_t));

        stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
        stAoChn0OutputPort0.u32DevId = pstAoOutChn->AoDevId;
        stAoChn0OutputPort0.u32ChnId = pstAoOutChn->AoChn;
        stAoChn0OutputPort0.u32PortId = 0;

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stAoChn0OutputPort0,&stBufInfo,&hHandle))
        {
            u32WriteSize = stBufInfo.stRawData.u32BufSize;

            write(pstAoOutChn->fd, stBufInfo.stRawData.pVirAddr, u32WriteSize);
            pstAoOutChn->u32WriteTotalSize += u32WriteSize;

            MI_SYS_ChnOutputPortPutBuf(hHandle);

            if(pstAoOutChn->u32WriteTotalSize >= u64NeedWriteSize)
                break;

            u32EndCnt = 0;
        }
        else
        {
            usleep(1);
            u32EndCnt++;
            if(u32EndCnt >=100)
                break;
        }
    }

    //printf("Ending,  pstAoOutChn->u32WriteTotalSize: %d, \n", pstAoOutChn->u32WriteTotalSize);

    return NULL;
#endif
}

void *_mi_ao_SendStream(void* data)
{
    AO_SEND_Param_S *pstSendParam = (AO_SEND_Param_S *)data;
    int fd = -1;
    MI_S32 s32ReadLen = 0;
    MI_U8 au8TempBuf[2048 * 2 * 2];
    MI_U8 au8LBuf[2048*2];
    MI_U8 au8RBuf[2048*2];
    MI_U8 au8LRBuf[2048*2*2];
    MI_AUDIO_Attr_t *pstAudioAttr = &pstSendParam->stAudioAttr;
    MI_AUDIO_Frame_t stAudioFrame;
    MI_S32 s32RetSendStatus = MI_SUCCESS;
    _WavHeader_t stWavHeaderInput;
    MI_U8 u8ThreadName[30];

    memset(u8ThreadName, 0, sizeof(u8ThreadName));
    sprintf(u8ThreadName, "SendStreamThread_%d", pstSendParam->AoDevId);
    prctl(PR_SET_NAME, u8ThreadName);

    fd = open(pstSendParam->szFilePath, O_RDONLY, 0666);
    if(fd <= 0)
    {
        printf("Open input file path:%s fail \n", pstSendParam->szFilePath);
        return NULL;
    }

    // skip wav head
    s32ReadLen = read(fd, &stWavHeaderInput, sizeof(_WavHeader_t));

    while(!g_bExitFlag)
    {
        s32ReadLen = read(fd, &au8TempBuf, MI_AUDIO_SAMPLE_PER_FRAME * 2 * (pstAudioAttr->u32ChnCnt));
        if (s32ReadLen == 0)
        {
            break;
        }

        if(s32ReadLen != MI_AUDIO_SAMPLE_PER_FRAME*2*(pstAudioAttr->u32ChnCnt))
        {
            memset(au8TempBuf + s32ReadLen, 0,  MI_AUDIO_SAMPLE_PER_FRAME*2*(pstAudioAttr->u32ChnCnt) - s32ReadLen);
            s32ReadLen = MI_AUDIO_SAMPLE_PER_FRAME*2*(pstAudioAttr->u32ChnCnt);
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
            stAudioFrame.u32Len = s32ReadLen;
            //_mi_ao_deinterleave(au8TempBuf, au8LBuf, au8RBuf, s32ReadLen);
            stAudioFrame.apVirAddr[0] = au8TempBuf;
            stAudioFrame.apVirAddr[1] = NULL;
        }

        do
        {
            s32RetSendStatus = MI_AO_SendFrame(pstSendParam->AoDevId, pstSendParam->AoChn, &stAudioFrame, 1);
            usleep(1);
        } while(s32RetSendStatus == MI_AO_ERR_NOBUF);

        if(s32RetSendStatus != MI_SUCCESS)

        printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n", s32RetSendStatus);

        if(s32ReadLen != MI_AUDIO_SAMPLE_PER_FRAME * 2 * (pstAudioAttr->u32ChnCnt))
            break;
    }

    close(fd);
}

void ST_AO_HandleSig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        g_bExitFlag = 1;
        printf("catch Ctrl + C, exit\n");
    }
}

int main(int argc, char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32RetSendStatus = 0;

    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_DEV AoDevId[] = {MI_AUDIO_DEV_LINEOUT, MI_AUDIO_DEV_I2S, MI_AUDIO_DEV_HDMI};
    MI_AO_CHN AoChn;
    MI_AUDIO_SampleRate_e eInSampleRate;
    MI_AUDIO_SampleRate_e eOutSampleRate;
    MI_S32 s32SetVolumeDb;
    MI_S32 s32GetVolumeDb;

    MI_AO_VqeConfig_t stSetVqeConfig;
    MI_AO_VqeConfig_t stGetVqeConfig;

    MI_S32 s32Opt = 0;
    MI_BOOL bEnableRes = FALSE;
    MI_BOOL bEnableVqe = FALSE;
    MI_S32 s32VolumeDb = 0;
    MI_S32 s32Idx;
    struct sigaction sigAction;

    int fdRd =-1;

    MI_S32 s32RdSize;
    MI_S8* ps8InputFilePath = NULL;
    MI_S8 szOutputFilePath[64];
    _WavHeader_t stWavHeaderInput;

    // get output port buffer thread
    AO_SEND_Param_S stSendParam[MI_MAX_AUDIO_DEV];
    _AoOutChn_t stAoOutchn[MI_MAX_AUDIO_DEV];

    sigAction.sa_handler = ST_AO_HandleSig;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	sigaction(SIGINT, &sigAction, NULL);

    // parsing command line
    while ((s32Opt = getopt(argc, argv, "i:r:ve:")) != -1)
    {
        switch(s32Opt)
        {
            case 'i':
                ps8InputFilePath = optarg;
                printf("Input file path: %s \n", ps8InputFilePath);
                break;

            case 'r':
                bEnableRes = TRUE;
                eOutSampleRate =(MI_AUDIO_SampleRate_e)atoi(optarg);
                printf("MI_AO enable Resampling, Output sample rate is %d \n", eOutSampleRate);
                break;

            case 'v':
                bEnableVqe = TRUE;
                printf("MI_AO enable VQE \n");
                break;

            case 'e':
                s32VolumeDb = atoi(optarg);
                printf("MI_AO Voulme is %d dB \n", s32VolumeDb);
                break;

            case '?':
                if(optopt == 'i')
                    printf("Missing input file path, please -i 'input file path' \n");
                else if(optopt == 'o')
                    printf("Missing input file path, please -o 'output file path' \n");
                else if(optopt == 'e')
                    printf("Missing volume setting \n");
                else if(optopt == 'r')
                    printf("Missing Output sample of resample, please -r 'eOutSampleRate' \n");
                else if(optopt == 'd')
                    printf("Missing AO device ID, please -d 'AO Device ID' \n");
                else
                    printf("Invalid option received\n");

            default:
                _mi_ao_test_help(argv);
                return 1;
        }
    }

    if(ps8InputFilePath == NULL)
    {
        _mi_ao_test_help(argv);
        return 1;
    }

    fdRd = open(ps8InputFilePath, O_RDONLY, 0666);
    if(fdRd <= 0)
    {
        printf("Open input file path:%s fail \n", ps8InputFilePath);
        return 1;
    }

    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (AoDevId[s32Idx] == -1)
        {
            continue;
        }

        memset(szOutputFilePath, 0, sizeof(szOutputFilePath));
        snprintf(szOutputFilePath, sizeof(szOutputFilePath) -1,
            "%s_output_%d", ps8InputFilePath, AoDevId[s32Idx]);

        memset(&stAoOutchn[s32Idx], 0, sizeof(_AoOutChn_t));
        stAoOutchn[s32Idx].fd = open(szOutputFilePath, O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (stAoOutchn[s32Idx].fd < 0)
        {
            printf("Open output file path:%s fail \n", szOutputFilePath);
            if(fdRd>0)
                close(fdRd);
            return 1;
        }
    }

    //MI_SYS init
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);

    STCHECKRESULT(ST_Hdmi_Init()); //Set hdmi outout 1080P

    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, E_MI_HDMI_TIMING_1080_60P));

    // read input wav file
    s32RdSize = read(fdRd, &stWavHeaderInput, sizeof(_WavHeader_t));
    close(fdRd);

    printf("WAV channel %d\n", stWavHeaderInput.channels);
    printf("WAV samplerate %d\n", stWavHeaderInput.sample_rate);
    printf("WAV byterate %d\n", stWavHeaderInput.byterate);
    printf("WAV bits per sample %d\n", stWavHeaderInput.bits_per_sample);

    AoChn = 0;

    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    stSetAttr.u32ChnCnt = stWavHeaderInput.channels;

    if(stSetAttr.u32ChnCnt == 2)
    {
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    }
    else if(stSetAttr.u32ChnCnt == 1)
    {
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    }

    stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;

    if(bEnableRes)
    {
        eInSampleRate = (MI_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;
        stSetAttr.eSamplerate = eOutSampleRate; // AO Device output sample rate
    }

    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        stAoOutchn[s32Idx].AoDevId = AoDevId[s32Idx];
        stAoOutchn[s32Idx].AoChn = AoChn;
        stAoOutchn[s32Idx].bEndOfStream = FALSE;
        stAoOutchn[s32Idx].u32WriteTotalSize = 0;
        stAoOutchn[s32Idx].u32InputFileSize = stWavHeaderInput.data_size;
        stAoOutchn[s32Idx].u32InSampleRate = stWavHeaderInput.sample_rate / 1000;
        stAoOutchn[s32Idx].u32OutSampleRate= ((MI_U32)stSetAttr.eSamplerate) /1000;
    }

    /* set ao public attr*/
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;
    // MI_S32 s32Ret = MI_SUCCESS;

    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (AoDevId[s32Idx] == -1)
        {
            continue;
        }

        if (MI_SUCCESS != (s32Ret = MI_AO_SetPubAttr(AoDevId[s32Idx], &stSetAttr)))
        {
            printf("%s %d, AoDev:%d, 0x%X\n", __func__, __LINE__, AoDevId[s32Idx], s32Ret);
            return 1;
        }
        // ExecFunc(MI_AO_SetPubAttr(AoDevId[s32Idx], &stSetAttr), MI_SUCCESS);

        /* get ao device*/
        ExecFunc(MI_AO_GetPubAttr(AoDevId[s32Idx], &stGetAttr), MI_SUCCESS);

        /* enable ao device */
        ExecFunc(MI_AO_Enable(AoDevId[s32Idx]), MI_SUCCESS);

        /* enable ao channel of device*/
        ExecFunc(MI_AO_EnableChn(AoDevId[s32Idx], AoChn), MI_SUCCESS);

        // if test resample, enable Resample
        if(bEnableRes)
        {
            ExecFunc(MI_AO_EnableReSmp(AoDevId[s32Idx], AoChn, eInSampleRate), MI_SUCCESS);
        }

        /* if test VQE: set attribute of AO VQE  */
        if(bEnableVqe)
        {
            memset(&stSetVqeConfig, 0, sizeof(MI_AO_SetVqeAttr));
            stSetVqeConfig.bHpfOpen = FALSE;
            stSetVqeConfig.bAnrOpen = TRUE;
            stSetVqeConfig.bAgcOpen = FALSE;
            stSetVqeConfig.bEqOpen = FALSE;

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

            ExecFunc(MI_AO_SetVqeAttr(AoDevId, AoChn, &stSetVqeConfig), MI_SUCCESS);
            ExecFunc(MI_AO_GetVqeAttr(AoDevId, AoChn, &stGetVqeConfig), MI_SUCCESS);
            ExecFunc(MI_AO_EnableVqe(AoDevId, AoChn), MI_SUCCESS);

        }

        /* if test AO Volume */
        s32SetVolumeDb = s32VolumeDb; //
        if(0 != s32VolumeDb)
        {
            ExecFunc(MI_AO_SetVolume(AoDevId[s32Idx], s32SetVolumeDb), MI_SUCCESS);
            /* get AO volume */
            ExecFunc(MI_AO_GetVolume(AoDevId[s32Idx], &s32GetVolumeDb), MI_SUCCESS);
        }

        // test get output port buffer
        stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
        stAoChn0OutputPort0.u32DevId = AoDevId[s32Idx];
        stAoChn0OutputPort0.u32ChnId = AoChn;
        stAoChn0OutputPort0.u32PortId = 0;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13), MI_SUCCESS);
    }

    // write wav header to output file
    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (stAoOutchn[s32Idx].fd > 0)
        {
            write(stAoOutchn[s32Idx].fd, &stWavHeaderInput, sizeof(_WavHeader_t));
        }
    }

    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (AoDevId[s32Idx] == -1)
        {
            continue;
        }

        // create a thread to get output buffer
        pthread_create(&stAoOutchn[s32Idx].pt, NULL, _mi_ao_GetOutPortBuf, &stAoOutchn[s32Idx]);

        // create a thread to send stream to ao
        memset(&stSendParam[s32Idx], 0, sizeof(AO_SEND_Param_S));
        stSendParam[s32Idx].AoDevId = AoDevId[s32Idx];
        stSendParam[s32Idx].AoChn = AoChn;
        memcpy(&stSendParam[s32Idx].stAudioAttr, &stSetAttr, sizeof(MI_AUDIO_Attr_t));
        snprintf(stSendParam[s32Idx].szFilePath, sizeof(stSendParam[s32Idx].szFilePath) - 1,
            "%s", ps8InputFilePath);

        pthread_create(&stSendParam[s32Idx].pt, NULL, _mi_ao_SendStream, &stSendParam[s32Idx]);
    }

    while (!g_bExitFlag)
    {
        sleep(1);
    }

    // wait pthread exit
    sleep (5);

    printf("exit normal\n");

    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (AoDevId[s32Idx] == -1)
        {
            continue;
        }

        memset(&stAoChn0OutputPort0, 0, sizeof(MI_SYS_ChnPort_t));
        stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
        stAoChn0OutputPort0.u32DevId = AoDevId[s32Idx];
        stAoChn0OutputPort0.u32ChnId = AoChn;
        stAoChn0OutputPort0.u32PortId = 0;
        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 0, 10), MI_SUCCESS);

        /* Disable Resample */
        if(bEnableRes)
        {
            ExecFunc(MI_AO_DisableReSmp(AoDevId[s32Idx], AoChn), MI_SUCCESS);
        }

        /* Disable VQE */
        if(bEnableVqe)
        {
            ExecFunc(MI_AO_DisableVqe(AoDevId[s32Idx], AoChn), MI_SUCCESS);
        }

        /* disable ao channel of */
        ExecFunc(MI_AO_DisableChn(AoDevId[s32Idx], AoChn), MI_SUCCESS);

        /* disable ao device */
        ExecFunc(MI_AO_Disable(AoDevId[s32Idx]), MI_SUCCESS);
    }

    // update wav header of output file ;
    for (s32Idx = 0; s32Idx < MI_MAX_AUDIO_DEV; s32Idx ++)
    {
        if (stAoOutchn[s32Idx].fd < 0)
        {
            continue;
        }

        memcpy(&stAoOutchn[s32Idx].stWavHeader, &stWavHeaderInput, sizeof(_WavHeader_t));
        stAoOutchn[s32Idx].stWavHeader.sample_rate = stSetAttr.eSamplerate;
        stAoOutchn[s32Idx].stWavHeader.data_size = stAoOutchn[s32Idx].u32WriteTotalSize;
        stAoOutchn[s32Idx].stWavHeader.ChunkSize = stAoOutchn[s32Idx].stWavHeader.data_size + 36;

        lseek(stAoOutchn[s32Idx].fd, 0, SEEK_SET);
        write(stAoOutchn[s32Idx].fd, &stAoOutchn[s32Idx].stWavHeader, sizeof(_WavHeader_t));

        close(stAoOutchn[s32Idx].fd);
        stAoOutchn[s32Idx].fd = -1;
    }

    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    STCHECKRESULT(ST_Sys_Exit());


    return 0;

}
