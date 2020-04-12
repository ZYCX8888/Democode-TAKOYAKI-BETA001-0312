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
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "st_common.h"
#include "mi_venc.h"

// #define SUPPORT_UVC

#ifdef SUPPORT_UVC
#include "mi_uvc.h"
#endif

#define MAX_VENC_NUM    16

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_U32 u32MainWidth;
    MI_U32 u32MainHeight;
    MI_VENC_ModType_e eType;
    int vencFd;
} VENC_Attr_t;

typedef struct
{
    pthread_t ptGetEs;
    pthread_t ptFillYuv;
    VENC_Attr_t stVencAttr[MAX_VENC_NUM];
    MI_U32 u32ChnNum;
    MI_BOOL bRunFlag;
} VENC_Thread_Args_t;

#define SUB_VENC_INTERVAL       16

static int g_yuv_width = 1920;
static int g_yuv_height = 1080;
static int g_yuv_format = 0;
static char *g_file_path = NULL;

static MI_BOOL g_bExit = FALSE;
static MI_U32 g_u32CaseIndex = 0;

VENC_Thread_Args_t g_stVencThreadArgs[MAX_CHN_NUM];

ST_CaseDesc_t g_stCaseDesc[] =
{
    {
        .stDesc =
        {
            .szDesc = "1x4K(3840x2160)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 3840,
                        .u16MainHeight = 2160,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x4K(3840x2160)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 3840,
                        .u16MainHeight = 2160,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x1080P@25 H264 + 1xD1@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x1080P(1920x1080)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x1080P(1920x1080)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "2x1080P(1920x1080)@25 H264 Encode",
            .u32VencNum = 2,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "4x1080P(1920x1080)@25 H264 Encode",
            .u32VencNum = 4,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "4x1080P(1920x1080)@25 H265 Encode",
            .u32VencNum = 4,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "8x1080P(1920x1080)@25 H264 Encode",
            .u32VencNum = 8,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1080,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1xCIF(352x288)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 352,
                        .u16MainHeight = 288,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1xCIF(352x288)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 352,
                        .u16MainHeight = 288,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x720P(1280x720)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x720P(1280x720)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "4x720P(1280x720)@25 H264 Encode",
            .u32VencNum = 4,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "4x720P(1280x720)@25 H265 Encode",
            .u32VencNum = 4,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 1280,
                        .u16MainHeight = 720,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1xD1(736x576)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 15,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1xD1(720x576)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 15,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x960H(960x576)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x960H(960x576)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "9x960H(960x576)@25 H264 Encode",
            .u32VencNum = 9,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 8,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "9x960H(960x576)@25 H265 Encode",
            .u32VencNum = 9,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 8,
                        .u16MainWidth = 960,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "16xD1(736x576)@25 H264 Encode",
            .u32VencNum = 16,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 8,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 9,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 10,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 11,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 12,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 13,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 14,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 15,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "16xD1(720x576)@25 H265 Encode",
            .u32VencNum = 16,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 8,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 9,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 10,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 11,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 12,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 13,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 14,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 15,
                        .u16MainWidth = 720,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "16xD1(736x576)@25 H264 + 16xD1(736x576)@25 H265 Encode",
            .u32VencNum = 32,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 4,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 5,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 6,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 7,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 8,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 9,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 10,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 11,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 12,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 13,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 14,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 15,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 16,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 17,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 18,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 19,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 20,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 21,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 22,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 23,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 24,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 25,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 26,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 27,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 28,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 29,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 30,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 31,
                        .u16MainWidth = 736,
                        .u16MainHeight = 576,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 25,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x4MP(2560x1440)@25 H265 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x4MP(2560x1440)@25 H264 Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "4x4MP(2560x1440)@25 H264 Encode",
            .u32VencNum = 4,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 1,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 2,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 3,
                        .u16MainWidth = 2560,
                        .u16MainHeight = 1440,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1x4K(3840x2160)@25 JPEG Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 3840,
                        .u16MainHeight = 2160,
                        .eType = E_MI_VENC_MODTYPE_JPEGE,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
    {
        .stDesc =
        {
            .szDesc = "1xFHD(1920x1088)@25 JPEG Encode",
            .u32VencNum = 1,
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVencChnArg =
                    {
                        .u32Chn = 0,
                        .u16MainWidth = 1920,
                        .u16MainHeight = 1088,
                        .eType = E_MI_VENC_MODTYPE_JPEGE,
                        .u16SubWidth = 720,
                        .u16SubHeight = 576,
                        .s32MainBitRate = 100,
                        .s32SubBitRate = 100,
                        .s32MainFrmRate = 30,
                        .s32SubFrmRate = 25
                    },
                }
            },
        },
    },
};

