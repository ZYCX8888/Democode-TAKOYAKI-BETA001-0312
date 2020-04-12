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
#include <stdbool.h>

#include "mi_common_datatype.h"
#include "mi_uac.h"
#include "mi_ai.h"
#include "mi_ao.h"

#define ExecFunc(func, _ret_) \
    if (func != _ret_)\
    {\
        printf("AI_TEST [%d] %s exec function failed\n",__LINE__, #func);\
        return 1;\
    }\
    else\
    {\
        printf("AI_TEST [%d] %s  exec function pass\n", __LINE__, #func);\
    }

#define MI_AUDIO_SAMPLE_PER_FRAME 256
#define MI_AUDIO_AO_LINEOUT  0

void help(int argc,void **argv)
{
    putchar('\n');
    printf("this is uac test app: \n");
    printf("--->%s -f num \n",argv[0]);
    printf("----->num 1: test01_for_ai_uac \n");
    printf("----->num 2: test02_for_uac_ao \n");
    printf("----->num 3: test03_for_ai_uac_ao \n");
    printf("----->num 4: test04_for_ai_uac_sendframe \n");
}

void stop_help()
{
    printf("\n\nenter 'q' to exit \n");
}

bool g_ai_uac_capture_exit=false;

int ai_uac_sendframe_work()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1,UacDevId = UAC_CAPTURE_DEV;
    MI_AI_CHN AiChn = 0;
    MI_UAC_CHN UacChn   = 0;
    MI_AUDIO_Frame_t stAiFrm;
    MI_UAC_Frame_t dtUacFrm;

     while(false==g_ai_uac_capture_exit){
        MI_AI_GetFrame(AiDevId, AiChn, &stAiFrm, NULL, 16);//256 / 16000 = 16ms
        if (0 == stAiFrm.u32Len){
            continue;
        }
        dtUacFrm.u32Len = stAiFrm.u32Len;
        dtUacFrm.pu8Addr = stAiFrm.apVirAddr[0];
        dtUacFrm.u64PTS = stAiFrm.u64TimeStamp;
        MI_UAC_SendFrame(UacChn,&dtUacFrm,0);
        MI_AI_ReleaseFrame(AiDevId,  AiChn, &stAiFrm, NULL);
     }
}

int test04_for_ai_uac_sendframe(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1,UacDevId = UAC_CAPTURE_DEV;
    MI_AUDIO_Attr_t stAiAttr;
    MI_AI_CHN AiChn = 0;
    MI_U32 UacChn   = 0;
    MI_SYS_ChnPort_t stAiPort;
    MI_U32 u32FrameDepth = 12;
    MI_U32 u32BuffQueue = 13;
    pthread_t thread_work;

/*SYS Init*/
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);
    ExecFunc(MI_UAC_OpenDevice(UacDevId),MI_SUCCESS);
/*AI PARAM*/
    memset(&stAiAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAiAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAiAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAiAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAiAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAiAttr.u32ChnCnt = 2;
    stAiAttr.u32FrmNum = 16;
    stAiAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;

    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChn(AiDevId, AiChn), MI_SUCCESS);
/*AI Depth*/
    memset(&stAiPort, 0, sizeof(MI_SYS_ChnPort_t));
    stAiPort.eModId = E_MI_MODULE_ID_AI;
    stAiPort.u32ChnId = AiChn;
    stAiPort.u32DevId = AiDevId;
    stAiPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAiPort , u32FrameDepth, u32BuffQueue), MI_SUCCESS);

/* Get A Buf And Send To Uac Capture Device */
    ExecFunc(MI_UAC_StartDev(UacDevId),MI_SUCCESS);
    ExecFunc(pthread_create(&thread_work,NULL,ai_uac_sendframe_work,NULL),MI_SUCCESS);
    stop_help();
    while(getchar()!='q')
        usleep(1000);
    g_ai_uac_capture_exit = true;
    ExecFunc(pthread_join(thread_work,NULL),MI_SUCCESS);
    ExecFunc(MI_UAC_StopDev(UacDevId),MI_SUCCESS);
    ExecFunc(MI_UAC_CloseDevice(UacDevId),MI_SUCCESS);
    ExecFunc(MI_AI_DisableChn(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_SYS_Exit(),MI_SUCCESS);
    return MI_SUCCESS;
}

