/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
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

#include <vector>
#include <string>
#include <map>
#include "tem.h"

#include "st_common.h"
#include "st_vif.h"
#include "st_vpe.h"
#include "st_venc.h"
#include "st_rgn.h"
//#include "st_ao.h"


#include "dot_matrix_font.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "Live555RTSPServer.hh"

#include "mi_rgn.h"
#include "mi_divp.h"
#include "mi_ai.h"

//#include "mi_od.h"
//#include "mi_md.h"

//#include "mi_vdf.h"
//#include "mi_ao.h"
#include "mi_isp.h"
#include "mi_iqserver.h"
#include "mi_venc.h"
#include "onvif_server.h"

#include "linux_list.h"

#include "mi_sensor_datatype.h"

void *g_pIspBuffer;

//#define ENABLE_RGN


#define RTSP_LISTEN_PORT        554
#define MAIN_STREAM                "main_stream"
#define SUB_STREAM0                "sub_stream0"
#define SUB_STREAM1                "sub_stream1"
#define SUB_STREAM2                "sub_stream2"
#define SUB_STREAM3                "sub_stream3"

#define PATH_PREFIX                "/customer"
#define DEBUG_ES_FILE            0

#define RGN_OSD_TIME_START        8
#define RGN_OSD_MAX_NUM         4
#define RGN_OSD_TIME_WIDTH        180
#define RGN_OSD_TIME_HEIGHT        32

#define DOT_FONT_FILE            "/customer/mi_demo/gb2312.hzk"

#define MAX_CAPTURE_NUM            4
#define CAPTURE_PATH            "/mnt/capture"

#define BUF_POOL_SIZE 4096 * 1024 // 4mb

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

#define RGB_TO_CRYCB(r, g, b)                                                            \
        (((unsigned int)(( 0.439f * (r) - 0.368f * (g) - 0.071f * (b)) + 128.0f)) << 16) |    \
        (((unsigned int)(( 0.257f * (r) + 0.564f * (g) + 0.098f * (b)) + 16.0f)) << 8) |        \
        (((unsigned int)((-0.148f * (r) - 0.291f * (g) + 0.439f * (b)) + 128.0f)))

#define IQ_FILE_PATH    "./test.bin"

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

#define RGN_OSD_HANDLE                  0
#define VPE_PORT1_OSD_HANDLE            100
#define VPE_PORT1_COVER_HANDLE          101
#define VPE_PORT1_COVER_1_HANDLE          102

#define RGN_FOR_VDF_BEGIN               12

#define MAX_FULL_RGN_NULL               3
#define ENABLE_BUF_POOL                1

#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#else
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif
#define MI_FAIL     1

struct ST_Stream_Attr_T
{
    MI_BOOL        bEnable;
    ST_Sys_Input_E enInput;
    MI_U32     u32InputChn;
    MI_U32     u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_U32     u32Mbps;
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
#if 0
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
#endif
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

typedef struct
{
    char fileName[256];
    MI_RGN_PixelFormat_e ePixelFmt;
    MI_U16 u16RgnWidth;
    MI_U16 u16RgnHeight;
}ST_TestFileInfo_t;


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
#endif
static Live555RTSPServer *g_pRTSPServer = NULL;
static ST_RGN_Osd_T g_stRgnOsd;
static MI_BOOL g_bExit = FALSE;
//static MI_AUDIO_Frame_t _gstAoTestFrame;
//static _AoOutChn_t _gstAoOutchn;
//static ST_AoInInfo_T g_stAoInInfo;
MI_BOOL bEnableRes = FALSE;
//MI_BOOL bEnableVqe = FALSE;
//static _WavHeader_t stWavHeaderInput;
//static _WavHeader_t stWavHeaderOutput;
MI_U32 g_u32CapWidth = 0;
MI_U32 g_u32CapHeight = 0;
ST_Config_S g_stConfig;

MI_BOOL bCh0NeedPipeLine = FALSE;


static struct ST_Stream_Attr_T g_stStreamAttr[] =
{
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_VPE,
        .u32InputChn = 0,
        .u32InputPort = 1,
        .vencChn = 0,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .u32Mbps = 2,
        .u32Width = 1920,
        .u32Height = 1080,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = MAIN_STREAM,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,//E_MI_SYS_BIND_TYPE_HW_RING,//
        .u32BindPara = 0,
        .u32Cover1Handle = 3,
        .u32Cover2Handle = 4,
    },

    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_DIVP,
        .u32InputChn = 0,
        .u32InputPort = 2,
        .vencChn = 1,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .u32Mbps = 2,
        .u32Width = 640,
        .u32Height = 360,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = SUB_STREAM0,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,
        .u32BindPara = 0,
        .u32Cover1Handle = 5,
        .u32Cover2Handle = 6,
    },
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_DIVP,
        .u32InputChn = 0,
        .u32InputPort = 2,
        .vencChn = 2,
        .eType = E_MI_VENC_MODTYPE_H264E,
        .u32Mbps = 2,
        .u32Width = 384,
        .u32Height = 216,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = SUB_STREAM1,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,
        .u32BindPara = 0,
        .u32Cover1Handle = 7,
        .u32Cover2Handle = 8,
    },
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_DIVP,
        .u32InputChn = 0,
        .u32InputPort = 2,
        .vencChn = 3,
        .eType = E_MI_VENC_MODTYPE_JPEGE,
        .u32Mbps = 2,
        .u32Width = 384,
        .u32Height = 216,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_RTSP,
        .pszStreamName = SUB_STREAM2,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,
        .u32BindPara = 0,
        .u32Cover1Handle = 9,
        .u32Cover2Handle = 10,
    },
    {
        .bEnable = TRUE,
        .enInput = ST_Sys_Input_DIVP,
        .u32InputChn = 0,
        .u32InputPort = 2,
        .vencChn = 4,
        .eType = E_MI_VENC_MODTYPE_MAX,//for YUV
        .u32Mbps = 2,
        .u32Width = 384,
        .u32Height = 216,
        .u32CropX = 0,
        .u32CropY = 0,
        .u32CropWidth = 0,
        .u32CropHeight = 0,
        .enFunc = ST_Sys_Func_BUTT,
        .pszStreamName = SUB_STREAM3,
        .eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE,
        .u32BindPara = 0,
        .u32Cover1Handle = 11,
        .u32Cover2Handle = 12,
    },
};


