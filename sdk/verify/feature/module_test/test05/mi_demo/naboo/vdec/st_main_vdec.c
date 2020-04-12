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
#include <pthread.h>
#include <sys/prctl.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp.h"
//#include "mi_vdisp.h"

#include "mi_rgn.h"

#include "st_hdmi.h"
#include "st_common.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_fb.h"
#include "st_divp.h"

typedef struct
{
    pthread_t pt;
    pthread_t ptsnap;
    MI_VDEC_CHN vdecChn;
    char szFileName[64];
    MI_BOOL bRunFlag;
} VDEC_Thread_Args_t;

static MI_BOOL g_bExit = FALSE;
static MI_BOOL g_subExit = FALSE;
static MI_BOOL g_PushEsExit = FALSE;

static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32SubCaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;
VDEC_Thread_Args_t g_stVdecThreadArgs[MAX_CHN_NUM];

static pthread_t g_vdisp_pt;
static MI_BOOL g_bGetBuf = FALSE;
static pthread_t g_writeFilePt;

#define ADD_HEADER_ES

#define USE_DISP_VGA 0

#if 0
typedef struct
{
    MI_U32 u32CaseIndex;            // case index
    MI_U32 u32Chn;                  // vdec chn
    MI_VDEC_CodecType_e eCodecType; // vdec caode type
    char szFilePath[64];            // h26x file path
    MI_U32 u32MaxWidth;             // max of width
    MI_U32 u32MaxHeight;            // max of height
} ST_VdecChnArgs_t;
#endif
ST_CaseDesc_t g_stCaseDesc[] =
{
    {
        .stDesc =
        {
            .u32CaseIndex = 0,
            .szDesc = "1x1080P@30 H264 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 9,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P@50 timing",
                .eDispoutTiming = E_ST_TIMING_720P_50,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 720P@60 timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 1080P@50 timing",
                .eDispoutTiming = E_ST_TIMING_1080P_50,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P@60 timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1600x1200 timing",
                .eDispoutTiming = E_ST_TIMING_1600x1200_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 10,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 1,
            .szDesc = "1x1080P@30 H265 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 2,
            .szDesc = "1xD1 H264 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 3,
            .szDesc = "1xD1 H265 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 4,
            .szDesc = "4x720P@30 H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 9,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1600x1200 timing",
                .eDispoutTiming = E_ST_TIMING_1600x1200_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 5,
            .szDesc = "4x720P@30 H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 6,
            .szDesc = "4x1080P@30 H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 7,
            .szDesc = "4x1080P@30 H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 8,
            .szDesc = "2x1080P@30 H264 + 2x1080P@30 H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 9,
            .szDesc = "4xD1 H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 10,
            .szDesc = "4xD1 H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 11,
            .szDesc = "4x4K@15 H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4K_H264_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4K_H264_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4K_H264_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4K_H264_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 12,
            .szDesc = "4x4K@15 H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4K_H265_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4K_H265_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4K_H265_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4K_H265_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 13,
            .szDesc = "1x1080P30 H264 + 7xD1 H264 Decode",
            .u32WndNum = 8,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 7,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 8 channel",
                .u32WndNum = 8,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 14,
            .szDesc = "1x1080P30 H265 + 7xD1 H265 Decode",
            .u32WndNum = 8,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 7,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 8 channel",
                .u32WndNum = 8,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 15,
            .szDesc = "1x1080P30 H264 + 7xD1 H265 Decode",
            .u32WndNum = 8,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 7,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 8 channel",
                .u32WndNum = 8,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 16,
            .szDesc = "9xD1 H264 Decode",
            .u32WndNum = 9,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 17,
            .szDesc = "9xD1 H265 Decode",
            .u32WndNum = 9,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 7,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            }
        },
    },
#if 0
    {
        .stDesc =
        {
            .u32CaseIndex = 18,
            .szDesc = "16xD1 H264 Decode",
            .u32WndNum = 16,
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 19,
            .szDesc = "16xD1 H265 Decode",
            .u32WndNum = 16,
        },
    },
#endif
    {
        .stDesc =
        {
            .u32CaseIndex = 18,
            .szDesc = "1x1080P@30 H264 Decode + PIP",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .s32SplitMode = E_ST_SPLIT_MODE_TWO,
        .u32SubCaseNum = 9,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1600x1200 timing",
                .eDispoutTiming = E_ST_TIMING_1600x1200_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 19,
            .szDesc = "1x1080P@30 H264 Decode + 5 x D1 H264 Decode",
            .u32WndNum = 6,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 20,
            .szDesc = "1x720P@30 H264 Decode + 5 x D1 H264 Decode",
            .u32WndNum = 6,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_25_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 21,
            .szDesc = "1x1080P@25 H264 Decode + 5xVGA(640x480) H264 Decode",
            .u32WndNum = 6,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_VGA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 480,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_VGA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 480,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_VGA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 480,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_VGA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 480,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_VGA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 480,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 22,
            .szDesc = "16xD1 H264 Decode",
            .u32WndNum = 16,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 9,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 23,
            .szDesc = "25xD1 H264 Decode",
            .u32WndNum = 25,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "show 16 channel",
                .u32WndNum = 25,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 16,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 17,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 18,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 19,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 20,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 21,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 22,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 23,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 24,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 24,
            .szDesc = "16xD1 H265 Decode",
            .u32WndNum = 16,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 9,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 25,
            .szDesc = "25xD1 H265 Decode",
            .u32WndNum = 25,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "show 16 channel",
                .u32WndNum = 25,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 16,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 17,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 18,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 19,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 20,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 21,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 22,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 23,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 24,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 26,
            .szDesc = "4xSIZE(640x360) H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_AA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 360,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_AA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 360,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_AA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 360,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_AA_H264_25_FILE,
                        .u32MaxWidth = 640,
                        .u32MaxHeight = 360,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 27,
            .szDesc = "1x720P H264 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_25_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 28,
            .szDesc = "1x720P H265 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_25_FILE,
                        .u32MaxWidth = 1280,
                        .u32MaxHeight = 720,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 29,
            .szDesc = "4x1080P H264 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 30,
            .szDesc = "4x1080P H265 Decode",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 31,
            .szDesc = "8x1080P H264 Decode",
            .u32WndNum = 8,
        },
        .eDispoutTiming = E_ST_TIMING_3840x2160_30,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 32,
            .szDesc = "8x1080P H265 Decode",
            .u32WndNum = 8,
        },
        .eDispoutTiming = E_ST_TIMING_3840x2160_30,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 33,
            .szDesc = "1x8MP(3840x2160) H264 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_8MP_H264_25_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 34,
            .szDesc = "1x8MP(3840x2160) H265 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_8MP_H265_25_FILE,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 35,
            .szDesc = "1x4MP(2560x1440) H264 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4MP_H264_25_FILE,
                        .u32MaxWidth = 2560,
                        .u32MaxHeight = 1440,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 36,
            .szDesc = "1x4MP(2560x1440) H265 Decode",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 6,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4MP_H265_25_FILE,
                        .u32MaxWidth = 2560,
                        .u32MaxHeight = 1440,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 37,
            .szDesc = "1x1080P Baseline Jpd",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_1600_1200_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 38,
            .szDesc = "4x320*240 Progssive Jpd",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_320_240_P_FILE,
                        .u32MaxWidth = 320,
                        .u32MaxHeight = 240,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_320_240_P_FILE,
                        .u32MaxWidth = 320,
                        .u32MaxHeight = 240,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_320_240_P_FILE,
                        .u32MaxWidth = 320,
                        .u32MaxHeight = 240,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_320_240_P_FILE,
                        .u32MaxWidth = 320,
                        .u32MaxHeight = 240,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 39,
            .szDesc = "1x320*240 Progssive Jpd",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_320_240_P_FILE,
                        .u32MaxWidth = 320,
                        .u32MaxHeight = 240,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 40,
            .szDesc = "1x1024*768 Progssive Jpd",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_1024_768_P_FILE,
                        .u32MaxWidth = 1024,
                        .u32MaxHeight = 768,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 41,
            .szDesc = "1x1080P Progssive Jpd",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG,
                        .szFilePath = PREFIX_PATH ST_MJPEG_1080P_P_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 42,
            .szDesc = "32xD1 H264 Decode",
            .u32WndNum = 32,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "show 32 channel",
                .u32WndNum = 32,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 16,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 17,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 18,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 19,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 20,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 21,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 22,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 23,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 24,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 25,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 26,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 27,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 28,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 29,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 30,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 31,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 43,
            .szDesc = "32xD1 H265 Decode",
            .u32WndNum = 32,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "show 32 channel",
                .u32WndNum = 32,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 16,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 17,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 18,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 19,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 20,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 21,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 22,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 23,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 24,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 25,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 26,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 27,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 28,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 29,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 30,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 31,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 44,
            .szDesc = "1 x 4K@30 H264 Decode (corridor)",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_4K_H264_CORRIDOR_FILE,
                        .u32MaxWidth = 2160,
                        .u32MaxHeight = 3840,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 45,
            .szDesc = "3xD1@30 H264 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 46,
            .szDesc = "3x720P@30 H264 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_720P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 47,
            .szDesc = "3x1080P@30 H264 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_1080P_H264_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 48,
            .szDesc = "1 x 4K@30 H265 Decode (corridor)",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_4K_H265_CORRIDOR_FILE,
                        .u32MaxWidth = 2160,
                        .u32MaxHeight = 3840,
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 49,
            .szDesc = "3xD1@30 H265 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_D1_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 576,
                        .u32MaxHeight = 720,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 50,
            .szDesc = "3x720P@30 H265 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_720P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 1280,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 51,
            .szDesc = "3x1080P@30 H265 Decode (corridor)",
            .u32WndNum = 3,
        },
        .eDispoutTiming = E_ST_TIMING_1280x1024_60,
        .u32SubCaseNum = 2,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
           	{
           	    .u32CaseIndex = 1,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_30_CORRIDOR_FILE,
                        .u32MaxWidth = 1080,
                        .u32MaxHeight = 1920,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 52,
            .szDesc = "36window 32xD1 H264 Decode",
            .u32WndNum = 32,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 10,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "show 1 channel",
                .u32WndNum = 1,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "show 4 channel",
                .u32WndNum = 4,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "show 9 channel",
                .u32WndNum = 9,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "show 16 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "show 32 channel",
                .u32WndNum = 32,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 8,
                .szDesc = "test region",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 9,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 4,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 5,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 6,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 7,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 8,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 9,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 10,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 11,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 12,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 13,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 14,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 15,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 16,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 17,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 18,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 19,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 20,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 21,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 22,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 23,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 24,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 25,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 26,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 27,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 28,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 29,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 30,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 31,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = PREFIX_PATH ST_D1_H264_25_FILE,
                        .u32MaxWidth = 720,
                        .u32MaxHeight = 576,
                    },
                }
            },
        },
    }
};