int test03_for_ai_uac_ao(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1,AoDevId=MI_AUDIO_AO_LINEOUT;
    MI_U32 UacDevId_C = UAC_CAPTURE_DEV,UacDevId_P = UAC_PLAYBACK_DEV;
    MI_AUDIO_Attr_t stAiAttr,stAoAttr;
    MI_AI_CHN AiChn = 0,AoChn = 0;
    MI_U32 u32FrameDepth = 12;
    MI_U32 u32BuffQueue = 13;
    MI_SYS_ChnPort_t stSrcChnPort, stDstChnPort;
    MI_U32 u32SrcFrmRate = 0;
    MI_U32 u32DstFrmRate = 0;
/*SYS Init*/
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);
    ExecFunc(MI_UAC_OpenDevice(UacDevId_C),MI_SUCCESS);
    ExecFunc(MI_UAC_OpenDevice(UacDevId_P),MI_SUCCESS);
/*AI Init Param & Set Depth & Bind*/
    memset(&stAiAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAiAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAiAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAiAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAiAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAiAttr.u32ChnCnt = 2;
    stAiAttr.u32FrmNum = 6;
    stAiAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_GetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChn(AiDevId, AiChn), MI_SUCCESS);
    /* Bind AI To UAC */
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId    = E_MI_MODULE_ID_AI;
    stSrcChnPort.u32ChnId  = AiChn;
    stSrcChnPort.u32DevId  = AiDevId;
    stSrcChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stSrcChnPort , u32FrameDepth, u32BuffQueue), MI_SUCCESS);

    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_UAC;
    stDstChnPort.u32DevId  = UacDevId_C;
    stDstChnPort.u32ChnId  = 0;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmRate, u32DstFrmRate), MI_SUCCESS);
/*AO Init Param & Set Depth & Bind*/
    stAoAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAoAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAoAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAoAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoAttr.u32ChnCnt = 1;
    stAoAttr.u32FrmNum = 6;
    stAoAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_GetPubAttr(AoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);
    /* Bind uac to ao */
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_UAC;
    stSrcChnPort.u32DevId = UacDevId_P;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stSrcChnPort, 3, 6), MI_SUCCESS);

    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_AO;
    stDstChnPort.u32ChnId = AoChn;
    stDstChnPort.u32DevId = AoDevId;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmRate, u32DstFrmRate), MI_SUCCESS);
/* Start Device */
    ExecFunc(MI_UAC_StartDev(UacDevId_C),MI_SUCCESS);
    ExecFunc(MI_UAC_StartDev(UacDevId_P),MI_SUCCESS);
    stop_help();
    while(getchar()!='q')
        sleep(1);
    ExecFunc(MI_UAC_StopDev(UacDevId_C),MI_SUCCESS);
    ExecFunc(MI_UAC_StopDev(UacDevId_P),MI_SUCCESS);
/* ubind port uac ai */
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId    = E_MI_MODULE_ID_AI;
    stSrcChnPort.u32DevId  = AiDevId;
    stSrcChnPort.u32ChnId  = AiChn;
    stSrcChnPort.u32PortId = 0;
    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_UAC;
    stDstChnPort.u32DevId  = UacDevId_C;
    stDstChnPort.u32ChnId  = 0;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    ExecFunc(MI_AI_DisableChn(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);
/* ubind port uac ao */
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_UAC;
    stSrcChnPort.u32DevId = UacDevId_P;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32PortId = 0;
    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_AO;
    stDstChnPort.u32DevId = AoDevId;
    stDstChnPort.u32ChnId = AoChn;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    ExecFunc(MI_AO_DisableChn(AoDevId, AoChn), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);

    ExecFunc(MI_UAC_CloseDevice(UacDevId_C),MI_SUCCESS);
    ExecFunc(MI_UAC_CloseDevice(UacDevId_P),MI_SUCCESS);
    ExecFunc(MI_SYS_Exit(),MI_SUCCESS);
    return MI_SUCCESS;
}

int test02_for_uac_ao(void)
{
    // for I2 AiDedId must 1, for k6 AiDevId must be 0
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AoDevId = MI_AUDIO_AO_LINEOUT,UacDevId = UAC_PLAYBACK_DEV;
    MI_AUDIO_Attr_t stAoAttr;
    MI_AO_CHN AoChn = 0,UacChn = 0;
    MI_SYS_ChnPort_t UacPort,stAoPort;
    MI_U32 u32FrameDepth = 12;
    MI_U32 u32BuffQueue = 13;
    MI_SYS_ChnPort_t stSrcChnPort, stDstChnPort;
    MI_U32 u32SrcFrmRate = 0;
    MI_U32 u32DstFrmRate = 0;

    UacPort.eModId    = E_MI_MODULE_ID_UAC;
    UacPort.u32ChnId  = UacChn;
    UacPort.u32DevId  = UacDevId;
    UacPort.u32PortId = 0;
/*SYS Init*/
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);
    ExecFunc(MI_UAC_OpenDevice(UacDevId),MI_SUCCESS);
