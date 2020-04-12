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
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <poll.h>

#include "st_common.h"
#include "st_vif.h"
#include "st_vpe.h"
#include "st_venc.h"
#include "st_rgn.h"
#include "st_ao.h"


#include "dot_matrix_font.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "Live555RTSPServer.hh"

#include "mi_rgn.h"
#include "mi_divp.h"

#include "mi_od.h"
#include "mi_md.h"

#include "mi_vdf.h"
//#include "mi_ao.h"
#include "mi_isp.h"
#include "mi_iqserver.h"

#include "onvif_server.h"

#include "linux_list.h"

#define RTSP_LISTEN_PORT        554
#define MAIN_STREAM                "main_stream"
#define SUB_STREAM0                "sub_stream0"
#define SUB_STREAM1                "sub_stream1"

#define PATH_PREFIX                "/mnt"
#define DEBUG_ES_FILE            0

#define RGN_OSD_TIME_START        8
#define RGN_OSD_MAX_NUM         4
#define RGN_OSD_TIME_WIDTH        180
#define RGN_OSD_TIME_HEIGHT        32

#define DOT_FONT_FILE            "/config/mi_demo/gb2312.hzk"

#define MAX_CAPTURE_NUM            4
#define CAPTURE_PATH            "/mnt/capture"


#define USE_AUDIO                 0
#define MI_AUDIO_SAMPLE_PER_FRAME 768
#define AO_INPUT_FILE            "8K_16bit_STERO_30s.wav"
#define AO_OUTPUT_FILE            "./tmp.pcm"

#define DIVP_CHN_FOR_OSD        0
#define DIVP_CHN_FOR_DLA        1
#define DIVP_CHN_FOR_VDF        2
#define DIVP_CHN_FOR_ROTATION    3
#define DIVP_CHN_FOR_SCALE        3
#define VENC_CHN_FOR_CAPTURE    12

#define SCALE_TEST    0
#define USE_STREAM_FILE 0
#define DIVP_CHN_FOR_RESOLUTION 0
#define DIVP_SCALE_IPNUT_FILE    "vpe0_port0_1920x1088_0000.yuv"

#define MAX_CHN_NEED_OSD        4

#define RAW_W                     384
#define RAW_H                     288
#define VDF_BASE_WIDTH            3840
#define VDF_BASE_HEIGHT            2160
#define SHOW_PANEL 0
#define PANEL_DIVP_ROTATE        0

#define ENABLE_INTELL_VENC			1

#define MI_FAIL     1

#define RGB_TO_CRYCB(r, g, b)                                                            \
        (((unsigned int)(( 0.439f * (r) - 0.368f * (g) - 0.071f * (b)) + 128.0f)) << 16) |    \
        (((unsigned int)(( 0.257f * (r) + 0.564f * (g) + 0.098f * (b)) + 16.0f)) << 8) |        \
        (((unsigned int)((-0.148f * (r) - 0.291f * (g) + 0.439f * (b)) + 128.0f)))

#define IQ_FILE_PATH    "/config/iqfile/day_wdr.bin"

typedef void (*ST_UserCmdProcess)(void *args);

#if (SHOW_PANEL == 1)
#include "mi_disp.h"
#include "mi_panel.h"
//#include "st_spi.h"
//#include "spi_cmd_480x854.h"
//#include "LX50FWB4001_RM68172_480x854.h"
//#include "AT070TN94_800x480.h"

#define ST_PANEL_WIDTH                480
#define ST_PANEL_HEIGHT                854
#endif

#define MAX_RGN_COVER_W               8192
#define MAX_RGN_COVER_H               8192

#define RGN_OSD_HANDLE                	0
#define RGN_FOR_VDF_BEGIN				12

#define MAX_FULL_RGN_NULL				3

#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#else
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif
struct ST_Stream_Attr_T
{
    MI_BOOL        bEnable;
    ST_Sys_Input_E enInput;
    MI_U32     u32InputChn;
    MI_U32     u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_VENC_RcMode_e eRcMode;
    float     f32Mbps;
    MI_U32    u32Fps;
    MI_U32    u32Gop;
    MI_U32    u32Width;
    MI_U32     u32Height;
    MI_U32    u32CropX;
    MI_U32    u32CropY;
    MI_U32    u32CropWidth;
    MI_U32     u32CropHeight;

    MI_U32 enFunc;
    const char    *pszStreamName;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindPara;
    MI_U32         u32Cover1Handle;
    MI_U32         u32Cover2Handle;
};

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e enType;
    char szStreamName[64];

    MI_BOOL bWriteFile;
    int fd;
    char szDebugFile[128];
} ST_StreamInfo_T;

typedef struct
{
    MI_VENC_CHN vencChn;
    pthread_t pt;
    MI_BOOL bRun;
} ST_CatpureJPGInfo_T;

typedef struct
{
    pthread_t pt;
    MI_BOOL bRun;

    ST_OSD_INFO_S stOsdInfo[MAX_CHN_NEED_OSD][MAX_OSD_NUM_PER_CHN];
} ST_RGN_Osd_T;

typedef struct
{
    MI_S32 s32UseOnvif;     //0:not use, else use
    MI_S32 s32UseVdf;         // 0: not use, else use
#if USE_AUDIO
    MI_S32 s32UseAudio;        // 0: not use, else use
#endif
    MI_S32 s32LoadIQ;        // 0: not load, else load IQ file
#if (SHOW_PANEL == 1)
    MI_S32 s32UsePanel;        // 0: not use, else use
    MI_SYS_PixelFormat_e enPixelFormat;
    MI_S32 s32Rotate;        // 0: not rotate, else rotate
#endif
    MI_S32 s32HDRtype;
    ST_Sensor_Type_T enSensorType;
    MI_SYS_Rotate_e enRotation;
    MI_VPE_3DNR_Level_e en3dNrLevel;
} ST_Config_S;

typedef struct
{
    char szDesc[256];
    ST_UserCmdProcess process;
    void *args;
    int cmdID;

    struct list_head list;
} ST_Input_Cmd_S;

typedef struct
{
    int count;

    struct list_head head;
} ST_InputCmd_Mng_S;

typedef struct
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ModId_e eModId;
    MI_U32 u32Chn;
    MI_U32 u32Port;
} ST_VDF_OSD_Info_T;

typedef struct _WavHeader_s
{
    MI_U8    riff[4];                // RIFF string
    MI_U32    ChunkSize;                // overall size of file in bytes
    MI_U8    wave[4];                // WAVE string
    MI_U8    fmt_chunk_marker[4];    // fmt string with trailing null char
    MI_U32    length_of_fmt;            // length of the format data
    MI_U16    format_type;            // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    MI_U16    channels;                // no.of channels
    MI_U32    sample_rate;            // sampling rate (blocks per second)
    MI_U32    byterate;                // SampleRate * NumChannels * BitsPerSample/8
    MI_U16    block_align;            // NumChannels * BitsPerSample/8
    MI_U16    bits_per_sample;        // bits per sample, 8- 8bits, 16- 16 bits etc
    MI_U8    data_chunk_header [4];    // DATA string or FLLR string
    MI_U32    data_size;                // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} _WavHeader_t;

typedef struct _AoOutChn_s
{

    pthread_t pt;
    MI_BOOL bRunFlag;

    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    int fdWr;
    MI_BOOL bEndOfStream;
    MI_U32 u32InputFileSize;
    MI_U32 u32WriteTotalSize;
    MI_U32 u32InSampleRate;
    MI_U32 u32OutSampleRate;
} _AoOutChn_t;

typedef struct ST_MDOD_Area_s
{
    MI_U32 u32Chn;
    ST_Rect_T stArea;
} ST_MDOD_Area_T;

typedef struct
{
    MI_U16 u16VdfInWidth;
    MI_U16 u16VdfInHeight;
    MI_U16 u16stride;

    MI_U16 u16OdNum;            // od chn num
    MI_U16 u16MdNum;            // md chn num
    MI_U16 u16VgNum;            // vg chn num
    ST_MDOD_Area_T stOdArea[4];         // od detect area
    ST_MDOD_Area_T stMdArea[4];         // md detect area
    ST_MDOD_Area_T stVgArea[4];         // vg detect area
} ST_VdfChnArgs_t;

typedef struct
{
	pthread_t pt;
	MI_BOOL bRunFlag;

	MI_U32 u32Chn;
	MI_U32 u32Port;

	MI_VENC_CHN VencChn;
    MI_VENC_ModType_e eModType;
    MI_VENC_RcMode_e eRcMode;
	MI_U32 u32Width;
	MI_U32 u32Height;
    MI_U32 u32BitRate;
} VENC_SendBuf_Args_S;

struct ST_VdfSetting_Attr_T
{
    ST_Sys_Input_E enInput;
    MI_U32  u32InputChn;
    MI_U32  u32InputPort;
    ST_VdfChnArgs_t stVdfArgs;
};

static ST_InputCmd_Mng_S g_inputCmdMng =
{
    .count = 0,
    .head = LIST_HEAD_INIT(g_inputCmdMng.head)
};

#if 0
typedef struct
{
    pthread_t pt;
    MI_BOOL bRunFlag;

    int fdRd;
    MI_AUDIO_DEV AoDevId;
    MI_AO_CHN AoChn;
    MI_U32 u32ChnCnt;
    MI_AUDIO_BitWidth_e eBitwidth; /*audio frame bitwidth*/
    MI_AUDIO_SoundMode_e eSoundmode; /*audio frame momo or stereo mode*/
    MI_AUDIO_SampleRate_e eInSampleRate;
    MI_AUDIO_SampleRate_e eSamplerate;
} ST_AoInInfo_T;
#endif

typedef struct
{
    pthread_t pt;
    MI_VDF_CHANNEL vdfChn;
    MI_BOOL bRunFlag;
    MI_VDF_WorkMode_e enWorkMode;
    MI_U16 u16Width;
    MI_U16 u16Height;
} VDF_Thread_Args_t;

VDF_Thread_Args_t g_stVdfThreadArgs[16];
static Live555RTSPServer *g_pRTSPServer = NULL;
static ST_RGN_Osd_T g_stRgnOsd;
static MI_BOOL g_bExit = FALSE;
MI_BOOL bEnableRes = FALSE;
MI_BOOL bEnableVqe = FALSE;
MI_U32 g_u32CapWidth = 0;
MI_U32 g_u32CapHeight = 0;
ST_Config_S g_stConfig;
ST_VDF_OSD_Info_T g_stVDFOsdInfo[MAX_FULL_RGN_NULL];
static VENC_SendBuf_Args_S g_stVENCSendBuf[4];
static ST_Rect_T g_stRect[(RAW_W / 4) * (RAW_H / 4)] = {{0,},};
static MI_S32 g_md_detect_cnt = 0;
static pthread_mutex_t g_st_md_rect_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct ST_VdfSetting_Attr_T g_stVdfSettingAttr[] =
{
    {
        .enInput = ST_Sys_Input_DIVP,
        .u32InputChn = DIVP_CHN_FOR_VDF,
        .u32InputPort = 0,
        .stVdfArgs =
        {
            .u16VdfInWidth = 384,
            .u16VdfInHeight = 288,
            .u16stride = 384,
            .u16OdNum = 0,
            .u16MdNum = 1,
        },
    },
};

static struct ST_Stream_Attr_T g_stStreamAttr[] =
{
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_VPE,
        .u32InputChn = 0,
        .u32InputPort = 0,
        .vencChn = 0,
        .eType = E_MI_VENC_MODTYPE_JPEGE,
        .eRcMode = E_MI_VENC_RC_MODE_H264CBR,
        .f32Mbps =   2,
        .u32Fps =   30,
        .u32Gop =   50,
        .u32Width = 1920,
        .u32Height = 1080,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = SUB_STREAM0,
        .eBindType = E_MI_SYS_BIND_TYPE_REALTIME,
        .u32BindPara = 0,
        .u32Cover1Handle = 1,
        .u32Cover2Handle = 2,
    },
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_VPE,
        .u32InputChn = 0,
        .u32InputPort = 1,
        .vencChn = 1,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .eRcMode = E_MI_VENC_RC_MODE_H264CBR,
        .f32Mbps =   2,
        .u32Fps =   30,
        .u32Gop =   50,
        .u32Width = 1920,
        .u32Height = 1080,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = MAIN_STREAM,
        .eBindType = E_MI_SYS_BIND_TYPE_HW_RING,
        .u32BindPara = 1080,
        .u32Cover1Handle = 3,
        .u32Cover2Handle = 4,
    },
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_VPE,
        .u32InputChn = 0,
        .u32InputPort = 2,
        .vencChn = 2,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .eRcMode = E_MI_VENC_RC_MODE_H264CBR,
        .f32Mbps =   1,
        .u32Fps =   30,
        .u32Gop =   50,
        .u32Width = 720,
        .u32Height = 576,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = SUB_STREAM1,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,
        .u32BindPara = 0,
        .u32Cover1Handle = 5,
        .u32Cover2Handle = 6,
    },
};

class CanvasScopeLock
{
    public:
        explicit CanvasScopeLock()
        {
            pthread_mutex_lock(&_gstCanvasMutex);
        }
        ~CanvasScopeLock()
        {
            pthread_mutex_unlock(&_gstCanvasMutex);
        }
        static void CanvasLock()
        {
            pthread_mutex_lock(&_gstCanvasMutex);
        }
        static void CanvasUnlock()
        {
            pthread_mutex_unlock(&_gstCanvasMutex);
        }
    private:
        static pthread_mutex_t _gstCanvasMutex;
};
pthread_mutex_t CanvasScopeLock::_gstCanvasMutex = PTHREAD_MUTEX_INITIALIZER;

#define CANVAS_LOCK CanvasScopeLock::CanvasLock()
#define CANVAS_UNLOCK CanvasScopeLock::CanvasUnlock()

int ST_VDFGetMDRect(ST_Rect_T *pstRect);

typedef struct ST_MdStatCtx_s
{
    double mdBox[30];
    MI_U32 mdIdx;
    double  md_level;
}ST_MdStatCtx_t;

static int ST_CalcMdLevel(ST_MdStatCtx_t *pstMdStat, MI_U32 mdCnt)
{
    MI_U32 i = 0;
    float sum = 0;
    MI_U32 den = 1;

    pstMdStat->mdBox[pstMdStat->mdIdx] = mdCnt;

    for(i = pstMdStat->mdIdx; i >= 0; i--)
    {
        sum = sum + pstMdStat->mdBox[i]/den;
        den++;
    }
    for(i = 29; i > pstMdStat->mdIdx; i--)
    {
        sum = sum + pstMdStat->mdBox[i]/den;
        den++;
    }
    pstMdStat->mdIdx++;
    pstMdStat->md_level = sum;
    return 0;
}