static ST_TestFileInfo_t g_stArgb1555FileInfo[] = { {"testFile/1555/64X48.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 64, 48},   {"testFile/1555/80X80.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 80, 80},
                                                    {"testFile/1555/128X96.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 128, 96},  {"testFile/1555/200X200.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 200, 200},
                                                    {"testFile/1555/200X301.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 200, 301}, {"testFile/1555/300X360.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 300, 360},
                                                    {"testFile/1555/304X200.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 304, 200}, {"testFile/1555/400X266.argb1555", E_MI_RGN_PIXEL_FORMAT_ARGB1555, 400, 266} };

#if 0
static ST_TestFileInfo_t g_stI4FileInfo[] =       { {"testFile/I4/64X48.i4", E_MI_RGN_PIXEL_FORMAT_I4, 64, 48}, /*{"testFile/I4/216X100.i4", E_MI_RGN_PIXEL_FORMAT_I4, 216, 100},*/ {"testFile/I4/80X80.i4", E_MI_RGN_PIXEL_FORMAT_I4, 80, 80},
                                                    {"testFile/I4/200X200.i4", E_MI_RGN_PIXEL_FORMAT_I4, 200, 200},  {"testFile/I4/304X200.i4", E_MI_RGN_PIXEL_FORMAT_I4, 304, 200} };

static ST_TestFileInfo_t g_stArgb4444FileInfo[] = { {"testFile/4444/64X48.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 64, 48},   {"testFile/4444/80X80.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 80, 80},
                                                    {"testFile/4444/128X96.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 128, 96},  {"testFile/4444/200X200.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 200, 200},
                                                    {"testFile/4444/200X301.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 200, 301}, {"testFile/4444/300X360.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 300, 360},
                                                    {"testFile/4444/304X200.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 304, 200}, {"testFile/4444/400X266.argb4444", E_MI_RGN_PIXEL_FORMAT_ARGB4444, 400, 266} };

static ST_TestFileInfo_t g_stI2FileInfo[] =       { {"testFile/I2/64X48.i2", E_MI_RGN_PIXEL_FORMAT_I2, 64, 48},   {"testFile/I2/80X80.i2", E_MI_RGN_PIXEL_FORMAT_I2, 80, 80},
                                                    {"testFile/I2/200X200.i2", E_MI_RGN_PIXEL_FORMAT_I2, 200, 200},  {"testFile/I2/304X200.i2", E_MI_RGN_PIXEL_FORMAT_I2, 304, 200} };

static ST_TestFileInfo_t g_stI8FileInfo[] =       { {"testFile/I8/64X48.i8", E_MI_RGN_PIXEL_FORMAT_I8, 64, 48},   {"testFile/I8/80X80.i8", E_MI_RGN_PIXEL_FORMAT_I8, 80, 80},
                                                    {"testFile/I8/200X200.i8", E_MI_RGN_PIXEL_FORMAT_I8, 200, 200},  {"testFile/I8/304X200.i8", E_MI_RGN_PIXEL_FORMAT_I8, 304, 200} };

static ST_TestFileInfo_t g_stRgb565FileInfo[] =   { {"testFile/565/64X48.rgb565",E_MI_RGN_PIXEL_FORMAT_RGB565, 64, 48},   {"testFile/565/80X80.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 80, 80},
                                                    {"testFile/565/128X96.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 128, 96},  {"testFile/565/200X200.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 200, 200},
                                                    {"testFile/565/200X301.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 200, 301}, {"testFile/565/300X360.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 300, 360},
                                                    {"testFile/565/304X200.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 304, 200}, {"testFile/565/400X266.rgb565", E_MI_RGN_PIXEL_FORMAT_RGB565, 400, 266} };

static ST_TestFileInfo_t g_stArgb8888FileInfo[] = { {"testFile/8888/64X48.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 64, 48},   {"testFile/8888/80X80.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 80, 80},
                                                    {"testFile/8888/128X96.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 128, 96},  {"testFile/8888/200X200.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 200, 200},
                                                    {"testFile/8888/200X301.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 200, 301}, {"testFile/8888/300X360.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 300, 360},
                                                    {"testFile/8888/304X200.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 304, 200}, {"testFile/8888/400X266.argb8888", E_MI_RGN_PIXEL_FORMAT_ARGB8888, 400, 266} };
#endif

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

int first_frame =1;
MI_S32 count = 0;

typedef struct
{
	pthread_t pthread;
	int thread_exit;
	int exit;
	const char* threadname;
} ST_pthreadHandel_T;

ST_pthreadHandel_T pthreadHandlearray[5];

void *ST_DIVPGetResult(void *args)
{
    ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_S32 s32Ret = MI_SUCCESS;
    char szFileName[128];
    int fd = 0;
    MI_U32 u32GetFramesCount = 0;
    MI_BOOL _bWriteFile = TRUE;

    for(i = 0; i < u32ArraySize; i ++)
    {
         if(!strncmp(ppthreadHandel->threadname, pstStreamAttr[i].pszStreamName,strlen(pstStreamAttr[i].pszStreamName)))
         {
             break;
         }
    }

    if( (i >= u32ArraySize) || ( pstStreamAttr[i].eType != E_MI_VENC_MODTYPE_MAX ) )
    {
         ST_ERR("not found this stream, \"%s\"\n", ppthreadHandel->threadname);
         ppthreadHandel->thread_exit=1;
         return NULL;
    }

    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = i;
    stChnPort.u32PortId = 0;

    sprintf(szFileName, "divp_chn_%d.nv12", stChnPort.u32ChnId);
    printf(COLOR_GREEN"[%s]start to record %s\n"COLOR_NONE,__func__, szFileName);
    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        ST_ERR("create %s fail\n", szFileName);
    }

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 4);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth err:%x, chn:%d,port:%d\n", s32Ret,
            stChnPort.u32ChnId, stChnPort.u32PortId);
        return NULL;
    }

    while (ppthreadHandel->exit ==0)
    {
        usleep(100 * 1000);
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);

        if(MI_SUCCESS != s32Ret)
        {
            //printf("MI_SYS_ChnOutputPortGetBuf err, %x\n", s32Ret);
            continue;
        }


#if 1
        ((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts = stBufInfo.u64Pts;

        if(MI_ISP_OK == MI_ISP_GetFrameMetaInfo(pstStreamAttr[i].u32InputChn, (IspFrameMetaInfo_t*)g_pIspBuffer))
            printf(COLOR_YELLOW"[%s]u64Pts=%llu,  %lf(s), u32Shutter = %u, u32SensorGain = %u, u32ColorTmp = %u\n"COLOR_NONE,__func__,
            ((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts,  (double)(((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts)/(1000*1000), ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32Shutter, ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32SensorGain, ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32ColorTmp);
        else
            printf("[%s]get frame meta info failed\n",__func__);
#endif


#if 1
        // save one Frame YUV data
        if (fd > 0)
        {
            if(_bWriteFile)
            {
              write(fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0] +
                                stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
            }

        }
#endif

#if 1
        ++u32GetFramesCount;
        printf(COLOR_GREEN"[%s]channelId[%u] u32GetFramesCount[%u] [%dx%d] \n"COLOR_NONE,__func__, stChnPort.u32ChnId, u32GetFramesCount, stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u16Height );
#endif
        MI_SYS_ChnOutputPortPutBuf(stBufHandle);

        ppthreadHandel->exit =1; // save one Frame YUV data


    }

    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }

    printf(COLOR_GREEN"[%s]exit record %s\n"COLOR_NONE,__func__,szFileName);
    ppthreadHandel->thread_exit=1;
    return NULL;
}

void *ST_DIVPPreoloadGetResult(void *args)
{
    ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_S32 s32Ret = MI_SUCCESS;
    char szFileName[128];
    int fd = 0;
    MI_U32 u32GetFramesCount = 0;
    MI_BOOL _bWriteFile = TRUE;

    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;

    sprintf(szFileName, "divp_chn_%d.nv12", stChnPort.u32ChnId);
    printf(COLOR_GREEN"[%s]start to record %s\n"COLOR_NONE,__func__, szFileName);
    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        ST_ERR("create %s fail\n", szFileName);
    }


    while (ppthreadHandel->exit ==0)
    {
        usleep(1000);
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);

        if(MI_SUCCESS != s32Ret)
        {
            //printf("MI_SYS_ChnOutputPortGetBuf err, %x\n", s32Ret);
            continue;
        }


#if 0
        ((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts = stBufInfo.u64Pts;

        if(MI_ISP_OK == MI_ISP_GetFrameMetaInfo(0, (IspFrameMetaInfo_t*)g_pIspBuffer))
            printf(COLOR_YELLOW"[%s]u64Pts=%llu,  %lf(s), u32Shutter = %u, u32SensorGain = %u, u32ColorTmp = %u\n"COLOR_NONE,__func__,
            ((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts,  (double)(((IspFrameMetaInfo_t*)g_pIspBuffer)->u64Pts)/(1000*1000), ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32Shutter, ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32SensorGain, ((IspFrameMetaInfo_t*)g_pIspBuffer)->u32ColorTmp);
        else
            printf("[%s]get frame meta info failed\n",__func__);
#endif


#if 1
        // save one Frame YUV data
        if (fd > 0)
        {
            if(_bWriteFile)
            {
                write(fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0] +
                                stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
                _bWriteFile = FALSE;
            }

        }
        ++u32GetFramesCount;
        printf(COLOR_GREEN"[%s]channelId[%u] u32GetFramesCount[%u] [%dx%d] u64Pts=%llu\n"COLOR_NONE,__func__, stChnPort.u32ChnId, u32GetFramesCount, stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u16Height,stBufInfo.u64Pts);
#endif
        MI_SYS_ChnOutputPortPutBuf(stBufHandle);
        if(25 == u32GetFramesCount)
           ppthreadHandel->exit =1; // save one Frame YUV data


    }

    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }

    MI_SYS_SetChnOutputPortDepth(&stChnPort , 2 ,4);

    printf(COLOR_GREEN"[%s]exit record %s\n"COLOR_NONE,__func__,szFileName);
    ppthreadHandel->thread_exit=1;
    return NULL;
}

void *ST_AIPreoloadGetResult(void *args)
{
    ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32GetFramesCount = 0;

    stChnPort.eModId = E_MI_MODULE_ID_AI;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;


    while (ppthreadHandel->exit ==0)
    {
        usleep(1000);
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &stBufHandle);

        if(MI_SUCCESS != s32Ret)
        {
            //printf("MI_SYS_ChnOutputPortGetBuf err, %x\n", s32Ret);
            continue;
        }
        u32GetFramesCount ++;
        MI_SYS_ChnOutputPortPutBuf(stBufHandle);
        if(200 == u32GetFramesCount)
            ppthreadHandel->exit =1; // save one Frame YUV data
    }

    MI_SYS_SetChnOutputPortDepth(&stChnPort , 2 ,4);
    printf(COLOR_GREEN"[%s]exit record\n"COLOR_NONE,__func__);

    ppthreadHandel->thread_exit=1;
    return NULL;
}

void *ST_VencPreoloadGetResult(void *args)
{
    ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32GetFramesCount = 0;
    MI_SYS_ChnPort_t stChnPort;

    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;

    memset(&stStream, 0, sizeof(MI_VENC_Stream_t));
    memset(&stPack, 0, sizeof(MI_VENC_Pack_t));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;


    while (ppthreadHandel->exit ==0)
    {
        usleep(1000);
        s32Ret = MI_VENC_GetStream(0, &stStream, 40);
        if(MI_SUCCESS == s32Ret)
        {
            s32Ret = MI_VENC_ReleaseStream(0, &stStream);
            if(MI_SUCCESS != s32Ret)
            {
                ST_DBG("MI_VENC_ReleaseStream fail, ret:0x%x\n", s32Ret);
            }

            u32GetFramesCount ++;
            if(u32GetFramesCount == 150)
                ppthreadHandel->exit =1; // save one Frame YUV data
         }
        else
            continue;
    }

    stChnPort.eModId = E_MI_MODULE_ID_VENC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort , 2 ,4);

    printf(COLOR_GREEN"[%s]exit record\n"COLOR_NONE,__func__);

    ppthreadHandel->thread_exit=1;
    return NULL;
}



int ST_Thread_Start(void *args, const char* threadname, void*(*CB)(void *) )
{
	ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;
	memset(ppthreadHandel, 0, sizeof(ST_pthreadHandel_T));
	ppthreadHandel->threadname=threadname;
	prctl(PR_SET_NAME, threadname);
	pthread_create(&ppthreadHandel->pthread, NULL, CB, args);
    return 0;
}

int ST_Thread_Stop(void *args, const char* threadname)
{
	ST_pthreadHandel_T* ppthreadHandel=(ST_pthreadHandel_T*)args;

	if(!ppthreadHandel->pthread)
	{
		printf("[%s] nothing exit\n",__FUNCTION__);
		return 0;
	}

    ppthreadHandel->exit= 1;

    while(ppthreadHandel->thread_exit == 0)
    {
        printf("[%s] wait for %s exit\n",__FUNCTION__,ppthreadHandel->threadname);
        usleep(200*1000);
    }

	printf("[%s] thread join+\n", __FUNCTION__);
	pthread_join(ppthreadHandel->pthread, NULL);
	printf("[%s] thread join-\n", __FUNCTION__);

	memset(ppthreadHandel, 0, sizeof(ST_pthreadHandel_T));
	return 0;
}


#if (ENABLE_BUF_POOL == 1)
typedef struct
{
    void *pDataAddr;
    void *pDataAddrStart;
    void *pDataAddrAlign;
    MI_U32 u32DataLen;
    MI_BOOL bExit;
}ST_DataPackage;
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
}ST_DataMutexCond;
typedef struct
{
    int totalsize;
    std::vector<ST_DataPackage> package;
}ST_DataPackageHead;
static std::map<MI_U32, ST_DataPackageHead> mapVencPackage;
static ST_DataMutexCond gstDataMuxCond;

static void *_ST_VencGetData(ST_TEM_BUFFER stTemBuf)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    MI_SYS_ChnPort_t *pstChnPort;
    ST_DataPackage stDataPackage;
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;
    MI_U32 u32Len = 0;

    ASSERT(sizeof(MI_SYS_ChnPort_t) == stTemBuf.u32TemBufferSize);
    pstChnPort = (MI_SYS_ChnPort_t *)stTemBuf.pTemBuffer;
    MI_S32 s32Fd;
    struct timeval tv;
    fd_set read_fds;

    s32Fd = MI_VENC_GetFd((MI_VENC_CHN)pstChnPort->u32ChnId);
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
        memset(&stStream, 0, sizeof(stStream));
        memset(stPack, 0, sizeof(stPack));
        stStream.pstPack = stPack;
        s32Ret = MI_VENC_Query((MI_VENC_CHN)pstChnPort->u32ChnId, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            return NULL;
        }
        stStream.u32PackCount = stStat.u32CurPacks;;
        s32Ret = MI_VENC_GetStream((MI_VENC_CHN)pstChnPort->u32ChnId, &stStream, 40);
        if(MI_SUCCESS == s32Ret)
        {
            ASSERT(stStream.pstPack[0].u32Len);
            memset(&stDataPackage, 0, sizeof(ST_DataPackage));
            for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
            {
                ASSERT(stStream.pstPack[i].u32Len);
                stDataPackage.u32DataLen += stStream.pstPack[i].u32Len;
            }
            stDataPackage.pDataAddr = stDataPackage.pDataAddrStart = stDataPackage.pDataAddrAlign = (void *)malloc(stDataPackage.u32DataLen);
            ASSERT(stDataPackage.pDataAddr);
            for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
            {
                memcpy((char *)stDataPackage.pDataAddrStart + u32Len, stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
                u32Len += stStream.pstPack[i].u32Len;
            }
            //printf("Get stream size %d addr %p\n", stStream.pstPack[0].u32Len, stDataPackage.pDataAddr);
            pthread_mutex_lock(&gstDataMuxCond.mutex);
            iter = mapVencPackage.find(pstChnPort->u32ChnId);
            if(iter == mapVencPackage.end())
            {
                printf("Error!!!! venc chn %d not open!!!\n", pstChnPort->u32ChnId);
                free(stDataPackage.pDataAddr);
                stDataPackage.pDataAddr = stDataPackage.pDataAddrStart = stDataPackage.pDataAddrAlign = NULL;
                pthread_mutex_unlock(&gstDataMuxCond.mutex);

                goto RELEASE_STREAM;
            }
            if (iter->second.totalsize > BUF_POOL_SIZE)
            {
                printf("Error!!!! buf pool full!!!!!\n");
                free(stDataPackage.pDataAddr);
                stDataPackage.pDataAddr = stDataPackage.pDataAddrStart = stDataPackage.pDataAddrAlign = NULL;
                pthread_mutex_unlock(&gstDataMuxCond.mutex);

                goto RELEASE_STREAM;
            }
            iter->second.package.push_back(stDataPackage);
            iter->second.totalsize += stDataPackage.u32DataLen;
            PTH_RET_CHK(pthread_cond_signal(&gstDataMuxCond.cond));
            pthread_mutex_unlock(&gstDataMuxCond.mutex);

RELEASE_STREAM:
            MI_VENC_ReleaseStream((MI_VENC_CHN)pstChnPort->u32ChnId, &stStream);
        }
    }
RET:
    MI_VENC_CloseFd(s32Fd);

    return NULL;
}
MI_S32 ST_TermBufPool(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    ST_Sys_Input_E enInput = ST_Sys_Input_BUTT;
    ST_DataPackage stDataPackage;
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;

    for(i = 0; i < u32ArraySize; i ++)
    {
        enInput = pstStreamAttr[i].enInput;
        if(enInput == ST_Sys_Input_VPE)
        {
            pthread_mutex_lock(&gstDataMuxCond.mutex);
            iter = mapVencPackage.find(pstStreamAttr[i].vencChn);
            if(iter != mapVencPackage.end())
            {
                memset(&stDataPackage, 0, sizeof(ST_DataPackage));
                stDataPackage.bExit = TRUE;
                iter->second.package.push_back(stDataPackage);
                PTH_RET_CHK(pthread_cond_signal(&gstDataMuxCond.cond));
            }
            pthread_mutex_unlock(&gstDataMuxCond.mutex);
        }
    }

    return MI_SUCCESS;

    return 0;
}

MI_S32 ST_InitBufPoolEnv(void)
{
    PTH_RET_CHK(pthread_mutex_init(&gstDataMuxCond.mutex, NULL));
    PTH_RET_CHK(pthread_condattr_init(&gstDataMuxCond.condattr));
    PTH_RET_CHK(pthread_condattr_setclock(&gstDataMuxCond.condattr, CLOCK_MONOTONIC));
    PTH_RET_CHK(pthread_cond_init(&gstDataMuxCond.cond, &gstDataMuxCond.condattr));

    return MI_SUCCESS;
}
MI_S32 ST_DeinitBufPoolEnv(void)
{
    PTH_RET_CHK(pthread_condattr_destroy(&gstDataMuxCond.condattr));
    PTH_RET_CHK(pthread_cond_destroy(&gstDataMuxCond.cond));
    PTH_RET_CHK(pthread_mutex_destroy(&gstDataMuxCond.mutex));

    return MI_SUCCESS;
}

MI_BOOL ST_BufPoolEmptyAndWait(void)
{
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_U32 i = 0;
    ST_Sys_Input_E enInput = ST_Sys_Input_BUTT;
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;

    pthread_mutex_lock(&gstDataMuxCond.mutex);
    for(i = 0; i < u32ArraySize; i ++)
    {
        iter = mapVencPackage.find(pstStreamAttr[i].vencChn);
        if(iter != mapVencPackage.end())
        {
            if (iter->second.package.size())
            {
                pthread_mutex_unlock(&gstDataMuxCond.mutex);

                return FALSE;
            }
        }
    }
    pthread_cond_wait(&gstDataMuxCond.cond, &gstDataMuxCond.mutex);
    pthread_mutex_unlock(&gstDataMuxCond.mutex);

    return TRUE;
}
MI_S32 ST_OpenBufPool(MI_VENC_CHN stVencChn)
{
    ST_TEM_ATTR stTemAttr;
    char vencGetDataTName[30];
    MI_SYS_ChnPort_t stVencChnPort;

    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));
    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    memset(&stVencChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stVencChnPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnPort.u32ChnId = stVencChn;
    stVencChnPort.u32PortId = 0;
    ExecFunc(MI_VENC_GetChnDevid(stVencChn, &stVencChnPort.u32DevId), MI_SUCCESS);
    stTemAttr.fpThreadDoSignal = NULL;
    stTemAttr.fpThreadWaitTimeOut = _ST_VencGetData;
    stTemAttr.u32ThreadTimeoutMs = 10;
    stTemAttr.bSignalResetTimer = 0;
    stTemAttr.stTemBuf.pTemBuffer = (void *)&stVencChnPort;
    stTemAttr.stTemBuf.u32TemBufferSize = sizeof(MI_SYS_ChnPort_t);
    memset(vencGetDataTName, 0, 30);
    sprintf(vencGetDataTName, "venc_getdata_chn%d", stVencChn);
    TemOpen(vencGetDataTName, stTemAttr);
    pthread_mutex_lock(&gstDataMuxCond.mutex);
    ASSERT(mapVencPackage[stVencChn].package.size() == 0);
    mapVencPackage[stVencChn].totalsize = 0;
    pthread_mutex_unlock(&gstDataMuxCond.mutex);

    return MI_SUCCESS;

}
MI_S32 ST_CloseBufPool(MI_VENC_CHN stVencChn)
{
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;
    std::vector<ST_DataPackage>::iterator it;
    char vencGetDataTName[30];

    memset(vencGetDataTName, 0, 30);
    sprintf(vencGetDataTName, "venc_getdata_chn%d", stVencChn);
    TemClose(vencGetDataTName);
    pthread_mutex_lock(&gstDataMuxCond.mutex);
    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        for (it = iter->second.package.begin(); it != iter->second.package.end(); ++it)
        {
            free(it->pDataAddr);
            iter->second.totalsize -= it->u32DataLen;
        }
        ASSERT(iter->second.totalsize == 0);
        iter->second.package.clear();
        mapVencPackage.erase(iter);
    }
    pthread_mutex_unlock(&gstDataMuxCond.mutex);

    return MI_SUCCESS;
}
MI_S32 ST_StartBufPool(MI_VENC_CHN stVencChn)
{
    char vencGetDataTName[30];

    memset(vencGetDataTName, 0, 30);
    sprintf(vencGetDataTName, "venc_getdata_chn%d", stVencChn);
    TemStartMonitor(vencGetDataTName);

    return MI_SUCCESS;
}
MI_S32 ST_StopBufPool(MI_VENC_CHN stVencChn)
{
    char vencGetDataTName[30];

    memset(vencGetDataTName, 0, 30);
    sprintf(vencGetDataTName, "venc_getdata_chn%d", stVencChn);
    TemStop(vencGetDataTName);

    return MI_SUCCESS;
}
MI_U32 ST_DequeueBufPool(MI_VENC_CHN stVencChn, void *pData, MI_U32 u32Size)
{
    MI_U32 size = 0;
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;
    std::vector<ST_DataPackage>::iterator it;

    ST_BufPoolEmptyAndWait();
    pthread_mutex_lock(&gstDataMuxCond.mutex);
    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        it = iter->second.package.begin();
        if (it != iter->second.package.end())
        {
            if (!it->bExit)
            {
                size = MIN(it->u32DataLen, u32Size);
                memcpy(pData, it->pDataAddrStart, size);
                //printf("size %d ucbuf %p paddr %p gap %d data_length %d Free %p total %d package cnt %d\n", size, ucpBuf, pAddr, u32Gap, it->u32DataLen, it->pDataAddr, iter->second.totalsize, iter->second.package.size());
                free(it->pDataAddr);
                iter->second.totalsize -= it->u32DataLen;
                it->u32DataLen = 0;
                iter->second.package.erase(it);
            }
        }
    }
    pthread_mutex_unlock(&gstDataMuxCond.mutex);

    return size;

}
MI_S32 ST_FlushBufPool(MI_VENC_CHN stVencChn)
{
    std::map<MI_U32, ST_DataPackageHead>::iterator iter;
    std::vector<ST_DataPackage>::iterator it;

    iter = mapVencPackage.find((MI_U32)stVencChn);
    if(iter != mapVencPackage.end())
    {
        pthread_mutex_lock(&gstDataMuxCond.mutex);
        for (it = iter->second.package.begin(); it != iter->second.package.end(); ++it)
        {
            if (it->bExit)
                continue;

            free(it->pDataAddr);
            iter->second.totalsize -= it->u32DataLen;
        }
        ASSERT(iter->second.totalsize == 0);
        iter->second.package.clear();
        pthread_mutex_unlock(&gstDataMuxCond.mutex);
    }

    return MI_SUCCESS;
}
#else
static MI_U32 _ST_GetDataDirect(MI_VENC_CHN stVencChn, void *pData, MI_U32 u32Maxlen)
{
    MI_U32 u32Size = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    MI_S32 s32Fd;
    MI_S32 s32Ret = MI_SUCCESS;
    struct timeval tv;
    fd_set read_fds;
    MI_U32 u32Len = 0;

    s32Fd = MI_VENC_GetFd((MI_VENC_CHN)stVencChn);
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
        memset(&stStream, 0, sizeof(stStream));
        memset(stPack, 0, sizeof(stPack));
        stStream.pstPack = stPack;
        s32Ret = MI_VENC_Query(stVencChn, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            return NULL;
        }
        stStream.u32PackCount = stStat.u32CurPacks;
        s32Ret = MI_VENC_GetStream(stVencChn, &stStream, 40);
        if(MI_SUCCESS == s32Ret)
        {
            u32Size = MIN(stStream.pstPack[0].u32Len, u32Maxlen);
            ASSERT(u32Size);
            for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
            {
                memcpy((char *)pData + u32Len, stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
                u32Len += stStream.pstPack[i].u32Len;
            }
            //printf("Get stream size %d addr %p\n", stStream.pstPack[0].u32Len, stDataPackage.pDataAddr);
            MI_VENC_ReleaseStream(stVencChn, &stStream);
        }
    }
 RET:
    MI_VENC_CloseFd(s32Fd);

    return u32Size;
}
#endif
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

    if(pstStreamInfo->enType == E_MI_VENC_MODTYPE_H264E)
       s32Ret = MI_VENC_RequestIdr(pstStreamInfo->vencChn, TRUE);

    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, pstStreamInfo->vencChn);

    if(MI_SUCCESS != s32Ret)
    {
        ST_WARN("request IDR fail, error:%x\n", s32Ret);
    }

#if (ENABLE_BUF_POOL == 1)
    ST_StartBufPool(pstStreamAttr[i].vencChn);
#endif
    return pstStreamInfo;
}
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;

#if (ENABLE_BUF_POOL == 1)
    return (int)ST_DequeueBufPool(pstStreamInfo->vencChn, (void *)ucpBuf, BufLen);
#else
    return (int)_ST_GetDataDirect(pstStreamInfo->vencChn, (void *)ucpBuf, BufLen);
#endif
}
int ST_CloseStream(void *handle, void *arg)
{
    if(handle == NULL)
    {
        return -1;
    }

    ST_StreamInfo_T *pstStreamInfo = (ST_StreamInfo_T *)handle;

    ST_DBG("close \"%s\" success\n", pstStreamInfo->szStreamName);

#if (ENABLE_BUF_POOL == 1)
    ST_StopBufPool(pstStreamInfo->vencChn);
    ST_FlushBufPool(pstStreamInfo->vencChn);
#endif
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
    MI_U8 u8CurResIdx;
    MI_SNR_Res_t stCurRes;

    pRTSPServer = new Live555RTSPServer();

    if(pRTSPServer == NULL)
    {
        ST_ERR("malloc error\n");
        return -1;
    }

	if (MI_SUCCESS != MI_SNR_GetCurRes(E_MI_SNR_PAD_ID_0, &u8CurResIdx, &stCurRes))
	{
		ST_ERR("MI_SNR_GetCurRes error\n");
		delete pRTSPServer;
  		pRTSPServer = NULL;
		return -1;
	}

    printf("Sensor max fps %d\n", stCurRes.u32MaxFps);

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
                                 ST_CloseStream, stCurRes.u32MaxFps);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H265E)
            {
                subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, stCurRes.u32MaxFps);
            }
            else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_JPEGE)
            {
                subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                                 *(pRTSPServer->GetUsageEnvironmentObj()),
                                 pstStreamAttr[i].pszStreamName,
                                 ST_OpenStream,
                                 ST_VideoReadStream,
                                 ST_CloseStream, stCurRes.u32MaxFps);
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
#ifdef ENABLE_RGN
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
#endif

    CanvasScopeLock ScopeLock;

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
        stChnAttr.stVeAttr.stAttrH264e.u32Profile = 1; //main profile
        //stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstStreamAttr[i].u32Mbps * 1024 * 1024;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 60;
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
        stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstStreamAttr[i].u32Mbps * 1024 * 1024;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 60;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
        stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
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

