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
#include "st_vif.h"

typedef struct
{
    pthread_t pt;
    MI_VDEC_CHN vdecChn;
    char szFileName[64];        // H26X file path
} VDEC_Thread_Args_t;

typedef struct
{
    pthread_t pt;
    MI_VPE_CHANNEL vpeChn;
    MI_RGN_HANDLE rgnHandle;
    char szFileName[64];        // bmp file path
} RGN_Thread_Args_t;

//Config logic chn trans to phy chn
ST_VifChnConfig_t g_stVifChnCfg[VIF_MAX_CHN_NUM] = {
    {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, //16main
    {1, 4, 0}, {1, 5, 0}, {1, 6, 0}, {1, 7, 0},
    {2, 8, 0}, {2, 9, 0}, {2, 10, 0}, {2, 11, 0},
    {3, 12, 0}, {3, 13, 0}, {3, 14, 0}, {3, 15, 0},
    {0, 0, 1}, {0, 1, 1}, {0, 2, 1}, {0, 3, 1}, //16sub
    {1, 4, 1}, {1, 5, 1}, {1, 6, 1}, {1, 7, 1},
    {2, 8, 1}, {2, 9, 1}, {2, 10, 1}, {2, 11, 1},
    {3, 12, 1}, {3, 13, 1}, {3, 14, 1}, {3, 15, 1},
};

static MI_BOOL g_bExit = FALSE;
static MI_BOOL g_subExit = FALSE;
static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32SubCaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;
ST_CaseDesc_t g_stCaseDesc[] =
{
    {
        .stDesc =
        {
            .u32CaseIndex = 0,
            .szDesc = "1x4K@15 H264 Decode + OSD/COVER",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
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
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 1,
            .szDesc = "4x1080P@30 H264 Decode + OSD/COVER",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
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
            .u32CaseIndex = 2,
            .szDesc = "4xD1 H264 Decode + OSD/COVER",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
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
            .u32CaseIndex = 3,
            .szDesc = "1x4K@15 VIF->DISPLAY + OSD/COVER",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32Chn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .u32MaxWidth = 3840,
                        .u32MaxHeight = 2160,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 4,
            .szDesc = "4x1080P@30 VIF->DISPLAY + OSD/COVER",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
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
                        .s32FrmRate = 25,
                    },
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
                        .s32FrmRate = 25,
                    },
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
                        .s32FrmRate = 25,
                    },
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
                        .s32FrmRate = 25,
                    },
                }
            },
        },
    },
#if 0
    {
        .stDesc =
        {
            .u32CaseIndex = 5,
            .szDesc = "4x1080P@30 VIF->ENCODE + OSD/COVER",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 0,
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = 25,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = 25,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = 25,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_VENC_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = 25,
                    },
                }
            },
        },
    }
#endif
};

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

void ST_RgnUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 i = 0;

    for (i = 0; i < u32Size; i ++)
    {
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
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

    MI_S32 readfd = 0, s32ReadCnt = 0;
    MI_U8 *pu8Buf = NULL;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_VideoStream_t stVdecStream;

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
    pu8Buf = malloc(NALU_PACKET_SIZE);

    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5));

    //stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    //STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5));

    s32Ms = 33;
    printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (!g_bExit)
    {
        usleep(s32Ms * 1000);//33

        memset(au8Header, 0, 16);
        u32Pos = lseek(readfd, 0L, SEEK_CUR);
        s32Len = read(readfd, au8Header, 16);
        if(s32Len <= 0)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }

        u32FrameLen = MI_U32VALUE(au8Header, 4);
        // printf("u32FrameLen:%d\n", u32FrameLen);
        if(u32FrameLen > NALU_PACKET_SIZE)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }
        s32Len = read(readfd, pu8Buf, u32FrameLen);
        if(s32Len <= 0)
        {
            lseek(readfd, 0, SEEK_SET);
            continue;
        }
        stBufConf.stRawCfg.u32Size = s32Len;