void *ST_VENCSendStreamManual(void *args)
{
	VENC_SendBuf_Args_S *pstVENCSendBuf = (VENC_SendBuf_Args_S *)args;
	MI_SYS_ChnPort_t stVPEChnOutputPort;
	MI_SYS_ChnPort_t stVencChnInputPort;
	MI_S32 s32Ret = MI_SUCCESS;
	MI_S32 s32Fd = -1;
	fd_set read_fds;
    struct timeval TimeoutVal;
	MI_SYS_BufInfo_t stVPEBufInfo;
    MI_SYS_BUF_HANDLE bufVPEHandle;
	MI_SYS_BUF_HANDLE bufVENCHandle;
	MI_SYS_BufConf_t stVENCBufConf;
	MI_SYS_BufInfo_t stVENCBufInfo;
    MI_VENC_RcParam_t stRcParam;
    ST_Rect_T stRect[(RAW_W / 4) * (RAW_H / 4)] = {{0,},};
	MI_U32 u32DevId = -1;
    MI_U32 mdCnt = 0;
    ST_MdStatCtx_t stMdStat;
    MI_U32 change_pos = 100, pre_change_pos = 100;
    MI_U32 bits_per_block;

	memset(&stVPEChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

    stVPEChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVPEChnOutputPort.u32DevId = 0;
    stVPEChnOutputPort.u32ChnId = pstVENCSendBuf->u32Chn;
    stVPEChnOutputPort.u32PortId = pstVENCSendBuf->u32Port;

    s32Ret = MI_SYS_GetFd(&stVPEChnOutputPort, (MI_S32 *)&s32Fd);
    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_GetFd 0, error, %X\n", s32Ret);
        return NULL;
    }
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVPEChnOutputPort, 2, 3);

	MI_VENC_GetChnDevid(pstVENCSendBuf->VencChn, &u32DevId);
	stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = pstVENCSendBuf->VencChn;
    stVencChnInputPort.u32PortId = 0;

	memset(&stVENCBufConf, 0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stVENCBufConf.u64TargetPts);
    stVENCBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stVENCBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVENCBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stVENCBufConf.stFrameCfg.u16Width = pstVENCSendBuf->u32Width;
    stVENCBufConf.stFrameCfg.u16Height = pstVENCSendBuf->u32Height;

    bits_per_block = pstVENCSendBuf->u32BitRate / (pstVENCSendBuf->u32Width * pstVENCSendBuf->u32Height /16/16);
    memset(&stRcParam, 0, sizeof(MI_VENC_RcParam_t));
    memset(&stMdStat, 0, sizeof(ST_MdStatCtx_t));
	while(pstVENCSendBuf->bRunFlag)
	{
		FD_ZERO(&read_fds);
	    FD_SET(s32Fd, &read_fds);

	    s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
	    if (s32Ret < 0)
	    {
	        // select failed
	    }
	    else if (0 == s32Ret)
	    {
	        // timeout
	    }
		else
		{
			if (FD_ISSET(s32Fd, &read_fds))
			{
				memset(&stVPEBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
				s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVPEChnOutputPort, &stVPEBufInfo, &bufVPEHandle);
				if (MI_SUCCESS != s32Ret)
		        {
		            ST_ERR("Get buffer error, %x\n", s32Ret);
		        }

				// send stream to venc
				s32Ret = MI_SYS_ChnInputPortGetBuf(&stVencChnInputPort, &stVENCBufConf, &stVENCBufInfo, &bufVENCHandle, 1000);
				if (MI_SUCCESS == s32Ret)
				{
					// copy Y data
					memcpy((MI_U8 *)stVENCBufInfo.stFrameData.pVirAddr[0], (MI_U8 *)stVPEBufInfo.stFrameData.pVirAddr[0],
								stVPEBufInfo.stFrameData.u16Height * stVPEBufInfo.stFrameData.u32Stride[0]);

					// copy UV data
					memcpy((MI_U8 *)stVENCBufInfo.stFrameData.pVirAddr[1], (MI_U8 *)stVPEBufInfo.stFrameData.pVirAddr[1],
								stVPEBufInfo.stFrameData.u16Height * stVPEBufInfo.stFrameData.u32Stride[1] / 2);

                    stVENCBufInfo.u64Pts = stVPEBufInfo.u64Pts;
                    mdCnt = ST_VDFGetMDRect(stRect);
                    ST_CalcMdLevel(&stMdStat, mdCnt);
                    printf("md_level:%f, mdCnt:%d\n", stMdStat.md_level, mdCnt);

                    if(stMdStat.md_level < 50)
                    {
                        if(bits_per_block < 128)
                        {
                            change_pos = 90;
                        }
                        else if(bits_per_block > 384)
                        {
                            change_pos = 50;
                        }
                        else
                        {
                            change_pos = 50 + 40 * (384 - bits_per_block) / 256;
                        }
                    }
                    else if(stMdStat.md_level < 200)
                    {
                        if(bits_per_block < 128)
                        {
                            change_pos = 90;
                        }
                        else if(bits_per_block > 384)
                        {
                            change_pos = 70;
                        }
                        else
                        {
                            change_pos = 70 + 20 * (384 - bits_per_block) / 256;
                        }
                    }
                    else
                    {
                        change_pos = 100;
                    }

                    if(pre_change_pos != change_pos)
                    {
                        MI_VENC_GetRcParam(pstVENCSendBuf->VencChn, &stRcParam);
                        if(pstVENCSendBuf->eRcMode == E_MI_VENC_RC_MODE_H264VBR)
                        {
                            stRcParam.stParamH264VBR.s32ChangePos = change_pos;
                            stRcParam.stParamH264VBR.u32MaxIQp = 48;
                            stRcParam.stParamH264VBR.u32MinIQp = 12;
                            stRcParam.stParamH264VBR.s32IPQPDelta = 2;
                        }
                        else if(pstVENCSendBuf->eRcMode == E_MI_VENC_RC_MODE_H265VBR)
                        {
                            stRcParam.stParamH265Vbr.s32ChangePos = change_pos;
                            stRcParam.stParamH265Vbr.u32MaxIQp = 48;
                            stRcParam.stParamH265Vbr.u32MinIQp = 12;
                            stRcParam.stParamH265Vbr.s32IPQPDelta = 2;
                        }
                        MI_VENC_SetRcParam(pstVENCSendBuf->VencChn, &stRcParam);
                        printf("pos:%d\n", change_pos);
                        pre_change_pos = change_pos;
                    }


					// ST_DBG("%d,%d\n", stVENCBufInfo.stFrameData.u32Stride[0], stVENCBufInfo.stFrameData.u32Stride[1]);
					s32Ret = MI_SYS_ChnInputPortPutBuf(bufVENCHandle ,&stVENCBufInfo , FALSE);
		            if (MI_SUCCESS != s32Ret)
		            {
		                ST_ERR("MI_SYS_ChnInputPortPutBuf error, %X\n", s32Ret);
		            }
				}

				s32Ret = MI_SYS_ChnOutputPortPutBuf(bufVPEHandle);
				if (MI_SUCCESS != s32Ret)
		        {
		            ST_ERR("Get buffer error, %x\n", s32Ret);
		        }
			}
		}
	}

	MI_SYS_CloseFd(s32Fd);

	return NULL;
}

void *ST_OpenStream(char const *szStreamName, void *arg)
{
    ST_StreamInfo_T *pstStreamInfo = NULL;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    pstStreamInfo = (ST_StreamInfo_T *)malloc(sizeof(ST_StreamInfo_T));

    if(pstStreamInfo == NULL)
    {
        ST_ERR("malloc error\n");
        return NULL;
    }

    memset(pstStreamInfo, 0, sizeof(ST_StreamInfo_T));

    for(i = 0; i < u32ArraySize; i ++)
    {
        if(!strncmp(szStreamName, pstStreamAttr[i].pszStreamName,
                    strlen(pstStreamAttr[i].pszStreamName)))
        {
            break;
        }
    }

    if(i >= u32ArraySize)
    {
        ST_ERR("not found this stream, \"%s\"", szStreamName);
        free(pstStreamInfo);
        return NULL;
    }

    pstStreamInfo->vencChn = pstStreamAttr[i].vencChn;
    pstStreamInfo->enType = pstStreamAttr[i].eType;
    snprintf(pstStreamInfo->szStreamName, sizeof(pstStreamInfo->szStreamName) - 1,
             "%s", szStreamName);

#if DEBUG_ES_FILE
    // whether write frame to file
    int len = 0;
    time_t now = 0;
    struct tm *tm = NULL;

    now = time(NULL);
    tm = localtime(&now);

    // len += sprintf(pstStreamInfo->szDebugFile + len, "%s/", PATH_PREFIX);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%s_venc%02d_", szStreamName, pstStreamInfo->vencChn);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%d_", tm->tm_year + 1900);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_mon);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d-", tm->tm_mday);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_hour);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d_", tm->tm_min);
    len += sprintf(pstStreamInfo->szDebugFile + len, "%02d", tm->tm_sec);

    pstStreamInfo->bWriteFile = TRUE;
    pstStreamInfo->fd = 0;
    pstStreamInfo->fd = open(pstStreamInfo->szDebugFile,
                             O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(pstStreamInfo->fd <= 0)
    {
        ST_WARN("create file %s error\n", pstStreamInfo->szDebugFile);
    }
    else
    {
        ST_DBG("open %s success\n", pstStreamInfo->szDebugFile);
    }

#endif
    s32Ret = MI_VENC_RequestIdr(pstStreamInfo->vencChn, TRUE);

    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, pstStreamInfo->vencChn);

    if(MI_SUCCESS != s32Ret)
    {
        ST_WARN("request IDR fail, error:%x\n", s32Ret);
    }

    return pstStreamInfo;
}

int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    //struct timeval curtime;

    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn ;
    MI_S32 s32Fd;
    struct timeval tv;
    fd_set read_fds;

    if(handle == NULL)
    {
        return -1; // disconnect
    }

    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;
    s32Fd = MI_VENC_GetFd(pstStreamInfo->vencChn);
    if (s32Fd < 0)
        return 0;

    FD_ZERO(&read_fds);
    FD_SET(s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 10 * 1000;

    s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
    if (s32Ret < 0)
    {
        goto RET;
    }
    else if (0 == s32Ret)
    {
        goto RET;
    }
    else
    {
        vencChn = pstStreamInfo->vencChn;

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack, 0, sizeof(stPack));
        stStream.pstPack = &stPack;
        stStream.u32PackCount = 1;
        s32Ret = MI_VENC_Query(vencChn, &stStat);

        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            goto RET;
        }

        s32Ret = MI_VENC_GetStream(vencChn, &stStream, 40);

        // ST_DBG("GET, seq:%d, s32Ret:%x\n", stStream.u32Seq, s32Ret);
        if(MI_SUCCESS == s32Ret)
        {
            //len = fwrite(stStream.pstPack[0].pu8Addr, 1,stStream.pstPack[0].u32Len,fd);
            len = stStream.pstPack[0].u32Len;
            memcpy(ucpBuf, stStream.pstPack[0].pu8Addr, len);

#if DEBUG_ES_FILE

            if((pstStreamInfo->bWriteFile == TRUE) &&
                    (pstStreamInfo->fd > 0))
            {
                writeLen = write(pstStreamInfo->fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);

                if(writeLen != stStream.pstPack[0].u32Len)
                {
                    ST_WARN("write file uncompletly\n");
                }
            }

#endif
            //gettimeofday(p_Timestamp, NULL);

            s32Ret = MI_VENC_ReleaseStream(vencChn, &stStream);
            // ST_DBG("Release, s32Ret:%x\n", s32Ret);
        }
    }
RET:
    MI_VENC_CloseFd(s32Fd);

    return len;
}

int ST_CloseStream(void *handle, void *arg)
{
    if(handle == NULL)
    {
        return -1;
    }

    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;

#if DEBUG_ES_FILE
    if((pstStreamInfo->bWriteFile == TRUE) &&
            (pstStreamInfo->fd > 0))
    {
        pstStreamInfo->fd = 0;

        ST_DBG("close %s success\n", pstStreamInfo->szDebugFile);
    }
#endif

    MI_VENC_CloseFd(pstStreamInfo->vencChn);
    ST_DBG("close \"%s\" success\n", pstStreamInfo->szStreamName);

    free(pstStreamInfo);

    return 0;
}

MI_S32 ST_RtspServerStart(void)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;
    int iRet = 0;
    char *urlPrefix = NULL;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    int i = 0;
    ServerMediaSession *mediaSession = NULL;
    ServerMediaSubsession *subSession = NULL;
    Live555RTSPServer *pRTSPServer = NULL;

    pRTSPServer = new Live555RTSPServer();

    if(pRTSPServer == NULL)
    {
        ST_ERR("malloc error\n");
        return -1;
    }

#if 0
    pRTSPServer->addUserRecord("admin", "111111");
#endif

    iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);

    while(iRet < 0)
    {
        rtspServerPortNum++;

        if(rtspServerPortNum > 65535)
        {
            ST_INFO("Failed to create RTSP server: %s\n", pRTSPServer->getResultMsg());
            delete pRTSPServer;
            pRTSPServer = NULL;
            return -2;
        }

        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }

    urlPrefix = pRTSPServer->rtspURLPrefix();
    printf("=================URL===================\n");

    for(i = 0; i < arraySize; i ++)
    {
        if (pstStreamAttr[i].bEnable == FALSE)
        {
            continue;
        }

        if(pstStreamAttr[i].enFunc & ST_Sys_Func_RTSP)
        {
            printf("%s%s\n", urlPrefix, pstStreamAttr[i].pszStreamName);
        }
    }

    printf("=================URL===================\n");

    for(i = 0; i < arraySize; i ++)
    {
        if (pstStreamAttr[i].bEnable == FALSE)
        {
            continue;
        }

        if(pstStreamAttr[i].enFunc & ST_Sys_Func_RTSP)
        {
            pRTSPServer->createServerMediaSession(mediaSession,
                                                  pstStreamAttr[i].pszStreamName,
                                                  NULL, NULL);

            if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
            {
                subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
            {
                subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
            {
                subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, 30);
            }

            pRTSPServer->addSubsession(mediaSession, subSession);
            pRTSPServer->addServerMediaSession(mediaSession);
        }
    }

    pRTSPServer->Start();

    g_pRTSPServer = pRTSPServer;

    return 0;
}

void ST_SaveStreamToFile(MI_VENC_Stream_t *pstStream)
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

    if(stat(CAPTURE_PATH, &st) == -1)
    {
        if(mkdir(CAPTURE_PATH, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
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

    if(fd <= 0)
    {
        // ST_WARN("create file %s error\n", szFileName);
    }
    else
    {
        // ST_DBG("open %s success\n", szFileName);
    }

    for(i = 0; i < pstStream->u32PackCount; i ++)
    {
        pstPack = &pstStream->pstPack[i];
        write(fd, pstPack->pu8Addr + pstPack->u32Offset, pstPack->u32Len - pstPack->u32Offset);
    }

    close(fd);

    printf("write to %s\n", szFileName);
}

void ST_CaptureJPGProc(MI_VENC_CHN VencChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_U32 u32BypassCnt = 0;

    s32Ret = MI_VENC_StartRecvPic(VencChn);

    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("ST_Venc_StartChannel fail, 0x%x\n", s32Ret);
        return;
    }

    memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
    memset(&stPack, 0, sizeof(MI_VENC_Pack_t));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;

    while(1)
    {
        s32Ret = MI_VENC_GetStream(VencChn, &stStream, 40);
        if(MI_SUCCESS == s32Ret)
        {
            if (0 == u32BypassCnt)
            {
                printf("##########Start to write file!!!#####################\n");
                ST_SaveStreamToFile(&stStream);
                printf("##########End to write file!!!#####################\n");
            }
            else
            {
                printf("Bypasss frame, because region osd attach action not currect!\n");
            }
            s32Ret = MI_VENC_ReleaseStream(VencChn, &stStream);
            if(MI_SUCCESS != s32Ret)
            {
                ST_DBG("MI_VENC_ReleaseStream fail, ret:0x%x\n", s32Ret);
            }

            ST_DBG("u32BypassCnt:%d\n", u32BypassCnt);

            if (0 == u32BypassCnt)
                break;

            u32BypassCnt--;
        }
        else
            printf("Continue!!!\n");

        usleep(200 * 1000);
    }

    s32Ret = MI_VENC_StopRecvPic(VencChn);

    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("ST_Venc_StopChannel fail, 0x%x\n", s32Ret);
    }
}

MI_S32 ST_StartPipeLine(MI_U8 i, MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32CropW, MI_U32 u32CropH, MI_U32 u32CropX, MI_U32 u32CropY)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    ST_Sys_BindInfo_T stBindInfo;
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;
    MI_SYS_WindowRect_t stRect;
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
	ST_Config_S *pstConfig = &g_stConfig;
	ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    CanvasScopeLock ScopeLock;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);

    if(i >= arraySize)
    {
        printf("index is out of bounds!\n");
        return MI_FAIL;
    }

    memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
    stVpePortInfo.DepVpeChannel = pstStreamAttr[i].u32InputChn;
    stVpePortInfo.u16OutputWidth = u32Width;
    stVpePortInfo.u16OutputHeight = u32Height;
    printf("Vpe create port w %d h %d\n", u32Width, u32Height);

    if (pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE &&
        pstStreamAttr[i].eBindType == E_MI_SYS_BIND_TYPE_REALTIME)
    {
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    }
    else
    {
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    }

    stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    if (u32CropW != 0 && u32CropH != 0)
    {
        stRect.u16Width = u32CropW;
        stRect.u16Height = u32CropH;
        stRect.u16X = u32CropX;
        stRect.u16Y = u32CropY;
        ExecFunc(MI_VPE_SetPortCrop(pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort, &stRect), MI_SUCCESS);
    }
    STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[i].u32InputPort, &stVpePortInfo));

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
    {
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;

        stChnAttr.stRcAttr.eRcMode = pstStreamAttr[i].eRcMode;
        if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H264CBR)
        {
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstStreamAttr[i].f32Mbps * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = pstStreamAttr[i].u32Gop;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        }
        else
        {
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pstStreamAttr[i].f32Mbps * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = pstStreamAttr[i].u32Gop;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 12;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
        }

    }
    else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
    {
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

        stChnAttr.stRcAttr.eRcMode = pstStreamAttr[i].eRcMode;
        if(stChnAttr.stRcAttr.eRcMode == E_MI_VENC_RC_MODE_H265CBR)
        {
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstStreamAttr[i].f32Mbps * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = pstStreamAttr[i].u32Gop;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
        }
        else
        {
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pstStreamAttr[i].f32Mbps * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = pstStreamAttr[i].u32Gop;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 12;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
        }
    }
    else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth =  u32Width;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =  u32Width;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    }
    stChnAttr.stVeAttr.eType = pstStreamAttr[i].eType;
    VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
        VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_StartChannel(VencChn));
    // create osd, VPE port 2 has't OSD
    if (pstStreamAttr[i].u32InputPort != 2)
    {
        // attach
        memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
        stChnPort.eModId = E_MI_RGN_MODID_VPE;
        stChnPort.s32DevId = 0;
        stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
        stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
        stChnPortParam.bShow = TRUE;
        stChnPortParam.stPoint.u32X = u32Width - RGN_OSD_TIME_WIDTH - 10;
        stChnPortParam.stPoint.u32Y = 10;
        stChnPortParam.unPara.stCoverChnPort.u32Layer = 100;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 0;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 0;
        stChnPortParam.unPara.stCoverChnPort.u32Color = 0;
        ExecFunc(MI_RGN_AttachToChn(RGN_OSD_HANDLE, &stChnPort, &stChnPortParam), MI_RGN_OK);
    }