void ST_Venc_Help(const char *porgName)
{
    printf("%s [-options] source.yuv\n", porgName);
    printf(" -s <width> <height> .... source resolution.\n");
    printf(" -p <yuv format> ........ 0: yuv420, 1: yuv422.\n");
    printf(" -h ..................... print this help\n");
    exit(0);
}

void ST_Venc_ParseOptions(int argc, char **argv)
{
    if (argc <= 1)
    {
        ST_Venc_Help(argv[0]);
    }

    int i = 0;
    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help"))
        {
            ST_Venc_Help(argv[0]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            g_yuv_width = atoi(argv[++i]);    // NOLINT
            g_yuv_height = atoi(argv[++i]);   // NOLINT
        }
        else if (!strcmp(argv[i], "-p"))
        {
            g_yuv_format = atoi(argv[++i]);
        }
        else if (g_file_path == NULL)
        {
            g_file_path = argv[i];
        }
    }
}

void ST_VencUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 i = 0;

    printf("\n");

    for (i = 0; i < u32Size; i ++)
    {
        pstCaseDesc[i].stDesc.u32CaseIndex = i;
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
    }

    printf("print twice Enter to exit\n");
}

void ST_VENC_HandleSig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        printf("catch Ctrl + C, exit\n");

        g_bExit = TRUE;
        exit(0);
    }
}

MI_U32 ST_VENC_GetDevID(MI_VENC_ModType_e eType)
{
    MI_U32 u32Dev = 2;

    if (eType == E_MI_VENC_MODTYPE_H264E)
    {
        u32Dev = 2;
    }
    else if (eType == E_MI_VENC_MODTYPE_H265E)
    {
        u32Dev = 0;
    }
    else if (eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        u32Dev = 4;
    }

    return u32Dev;
}

#ifdef SUPPORT_UVC
MI_S32 UVC_Init(void *uvc)
{
    return MI_SUCCESS;
}

MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}

MI_S32 _UVC_FillBuffer_Encoded(MI_UVC_Device_t *uvc, MI_U32 *length, void *buf)
{
    MI_S32 vencFd = -1;
    fd_set read_fds;
    struct timeval TimeoutVal;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn = 0;
    MI_VENC_Stream_t stStream;

    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;

    vencFd = MI_VENC_GetFd(vencChn);

    FD_ZERO(&read_fds);
    FD_SET(vencFd, &read_fds);

    s32Ret = select(vencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        // select failed
        return MI_SUCCESS;
    }
    else if (0 == s32Ret)
    {
        // timeout
        return MI_SUCCESS;
    }
    else
    {
        if (FD_ISSET(vencFd, &read_fds))
        {
            memset(&stStat, 0, sizeof(MI_VENC_ChnStat_t));
            s32Ret = MI_VENC_Query(vencChn, &stStat);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_VENC_Query error, %X\n", __func__, __LINE__, s32Ret);
            }

            printf("u32CurPacks:%d, u32LeftStreamFrames:%d\n", stStat.u32CurPacks, stStat.u32LeftStreamFrames);

            memset(&stStream, 0, sizeof(MI_VENC_Stream_t));

            stStream.u32PackCount = 10;
            stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStream.u32PackCount);
            s32Ret = MI_VENC_GetStream(vencChn, &stStream, -1);
            if (MI_SUCCESS == s32Ret)
            {
                printf("u32PackCount:%d, u32Seq:%d, offset:%d,len:%d\n", stStream.u32PackCount, stStream.u32Seq,
                    stStream.pstPack[0].u32Offset, stStream.pstPack[0].u32Len);

#if 0
                for (i = 0; i < stStream.u32PackCount; i ++)
                {
                    *length = stBufInfo.stRawData.u32ContentSize;
                    memcpy(buf, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
                    write(fd, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
                        stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                }
#endif
                *length = stStream.pstPack[0].u32Len;
                memcpy(buf, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);

                MI_VENC_ReleaseStream(vencChn, &stStream);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
            }
            else
            {
                // printf("%s %d, MI_VENC_GetStream error, %X\n", __func__, __LINE__, s32Ret);
            }
        }
    }

    return MI_SUCCESS;
}