/*AO PARAM*/
    //for k6 eSamplerate must be 48000
    stAoAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
//    stAoAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_8000;
    stAoAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAoAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAoAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoAttr.u32ChnCnt = 1;
    stAoAttr.u32FrmNum = 6;
    stAoAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_GetPubAttr(AoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);
/*AO Depth*/
    memset(&stAoPort, 0, sizeof(MI_SYS_ChnPort_t));
    stAoPort.eModId = E_MI_MODULE_ID_AO;
    stAoPort.u32ChnId = AoChn;
    stAoPort.u32DevId = AoDevId;
    stAoPort.u32PortId = 0;
    //bind uac to ao
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_UAC;
    stSrcChnPort.u32ChnId = UacChn;
    stSrcChnPort.u32DevId = UacDevId;
    stSrcChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&UacPort, 3, 6), MI_SUCCESS);

    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_AO;
    stDstChnPort.u32ChnId = AoChn;
    stDstChnPort.u32DevId = AoDevId;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmRate, u32DstFrmRate), MI_SUCCESS);
    ExecFunc(MI_UAC_StartDev(UacDevId),MI_SUCCESS);
    stop_help();
    while((getchar()!='q'))
        sleep(1);
    ExecFunc(MI_UAC_StopDev(UacDevId),MI_SUCCESS);
    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    ExecFunc(MI_AO_DisableChn(AoDevId, AoChn), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);
    ExecFunc(MI_UAC_CloseDevice(UacDevId),MI_SUCCESS);
    ExecFunc(MI_SYS_Exit(),MI_SUCCESS);
    return MI_SUCCESS;
}

int test01_for_ai_uac(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId = 1,UacDevId = UAC_CAPTURE_DEV;
    MI_AUDIO_Attr_t stAiAttr;
    MI_AI_CHN AiChn = 0;
    MI_U32 UacChn   = 0;
    MI_SYS_ChnPort_t stAiPort;
    MI_U32 u32FrameDepth = 12;
    MI_U32 u32BuffQueue = 13;
    MI_SYS_ChnPort_t stSrcChnPort, stDstChnPort;
    MI_U32 u32SrcFrmRate = 0;
    MI_U32 u32DstFrmRate = 0;
/*SYS Init*/
    ExecFunc(MI_SYS_Init(),MI_SUCCESS);
    ExecFunc(MI_UAC_OpenDevice(UacDevId),MI_SUCCESS);
/*AI PARAM*/
    memset(&stAiAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAiAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAiAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAiAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAiAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAiAttr.u32ChnCnt = 2;
    stAiAttr.u32FrmNum = 6;
    stAiAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_GetPubAttr(AiDevId, &stAiAttr), MI_SUCCESS);
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChn(AiDevId, AiChn), MI_SUCCESS);
/*AI Depth*/
    memset(&stAiPort, 0, sizeof(MI_SYS_ChnPort_t));
    stAiPort.eModId = E_MI_MODULE_ID_AI;
    stAiPort.u32ChnId = AiChn;
    stAiPort.u32DevId = AiDevId;
    stAiPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAiPort , u32FrameDepth, u32BuffQueue), MI_SUCCESS);
/* Bind */
    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId    = E_MI_MODULE_ID_AI;
    stSrcChnPort.u32ChnId  = AiChn;
    stSrcChnPort.u32DevId  = AiDevId;
    stSrcChnPort.u32PortId = 0;

    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_UAC;
    stDstChnPort.u32ChnId  = UacChn;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmRate, u32DstFrmRate), MI_SUCCESS);
    ExecFunc(MI_UAC_StartDev(UacDevId),MI_SUCCESS);
    stop_help();
    while(getchar()!='q')
        sleep(1);
    ExecFunc(MI_UAC_StopDev(UacDevId),MI_SUCCESS);
    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    ExecFunc(MI_AI_DisableChn(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_UAC_CloseDevice(UacDevId),MI_SUCCESS);
    ExecFunc(MI_SYS_Exit(),MI_SUCCESS);
    return MI_SUCCESS;
}

int main(int argc,void **argv)
{
    int ch = 0,ret = 6;
    signal(SIGINT,stop_help);
    while(-1 != (ch=getopt(argc,(char**)argv,"f:")) ){
        switch(ch){
        case 'f':
            if( 1 == atoi(optarg))
               ret = test01_for_ai_uac();
            else if( 2 == atoi(optarg))
               ret = test02_for_uac_ao();
            else if( 3 == atoi(optarg))
               ret = test03_for_ai_uac_ao();
            else if( 4 == atoi(optarg))
               ret = test04_for_ai_uac_sendframe();
            break;
        default:
            break;
        }
    }
    if(6 == ret)
        help(argc,argv);
}