//==============================================================================
FILE *g_pStreamFile[32] = {NULL};

typedef struct
{
    int startcodeprefix_len;
    unsigned int len;
    unsigned int max_size;
    char *buf;
    unsigned short lost_packets;
} NALU_t;

static int info2 = 0, info3 = 0;

static int FindStartCode2 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 1))
        return 0;
    else
        return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0) || (Buf[3] != 1))
        return 0;
    else
        return 1;
}

NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n;
    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    {
        printf("AllocNALU: n");
        exit(0);
    }
    n->max_size=buffersize;
    if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
    {
        free (n);
        printf ("AllocNALU: n->buf");
        exit(0);
    }
    return n;
}

void FreeNALU(NALU_t *n)
{
    if (n)
    {
        if (n->buf)
        {
            free(n->buf);
            n->buf=NULL;
        }
        free (n);
    }
}

int GetAnnexbNALU (NALU_t *nalu, MI_S32 chn)
{
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
        printf ("GetAnnexbNALU: Could not allocate Buf memory\n");
    nalu->startcodeprefix_len=3;
    if (3 != fread (Buf, 1, 3, g_pStreamFile[chn]))
    {
        free(Buf);
        return 0;
    }
    info2 = FindStartCode2 (Buf);
    if(info2 != 1)
    {
        if(1 != fread(Buf+3, 1, 1, g_pStreamFile[chn]))
        {
            free(Buf);
            return 0;
        }
        info3 = FindStartCode3 (Buf);
        if (info3 != 1)
        {
            free(Buf);
            return -1;
        }
        else
        {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    }
    else
    {
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;
    while (!StartCodeFound)
    {
        if (feof (g_pStreamFile[chn]))
        {
            nalu->len = (pos-1)-nalu->startcodeprefix_len;
            memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            free(Buf);
            fseek(g_pStreamFile[chn], 0, 0);
            return pos-1;
        }
        Buf[pos++] = fgetc (g_pStreamFile[chn]);
        info3 = FindStartCode3(&Buf[pos-4]);
        if(info3 != 1)
            info2 = FindStartCode2(&Buf[pos-3]);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    rewind = (info3 == 1) ? -4 : -3;
    if (0 != fseek (g_pStreamFile[chn], rewind, SEEK_CUR))
    {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }
    nalu->len = (pos+rewind);
    memcpy (nalu->buf, &Buf[0], nalu->len);
    free(Buf);
    return (pos+rewind);
}

void dump(NALU_t *n)
{
    if (!n)
        return;
    //printf(" len: %d  ", n->len);
    //printf("nal_unit_type: %x\n", n->nal_unit_type);
}

void ST_VdecUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 i = 0;

    for (i = 0; i < u32Size; i ++)
    {
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
    }

    printf("print twice Enter to exit\n");
}

void ST_CaseUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 i = 0;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    for (i = 0; i < pstCaseDesc[u32CaseIndex].u32SubCaseNum; i ++)
    {
        printf("\t%d) %s\n", pstCaseDesc[u32CaseIndex].stSubDesc[i].u32CaseIndex + 1,
            pstCaseDesc[u32CaseIndex].stSubDesc[i].szDesc);
    }
}

void *ST_SnapProcess(void *args)
{
    VDEC_Thread_Args_t *pstArgs = (VDEC_Thread_Args_t *)args;
    MI_SYS_ChnPort_t stChnPort;
    char szFileName[64];
    int count = 0;
    FILE *fp = NULL;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));

    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = pstArgs->vdecChn;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 5, 20);

    printf("%s %d, vdecChn:%d\n", __func__, __LINE__, pstArgs->vdecChn);

    while (1)
    {
        usleep(1000*1000);

        memset(szFileName, 0, sizeof(szFileName));
        sprintf(szFileName, "/mnt/mjpeg/vdec_%d.yuv", count ++);

        memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        //if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        //{
        //    continue;
        //}

#if 0
        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_META)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }
        else
        {
            printf("Chn(%d), eDataFromModule:%d, phyAddr:0x%llx, pVirAddr:0x%p, u32Size:%d\n",
                pstArgs->vdecChn,
                stBufInfo.stMetaData.eDataFromModule,
                stBufInfo.stMetaData.phyAddr,
                stBufInfo.stMetaData.pVirAddr,
                stBufInfo.stMetaData.u32Size);

            if (fp)
                fwrite(stBufInfo.stMetaData.pVirAddr, 1, stBufInfo.stMetaData.u32Size, fp);
            fclose(fp);
            fp = NULL;
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
#endif
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort ,&stBufInfo ,&hHandle))
        {
            fp = fopen(szFileName, "w+");
            if (fp == NULL)
            {
                printf("open %s fail\n", szFileName);
                continue;
            }
            printf("open %s success\n", szFileName);
            MI_U32 u32BufSize = stBufInfo.stFrameData.u16Width * stBufInfo.stFrameData.u16Height * 2;
            //get buffer virtual address
            void *pvirFramAddr = NULL;

            //fix me:
            //chnOutput buf already map VA.
            //Why need to remap here ???
            if(MI_SYS_Mmap(stBufInfo.stFrameData.phyAddr[0], u32BufSize, &pvirFramAddr, TRUE))
            {
                printf(" MI_SYS_Mmap failed!.\n");
            }
            else
            {
                printf("pvirFramAddr = %p. \n", pvirFramAddr);
            }
            stBufInfo.stFrameData.pVirAddr[0] = pvirFramAddr;
            printf("get buf success w%d h%d.\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
            if (fp)
                fwrite(stBufInfo.stFrameData.pVirAddr[0], 1, u32BufSize, fp);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
    }

    return NULL;
}

