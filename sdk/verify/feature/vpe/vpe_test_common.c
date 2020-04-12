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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mi_vpe_test.h"
#ifndef ENABLE_WITH_DISP_TEST
#define ENABLE_WITH_DISP_TEST
#endif

MI_S32 test_vpe_CreatChannel_MaxSize(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstSrcWin,MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin,MI_SYS_PixelFormat_e ePixFmt)
{
    MI_VPE_ChannelAttr_t stChannelVpeAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpeAttr.u16MaxW = pstSrcWin->u16Width;
    stChannelVpeAttr.u16MaxH = pstSrcWin->u16Height;
    stChannelVpeAttr.bNrEn= FALSE;
    stChannelVpeAttr.bEdgeEn= FALSE;
    stChannelVpeAttr.bEsEn= FALSE;
    stChannelVpeAttr.bContrastEn= FALSE;
    stChannelVpeAttr.bUvInvert= FALSE;
    stChannelVpeAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stChannelVpeAttr.eRunningMode = E_MI_VPE_RUN_DVR_MODE;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);

    stChannelVpeAttr.bContrastEn = FALSE;
    stChannelVpeAttr.bNrEn = FALSE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = pstCropWin->u16X;
    stCropWin.u16Y = pstCropWin->u16Y;
    stCropWin.u16Width = pstCropWin->u16Width;
    stCropWin.u16Height = pstCropWin->u16Height;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height= pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);
}

MI_S32 test_vpe_CreatChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin, MI_SYS_PixelFormat_e InePixFmt, MI_SYS_PixelFormat_e OutePixFmt)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = InePixFmt;
    stChannelVpssAttr.eRunningMode = E_MI_VPE_RUN_DVR_MODE;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = FALSE;
    stChannelVpssAttr.bNrEn = FALSE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = pstCropWin->u16X;
    stCropWin.u16Y = pstCropWin->u16Y;
    stCropWin.u16Width = pstCropWin->u16Width;
    stCropWin.u16Height = pstCropWin->u16Height;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = OutePixFmt;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height= pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);
}


MI_S32 test_vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VPE_StopChannel (VpeChannel), MI_VPE_OK);

    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);
}
void test_vpe_PrintfData(MI_SYS_BufInfo_t *pstBufInfo)
{
    int i = 0;
    if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_RAW)
    {
        //MI_VPE_TEST_DBG("%s \n", pstBufInfo->stRawData.pVirAddr);
        MI_VPE_TEST_DBG("u32ContentSize : %d \n",pstBufInfo->stRawData.u32ContentSize);
    }
    else if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_FRAME)
    {
        MI_VPE_TEST_DBG("height :%d  width : %d \n", pstBufInfo->stFrameData.u16Height , pstBufInfo->stFrameData.u16Width);
    }
    else if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_META)
    {
        MI_VPE_TEST_DBG("From Module :%d \n",pstBufInfo->stMetaData.eDataFromModule);
    }
    else
        MI_VPE_TEST_DBG("error \n");
}

void test_vpe_FdRewind(int srcFd)
{
    lseek(srcFd, 0, SEEK_SET);
}
#if 0
MI_S32 test_vpe_GetOneFrame(int srcFd, int offset, char *pData, int yuvSize)
{
    int size = 0;
    char buff[256];
    // return file start

    // seek to file offset
    //lseek(srcFd, offset, SEEK_SET);
    do {
        memset(buff, 0, sizeof(buff));
        if (yuvSize < sizeof(buff))
        {
            size = yuvSize;
        }
        else
        {
            size = sizeof(buff);
        }

        size = read(srcFd, buff, size);
        if (size == 0)
        {
            break;
        }
        else if (size < 0)
        {
            break;
        }
        memcpy(pData, buff, size);
        pData += size;
        yuvSize = yuvSize - size;
    } while (yuvSize > 0);

    return 0;
}
#else
MI_S32 test_vpe_GetOneFrame(int srcFd, int offset, char *pData, int yuvSize)
{
    int size = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < yuvSize)
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);
    if (read(srcFd, pData, yuvSize) < yuvSize)
    {
        return 0;
    }

    return 1;
}

