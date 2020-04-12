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
#include <string.h>
#include "mi_sys.h"
#include "../common/st_common.h"
#include "../common/st_divp.h"

typedef struct Test_Parames_s
{
    MI_U16 u16InWidth;
    MI_U16 u16InHeight;
    MI_SYS_PixelFormat_e eInPixelFormat;
    const char *pSrcFile;
    int Srcsd;
    MI_U32 u32SrcOffset;

    MI_U16 u16OutWidth;
    MI_U16 u16OutHeight;
    MI_SYS_PixelFormat_e eOutPixelFormat;
    const char *pDestFile;
    int DestSd;
    MI_U32 u32DestOffset;

    int cnt;
}Test_Parames_t;

int checkParame(Test_Parames_t     *pstParamer)
{
    int cnt =pstParamer->cnt;
    const char *pDestFile = pstParamer->pDestFile;
    const char *pSrctFile = pstParamer->pSrcFile;
    MI_U16 u16Width=pstParamer->u16InWidth, u16Height=pstParamer->u16InHeight;
    MI_SYS_PixelFormat_e ePixelFormat = pstParamer->eInPixelFormat;

    if(u16Width < 0 || u16Width > 1920 || u16Height < 0 || u16Height > 1080)
    {
        printf("width %d, height %d err \n", u16Width, u16Height);
        return 1;
    }

    if(ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV
        && ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420
        && ePixelFormat !=E_MI_SYS_PIXEL_FRAME_ARGB8888
        && ePixelFormat !=E_MI_SYS_PIXEL_FRAME_ABGR8888
        && ePixelFormat !=E_MI_SYS_PIXEL_FRAME_RGB565)
    {
        printf("pixel %d err \n", ePixelFormat);
        return 1;
    }

    if(pDestFile == NULL || cnt ==0 || pSrctFile == NULL)
    {
        printf("srcfile %p, destfile is %p, cnt %d \n",pSrctFile, pDestFile, cnt);
        return 1;
    }

    return MI_SUCCESS;
}

static MI_S32 ST_ReadFileYUV420(Test_Parames_t     *pstParamer, MI_SYS_BufInfo_t *pstBufInfo, MI_SYS_BUF_HANDLE hHandle)
{
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U16  u16BufInfoWidth =0;
    MI_U32 y_size =0, uv_size=0;

    u16BufInfoStride   = pstBufInfo->stFrameData.u32Stride[0];
    u16BufInfoHeight  = pstBufInfo->stFrameData.u16Height;
    u16BufInfoWidth = pstBufInfo->stFrameData.u16Width;
    y_size  = u16BufInfoWidth*u16BufInfoHeight;
    u16BufInfoStride   = pstBufInfo->stFrameData.u32Stride[1];
    uv_size  = u16BufInfoWidth*u16BufInfoHeight/2;

    printf("buffer stride[0] %d, stride[1] %d, buffer height %d\n",pstBufInfo->stFrameData.u32Stride[0], pstBufInfo->stFrameData.u32Stride[1],  u16BufInfoHeight);
    if (1 == ST_ReadOneFrameYUV420ByStride(pstParamer->Srcsd, pstBufInfo->stFrameData.pVirAddr[0], pstBufInfo->stFrameData.pVirAddr[1], y_size, uv_size,u16BufInfoHeight, u16BufInfoWidth, u16BufInfoStride, u16BufInfoStride))
    {
        STCHECKRESULT(MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[0], y_size));
        STCHECKRESULT(MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[1], uv_size));
        pstParamer->u32SrcOffset += y_size + uv_size;
        STCHECKRESULT(MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, FALSE));
        printf("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
    }
    else
    {
        STCHECKRESULT(MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, TRUE));
        pstParamer->u32SrcOffset = 0;
        STCHECKRESULT(ST_FdRewind(pstParamer->Srcsd));
    }

    return MI_SUCCESS;
}

