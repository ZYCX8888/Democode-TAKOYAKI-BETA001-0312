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
#include <unistd.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp.h"
#include "mi_rgn.h"

#include "st_common.h"
#include "st_vif.h"
#include "st_vpe.h"
#include "st_venc.h"
#include "st_disp.h"
#include "st_fb.h"

#include "dot_matrix_font.h"
#define VENC_H264_ALIGN_W   32
#define VENC_H264_ALIGN_H   8

#define DISP_XPOS_ALIGN     2
#define DISP_YPOS_ALIGN     2
#define DISP_WIDTH_ALIGN    2
#define DISP_HEIGHT_ALIGN   2

#define DIVP_WIDTH_ALIGN    2
#define DIVP_HEIGHT_ALIGN   2

#define FB_SHOW_ON_PANEL    1

#define VENC_FILE_PATH      "/mnt"

#define SUPPORT_RGN_TEST    1

typedef struct
{
    pthread_t pt;
    pthread_t pt_osd;
    MI_BOOL bRunFlag;
    MI_U32 u32VdecChn;

    char szFileName[64];
} ST_VdecInfo_T;

typedef struct
{
    pthread_t pt;
    pthread_t pt_osd;
    pthread_t pt_output;
    MI_VENC_CHN vencChn;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_VENC_ModType_e eType;
    MI_BOOL bRunFlag;
} ST_VencInfo_T;

#define PANEL_DISP_W 720
#define PANEL_DISP_H 1280

#define HDMI_DISP_W 1920
#define HDMI_DISP_H 1080

static MI_U32 g_u32CaseIndex = -1;
static MI_BOOL g_bExit = FALSE;
static ST_VdecInfo_T g_stVdecInfo;
static ST_VencInfo_T g_stVencInfo;

ST_CaseDesc_t g_stCaseDesc[] =
{
    {
        .stDesc =
        {
            .u32CaseIndex = 0,
            .szDesc = "1x1080P@30 H264 Decode + Panel Dispout",
            .u32WndNum = 1,
        },
#if FB_SHOW_ON_PANEL
        .eDispoutTiming = E_ST_TIMING_USER_CUSTOM,
        .u32DispWidth = PANEL_DISP_W,
        .u32DispHeight = PANEL_DISP_H,
#else
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32DispWidth = HDMI_DISP_W,
        .u32DispHeight = HDMI_DISP_H,
#endif
        .enLinkType = E_MI_PNL_LINK_MIPI_DSI,
        .u32SubCaseNum = 0,
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32VdecChn = 0,
                        .u32DivpChn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = ST_1080P_H264_25_FILE,
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
            .szDesc = "1x1080P@30 VIF + Panel Dispout + Venc",
            .u32WndNum = 1,
        },
#if FB_SHOW_ON_PANEL
        .eDispoutTiming = E_ST_TIMING_USER_CUSTOM,
        .u32DispWidth = PANEL_DISP_W,
        .u32DispHeight = PANEL_DISP_H,
#else
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32DispWidth = HDMI_DISP_W,
        .u32DispHeight = HDMI_DISP_H,
#endif
        .enLinkType = E_MI_PNL_LINK_MIPI_DSI,
        .u32SubCaseNum = 0,
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32VifChn = 0,
                        .u32VpeChn = 0,
                        .u32VencChn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    },
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 2,
            .szDesc = "1x1080P@30 VIF + 1x1080P@30Vdec + Panel Disp",
            .u32WndNum = 2,
        },
#if FB_SHOW_ON_PANEL
        .eDispoutTiming = E_ST_TIMING_USER_CUSTOM,
        .u32DispWidth = PANEL_DISP_W,
        .u32DispHeight = PANEL_DISP_H,
#else
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32DispWidth = HDMI_DISP_W,
        .u32DispHeight = HDMI_DISP_H,
#endif
        .enLinkType = E_MI_PNL_LINK_MIPI_DSI,
        .u32SubCaseNum = 0,
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VDEC_CHN,
                .uChnArg =
                {
                    .stVdecChnArg =
                    {
                        .u32VdecChn = 0,
                        .u32DivpChn = 0,
                        .eCodecType = E_MI_VDEC_CODEC_TYPE_H264,
                        .szFilePath = ST_1080P_H264_25_FILE,
                        .u32MaxWidth = 1920,
                        .u32MaxHeight = 1080,
                    },
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32VifChn = 0,
                        .u32VpeChn = 0,
                        .u32VencChn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    },
                }
            }
        },
    }
};