void *ST_VdecGetVdispOutputBuf(void *args)
{
    MI_SYS_ChnPort_t stVdispOutputPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    ST_DispoutTiming_e eCurDispoutTiming = E_ST_TIMING_MAX;
    MI_SYS_BUF_HANDLE bufhandle;
    MI_S32 frameCount = 5;
    MI_U32 u32Size = 0;
    char szFileName[128];
    FILE *pFile = NULL;
    int i = 0;

    eCurDispoutTiming = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].eDispoutTiming;

    stVdispOutputPort.eModId = E_MI_MODULE_ID_DIVP;//E_MI_MODULE_ID_VDISP;
    stVdispOutputPort.u32DevId = 0;
    stVdispOutputPort.u32ChnId = 0;
    stVdispOutputPort.u32PortId = 0;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u32Flags = 0;
    stBufConf.u64TargetPts = 0;
    stBufConf.stFrameCfg.eFormat=E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = 1280;
    stBufConf.stFrameCfg.u16Height = 720;

    while (1)
    {
        if (g_bGetBuf == FALSE)
        {
            sleep(1);
            continue;
        }

        sleep(10);

        if (frameCount <= 0)
        {
            printf("%s %d, stop get buf\n", __func__, __LINE__);
            g_bGetBuf = FALSE;
        }

        if (frameCount == 5)
        {
            MI_SYS_SetChnOutputPortDepth(&stVdispOutputPort,3,5);
        }

        printf("%s %d, get buf count:%d\n", __func__, __LINE__, frameCount);
        memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stVdispOutputPort, &stBufInfo, &bufhandle))
        {
            continue;
        }

        //
        memset(szFileName, 0, sizeof(szFileName));
        snprintf(szFileName, sizeof(szFileName) - 1, "divp_%dx%d_%d.yuv", stBufInfo.stFrameData.u16Width,
            stBufInfo.stFrameData.u16Height, frameCount);

        pFile = fopen(szFileName, "wb");
        u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u16Width * 2;//stBufInfo.stFrameData.u32Stride[0];
        if (pFile)
        {
            for (i = 0; i < stBufInfo.stFrameData.u16Height; i ++)
            {
                fwrite(stBufInfo.stFrameData.pVirAddr[0] + stBufInfo.stFrameData.u32Stride[0] * i,
                    stBufInfo.stFrameData.u16Width * 2, 1, pFile);
            }

            fclose(pFile);
            pFile = NULL;
        }

        //no need flushcache here.
        //MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
        printf("%s %d, ePixelFormat:%d, u16Height:%d,u32Stride:%d,u32Size:%d\n",
            __func__, __LINE__, stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.u16Height,stBufInfo.stFrameData.u32Stride[0],
            u32Size);
        frameCount --;
        MI_SYS_ChnOutputPortPutBuf(bufhandle);

        // sleep(1);
    }
}

int ST_DumpFile(char *szSrcFile, char *szDstFile)
{
    int srcFd = -1, dstFd = -1;
    struct stat sbuf;
	unsigned char *ptr = NULL;
    int ret = 0;

    srcFd = open (szSrcFile, O_RDONLY|0);
    if (srcFd == -1)
    {
        // printf("%s %d, open %s error\n", __func__, __LINE__, szSrcFile);
        ret = -1;
        goto END;
    }

    if ((dstFd = open(szDstFile, O_WRONLY|O_CREAT)) < 0)
    {
        // printf("%s %d, open %s error\n", __func__, __LINE__, szDstFile);
        ret = -1;
        goto END;
    }

    if (fstat(srcFd, &sbuf) < 0)
    {
        // printf("%s %d, fstat error\n", __func__, __LINE__);
        ret = -1;
		goto END;
	}

    ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, srcFd, 0);
	if (ptr == MAP_FAILED)
    {
        // printf("%s %d, mmap error\n", __func__, __LINE__);
        ret = -1;
	    goto END;
	}

    if (write(dstFd, ptr, sbuf.st_size) != sbuf.st_size)
    {
        // printf("%s %d, write error, real:%d\n", __func__, __LINE__, sbuf.st_size);
    }

END:
    if (ptr)
        (void) munmap((void *)ptr, sbuf.st_size);

    if (srcFd > 0)
        close(srcFd);

    if (dstFd > 0)
        close(dstFd);

    return -1;
}

void *ST_VdecWriteFileToDisk(void *args)
{
    int iFileIndex = 0;
    char szFileName[64];
    char szSrcFileName[] = "/mnt/1080P25.h264";

    while (!g_bExit)
    {
        memset(szFileName, 0, sizeof(szFileName));
        snprintf(szFileName, sizeof(szFileName) - 1, "/disk/test_%d.yuv", iFileIndex ++);

        ST_DumpFile(szSrcFileName, szFileName);

        usleep(10 * 1000);
    }
}

void *ST_VdecSendStream(void *args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32TimeOutMs = 20, s32ChannelId = 0, s32TempHeight = 0;
    MI_S32 s32Ms = 30;
    MI_BOOL bVdecChnEnable;
    MI_U16 u16Times = 20000;

    MI_S32 s32ReadCnt = 0;
    FILE *readfp = NULL;
    MI_U8 *pu8Buf = NULL;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[32] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_VideoStream_t stVdecStream;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    VDEC_Thread_Args_t *pstArgs = (VDEC_Thread_Args_t *)args;

    char tname[32];
    memset(tname, 0, 32);

#ifndef ADD_HEADER_ES
    NALU_t *n;
    n = AllocNALU(2000000);
#endif

    vdecChn = pstArgs->vdecChn;
    snprintf(tname, 32, "push_t_%u", vdecChn);
    prctl(PR_SET_NAME, tname);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;//0 1 2 3
    stChnPort.u32PortId = 0;

    readfp = fopen(pstArgs->szFileName, "rb"); //ES
    if (!readfp)
    {
        ST_ERR("Open %s failed!\n", pstArgs->szFileName);
        return NULL;
    }
    else
    {
        g_pStreamFile[vdecChn] = readfp;
    }

    printf("open %s success, vdec chn:%d\n", pstArgs->szFileName, vdecChn);
    // s32Ms = _stTestParam.stChannelInfo[s32VoChannel].s32PushDataMs;
    // bVdecChnEnable = _stTestParam.stChannelInfo[0].bVdecChnEnable;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = malloc(NALU_PACKET_SIZE);

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth error, %X\n", s32Ret);
        return NULL;
    }

    //stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    //STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5));

    s32Ms = 50;
    //printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (!g_PushEsExit)
    {
        usleep(s32Ms * 1000);//33
        if (pstArgs->bRunFlag == FALSE)
        {
            continue;
        }

#ifdef ADD_HEADER_ES
        memset(au8Header, 0, 16);
        u32Pos = fseek(readfp, 0, SEEK_CUR);
        s32Len = fread(au8Header, 1, 16, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }

        u32FrameLen = MI_U32VALUE(au8Header, 4);
        // printf("vdecChn:%d, u32FrameLen:%d, %d\n", vdecChn, u32FrameLen, NALU_PACKET_SIZE);
        if(u32FrameLen > NALU_PACKET_SIZE)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }
        s32Len = fread(pu8Buf, 1, u32FrameLen, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }

        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = s32Len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len + 16; //back length
