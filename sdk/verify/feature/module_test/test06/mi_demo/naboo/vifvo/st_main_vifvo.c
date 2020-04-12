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
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <linux/input.h>

#include "mi_sys.h"
#include "mi_divp.h"
#include "mi_rgn.h"
#include "mi_od.h"
#include "mi_md.h"
#include "mi_vdf.h"

#include "st_hdmi.h"
#include "st_common.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"
#include "mi_venc.h"
#include "st_fb.h"

#include "i2c.h"

#include "AddaSysApi.h"

pthread_t pt;
static MI_BOOL _bThreadRunning = FALSE;

static MI_BOOL g_subExit = FALSE;
static MI_BOOL g_bExit = FALSE;
//static MI_U32 g_u32SubCaseIndex = 0;

static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;

#define VENC_H264_ALIGN_W   32
#define VENC_H264_ALIGN_H   8
#define VENC_H265_ALIGN_W   16
#define VENC_H265_ALIGN_H   8

#define MAX_VIF_DEV_NUM 4
#define MAX_VIF_CHN_NUM 16
#define SUPPORT_DIVP_DI 1
#define SUPPORT_VIDEO_ENCODE
#define SUPPORT_VIDEO_PREVIEW
#define SUPPORT_RGN
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

#define VENC_SUB_CHN_START  5
#define VPE_SUB_CHN_START   5

#define SUB_VENC_W 352
#define SUB_VENC_H 288
#define SUB_VENC_SUPPORT_FLAG 1
#define MD_OD_SUPPORT_FLAG 0 //Need (#define SUB_VENC_SUPPORT_FLAG 1)
#define VENC_MAX_FRM_CNT 20 //set 0 to use original setting
#define VENC_MAX_USER_FRM_CNT (VENC_MAX_FRM_CNT)

#define VENC_GET_STREAM_FLAG 1 //this must leave as 1 after merging branch LL due to blocked user buffer
#define VENC_GET_STREAM_WRITE (0)
#define VDA_CHN_W 352
#define VDA_CHN_H 288

//#define SUPPORT_UVC
#define USE_DISP_VGA 0

#define ADD_HEADER_ES

#define OD_START_PORT 8
#define OD_DIVP_OFFSET 8

#define MOUSE_DEV_NAME   "/dev/input/mice"

#ifdef SUPPORT_UVC
#include "mi_uvc.h"
#endif

#ifdef SUPPORT_RGN
#define MAX_RGN_W           8192
#define MAX_RGN_H           8192
#endif
#ifdef SUPPORT_DYNAMIC_DETECT_VIF
static pthread_t t_VifDetect = 0;
#endif
pthread_t g_mouse_user_pt;

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
    VENC_Attr_t stVencAttr[MAX_VIF_CHN_NUM];
    MI_U32 u32ChnNum;
    MI_BOOL bRunFlag;
} Venc_Args_t;

typedef struct
{
    pthread_t pt;
    pthread_t ptsnap;
    MI_VDEC_CHN vdecChn;
    char szFileName[64];
    MI_BOOL bRunFlag;
} VDEC_Thread_Args_t;

typedef struct
{
    pthread_t pt;
    MI_VDF_CHANNEL vdfChn;
    MI_BOOL bRunFlag;
    MI_VDF_WorkMode_e enWorkMode;
    MI_U16 u16Width;
    MI_U16 u16Height;
} VDF_Thread_Args_t;

typedef enum
{
    FB_DRAW_BEGIN = 0,
    FB_DRAW_ING,
    FB_DRAW_END,
} FB_DRAW_E;

Venc_Args_t g_stVencArgs[MAX_VIF_CHN_NUM * 2];
VDEC_Thread_Args_t g_stVdecThreadArgs[MAX_CHN_NUM];
VDF_Thread_Args_t g_stVdfThreadArgs[1024];
static MI_BOOL g_PushEsExit = FALSE;

typedef struct ST_ChnInfo_s
{
    MI_S32 s32VideoFormat; //720P 1080P ...
    MI_S32 s32VideoType; //CVI TVI AHD CVBS ...
    MI_S32 s32VideoLost;
    MI_U8 u8EnChannel;
    MI_U8 u8ViDev;
    MI_U8 u8ViChn;
    MI_U8 u8ViPort; //main or sub
} ST_ChnInfo_t;

typedef struct ST_TestInfo_s
{
    ST_ChnInfo_t stChnInfo[MAX_VIF_CHN_NUM];
} ST_TestInfo_t;

static ST_TestInfo_t stTestInfo;

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
            //string is too long
            //.szDesc = "4x1080P@30 4MainVenc+4SubVenc+4Vdec+1VirtualVenc+4Vdf 1080P@DISPOUT",
            .szDesc = "4x1080P@30 (4Main+4Sub+1Virt)Venc+4Vdec+4Vdf 1080P@DISP",
            .u32WndNum = 4,
            .bNeedVdisp = TRUE,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VDISP_CHN,
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
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 1,
            //.szDesc = "4x1080P@30 4MainVenc+4SubVenc+4Vdec+1VirtualVenc+4Vdf 4K@DISPOUT",
            .szDesc = "4x1080P@30 (4Main+4Sub+1Virt)Venc+4Vdec+4Vdf 4K@DISP",
            .u32WndNum = 4,
            .bNeedVdisp = TRUE,
        },
        .eDispoutTiming = E_ST_TIMING_3840x2160_30,
        .u32SubCaseNum = 3,
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
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 1,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 2,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN | E_ST_VDEC_CHN | E_ST_VDF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,

                        .s32FrmRateSub = E_MI_VIF_FRAMERATE_FULL,
                        .eSubType = E_MI_VENC_MODTYPE_H265E,

                        .u16VdfInWidth = 352,
                        .u16VdfInHeight = 288,
                        .u16OdNum = 1,
                        .stOdArea =
                        {
                            {
                                .s32X = 0,
                                .s32Y = 0,
                                .u16PicW = 352,
                                .u16PicH = 288,
                            },
                        },
                        .u16MdNum = 1,
                        .stMdArea =
                        {
                            {
                                 .s32X = 0,
                                 .s32Y = 0,
                                 .u16PicW = 352,
                                 .u16PicH = 288,

                            },
                        },
                    },
                    .stVdecChnArg =
                    {
                        .u32Chn = 3,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H265,
                        .szFilePath = PREFIX_PATH ST_1080P_H265_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VDISP_CHN,
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
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 2,
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
    MI_SYS_ChnPort_t stVencChnInputPort;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = 2;
    if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[g_u32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stVencChnInputPort.u32DevId = 4;
    }
    else if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[g_u32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stVencChnInputPort.u32DevId = 2;
    }
    stVencChnInputPort.u32ChnId = 0;
    stVencChnInputPort.u32PortId = 0;

    hHandle = MI_HANDLE_NULL;
    memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

    if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle)))
    {
        if(hHandle == MI_HANDLE_NULL)
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


        // len = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
        printf("send buf length : %d\n", stBufInfo.stRawData.u32ContentSize);
        *length = stBufInfo.stRawData.u32ContentSize;
        memcpy(buf, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);

        //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
        //    len, stBufInfo.stRawData.u32ContentSize);

        MI_SYS_ChnOutputPortPutBuf(hHandle);
    }
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

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 5);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth error, %X\n", s32Ret);
        return NULL;
    }

    s32Ms = 31;
    //printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (!g_PushEsExit)
    {
        usleep(s32Ms * 1000);
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

/* Mouse button bits*/
#define WHEEL_UP    0x10
#define WHEEL_DOWN  0x08

#define BUTTON_L    0x04
#define BUTTON_M    0x02
#define BUTTON_R    0x01
#define SCALE       1 /* default scaling factor for acceleration */
#define THRESH      1 /* default threshhold for acceleration */

static int xpos; /* current x position of mouse */
static int ypos; /* current y position of mouse */
static int minx; /* minimum allowed x position */
static int maxx; /* maximum allowed x position */
static int miny; /* minimum allowed y position */
static int maxy; /* maximum allowed y position */
// static int buttons; /* current state of buttons */

static int IMPS2_Read(int fd, int *dx, int *dy, int *dz, int *bp)
{
    static unsigned char buf[5];
    static int buttons[7] = {0, 1, 3, 0, 2, 0, 0};// 1:left button, 2: mid button, 3: right button
    static int nbytes = 0;
    int n;

    while ((n = read (fd, &buf [nbytes], 4 - nbytes)))
    {
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        nbytes += n;

        if (nbytes == 4)
        {
            int wheel;
            // printf("[luther.gliethttp]: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
            if ((buf[0] & 0xc0) != 0)
            {
                buf[0] = buf[1];
                buf[1] = buf[2];
                buf[2] = buf[3];
                nbytes = 3;
                return -1;
            }

            /* FORM XFree86 4.0.1 */
            *bp = buttons[(buf[0] & 0x07)];
            *dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
            *dy = (buf[0] & 0x20) ? -(buf[2] - 256) : -buf[2];

            /* Is a wheel event? */
            if ((wheel = buf[3]) != 0)
            {
                if(wheel > 0x7f)
                {
                    *bp |= WHEEL_UP;
                }
                else
                {
                    *bp |= WHEEL_DOWN;
                }
            }

            *dz = 0;
            nbytes = 0;
            return 1;
        }
    }

    return 0;
}

void mouse_setrange (int newminx, int newminy, int newmaxx, int newmaxy)
{
    minx = newminx;
    miny = newminy;
    maxx = newmaxx;
    maxy = newmaxy;
}

int mouse_update(int dx, int dy, int dz)
{
    int r;
    int sign;

    sign = 1;
    if (dx < 0)
    {
        sign = -1;
        dx = -dx;
    }

    if (dx > THRESH)
        dx = THRESH + (dx - THRESH) * SCALE;

    dx *= sign;
    xpos += dx;

    if( xpos < minx )
        xpos = minx;
    if( xpos > maxx )
        xpos = maxx;

    sign = 1;
    if (dy < 0)
    {
        sign = -1;
        dy = -dy;
    }

    if (dy > THRESH)
         dy = THRESH + (dy - THRESH) * SCALE;

    dy *= sign;
    ypos += dy;

    if (ypos < miny)
        ypos = miny;

    if (ypos > maxy)
        ypos = maxy;

    return 1;
}

void *ST_FBMouseUserProc(void *args)
{
    int fd,retval;
    fd_set readfds;
    struct input_event inputEv[64];
    int readlen = 0;
    int i = 0;
    unsigned char buf[32];
    int x_pos, y_pos;
    unsigned char imps2_param [] = {243, 200, 243, 100, 243, 80};
    int dx, dy, dz, button;
    FB_DRAW_E enDraw = FB_DRAW_END;
    int old_xpos, old_ypos;
    MI_SYS_WindowRect_t stRect;
    MI_U32 u32ColorIndex = 0;
    MI_U32 u32Color[] = {ARGB888_RED, ARGB888_GREEN, ARGB888_BLACK};
    int d_xpos, d_ypos = 0;

    do
    {
        fd = open(MOUSE_DEV_NAME, O_RDWR);
        if (fd < 0)
        {
            printf("can not open %s\n", MOUSE_DEV_NAME);
        }
        sleep (5);
    } while (fd < 0);

    printf("open %s success, fd:%d\n", MOUSE_DEV_NAME, fd);

    write(fd, imps2_param, sizeof (imps2_param));

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        retval = select(fd + 1, &readfds, NULL, NULL, NULL);
        if(retval == 0)
        {
            printf("Time out!\n");
            continue;
        }

        if(FD_ISSET(fd, &readfds))
        {
            IMPS2_Read(fd, &dx, &dy, &dz, &button);

            mouse_update(dx, dy, dz);

            if (button == 0x1)
            {
                // left button down
                if (enDraw == FB_DRAW_END)
                {
                    enDraw = FB_DRAW_BEGIN;
                    old_xpos = xpos;
                    old_ypos = ypos;
                }
                else if (enDraw == FB_DRAW_BEGIN)
                {
                    enDraw = FB_DRAW_ING;
                }
            }
            else
            {
                // left button up
                enDraw = FB_DRAW_END;
            }

            if (button == 0x3)
            {
                // right button down
                u32ColorIndex ++;
            }

            if (enDraw == FB_DRAW_BEGIN ||
                enDraw == FB_DRAW_ING)
            {
                d_xpos = abs(xpos - old_xpos);
                d_ypos = abs(ypos - old_ypos);
                if (d_xpos > 0)
                {
                    for (i = MIN(xpos, old_xpos); i < MAX(xpos, old_xpos); i ++)
                    {
                        ST_Fb_FillCircle(i, ypos, 10, u32Color[u32ColorIndex % ARRAY_SIZE(u32Color)]);
                    }
                }

                if (d_ypos > 0)
                {
                    for (i = MIN(ypos, old_ypos); i < MAX(ypos, old_ypos); i ++)
                    {
                        ST_Fb_FillCircle(xpos, i, 10, u32Color[u32ColorIndex % ARRAY_SIZE(u32Color)]);
                    }
                }
            }

            // printf("xpos:%d,ypos:%d, button:0x%X\n", xpos, ypos, button);
            ST_Fb_MouseSet(xpos, ypos);
            old_xpos = xpos;
            old_ypos = ypos;
        }
    }

    close(fd);

    return NULL;
}