static MI_S32 g_VencFlag = 1;
void *ST_VencGetEsBufferProc(void *args)
{
    MI_SYS_ChnPort_t stVencChnInputPort;
    MI_S32 fd[10] = {0,0,0};
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0, i;
    MI_U32 u32DevId = 0;
    MI_S32 s32VencChn = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack0;
    MI_VENC_ChnStat_t stStat;
    MI_S32 totalLen = 0;
    MI_S32 s32FdStream[3] = {0,0,0};
    fd_set fdsread;
    struct timeval tv;

    MI_S32 maxfd = 0;

    s32FdStream[0] = MI_VENC_GetFd(s32VencChn);
    if(s32FdStream[0] <= 0)
    {
        printf("Unable to get FD:%d for ch:%2d\n", s32FdStream[0], s32VencChn);
        return NULL;
    }
    else
    {
        printf("CH%2d FD%d\n", s32VencChn, s32FdStream[0]);
    }
    s32FdStream[1] = MI_VENC_GetFd(s32VencChn + 1);
    if(s32FdStream[1] <= 0)
    {
        printf("Unable to get FD:%d for ch:%2d\n", s32FdStream[1], s32VencChn + 1);
        return NULL;
    }
    else
    {
        printf("CH%2d FD%d\n", s32VencChn + 1, s32FdStream[1]);
    }
    s32FdStream[2] = MI_VENC_GetFd(s32VencChn +2);
    if(s32FdStream[2] <= 0)
    {
        printf("Unable to get FD:%d for ch:%2d\n", s32FdStream[2], s32VencChn + 2);
        return NULL;
    }
    else
    {
        printf("CH%2d FD%d\n", s32VencChn + 2, s32FdStream[2]);
    }


    fd[0] = open("chn_1.es", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd[0] <= 0)
    {
        printf("%s %d create error\n", __func__, __LINE__);
        return NULL;
    }
    fd[1] = open("chn_2.es", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd[1] <= 0)
    {
        printf("%s %d create error\n", __func__, __LINE__);
        return NULL;
    }
    fd[2] = open("chn_3.es", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd[2] <= 0)
    {
        printf("%s %d create error\n", __func__, __LINE__);
        return NULL;
    }

    printf("%s %d create success\n", __func__, __LINE__);

    if (s32FdStream[0] > maxfd)
    {
        maxfd = s32FdStream[0];
    }
    if (s32FdStream[1] > maxfd)
    {
        maxfd = s32FdStream[1];
    }
    if (s32FdStream[2] > maxfd)
    {
        maxfd = s32FdStream[2];
    }
    printf("s32FdStream[0]=(%d).....s32FdStream[1]=(%d).s32FdStream[2]=(%d).\n", s32FdStream[0], s32FdStream[1], s32FdStream[2]);

    while (g_VencFlag)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fdsread);
        FD_SET(s32FdStream[0], &fdsread);
        FD_SET(s32FdStream[1], &fdsread);
        FD_SET(s32FdStream[2], &fdsread);
        s32Ret = select(maxfd + 1, &fdsread, NULL, NULL, &tv);
        if(s32Ret < 0)
        {
            perror("select");
        }
        else if(s32Ret == 0)
        {
            printf("timeout\n");
        }
        else
        {
            for (i = 0; i < 3; i++)
            {
                if(FD_ISSET(s32FdStream[i],&fdsread))
                {
                    s32VencChn = i;
                    memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
                    memset(&stStream, 0, sizeof(stStream));
                    memset(&stPack0, 0, sizeof(stPack0));
                    stStream.pstPack = &stPack0;
                    stStream.u32PackCount = 1;
                    s32Ret = MI_VENC_Query(s32VencChn, &stStat);
                    if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
                    {
                        continue;
                    }
                    s32Ret = MI_VENC_GetStream(s32VencChn, &stStream, 10);
                    if (MI_SUCCESS == s32Ret)
                    {
                        if (totalLen >= 100 * 1024 * 1024)
                        {
                            totalLen = 0;
                            lseek(fd[s32VencChn], 0, SEEK_SET);
                        }
                        if (1 == g_u32CaseIndex)
                        {
                            len = write(fd[s32VencChn], stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
                            if (len != stStream.pstPack[0].u32Len)
                            {
                                printf("write es buffer fail.\n");
                            }
                        }
                        if (MI_SUCCESS != (s32Ret = MI_VENC_ReleaseStream(s32VencChn, &stStream)))
                        {
                            printf("%s %d, MI_VENC_ReleaseStream error, %X\n", __func__, __LINE__, s32Ret);
                        }

                        totalLen += len;
                    }
                    else
                    {
                        if ((MI_ERR_VENC_NOBUF != s32Ret) && (MI_ERR_SYS_NOBUF != s32Ret))
                        {
                            printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X %x\n", __func__, __LINE__, s32Ret, MI_ERR_VENC_BUF_EMPTY);
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < 3; i++)
    {
        close(fd[i]);
    }
}

MI_S32 CreateVencChannel(MI_S32 s32VencChn, MI_S32 s32VencType, MI_U16 u16Width, MI_U16 u16Height, MI_S32 s32FrameRate)
{
    // main+sub venc
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
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = s32FrameRate;
        stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

        stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 36;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 36;
#else
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
        if (0 == s32VencChn)
        {
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*1;
        }
        else
        {
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 1024*1024*4;
        }
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
    s32Ret = MI_VENC_CreateChn(s32VencChn, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    if (E_MI_VENC_MODTYPE_JPEGE == s32VencType)
    {
        MI_VENC_ParamJpeg_t stParamJpeg;

        memset(&stParamJpeg, 0, sizeof(stParamJpeg));
        s32Ret = MI_VENC_GetJpegParam(s32VencChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
        printf("Get Qf:%d\n", stParamJpeg.u32Qfactor);

        stParamJpeg.u32Qfactor = 30;
        s32Ret = MI_VENC_SetJpegParam(s32VencChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
    }

    s32Ret = MI_VENC_GetChnDevid(s32VencChn, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    stVencChnOutputPort.u32DevId = u32DevId;
    stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnOutputPort.u32ChnId = s32VencChn;
    stVencChnOutputPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 5, 10);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }

    s32Ret = MI_VENC_StartRecvPic(s32VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
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

void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_ChnPort_t stVencChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS, s32VoChannel = 0;
    MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;
    FILE *fp = NULL;

    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hVencHandle;
    MI_SYS_BufInfo_t stVencBufInfo;
    MI_U32 u32DevId = 0;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 1;
    stChnPort.u32PortId = 0;

    s32ReadCnt = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4); //Default queue frame depth--->20

    MI_VENC_GetChnDevid(2, &u32DevId);
    memset(&stVencChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stVencChnPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnPort.u32DevId = u32DevId;
    stVencChnPort.u32ChnId = 2;
    stVencChnPort.u32PortId = 0;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = 1280;
    stBufConf.stFrameCfg.u16Height = 720;

    fp = fopen("dump_vpe1_port0.yuv","wb");
    while (1)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

            if (50 == s32ReadCnt++)
            {
                if (fp)
                {
                    #if 0//YUYV
                    fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                    #endif
                    //fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                    //fwrite(stBufInfo.stFrameData.pVirAddr[1], size/2, 1, fp);
                    sleep(1);
                    fclose(fp);
                }
                printf("\t vif(%d) size(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, size, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
            }
            #if 0
            memset(&stVencBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
            s32Ret = MI_SYS_ChnInputPortGetBuf(&stVencChnPort, &stBufConf, &stVencBufInfo, &hVencHandle, 1000);
            if(MI_SUCCESS == s32Ret)
            {
                memcpy(stVencBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[0], 1280*720);
                memcpy(stVencBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[1], 1280*360);

                MI_SYS_FlushInvCache(stVencBufInfo.stFrameData.pVirAddr[0], 1280*720);
                MI_SYS_FlushInvCache(stVencBufInfo.stFrameData.pVirAddr[1], 1280*360);

                s32Ret = MI_SYS_ChnInputPortPutBuf(hVencHandle ,&stVencBufInfo , FALSE);
                if (MI_SUCCESS != s32Ret)
                {
                    printf("%s %d, MI_SYS_ChnInputPortPutBuf error, %X\n", __func__, __LINE__, s32Ret);
                }
            }
            #endif
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
    ST_VdecInfo_T *pstVdecInfo = NULL;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    char tname[32];
    memset(tname, 0, 32);

    pstVdecInfo = (ST_VdecInfo_T *)args;

    vdecChn = pstVdecInfo->u32VdecChn;
    snprintf(tname, 32, "push_t_%u", vdecChn);
    prctl(PR_SET_NAME, tname);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;
    stChnPort.u32PortId = 0;

    readfp = fopen(pstVdecInfo->szFileName, "rb");
    if (!readfp)
    {
        ST_ERR("Open failed!\n");
        return NULL;
    }

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

    s32Ms = 33;
    //printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (pstVdecInfo->bRunFlag)
    {
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

        if(0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1]
            && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]
            && 0x65 == stVdecStream.pu8Addr[4] || 0x61 == stVdecStream.pu8Addr[4]
            || 0x26 == stVdecStream.pu8Addr[4] || 0x02 == stVdecStream.pu8Addr[4]
            || 0x41 == stVdecStream.pu8Addr[4])
        {
            usleep(s32Ms * 1000);
        }

        if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(vdecChn, &stVdecStream, s32TimeOutMs)))
        {
            //ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(readfp, - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + ST_Sys_GetPts(30);

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

static void St_FbInit(MI_U32 u32Width, MI_U32 u32Height)
{
    MI_SYS_WindowRect_t Rect;
    ST_Fb_Init();

    sleep(1);

    ST_Fb_SetColorKey(ARGB888_BLUE);

    // change fb resolution
    ST_DBG("u32Width:%d,u32Height:%d\n", u32Width, u32Height);
    ST_FB_ChangeResolution(u32Width, u32Height);

    // after change resolution, fb default size is 1280x720, if DISP init at 1024x768,
    // set color format will fail
    ST_Fb_SetColorFmt(E_MI_FB_COLOR_FMT_ARGB1555);

#if FB_SHOW_ON_PANEL
    Rect.u16X = 0;
    Rect.u16Y = 0;
    Rect.u16Width = 1280;
    Rect.u16Height = 720;
#else
    Rect.u16X = 0;
    Rect.u16Y = 0;
    Rect.u16Width = 1920;
    Rect.u16Height = 1080;
#endif
    ST_Fb_FillRect(&Rect, ARGB888_BLUE);

    MI_FB_GlobalAlpha_t stAlphaInfo;

    memset(&stAlphaInfo, 0, sizeof(MI_FB_GlobalAlpha_t));

    ST_FB_GetAlphaInfo(&stAlphaInfo);

    printf("FBIOGET_GLOBAL_ALPHA alpha info: alpha blend enable=%d,Multialpha enable=%d,Global Alpha=%d,u8Alpha0=%d,u8Alpha1=%d\n",
            stAlphaInfo.bAlphaEnable,stAlphaInfo.bAlphaChannel,stAlphaInfo.u8GlobalAlpha,stAlphaInfo.u8Alpha0,stAlphaInfo.u8Alpha1);

    stAlphaInfo.bAlphaEnable = TRUE;
    stAlphaInfo.bAlphaChannel= TRUE;
    stAlphaInfo.u8GlobalAlpha = 0xFF;
    ST_FB_SetAlphaInfo(&stAlphaInfo);

    ST_FB_ShowBMP(0, 0, "background_1.bmp");

    ST_FB_Show(TRUE);
}

#if SUPPORT_RGN_TEST
#define RGN_OSD_WIDTH 200
#define RGN_OSD_HEIGHT 200
static MI_S32 g_updateOsdFlag = 0;

MI_S32 St_OsdInit(MI_S32 s32ModId, MI_S32 s32Chn, MI_S32 s32Port)
{
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_S32 s32Ret = -1;
    memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    stRgnAttr.stOsdInitParam.stSize.u32Width = RGN_OSD_WIDTH;
    stRgnAttr.stOsdInitParam.stSize.u32Height = RGN_OSD_HEIGHT;
    hHandle = s32Chn;
    s32Ret = MI_RGN_Create(hHandle, &stRgnAttr);
    if (MI_RGN_OK != s32Ret)
    {
        ST_ERR("MI_RGN_Create error, %X\n", s32Ret);
        return s32Ret;
    }

    stChnPort.eModId = s32ModId;//E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = s32Chn;
    stChnPort.s32OutputPortId = s32Port;

    memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPortParam.bShow = TRUE;
    stChnPortParam.stPoint.u32X = 10;
    stChnPortParam.stPoint.u32Y = 10;
    stChnPortParam.stCoverPara.u32Layer = hHandle;
    stChnPortParam.stCoverPara.stSize.u32Width = RGN_OSD_WIDTH;
    stChnPortParam.stCoverPara.stSize.u32Height = RGN_OSD_HEIGHT;
    stChnPortParam.stCoverPara.u32Color = 0;

    s32Ret = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
    if (MI_RGN_OK != s32Ret)
    {
        ST_ERR("MI_RGN_AttachToChn error, %X\n", s32Ret);
        return s32Ret;
    }

    return hHandle;
}

void *ST_UpdateOsdProc(void *args)
{
    int i = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    time_t now = 0;
	struct tm *tm = NULL;
	char szTime[64];
    int len = 0;
    struct timeval tv;
    unsigned char *pBuf = NULL;
    int maxWidth = 0, maxHeight = 0;
    int minWidth = 0, minHeight = 0;
    DMF_BitMapData_S stBitMapData;
    unsigned int j = 0, k = 0;
    MI_RGN_HANDLE hUpdateHandle = 0;
    MI_RGN_CanvasInfo_t stCanvasInfo;


    if (0 != DMF_LoadBitMapFile(16, 16, "gb2312.hzk"))
    {
        ST_ERR("load bitmap file error\n");
        return NULL;
    }

    maxWidth = 0;
    maxHeight = 0;

    s32Ret = MI_RGN_GetCanvasInfo(hUpdateHandle, &stCanvasInfo);
    if (MI_RGN_OK != s32Ret)
    {
        ST_ERR("MI_RGN_GetCanvasInfo error, %X, handle:%d\n", s32Ret, hUpdateHandle);
    }

    maxWidth = MAX(maxWidth, stCanvasInfo.stSize.u32Width);
    maxHeight = MAX(maxHeight, stCanvasInfo.stSize.u32Height);

    ST_DBG("maxWidth:%d,maxHeight:%d\n", maxWidth, maxHeight);
    pBuf = (unsigned char *)malloc(maxWidth * maxHeight * 2); // default ARGB1555
    if (pBuf == NULL)
    {
        ST_ERR("not enough memory\n");
        return NULL;
    }

    memset(&stBitMapData, 0, sizeof(DMF_BitMapData_S));
    stBitMapData.pBuf = pBuf;
    stBitMapData.bufLen = maxWidth * maxHeight * 2;

    g_updateOsdFlag = 1;
    while (g_updateOsdFlag)
    {
        tv.tv_sec = 1;
		tv.tv_usec = 0;

        if (0 == select(0, NULL, NULL, NULL, &tv))
        {
            // time out
            now = time(NULL);
            if ((tm = localtime(&now)) == NULL)
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

            if (0 != DMF_DumpToBMPBuf(szTime, &stBitMapData))
            {
                continue;
            }

            //
            minWidth = MIN(stBitMapData.width, stCanvasInfo.stSize.u32Width);
            minHeight = MIN(stBitMapData.height, stCanvasInfo.stSize.u32Height);
            MI_RGN_GetCanvasInfo(hUpdateHandle, &stCanvasInfo);
            for (j = 0; j < minHeight; j ++)
            {
                memcpy((MI_U8 *)(stCanvasInfo.virtAddr + j * stCanvasInfo.u32Stride),
                    stBitMapData.pBuf + j * stBitMapData.width * 2, stBitMapData.width * 2);
            }
            s32Ret = MI_RGN_UpdateCanvas(hUpdateHandle);
            if (MI_RGN_OK != s32Ret)
            {
                ST_ERR("MI_RGN_UpdateCanvas error, %X, handle:%d\n", s32Ret, hUpdateHandle);
            }
        }
    }

    if (pBuf)
    {
        free(pBuf);
        pBuf = NULL;
    }

    DMF_CloseBitMapFile();

    return NULL;
}
#endif

int ST_DVRH26X(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    ST_Rect_t stDispOutRect;
    MI_U32 u32WndNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;
    MI_U32 i = 0;
    ST_Rect_t stDispWndRect[32] = {0};

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    if (E_ST_TIMING_USER_CUSTOM == pstCaseDesc[u32CaseIndex].eDispoutTiming)
    {
        stDispOutRect.u16PicW = pstCaseDesc[u32CaseIndex].u32DispWidth;
        stDispOutRect.u16PicH = pstCaseDesc[u32CaseIndex].u32DispHeight;

        // default disp rect
        stDispWndRect[0].s32X = 0;
        stDispWndRect[0].s32Y = 0;
        #if FB_SHOW_ON_PANEL
        stDispWndRect[0].u16PicW = PANEL_DISP_W;
        stDispWndRect[0].u16PicH = PANEL_DISP_H;
        #else
        stDispWndRect[0].u16PicW = HDMI_DISP_W;
        stDispWndRect[0].u16PicH = HDMI_DISP_H;
        #endif
        stDispWndRect[1].s32X = 100;
        stDispWndRect[1].s32Y = 320;
        stDispWndRect[1].u16PicW = 320;
        stDispWndRect[1].u16PicH = 240;
    }
    else
    {
        STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[u32CaseIndex].eDispoutTiming,
                &s32HdmiTiming, &s32DispTiming, &stDispOutRect.u16PicW, &stDispOutRect.u16PicH));

        // default disp rect
        stDispWndRect[0].s32X = 0;
        stDispWndRect[0].s32Y = 0;
        stDispWndRect[0].u16PicW = 1920;
        stDispWndRect[0].u16PicH = 1080;

#if 0
        stDispWndRect[1].s32X = 960;
        stDispWndRect[1].s32Y = 0;
        stDispWndRect[1].u16PicW = 960;
        stDispWndRect[1].u16PicH = 540;
#endif
    }

    stDispOutRect.s32X     = 0;
    stDispOutRect.s32Y     = 0;

    // init hdmi
    // STCHECKRESULT(ST_Hdmi_Init());
    // MI_HDMI_SetAvMute(E_MI_HDMI_ID_0, TRUE);

    /************************************************
    Step2:  init VIF dev
    *************************************************/
    ST_VIF_PortInfo_T stVifPortInfoInfo;

    STCHECKRESULT(ST_Vif_EnableDev(0, SAMPLE_VI_MODE_MIPI_1_1080P_FRAME));

    /************************************************
    step2:  create VDEC chn
    *************************************************/
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_CHN vdecChn = 0;
    MI_VIF_CHN vifChn = 0;

    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            vdecChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn;

            // create VDEC chn
            memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
            stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
            stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
            stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
            stVdecChnAttr.u32PicWidth   = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stVdecChnAttr.u32PicHeight  = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;
            stVdecChnAttr.u32Priority   = 0;
            stVdecChnAttr.eCodecType    = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.eCodecType;

            ExecFunc(MI_VDEC_CreateChn(vdecChn, &stVdecChnAttr), MI_SUCCESS);
            ExecFunc(MI_VDEC_StartChn(vdecChn), MI_SUCCESS);

            // create DIVP
            MI_DIVP_ChnAttr_t stDivpChnAttr;
            MI_DIVP_OutputPortAttr_t stOutputPortAttr;
            MI_DIVP_CHN divpChn = 0;

            divpChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn;
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
            stDivpChnAttr.u32MaxWidth           = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxWidth;
            stDivpChnAttr.u32MaxHeight          = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32MaxHeight;

            ExecFunc(MI_DIVP_CreateChn(divpChn, &stDivpChnAttr), MI_SUCCESS);
            ExecFunc(MI_DIVP_StartChn(divpChn), MI_SUCCESS);

            memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
            stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
            stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stOutputPortAttr.u32Width           = ALIGN_BACK(stDispWndRect[i].u16PicW, DISP_WIDTH_ALIGN);
            stOutputPortAttr.u32Height          = ALIGN_BACK(stDispWndRect[i].u16PicH, DISP_HEIGHT_ALIGN);

            STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            // create VIF port
            vifChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn;

            memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_T));
            stVifPortInfoInfo.u32RectX = 0;
            stVifPortInfoInfo.u32RectY = 0;
            stVifPortInfoInfo.u32RectWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32RectHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            stVifPortInfoInfo.u32DestWidth = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVifPortInfoInfo.u32DestHeight = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            stVifPortInfoInfo.s32FrameRate = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
            stVifPortInfoInfo.u32IsInterlace = FALSE;
            stVifPortInfoInfo.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_RG;
            STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0, &stVifPortInfoInfo));

            // init IQ service
            MI_IQServer_Open(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth,
                             pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 0);

            // create VPE
            ST_VPE_ChannelInfo_T stVpeChannelInfo;
            ST_VPE_PortInfo_T stPortInfo;
            MI_VPE_CHANNEL vpeChannel = 0;

            vpeChannel = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;

            memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
            stVpeChannelInfo.u16VpeMaxW = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            stVpeChannelInfo.u16VpeMaxH = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            stVpeChannelInfo.u32X = 0;
            stVpeChannelInfo.u32Y = 0;
            stVpeChannelInfo.u16VpeCropW = 0;
            stVpeChannelInfo.u16VpeCropH = 0;
            stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_RG;

            stVpeChannelInfo.eRunningMode = E_MI_VPE_RUNNING_MODE_FRAMEBUF_CAM_MODE;
            stVpeChannelInfo.eSensorBindId = E_MI_VPE_SENSOR0;

            STCHECKRESULT(ST_Vpe_CreateChannel(vpeChannel, &stVpeChannelInfo));
            STCHECKRESULT(ST_Vpe_StartChannel(vpeChannel));

            memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_T));
            stPortInfo.DepVpeChannel = vpeChannel;
            stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stPortInfo.u16OutputWidth = stDispWndRect[i].u16PicW;
            stPortInfo.u16OutputHeight = stDispWndRect[i].u16PicH;

            ST_DBG("vpe port %d, size, %dx%d\n", DISP_PORT, stPortInfo.u16OutputWidth, stPortInfo.u16OutputHeight);
            STCHECKRESULT(ST_Vpe_StartPort(DISP_PORT, &stPortInfo));

            memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_T));
            stPortInfo.DepVpeChannel = vpeChannel;
            stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stPortInfo.u16OutputWidth = 1920;
            stPortInfo.u16OutputHeight = 1080;
            STCHECKRESULT(ST_Vpe_StartPort(MAIN_VENC_PORT, &stPortInfo));

            CreateVencChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn, E_MI_VENC_MODTYPE_H264E,
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth,
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 30);
            memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_T));
            stPortInfo.DepVpeChannel = vpeChannel;
            stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            STCHECKRESULT(ST_Vpe_StartPort(SUB_VENC_PORT, &stPortInfo));
            CreateVencChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn+1, E_MI_VENC_MODTYPE_H264E,
                1280,
                720, 30);
            memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_T));
            stPortInfo.DepVpeChannel = vpeChannel;
            stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stPortInfo.u16OutputWidth = 1280;
            stPortInfo.u16OutputHeight = 720;
            STCHECKRESULT(ST_Vpe_StartPort(JPEG_VENC_PORT, &stPortInfo));
            CreateVencChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn+2, E_MI_VENC_MODTYPE_JPEGE,
                1280,
                720, 30);
            //only case 1 to create vpe channel-1
            {
                //Create vpe1 for ai detect
                vpeChannel += 1;
                memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
                stVpeChannelInfo.u16VpeMaxW = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
                stVpeChannelInfo.u16VpeMaxH = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
                stVpeChannelInfo.u32X = 0;
                stVpeChannelInfo.u32Y = 0;
                stVpeChannelInfo.u16VpeCropW = 0;
                stVpeChannelInfo.u16VpeCropH = 0;
                stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_RG;

                stVpeChannelInfo.eRunningMode = E_MI_VPE_RUNNING_MODE_FRAMEBUF_CAM_MODE;
                stVpeChannelInfo.eSensorBindId = E_MI_VPE_SENSOR_INVALID;

                STCHECKRESULT(ST_Vpe_CreateChannel(vpeChannel, &stVpeChannelInfo));
                STCHECKRESULT(ST_Vpe_StartChannel(vpeChannel));

                memset(&stPortInfo, 0, sizeof(ST_VPE_PortInfo_T));
                stPortInfo.DepVpeChannel = vpeChannel;
                stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
                stPortInfo.u16OutputWidth = 1280;
                stPortInfo.u16OutputHeight = 720;
                STCHECKRESULT(ST_Vpe_StartPort(MAIN_VENC_PORT, &stPortInfo)); //vpe1
                MI_SYS_ChnPort_t stChnPort;
                memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
                stChnPort.eModId = E_MI_MODULE_ID_VPE;
                stChnPort.u32DevId = 0;
                stChnPort.u32ChnId = vpeChannel;
                stChnPort.u32PortId = MAIN_VENC_PORT;
                MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4); //Default queue frame depth--->20
            }
        }
    }

    /************************************************
    step4:  start DISP device and chn
    *************************************************/
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_PANEL_ParamConfig_t *pstPanelConfig = NULL;

    pstPanelConfig = (MI_PANEL_ParamConfig_t *)ST_Panel_GetParam((MI_PANEL_LinkType_e)pstCaseDesc[u32CaseIndex].enLinkType);

    if (E_ST_TIMING_USER_CUSTOM == pstCaseDesc[u32CaseIndex].eDispoutTiming)
    {
        memset(&stPubAttr, 0, sizeof(stPubAttr));
        stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
        stPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
        //720x1280
        stPubAttr.stSyncInfo.u16Vact = pstPanelConfig->u16Height;
        stPubAttr.stSyncInfo.u16Vbb = pstPanelConfig->u16VSyncBackPorch;
        stPubAttr.stSyncInfo.u16Vfb = pstPanelConfig->u16VTotal - (pstPanelConfig->u16VSyncWidth + pstPanelConfig->u16Height + pstPanelConfig->u16VSyncBackPorch);
        stPubAttr.stSyncInfo.u16Hact = pstPanelConfig->u16Width;
        stPubAttr.stSyncInfo.u16Hbb = pstPanelConfig->u16HSyncBackPorch;
        stPubAttr.stSyncInfo.u16Hfb = pstPanelConfig->u16HTotal - (pstPanelConfig->u16HSyncWidth + pstPanelConfig->u16Width + pstPanelConfig->u16HSyncBackPorch);
        stPubAttr.stSyncInfo.u16Bvact = 0;
        stPubAttr.stSyncInfo.u16Bvbb = 0;
        stPubAttr.stSyncInfo.u16Bvfb = 0;
        stPubAttr.stSyncInfo.u16Hpw = pstPanelConfig->u16HSyncWidth;
        stPubAttr.stSyncInfo.u16Vpw = pstPanelConfig->u16VSyncWidth;
        stPubAttr.stSyncInfo.u32FrameRate = pstPanelConfig->u16DCLK*1000000/(pstPanelConfig->u16HTotal*pstPanelConfig->u16VTotal);

        ExecFunc(MI_DISP_SetPubAttr(ST_DISP_DEV0,  &stPubAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_Enable(ST_DISP_DEV0), MI_SUCCESS);

        //set layer0
        memset(&stLayerAttr, 0, sizeof(stLayerAttr));
        stLayerAttr.stVidLayerSize.u16Width = stDispOutRect.u16PicW;
        stLayerAttr.stVidLayerSize.u16Height= stDispOutRect.u16PicH;
        stLayerAttr.stVidLayerDispWin.u16X = 0;
        stLayerAttr.stVidLayerDispWin.u16Y = 0;
        stLayerAttr.stVidLayerDispWin.u16Width = stDispOutRect.u16PicW;
        stLayerAttr.stVidLayerDispWin.u16Height = stDispOutRect.u16PicH;
        stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        ExecFunc(MI_DISP_BindVideoLayer(ST_DISP_LAYER0, ST_DISP_DEV0), MI_SUCCESS);
        ExecFunc(MI_DISP_SetVideoLayerAttr(ST_DISP_LAYER0, &stLayerAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetVideoLayerAttr(ST_DISP_LAYER0, &stLayerAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableVideoLayer(ST_DISP_LAYER0), MI_SUCCESS);

        //set layer1
        memset(&stLayerAttr, 0, sizeof(stLayerAttr));
        stLayerAttr.stVidLayerSize.u16Width = stDispOutRect.u16PicW;
        stLayerAttr.stVidLayerSize.u16Height= stDispOutRect.u16PicH;
        stLayerAttr.stVidLayerDispWin.u16X = 0;
        stLayerAttr.stVidLayerDispWin.u16Y = 0;
        stLayerAttr.stVidLayerDispWin.u16Width = stDispOutRect.u16PicW;
        stLayerAttr.stVidLayerDispWin.u16Height = stDispOutRect.u16PicH;
        stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        ExecFunc(MI_DISP_BindVideoLayer(ST_DISP_LAYER1, ST_DISP_DEV0), MI_SUCCESS);
        ExecFunc(MI_DISP_SetVideoLayerAttr(ST_DISP_LAYER1, &stLayerAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetVideoLayerAttr(ST_DISP_LAYER1, &stLayerAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableVideoLayer(ST_DISP_LAYER1), MI_SUCCESS);
    }
    else
    {
        ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32DispTiming);
    }

#if FB_SHOW_ON_PANEL
    // init panel
    ExecFunc(ST_Panel_Init(pstCaseDesc[u32CaseIndex].enLinkType), MI_SUCCESS);
#else
    // init HDMI
    STCHECKRESULT(ST_Hdmi_Init());
    ExecFunc(MI_HDMI_SetAvMute(E_MI_HDMI_ID_0, FALSE), MI_SUCCESS);
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32HdmiTiming));
#endif

    // init fb
    St_FbInit(stDispOutRect.u16PicW, stDispOutRect.u16PicH);

    ST_DispChnInfo_t stDispChnInfo;

    memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));

    stDispChnInfo.InputPortNum = 1;
    for (i = 0; i < 1; i++)
    {
        stDispChnInfo.stInputPortAttr[0].u32Port = 0;
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X =
            ALIGN_BACK(stDispWndRect[i].s32X, DISP_XPOS_ALIGN);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y =
            ALIGN_BACK(stDispWndRect[i].s32Y, DISP_YPOS_ALIGN);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width =
            ALIGN_BACK(stDispWndRect[i].u16PicW, DISP_WIDTH_ALIGN);
        stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height =
            ALIGN_BACK(stDispWndRect[i].u16PicH, DISP_HEIGHT_ALIGN);
    }
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER0, &stDispChnInfo));
    if (2 == u32CaseIndex)
    {
        for (i = 1; i < 2; i++)
        {
            stDispChnInfo.stInputPortAttr[0].u32Port = 0;
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X =
                ALIGN_BACK(stDispWndRect[i].s32X, DISP_XPOS_ALIGN);
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y =
                ALIGN_BACK(stDispWndRect[i].s32Y, DISP_YPOS_ALIGN);
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width =
                ALIGN_BACK(stDispWndRect[i].u16PicW, DISP_WIDTH_ALIGN);
            stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height =
                ALIGN_BACK(stDispWndRect[i].u16PicH, DISP_HEIGHT_ALIGN);
        }
        STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER1, &stDispChnInfo));
    }

    /************************************************
    step5:  bind modules
    *************************************************/
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = 0;

    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            // bind vdec->divp
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_T));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;

            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

            ST_DBG("vdec chn(%d)->divp chn(%d) bind\n",
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn,
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn);

            // bind divp->disp
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_T));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;

            stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER0;
            if ((2 == u32CaseIndex) && (1 == i))
            {
                stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER1;
            }
            stBindInfo.stDstChnPort.u32PortId = 0; //port0 enable

            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

            ST_DBG("vdec divp(%d)->disp chn(%d) bind\n",
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn, i);
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            // bind vif->vpe
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;

            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;

            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
            //only case 1 to create vpe channel-1
            {
                memset(&stBindInfo, 0x0, sizeof(stBindInfo));

                stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
                stBindInfo.stSrcChnPort.u32DevId = 0;
                stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn;
                stBindInfo.stSrcChnPort.u32PortId = 0;

                stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
                stBindInfo.stDstChnPort.u32PortId = 0;
                stBindInfo.stDstChnPort.u32DevId = 0;
                stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn + 1;

                stBindInfo.u32SrcFrmrate = 0;
                stBindInfo.u32DstFrmrate = 0;

                STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
            }

            ST_DBG("vif chn(%d) port(0)->vpe chn(%d) bind\n",
                    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn,
                    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn);

            // bind vpe->disp
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER0;
            if ((2 == u32CaseIndex) && (1 == i))
            {
                stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER1;
            }
            stBindInfo.stDstChnPort.u32PortId = 0; //port0 enable

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;

            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
            ST_DBG("vpe chn(%d) port(%d)->disp chn(%d) bind\n",
                    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, DISP_PORT, i);

            // bind vpe->venc
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;

            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn,
                    &u32DevId);

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;

            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
            ST_DBG("vpe chn(%d) port(%d)->venc dev(%d) chn(%d) bind\n",
                    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, MAIN_VENC_PORT,
                    u32DevId, pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn);

            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = SUB_VENC_PORT;
            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 1,
                    &u32DevId);
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 1;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = JPEG_VENC_PORT;
            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 2,
                    &u32DevId);
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 2;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            //STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
        }
    }

    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            memset(&g_stVdecInfo, 0, sizeof(ST_VdecInfo_T));

            g_stVdecInfo.bRunFlag = TRUE;
            g_stVdecInfo.u32VdecChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn;
            snprintf(g_stVdecInfo.szFileName, sizeof(g_stVdecInfo.szFileName) - 1, "%s",
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.szFilePath);
            pthread_create(&g_stVdecInfo.pt, NULL, ST_VdecSendStream, &g_stVdecInfo);
