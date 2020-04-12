/*
* module_audio.cpp- Sigmastar
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
#include "module_common.h"
#include "module_config.h"
#include "mid_AudioEncoder.h"
#include "mi_ai.h"
#include "mid_common.h"
#include "PacketModule.h"

//PacketModule *SysPacketModule = NULL;

extern void StopPlayMedia();
extern void startPlayMedia(MixerAudioOutParam * pstAudioOutParam);

MixerAudioOutParam g_stAudioOutParam[MIXER_AO_MAX_NUMBER];


int audioOpen(MixerAudioInParam *pstAudioInParam, MI_S32 audioNumber)
{
    if (MI_AudioEncoder::g_audioEncoderNumber > 0)
    {
        MIXER_ERR("MI_AudioEncoder::g_audioEncoderNumber=%d\n", MI_AudioEncoder::g_audioEncoderNumber);
        return 0;
    }

    if(NULL == pstAudioInParam)
    {
        printf("%s input mixer param is NULL!\n", __FUNCTION__);
        return 0;
    }

    if(MI_AudioEncoder::g_s32AudioInNum > MIXER_AI_MAX_NUMBER)
    {
        return 0;
    }

    for(MI_U32 i = 0 ; i < MIXER_AI_MAX_NUMBER; i++)
    {
        MI_AudioEncoder::g_pAudioEncoderArray[i] = NULL;
    }
/*
    for(MI_U32 i = 0 ; i < MI_AudioEncoder::g_s32AudioInNum; i++)
    {
        MI_AI_Disable((MI_AUDIO_DEV)pstAudioInParam->stAudioInChnPort.u32DevId);
    }
*/
    /*SysPacketModule = PacketModule::getInstance();
    if(NULL == SysPacketModule)
    {
        MIXER_ERR("packet module init err\n");
        return -1;
    }

    SysPacketModule->init();
    SysPacketModule->start();
*/
    MI_AudioEncoder::g_audio_exit = FALSE;
    MI_AudioEncoder::g_audio_thread_exit = 0;


    printf("g_s32AudioInNum=%d\n", MI_AudioEncoder::g_s32AudioInNum);

    if(0 < MI_AudioEncoder::g_s32AudioInNum)
    {
        for(MI_U32 i = 0; i < MI_AudioEncoder::g_s32AudioInNum; i++)
        {
            MI_AudioEncoder::g_pAudioEncoderArray[i] = MI_AudioEncoder::createNew(i);
            MI_AudioEncoder::g_pAudioEncoderArray[i]->initAudioEncoder(pstAudioInParam + i, i);
            MI_AudioEncoder::g_pAudioEncoderArray[i]->startAudioEncoder(i);
        }
    }

    return 0;
}

int audioClose(MI_S32 audioNumber)
{
    if (MI_AudioEncoder::g_audioEncoderNumber <= 0)
    {
        printf("MI_AudioEncoder::g_audioEncoderNumber=%d\n", MI_AudioEncoder::g_audioEncoderNumber);
        return 0;
    }

    MI_AudioEncoder::stopAudioEncoder();
    MI_AudioEncoder::uninitAudioEncoder();

    for(int i = 0 ; i < audioNumber; i++)
    {
        delete MI_AudioEncoder::g_pAudioEncoderArray[i];
        MI_AudioEncoder::g_pAudioEncoderArray[i] = NULL;
    }

   /* if(SysPacketModule)
    {
        SysPacketModule->stop();
        SysPacketModule->unInit();
        SysPacketModule = NULL;
    }*/

    return 0;
}


int audio_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    switch(id)
    {
        case CMD_AUDIO_OPEN:
            {
                MixerAudioInParam *pstAudioInParam = (MixerAudioInParam *)param;
                MI_AudioEncoder::g_s32AudioInNum  = pstAudioInParam->u8AudioInNum;
                audioOpen(pstAudioInParam, MI_AudioEncoder::g_s32AudioInNum);
            }
            break;

        case CMD_AUDIO_CLOSE:
            audioClose(MI_AudioEncoder::g_s32AudioInNum);
            break;

        case CMD_AUDIO_PLAY_MEDIA:
            {
                memset(g_stAudioOutParam, 0x00, sizeof(g_stAudioOutParam) * MIXER_AO_MAX_NUMBER);
                memcpy(g_stAudioOutParam, param, paramLen);
                startPlayMedia(g_stAudioOutParam);
            }
            break;

        case CMD_AUDIO_STOPPLAY:
            {
                StopPlayMedia();
                break;
            }

#if MIXER_AED_ENABLE
        case CMD_AUDIO_SET_AED_SENSITIVITY:
            {
                AedSetSensitivity((AedSensitivity)*param);
            }
            break;

        case CMD_AUDIO_SET_AED_OPERATIONGPOINT:
            {
                S32 point ;
                memcpy(&point, param, sizeof(point));
                AedSetOperatingPoint(point);
            }
            break;
#endif

        case CMD_AUDIO_SET_SAMPLES:
        case CMD_AUDIO_SET_BITWIDE:
        case CMD_AUDIO_SET_ENCODER:
        case CMD_AUDIO_GET_SAMPLES:
        case CMD_AUDIO_GET_BITWIDE:
        case CMD_AUDIO_GET_ENCODER:
            break;
        case CMD_AUDIO_SET_AIVOLUME:
            {
                MI_S32 vol;
                MI_S32 s32AudioInIdx;
                MI_S32 Audioparam[2];
                memset(Audioparam,0, sizeof(Audioparam));
                paramLen = (paramLen > sizeof(Audioparam) ? sizeof(Audioparam):paramLen);
                memcpy(Audioparam, param, paramLen);
                s32AudioInIdx = Audioparam[0];
                vol = Audioparam[1];
                SetAIVolume(s32AudioInIdx, vol);
            }
            break;

        case CMD_AUDIO_SET_AOVOLUME:
            {
                MI_S32 vol;
                MI_S32 s32AudioInIdx;
                MI_S32 Audioparam[2];
                memset(Audioparam, 0, sizeof(Audioparam));
                paramLen = (paramLen > sizeof(Audioparam) ? sizeof(Audioparam):paramLen);
                memcpy(Audioparam, param, paramLen);
                s32AudioInIdx = Audioparam[0];
                vol = Audioparam[1];
                SetAOVolume(s32AudioInIdx,vol);
            }
            break;
        case CDM_AUDIO_SETVQEMODULE:
            {
                MI_S8 s8AudioVqeData[AUDIO_VQE_SIZE_MAX + 10];
				MI_S8 s8Module = param[0];
                memset(s8AudioVqeData, 0, sizeof(s8AudioVqeData));
                paramLen = (paramLen - 1 > sizeof(s8AudioVqeData) ? sizeof(s8AudioVqeData) : paramLen - 1);
                memcpy(s8AudioVqeData, param + 1, paramLen);
                SetVQEStatus(s8Module, s8AudioVqeData,paramLen);
            }
            break;
        default:
            break;
    }

    return 0;
}
