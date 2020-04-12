#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "mi_sys.h"
#include "mi_disp.h"
#include "mi_divp.h"
#include "mi_common.h"
#include "mi_vdec.h"
#include "mi_venc.h"
#include "mi_panel.h"
#include "mi_hdmi.h"

#include "init_panel_driveric.h"
//#include "SAT070AT50_800x480.h"
//#include "SAT070AT40_800x480.h"
//#include "SAT070CP50_1024x600.h"
//#include "RM68200_720x1280_MIPI.h"
//#include "SAT070BO30I21Y0_1024x600_MIPI.h"
#include "VS043ISN26V10_480x272.h"
//#include "ST7701S_480x480_MIPI.h"
//#include "LX50FWB4001_RM68172_480x854.h"
//#include "ST7701S_480x854.h"

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)
#define MAX(a,b) ({\
        typeof(a) _a = a;\
        typeof(b) _b = b;\
        (_a) > (_b) ? (_a) : (_b);\
        })
#define MIN(a,b) ({\
        typeof(a) _a = a;\
        typeof(b) _b = b;\
        (_a) < (_b) ? (_a) : (_b);\
        })

#define DISP_INPUT_PORT_MAX 16
#define DISP_LAYER_MAX 2
#define DISP_DEV_MAX 2

#define cus_print(fmt, args...) {do{printf("\033[32m");printf(fmt, ##args);printf("\033[0m");}while(0);}

#ifndef ALIGN_UP
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (( (val)/(alignment))*(alignment))
#endif

#define ENABLE_CAPTURE_TEST 0
#define ENABLE_GAMMA_TEST 0
#define CROP_TEST 0

#if (ENABLE_GAMMA_TEST == 1)
#include "gamma_normal.h"
#include "gamma_customer.h"
#endif

typedef struct stDispTestPubBuffThreadArgs_s
{
    pthread_t pt;
    pthread_t ptsnap;
    char FileName[50];
    MI_BOOL bRunFlag;
    MI_DISP_LAYER DispLayer;
    MI_U32 u32PortId;
    MI_U16 u16BuffWidth;
    MI_U16 u16BuffHeight;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_SYS_ChnPort_t stSysChnPort;
}stDispTestPutBuffThreadArgs_t;

typedef struct stDispUtDev_s
{
    MI_BOOL bDevEnable;
    MI_BOOL bPanelInit;
    MI_BOOL bDevBindLayer[DISP_LAYER_MAX];
    MI_DISP_PubAttr_t stPubAttr;
    MI_PANEL_LinkType_e eLinkType;
}stDispUtDev_t;

typedef struct stDispUtLayer_s
{
    MI_BOOL bLayerEnable;
    MI_DISP_RotateMode_e eRotateMode;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
}stDispUtLayer_t;

typedef struct stDispUtPort_s
{
    MI_BOOL bPortEnable;
    MI_SYS_PixelFormat_e    ePixFormat;         /* Pixel format of the video layer */
    MI_DISP_VidWinRect_t stCropWin;                     /* rect of video out chn */
    MI_DISP_InputPortAttr_t stInputPortAttr;
    char FilePath[50];
}stDispUtPort_t;

typedef struct stTimingArray_s
{
    char desc[50];
    MI_DISP_OutputTiming_e eOutputTiming;
    MI_HDMI_TimingType_e eHdmiTiming;
    MI_U16 u16Width;
    MI_U16 u16Height;
}stTimingArray_t;

typedef struct stPixelFormatArray_s
{
    char desc[50];
    MI_SYS_PixelFormat_e eSysPixelFormat;
}stPixelFormatArray_t;

typedef struct stVdecParam_s
{
    MI_BOOL bCreate;
    MI_U32 u32VdecSrcWidth;
    MI_U32 u32VdecSrcHeight;
    MI_U32 u32VdecDstWidth;
    MI_U32 u32VdecDstHeight;
    MI_VDEC_VideoMode_e eVideoMode;
    MI_VDEC_CodecType_e eCodecType;
    MI_VDEC_DPB_BufMode_e eDpbBufMode;
    char esfilepath[50];
}stVdecParam_t;

#define VDEC_CHN_MAX (33)
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (1024 * 1024)

static stVdecParam_t g_stVdecParam[VDEC_CHN_MAX];
static pthread_t g_thrPush[VDEC_CHN_MAX];
static pthread_t g_thrGet[VDEC_CHN_MAX];
static MI_BOOL g_bRun[VDEC_CHN_MAX];
static MI_U8  g_u8FrameRate = 30;
static MI_BOOL g_bPipEnable = 0;
static FILE *g_hFile[VDEC_CHN_MAX];

static MI_BOOL g_PushDataExit[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX] = {FALSE};
static MI_BOOL g_PushDataStop[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX] = {FALSE};
static stDispTestPutBuffThreadArgs_t gastDispTestPutBufThread[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX];

static stDispUtDev_t g_astDispUtDev[DISP_DEV_MAX];
static stDispUtLayer_t g_astDispUtLayer[DISP_LAYER_MAX];
static stDispUtPort_t g_astDispUtPort[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX];

stTimingArray_t astTimingArray[] = {
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

stPixelFormatArray_t astPixelFormatArray[] = {
    {
        .desc = "yuv422_yuyv",
        .eSysPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
    },
    {
        .desc = "yuv420sp",
        .eSysPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,
    },
    {
        .desc = "mst420",
        .eSysPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_MST_420,
    },
};

static MI_U64 get_pts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }
    return (MI_U64)(1000 / u32FrameRate);
}

static MI_S32 Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;

    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));

    MI_SYS_Init();

    MI_SYS_GetVersion(&stVersion);
    MI_SYS_GetCurPts(&u64Pts);

    u64Pts = 0xF1237890F1237890;
    MI_SYS_InitPtsBase(u64Pts);

    u64Pts = 0xE1237890E1237890;
    MI_SYS_SyncPts(u64Pts);

    return MI_SUCCESS;
}

static MI_S32 Sys_Exit(void)
{
    MI_SYS_Exit();
    return MI_SUCCESS;
}

static MI_BOOL disp_test_OpenSourceFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    if (src_fd < 0)
    {
        printf("src_file: %s.\n", pFileName);

        perror("open");
        return -1;
    }
    *pSrcFd = src_fd;

    return TRUE;
}

static MI_BOOL disp_test_GetInputFrameDataYuv(int srcFd, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = FALSE;
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
            bRet = TRUE;
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
                bRet = TRUE;
            }
            else
            {
                printf("read file error. u32ReadSize = %u. \n", u32ReadSize);
                bRet = FALSE;
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
            bRet = TRUE;
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
                bRet = TRUE;
            }
            else
            {
                printf("read file error. u32YSize = %u, u32UVSize = %u. \n", u32YSize, u32UVSize);
                bRet = FALSE;
            }
        }
        else
        {
            bRet = FALSE;
            printf("[%s][%d][y_size:%d][uv_size:%d][frame_size:%d]\n",__func__, __LINE__, u32YSize, u32UVSize, u32FrameDataSize);
        }
    }

    return bRet;
}

#if (defined INTERFACE_HDMI) && (INTERFACE_HDMI == 1)
static MI_S32 Hdmi_callback(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_EventType_e Event, void *pEventParam, void *pUsrParam)
{
    switch (Event)
    {
        case E_MI_HDMI_EVENT_HOTPLUG:
            printf("E_MI_HDMI_EVENT_HOTPLUG.\n");
            MI_HDMI_Start(eHdmi);
            break;
        case E_MI_HDMI_EVENT_NO_PLUG:
            printf("E_MI_HDMI_EVENT_NO_PLUG.\n");
            MI_HDMI_Stop(eHdmi);
            break;
        default:
            printf("Unsupport event.\n");
            break;
    }
    return MI_SUCCESS;
}

