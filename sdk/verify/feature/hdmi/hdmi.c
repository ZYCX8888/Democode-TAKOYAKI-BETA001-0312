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
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "mi_sys.h"
#include "mi_hdmi.h"
#include "mi_disp.h"
#include "mi_ai.h"
#include "mi_ao.h"

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)

#define DISP_INPUT_PORT_MAX 16
#define DISP_LAYER_MAX 2
#define DISP_DEV_MAX 2

typedef struct _arg {
    bool Inited;
    bool hdmi_enable;
    bool audio_enable;
    bool disp_enable;
} Arg_t;

typedef struct _HdmiAttr {
    MI_HDMI_DeviceId_e eHdmi;
    MI_HDMI_Attr_t stAttr;
    int inWidth, inHeight;
    char AudioPath[50];
    char VideoPath[50];
} HdmiAttr_t;

typedef struct stDispUtPort_s
{
    MI_BOOL bPortEnable;
    MI_DISP_LAYER DispLayer;
    MI_U32 u32PortId;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_SYS_ChnPort_t stSysChnPort;
    char FilePath[50];
    pthread_t task;
} StDispUtPort_t;

typedef struct _DispAttr {
    MI_DISP_DEV DispDev;
    MI_DISP_LAYER DispLayer;
    MI_U8 u8PortNum;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    StDispUtPort_t * stInPortAttr;
} DispAttr_t;

typedef struct _AudioAttr {
    MI_BOOL bDevEnable;
    MI_AUDIO_DEV aoDevId;
    MI_AO_CHN aoChn;
    MI_AUDIO_Attr_t stAoAttr;
    char FilePath[50];
    pthread_t task;
} AudioAttr_t;

typedef struct stTimingArray_s
{
    char desc[50];
    MI_DISP_OutputTiming_e eOutputTiming;
    MI_HDMI_TimingType_e eHdmiTiming;
    MI_U16 u16Width;
    MI_U16 u16Height;
} TimingArray_t;

TimingArray_t astTimingArray[] = {
    {
        .desc = "user",
        .eOutputTiming = E_MI_DISP_OUTPUT_USER,
        .eHdmiTiming = E_MI_HDMI_TIMING_MAX,
    },
    {
        .desc = "480p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_480P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_480_60P,
        .u16Width = 640,.u16Height = 480
    },
    {
        .desc = "576p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_576P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_576_50P,
        .u16Width = 720,.u16Height = 576
    },
    {
        .desc = "720p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_720P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_720_50P,
            .u16Width = 1280,.u16Height = 720
    },
    {
        .desc = "720p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_720P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_720_60P,
            .u16Width = 1280,.u16Height = 720
    },
    {
        .desc = "1024x768_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1024x768_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1024x768_60P,
            .u16Width = 1024,.u16Height = 768
    },
    {
        .desc = "1080p24",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P24,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_24P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p25",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P25,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_25P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p30",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P30,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_30P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_50P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_60P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1280x800_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1280x800_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1280x800_60P,
            .u16Width = 1280,.u16Height = 800
    },
    {
        .desc = "1280x1024_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1280x1024_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1280x1024_60P,
            .u16Width = 1280,.u16Height = 1024
    },
    {
        .desc = "1366x768_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1366x768_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1366x768_60P,//HDMI don't support this timing
            .u16Width = 1366,.u16Height = 768
    },
    {
        .desc = "1440x900_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1440x900_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1440x900_60P,
            .u16Width = 1440,.u16Height = 900
    },
    {
        .desc = "1680x1050_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1680x1050_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1680x1050_60P,//HDMI don't support this timing
            .u16Width = 1680,.u16Height = 1050
    },
    {
        .desc = "1600x1200_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1600x1200_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1600x1200_60P,
            .u16Width = 1600,.u16Height = 1200
    },
    {
        .desc = "2560x1440_30",
        .eOutputTiming = E_MI_DISP_OUTPUT_2560x1440_30,
         .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1440
    },
    {
        .desc = "2560x1440_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_MAX,//not defined
         .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1440
    },
    {
        .desc = "2560x1600_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_2560x1600_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1600
    },
    {
        .desc = "3840x2160_30",
        .eOutputTiming = E_MI_DISP_OUTPUT_3840x2160_30,
        .eHdmiTiming = E_MI_HDMI_TIMING_4K2K_30P,
            .u16Width = 3840,.u16Height = 2160
    },
    {
        .desc = "3840x2160_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_3840x2160_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_4K2K_60P,
            .u16Width = 3840,.u16Height = 2160
    },
};

