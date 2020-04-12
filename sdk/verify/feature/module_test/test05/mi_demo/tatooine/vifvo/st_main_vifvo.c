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
#include "st_hdmi.h"
#include "st_common.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"

pthread_t pt;
static MI_BOOL _bThreadRunning = FALSE;

static MI_BOOL g_subExit = FALSE;
static MI_BOOL g_bExit = FALSE;
static MI_U32 g_u32SubCaseIndex = 0;

static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;

//Config logic chn trans to phy chn
ST_VifChnConfig_t stVifChnCfg[VIF_MAX_CHN_NUM] = {
    {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, //16main
    {0, 4, 0}, {0, 5, 0}, {0, 6, 0}, {0, 7, 0},
    {0, 8, 0}, {0, 9, 0}, {0, 10, 0}, {0, 11, 0},
    {0, 12, 0}, {0, 13, 0}, {0, 14, 0}, {0, 15, 0},
    {0, 0, 1}, {0, 1, 1}, {0, 2, 1}, {0, 3, 1}, //16sub
    {0, 4, 1}, {0, 5, 1}, {0, 6, 1}, {0, 7, 1},
    {0, 8, 1}, {0, 9, 1}, {0, 10, 1}, {0, 11, 1},
    {0, 12, 1}, {0, 13, 1}, {0, 14, 1}, {0, 15, 1},
};

ST_CaseDesc_t g_stVifCaseDesc[] =
{
    {
        .stDesc =
        {
            .u32CaseIndex = 0,
            .szDesc = "1x1080P@30 Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 8,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 1,
            .szDesc = "1xD1(720x576)@30 Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 8,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 2,
            .szDesc = "4x1080P@30 Video Capture",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 3,
            .szDesc = "4xD1(720x576)@30 Video Capture",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
#if 0
    {
        .stDesc =
        {
            .u32CaseIndex = 4,
            .szDesc = "1x1080P@25 + 7xD1(720x576)@25  Video Capture",
            .u32WndNum = 9,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 4,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 5,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 6,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 7,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            }
        },
    },
#endif
    {
        .stDesc =
        {
            .u32CaseIndex = 4,
            .szDesc = "9xD1(720x576)@25  Video Capture",
            .u32WndNum = 9,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 4,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 4,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 5,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 6,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 7,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 8,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            }
        },
    },
#if 1
    {
        .stDesc =
        {
            .u32CaseIndex = 5,
            .szDesc = "16x576P@25 Video Capture",
            .u32WndNum = 16,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 5,
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
                .szDesc = "show 9 channel",
                .u32WndNum = 16,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 4,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 5,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 6,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 7,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 8,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 9,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 10,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 11,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 12,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 13,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 14,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 15,
                        .u16CapWidth = 720,
                        .u16CapHeight = 576,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            }
        },
    },
#endif
    {
        .stDesc =
        {
            .u32CaseIndex = 6,
            .szDesc = "exit",
            .u32WndNum = 0,
        }
    }
};

void ST_VifUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 i = 0;

    for (i = 0; i < u32Size; i ++)
    {
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
    }
}

void ST_CaseSubUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
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

void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS, s32VoChannel = 0;
    MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = s32VoChannel;
    stChnPort.u32PortId = 0;
    printf("..st_GetOutputDataThread.s32VoChannel(%d)...\n", s32VoChannel);

    s32ReadCnt = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5); //Default queue frame depth--->20

    while (!_bThreadRunning)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            //if (0 != writefd)
            {
            //    write(writefd, stBufInfo.stFrameData.pVirAddr[0], size);
            }
            if (0 == (s32ReadCnt++ % 30))
                printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...\n", s32VoChannel, s32ReadCnt);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            if(stBufInfo.bEndOfStream)
                break;
        }
        usleep(10*1000);
    }
    printf("\n\n");
    usleep(3000000);

    return NULL;
}