static MI_S32 Hdmi_Start(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_TimingType_e eTimingType)
{
    MI_HDMI_Attr_t stAttr;

    MI_HDMI_Start(eHdmi);
    memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
    if(eTimingType==E_MI_HDMI_TIMING_MAX)
    {
        printf("[%s][%d]unsupported hdmi timing %d,reset to 1080p60\n",__FUNCTION__,__LINE__,eTimingType);
        eTimingType = E_MI_HDMI_TIMING_1080_60P;
    }
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stAttr.stAudioAttr.bEnableAudio = TRUE;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = TRUE;
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_YCBCR444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = eTimingType;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    MI_HDMI_SetAttr(eHdmi, &stAttr);

    return MI_SUCCESS;
}
#endif

static void GetLayerDisplaySize(MI_DISP_OutputTiming_e eOutputTiming, MI_U32 *LayerDisplayWidth, MI_U32 *LayerDisplayHeight)
{
    int index = 0;

    if(eOutputTiming == E_MI_DISP_OUTPUT_USER){
        *LayerDisplayWidth = stPanelParam.u16Width;
        *LayerDisplayHeight = stPanelParam.u16Height;
        return;
    }
    for(index=0;index<sizeof(astTimingArray);index++){
        if(eOutputTiming == astTimingArray[index].eOutputTiming){
            *LayerDisplayWidth = astTimingArray[index].u16Width;
            *LayerDisplayHeight = astTimingArray[index].u16Height;
                return;
        }
    }
    if(index == (int)sizeof(astTimingArray)){
        cus_print("invalid outputtiming\n");
    }
}

static MI_HDMI_TimingType_e gethdmitiming(MI_DISP_OutputTiming_e eOutputTiming)
{
    int index = 0;
    MI_HDMI_TimingType_e eHdmiTiming;

    for(index=0;index<sizeof(astTimingArray);index++)
    {
        if(eOutputTiming == astTimingArray[index].eOutputTiming)
        {
            eHdmiTiming = astTimingArray[index].eHdmiTiming;
            break;
        }
    }
    return eHdmiTiming;
}

#if (defined INTERFACE_VDEC) && (INTERFACE_VDEC == 1)
static void *push_stream(void* args)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_U8 *pu8Buf = NULL;
    MI_U32 u32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    //MI_VDEC_ChnStat_t stChnStat;
    MI_U32 u32Param = (MI_U32)args;
    MI_VDEC_VideoStream_t stVdecStream;
    MI_S32 s32TimeOutMs = 20;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)(u32Param >> 16);
    MI_VDEC_VideoMode_e eVideoMode = (MI_VDEC_VideoMode_e)(u32Param & 0xFF);

    pu8Buf = malloc(VESFILE_READER_BATCH);
    printf("chn(%d) %s\n", VdecChn, (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode) ? "stream" : "frame");

    while (g_bRun[VdecChn])
    {
        usleep(1000 / g_u8FrameRate * 1000);

        if (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode)
        {
            ///stream mode, read 128k every time
            u32FrameLen = VESFILE_READER_BATCH;
            u32Pos = fseek(g_hFile[VdecChn], 0L, SEEK_CUR);
        }
        else
        {
            ///frame mode, read one frame lenght every time
            memset(au8Header, 0, 16);
            u32Pos = fseek(g_hFile[VdecChn], 0L, SEEK_CUR);
            u32Len = fread(au8Header, 16, 1, g_hFile[VdecChn]);
            if(u32Len <= 0)
            {
                fseek(g_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }

            u32FrameLen = MI_U32VALUE(au8Header, 4);
            if(u32FrameLen > VESFILE_READER_BATCH)
            {
                fseek(g_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }
        }

        u32Len = fread(pu8Buf, u32FrameLen, 1, g_hFile[VdecChn]);
        if(u32Len <= 0)
        {
            fseek(g_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = u32FrameLen;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len + 16; //back length

#if 0
        printf("%s, %d, VdecChn: %d, pu8Addr: %p, u32Len: %u, u64PTS: %llu.\n", __FUNCTION__, __LINE__,
            VdecChn,
            stVdecStream.pu8Addr,
            stVdecStream.u32Len,
            stVdecStream.u64PTS);
#endif

        if(MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(VdecChn, &stVdecStream, s32TimeOutMs)))
        {
            //ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(g_hFile[VdecChn], - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + get_pts(30);
    }

    free(pu8Buf);
    return NULL;
}

static void create_push_stream_thread(MI_VDEC_CHN VdecChn, MI_BOOL bCreateGetThead, MI_VDEC_ChnAttr_t *pstChnAttr, char* filepath)
{
    FILE *readfp = NULL;

    g_bRun[VdecChn] = 1;

    printf("\033[1;32m""fopen File:%s\n""\033[0m", filepath);

    readfp = fopen(filepath, "rb");
    if (readfp)
    {
        g_hFile[VdecChn] = readfp;
        if (pthread_create(&g_thrPush[VdecChn], NULL, push_stream, (void *)((VdecChn << 16) | pstChnAttr->eVideoMode)))
        {
            assert(0);
        }

        if (!bCreateGetThead)
        {
            return;
        }
    }
    else
    {
        printf("\033[1;31m""fopen File:%s Error\n""\033[0m", filepath);
    }
}
static void destroy_push_stream_thread(MI_VDEC_CHN VdecChn)
{
    if (g_bRun[VdecChn])
    {
        g_bRun[VdecChn] = 0;
        if (g_thrPush[VdecChn])
        {
            pthread_join(g_thrPush[VdecChn], NULL);
        }

        if (g_thrGet[VdecChn])
        {
            pthread_join(g_thrGet[VdecChn], NULL);
        }
    }

    if (g_hFile[VdecChn])
    {
        fclose(g_hFile[VdecChn]);
        g_hFile[VdecChn] = NULL;
    }
}
#endif

static void *disp_test_PutBuffer(void *pData)
{
    int srcFd = 0;
    int i = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_U8 u8PortId = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32BuffSize;
    MI_SYS_BUF_HANDLE hHandle;
    struct timeval stTv;
    struct timeval stGetBuffer, stReadFile, stFlushData, stPutBuffer, stRelease,swinShift;
    stDispTestPutBuffThreadArgs_t *pstDispTestPutBufArgs = (stDispTestPutBuffThreadArgs_t *)pData;
    const char *filePath = pstDispTestPutBufArgs->FileName;
    MI_SYS_ChnPort_t *pstSysChnPort = &pstDispTestPutBufArgs->stSysChnPort;
    u8PortId = pstDispTestPutBufArgs->u32PortId;
    DispLayer = pstDispTestPutBufArgs->DispLayer;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));
    memset(&stTv, 0, sizeof(stTv));

    printf("----------------------start-[%d]-[%2d]--------------------\n",DispLayer,u8PortId);
    if (TRUE == disp_test_OpenSourceFile(filePath, &srcFd))
    {
        while(!g_PushDataExit[DispLayer][u8PortId])
        {
            if(g_PushDataStop[DispLayer][u8PortId] == FALSE)
            {
                if(g_bPipEnable){
                    static int cnt = 0;
                    if(cnt++ >= 60){
                        cnt = 0;
                        MI_U32 u32LayerDispWidth,u32LayerDispHeight;
                        MI_DISP_InputPortAttr_t stInputPortAttr;
                        GetLayerDisplaySize(g_astDispUtDev[0].stPubAttr.eIntfSync, &u32LayerDispWidth, &u32LayerDispHeight);
                        MI_DISP_GetInputPortAttr(1,0,&stInputPortAttr);
                        srand((unsigned int)time(NULL));
                        stInputPortAttr.stDispWin.u16X = ALIGN_DOWN(rand()%(u32LayerDispWidth-stInputPortAttr.u16SrcWidth),2);
                        stInputPortAttr.stDispWin.u16Y = ALIGN_DOWN(rand()%(u32LayerDispHeight-stInputPortAttr.u16SrcHeight),2);
                        MI_DISP_SetInputPortAttr(1,0,&stInputPortAttr);
                    }
                }
                gettimeofday(&stTv, NULL);
                stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
                stBufConf.stFrameCfg.u16Width = pstDispTestPutBufArgs->u16BuffWidth;
                stBufConf.stFrameCfg.u16Height = pstDispTestPutBufArgs->u16BuffHeight;
                stBufConf.stFrameCfg.eFormat = pstDispTestPutBufArgs->ePixelFormat;
                stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                gettimeofday(&stGetBuffer, NULL);

                if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(pstSysChnPort,&stBufConf,&stBufInfo,&hHandle, -1))
                {
                    stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                    stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                    stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
                    stBufInfo.bEndOfStream = FALSE;

                    u32BuffSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                    gettimeofday(&stReadFile, NULL);
                    if(disp_test_GetInputFrameDataYuv(srcFd, &stBufInfo) == TRUE)
                    {
                        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                    }
                    else
                    {
                        printf("disp_test getframe failed\n");
                        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , TRUE);
                    }
                }
                else
                {
                    printf("disp_test sys get buf fail\n");
                }
                usleep(40*1000);
            }

        }
        close(srcFd);
    }
    else
    {
        printf(" open file fail. \n");
    }
    printf("----------------------end----------------------\n");

    return MI_DISP_SUCCESS;
}