static Arg_t _gArg = {false, false, false, false};
static HdmiAttr_t g_HdmiAttr;
static DispAttr_t g_DispAttr;
static AudioAttr_t g_AudioAttr;

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("Test [%d] exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("Test [%d] exec function pass\n", __LINE__);\
    }

int hdmi_setAnalogDrvCur();
int hdmi_setInfoframe();
int hdmi_stop();
int disp_stop();
int audio_stop();
static MI_U32 disp_getuserportsetting(MI_DISP_LAYER DispLayer,
    MI_U32 u32LayerWidth, MI_U32 u32LayerHeight, MI_U8 u8PortNum ,char *filepath);

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

static int Hdmi_get_filePath(char *Path)
{
    char filePath[100];
    int fd;

    while(1)
    {
        printf(" >>> Enter A File Path: ");
        scanf("%s", filePath);

        fd = open(filePath, O_DIRECTORY);
        if (fd!=-1)
        {
            printf("Is A Dir\n");
            close(fd);
            continue;
        }

        fd = open(filePath, O_RDONLY);
        if(fd==-1)
        {
            printf("Invaild Path\n");
            continue;
        }
        break;
    }
    close(fd);
    strcpy(Path, filePath);
    printf("\n");
    return 0;
}

static MI_HDMI_BitDepth_e Hdmi_get_bitwidth()
{
    char u8Ch[10];

    printf("\n=======================================\n");
    printf(">>>>>> Test Hdmi BitDepth Select <<<<<<\n\n");
    printf("1: E_MI_HDMI_BIT_DEPTH_16\n");
    printf("2: E_MI_HDMI_BIT_DEPTH_24\n");

    printf("\n>>>>> Select A Hdmi BitDepth: ");
    while(1)
    {
        scanf("%s", u8Ch);
        switch(u8Ch[0])
        {
        case '1':
            return E_MI_HDMI_BIT_DEPTH_16;
        case '2':
           return  E_MI_HDMI_BIT_DEPTH_24;
           continue;
        default:
            continue;
        }
    }
    printf("\n");
}

static MI_HDMI_SampleRate_e Hdmi_get_samplerate()
{
    char u8Ch[10];

    printf("\n=======================================\n");
    printf(">>>>> Test Hdmi SampleRate Select <<<<<\n\n");
    printf("1: E_MI_HDMI_AUDIO_SAMPLERATE_32K\n");
    printf("2: E_MI_HDMI_AUDIO_SAMPLERATE_48K\n");

    printf("\n>>>> Select A Hdmi SampleRate: ");
    while(1)
    {
        scanf("%s", u8Ch);
        switch(u8Ch[0])
        {
        case '1':
            return E_MI_HDMI_AUDIO_SAMPLERATE_32K;
        case '2':
           return  E_MI_HDMI_AUDIO_SAMPLERATE_48K;
        default:
            continue;
        }
    }
    printf("\n");
}

static MI_HDMI_ColorType_e Hdmi_get_colortype()
{
    char u8Ch[10];

    printf("\n=======================================\n");
    printf(">>>>> Test Hdmi ColorType Select <<<<<<\n\n");
    printf("1: E_MI_HDMI_COLOR_TYPE_RGB444\n");
    printf("2: E_MI_HDMI_COLOR_TYPE_YCBCR422\n");
    printf("3: E_MI_HDMI_COLOR_TYPE_YCBCR444\n");
    printf("4: E_MI_HDMI_COLOR_TYPE_YCBCR420\n");
    printf("\n>>>> Select A Hdmi ColorType: ");
    while(1)
    {
        scanf("%s", u8Ch);
        switch(u8Ch[0])
        {
        case '1':
            return E_MI_HDMI_COLOR_TYPE_RGB444;
        case '2':
            return E_MI_HDMI_COLOR_TYPE_YCBCR422;
        case '3':
            return E_MI_HDMI_COLOR_TYPE_YCBCR444;
        case '4':
            return E_MI_HDMI_COLOR_TYPE_YCBCR420;
        default:
            continue;
        }
    }
    printf("\n");
}

static MI_HDMI_TimingType_e Hdmi_get_timming()
{
    char u8Ch[10];
    const char *nptr = u8Ch;

    printf("\n=======================================\n");
    printf(">>>>>> Test Hdmi Timming Select <<<<<<<\n\n");
    printf("1: E_MI_HDMI_TIMING_1080_24P\n");
    printf("2: E_MI_HDMI_TIMING_1080_25P\n");
    printf("3: E_MI_HDMI_TIMING_1080_30P\n");
    printf("4: E_MI_HDMI_TIMING_720_50P\n");
    printf("5: E_MI_HDMI_TIMING_720_60P\n");
    printf("6: E_MI_HDMI_TIMING_1080_50P\n");
    printf("7: E_MI_HDMI_TIMING_1080_60P\n");
    printf("8: E_MI_HDMI_TIMING_576_50P\n");
    printf("9: E_MI_HDMI_TIMING_480_60P\n");
    printf("10: E_MI_HDMI_TIMING_1024x768_60P\n");
    printf("11: E_MI_HDMI_TIMING_1280x1024_60P\n");
    printf("12: E_MI_HDMI_TIMING_1366x768_60P\n");
    printf("13: E_MI_HDMI_TIMING_1440x900_60P\n");
    printf("14: E_MI_HDMI_TIMING_1280x800_60P\n");
    printf("15: E_MI_HDMI_TIMING_1680x1050_60P\n");
    printf("16: E_MI_HDMI_TIMING_1600x1200_60P\n");
    printf("\n>>>> Select A Hdmi Timming: ");
    while(1)
    {
        scanf("%s", u8Ch);
        switch(atoi(nptr))
        {
        case 1:
            return E_MI_HDMI_TIMING_1080_24P;
        case 2:
            return E_MI_HDMI_TIMING_1080_25P;
        case 3:
            return E_MI_HDMI_TIMING_1080_30P;
        case 4:
            return E_MI_HDMI_TIMING_720_50P;
        case 5:
            return E_MI_HDMI_TIMING_720_60P;
        case 6:
            return E_MI_HDMI_TIMING_1080_50P;
        case 7:
            return E_MI_HDMI_TIMING_1080_60P;
        case 8:
            return E_MI_HDMI_TIMING_576_50P;
        case 9:
            return E_MI_HDMI_TIMING_480_60P;
        case 10:
            return E_MI_HDMI_TIMING_1024x768_60P;
        case 11:
            return E_MI_HDMI_TIMING_1280x1024_60P;
        case 12:
            return E_MI_HDMI_TIMING_1366x768_60P;
        case 13:
            return E_MI_HDMI_TIMING_1440x900_60P;
        case 14:
            return E_MI_HDMI_TIMING_1280x800_60P;
        case 15:
            return E_MI_HDMI_TIMING_1680x1050_60P;
        case 16:
            return E_MI_HDMI_TIMING_1600x1200_60P;
        default:
            if (atoi(nptr) >= E_MI_HDMI_TIMING_MAX)
                continue;
            else
                return atoi(nptr);
        }
    }
    printf("\n");
}

static int HdmiTim_to_Format(MI_HDMI_TimingType_e timming, int *width, int *height)
{
    assert(height);
    assert(width);

    TimingArray_t *Array = NULL;
    int i, count = sizeof(astTimingArray)/sizeof(TimingArray_t);
    for(i = 0; i < count; i++)
    {
        Array = &astTimingArray[i];
        if (timming == Array->eHdmiTiming)
        {
            *width = Array->u16Width;
            *height = Array->u16Height;
            break;
        }
    }

    if(i == count)
    {
       printf("Hdmi No Support Such Format：%d\n", timming);
       return -EINVAL;
    }
    return 0;

}

static MI_AUDIO_BitWidth_e HdmiBitDepth_to_AudioBitwidth(MI_HDMI_BitDepth_e eBitDepth)
{
    switch(eBitDepth)
    {
    case E_MI_HDMI_BIT_DEPTH_16:
        return E_MI_AUDIO_BIT_WIDTH_16;
    case E_MI_HDMI_BIT_DEPTH_24:
        return E_MI_AUDIO_BIT_WIDTH_24;
    default:
        printf("Audio No Support Such BitDepth: %d\n", eBitDepth);
        return E_MI_AUDIO_BIT_WIDTH_MAX;
    }
}

static MI_AUDIO_SampleRate_e HdmiSampleRate_to_AudioSampleRate(MI_HDMI_SampleRate_e eRate)
{
   switch(eRate)
   {
   case  E_MI_HDMI_AUDIO_SAMPLERATE_32K:
       return E_MI_AUDIO_SAMPLE_RATE_32000;
   case E_MI_HDMI_AUDIO_SAMPLERATE_48K:
       return E_MI_AUDIO_SAMPLE_RATE_48000;
   default:
       printf("Audio No Support Such Sample Rate: %d\n", eRate);
       return E_MI_HDMI_AUDIO_SAMPLERATE_UNKNOWN;
   }
}

static MI_DISP_OutputTiming_e HdmiTim_to_DispTim(MI_HDMI_TimingType_e eTimingType)
{
    TimingArray_t *Array = NULL;
    int i, count = sizeof(astTimingArray)/sizeof(TimingArray_t);
    for(i = 0; i < count; i++)
    {
        Array = &astTimingArray[i];
        if (eTimingType == Array->eHdmiTiming)
        {
            return Array->eOutputTiming;
        }
    }

    return E_MI_DISP_OUTPUT_MAX;
}

int param_deinit()
{
    free(g_DispAttr.stInPortAttr);
}

/* control:
 * 1: init
 * 2: change timming
 * 3: change audio rate
 * 4: change banwidth
 * */
int param_init(int control)
{
    /* Hdmi Param Init*/
    MI_HDMI_Attr_t stAttr;

    if (control==1)
        memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
    else
        stAttr = g_HdmiAttr.stAttr;

    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = true;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = true;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = true;
    stAttr.stAudioAttr.bEnableAudio = true;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;

    switch (control)
    {
        case 1:
            stAttr.stAudioAttr.eBitDepth = Hdmi_get_bitwidth();
            stAttr.stAudioAttr.eSampleRate = Hdmi_get_samplerate();
            printf("audio pcm file:\n");
            Hdmi_get_filePath(g_HdmiAttr.AudioPath);
            stAttr.stVideoAttr.eTimingType = Hdmi_get_timming();
            HdmiTim_to_Format(stAttr.stVideoAttr.eTimingType,
                   &g_HdmiAttr.inWidth, &g_HdmiAttr.inHeight);
            stAttr.stVideoAttr.eColorType = Hdmi_get_colortype();
            printf("Src Inwidth %d inHeight%d \n", g_HdmiAttr.inWidth,  g_HdmiAttr.inHeight);
            printf("video src file:\n");
            Hdmi_get_filePath(g_HdmiAttr.VideoPath);
            break;
        case 2:
            stAttr.stVideoAttr.eTimingType = Hdmi_get_timming();
            break;
        case 3:
            stAttr.stAudioAttr.eBitDepth = Hdmi_get_bitwidth();
            printf("audio pcm file:\n");
            Hdmi_get_filePath(g_HdmiAttr.AudioPath);
            break;
        case 4:
            stAttr.stAudioAttr.eSampleRate = Hdmi_get_samplerate();
            printf("audio pcm file:\n");
            Hdmi_get_filePath(g_HdmiAttr.AudioPath);
            break;
        default:
            break;
    }
    stAttr.stVideoAttr.bEnableVideo = true;
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_30BIT;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    g_HdmiAttr.stAttr = stAttr;

    /* Disp Param Init */
    MI_U8 u8PortNum = 1;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_U32 u32LayerWidth, u32LayerHeight;
    MI_SYS_PixelFormat_e ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    MI_DISP_OutputTiming_e eOutputTiming = E_MI_DISP_OUTPUT_MAX;
    StDispUtPort_t * stInPortAttr;
    char Path[50];

    strcpy(Path, g_HdmiAttr.VideoPath);
    HdmiTim_to_Format(g_HdmiAttr.stAttr.stVideoAttr.eTimingType,
                   &u32LayerWidth, &u32LayerHeight);
    eOutputTiming =
            HdmiTim_to_DispTim(g_HdmiAttr.stAttr.stVideoAttr.eTimingType);
    stInPortAttr = calloc(u8PortNum, sizeof(StDispUtPort_t));
    g_DispAttr.stInPortAttr = stInPortAttr;
    disp_getuserportsetting(DispLayer, u32LayerWidth, u32LayerHeight, u8PortNum, Path);

    memset(&stPubAttr, 0x00, sizeof(MI_DISP_PubAttr_t));
    stPubAttr.u32BgColor = YUYV_BLACK;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    stPubAttr.eIntfSync = eOutputTiming;
    memset(&stLayerAttr, 0x00, sizeof(MI_DISP_VideoLayerAttr_t));
    stLayerAttr.ePixFormat = ePixelFormat;
    //input disp width and height
    stLayerAttr.stVidLayerSize.u16Width = u32LayerWidth;
    stLayerAttr.stVidLayerSize.u16Height = u32LayerHeight;
    stLayerAttr.stVidLayerDispWin.u16X = 0;
    stLayerAttr.stVidLayerDispWin.u16Y = 0;

    //final disp width and height
    stLayerAttr.stVidLayerDispWin.u16Width = u32LayerWidth;
    stLayerAttr.stVidLayerDispWin.u16Height = u32LayerHeight;

    g_DispAttr.DispDev = DispDev;
    g_DispAttr.DispLayer = DispLayer;
    g_DispAttr.u8PortNum = u8PortNum;
    g_DispAttr.stPubAttr = stPubAttr;
    g_DispAttr.stLayerAttr = stLayerAttr;

    /* Audio Param Init */
    MI_U32 sample_per_frame = 4096;
    MI_AUDIO_Attr_t stAoAttr;
    MI_AUDIO_DEV aoDevId = 2;
    MI_AO_CHN aoChn = 0;

    stAoAttr.eBitwidth =
        HdmiBitDepth_to_AudioBitwidth(g_HdmiAttr.stAttr.stAudioAttr.eBitDepth);
    stAoAttr.eSamplerate =
        HdmiSampleRate_to_AudioSampleRate(g_HdmiAttr.stAttr.stAudioAttr.eSampleRate);
    stAoAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAoAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoAttr.u32ChnCnt = 1;
    stAoAttr.u32FrmNum = 6;
    stAoAttr.u32PtNumPerFrm = sample_per_frame;

    strcpy(g_AudioAttr.FilePath, g_HdmiAttr.AudioPath);
    g_AudioAttr.stAoAttr = stAoAttr;
    g_AudioAttr.aoDevId = aoDevId;
    g_AudioAttr.aoChn = aoChn;
}

int hdmi_start()
{
    if(_gArg.hdmi_enable)
    {
        ExecFunc(hdmi_stop(), MI_SUCCESS);
    }
    printf("\nEnter Hdmi Start\n");

    MI_HDMI_DeviceId_e eHdmi = g_HdmiAttr.eHdmi;
    MI_HDMI_Attr_t stAttr = g_HdmiAttr.stAttr;

    ExecFunc(MI_HDMI_SetAttr(eHdmi, &stAttr), MI_SUCCESS);
    ExecFunc(MI_HDMI_Start(eHdmi), MI_SUCCESS);
    ExecFunc(MI_HDMI_SetAvMute(eHdmi, false), MI_SUCCESS);
    _gArg.hdmi_enable = true;

    return MI_SUCCESS;
}

int hdmi_stop()
{
    if (!_gArg.hdmi_enable)
       return 0;

    printf("\nEnter Hdmi Stop\n");

    MI_HDMI_DeviceId_e eHdmi = g_HdmiAttr.eHdmi;

    ExecFunc(MI_HDMI_Stop(eHdmi), MI_SUCCESS);
    ExecFunc(MI_HDMI_SetAvMute(eHdmi, true), MI_SUCCESS);
    _gArg.hdmi_enable = false;
    return 0;
}

int hdmi_setAnalogDrvCur()
{
    MI_HDMI_AnalogDrvCurrent_t CurInfo = {};

    memset(&CurInfo, 0xFF, sizeof(CurInfo));
    MI_HDMI_SetAnalogDrvCurrent(g_HdmiAttr.eHdmi, &CurInfo);
}

int hdmi_setInfoframe()
{
    char u8Ch[10];
    const char *nptr = u8Ch;

    MI_HDMI_DeviceId_e eHdmi = g_HdmiAttr.eHdmi;
    MI_HDMI_InfoFrame_t stAviInfoFrame = {E_MI_HDMI_INFOFRAME_TYPE_AVI};
    MI_HDMI_InfoFrame_t stAudInfoFrame = {E_MI_HDMI_INFOFRAME_TYPE_AUDIO};
    MI_HDMI_InfoFrame_t stSpdInfoFrame = {E_MI_HDMI_INFOFRAME_TYPE_SPD};

    /* AviInfoFrame */
    stAviInfoFrame.unInforUnit.stAviInfoFrame.bEnableAfdOverWrite = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.A0Value = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eColorType = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eColorimetry = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eExtColorimetry = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eYccQuantRange = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eTimingType = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eAfdRatio = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eScanInfo = 0;
    stAviInfoFrame.unInforUnit.stAviInfoFrame.eAspectRatio = 0;
    /* AudioInfoFrame */
    stAudInfoFrame.unInforUnit.stAudInfoFrame.u32ChannelCount = 0;
    stAudInfoFrame.unInforUnit.stAudInfoFrame.eAudioCodeType = 0;
    stAudInfoFrame.unInforUnit.stAudInfoFrame.eSampleRate = 0;
    /* SpdInfoFrame */
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[0] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[1] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[2] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[3] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[4] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[5] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[6] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8VendorName[7] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[0] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[1] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[2] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[3] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[4] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[5] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[6] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[7] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[8] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[9] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[10] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[11] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[12] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[13] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[14] = 0;
    stSpdInfoFrame.unInforUnit.stSpdInfoFrame.au8ProductDescription[15] = 0;
    MI_HDMI_SetInfoFrame(eHdmi, &stAviInfoFrame);
}

static MI_U32 disp_getuserportsetting(MI_DISP_LAYER DispLayer,
    MI_U32 u32LayerWidth, MI_U32 u32LayerHeight, MI_U8 u8PortNum ,char *filepath)
{
    MI_U8 n = 0;
    MI_U8 u8Factor = 0;
    char usercmd[256];
    MI_U32 u32PortX = 0;
    MI_U32 u32PortY = 0;
    MI_U32 u32PortSizeWidth = 0;
    MI_U32 u32PortSizeHeight = 0;

    if(u8PortNum > DISP_INPUT_PORT_MAX || u8PortNum == 0)
    {
        printf("port num is invalid\n");
        return E_MI_ERR_FAILED;
    }
    if(u8PortNum == 1)
    {
        u32PortSizeWidth = u32LayerWidth;
        u32PortSizeHeight = u32LayerHeight;
        u8Factor = 1;
    }
    else if(u8PortNum <= 4)
    {
        u32PortSizeWidth = u32LayerWidth/2;
        u32PortSizeHeight = u32LayerHeight/2;
        u8Factor = 2;
    }
    else if(u8PortNum <= 9)
    {
        u32PortSizeWidth = u32LayerWidth/3;
        u32PortSizeHeight = u32LayerHeight/3;
        u8Factor = 3;
    }
    else if(u8PortNum <= 16)
    {
        u32PortSizeWidth = u32LayerWidth/4;
        u32PortSizeHeight = u32LayerHeight/4;
        u8Factor = 4;
    }
    for(n = 0; n < u8PortNum; n++)
    {
        MI_DISP_InputPortAttr_t *stInputPortAttr = &g_DispAttr.stInPortAttr[n].stInputPortAttr;

        stInputPortAttr->stDispWin.u16X = (n%u8Factor)*u32PortSizeWidth;
        stInputPortAttr->stDispWin.u16Y = (n/u8Factor)*u32PortSizeHeight;
        stInputPortAttr->stDispWin.u16Width = u32PortSizeWidth;
        stInputPortAttr->stDispWin.u16Height = u32PortSizeHeight;
        stInputPortAttr->u16SrcWidth = g_HdmiAttr.inWidth;
        stInputPortAttr->u16SrcHeight = g_HdmiAttr.inHeight;
        strncpy(g_DispAttr.stInPortAttr[n].FilePath,filepath,sizeof(g_DispAttr.stInPortAttr[n].FilePath));
    }

    return u8PortNum;
}

static MI_BOOL disp_GetInputFrameDataYuv(int srcFd, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = false;
    MI_U32 u32ReadSize = 0;
    MI_U32 u32LineNum = 0;
    MI_U32 u32BytesPerLine = 0;
    MI_U32 u32Index = 0;
    MI_U32 u32FrameDataSize = 0;
    MI_U32 u32YSize = 0;
    MI_U32 u32UVSize = 0;

    if(E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width * 2;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;

        for (u32Index = 0; u32Index < u32LineNum; u32Index ++)
        {
            u32ReadSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], u32BytesPerLine);
        }
        if(u32ReadSize == u32FrameDataSize)
        {
            bRet = true;
        }
        else if(u32ReadSize < u32FrameDataSize)
        {
            lseek(srcFd, 0, SEEK_SET);
            u32ReadSize = 0;

            for (u32Index = 0; u32Index < u32LineNum; u32Index ++)
            {
                u32ReadSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], u32BytesPerLine);
            }

            if(u32ReadSize == u32FrameDataSize)
            {
                bRet = true;
            }
            else
            {
                printf("read file error. u32ReadSize = %u. \n", u32ReadSize);
                bRet = false;
            }
        }

    }
    else if(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height * 3 / 2;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;
        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
        {
            u32YSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], u32BytesPerLine);
        }

        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
        {
            u32UVSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], u32BytesPerLine);
        }

        if(u32YSize + u32UVSize == u32FrameDataSize)
        {
            bRet = true;
        }
        else if(u32YSize + u32UVSize < u32FrameDataSize)
        {
            lseek(srcFd, 0, SEEK_SET);
            u32YSize = 0;
            u32UVSize = 0;

            for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
            {
                u32YSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], u32BytesPerLine);
            }

            for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
            {
                u32UVSize += read(srcFd, pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], u32BytesPerLine);
            }

            if(u32YSize + u32UVSize == u32FrameDataSize)
            {
                bRet = true;
            }
            else
            {
                printf("read file error. u32YSize = %u, u32UVSize = %u. \n", u32YSize, u32UVSize);
                bRet = false;
            }
        }
        else
        {
            bRet = false;
            printf("[%s][%d][y_size:%d][uv_size:%d][frame_size:%d]\n",__func__, __LINE__, u32YSize, u32UVSize, u32FrameDataSize);
        }
    }

    return bRet;
}