MI_S32 UVC_FillBuffer(void *uvc_dev,MI_U32 *length,void *buf)
{
    MI_UVC_Device_t *uvc = (MI_UVC_Device_t*)uvc_dev;

    switch(uvc->stream_param.fcc)
    {
        case V4L2_PIX_FMT_H264:
        {
            _UVC_FillBuffer_Encoded(uvc, length, buf);
        }
        break;

        case V4L2_PIX_FMT_MJPEG:
        {
            _UVC_FillBuffer_Encoded(uvc,length,buf);
        }
        break;

        default:
        {
            _UVC_FillBuffer_Encoded(uvc,length,buf);
            //printf("%s %d, not support this type, %d\n", __func__, __LINE__, uvc->stream_param.fcc);
        }
        break;
    }

    return MI_SUCCESS;
}

MI_S32 UVC_StartCapture(void *uvc,Stream_Params_t format)
{
    return MI_SUCCESS;
}

MI_S32 UVC_StopCapture(void *uvc)
{
    return MI_SUCCESS;
}

void ST_UVCInit()
{
    MI_U32 u32ChnId  = 0;
    MI_U32 u32PortId = 0;

    MI_UVC_Setting_t pstSet={4,USB_ISOC_MODE};
    MI_UVC_OPS_t fops = { UVC_Init ,
                          UVC_Deinit,
                          UVC_FillBuffer,
                          UVC_StartCapture,
                          UVC_StopCapture};

    MI_UVC_ChnAttr_t pstAttr ={pstSet,fops};
    MI_UVC_Init("/dev/video0");
    MI_UVC_CreateDev(u32ChnId,u32PortId,&pstAttr);
    MI_UVC_StartDev();
}
#endif

MI_VENC_RcMode_e ST_VENC_GetRcMode(MI_VENC_ModType_e eType)
{
    MI_VENC_RcMode_e eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;

    if (eType == E_MI_VENC_MODTYPE_H264E)
    {
        eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
    }
    else if (eType == E_MI_VENC_MODTYPE_H265E)
    {
        eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
    }

    return eRcMode;
}