#if 1
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        hSysBuf = MI_HANDLE_NULL;
        // printf("MI_SYS_ChnInputPortGetBuf modid(%d) chn(%d) portid(%d)\n", stChnPort.eModId, stChnPort.u32ChnId,
            // stChnPort.u32ChnId);
        if (MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf, s32TimeOutMs))
        {
            lseek(readfd, u32Pos, SEEK_SET);
            continue;
        }
        //if (NULL == pu8Buf)
        if (0)
        {
            if (stBufInfo.eBufType)
                printf("stBufInfo.eBufType....(%d)...\n", stBufInfo.eBufType);

            printf("stBufInfo.stRawData.pVirAddr --0x%08llx--%p--%p---(%d)-%d---(%p)\n",
                stBufInfo.stRawData.phyAddr, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize, pu8Buf, s32Len, &stBufInfo);

        }
        //my_memset(stBufInfo.stRawData.pVirAddr, u32Len);
        memcpy(stBufInfo.stRawData.pVirAddr, pu8Buf, s32Len);
        //my_memcpy(stBufInfo.stRawData.pVirAddr, pu8Buf, s32Len);

        stBufInfo.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufInfo.bEndOfStream = FALSE;
        stBufInfo.u64Pts = u64Pts;
        stBufInfo.stRawData.bEndOfFrame = TRUE;
        stBufInfo.stRawData.u32ContentSize = s32Len;
        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, FALSE))
        {
            lseek(readfd, u32Pos, SEEK_SET);
            printf("MI_SYS_ChnInputPortPutBuf fail.\n");
            continue;
        }
#endif

#if 0
        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = s32Len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;
        if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(s32VoChannel, &stVdecStream, s32TimeOutMs)))
        {
            printf("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", s32VoChannel, s32Ret);
        }
#endif

        u64Pts = u64Pts + ST_Sys_GetPts(30);
        //memset(&stChnStat, 0x0, sizeof(stChnStat));
        //MI_VDEC_GetChnStat(s32VoChannel, &stChnStat);

        if (0 == (s32ReadCnt++ % 30))
            ;// printf("vdec(%d) push buf cnt (%d)...\n", s32VoChannel, s32ReadCnt)
            ;//printf("###### ==> Chn(%d) push frame(%d) Frame Dec:%d  Len:%d\n", s32VoChannel, s32ReadCnt, stChnStat.u32DecodeStreamFrames, u32Len);
    }
    printf("\n\n");
    usleep(3000000);
    free(pu8Buf);
    return NULL;
}

void *ST_RgnProc(void *args)
{
    RGN_Thread_Args_t *pstArgs = (RGN_Thread_Args_t *)args;

    MI_HANDLE hHandle = pstArgs->rgnHandle;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_PaletteTable_t stPaletteTable;

    MI_RGN_Bitmap_t stBitmap;
    MI_U32 u32FileSize = 0;
    MI_U8 *pu8FileBuffer = NULL;

    memset(&stPaletteTable, 0, sizeof(MI_RGN_PaletteTable_t));

    stPaletteTable.astElement[0].u8Alpha = 0;
    stPaletteTable.astElement[0].u8Red = 255;
    stPaletteTable.astElement[0].u8Green = 255;
    stPaletteTable.astElement[0].u8Blue = 255;
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_RGN_Init(&stPaletteTable);
    if (s32Ret != MI_RGN_OK)
    {
        printf("MI_RGN_Init failed, s32Ret:%d\n", s32Ret);
        return NULL;
    }
    // STCHECKRESULT(MI_RGN_Init(&stPaletteTable));

    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;// not supply cover?
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    stRgnAttr.stOsdInitParam.stSize.u32Width = 40;
    stRgnAttr.stOsdInitParam.stSize.u32Height = 40;
    s32Ret = MI_RGN_Create(hHandle, &stRgnAttr);
    if (s32Ret != MI_RGN_OK)
    {
        printf("MI_RGN_Create failed, s32Ret:%X\n", s32Ret);
        return NULL;
    }
    // STCHECKRESULT(MI_RGN_Create(hHandle, &stRgnAttr));

    memset(&stChnPort, 0, sizeof(MI_RGN_ChnPort_t));
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = 0;
    stChnPort.s32OutputPortId = 0;
    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 0;
    stChnPortParam.stPoint.u32Y = 0;
    stChnPortParam.stCoverPara.u32Layer = 0;
    stChnPortParam.stCoverPara.stSize.u32Width = 100;
    stChnPortParam.stCoverPara.stSize.u32Height = 100;
    stChnPortParam.stCoverPara.u32Color = 0;
    s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
    if (s32Ret != MI_RGN_OK)
    {
        printf("MI_RGN_AttachToChn failed, s32Ret:%X\n", s32Ret);
        return NULL;
    }
    // STCHECKRESULT(MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam));
    s32Ret = MI_RGN_UpdateCanvas(hHandle);
    if (s32Ret != MI_RGN_OK)
    {
        printf("MI_RGN_UpdateCanvas failed, s32Ret:%X, %d\n", s32Ret, s32Ret == MI_ERR_RGN_INVALID_HANDLE);
        return NULL;
    }
    // STCHECKRESULT(MI_RGN_UpdateCanvas(hHandle));
#if 0
    FILE *pFile = fopen(pstArgs->szFileName, "rb");
    if (pFile == NULL)
    {
        printf("open file %s failed \n", pstArgs->szFileName);
        return;
    }

    u32FileSize = u16PicW * u16PicH * 2;
    pu8FileBuffer = (MI_U8*)malloc(u32FileSize);
    if (pu8FileBuffer == NULL)
    {
        printf("malloc failed fileSize=%d\n", u32FileSize);
        fclose(pFile);
        return;
    }

    memset(pu8FileBuffer, 0, u32FileSize);
    fread(pu8FileBuffer, 1,  u32FileSize, pFile);
    fclose(pFile);

    stBitmap.stSize.u32Width = u16PicW;
    stBitmap.stSize.u32Height = u16PicH;
    stBitmap.ePixelFormat = E_MI_RGN_PIXEL_FORMAT_RGB1555;
    stBitmap.pData = pu8FileBuffer;
    STCHECKRESULT(MI_RGN_SetBitMap(hHandle, &stBitmap));

    free(pu8FileBuffer);
#endif

    sleep(10);
    s32Ret = MI_RGN_Destroy(hHandle);
    if (s32Ret != MI_RGN_OK)
    {
        printf("MI_RGN_Destroy failed, s32Ret:%X\n", s32Ret);
        return NULL;
    }
    // STCHECKRESULT(MI_RGN_Destroy(hHandle));

    return NULL;
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
    }
}