#if 0
    // create cover1
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[i].u32Cover1Handle, &stRgnAttr), MI_RGN_OK);
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 100;
    stChnPortParam.stPoint.u32Y = 100;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[i].u32Cover1Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 255, 0);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[i].u32Cover1Handle, &stChnPort, &stChnPortParam), MI_RGN_OK);
    // create cover2
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[i].u32Cover2Handle, &stRgnAttr), MI_RGN_OK);
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 400;
    stChnPortParam.stPoint.u32Y = 400;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[i].u32Cover2Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 255);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[i].u32Cover2Handle, &stChnPort, &stChnPortParam), MI_RGN_OK);
#endif

	if (pstConfig->s32UseVdf)
	{
		if (i < MAX_FULL_RGN_NULL)
		{
			pstVDFOsdInfo[i].hHandle = RGN_FOR_VDF_BEGIN + i;
			pstVDFOsdInfo[i].eModId = E_MI_RGN_MODID_VPE;
			pstVDFOsdInfo[i].u32Chn = pstStreamAttr[i].u32InputChn;
			pstVDFOsdInfo[i].u32Port = pstStreamAttr[i].u32InputPort;

			memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
		    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
		    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
		    stRgnAttr.stOsdInitParam.stSize.u32Width = pstStreamAttr[i].u32Width;
		    stRgnAttr.stOsdInitParam.stSize.u32Height = pstStreamAttr[i].u32Height;
		    ExecFunc(ST_OSD_Create(pstVDFOsdInfo[i].hHandle, &stRgnAttr), MI_RGN_OK);

			memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
	        stChnPort.eModId = E_MI_RGN_MODID_VPE;
	        stChnPort.s32DevId = 0;
	        stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
	        stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
	        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
	        stChnPortParam.bShow = TRUE;
	        stChnPortParam.stPoint.u32X = 0;
	        stChnPortParam.stPoint.u32Y = 0;
	        stChnPortParam.unPara.stOsdChnPort.u32Layer = 0;
	        ExecFunc(MI_RGN_AttachToChn(pstVDFOsdInfo[i].hHandle, &stChnPort, &stChnPortParam), MI_RGN_OK);
		}
	}

#if defined(ENABLE_INTELL_VENC) && (ENABLE_INTELL_VENC == 1)
	if (pstStreamAttr[i].u32InputPort != 1)
	{
		// bind VPE->VENC
	    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
	    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
	    stBindInfo.stSrcChnPort.u32DevId = 0;
	    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
	    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
	    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
	    stBindInfo.stDstChnPort.u32DevId = u32DevId;
	    stBindInfo.stDstChnPort.u32ChnId = VencChn;
	    stBindInfo.stDstChnPort.u32PortId = 0;
	    stBindInfo.u32SrcFrmrate = 30;
	    stBindInfo.u32DstFrmrate = 30;
	    stBindInfo.eBindType = pstStreamAttr[i].eBindType;
	    if (stBindInfo.eBindType == E_MI_SYS_BIND_TYPE_HW_RING)
	    {
	        stBindInfo.u32BindParam = u32Height;
	    }
	    else
	    {
	        stBindInfo.u32BindParam = 0;
	    }
	    printf("Bind type %d, para %d\n", stBindInfo.eBindType, stBindInfo.u32BindParam);
	    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
	}
	else
	{
		if (i < 4)
		{
			g_stVENCSendBuf[i].bRunFlag = TRUE;
			g_stVENCSendBuf[i].u32Chn = pstStreamAttr[i].u32InputChn;
			g_stVENCSendBuf[i].u32Port = pstStreamAttr[i].u32InputPort;
			g_stVENCSendBuf[i].VencChn = VencChn;
            g_stVENCSendBuf[i].eModType = pstStreamAttr[i].eType;
			g_stVENCSendBuf[i].u32Width = pstStreamAttr[i].u32Width;
			g_stVENCSendBuf[i].u32Height = pstStreamAttr[i].u32Height;
            g_stVENCSendBuf[i].eRcMode = pstStreamAttr[i].eRcMode;
            g_stVENCSendBuf[i].u32BitRate = pstStreamAttr[i].f32Mbps * 1024 * 1024;
			pthread_create(&g_stVENCSendBuf[i].pt, NULL, ST_VENCSendStreamManual, (void *)&g_stVENCSendBuf[i]);
		}
		else
		{
			ST_WARN("the max is 4\n");
		}
	}
#else
	// bind VPE->VENC
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = pstStreamAttr[i].eBindType;
    if (stBindInfo.eBindType == E_MI_SYS_BIND_TYPE_HW_RING)
    {
        stBindInfo.u32BindParam = u32Height;
    }
    else
    {
        stBindInfo.u32BindParam = 0;
    }
    printf("Bind type %d, para %d\n", stBindInfo.eBindType, stBindInfo.u32BindParam);
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif

    return MI_SUCCESS;
}

MI_S32 ST_StartPipeLineWithDip(MI_U8 i, MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32CropW, MI_U32 u32CropH, MI_U32 u32CropX, MI_U32 u32CropY)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    ST_Sys_BindInfo_T stBindInfo;
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;
    MI_SYS_WindowRect_t stRect;
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_DIVP_ChnAttr_t stDivpChnAttr;

    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;

	ST_Config_S *pstConfig = &g_stConfig;
	ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    CanvasScopeLock ScopeLock;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);

    if(i >= arraySize)
    {
        printf("index is out of bounds!\n");
        return MI_FAIL;
    }

    memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
    stVpePortInfo.DepVpeChannel = pstStreamAttr[i].u32InputChn;
    stVpePortInfo.u16OutputWidth = u32Width;
    stVpePortInfo.u16OutputHeight = u32Height;
    printf("Vpe create port w %d h %d\n", u32Width, u32Height);
    stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    if (u32CropW != 0 && u32CropH != 0)
    {
        stRect.u16Width = u32CropW;
        stRect.u16Height = u32CropH;
        stRect.u16X = u32CropX;
        stRect.u16Y = u32CropY;
        ExecFunc(MI_VPE_SetPortCrop(pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort, &stRect), MI_SUCCESS);
    }
    STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[i].u32InputPort, &stVpePortInfo));
    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0;
    stDivpChnAttr.stCropRect.u16Y       = 0;
    stDivpChnAttr.stCropRect.u16Width   = 0;
    stDivpChnAttr.stCropRect.u16Height  = 0;
    stDivpChnAttr.u32MaxWidth           = u32Width;
    stDivpChnAttr.u32MaxHeight          = u32Height;
    ExecFunc(MI_DIVP_CreateChn(DIVP_CHN_FOR_SCALE + i, &stDivpChnAttr), MI_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(DIVP_CHN_FOR_SCALE + i), MI_SUCCESS);
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = u32Width;
    stOutputPortAttr.u32Height          = u32Height;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DIVP_CHN_FOR_SCALE + i, &stOutputPortAttr));
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
    {
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;

        //stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024 * 1024 * 2;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;

    }
    else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
    {
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = 1024 * 1024 * 2;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
    }
    else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    }
    stChnAttr.stVeAttr.eType = pstStreamAttr[i].eType;
    VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_StartChannel(VencChn));

    // create osd, VPE port 2 has't OSD
    // attach
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
    stChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = u32Width - RGN_OSD_TIME_WIDTH - 10;
    stChnPortParam.stPoint.u32Y = 10;
    ExecFunc(MI_RGN_AttachToChn(RGN_OSD_HANDLE, &stChnPort, &stChnPortParam), MI_RGN_OK);
    // create cover1
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[i].u32Cover1Handle, &stRgnAttr), MI_RGN_OK);
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
    stChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 100;
    stChnPortParam.stPoint.u32Y = 100;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[i].u32Cover1Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 255, 0);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[i].u32Cover1Handle, &stChnPort, &stChnPortParam), MI_RGN_OK);
    // create cover2
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[i].u32Cover2Handle, &stRgnAttr), MI_RGN_OK);
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
    stChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 400;
    stChnPortParam.stPoint.u32Y = 400;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[i].u32Cover2Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 255);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[i].u32Cover2Handle, &stChnPort, &stChnPortParam), MI_RGN_OK);

	if (pstConfig->s32UseVdf)
	{
		if (i < MAX_FULL_RGN_NULL)
		{
			pstVDFOsdInfo[i].hHandle = RGN_FOR_VDF_BEGIN + i;
			pstVDFOsdInfo[i].eModId = E_MI_RGN_MODID_DIVP;
			pstVDFOsdInfo[i].u32Chn = DIVP_CHN_FOR_SCALE + i;
			pstVDFOsdInfo[i].u32Port = 0;

			memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
		    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
		    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
		    stRgnAttr.stOsdInitParam.stSize.u32Width = pstStreamAttr[i].u32Width;
		    stRgnAttr.stOsdInitParam.stSize.u32Height = pstStreamAttr[i].u32Height;
		    ExecFunc(ST_OSD_Create(pstVDFOsdInfo[i].hHandle, &stRgnAttr), MI_RGN_OK);

			memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
	        stChnPort.eModId = E_MI_RGN_MODID_DIVP;
	        stChnPort.s32DevId = 0;
	        stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
	        stChnPort.s32OutputPortId = 0;
	        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
	        stChnPortParam.bShow = TRUE;
	        stChnPortParam.stPoint.u32X = 0;
	        stChnPortParam.stPoint.u32Y = 0;
	        stChnPortParam.unPara.stOsdChnPort.u32Layer = 100;
	        ExecFunc(MI_RGN_AttachToChn(pstVDFOsdInfo[i].hHandle, &stChnPort, &stChnPortParam), MI_RGN_OK);
		}
	}

    // bind VPE to divp
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_SCALE + i;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    // bind divp to venc
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = DIVP_CHN_FOR_SCALE + i;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    return MI_SUCCESS;

}
MI_S32 ST_StopPipeLine(MI_U8 i)
{
    MI_VENC_CHN VencChn = 0;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = -1;
    MI_RGN_ChnPort_t stChnPort;

	ST_Config_S *pstConfig = &g_stConfig;
	ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    CanvasScopeLock ScopeLock;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);

    if(i >= arraySize)
    {
        printf("index is out of bounds!\n");
        return MI_FAIL;
    }
    // destory cover
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[i].u32Cover1Handle), MI_RGN_OK);
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[i].u32Cover2Handle), MI_RGN_OK);
    // Destory osd
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
    ExecFunc(MI_RGN_DetachFromChn(RGN_OSD_HANDLE, &stChnPort), MI_RGN_OK);

	if (pstConfig->s32UseVdf)
	{
		if (i < MAX_FULL_RGN_NULL)
		{
			stChnPort.eModId = E_MI_RGN_MODID_VPE;
		    stChnPort.s32DevId = 0;
		    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
		    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
			ExecFunc(MI_RGN_DetachFromChn(pstVDFOsdInfo[i].hHandle, &stChnPort), MI_RGN_OK);

			ExecFunc(ST_OSD_Destroy(pstVDFOsdInfo[i].hHandle), MI_RGN_OK);

			pstVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
		}
	}

    VencChn = pstStreamAttr[i].vencChn;
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Venc_StopChannel(VencChn));
    STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
    STCHECKRESULT(ST_Vpe_StopPort(pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort));


    return MI_SUCCESS;
}
MI_S32 ST_StopPipeLineWithDip(MI_U8 i)
{
    MI_VENC_CHN VencChn = 0;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = -1;
    MI_RGN_ChnPort_t stChnPort;

	ST_Config_S *pstConfig = &g_stConfig;
	ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    CanvasScopeLock ScopeLock;
    int arraySize = ARRAY_SIZE(g_stStreamAttr);

    if(i >= arraySize)
    {
        printf("index is out of bounds!\n");
        return MI_FAIL;
    }

    // destory cover
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[i].u32Cover1Handle), MI_RGN_OK);
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[i].u32Cover2Handle), MI_RGN_OK);
    // Destory osd
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
    stChnPort.s32OutputPortId = 0;
    ExecFunc(MI_RGN_DetachFromChn(RGN_OSD_HANDLE, &stChnPort), MI_RGN_OK);

	if (pstConfig->s32UseVdf)
	{
		if (i < MAX_FULL_RGN_NULL)
		{
			stChnPort.eModId = E_MI_RGN_MODID_DIVP;
		    stChnPort.s32DevId = 0;
		    stChnPort.s32ChnId = DIVP_CHN_FOR_SCALE + i;
		    stChnPort.s32OutputPortId = 0;
			ExecFunc(MI_RGN_DetachFromChn(pstVDFOsdInfo[i].hHandle, &stChnPort), MI_RGN_OK);

			ExecFunc(ST_OSD_Destroy(pstVDFOsdInfo[i].hHandle), MI_RGN_OK);

			pstVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
		}
	}

    VencChn = pstStreamAttr[i].vencChn;
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_SCALE + i;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = DIVP_CHN_FOR_SCALE + i;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Venc_StopChannel(VencChn));
    STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
    ExecFunc(MI_DIVP_StopChn(DIVP_CHN_FOR_SCALE + i), MI_SUCCESS);
    ExecFunc(MI_DIVP_DestroyChn(DIVP_CHN_FOR_SCALE + i), MI_SUCCESS);
    STCHECKRESULT(ST_Vpe_StopPort(pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort));

    return MI_SUCCESS;
}

MI_S32 ST_RtspServerStop(void)
{
    if(g_pRTSPServer)
    {
        g_pRTSPServer->Join();
        delete g_pRTSPServer;
        g_pRTSPServer = NULL;
    }

    return 0;
}
void ST_Flush(void)
{
    char c;

    while((c = getchar()) != '\n' && c != EOF);
}

MI_S32 ST_BaseModuleInit(ST_Config_S* pstConfig)
{
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_RGN_Attr_t stRgnAttr;
    ST_VPE_ChannelInfo_T stVpeChannelInfo;
    ST_Sys_BindInfo_T stBindInfo;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U32 u32ChocieRes =0;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    if(pstConfig->s32HDRtype > 0)
        MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, TRUE);
    else
        MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE);

    MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }

    printf("choice which resolution use, cnt %d\n", u32ResCount);
    do
    {
        scanf("%d", &u32ChocieRes);
        ST_Flush();
        MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32ResCount);
        if(u32ChocieRes >= u32ResCount)
        {
            printf("choice err res %d > =cnt %d\n", u32ChocieRes, u32ResCount);
        }
    }while(u32ChocieRes >= u32ResCount);

    printf("You select %d res\n", u32ChocieRes);

    MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,u32ChocieRes);
    MI_SNR_Enable(E_MI_SNR_PAD_ID_0);

    MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info);
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    g_u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    g_u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    //g_stStreamAttr[0].u32Width = u32CapWidth;
    //g_stStreamAttr[0].u32Height = u32CapHeight;

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;

    STCHECKRESULT(ST_Vif_EnableDev(0,eVifHdrType, &stPad0Info));

    ST_VIF_PortInfo_T stVifPortInfoInfo;
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, 0, 0));
    //if (enRotation != E_MI_SYS_ROTATE_NONE)
    {
        MI_BOOL bMirror = FALSE, bFlip = FALSE;
        //ExecFunc(MI_VPE_SetChannelRotation(0, enRotation), MI_SUCCESS);

        switch(pstConfig->enRotation)
        {
        case E_MI_SYS_ROTATE_NONE:
            bMirror= FALSE;
            bFlip = FALSE;
            break;
        case E_MI_SYS_ROTATE_90:
            bMirror = FALSE;
            bFlip = TRUE;
            break;
        case E_MI_SYS_ROTATE_180:
            bMirror = TRUE;
            bFlip = TRUE;
            break;
        case E_MI_SYS_ROTATE_270:
            bMirror = TRUE;
            bFlip = FALSE;
            break;
        default:
            break;
        }

        MI_SNR_SetOrien(E_MI_SNR_PAD_ID_0, bMirror, bFlip);
        MI_VPE_SetChannelRotation(0, pstConfig->enRotation);
    }
    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = u32CapWidth;
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = ePixFormat;
    stVpeChannelInfo.e3DNRLevel = pstConfig->en3dNrLevel;
    stVpeChannelInfo.eHDRtype = eVpeHdrType;
    stVpeChannelInfo.bRotation = FALSE;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = g_stStreamAttr[1].u32Fps;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    STCHECKRESULT(ST_OSD_Init());
    MI_IQSERVER_Open(u32CapWidth, u32CapHeight, 0);
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stRgnAttr.stOsdInitParam.stSize.u32Width = RGN_OSD_TIME_WIDTH;
    stRgnAttr.stOsdInitParam.stSize.u32Height = RGN_OSD_TIME_HEIGHT;
    ExecFunc(ST_OSD_Create(RGN_OSD_HANDLE, &stRgnAttr), MI_RGN_OK);

    return MI_SUCCESS;
}