#if (ENABLE_CAPTURE_TEST == 1)
#define CAPTURE_PATH "capture"
void SaveCaptureStreamToFile(MI_VENC_Stream_t *pstStream)
{
    MI_U32 i = 0;
    char szFileName[64];
    time_t now = 0;
    struct tm *tm = NULL;
    MI_U32 len = 0;
    int fd = 0;
    MI_VENC_Pack_t *pstPack = NULL;

    memset(szFileName, 0, sizeof(szFileName));

    now = time(NULL);
    tm = localtime(&now);
    len = 0;

    struct stat st;
    if (stat(CAPTURE_PATH, &st) == -1)
    {
        if (mkdir(CAPTURE_PATH, S_IRWXU |S_IRGRP | S_IXGRP|S_IROTH | S_IXOTH) == -1)
        {
            return;
        }
    }

    len += sprintf(szFileName + len, "%s/", CAPTURE_PATH);
    len += sprintf(szFileName + len, "%d_", tm->tm_year + 1900);
    len += sprintf(szFileName + len, "%02d_", tm->tm_mon);
    len += sprintf(szFileName + len, "%02d-", tm->tm_mday);
    len += sprintf(szFileName + len, "%02d_", tm->tm_hour);
    len += sprintf(szFileName + len, "%02d_", tm->tm_min);
    len += sprintf(szFileName + len, "%02d", tm->tm_sec);
    len += sprintf(szFileName + len, ".%s", "jpg");

    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd <= 0)
    {
        // ST_WARN("create file %s error\n", szFileName);
    }
    else
    {
        // ST_DBG("open %s success\n", szFileName);
    }

    for (i = 0; i < pstStream->u32PackCount; i ++)
    {
        pstPack = &pstStream->pstPack[i];
        write(fd, pstPack->pu8Addr + pstPack->u32Offset, pstPack->u32Len - pstPack->u32Offset);
    }

    close(fd);
}