int ST_SplitWindow()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
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
    VDEC->DIVP->VDISP->DISP

    (1) unbind VDEC DIVP
    (2) VDISP disable input port
    (3) divp set output port attr
    (4) vdisp set input port attr
    (5) bind vdec divp
    (6) enable vdisp input port

    VDEC->DIVP->VPE->VDISP->DISP
    (1) unbind vdec divp
    (2) unbind divp vpe
    (3) vdisp disable input port
    (4) divp set chn attr
    (5) vpe set port mode
    (6) vdisp set input port attr
    (7) bind vdec divp
    (8) bind divp vpe
    (9) enable vdisp input port
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
    }

    // 2, unbind DIVP to VPE
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    // 3, disable vdisp input port
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_VDISP_PORT vdispPort = 0;
    MI_VDISP_InputPortAttr_t stInputPortAttr;

#if 1
    // stop divp and vpe
    for (i = 0; i < u32LastWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StopChn(i));
    }

    for (i = 0; i < u32LastWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_DisablePort(vpeChn, 0));
    }
#endif

    for (i = 0; i < u32LastWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
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

    // 4, divp set output port chn attr
#if 0
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    for (i = 0; i < u32CurWndNum; i++)
    {
        divpChn = i;
        STCHECKRESULT(MI_DIVP_GetOutputPortAttr(divpChn, &stOutputPortAttr));
        printf("change divp from %dx%d ", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        stOutputPortAttr.u32Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stOutputPortAttr.u32Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        printf("to %dx%d\n", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }
#endif

    // 5, set vpe port mode
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }

    // 6, set vdisp input port attribute
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16DispHeight / u32Square, 2);

        printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
             stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }

    // start divp and vpe
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StartChn(i));
    }

    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_EnablePort(vpeChn, 0));
    }

    // 9, enable vdisp input port
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
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

    // 8, bind DIVP to VPE
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    pstCaseDesc[u32CaseIndex].u32ShowWndNum =
        pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;

    return 0;
}

MI_S32 ST_SubExit()
{
    MI_U32 u32WndNum = 0, u32ShowWndNum;
    MI_S32 i = 0;
    ST_Sys_BindInfo_t stBindInfo;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    u32WndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;
    u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].u32ShowWndNum;

    /************************************************
    step1:  unbind VIF to VPE
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = stVifChnCfg[i].u8ViDev;
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = stVifChnCfg[i].u8ViPort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step4:  unbind VPE to VDISP
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step5:  unbind VDISP TO DISP
    *************************************************/
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    /************************************************
    step5:  destroy vif  vpe vdisp disp
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        STCHECKRESULT(ST_Vif_StopPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort));
        STCHECKRESULT(ST_Vpe_StopPort(i, 0));
        STCHECKRESULT(ST_Vpe_StopChannel(i));
        STCHECKRESULT(ST_Vpe_DestroyChannel(i));

        STCHECKRESULT(ST_Vdisp_DisableInputPort(MI_VDISP_DEV_0, i));
    }

    STCHECKRESULT(ST_Vif_DisableDev(0));//0 1 2 3?

    STCHECKRESULT(ST_Vdisp_StopDevice(0));
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0 , 1)); //disp input port 0
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    STCHECKRESULT(ST_Vdisp_Exit());
    STCHECKRESULT(ST_Fb_DeInit());
    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

int ST_ChangeDisplayTiming()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
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
    (1), stop HDMI
    (2), stop VDISP
    (3), stop DISP
    (4), set vpe port mode
    (5), set vdisp input port chn attr
    (6), start disp
    (7), start vdisp
    (8), star HDMI
    */

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

    // stop hdmi
    ExecFunc(MI_HDMI_Stop(E_MI_HDMI_ID_0), MI_SUCCESS);

    // stop vdisp
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_S32 s32FrmRate = 30;
    MI_S32 s32OutputPort = 0;
    MI_VDISP_PORT vdispPort = 0;

    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Vdisp_StopDevice(vdispDev));

    // stop disp
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0, DISP_MAX_CHN));

    // set vpe port mode
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16CurDispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }
    // start vdisp
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
    }
    ST_Rect_t stVdispOutRect, stRect;
    stVdispOutRect.u16PicW = u16CurDispWidth;
    stVdispOutRect.u16PicH = u16CurDispHeight;
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev, s32OutputPort, &stVdispOutRect, s32FrmRate, 1));

    // set vdisp input port chn attr
    MI_VDISP_InputPortAttr_t stInputPortAttr;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16CurDispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16CurDispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16CurDispHeight / u32Square, 2);

        // printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
        //     stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
    }

    STCHECKRESULT(ST_Vdisp_StartDevice(vdispDev));

    // start disp
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, s32CurDispTiming));
    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    // start HDMI
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32CurHdmiTiming));

    pstCaseDesc[u32CaseIndex].eDispoutTiming = eCurDispoutTiming;

    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;
}