void *ST_UpdateRgnOsdProc(void *args)
{
    MI_S32 s32Ret = MI_SUCCESS;
    time_t now = 0;
    struct tm *tm = NULL;
    char szTime[64];
    int len = 0;
    struct timeval tv;
    unsigned char *pBuf = NULL;
    DMF_BitMapData_S stBitMapData;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_U16 i;
    MI_U16 u16MinWidth, u16MinHeight;

    if(0 != DMF_LoadBitMapFile(16, 16, DOT_FONT_FILE))
    {
        ST_ERR("load bitmap file %s error\n", DOT_FONT_FILE);
        return NULL;
    }

    while(g_stRgnOsd.bRun)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 800 * 1000;

        if(0 == select(0, NULL, NULL, NULL, &tv))
        {
            // time out
            now = time(NULL);

            if((tm = localtime(&now)) == NULL)
            {
                printf("localtime error\n");
                return NULL;
            }

            memset(szTime, 0, sizeof(szTime));
            len = 0;

            len += sprintf(szTime + len, "%d/", tm->tm_year + 1900);
            len += sprintf(szTime + len, "%02d/", tm->tm_mon);
            len += sprintf(szTime + len, "%02d-", tm->tm_mday);
            len += sprintf(szTime + len, "%02d/", tm->tm_hour);
            len += sprintf(szTime + len, "%02d/", tm->tm_min);
            len += sprintf(szTime + len, "%02d", tm->tm_sec);

            pBuf = (unsigned char *)malloc(RGN_OSD_TIME_WIDTH * RGN_OSD_TIME_HEIGHT * 2); // default ARGB1555

            if(pBuf == NULL)
            {
                ST_ERR("malloc error\n");
                return NULL;
            }

            memset(&stBitMapData, 0, sizeof(DMF_BitMapData_S));
            stBitMapData.pBuf = pBuf;
            stBitMapData.bufLen = RGN_OSD_TIME_WIDTH * RGN_OSD_TIME_HEIGHT * 2;

            if(0 != DMF_DumpToBMPBuf(szTime, &stBitMapData))
            {
                continue;
            }

            CANVAS_LOCK;

            s32Ret = MI_RGN_GetCanvasInfo(0, &stCanvasInfo);

            if(s32Ret != MI_RGN_OK)
            {
                ST_ERR("RGN SetBitMap fail!\n");
            }

            u16MinWidth = MIN(stCanvasInfo.stSize.u32Width, (MI_U32)stBitMapData.width);
            u16MinHeight = MIN(stCanvasInfo.stSize.u32Height, (MI_U32)stBitMapData.height);

            for(i = 0; i < u16MinHeight; i++)
            {
                memcpy((void *)(stCanvasInfo.virtAddr + stCanvasInfo.u32Stride * i), (void *)(pBuf + stBitMapData.width * 2 * i), u16MinWidth * 2);
            }

            s32Ret = MI_RGN_UpdateCanvas(0);

            if(s32Ret != MI_RGN_OK)
            {
                ST_ERR("RGN SetBitMap fail!\n");
            }

            CANVAS_UNLOCK;

            if(pBuf)
            {
                free(pBuf);
                pBuf = NULL;
            }
        }
    }

    DMF_CloseBitMapFile();

    return NULL;
}

void *ST_UpdateRgnOsdProcExt(void *args)
{
    time_t now = 0;
    struct tm *tm = NULL;
    char szTime[64];
    int len = 0;
    struct timeval tv;
    MI_RGN_CanvasInfo_t* pstCanvasInfo = NULL;
    ST_Point_T stPoint;
    time_t defaultInterval = 980 * 1000;
    struct timeval timeBegin, timeEnd;

    while(g_stRgnOsd.bRun)
    {
        tv.tv_sec = 0;
        tv.tv_usec = defaultInterval;

        if(0 == select(0, NULL, NULL, NULL, &tv))
        {
            // time out
            gettimeofday(&timeBegin, NULL);

            now = time(NULL);

            if((tm = localtime(&now)) == NULL)
            {
                printf("localtime error\n");
                return NULL;
            }

            memset(szTime, 0, sizeof(szTime));
            len = 0;

            len += sprintf(szTime + len, "%d/", tm->tm_year + 1900);
            len += sprintf(szTime + len, "%02d/", tm->tm_mon + 1);
            len += sprintf(szTime + len, "%02d-", tm->tm_mday);
            len += sprintf(szTime + len, "%02d/", tm->tm_hour);
            len += sprintf(szTime + len, "%02d/", tm->tm_min);
            len += sprintf(szTime + len, "%02d", tm->tm_sec);

            stPoint.u32X = 0;
            stPoint.u32Y = 0;

            CANVAS_LOCK;

            (void)ST_OSD_GetCanvasInfo(RGN_OSD_HANDLE, &pstCanvasInfo);

            (void)ST_OSD_Clear(RGN_OSD_HANDLE, NULL);

            (void)ST_OSD_DrawText(RGN_OSD_HANDLE, stPoint, szTime, I4_BLACK, DMF_Font_Size_16x16);

            (void)ST_OSD_Update(RGN_OSD_HANDLE);

            CANVAS_UNLOCK;

            gettimeofday(&timeEnd, NULL);

            defaultInterval = 1000 * 1000 - ((timeEnd.tv_sec * 1000000 + timeEnd.tv_usec) -
                                (timeBegin.tv_sec * 1000000 + timeBegin.tv_usec));

            // ST_DBG("defaultInterval:%d, szTime:%s\n", defaultInterval, szTime);
        }
    }

    return NULL;
}

MI_S32 ST_BaseModuleUnInit(void)
{
    ST_Sys_BindInfo_T stBindInfo;

    ExecFunc(ST_OSD_Destroy(RGN_OSD_HANDLE), MI_RGN_OK);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    /************************************************
    Step1:  destory VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));

    /************************************************
    Step2:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));

    /************************************************
    Step3:  destory SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Exit());

    MI_IQSERVER_Close();

    return MI_SUCCESS;
}

void *venc_channel_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32GetFramesCount;
    char filename[128];
    FILE *pfile = NULL;
    MI_BOOL _bWriteFile = FALSE;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;

    sprintf(filename, "venc%d.es", stChnPort.u32ChnId);
    printf("start to record %s\n", filename);
    pfile = fopen(filename, "wb");
    if(NULL == pfile)
    {
        printf("error: fopen venc.es failed\n");
    }

    u32GetFramesCount = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 5, 5);
    while(1)
    {
        hSysBuf = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            usleep(1*1000);
            continue;
        }
        else
        {
            if(pfile)
            {
                if(_bWriteFile)
                {
                    printf("write len = %u\n",stBufInfo.stRawData.u32BufSize);
                    fwrite(stBufInfo.stRawData.pVirAddr, 1,  stBufInfo.stRawData.u32BufSize, pfile);
                    fflush(pfile);
                }
            }
            if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
            {
                continue;
            }

            ++u32GetFramesCount;
            printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);
            usleep(10 * 1000);
        }
    }

    if(pfile)
    {
        fclose(pfile);
    }
    printf("exit record\n");
    return NULL;
}

MI_S32 ST_VencStart(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    for(i = 0; i < u32ArraySize; i ++)
    {
        if (pstStreamAttr[i].bEnable == FALSE)
        {
            continue;
        }
        if(pstStreamAttr[i].enInput == ST_Sys_Input_VPE)
        {
            ST_StartPipeLine(i, pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height, pstStreamAttr[i].u32CropWidth, pstStreamAttr[i].u32CropHeight, pstStreamAttr[i].u32CropX, pstStreamAttr[i].u32CropY);
        }
        else if (pstStreamAttr[i].enInput == ST_Sys_Input_DIVP)
        {
            ST_StartPipeLineWithDip(i, pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height, pstStreamAttr[i].u32CropWidth, pstStreamAttr[i].u32CropHeight, pstStreamAttr[i].u32CropX, pstStreamAttr[i].u32CropY);
        }
    }

    /************************************************
    Step3:  start VENC
    *************************************************/
    // g_stRgnOsd.bRun = TRUE;
    // pthread_create(&g_stRgnOsd.pt, NULL, ST_UpdateRgnOsdProc, NULL);
    // pthread_create(&g_stRgnOsd.pt, NULL, ST_UpdateRgnOsdProcExt, NULL);

    //pthread_create(&pt_venc, NULL, venc_channel_func, NULL);
    return MI_SUCCESS;
}
MI_S32 ST_VencStop(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;

    g_stRgnOsd.bRun = FALSE;
    if(g_stRgnOsd.pt != 0)
        pthread_join(g_stRgnOsd.pt, NULL);

    /************************************************
    Step2:  unbind and stop VPE port
    *************************************************/

    for(i = 0; i < u32ArraySize; i ++)
    {
        if (pstStreamAttr[i].bEnable == FALSE)
        {
            continue;
        }
        if(pstStreamAttr[i].enInput == ST_Sys_Input_VPE)
        {
            ST_StopPipeLine(i);
        }
        else if (pstStreamAttr[i].enInput == ST_Sys_Input_DIVP)
        {
            ST_StopPipeLineWithDip(i);
        }
    }

    return MI_SUCCESS;
}

MI_S32 ST_WriteOneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;

    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for(i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset * i;
        yuvSize = line_size;

        //printf("write lind %d begin\n", i);
        do
        {
            if(yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }

            size = write(dstFd, pData, size);

            if(size == 0)
            {
                break;
            }
            else if(size < 0)
            {
                break;
            }

            pData += size;
            yuvSize -= size;
        }
        while(yuvSize > 0);

        //printf("write lind %d end\n", i);
    }

    return MI_SUCCESS;
}

#if USE_AUDIO
void *ST_AoOutProc(void *data)
{
    MI_AUDIO_Attr_t stGetAttr;
    _AoOutChn_t *pstAoOutChn = (_AoOutChn_t *)data;
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
    MI_AO_GetPubAttr(pstAoOutChn->AoDevId, &stGetAttr);
    u64NeedWriteSize = (pstAoOutChn->u32InputFileSize * (pstAoOutChn->u32OutSampleRate) + pstAoOutChn->u32InSampleRate - 1) / (pstAoOutChn->u32InSampleRate);
    //printf("u64NeedWriteSize is : %d, \n",u64NeedWriteSize);


    // get data of sending by AO output port
    if(fdWr > 0)
    {
        while(pstAoOutChn->bRunFlag)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stAoChn0OutputPort0, &stBufInfo, &hHandle))
            {
                u32WriteSize = stBufInfo.stRawData.u32BufSize;

                write(fdWr, stBufInfo.stRawData.pVirAddr, u32WriteSize);
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

                if(u32EndCnt >= 100000)
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


void *ST_AoInProc(void *args)
{
    MI_S32 s32RetSendStatus = 0;
    MI_S32 s32RdSize;
    MI_U8 au8TempBuf[2048 * 2 * 2];


    ST_AoInInfo_T *pstAoInChn = (ST_AoInInfo_T *)args;

    while(pstAoInChn->bRunFlag)
    {
        s32RdSize = read(pstAoInChn->fdRd, &au8TempBuf, MI_AUDIO_SAMPLE_PER_FRAME * 2 * (pstAoInChn->u32ChnCnt));

        if(s32RdSize != MI_AUDIO_SAMPLE_PER_FRAME * 2 * (pstAoInChn->u32ChnCnt))
        {
            // memset(au8TempBuf+s32RdSize, 0,  MI_AUDIO_SAMPLE_PER_FRAME*2*(g_stAoInInfo->u32ChnCnt) - s32RdSize);
            // s32RdSize = MI_AUDIO_SAMPLE_PER_FRAME*2*(g_stAoInInfo->u32ChnCnt);
            lseek(pstAoInChn->fdRd, 0, SEEK_SET);
            continue;
        }

        //read data and send to AO module
        _gstAoTestFrame.eBitwidth = pstAoInChn->eBitwidth;
        _gstAoTestFrame.eSoundmode = pstAoInChn->eSoundmode;

        if(pstAoInChn->eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
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


        do
        {
            s32RetSendStatus = MI_AO_SendFrame(pstAoInChn->AoDevId, pstAoInChn->AoChn, &_gstAoTestFrame, 1);

            if(bEnableRes)
            {
                usleep(((MI_AUDIO_SAMPLE_PER_FRAME * 1000) / pstAoInChn->eInSampleRate - 20) * 1000);
            }
            else
            {
                usleep(((MI_AUDIO_SAMPLE_PER_FRAME * 1000) / pstAoInChn->eSamplerate - 20) * 1000);
            }
        }
        while(s32RetSendStatus == MI_AO_ERR_NOBUF);

        if(s32RetSendStatus != MI_SUCCESS)
            printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n", s32RetSendStatus);


        if(s32RdSize != MI_AUDIO_SAMPLE_PER_FRAME * 2 * (pstAoInChn->u32ChnCnt))
            break;
    }
}

int ST_AOInit()
{

    MI_S32 s32Ret = E_MI_ERR_FAILED;
    //MI_S32 s32RetSendStatus = 0;

    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_DEV AoDevId = 0;
    MI_AO_CHN AoChn;
    MI_AUDIO_SampleRate_e eInSampleRate;
    MI_AUDIO_SampleRate_e eOutSampleRate;
    MI_S32 s32SetVolumeDb;
    MI_S32 s32GetVolumeDb;

    MI_AO_VqeConfig_t stSetVqeConfig;
    MI_AO_VqeConfig_t stGetVqeConfig;

    MI_S32 s32Opt = 0;
    MI_S32 s32VolumeDb = 0;
    MI_S32 s32Idx;

    // file read/write
    int fdRd = -1;
    int fdWr = -1;
    MI_U8 au8TempBuf[2048 * 2 * 2];
    MI_U8 au8LBuf[2048 * 2];
    MI_U8 au8RBuf[2048 * 2];
    MI_U8 au8LRBuf[2048 * 2 * 2];

    MI_S32 s32RdSize;
    MI_S8 *ps8InputFilePath = NULL;
    MI_S8 *ps8OutputFilePath = NULL;


    // get output port buffer thread
    memset(&g_stAoInInfo, 0x00, sizeof(g_stAoInInfo));
    memset(&_gstAoOutchn, 0x00, sizeof(_gstAoOutchn));

    /* open file  to read data and write data */
    fdRd = open(AO_INPUT_FILE, O_RDONLY, 0666);

    if(fdRd <= 0)
    {
        printf("Open input file path:%s fail \n", AO_INPUT_FILE);
        return 1;
    }

    fdWr = open(AO_OUTPUT_FILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(fdWr < 0)
    {
        printf("Open output file path:%s fail \n", AO_OUTPUT_FILE);

        if(fdRd > 0)
            close(fdRd);

        return 1;
    }

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


    /* set ao public attr*/
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stSetAttr), MI_SUCCESS);

    /* get ao device*/
    ExecFunc(MI_AO_GetPubAttr(AoDevId, &stGetAttr), MI_SUCCESS);

    /* enable ao device */
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);

    /* enable ao channel of device*/
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);

    // if test resample, enable Resample
    if(bEnableRes)
    {
        ExecFunc(MI_AO_EnableReSmp(AoDevId, AoChn, eInSampleRate), MI_SUCCESS);
    }

    /* if test VQE: set attribute of AO VQE  */

    if(bEnableVqe)
    {
        memset(&stSetVqeConfig, 0, sizeof(MI_AO_VqeConfig_t));
        stSetVqeConfig.bHpfOpen = FALSE;
        stSetVqeConfig.bAnrOpen = TRUE;
        stSetVqeConfig.bAgcOpen = FALSE;
        stSetVqeConfig.bEqOpen = FALSE;

        stSetVqeConfig.s32WorkSampleRate = stSetAttr.eSamplerate;
        stSetVqeConfig.s32FrameSample = 128;

        stSetVqeConfig.stAnrCfg.bUsrMode = FALSE;
        stSetVqeConfig.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_LOW;
        stSetVqeConfig.stAnrCfg.u32NrIntensity = 10;
        stSetVqeConfig.stAnrCfg.u32NrSmoothLevel = 10;

        // need to check algorithm configure
        ExecFunc(MI_AO_SetVqeAttr(AoDevId, AoChn, &stSetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_GetVqeAttr(AoDevId, AoChn, &stGetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_EnableVqe(AoDevId, AoChn), MI_SUCCESS);
    }

    /* if test AO Volume */
    s32SetVolumeDb = s32VolumeDb; //

    if(0 != s32VolumeDb)
    {
        ExecFunc(MI_AO_SetVolume(AoDevId, s32SetVolumeDb), MI_SUCCESS);
        /* get AO volume */
        ExecFunc(MI_AO_GetVolume(AoDevId, &s32GetVolumeDb), MI_SUCCESS);

    }

    // test get output port buffer
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stAoChn0OutputPort0;
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13), MI_SUCCESS);

    // write wav header to output file
    if((fdWr > 0))
        write(fdWr, &stWavHeaderInput, sizeof(_WavHeader_t));

    if((fdRd > 0)  && (fdWr > 0))
    {

        _gstAoOutchn.fdWr = fdWr;
        _gstAoOutchn.AoDevId = AoDevId;
        _gstAoOutchn.AoChn = AoChn;
        _gstAoOutchn.bEndOfStream = FALSE;
        _gstAoOutchn.u32WriteTotalSize = 0;
        _gstAoOutchn.u32InputFileSize = stWavHeaderInput.data_size;
        _gstAoOutchn.u32InSampleRate = stWavHeaderInput.sample_rate / 1000;
        _gstAoOutchn.u32OutSampleRate = ((MI_U32)stSetAttr.eSamplerate) / 1000;
        _gstAoOutchn.bEndOfStream = TRUE;

        g_stAoInInfo.fdRd = fdRd;
        g_stAoInInfo.AoDevId = AoDevId;
        g_stAoInInfo.AoChn = AoChn;
        g_stAoInInfo.u32ChnCnt = stSetAttr.u32ChnCnt;
        g_stAoInInfo.eBitwidth = stGetAttr.eBitwidth;
        g_stAoInInfo.eSoundmode = stGetAttr.eSoundmode;
        g_stAoInInfo.eInSampleRate = eInSampleRate;
        g_stAoInInfo.eSamplerate = stSetAttr.eSamplerate;


        //create a thread to get output buffer
        _gstAoOutchn.bRunFlag = TRUE;
        pthread_create(&_gstAoOutchn.pt, NULL, ST_AoOutProc, &_gstAoOutchn);
        //create a thread to send input buffer
        g_stAoInInfo.bRunFlag = TRUE;
        pthread_create(&g_stAoInInfo.pt, NULL, ST_AoInProc, &g_stAoInInfo);

    }


    return MI_SUCCESS;
}