static void *disp_PutBuffer (void *pData)
{
    int srcFd = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_U8 u8PortId = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32BuffSize;
    MI_SYS_BUF_HANDLE hHandle;
    struct timeval stTv;
    struct timeval stGetBuffer, stReadFile, stFlushData, stPutBuffer, stRelease;
    time_t stTime = 0;

    StDispUtPort_t * pstInPortAttr = (StDispUtPort_t *)pData;
    MI_SYS_ChnPort_t *pstSysChnPort = &pstInPortAttr->stSysChnPort;
    MI_DISP_InputPortAttr_t *pstInputPortAttr = &pstInPortAttr->stInputPortAttr;
    MI_SYS_PixelFormat_e ePixelFormat = g_DispAttr.stLayerAttr.ePixFormat;

    u8PortId = pstInPortAttr->u32PortId;
    DispLayer = pstInPortAttr->DispLayer;

    const char *filePath = pstInPortAttr->FilePath;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));
    memset(&stTv, 0, sizeof(stTv));

    srcFd = open(filePath, O_RDONLY);
    if (srcFd < 0)
    {
        printf("src_file: %s.\n", filePath);
        perror("open");
        return NULL;
    }

    while(pstInPortAttr->bPortEnable)
    {
        gettimeofday(&stTv, NULL);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
        stBufConf.stFrameCfg.u16Width = g_HdmiAttr.inWidth;
        stBufConf.stFrameCfg.u16Height = g_HdmiAttr.inHeight;
        stBufConf.stFrameCfg.eFormat = ePixelFormat;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        gettimeofday(&stGetBuffer, NULL);

        if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(pstSysChnPort,&stBufConf,&stBufInfo,&hHandle, -1))
        {
            stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
            stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
            stBufInfo.bEndOfStream = false;

            u32BuffSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            gettimeofday(&stReadFile, NULL);
            if(disp_GetInputFrameDataYuv(srcFd, &stBufInfo) == true)
            {
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , false);
            }
            else
            {
                printf("disp_test getframe failed\n");
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , true);
            }
        }
        else
        {
            printf("disp_test sys get buf fail\n");
        }
        usleep(200*1000);
    }
    close(srcFd);

    return MI_DISP_SUCCESS;
}