void *ST_VencGetEsProc(void *args)
{
    VENC_Thread_Args_t *pArgs = (VENC_Thread_Args_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    char szFileName[128];
    int fd = -1;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_ChnStat_t stStat;
    MI_U32 u32DevId = 0;
    MI_U32 i = 0;
    MI_S32 vencFd = -1;
    fd_set read_fds;
    struct timeval TimeoutVal;

    MI_VENC_GetChnDevid(pArgs->stVencAttr[0].vencChn, &u32DevId);

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;//ST_VENC_GetDevID(pArgs->stVencAttr[0].eType);
    stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
    stVencChnInputPort.u32PortId = 0;

    memset(szFileName, 0, sizeof(szFileName));

    len = snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%dx%d.",
        stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
        pArgs->stVencAttr[0].u32MainWidth, pArgs->stVencAttr[0].u32MainHeight);
    if (pArgs->stVencAttr[0].eType == E_MI_VENC_MODTYPE_H264E)
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h264");
    }
    else if (pArgs->stVencAttr[0].eType == E_MI_VENC_MODTYPE_H265E)
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h265");
    }
    else
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "mjpeg");
    }

    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd <= 0)
    {
        printf("%s %d create %s error\n", __func__, __LINE__, szFileName);
        return NULL;
    }

    vencFd = MI_VENC_GetFd(pArgs->stVencAttr[0].vencChn);

    printf("%s %d create %s success\n", __func__, __LINE__, szFileName);

    FD_ZERO(&read_fds);
    FD_SET(vencFd, &read_fds);

    while(pArgs->bRunFlag)
    {
#if 1
        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle)))
        {
            if(hHandle == NULL)
            {
                printf("%s %d NULL output port buffer handle.\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.pVirAddr == NULL)
            {
                printf("%s %d unable to read buffer. VA==0\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.u32ContentSize >= 200 * 1024)  //MAX_OUTPUT_ES_SIZE in KO
            {
                printf("%s %d unable to read buffer. buffer overflow\n", __func__, __LINE__);
            }

            len = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);

            //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
            //    len, stBufInfo.stRawData.u32ContentSize);

            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        else
        {
            // printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X\n", __func__, __LINE__, s32Ret);
            usleep(10 * 1000);//sleep 10 ms
        }
#else
        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;

        s32Ret = select(vencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            // select failed
            usleep(10 * 1000);
            continue;
        }
        else if (0 == s32Ret)
        {
            // timeout
            usleep(10 * 1000);
            continue;
        }
        else
        {
            if (FD_ISSET(vencFd, &read_fds))
            {
                memset(&stStat, 0, sizeof(MI_VENC_ChnStat_t));
                s32Ret = MI_VENC_Query(pArgs->stVencAttr[0].vencChn, &stStat);
                if (MI_SUCCESS != s32Ret)
                {
                    printf("%s %d, MI_VENC_Query error, %X\n", __func__, __LINE__, s32Ret);
                    usleep(10 * 1000);//sleep 10 ms
                }

                printf("u32CurPacks:%d, u32LeftStreamFrames:%d\n", stStat.u32CurPacks, stStat.u32LeftStreamFrames);

                memset(&stStream, 0, sizeof(MI_VENC_Stream_t));

                stStream.u32PackCount = 10;
                stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStream.u32PackCount);
                s32Ret = MI_VENC_GetStream(pArgs->stVencAttr[0].vencChn, &stStream, -1);
                if (MI_SUCCESS == s32Ret)
                {
                    printf("u32PackCount:%d, u32Seq:%d, offset:%d,len:%d\n", stStream.u32PackCount, stStream.u32Seq,
                        stStream.pstPack[0].u32Offset, stStream.pstPack[0].u32Len);

                    for (i = 0; i < stStream.u32PackCount; i ++)
                    {
                        write(fd, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
                            stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                    }

                    MI_VENC_ReleaseStream(pArgs->stVencAttr[0].vencChn, &stStream);
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
                else
                {
                    // printf("%s %d, MI_VENC_GetStream error, %X\n", __func__, __LINE__, s32Ret);
                    usleep(10 * 1000);//sleep 10 ms
                }
            }
        }
#endif
    }

    close(fd);
}

void *ST_VencFillYUVProc(void *args)
{
    VENC_Thread_Args_t *pArgs = (VENC_Thread_Args_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    FILE *pFile = NULL;
    MI_U32 u32FrameSize = 0;
    MI_U32 u32YSize = 0;
    MI_U32 u32FilePos = 0;
    struct stat st;
    MI_U32 u32DevId = 0;

#if 1
    char szUserData[] = "Insert User Data";
    MI_U32 u32DataLen = strlen(szUserData);
    MI_U32 u32FrameCount = 0;
#endif

    memset(&stVencChnInputPort, 0, sizeof(MI_SYS_ChnPort_t));
    MI_VENC_GetChnDevid(pArgs->stVencAttr[0].vencChn, &u32DevId);

    printf("%s %d, chn:%d, dev:%d\n", __func__, __LINE__, pArgs->stVencAttr[0].vencChn, u32DevId);

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;//ST_VENC_GetDevID(pArgs->stVencAttr[0].eType);
    stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
    stVencChnInputPort.u32PortId = 0;

    memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = g_yuv_width;
    stBufConf.stFrameCfg.u16Height = g_yuv_height;

    if (0 != stat(g_file_path, &st))
    {
        printf("stat %s error\n", g_file_path);
    }

    pFile = fopen(g_file_path, "rb");
    if (pFile == NULL)
    {
        printf("%s %d, open %s error\n", __func__, __LINE__, g_file_path);
        return NULL;
    }

    printf("open %s success, total size:%d bytes\n", g_file_path, st.st_size);

    u32YSize = g_yuv_width * g_yuv_height;
    u32FrameSize = (u32YSize >> 1) + u32YSize;

    printf("%s %d, chn:%d u32YSize:%d,u32FrameSize:%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
        u32YSize, u32FrameSize);

    while (pArgs->bRunFlag)
    {
        memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));

        // printf("devid(%d),,chnid(%d),,,portid(%d)\n", stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId);
        // printf("%s %d, %d\n", __func__, __LINE__, ftell(pFile));
        u32FilePos = ftell(pFile);
        if ((st.st_size - u32FilePos) < u32FrameSize)
        {
            fseek(pFile, 0L, SEEK_SET);
            printf("seek to the begin of the file, u32FilePos:%d, curpos:%ld\n", u32FilePos, ftell(pFile));
        }

        s32Ret = MI_SYS_ChnInputPortGetBuf(&stVencChnInputPort, &stBufConf, &stBufInfo, &hHandle, 1000);
        if(MI_SUCCESS == s32Ret)
        {
            if (0 >= fread(stBufInfo.stFrameData.pVirAddr[0], 1, u32YSize, pFile))
            {
                fseek(pFile, 0, SEEK_SET);

                stBufInfo.bEndOfStream = TRUE;
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if (MI_SUCCESS == s32Ret)
                {
                    printf("%s %d, MI_SYS_ChnInputPortPutBuf error, %X\n", __func__, __LINE__, s32Ret);
                }
                continue;
            }

            if (0 >= fread(stBufInfo.stFrameData.pVirAddr[1], 1, u32YSize >> 1, pFile))
            {
                fseek(pFile, 0, SEEK_SET);

                stBufInfo.bEndOfStream = TRUE;
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if (MI_SUCCESS == s32Ret)
                {
                    printf("%s %d, MI_SYS_ChnInputPortPutBuf error, %X\n", __func__, __LINE__, s32Ret);
                }
                continue;
            }


            s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_SYS_ChnInputPortPutBuf error, %X\n", __func__, __LINE__, s32Ret);
            }
            u32FrameCount ++;
        }
        else
        {
            printf("%s %d, MI_SYS_ChnInputPortGetBuf error, chn:%d, %X\n",
                __func__, __LINE__, pArgs->stVencAttr[0].vencChn, s32Ret);
        }
    }

    fclose(pFile);

    return NULL;
}

void ST_VENC_AttrInit(void)
{
    MI_U32 i = 0;

    g_stVencThreadArgs[0].u32ChnNum = 0;

    for (i = 0; i < MAX_VENC_NUM; i ++)
    {
        g_stVencThreadArgs[0].stVencAttr[i].vencChn = -1;
        g_stVencThreadArgs[0].stVencAttr[i].u32MainWidth = 0;
        g_stVencThreadArgs[0].stVencAttr[i].u32MainHeight = 0;
        g_stVencThreadArgs[0].stVencAttr[i].eType = E_MI_VENC_MODTYPE_MAX;
        g_stVencThreadArgs[0].stVencAttr[i].vencFd = -1;
    }
}

int ST_VencH26X(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32DevId = 0;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return 0;
    }

    MI_U32 u32VencNum = pstCaseDesc[u32CaseIndex].stDesc.u32VencNum;

    // main+sub venc
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;

    MI_VENC_RoiCfg_t stVencRoiCfg;

#if 0
    char szUserData[] = "Mstar";
    MI_U32 u32DataLen = strlen(szUserData);
#endif

    printf("%s %d, total chn num:%d\n", __func__, __LINE__, u32VencNum);

    for (i = 0; i < u32VencNum; i ++)
    {
        VencChn = i;

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

        //printf("%s %d, chn:%d,eType:%d\n", __func__, __LINE__, VencChn,
        //    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType);

        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;

            ST_DBG("u32PicWidth:%d, u32PicHeight:%d\n", stChnAttr.stVeAttr.stAttrH264e.u32PicWidth,
                stChnAttr.stVeAttr.stAttrH264e.u32PicHeight);
#if 0
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
#endif

#if 1
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
#endif

#if 0
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = 4 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 30;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 25;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
#endif

#if 0
            // not support ABR mode, 2018-02-06
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264ABR;
            stChnAttr.stRcAttr.stAttrH264Abr.u32AvgBitRate = 4 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Abr.u32MaxBitRate = 4 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Abr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Abr.u32SrcFrmRate =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH264Abr.u32StatTime = 0;
            // not support ABR mode, 2018-02-06
#endif
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;

#if 0
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRate =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 25;
#endif

#if 1
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = 2 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
#endif

#if 0
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 2 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 30;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate =
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
#endif
        }
        else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
        }

        // ExecFunc(MI_VENC_CreateChn(VencChn, &stChnAttr), MI_SUCCESS);
        s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        printf("%s %d MI_VENC_CreateChn, vencChn:%d\n", __func__, __LINE__, VencChn);
        if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            MI_VENC_ParamJpeg_t stParamJpeg;

            memset(&stParamJpeg, 0, sizeof(stParamJpeg));
            s32Ret = MI_VENC_GetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
            printf("Get Qf:%d\n", stParamJpeg.u32Qfactor);

            stParamJpeg.u32Qfactor = 100;
            s32Ret = MI_VENC_SetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
        }
        // printf("%s %d, create venc %d success, refnum:%d,u32Profile:%d\n", __func__, __LINE__, VencChn,
        //    stChnAttr.stVeAttr.stAttrH264e.u32RefNum, stChnAttr.stVeAttr.stAttrH264e.u32Profile);
        s32Ret = MI_VENC_GetChnDevid(VencChn, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        stVencChnOutputPort.u32DevId = u32DevId;
        stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
        stVencChnOutputPort.u32ChnId = VencChn;
        stVencChnOutputPort.u32PortId = 0;
        //This was set to (5, 10) and might be too big for kernel
        // ExecFunc(MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 3), MI_SUCCESS);
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 3);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

