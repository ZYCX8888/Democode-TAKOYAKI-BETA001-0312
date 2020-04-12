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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>

#include "mi_sys.h"
#include "mi_divp.h"
#include "st_common.h"
#include "Mdb_Divp.h"

Mdb_Divp::Mdb_Divp()
{
    printf("%s\n", __FUNCTION__);
    PREPARE_COMMAND("createchannel", &Mdb_Divp::CreateChannel, 10);
    PREPARE_COMMAND("setchnattr", &Mdb_Divp::SetChannelAttr, 10);
    PREPARE_COMMAND("setoutput", &Mdb_Divp::SetOutputAttr, 4);
    PREPARE_COMMAND("startchannel", &Mdb_Divp::StartChannel, 1);
    PREPARE_COMMAND("stopchannel", &Mdb_Divp::StopChannel, 1);
    PREPARE_COMMAND("destroychannel", &Mdb_Divp::DestroyChannel, 1);
    PREPARE_COMMAND("writedivpfile", &Mdb_Divp::WriteDivpFile, 3);
    PREPARE_COMMAND("readdivpfile", &Mdb_Divp::ReadDivpFile, 3);
}

Mdb_Divp::~Mdb_Divp()
{
    printf("%s\n", __FUNCTION__);
}

void Mdb_Divp::CreateChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_DIVP_ChnAttr_t stDivpChannelAttr;
    memset(&stDivpChannelAttr, 0x0, sizeof(stDivpChannelAttr));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stDivpChannelAttr.u32MaxWidth = (MI_U32)Atoi(inStrings[1]);
        stDivpChannelAttr.u32MaxHeight = (MI_U32)Atoi(inStrings[2]);
        stDivpChannelAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChannelAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChannelAttr.eRotateType = (MI_SYS_Rotate_e)Atoi(inStrings[3]);
        stDivpChannelAttr.stCropRect.u16X = (MI_U16)Atoi(inStrings[4]);
        stDivpChannelAttr.stCropRect.u16Y = (MI_U16)Atoi(inStrings[5]);
        stDivpChannelAttr.stCropRect.u16Width = (MI_U16)Atoi(inStrings[6]);
        stDivpChannelAttr.stCropRect.u16Height =(MI_U16)Atoi(inStrings[7]);
        stDivpChannelAttr.bHorMirror = (MI_BOOL)Atoi(inStrings[8]);
        stDivpChannelAttr.bVerMirror = (MI_BOOL)Atoi(inStrings[9]);

        printf("channel %d, MaxWH(%dx%d), rotate %d,crop(%d,%d,%d,%d), HVMirror(%d, %d)\n", u8ChannelId, stDivpChannelAttr.u32MaxWidth, stDivpChannelAttr.u32MaxHeight, stDivpChannelAttr.eRotateType,
            stDivpChannelAttr.stCropRect.u16X, stDivpChannelAttr.stCropRect.u16Y, stDivpChannelAttr.stCropRect.u16Width, stDivpChannelAttr.stCropRect.u16Height,stDivpChannelAttr.bHorMirror, stDivpChannelAttr.bVerMirror);

        MDB_EXPECT_OK("MI_DIVP_CreateChn", strOut, MI_DIVP_CreateChn(u8ChannelId, &stDivpChannelAttr), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Divp::SetChannelAttr(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_DIVP_ChnAttr_t stDivpChannelAttr;
    memset(&stDivpChannelAttr, 0x0, sizeof(stDivpChannelAttr));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stDivpChannelAttr.u32MaxWidth = (MI_U32)Atoi(inStrings[1]);
        stDivpChannelAttr.u32MaxHeight = (MI_U32)Atoi(inStrings[2]);
        stDivpChannelAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChannelAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChannelAttr.eRotateType = (MI_SYS_Rotate_e)Atoi(inStrings[3]);
        stDivpChannelAttr.stCropRect.u16X = (MI_U16)Atoi(inStrings[4]);
        stDivpChannelAttr.stCropRect.u16Y = (MI_U16)Atoi(inStrings[5]);
        stDivpChannelAttr.stCropRect.u16Width = (MI_U16)Atoi(inStrings[6]);
        stDivpChannelAttr.stCropRect.u16Height =(MI_U16)Atoi(inStrings[7]);
        stDivpChannelAttr.bHorMirror = (MI_BOOL)Atoi(inStrings[8]);
        stDivpChannelAttr.bVerMirror = (MI_BOOL)Atoi(inStrings[9]);

        printf("channel %d, MaxWH(%dx%d), rotate %d,crop(%d,%d,%d,%d), HVMirror(%d, %d)\n", u8ChannelId, stDivpChannelAttr.u32MaxWidth, stDivpChannelAttr.u32MaxHeight, stDivpChannelAttr.eRotateType,
            stDivpChannelAttr.stCropRect.u16X, stDivpChannelAttr.stCropRect.u16Y, stDivpChannelAttr.stCropRect.u16Width, stDivpChannelAttr.stCropRect.u16Height,stDivpChannelAttr.bHorMirror, stDivpChannelAttr.bVerMirror);

        MDB_EXPECT_OK("MI_DIVP_SetChnAttr", strOut, MI_DIVP_SetChnAttr(u8ChannelId, &stDivpChannelAttr), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Divp::SetOutputAttr(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_DIVP_OutputPortAttr_t stDivpOutput;
    memset(&stDivpOutput, 0, sizeof(stDivpOutput));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stDivpOutput.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpOutput.ePixelFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[1]);
        stDivpOutput.u32Width = (MI_U16)Atoi(inStrings[2]);
        stDivpOutput.u32Height= (MI_U16)Atoi(inStrings[3]);

        printf("channel %d, port(%dx%d), pixel %d\n", u8ChannelId, stDivpOutput.u32Width, stDivpOutput.u32Height, stDivpOutput.ePixelFormat);
        MDB_EXPECT_OK("MI_DIVP_SetOutputPortAttr",strOut, MI_DIVP_SetOutputPortAttr(u8ChannelId, &stDivpOutput), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Divp::StartChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("start channel %d\n", u8Channel);
        MDB_EXPECT_OK("MI_DIVP_StartChn", strOut,MI_DIVP_StartChn(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Divp::StopChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("disable %d channel \n", u8Channel);
        MDB_EXPECT_OK("MI_DIVP_StopChn",strOut, MI_DIVP_StopChn(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Divp::DestroyChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("disable %d channel \n", u8Channel);
        MDB_EXPECT_OK("MI_DIVP_DestroyChn",strOut, MI_DIVP_DestroyChn(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

typedef struct stRWFile_s
{
    MI_U8 u8ChannelId;
    MI_U8 u8PortId;
    MI_U16 u16BufNum;
    const char *pFileName;
    MI_U16  u16InputW;
    MI_U16  u16InputH;
    MI_SYS_PixelFormat_e ePixelFormat;
}stRWFile_t;

static MI_S32 DIVP_ReadFileYUV420(int sd, MI_U32 *pu32SrcOffset, MI_SYS_BufInfo_t *pstBufInfo, MI_SYS_BUF_HANDLE hHandle)
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
    if (1 == ST_ReadOneFrameYUV420ByStride(sd, (char *)pstBufInfo->stFrameData.pVirAddr[0], (char *)pstBufInfo->stFrameData.pVirAddr[1], y_size, uv_size,u16BufInfoHeight, u16BufInfoWidth, u16BufInfoStride, u16BufInfoStride))
    {
        STCHECKRESULT(MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[0], y_size));
        STCHECKRESULT(MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[1], uv_size));
        *pu32SrcOffset += y_size + uv_size;
        STCHECKRESULT(MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, FALSE));
        printf("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
    }
    else
    {
        STCHECKRESULT(MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, TRUE));
        *pu32SrcOffset = 0;
        STCHECKRESULT(ST_FdRewind(sd));
    }

    return MI_SUCCESS;
}

static MI_S32 DIVP_ReadOnePackFrameByStride(int sd, MI_U32 *pu32SrcOffset, MI_SYS_BufInfo_t *pstBufInfo, MI_SYS_BUF_HANDLE hHandle, MI_U8 u8WidthAm)
{
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U16  u16BufInfoWidth =0;
    MI_U32  Data_size =0;

    u16BufInfoStride   = pstBufInfo->stFrameData.u32Stride[0];
    u16BufInfoHeight  = pstBufInfo->stFrameData.u16Height;
    u16BufInfoWidth = pstBufInfo->stFrameData.u16Width;
    Data_size  = u16BufInfoStride*u16BufInfoHeight;

    if (1 == ST_ReadOneFramePackByStride(sd, (char *)pstBufInfo->stFrameData.pVirAddr[0], u16BufInfoHeight, u16BufInfoWidth, u16BufInfoStride,u8WidthAm))
    {
        MI_SYS_FlushInvCache(pstBufInfo->stFrameData.pVirAddr[0], Data_size);
        *pu32SrcOffset += Data_size;
        MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, FALSE);
        //printf("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
    }
    else
    {
        MI_SYS_ChnInputPortPutBuf(hHandle,pstBufInfo, TRUE);
        *pu32SrcOffset = 0;
        ST_FdRewind(sd);
    }

    return MI_SUCCESS;
}

void *ReadFileTask(void *args)
{

    MI_SYS_ChnPort_t stDivpChninputPort;
    stRWFile_t *pstWriteFileParam = (stRWFile_t *)args;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    int sd =0;
    MI_U32 u32SrcOffset =0;

    memset(&stBufInfo, 0x0, sizeof(stBufInfo));
    memset(&stBufConf, 0x0, sizeof(stBufConf));
    memset(&stDivpChninputPort, 0x0, sizeof(stDivpChninputPort));

    stDivpChninputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChninputPort.u32DevId = 0;
    stDivpChninputPort.u32ChnId = pstWriteFileParam->u8ChannelId;
    stDivpChninputPort.u32PortId = 0;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u64TargetPts = 0x1234;
    stBufConf.stFrameCfg.eFormat = pstWriteFileParam->ePixelFormat;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = pstWriteFileParam->u16InputW;
    stBufConf.stFrameCfg.u16Height = pstWriteFileParam->u16InputH;

    ST_OpenSrcFile(pstWriteFileParam->pFileName, &sd);

    do{
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stDivpChninputPort,&stBufConf,&stBufInfo,&hHandle,0))
        {
            /*printf("get input buffer info pixel %d, size(%dx%d), vir(%p, %p, %p)", stBufInfo.stFrameData.ePixelFormat,
                stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
                */
            if(pstWriteFileParam->ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                //printf("read yuv420file \n");
                DIVP_ReadFileYUV420(sd, &u32SrcOffset, &stBufInfo, hHandle);
            }
            else if(pstWriteFileParam->ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV || pstWriteFileParam->ePixelFormat == E_MI_SYS_PIXEL_FRAME_RGB565)
            {
                //printf("read yuv422file \n");
                DIVP_ReadOnePackFrameByStride(sd, &u32SrcOffset, &stBufInfo, hHandle, 2);
            }
            else
            {
                //printf("read pixel File \n", pstParamer->eInPixelFormat);
                DIVP_ReadOnePackFrameByStride(sd, &u32SrcOffset, &stBufInfo, hHandle, 4);
            }
        }
        pstWriteFileParam->u16BufNum --;
    }while(pstWriteFileParam->u16BufNum > 0);
}

void Mdb_Divp::ReadDivpFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    stRWFile_t stReadFileParam;
    pthread_t  pReadFileTask;

    memset(&stReadFileParam, 0x0, sizeof(stReadFileParam));

    if (!inStrings.empty())
    {
        stReadFileParam.u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stReadFileParam.u16BufNum = (MI_U16)Atoi(inStrings[1]);
        stReadFileParam.pFileName = inStrings[2].c_str();
        stReadFileParam.u16InputW = (MI_U16)Atoi(inStrings[3]);
        stReadFileParam.u16InputH = (MI_U16)Atoi(inStrings[4]);
        stReadFileParam.ePixelFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[5]);

        pthread_create(&pReadFileTask, NULL, ReadFileTask, (void *)&stReadFileParam);
    }
    else
    {
         printf("instring Empty\n");
    }
}

void *WriteFileTask(void *args)
{
    MI_SYS_ChnPort_t stDivpChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U32  u32FrameSize =0;

    MI_SYS_ChnPort_t stChnPort;
    int sd =0;
    MI_U32 offset =0;
    stRWFile_t *pstWriteFileParam = (stRWFile_t *)args;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = pstWriteFileParam->u8ChannelId;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 6);

    stDivpChnOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnOutputPort.u32DevId = 0;
    stDivpChnOutputPort.u32ChnId = pstWriteFileParam->u8ChannelId;
    stDivpChnOutputPort.u32PortId = 0;

    ST_OpenDestFile(pstWriteFileParam->pFileName, &sd);

    do{
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChnOutputPort , &stBufInfo,&hHandle))
        {
            // Add user write buffer to file
            u16BufInfoStride  = stBufInfo.stFrameData.u32Stride[0];
            u16BufInfoHeight = stBufInfo.stFrameData.u16Height;
            u32FrameSize = u16BufInfoStride*u16BufInfoHeight;
            // put frame
            printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
            stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
            stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);

            printf("begin write file\n");
            ST_Write_OneFrame(sd, offset, (char *)stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight);
            offset += u32FrameSize;

            if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                ST_Write_OneFrame(sd, offset, (char *)stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2);
                offset += u32FrameSize/2;
            }

            MI_SYS_ChnOutputPortPutBuf(hHandle);
            printf("put buf %d done\n", pstWriteFileParam->u16BufNum);
            pstWriteFileParam->u16BufNum--;
        }
    }while(pstWriteFileParam->u16BufNum > 0);

    ST_CloseFile(sd);
}

void Mdb_Divp::WriteDivpFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    pthread_t pDivpWriteFile;

    stRWFile_t stWriteFileParam;
    memset(&stWriteFileParam, 0x0, sizeof(stRWFile_t));

    if (!inStrings.empty())
    {
        stWriteFileParam.u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stWriteFileParam.u16BufNum = (MI_U16)Atoi(inStrings[1]);
        stWriteFileParam.pFileName = inStrings[2].c_str();

        pthread_create(&pDivpWriteFile, NULL, WriteFileTask, (void *)&stWriteFileParam);
    }
    else
    {
         printf("instring Empty\n");
    }
}