void *ST_VifDetectProcess(void *args)
{
    AD_DETECT_STATUS_S ad_status;
    MI_S32 i = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32WndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;
    ST_VIF_PortInfo_t stPortInfo;

    while (1)
    {
        usleep(1000*1000);
        for (i = 0; i < u32WndNum; i++)
        {
            Adda_GetVideoInStatus(i, &ad_status);//0-15 ???

            //ST_INFO("\nchn %d vfmt: %d, lost: %d, report fmt: %d, sigtype: %d\n", i,
            //    ad_status.iVideoFormat, ad_status.iLostStatus, ad_status.iReportFormat, ad_status.iVideoSignalType);
            if (1 == ad_status.iLostStatus)
            {
                ST_INFO("Video lost Chn(%d)!!!\n", i);
                continue;
            }
            //printf("xxxxxxxxxxlast(%d) current(%d)...\n", stTestInfo.stChnInfo[i].s32VideoFormat, ad_status.iVideoFormat);
            if (stTestInfo.stChnInfo[i].s32VideoFormat != ad_status.iVideoFormat)
            {
                switch (ad_status.iVideoFormat)
                {
                    memset(&stPortInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
                    case AD_VIDEO_IN_HD_1080P_25HZ:
                    case AD_VIDEO_IN_HD_1080P_30HZ:
                        stPortInfo.u32RectX = 0;
                        stPortInfo.u32RectY = 0;
                        stPortInfo.u32RectWidth = 1920;
                        stPortInfo.u32RectHeight = 1080;
                        stPortInfo.u32DestWidth = 1920;
                        stPortInfo.u32DestHeight = 1080;
                        stPortInfo.u32IsInterlace = FALSE;
                        printf("Video input change to 1080P...\n");
                        break;
                    case AD_VIDEO_IN_HD_720P_25HZ:
                    case AD_VIDEO_IN_HD_720P_30HZ:
                        stPortInfo.u32RectX = 0;
                        stPortInfo.u32RectY = 0;
                        stPortInfo.u32RectWidth = 1280;
                        stPortInfo.u32RectHeight = 720;
                        stPortInfo.u32DestWidth = 1280;
                        stPortInfo.u32DestHeight = 720;
                        stPortInfo.u32IsInterlace = FALSE;
                        printf("Video input change to 720P...\n");
                        break;
                    case AD_VIDEO_IN_SD_PAL:
                    case AD_VIDEO_IN_SD_NTSC:
                    case AD_VIDEO_IN_HD_2560x1440_25HZ:
                    case AD_VIDEO_IN_HD_2560x1440_30HZ:
                    case AD_VIDEO_IN_HD_3840x2160_15HZ:
                    default:
                        break;
                }
                ST_Vif_StopPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort);
                ST_Vif_CreatePort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort, &stPortInfo);
                ST_Vif_StartPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort);
                stTestInfo.stChnInfo[i].s32VideoFormat = ad_status.iVideoFormat;
            }
        }
    }
}

void *ST_VDFGetResult(void *args)
{
    VDF_Thread_Args_t *pstArgs = (VDF_Thread_Args_t *)args;
    MI_VDF_Result_t stVdfResult;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U8* pu8MdRstData = NULL;
    MI_VDF_CHANNEL vdfChn = pstArgs->vdfChn;
    MI_U32 col, row, buffer_size;
    MI_U32 iii = 0;

    if (pstArgs->enWorkMode == E_MI_VDF_WORK_MODE_MD)
        ST_DBG("Get md result, chn:%d\n", vdfChn);
    else
        ST_DBG("Get od result, chn:%d\n", vdfChn);

    while(pstArgs->bRunFlag)
    {
        memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 0;
		stVdfResult.enWorkMode = pstArgs->enWorkMode;
		s32Ret = MI_VDF_GetResult(vdfChn, &stVdfResult, 0);
		if((0 == s32Ret) &&
           ((1 == stVdfResult.unVdfResult.stMdResult.u8Enable) ||
            (1 == stVdfResult.unVdfResult.stOdResult.u8Enable)))
		{
		    if (pstArgs->enWorkMode == E_MI_VDF_WORK_MODE_MD)
            {
                pu8MdRstData = (MI_U8*)stVdfResult.unVdfResult.stMdResult.pstMdResultSad;
    			printf("[MD_TEST][HDL=0xA0] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                    stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                    stVdfResult.enWorkMode,                      	\
                                    stVdfResult.unVdfResult.stMdResult.u8Enable);

                col = (pstArgs->u16Width >> 1) >> (1 + 2);
    		    row = pstArgs->u16Height >> (1 + 2);
    		    buffer_size = col * row * (2 - 1);

    			for(iii = 1; iii < buffer_size; iii++)
    			{
    				//if(0 == iii%16) printf("\n");
    				//else if(0 == iii%8) printf(" ");
    				//else printf("0x%02x ", *pu8MdRstData++);
    			}
            }
            else
            {
                printf("[OD_TEST][HDL=02] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
    								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
    								stVdfResult.enWorkMode, 						\
    								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
    								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
    								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
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
            }

			MI_VDF_PutResult(vdfChn, &stVdfResult);
		}
        else
        {
            // printf("[MD_TEST][HDL=0x0] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, s32Ret);
        }

        usleep(1000*10);
    }
}

#define DISK0_PATH_PREFIX ""
#define DISK1_PATH_PREFIX ""

void *ST_VencGetEsBufferProc(void *args)
{
    Venc_Args_t *pArgs = (Venc_Args_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    char szFileName[128];
    int fd = -1;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_U32 u32DevId = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack0;
    MI_VENC_ChnStat_t stStat;

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;

    s32Ret = MI_VENC_GetChnDevid(pArgs->stVencAttr[0].vencChn, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n",
            __func__, __LINE__, pArgs->stVencAttr[0].vencChn, s32Ret);
    }
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
    stVencChnInputPort.u32PortId = 0;

    memset(szFileName, 0, sizeof(szFileName));
	if (pArgs->stVencAttr[0].vencChn % 2)
	{
		len = snprintf(szFileName, sizeof(szFileName) - 1, DISK0_PATH_PREFIX"venc_dev%d_chn%d_port%d_%dx%d.",
			stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
			pArgs->stVencAttr[0].u32MainWidth, pArgs->stVencAttr[0].u32MainHeight);
	}
	else
	{
		len = snprintf(szFileName, sizeof(szFileName) - 1, DISK1_PATH_PREFIX"venc_dev%d_chn%d_port%d_%dx%d.",
			stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
			pArgs->stVencAttr[0].u32MainWidth, pArgs->stVencAttr[0].u32MainHeight);
	}

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

    printf("%s %d create %s success\n", __func__, __LINE__, szFileName);
    while (1)
    {
        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack0, 0, sizeof(stPack0));
        stStream.pstPack = &stPack0;
        stStream.u32PackCount = 1;
        s32Ret = MI_VENC_Query(pArgs->stVencAttr[0].vencChn, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            usleep(10 * 1000);
            continue;
        }
        s32Ret = MI_VENC_GetStream(pArgs->stVencAttr[0].vencChn, &stStream, 40);

        if (MI_SUCCESS == s32Ret)
        {
#if 0
        if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle)))
        {
            if(hHandle == MI_HANDLE_NULL)
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
#endif
            if(0)//if(stStream.u32Seq % 10 == 9)
            {
                printf("%s %d, chn:%d write frame len=%d offset = %d, u32Seq:%d, u32PackCount:%d\n", __func__, __LINE__,
                       stVencChnInputPort.u32ChnId,
                       stStream.pstPack[0].u32Len, stStream.pstPack[0].u32Offset, stStream.u32Seq,
                       stStream.u32PackCount);
            }

#if VENC_GET_STREAM_WRITE
            len = write(fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
            if (len != stStream.pstPack[0].u32Len)
            {
                printf("write es buffer fail.\n");
            }
#endif
            if (MI_SUCCESS != (s32Ret = MI_VENC_ReleaseStream(pArgs->stVencAttr[0].vencChn, &stStream)))
            {
                printf("%s %d, MI_VENC_ReleaseStream error, %X\n", __func__, __LINE__, s32Ret);
            }
        }
        else
        {
            if ((MI_ERR_VENC_NOBUF != s32Ret) && (MI_ERR_SYS_NOBUF != s32Ret))
            {
                printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X %x\n", __func__, __LINE__, s32Ret, MI_ERR_VENC_BUF_EMPTY);
            }
            usleep(10 * 1000);
        }
    }

    close(fd);
}