MI_S32 ST_AOExit(void)
{
    g_stAoInInfo.bRunFlag = FALSE;
    pthread_join(g_stAoInInfo.pt, NULL);

    _gstAoOutchn.bRunFlag = FALSE;
    pthread_join(_gstAoOutchn.pt, NULL);

    MI_U32 AoDevId = g_stAoInInfo.AoDevId;
    MI_U32 AoChn = g_stAoInInfo.AoChn;

    /* Disable Resample */
    if(bEnableRes)
    {
        ExecFunc(MI_AO_DisableReSmp(AoDevId, AoChn), MI_SUCCESS);
    }

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
    stWavHeaderOutput.sample_rate = g_stAoInInfo.eSamplerate;
    stWavHeaderOutput.data_size = _gstAoOutchn.u32WriteTotalSize;
    stWavHeaderOutput.ChunkSize = stWavHeaderOutput.data_size + 36;
    lseek(_gstAoOutchn.fdWr, 0, SEEK_SET);
    write(_gstAoOutchn.fdWr, &stWavHeaderOutput, sizeof(_WavHeader_t));

    if(g_stAoInInfo.fdRd > 0)
        close(g_stAoInInfo.fdRd);

    if(_gstAoOutchn.fdWr > 0)
        close(_gstAoOutchn.fdWr);


    return MI_SUCCESS;
}
#endif

int ST_VDFMDSadMdNumCal(MI_U8 *pu8MdRstData, int i, int j, int col, int row, int cusCol, int cusRow)
{
    int c, r;
    int rowIdx = 0;
    int sad8BitThr = 20;
    int mdNum = 0;

    for(r = 0; r < cusRow; r++)
    {
        rowIdx = (i + r) * col + j;

        for(c = 0; c < cusCol; c++)
        {
            if(pu8MdRstData[rowIdx + c] > sad8BitThr) mdNum ++;
        }
    }

    return mdNum;
}

/*
ST_Rect_T g_stRect[(RAW_W / 4) * (RAW_H / 4)]
*/
int ST_VDFGetMDRect(ST_Rect_T *pstRect)
{
	int mdCnt = 0;

	pthread_mutex_lock(&g_st_md_rect_mutex);

	mdCnt = g_md_detect_cnt;

	memcpy(pstRect, g_stRect, sizeof(g_stRect));

	pthread_mutex_unlock(&g_st_md_rect_mutex);

	return mdCnt;
}

int ST_VDFMDtoRECT_SAD(MI_U8 *pu8MdRstData, int col, int row)
{
    int i, j;
    MI_RGN_CanvasInfo_t *pstCanvasInfo = NULL;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32Width    = 0, u32Height = 0;
    MI_VPE_PortMode_t stVpeMode;
	ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    int cusCol = 4;        // 4 macro block Horizontal
    int cusRow = 2;     // 3 macro block vertical

    for(i = 0; i < MAX_FULL_RGN_NULL; i ++)
    {
        if(ST_OSD_HANDLE_INVALID == pstVDFOsdInfo[i].hHandle)
        {
            continue;
        }

        // clear osd
        ST_OSD_GetCanvasInfo(pstVDFOsdInfo[i].hHandle, &pstCanvasInfo);

        ST_OSD_Clear(pstVDFOsdInfo[i].hHandle, NULL);

        ST_OSD_Update(pstVDFOsdInfo[i].hHandle);
        // clear osd
    }

    // draw rect
    if(pu8MdRstData)
    {
    	pthread_mutex_lock(&g_st_md_rect_mutex);
    	g_md_detect_cnt = 0;
        for(i = 0; i < row; i += cusRow)
        {
            for(j = 0; j < col; j += cusCol)
            {
                // clac all macro block result
                if(ST_VDFMDSadMdNumCal(pu8MdRstData, i, j, col, row, cusCol, cusRow) > cusRow * cusCol / 2)
                {
#if 0
                    stRect[md_detect_cnt].u32X = (j * 1920 / col) & 0xFFFE;
                    stRect[md_detect_cnt].u32Y = (i * 1080 / row) & 0xFFFE;
                    stRect[md_detect_cnt].u16PicW = (cusCol * 1920 / col) & 0xFFFE;
                    stRect[md_detect_cnt].u16PicH = (cusRow * 1080 / row) & 0xFFFE;
#endif

                    g_stRect[g_md_detect_cnt].u32X = ((j * 1920) / col) & 0xFFFE;
                    g_stRect[g_md_detect_cnt].u32Y = ((i * 1080) / row) & 0xFFFE;
                    g_stRect[g_md_detect_cnt].u16PicW = ((cusCol * 1920) / col) & 0xFFFE;
                    g_stRect[g_md_detect_cnt].u16PicH = ((cusRow * 1080) / row) & 0xFFFE;
                    // printf("osd idx %d, x %d,y %d,w %d,h %d\n", md_detect_cnt, stRect[md_detect_cnt].u32X, stRect[md_detect_cnt].u32Y,
                    //                                stRect[md_detect_cnt].u16PicW, stRect[md_detect_cnt].u16PicH);

                    g_md_detect_cnt++;
                }
            }
        }

		pthread_mutex_unlock(&g_st_md_rect_mutex);
    }

    for(i = 0; i < MAX_FULL_RGN_NULL; i ++)
    {
        if(ST_OSD_HANDLE_INVALID == pstVDFOsdInfo[i].hHandle)
        {
            continue;
        }

        // get layer w&h
        if(pstVDFOsdInfo[i].eModId == E_MI_RGN_MODID_VPE)
        {
            memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
            s32Ret = MI_VPE_GetPortMode(pstVDFOsdInfo[i].u32Chn,
                                        pstVDFOsdInfo[i].u32Port, &stVpeMode);

            if(MI_SUCCESS != s32Ret)
            {
                ST_ERR("MI_VPE_SetPortMode err:%x, chn:%d, port:%d\n", s32Ret,
                       pstVDFOsdInfo[i].u32Chn, pstVDFOsdInfo[i].u32Port);
                return -1;
            }

            u32Width = stVpeMode.u16Width;
            u32Height = stVpeMode.u16Height;
        }
        else if(pstVDFOsdInfo[i].eModId == E_MI_RGN_MODID_DIVP)
        {
            memset(&stOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));

            s32Ret = MI_DIVP_GetOutputPortAttr(pstVDFOsdInfo[i].u32Chn, &stOutputPortAttr);

            if(MI_SUCCESS != s32Ret)
            {
                ST_ERR("MI_DIVP_SetOutputPortAttr err:%x, chn:%d\n", s32Ret, pstVDFOsdInfo[i].u32Chn);
                return -1;
            }

            u32Width = stOutputPortAttr.u32Width;
            u32Height = stOutputPortAttr.u32Height;
        }
        else
        {
            ST_ERR("not support\n");
            continue;
        }

        ST_OSD_GetCanvasInfo(pstVDFOsdInfo[i].hHandle, &pstCanvasInfo);

        for(j = 0; j < g_md_detect_cnt; j++)
        {
			//ST_DBG("handle:%d, chn:%d, port:%d, rect(%d, %d, %dx%d)\n", pstVDFOsdInfo[i].hHandle,
			//    pstVDFOsdInfo[i].u32Chn, pstVDFOsdInfo[i].u32Port,
			//    stRect[j].u32X, stRect[j].u32Y, stRect[j].u16PicW, stRect[j].u16PicH);

            ST_OSD_DrawRect(pstVDFOsdInfo[i].hHandle, g_stRect[j], 3, I4_RED);
        }

        ST_OSD_Update(pstVDFOsdInfo[i].hHandle);
    }

    UNUSED(u32Width);
    UNUSED(u32Height);
    return 0;
}

void *ST_DIVPGetResult(void *args)
{
	MI_SYS_ChnPort_t stChnPort;
	MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
	MI_S32 s32Ret = MI_SUCCESS;
	MI_S32 s32Fd = 0;
	fd_set read_fds;
    struct timeval TimeoutVal;
	char szFileName[128];
	int fd = 0;
	MI_U32 u32GetFramesCount = 0;
	MI_BOOL _bWriteFile = TRUE;

	stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = DIVP_CHN_FOR_VDF;
    stChnPort.u32PortId = 0;

	s32Ret = MI_SYS_GetFd(&stChnPort, &s32Fd);
    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_GetFd 0, error, %X\n", s32Ret);
        return NULL;
    }
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 3);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth err:%x, chn:%d,port:%d\n", s32Ret,
            stChnPort.u32ChnId, stChnPort.u32PortId);
        return NULL;
    }

	sprintf(szFileName, "divp%d.es", stChnPort.u32ChnId);
	printf("start to record %s\n", szFileName);
	fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0)
	{
		ST_ERR("create %s fail\n", szFileName);
	}

	while (1)
	{
		FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);

		TimeoutVal.tv_sec  = 1;
        TimeoutVal.tv_usec = 0;

        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);

		if(s32Ret < 0)
        {
            ST_ERR("select failed!\n");
            //  usleep(10 * 1000);
            continue;
        }
        else if(s32Ret == 0)
        {
            ST_ERR("get divp frame time out\n");
            //usleep(10 * 1000);
            continue;
        }
		else
		{
			if(FD_ISSET(s32Fd, &read_fds))
			{
				s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);

				if(MI_SUCCESS != s32Ret)
                {
                    //ST_ERR("MI_SYS_ChnOutputPortGetBuf err, %x\n", s32Ret);
                    continue;
                }

				// save one Frame YUV data
		        if (fd > 0)
		        {
		        	if(_bWriteFile)
	        		{
	        			write(fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0] +
							stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
	        		}

		        }

				++u32GetFramesCount;
				printf("channelId[%u] u32GetFramesCount[%u]\n", stChnPort.u32ChnId, u32GetFramesCount);

				MI_SYS_ChnOutputPortPutBuf(stBufHandle);
			}
		}
	}

	if (fd > 0)
	{
		close(fd);
		fd = -1;
	}

	printf("exit record\n");
	return NULL;
}

