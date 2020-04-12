#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "mi_hdmi.h"

static MI_S32 _gs32HdmiRunnig = FALSE;

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("Test [%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("Test [%d]exec function pass\n", __LINE__);\
    }

static MI_S32 Hdmi_callback_impl(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_EventType_e Event, void *pEventParam, void *pUsrParam)
{
    switch (Event)
    {
        case E_MI_HDMI_EVENT_HOTPLUG:
            printf("E_MI_HDMI_EVENT_HOTPLUG.\n");
            break;
        case E_MI_HDMI_EVENT_NO_PLUG:
            printf("E_MI_HDMI_EVENT_NO_PLUG.\n");
            break;
        default:
            printf("Unsupport event.\n");
            break;
    }

    return MI_SUCCESS;
}

int main(int argc, const char *argv[])
{
    MI_S32 s32ret = -1, i = 0;
    MI_U8 u8Ch;
    MI_HDMI_InitParam_t stInitParam;
    MI_HDMI_Attr_t stAttr;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;

    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = Hdmi_callback_impl;

    ExecFunc(MI_HDMI_Init(&stInitParam), MI_SUCCESS);

    ExecFunc(MI_HDMI_Open(eHdmi), MI_SUCCESS);


    _gs32HdmiRunnig = TRUE;

    while (_gs32HdmiRunnig)
    {
        u8Ch = getchar();
        switch (u8Ch)
        {
            case '1'://1080p
            {
                memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
                stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
                stAttr.stAudioAttr.bEnableAudio = TRUE;
                stAttr.stAudioAttr.bIsMultiChannel = 0;
                stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
                stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
                stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
                stAttr.stVideoAttr.bEnableVideo = TRUE;
                stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
                stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
                stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
                stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
                ExecFunc(MI_HDMI_SetAttr(eHdmi, &stAttr), MI_SUCCESS);

                ExecFunc(MI_HDMI_Start(eHdmi), MI_SUCCESS);
                continue;
            }
            case '7'://720p
            {
                memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
                stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
                stAttr.stAudioAttr.bEnableAudio = TRUE;
                stAttr.stAudioAttr.bIsMultiChannel = 0;
                stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
                stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
                stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
                stAttr.stVideoAttr.bEnableVideo = TRUE;
                stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
                stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
                stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_720_60P;
                stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
                ExecFunc(MI_HDMI_SetAttr(eHdmi, &stAttr), MI_SUCCESS);

                ExecFunc(MI_HDMI_Start(eHdmi), MI_SUCCESS);
                continue;
            }
            case 'r': //reset timing
                s32ret = MI_HDMI_SetAvMute(eHdmi, TRUE);
                s32ret = MI_HDMI_GetAttr(eHdmi, &stAttr);
                stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
                stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
                stAttr.stAudioAttr.bEnableAudio = TRUE;
                stAttr.stAudioAttr.bIsMultiChannel = 0;
                stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
                stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
                stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
                stAttr.stVideoAttr.bEnableVideo = TRUE;
                stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
                stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
                stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
                stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;//1080P60 -> 720P60
                s32ret = MI_HDMI_SetAttr(eHdmi, &stAttr);
                s32ret = MI_HDMI_SetAvMute(eHdmi, FALSE);
                continue;
            case '2': //get edid
            {
                MI_HDMI_Edid_t stEdid;
                s32ret = MI_HDMI_ForceGetEdid(eHdmi, &stEdid);
                for (i = 0; i < stEdid.u32Edidlength; i++)
                {
                    printf("[%x] ", stEdid.au8Edid[i]);
                }
                printf("\n");
                continue;
            }
            case '3': //get sink info
            {
                MI_HDMI_SinkInfo_t stSinkInfo;
                s32ret = MI_HDMI_GetSinkInfo(eHdmi, &stSinkInfo);
                if (MI_ERR_HDMI_EDID_PRASE_ERR == s32ret)
                {
                    printf("MI_ERR_HDMI_EDID_PRASE_ERR....]]]\n");
                }
                continue;
            }
            case 'q':
                ExecFunc(MI_HDMI_Stop(eHdmi), MI_SUCCESS);
                ExecFunc(MI_HDMI_Close(eHdmi), MI_SUCCESS);
                ExecFunc(MI_HDMI_DeInit(), MI_SUCCESS);
                exit(0);
            default:
                break;
        }
    }

    return 0;
}