#ifdef ENABLE_RGN
    // attach osd
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;

    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = pstStreamAttr[i].u32Width - RGN_OSD_TIME_WIDTH - 10;
    stChnPortParam.stPoint.u32Y = 10;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xFF;
    ExecFunc(MI_RGN_AttachToChn(RGN_OSD_HANDLE, &stChnPort, &stChnPortParam), MI_RGN_OK);
    printf("attach osd %d to vpe(%d,%d)\n", RGN_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);

    // attach osd 100 to vpe port1
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 200;
    stChnPortParam.stPoint.u32Y = 400;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xFF;
    STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_OSD_HANDLE, &stChnPort, &stChnPortParam));
    printf("attach osd %d to vpe(%d,%d)\n", VPE_PORT1_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);

    // to deal with
    if (bCh0NeedPipeLine)
    {
        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
        stChnPortParam.bShow = TRUE;
        stChnPortParam.stPoint.u32X = 2000;
        stChnPortParam.stPoint.u32Y = 2000;
        stChnPortParam.unPara.stCoverChnPort.u32Layer = VPE_PORT1_COVER_HANDLE;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
        stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 0);
        STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_COVER_HANDLE, &stChnPort, &stChnPortParam));

        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
        stChnPortParam.bShow = TRUE;
        stChnPortParam.stPoint.u32X = 4000;
        stChnPortParam.stPoint.u32Y = 4000;
        stChnPortParam.unPara.stCoverChnPort.u32Layer = VPE_PORT1_COVER_1_HANDLE;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 800;
        stChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 800;
        stChnPortParam.unPara.stCoverChnPort.u32Color = RGB_TO_CRYCB(0, 0, 0);
        STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_COVER_1_HANDLE, &stChnPort, &stChnPortParam));
    }

    printf("ST_StartPipeLine[%d]: attach osd %d to vpe(%d,%d)\n", i, RGN_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);


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