void *ST_VDFGetResult(void *args)
{
    VDF_Thread_Args_t *pstArgs = (VDF_Thread_Args_t *)args;
    MI_VDF_Result_t stVdfResult;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U8 *pu8MdRstData = NULL;
    MI_VDF_CHANNEL vdfChn = pstArgs->vdfChn;
    MI_U32 buffer_size;
    MI_U32 col = 0;
    MI_U32 row = 0;
    MI_U8 stSadDataArry[(RAW_W / 4) * (RAW_H / 4) * 2] = {0,};

    if(pstArgs->enWorkMode == E_MI_VDF_WORK_MODE_MD)
    {
        ST_DBG("Get md result, chn:%d\n", vdfChn);

        // hard code, MDMB_MODE_MB_8x8
        col = pstArgs->u16Width >> 3;    // 48
        row = pstArgs->u16Height >> 3;    // 36

        buffer_size = col * row;    // MDSAD_OUT_CTRL_8BIT_SAD
    }
    else
    {
        ST_DBG("Get od result, chn:%d\n", vdfChn);
    }

    while(pstArgs->bRunFlag)
    {
        memset(&stVdfResult, 0x00, sizeof(stVdfResult));
        memset(&stSadDataArry, 0, sizeof(stSadDataArry));

        stVdfResult.enWorkMode = pstArgs->enWorkMode;
        s32Ret = MI_VDF_GetResult(vdfChn, &stVdfResult, 0);

        if((0 == s32Ret) &&
                ((1 == stVdfResult.stMdResult.u8Enable) ||
                 (1 == stVdfResult.stOdResult.u8Enable)))
        {
            if(pstArgs->enWorkMode == E_MI_VDF_WORK_MODE_MD)
            {
                pu8MdRstData = (MI_U8 *)stVdfResult.stMdResult.pstMdResultSad->paddr;
#if 0
                printf("[MD_TEST][HDL=0xA0] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                       stVdfResult.stMdResult.u64Pts,      \
                       stVdfResult.enWorkMode,                         \
                       stVdfResult.stMdResult.u8Enable);

                // col = (pstArgs->u16Width >> 1) >> (1 + 2);
                // row = pstArgs->u16Height >> (1 + 2);
                // buffer_size = col * row * (2 - 1);


                // printf("buffer_size:%d\n", buffer_size);
#endif

                memcpy(stSadDataArry, pu8MdRstData, buffer_size);
            }
            else
            {
#if 0
                printf("[OD_TEST][HDL=02] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
                       stVdfResult.unVdfResult.stOdResult.u64Pts,      \
                       stVdfResult.enWorkMode,                         \
                       stVdfResult.unVdfResult.stOdResult.u8Enable,    \
                       stVdfResult.unVdfResult.stOdResult.u8DataLen,   \
                       stVdfResult.unVdfResult.stOdResult.u8WideDiv,   \
                       stVdfResult.unVdfResult.stOdResult.u8HightDiv);
                printf("{%u %u %u  %u %u %u  %u %u %u}\n",
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
                       stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
#endif
            }

            MI_VDF_PutResult(vdfChn, &stVdfResult);

            ST_VDFMDtoRECT_SAD(stSadDataArry, col, row);
        }
        else
        {
            // printf("[MD_TEST][HDL=0x0] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, s32Ret);
        }

        usleep(30 * 1000);
    }

    return NULL;
}

MI_S32 ST_VdfStart(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVdfSettingAttr);
    MI_U32 i,j = 0;
    ST_Sys_Input_E enInput = ST_Sys_Input_BUTT;
    ST_Sys_BindInfo_T stBindInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDF_CHANNEL vdfChn = 0;
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_SYS_WindowRect_t stRect;

    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    MI_U32 u32CaseIndex = 2;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;

    // create DIVP for vdf
    divpChn = DIVP_CHN_FOR_VDF;

    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0;
    stDivpChnAttr.stCropRect.u16Y       = 0;
    stDivpChnAttr.stCropRect.u16Width   = 0;//g_stVdfSettingAttr[0].stVdfArgs.u16VdfInWidth;
    stDivpChnAttr.stCropRect.u16Height  = 0;//g_stVdfSettingAttr[0].stVdfArgs.u16VdfInHeight;
    stDivpChnAttr.u32MaxWidth           = g_stVdfSettingAttr[0].stVdfArgs.u16VdfInWidth;
    stDivpChnAttr.u32MaxHeight          = g_stVdfSettingAttr[0].stVdfArgs.u16VdfInHeight;

    ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = g_stVdfSettingAttr[0].stVdfArgs.u16VdfInWidth;
    stOutputPortAttr.u32Height          = g_stVdfSettingAttr[0].stVdfArgs.u16VdfInHeight;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));

    if (pstStreamAttr[u32CaseIndex].bEnable == FALSE)
    {
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = pstStreamAttr[u32CaseIndex].u32InputChn;
        stVpePortInfo.u16OutputWidth = pstStreamAttr[u32CaseIndex].u32Width;
        stVpePortInfo.u16OutputHeight = pstStreamAttr[u32CaseIndex].u32Height;
        printf("Vpe create port w %d h %d\n", stVpePortInfo.u16OutputWidth, stVpePortInfo.u16OutputHeight);

        if (pstStreamAttr[u32CaseIndex].eType == E_MI_VENC_MODTYPE_JPEGE &&
            pstStreamAttr[u32CaseIndex].eBindType == E_MI_SYS_BIND_TYPE_REALTIME)
        {
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        }
        else
        {
            stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        }

        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        if (pstStreamAttr[u32CaseIndex].u32CropWidth != 0 &&
            pstStreamAttr[u32CaseIndex].u32CropHeight != 0)
        {
            stRect.u16Width = pstStreamAttr[u32CaseIndex].u32CropWidth;
            stRect.u16Height = pstStreamAttr[u32CaseIndex].u32CropHeight;
            stRect.u16X = pstStreamAttr[u32CaseIndex].u32CropX;
            stRect.u16Y = pstStreamAttr[u32CaseIndex].u32CropY;
            ExecFunc(MI_VPE_SetPortCrop(pstStreamAttr[u32CaseIndex].u32InputChn,
                                        pstStreamAttr[u32CaseIndex].u32InputPort, &stRect), MI_SUCCESS);
        }
        STCHECKRESULT(ST_Vpe_StartPort(pstStreamAttr[u32CaseIndex].u32InputPort, &stVpePortInfo));
    }

    // bind VPE to divp
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[u32CaseIndex].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[u32CaseIndex].u32InputPort;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;

    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    stBindInfo.u32BindParam = 0;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    /************************************************
    Step1:  start VPE port ,bind vpe -> vdf, enable vdf
    *************************************************/

    for (i = 0; i < u32ArraySize; i ++)
    {
        enInput = g_stVdfSettingAttr[i].enInput;

        if (enInput == ST_Sys_Input_DIVP)
        {
            for(j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16MdNum; j++)
            {
                vdfChn = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].u32Chn;
                memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = g_stVdfSettingAttr[i].u32InputChn;
                stBindInfo.stSrcChnPort.u32PortId = g_stVdfSettingAttr[i].u32InputPort;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
                stBindInfo.stDstChnPort.u32DevId = 0;
                stBindInfo.stDstChnPort.u32ChnId = vdfChn;
                stBindInfo.stDstChnPort.u32PortId = 0;

                stBindInfo.u32SrcFrmrate = 30;
                stBindInfo.u32DstFrmrate = 5;
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                stBindInfo.u32BindParam = 0;
                ST_DBG("divp chn-port:(%d %d) vdf chn-port:(%d %d)\n", g_stVdfSettingAttr[i].u32InputChn, g_stVdfSettingAttr[i].u32InputPort, vdfChn, 0);
                STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

                if(MI_SUCCESS != (s32Ret = MI_VDF_EnableSubWindow(stBindInfo.stDstChnPort.u32ChnId, 0, 0, 1)))
                {
                    ST_ERR("MI_VDF_EnableSubWindow err, chn %d, %x\n", stBindInfo.stDstChnPort.u32ChnId, s32Ret);
                    return 1;
                }
                else
                {
                    ST_DBG("MI_VDF_EnableSubWindow ok, chn %d, %x\n", stBindInfo.stDstChnPort.u32ChnId, s32Ret);
                }

                g_stVdfThreadArgs[vdfChn].enWorkMode = E_MI_VDF_WORK_MODE_MD;
                g_stVdfThreadArgs[vdfChn].vdfChn = vdfChn;
                g_stVdfThreadArgs[vdfChn].bRunFlag = TRUE;
                g_stVdfThreadArgs[vdfChn].u16Width = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInWidth;
                g_stVdfThreadArgs[vdfChn].u16Height = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInHeight;

                pthread_create(&g_stVdfThreadArgs[vdfChn].pt, NULL, ST_VDFGetResult, (void *)&g_stVdfThreadArgs[vdfChn]);
            }

            for(j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16OdNum; j++)
            {
                vdfChn = g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].u32Chn;

                memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = g_stVdfSettingAttr[i].u32InputChn;
                stBindInfo.stSrcChnPort.u32PortId = g_stVdfSettingAttr[i].u32InputPort;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
                stBindInfo.stDstChnPort.u32DevId = 0;
                stBindInfo.stDstChnPort.u32ChnId = vdfChn;
                stBindInfo.stDstChnPort.u32PortId = 0;

                stBindInfo.u32SrcFrmrate = 30;
                stBindInfo.u32DstFrmrate = 5;

                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                stBindInfo.u32BindParam = 0;
                ST_DBG("vpe chn-port:(%d %d) vdf chn-port:(%d %d)\n", g_stVdfSettingAttr[i].u32InputChn, g_stVdfSettingAttr[i].u32InputPort, vdfChn, 0);
                STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

                if(MI_SUCCESS != (s32Ret = MI_VDF_EnableSubWindow(stBindInfo.stDstChnPort.u32ChnId, 0, 0, 1)))
                {
                    ST_ERR("MI_VDF_EnableSubWindow err, chn %d, %x\n", stBindInfo.stDstChnPort.u32ChnId, s32Ret);
                    return 1;
                }
                else
                {
                    ST_DBG("MI_VDF_EnableSubWindow ok, chn %d, %x\n", stBindInfo.stDstChnPort.u32ChnId, s32Ret);
                }

                g_stVdfThreadArgs[vdfChn].enWorkMode = E_MI_VDF_WORK_MODE_OD;
                g_stVdfThreadArgs[vdfChn].vdfChn = vdfChn;
                g_stVdfThreadArgs[vdfChn].bRunFlag = TRUE;
                g_stVdfThreadArgs[vdfChn].u16Width = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInWidth;
                g_stVdfThreadArgs[vdfChn].u16Height = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInHeight;

                pthread_create(&g_stVdfThreadArgs[vdfChn].pt, NULL, ST_VDFGetResult, (void *)&g_stVdfThreadArgs[vdfChn]);
            }
        }
    }

    return MI_SUCCESS;
}

MI_S32 ST_ModuleInit_VDF_MDOD_Rect(void)
{
    MI_U32 i,j = 0;
    MI_U32 u32Chn = 0;

    ST_Rect_T stArea = {
        .u32X = 0,
        .u32Y = 0,
        .u16PicW = 384,
        .u16PicH = 288,
    };

    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVdfSettingAttr);

    for (i = 0; i < u32ArraySize; i++)
    {
        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16MdNum; j++)
        {
            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].u32Chn = u32Chn;
            u32Chn++;
            memcpy(&g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea, &stArea, sizeof(ST_Rect_T));
        }

        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16OdNum; j++)
        {
            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].u32Chn = u32Chn;
            u32Chn++;
            memcpy(&g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea, &stArea, sizeof(ST_Rect_T));
        }

        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16VgNum; j++)
        {
            g_stVdfSettingAttr[i].stVdfArgs.stVgArea[j].u32Chn = u32Chn;
            u32Chn++;
            memcpy(&g_stVdfSettingAttr[i].stVdfArgs.stVgArea[j].stArea, &stArea, sizeof(ST_Rect_T));
        }
    }
    return MI_SUCCESS;
}

MI_S32 ST_ModuleInit_VDF(void)
{
    MI_VDF_ChnAttr_t stVdfAttr;
    MI_VDF_CHANNEL vdfChn = 0;
    MI_U32 i = 0, j = 0;
    //int mdTotalNum = 0, odTotalNum = 0;
    MI_S32 s32Ret = 0;
    MI_U32 version;

    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVdfSettingAttr);
    STCHECKRESULT(ST_ModuleInit_VDF_MDOD_Rect());
    STCHECKRESULT(MI_VDF_Init());

    for (i = 0; i < u32ArraySize; i++)
    {
        // create md chn
        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16MdNum; j ++)
        {
            vdfChn = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].u32Chn;

            memset(&stVdfAttr, 0, sizeof(MI_VDF_ChnAttr_t));
            stVdfAttr.enWorkMode = E_MI_VDF_WORK_MODE_MD;
            stVdfAttr.stMdAttr.u8Enable    = 1;
            stVdfAttr.stMdAttr.u8MdBufCnt  = 4;
            stVdfAttr.stMdAttr.u8VDFIntvl  = 0;

            stVdfAttr.stMdAttr.ccl_ctrl.u16InitAreaThr = 8;
            stVdfAttr.stMdAttr.ccl_ctrl.u16Step = 2;

            stVdfAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
            stVdfAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
            stVdfAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
            stVdfAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

            stVdfAttr.stMdAttr.stMdStaticParamsIn.width   =
                                        g_stVdfSettingAttr[i].stVdfArgs.u16VdfInWidth;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.height  =
                                        g_stVdfSettingAttr[i].stVdfArgs.u16VdfInHeight;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.stride  = g_stVdfSettingAttr[i].stVdfArgs.u16stride;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.color   = 1;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X +
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicW - 1;
            //720 - 1;//g_width - 1;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X +
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicW - 1;
            // 720 - 1;//g_width - 1;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y +
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicH - 1;
            //576 - 1;//g_height - 1;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X;
            //0;
            stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y +
                                            g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicH - 1;
            //576 - 1;//g_height - 1;

            ST_DBG("MD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.width,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.height,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y);

            if (MI_SUCCESS != (s32Ret = MI_VDF_CreateChn(vdfChn, &stVdfAttr)))
            {
                ST_ERR("MI_VDF_CreateChn err, chn %d, %x\n", vdfChn, s32Ret);
                return 1;
            }

            MI_VDF_GetLibVersion(vdfChn, &version);

            ST_DBG("MD MI_VDF_CreateChn success, chn %d\n", vdfChn);
        }
        //mdTotalNum += g_stVdfSettingAttr[i].stVdfArgs.u16MdNum;

        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16OdNum; j ++)
        {
            vdfChn = g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].u32Chn;

            stVdfAttr.enWorkMode = E_MI_VDF_WORK_MODE_OD;
            stVdfAttr.stOdAttr.u8OdBufCnt  = 4;
            stVdfAttr.stOdAttr.u8VDFIntvl  = 0;

            stVdfAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 3;
            stVdfAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
            stVdfAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

            stVdfAttr.stOdAttr.stOdStaticParamsIn.inImgW =
                                            g_stVdfSettingAttr[i].stVdfArgs.u16VdfInWidth;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.inImgH =
                                            g_stVdfSettingAttr[i].stVdfArgs.u16VdfInHeight;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.inImgStride = g_stVdfSettingAttr[i].stVdfArgs.u16stride;;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.nClrType = (ODColor_e) 1;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.M = 120;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.MotionSensitivity = 0;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.num = 4;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32X;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32Y;//0;

            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32X +
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u16PicW - 1;//g_width - 1;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32Y;//0;

            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32X +
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u16PicW - 1;//g_width - 1;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32Y +
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u16PicH - 1;//g_height - 1;

            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32X;//0;
            stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y =
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u32Y +
                                            g_stVdfSettingAttr[i].stVdfArgs.stOdArea[j].stArea.u16PicH - 1;//g_height - 1;

            ST_DBG("OD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.width,
                   stVdfAttr.stMdAttr.stMdStaticParamsIn.height,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x,
                   stVdfAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y);

            if (MI_SUCCESS != (s32Ret = MI_VDF_CreateChn(vdfChn, &stVdfAttr)))
            {
                ST_ERR("MI_VDF_CreateChn err, chn %d, %x\n", vdfChn, s32Ret);
                return 1;
            }
            MI_VDF_GetLibVersion(vdfChn, &version);

            ST_DBG("OD MI_VDF_CreateChn success, chn %d\n", vdfChn);

        }

        for (j = 0; j < g_stVdfSettingAttr[i].stVdfArgs.u16VgNum; j ++)
        {
            vdfChn = g_stVdfSettingAttr[i].stVdfArgs.stVgArea[j].u32Chn;

            stVdfAttr.enWorkMode = E_MI_VDF_WORK_MODE_VG;
            stVdfAttr.stVgAttr.u8VgBufCnt  = 4;
            stVdfAttr.stVgAttr.u8VDFIntvl  = 0;

            stVdfAttr.stVgAttr.height = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInHeight;
            stVdfAttr.stVgAttr.width = g_stVdfSettingAttr[i].stVdfArgs.u16VdfInWidth;
            stVdfAttr.stVgAttr.stride = g_stVdfSettingAttr[i].stVdfArgs.u16stride;

            stVdfAttr.stVgAttr.object_size_thd = 3;
            stVdfAttr.stVgAttr.indoor = 1;
            stVdfAttr.stVgAttr.function_state = VG_VIRTUAL_GATE;
            stVdfAttr.stVgAttr.line_number = 2;

            if( stVdfAttr.stVgAttr.function_state == VG_VIRTUAL_GATE )
            {
                if( stVdfAttr.stVgAttr.line_number >= 1 )
                {
                    //First Line
                    stVdfAttr.stVgAttr.line[0].px.x = 15;
                    stVdfAttr.stVgAttr.line[0].px.y = 95;
                    stVdfAttr.stVgAttr.line[0].py.x = 115;
                    stVdfAttr.stVgAttr.line[0].py.y = 95;
                    stVdfAttr.stVgAttr.line[0].pdx.x = 50;
                    stVdfAttr.stVgAttr.line[0].pdx.y = 150;
                    stVdfAttr.stVgAttr.line[0].pdy.x = 50;
                    stVdfAttr.stVgAttr.line[0].pdy.y = 15;
                }

                if( stVdfAttr.stVgAttr.line_number == 2 )
                {
                    //Second Line
                    stVdfAttr.stVgAttr.line[1].px.x = 220;
                    stVdfAttr.stVgAttr.line[1].px.y = 100;
                    stVdfAttr.stVgAttr.line[1].py.x = 225;
                    stVdfAttr.stVgAttr.line[1].py.y = 50;
                    stVdfAttr.stVgAttr.line[1].pdx.x = 220;
                    stVdfAttr.stVgAttr.line[1].pdx.y = 50;
                    stVdfAttr.stVgAttr.line[1].pdy.x = 230;
                    stVdfAttr.stVgAttr.line[1].pdy.y = 50;
                }
            }
            else  //VG_REGION_INVASION
            {
                stVdfAttr.stVgAttr.vg_region.p_one.x = 15;
                stVdfAttr.stVgAttr.vg_region.p_one.y = 20;
                stVdfAttr.stVgAttr.vg_region.p_two.x = 115;
                stVdfAttr.stVgAttr.vg_region.p_two.y = 20;
                stVdfAttr.stVgAttr.vg_region.p_three.x = 115;
                stVdfAttr.stVgAttr.vg_region.p_three.y = 95;
                stVdfAttr.stVgAttr.vg_region.p_four.x = 15;
                stVdfAttr.stVgAttr.vg_region.p_four.y = 95;

                //Set region direction
                stVdfAttr.stVgAttr.vg_region.region_dir = VG_REGION_ENTER;
                //vg_region.region_dir = VG_REGION_LEAVING;
                //vg_region.region_dir = VG_REGION_CROSS;
            }

            if (MI_SUCCESS != (s32Ret = MI_VDF_CreateChn(vdfChn, &stVdfAttr)))
            {
                ST_ERR("MI_VDF_CreateChn err, chn %d, %x\n", vdfChn, s32Ret);
                return 1;
            }
            MI_VDF_GetLibVersion(vdfChn, &version);

            ST_DBG("OD MI_VDF_CreateChn success, chn %d\n", vdfChn);

        }
        //odTotalNum += g_stVdfSettingAttr[i].stVdfArgs.u16OdNum;
    }

    STCHECKRESULT(MI_VDF_Run(E_MI_VDF_WORK_MODE_MD));
    STCHECKRESULT(MI_VDF_Run(E_MI_VDF_WORK_MODE_OD));
    // STCHECKRESULT(MI_VDF_Run(E_MI_VDF_WORK_MODE_VG));
    return MI_SUCCESS;
}

MI_S32 ST_PanelDispStart(MI_SYS_PixelFormat_e enPixelFormat, MI_S32 s32Rotate)
{
    return 0;
}

MI_S32 ST_PanelDispStop(void)
{
    return 0;
}

void ST_DefaultConfig(ST_Config_S *pstConfig)
{
    pstConfig->s32UseOnvif     = 0;
    pstConfig->s32UseVdf    = 0;
#if USE_AUDIO
    pstConfig->s32UseAudio    = 0;
#endif
    pstConfig->s32LoadIQ    = 0;
#if (SHOW_PANEL == 1)
    pstConfig->s32UsePanel    = 0;
    pstConfig->enPixelFormat= E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    pstConfig->s32Rotate    = 0;
#endif
    pstConfig->s32HDRtype    = 0;
    pstConfig->enSensorType = ST_Sensor_Type_IMX291;
    pstConfig->enRotation = E_MI_SYS_ROTATE_NONE;
}