void *st_DumpAddaRegisterThread(void * args)
{
    while (!_bThreadRunning)
    {
        Adda_DumpReg(0);
        usleep(10*1000*1000);
    }
    printf("\n\n");

    return NULL;
}

#if 0
void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS, s32VoChannel = 0;
    MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;
    FILE *fp = NULL;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = s32VoChannel;
    stChnPort.u32PortId = 2;
    printf("..st_GetOutputDataThread.s32VoChannel(%d)...\n", s32VoChannel);

    s32ReadCnt = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3); //Default queue frame depth--->20
    fp = fopen("vpe_2.yuv","wb");
    while (!_bThreadRunning)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == stBufInfo.stFrameData.ePixelFormat)
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u16Width;

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                        fwrite(stBufInfo.stFrameData.pVirAddr[1], size/2, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
            else
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
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
#endif

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
    MI_DISP_InputPortAttr_t stInputPortAttr;
    printf("ST_SplitWindow line(%d)...\n", __LINE__);

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }
    printf("ST_SplitWindow line(%d)...\n", __LINE__);
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

    // unbind VPE to DISP
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = ST_DISP_LAYER0;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

        #if USE_DISP_VGA
        stBindInfo.stDstChnPort.u32DevId = ST_DISP_LAYER1;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        #endif
    }
    // Destroy disp channel
    for (i = 0; i < u32LastWndNum; i++)
    {
        STCHECKRESULT(MI_DISP_DisableInputPort(ST_DISP_LAYER0, i));
        #if USE_DISP_VGA
        STCHECKRESULT(MI_DISP_DisableInputPort(ST_DISP_LAYER1, i));
        #endif
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
        ExecFunc(MI_VPE_GetPortMode(vpeChn, DISP_PORT, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, DISP_PORT, &stVpeMode), MI_VPE_OK);
    }

    // 6, set vdisp input port attribute
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

        ExecFunc(MI_DISP_GetInputPortAttr(ST_DISP_LAYER0, i, &stInputPortAttr), MI_SUCCESS);
        stInputPortAttr.stDispWin.u16X      = ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.stDispWin.u16Y      = ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.stDispWin.u16Width  = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stInputPortAttr.stDispWin.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);

        ExecFunc(MI_DISP_SetInputPortAttr(ST_DISP_LAYER0, i, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetInputPortAttr(ST_DISP_LAYER0, i, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableInputPort(ST_DISP_LAYER0, i), MI_SUCCESS);
        ExecFunc(MI_DISP_SetInputPortSyncMode(ST_DISP_LAYER0, i, E_MI_DISP_SYNC_MODE_FREE_RUN), MI_SUCCESS);
#if USE_DISP_VGA
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

        ExecFunc(MI_DISP_GetInputPortAttr(ST_DISP_LAYER1, i, &stInputPortAttr), MI_SUCCESS);
        stInputPortAttr.stDispWin.u16X      = ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.stDispWin.u16Y      = ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.stDispWin.u16Width  = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stInputPortAttr.stDispWin.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);

        ExecFunc(MI_DISP_SetInputPortAttr(ST_DISP_LAYER1, i, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetInputPortAttr(ST_DISP_LAYER1, i, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableInputPort(ST_DISP_LAYER1, i), MI_SUCCESS);
        ExecFunc(MI_DISP_SetInputPortSyncMode(ST_DISP_LAYER1, i, E_MI_DISP_SYNC_MODE_FREE_RUN), MI_SUCCESS);
#endif
    }

    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_EnablePort(vpeChn, 0));
    }

    // bind VPE to DISP
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = ST_DISP_LAYER0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        #if USE_DISP_VGA
        stBindInfo.stDstChnPort.u32DevId = ST_DISP_LAYER1;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        #endif
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
    for (i = 0; i < u32WndNum; i++)
    {
        STCHECKRESULT(ST_Vif_StopPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort));
    }
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

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }
#if USE_DISP_VGA
    for (i = 0; i < u32ShowWndNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
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
    step5:  destroy vif  vpe vdisp disp
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        STCHECKRESULT(ST_Vpe_StopPort(i, 0));
        STCHECKRESULT(ST_Vpe_StopChannel(i));
        STCHECKRESULT(ST_Vpe_DestroyChannel(i));
    }

    STCHECKRESULT(ST_Vif_DisableDev(0));//0 1 2 3?

    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0 , u32ShowWndNum));
#if USE_DISP_VGA
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV1, 1 , u32ShowWndNum));
#endif
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
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
    ST_Rect_t stdispRect, stRect;
    stdispRect.u16PicW = u16CurDispWidth;
    stdispRect.u16PicH = u16CurDispHeight;
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev, s32OutputPort, &stdispRect, s32FrmRate, 1));

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
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32CurDispTiming));
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

MI_S32 ST_CreateVencChannel(MI_S32 s32VencChn, MI_S32 s32VencType, MI_U16 u16Width, MI_U16 u16Height, MI_S32 s32FrameRate)
{
#ifdef SUPPORT_VIDEO_ENCODE //Create Video encode Channel
    // main+sub venc
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;
    MI_U32 u32DevId = 0, s32Ret;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

    //printf("%s %d, chn:%d,eType:%d\n", __func__, __LINE__, VencChn,
    //    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType);
    if (E_MI_VENC_MODTYPE_H264E == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
    #if 0
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum =
            pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
		stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

        stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
    #else
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*2;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 25;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = s32FrameRate;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
    #endif
    }
    else if (E_MI_VENC_MODTYPE_H265E == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = s32FrameRate;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 25;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 25;
    }
    else if (E_MI_VENC_MODTYPE_JPEGE == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = ALIGN_N(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_N(u16Height, VENC_H264_ALIGN_H);
    }
    s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
    }
    if (E_MI_VENC_MODTYPE_JPEGE == s32VencType)
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

#if VENC_MAX_FRM_CNT
    s32Ret = MI_VENC_SetMaxStreamCnt(VencChn, VENC_MAX_FRM_CNT);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_SetMaxStreamCnt %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
    }
#else
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
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 4);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
    }
#endif

    s32Ret = MI_VENC_StartRecvPic(VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
    }
#endif
    return 0;
}