void CaptureCreate(MI_U8 u8ChnId)
{
    MI_U32 u32DevId = 0;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_DIVP_ChnAttr_t stDivpChnAttr;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateNum = 30;
    stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateDen = 1;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = g_stVdecParam[u8ChnId].u32VdecSrcWidth;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = g_stVdecParam[u8ChnId].u32VdecSrcHeight;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = g_stVdecParam[u8ChnId].u32VdecSrcWidth;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = g_stVdecParam[u8ChnId].u32VdecSrcHeight;
    stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;

    MI_VENC_CreateChn(u8ChnId, &stChnAttr);
    if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        MI_VENC_ParamJpeg_t stParamJpeg;

        memset(&stParamJpeg, 0, sizeof(stParamJpeg));
        MI_VENC_GetJpegParam(u8ChnId, &stParamJpeg);

        stParamJpeg.u32Qfactor = 50;
        MI_VENC_SetJpegParam(u8ChnId, &stParamJpeg);
    }

    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = u8ChnId;
    stSrcChnPort.u32PortId = 1; //for capture
    MI_SYS_SetChnOutputPortDepth(&stSrcChnPort, 0, 3);

    MI_VENC_GetChnDevid(u8ChnId, &u32DevId);
    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.u32DevId = u32DevId;
    stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stDstChnPort.u32ChnId = u8ChnId;
    stDstChnPort.u32PortId = 0;
    MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, 30, 30);

    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.u32MaxWidth = 1920;
    stDivpChnAttr.u32MaxHeight = 1920;
    stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.bHorMirror = false;
    stDivpChnAttr.bVerMirror = false;
    MI_DIVP_CreateChn(6, &stDivpChnAttr);
}
void CaptureGetStream(MI_U8 u8ChnId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_VENC_ChnStat_t stStat;

    s32Ret = MI_VENC_StartRecvPic(u8ChnId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("ST_Venc_StartChannel fail, 0x%x\n", s32Ret);
        return;
    }

    memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
    memset(&stPack, 0, sizeof(MI_VENC_Pack_t));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;
    while (1)
    {
#if 1
        s32Ret = MI_VENC_GetStream(u8ChnId, &stStream, 40);
        if (MI_SUCCESS == s32Ret)
        {
            printf("##########Start to write file!!!#####################\n");
            SaveCaptureStreamToFile(&stStream);
            printf("##########End to write file!!!#####################\n");
            s32Ret = MI_VENC_ReleaseStream(u8ChnId, &stStream);
            if (MI_SUCCESS != s32Ret)
            {
                printf("MI_VENC_ReleaseStream fail, ret:0x%x\n", s32Ret);
            }
            break;
        }
        else
            printf("Continue!!!\n");
#endif
        usleep(200 * 1000);
    }

    s32Ret = MI_VENC_StopRecvPic(u8ChnId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("ST_Venc_StopChannel fail, 0x%x\n", s32Ret);
    }
}
void CaptureDestroy(MI_U8 u8ChnId)
{
    MI_U32 u32DevId = 0;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    memset(&stSrcChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = u8ChnId;
    stSrcChnPort.u32PortId = 1; //for capture

    MI_VENC_GetChnDevid(u8ChnId, &u32DevId);
    memset(&stDstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stDstChnPort.u32DevId = u32DevId;
    stDstChnPort.u32ChnId = u8ChnId;
    stDstChnPort.u32PortId = 0;

    MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort);
    MI_VENC_DestroyChn(u8ChnId);
    MI_DIVP_DestroyChn(6);
}
#endif
static MI_BOOL disp_ut_setdev(MI_DISP_DEV DispDev)
{
    MI_DISP_PubAttr_t stPubAttr;

    memcpy(&stPubAttr, &g_astDispUtDev[DispDev].stPubAttr, sizeof(MI_DISP_PubAttr_t));
    //set disp pub
    stPubAttr.u32BgColor = YUYV_BLACK;
    MI_DISP_SetPubAttr(DispDev,  &stPubAttr);
    if(E_MI_DISP_INTF_HDMI == stPubAttr.eIntfType){
        stPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
        MI_DISP_SetPubAttr(DispDev,  &stPubAttr);
    }
    else if(E_MI_DISP_INTF_VGA == stPubAttr.eIntfType){
        stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
        MI_DISP_SetPubAttr(DispDev,  &stPubAttr);
    }
    MI_DISP_Enable(DispDev);
    g_astDispUtDev[DispDev].bDevEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_setlayer(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer)
{
    MI_DISP_VideoLayerAttr_t stLayerAttr;

    memcpy(&stLayerAttr, &g_astDispUtLayer[DispLayer].stLayerAttr, sizeof(MI_DISP_VideoLayerAttr_t));
    MI_DISP_BindVideoLayer(DispLayer,DispDev);
    MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
    MI_DISP_EnableVideoLayer(DispLayer);
    g_astDispUtDev[DispDev].bDevBindLayer[DispLayer] = TRUE;
    g_astDispUtLayer[DispLayer].bLayerEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_setport(MI_DISP_LAYER DispLayer, MI_U32 DispInport)
{
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_DISP_VidWinRect_t stWinRect;
    MI_DISP_RotateConfig_t stRotateConfig;

    memcpy(&stInputPortAttr, &g_astDispUtPort[DispLayer][DispInport].stInputPortAttr, sizeof(MI_DISP_InputPortAttr_t));
    memcpy(&stWinRect, &g_astDispUtPort[DispLayer][DispInport].stCropWin, sizeof(MI_DISP_VidWinRect_t));
    printf("%s:%d layer:%d port:%d srcwidth:%d srcheight:%d x:%d y:%d outwidth:%d outheight:%d\n",__FUNCTION__,__LINE__,
        DispLayer,DispInport,
        stInputPortAttr.u16SrcWidth,stInputPortAttr.u16SrcHeight,
        stInputPortAttr.stDispWin.u16X,stInputPortAttr.stDispWin.u16Y,
        stInputPortAttr.stDispWin.u16Width,stInputPortAttr.stDispWin.u16Height);

    stRotateConfig.eRotateMode = g_astDispUtLayer[DispLayer].eRotateMode;
    MI_DISP_SetInputPortAttr(DispLayer, DispInport, &stInputPortAttr);
    MI_DISP_SetZoomInWindow(DispLayer, DispInport, &stWinRect);
    MI_DISP_SetVideoLayerRotateMode(DispLayer, &stRotateConfig);
    MI_DISP_EnableInputPort(DispLayer, DispInport);
    MI_DISP_SetInputPortSyncMode(DispLayer, DispInport, E_MI_DISP_SYNC_MODE_FREE_RUN);

    g_astDispUtPort[DispLayer][DispInport].bPortEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_disabledev(MI_DISP_DEV DispDev)
{
    MI_DISP_Disable(DispDev);
    g_astDispUtDev[DispDev].bDevEnable = FALSE;

    return MI_SUCCESS;
}
static MI_BOOL disp_ut_disablelayer(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer)
{
    MI_DISP_DisableVideoLayer(DispLayer);
    MI_DISP_UnBindVideoLayer(DispLayer, DispDev);
    g_astDispUtDev[DispDev].bDevBindLayer[DispLayer] = FALSE;
    g_astDispUtLayer[DispLayer].bLayerEnable = FALSE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_disableport(MI_DISP_LAYER DispLayer, MI_U32 DispInport)
{
    MI_DISP_DisableInputPort(DispLayer, DispInport);
    g_astDispUtPort[DispLayer][DispInport].bPortEnable = FALSE;
    return MI_SUCCESS;
}
static void disp_ut_pip_config(void)
{
    char str[150];
    char *param[150];
    char *token;
    int i=0, j=0;

    cus_print("input pip port param:filepath src_w src_h show_x show_y show_w show_h\n");
    fgets(str, sizeof(str), stdin);
    token = strtok(str, " ");
    while(token != NULL){
        param[i++] = token;
        token = strtok(NULL, " ");
    }
    for(j=0;j<i;j++){
        cus_print("param[%d]:%s\n",j,param[j]);
    }
    strcpy(g_astDispUtPort[1][0].FilePath,param[0]);
    g_astDispUtPort[1][0].stInputPortAttr.u16SrcWidth = atoi(param[1]);
    g_astDispUtPort[1][0].stInputPortAttr.u16SrcHeight = atoi(param[2]);
    g_astDispUtPort[1][0].stInputPortAttr.stDispWin.u16X = atoi(param[3]);
    g_astDispUtPort[1][0].stInputPortAttr.stDispWin.u16Y = atoi(param[4]);
    g_astDispUtPort[1][0].stInputPortAttr.stDispWin.u16Width = atoi(param[5]);
    g_astDispUtPort[1][0].stInputPortAttr.stDispWin.u16Height = atoi(param[6]);
    g_astDispUtPort[1][0].stCropWin.u16X = 0;
    g_astDispUtPort[1][0].stCropWin.u16Y = 0;
    g_astDispUtPort[1][0].stCropWin.u16Width = g_astDispUtPort[1][0].stInputPortAttr.u16SrcWidth;
    g_astDispUtPort[1][0].stCropWin.u16Height = g_astDispUtPort[1][0].stInputPortAttr.u16SrcHeight;
    g_astDispUtPort[1][0].ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
}

static MI_BOOL disp_ut_001(MI_U8 u8ChnNum)
{
    MI_U32 n = 0;
    MI_DISP_DEV DispDev = 0;
    MI_U8 u8PortId = 0;
    MI_SYS_ChnPort_t stSysChnPort;

    MI_SYS_Init();

    if(E_MI_DISP_INTF_HDMI == g_astDispUtDev[DispDev].stPubAttr.eIntfType ||
            E_MI_DISP_INTF_VGA == g_astDispUtDev[DispDev].stPubAttr.eIntfType)
    {
#if (defined INTERFACE_HDMI) && (INTERFACE_HDMI == 1)
        MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
        MI_HDMI_InitParam_t stInitParam;
        MI_HDMI_TimingType_e eHdmiTiming;
        memset(&stInitParam, 0, sizeof(MI_HDMI_InitParam_t));
        stInitParam.pCallBackArgs = NULL;
        stInitParam.pfnHdmiEventCallback = Hdmi_callback;
        MI_HDMI_Init(&stInitParam);
        MI_HDMI_Open(eHdmi);
        eHdmiTiming = gethdmitiming(g_astDispUtDev[DispDev].stPubAttr.eIntfSync);
        Hdmi_Start(eHdmi,eHdmiTiming);
#endif
    }
    else if(g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD)
    {
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vact = stPanelParam.u16Height;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth + stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hact = stPanelParam.u16Width;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth + stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvact = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvbb = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvfb = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u32FrameRate = stPanelParam.u16DCLK*1000000/(stPanelParam.u16HTotal*stPanelParam.u16VTotal);
        g_astDispUtDev[DispDev].stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
        g_astDispUtDev[DispDev].stPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    }

    //set disp pub
    disp_ut_setdev(DispDev);
    //set layer
    disp_ut_setlayer(DispDev, 0);
    //set inputport
    for(n= 0; n < u8ChnNum; n++)
    {
        u8PortId = n;
        disp_ut_setport(0, u8PortId);
    }
    //set panel config
    if((g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD) &&
            ((g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_LVDS)
            || (g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
            || (g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_TTL)))
    {
        //init_panel_LX50FWB();
        //init_panel_ST7701S();
        MI_PANEL_Init(g_astDispUtDev[DispDev].eLinkType);
        MI_PANEL_SetPanelParam(&stPanelParam);
        if(g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
        {
            MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
        }
        g_astDispUtDev[DispDev].bPanelInit = TRUE;
    }
    if(g_bPipEnable){
        disp_ut_pip_config();
        disp_ut_setlayer(0, 1);
        disp_ut_setport(1, 0);

        stSysChnPort.eModId = E_MI_MODULE_ID_DISP;
        stSysChnPort.u32DevId = DispDev;
        stSysChnPort.u32ChnId = 16;
        stSysChnPort.u32PortId = 0;

        strcpy(gastDispTestPutBufThread[1][0].FileName, g_astDispUtPort[1][0].FilePath);
        gastDispTestPutBufThread[1][0].bRunFlag = TRUE;
        gastDispTestPutBufThread[1][0].DispLayer = 1;
        gastDispTestPutBufThread[1][0].u32PortId= 0;
        gastDispTestPutBufThread[1][0].u16BuffWidth = g_astDispUtPort[1][0].stInputPortAttr.u16SrcWidth;
        gastDispTestPutBufThread[1][0].u16BuffHeight = g_astDispUtPort[1][0].stInputPortAttr.u16SrcHeight;
        gastDispTestPutBufThread[1][0].ePixelFormat = g_astDispUtPort[1][0].ePixFormat;
        memcpy(&gastDispTestPutBufThread[1][0].stSysChnPort, &stSysChnPort, sizeof(MI_SYS_ChnPort_t));
        pthread_create(&gastDispTestPutBufThread[1][0].pt, NULL, disp_test_PutBuffer, &gastDispTestPutBufThread[1][0]);
    }
    //create put buff thread
    for(n = 0; n < u8ChnNum; n++)
    {
        stSysChnPort.eModId = E_MI_MODULE_ID_DISP;
        stSysChnPort.u32DevId = DispDev;
        stSysChnPort.u32ChnId = n;
        stSysChnPort.u32PortId = 0;

        u8PortId = n;
        strcpy(gastDispTestPutBufThread[0][u8PortId].FileName, g_astDispUtPort[0][u8PortId].FilePath);
        gastDispTestPutBufThread[0][u8PortId].bRunFlag = TRUE;
        gastDispTestPutBufThread[0][u8PortId].DispLayer = 0;
        gastDispTestPutBufThread[0][u8PortId].u32PortId = u8PortId;
        gastDispTestPutBufThread[0][u8PortId].u16BuffWidth = g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcWidth;
        gastDispTestPutBufThread[0][u8PortId].u16BuffHeight = g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcHeight;
        gastDispTestPutBufThread[0][u8PortId].ePixelFormat = g_astDispUtPort[0][u8PortId].ePixFormat;
        memcpy(&gastDispTestPutBufThread[0][u8PortId].stSysChnPort, &stSysChnPort, sizeof(MI_SYS_ChnPort_t));

        pthread_create(&gastDispTestPutBufThread[0][u8PortId].pt, NULL, disp_test_PutBuffer, &gastDispTestPutBufThread[0][u8PortId]);
    }

#if (CROP_TEST == 1)
    {
        unsigned int crop_w = 0;
        unsigned int crop_h = 0;
        unsigned int tmpc_x = 0;
        unsigned int tmpc_y = 0;
        unsigned int tmpc_w = 0;
        unsigned int tmpc_h = 0;
        int loop_cnt = 50;
        MI_DISP_InputPortAttr_t stDispInPortAttr;
        MI_DISP_VidWinRect_t stDispWinRect;

        for(;loop_cnt > 0; loop_cnt--){
            MI_DISP_GetInputPortAttr(0,0,&stDispInPortAttr);
            crop_w = ALIGN_UP(stDispInPortAttr.u16SrcWidth/4,2);
            crop_h = ALIGN_UP(stDispInPortAttr.u16SrcHeight/4,2);
            tmpc_x = MIN((stDispInPortAttr.u16SrcWidth-crop_w),ALIGN_DOWN(rand()%(stDispInPortAttr.u16SrcWidth),2));
            tmpc_y = MIN((stDispInPortAttr.u16SrcHeight-crop_h),ALIGN_DOWN(rand()%(stDispInPortAttr.u16SrcHeight),2));
            tmpc_w = MAX(crop_w,(ALIGN_DOWN(rand()%(stDispInPortAttr.u16SrcWidth - tmpc_x),2)));
            tmpc_h = MAX(crop_h,(ALIGN_DOWN(rand()%(stDispInPortAttr.u16SrcHeight - tmpc_y),2)));
            stDispWinRect.u16X = tmpc_x;
            stDispWinRect.u16Y = tmpc_y;
            stDispWinRect.u16Width = tmpc_w;
            stDispWinRect.u16Height = tmpc_h;
            MI_DISP_SetZoomInWindow(0,0,&stDispWinRect);
            cus_print("loop_cnt:%d crop_min(%d_%d), crop(%d_%d_%d_%d)\n",loop_cnt,crop_w,crop_h,tmpc_x,tmpc_y,tmpc_w,tmpc_h);

            usleep(1000*1000*3);
        }
    }
#endif

#if (ENABLE_GAMMA_TEST == 1)
    {
        char buf[256];
        fd_set stFdSet;
        MI_DISP_GammaParam_t stGammaParam;

        FD_ZERO(&stFdSet);
        FD_SET(0,&stFdSet);
        for(;;)
        {
            select(1, &stFdSet, NULL, NULL, NULL);
            if(FD_ISSET(0, &stFdSet))
            {
                int i = read(0, buf, sizeof(buf));
                if(i>0)
                {
                    if(buf[0] == 'q')
                        break;
                    if(buf[0] == '0')
                    {
                        stGammaParam.bEn = TRUE;
                        stGammaParam.u16EntryNum = sizeof(tnormalGammaR);
                        stGammaParam.pu8ColorR = tnormalGammaR;
                        stGammaParam.pu8ColorG = tnormalGammaG;
                        stGammaParam.pu8ColorB = tnormalGammaB;
                        MI_DISP_DeviceSetGammaParam(0, &stGammaParam);
                    }
                    else if(buf[0] == '1')
                    {
                        stGammaParam.bEn = TRUE;
                        stGammaParam.u16EntryNum = sizeof(tcustomerGammaR);
                        stGammaParam.pu8ColorR = tcustomerGammaR;
                        stGammaParam.pu8ColorG = tcustomerGammaG;
                        stGammaParam.pu8ColorB = tcustomerGammaB;
                        MI_DISP_DeviceSetGammaParam(0, &stGammaParam);
                    }
                    else
                        printf("param is invalid\n");
                }
                else
                    printf("input is invalid\n");
            }
        }
    }
#endif

    return 0;
}

#if (defined INTERFACE_VDEC) && (INTERFACE_VDEC == 1)
static MI_BOOL disp_ut_002(MI_U8 u8ChnNum) //vdec_disp
{
    MI_DISP_DEV DispDev = 0;
    MI_U8 u8PortId = 0;
    MI_U8 u8ChnIdx = 0;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_BOOL bCreateGetThead = TRUE;

    memset(g_thrPush,    0x0, sizeof(g_thrPush));
    memset(g_thrGet,     0x0, sizeof(g_thrGet));
    memset(g_bRun,       0x0, sizeof(g_bRun));
    memset(&stVdecChnAttr, 0x0, sizeof(MI_VDEC_ChnAttr_t));

    MI_SYS_Init();

    if(g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_HDMI ||
            g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_VGA)
    {
#if (defined INTERFACE_HDMI) && (INTERFACE_HDMI == 1)
        MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
        MI_HDMI_InitParam_t stInitParam;
        MI_HDMI_TimingType_e eHdmiTiming;
        memset(&stInitParam, 0, sizeof(MI_HDMI_InitParam_t));
        stInitParam.pCallBackArgs = NULL;
        stInitParam.pfnHdmiEventCallback = Hdmi_callback;
        MI_HDMI_Init(&stInitParam);
        MI_HDMI_Open(eHdmi);
        eHdmiTiming = gethdmitiming(g_astDispUtDev[DispDev].stPubAttr.eIntfSync);
        Hdmi_Start(eHdmi,eHdmiTiming);
#endif
    }
    else if(g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD)
    {
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vact = stPanelParam.u16Height;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth + stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hact = stPanelParam.u16Width;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth + stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvact = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvbb = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvfb = 0;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
        g_astDispUtDev[DispDev].stPubAttr.stSyncInfo.u32FrameRate = stPanelParam.u16DCLK*1000000/(stPanelParam.u16HTotal*stPanelParam.u16VTotal);
        g_astDispUtDev[DispDev].stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
        g_astDispUtDev[DispDev].stPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    }
    //set disp pub
    disp_ut_setdev(DispDev);
    //set layer
    disp_ut_setlayer(DispDev, 0);
    //set inputport
    for(u8ChnIdx = 0; u8ChnIdx < u8ChnNum; u8ChnIdx++)
    {
        u8PortId = u8ChnIdx;
        disp_ut_setport(0, u8PortId);
    }

    //set panel config
    if((g_astDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD) &&
            ((g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_LVDS)
            || (g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
            || (g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_TTL)))
    {
        //init_panel_LX50FWB();
        MI_PANEL_Init(g_astDispUtDev[DispDev].eLinkType);
        MI_PANEL_SetPanelParam(&stPanelParam);
        if(g_astDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
        {
            MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
        }
        g_astDispUtDev[DispDev].bPanelInit = TRUE;
    }

    for(u8ChnIdx = 0; u8ChnIdx < u8ChnNum; u8ChnIdx++)
    {
        stVdecChnAttr.eCodecType =g_stVdecParam[u8ChnIdx].eCodecType;
        stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 1;
        stVdecChnAttr.eVideoMode = g_stVdecParam[u8ChnIdx].eVideoMode;
        stVdecChnAttr.u32BufSize = 1 * 1024 * 1024;
        stVdecChnAttr.u32PicWidth = g_stVdecParam[u8ChnIdx].u32VdecSrcWidth;
        stVdecChnAttr.u32PicHeight = g_stVdecParam[u8ChnIdx].u32VdecSrcHeight;
        stVdecChnAttr.eDpbBufMode = g_stVdecParam[u8ChnIdx].eDpbBufMode;
        stVdecChnAttr.u32Priority = 0;
        MI_VDEC_CreateChn(u8ChnIdx, &stVdecChnAttr);
        MI_VDEC_StartChn(u8ChnIdx);

        MI_VDEC_OutputPortAttr_t stOutputPortAttr;
        stOutputPortAttr.u16Width = g_stVdecParam[u8ChnIdx].u32VdecDstWidth;
        stOutputPortAttr.u16Height = g_stVdecParam[u8ChnIdx].u32VdecDstHeight;
        MI_VDEC_SetOutputPortAttr(u8ChnIdx, &stOutputPortAttr);

        g_stVdecParam[u8ChnIdx].bCreate = TRUE;

        stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stSrcChnPort.u32DevId = 0;
        stSrcChnPort.u32ChnId = u8ChnIdx;
        stSrcChnPort.u32PortId = 0;
        MI_SYS_SetChnOutputPortDepth(&stSrcChnPort,0,4);

        stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stDstChnPort.u32DevId = 0;
        stDstChnPort.u32ChnId = u8ChnIdx;
        stDstChnPort.u32PortId = 0;
        MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, 30, 30);

        create_push_stream_thread(u8ChnIdx, bCreateGetThead, &stVdecChnAttr, g_stVdecParam[u8ChnIdx].esfilepath);
    }
#if (ENABLE_CAPTURE_TEST == 1)
    {
        MI_U32 u32Index = 0;
        usleep(1000*200);
        while(u32Index <= 1000)
        {
            CaptureCreate(0);
            CaptureGetStream(0);
            CaptureDestroy(0);
            u32Index++;
            usleep(1000*100);
        }
    }
#endif

    return 0;
}
#endif

static MI_U32 disp_ut_portshowsize(MI_U8 u8ChnNum, MI_DISP_OutputTiming_e eOutputTiming)
{
    MI_U8 n = 0;
    MI_U8 u8Factor = 4;
    MI_U8 u8LayerId = 0;
    MI_U8 u8PortId = 0;
    MI_U32 u32PortOutWidth = 0;
    MI_U32 u32PortOutHeight = 0;
    MI_U32 u32LayerDispWidth = 0;
    MI_U32 u32LayerDispHeight = 0;

    GetLayerDisplaySize(eOutputTiming, &u32LayerDispWidth, &u32LayerDispHeight);

    if(/*u8ChnNum > DISP_INPUT_PORT_MAX ||*/ u8ChnNum == 0)
    {
        printf("port num is invalid\n");
        return E_MI_ERR_FAILED;
    }
    if(u8ChnNum == 1)
    {
        u32PortOutWidth = u32LayerDispWidth;
        u32PortOutHeight = u32LayerDispHeight;
        u8Factor = 1;
    }
    else if(u8ChnNum <= 4)
    {
        u32PortOutWidth = u32LayerDispWidth/2;
        u32PortOutHeight = u32LayerDispHeight/2;
        u8Factor = 2;
    }
    else if(u8ChnNum <= 9)
    {
        u32PortOutWidth = u32LayerDispWidth/3;
        u32PortOutHeight = u32LayerDispHeight/3;
        u8Factor = 3;
    }
    else
    {
        u32PortOutWidth = u32LayerDispWidth/4;
        u32PortOutHeight = u32LayerDispHeight/4;
        u8Factor = 4;
    }
    for(n = 0; n < u8ChnNum; n++)
    {
        u8PortId = n;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerSize.u16Height = u32LayerDispHeight;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerSize.u16Width = u32LayerDispWidth;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerDispWin.u16X = 0;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerDispWin.u16Y = 0;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerDispWin.u16Width = u32LayerDispWidth;
        g_astDispUtLayer[0].stLayerAttr.stVidLayerDispWin.u16Height = u32LayerDispHeight;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16X = (n%u8Factor)*u32PortOutWidth;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Y = ((n/u8Factor)% u8Factor)*u32PortOutHeight;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Width =(n>=16) ?  u32PortOutWidth<<1 : u32PortOutWidth;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Height = (n>=16) ?  u32PortOutHeight<<1 : u32PortOutHeight;
     }
    return u8ChnNum;
}

static MI_BOOL disp_ut_changetiming(MI_U8 u8ChnNum)
{
    MI_U32 n;
    char usercmd[256];
    MI_U8 u8DevIndex = 0;
    MI_U8 u8LayerIndex;
    MI_U8 u8PortIndex;
    MI_U8 u8PortNum = u8ChnNum;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_DISP_PubAttr_t stPubAttr;

    memcpy(&stPubAttr, &g_astDispUtDev[u8DevIndex].stPubAttr, sizeof(MI_DISP_PubAttr_t));

    for(u8PortIndex = 0; u8PortIndex < u8PortNum; u8PortIndex++)
    {
        MI_U8 u8ChnIdx = u8PortIndex;
        g_PushDataStop[0][u8PortIndex] = TRUE;
#if (defined INTERFACE_VDEC) && (INTERFACE_VDEC == 1)
        if(TRUE == g_stVdecParam[u8ChnIdx].bCreate){
            MI_VDEC_StopChn(u8ChnIdx);
        }
#endif
        MI_DISP_DisableInputPort(0, u8PortIndex);
    }
    MI_DISP_Disable(u8DevIndex);

    cus_print("please select output timing\n");
    for(n = 0; n < sizeof(astTimingArray)/sizeof(stTimingArray_t); n++)
    {
        cus_print("%d:%s\n", n, astTimingArray[n].desc); //list valid output timing
    }
    fgets(usercmd, sizeof(usercmd), stdin);
    n = atoi(usercmd);
    stPubAttr.eIntfSync = astTimingArray[n].eOutputTiming;
    MI_DISP_SetPubAttr(u8DevIndex, &stPubAttr);
#if (defined INTERFACE_HDMI) && (INTERFACE_HDMI == 1)
    if(E_MI_DISP_INTF_HDMI == stPubAttr.eIntfType){
        stPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
        MI_DISP_SetPubAttr(u8DevIndex,  &stPubAttr);
    }
    else if(E_MI_DISP_INTF_VGA == stPubAttr.eIntfType){
        Hdmi_Start(eHdmi,astTimingArray[n].eHdmiTiming);
        stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
        MI_DISP_SetPubAttr(u8DevIndex,  &stPubAttr);
    }
#endif
    MI_DISP_Enable(u8DevIndex);

    disp_ut_portshowsize(u8PortNum, astTimingArray[n].eOutputTiming);
    for(u8PortIndex = 0; u8PortIndex < u8PortNum; u8PortIndex++)
    {
        MI_U8 u8ChnIdx = u8PortIndex;
        MI_DISP_InputPortAttr_t stDispInPortAttr;
        memcpy(&stDispInPortAttr, &g_astDispUtPort[0][u8PortIndex].stInputPortAttr, sizeof(MI_DISP_InputPortAttr_t));

#if (defined INTERFACE_HDMI) && (INTERFACE_HDMI == 1)
        if(TRUE == g_stVdecParam[u8ChnIdx].bCreate){
            MI_VDEC_OutputPortAttr_t stVdecOutPortAttr;
            stVdecOutPortAttr.u16Width = ALIGN_DOWN(stDispInPortAttr.stDispWin.u16Width,2);
            stVdecOutPortAttr.u16Height = ALIGN_DOWN(stDispInPortAttr.stDispWin.u16Height,2);
            MI_VDEC_SetOutputPortAttr(u8ChnIdx, &stVdecOutPortAttr);
            MI_VDEC_StartChn(u8ChnIdx);
            stDispInPortAttr.u16SrcWidth = stVdecOutPortAttr.u16Width;
            stDispInPortAttr.u16SrcHeight = stVdecOutPortAttr.u16Height;
        }
#endif
        MI_DISP_SetInputPortAttr(0, u8PortIndex, &stDispInPortAttr);
        MI_DISP_EnableInputPort(0, u8PortIndex);
        MI_DISP_SetInputPortSyncMode(0, u8PortIndex, E_MI_DISP_SYNC_MODE_FREE_RUN);
        g_PushDataStop[0][u8PortIndex] = FALSE;
    }
    return MI_SUCCESS;
}


static MI_DISP_OutputTiming_e parse_outputtiming(char *str)
{
    MI_DISP_OutputTiming_e eOutputTiming;

    int timingNum = sizeof(astTimingArray)/sizeof(stTimingArray_t);
    int i = 0;
    for(i = 0;i<timingNum;i++)
    {
        if(strcasecmp(str,astTimingArray[i].desc)==0)
        {
            return astTimingArray[i].eOutputTiming;
        }
    }
    printf("using default outputtiming 1080p60\n");
    return E_MI_DISP_OUTPUT_1080P60;
}
static MI_SYS_PixelFormat_e parse_pixformat(char *str)
{
    MI_SYS_PixelFormat_e epixformat;

    if(strcmp(str, "yuv422") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    }
    else if(strcmp(str,"yuv420") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    }
    else if(strcmp(str,"mst420") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
    }
    else
    {
        printf("using default pixformat\n");
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    }
    return epixformat;
}
static MI_PANEL_LinkType_e parse_linktype(char *str)
{
    MI_PANEL_LinkType_e eLinkType;

    if(strcmp(str, "ttl") == 0)
    {
        eLinkType = E_MI_PNL_LINK_TTL;
    }
    else if(strcmp(str, "mipi") == 0)
    {
        eLinkType = E_MI_PNL_LINK_MIPI_DSI;
    }
    else if(strcmp(str, "lvds") == 0)
    {
        eLinkType = E_MI_PNL_LINK_LVDS;
    }
    else
    {
        eLinkType = E_MI_PNL_LINK_TTL;
        printf("using default interface\n");
    }
    return eLinkType;
}

static MI_DISP_Interface_e parse_interface(char *str)
{
    MI_DISP_Interface_e eInterface;

    if(strcmp(str, "hdmi") == 0)
    {
        eInterface = E_MI_DISP_INTF_HDMI;
    }
    else if(strcmp(str, "vga") == 0)
    {
        eInterface = E_MI_DISP_INTF_VGA;
    }
    else if(strcmp(str, "lcd") == 0)
    {
        eInterface = E_MI_DISP_INTF_LCD;
    }
    else
    {
        eInterface = E_MI_DISP_INTF_HDMI;
        printf("using default interface\n");
    }
    return eInterface;
}

static MI_DISP_RotateMode_e parse_rotate(char *str)
{
    MI_DISP_RotateMode_e eRotateMode;

    if(strcmp(str, "0") == 0)
    {
        eRotateMode = E_MI_DISP_ROTATE_NONE;
    }
    else if(strcmp(str, "90") == 0)
    {
        eRotateMode = E_MI_DISP_ROTATE_90;
    }
    else if(strcmp(str, "180") == 0)
    {
        eRotateMode = E_MI_DISP_ROTATE_180;
    }
    else if(strcmp(str, "270") == 0)
    {
        eRotateMode = E_MI_DISP_ROTATE_270;
    }
    else
    {
        eRotateMode = E_MI_DISP_ROTATE_NONE;
        printf("using default interface\n");
    }
    return eRotateMode;
}

static int parse_optarg(char *optarg,char* args[])
{
    char *token;
    int i = 0;

    token = strtok(optarg, "_");
    while(token != NULL){
        args[i++] = token;
        token = strtok(NULL, "_");
    }

    return i;
}

static struct option long_options[] = {
    {"pip",         no_argument,            0,  '0'},
    {"interface",   required_argument,      0,  '1'},
    {0,0,0,0}
};

static void help(void)
{
    printf("******************************* usage **************************************\n");
    printf("./disp_ut [options]\n");
    printf("options:\n");
    printf("--interface   lcd(show on panel) hdmi or vga\n");
    printf("-l            if --interface=lcd, -l specify panel interface type(ttl mipi or lvds)\n");
    printf("-s            if --interface=hdmi or vga, -s specify output timing such as 1080p60 720p60 1024x768_60\n");
    printf("-f            source file path\n");
    printf("-t            source file format(yuv420 yuv422 h264 or h265)\n");
    printf("-n            chnnum or num of windows to be used\n");
    printf("-i            input frame size\n");
    printf("-c            disp crop parameters of each port\n");
    printf("-o            disp output size of each port,if not set,output will be as big as the panel size\n");
    printf("-r            disp rotation angle(90 or 270) of each layer\n");
    printf("--pip         open pip function\n");
    printf("Examples:\n");
    printf("./disp_ut --interface lcd -l ttl -f ./YUV420SP_800_480.yuv -t yuv420 -n 1 -i 800_480 -c 0_0_800_480 -o 0_0_800_480\n");
    printf("./disp_ut --interface hdmi -s 1080p60 -f /mnt/esfile/h264_720_576.es -t h264 -n 16 -i 720_576 -d 480_270 -c 0_0_480_270\n");
    printf("./disp_ut --interface vga -s 1024x768_60 -f /mnt/esfile/720P25.h265 -t h265 -n 4 -i 1280_720 -d 512_384 -c 0_0_512_384\n");
    printf("****************************************************************************\n");
}
int main(int argc, char **argv)
{
    MI_BOOL ret;
    int opt = 0;
    int option_index = 0;
    char filepath[256];
    char filetype[256];
    char buf[50];
    fd_set stFdSet;
    MI_DISP_Interface_e eInterface = E_MI_DISP_INTF_LCD;
    MI_DISP_OutputTiming_e eOutputTiming = E_MI_DISP_OUTPUT_USER;
    MI_DISP_RotateMode_e eRotateMode = E_MI_DISP_ROTATE_NONE;
    MI_SYS_PixelFormat_e ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    MI_PANEL_LinkType_e eLinkType;
    MI_U8 u8LayerId = 0;
    MI_U8 u8PortId = 0;
    MI_U8 u8ChnNum = 0;
    MI_U32 u32SrcWidth = 0;
    MI_U32 u32SrcHeight = 0;
    MI_U32 u32Crop_x = 0;
    MI_U32 u32Crop_y = 0;
    MI_U32 u32Crop_w = 0;
    MI_U32 u32Crop_h = 0;
    MI_U32 u32Show_x = 0;
    MI_U32 u32Show_y = 0;
    MI_U32 u32Show_w = 0;
    MI_U32 u32Show_h = 0;
    MI_U32 u32Dec_w = 0;
    MI_U32 u32Dec_h = 0;
    MI_U8 u8DevIndex = 0;
    MI_U8 u8LayerIndex = 0;
    MI_U8 u8PortIndex = 0;
    MI_U8 u8ChnIndex = 0;

    memset(g_astDispUtDev, 0, sizeof(g_astDispUtDev));
    memset(g_astDispUtLayer, 0, sizeof(g_astDispUtLayer));
    memset(g_astDispUtPort, 0, sizeof(g_astDispUtPort));

    if(argc<=1){
        help();
        return -1;
    }
    while ((opt = getopt_long(argc, argv, "l:s:f:t:n:i:c:d:o:r:", long_options, &option_index)) != -1) {
        int cnt;
        char *token;
        char* args[50];
        switch(opt){
            case 'l':
                eLinkType = parse_linktype(optarg);
                break;
            case 's':
                eOutputTiming = parse_outputtiming(optarg);
                break;
            case 'f':
                strcpy(filepath,optarg);
                break;
            case 't':
                strcpy(filetype,optarg);
                break;
            case 'n':
                u8ChnNum = atoi(optarg);
                break;
            case 'i':
                cnt = parse_optarg(optarg,args);
                if(cnt == 2){
                    u32SrcWidth = atoi(args[0]);
                    u32SrcHeight = atoi(args[1]);
                }
                else{
                    cus_print("-i optarg invalid\n");
                    return -1;
                }
                break;
            case 'c':
                cnt = parse_optarg(optarg,args);
                if(cnt == 4){
                    u32Crop_x = atoi(args[0]);
                    u32Crop_y = atoi(args[1]);
                    u32Crop_w = atoi(args[2]);
                    u32Crop_h = atoi(args[3]);
                }
                else{
                    cus_print("-c optarg invalid\n");
                    return -1;
                }
                break;
            case 'd':
                cnt = parse_optarg(optarg,args);
                if(cnt == 2){
                    u32Dec_w = atoi(args[0]);
                    u32Dec_h = atoi(args[1]);
                }
                else{
                    cus_print("-d optarg invalid\n");
                    return -1;
                }
                break;
            case 'o':
                cnt = parse_optarg(optarg,args);
                if(cnt == 4){
                    u32Show_x = atoi(args[0]);
                    u32Show_y = atoi(args[1]);
                    u32Show_w = atoi(args[2]);
                    u32Show_h = atoi(args[3]);
                }
                else{
                    cus_print("-o optarg invalid\n");
                    return -1;
                }
                break;
            case 'r':
                eRotateMode = parse_rotate(optarg);
                break;
            case '0':
                g_bPipEnable = 1;
                break;
            case '1':
                eInterface = parse_interface(optarg);
                break;
            default:
                help();
                return -1;
        }
    }

    cus_print("running\n");

    if (optind != argc) {
        fprintf(stderr, "Expected argument after options\n");
    }

    g_astDispUtDev[0].stPubAttr.eIntfSync = eOutputTiming;
    g_astDispUtDev[0].stPubAttr.eIntfType = eInterface;
    g_astDispUtDev[0].eLinkType = eLinkType;
    g_astDispUtLayer[0].eRotateMode = eRotateMode;
    disp_ut_portshowsize(u8ChnNum, eOutputTiming);

    for(u8ChnIndex = 0; u8ChnIndex < u8ChnNum; u8ChnIndex++){
        u8PortId = u8ChnIndex;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcWidth = u32SrcWidth;
        g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcHeight = u32SrcHeight;

        if(u32Show_x || u32Show_y || u32Show_w || u32Show_h){
            g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16X = u32Show_x;
            g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Y = u32Show_y;
            g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Width = u32Show_w;
            g_astDispUtPort[0][u8PortId].stInputPortAttr.stDispWin.u16Height = u32Show_h;
        }
        g_astDispUtPort[0][u8PortId].stCropWin.u16X = u32Crop_x;
        g_astDispUtPort[0][u8PortId].stCropWin.u16Y = u32Crop_y;
        g_astDispUtPort[0][u8PortId].stCropWin.u16Width = u32Crop_w?u32Crop_w:u32SrcWidth;
        g_astDispUtPort[0][u8PortId].stCropWin.u16Height = u32Crop_h?u32Crop_h:u32SrcHeight;
        g_astDispUtPort[0][u8PortId].ePixFormat = ePixelFormat;
        strncpy(g_astDispUtPort[0][u8PortId].FilePath,filepath,sizeof(g_astDispUtPort[0][u8PortId].FilePath));
    }
    if((strcmp(filetype, "h264") == 0) || (strcmp(filetype, "h265") == 0)){
        for(u8ChnIndex = 0; u8ChnIndex < u8ChnNum; u8ChnIndex++){
            u8PortId = u8ChnIndex;
            if(strcmp(filetype, "h264") == 0){
                g_stVdecParam[u8ChnIndex].eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
            }
            else
                g_stVdecParam[u8ChnIndex].eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
            g_stVdecParam[u8ChnIndex].eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
            g_stVdecParam[u8ChnIndex].eDpbBufMode = E_MI_VDEC_DPB_MODE_NORMAL;
            g_stVdecParam[u8ChnIndex].u32VdecSrcWidth = u32SrcWidth;
            g_stVdecParam[u8ChnIndex].u32VdecSrcHeight = u32SrcHeight;
            g_stVdecParam[u8ChnIndex].u32VdecDstWidth = u32Dec_w;
            g_stVdecParam[u8ChnIndex].u32VdecDstHeight = u32Dec_h;
            strncpy(g_stVdecParam[u8ChnIndex].esfilepath,filepath,sizeof(g_stVdecParam[u8ChnIndex].esfilepath));
            g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcWidth = u32Dec_w;
            g_astDispUtPort[0][u8PortId].stInputPortAttr.u16SrcHeight = u32Dec_h;
        }
#if (defined INTERFACE_VDEC) && (INTERFACE_VDEC == 1)
        ret = disp_ut_002(u8ChnNum);
#endif
    }
    else
        ret = disp_ut_001(u8ChnNum);
    if(ret){
        cus_print("function exec failed\n");
        return -1;
    }

    FD_ZERO(&stFdSet);
    FD_SET(0,&stFdSet);
    for(;;)
    {
        cus_print("input 'q' exit\ninput '1' changetiming\n");
        select(1, &stFdSet, NULL, NULL, NULL);
        if(FD_ISSET(0, &stFdSet))
        {
            int i = read(0, buf, sizeof(buf));
            if(i>0 && (buf[0] == 'q'))
            {
                break;
            }
            if(i>0 && (buf[0] == '1'))
            {
                disp_ut_changetiming(u8ChnNum);
            }
        }
    }

#if (defined INTERFACE_VDEC) && (INTERFACE_VDEC == 1)
    for(u8ChnIndex = 0; u8ChnIndex < u8ChnNum+1; u8ChnIndex++)
    {
        if(g_stVdecParam[u8ChnIndex].bCreate == TRUE)
        {
            destroy_push_stream_thread(u8ChnIndex);
            MI_VDEC_StopChn(u8ChnIndex);
            MI_VDEC_DestroyChn(u8ChnIndex);
            g_stVdecParam[u8ChnIndex].bCreate = FALSE;
            cus_print("destroy vdec chn:%d\n",u8ChnIndex);
        }
    }
#endif
    for(u8DevIndex = 0; u8DevIndex < DISP_DEV_MAX; u8DevIndex++)
    {
        if(g_astDispUtDev[u8DevIndex].bDevEnable == TRUE)
        {
            for(u8LayerIndex = 0; u8LayerIndex < DISP_LAYER_MAX; u8LayerIndex++)
            {
                if(g_astDispUtLayer[u8LayerIndex].bLayerEnable == TRUE)
                {
                    for(u8PortIndex = 0; u8PortIndex < DISP_INPUT_PORT_MAX; u8PortIndex++)
                    {
                        if(g_astDispUtPort[u8LayerIndex][u8PortIndex].bPortEnable == TRUE)
                        {
                            g_PushDataExit[u8LayerIndex][u8PortIndex] = TRUE;
                            if(gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt)
                            {
                                pthread_join(gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt,NULL);
                                gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt = 0;
                            }
                            disp_ut_disableport(u8LayerIndex, u8PortIndex);
                            cus_print("disable layerid:%d portid:%d\n",u8LayerIndex,u8PortIndex);
                        }
                    }
                    disp_ut_disablelayer(u8DevIndex, u8LayerIndex);
                    cus_print("disable layerid:%d\n",u8LayerIndex);
                }
            }
            disp_ut_disabledev(u8DevIndex);
            cus_print("disable devid:%d\n",u8DevIndex);
        }
        if(g_astDispUtDev[u8DevIndex].bPanelInit == TRUE)
        {
            MI_PANEL_DeInit();
            cus_print("deinit panel\n");
        }
    }
    MI_SYS_Exit();

    printf("--------EXIT--------\n");
    return 0;
}