//    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;

//    ST_Config_S *pstConfig = &g_stConfig;

    CanvasScopeLock ScopeLock;

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
    ExecFunc(MI_DIVP_CreateChn(i, &stDivpChnAttr), MI_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(i), MI_SUCCESS);
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = u32Width;
    stOutputPortAttr.u32Height          = u32Height;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(i, &stOutputPortAttr));

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_H264E)
    {
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
        stChnAttr.stVeAttr.stAttrH264e.u32Profile = 1; //main profile
        //stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
        //stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstStreamAttr[i].u32Mbps * 1024 * 1024;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 60;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
	    if(i == 2)
	    {
	        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 10;
	    }
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
        stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstStreamAttr[i].u32Mbps * 1024 * 1024;
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
    else if(pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_MAX)//for YUV
    {
        // only bind VPE to divp
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
        stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 10;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        printf("[YUV420] DIVP chn=%d\n", i);
        goto Exit;
    }
    stChnAttr.stVeAttr.eType = pstStreamAttr[i].eType;
    VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_StartChannel(VencChn));

#ifdef ENABLE_RGN
    // create osd, VPE port 2 has't OSD
    // attach
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = i;
    stChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));

    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = u32Width - RGN_OSD_TIME_WIDTH - 10;
    stChnPortParam.stPoint.u32Y = 10;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0;
    stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xFF;
    ExecFunc(MI_RGN_AttachToChn(RGN_OSD_HANDLE, &stChnPort, &stChnPortParam), MI_RGN_OK);
    printf("ST_StartPipeLineWithDip[%d]: attach osd %d to divp(%d,0)\n", i, RGN_OSD_HANDLE, i);

#if 0
    // create cover1
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
    ExecFunc(MI_RGN_Create(pstStreamAttr[i].u32Cover1Handle, &stRgnAttr), MI_RGN_OK);
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = i;
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
    stChnPort.s32ChnId = i;
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
#endif
#endif

    // bind VPE to divp
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = i;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    // bind divp to venc
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = i;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    if(i == 2)
    {
       stBindInfo.u32DstFrmrate = 10;
    }
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
Exit:
    return MI_SUCCESS;

}
MI_S32 ST_StopPipeLine(MI_U8 i)
{
    MI_VENC_CHN VencChn = 0;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = -1;
    MI_RGN_ChnPort_t stChnPort;

    CanvasScopeLock ScopeLock;

#ifdef ENABLE_RGN
    // Destory osd
    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
    stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
    ExecFunc(MI_RGN_DetachFromChn(RGN_OSD_HANDLE, &stChnPort), MI_RGN_OK);
    ExecFunc(MI_RGN_DetachFromChn(VPE_PORT1_OSD_HANDLE, &stChnPort), MI_RGN_OK);

    if (bCh0NeedPipeLine)
    {
        ExecFunc(MI_RGN_DetachFromChn(VPE_PORT1_COVER_HANDLE, &stChnPort), MI_RGN_OK);
        ExecFunc(MI_RGN_DetachFromChn(VPE_PORT1_COVER_1_HANDLE, &stChnPort), MI_RGN_OK);
    }
    printf("ST_StopPipeLine[%d]: detach osd %d from vpe(%d,%d)\n", i, RGN_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);
#endif

    VencChn = pstStreamAttr[i].vencChn;
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    STCHECKRESULT(ST_Venc_StopChannel(VencChn));
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

    CanvasScopeLock ScopeLock;

#ifdef ENABLE_RGN
    if (pstStreamAttr[i].eType != 0)
    {
#if 0
        // destory cover
        ExecFunc(ST_OSD_Destroy(pstStreamAttr[i].u32Cover1Handle), MI_RGN_OK);
        ExecFunc(ST_OSD_Destroy(pstStreamAttr[i].u32Cover2Handle), MI_RGN_OK);
#endif
        // detach osd
        memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
        stChnPort.eModId = E_MI_RGN_MODID_DIVP;
        stChnPort.s32DevId = 0;
        stChnPort.s32ChnId = i;
        stChnPort.s32OutputPortId = 0;
        ExecFunc(MI_RGN_DetachFromChn(RGN_OSD_HANDLE, &stChnPort), MI_RGN_OK);
        printf("ST_StopPipeLineWithDip[%d]: detach osd %d from divp(%d,0)\n", i, RGN_OSD_HANDLE, i);
    }
#endif

    if (pstStreamAttr[i].eType == E_MI_VENC_MODTYPE_MAX)
        goto EXIT;
	VencChn = pstStreamAttr[i].vencChn;
    STCHECKRESULT(ST_Venc_StopChannel(VencChn));
    // unbind divp & venc
    
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = i;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));

EXIT:
    ExecFunc(MI_DIVP_StopChn(i), MI_SUCCESS);
    // unbind vpe & divp
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
    stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = i;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    ExecFunc(MI_DIVP_DestroyChn(i), MI_SUCCESS);
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

typedef enum
{
   STUB_SENSOR_TYPE_IMX323 = 0,
   STUB_SENSOR_TYPE_IMX291 = 1,
   STUB_SENSOR_TYPE_IMX307 = 2,
}EN_RTSP_SensorType_e;

MI_VIF_DevAttr_t DEV_ATTR_DVP_BASE_rtsp =
{
    E_MI_VIF_MODE_DIGITAL_CAMERA,
    E_MI_VIF_WORK_MODE_RGB_REALTIME,
    E_MI_VIF_HDR_TYPE_OFF,
    E_MI_VIF_CLK_EDGE_DOUBLE,
    E_MI_VIF_INPUT_DATA_YUYV,
    E_MI_VIF_BITORDER_NORMAL
};

MI_VIF_DevAttr_t DEV_ATTR_MIPI_BASE_rtsp =
{
    E_MI_VIF_MODE_MIPI,
    E_MI_VIF_WORK_MODE_RGB_REALTIME,
    E_MI_VIF_HDR_TYPE_OFF,
    E_MI_VIF_CLK_EDGE_DOUBLE,
    E_MI_VIF_INPUT_DATA_YUYV,
    E_MI_VIF_BITORDER_NORMAL
};



//static EN_RTSP_SensorType_e _u32SensorType = STUB_SENSOR_TYPE_IMX323;

static void setRtspVifDevAttr(MI_VIF_DevAttr_t *pVifDev, EN_RTSP_SensorType_e sensorType)
{
    switch(sensorType)
    {
    case STUB_SENSOR_TYPE_IMX323:
        memcpy(pVifDev, &DEV_ATTR_DVP_BASE_rtsp, sizeof(MI_VIF_DevAttr_t));
        break;
    case STUB_SENSOR_TYPE_IMX291:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_rtsp, sizeof(MI_VIF_DevAttr_t));
        break;
    case STUB_SENSOR_TYPE_IMX307:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_rtsp, sizeof(MI_VIF_DevAttr_t));
        break;
    default:
        memcpy(pVifDev, &DEV_ATTR_MIPI_BASE_rtsp, sizeof(MI_VIF_DevAttr_t));
        break;
    }
    pVifDev->eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    return;
}


static void setRtspVifChnAttr(MI_VIF_ChnPortAttr_t *pVifChnPortAttr, EN_RTSP_SensorType_e sensorType)
{
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    switch(sensorType)
    {
    case STUB_SENSOR_TYPE_IMX323:
        pVifChnPortAttr->stCapRect.u16X = 108;
        pVifChnPortAttr->stCapRect.u16Y = 28;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    case STUB_SENSOR_TYPE_IMX291:
        pVifChnPortAttr->stCapRect.u16X = 0;
        pVifChnPortAttr->stCapRect.u16Y = 0;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    case STUB_SENSOR_TYPE_IMX307:
        pVifChnPortAttr->stCapRect.u16X = 0;
        pVifChnPortAttr->stCapRect.u16Y = 0;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    default:
        pVifChnPortAttr->stCapRect.u16X = 108;
        pVifChnPortAttr->stCapRect.u16Y = 28;
        pVifChnPortAttr->ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
        break;
    }

    pVifChnPortAttr->stCapRect.u16Width  = 1920;
    pVifChnPortAttr->stCapRect.u16Height = 1080;
    pVifChnPortAttr->stDestSize.u16Width  = 1920;
    pVifChnPortAttr->stDestSize.u16Height = 1080;

    pVifChnPortAttr->eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    pVifChnPortAttr->eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    //E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pVifChnPortAttr->eFrameRate = E_MI_VIF_FRAMERATE_FULL;
}

static MI_SYS_PixelFormat_e getRtspVpeRawType_venc(EN_RTSP_SensorType_e sensorType)
{
    MI_SYS_PixelFormat_e eVpeRawType = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_NUM;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;

    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);
    eVpeRawType =  (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    return eVpeRawType;
}

MI_S32 CreatePicOsd(MI_RGN_HANDLE handle, ST_TestFileInfo_t *pstTestFileInfo)
{
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    FILE *pFile = NULL;
    MI_U8 *pData = NULL;
    MI_U32 u32FileSize = 0;
    MI_U16 u16CopyLineBytes = 0;
    MI_S32 i = 0;

    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    stRgnAttr.stOsdInitParam.ePixelFmt = pstTestFileInfo->ePixelFmt;
    stRgnAttr.stOsdInitParam.stSize.u32Width = pstTestFileInfo->u16RgnWidth;
    stRgnAttr.stOsdInitParam.stSize.u32Height = pstTestFileInfo->u16RgnHeight;

    // create pic osd
    ExecFunc(ST_OSD_Create(handle, &stRgnAttr), MI_RGN_OK);

    // read pic data
    switch (pstTestFileInfo->ePixelFmt)
    {
        case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
        case E_MI_RGN_PIXEL_FORMAT_ARGB4444:
        case E_MI_RGN_PIXEL_FORMAT_RGB565:
            u16CopyLineBytes = pstTestFileInfo->u16RgnWidth*2;
            break;
        case E_MI_RGN_PIXEL_FORMAT_I2:
            u16CopyLineBytes = pstTestFileInfo->u16RgnWidth/4;
            break;
        case E_MI_RGN_PIXEL_FORMAT_I4:
            u16CopyLineBytes = pstTestFileInfo->u16RgnWidth/2;
            break;
        case E_MI_RGN_PIXEL_FORMAT_I8:
            u16CopyLineBytes = pstTestFileInfo->u16RgnWidth;
            break;
        case E_MI_RGN_PIXEL_FORMAT_ARGB8888	:
            u16CopyLineBytes = pstTestFileInfo->u16RgnWidth*4;
            break;
        default:
            printf("pixel format is not supported\n");
            return -1;
    }

    u32FileSize = u16CopyLineBytes * pstTestFileInfo->u16RgnHeight;
    pData = (MI_U8*)malloc(u32FileSize);
    memset(pData, 0, u32FileSize);

    pFile = fopen(pstTestFileInfo->fileName, "rb");
    if (!pFile)
    {
        printf("set usr pic failed\n");
        return -1;
    }

    fread(pData, 1, u32FileSize, pFile);
    fclose(pFile);
    pFile = NULL;

    printf("file fmt:%d, w:%d, h:%d\n", pstTestFileInfo->ePixelFmt, pstTestFileInfo->u16RgnWidth, pstTestFileInfo->u16RgnHeight);

    // copy data to bitmap struct
    memset(&stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));
    ExecFunc(MI_RGN_GetCanvasInfo(handle, &stCanvasInfo), MI_RGN_OK);

    for (i = 0; i < pstTestFileInfo->u16RgnHeight; i++)
        memcpy((MI_U8*)stCanvasInfo.virtAddr+i*stCanvasInfo.u32Stride, pData+i*u16CopyLineBytes, u16CopyLineBytes);

    ExecFunc(MI_RGN_UpdateCanvas(handle), MI_RGN_OK);

    free(pData);
    pData = NULL;

    return MI_SUCCESS;
}