MI_S32 ST_DestroyVencChannel(MI_S32 s32VencChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_VENC_StopRecvPic(s32VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    s32Ret |= MI_VENC_DestroyChn(s32VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }

    return s32Ret;
}

MI_S32 ST_CreateDivpChannel(MI_DIVP_CHN DivpChn, MI_U16 u16InputWidth, MI_U16 u16InputHeight, MI_U16 u16OutputWidth, MI_U16 u16OutputHeight, MI_BOOL bDiFlag)
{
    MI_DIVP_ChnAttr_t stAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    memset(&stAttr, 0, sizeof(stAttr));

    stAttr.bHorMirror = false;
    stAttr.bVerMirror = false;
    if (bDiFlag)
    {
        stAttr.eDiType = E_MI_DIVP_DI_TYPE_3D;
    }
    else
    {
        stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    }
    stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_MIDDLE;
    stAttr.stCropRect.u16X = 0;
    stAttr.stCropRect.u16Y = 0;
    stAttr.stCropRect.u16Width = u16InputWidth;
    stAttr.stCropRect.u16Height = u16InputHeight;
    stAttr.u32MaxWidth = u16InputWidth;
    stAttr.u32MaxHeight = u16InputHeight * 2;
    ExecFunc(MI_DIVP_CreateChn(DivpChn, &stAttr), MI_DISP_SUCCESS);

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    u16OutputWidth = ALIGN_UP(u16OutputWidth,32);
    stOutputPortAttr.u32Width = u16OutputWidth;
    stOutputPortAttr.u32Height = u16OutputHeight;
    ExecFunc(MI_DIVP_SetOutputPortAttr(DivpChn, &stOutputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DIVP_StartChn(DivpChn), MI_DISP_SUCCESS);

    return 0;
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
                MI_VENC_ChnAttr_t stChnAttr;
                MI_VENC_GetChnAttr(0, &stChnAttr);
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 20;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
                MI_VENC_SetChnAttr(0, &stChnAttr);
                printf("xxxxxxxxxxxxxxxxtype(%d)\n", stChnAttr.stRcAttr.eRcMode);
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "hide", 4)))
            {
                ST_Disp_ShowStatus(0, 0, FALSE);//HDMI
                #if USE_DISP_VGA
                ST_Disp_ShowStatus(1, 0, FALSE);//VGA
                #endif
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "show", 4)))
            {
                ST_Disp_ShowStatus(0, 0, TRUE);
                #if USE_DISP_VGA
                ST_Disp_ShowStatus(1, 0, TRUE);
                #endif
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "testvenc", 8)))
            {
                ST_DestroyVencChannel(0);
                ST_CreateVencChannel(0, E_MI_VENC_MODTYPE_H264E, 1280, 720, 25);
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
    ST_Rect_t stdispRect = {0, 0, 1920, 1080};
    ST_Rect_t stvdispRect = {0, 0, 1920, 1080};
    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_TimingType_e eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
    MI_DISP_OutputTiming_e eDispoutTiming = E_MI_DISP_OUTPUT_1080P60;
    MI_S32 s32CapChnNum = 0, s32DispChnNum = 0, i = 0, j =0;
    MI_S32 s32DevNum = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32Square = 0;
    MI_U32 u32SubCaseSize = pstCaseDesc[s32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    ST_Rect_t stDispWndRect[16] = {0};
    ST_Rect_t stVdispWndRect[16] = {0};
    MI_S32 s32AdId[4];
    MI_S32 s32AdWorkMode = 0;
    MI_U32 u32VencNum = 0;
    MI_S32 s32Ret = 0;
    MI_VDEC_CHN vdecChn = 0;
    MI_VDEC_ChnAttr_t stVdecChnAttr;

    s32DispChnNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    s32CapChnNum = s32DispChnNum;
    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[s32CaseIndex].eDispoutTiming, (MI_S32 *)&eHdmiTiming,
        (MI_S32 *)&eDispoutTiming, (MI_U16*)&stdispRect.u16PicW, (MI_U16*)&stdispRect.u16PicH));
    for (i = 0; i < 4; i++)
    {
        s32AdId[i] = 0;
    }
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

    if (6 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 3, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 3, 2);
        MI_U16 u16VdispDivW = ALIGN_BACK(stvdispRect.u16PicW / 3, 2);
        MI_U16 u16VdispDivH = ALIGN_BACK(stvdispRect.u16PicH / 3, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 2, 2},
            {2, 0, 1, 1},
            {2, 1, 1, 1},
            {0, 2, 1, 1},
            {1, 2, 1, 1},
            {2, 2, 1, 1},
        };//3x3 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
        for (i = 0; i < u32WndNum; i++)
        {
            stVdispWndRect[i].s32X = stRectSplit[i].s32X * u16VdispDivW;
            stVdispWndRect[i].s32Y = stRectSplit[i].s32Y * u16VdispDivH;
            stVdispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16VdispDivW;
            stVdispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16VdispDivH;
        }
    }
    else if (8 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 4, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 4, 2);
        MI_U16 u16VdispDivW = ALIGN_BACK(stvdispRect.u16PicW / 3, 2);
        MI_U16 u16VdispDivH = ALIGN_BACK(stvdispRect.u16PicH / 3, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 3, 3},
            {3, 0, 1, 1},
            {3, 1, 1, 1},
            {3, 2, 1, 1},
            {0, 3, 1, 1},
            {1, 3, 1, 1},
            {2, 3, 1, 1},
            {3, 3, 1, 1},
        };//4x4 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
        for (i = 0; i < u32WndNum; i++)
        {
            stVdispWndRect[i].s32X = stRectSplit[i].s32X * u16VdispDivW;
            stVdispWndRect[i].s32Y = stRectSplit[i].s32Y * u16VdispDivH;
            stVdispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16VdispDivW;
            stVdispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16VdispDivH;
        }
    }
    else
    {
        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X    = ALIGN_BACK((stdispRect.u16PicW / u32Square) * (i % u32Square), 2);
            stDispWndRect[i].s32Y    = ALIGN_BACK((stdispRect.u16PicH / u32Square) * (i / u32Square), 2);
            stDispWndRect[i].u16PicW = ALIGN_BACK((stdispRect.u16PicW / u32Square), 2);
            stDispWndRect[i].u16PicH = ALIGN_BACK((stdispRect.u16PicH / u32Square), 2);
        }
        for (i = 0; i < u32WndNum; i++)
        {
            stVdispWndRect[i].s32X    = ALIGN_BACK((stvdispRect.u16PicW / u32Square) * (i % u32Square), 2);
            stVdispWndRect[i].s32Y    = ALIGN_BACK((stvdispRect.u16PicH / u32Square) * (i / u32Square), 2);
            stVdispWndRect[i].u16PicW = ALIGN_BACK((stvdispRect.u16PicW / u32Square), 2);
            stVdispWndRect[i].u16PicH = ALIGN_BACK((stvdispRect.u16PicH / u32Square), 2);
        }
    }
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    ExecFunc(vif_i2c_init(), 0);
    /************************************************
    Step2:  init HDMI
    *************************************************/
    STCHECKRESULT(ST_Hdmi_Init());

#ifdef SUPPORT_RGN
    MI_RGN_PaletteTable_t stPaletteTable;

    memset(&stPaletteTable, 0, sizeof(MI_RGN_PaletteTable_t));

    stPaletteTable.astElement[0].u8Alpha = 0;
    stPaletteTable.astElement[0].u8Red = 255;
    stPaletteTable.astElement[0].u8Green = 255;
    stPaletteTable.astElement[0].u8Blue = 255;
    s32Ret = MI_RGN_Init(&stPaletteTable);
    if (MI_RGN_OK != s32Ret)
    {
        ST_ERR("MI_RGN_Init error, %X\n", s32Ret);
        return 1;
    }
#endif

    /************************************************
    Step3:  init VIF
    *************************************************/
    s32DevNum = s32CapChnNum / 4;

    switch (s32CaseIndex)
    {
        case 0:
            for (i = 0; i < 4; i++)
            {
                s32AdId[i] = 1;
                stVifChnCfg[i].u8ViDev = 0;
                stVifChnCfg[i].u8ViChn = 4 * i; //0 4 8 12
                stTestInfo.stChnInfo[i].s32VideoFormat = AD_VIDEO_IN_HD_1080P_25HZ;
            }
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        default:
            ST_DBG("Unkown test case(%d)!\n", s32CaseIndex);
            return 0;
    }
    for (i = 0; i < MAX_VIF_DEV_NUM; i++) //init vif device
    {
        if (s32AdId[i] && (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN))
        {
            ST_DBG("ST_Vif_CreateDev....DEV(%d)...s32AdWorkMode(%d)...\n", i, s32AdWorkMode);
            STCHECKRESULT(ST_Vif_CreateDev(i, s32AdWorkMode));
        }
    }
    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
            stVifPortInfoInfo.u32RectX = 0;
            stVifPortInfoInfo.u32RectY = 0;
            stVifPortInfoInfo.u32RectWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32RectHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            stVifPortInfoInfo.u32DestWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32DestHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            if (288 == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight)
            {
                stVifPortInfoInfo.u32IsInterlace = TRUE;
            }
            else
            {
                stVifPortInfoInfo.u32IsInterlace = FALSE;
            }
            STCHECKRESULT(ST_Vif_CreatePort(stVifChnCfg[i].u8ViChn, 0, &stVifPortInfoInfo));

            ST_NOP("============vif channel(%d) x(%d)-y(%d)-w(%d)-h(%d)..\n", i, stVifPortInfoInfo.u32RectX, stVifPortInfoInfo.u32RectY,
                stVifPortInfoInfo.u32RectWidth, stVifPortInfoInfo.u32RectHeight);
        }
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            vdecChn = i;
            memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
            stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
            stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
            stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
            stVdecChnAttr.u32PicWidth   = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stVdecChnAttr.u32PicHeight  = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
            stVdecChnAttr.u32Priority   = 0;
            stVdecChnAttr.eCodecType    = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.eCodecType;

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

            ST_DBG("start vdec chn %d success\n", vdecChn);
        }
    }

    /************************************************
    *   Init Divp
    *************************************************/
    MI_U16 u16DivpOutHeight = 0;
    MI_BOOL bDiFlag = FALSE;
    for (i = 0; i < s32CapChnNum; i++)
    {
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            if (SUPPORT_DIVP_DI && (288 == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight))
            {
                bDiFlag = TRUE;
                u16DivpOutHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight * 2;
                ST_CreateDivpChannel(i, pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth,
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight,
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth,
                    u16DivpOutHeight, bDiFlag);
            }
            else
            {
                bDiFlag = FALSE;
                u16DivpOutHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            }
        }
    }
    /************************************************
    Step4:  init VPE
    *************************************************/
    MI_U16 u16VpeInHeight = 0;
    MI_SYS_ChnPort_t stVpeChnOutputPort;
    MI_VPE_CHANNEL vpeChn = 0;

    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;

    for (i = 0; i < s32CapChnNum; i++)
    {
        stVpeChannelInfo.u16VpeMaxW = 1920;
        stVpeChannelInfo.u16VpeMaxH = 1080; // max res support to FHD
        stVpeChannelInfo.u32X = 0;
        stVpeChannelInfo.u32Y = 0;
        stVpeChannelInfo.u16VpeCropW = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        if (SUPPORT_DIVP_DI && (288 == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight))
        {
            u16VpeInHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight * 2;
        }
        else
        {
            u16VpeInHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        }
        stVpeChannelInfo.u16VpeCropH = u16VpeInHeight;
        STCHECKRESULT(ST_Vpe_CreateChannel(i, &stVpeChannelInfo));
        STCHECKRESULT(ST_Vpe_StartChannel(i));

        stPortInfo.DepVpeChannel = i;

#ifdef SUPPORT_VIDEO_PREVIEW
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stPortInfo.u16OutputWidth = stDispWndRect[i].u16PicW;
        stPortInfo.u16OutputHeight = stDispWndRect[i].u16PicH;
        STCHECKRESULT(ST_Vpe_CreatePort(DISP_PORT, &stPortInfo));
        stVpeChnOutputPort.u32DevId = 0;
        stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort.u32ChnId = i;
        stVpeChnOutputPort.u32PortId = DISP_PORT;
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, 1, 4); // For Loop disp
#endif