#else
        GetAnnexbNALU(n, vdecChn);
        dump(n);
        stVdecStream.pu8Addr = (MI_U8 *)n->buf;
        stVdecStream.u32Len = n->len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len; //back length
#endif
        if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(vdecChn, &stVdecStream, s32TimeOutMs)))
        {
            ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(readfp, - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + ST_Sys_GetPts(30);
        //memset(&stChnStat, 0x0, sizeof(stChnStat));
        //MI_VDEC_GetChnStat(s32VoChannel, &stChnStat);

        if (0 == (s32ReadCnt++ % 30))
            ;// printf("vdec(%d) push buf cnt (%d)...\n", s32VoChannel, s32ReadCnt)
            ;//printf("###### ==> Chn(%d) push frame(%d) Frame Dec:%d  Len:%d\n", s32VoChannel, s32ReadCnt, stChnStat.u32DecodeStreamFrames, u32Len);
    }
    printf("\n\n");
    usleep(300000);
    free(pu8Buf);

    printf("End----------------------%d------------------End\n", stChnPort.u32ChnId);

    return NULL;
}

#if 0
void *ST_VdecSendStream(void *args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32TimeOutMs = 20, s32ChannelId = 0, s32TempHeight = 0;
    MI_S32 s32Ms = 30;
    MI_BOOL bVdecChnEnable;
    MI_U16 u16Times = 20000;

    MI_S32 readfd = 0, s32ReadCnt = 0;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_ChnStat_t stVdecStat;

    VDEC_Thread_Args_t *pstArgs = (VDEC_Thread_Args_t *)args;

    char tname[32];
    memset(tname, 0, 32);

    vdecChn = pstArgs->vdecChn;
    snprintf(tname, 32, "push_t_%u", vdecChn);
    prctl(PR_SET_NAME, tname);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;//0 1 2 3
    stChnPort.u32PortId = 0;

    readfd = open(pstArgs->szFileName,  O_RDONLY, 0); //ES
    if (0 > readfd)
    {
        printf("Open %s failed!\n", pstArgs->szFileName);
        return NULL;
    }

    // s32Ms = _stTestParam.stChannelInfo[s32VoChannel].s32PushDataMs;
    // bVdecChnEnable = _stTestParam.stChannelInfo[0].bVdecChnEnable;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5));

    //stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    //STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5));

    s32Ms = 40;
    printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (!g_PushEsExit)
    {
        usleep(s32Ms * 1000);//33

        if (pstArgs->bRunFlag == FALSE)
        {
            continue;
        }

        memset(au8Header, 0, 16);
        s32Len = read(readfd, au8Header, 16);
        if(s32Len <= 0)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }

        u32FrameLen = MI_U32VALUE(au8Header, 4);
        // printf("vdecChn:%d, u32FrameLen:%d\n", vdecChn, u32FrameLen);
        if(u32FrameLen > NALU_PACKET_SIZE)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }

        memset(&stVdecStat, 0, sizeof(MI_VDEC_ChnStat_t));
        if (MI_SUCCESS == (s32Ret = MI_VDEC_GetChnStat(vdecChn, &stVdecStat)))
        {
            if (stVdecStat.u32LeftStreamFrames > 3)
                printf("vdecChn:%d,u32RecvStreamFrames:%d,u32LeftStreamBytes:%d,u32LeftStreamFrames:%d\n", vdecChn,
                    stVdecStat.u32RecvStreamFrames, stVdecStat.u32LeftStreamBytes, stVdecStat.u32LeftStreamFrames);
        }
        else
        {
            printf("vdec:%d, MI_VDEC_GetChnStat Error, 0x%X\n", vdecChn, s32Ret);
        }
        stChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stChnPort.u32DevId = 0;
        stChnPort.u32ChnId = vdecChn;
        stChnPort.u32PortId = 0;

        stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufConf.u64TargetPts = MI_SYS_INVALID_PTS;
        stBufConf.stRawCfg.u32Size = u32FrameLen;

        s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf, s32TimeOutMs);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_ChnInputPortGetBuf error, 0x%X\n", __func__, __LINE__, s32Ret);
            return NULL;
        }
VDEC DESTORY UNBIND
        if (stBufInfo.stRawData.u32BufSize < stBufConf.stRawCfg.u32Size)
        {
            MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, TRUE);
            printf("Get Buffer Error, Size Error\n");
            return NULL;
        }

        s32Len = read(readfd, (void *)stBufInfo.stRawData.pVirAddr, u32FrameLen);
        if(s32Len <= 0)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }

        stBufInfo.u64Pts = u64Pts;
        stBufInfo.stRawData.u32ContentSize = u32FrameLen;
        MI_SYS_FlushInvCache(stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize);
        s32Ret = MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, FALSE);
        u64Pts = u64Pts + ST_Sys_GetPts(30);
        //memset(&stChnStat, 0x0, sizeof(stChnStat));
        //MI_VDEC_GetChnStat(s32VoChannel, &stChnStat);

        if (0 == (s32ReadCnt++ % 30))
            ;// printf("vdec(%d) push buf cnt (%d)...\n", s32VoChannel, s32ReadCnt)
            ;//printf("###### ==> Chn(%d) push frame(%d) Frame Dec:%d  Len:%d\n", s32VoChannel, s32ReadCnt, stChnStat.u32DecodeStreamFrames, u32Len);
    }
    printf("\n\n");
    usleep(300000);
    close(readfd);

    printf("End----------------------%d------------------End\n", stChnPort.u32ChnId);

    return NULL;
}
#endif

int ST_TestRegion()
{
    pid_t fPid;

    fPid = fork();
    if (fPid < 0)
    {
        printf("error in fork!");
    }
    else if(fPid == 0)
    {
        if(execl("mdb", NULL, NULL) < 0)
        {
            perror("execlp error!");
            return -1 ;
        }
    }
    waitpid(fPid, NULL, 0);

    return 0;
}