#if 0
        s32Ret = MI_VENC_InsertUserData(VencChn, szUserData, u32DataLen + 1);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_InsertUserData %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#endif

        // ExecFunc(MI_VENC_StartRecvPic(VencChn), MI_SUCCESS);
        s32Ret = MI_VENC_StartRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

#if 0
        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        s32Ret = MI_VENC_GetChnAttr(VencChn, &stChnAttr);
        if (MI_VENC_OK != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnAttr %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

        printf("%s %d, u32MaxPicHeight:%d, u32MaxPicWidth:%d\n", __func__, __LINE__,
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight, stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth);
#endif

#if 0
        // roi must set after venc start
        memset(&stVencRoiCfg, 0, sizeof(MI_VENC_RoiCfg_t));
        stVencRoiCfg.u32Index = 0;
        stVencRoiCfg.bAbsQp = FALSE;
        stVencRoiCfg.bEnable = TRUE;
        stVencRoiCfg.s32Qp = 7;
        stVencRoiCfg.stRect.u32Left = 0;
        stVencRoiCfg.stRect.u32Top = 528;
        stVencRoiCfg.stRect.u32Width = 960;
        stVencRoiCfg.stRect.u32Height = 528;

        s32Ret = MI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_SetRoiCfg %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#endif

        g_stVencThreadArgs[i].stVencAttr[0].vencChn = VencChn;
        g_stVencThreadArgs[i].stVencAttr[0].u32MainWidth =
            pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
        g_stVencThreadArgs[i].stVencAttr[0].u32MainHeight =
            pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
        g_stVencThreadArgs[i].stVencAttr[0].eType =
            pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType;
        g_stVencThreadArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);

        g_stVencThreadArgs[i].bRunFlag = TRUE;

        // printf("%s %d, venc:%d,fd:%d\n", __func__, __LINE__, VencChn, g_stVencThreadArgs[0].stVencAttr[i].vencFd);
        printf("%s %d, venc:%d\n", __func__, __LINE__, VencChn);

        // get es stream
        pthread_create(&g_stVencThreadArgs[i].ptGetEs, NULL, ST_VencGetEsProc, (void *)&g_stVencThreadArgs[i]);

        // put yuv data to venc
        pthread_create(&g_stVencThreadArgs[i].ptFillYuv, NULL, ST_VencFillYUVProc, (void *)&g_stVencThreadArgs[i]);

        #ifdef SUPPORT_UVC
            ST_UVCInit();
        #endif
    }

    return 0;
}

int main(int argc, char **argv)
{
    ST_Venc_ParseOptions(argc, argv);

    printf("source yuv info:  format=%d,width=%d,height=%d,file=%s\n",
        g_yuv_format, g_yuv_width, g_yuv_height, g_file_path);

    struct sigaction sigAction;
    char szCmd[16];
    MI_U32 u32Index = 0;
    int enterCount = 0;

    sigAction.sa_handler = ST_VENC_HandleSig;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	sigaction(SIGINT, &sigAction, NULL);

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    ST_VencUsage();

    while (!g_bExit)
    {
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

        ST_VencH26X();
    }

    return 0;
}