#ifdef SUPPORT_VIDEO_ENCODE
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stPortInfo.u16OutputWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H264_ALIGN_W);
            stPortInfo.u16OutputHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H264_ALIGN_H);
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stPortInfo.u16OutputWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H265_ALIGN_W);
            stPortInfo.u16OutputHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H265_ALIGN_H);
        }
        else
        {
            stPortInfo.u16OutputWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stPortInfo.u16OutputHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
        }
        STCHECKRESULT(ST_Vpe_CreatePort(MAIN_VENC_PORT, &stPortInfo)); //Main Venc
        if (SUB_VENC_SUPPORT_FLAG)
        {
            if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
            {
                stPortInfo.u16OutputWidth =
                    ALIGN_N(SUB_VENC_W, VENC_H264_ALIGN_W);
                stPortInfo.u16OutputHeight =
                    ALIGN_N(SUB_VENC_H, VENC_H264_ALIGN_H);
            }
            else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
            {
                stPortInfo.u16OutputWidth =
                    ALIGN_N(SUB_VENC_W, VENC_H265_ALIGN_W);
                stPortInfo.u16OutputHeight =
                    ALIGN_N(SUB_VENC_H, VENC_H265_ALIGN_H);
            }
            else
            {
                stPortInfo.u16OutputWidth =
                    ALIGN_N(SUB_VENC_W, 32);
                stPortInfo.u16OutputHeight =
                    ALIGN_N(SUB_VENC_H, 32);
            }
            STCHECKRESULT(ST_Vpe_CreatePort(SUB_VENC_PORT, &stPortInfo)); //Sub Venc
            #if 0
            stVpeChnOutputPort.u32DevId = 0;
            stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
            stVpeChnOutputPort.u32ChnId = i;
            stVpeChnOutputPort.u32PortId = SUB_VENC_PORT;
            s32Ret = MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, 1, 4); //
            #endif
        }
        if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
        {
            stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stPortInfo.u16OutputWidth = stVdispWndRect[i].u16PicW;
            stPortInfo.u16OutputHeight = stVdispWndRect[i].u16PicH;
            ST_DBG("vpe %d:%d for vdisp, width:%d,height:%d\n",  stPortInfo.DepVpeChannel, VDISP_PORT,
                stPortInfo.u16OutputWidth,
                stPortInfo.u16OutputHeight);
            STCHECKRESULT(ST_Vpe_CreatePort(VDISP_PORT, &stPortInfo)); //Virtual Venc
        }
#endif
        ST_NOP("============vpe channel(%d) x(%d)-y(%d)-w(%d)-h(%d).outw(%d)-outh(%d).\n", i, stVpeChannelInfo.u32X, stVpeChannelInfo.u32Y,
            stVpeChannelInfo.u16VpeCropW, stVpeChannelInfo.u16VpeCropH, stPortInfo.u16OutputWidth, stPortInfo.u16OutputHeight);

        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
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
            stDivpChnAttr.stCropRect.u16Width   = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stDivpChnAttr.stCropRect.u16Height  = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
            stDivpChnAttr.u32MaxWidth           = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stDivpChnAttr.u32MaxHeight          = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;

            ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
            ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

            memset(&stDivpOutputPortAttr, 0, sizeof(stDivpOutputPortAttr));
            stDivpOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
            stDivpOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
            stDivpOutputPortAttr.u32Width           = stDispWndRect[i].u16PicW;;
            stDivpOutputPortAttr.u32Height          = stDispWndRect[i].u16PicH;

            STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stDivpOutputPortAttr));
            ST_DBG("create divp %d MAX:%dx%d, OUT:%dx%d success\n", divpChn,
                stDivpChnAttr.u32MaxWidth, stDivpChnAttr.u32MaxHeight,
                stDivpOutputPortAttr.u32Width, stDivpOutputPortAttr.u32Height);
        }
        //Scaling down vpe frame data to 352*288
        if ((pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDF_CHN) &&
            (SUB_VENC_SUPPORT_FLAG) && (MD_OD_SUPPORT_FLAG))
        {
            divpChn = i + OD_DIVP_OFFSET;
            memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
            stDivpChnAttr.bHorMirror            = FALSE;
            stDivpChnAttr.bVerMirror            = FALSE;
            stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
            stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
            stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
            stDivpChnAttr.stCropRect.u16X       = 0;
            stDivpChnAttr.stCropRect.u16Y       = 0;
            stDivpChnAttr.stCropRect.u16Width   = SUB_VENC_W;//Vpe sub venc output size 704*576
            stDivpChnAttr.stCropRect.u16Height  = SUB_VENC_H;
            stDivpChnAttr.u32MaxWidth           = SUB_VENC_W;
            stDivpChnAttr.u32MaxHeight          = SUB_VENC_H;

            ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
            ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

            memset(&stDivpOutputPortAttr, 0, sizeof(stDivpOutputPortAttr));
            stDivpOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
            stDivpOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stDivpOutputPortAttr.u32Width           = VDA_CHN_W;//Vdf input size 352*288
            stDivpOutputPortAttr.u32Height          = VDA_CHN_H;

            STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stDivpOutputPortAttr));
        }
    }

    ///// vdisp
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_S32 s32FrmRate = 30;
    MI_S32 s32OutputPort = 0;
    MI_VDISP_PORT vdispPort = 0;
    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        STCHECKRESULT(ST_Vdisp_Init());
        STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev,
                        s32OutputPort, &stvdispRect, s32FrmRate, 1));
        STCHECKRESULT(ST_Vdisp_StartDevice(vdispDev));
    }

    /************************************************
    Step6:  init DISP
    *************************************************/
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, eDispoutTiming)); //Dispout timing

#if USE_DISP_VGA
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV1, ST_DISP_LAYER1, eDispoutTiming));
#endif

    ST_DispChnInfo_t stDispChnInfo;
    MI_BOOL bVdecFlag = 0;

#ifdef SUPPORT_VIDEO_PREVIEW
    stDispChnInfo.InputPortNum = 0;
    for (i = 0; i < s32DispChnNum; i++)
    {
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            stDispChnInfo.InputPortNum ++;
            stDispChnInfo.stInputPortAttr[i].u32Port = i;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X = stDispWndRect[i].s32X;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y = stDispWndRect[i].s32Y;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width = stDispWndRect[i].u16PicW;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height = stDispWndRect[i].u16PicH;

            ST_DBG("===========disp channel(%d) x(%d)-y(%d)-w(%d)-h(%d)...\n", i,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height);

            bVdecFlag = 1;
        }
    }

    if (bVdecFlag == 0)
    {
        stDispChnInfo.InputPortNum = s32DispChnNum;
        for (i = 0; i < s32DispChnNum; i++)
        {
            stDispChnInfo.stInputPortAttr[i].u32Port = i;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X = stDispWndRect[i].s32X;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y = stDispWndRect[i].s32Y;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width = stDispWndRect[i].u16PicW;
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height = stDispWndRect[i].u16PicH;
            ST_DBG("===========disp channel(%d) x(%d)-y(%d)-w(%d)-h(%d)...\n", i,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width,
                stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height);
        }
    }

    ST_DBG("stDispChnInfo.InputPortNum:%d\n", stDispChnInfo.InputPortNum);
    STCHECKRESULT(ST_Disp_ChnInit(0, &stDispChnInfo));
#endif

#if USE_DISP_VGA
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo));
#endif

#ifdef SUPPORT_VIDEO_ENCODE
    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        for (i = 0; i < s32DispChnNum; i++)
        {
            vdispPort = i;
            stRect.s32X     = ALIGN_BACK(stVdispWndRect[i].s32X, 16);
            stRect.s32Y     = ALIGN_BACK(stVdispWndRect[i].s32Y, 2);
            stRect.u16PicW  = ALIGN_BACK(stVdispWndRect[i].u16PicW, 16);
            stRect.u16PicH  = ALIGN_BACK(stVdispWndRect[i].u16PicH, 2);

            ST_DBG("vdisp input port:%d u16PicW:%d,u16PicH:%d\n", vdispPort, stRect.u16PicW, stRect.u16PicH);

            STCHECKRESULT(ST_Vdisp_SetInputPortAttr(vdispDev, vdispPort, &stRect));
            STCHECKRESULT(ST_Vdisp_EnableInputPort(vdispDev, vdispPort));
        }
    }
