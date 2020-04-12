#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "st_venc.h"
#include "st_common.h"
#include "st_socket.h"

#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "app_config.h"

static MI_BOOL g_bVencRun = FALSE;
static pthread_t tid_Venc = 0;
#define VENC_SAVE_FILE 0

MI_S32 ST_VencCreateChannel(MI_S32 s32VencChn, MI_S32 s32VencType, MI_U16 u16Width, MI_U16 u16Height, MI_S32 s32FrameRate)
{
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;
    MI_U32 s32Ret;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

    if (E_ST_H264 == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = ALIGN_UP(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_UP(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = ALIGN_UP(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_UP(u16Height, VENC_H264_ALIGN_H);
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
    else if (E_ST_H265 == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
        stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = ALIGN_UP(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_UP(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = ALIGN_UP(u16Width, VENC_H264_ALIGN_W);
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_UP(u16Height, VENC_H264_ALIGN_H);
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = s32FrameRate;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 25;
        stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 25;
    }
    else if (E_ST_JPEG == s32VencType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = ALIGN_UP(u16Width, VENC_JPEG_ALIGN_W);
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_UP(u16Height, VENC_JPEG_ALIGN_H);
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = ALIGN_UP(u16Width, VENC_JPEG_ALIGN_W);
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_UP(u16Height, VENC_JPEG_ALIGN_H);
    }
    s32Ret = MI_VENC_CreateChn(s32VencChn, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    else
    {
        printf("%s %d, MI_VENC_CreateChn %d success, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    if (E_ST_JPEG == s32VencType)
    {
        MI_VENC_ParamJpeg_t stParamJpeg;

        memset(&stParamJpeg, 0, sizeof(stParamJpeg));
        s32Ret = MI_VENC_GetJpegParam(s32VencChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
        printf("Get Qf:%d\n", stParamJpeg.u32Qfactor);

        stParamJpeg.u32Qfactor = 50;
        s32Ret = MI_VENC_SetJpegParam(s32VencChn, &stParamJpeg);
        if(s32Ret != MI_SUCCESS)
        {
            return s32Ret;
        }
    }

    s32Ret = MI_VENC_SetMaxStreamCnt(s32VencChn, OUT_DEPTH_VENC);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    else
    {
        printf("%s %d, MI_VENC_CreateChn %d success, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }

    s32Ret = MI_VENC_StartRecvPic(s32VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    else
    {
        printf("%s %d, MI_VENC_StartRecvPic %d success, %X\n", __func__, __LINE__, s32VencChn, s32Ret);
    }
    
    return s32Ret;
}

MI_S32 ST_VencDestroyChannel(MI_S32 s32VencChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    printf("ST_VencDestroyChannel...........\n");
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

static void *ST_VencGetEsProcess(void *args)
{
    VencRunParam_T *pstVencRunParam = (VencRunParam_T *)args;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 s32Len = 0;
    MI_S32 s32VencChn = 0, s32SendSocket = -1, s32SaveFile = FALSE;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack0;
    MI_VENC_ChnStat_t stStat;
    MI_S32 totalLen = 0;
    MI_S32 s32FdStream = 0;
    fd_set fdsread;
    struct timeval tv;
    MI_S32 fd = 0;
    MI_S32 maxfd = 0;
    unsigned long SerIPaddr = pstVencRunParam->IPaddr;
    s32VencChn = pstVencRunParam->s32VencChn;
    s32SendSocket = pstVencRunParam->s32Socket;
    s32SaveFile = pstVencRunParam->s32SaveFileFlag;

    ST_DBG("Get video stream vencChn(%d) socket(%d)..flag(%d). SerIPaddr(0x%x)\n", s32VencChn, s32SendSocket, s32SaveFile, SerIPaddr);
    //sleep(1);
    s32FdStream = MI_VENC_GetFd(s32VencChn);
    if(s32FdStream <= 0)
    {
        printf("Unable to get FD:%d for ch:%2d\n", s32FdStream, s32VencChn);
        return NULL;
    }
    else
    {
        printf("MI_VENC_GetFd CH%2d FD%d\n", s32VencChn, s32FdStream);
    }
    if (s32SaveFile)
    {
        fd = open("/var/chn_1.es", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd <= 0)
        {
            printf("%s %d create error\n", __func__, __LINE__);
            return NULL;
        }
    }

    printf("%s %d create success\n", __func__, __LINE__);

    while (g_bVencRun)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fdsread);
        FD_SET(s32FdStream, &fdsread);
        maxfd = s32FdStream;

        s32Ret = select(maxfd + 1, &fdsread, NULL, NULL, &tv);
        if (s32Ret < 0)
        {
            perror("select venc");
        }
        else if (s32Ret == 0)
        {
            printf("select venc timeout\n");
        }
        else
        {
            if (FD_ISSET(s32FdStream,&fdsread))
            {
                memset(&stStream, 0, sizeof(stStream));
                memset(&stPack0, 0, sizeof(stPack0));
                stStream.pstPack = &stPack0;
                stStream.u32PackCount = 1;
                s32Ret = MI_VENC_Query(s32VencChn, &stStat);
                if (s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
                {
                    continue;
                }
                s32Ret = MI_VENC_GetStream(s32VencChn, &stStream, 10);
                if (MI_SUCCESS == s32Ret)
                {
                    if (totalLen >= 100 * 1024 * 1024)
                    {
                        totalLen = 0;
                        lseek(fd, 0, SEEK_SET);
                    }
                    if (s32SaveFile)
                    {
                        s32Len = write(fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
                        if (s32Len != stStream.pstPack[0].u32Len)
                        {
                            printf("write es buffer fail.\n");
                        }
                        totalLen += s32Len;
                    }
                    ST_Socket_PackNalu_UdpSend(s32SendSocket, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len,
                        SerIPaddr, RECV_VIDEO_PORT);
                    if (MI_SUCCESS != (s32Ret = MI_VENC_ReleaseStream(s32VencChn, &stStream)))
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
                }
            }
        }
    }
    if (s32SaveFile)
    {
        close(fd);
        fd = -1;
    }
    if (s32SendSocket)
    {
        close(s32SendSocket);
        ST_DBG("close send video socket(%d)..\n", s32SendSocket);
    }
    ST_DBG("ST_VencGetEsProcess...exit\n");

    return NULL;
}

MI_S32 ST_VencStartGetStream(VencRunParam_T *pstVencRunParam)
{
    g_bVencRun = TRUE;

    pthread_create(&tid_Venc, NULL, ST_VencGetEsProcess, (void *)pstVencRunParam);
    usleep(400*1000);
    return MI_SUCCESS;
}

MI_S32 ST_VencStopGetStream()
{
    g_bVencRun = FALSE;
    if (tid_Venc)
    {        
        if (0 == pthread_join(tid_Venc, NULL))
        {
            ST_DBG("tid_Venc pthread_join...OK...\n");
            tid_Venc = -1;
        }
    }
    ST_DBG("ST_VencStopGetStream...exit\n");

    return MI_SUCCESS;
}