MI_S32 test_vpe_GetOneFrameYUV422ByStride(int srcFd, char *pYUVData, int height, int width, int Stride)
{
    int size = 0;
    int i = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < width*height*2)
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);
#if 0
    if (read(srcFd, pYData, ySize) < ySize)
    {
        return 0;
    }
    else if (read(srcFd, pUvData, uvSize) < uvSize)
    {
        return 0;
    }
#endif

    for (i = 0; i < height; i++)
    {
        if (read(srcFd, pYUVData+ i * Stride, width*2) < width*2)
        {
            return 0;
        }
    }

    return 1;
}

MI_S32 test_vpe_GetOneFrameYUV420(int srcFd, char *pYData, char *pUvData, int ySize, int uvSize)
{
    int size = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < (ySize + uvSize))
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);
    if (read(srcFd, pYData, ySize) < ySize)
    {
        return 0;
    }
    else if (read(srcFd, pUvData, uvSize) < uvSize)
    {
        return 0;
    }


    return 1;
}

MI_S32 test_vpe_GetOneFrameYUV420ByStride(int srcFd, char *pYData, char *pUvData, int ySize, int uvSize, int height, int width, int yStride, int uvStride)
{
    int size = 0;
    int i = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < (ySize + uvSize))
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);
#if 0
    if (read(srcFd, pYData, ySize) < ySize)
    {
        return 0;
    }
    else if (read(srcFd, pUvData, uvSize) < uvSize)
    {
        return 0;
    }
#endif

    for (i = 0; i < height; i++)
    {
        if (read(srcFd, pYData+ i * yStride, width) < width)
        {
            return 0;
        }
    }

    for (i = 0; i < height/2; i++)
    {
        if (read(srcFd, pUvData+ i * uvStride, width) < width)
        {
            return 0;
        }
    }

    return 1;
}

#endif
MI_S32 test_vpe_PutOneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;
    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for (i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset*i;
        yuvSize = line_size;
        do {
            if (yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }

            size = write(dstFd, pData, size);
            if (size == 0)
            {
                break;
            }
            else if (size < 0)
            {
                break;
            }
            pData += size;
            yuvSize -= size;
        } while (yuvSize > 0);
    }

    return 0;
}

void test_vpe_ShowFrameInfo (const char *s, MI_SYS_FrameData_t *pstFrameInfo)
{
    MI_VPE_TEST_INFO("%s %u x %u  stride: %u Phy: 0x%llx VirtualAddr: %p.\n", s, pstFrameInfo->u16Width, pstFrameInfo->u16Height,
        pstFrameInfo->u32Stride[0], pstFrameInfo->phyAddr[0], pstFrameInfo->pVirAddr[0]);
}

MI_BOOL test_vpe_OpenSourceFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    if (src_fd < 0)
    {
        printf("src_file: %s.\n", pFileName);

        perror("open");
        return -1;
    }
    *pSrcFd = src_fd;

    return TRUE;
}

MI_BOOL test_vpe_OpenDestFile(const char *pFileName, int *pDestFd)
{
    int dest_fd = open(pFileName, O_WRONLY|O_CREAT, 0777);
    if (dest_fd < 0)
    {
        printf("dest_file: %s.\n", pFileName);
        perror("open");
        return -1;
    }

    *pDestFd = dest_fd;

    return TRUE;
}
MI_BOOL test_vpe_OpenDestFileNoCreate(const char *pFileName, int *pDestFd)
{
    int dest_fd = open(pFileName, O_WRONLY);
    if (dest_fd < 0)
    {
        printf("dest_file: %s.\n", pFileName);
        perror("open");
        return -1;
    }

    *pDestFd = dest_fd;

    return TRUE;
}

void test_vpe_CloseFd(int fd)
{
    close(fd);
}

MI_BOOL test_vpe_SysEnvInit(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    // init MI_SYS
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    // set sys PTS
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    ExecFunc(MI_SYS_GetVersion(&stVersion), MI_SUCCESS);
    MI_VPE_TEST_DBG("(%d) u8Version:0x%llx\n", __LINE__, stVersion.u8Version);

    ExecFunc(MI_SYS_GetCurPts(&u64Pts), MI_SUCCESS);
    MI_VPE_TEST_DBG("(%d) u64Pts:0x%llx\n", __LINE__, u64Pts);

    u64Pts = 0xF1237890F1237890;
    ExecFunc(MI_SYS_InitPtsBase(u64Pts), MI_SUCCESS);

    u64Pts = 0xE1237890E1237890;
    ExecFunc(MI_SYS_SyncPts(u64Pts), MI_SUCCESS);
    return TRUE;
}