void ST_WaitSubCmd(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    char szCmd[16];
    MI_U32 index = 0;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;

    while (!g_subExit)
    {
        ST_CaseSubUsage();

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

MI_S32 ST_VifToDisp(MI_S32 s32CaseIndex)
{
    MI_VIF_DEV VifDevId = 0;
    MI_U16 u16DispLayerW = 1920;
    MI_U16 u16DispLayerH = 1080;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    ST_Rect_t stVdispOutRect = {0, 0, 1920, 1080};
    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_TimingType_e eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
    MI_DISP_OutputTiming_e eDispoutTiming = E_MI_DISP_OUTPUT_1080P60;
    MI_S32 s32CapChnNum = 0, s32DispChnNum = 0, i = 0;
    MI_S32 s32DevNum = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32Square = 0;
    MI_U32 u32SubCaseSize = pstCaseDesc[s32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    ST_Rect_t stDispWndRect[16] = {0};

    s32DispChnNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    s32CapChnNum = s32DispChnNum;
    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[s32CaseIndex].eDispoutTiming, (MI_S32 *)&eHdmiTiming,
        (MI_S32 *)&eDispoutTiming, (MI_U16*)&stVdispOutRect.u16PicW, (MI_U16*)&stVdispOutRect.u16PicH));

    if (u32WndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32WndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32WndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32WndNum <= 16)
    {
        u32Square = 4;
    }

    if (u32WndNum == 6) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stVdispOutRect.u16PicW / 3, 2);
        MI_U16 u16DivH = ALIGN_BACK(stVdispOutRect.u16PicH / 3, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 2, 2},
            {2, 0, 1, 1},
            {2, 1, 1, 1},
            {0, 2, 1, 1},
            {1, 2, 1, 1},
            {2, 2, 1, 1},
        };//3x3 split divX

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
    }
    else
    {
        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X    = ALIGN_BACK((stVdispOutRect.u16PicW / u32Square) * (i % u32Square), 2);
            stDispWndRect[i].s32Y    = ALIGN_BACK((stVdispOutRect.u16PicH / u32Square) * (i / u32Square), 2);
            stDispWndRect[i].u16PicW = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 2);
            stDispWndRect[i].u16PicH = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);
        }
    }
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init HDMI
    *************************************************/
    STCHECKRESULT(ST_Hdmi_Init());

    /************************************************
    Step3:  init VIF
    *************************************************/
    s32DevNum = s32CapChnNum / 4;
    if ((s32CapChnNum % 4) > 0)
    {
        s32DevNum += 1;
    }
    for (i = 0; i < s32DevNum; i++) //init vif device
    {
        STCHECKRESULT(ST_Vif_CreateDev(i));
    }
    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
        stVifPortInfoInfo.u32RectX = 0;
        stVifPortInfoInfo.u32RectY = 0;
        stVifPortInfoInfo.u32RectWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVifPortInfoInfo.u32RectHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;

        stVifPortInfoInfo.u32DestWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVifPortInfoInfo.u32DestHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;

        STCHECKRESULT(ST_Vif_CreatePort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort, &stVifPortInfoInfo));
        STCHECKRESULT(ST_Vif_StartPort(stVifChnCfg[i].u8ViDev, stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort));
    }

    /************************************************
    Step4:  init VPE
    *************************************************/
    for (i = 0; i < s32CapChnNum; i++)
    {
        stVpeChannelInfo.u16VpeMaxW = 1920;
        stVpeChannelInfo.u16VpeMaxH = 1080; // max res support to FHD
        stVpeChannelInfo.u32X = 0;
        stVpeChannelInfo.u32Y = 0;

        stVpeChannelInfo.u16VpeCropW = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVpeChannelInfo.u16VpeCropH = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        STCHECKRESULT(ST_Vpe_CreateChannel(i, &stVpeChannelInfo));

        stPortInfo.DepVpeChannel = i;
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stPortInfo.u16OutputWidth = stDispWndRect[i].u16PicW;
        stPortInfo.u16OutputHeight = stDispWndRect[i].u16PicH;

        STCHECKRESULT(ST_Vpe_CreatePort(DISP_PORT, &stPortInfo)); //default support port0 --->>> vdisp
        STCHECKRESULT(ST_Vpe_StartChannel(i));
    }

    /************************************************
    Step5:  init VDISP
    *************************************************/
    STCHECKRESULT(ST_Vdisp_Init()); //Vdisp Init
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(MI_VDISP_DEV_0, 0, &stVdispOutRect, 30, 1));
    STCHECKRESULT(ST_Vdisp_StartDevice(MI_VDISP_DEV_0));

    for (i = 0; i < s32DispChnNum; i++)
    {
        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(MI_VDISP_DEV_0, i, &stDispWndRect[i]));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(MI_VDISP_DEV_0, i));
    }

    /************************************************
    Step6:  init DISP
    *************************************************/
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, eDispoutTiming)); //Dispout timing

    // must init after disp
    ST_Fb_Init();
    ST_FB_Show(FALSE);

    /************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    for (i = 0; i < s32CapChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = stVifChnCfg[i].u8ViDev;
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = stVifChnCfg[i].u8ViPort;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = DISP_PORT;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    /************************************************
    Step8:  Bind VPE->VDISP
    *************************************************/
    for (i = 0; i < s32DispChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    /*******************************************************
    Step9:  Bind VDISP->DISP, k6l only support single window
    ********************************************************/
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    STCHECKRESULT(ST_Hdmi_Start(eHdmi, eHdmiTiming)); //Hdmi timing
    g_u32LastSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    g_u32CurSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[s32CaseIndex].u32ShowWndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    pthread_create(&pt, NULL, st_GetOutputDataThread, NULL);

    return MI_SUCCESS;
}