int disp_start()
{
    if(_gArg.disp_enable)
    {
        ExecFunc(disp_stop(), MI_SUCCESS);
    }

    MI_U8 u8PortNum = g_DispAttr.u8PortNum, i;
    MI_DISP_DEV DispDev = g_DispAttr.DispDev;
    MI_DISP_LAYER DispLayer = g_DispAttr.DispLayer;
    MI_DISP_PubAttr_t stPubAttr = g_DispAttr.stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr = g_DispAttr.stLayerAttr;
    MI_SYS_ChnPort_t stSysChnPort;

    printf("\nEnter Disp Start\n");

    /* Set Disp Pub */
    ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_Enable(DispDev), MI_SUCCESS);

    /* Disp Layer Releated */
    MI_DISP_BindVideoLayer(DispLayer,DispDev);
    MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
    MI_DISP_EnableVideoLayer(DispLayer);

    /* Set Input Port */
    for (i=0; i < u8PortNum; i++)
    {
        MI_DISP_InputPortAttr_t *stInputPortAttr = &g_DispAttr.stInPortAttr[i].stInputPortAttr;
        MI_DISP_SetInputPortAttr(DispLayer, i, stInputPortAttr);
        MI_DISP_EnableInputPort(DispLayer, i);
        MI_DISP_SetInputPortSyncMode(DispLayer, i, E_MI_DISP_SYNC_MODE_FREE_RUN);
        g_DispAttr.stInPortAttr[i].u32PortId = i;
        g_DispAttr.stInPortAttr[i].bPortEnable = true;
    }

    //create put buff thread
    for(i = 0; i < u8PortNum; i++)
    {
        StDispUtPort_t * pstInPortAttr = &g_DispAttr.stInPortAttr[i];

        stSysChnPort.eModId = E_MI_MODULE_ID_DISP;
        stSysChnPort.u32DevId = DispDev;
        stSysChnPort.u32ChnId = 0;
        stSysChnPort.u32PortId = i;
        pstInPortAttr->stSysChnPort = stSysChnPort;

        pthread_create(&pstInPortAttr->task, NULL, disp_PutBuffer, pstInPortAttr);
    }

    _gArg.disp_enable = true;
    return 0;
}