MI_BOOL test_vpe_SysEnvDeinit(void)
{
    //ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    return TRUE;
}

static void test_vpe_DrawHLineYuv422(MI_U16 *pData, int size, MI_U16 u16Color)
{
    int i = 0;
    MI_U16 *p = NULL;
    for (i = 0; i < size; i++)
    {
        p  = pData + i;
        *p = u16Color;
    }
}

static void test_vpe_DrawVLineYuv422(MI_U16 *pData, int size, int line_offset, MI_U16 u16Color)
{
    int i = 0;
    MI_U16 *p = NULL;
    for (i = 0; i < size; i++)
    {
        p  = pData + i*line_offset;
        *p = u16Color;
    }
}

static void test_vpe_DrawHLineYuv420(MI_U8 *pData, int size, MI_U16 u16Color)
{
    int i = 0;
    MI_U16 *p = NULL;
    for (i = 0; i < size; i++)
    {
        p  = pData + i;
        *p = u16Color;
    }
}

static void test_vpe_DrawVLineYuv420(MI_U8 *pData, int size, int line_offset, MI_U16 u16Color)
{
    int i = 0;
    MI_U16 *p = NULL;
    for (i = 0; i < size; i++)
    {
        p  = pData + i*line_offset;
        *p = u16Color;
    }
}

//-------------------------------------------------
// Rect Defination:
//       x1y1                  x2y1
//       *---------------------*
//       |                     |
//       |                     |
//       *---------------------*
//       x1y2                  x2y1
//--------------------------------------------------
void test_vpe_WriteBonderYuv422(MI_U16 u16Color, int font, MI_SYS_WindowRect_t *pstRect, MI_U16 *pData,  int line_offset)
{
    MI_U16 x1, x2, y1, y2;
    int i = 0;
    x1 = pstRect->u16X;
    x2 = x1 + pstRect->u16Width-1;
    y1 = pstRect->u16Y;
    y2 = y1 + pstRect->u16Height-1;
    // Draw H Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawHLineYuv422(pData + line_offset*(y1 + i) + x1, pstRect->u16Width, u16Color);
        test_vpe_DrawHLineYuv422(pData + line_offset*(y2 - i) + x1, pstRect->u16Width, u16Color);
    }
    // Draw V Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawVLineYuv422(pData + line_offset*y1 + x1 + i, pstRect->u16Height, line_offset, u16Color);
        test_vpe_DrawVLineYuv422(pData + line_offset*y1 + x2 - i, pstRect->u16Height, line_offset, u16Color);
    }
}

void test_vpe_WriteBonderYuv420Luma(MI_U16 u16Color, int font, MI_SYS_WindowRect_t *pstRect, MI_U8 *pData,  int line_offset)
{
    MI_U16 x1, x2, y1, y2;
    int i = 0;
    x1 = pstRect->u16X;
    x2 = x1 + pstRect->u16Width-1;
    y1 = pstRect->u16Y;
    y2 = y1 + pstRect->u16Height-1;
    // Draw H Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawHLineYuv420(pData + line_offset*(y1 + i) + x1, pstRect->u16Width, u16Color);
        test_vpe_DrawHLineYuv420(pData + line_offset*(y2 - i) + x1, pstRect->u16Width, u16Color);
    }
    // Draw V Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawVLineYuv420(pData + line_offset*y1 + x1 + i, pstRect->u16Height, line_offset, u16Color);
        test_vpe_DrawVLineYuv420(pData + line_offset*y1 + x2 - i, pstRect->u16Height, line_offset, u16Color);
    }
}