static MI_S32 ST_ReadOnePackFrameByStride(Test_Parames_t     *pstParamer, MI_SYS_BufInfo_t *pstBufInfo, MI_SYS_BUF_HANDLE hHandle, MI_U8 u8WidthAm)
{
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U16  u16BufInfoWidth =0;
    MI_U32  Data_size =0;

    u16BufInfoStride   = pstBufInfo->stFrameData.u32Stride[0];
    u16BufInfoHeight  = pstBufInfo->stFrameData.u16Height;
    u16BufInfoWidth = pstBufInfo->stFrameData.u16Width;
    Data_size  = u16BufInfoStride*u16BufInfoHeight;

    if (1 == ST_ReadOneFramePackByStride(pstParamer->Srcsd, pstBufInfo->stFrameData.pVirAddr[0], u16BufInfoHeight, u16BufInfoWidth, u16BufInfoStride,u8WidthAm))
    {
        MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[0], Data_size);
        pstParamer->u32SrcOffset += Data_size;
        MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, FALSE);
        //printf("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
    }
    else
    {
        MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, TRUE);
        pstParamer->u32SrcOffset = 0;
        ST_FdRewind(pstParamer->Srcsd);
    }

    return MI_SUCCESS;
}

static MI_S32 St_WriteFile(Test_Parames_t     *pstParamer)
{
    MI_SYS_ChnPort_t stDivpChnOutputPort;
    MI_SYS_ChnPort_t stDivpChninputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    MI_DIVP_ChnAttr_t stDivpAttr;

    memset(&stDivpAttr, 0x0, sizeof(stDivpAttr));

    memset(&stBufConf , 0, sizeof(stBufConf));

    stDivpChnOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnOutputPort.u32DevId = 0;
    stDivpChnOutputPort.u32ChnId = 0;
    stDivpChnOutputPort.u32PortId = 0;

    stDivpChninputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChninputPort.u32DevId = 0;
    stDivpChninputPort.u32ChnId = 0;
    stDivpChninputPort.u32PortId = 0;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u64TargetPts = 0x1234;
    stBufConf.stFrameCfg.eFormat = pstParamer->eInPixelFormat;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = pstParamer->u16InWidth;
    stBufConf.stFrameCfg.u16Height = pstParamer->u16InHeight;

    printf("bufcfg width %d, height %d, pixel %d\n", stBufConf.stFrameCfg.u16Width, stBufConf.stFrameCfg.u16Height, stBufConf.stFrameCfg.eFormat);
    do{

        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stDivpChninputPort,&stBufConf,&stBufInfo,&hHandle,0))
        {
            /*printf("get input buffer info pixel %d, size(%dx%d), vir(%p, %p, %p)", stBufInfo.stFrameData.ePixelFormat,
                stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
                */
            if(pstParamer->eInPixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                //printf("read yuv420file \n");
                ST_ReadFileYUV420(pstParamer, &stBufInfo, hHandle);
            }
            else if(pstParamer->eInPixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV || pstParamer->eInPixelFormat == E_MI_SYS_PIXEL_FRAME_RGB565)
            {
                //printf("read yuv422file \n");
                ST_ReadOnePackFrameByStride(pstParamer, &stBufInfo, hHandle, 2);
            }
            else
            {
                //printf("read pixel File \n", pstParamer->eInPixelFormat);
                ST_ReadOnePackFrameByStride(pstParamer, &stBufInfo, hHandle, 4);
            }
        }

        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChnOutputPort , &stBufInfo,&hHandle))
        {
            // Add user write buffer to file
            MI_U16  u16BufInfoStride =0;
            MI_U16  u16BufInfoHeight =0;
            MI_U32  u32FrameSize =0;
            int     offset =0;
            MI_U32 y_size =0, uv_size=0;
            u16BufInfoStride  = stBufInfo.stFrameData.u32Stride[0];
            u16BufInfoHeight = stBufInfo.stFrameData.u16Height;
            u32FrameSize = u16BufInfoStride*u16BufInfoHeight;
            // put frame
            printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
            stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
            stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
            printf("begin write file\n");
            STCHECKRESULT(ST_Write_OneFrame(pstParamer->DestSd, offset, stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight));
            offset += u32FrameSize;

            if(pstParamer->eOutPixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                STCHECKRESULT(ST_Write_OneFrame(pstParamer->DestSd, offset, stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2));
                offset += u32FrameSize/2;
            }

            STCHECKRESULT(MI_SYS_ChnOutputPortPutBuf(hHandle));

            STCHECKRESULT(MI_DIVP_GetChnAttr(0, &stDivpAttr));
            printf("getcrop(%d,%d,%d,%d)\n", stDivpAttr.stCropRect.u16X,stDivpAttr.stCropRect.u16Y, stDivpAttr.stCropRect.u16Width, stDivpAttr.stCropRect.u16Height);
            if(stDivpAttr.stCropRect.u16X + stDivpAttr.stCropRect.u16Width < 1920)
                stDivpAttr.stCropRect.u16Width += 2;

            if(stDivpAttr.stCropRect.u16Y + stDivpAttr.stCropRect.u16Height < 1080)
                stDivpAttr.stCropRect.u16Height += 2;

            if(stDivpAttr.stCropRect.u16Width == 1920 && stDivpAttr.stCropRect.u16Height == 1080)
                break;
            printf("setcrop (%d,%d,%d,%d)\n", stDivpAttr.stCropRect.u16X,stDivpAttr.stCropRect.u16Y, stDivpAttr.stCropRect.u16Width, stDivpAttr.stCropRect.u16Height);
            STCHECKRESULT(MI_DIVP_SetChnAttr(0, &stDivpAttr));

            printf("put buf %d done\n", pstParamer->cnt);
            pstParamer->cnt--;
        }
    }while(1);

    return MI_SUCCESS;
}

