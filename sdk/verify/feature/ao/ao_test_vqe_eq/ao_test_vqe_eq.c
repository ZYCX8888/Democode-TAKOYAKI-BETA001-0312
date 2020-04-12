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
#include "mi_ao.h"


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

#define MI_AUDIO_SAMPLE_PER_FRAME 768

#define MI_AO_USR_CHECK_HPFFREQUENCY(eHpfFreq)    \
    if( (eHpfFreq != E_MI_AUDIO_HPF_FREQ_80) && (eHpfFreq != E_MI_AUDIO_HPF_FREQ_120) &&\
        (eHpfFreq != E_MI_AUDIO_HPF_FREQ_150)) \
    { \
        DBG_ERR("Frequency of HPF setting is illegal = %u.\n", eHpfFreq);   \
        return MI_AO_ERR_ILLEGAL_PARAM;   \
    }


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
    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    int fdWr;
    MI_BOOL bEndOfStream;
    MI_U32 u32InputFileSize;
    MI_U32 u32WriteTotalSize;
    MI_U32 u32InSampleRate;
    MI_U32 u32OutSampleRate;
}_AoOutChn_t;

static MI_AUDIO_Frame_t _gstAoTestFrame;
static _AoOutChn_t _gstAoOutchn;
static MI_U16  _gau16GaindB[129] = {256, 256, 100, 50, 10, 1, 1, 1,
                                                             1, 1 ,1 ,1 ,1 ,1 ,10 ,50 ,100 ,200 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256 ,256};
static void _mi_ao_test_vqe_help()
{
    printf("\n");
    printf("Please enter: ./feature_ao_test_vqe_eq -i xxx -o yyy -e zzz -g aaa -d bbb \n");
    printf("xxx: input file path \n");
    printf("yyy: output file path \n");
    printf("zzz: No. Frequency of EQ \n");
    printf("aaa: Index gain value of EQ \n");
    printf("bbb: AO device ID\n");
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
    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    for(s32Iidx=0; s32Iidx<u32Size/2; s32Iidx++)
    {
        pu8LRBuf[s32Jidx++] = pu8LBuf[s32Iidx];
        pu8LRBuf[s32Jidx++] = pu8RBuf[s32Iidx];
    }

}


void *_mi_ao_GetOutPortBuf(void* data)
{
    MI_AUDIO_Attr_t stGetAttr;
    _AoOutChn_t* pstAoOutChn = (_AoOutChn_t*)data;
    MI_U64 u64NeedWriteSize = 0;
    MI_S16 s16SrcUpDownStatus = 0;
    MI_U32 u32WriteSize = 0;
    MI_U32 u32EndCnt = 0;

    int fdWr;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stAoChn0OutputPort0;
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = pstAoOutChn->AoDevId;
    stAoChn0OutputPort0.u32ChnId = pstAoOutChn->AoChn;
    stAoChn0OutputPort0.u32PortId = 0;

    fdWr = pstAoOutChn->fdWr;

    /* get ao device*/
    ExecFunc(MI_AO_GetPubAttr(pstAoOutChn->AoDevId, &stGetAttr), MI_SUCCESS);
    u64NeedWriteSize = (pstAoOutChn->u32InputFileSize * (pstAoOutChn->u32OutSampleRate) + pstAoOutChn->u32InSampleRate -1) /(pstAoOutChn->u32InSampleRate);
    //printf("u64NeedWriteSize is : %d, \n",u64NeedWriteSize);

    // get data of sending by AO output port
    if(fdWr > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stAoChn0OutputPort0,&stBufInfo,&hHandle))
            {
                u32WriteSize = stBufInfo.stRawData.u32BufSize;

                write(fdWr,stBufInfo.stRawData.pVirAddr, u32WriteSize);
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
                if(u32EndCnt >=100000)
                    break;
            }
        }

    }
    else
    {
        printf("Open output file  fail: fdr %d \n", fdWr);
    }

    //printf("Ending,  pstAoOutChn->u32WriteTotalSize: %d, \n", pstAoOutChn->u32WriteTotalSize);

    return NULL;
}