void test_vpe_WriteBonderYuv420Chrom(MI_U16 u16Color, int font, MI_SYS_WindowRect_t *pstRect, MI_U8 *pData,  int line_offset)
{
    MI_U16 x1, x2, y1, y2;
    int i = 0;
    x1 = pstRect->u16X;
    x2 = x1 + pstRect->u16Width-1;
    y1 = pstRect->u16Y;
    y2 = y1 + pstRect->u16Height-1;
    // Draw H Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawHLineYuv420(pData + line_offset*(y1 + i) + x1, pstRect->u16Width, u16Color);
        test_vpe_DrawHLineYuv420(pData + line_offset*(y2 - i) + x1, pstRect->u16Width, u16Color);
    }
    // Draw V Line
    for (i = 0; i < font; i++)
    {
        test_vpe_DrawVLineYuv420(pData + line_offset*y1 + x1 + i, pstRect->u16Height/2, line_offset, u16Color);
        test_vpe_DrawVLineYuv420(pData + line_offset*y1 + x2 - i, pstRect->u16Height/2, line_offset, u16Color);
    }
}

#ifdef ENABLE_WITH_DISP_TEST
MI_S32 test_vpe_HdmiInit(void)
{
    MI_HDMI_InitParam_t stInitParam;
    MI_HDMI_Attr_t stAttr;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;

    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = NULL;

    MI_HDMI_Init(&stInitParam);

    ExecFunc(MI_HDMI_Open(eHdmi), MI_SUCCESS);
    memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stAttr.stAudioAttr.bEnableAudio = TRUE;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = TRUE;
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_RGB444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = E_MI_HDMI_TIMING_1080_60P;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    MI_HDMI_SetAttr(eHdmi, &stAttr);

    MI_HDMI_Start(eHdmi), MI_SUCCESS;
    return MI_SUCCESS;
}

MI_S32 test_vpe_InitDisp(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_DISP_INPUTPORT LayerInputPort, MI_SYS_WindowRect_t *pstCanvas, MI_SYS_WindowRect_t *pstDispWin, MI_SYS_PixelFormat_e ePixFmt)
{
    // Test Device
    //MI_DISP_DEV DispDev = 0;

    MI_DISP_PubAttr_t stPubAttr;

    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Enable(DispDev), MI_DISP_SUCCESS);
    test_vpe_HdmiInit();

    //MI_DISP_LAYER DispLayer = 0;

    MI_U32 u32Toleration = 500;
    MI_DISP_CompressAttr_t stCompressAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr, stLayerAttrBackup;
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));

    stLayerAttr.stVidLayerSize.u16Width = pstCanvas->u16Width;
    stLayerAttr.stVidLayerSize.u16Height= pstCanvas->u16Height;
    stLayerAttr.stVidLayerDispWin.u16X = pstDispWin->u16X;
    stLayerAttr.stVidLayerDispWin.u16Y = pstDispWin->u16Y;
    stLayerAttr.stVidLayerDispWin.u16Width  = pstDispWin->u16Width;
    stLayerAttr.stVidLayerDispWin.u16Height = pstDispWin->u16Height;
    stLayerAttr.ePixFormat = ePixFmt;
    ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    printf("[%s %d]Get Video Layer Size [%d, %d] !!!\n", __FUNCTION__, __LINE__, stLayerAttr.stVidLayerSize.u16Width, stLayerAttr.stVidLayerSize.u16Height);
    printf("[%s %d]Get Video Layer DispWin [%d, %d, %d, %d] !!!\n", __FUNCTION__, __LINE__,
        stLayerAttr.stVidLayerDispWin.u16X, stLayerAttr.stVidLayerDispWin.u16Y, stLayerAttr.stVidLayerDispWin.u16Width, stLayerAttr.stVidLayerDispWin.u16Height);

    ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_DISP_SUCCESS);

    //MI_DISP_INPUTPORT LayerInputPort = 0;

    MI_DISP_InputPortAttr_t stInputPortAttr;
    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width    = pstCanvas->u16Width;
    stInputPortAttr.stDispWin.u16Height = pstCanvas->u16Height;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, LayerInputPort), MI_SUCCESS);

    return 0;
}