#endif

    // must init after disp
    MI_SYS_WindowRect_t Rect;

    ST_Fb_Init(E_MI_FB_COLOR_FMT_ARGB1555);
    ST_FB_Show(FALSE);

    sleep(1);
    // change fb resolution
    ST_FB_ChangeResolution(stdispRect.u16PicW, stdispRect.u16PicH);

    ST_FB_Show(TRUE);

    ST_Fb_SetColorKey(ARGB888_BLUE);

    MI_FB_GlobalAlpha_t stAlphaInfo;

    memset(&stAlphaInfo, 0, sizeof(MI_FB_GlobalAlpha_t));
    ST_FB_GetAlphaInfo(&stAlphaInfo);
    printf("FBIOGET_GLOBAL_ALPHA alpha info: alpha blend enable=%d,Multialpha enable=%d,Global Alpha=%d,u8Alpha0=%d,u8Alpha1=%d\n",
        stAlphaInfo.bAlphaEnable,stAlphaInfo.bAlphaChannel,stAlphaInfo.u8GlobalAlpha,stAlphaInfo.u8Alpha0,stAlphaInfo.u8Alpha1);
    stAlphaInfo.bAlphaEnable = TRUE;
    stAlphaInfo.bAlphaChannel= TRUE;
    stAlphaInfo.u8GlobalAlpha = 0x70;
    ST_FB_SetAlphaInfo(&stAlphaInfo);

    ST_Fb_InitMouse(44, 56, 4, "/mnt/cursor.raw");
    ST_Fb_MouseSet(1, 1);

    mouse_setrange(0, 0, stdispRect.u16PicW, stdispRect.u16PicH);

    MI_BOOL bNeedVdf = FALSE;
    for (i = 0; i < s32CapChnNum; i++)
    {
        if ((pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDF_CHN) &&
            (SUB_VENC_SUPPORT_FLAG) && (MD_OD_SUPPORT_FLAG))
        {
            bNeedVdf = TRUE;
            break;
        }
    }

    ///// VDF
    if (bNeedVdf)
    {
        STCHECKRESULT(MI_VDF_Init());
    }

    MI_VDF_ChnAttr_t stVdfAttr;
    MI_VDF_CHANNEL vdfChn = 0;
    int ii = 0, jj = 0;
    int mdTotalNum = 0, odTotalNum = 0;

    for (i = 0; i < s32CapChnNum; i++)
    {
        if ((pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDF_CHN) &&
            (SUB_VENC_SUPPORT_FLAG) && (MD_OD_SUPPORT_FLAG))
        {
            // create md chn
            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum; j ++)
            {
                vdfChn = mdTotalNum + j;

                memset(&stVdfAttr, 0, sizeof(MI_VDF_ChnAttr_t));
                stVdfAttr.enWorkMode = E_MI_VDF_WORK_MODE_MD;
            	stVdfAttr.unAttr.stMdAttr.u8Enable    = 1;
            	stVdfAttr.unAttr.stMdAttr.u8SrcChnNum = 0;
            	stVdfAttr.unAttr.stMdAttr.u8MdBufCnt  = 3;
            	stVdfAttr.unAttr.stMdAttr.u8VDFIntvl  = 0;

            	stVdfAttr.unAttr.stMdAttr.ccl_ctrl.u16InitAreaThr = 8;
            	stVdfAttr.unAttr.stMdAttr.ccl_ctrl.u16Step = 2;

                stVdfAttr.unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 70;
                stVdfAttr.unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
                stVdfAttr.unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
                stVdfAttr.unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.width   =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;//720;//g_width;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.height  =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;//576;//g_height;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.color   = 1;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32X;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32Y;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32X +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicW - 1;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32Y;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32X +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicW - 1;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32Y +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicH - 1;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32X;
                stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32Y +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicH - 1;

                ST_DBG("MD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.width,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.height,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y);

                Rect.u16X = stDispWndRect[i].s32X +
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32X *
                    stDispWndRect[i].u16PicW) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;

                Rect.u16Y = stDispWndRect[i].s32Y +
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].s32Y *
                    stDispWndRect[i].u16PicH) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;

                Rect.u16Width = (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicW *
                    stDispWndRect[i].u16PicW) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;

                Rect.u16Height =
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stMdArea[j].u16PicH *
                    stDispWndRect[i].u16PicH) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;

                ST_DBG("Draw MD Area (%d,%d,%d,%d)\n", Rect.u16X, Rect.u16Y, Rect.u16Width, Rect.u16Height);

                ST_Fb_DrawRect(&Rect, ARGB888_RED);

                if (MI_SUCCESS != (s32Ret = MI_VDF_CreateChn(vdfChn, &stVdfAttr)))
                {
                    ST_ERR("MI_VDF_CreateChn err, chn %d, %x\n", vdfChn, s32Ret);
                    return 1;
                }

                ST_DBG("MD MI_VDF_CreateChn success, chn %d\n", vdfChn);
            }

            mdTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum;

            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum; j ++)
            {
                vdfChn = odTotalNum + j + OD_START_PORT;

                stVdfAttr.enWorkMode = E_MI_VDF_WORK_MODE_OD;
                stVdfAttr.unAttr.stOdAttr.u8SrcChnNum = 0;
                stVdfAttr.unAttr.stOdAttr.u8OdBufCnt  = 16;
                stVdfAttr.unAttr.stOdAttr.u8VDFIntvl  = 5;

                stVdfAttr.unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 3;
                stVdfAttr.unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
                stVdfAttr.unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.inImgW =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.inImgH =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.nClrType = 1;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
            	stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.M = 120;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.MotionSensitivity = 0;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.num = 4;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32X;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32Y;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32X +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicW - 1;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32Y;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32X +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicW - 1;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32Y +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicH - 1;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32X;
                stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32Y +
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicH - 1;

                Rect.u16X = stDispWndRect[i].s32X +
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32X *
                    stDispWndRect[i].u16PicW) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;

                Rect.u16Y = stDispWndRect[i].s32Y +
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].s32Y *
                    stDispWndRect[i].u16PicH) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;

                Rect.u16Width = (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicW *
                    stDispWndRect[i].u16PicW) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;

                Rect.u16Height =
                    (pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.stOdArea[j].u16PicH *
                    stDispWndRect[i].u16PicH) / pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;

                ST_DBG("Draw OD Area (%d,%d,%d,%d)\n", Rect.u16X, Rect.u16Y, Rect.u16Width, Rect.u16Height);

                ST_Fb_DrawRect(&Rect, ARGB888_GREEN);

                ST_DBG("OD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.width,
                    stVdfAttr.unAttr.stMdAttr.stMdStaticParamsIn.height,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x,
                    stVdfAttr.unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y);

                if (MI_SUCCESS != (s32Ret = MI_VDF_CreateChn(vdfChn, &stVdfAttr)))
                {
                    ST_ERR("MI_VDF_CreateChn err, chn %d, %x\n", vdfChn, s32Ret);
                    return 1;
                }

                ST_DBG("OD MI_VDF_CreateChn success, chn %d\n", vdfChn);
            }
            odTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum;
        }
    }

    if (bNeedVdf)
    {
        STCHECKRESULT(MI_VDF_Run(E_MI_VDF_WORK_MODE_MD));
        STCHECKRESULT(MI_VDF_Run(E_MI_VDF_WORK_MODE_OD));
    }

    /************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    for (i = 0; i < s32CapChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream

        if (SUPPORT_DIVP_DI && (288 == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight))
        {
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32PortId = 0;
        }
        else
        {
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stDstChnPort.u32PortId = 0;
        }
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

#ifdef SUPPORT_VIDEO_ENCODE //Create Video encode Channel
    // main+sub venc
    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;
    MI_U32 u32DevId = 0;

    if (pstCaseDesc[s32CaseIndex].stDesc.u32VencNum != 0)
    {
        u32VencNum = pstCaseDesc[s32CaseIndex].stDesc.u32VencNum;
    }
    else
    {
        u32VencNum = s32CapChnNum;
    }

    ST_DBG("total venc chn num:%d\n", u32VencNum);

    for (i = 0; i < u32VencNum; i ++)
    {
        VencChn = i;

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H264_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H264_ALIGN_H);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H264_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H264_ALIGN_H);
            #if 0
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
				stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
            #else
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*2;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 25;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 25;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            #endif
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H265_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H265_ALIGN_H);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H265_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H265_ALIGN_H);
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
				stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 36;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 36;
        }
        else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
        }
        s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
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

#if VENC_MAX_FRM_CNT
        s32Ret = MI_VENC_SetMaxStreamCnt(VencChn, VENC_MAX_FRM_CNT);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_SetMaxStreamCnt %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#else
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
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 4);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#endif

        s32Ret = MI_VENC_StartRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

        if (SUB_VENC_SUPPORT_FLAG)
        {
            VencChn = i + VENC_SUB_CHN_START;

            memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
            memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

            if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
            {
                stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
                stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
                stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
                stChnAttr.stVeAttr.stAttrH264e.u32PicWidth =
                    ALIGN_N(SUB_VENC_H, VENC_H264_ALIGN_W);
                stChnAttr.stVeAttr.stAttrH264e.u32PicHeight =
                    ALIGN_N(SUB_VENC_W, VENC_H264_ALIGN_H);
                stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth =
                    ALIGN_N(SUB_VENC_H, VENC_H264_ALIGN_W);
                stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight =
                    ALIGN_N(SUB_VENC_W, VENC_H264_ALIGN_H);
                #if 0
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

                stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
                #else
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*2;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 25;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 25;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
                #endif
            }
            else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
            {
                stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
                stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
                stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
                    ALIGN_N(SUB_VENC_W, VENC_H265_ALIGN_W);
                stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
                    ALIGN_N(SUB_VENC_H, VENC_H265_ALIGN_H);
                stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
                    ALIGN_N(SUB_VENC_W, VENC_H265_ALIGN_W);
                stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
                    ALIGN_N(SUB_VENC_H, VENC_H265_ALIGN_H);
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 36;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 36;
            }
            else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
            {
                stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
                stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
                stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth =
                    ALIGN_N(SUB_VENC_W, 32);
                stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight =
                    ALIGN_N(SUB_VENC_H, 32);
                stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =
                    ALIGN_N(SUB_VENC_W, 32);
                stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight =
                    ALIGN_N(SUB_VENC_H, 32);
            }
            s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
            }
            if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
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

#if VENC_MAX_FRM_CNT
        s32Ret = MI_VENC_SetMaxStreamCnt(VencChn, VENC_MAX_FRM_CNT);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_SetMaxStreamCnt %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#else
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
            s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 4);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
            }
#endif
            s32Ret = MI_VENC_StartRecvPic(VencChn);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
            }
        }
    }

    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        VencChn = u32VencNum;

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth, VENC_H264_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight, VENC_H264_ALIGN_H);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth, VENC_H264_ALIGN_W);
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight, VENC_H264_ALIGN_H);
            #if 0
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
			stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
            #else
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*2;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            #endif
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;

            ST_DBG("width:%d,height:%d, maxw:%d,maxh:%d\n",
                stChnAttr.stVeAttr.stAttrH265e.u32PicWidth, stChnAttr.stVeAttr.stAttrH265e.u32PicHeight,
                stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth, stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight);

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 36;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 36;
        }
        else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
        }
        s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
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

#if VENC_MAX_FRM_CNT
        s32Ret = MI_VENC_SetMaxStreamCnt(VencChn, VENC_MAX_FRM_CNT);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_SetMaxStreamCnt %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#else
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
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 4);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
#endif

        s32Ret = MI_VENC_StartRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
    }
#endif

#ifdef SUPPORT_RGN
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;

    for (i = 0; i < s32DispChnNum; i ++)
    {
#ifdef SUPPORT_VIDEO_PREVIEW
        memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;

        hHandle = i;
        s32Ret = MI_RGN_Create(hHandle, &stRgnAttr);
        if (MI_RGN_OK != s32Ret)
        {
            ST_ERR("MI_RGN_Create error, %X\n", s32Ret);
            return 1;
        }

        stChnPort.eModId = E_MI_RGN_MODID_VPE;
        stChnPort.s32DevId = 0;
        stChnPort.s32ChnId = i;
        stChnPort.s32OutputPortId = DISP_PORT;

        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
        stChnPortParam.bShow = TRUE;
        stChnPortParam.stPoint.u32X = (10 * MAX_RGN_W) / stDispWndRect[i].u16PicW;
        stChnPortParam.stPoint.u32Y = (10 * MAX_RGN_H) / stDispWndRect[i].u16PicH;
        stChnPortParam.stCoverPara.u32Layer = hHandle;
        stChnPortParam.stCoverPara.stSize.u32Width = (100 * MAX_RGN_W) / stDispWndRect[i].u16PicW;
        stChnPortParam.stCoverPara.stSize.u32Height = (100 * MAX_RGN_H) / stDispWndRect[i].u16PicH;
        stChnPortParam.stCoverPara.u32Color = ARGB888_GREEN;
        s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
        if (MI_RGN_OK != s32Ret)
        {
            ST_ERR("MI_RGN_AttachToChn error, %X\n", s32Ret);
            return 1;
        }

        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_SUB_CHN)
        {
            stChnPort.eModId = E_MI_RGN_MODID_VPE;
            stChnPort.s32DevId = 0;
            stChnPort.s32ChnId = i + VPE_SUB_CHN_START;
            stChnPort.s32OutputPortId = DISP_PORT;

            memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
            stChnPortParam.bShow = TRUE;
            stChnPortParam.stPoint.u32X = (10 * MAX_RGN_W) / stDispWndRect[i].u16PicW;
            stChnPortParam.stPoint.u32Y = (10 * MAX_RGN_H) / stDispWndRect[i].u16PicH;
            stChnPortParam.stCoverPara.u32Layer = hHandle;
            stChnPortParam.stCoverPara.stSize.u32Width = (100 * MAX_RGN_W) / stDispWndRect[i].u16PicW;
            stChnPortParam.stCoverPara.stSize.u32Height = (100 * MAX_RGN_H) / stDispWndRect[i].u16PicH;
            stChnPortParam.stCoverPara.u32Color = ARGB888_GREEN;
            s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
            if (MI_RGN_OK != s32Ret)
            {
                ST_ERR("MI_RGN_AttachToChn error, %X\n", s32Ret);
                return 1;
            }
        }
#endif

#ifdef SUPPORT_VIDEO_ENCODE
        memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;

        hHandle = i + s32DispChnNum;
        s32Ret = MI_RGN_Create(hHandle, &stRgnAttr);
        if (MI_RGN_OK != s32Ret)
        {
            ST_ERR("MI_RGN_Create error, %X\n", s32Ret);
            return 1;
        }

        stChnPort.eModId = E_MI_RGN_MODID_VPE;
        stChnPort.s32DevId = 0;
        stChnPort.s32ChnId = i;
        stChnPort.s32OutputPortId = MAIN_VENC_PORT;

        memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
        stChnPortParam.bShow = TRUE;
        stChnPortParam.stPoint.u32X = (10 * MAX_RGN_W) /
            ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H264_ALIGN_W);
        stChnPortParam.stPoint.u32Y = (10 * MAX_RGN_H) /
            ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H264_ALIGN_H);
        stChnPortParam.stCoverPara.u32Layer = hHandle;
        stChnPortParam.stCoverPara.stSize.u32Width = (100 * MAX_RGN_W) /
            ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, VENC_H264_ALIGN_W);
        stChnPortParam.stCoverPara.stSize.u32Height = (100 * MAX_RGN_H) /
            ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, VENC_H264_ALIGN_H);
        stChnPortParam.stCoverPara.u32Color = ARGB888_GREEN;
        s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
        if (MI_RGN_OK != s32Ret)
        {
            ST_ERR("MI_RGN_AttachToChn error, %X\n", s32Ret);
            return 1;
        }

        if (SUB_VENC_SUPPORT_FLAG)
        {
            stChnPort.eModId = E_MI_RGN_MODID_VPE;
            stChnPort.s32DevId = 0;
            stChnPort.s32ChnId = i + VPE_SUB_CHN_START;
            stChnPort.s32OutputPortId = MAIN_VENC_PORT;

            memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
            stChnPortParam.bShow = TRUE;
            stChnPortParam.stPoint.u32X = (10 * MAX_RGN_W) /
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth / 2, VENC_H264_ALIGN_W);
            stChnPortParam.stPoint.u32Y = (10 * MAX_RGN_H) /
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight / 2, VENC_H264_ALIGN_H);
            stChnPortParam.stCoverPara.u32Layer = hHandle;
            stChnPortParam.stCoverPara.stSize.u32Width = (100 * MAX_RGN_W) /
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth / 2, VENC_H264_ALIGN_W);
            stChnPortParam.stCoverPara.stSize.u32Height = (100 * MAX_RGN_H) /
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight / 2, VENC_H264_ALIGN_H);
            stChnPortParam.stCoverPara.u32Color = ARGB888_GREEN;
            s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
        }
#endif
    }
#endif
    /************************************************
    Bind DIVP->VPE
    *************************************************/
    for (i = 0; i < s32DispChnNum; i++)
    {
        if (SUPPORT_DIVP_DI && (288 == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight))
        {
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
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }

#ifdef SUPPORT_VIDEO_PREVIEW
    /************************************************
    Step8:  Bind VPE->DISP
    *************************************************/
    if (bVdecFlag == 0)
    {
        for (i = 0; i < s32DispChnNum; i++)
        {
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i + VPE_SUB_CHN_START;
            stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 0;
            stBindInfo.stDstChnPort.u32PortId = i;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }

#if 1
    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        // bind vpe->vdisp
        for (i = 0; i < s32DispChnNum; i++)
        {
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i;
            stBindInfo.stSrcChnPort.u32PortId = VDISP_PORT;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 0;
            stBindInfo.stDstChnPort.u32PortId = i;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }
#endif

    for (i = 0; i < s32DispChnNum; i++)
    {
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
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

            ST_DBG("vdec(%d)->divp(%d) bind\n", i, i);

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

            if (MI_SUCCESS != ST_Sys_Bind(&stBindInfo))
            {
                ST_ERR("divp(%d)->disp(%d) bind failed\n", i, i);
                return 1;
            }
            ST_DBG("divp(%d)->disp(%d) bind\n", i, i);
        }
    }
#endif

#if USE_DISP_VGA
    for (i = 0; i < u32WndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;

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

#ifdef SUPPORT_VIDEO_ENCODE
    for (i = 0; i < u32VencNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        if (pstCaseDesc[s32CaseIndex].stDesc.u32VencNum != 0)
        {
            stBindInfo.stSrcChnPort.u32ChnId = 0;
        }
        else
        {
            stBindInfo.stSrcChnPort.u32ChnId = i;
        }

        stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        s32Ret = MI_VENC_GetChnDevid(i, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, i, s32Ret);
        }
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        if (SUB_VENC_SUPPORT_FLAG)
        {
            VencChn = i + VENC_SUB_CHN_START;
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i;
            stBindInfo.stSrcChnPort.u32PortId = SUB_VENC_PORT;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            s32Ret = MI_VENC_GetChnDevid(VencChn, &u32DevId);
            if (MI_SUCCESS != s32Ret)
            {
                printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
            }
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }

    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        VencChn = u32VencNum;

        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        s32Ret = MI_VENC_GetChnDevid(VencChn, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        printf("%s %d vdisp(0)->venc(%d) bind\n", __func__, __LINE__, VencChn);
    }
#endif

    // bind vpe to vdf
    ST_DBG("s32CapChnNum:%d\n", s32CapChnNum);

    mdTotalNum = 0;
    odTotalNum = 0;
    MI_S32 s32DivpPort = 0;
    for (i = 0; i < s32CapChnNum; i++)
    {
        if ((pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDF_CHN) &&
            (SUB_VENC_SUPPORT_FLAG) && (MD_OD_SUPPORT_FLAG))
        {
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = i;
            stBindInfo.stSrcChnPort.u32PortId = SUB_VENC_PORT;

            s32DivpPort = i + OD_DIVP_OFFSET;
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = s32DivpPort;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 5;

            if (MI_SUCCESS == ST_Sys_Bind(&stBindInfo))
            {
                ST_DBG("vpe bind vdf(MD), %d->%d success\n", i, vdfChn);
            }
            else
            {
                ST_ERR("vpe bind vdf(MD), %d->%d fail\n", i, vdfChn);
            }
            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum; j ++)
            {
                memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

                vdfChn = mdTotalNum + j;

                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = s32DivpPort;
                stBindInfo.stSrcChnPort.u32PortId = 0;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
                stBindInfo.stDstChnPort.u32DevId = 0;
                stBindInfo.stDstChnPort.u32ChnId = vdfChn;
                stBindInfo.stDstChnPort.u32PortId = 0;
                stBindInfo.u32SrcFrmrate = 0;
                stBindInfo.u32DstFrmrate = 0;

                if (MI_SUCCESS == ST_Sys_Bind(&stBindInfo))
                {
                    ST_DBG("vpe bind vdf(MD), %d->%d success\n", i, vdfChn);
                }
                else
                {
                    ST_ERR("vpe bind vdf(MD), %d->%d fail\n", i, vdfChn);
                }
                if (MI_SUCCESS != (s32Ret = MI_VDF_EnableSubWindow(vdfChn, 0, 0, 1)))
                {
                    ST_ERR("MI_VDF_EnableSubWindow err, chn %d, %x\n", vdfChn, s32Ret);
                    return 1;
                }
                else
                {
                    ST_DBG("MI_VDF_EnableSubWindow ok, chn %d, %x\n", vdfChn, s32Ret);
                }
            }
            mdTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum;

            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum; j ++)
            {
                memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

                vdfChn = j + odTotalNum + OD_START_PORT;

                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = s32DivpPort;
                stBindInfo.stSrcChnPort.u32PortId = 0;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
                stBindInfo.stDstChnPort.u32DevId = 0;
                stBindInfo.stDstChnPort.u32ChnId = vdfChn;
                stBindInfo.stDstChnPort.u32PortId = 0;
                stBindInfo.u32SrcFrmrate = 0;
                stBindInfo.u32DstFrmrate = 0;

                if (MI_SUCCESS == ST_Sys_Bind(&stBindInfo))
                {
                    ST_DBG("vpe bind vdf(OD), %d->%d success\n", i, vdfChn);
                }
                else
                {
                    ST_ERR("vpe bind vdf(OD), %d->%d fail\n", i, vdfChn);
                }
                if (MI_SUCCESS != (s32Ret = MI_VDF_EnableSubWindow(vdfChn, 0, 0, 1)))
                {
                    ST_ERR("MI_VDF_EnableSubWindow err, chn %d, %x\n", vdfChn, s32Ret);
                    return 1;
                }
                else
                {
                    ST_DBG("MI_VDF_EnableSubWindow ok, chn %d, %x\n", vdfChn, s32Ret);
                }
            }
            odTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum;
        }
    }

    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        STCHECKRESULT(ST_Vif_StartPort(stVifChnCfg[i].u8ViChn, 0));
    }

