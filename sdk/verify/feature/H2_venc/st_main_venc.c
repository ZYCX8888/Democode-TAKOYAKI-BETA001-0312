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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "st_main_venc.h"

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
static int g_encode_type = 2;
static int g_frame_rate = 10;

static char *g_file_path = NULL;

static MI_BOOL g_bExit = FALSE;
static MI_U32 g_u32ParamIndex = 0;
#define MAX_CHN_NUM 16

VENC_Thread_Args_t g_stVencThreadArgs[MAX_CHN_NUM];

enum
{
    E_VENC_Test_NULL,
    E_VENC_Test_InsertUserData,
    E_VENC_Test_ROI,
    E_VENC_Test_IDR_Config,
    E_VENC_Test_Slice_Split,
    E_VENC_Test_Intra_Pred,
    E_VENC_Test_Inter_Pred,
    E_VENC_Test_Trans,
    E_VENC_Test_Entropy,
    E_VENC_Test_VUI,
    E_VENC_Test_LTR,
    E_VENC_Test_Crop,
    E_VENC_Test_Super_Frame,
    E_VENC_Test_RateControl_Priority,
    E_VENC_Test_RateControl,
    E_VENC_Test_Deblocking,
    E_VENC_Test_DropFrame_strategy,
    E_VENC_Test_MAX
} ExtendParamType_e;

void ST_Venc_Help(const char *porgName)
{
    printf("%s [-options] source.yuv\n", porgName);
    printf(" -s <width> <height> .... source resolution.\n");
    printf(" -p <yuv format> ........ 0: yuv420, 1: yuv422.\n");
    printf(" -e <encode type> ....... 2:H264 3:H265 4:Jpeg\n");
    printf(" -f <frame rate> ........ .\n");
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
        else if (!strcmp(argv[i], "-e"))
        {
            g_encode_type = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-f"))
        {
            g_frame_rate = atoi(argv[++i]);
        }
        else if (g_file_path == NULL)
        {
            g_file_path = argv[i];
        }
    }
    printf("Param Prase W(%d) H(%d) Etype(%d) FrameRate(%d) Filename(%s)...\n",
        g_yuv_width, g_yuv_height, g_encode_type, g_frame_rate, g_file_path);
}

void ST_VencActionUsage()
{
    //printf("Test Case: xxxxx");
    printf("1)\t Run\n");
    printf("2)\t SetParam\n");
    //printf("3)\t Back\n");
}