int ST_ChangeDisplayTiming()
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    ST_DispoutTiming_e eLastDispoutTiming = E_ST_TIMING_MAX;
    ST_DispoutTiming_e eCurDispoutTiming = E_ST_TIMING_MAX;
    MI_S32 s32LastHdmiTiming, s32CurHdmiTiming;
    MI_S32 s32LastDispTiming, s32CurDispTiming;
    MI_U16 u16LastDispWidth = 0, u16LastDispHeight = 0;
    MI_U16 u16CurDispWidth = 0, u16CurDispHeight = 0;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32TotalWnd = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 0;
    ST_Sys_BindInfo_t stBindInfo;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_HDMI_Attr_t stHdmiAttr;
    MI_DISP_PubAttr_t stPubAttr;

    memset(&stHdmiAttr, 0, sizeof(MI_HDMI_Attr_t));
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    memset(&stPubAttr, 0, sizeof(stPubAttr));

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    eCurDispoutTiming = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].eDispoutTiming;
    eLastDispoutTiming = pstCaseDesc[u32CaseIndex].eDispoutTiming;

    if (eCurDispoutTiming == eLastDispoutTiming)
    {
        printf("the same timing, skip\n");
        return 0;
    }

    u32CurWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    STCHECKRESULT(ST_GetTimingInfo(eCurDispoutTiming,
                &s32CurHdmiTiming, &s32CurDispTiming, &u16CurDispWidth, &u16CurDispHeight));

    STCHECKRESULT(ST_GetTimingInfo(eLastDispoutTiming,
                &s32LastHdmiTiming, &s32LastDispTiming, &u16LastDispWidth, &u16LastDispHeight));

    printf("change from %dx%d to %dx%d\n", u16LastDispWidth, u16LastDispHeight, u16CurDispWidth,
                u16CurDispHeight);

    /*
    (1) stop HDMI
    (2) stop DISP and disable input port
    (3) start disp
    (4) start HDMI
    (5) set disp input port attribute and enable
    */

    // stop hdmi
            //MI_HDMI_Close(E_MI_HDMI_ID_0);
    ExecFunc(MI_HDMI_Stop(E_MI_HDMI_ID_0), MI_SUCCESS);

    // stop disp dev and disable input port
    //STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, ST_DISP_LAYER0, u32CurWndNum));
    ExecFunc(MI_DISP_Disable(ST_DISP_DEV0), MI_SUCCESS);

    stPubAttr.u32BgColor = YUYV_BLACK;
    stPubAttr.eIntfSync = s32CurDispTiming;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    ExecFunc(MI_DISP_SetPubAttr(ST_DISP_DEV0, &stPubAttr), MI_SUCCESS);

    ExecFunc(MI_DISP_GetVideoLayerAttr(ST_DISP_LAYER0, &stLayerAttr), MI_SUCCESS);
    stLayerAttr.stVidLayerDispWin.u16Width = u16CurDispWidth;
    stLayerAttr.stVidLayerDispWin.u16Height = u16CurDispHeight;
    ExecFunc(MI_DISP_SetVideoLayerAttr(ST_DISP_LAYER0, &stLayerAttr), MI_SUCCESS);
    ExecFunc(MI_DISP_Enable(ST_DISP_LAYER0), MI_SUCCESS);

    // start disp
    // STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32CurDispTiming));

#if USE_DISP_VGA
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV1, ST_DISP_LAYER1, s32CurDispTiming));
#endif
    // start HDMI
    //MI_HDMI_GetAttr(E_MI_HDMI_ID_0,&stHdmiAttr);
    //stHdmiAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_720_60P;
    //MI_HDMI_SetAttr(E_MI_HDMI_ID_0,&stHdmiAttr);
    //ExecFunc(MI_HDMI_Start(E_MI_HDMI_ID_0), MI_SUCCESS);
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32CurHdmiTiming));

    /*if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }
    else if (u32CurWndNum <= 25)
    {
        u32Square = 5;
    }
    else if (u32CurWndNum <= 36)
    {
        u32Square = 6;
    }


    // set divp outport attribute
    MI_DIVP_CHN divpChn = 0;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;

    for (i = 0; i < u32CurWndNum; i++)
    {
        divpChn = i;
        memset(&stOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));

        STCHECKRESULT(MI_DIVP_GetOutputPortAttr(divpChn, &stOutputPortAttr));
        stOutputPortAttr.u32Width       = ALIGN_BACK(u16CurDispWidth / u32Square, 16);
        stOutputPortAttr.u32Height      = ALIGN_BACK(u16CurDispHeight / u32Square, 2);
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }

    // set disp attribute
    ST_DispChnInfo_t stDispChnInfo;

    memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));
    stDispChnInfo.InputPortNum = u32CurWndNum;
    for (i = 0; i < u32CurWndNum; i++)
    {
        stDispChnInfo.stInputPortAttr[i].u32Port = i;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X =
            ALIGN_BACK((u16CurDispWidth / u32Square) * (i % u32Square), 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y =
            ALIGN_BACK((u16CurDispHeight / u32Square) * (i / u32Square), 2);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width =
            ALIGN_BACK(u16CurDispWidth / u32Square, 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height =
            ALIGN_BACK(u16CurDispHeight / u32Square, 2);
    }
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER0, &stDispChnInfo));
*/
    pstCaseDesc[u32CaseIndex].eDispoutTiming = eCurDispoutTiming;

    // g_bGetBuf = TRUE;
    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;
}

int ST_SplitWindow()
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32LastWndNum = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 1;
    MI_U16 u16DispWidth = 0, u16DispHeight = 0;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    ST_Sys_BindInfo_t stBindInfo;
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stVpeRect;

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    /*
    VDEC->DIVP->DISP

    (1) stop send stream
    (2) unbind VDEC DIVP
    (3) stop divp, disable disp
    (4) set disp port attribute
    (5) start divp, enable disp
    (6) bind vdec divp
    (7) start send stream
    */

    u32CurWndNum = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    u32LastWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    if (u32CurWndNum == u32LastWndNum)
    {
        printf("same wnd num, skip\n");
        return 0;
    }
    else
    {
        printf("split window from %d to %d\n", u32LastWndNum, u32CurWndNum);
    }

    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[u32CaseIndex].eDispoutTiming,
                &s32HdmiTiming, &s32DispTiming, &u16DispWidth, &u16DispHeight));

    printf("%s %d, u16DispWidth:%d,u16DispHeight:%d\n", __func__, __LINE__, u16DispWidth,
        u16DispHeight);

    // stop send es stream to vdec
    for (i = 0; i < u32LastWndNum; i++)
    {
        g_stVdecThreadArgs[i].bRunFlag = FALSE;
    }

    // 1, unbind VDEC to DIVP
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

        printf("%s %d vdec(%d)->divp(%d) unbind\n", __func__, __LINE__, i, i);
    }

    // stop divp
    for (i = 0; i < u32LastWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StopChn(i));
    }

    // disable disp port
    for (i = 0; i < u32LastWndNum; i ++)
    {
        ExecFunc(MI_DISP_DisableInputPort(ST_DISP_LAYER0, i), MI_SUCCESS);
    }


    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }
    else if (u32CurWndNum <= 25)
    {
        u32Square = 5;
    }
    else if (u32CurWndNum <= 36)
    {
        u32Square = 6;
    }

    // set divp outport attribute
    MI_DIVP_CHN divpChn = 0;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;

    for (i = 0; i < u32CurWndNum; i++)
    {
        divpChn = i;
        memset(&stOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));

        STCHECKRESULT(MI_DIVP_GetOutputPortAttr(divpChn, &stOutputPortAttr));
        stOutputPortAttr.u32Width       = ALIGN_BACK(u16DispWidth / u32Square, 16);
        stOutputPortAttr.u32Height      = ALIGN_BACK(u16DispHeight / u32Square, 2);
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }

    // set disp port attribute
    ST_DispChnInfo_t stDispChnInfo;

    memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));
    stDispChnInfo.InputPortNum = u32CurWndNum;
    for (i = 0; i < u32CurWndNum; i++)
    {
        stDispChnInfo.stInputPortAttr[i].u32Port = i;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X =
            ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y =
            ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width =
            ALIGN_BACK(u16DispWidth / u32Square, 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height =
            ALIGN_BACK(u16DispHeight / u32Square, 2);
    }
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER0, &stDispChnInfo));

    // start divp
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StartChn(i));
    }

    // enable disp
    for (i = 0; i < u32CurWndNum; i ++)
    {
        ExecFunc(MI_DISP_EnableInputPort(ST_DISP_LAYER0, i), MI_SUCCESS);
    }

    // 7, bind VDEC to DIVP
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    // start vdec send thread
    for (i = 0; i < u32CurWndNum; i++)
    {
        g_stVdecThreadArgs[i].bRunFlag = TRUE;
    }

    pstCaseDesc[u32CaseIndex].u32ShowWndNum =
        pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;

    return 0;
}