#if SUPPORT_RGN_TEST
            if (0 == i)
            {
                St_OsdInit(E_MI_RGN_MODID_DIVP, 0, 0); //OSD attach to vdec pipeline
                pthread_create(&g_stVencInfo.pt_osd, NULL, ST_UpdateOsdProc, NULL);
            }
#endif
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            memset(&g_stVencInfo, 0, sizeof(ST_VencInfo_T));

            g_stVencInfo.bRunFlag = TRUE;
            g_stVencInfo.vencChn = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn;
            g_stVencInfo.u32Width = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
            g_stVencInfo.u32Height = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
            g_stVencInfo.eType = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType;
            pthread_create(&g_stVencInfo.pt, NULL, ST_VencGetEsBufferProc, &g_stVencInfo);
            pthread_create(&g_stVencInfo.pt_output, NULL, st_GetOutputDataThread, NULL);
#if SUPPORT_RGN_TEST
            if (0 == i)
            {
                St_OsdInit(E_MI_RGN_MODID_VPE, 0, 3); //OSD attach to vif pipeline
                pthread_create(&g_stVencInfo.pt_osd, NULL, ST_UpdateOsdProc, NULL);
            }
#endif
        }
    }

    return 0;
}

int ST_DVRExit(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    ST_Rect_t stDispOutRect;
    MI_U32 u32WndNum = pstCaseDesc[u32CaseIndex].stDesc.u32WndNum;
    MI_U32 i = 0;
    ST_Rect_t stDispWndRect[32] = {0};

    if (g_u32CaseIndex == -1)
    {
        return 0;
    }
    // stop osd thread
    g_updateOsdFlag = 0;
    g_VencFlag = 0;

    // stop thread
    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            g_stVdecInfo.bRunFlag = FALSE;
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            g_stVencInfo.bRunFlag = TRUE;
        }
    }

    // wait thread exit
    sleep(2);

    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 u32DevId = 0;
    // unbind
    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_T));

            // unbind divp->disp
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_T));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER0;
            if ((2 == u32CaseIndex) && (1 == i))
            {
                stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER1;
            }
            stBindInfo.stDstChnPort.u32PortId = 0; //port0 enable

            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            ST_DBG("vdec divp(%d)->disp chn(%d) unbind\n",
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn, i);

            // unbind vdec->divp
            memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_T));

            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;

            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            ST_DBG("vdec chn(%d)->divp chn(%d) unbind\n",
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn,
                pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn);
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            // unbind vpe->mainvenc
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn,
                    &u32DevId);
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            // unbind vpe->subvenc
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = SUB_VENC_PORT;
            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 1,
                    &u32DevId);
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 1;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            // unbind vpe->jpegvenc
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = JPEG_VENC_PORT;
            u32DevId = 0;
            MI_VENC_GetChnDevid(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 2,
                    &u32DevId);
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
            stBindInfo.stDstChnPort.u32DevId = u32DevId;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VencChn + 2;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            //STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            // unbind vpe->disp
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER0;
            if ((2 == u32CaseIndex) && (1 == i))
            {
                stBindInfo.stDstChnPort.u32ChnId = ST_DISP_LAYER1;
            }
            stBindInfo.stDstChnPort.u32PortId = 0; //port0 enable
            stBindInfo.u32SrcFrmrate = 30;
            stBindInfo.u32DstFrmrate = 30;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            // unbind vif->vpe0
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn;
            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

            // unbind vif->vpe1
            memset(&stBindInfo, 0x0, sizeof(stBindInfo));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn;
            stBindInfo.stSrcChnPort.u32PortId = 0;
            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
            stBindInfo.stDstChnPort.u32PortId = 0;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn + 1;
            stBindInfo.u32SrcFrmrate = 0;
            stBindInfo.u32DstFrmrate = 0;
            STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        }
    }

    for (i = 0; i < u32WndNum; i++)
    {
        if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VDEC_CHN)
        {
            // stop vdec
            STCHECKRESULT(MI_VDEC_StopChn(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn));
            STCHECKRESULT(MI_VDEC_DestroyChn(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32VdecChn));

            // stop divp
            STCHECKRESULT(MI_DIVP_StopChn(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn));
            STCHECKRESULT(MI_DIVP_DestroyChn(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVdecChnArg.u32DivpChn));
        }
        else if (pstCaseDesc[u32CaseIndex].stCaseArgs[i].eVideoChnType & E_ST_VIF_CHN)
        {
            // stop vif
            STCHECKRESULT(ST_Vif_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VifChn, 0));

            // stop vpe0
            STCHECKRESULT(ST_Vpe_StopChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn));
            STCHECKRESULT(ST_Vpe_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, DISP_PORT));
            STCHECKRESULT(ST_Vpe_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, MAIN_VENC_PORT));
            STCHECKRESULT(ST_Vpe_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, SUB_VENC_PORT));
            STCHECKRESULT(ST_Vpe_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn, JPEG_VENC_PORT));
            STCHECKRESULT(ST_Vpe_DestroyChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn));

            // stop vpe1
            STCHECKRESULT(ST_Vpe_StopChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn + 1));
            STCHECKRESULT(ST_Vpe_StopPort(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn + 1, MAIN_VENC_PORT));
            STCHECKRESULT(ST_Vpe_DestroyChannel(pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u32VpeChn + 1));

            ST_DestroyVencChannel(0);
            ST_DestroyVencChannel(1);
            ST_DestroyVencChannel(2);
            MI_IQServer_Close();
        }
    }

    // disable vif dev
    ST_Vif_DisableDev(0);
    // disable displayer1
    ExecFunc(MI_DISP_DisableInputPort(ST_DISP_LAYER1, 0), MI_SUCCESS);
    ExecFunc(MI_DISP_DisableVideoLayer(ST_DISP_LAYER1), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(ST_DISP_LAYER1, ST_DISP_DEV0), MI_SUCCESS);

    // disable displayer0 & disable dispdev0
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, ST_DISP_LAYER0, u32WndNum));
#if FB_SHOW_ON_PANEL
    // uninit panel
    ExecFunc(ST_Panel_DeInit(), MI_SUCCESS);