MI_S32 test_vpe_InitDispWithoutHDMI(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_DISP_INPUTPORT LayerInputPort, MI_SYS_WindowRect_t *pstInputSize, MI_SYS_WindowRect_t *pstCanvas, MI_SYS_WindowRect_t *pstDispWin)
{
    // Test Device
    //MI_DISP_DEV DispDev = 0;

    MI_DISP_PubAttr_t stPubAttr;

    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_480P60;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    ExecFunc(MI_DISP_SetPubAttr(DispDev,  &stPubAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Enable(DispDev), MI_DISP_SUCCESS);
    //test_vpe_HdmiInit();

    //MI_DISP_LAYER DispLayer = 0;

    MI_U32 u32Toleration = 500;
    MI_DISP_CompressAttr_t stCompressAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr, stLayerAttrBackup;
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));

    stLayerAttr.stVidLayerSize.u16Width = pstCanvas->u16Width;
    stLayerAttr.stVidLayerSize.u16Height= pstCanvas->u16Height;
    stLayerAttr.stVidLayerDispWin.u16X = pstDispWin->u16X;
    stLayerAttr.stVidLayerDispWin.u16Y = pstDispWin->u16Y;
    stLayerAttr.stVidLayerDispWin.u16Width  = pstDispWin->u16Width;
    stLayerAttr.stVidLayerDispWin.u16Height = pstDispWin->u16Height;
    ExecFunc(MI_DISP_BindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetVideoLayerAttr(DispLayer, &stLayerAttr), MI_DISP_SUCCESS);
    printf("[%s %d]Get Video Layer Size [%d, %d] !!!\n", __FUNCTION__, __LINE__, stLayerAttr.stVidLayerSize.u16Width, stLayerAttr.stVidLayerSize.u16Height);
    printf("[%s %d]Get Video Layer DispWin [%d, %d, %d, %d] !!!\n", __FUNCTION__, __LINE__,
        stLayerAttr.stVidLayerDispWin.u16X, stLayerAttr.stVidLayerDispWin.u16Y, stLayerAttr.stVidLayerDispWin.u16Width, stLayerAttr.stVidLayerDispWin.u16Height);
    ExecFunc(MI_DISP_EnableVideoLayer(DispLayer), MI_DISP_SUCCESS);

    //MI_DISP_INPUTPORT LayerInputPort = 0;

    MI_DISP_InputPortAttr_t stInputPortAttr;
    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    stInputPortAttr.stDispWin.u16Width    = pstInputSize->u16Width;
    stInputPortAttr.stDispWin.u16Height = pstInputSize->u16Height;
    ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, LayerInputPort, &stInputPortAttr), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_EnableInputPort(DispLayer, LayerInputPort), MI_SUCCESS);

    return 0;
}

MI_S32 test_vpe_DeinitDisp(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_DISP_INPUTPORT LayerInputPort)
{
    ExecFunc(MI_DISP_DisableInputPort(DispLayer, LayerInputPort), MI_SUCCESS);
    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_DISP_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_DISP_SUCCESS);
}

MI_S32 test_vpeBinderDisp(MI_U32 VpeOutputPort, MI_U32 DispInputPort)
{
    //Bind VPE to DISP
     MI_SYS_ChnPort_t stSrcChnPort;
     MI_SYS_ChnPort_t stDstChnPort;
     MI_U32 u32SrcFrmrate;
     MI_U32 u32DstFrmrate;

     stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
     stSrcChnPort.u32DevId = 0;
     stSrcChnPort.u32ChnId = 0;
     stSrcChnPort.u32PortId = VpeOutputPort;

     stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
     stDstChnPort.u32DevId = 0;
     stDstChnPort.u32ChnId = 0;
     stDstChnPort.u32PortId = DispInputPort;

     u32SrcFrmrate = 30;
     u32DstFrmrate = 30;

     ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
     return 0;
}

MI_S32 test_vpeUnBinderDisp(MI_U32 VpeOutputPort, MI_U32 DispInputPort)
{
    //Bind VPE to DISP
     MI_SYS_ChnPort_t stSrcChnPort;
     MI_SYS_ChnPort_t stDstChnPort;
     MI_U32 u32SrcFrmrate;
     MI_U32 u32DstFrmrate;

     stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
     stSrcChnPort.u32DevId = 0;
     stSrcChnPort.u32ChnId = 0;
     stSrcChnPort.u32PortId = VpeOutputPort;

     stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
     stDstChnPort.u32DevId = 0;
     stDstChnPort.u32ChnId = 0;
     stDstChnPort.u32PortId = DispInputPort;

     ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
     return 0;
}
#endif