MI_S32 ST_SubExit()
{
    MI_U32 u32WndNum = 0, u32ShowWndNum, u32WndNum_Disp =0;
    MI_S32 i = 0;
    ST_Sys_BindInfo_t stBindInfo;
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_BOOL bFlag = FALSE;
    u32WndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;
    u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].u32ShowWndNum;

    if((g_u32CaseIndex == 42) || (g_u32CaseIndex == 43) ||((g_u32CaseIndex == 52)))
    {
        u32WndNum_Disp = 16;
    }
    else
        u32WndNum_Disp = u32ShowWndNum;
    /************************************************
    step1:  stop send es stream
    *************************************************/
    for (i = 0; i < u32WndNum; i++)
    {
        if (g_stVdecThreadArgs[i].pt)
            bFlag = TRUE;
    }

    if (bFlag == FALSE)
    {
        return MI_SUCCESS;
    }

    g_PushEsExit = TRUE;
    for (i = 0; i < u32WndNum; i++)
    {
        pthread_join(g_stVdecThreadArgs[i].pt, NULL);
        g_stVdecThreadArgs[i].pt = 0;
    }

    /************************************************
    step2:  unbind VDEC to DIVP
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step4:  unbind DIVP to VDISP
    *************************************************/
    for (i = 0; i < u32WndNum_Disp; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    if((g_u32CaseIndex == 42) || (g_u32CaseIndex == 43) || (g_u32CaseIndex == 52))
    {
        for (i = 0; i < u32WndNum_Disp; i++)
        {
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i+u32WndNum_Disp;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 1; //only equal zero
            stBindInfo.stDstChnPort.u32PortId = i;

            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        }
    }
#if USE_DISP_VGA
    for (i = 0; i < u32ShowWndNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 1;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }
#endif
    /************************************************
    step5:  destroy vdec divp vpe vdisp
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        STCHECKRESULT(MI_VDEC_StopChn(i));
        STCHECKRESULT(MI_VDEC_DestroyChn(i));

        STCHECKRESULT(MI_DIVP_StopChn(i));
        STCHECKRESULT(MI_DIVP_DestroyChn(i));
    }

    if((g_u32CaseIndex == 42) || (g_u32CaseIndex == 43) ||((g_u32CaseIndex == 52)))
    {
        MI_S32 s32InputPort = 0;
        for (s32InputPort = 0; s32InputPort < u32WndNum_Disp; s32InputPort++)
        {
            ExecFunc(MI_DISP_DisableInputPort(ST_DISP_LAYER1, s32InputPort), MI_SUCCESS);
        }
        ExecFunc(MI_DISP_DisableVideoLayer(ST_DISP_LAYER1), MI_SUCCESS);
        ExecFunc(MI_DISP_UnBindVideoLayer(ST_DISP_LAYER1, ST_DISP_DEV0), MI_SUCCESS);
    }

    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, ST_DISP_LAYER0, u32WndNum_Disp)); //disp input port 0
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    STCHECKRESULT(ST_Fb_DeInit());

    return MI_SUCCESS;
}

// zoomin chn0
MI_S32 ST_ToggleZoomChn0()
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;
    ST_DispoutTiming_e eCurDispoutTiming = E_ST_TIMING_MAX;
    MI_S32 s32CurHdmiTiming;
    MI_S32 s32CurDispTiming;
    MI_U16 u16CurDispWidth = 0, u16CurDispHeight = 0;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32Square = 1;
    static MI_U32 u32ToggleFlag = 0; // 0: normal, 1:zoomin

    eCurDispoutTiming = pstCaseDesc[u32CaseIndex].eDispoutTiming;

    ST_DBG("eCurDispoutTiming:%d, u32CaseIndex:%d, u32CurSubCaseIndex:%d\n",
        eCurDispoutTiming, u32CaseIndex, u32CurSubCaseIndex);
    STCHECKRESULT(ST_GetTimingInfo(eCurDispoutTiming,
                &s32CurHdmiTiming, &s32CurDispTiming, &u16CurDispWidth, &u16CurDispHeight));

    u32CurWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }
    else if (u32CurWndNum <= 25)
    {
        u32Square = 5;
    }
    else if (u32CurWndNum <= 36)
    {
        u32Square = 6;
    }

    MI_DIVP_CHN divpChn = 0;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_U32 u32divpOrigWidth = 0, u32divpOrigHeight = 0;

    u32divpOrigWidth = ALIGN_BACK(u16CurDispWidth / u32Square, 16);
    u32divpOrigHeight = ALIGN_BACK(u16CurDispHeight / u32Square, 2);

    STCHECKRESULT(MI_DIVP_StopChn(divpChn));

    STCHECKRESULT(MI_DIVP_GetChnAttr(divpChn, &stDivpChnAttr));
    ST_DBG("u32CurWndNum:%d, disp: %dx%d\n", u32CurWndNum, u16CurDispWidth, u16CurDispHeight);
    if (u32ToggleFlag == 0)
    {
        stDivpChnAttr.stCropRect.u16Width = ALIGN_BACK(u32divpOrigWidth / 2, 16);
        stDivpChnAttr.stCropRect.u16Height = ALIGN_BACK(u32divpOrigHeight / 2, 2);;
        u32ToggleFlag = 1;
    }
    else if (u32ToggleFlag == 1)
    {
        stDivpChnAttr.stCropRect.u16Width = u32divpOrigWidth;
        stDivpChnAttr.stCropRect.u16Height = u32divpOrigHeight;
        u32ToggleFlag = 0;
    }

    STCHECKRESULT(MI_DIVP_SetChnAttr(divpChn, &stDivpChnAttr));
    STCHECKRESULT(MI_DIVP_StartChn(divpChn));

    return MI_SUCCESS;
}

void ST_WaitSubCmd(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    char szCmd[16];
    MI_U32 index = 0;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;

    while (!g_subExit)
    {
        ST_CaseUsage();

        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        index = atoi(szCmd);

        if (index <= 0 || index > u32SubCaseSize)
        {
            continue;
        }

        g_u32CurSubCaseIndex = index - 1;

        if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
        {
            ST_ChangeDisplayTiming(); //change timing
        }
        else
        {
            if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
            {
                ST_SubExit();
                return;
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
            {
                ST_ToggleZoomChn0();
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "test region", 11)))
            {
                ST_TestRegion();
            }
            else
            {
                if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
                {
                    ST_SplitWindow(); //switch screen
                }
            }
        }
    }
}

MI_DISP_VidWinRect_t stDispLayer1PortPos[16] = {
    {1280,360,320,180},
    {1600,360,320,180},
    {0,   540,320,180},
    {320, 540,320,180},
    {640, 540,320,180},
    {960, 540,320,180},
    {1280,540,320,180},
    {1600,540,320,180},
    {0,   720,320,180},
    {320, 720,320,180},
    {640, 720,320,180},
    {960, 720,320,180},
    {1280,720,320,180},
    {1600,720,320,180},
    {0,   900,320,180},
    {320, 900,320,180},
};

int ST_VdecH26X(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 i = 0;
    ST_Rect_t stRect;
    MI_U32 u32Square = 0;
    ST_Rect_t stVdispOutRect;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    MI_S32 s32Ret;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;
    MI_U32 u32WndNum_Disp = 0;

    if((u32CaseIndex == 42) || (u32CaseIndex == 43))
    {
        u32WndNum_Disp = 16;
    }
    else
    {
        u32WndNum_Disp = u32WndNum;
    }

    stVdispOutRect.s32X     = 0;
    stVdispOutRect.s32Y     = 0;

    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[u32CaseIndex].eDispoutTiming,
                &s32HdmiTiming, &s32DispTiming, &stVdispOutRect.u16PicW, &stVdispOutRect.u16PicH));

    if((u32CaseIndex == 42) || (u32CaseIndex == 43))
    {
        stVdispOutRect.u16PicH = stVdispOutRect.u16PicH/2;
    }
    /************************************************
    step2:  create VDEC chn
    *************************************************/
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_CHN vdecChn = 0;

    for (i = 0; i < u32WndNum; i++)
    {
        vdecChn = i;
        memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
        stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
        stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
        stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
        stVdecChnAttr.u32PicWidth   = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
        stVdecChnAttr.u32PicHeight  = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
        stVdecChnAttr.u32Priority   = 0;
        stVdecChnAttr.eCodecType    = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.eCodecType;

        //ExecFunc(MI_VDEC_CreateChn(vdecChn, &stVdecChnAttr), MI_SUCCESS);
        //ExecFunc(MI_VDEC_StartChn(vdecChn), MI_SUCCESS);
        s32Ret = MI_VDEC_CreateChn(vdecChn, &stVdecChnAttr);
        if (s32Ret != MI_SUCCESS)
        {
            ST_ERR("MI_VDEC_CreateChn error, chn:%d, %X\n", vdecChn, s32Ret);
            return 1;
        }

        s32Ret = MI_VDEC_StartChn(vdecChn);
        if (s32Ret != MI_SUCCESS)
        {
            ST_ERR("MI_VDEC_StartChn error, chn:%d, %X\n", vdecChn, s32Ret);
            return 1;
        }

        printf("%s %d, start vdec chn %d success\n", __func__, __LINE__, vdecChn);
    }

#if 0
    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        vdecChn = i;
        memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
        stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
        stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
        stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
        stVdecChnAttr.u32PicWidth   = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxWidth;
        stVdecChnAttr.u32PicHeight  = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxHeight;
        stVdecChnAttr.u32Priority   = 0;
        stVdecChnAttr.eCodecType    = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.eCodecType;

        ExecFunc(MI_VDEC_CreateChn(vdecChn, &stVdecChnAttr), MI_SUCCESS);
        ExecFunc(MI_VDEC_StartChn(vdecChn), MI_SUCCESS);
    }