static MI_S32 St_BaseModuleInit(Test_Parames_t     *pstParamer)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_WindowRect_t stCropRect;
    MI_SYS_WindowRect_t stOutputWin;

    memset(&stOutputWin, 0, sizeof(stOutputWin));
    memset(&stCropRect, 0, sizeof(stCropRect));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    STCHECKRESULT(ST_Sys_Init());

    stCropRect.u16Width = 128;
    stCropRect.u16Height = 128;
    STCHECKRESULT(ST_Divp_CreatChannel(0, E_MI_SYS_ROTATE_NONE, &stCropRect));


    stOutputWin.u16Width = pstParamer->u16OutWidth;
    stOutputWin.u16Height = pstParamer->u16OutHeight;
    STCHECKRESULT(ST_Divp_SetOutputAttr(0, pstParamer->eOutPixelFormat, &stOutputWin));
    STCHECKRESULT(MI_SYS_SetChnMMAConf(E_MI_MODULE_ID_DIVP, 0, 0, "mma_heap_name1"));
    STCHECKRESULT(ST_Divp_StartChn(0));

    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5);

    return MI_SUCCESS;
}

static MI_S32 St_BaseModuleDeinit(void)
{
    STCHECKRESULT(ST_Divp_StopChn(0));
    STCHECKRESULT(ST_Divp_DestroyChn(0));
    STCHECKRESULT(ST_Sys_Exit());
    return MI_SUCCESS;
}

int main(int argc, char **argv)
{
    Test_Parames_t  stParamer;
    memset(&stParamer, 0x0, sizeof(Test_Parames_t));

    if(argc < 10)
    {
       printf("paramer cnt %d < 7 \n", argc);
       printf("Inwidth[1], Inheight[2], Inpixel[3],Srcfilepath[4], OutWidth[5], OutHeight[6], Outpixel[7](YUV422:0, YUV420:10, ARGB8888:1, ABGR8888:2,RGB565:3), DstFilePath[8], cnt[9]\n");
       return 1;
    }

    stParamer.u16InWidth = atoi(argv[1]);
    stParamer.u16InHeight = atoi(argv[2]);
    stParamer.eInPixelFormat = atoi(argv[3]);
    stParamer.pSrcFile = argv[4];

    stParamer.u16OutWidth = atoi(argv[5]);
    stParamer.u16OutHeight = atoi(argv[6]);
    stParamer.eOutPixelFormat = atoi(argv[7]);
    stParamer.pDestFile = argv[8];

    stParamer.cnt = atoi(argv[9]);

    printf("Inwidth %d, Inheight %d, Inpixel %d, Srcfilename %s, Outwidth %d, Outheight %d, Outpixel %d,DestFilename %s, cnt %d\n",
        stParamer.u16InWidth, stParamer.u16InHeight,stParamer.eInPixelFormat,stParamer.pSrcFile,stParamer.u16OutWidth, stParamer.u16OutHeight,stParamer.eOutPixelFormat, stParamer.pDestFile,stParamer.cnt);

    STCHECKRESULT(checkParame(&stParamer));
    STCHECKRESULT(ST_OpenSrcFile(stParamer.pSrcFile, &stParamer.Srcsd));
    STCHECKRESULT(ST_OpenDestFile(stParamer.pDestFile, &stParamer.DestSd));
    STCHECKRESULT(St_BaseModuleInit(&stParamer));

    STCHECKRESULT(St_WriteFile(&stParamer));
    STCHECKRESULT(St_BaseModuleDeinit());
}