#ifdef SUPPORT_VIDEO_ENCODE
    for (i = 0; i < u32VencNum; i++) //init vif channel
    {
        VencChn = i;
        if (VENC_GET_STREAM_FLAG)
        {
            g_stVencArgs[i].stVencAttr[0].vencChn = i;
            g_stVencArgs[i].stVencAttr[0].u32MainWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            g_stVencArgs[i].stVencAttr[0].u32MainHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            g_stVencArgs[i].stVencAttr[0].eType = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType;
            g_stVencArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);
            g_stVencArgs[i].bRunFlag = TRUE;
            ST_DBG("i:%d, eType:%d\n", i, g_stVencArgs[i].stVencAttr[0].eType);
            pthread_create(&g_stVencArgs[i].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[i]);

            if (SUB_VENC_SUPPORT_FLAG)
            {
                VencChn = i + VENC_SUB_CHN_START;

                g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].vencChn = VencChn;
                g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].u32MainWidth =
                    ALIGN_N(SUB_VENC_W, VENC_H264_ALIGN_W);
                g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].u32MainHeight =
                    ALIGN_N(SUB_VENC_H, VENC_H264_ALIGN_H);
                g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].eType =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType;
                g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);
                g_stVencArgs[i + VENC_SUB_CHN_START].bRunFlag = TRUE;
                ST_DBG("i:%d, eType:%d\n", i + VENC_SUB_CHN_START,
                    g_stVencArgs[i + VENC_SUB_CHN_START].stVencAttr[0].eType);
                pthread_create(&g_stVencArgs[i + VENC_SUB_CHN_START].ptGetEs, NULL,
                    ST_VencGetEsBufferProc, (void *)&g_stVencArgs[i + VENC_SUB_CHN_START]);
            }
        }
    }

    if (pstCaseDesc[s32CaseIndex].stDesc.bNeedVdisp)
    {
        VencChn = u32VencNum;
        if (VENC_GET_STREAM_FLAG)
        {
            g_stVencArgs[i].stVencAttr[0].vencChn = u32VencNum;
            g_stVencArgs[i].stVencAttr[0].u32MainWidth =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainWidth;
            g_stVencArgs[i].stVencAttr[0].u32MainHeight =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.u16MainHeight;
            g_stVencArgs[i].stVencAttr[0].eType =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType;
            g_stVencArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);
            g_stVencArgs[i].bRunFlag = TRUE;

            ST_DBG("i:%d, eType:%d\n", i, g_stVencArgs[i].stVencAttr[0].eType);
            pthread_create(&g_stVencArgs[i].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[i]);
        }
    }

