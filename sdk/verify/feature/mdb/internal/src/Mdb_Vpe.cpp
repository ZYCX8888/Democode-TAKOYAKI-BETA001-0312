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
#include "mi_vpe.h"
#include "st_common.h"
#include "Mdb_Vpe.h"

Mdb_Vpe::Mdb_Vpe()
{
    printf("%s\n", __FUNCTION__);
    PREPARE_COMMAND("createchannel", &Mdb_Vpe::CreateChannel, 4);
    PREPARE_COMMAND("setcrop", &Mdb_Vpe::SetCrop, 5);
    PREPARE_COMMAND("createport", &Mdb_Vpe::CreatePort, 5);
    PREPARE_COMMAND("startchannel", &Mdb_Vpe::StartChannel, 1);
    PREPARE_COMMAND("stopport", &Mdb_Vpe::StopPort, 2);
    PREPARE_COMMAND("stopchannel", &Mdb_Vpe::StopChannel, 1);
    PREPARE_COMMAND("destroychannel", &Mdb_Vpe::DestroyChannel, 1);
    PREPARE_COMMAND("writevpefile", &Mdb_Vpe::WriteVpeFile, 4);
}

Mdb_Vpe::~Mdb_Vpe()
{
    printf("%s\n", __FUNCTION__);
}

void Mdb_Vpe::CreateChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_VPE_ChannelAttr_t stVepChannelAttr;
    memset(&stVepChannelAttr, 0x0, sizeof(stVepChannelAttr));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stVepChannelAttr.u16MaxW = (MI_U16)Atoi(inStrings[1]);
        stVepChannelAttr.u16MaxH = (MI_U16)Atoi(inStrings[2]);
        stVepChannelAttr.bNrEn = FALSE;
        stVepChannelAttr.bEdgeEn = FALSE;
        stVepChannelAttr.bEsEn = FALSE;
        stVepChannelAttr.bContrastEn = FALSE;
        stVepChannelAttr.bUvInvert = FALSE;
        stVepChannelAttr.eRunningMode = (MI_VPE_RunningMode_e)Atoi(inStrings[3]);
        printf("channel %d, MaxWH(%dx%d), RunMode %d,\n", u8ChannelId, stVepChannelAttr.u16MaxW, stVepChannelAttr.u16MaxH, stVepChannelAttr.eRunningMode);
        MDB_EXPECT_OK("MI_VPE_CreateChannel", strOut, MI_VPE_CreateChannel(u8ChannelId, &stVepChannelAttr), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::SetCrop(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_SYS_WindowRect_t stCropWin;
    memset(&stCropWin, 0x0, sizeof(stCropWin));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        stCropWin.u16X = (MI_U16)Atoi(inStrings[1]);
        stCropWin.u16Y = (MI_U16)Atoi(inStrings[2]);
        stCropWin.u16Width = (MI_U16)Atoi(inStrings[3]);
        stCropWin.u16Height = (MI_U16)Atoi(inStrings[4]);

        printf("channel %d, crop(%d,%d,%d,%d)\n", u8ChannelId, stCropWin.u16X, stCropWin.u16Y, stCropWin.u16Width, stCropWin.u16Height);
        MDB_EXPECT_OK("MI_VPE_SetChannelCrop", strOut, MI_VPE_SetChannelCrop(u8ChannelId, &stCropWin), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::CreatePort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0, u8PortId =0;
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        u8PortId = (MI_U8)Atoi(inStrings[1]);
        stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpeMode.ePixelFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[2]);
        stVpeMode.u16Width = (MI_U16)Atoi(inStrings[3]);
        stVpeMode.u16Height= (MI_U16)Atoi(inStrings[4]);

        printf("channel %d, port %d, port(%dx%d), pixel %d\n", u8ChannelId,u8PortId, stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat);
        MDB_EXPECT_OK("MI_VPE_SetPortMode",strOut, MI_VPE_SetPortMode(u8ChannelId, u8PortId, &stVpeMode), MI_SUCCESS);
        MDB_EXPECT_OK("MI_VPE_EnablePort",strOut, MI_VPE_EnablePort(u8ChannelId, u8PortId), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::StartChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("start channel %d\n", u8Channel);
        MDB_EXPECT_OK("MI_VPE_StartChannel", strOut,MI_VPE_StartChannel (u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::StopPort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0, u8PortId =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        u8PortId = (MI_U8)Atoi(inStrings[1]);
        printf("disable %d channel port %d\n", u8Channel, u8PortId);
        MDB_EXPECT_OK("MI_VPE_DisablePort",strOut, MI_VPE_DisablePort(u8Channel, u8PortId), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::StopChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("disable %d channel \n", u8Channel);
        MDB_EXPECT_OK("MI_VPE_StopChannel",strOut, MI_VPE_StopChannel(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::DestroyChannel(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        printf("disable %d channel \n", u8Channel);
        MDB_EXPECT_OK("MI_VPE_DestroyChannel",strOut, MI_VPE_DestroyChannel(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vpe::WriteVpeFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stVpeChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U32  u32FrameSize =0;

    MI_U8 u8ChannelId=0, u8PortId =0;
    MI_U16 u16BufNum =0;
    const char *pDstFileName = NULL;
    MI_SYS_ChnPort_t stChnPort;
    int sd =0;
    MI_U32 offset =0;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        u8PortId = (MI_U8)Atoi(inStrings[1]);
        u16BufNum = (MI_U16)Atoi(inStrings[2]);
        pDstFileName = inStrings[3].c_str();
    }
    else
    {
         printf("instring Empty\n");
    }


    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = u8ChannelId;
    stChnPort.u32PortId = u8PortId;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 6);

    stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort.u32DevId = 0;
    stVpeChnOutputPort.u32ChnId = u8ChannelId;
    stVpeChnOutputPort.u32PortId = u8PortId;

    ST_OpenDestFile(pDstFileName, &sd);

    do{
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort , &stBufInfo,&hHandle))
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
            MDB_EXPECT_OK("ST_Write_OneFrame", strOut, ST_Write_OneFrame(sd, offset, (char *)stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight), MI_SUCCESS);
            offset += u32FrameSize;

            if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {
                MDB_EXPECT_OK("ST_Write_OneFrame", strOut, ST_Write_OneFrame(sd, offset, (char *)stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2), MI_SUCCESS);
                offset += u32FrameSize/2;
            }

            MDB_EXPECT_OK("MI_SYS_ChnOutputPortPutBuf", strOut, MI_SYS_ChnOutputPortPutBuf(hHandle), MI_SUCCESS);
            printf("put buf %d done\n", u16BufNum);
            u16BufNum--;
        }
    }while(u16BufNum > 0);

    ST_CloseFile(sd);
}