MI_S32 DestroyPicOsd(MI_RGN_HANDLE handle)
{
    ExecFunc(ST_OSD_Destroy(handle), MI_RGN_OK);

    return MI_SUCCESS;
}

MI_S32 ST_OsdStart()
{
    MI_RGN_Attr_t stRgnAttr;
//  MI_RGN_CanvasInfo_t stCanvasInfo;
//    int i = 0;

    ExecFunc(ST_OSD_Init(), MI_RGN_OK);

    // osd
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
//    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    stRgnAttr.stOsdInitParam.stSize.u32Width = RGN_OSD_TIME_WIDTH;
    stRgnAttr.stOsdInitParam.stSize.u32Height = RGN_OSD_TIME_HEIGHT;
    ExecFunc(ST_OSD_Create(RGN_OSD_HANDLE, &stRgnAttr), MI_RGN_OK);

    // create osd for vpe port1, handle 100
    CreatePicOsd(VPE_PORT1_OSD_HANDLE, &g_stArgb1555FileInfo[2]);

    if (bCh0NeedPipeLine)
    {
        // create cover for vpe port1, handle 101
        memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
        STCHECKRESULT(MI_RGN_Create(VPE_PORT1_COVER_HANDLE, &stRgnAttr));
        // create cover for vpe port1, handle 102
        memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
        STCHECKRESULT(MI_RGN_Create(VPE_PORT1_COVER_1_HANDLE, &stRgnAttr));

    }

#if 0
    memset(&stCanvasInfo, 0, sizeof(stCanvasInfo));
    ExecFunc(MI_RGN_GetCanvasInfo(RGN_OSD_HANDLE, &stCanvasInfo), MI_RGN_OK);

    for (i = 0; i < stCanvasInfo.stSize.u32Height; i++)
    {
        // I4
        memset((MI_U8*)(stCanvasInfo.virtAddr + i*stCanvasInfo.u32Stride), 0x22, stCanvasInfo.u32Stride);

        //  argb1555
//        memset((MI_U16*)(stCanvasInfo.virtAddr + i*stCanvasInfo.u32Stride), 0xFC00, stCanvasInfo.u32Stride/2);
    }
    ExecFunc(MI_RGN_UpdateCanvas(RGN_OSD_HANDLE), MI_RGN_OK);
#endif


    return MI_SUCCESS;
}

MI_S32 ST_OsdStop()
{
    ExecFunc(DestroyPicOsd(VPE_PORT1_OSD_HANDLE), MI_SUCCESS);
    ExecFunc(ST_OSD_Destroy(VPE_PORT1_COVER_HANDLE), MI_RGN_OK);
    ExecFunc(ST_OSD_Destroy(VPE_PORT1_COVER_1_HANDLE), MI_RGN_OK);
    ExecFunc(ST_OSD_Destroy(RGN_OSD_HANDLE), MI_RGN_OK);
    //ExecFunc(ST_OSD_DeInit(), MI_RGN_OK);

    return MI_SUCCESS;
}


MI_S32 ST_BaseModuleInit(ST_Config_S* pstConfig)
{
//    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
//    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
//    MI_SYS_PixelFormat_e ePixFormat;
//    MI_RGN_Attr_t stRgnAttr;
    MI_VPE_ChannelAttr_t stVpeChannelInfo;
    ST_Sys_BindInfo_T stBindInfo;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
//    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
//    MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
    MI_SNR_Res_t stRes;

     MI_U32 u32VifDevId;
    MI_U32 u32VifChnId;
    MI_U32 u32VifPortId;

    MI_VIF_DevAttr_t stVifDevAttr;
    MI_VIF_ChnPortAttr_t stVifChnPortAttr;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));


    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(MI_SYS_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0,0);
    /************************************************
    Step2:  init VIF
    *************************************************/
    u32VifDevId = 0;
    u32VifChnId = 0;
    u32VifPortId = 0;
    memset(&stVifDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));
    setRtspVifDevAttr(&stVifDevAttr, STUB_SENSOR_TYPE_IMX307);
    stVifDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    STCHECKRESULT(MI_VIF_SetDevAttr(u32VifDevId, &stVifDevAttr));
    STCHECKRESULT(MI_VIF_EnableDev(u32VifDevId));

    memset(&stVifChnPortAttr, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    setRtspVifChnAttr(&stVifChnPortAttr, STUB_SENSOR_TYPE_IMX307);
    STCHECKRESULT(MI_VIF_SetChnPortAttr(u32VifChnId, u32VifPortId, &stVifChnPortAttr));
    MI_SNR_Enable(E_MI_SNR_PAD_ID_0);
    STCHECKRESULT(MI_VIF_EnableChnPort(u32VifChnId, u32VifPortId));
    MI_SNR_SetFps(E_MI_SNR_PAD_ID_0, 30);



    memset(&stVpeChannelInfo, 0x0, sizeof(MI_VPE_ChannelAttr_t));
    stVpeChannelInfo.u16MaxW = 1920;
    stVpeChannelInfo.u16MaxH = 1080;
    stVpeChannelInfo.bNrEn = FALSE;
    stVpeChannelInfo.bEsEn = FALSE;
    stVpeChannelInfo.bEdgeEn = FALSE;
    stVpeChannelInfo.bUvInvert = FALSE;
    stVpeChannelInfo.bContrastEn = FALSE;
    stVpeChannelInfo.ePixFmt  = getRtspVpeRawType_venc(STUB_SENSOR_TYPE_IMX307);
    stVpeChannelInfo.eHDRType = E_MI_VPE_HDR_TYPE_OFF;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eSensorBindId = E_MI_VPE_SENSOR0;
    printf("vpe raw type = %d\n", stVpeChannelInfo.ePixFmt );
    STCHECKRESULT(MI_VPE_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(MI_VPE_StartChannel(0));



    //return 1;
    //STCHECKRESULT(ST_Vpe_StartChannel(0));
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
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

#ifdef ENABLE_RGN
/*
    // rgn init
    STCHECKRESULT(ST_OSD_Init());
//    STCHECKRESULT(MI_RGN_Init(&_gstPaletteTable));
   // MI_IQSERVER_Open(u32CapWidth, u32CapHeight, 0);
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stRgnAttr.stOsdInitParam.stSize.u32Width = RGN_OSD_TIME_WIDTH;
    stRgnAttr.stOsdInitParam.stSize.u32Height = RGN_OSD_TIME_HEIGHT;
    ExecFunc(MI_RGN_Create(RGN_OSD_HANDLE, &stRgnAttr), MI_RGN_OK);
*/
    STCHECKRESULT(ST_OsdStart());
#endif
#if (ENABLE_BUF_POOL == 1)
    STCHECKRESULT(ST_InitBufPoolEnv());
#endif
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

            switch (pstCanvasInfo->ePixelFmt)
            {
                case E_MI_RGN_PIXEL_FORMAT_I4:
                    (void)ST_OSD_DrawText(RGN_OSD_HANDLE, stPoint, szTime, I4_BLACK, DMF_Font_Size_16x16);
                    break;
                case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
                    (void)ST_OSD_DrawText(RGN_OSD_HANDLE, stPoint, szTime, RGB2PIXEL1555(0,0,0), DMF_Font_Size_16x16);
                    break;
                default:
                    printf("pixel format is not supported!\n");
                    break;
            }

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

#ifdef ENABLE_RGN
    ExecFunc(ST_OsdStop(), MI_SUCCESS);
#endif

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

//    // rgn deinit
//    STCHECKRESULT(MI_RGN_DeInit());

    /************************************************
    Step3:  destory SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Exit());

    //MI_IQSERVER_Close();

#if (ENABLE_BUF_POOL == 1)
    ST_DeinitBufPoolEnv();
#endif
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
        if (0 == i && FALSE == bCh0NeedPipeLine)    // run rtos base flow
        {
#ifdef ENABLE_RGN
            // attach
            MI_RGN_ChnPort_t stChnPort;
            MI_RGN_ChnPortParam_t stChnPortParam;

            memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
            stChnPort.eModId = E_MI_RGN_MODID_VPE;
            stChnPort.s32DevId = 0;
            stChnPort.s32ChnId = pstStreamAttr[i].u32InputChn;
            stChnPort.s32OutputPortId = pstStreamAttr[i].u32InputPort;
            memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));

            stChnPortParam.bShow = TRUE;
            stChnPortParam.stPoint.u32X = pstStreamAttr[i].u32Width - RGN_OSD_TIME_WIDTH - 10;
            stChnPortParam.stPoint.u32Y = 10;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xFF;
            ExecFunc(MI_RGN_AttachToChn(RGN_OSD_HANDLE, &stChnPort, &stChnPortParam), MI_RGN_OK);
            printf("attach osd %d to vpe(%d,%d)\n", RGN_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);

            // attach osd 100 to vpe port1
            memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
            stChnPortParam.bShow = TRUE;
            stChnPortParam.stPoint.u32X = 200;
            stChnPortParam.stPoint.u32Y = 400;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0;
            stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xFF;
            STCHECKRESULT(MI_RGN_AttachToChn(VPE_PORT1_OSD_HANDLE, &stChnPort, &stChnPortParam));
            printf("attach osd %d to vpe(%d,%d)\n", VPE_PORT1_OSD_HANDLE, pstStreamAttr[i].u32InputChn, pstStreamAttr[i].u32InputPort);

            printf("ch 0 not init continue\n");
#endif
#if (ENABLE_BUF_POOL == 1)
            STCHECKRESULT(ST_OpenBufPool(pstStreamAttr[i].vencChn));
#endif
			continue;
        }
        if(pstStreamAttr[i].enInput == ST_Sys_Input_VPE)
        {
            printf("chn %d startPipeLine\n", i);
            ST_StartPipeLine(i, pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height, pstStreamAttr[i].u32CropWidth, pstStreamAttr[i].u32CropHeight, pstStreamAttr[i].u32CropX, pstStreamAttr[i].u32CropY);
        }
        else if (pstStreamAttr[i].enInput == ST_Sys_Input_DIVP)
        {
            printf("chn %d startPipeLineWithDip\n", i);
            ST_StartPipeLineWithDip(i, pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height, pstStreamAttr[i].u32CropWidth, pstStreamAttr[i].u32CropHeight, pstStreamAttr[i].u32CropX, pstStreamAttr[i].u32CropY);
        }
#if (ENABLE_BUF_POOL == 1)
       if(pstStreamAttr[i].eType != E_MI_VENC_MODTYPE_MAX)
       {
		    STCHECKRESULT(ST_OpenBufPool(pstStreamAttr[i].vencChn));
       }
#endif
    }

    /************************************************
    Step3:  start VENC
    *************************************************/
    g_stRgnOsd.bRun = TRUE;
#ifdef ENABLE_RGN
    // pthread_create(&g_stRgnOsd.pt, NULL, ST_UpdateRgnOsdProc, NULL);
    pthread_create(&g_stRgnOsd.pt, NULL, ST_UpdateRgnOsdProcExt, NULL);
#endif
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
#if (ENABLE_BUF_POOL == 1)
        ST_CloseBufPool(pstStreamAttr[i].vencChn);
#endif
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


#if 0
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

    //ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13), MI_SUCCESS);

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
        //_gstAoOutchn.bRunFlag = TRUE;
        //pthread_create(&_gstAoOutchn.pt, NULL, ST_AoOutProc, &_gstAoOutchn);
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

    //_gstAoOutchn.bRunFlag = FALSE;
    //pthread_join(_gstAoOutchn.pt, NULL);

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

int ST_VDFMDSadMdNumCal(MI_U8 *pu8MdRstData, int i, int j, int col, int row, int cusCol, int cusRow)
{
    int c, r;
    int rowIdx = 0;
    int sad8BitThr = 15;
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

int ST_VDFMDtoRECT_SAD(MI_U8 *pu8MdRstData, int col, int row)
{
    int i, j;
    MI_S32 md_detect_cnt = 0;
    MI_RGN_HANDLE hHandle = 0;
    MI_U8 u8BorderWidth = 1;
    ST_Rect_T stRect[(RAW_W / 4) * (RAW_H / 4)] = {{0,},};
    MI_RGN_CanvasInfo_t *pstCanvasInfo = NULL;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32Width    = 0, u32Height = 0;
    MI_VPE_PortMode_t stVpeMode;
	//ST_VDF_OSD_Info_T *pstVDFOsdInfo = g_stVDFOsdInfo;

    int cusCol = 4;        // 4 macro block Horizontal
    int cusRow = 2;     // 3 macro block vertical

    for(i = 0; i < MAX_FULL_RGN_NULL; i ++)
    {
        if(ST_OSD_HANDLE_INVALID == pstVDFOsdInfo[i].hHandle)
        {
            continue;
        }

        // clear osd
        CANVAS_LOCK;

        ST_OSD_GetCanvasInfo(pstVDFOsdInfo[i].hHandle, &pstCanvasInfo);

        ST_OSD_Clear(pstVDFOsdInfo[i].hHandle, NULL);

        ST_OSD_Update(pstVDFOsdInfo[i].hHandle);

         CANVAS_UNLOCK;
        // clear osd
    }

    // draw rect
    if(pu8MdRstData)
    {
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

                    stRect[md_detect_cnt].u32X = (j * VDF_BASE_WIDTH / col) & 0xFFFE;
                    stRect[md_detect_cnt].u32Y = (i * VDF_BASE_HEIGHT / row) & 0xFFFE;
                    stRect[md_detect_cnt].u16PicW = (cusCol * VDF_BASE_WIDTH / col) & 0xFFFE;
                    stRect[md_detect_cnt].u16PicH = (cusRow * VDF_BASE_HEIGHT / row) & 0xFFFE;
                    // printf("osd idx %d, x %d,y %d,w %d,h %d\n", md_detect_cnt, stRect[md_detect_cnt].u32X, stRect[md_detect_cnt].u32Y,
                    //                                stRect[md_detect_cnt].u16PicW, stRect[md_detect_cnt].u16PicH);

                    md_detect_cnt++;
                }
            }
        }
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
        CANVAS_LOCK;

        ST_OSD_GetCanvasInfo(pstVDFOsdInfo[i].hHandle, &pstCanvasInfo);

        for(j = 0; j < md_detect_cnt; j++)
        {
			//ST_DBG("handle:%d, chn:%d, port:%d, rect(%d, %d, %dx%d)\n", pstVDFOsdInfo[i].hHandle,
			//    pstVDFOsdInfo[i].u32Chn, pstVDFOsdInfo[i].u32Port,
			//    stRect[j].u32X, stRect[j].u32Y, stRect[j].u16PicW, stRect[j].u16PicH);

            stRect[j].u32X = stRect[j].u32X * u32Width / VDF_BASE_WIDTH;
            stRect[j].u32Y = stRect[j].u32Y * u32Height / VDF_BASE_HEIGHT;
            stRect[j].u16PicW = stRect[j].u16PicW * u32Width / VDF_BASE_WIDTH;
            stRect[j].u16PicH = stRect[j].u16PicH * u32Height / VDF_BASE_HEIGHT;
            ST_OSD_DrawRect(pstVDFOsdInfo[i].hHandle, stRect[j], 3, I4_RED);
        }

        ST_OSD_Update(pstVDFOsdInfo[i].hHandle);

        CANVAS_UNLOCK;
    }

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
    MI_U32 col, row, buffer_size;
    MI_U8 stSadDataArry[(RAW_W / 4) * (RAW_H / 4) * 2] = {0,};

    if(pstArgs->enWorkMode == E_MI_VDF_WORK_MODE_MD)
    {
        ST_DBG("Get md result, chn:%d\n", vdfChn);

        // hard code, MDMB_MODE_MB_8x8
        col = RAW_W >> 3;    // 48
        row = RAW_H >> 3;    // 36

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
                pu8MdRstData = (MI_U8 *)stVdfResult.stMdResult.pstMdResultSad;
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
        }
        else
        {
            // printf("[MD_TEST][HDL=0x0] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, s32Ret);
        }

        ST_VDFMDtoRECT_SAD(stSadDataArry, col, row);

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
                stBindInfo.u32DstFrmrate = 12;
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

                //pthread_create(&g_stVdfThreadArgs[vdfChn].pt, NULL, ST_VDFGetResult, (void *)&g_stVdfThreadArgs[vdfChn]);
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
                stBindInfo.u32DstFrmrate = 12;

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
    MI_SYS_WindowRect_t Rect;
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

            stVdfAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 70;
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

            Rect.u16X = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X;
            Rect.u16Y = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y;
            Rect.u16Height = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicH;
            Rect.u16Height = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicW;
            ST_DBG("Draw MD Area (%d,%d,%d,%d)\n", Rect.u16X, Rect.u16Y, Rect.u16Width, Rect.u16Height);

            // ST_Fb_DrawRect(&Rect, ARGB888_RED);

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

            Rect.u16X = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32X;
            Rect.u16Y = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u32Y;
            Rect.u16Height = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicH;
            Rect.u16Height = g_stVdfSettingAttr[i].stVdfArgs.stMdArea[j].stArea.u16PicW;
            ST_DBG("Draw OD Area (%d,%d,%d,%d)\n", Rect.u16X, Rect.u16Y, Rect.u16Width, Rect.u16Height);

            // ST_Fb_DrawRect(&Rect, ARGB888_GREEN);

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
#endif
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
//    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
	MI_U32 i = 0, j = 0;

    memset(pstConfig, 0, sizeof(ST_Config_S));
    ST_DefaultConfig(pstConfig);

    pstConfig->s32UseOnvif = 0;
    pstConfig->s32UseVdf = 1;
    pstConfig->s32LoadIQ = 0;
    pstConfig->enRotation = E_MI_SYS_ROTATE_NONE;
    pstConfig->en3dNrLevel = E_MI_VPE_3DNR_LEVEL1;

    pstStreamAttr[0].bEnable = FALSE;

    pstStreamAttr[1].bEnable = TRUE;
    pstStreamAttr[1].eType = E_MI_VENC_MODTYPE_H264E;
    pstStreamAttr[1].u32Width = 1920;
    pstStreamAttr[1].u32Height = 1080;
    pstStreamAttr[1].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    pstStreamAttr[1].u32BindPara = pstStreamAttr[1].u32Height;

    pstStreamAttr[2].bEnable = TRUE;
    pstStreamAttr[2].eType = E_MI_VENC_MODTYPE_H264E;
    pstStreamAttr[2].u32Width = 720;
    pstStreamAttr[2].u32Height = 576;
    pstStreamAttr[2].eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    pstStreamAttr[2].u32BindPara = 0;
    pstStreamAttr[2].enInput = ST_Sys_Input_DIVP;

	for(i = 0; i < MAX_CHN_NEED_OSD; i ++)
    {
        for(j = 0; j < MAX_OSD_NUM_PER_CHN; j ++)
        {
            g_stRgnOsd.stOsdInfo[i][j].hHandle = ST_OSD_HANDLE_INVALID;
        }
    }
/*
	for (i = 0; i < MAX_FULL_RGN_NULL; i ++)
	{
		g_stVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
	}
	*/
}