#endif

    for (i = 0; i < u32VencNum; i++)
    {
        if (pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            g_stVdecThreadArgs[i].vdecChn = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32Chn;
            snprintf(g_stVdecThreadArgs[i].szFileName, sizeof(g_stVdecThreadArgs[i].szFileName) - 1,
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.szFilePath);

            g_stVdecThreadArgs[i].bRunFlag = TRUE;
            g_PushEsExit = FALSE;
            pthread_create(&g_stVdecThreadArgs[i].pt, NULL, ST_VdecSendStream, (void *)&g_stVdecThreadArgs[i]);
        }
    }

    mdTotalNum = 0;
    odTotalNum = 0;
    for (i = 0; i < s32CapChnNum; i++)
    {
        if ((pstCaseDesc[s32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDF_CHN) &&
            (SUB_VENC_SUPPORT_FLAG) && (MD_OD_SUPPORT_FLAG))
        {
            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum; j ++)
            {
                // vdfChn = i * s32CapChnNum + j;
                vdfChn = j + mdTotalNum;

                g_stVdfThreadArgs[vdfChn].enWorkMode = E_MI_VDF_WORK_MODE_MD;
                g_stVdfThreadArgs[vdfChn].vdfChn = vdfChn;
                g_stVdfThreadArgs[vdfChn].bRunFlag = TRUE;
                g_stVdfThreadArgs[vdfChn].u16Width =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;
                g_stVdfThreadArgs[vdfChn].u16Height =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;
                pthread_create(&g_stVdfThreadArgs[vdfChn].pt, NULL, ST_VDFGetResult, (void *)&g_stVdfThreadArgs[vdfChn]);
            }
            mdTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16MdNum;

            for (j = 0; j < pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum; j ++)
            {
                // vdfChn = i * s32CapChnNum + j + 100;
                vdfChn = j + odTotalNum + OD_START_PORT;

                g_stVdfThreadArgs[vdfChn].enWorkMode = E_MI_VDF_WORK_MODE_OD;
                g_stVdfThreadArgs[vdfChn].vdfChn = vdfChn;
                g_stVdfThreadArgs[vdfChn].bRunFlag = TRUE;
                g_stVdfThreadArgs[vdfChn].u16Width =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInWidth;
                g_stVdfThreadArgs[vdfChn].u16Height =
                    pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16VdfInHeight;
                pthread_create(&g_stVdfThreadArgs[vdfChn].pt, NULL, ST_VDFGetResult, (void *)&g_stVdfThreadArgs[vdfChn]);
            }
            odTotalNum += pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16OdNum;
        }
    }

    pthread_create(&g_mouse_user_pt, NULL, ST_FBMouseUserProc, NULL);
#ifdef SUPPORT_UVC
    ST_UVCInit();
#endif

    STCHECKRESULT(ST_Hdmi_Start(eHdmi, eHdmiTiming)); //Hdmi timing
    g_u32LastSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    g_u32CurSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[s32CaseIndex].u32ShowWndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    //pthread_create(&pt, NULL, st_GetOutputDataThread, NULL);

#ifdef SUPPORT_DYNAMIC_DETECT_VIF
    pthread_create(&t_VifDetect, NULL, ST_VifDetectProcess, NULL);
#endif

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

MI_S32 test_main(int argc, char **argv)
{
    MI_S32 a = 0, b = 1;
    MI_S32 sum = 0;

    while (1)
    {
        sum = a + b;
    }

    return 0;
}

MI_S32 main(int argc, char **argv)
{
    char szCmd[16];
    MI_U32 u32Index = 0;

    //return test_main(argc, argv);

    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    signal(SIGCHLD, SIG_IGN);

    printf(__TIME__"\n");
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