int main(int argc, char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32RetSendStatus = 0;

    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_DEV AoDevId = 0;
    MI_AO_CHN AoChn;
    MI_AUDIO_SampleRate_e eInSampleRate;
    MI_AUDIO_SampleRate_e eOutSampleRate;

    MI_AO_VqeConfig_t stSetVqeConfig;
    MI_AO_VqeConfig_t stGetVqeConfig;

    MI_S32 s32Opt = 0;
	MI_BOOL bEnableRes = FALSE;
    MI_BOOL bEnableVqe = TRUE;
    MI_BOOL bVqeEqEnable = FALSE;
    MI_U32 u32IdxFreqEq = 0;
    MI_U32 u32IdxGainEq = 256;
    MI_AUDIO_HpfFreq_e eHpfFreq = E_MI_AUDIO_HPF_FREQ_120;

    MI_S32 s32Idx;

    // file read/write
    int fdRd =-1;
    int fdWr= -1;
    MI_U8 au8TempBuf[2048*2*2];
    MI_U8 au8LBuf[2048*2];
    MI_U8 au8RBuf[2048*2];
    MI_U8 au8LRBuf[2048*2*2];

    MI_S32 s32RdSize;
    MI_S8* ps8InputFilePath = NULL;
    MI_S8* ps8OutputFilePath = NULL;
    _WavHeader_t stWavHeaderInput;
    _WavHeader_t stWavHeaderOutput;

    // get output port buffer thread
    pthread_t tid;

    // parsing command line
    while ((s32Opt = getopt(argc, argv, "i:o:e:g:d:")) != -1)
    {
            switch(s32Opt)
            {
                case 'i':
                    ps8InputFilePath = optarg;
                    printf("Input file path: %s \n", ps8InputFilePath);
                    break;

                case 'o':
                    ps8OutputFilePath = optarg;
                    printf("Output file path: %s \n", ps8OutputFilePath);
                    break;

                case 'e':
                    bVqeEqEnable = TRUE;
                    u32IdxFreqEq = atoi(optarg);
                    printf("Enable EQ of VQE, Set i%th frequency of EQ is %d \n", u32IdxFreqEq);
                    break;

                case 'g':
                    u32IdxGainEq = atoi(optarg);
                    printf(" Set Gain of Eq:%d \n", u32IdxGainEq);
                    break;

                case 'd':
                    AoDevId = atoi(optarg);
                    printf("Enable MI_AO Device %d !\n", AoDevId);
                    break;

                case '?':
                    if(optopt == 'i')
                        printf("Missing input file path, please -i 'input file path' \n");
                    else if(optopt == 'o')
                        printf("Missing input file path, please -o 'output file path' \n");
                    else if(optopt =='e')
                        printf("Missing ith frequency of EQ setting, please -e 'IdxFreqEq:[0, 128]' \n");
                    else if(optopt == 'g')
                        printf("Missing idx gain of EQ, olease -g ''IdxGainEq:[1, 512] \n");
                    else if(optopt == 'd')
                        printf("Missing AO device ID, please -d 'AO Device ID' \n");
                    else
                        printf("Invalid option received\n");

                default:
                    _mi_ao_test_vqe_help();
                    return 1;
            }

    }

    if(ps8InputFilePath == NULL || ps8OutputFilePath == NULL)
    {
        _mi_ao_test_vqe_help();
        return 1;
    }

    /* open file  to read data and write data */
    fdRd = open(ps8InputFilePath, O_RDONLY, 0666);
    if(fdRd <= 0)
    {
        printf("Open input file path:%s fail \n", ps8InputFilePath);
        return 1;
    }

    fdWr = open(ps8OutputFilePath, O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fdWr < 0)
    {
        printf("Open output file path:%s fail \n", ps8OutputFilePath);
        if(fdRd>0)
            close(fdRd);
        return 1;
    }

    //MI_SYS init
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);

    // read input wav file
    s32RdSize = read(fdRd, &stWavHeaderInput, sizeof(_WavHeader_t));

    AoChn = 0;
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    stSetAttr.u32ChnCnt = stWavHeaderInput.channels;

    if(stSetAttr.u32ChnCnt == 2)
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    else if(stSetAttr.u32ChnCnt == 1)
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;

     stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;
     if(bEnableRes)
     {
        eInSampleRate = (MI_AUDIO_SampleRate_e)stWavHeaderInput.sample_rate;
        stSetAttr.eSamplerate = eOutSampleRate; // AO Device output sample rate
     }

    _gstAoOutchn.fdWr = fdWr;
    _gstAoOutchn.AoDevId = AoDevId;
    _gstAoOutchn.AoChn = AoChn;
    _gstAoOutchn.bEndOfStream = FALSE;
    _gstAoOutchn.u32WriteTotalSize = 0;
    _gstAoOutchn.u32InputFileSize = stWavHeaderInput.data_size;
    _gstAoOutchn.u32InSampleRate = stWavHeaderInput.sample_rate / 1000;
    _gstAoOutchn.u32OutSampleRate= ((MI_U32)stSetAttr.eSamplerate) /1000;

    /* set ao public attr*/
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stSetAttr), MI_SUCCESS);

    /* get ao device*/
    ExecFunc(MI_AO_GetPubAttr(AoDevId, &stGetAttr), MI_SUCCESS);

    /* enable ao device */
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);

    /* enable ao channel of device*/
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);


    /* if test VQE: set attribute of AO VQE  */
    if(bEnableVqe)
    {
        memset(&stSetVqeConfig, 0, sizeof(MI_AO_SetVqeAttr));
        memset(&stSetVqeConfig, 0, sizeof(stSetVqeConfig));
        stSetVqeConfig.bHpfOpen = FALSE;
        stSetVqeConfig.bAnrOpen = FALSE;
        stSetVqeConfig.bAgcOpen = FALSE;
        stSetVqeConfig.bEqOpen = bVqeEqEnable;

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

         // need to check algorithm configure
        ExecFunc(MI_AO_SetVqeAttr(AoDevId, AoChn, &stSetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_GetVqeAttr(AoDevId, AoChn, &stGetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_EnableVqe(AoDevId, AoChn), MI_SUCCESS);

    }

    // test get output port buffer
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stAoChn0OutputPort0;
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0,12,13), MI_SUCCESS);

    // write wav header to output file
    if((fdWr>0))
       write(fdWr, &stWavHeaderInput, sizeof(_WavHeader_t));


    if( (fdRd >0)  && (fdWr>0) )
    {
        //create a thread to get output buffer
        pthread_create(&tid, NULL, _mi_ao_GetOutPortBuf, &_gstAoOutchn);

        while( s32RdSize = read(fdRd, &au8TempBuf, MI_AUDIO_SAMPLE_PER_FRAME*2*(stSetAttr.u32ChnCnt) ) )
        {

            if(s32RdSize != MI_AUDIO_SAMPLE_PER_FRAME*2*(stSetAttr.u32ChnCnt))
            {
                memset(au8TempBuf+s32RdSize, 0,  MI_AUDIO_SAMPLE_PER_FRAME*2*(stSetAttr.u32ChnCnt) - s32RdSize);
                s32RdSize = MI_AUDIO_SAMPLE_PER_FRAME*2*(stSetAttr.u32ChnCnt);
            }

            //read data and send to AO module
            _gstAoTestFrame.eBitwidth = stGetAttr.eBitwidth;
            _gstAoTestFrame.eSoundmode = stGetAttr.eSoundmode;

            if(stGetAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
            {
                _gstAoTestFrame.u32Len = s32RdSize;
                _gstAoTestFrame.apVirAddr[0] = au8TempBuf;
                _gstAoTestFrame.apVirAddr[1] = NULL;
            }
            else  // stereo mode
            {
                _gstAoTestFrame.u32Len = s32RdSize;
                //_mi_ao_deinterleave(au8TempBuf, au8LBuf, au8RBuf, s32RdSize);
                _gstAoTestFrame.apVirAddr[0] = au8TempBuf;
                _gstAoTestFrame.apVirAddr[1] = NULL;
            }


            do{
                 s32RetSendStatus = MI_AO_SendFrame(AoDevId, AoChn, &_gstAoTestFrame, 1);
                 if (bEnableRes)
                 {
                     usleep( ((MI_AUDIO_SAMPLE_PER_FRAME * 1000) / eInSampleRate - 10) * 1000);
                 }
                 else
                 {
                    usleep( ((MI_AUDIO_SAMPLE_PER_FRAME * 1000) / stSetAttr.eSamplerate - 10) * 1000);
                 }

            }while(s32RetSendStatus == MI_AO_ERR_NOBUF);
            if(s32RetSendStatus != MI_SUCCESS)
                printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n",s32RetSendStatus);

            if(s32RdSize != MI_AUDIO_SAMPLE_PER_FRAME*2*(stSetAttr.u32ChnCnt))
                break;

        }

        _gstAoOutchn.bEndOfStream = TRUE;
        pthread_join(tid, NULL);
    }

    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0,0,10);


    /* Disable VQE */
    if(bEnableVqe)
    {
        ExecFunc(MI_AO_DisableVqe(AoDevId, AoChn), MI_SUCCESS);
    }

    /* disable ao channel of */
    ExecFunc(MI_AO_DisableChn(AoDevId, AoChn), MI_SUCCESS);

    /* disable ao device */
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);

    // update wav header of output file ;
    memcpy(&stWavHeaderOutput, &stWavHeaderInput, sizeof(_WavHeader_t));
    stWavHeaderOutput.sample_rate = stSetAttr.eSamplerate;
    stWavHeaderOutput.data_size = _gstAoOutchn.u32WriteTotalSize;
    stWavHeaderOutput.ChunkSize = stWavHeaderOutput.data_size + 36;
    lseek(fdWr, 0, SEEK_SET);
    write(fdWr, &stWavHeaderOutput, sizeof(_WavHeader_t));

    if(fdRd>0)
        close(fdRd);

    if(fdWr>0)
        close(fdWr);

    return 0;

}