void ST_ResetArgs(ST_Config_S *pstConfig)
{
//    MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
//    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
//    MI_U32 i = 0, j = 0;
//    MI_S32 s32GetData = 0;
//    MI_U32 u32Temp = 0;

    memset(pstConfig, 0, sizeof(ST_Config_S));
    ST_DefaultConfig(pstConfig);
#if 1
        pstConfig->en3dNrLevel = E_MI_VPE_3DNR_LEVEL2;

#else
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

#if USE_AUDIO
    printf("Use audio ?\n 0 not use, 1 use\n");
    scanf("%d", &pstConfig->s32UseAudio);
    ST_Flush();
    printf("You select %s audio out\n", pstConfig->s32UseAudio ? "use" : "not use");
#endif

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
        printf("Encode bitrate (Mbps)? \n");
        scanf("%d", &s32GetData);
        ST_Flush();
        if (s32GetData > 0 && s32GetData < 10)
        {
            pstStreamAttr[i].u32Mbps = s32GetData;
        }
        printf("Encode size, %dx%d, %dMbps\n", pstStreamAttr[i].u32Width, pstStreamAttr[i].u32Height, pstStreamAttr[i].u32Mbps);

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
            pstStreamAttr[i].u32Height = ALIGN_DOWN(u32Temp, 32);

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
/*
	for (i = 0; i < MAX_FULL_RGN_NULL; i ++)
	{
		g_stVDFOsdInfo[i].hHandle = ST_OSD_HANDLE_INVALID;
	}
	*/
#endif
}
void ST_DestoryInputCmd(void);
void ST_HandleSig(MI_S32 signo)
{
    if(signo == SIGINT)
    {
        ST_INFO("catch Ctrl + C, exit normally\n");
    }
	else if (signo == SIGTERM){
		ST_INFO("catch SIGTERM, exit normally\n");
	}

	if(g_bExit)
		goto END_HANDLE_SIG;

	g_bExit = TRUE;

#if 1
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

	ST_Thread_Stop(&pthreadHandlearray[0], SUB_STREAM3);
    //ST_RtspServerStop();
    ST_VencStop();

    ST_BaseModuleUnInit();
    ST_DestoryInputCmd();
#endif
END_HANDLE_SIG:
	ST_INFO("%s exit\n",__FUNCTION__);
	_exit(0);
}

#ifdef __cplusplus
extern "C"
{
#endif
//int MI_IQSERVER_GetSensorRawData(char** GetBuf, int* GetLen);
//int MI_IQSERVER_GetSensorRawData_ByWDMA6(MI_U32 vpeChn, char** GetBuf, int* GetLen);
int MI_IQSERVER_GetSensorRawDataEx(MI_U32 vpeChn, char** GetBuf, int* GetLen);

int MI_IQSERVER_GetIspOutput(MI_U32 vpeChn, char** GetBuf, int* GetLen);
int MI_IQSERVER_GetVpeOutput(MI_U32 vpeChn, char** GetBuf, int* GetLen);
int MI_IQSERVER_GetJpeg(MI_U32 vpeChn, char** GetBuf, int* GetLen);

#ifdef __cplusplus
}   //end of extern C
#endif

#define CALI_FILE_PATH    "./cali.data"
#define CALI_AWB_FILE_PATH    "./ps5250_awb_cali.data"

void ST_DoLoadCaliDataFile(void *args)
{
    int ret;

    ret= MI_ISP_API_CmdLoadCaliData(0, SS_CALI_ITEM_ALSC , (char *)CALI_FILE_PATH );

    printf("load cali bin SS_CALI_ITEM_ALSC  file name = %s ,ret=%d",CALI_FILE_PATH,ret);
    ret= MI_ISP_API_CmdLoadCaliData(0, SS_CALI_ITEM_AWB , (char *)CALI_AWB_FILE_PATH );
    printf("load cali bin SS_CALI_ITEM_AWB  file name = %s ,ret=%d",CALI_AWB_FILE_PATH, ret);

}

extern MI_U32 g_expMode;
void ST_DoCaptureBayerRAW(void *args)
{
    CusImageResolution_t timage_resolution;
    MI_ISP_CUS3A_GetImageResolution(0, &timage_resolution);
    timage_resolution.u32Node = (MI_U32)eRawStoreNode_P0TAIL; //eRawStoreNode_P1HEAD;
    printf("Image width=%d, height=%d\n", timage_resolution.u32image_width, timage_resolution.u32image_height);

	int  TestLen = timage_resolution.u32image_width*timage_resolution.u32image_height;
	char * TestBuf = (char*)malloc(TestLen);
	g_expMode=0;
	//MI_IQSERVER_GetSensorRawData(&TestBuf,&TestLen);
	MI_IQSERVER_GetSensorRawDataEx(0,&TestBuf,&TestLen);

	int fd;
	fd = open("test.raw", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, TestBuf, TestLen);
	close(fd);
	free(TestBuf);
}

void ST_DoCaptureIspOutput(void *args)
{
	int  TestLen = 1920*1080;
	char * TestBuf = (char*)malloc(TestLen);

	MI_IQSERVER_GetIspOutput(0,&TestBuf,&TestLen);

	int fd;
	fd = open("test.yuyv", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, TestBuf, TestLen);
	close(fd);
	free(TestBuf);
}

void ST_DoCaptureVpeOutput(void *args)
{
	int  TestLen = 1920*1080;
	char * TestBuf = (char*)malloc(TestLen);

	MI_IQSERVER_GetVpeOutput(0,&TestBuf,&TestLen);
	int fd;
	fd = open("test.nv12", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, TestBuf, TestLen);
	close(fd);
	free(TestBuf);
}