#endif

    /************************************************
    step3:  create DIVP chn
    *************************************************/
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;

    if (u32WndNum_Disp <= 1)
    {
        u32Square = 1;
    }
    else if (u32WndNum_Disp <= 4)
    {
        u32Square = 2;
    }
    else if (u32WndNum_Disp <= 9)
    {
        u32Square = 3;
    }
    else if (u32WndNum_Disp <= 16)
    {
        u32Square = 4;
    }
    else if (u32WndNum_Disp <= 25)
    {
        u32Square = 5;
    }
    else if (u32WndNum_Disp <= 36)
    {
        u32Square = 6;
    }

    for (i = 0; i < u32WndNum; i++)
    {
        divpChn = i;
        memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
        stDivpChnAttr.bHorMirror            = FALSE;
        stDivpChnAttr.bVerMirror            = FALSE;
        stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X       = 0;
        stDivpChnAttr.stCropRect.u16Y       = 0;
        stDivpChnAttr.stCropRect.u16Width   = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth; //Vdec pic w
        stDivpChnAttr.stCropRect.u16Height  = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight; //Vdec pic h
        stDivpChnAttr.u32MaxWidth           = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth; //max size picture can pass
        stDivpChnAttr.u32MaxHeight          = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;

        ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
        ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

        memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
        stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
        stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stOutputPortAttr.u32Width           = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 16);
        stOutputPortAttr.u32Height          = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);

        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }

    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        divpChn = ST_DIVP_PIP_CHN;
        memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
        stDivpChnAttr.bHorMirror            = FALSE;
        stDivpChnAttr.bVerMirror            = FALSE;
        stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X       = 0;
        stDivpChnAttr.stCropRect.u16Y       = 0;
        stDivpChnAttr.stCropRect.u16Width   = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxWidth; //Vdec pic w
        stDivpChnAttr.stCropRect.u16Height  = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxHeight; //Vdec pic h
        stDivpChnAttr.u32MaxWidth           = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxWidth; //max size picture can pass
        stDivpChnAttr.u32MaxHeight          = pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.u32MaxHeight;

        ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
        ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

        memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
        stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
        stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stOutputPortAttr.u32Width           = ALIGN_BACK(stVdispOutRect.u16PicW / 3, 16);
        stOutputPortAttr.u32Height          = ALIGN_BACK(stVdispOutRect.u16PicH / 3, 2);

        ST_DBG("DIVP PIP position, disp:%dx%d (%d,%d)\n", stVdispOutRect.u16PicW, stVdispOutRect.u16PicH,
            stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);

        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }

    /************************************************
    step4:  start DISP device and chn
    *************************************************/
    #if 1
    ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32DispTiming);

    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER1, s32DispTiming);
    }

#if USE_DISP_VGA
    ST_Disp_DevInit(ST_DISP_DEV1, ST_DISP_LAYER1, s32DispTiming);
#endif

    // must init after disp
    //ST_Fb_Init(E_MI_FB_COLOR_FMT_ARGB1555);
    //ST_FB_Show(FALSE);

    // init hdmi
    STCHECKRESULT(ST_Hdmi_Init()); //Set hdmi outout 1080P
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32HdmiTiming));

    //
    ST_DispChnInfo_t stDispChnInfo;

    memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));
    if(u32CaseIndex == 52)
        u32WndNum_Disp = 16;

    stDispChnInfo.InputPortNum = u32WndNum_Disp;

    for (i = 0; i < u32WndNum_Disp; i++)
    {
        stDispChnInfo.stInputPortAttr[i].u32Port = i;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X =
            ALIGN_BACK((stVdispOutRect.u16PicW / u32Square) * (i % u32Square), 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y =
            ALIGN_BACK((stVdispOutRect.u16PicH / u32Square) * (i / u32Square), 2);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width =
            ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 16);
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height =
            ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);
    }
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER0, &stDispChnInfo));

#if USE_DISP_VGA
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo));
#endif

    if((u32CaseIndex == 42) || (u32CaseIndex == 43))
    {
        ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER1, s32DispTiming);
        ST_DispChnInfo_t stDispChnInfo1;

        memset(&stDispChnInfo1, 0, sizeof(ST_DispChnInfo_t));
        stDispChnInfo1.InputPortNum = u32WndNum_Disp;

        for (i = 0; i < u32WndNum_Disp; i++)
        {
            stDispChnInfo1.stInputPortAttr[i].u32Port = i;
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16X =
                ALIGN_BACK((stVdispOutRect.u16PicW / u32Square) * (i % u32Square), 16);
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Y =
                ALIGN_BACK((stVdispOutRect.u16PicH / u32Square) * (i / u32Square), 2)+stVdispOutRect.u16PicH;
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Width =
                ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 16);
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Height =
                ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);
        }
        STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo1));
    }

    if(u32CaseIndex == 52)
    {
        ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER1, s32DispTiming);
        ST_DispChnInfo_t stDispChnInfo1;

        memset(&stDispChnInfo1, 0, sizeof(ST_DispChnInfo_t));
        stDispChnInfo1.InputPortNum = u32WndNum_Disp;

        for (i = 0; i < u32WndNum_Disp; i++)
        {
            stDispChnInfo1.stInputPortAttr[i].u32Port = i;
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16X =
                ALIGN_BACK(stDispLayer1PortPos[i].u16X, 16);
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Y =
                ALIGN_BACK(stDispLayer1PortPos[i].u16Y, 2);
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Width =
                ALIGN_BACK(stDispLayer1PortPos[i].u16Width, 16);
            stDispChnInfo1.stInputPortAttr[i].stAttr.stDispWin.u16Height =
                ALIGN_BACK(stDispLayer1PortPos[i].u16Height, 2);
        }
        STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo1));
    }

    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));

        stDispChnInfo.InputPortNum = 1;
        stDispChnInfo.stInputPortAttr[0].u32Port = ST_DISP_PIP_PORT;
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X =
            ALIGN_BACK((stVdispOutRect.u16PicW / 3) * (8 % 3), 16);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y =
            ALIGN_BACK((stVdispOutRect.u16PicH / 3) * (8 / 3), 2);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width =
            ALIGN_BACK(stVdispOutRect.u16PicW / 3, 16);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height =
            ALIGN_BACK(stVdispOutRect.u16PicH / 3, 2);

        ST_DBG("DISP PIP position, disp:%dx%d (%d,%d,%d,%d)\n", stVdispOutRect.u16PicW, stVdispOutRect.u16PicH,
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X,
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y,
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width,
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height);
        STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo));
    }