void ST_DefaultArgs(ST_Config_S *pstConfig)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
	MI_U32 i = 0, j = 0;
    MI_U32 u32Ret = 0;
    float f32Ret;

    memset(pstConfig, 0, sizeof(ST_Config_S));
    ST_DefaultConfig(pstConfig);

    pstConfig->s32UseOnvif = 0;
    pstConfig->s32UseVdf = 1;
    pstConfig->s32LoadIQ = 0;
    pstConfig->enRotation = E_MI_SYS_ROTATE_NONE;
    pstConfig->en3dNrLevel = E_MI_VPE_3DNR_LEVEL1;

    pstStreamAttr[0].bEnable = FALSE;

    pstStreamAttr[1].bEnable = TRUE;
    printf("0)H264. 1)H265.\n");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(u32Ret == 0)
        pstStreamAttr[1].eType = E_MI_VENC_MODTYPE_H264E;
    else
        pstStreamAttr[1].eType = E_MI_VENC_MODTYPE_H265E;

    printf("Width:");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(320 <= u32Ret && u32Ret <= 2560)
        pstStreamAttr[1].u32Width = u32Ret;
    else
        pstStreamAttr[1].u32Width = 1920;

    printf("Height:");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(240 <= u32Ret && u32Ret <= 1536)
        pstStreamAttr[1].u32Height = u32Ret;
    else
        pstStreamAttr[1].u32Height = 1080;

    pstStreamAttr[1].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    pstStreamAttr[1].u32BindPara = pstStreamAttr[1].u32Height;

    printf("0)CBR. 1)VBR.\n");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(u32Ret == 0)
    {
        if(pstStreamAttr[1].eType == E_MI_VENC_MODTYPE_H264E)
            pstStreamAttr[1].eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        else
            pstStreamAttr[1].eRcMode = E_MI_VENC_RC_MODE_H265CBR;
    }
    else
    {
        if(pstStreamAttr[1].eType == E_MI_VENC_MODTYPE_H264E)
            pstStreamAttr[1].eRcMode = E_MI_VENC_RC_MODE_H264VBR;
        else
            pstStreamAttr[1].eRcMode = E_MI_VENC_RC_MODE_H265VBR;
    }

    printf("BitRate(Mbps):\n");
    scanf("%f", &f32Ret);
    ST_Flush();
    if(0 < f32Ret && f32Ret < 10)
        pstStreamAttr[1].f32Mbps = f32Ret;
    else
        pstStreamAttr[1].f32Mbps = 2;

    printf("Fps:\n");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(0 < u32Ret && u32Ret <= 30)
        pstStreamAttr[1].u32Fps = u32Ret;
    else
        pstStreamAttr[1].f32Mbps = 2;


    printf("Gop:\n");
    scanf("%u", &u32Ret);
    ST_Flush();
    if(10 < u32Ret && u32Ret < 100)
        pstStreamAttr[1].u32Gop = u32Ret;
    else
        pstStreamAttr[1].u32Gop = 50;

    pstStreamAttr[2].bEnable = FALSE;
    //pstStreamAttr[2].eType = E_MI_VENC_MODTYPE_H264E;
    //pstStreamAttr[2].u32Width = 704;
    //pstStreamAttr[2].u32Height = 576;
    //pstStreamAttr[2].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    //pstStreamAttr[2].u32BindPara = 0;
    //pstStreamAttr[2].enInput = ST_Sys_Input_DIVP;

	for(i = 0; i < MAX_CHN_NEED_OSD; i ++)
    {
        for(j = 0; j < MAX_OSD_NUM_PER_CHN; j ++)
        {
            g_stRgnOsd.stOsdInfo[i][j].hHandle = ST_OSD_HANDLE_INVALID;
        }
    }

	for (i = 0; i < MAX_FULL_RGN_NULL; i ++)
	{
		g_stVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
	}
}

void ST_ResetArgs(ST_Config_S *pstConfig)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0, j = 0;
    MI_S32 s32GetData = 0;
    MI_U32 u32Temp = 0;

    memset(pstConfig, 0, sizeof(ST_Config_S));
    ST_DefaultConfig(pstConfig);

    printf("Use onvif ?\n 0 not use, 1 use\n");
    scanf("%d", &pstConfig->s32UseOnvif);
    ST_Flush();
    printf("You select %s onvif\n", pstConfig->s32UseOnvif ? "use" : "not use");

    printf("Use VDF ?\n 0 not use, 1 use\n");
    scanf("%d", &pstConfig->s32UseVdf);
    ST_Flush();
    printf("You select %s VDF\n", pstConfig->s32UseVdf ? "use" : "not use");

    printf("Load IQ file ?\n 0 not load, 1 load\n");
    scanf("%d", &pstConfig->s32LoadIQ);
    ST_Flush();
    printf("You select %s IQ file, %s\n", pstConfig->s32LoadIQ ? "load" : "not load", IQ_FILE_PATH);

    printf("Rotation ?\n 0 no rotation, 1 rotate 90, 2: rotate 180, 3: rotate 270\n");
    scanf("%d", &s32GetData);
    ST_Flush();
    printf("You select rotate %s\n", s32GetData == 1 ? "rotate 90" :
                                    s32GetData == 2 ? "rotate 180" :
                                    s32GetData == 3 ? "rotate 270" : "no rotate");
    pstConfig->enRotation = (s32GetData >= 0 && s32GetData < 4) ? (MI_SYS_Rotate_e)s32GetData : E_MI_SYS_ROTATE_NONE;

    printf("3D NR level ?\n[0-2]:\n");
    scanf("%d", &s32GetData);
    ST_Flush();
    if (s32GetData > E_MI_VPE_3DNR_LEVEL2)
    {
        pstConfig->en3dNrLevel = E_MI_VPE_3DNR_LEVEL2;
        printf("You select level error use default level 1\n");
    }
    else
    {
        printf("You select level %d\n", s32GetData);
        pstConfig->en3dNrLevel = (MI_VPE_3DNR_Level_e)s32GetData;
    }

#if (SHOW_PANEL == 1)
    printf("Use panel ?\n 0 not use, 1 use\n");
    scanf("%d", &pstConfig->s32UsePanel);
    ST_Flush();
    printf("You select %s panel to show video\n", pstConfig->s32UsePanel ? "use" : "not use");

    if(pstConfig->s32UsePanel == 1)
    {
        printf("Which format to panel ?\n 0 E_MI_SYS_PIXEL_FRAME_YUV422_YUYV, 1 E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420\n");
        scanf("%d", &s32GetData);
        ST_Flush();
        printf("You select %s to panel\n", s32GetData ? "E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420" : "E_MI_SYS_PIXEL_FRAME_YUV422_YUYV");
        if(s32GetData == 0)
        {
            pstConfig->enPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        }
        else
        {
            pstConfig->enPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        }

        printf("Use rotate ?\n 0 not use, 1 use\n");
        scanf("%d", &pstConfig->s32Rotate);
        ST_Flush();
        printf("You select %s rotate\n", pstConfig->s32Rotate ? "use" : "not use");
    }
#endif

    //
    for(i = 0; i < u32ArraySize; i ++)
    {
        printf("\033[1;32mDo VPE port %d STREAM: %s\n\033[0m", pstStreamAttr[i].u32InputPort, pstStreamAttr[i].pszStreamName);
        printf("Start-up ?\n0 disable, 1 enable.");
        scanf("%d", &s32GetData);
        ST_Flush();
        if (s32GetData == 0)
        {
            pstStreamAttr[i].bEnable = FALSE;
            continue;
        }
        printf("Encode with ?\n0 h264, 1 h265 2 mjpeg\n");
        scanf("%d", &s32GetData);
        ST_Flush();
        if(s32GetData == 0)
        {
            pstStreamAttr[i].eType = E_MI_VENC_MODTYPE_H264E;
        }
        else if(s32GetData == 1)
        {
            pstStreamAttr[i].eType = E_MI_VENC_MODTYPE_H265E;
        }
        else if(s32GetData == 2)
        {
            pstStreamAttr[i].eType = E_MI_VENC_MODTYPE_JPEGE;
        }

        printf("Encode width ? \n");
        scanf("%d", &s32GetData);
        ST_Flush();
        if (s32GetData > 176 && s32GetData < 3840)
        {
            pstStreamAttr[i].u32Width = s32GetData;
        }
        printf("Encode height ? \n");
        scanf("%d", &s32GetData);
        ST_Flush();
        if (s32GetData > 144 && s32GetData < 2160)
        {
            pstStreamAttr[i].u32Height = s32GetData;
        }
        printf("Encode size, %dx%d\n", pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height);

        printf("Mode ?\n0 frame mode, 1 ring mode, 2 imi mode, 3 with dip\n");
        scanf("%d", &s32GetData);
        ST_Flush();
        if(s32GetData == 0)
        {
            pstStreamAttr[i].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            pstStreamAttr[i].u32BindPara = 0;
        }
        else if(s32GetData == 1)
        {
            pstStreamAttr[i].eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
            pstStreamAttr[i].u32BindPara = pstStreamAttr[i].u32Height;
        }
        else if(s32GetData == 2)
        {
            pstStreamAttr[i].eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
            pstStreamAttr[i].u32BindPara = 0;
        }
        else if(s32GetData == 3)
        {
            pstStreamAttr[i].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            pstStreamAttr[i].u32BindPara = 0;
            pstStreamAttr[i].enInput = ST_Sys_Input_DIVP;
        }
    }
    if (E_MI_SYS_ROTATE_90 == pstConfig->enRotation || E_MI_SYS_ROTATE_270 == pstConfig->enRotation)
    {
        for(i = 0; i < u32ArraySize; i ++)
        {
            if (pstStreamAttr[i].bEnable == FALSE)
            {
                continue;
            }

            u32Temp = pstStreamAttr[i].u32Width;
            pstStreamAttr[i].u32Width = ALIGN_DOWN(pstStreamAttr[i].u32Height, 32);
            pstStreamAttr[i].u32Height = ALIGN_DOWN(u32Temp, 16);

            if (pstStreamAttr[i].u32InputPort == 0)
            {
                pstStreamAttr[i].u32CropWidth = pstStreamAttr[i].u32Width;
                pstStreamAttr[i].u32CropHeight= pstStreamAttr[i].u32Height;
                pstStreamAttr[i].u32CropX = pstStreamAttr[i].u32CropY = 0;
            }
        }
    }

    memset(&g_stRgnOsd, 0, sizeof(ST_RGN_Osd_T));

    for(i = 0; i < MAX_CHN_NEED_OSD; i ++)
    {
        for(j = 0; j < MAX_OSD_NUM_PER_CHN; j ++)
        {
            g_stRgnOsd.stOsdInfo[i][j].hHandle = ST_OSD_HANDLE_INVALID;
        }
    }

	for (i = 0; i < MAX_FULL_RGN_NULL; i ++)
	{
		g_stVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
	}
}

void ST_HandleSig(MI_S32 signo)
{
    if(signo == SIGINT)
    {
        ST_INFO("catch Ctrl + C, exit normally\n");

        g_bExit = TRUE;
    }
}

MI_BOOL ST_DoCaptureJPGProcExt(MI_U16 u16Width, MI_U16 u16Hight, MI_SYS_Rotate_e enRotation)
{
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    MI_RGN_HANDLE hRgnHandle;
    MI_RGN_ChnPort_t stRgnChnPort;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_VENC_ChnAttr_t stChnAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_U32 u32Width = u16Width;
    MI_U32 u32Height = u16Hight;

    if (pstStreamAttr[0].bEnable == TRUE)
    {
        printf("Stream0 is used, IMI mode not support!\n");
        return FALSE;
    }

#if 0
    printf("=======================begin mma_heap_name0=================\n");
    system("cat /proc/mi_modules/mi_sys_mma/mma_heap_name0");
    printf("=======================begin mma_heap_name0=================\n");
#endif

    // port 0 can not scale, set cap width/height
    CanvasScopeLock ScopeLock;

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        u32Width = ALIGN_DOWN(u16Hight, 32);
        u32Height = u16Width;

        stRect.u16Width = ALIGN_DOWN(g_u32CapHeight, 32);
        stRect.u16Height = g_u32CapWidth;
        stRect.u16X = 0;
        stRect.u16Y = 0;
        ExecFunc(MI_VPE_SetPortCrop(0, 0, &stRect), MI_SUCCESS);
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = 0;
        stVpePortInfo.u16OutputWidth = ALIGN_DOWN(g_u32CapHeight, 32);
        stVpePortInfo.u16OutputHeight = g_u32CapWidth;
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        STCHECKRESULT(ST_Vpe_StartPort(0, &stVpePortInfo));
    }
    else
    {
        stRect.u16Width = u32Width;
        stRect.u16Height = u32Height;

        stRect.u16X = 0;
        stRect.u16Y = 0;
        ExecFunc(MI_VPE_SetPortCrop(0, 0, &stRect), MI_SUCCESS);
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = 0;
        stVpePortInfo.u16OutputWidth = g_u32CapWidth;
        stVpePortInfo.u16OutputHeight = g_u32CapHeight;
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        STCHECKRESULT(ST_Vpe_StartPort(0, &stVpePortInfo));
    }

    memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));

    hRgnHandle = RGN_OSD_HANDLE;
    stRgnChnPortParam.stPoint.u32X = u32Width - RGN_OSD_TIME_WIDTH - 10;
    stRgnChnPortParam.stPoint.u32Y = 10;
    stRgnChnPortParam.bShow = TRUE;
    // fix at vpe port 0
    memset(&stRgnChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stRgnChnPort.eModId = E_MI_RGN_MODID_VPE;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = pstStreamAttr[0].u32InputChn;;
    stRgnChnPort.s32OutputPortId = pstStreamAttr[0].u32InputPort;
    ExecFunc(MI_RGN_AttachToChn(hRgnHandle, &stRgnChnPort, &stRgnChnPortParam), MI_RGN_OK);

    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[0].u32Cover1Handle, &stRgnAttr), MI_RGN_OK);
    stRgnChnPort.eModId = E_MI_RGN_MODID_VPE;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = pstStreamAttr[0].u32InputChn;
    stRgnChnPort.s32OutputPortId = pstStreamAttr[0].u32InputPort;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 100;
    stChnPortParam.stPoint.u32Y = 100;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[0].u32Cover1Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 255, 0);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[0].u32Cover1Handle, &stRgnChnPort, &stChnPortParam), MI_RGN_OK);
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[0].u32Cover2Handle, &stRgnAttr), MI_RGN_OK);
    stRgnChnPort.eModId = E_MI_RGN_MODID_VPE;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = pstStreamAttr[0].u32InputChn;
    stRgnChnPort.s32OutputPortId = pstStreamAttr[0].u32InputPort;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 400;
    stChnPortParam.stPoint.u32Y = 400;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[0].u32Cover2Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 255);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[0].u32Cover2Handle, &stRgnChnPort, &stChnPortParam), MI_RGN_OK);

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
    STCHECKRESULT(ST_Venc_CreateChannel(VENC_CHN_FOR_CAPTURE, &stChnAttr));

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    ExecFunc(MI_VENC_GetChnDevid(VENC_CHN_FOR_CAPTURE, &stBindInfo.stDstChnPort.u32DevId), MI_SUCCESS);
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32ChnId = VENC_CHN_FOR_CAPTURE;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    ST_CaptureJPGProc(VENC_CHN_FOR_CAPTURE);

#if 0
    printf("=======================end mma_heap_name0=================\n");
    system("cat /proc/mi_modules/mi_sys_mma/mma_heap_name0");
    printf("=======================end mma_heap_name0=================\n");
#endif

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Venc_DestoryChannel(VENC_CHN_FOR_CAPTURE));
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover2Handle), MI_RGN_OK);
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover1Handle), MI_RGN_OK);

    hRgnHandle = RGN_OSD_HANDLE;
    memset(&stRgnChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stRgnChnPort.eModId = E_MI_RGN_MODID_VPE;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = pstStreamAttr[0].u32InputChn;
    stRgnChnPort.s32OutputPortId = pstStreamAttr[0].u32InputPort;
    ExecFunc(MI_RGN_DetachFromChn(hRgnHandle, &stRgnChnPort), MI_RGN_OK);

    STCHECKRESULT(ST_Vpe_StopPort(0, 0));

    return 0;
}