void ST_DoCaptureJPG(void *args)
{
	int  TestLen = 1920*1080;
	char * TestBuf = (char*)malloc(TestLen);
	MI_IQSERVER_GetJpeg(0,&TestBuf,&TestLen);

	int fd;
	fd = open("test.jpg", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	write(fd, TestBuf, TestLen);
	close(fd);
	free(TestBuf);
}

void ST_DoSetQP(void *args)
{

	MI_VENC_ChnAttr_t stChnAttr;
	memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

	MI_VENC_GetChnAttr(3, &stChnAttr);
	stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor= 90 ; //20~90
	MI_VENC_SetChnAttr(3, &stChnAttr);
	printf("stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor =%d\n", stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor );

}

MI_S32 _DoSetRES(void)
{

     static int i=0;
	 MI_U32 u32Width = 0;
	 MI_U32 u32Height = 0;

	if(i%3==0)
	{
		u32Width = 1280;
		u32Height = 720;
	}
	else if(i%3==1)
	{
		u32Width = 640;
		u32Height = 360;
	}
	else if(i%3==2)
	{
		u32Width = 1920;
		u32Height = 1080;
	}
	i++;

	//===============================================
	MI_VPE_PortMode_t stVpeMode;
	memset(&stVpeMode, 0, sizeof(stVpeMode));
	ExecFunc(MI_VPE_GetPortMode(0, 1, &stVpeMode), MI_VPE_OK);
	stVpeMode.u16Width = u32Width;
	stVpeMode.u16Height= u32Height;
	ExecFunc(MI_VPE_SetPortMode(0, 1, &stVpeMode), MI_VPE_OK);
	//===============================================


    return MI_SUCCESS;
}


void ST_DoSetRES(void *args)
{
	_DoSetRES();
}

void ST_DoSetFPS(void *args)
{
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = -1;

	static MI_U32 videoParam=0;
	MI_SNR_Res_t stSensorCurRes;
	MI_U32 u32CurFps = 0;
	MI_U8 g_u8SnrCurResIdx = 0;
	MI_U32 u32ArraySize = ARRAY_SIZE(g_stStreamAttr);
	ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
	MI_U32 i = 0;
	
	if ( (videoParam+=5)>30 )
		videoParam=5;

	MI_SNR_GetCurRes(E_MI_SNR_PAD_ID_0, &g_u8SnrCurResIdx, &stSensorCurRes);
	MI_SNR_GetFps(E_MI_SNR_PAD_ID_0, &u32CurFps);

	if((u32CurFps != videoParam) && (stSensorCurRes.u32MinFps <= videoParam) && (videoParam <= stSensorCurRes.u32MaxFps))
	{
		//MI_SNR_SetFps(E_MI_SNR_PAD_ID_0, videoParam);
	}
	printf("%s:%d current sensor fps:%d(min:%d, max:%d), Only set venc0  fps:%d\n", __func__, __LINE__,
										 u32CurFps, stSensorCurRes.u32MinFps, stSensorCurRes.u32MaxFps, videoParam);
#if 1
	MI_VENC_ChnAttr_t stChnAttr;
	MI_VENC_GetChnAttr(0, &stChnAttr);
	stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = videoParam;
	stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
	MI_VENC_SetChnAttr(0, &stChnAttr);

    MI_VENC_GetChnDevid(0, &u32DevId);
	// bind VPE->VENC
	memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
	stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
	stBindInfo.stSrcChnPort.u32DevId = 0;
	stBindInfo.stSrcChnPort.u32ChnId = 0;
	stBindInfo.stSrcChnPort.u32PortId = 1;
	stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
	stBindInfo.stDstChnPort.u32DevId = u32DevId;
	stBindInfo.stDstChnPort.u32ChnId = 0;
	stBindInfo.stDstChnPort.u32PortId = 0;
	stBindInfo.u32SrcFrmrate = 30;
	stBindInfo.u32DstFrmrate = videoParam;
	stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;

	ST_Sys_Bind(&stBindInfo);
#else
    //bind VIF->VPE
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
    stBindInfo.u32SrcFrmrate = videoParam;
    stBindInfo.u32DstFrmrate = videoParam;
	ST_Sys_Bind(&stBindInfo);
	
    //bind VPE->VENC or VPE->DISP->VENC
	for(i = 0; i < u32ArraySize; i ++)
	{
		if(ST_Sys_Input_VPE == pstStreamAttr[i].enInput)
		{
			printf("chn %d ST_Sys_Bind:VPE->VENC\n", i);
			//bind VPE->VENC
			memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
			stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
			stBindInfo.stSrcChnPort.u32DevId = 0;
			stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
			stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
			
			stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
			MI_VENC_GetChnDevid(pstStreamAttr[i].u32InputChn, &u32DevId);
			stBindInfo.stDstChnPort.u32DevId = u32DevId;
			stBindInfo.stDstChnPort.u32ChnId = pstStreamAttr[i].vencChn;
			stBindInfo.stDstChnPort.u32PortId = 0;
			stBindInfo.u32SrcFrmrate = videoParam;
			stBindInfo.u32DstFrmrate = videoParam;
			stBindInfo.eBindType = pstStreamAttr[i].eBindType;
			if(stBindInfo.eBindType == E_MI_SYS_BIND_TYPE_HW_RING)
			{
				stBindInfo.u32BindParam = pstStreamAttr[i].u32Height;
			}
			else
			{
				stBindInfo.u32BindParam = 0;
			}
			printf("Bind type %d, para %d\n", stBindInfo.eBindType, stBindInfo.u32BindParam);
			ST_Sys_Bind(&stBindInfo);
		}
		else if(ST_Sys_Input_DIVP == pstStreamAttr[i].enInput)
		{
			printf("chn %d ST_Sys_Bind:VPE->DISP->VENC\n", i);
			// bind VPE to divp
			memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
			stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
			stBindInfo.stSrcChnPort.u32DevId = 0;
			stBindInfo.stSrcChnPort.u32ChnId = pstStreamAttr[i].u32InputChn;
			stBindInfo.stSrcChnPort.u32PortId = pstStreamAttr[i].u32InputPort;
			
			stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
			stBindInfo.stDstChnPort.u32DevId = 0;
			stBindInfo.stDstChnPort.u32ChnId = i;
			stBindInfo.stDstChnPort.u32PortId = 0;
			stBindInfo.u32SrcFrmrate = videoParam;
			stBindInfo.u32DstFrmrate = videoParam;
			stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
			ST_Sys_Bind(&stBindInfo);

			// bind divp to venc
			memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
			stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
			stBindInfo.stSrcChnPort.u32DevId = 0;
			stBindInfo.stSrcChnPort.u32ChnId = i;
			stBindInfo.stSrcChnPort.u32PortId = 0;
			
			stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
			MI_VENC_GetChnDevid(pstStreamAttr[i].u32InputChn, &u32DevId);
			stBindInfo.stDstChnPort.u32DevId = u32DevId;
			stBindInfo.stDstChnPort.u32ChnId = pstStreamAttr[i].vencChn;;
			stBindInfo.stDstChnPort.u32PortId = 0;
			stBindInfo.u32SrcFrmrate = videoParam;
			stBindInfo.u32DstFrmrate = videoParam;
			stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
			stBindInfo.u32BindParam = 0;
			printf("Bind type %d, para %d\n", stBindInfo.eBindType, stBindInfo.u32BindParam);
			ST_Sys_Bind(&stBindInfo);
		}
		else
		{
			//do nothing
			printf("\033[1;31m chn[%d] not set enInput\033[0m \n",i);
		}
	}					
#endif	
}

void ST_DoSetCBR(void *args)
{
	 MI_VENC_ChnAttr_t stChnAttr;
	 MI_VENC_RcParam_t stRcParam;
	 memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
	 memset(&stRcParam, 0x00, sizeof(MI_VENC_RcParam_t));

	 MI_VENC_GetChnAttr(0, &stChnAttr);
	 MI_VENC_GetRcParam(0, &stRcParam);

	 stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate=((MI_U32)args)*1024*1024;

	printf("stChnAttr.stRcAttr.eRcMode =%d\n",                         stChnAttr.stRcAttr.eRcMode );
	printf("stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate =%d(M)\n",     stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate/1024/1024 );
	printf("stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop =%d\n",            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop );
	printf("stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum =%d\n",  stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum );
	printf("stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen =%d\n",  stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen );


	printf(" stRcParam.stParamH264Cbr.u32MaxQp=%d\n",       stRcParam.stParamH264Cbr.u32MaxQp);
	printf(" stRcParam.stParamH264Cbr.u32MinQp =%d\n",      stRcParam.stParamH264Cbr.u32MinQp );
	printf(" stRcParam.stParamH264Cbr.u32MaxIQp=%d\n",      stRcParam.stParamH264Cbr.u32MaxIQp);
	printf(" stRcParam.stParamH264Cbr.u32MinIQp=%d\n",      stRcParam.stParamH264Cbr.u32MinIQp);
	printf(" stRcParam.stParamH264Cbr.s32IPQPDelta=%d\n",   stRcParam.stParamH264Cbr.s32IPQPDelta);

	MI_VENC_SetChnAttr(0, &stChnAttr);
	MI_VENC_SetRcParam(0, &stRcParam);
}


void ST_DoLoadBinFile(void *args)
{
	int ret;

	ret= MI_ISP_API_CmdLoadBinFile(0, (char *)IQ_FILE_PATH,  1234);

	printf("load bin ret=%d",ret);

}
void ST_DoAE_AUTO(void *args)
{
	MI_ISP_AE_MODE_TYPE_e data;
	MI_ISP_AE_GetExpoMode(0, &data);
	data = 	SS_AE_MODE_A;
	MI_ISP_AE_SetExpoMode(0, &data);

}

void ST_DoAE_MANUEL(void *args)
{
	MI_ISP_AE_MODE_TYPE_e data;
	MI_ISP_AE_GetExpoMode(0, &data);
	data = SS_AE_MODE_M;
	MI_ISP_AE_SetExpoMode(0, &data);
}

void ST_DoAWB_AUTO(void *args)
{
    MI_ISP_AWB_ATTR_TYPE_t data;
    MI_ISP_AWB_GetAttr(0, &data);
    data.eOpType = SS_OP_TYP_AUTO;
    MI_ISP_AWB_SetAttr(0, &data);

}

void ST_DoAWB_MANUEL(void *args)
{
    MI_ISP_AWB_ATTR_TYPE_t data;
    MI_ISP_AWB_GetAttr(0, &data);
    data.eOpType = SS_OP_TYP_MANUAL;
    data.stManualParaAPI.u16Rgain  = 1024;
    data.stManualParaAPI.u16Grgain = 1024;
    data.stManualParaAPI.u16Gbgain = 1024;
    data.stManualParaAPI.u16Bgain  = 1024;
    MI_ISP_AWB_SetAttr(0, &data);

}

MI_BOOL ST_DoCaptureJPGProcExt(MI_U16 u16Width, MI_U16 u16Hight, MI_SYS_Rotate_e enRotation)
{
    ST_VPE_PortInfo_T stVpePortInfo;
//    MI_RGN_ChnPortParam_t stRgnChnPortParam;
//    MI_RGN_HANDLE hRgnHandle;
//    MI_RGN_ChnPort_t stRgnChnPort;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_VENC_ChnAttr_t stChnAttr;
    ST_Sys_BindInfo_T stBindInfo;
    MI_SYS_WindowRect_t stRect;
//    MI_RGN_Attr_t stRgnAttr;
//    MI_RGN_ChnPortParam_t stChnPortParam;
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
/*
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
*/
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
    stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateNum = 30;
    stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateDen = 1;
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
    //ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover2Handle), MI_RGN_OK);
   // ExecFunc(MI_RGN_Destroy(pstStreamAttr[0].u32Cover1Handle), MI_RGN_OK);
/*
    hRgnHandle = RGN_OSD_HANDLE;
    memset(&stRgnChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stRgnChnPort.eModId = E_MI_RGN_MODID_VPE;
    stRgnChnPort.s32DevId = 0;
    stRgnChnPort.s32ChnId = pstStreamAttr[0].u32InputChn;
    stRgnChnPort.s32OutputPortId = pstStreamAttr[0].u32InputPort;
    ExecFunc(MI_RGN_DetachFromChn(hRgnHandle, &stRgnChnPort), MI_RGN_OK);
*/
    STCHECKRESULT(ST_Vpe_StopPort(0, 0));

    return 0;
}

MI_BOOL ST_DoCaptureJPGProc(MI_U16 u16Width, MI_U16 u16Height, MI_SYS_Rotate_e enRotation)
{
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
//    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
//    ST_Sys_BindInfo_T stBindInfo;
    MI_VENC_ChnAttr_t stChnAttr;
//    MI_RGN_ChnPortParam_t stRgnChnPortParam;
//    MI_RGN_ChnPort_t stRgnChnPort;
//    MI_RGN_HANDLE hRgnHandle;
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;
    MI_SYS_WindowRect_t stRect;
//    MI_RGN_Attr_t stRgnAttr;
//    MI_RGN_ChnPortParam_t stChnPortParam;

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
/*
    ExecFunc(MI_DIVP_CreateChn(DIVP_CHN_FOR_OSD, &stDivpChnAttr), MI_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(DIVP_CHN_FOR_OSD), MI_SUCCESS);

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = u32Width;
    stOutputPortAttr.u32Height          = u32Height;

    ST_DBG("u32Width:%d,u32Height:%d\n", u32Width, u32Height);
    //STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DIVP_CHN_FOR_OSD, &stOutputPortAttr));

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
*/
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
    stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
    STCHECKRESULT(ST_Venc_CreateChannel(VENC_CHN_FOR_CAPTURE, &stChnAttr));
 /*   memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
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
*/
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

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        printf("not support cif now\n");
    }
    else
    {
        (void)ST_DoCaptureJPGProc(352, 288, enRotation);
    }
}
void ST_DoVpePort0ResizeTo1080P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;

    pstStreamAttr[1].u32CropWidth =  1920;
    pstStreamAttr[1].u32CropHeight = 1080;

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(1, 1920, ALIGN_UP(1080, 16));
    }
    else
    {
        (void)ST_DoStreamResize(1, 1920, 1080);
    }
}
void ST_DoVpePort0ResizeTo720P(void *args)
{
    ST_Stream_Attr_T *pstStreamAttr = g_stStreamAttr;

    pstStreamAttr[1].u32CropWidth =  1280;
    pstStreamAttr[1].u32CropHeight = 720;

    (void)ST_DoStreamResize(1, 1280, 720);
}