#endif
    /************************************************
    step5:  bind VDEC to DIVP
    *************************************************/
    ST_Sys_BindInfo_t stBindInfo;

    for (i = 0; i < u32WndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d vdec(%d)->divp(%d) bind\n", __func__, __LINE__, i, i);
    }

    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = ST_DIVP_PIP_CHN;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d vdec(%d)->divp(%d) bind\n", __func__, __LINE__, 0, ST_DIVP_PIP_CHN);
    }

    /************************************************
    step6:  bind DIVP to DISP
    *************************************************/
    for (i = 0; i < u32WndNum_Disp; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d divp(%d)->disp(%d) bind\n", __func__, __LINE__, i, i);
    }

    if((u32CaseIndex == 42) || (u32CaseIndex == 43)||((u32CaseIndex == 52)))
    {
        for (i = 0; i < u32WndNum_Disp; i++)
        {
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i+16;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 1; //only equal zero
            stBindInfo.stDstChnPort.u32PortId = i;
            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;

            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

            printf("%s %d divp(%d)->disp(%d) bind\n", __func__, __LINE__, i, i);
        }
    }

    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = ST_DIVP_PIP_CHN;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER1;
        stBindInfo.stDstChnPort.u32PortId = ST_DISP_PIP_PORT;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d divp(%d)->disp(%d) bind\n", __func__, __LINE__, ST_DIVP_PIP_CHN, ST_DISP_PIP_PORT);
    }
#if USE_DISP_VGA
    for (i = 0; i < u32WndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 1;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d divp(%d)->disp(%d) bind\n", __func__, __LINE__, i, i);
    }
#endif
    /************************************************
    step9:  send es stream to vdec
    *************************************************/
    for (i = 0; i < u32WndNum; i++)
    {
        g_stVdecThreadArgs[i].vdecChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32Chn;
        snprintf(g_stVdecThreadArgs[i].szFileName, sizeof(g_stVdecThreadArgs[i].szFileName) - 1,
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.szFilePath);

        g_stVdecThreadArgs[i].bRunFlag = TRUE;
        g_PushEsExit = FALSE;
        pthread_create(&g_stVdecThreadArgs[i].pt, NULL, ST_VdecSendStream, (void *)&g_stVdecThreadArgs[i]);

        // pthread_create(&g_stVdecThreadArgs[i].ptsnap, NULL, ST_SnapProcess, (void *)&g_stVdecThreadArgs[i]);
    }

#if 0
    if (pstCaseDesc[u32CaseIndex].s32SplitMode == E_ST_SPLIT_MODE_TWO)
    {
        g_stVdecThreadArgs[i].vdecChn = 1;
        snprintf(g_stVdecThreadArgs[i].szFileName, sizeof(g_stVdecThreadArgs[i].szFileName) - 1,
                pstCaseDesc[u32CaseIndex].stCaseArgs[0].uChnArg.stVdecChnArg.szFilePath);

        g_stVdecThreadArgs[i].bRunFlag = TRUE;
        g_PushEsExit = FALSE;
        pthread_create(&g_stVdecThreadArgs[i].pt, NULL, ST_VdecSendStream, (void *)&g_stVdecThreadArgs[i]);
    }
#endif

    // pthread_create(&g_writeFilePt, NULL, ST_VdecWriteFileToDisk, NULL);

    // pthread_create(&g_vdisp_pt, NULL, ST_VdecGetVdispOutputBuf, NULL);

    g_u32LastSubCaseIndex = pstCaseDesc[u32CaseIndex].u32SubCaseNum - 1;
    g_u32CurSubCaseIndex = pstCaseDesc[u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[u32CaseIndex].u32ShowWndNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;
}

void ST_DealCase(int argc, char **argv)
{
    MI_U32 u32Index = 0;
    MI_U32 u32SubIndex = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;

    if (argc != 3)
    {
        return;
    }

    u32Index = atoi(argv[1]);
    u32SubIndex = atoi(argv[2]);

    if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stCaseDesc))
    {
        printf("case index range (%d~%d)\n", 1, ARRAY_SIZE(g_stCaseDesc));
        return;
    }
    g_u32CaseIndex = u32Index - 1;

    if (u32SubIndex <= 0 || u32SubIndex > pstCaseDesc[g_u32CaseIndex].u32SubCaseNum)
    {
        printf("sub case index range (%d~%d)\n", 1, pstCaseDesc[g_u32CaseIndex].u32SubCaseNum);
        return;
    }

    g_u32LastSubCaseIndex = pstCaseDesc[g_u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[g_u32CaseIndex].u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;

    printf("case index %d, sub case %d-%d\n", g_u32CaseIndex, g_u32CurSubCaseIndex, g_u32LastSubCaseIndex);

    ST_VdecH26X();

    g_u32CurSubCaseIndex = u32SubIndex - 1;

    if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
    {
        ST_ChangeDisplayTiming(); //change timing
    }
    else
    {
        if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
        {
            ST_SubExit();
            return;
        }
        else if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
        {
        }
        else
        {
            if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
            {
                ST_SplitWindow(); //switch screen
            }
        }
    }

    ST_WaitSubCmd();
}

#define ENABLE_AUTOTEST 0

int autotest(int argc, char **argv)
{
    MI_S32 s32CaseArray[6] = {0,1,4,5,16,17};
    MI_S32 s32RandNumber = 0;
    MI_S32 s32LastRandNumber = 0;

    STCHECKRESULT(ST_Sys_Init());

    while (1)
    {
        s32RandNumber = rand() % ARRAY_SIZE(s32CaseArray);
        if (s32LastRandNumber != s32RandNumber)
        {
            sleep(1);
            printf("Run test case %d...\n", s32CaseArray[s32RandNumber]);
            sleep(1);
            g_u32CaseIndex = s32CaseArray[s32RandNumber];
            ST_VdecH26X();
            sleep(30);
            ST_SubExit();
            s32LastRandNumber = s32RandNumber;
        }
    }

    STCHECKRESULT(ST_Sys_Exit());
    return MI_SUCCESS;
}

void ST_VDEC_HandleSig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        printf("catch Ctrl + C, exit\n");

        ST_SubExit();

        sleep(1);

        ST_Sys_Exit();

        exit(0);
    }
}

int main(int argc, char **argv)
{
    MI_U32 u32WndNum = 4;
    MI_U32 u32Index = 0;
    MI_U32 u32Temp = 0;
    char szCmd[16];
    struct sigaction sigAction;
    int enterCount = 0;
#if ENABLE_AUTOTEST
    return autotest(argc, argv);
#endif

    sigAction.sa_handler = ST_VDEC_HandleSig;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	sigaction(SIGINT, &sigAction, NULL);

    /************************************************
    step1:  init sys
    *************************************************/

    STCHECKRESULT(ST_Sys_Init());
    ST_DealCase(argc, argv);

    while (!g_bExit)
    {
        ST_VdecUsage();
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        u32Index = atoi(szCmd);

        printf("%s %d, u32Index:%d\n", __func__, __LINE__, u32Index);

        if (u32Index == 0)
        {
            // enter key
            if (enterCount ++ >= 1)
            {
                break;
            }
        }

        if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stCaseDesc))
        {
            continue;
        }

        g_u32CaseIndex = u32Index - 1;

        ST_VdecH26X();
        ST_WaitSubCmd();
    }

    printf("process exit normal\n");

    ST_SubExit();
    STCHECKRESULT(ST_Sys_Exit());

    return 0;
}