int disp_stop()
{
    if(!_gArg.disp_enable)
       return 0;

    int i;
    MI_DISP_DEV DispDev = g_DispAttr.DispDev;
    MI_DISP_LAYER DispLayer = g_DispAttr.DispLayer;
    MI_U8 u8PortNum = g_DispAttr.u8PortNum;

    printf("\nEnter Disp Stop\n");

    //destroy put buff thread
    for(i = 0; i < u8PortNum; i++)
    {
        StDispUtPort_t * pstInPortAttr = &g_DispAttr.stInPortAttr[i];
        if(pstInPortAttr->bPortEnable)
        {
            pstInPortAttr->bPortEnable = false;
            pthread_join(pstInPortAttr->task, NULL);
            ExecFunc(MI_DISP_DisableInputPort(DispLayer, i), MI_SUCCESS);
        }
    }
    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_SUCCESS);

    _gArg.disp_enable = false;
    return 0;
}

static int audio_GetFrameFromeFile(int fd, void *bufferaddr, int size)
{
    int readsize;

    readsize = read(fd, bufferaddr, size);
    if(readsize < size)
        lseek(fd, 0, SEEK_SET);

    return readsize;
}

static void *audio_PutBuffer (void *pData)
{
    AudioAttr_t * audioAttr = (AudioAttr_t *)pData;

    MI_AUDIO_DEV aoDevId = audioAttr->aoDevId;
    MI_AO_CHN aoChn = audioAttr->aoChn;
    MI_AUDIO_Frame_t aoFrame;
    MI_AUDIO_Attr_t stAoAttr = audioAttr->stAoAttr;

    int bitwidth = stAoAttr.eBitwidth == E_MI_AUDIO_BIT_WIDTH_16?16:24;
    int sample_per_frame = stAoAttr.u32PtNumPerFrm;
    int buffersize = sample_per_frame * bitwidth / 8;

    int srcFd, ret;
    char *filePath = audioAttr->FilePath;
    char *buffer = NULL;

    buffer = (char*)malloc(buffersize * sizeof(char));
    srcFd = open(filePath, O_RDONLY);
    if (srcFd < 0)
    {
        printf("src_file: %s.\n", filePath);
        perror("open");
        free(buffer);
        return NULL;
    }
    aoFrame.apVirAddr[0] = buffer;
    while(audioAttr->bDevEnable)
    {
        aoFrame.u32Len =
            audio_GetFrameFromeFile(srcFd, aoFrame.apVirAddr[0], buffersize);
retry:
        ret = MI_AO_SendFrame(aoDevId, aoChn, &aoFrame, 1);
        if (ret != 0)
            goto retry;
    }
    close(srcFd);
    free(buffer);
    return NULL;
}