void ST_DoVpePort1ResizeTo4M(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_DOWN(1440, 16), 2304);
    }
    else
    {
        (void)ST_DoStreamResize(0, 2560, 1440);
    }
}

void ST_DoVpePort1ResizeTo3M(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_DOWN(1536, 16), 2304);
    }
    else
    {
        (void)ST_DoStreamResize(0, 2304, 1536);
    }
}

void ST_DoVpePort1ResizeTo1080P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_DOWN(1080, 32), 1920);
    }
    else
    {
        (void)ST_DoStreamResize(0, 1920, 1080);
    }
}
void ST_DoVpePort1ResizeTo720P(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_UP(720, 32), 1280);
    }
    else
    {
        (void)ST_DoStreamResize(0, 1280, 720);
    }
}
void ST_DoVpePort1ResizeToD1(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_UP(576, 32), 720);
    }
    else
    {
        (void)ST_DoStreamResize(0, 720, 576);
    }
}

void ST_DoVpePort1ResizeToCIF(void *args)
{
    MI_SYS_Rotate_e enRotation = (MI_SYS_Rotate_e)((MI_U32)args);

    if (enRotation == E_MI_SYS_ROTATE_90 || enRotation == E_MI_SYS_ROTATE_270)
    {
        (void)ST_DoStreamResize(0, ALIGN_UP(288, 32), 352);
    }
    else
    {
        (void)ST_DoStreamResize(0, 352, 288);
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

void ST_DoCheckBuildInfor(void *args)
{
    printf("Date : %s\n", __DATE__);
    printf("Time : %s\n", __TIME__);

}
void ST_DoExitProc(void *args)
{
    g_bExit = TRUE;
    printf("%s %d,--%d\n", __func__, __LINE__,(MI_S32)args);
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
    MI_U32 u32Cbr = 12;


    memset(szDesc, 0, sizeof(szDesc));
  /*  snprintf(szDesc, sizeof(szDesc) - 1,  "capture %dx%d JPEG(IMI mode)", g_u32CapWidth, g_u32CapHeight);
    ST_RegisterInputCmd(szDesc, ST_DoCaptureIMI1080PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 2560x1440 JPEG", ST_DoCapture4MJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 1920x1080 JPEG", ST_DoCapture1080PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 1280X720 JPEG", ST_DoCapture720PJPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 720x576 JPEG", ST_DoCaptureD1JPGProc, (void *)enRotation);
    ST_RegisterInputCmd("capture 352x288 JPEG", ST_DoCaptureCIFJPGProc, (void *)enRotation);
*/

    //ST_RegisterInputCmd("resize main_stream to 1920x1080", ST_DoMainStreamResizeTo1080P, NULL);
    //ST_RegisterInputCmd("resize main_stream to 1280x720", ST_DoMainStreamResizeTo720P, NULL);
    //vpe port0 not support scaling.
    if (pstStreamAttr[0].bEnable == TRUE)
    {

        ST_RegisterInputCmd("test AE_AUTO", ST_DoAE_AUTO, (void *)enRotation);
        ST_RegisterInputCmd("test AE_MANUEL", ST_DoAE_MANUEL, (void *)enRotation);
        ST_RegisterInputCmd("test AWB_AUTO", ST_DoAWB_AUTO, (void *)enRotation);
        ST_RegisterInputCmd("test AWB_MANUEL", ST_DoAWB_MANUEL, (void *)enRotation);
        ST_RegisterInputCmd("test LoadBinFile(default path: ./test.bin)", ST_DoLoadBinFile, (void *)enRotation);
        ST_RegisterInputCmd("test LoadCaliDataFile(default path: ./cali.data)",	ST_DoLoadCaliDataFile, NULL);
        if(!bCh0NeedPipeLine)
        {
            u32Cbr = 6;
            ST_RegisterInputCmd("test venc0 CBR=6Mbps", ST_DoSetCBR, (void *)u32Cbr);
        }
        else
            ST_RegisterInputCmd("test venc0 CBR=12Mbps", ST_DoSetCBR, (void *)u32Cbr);
        ST_RegisterInputCmd("test venc0 fps", ST_DoSetFPS, NULL);
        ST_RegisterInputCmd("test venc0 res", ST_DoSetRES, NULL);
        ST_RegisterInputCmd("test venc3 QP=90 ",  ST_DoSetQP, NULL);
        ST_RegisterInputCmd("test Capture Bayer RAW",  ST_DoCaptureBayerRAW, NULL);
        ST_RegisterInputCmd("test Capture 1920x1080 YUYU",  ST_DoCaptureIspOutput, NULL);
        ST_RegisterInputCmd("test Capture 1920x1080 NV12",  ST_DoCaptureVpeOutput, NULL);
        ST_RegisterInputCmd("test Capture 1920x1080 JPEG",  ST_DoCaptureJPG, NULL);

        ST_RegisterInputCmd("resize vpe port1 main_stream to 1920x1080", ST_DoVpePort1ResizeTo1080P, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 1280x720", ST_DoVpePort1ResizeTo720P, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 720x576", ST_DoVpePort1ResizeToD1, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port1 main_stream to 352x288", ST_DoVpePort1ResizeToCIF, (void *)enRotation);
    }
	/*
	if (pstStreamAttr[1].bEnable == TRUE)
    {
        ST_RegisterInputCmd("resize vpe port0 sub_stream0 to 1920x1080", ST_DoVpePort0ResizeTo1080P, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port0 sub_stream0 to 1280x720", ST_DoVpePort0ResizeTo720P, (void *)enRotation);
    }
*/
    if (pstStreamAttr[2].bEnable == TRUE)
    {
        ST_RegisterInputCmd("resize vpe port2 sub_stream1 to 720x576", ST_DoVpePort2ResizeToD1, (void *)enRotation);
        ST_RegisterInputCmd("resize vpe port2 sub_stream1 to 352x288", ST_DoVpePort2ResizeToCIF, (void *)enRotation);
    }
    // at the end
    ST_RegisterInputCmd("check build information", ST_DoCheckBuildInfor, NULL);
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
//    MI_S32 s32LastRandNumber = 0;

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

//            s32LastRandNumber = s32RandNumber;
        }
    }
}

int main(int argc, char **argv)
{
    struct sigaction sigAction;

    sigAction.sa_handler = ST_HandleSig;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT,  &sigAction, NULL);  //-2
    sigaction(SIGKILL, &sigAction, NULL);  //-9
    sigaction(SIGTERM, &sigAction, NULL);  //-15

#if 0
	MI_BOOL bInit = FALSE;
	if(0){
	    MI_S32 s32Opt = 0;
	    while ((s32Opt = getopt(argc, argv, "i:o:")) != -1)
	    {
	        switch(s32Opt)
	        {
	            case 'i':
	                bInit = atoi(optarg);
	                printf("rtsp Need to init %d \n", bInit);
	                break;
	            case '?':

	                if(optopt == 'i')
	                    printf("Missing channel number of data, please -i 'will be init~?' \n");
	                else
	                    printf("Invalid option received\n");

	            default:
	                printf("run rtsp: param error.\n");
	                return 1;
	        }
	    }
	}
#endif

#if 1
    ST_ResetArgs(&g_stConfig);
#else
    ST_DefaultArgs(&g_stConfig);
#endif

	if(open("/var/rtsp.inited", O_RDONLY|O_CREAT|O_EXCL, 0444) < 0 || access("/proc/dualos/video_init", F_OK) < 0){ //normal pure linux rtsp or second time bringup
        bCh0NeedPipeLine = TRUE;
		printf("rtsp run:Need to init\n");
        STCHECKRESULT(ST_BaseModuleInit(&g_stConfig));
		STCHECKRESULT(ST_VencStart());
	}
    else
    {
		printf("rtsp run:No Need to init\n");
        bCh0NeedPipeLine = FALSE;
        STCHECKRESULT(MI_SYS_Init());
#ifdef ENABLE_RGN
        STCHECKRESULT(ST_OsdStart());
#endif
#if (ENABLE_BUF_POOL == 1)
        STCHECKRESULT(ST_InitBufPoolEnv());
#endif		
		STCHECKRESULT(ST_VencStart());
     	MI_VENC_DupChn(0);
        MI_AI_DupChn(0,0);
        ST_Thread_Start(&pthreadHandlearray[1], "YUV_Preoload", ST_DIVPPreoloadGetResult);
        ST_Thread_Start(&pthreadHandlearray[2], "AI_Preoload", ST_AIPreoloadGetResult);
        ST_Thread_Start(&pthreadHandlearray[3], "venc_Preoload", ST_VencPreoloadGetResult);
    }

    MI_VPE_Alloc_IspDataBuf(100*1024, &g_pIspBuffer);

	ST_Thread_Start(&pthreadHandlearray[0], SUB_STREAM3, ST_DIVPGetResult);

    ST_RtspServerStart();

    MI_IQSERVER_Open(1920, 1080, 0);

    ST_InitInputCmd(g_stConfig.enRotation);
/*
    if(g_stConfig.s32UseVdf != 0)
    {
        ST_ModuleInit_VDF();
        ST_VdfStart();
    }
*/
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

    ST_Thread_Stop(&pthreadHandlearray[0], SUB_STREAM3);
    if(!bCh0NeedPipeLine)
    {
        ST_Thread_Stop(&pthreadHandlearray[1], "YUV_Preoload");
        ST_Thread_Stop(&pthreadHandlearray[2], "AI_Preoload");
        ST_Thread_Stop(&pthreadHandlearray[3], "venc_Preoload");
        MI_AI_DisableChn(0,0);
        MI_AI_Disable(0);
    }

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


    MI_VPE_Free_IspDataBuf(g_pIspBuffer);
#if (ENABLE_BUF_POOL == 1)
	STCHECKRESULT(ST_TermBufPool());
#endif
    ST_RtspServerStop();
    MI_IQSERVER_Close();

    STCHECKRESULT(ST_VencStop());

    STCHECKRESULT(ST_BaseModuleUnInit());

    ST_DestoryInputCmd();

    return 0;
}