int ST_VdecH26X(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 index = 0;
    MI_U32 i = 0;
    char szCmd[16];
    ST_Rect_t stRect;
    MI_U32 u32Square = 0;
    MI_DISP_OutputTiming_e eTiming = E_MI_DISP_OUTPUT_1080P60;
    ST_Rect_t stVdispOutRect;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;

    if (eTiming == E_MI_DISP_OUTPUT_1080P60)
    {
        stVdispOutRect.s32X     = 0;
        stVdispOutRect.s32Y     = 0;
        stVdispOutRect.u16PicW  = 1920;
        stVdispOutRect.u16PicH  = 1080;
    }

    /************************************************
    step1:  init SYS
    *************************************************/
    ST_Sys_Init();

    // init hdmi
    STCHECKRESULT(ST_Hdmi_Init()); //Set hdmi outout 1080P
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, E_MI_HDMI_TIMING_1080_60P));

    /************************************************
    step2:  create VDEC chn/VIF chn
    *************************************************/
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_CHN vdecChn = 0;
    ST_VIF_PortInfo_t stVifPortInfoInfo;

    // check if need init vif device
    MI_BOOL bNeedVifDev = FALSE;
    for (i = 0; i < u32WndNum; i ++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_CHN ||
            pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_VENC_CHN)
        {
            bNeedVifDev = TRUE;
            break;
        }
    }

    MI_U32 u32DevNum = 0;
    MI_U32 u32CapChnNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;

    if (bNeedVifDev)
    {
        u32DevNum = u32CapChnNum / 4;
        if ((u32CapChnNum % 4) > 0)
        {
            u32DevNum += 1;
        }

        for (i = 0; i < u32DevNum; i++) //init vif device
        {
            STCHECKRESULT(ST_Vif_CreateDev(i));
        }
    }

    for (i = 0; i < u32WndNum; i ++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VDEC_CHN)
        {
            vdecChn = i;
            memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
            stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
            stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
            stVdecChnAttr.u32BufSize    = 512 * 1024;// 1 * 1024 * 1024; //
            stVdecChnAttr.u32PicWidth   = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stVdecChnAttr.u32PicHeight  = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
            stVdecChnAttr.u32Priority   = 0;
            stVdecChnAttr.eCodecType    = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.eCodecType;

            ExecFunc(MI_VDEC_CreateChn(vdecChn, &stVdecChnAttr), MI_SUCCESS);
            ExecFunc(MI_VDEC_StartChn(vdecChn), MI_SUCCESS);
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_CHN ||
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_VENC_CHN)
        {
            //init vif channel
            memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
            stVifPortInfoInfo.u32RectX = 0;
            stVifPortInfoInfo.u32RectY = 0;
            stVifPortInfoInfo.u32RectWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32RectHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;

            stVifPortInfoInfo.u32DestWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32DestHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;

            STCHECKRESULT(ST_Vif_CreatePort(g_stVifChnCfg[i].u8ViChn, g_stVifChnCfg[i].u8ViPort, &stVifPortInfoInfo));
            STCHECKRESULT(ST_Vif_StartPort(g_stVifChnCfg[i].u8ViDev, g_stVifChnCfg[i].u8ViChn, g_stVifChnCfg[i].u8ViPort));
        }
    }

    /************************************************
    step3:  create DIVP chn
    *************************************************/
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    MI_U32 u32MaxWidth = 0, u32MaxHeight = 0;

    for (i = 0; i < u32WndNum; i ++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VDEC_CHN)
        {
            u32MaxWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            u32MaxHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_CHN ||
                 pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_VENC_CHN)
        {
            u32MaxWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            u32MaxHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        }

        divpChn = i;
        memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
        stDivpChnAttr.bHorMirror            = FALSE;
        stDivpChnAttr.bVerMirror            = FALSE;
        stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X       = 0;
        stDivpChnAttr.stCropRect.u16Y       = 0;
        stDivpChnAttr.stCropRect.u16Width   = u32MaxWidth; //Vdec pic w
        stDivpChnAttr.stCropRect.u16Height  = u32MaxHeight; //Vdec pic h
        stDivpChnAttr.u32MaxWidth           = u32MaxWidth; //max size picture can pass
        stDivpChnAttr.u32MaxHeight          = u32MaxHeight;

        ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
        ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

        memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
        stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
        stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stOutputPortAttr.u32Width           = u32MaxWidth;
        stOutputPortAttr.u32Height          = u32MaxHeight;

        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }

    // start VPE chn
    // because DIVP can not scale up, VPE can
    ST_VPE_ChannelInfo_t stVpeChnInfo;
    ST_VPE_PortInfo_t stVpePortInfo;
    MI_VPE_CHANNEL vpeChn = 0;

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
    for (i = 0; i < u32WndNum; i ++)
    {
        memset(&stVpeChnInfo, 0, sizeof(ST_VPE_ChannelInfo_t));
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_t));
        vpeChn = i;

        stVpeChnInfo.u16VpeMaxW = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 2);
        stVpeChnInfo.u16VpeMaxH = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);

        stVpeChnInfo.u32X = 0;
        stVpeChnInfo.u32Y = 0;
        stVpeChnInfo.u16VpeCropW = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 2);
        stVpeChnInfo.u16VpeCropH = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);

        STCHECKRESULT(ST_Vpe_CreateChannel(vpeChn, &stVpeChnInfo));

        stVpePortInfo.DepVpeChannel = vpeChn;
        stVpePortInfo.u16OutputWidth = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 2);
        stVpePortInfo.u16OutputHeight = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);
        stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;

        STCHECKRESULT(ST_Vpe_CreatePort(0, &stVpePortInfo));
        STCHECKRESULT(ST_Vpe_StartChannel(vpeChn));
    }
    /************************************************
    step4:  start DISP device and chn
    *************************************************/
    // MI_DISP_OutputTiming_e eTiming = E_MI_DISP_OUTPUT_1080P60;
    // ST_Disp_DevInit_Ex(ST_DISP_DEV0, eTiming);

    /************************************************
    step5:  start VDISP device and port
    *************************************************/
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_S32 s32FrmRate = 30;
    MI_S32 s32OutputPort = 0;
    MI_VDISP_PORT vdispPort = 0;

    STCHECKRESULT(ST_Vdisp_Init());
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev,
                    s32OutputPort, &stVdispOutRect, s32FrmRate, 1));
    STCHECKRESULT(ST_Vdisp_StartDevice(vdispDev));
    // need init after vdisp, else driver will print
    // mi_display_ResetPts[1822]: last u64FiredTimeStamp = 815112189,
    ST_Disp_DevInit(ST_DISP_DEV0, eTiming);

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

    for (i = 0; i < u32WndNum; i ++)
    {
        vdispPort = i;
        stRect.s32X     = ALIGN_BACK((stVdispOutRect.u16PicW / u32Square) * (i % u32Square), 2);
        stRect.s32Y     = ALIGN_BACK((stVdispOutRect.u16PicH / u32Square) * (i / u32Square), 2);;
        stRect.u16PicW  = ALIGN_BACK(stVdispOutRect.u16PicW / u32Square, 2);
        stRect.u16PicH  = ALIGN_BACK(stVdispOutRect.u16PicH / u32Square, 2);

        STCHECKRESULT(ST_Vdisp_SetInputPortAttr(vdispDev, vdispPort, &stRect));
        STCHECKRESULT(ST_Vdisp_EnableInputPort(vdispDev, vdispPort));
    }

    /************************************************
    step6:  bind VDEC to DIVP/VIF->VPE
    *************************************************/
    ST_Sys_BindInfo_t stBindInfo;
    for (i = 0; i < u32WndNum; i ++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VDEC_CHN)
        {
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
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_CHN ||
                 pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VIF_VENC_CHN)
        {
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
            stBindInfo.stSrcChnPort.u32DevId = g_stVifChnCfg[i].u8ViDev;
            stBindInfo.stSrcChnPort.u32ChnId = g_stVifChnCfg[i].u8ViChn;
            stBindInfo.stSrcChnPort.u32PortId = g_stVifChnCfg[i].u8ViPort;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = i;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
        }

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    /************************************************
    step7:  bind DIVP to VDISP
    *************************************************/
    for (i = 0; i < u32WndNum; i ++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    /************************************************
    step8:  bind VDISP to DISP
    *************************************************/
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

    /************************************************
    step9:  send es stream to vdec
    *************************************************/
    VDEC_Thread_Args_t stVdecThreadArgs[MAX_CHN_NUM];
    for (i = 0; i < u32WndNum; i ++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType == E_ST_VDEC_CHN)
        {
            stVdecThreadArgs[i].vdecChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32Chn;
            snprintf(stVdecThreadArgs[i].szFileName, sizeof(stVdecThreadArgs[i].szFileName) - 1,
                    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.szFilePath);

            pthread_create(&stVdecThreadArgs[i].pt, NULL, ST_VdecSendStream, (void *)&stVdecThreadArgs[i]);
        }
    }

    /************************************************
    stepX:  create rgn and attach to VPE,
    *************************************************/
    RGN_Thread_Args_t stRgnThreadArgs[MAX_CHN_NUM];
    for (i = 0; i < u32WndNum; i ++)
    {
        stRgnThreadArgs[i].vpeChn = i;
        stRgnThreadArgs[i].rgnHandle = (MI_RGN_HANDLE)i;

        pthread_create(&stRgnThreadArgs[i].pt, NULL, ST_RgnProc, (void *)&stRgnThreadArgs[i]);
    }
}