void ST_DealCase(int argc, char **argv)
{
    MI_U32 u32Index = 0;
    MI_U32 u32SubIndex = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;

    if (argc != 3)
    {
        return;
    }

    u32Index = atoi(argv[1]);
    u32SubIndex = atoi(argv[2]);

    if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))//case num
    {
        printf("case index range (%d~%d)\n", 1, ARRAY_SIZE(g_stVifCaseDesc));
        return;
    }
    g_u32CaseIndex = u32Index - 1;//real array index

    if (u32SubIndex <= 0 || u32SubIndex > pstCaseDesc[g_u32CaseIndex].u32SubCaseNum)
    {
        printf("sub case index range (%d~%d)\n", 1, pstCaseDesc[g_u32CaseIndex].u32SubCaseNum);
        return;
    }

    g_u32LastSubCaseIndex = pstCaseDesc[g_u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[g_u32CaseIndex].u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;

    printf("case index %d, sub case %d-%d\n", g_u32CaseIndex, g_u32CurSubCaseIndex, g_u32LastSubCaseIndex);

    ST_VifToDisp(g_u32CaseIndex);

    g_u32CurSubCaseIndex = u32SubIndex - 1;//select subIndex

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

MI_S32 main(int argc, char **argv)
{
    char szCmd[16];
    MI_U32 u32Index = 0;

    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    signal(SIGCHLD, SIG_IGN);

    ST_DealCase(argc, argv);
    while (!g_bExit)
    {
        ST_VifUsage();//case usage
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        u32Index = atoi(szCmd);

        if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))
        {
            continue;
        }
        g_u32CaseIndex = u32Index - 1;
        if (0 == (strncmp(g_stVifCaseDesc[g_u32CaseIndex].stDesc.szDesc, "exit", 4)))
        {
            return MI_SUCCESS;
        }
        ST_VifToDisp(g_u32CaseIndex);
        ST_WaitSubCmd();
    }

    return 0;
}