MI_BOOL ST_DoCaptureJPGProc(MI_U16 u16Width, MI_U16 u16Height, MI_SYS_Rotate_e enRotation)
{
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_HANDLE hRgnHandle;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPortParam_t stChnPortParam;

    MI_U32 u32Width = 0;
    MI_U32 u32Height = 0;

    // port 0 can not scale, set cap width/height
    CanvasScopeLock ScopeLock;

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        u32Width = ALIGN_DOWN(u16Height, 16);
        u32Height = u16Width;

        stRect.u16Width = ALIGN_DOWN(g_u32CapHeight, 16);
        stRect.u16Height = g_u32CapWidth;
        stRect.u16X = 0;
        stRect.u16Y = 0;
        ExecFunc(MI_VPE_SetPortCrop(0, 0, &stRect), MI_SUCCESS);
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = 0;
        stVpePortInfo.u16OutputWidth = ALIGN_DOWN(g_u32CapHeight, 16);
        stVpePortInfo.u16OutputHeight = g_u32CapWidth;
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    }
    else
    {
        u32Width = u16Width;
        u32Height = u16Height;

        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = 0;
        stVpePortInfo.u16OutputWidth = g_u32CapWidth;
        stVpePortInfo.u16OutputHeight = g_u32CapHeight;
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    }

    if (pstStreamAttr[0].bEnable == FALSE)
    {
        STCHECKRESULT(ST_Vpe_StartPort(0, &stVpePortInfo));
    }

    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0;
    stDivpChnAttr.stCropRect.u16Y       = 0;
    stDivpChnAttr.stCropRect.u16Width   = 0;
    stDivpChnAttr.stCropRect.u16Height  = 0;
    stDivpChnAttr.u32MaxWidth           = u32Width;
    stDivpChnAttr.u32MaxHeight          = u32Height;

    ExecFunc(MI_DIVP_CreateChn(DIVP_CHN_FOR_OSD, &stDivpChnAttr), MI_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(DIVP_CHN_FOR_OSD), MI_SUCCESS);

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = u32Width;
    stOutputPortAttr.u32Height          = u32Height;

    ST_DBG("u32Width:%d,u32Height:%d\n", u32Width, u32Height);
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DIVP_CHN_FOR_OSD, &stOutputPortAttr));

    // bind VPE to divp
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_OSD;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));

    hRgnHandle = RGN_OSD_HANDLE;
    stRgnChnPortParam.stPoint.u32X = u32Width - RGN_OSD_TIME_WIDTH - 10;
    stRgnChnPortParam.stPoint.u32Y = 10;
    stRgnChnPortParam.bShow = TRUE;
    memset(&stRgnChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stRgnChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = DIVP_CHN_FOR_OSD;
    stRgnChnPort.s32OutputPortId = 0;
    ExecFunc(MI_RGN_AttachToChn(hRgnHandle, &stRgnChnPort, &stRgnChnPortParam), MI_RGN_OK);

    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[0].u32Cover1Handle, &stRgnAttr), MI_RGN_OK);
    stRgnChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = DIVP_CHN_FOR_OSD;
    stRgnChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 100;
    stChnPortParam.stPoint.u32Y = 100;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[0].u32Cover1Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 255, 0);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[0].u32Cover1Handle, &stRgnChnPort, &stChnPortParam), MI_RGN_OK);
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[0].u32Cover2Handle, &stRgnAttr), MI_RGN_OK);
    stRgnChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = DIVP_CHN_FOR_OSD;
    stRgnChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 400;
    stChnPortParam.stPoint.u32Y = 400;
    stChnPortParam.unPara.stCoverChnPort.u32Layer = pstStreamAttr[0].u32Cover2Handle;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
    stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
    stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 255);
    ExecFunc(MI_RGN_AttachToChn(pstStreamAttr[0].u32Cover2Handle, &stRgnChnPort, &stChnPortParam), MI_RGN_OK);

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
    STCHECKRESULT(ST_Venc_CreateChannel(VENC_CHN_FOR_CAPTURE, &stChnAttr));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = DIVP_CHN_FOR_OSD;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    ExecFunc(MI_VENC_GetChnDevid(VENC_CHN_FOR_CAPTURE, &stBindInfo.stDstChnPort.u32DevId), MI_SUCCESS);
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32ChnId = VENC_CHN_FOR_CAPTURE;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    ST_CaptureJPGProc(VENC_CHN_FOR_CAPTURE);

    // system("echo dumpFrontBuf 1 0 0 /mnt/i6 > /proc/mi_modules/mi_rgn/mi_rgn0");
    // sleep(60);

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Venc_DestoryChannel(VENC_CHN_FOR_CAPTURE));
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover2Handle), MI_RGN_OK);
    ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover1Handle), MI_RGN_OK);
    memset(&stRgnChnPort, 0, sizeof(MI_RGN_ChnPort_t));

    hRgnHandle = RGN_OSD_HANDLE;
    stRgnChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = DIVP_CHN_FOR_OSD;
    stRgnChnPort.s32OutputPortId = 0;
    ExecFunc(MI_RGN_DetachFromChn(hRgnHandle, &stRgnChnPort), MI_RGN_OK);

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_OSD;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    ExecFunc(MI_DIVP_StopChn(DIVP_CHN_FOR_OSD), MI_SUCCESS);
    ExecFunc(MI_DIVP_DestroyChn(DIVP_CHN_FOR_OSD), MI_SUCCESS);
    if (pstStreamAttr[0].bEnable == FALSE)
    {
        STCHECKRESULT(ST_Vpe_StopPort(0, 0));
    }

    return 0;
}

int ST_DoStreamResize(MI_U32 u32Index, MI_U32 u32Width, MI_U32 u32Height)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;

    if (pstStreamAttr[u32Index].enInput == ST_Sys_Input_VPE)
    {
        STCHECKRESULT(ST_StopPipeLine((MI_U8)u32Index));
        STCHECKRESULT(ST_StartPipeLine((MI_U8)u32Index, u32Width, u32Height, pstStreamAttr[u32Index].u32CropWidth, pstStreamAttr[u32Index].u32CropHeight, pstStreamAttr[u32Index].u32CropX, pstStreamAttr[u32Index].u32CropY));

        pstStreamAttr[u32Index].u32Width = u32Width;
        pstStreamAttr[u32Index].u32Height = u32Height;
    }
    else if (pstStreamAttr[u32Index].enInput == ST_Sys_Input_DIVP)
    {
        STCHECKRESULT(ST_StopPipeLineWithDip((MI_U8)u32Index));
        STCHECKRESULT(ST_StartPipeLineWithDip((MI_U8)u32Index, u32Width, u32Height, pstStreamAttr[u32Index].u32CropWidth, pstStreamAttr[u32Index].u32CropHeight, pstStreamAttr[u32Index].u32CropX, pstStreamAttr[u32Index].u32CropY));

        pstStreamAttr[u32Index].u32Width = u32Width;
        pstStreamAttr[u32Index].u32Height = u32Height;
    }

    return MI_SUCCESS;
}

void ST_DoCaptureIMI1080PJPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProcExt(g_u32CapWidth, g_u32CapHeight, enRotation);
}

void ST_DoCapture4MJPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProc(2560, 1440, enRotation);
}

void ST_DoCapture1080PJPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProc(1920, 1080, enRotation);
}

void ST_DoCapture720PJPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProc(1280, 720, enRotation);
}

void ST_DoCaptureD1JPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProc(720, 576, enRotation);
}

void ST_DoCaptureCIFJPGProc(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    (void)ST_DoCaptureJPGProc(352, 288, enRotation);
}
void ST_DoVpePort0ResizeTo1080P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, 1920, ALIGN_UP(1080, 16));
    }
    else
    {
        (void)ST_DoStreamResize(0, 1920, 1080);
    }
}
void ST_DoVpePort0ResizeTo720P(void *args)
{
    (void)ST_DoStreamResize(1, 1280, 720);
}

void ST_DoVpePort1ResizeTo4M(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_DOWN(1440, 16), 2304);
    }
    else
    {
        (void)ST_DoStreamResize(1, 2560, 1440);
    }
}

void ST_DoVpePort1ResizeTo3M(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_DOWN(1536, 16), 2304);
    }
    else
    {
        (void)ST_DoStreamResize(1, 2304, 1536);
    }
}

void ST_DoVpePort1ResizeTo1080P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_DOWN(1080, 16), 1920);
    }
    else
    {
        (void)ST_DoStreamResize(1, 1920, 1080);
    }
}
void ST_DoVpePort1ResizeTo720P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_UP(720, 16), 1280);
    }
    else
    {
        (void)ST_DoStreamResize(1, 1280, 720);
    }
}
void ST_DoVpePort1ResizeToD1(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_UP(576, 16), 720);
    }
    else
    {
        (void)ST_DoStreamResize(1, 720, 576);
    }
}

void ST_DoVpePort1ResizeToCIF(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, ALIGN_UP(288, 16), 352);
    }
    else
    {
        (void)ST_DoStreamResize(1, 352, 288);
    }
}

void ST_DoVpePort2ResizeToD1(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(2, ALIGN_UP(576, 32), 720);
    }
    else
    {
        (void)ST_DoStreamResize(2, 720, 576);
    }
}

void ST_DoVpePort2ResizeToCIF(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(2, ALIGN_UP(288, 16), 352);
    }
    else
    {
        (void)ST_DoStreamResize(2, 352, 288);
    }
}

void ST_DoExitProc(void *args)
{
    g_bExit = TRUE;
}

void ST_RegisterInputCmd(const char *szDesc, ST_UserCmdProcess process, void *pArgs)
{
    ST_Input_Cmd_S *pstInputCmd = NULL;

    pstInputCmd = (ST_Input_Cmd_S *)malloc(sizeof(ST_Input_Cmd_S));
    if (pstInputCmd == NULL)
    {
        ST_ERR("malloc error\n");
        return;
    }

    memset(pstInputCmd, 0, sizeof(ST_Input_Cmd_S));
    snprintf(pstInputCmd->szDesc, sizeof(pstInputCmd->szDesc) - 1, "%s", szDesc);
    pstInputCmd->process = process;
    pstInputCmd->args = pArgs;
    pstInputCmd->cmdID = g_inputCmdMng.count;
    INIT_LIST_HEAD(&pstInputCmd->list);

    list_add_tail(&pstInputCmd->list, &g_inputCmdMng.head);

    g_inputCmdMng.count ++;
}

void ST_InitInputCmd(MI_SYS_Rotate_e enRotation)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    char szDesc[256];

    memset(szDesc, 0, sizeof(szDesc));
    snprintf(szDesc, sizeof(szDesc) - 1,  "capture %dx%d JPEG(IMI mode)", g_u32CapWidth, g_u32CapHeight);
    ST_RegisterInputCmd(szDesc, ST_DoCaptureIMI1080PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 2560x1440 JPEG", ST_DoCapture4MJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 1920x1080 JPEG", ST_DoCapture1080PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 1280X720 JPEG", ST_DoCapture720PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 720x576 JPEG", ST_DoCaptureD1JPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 352x288 JPEG", ST_DoCaptureCIFJPGProc, (void *)enRotation);

    //ST_RegisterInputCmd("resize main_stream to 1920x1080", ST_DoMainStreamResizeTo1080P, NULL);
    //ST_RegisterInputCmd("resize main_stream to 1280x720", ST_DoMainStreamResizeTo720P, NULL);
    //vpe port0 not support scaling.
    if (pstStreamAttr[1].bEnable == TRUE)
    {
    	if (g_u32CapWidth == 2560 || g_u32CapHeight == 1440)
		{
            ST_RegisterInputCmd("resize vpe port1 main_stream to 2560x1440", ST_DoVpePort1ResizeTo4M, (void *)enRotation);
            ST_RegisterInputCmd("resize vpe port1 main_stream to 2304x1536", ST_DoVpePort1ResizeTo3M, (void *)enRotation);
		}
        else if (g_u32CapWidth == 2304 || g_u32CapHeight == 1536)
        {
            ST_RegisterInputCmd("resize vpe port1 main_stream to 2304x1536", ST_DoVpePort1ResizeTo3M, (void *)enRotation);
        }
        ST_RegisterInputCmd("resize vpe port1 main_stream to 1920x1080", ST_DoVpePort1ResizeTo1080P, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 1280x720", ST_DoVpePort1ResizeTo720P, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 720x576", ST_DoVpePort1ResizeToD1, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 352x288", ST_DoVpePort1ResizeToCIF, (void *)enRotation);
    }
    if (pstStreamAttr[2].enInput == TRUE)
    {
        ST_RegisterInputCmd("resize vpe port2 sub_stream1 to 720x576", ST_DoVpePort2ResizeToD1, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port2 sub_stream1 to 352x288", ST_DoVpePort2ResizeToCIF, (void *)enRotation);
    }
    // at the end
    ST_RegisterInputCmd("exit the program", ST_DoExitProc, NULL);
}

void ST_DestoryInputCmd(void)
{
    struct list_head *pHead = NULL;
    struct list_head *pListPos = NULL;
    struct list_head *pListPosN = NULL;
    ST_Input_Cmd_S *pstInputCmd = NULL;

    pHead = &g_inputCmdMng.head;
    list_for_each_safe(pListPos, pListPosN, pHead)
    {
        pstInputCmd = list_entry(pListPos, ST_Input_Cmd_S, list);

        list_del(pListPos);
        free(pstInputCmd);
    }
}

void ST_ShowHelpMsg(void)
{
    struct list_head *pHead = NULL;
    struct list_head *pListPos = NULL;
    struct list_head *pListPosN = NULL;
    ST_Input_Cmd_S *pstInputCmd = NULL;

    pHead = &g_inputCmdMng.head;

    printf("############################################\n");
    printf("select cmd to process\n");
    list_for_each_safe(pListPos, pListPosN, pHead)
    {
        pstInputCmd = list_entry(pListPos, ST_Input_Cmd_S, list);
        printf("  %d: %s\n", pstInputCmd->cmdID, pstInputCmd->szDesc);
    }
}

void ST_DealInputCmd(void)
{
    int input = 0;
    struct list_head *pHead = NULL;
    struct list_head *pListPos = NULL;
    struct list_head *pListPosN = NULL;
    ST_Input_Cmd_S *pstInputCmd = NULL;

    ST_ShowHelpMsg();
    scanf("%d", &input);
    ST_Flush();
    printf("current input cmd:%d\n", input);

    pHead = &g_inputCmdMng.head;

    list_for_each_safe(pListPos, pListPosN, pHead)
    {
        pstInputCmd = list_entry(pListPos, ST_Input_Cmd_S, list);
        if (pstInputCmd->cmdID == input)
        {
            pstInputCmd->process(pstInputCmd->args);
            break;
        }
    }
}

void ST_AutoTest(void)
{
    MI_S32 s32CaseArray[1] = {0};
    MI_S32 s32RandNumber = 0;
    MI_S32 s32LastRandNumber = 0;

    struct list_head *pHead = NULL;
    struct list_head *pListPos = NULL;
    struct list_head *pListPosN = NULL;
    ST_Input_Cmd_S *pstInputCmd = NULL;

    pHead = &g_inputCmdMng.head;

    while (1)
    {
        s32RandNumber = rand() % ARRAY_SIZE(s32CaseArray);
        // if (s32LastRandNumber != s32RandNumber)
        {
            sleep(10);
            printf("Run test case %d...\n", s32CaseArray[s32RandNumber]);

            list_for_each_safe(pListPos, pListPosN, pHead)
            {
                pstInputCmd = list_entry(pListPos, ST_Input_Cmd_S, list);
                if (pstInputCmd->cmdID == s32CaseArray[s32RandNumber])
                {
                    pstInputCmd->process(pstInputCmd->args);
                }
            }

            s32LastRandNumber = s32RandNumber;
            UNUSED(s32LastRandNumber);
        }
    }
}

int main(int argc, char **argv)
{
    struct sigaction sigAction;
    sigAction.sa_handler = ST_HandleSig;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);

#if 0
    ST_ResetArgs(&g_stConfig);
#else
    ST_DefaultArgs(&g_stConfig);
#endif

    STCHECKRESULT(ST_BaseModuleInit(&g_stConfig));
    STCHECKRESULT(ST_VencStart());
    ST_RtspServerStart();

    ST_InitInputCmd(g_stConfig.enRotation);

    if(g_stConfig.s32UseVdf != 0)
    {
        ST_ModuleInit_VDF();
        ST_VdfStart();
    }

    if(g_stConfig.s32UseOnvif != 0)
    {
        MST_ONVIF_Init();
        MST_ONVIF_StartTask();
    }

#if (SHOW_PANEL == 1)
    if(g_stConfig.s32UsePanel != 0)
    {
        ST_PanelDispStart(g_stConfig.enPixelFormat, g_stConfig.s32Rotate);
    }
#endif

#if USE_AUDIO
    if(g_stConfig.s32UseAudio != 0)
    {
        ST_AOInit();
    }
#endif

    if(g_stConfig.s32LoadIQ != 0)
    {
        // wait vpe has stream
        usleep(1000 * 1000);
        MI_ISP_API_CmdLoadBinFile(0, (char *)IQ_FILE_PATH,  1234);
    }

#if 1
    while(!g_bExit)
    {
        ST_DealInputCmd();
        usleep(100 * 1000);
    }
#else
    ST_AutoTest();
#endif

    usleep(100 * 1000);

#if USE_AUDIO
    if(g_stConfig.s32UseAudio != 0)
    {
        ST_AOExit();
    }
#endif

    if(g_stConfig.s32UseOnvif != 0)
    {
        MST_ONVIF_StopTask();
    }

#if SHOW_PANEL
    if(g_stConfig.s32UsePanel != 0)
    {
        ST_PanelDispStop();
    }
#endif

    ST_RtspServerStop();
    STCHECKRESULT(ST_VencStop());

    STCHECKRESULT(ST_BaseModuleUnInit());

    ST_DestoryInputCmd();

    return 0;
}