void ST_VencExtendUsage()
{
    printf("1)\t Insert User Data\n");
    printf("2)\t ROI Config\n");
    printf("3)\t IDR Enable\n");
    printf("4)\t Slice Split\n");
    printf("5)\t Intra Pred\n");
    printf("6)\t Inter Pred\n");
    printf("7)\t Trans\n");
    printf("8)\t Entropy\n");
    printf("9)\t VUI\n");
    printf("10)\t LTR\n");
    printf("11)\t Crop\n");
    printf("12)\t Super Frame Config(Not supported)\n");
    printf("13)\t Rate Control Priority(Not Supported)\n");
    printf("14)\t Rate Control(need edit code)\n");
    printf("15)\t Deblocking\n");
    printf("16)\t Frame Lost Strategy(Not Supported)\n");
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

void ST_VencSetParam(MI_U32 index)
{
    MI_S32 s32Ret;
    MI_VENC_ChnAttr_t stChnAttr;

    s32Ret = MI_VENC_GetChnAttr(0, &stChnAttr);
    if(index == E_VENC_Test_InsertUserData)
    {
        s32Ret = MI_VENC_InsertUserData(0, _test_insert_data, sizeof(_test_insert_data));
        if(s32Ret)
            printf("%s %d, MI_VENC_InsertUserData error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_InsertUserData Success\n");
    }
    else if(index == E_VENC_Test_ROI)
    {
        s32Ret = MI_VENC_SetRoiCfg(0, &_test_roi_cfg);
        if(s32Ret)
            printf("%s %d, MI_VENC_SetRoiCfg error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetRoiCfg Success\n");
    }
    else if(index == E_VENC_Test_IDR_Config)
    {
        s32Ret = MI_VENC_EnableIdr(0, _test_idr_enable);
        if(s32Ret)
            printf("%s %d, MI_VENC_EnableIdr error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_EnableIdr Success\n");
        g_idr_config = TRUE;
    }
    else if(index == E_VENC_Test_Slice_Split)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264SliceSplit(0, &_test_split_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265SliceSplit(0, &_test_split_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetSliceSplit error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetSliceSplit Success\n");
    }
    else if(index == E_VENC_Test_Intra_Pred)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264IntraPred(0, &_test_intra_pred_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265IntraPred(0, &_test_intra_pred_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetIntraPred error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetIntraPred Success\n");
    }
    else if(index == E_VENC_Test_Intra_Pred)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264IntraPred(0, &_test_intra_pred_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265IntraPred(0, &_test_intra_pred_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetIntraPred error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetIntraPred Success\n");
    }
    else if(index == E_VENC_Test_Inter_Pred)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264InterPred(0, &_test_inter_pred_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265InterPred(0, &_test_inter_pred_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetInterPred error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetInterPred Success\n");
    }
    else if(index == E_VENC_Test_Trans)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264Trans(0, &_test_trans_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265Trans(0, &_test_trans_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetTrans error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetTrans Success\n");
    }
    else if(index == E_VENC_Test_Entropy)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264Entropy(0, &_test_entropy_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            printf("H.265 is not support SetEntropy\n");
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetEntropy error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetEntropy Success\n");
    }
    else if(index == E_VENC_Test_VUI)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264Vui(0, &_test_vui_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265Vui(0, &_test_vui_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetVui error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetVui Success\n");
    }
    else if(index == E_VENC_Test_LTR)
    {
        s32Ret = MI_VENC_SetRefParam(0, &_test_ltr);
        if(s32Ret)
            printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetRefParam Success\n");
    }
    else if(index == E_VENC_Test_Crop)
    {
        s32Ret = MI_VENC_SetCrop(0, &_test_crop);
        if(s32Ret)
            printf("%s %d, MI_VENC_SetCrop error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetCrop Success\n");
    }
    else if(index == E_VENC_Test_Deblocking)
    {
        if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
        {
            s32Ret = MI_VENC_SetH264Dblk(0, &_test_dblk_264);
        }
        else if(stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
        {
            s32Ret = MI_VENC_SetH265Dblk(0, &_test_dblk_265);
        }

        if(s32Ret)
            printf("%s %d, MI_VENC_SetDblk error, %X\n", __func__, __LINE__, s32Ret);
        else
            printf("MI_VENC_SetDblk Success\n");
    }
    else
    {
        printf("Not spport\n");
    }

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
    MI_U32 u32DevId = 0;
    MI_S32 vencFd = -1;
    fd_set read_fds;

    MI_VENC_GetChnDevid(pArgs->stVencAttr[0].vencChn, &u32DevId);

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
    stVencChnInputPort.u32PortId = 0;

    memset(szFileName, 0, sizeof(szFileName));

    len = snprintf(szFileName, sizeof(szFileName) - 1, "Venc_dev%d_chn%d_port%d_%dx%d.",
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
            else if(stBufInfo.stRawData.u32ContentSize >= 800 * 1024)  //MAX_OUTPUT_ES_SIZE in KO
            {
                printf("%s %d unable to read buffer. buffer overflow\n", __func__, __LINE__);
            }

            len = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
            //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
            //    len, stBufInfo.stRawData.u32ContentSize);

            if(g_idr_config && _test_idr_enable == FALSE)
            {
                if(_test_tmp_interval >= _test_idr_interval)
                {
                    _test_tmp_interval = 0;
                    s32Ret = MI_VENC_RequestIdr(0, TRUE);
                    if(s32Ret)
                        printf("%s %d, MI_VENC_RequestIdr error, %X\n", __func__, __LINE__, s32Ret);
                }
                else
                {
                    _test_tmp_interval ++;
                }
            }

            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        else
        {
            usleep(5 * 1000);//sleep 10 ms
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
    return NULL;
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
    MI_U32 u32FrameCount = 0;

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

    printf("open %s success, total size:%d bytes\n", g_file_path, (int)st.st_size);

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
            //printf("seek to the begin of the file, u32FilePos:%d, curpos:%ld\n", u32FilePos, ftell(pFile));
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

            MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0], u32YSize);
            MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[1], u32YSize >> 1);

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

int ST_VencStart(MI_S32 s32PicW, MI_S32 s32PicH, MI_S32 s32EType, MI_S32 s32FrameRate)
{
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32DevId = 0;

    MI_U32 u32VencNum = 1;//Create One Channel to encode ES stream

    MI_VENC_CHN VencChn = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;

    printf("%s %d, total chn num:%d\n", __func__, __LINE__, u32VencNum);

    for (i = 0; i < u32VencNum; i ++)
    {
        VencChn = i;

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

        if (E_MI_VENC_MODTYPE_H264E == s32EType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = s32PicH;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = s32PicH;

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
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = 2 * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = s32FrameRate;
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
        else if (E_MI_VENC_MODTYPE_H265E == s32EType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = s32PicH;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = s32PicH;

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
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = s32FrameRate;
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
        else if (E_MI_VENC_MODTYPE_JPEGE == s32EType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = s32PicH;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = s32PicW;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = s32PicH;
        }

        s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        printf("%s %d MI_VENC_CreateChn, vencChn:%d\n", __func__, __LINE__, VencChn);
        if (E_MI_VENC_MODTYPE_JPEGE == s32EType)
        {
            MI_VENC_ParamJpeg_t stParamJpeg;

            memset(&stParamJpeg, 0, sizeof(stParamJpeg));
            s32Ret = MI_VENC_GetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
            printf("Get Qf:%d\n", stParamJpeg.u32Qfactor);

            stParamJpeg.u32Qfactor = 80;
            s32Ret = MI_VENC_SetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
        }
        s32Ret = MI_VENC_GetChnDevid(VencChn, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        stVencChnOutputPort.u32DevId = u32DevId;
        stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
        stVencChnOutputPort.u32ChnId = VencChn;
        stVencChnOutputPort.u32PortId = 0;
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 1, 4);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

        ST_VencSetParam(g_u32ParamIndex);

        s32Ret = MI_VENC_StartRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

        g_stVencThreadArgs[i].stVencAttr[0].vencChn = VencChn;
        g_stVencThreadArgs[i].stVencAttr[0].u32MainWidth = s32PicW;
        g_stVencThreadArgs[i].stVencAttr[0].u32MainHeight = s32PicH;
        g_stVencThreadArgs[i].stVencAttr[0].eType = s32EType;
        g_stVencThreadArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);

        g_stVencThreadArgs[i].bRunFlag = TRUE;

        // printf("%s %d, venc:%d,fd:%d\n", __func__, __LINE__, VencChn, g_stVencThreadArgs[0].stVencAttr[i].vencFd);
        printf("%s %d, venc:%d\n", __func__, __LINE__, VencChn);

        // get es stream
        pthread_create(&g_stVencThreadArgs[i].ptGetEs, NULL, ST_VencGetEsProc, (void *)&g_stVencThreadArgs[i]);

        // put yuv data to venc
        pthread_create(&g_stVencThreadArgs[i].ptFillYuv, NULL, ST_VencFillYUVProc, (void *)&g_stVencThreadArgs[i]);
    }

    return 0;
}

MI_S32 ST_VencStop()
{
    MI_S32 i = 0, VencChn = 0, s32Ret = 0;
    MI_U32 u32VencNum = 0;
    for (i = 0; i < u32VencNum; i ++)
    {
        VencChn = i;
        s32Ret = MI_VENC_StopRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        s32Ret = MI_VENC_DestroyChn(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_DestroyChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    ST_Venc_ParseOptions(argc, argv);

    printf("source yuv info:  format=%d,width=%d,height=%d,file=%s\n",
        g_yuv_format, g_yuv_width, g_yuv_height, g_file_path);

    struct sigaction sigAction;

    sigAction.sa_handler = ST_VENC_HandleSig;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    ST_VencStart(g_yuv_width, g_yuv_height, g_encode_type, g_frame_rate);
    while (!g_bExit)
    {
        sleep(1);
    }
    g_stVencThreadArgs[0].bRunFlag = FALSE;
    pthread_join(g_stVencThreadArgs[0].ptGetEs, NULL);
    pthread_join(g_stVencThreadArgs[0].ptFillYuv, NULL);
    ST_VencStop();
    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);

    return 0;
}