void ST_DealCase(int argc, char **argv)
{
    MI_U32 u32Index = 0;
    MI_U32 u32SubIndex = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;

    if (argc == 2)
    {
        u32Index = atoi(argv[1]);
    }
    else if (argc == 3)
    {
        u32Index = atoi(argv[1]);
        u32SubIndex = atoi(argv[2]);
    }
    else
    {
        return;
    }

    if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stCaseDesc))
    {
        printf("case index range (%d~%d)\n", 1, ARRAY_SIZE(g_stCaseDesc));
        return;
    }
    g_u32CaseIndex = u32Index - 1;

    if (pstCaseDesc[g_u32CaseIndex].u32SubCaseNum != 0)
    {
        if (u32SubIndex <= 0 || u32SubIndex > pstCaseDesc[g_u32CaseIndex].u32SubCaseNum)
        {
            printf("sub case index range (%d~%d)\n", 1, pstCaseDesc[g_u32CaseIndex].u32SubCaseNum);
            return;
        }
    }

    g_u32LastSubCaseIndex = pstCaseDesc[g_u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[g_u32CaseIndex].u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;

    printf("case index %d, sub case %d-%d\n", g_u32CaseIndex, g_u32CurSubCaseIndex, g_u32LastSubCaseIndex);

    ST_VdecH26X();

    g_u32CurSubCaseIndex = u32SubIndex - 1;

    ST_WaitSubCmd();
}


int main(int argc, char **argv)
{
    MI_U32 u32WndNum = 4;
    int index = 0;
    char szCmd[16];

    ST_DealCase(argc, argv);

    while (!g_bExit)
    {
        ST_RgnUsage();
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        index = atoi(szCmd);

        if (index <= 0 || index > ARRAY_SIZE(g_stCaseDesc))
        {
            continue;
        }

        g_u32CaseIndex = index - 1;

        ST_VdecH26X();
        ST_WaitSubCmd();
    }

    return 0;
}