#else
    // uninit HDMI
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
#endif
    MI_RGN_Destroy(0);//Destroy rgn 0

    // uninit fb
    ST_Fb_DeInit();

    return 0;
}

void ST_VDEC_HandleSig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        printf("catch Ctrl + C, exit\n");

        // ST_SubExit();

        sleep(1);

        ST_Sys_Exit();

        exit(0);
    }
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

    printf("print twice Enter or 'q/Q' to exit\n");
}

int main(int argc, char **argv)
{
    struct sigaction sigAction;
    char szCmd[16];
    MI_U32 u32Index = 0, s32Ret = -1;
    int enterCount = 0;

    sigAction.sa_handler = ST_VDEC_HandleSig;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;
	sigaction(SIGINT, &sigAction, NULL);

    /************************************************
    step1:  init sys
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

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

    while (!g_bExit)
    {
        ST_VdecUsage();
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);
        if (0 == (strncmp(szCmd, "q", 1)) ||
            0 == (strncmp(szCmd, "Q", 1)))
        {
            break;
        }
        else if (0 == (strncmp(szCmd, "i", 1)) ||
            0 == (strncmp(szCmd, "I", 1)))
        {
            s32Ret = MI_ISP_API_CmdLoadBinFile(0, "i2307v1.bin", 1234);
            if (MI_SUCCESS != s32Ret)
            {
                printf("xxxxxxxxxxxxxxxxERR(0x%x)xxxxxxxxxxxxxx\n", s32Ret);
            }
            else
            {
                printf("xxxxxxxxxxxxxxxx Load IQfile Success xxxxxxxxxxxxxx\n");
            }
            continue;
        }
        else if (0 == (strncmp(szCmd, "stopvenc", 8)))
        {
            MI_VENC_StopRecvPic(2);
            continue;
        }
        else if (0 == (strncmp(szCmd, "startvenc", 9)))
        {
            MI_VENC_CropCfg_t stCropCfg;

            memset(&stCropCfg, 0, sizeof(MI_VENC_CropCfg_t));
            MI_VENC_GetCrop(2, &stCropCfg);
            stCropCfg.bEnable = 1;
            stCropCfg.stRect.u32Width = 720;
            stCropCfg.stRect.u32Height = 576;

            printf("crop enable(%d) (x(%d)-y(%d)-w(%d)-h(%d))...\n", stCropCfg.bEnable,
                stCropCfg.stRect.u32Left, stCropCfg.stRect.u32Top, stCropCfg.stRect.u32Width, stCropCfg.stRect.u32Height);
            MI_VENC_SetCrop(2, &stCropCfg);
            MI_VENC_StartRecvPic(2);
            continue;
        }
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

        ST_DVRH26X();
    }

    ST_DBG("process exit normal\n");

    ST_DVRExit();
    MI_RGN_DeInit();
    STCHECKRESULT(ST_Sys_Exit());

	return 0;
}