int audio_start()
{
    if(_gArg.audio_enable)
    {
       ExecFunc(audio_stop(), MI_SUCCESS);
    }

    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV aoDevId = g_AudioAttr.aoDevId;
    MI_AO_CHN aoChn = g_AudioAttr.aoChn;
    MI_AUDIO_Attr_t stAoAttr = g_AudioAttr.stAoAttr;

    ExecFunc(MI_AO_SetPubAttr(aoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_GetPubAttr(aoDevId, &stAoAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(aoDevId), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(aoDevId, aoChn), MI_SUCCESS);

    pthread_create(&g_AudioAttr.task, NULL, audio_PutBuffer, &g_AudioAttr);

    g_AudioAttr.bDevEnable = true;
    _gArg.audio_enable = true;
    return 0;
}

int audio_stop()
{
    if(!_gArg.audio_enable)
       return 0;

    printf("\nEnter Audio Stop\n");

    MI_AUDIO_DEV aoDevId = g_AudioAttr.aoDevId;
    MI_AO_CHN aoChn = g_AudioAttr.aoChn;

    g_AudioAttr.bDevEnable = false;
    pthread_join(g_AudioAttr.task, NULL);
    ExecFunc(MI_AO_DisableChn(aoDevId, aoChn), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(aoDevId), MI_SUCCESS);

    _gArg.audio_enable = false;
    return 0;
}

int hdmi_reset(MI_HDMI_DeviceId_e eHdmi)
{
    MI_S32 s32ret;
    MI_HDMI_Attr_t stAttr;

    s32ret = MI_HDMI_SetAvMute(eHdmi, true);
    s32ret = MI_HDMI_GetAttr(eHdmi, &stAttr);
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = true;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = true;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = true;
    stAttr.stAudioAttr.bEnableAudio = true;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = true;
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;//1080P60 -> 720P60
    s32ret = MI_HDMI_SetAttr(eHdmi, &stAttr);
    s32ret = MI_HDMI_SetAvMute(eHdmi, false);
    return 0;
}

void help()
{
    printf("\n=======================================\n");
    printf(">>>>>> Test Hdmi Option Select <<<<<<<\n\n");
    printf("- a: start/stop audio\n");
    printf("- d: start/stop Display\n");
    printf("- c: Test Set Analog DrvCurrent\n");
    printf("- R: Change Display Timming\n");
    printf("- e: Get edid\n");
    printf("- f: start/stop Hdmi\n");
    printf("- h: help message\n");
    printf("- q: quit Test Func\n");
    printf("- r: Reset Hdmi\n");
    printf("- R: Change Audio Sample Rate\n");
    printf("- s: Get sink info\n");
    printf("\n>>>> Function: ");
}

int main(int argc, const char *argv[])
{
    MI_S32 s32ret = -1, i = 0;
    MI_U8 u8Input[100];
    MI_HDMI_InitParam_t stInitParam;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;

    /* Base Module Init */
    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = Hdmi_callback_impl;
    ExecFunc(MI_HDMI_Init(&stInitParam), MI_SUCCESS);
    ExecFunc(MI_HDMI_Open(eHdmi), MI_SUCCESS);

    g_HdmiAttr.eHdmi = eHdmi;
    _gArg.Inited = true;

    /* Loop Task */
    param_init(1);
    ExecFunc(disp_start(), MI_SUCCESS);
    ExecFunc(audio_start(), MI_SUCCESS);
    ExecFunc(hdmi_start(), MI_SUCCESS);
    while (_gArg.Inited)
    {
        scanf("%s", u8Input);
        switch (u8Input[0])
        {
            case 'a': //open/stop audio
            {
                if(_gArg.audio_enable)
                {
                    ExecFunc(audio_stop(), MI_SUCCESS);
                }
                else
                {
                    ExecFunc(audio_start(), MI_SUCCESS);
                }
                continue;
            }
            case 'c': //set analog current
            {
                ExecFunc(hdmi_setAnalogDrvCur(), MI_SUCCESS);
                continue;
            }
            case 'd': //start/stop disp
            {
                if(_gArg.disp_enable)
                {
                    ExecFunc(disp_stop(), MI_SUCCESS);
                }
                else
                {
                    ExecFunc(disp_start(), MI_SUCCESS);
                }
                continue;
            }
            case 'D': //change Disp Timming
            {
                hdmi_stop();
                disp_stop();
                param_init(2);
                disp_start();
                hdmi_start();
                continue;
            }
            case 'e': //get edid
            {
                MI_HDMI_Edid_t stEdid;
                s32ret = MI_HDMI_ForceGetEdid(eHdmi, &stEdid);
                for (i = 0; i < stEdid.u32Edidlength; i++)
                {
                    if(!(i%8) && i!=0)
                        printf("\n");
                    printf("[0x%02x] ", stEdid.au8Edid[i]);
                }
                printf("\n");
                continue;
            }
            case 'f'://start/stop hdmi
            {
                if(_gArg.hdmi_enable)
                {
                    ExecFunc(hdmi_stop(), MI_SUCCESS);
                }
                else
                {
                    ExecFunc(hdmi_start(), MI_SUCCESS);
                }
                continue;
            }
            case 'R': //Rate
            {
                hdmi_stop();
                audio_stop();
                param_init(4);
                audio_start();
                hdmi_start();
                continue;
            }
            case 'r': //reset timing
            {
                hdmi_reset(eHdmi);
                continue;
            }
            case 's': //get sink info
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
            {
                audio_stop();
                disp_stop();
                hdmi_stop(eHdmi);
                _gArg.Inited = false;
                break;
            }
            case 'h':
            {
                help();
                continue;
            }
            default:
                continue;
        }
    }

    param_deinit();
    /* Exit the program */
    ExecFunc(MI_HDMI_Close(eHdmi), MI_SUCCESS);
    ExecFunc(MI_HDMI_DeInit(), MI_SUCCESS);
    return 0;
}
