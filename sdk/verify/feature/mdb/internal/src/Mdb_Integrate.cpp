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

#include <sstream>

#include "mi_sys.h"
#include "mi_venc.h"
#include "mi_vpe.h"
#include "mi_divp.h"
//#include "mi_ldc.h"
#include "mi_iqserver.h"
#include "mi_isp.h"
#include "st_common.h"
#include "st_vpe.h"
#include "st_vif.h"
#include "st_fb.h"
#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"

#include "Mdb_Integrate.h"

#ifndef ASSERT
#define ASSERT(_x_)                                                                         \
    do  {                                                                                   \
        if ( ! ( _x_ ) )                                                                    \
        {                                                                                   \
            printf("ASSERT FAIL: %s %s %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__);     \
            abort();                                                                        \
        }                                                                                   \
    } while (0)
#endif
#define RTSP_LISTEN_PORT    554
static MI_S32 St_SkipAndWriteFrame(int fd, MI_U8 u8Mod, MI_U32 u32Chn, MI_U32 u32VpePortId, MI_U16 byPassFrame, MI_U16 writeFrame)
{
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U32  u32FrameSize =0;
    int offset =0;
    int by_pass_frame = byPassFrame;
    int write_frame = writeFrame;

    stChnOutputPort.eModId = (MI_ModuleId_e)u8Mod;
    stChnOutputPort.u32DevId = 0;
    stChnOutputPort.u32ChnId = u32Chn;
    stChnOutputPort.u32PortId = u32VpePortId;
    MI_SYS_SetChnOutputPortDepth(&stChnOutputPort, 1, 1);

    while (1)
    {
        if (by_pass_frame)
        {
            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort , &stBufInfo,&hHandle))
            {
                // Add user write buffer to file
                u16BufInfoStride  = stBufInfo.stFrameData.u32Stride[0];
                u16BufInfoHeight = stBufInfo.stFrameData.u16Height;
                u32FrameSize = u16BufInfoStride*u16BufInfoHeight;
                // put frame
                printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
                stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
                stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
                STCHECKRESULT(MI_SYS_ChnOutputPortPutBuf(hHandle));
                by_pass_frame--;
                printf("########By pass cnt %d ###########\n", by_pass_frame);
                continue;
            }
        }
        else if (write_frame)
        {
            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort , &stBufInfo,&hHandle))
            {
                // Add user write buffer to file
                u16BufInfoStride  = stBufInfo.stFrameData.u32Stride[0];
                u16BufInfoHeight = stBufInfo.stFrameData.u16Height;
                u32FrameSize = u16BufInfoStride*u16BufInfoHeight;
                // put frame
                printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
                stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
                stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
                printf("########start to write file!!!!!!!!!!!!!!###########\n");
                STCHECKRESULT(MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0], ((u16BufInfoStride < 256) ? 256: u16BufInfoStride) * u16BufInfoHeight));
                STCHECKRESULT(ST_Write_OneFrame(fd, offset, (char *)stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight));
                offset += u32FrameSize;
                if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
                {
                    STCHECKRESULT(MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0], ((u16BufInfoStride < 256) ? 256: u16BufInfoStride) * u16BufInfoHeight));
                    STCHECKRESULT(ST_Write_OneFrame(fd, offset, (char *)stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2));
                    offset += u32FrameSize/2;
                }
                printf("########End to write file!!!!!!!! %d letf###########\n", write_frame);
                STCHECKRESULT(MI_SYS_ChnOutputPortPutBuf(hHandle));
                write_frame--;
                continue;
            }
        }
        else
        {
            break;
        }
        printf("Get buf error!\n");
        usleep(100 * 1000);
    }
    MI_SYS_SetChnOutputPortDepth(&stChnOutputPort, 0, 4);

    return MI_SUCCESS;
}
static MI_S32 St_SkipAndWriteVencData(int fd, MI_VENC_CHN VeChn, MI_U16 byPassFrame, MI_U16 u16FrameWriteCnt)
{
    MI_BOOL bWriteFile = FALSE;
    MI_S32 s32Ret;
    int by_pass_frame = byPassFrame;
    int frame_cnt = u16FrameWriteCnt;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    MI_VENC_Stream_t stStream;
    while (1)
    {
        if (by_pass_frame)
        {
            memset(&stStream, 0, sizeof(stStream));
            memset(&stPack, 0, sizeof(stPack));
            stStream.pstPack = stPack;
            stStream.u32PackCount = 1;
            s32Ret = MI_VENC_Query(VeChn, &stStat);
            if (s32Ret == MI_SUCCESS && stStat.u32CurPacks != 0)
            {
                if (MI_SUCCESS == MI_VENC_GetStream(VeChn, &stStream, 40))
                {
                    MI_VENC_ReleaseStream(VeChn, &stStream);
                    by_pass_frame--;
                    printf("######## Raw data By pass cnt %d ###########\n", by_pass_frame);
                    continue;
                }
            }
        }
        else if (frame_cnt)
        {
            MI_VENC_RequestIdr(VeChn, TRUE);
            memset(&stStream, 0, sizeof(stStream));
            memset(&stPack, 0, sizeof(stPack));
            stStream.pstPack = stPack;
            stStream.u32PackCount = 1;
            s32Ret = MI_VENC_Query(VeChn, &stStat);
            if (s32Ret == MI_SUCCESS && stStat.u32CurPacks != 0)
            {
                if (MI_SUCCESS == MI_VENC_GetStream(VeChn, &stStream, 40))
                {
                    printf("########start to write file!!!!!!!!###########\n");
                    for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
                    {
                        write(fd, (void *)stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
                    }
                    printf("########End to write file!!!!!!!!###########\n");
                    MI_VENC_ReleaseStream(VeChn, &stStream);
                    frame_cnt--;
                    continue;
                }
            }
        }
        else
        {
            break;
        }
        printf("Get buf error!\n");
        usleep(100 * 1000);
    }

    return MI_SUCCESS;
}

std::map<std::string, Live555RTSPServer*> Mdb_Integrate::mapRTSPServer;

Mdb_Integrate::Mdb_Integrate()
{
    PREPARE_COMMAND("vifinit", &Mdb_Integrate::VifInit, 3);
    PREPARE_COMMAND("vifdeinit", &Mdb_Integrate::VifDeinit, 0);

    PREPARE_COMMAND("vpeinit", &Mdb_Integrate::VpeInit, 4);
    PREPARE_COMMAND("vpedeinit", &Mdb_Integrate::VpeDeinit, 1);
    PREPARE_COMMAND("vpecreateport", &Mdb_Integrate::VpeCreatePort, 5);
    PREPARE_COMMAND("vpesetrot", &Mdb_Integrate::VpeSetRotation, 2);
    PREPARE_COMMAND("vpedestroyport", &Mdb_Integrate::VpeDestroyPort, 2);
    PREPARE_COMMAND("yuvwritefile", &Mdb_Integrate::YuvWriteFile, 6);

    PREPARE_COMMAND("divpinit", &Mdb_Integrate::DivpInit, 8);
    PREPARE_COMMAND("divpcreateport", &Mdb_Integrate::DivpCreatePort, 4);
    PREPARE_COMMAND("divpstart", &Mdb_Integrate::DivpStart, 1);
    PREPARE_COMMAND("divpstop", &Mdb_Integrate::DivpStop, 1);
    PREPARE_COMMAND("divpdeinit", &Mdb_Integrate::DivpDeInit, 1);

#if 0
    PREPARE_COMMAND("ldccreat", &Mdb_Integrate::LdcCreat, 1);
    PREPARE_COMMAND("ldcinit", &Mdb_Integrate::LdcInit, 1);
    PREPARE_COMMAND("ldcsetbin", &Mdb_Integrate::LdcSetBin, 2);
    PREPARE_COMMAND("ldcstart", &Mdb_Integrate::LdcStart, 1);
    PREPARE_COMMAND("ldcstop", &Mdb_Integrate::LdcStop, 1);
    PREPARE_COMMAND("ldcdeinit", &Mdb_Integrate::LdcDeInit, 1);
#endif 
    PREPARE_COMMAND("vencinit", &Mdb_Integrate::VencInit, 4);
    PREPARE_COMMAND("vencdeinit", &Mdb_Integrate::VencDeinit, 1);
    PREPARE_COMMAND("vencwritefile", &Mdb_Integrate::VencWriteFile, 4);
    PREPARE_COMMAND("vencinjectfrm", &Mdb_Integrate::VencInjectFrame, 5);
    PREPARE_COMMAND("vencstart", &Mdb_Integrate::VencStart, 1);
    PREPARE_COMMAND("vencstop", &Mdb_Integrate::VencStop, 1);
    PREPARE_COMMAND("vencsetsrc", &Mdb_Integrate::VencSetInput, 2);

    PREPARE_COMMAND("configpool", &Mdb_Integrate::ConfigPrivatePool, 8);

    PREPARE_COMMAND("setbind", &Mdb_Integrate::SetBindInfo, 10);
    PREPARE_COMMAND("setbind2", &Mdb_Integrate::SetBindInfo2, 12);
    PREPARE_COMMAND("setdepth", &Mdb_Integrate::SetDepth, 6);
    PREPARE_COMMAND("setunbind", &Mdb_Integrate::SetUnBindInfo, 8);
    PREPARE_COMMAND("setmma", &Mdb_Integrate::SetMmaConf, 4);

    PREPARE_COMMAND("iqopenserver", &Mdb_Integrate::IqOpenServer, 3);
    PREPARE_COMMAND("iqcloseserver", &Mdb_Integrate::IqCloseServer, 0);
    PREPARE_COMMAND("iqloadapibin", &Mdb_Integrate::IqLoadApiBin, 3);

    PREPARE_COMMAND("rtspstart", &Mdb_Integrate::RtspServerStart, 2);
    PREPARE_COMMAND("rtspstop", &Mdb_Integrate::RtspServerStop, 1);
    PREPARE_COMMAND("exportfile", &Mdb_Integrate::ExportFile, 2);
    PREPARE_COMMAND("fbinit", &Mdb_Integrate::FbInit, 0);
    PREPARE_COMMAND("fbfillrect", &Mdb_Integrate::FbFillRect, 5);
    PREPARE_COMMAND("fbinitmouse", &Mdb_Integrate::FbInitMouse, 4);
    PREPARE_COMMAND("fbsetmouse", &Mdb_Integrate::FbSetMouse, 2);
    PREPARE_COMMAND("fbshow", &Mdb_Integrate::FbShow, 1);
    PREPARE_COMMAND("fbfillrecttest", &Mdb_Integrate::FbFillRectTest, 5);
    PREPARE_COMMAND("fbsetcolorkey", &Mdb_Integrate::FbSetColorKey, 1);
    PREPARE_COMMAND("fbgetcolorkey", &Mdb_Integrate::FbGetColorKey, 0);
}
Mdb_Integrate::~Mdb_Integrate()
{
}
void Mdb_Integrate::ShowWelcome(std::string &strOut)
{
    strOut.assign("Welcome to integrate module test\n");
}
void Mdb_Integrate::YuvWriteFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    int fd = 0;

    MDB_EXPECT_OK("ST_OpenDestFile", strOut, ST_OpenDestFile(inStrings[0].c_str(), &fd), MI_SUCCESS);
    MDB_EXPECT_OK("St_SkipAndWriteVpeFrame", strOut, St_SkipAndWriteFrame(fd, Atoi(inStrings[1]), Atoi(inStrings[2]), Atoi(inStrings[3]), Atoi(inStrings[4]), Atoi(inStrings[5])), MI_SUCCESS);
    ST_CloseFile(fd);

}

void Mdb_Integrate::VifInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    MI_U8    u8vifworkmode = Atoi(inStrings[2]);
    MI_VIF_WorkMode_e  eWorkMode;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_U16 u16OutW = Atoi(inStrings[0]);
    MI_U16 u16OutH = Atoi(inStrings[1]);

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    MDB_EXPECT_OK("ST_Sys_Init", strOut, ST_Sys_Init(), MI_SUCCESS);
    if(u8vifworkmode == 0)
        eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    else if(u8vifworkmode == 1)
        eWorkMode = E_MI_VIF_WORK_MODE_RGB_FRAMEMODE;
    else
        eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;

    MDB_EXPECT_OK("MI_SNR_SetPlaneMode",strOut, MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE),MI_SUCCESS);
    MDB_EXPECT_OK("MI_SNR_SetRes",strOut, MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,0),MI_SUCCESS);
    MDB_EXPECT_OK("MI_SNR_Enable",strOut, MI_SNR_Enable(E_MI_SNR_PAD_ID_0),MI_SUCCESS);
    MDB_EXPECT_OK("MI_SNR_GetPadInfo",strOut,MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info),MI_SUCCESS);
    MDB_EXPECT_OK("MI_SNR_GetPlaneInfo",strOut,MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info),MI_SUCCESS);
    u32CapWidth = u16OutW;
    u32CapHeight = u16OutH;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    MDB_EXPECT_OK("ST_Vif_CreateDev", strOut, ST_Vif_CreateDev(0, E_MI_VIF_HDR_TYPE_OFF, &stPad0Info, eWorkMode), MI_SUCCESS);
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth; //venc hw limitation of width 32 pixel aligement.
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth; //venc hw limitation of width 32 pixel aligement.
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    MDB_EXPECT_OK("ST_Vif_CreatePort", strOut, ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo), MI_SUCCESS);
    MDB_EXPECT_OK("ST_Vif_StartPort", strOut, ST_Vif_StartPort(0, 0), MI_SUCCESS);
}


void Mdb_Integrate::VpeInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VPE_CHANNEL VpeChnl = Atoi(inStrings[0]);
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_U16 u16InputW =  Atoi(inStrings[1]);
    MI_U16 u16InputH = Atoi(inStrings[2]);
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVpeChannelInfo, 0x0, sizeof(ST_VPE_ChannelInfo_t));
    /************************************************
    Step4:  init VPE
    *************************************************/
    MDB_EXPECT_OK("MI_SNR_GetPlaneInfo",strOut,MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info),MI_SUCCESS);

    u32CapWidth = u16InputW;
    u32CapHeight = u16InputH;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    stVpeChannelInfo.u16VpeMaxW = u32CapWidth; //venc hw limitation of width 32 pixel aligement.
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode =(MI_VPE_RunningMode_e)Atoi(inStrings[3]);
    stVpeChannelInfo.eFormat = ePixFormat;
    if((stVpeChannelInfo.eRunningMode == E_MI_VPE_RUN_REALTIME_TOP_MODE)
        || (stVpeChannelInfo.eRunningMode == E_MI_VPE_RUN_REALTIME_BOTTOM_MODE))
    {
        stVpeChannelInfo.bRotation = TRUE;
    }
    MDB_EXPECT_OK("ST_Vpe_CreateChannel", strOut, ST_Vpe_CreateChannel(VpeChnl, &stVpeChannelInfo), MI_SUCCESS);
    MDB_EXPECT_OK("ST_Vpe_StartChannel", strOut, ST_Vpe_StartChannel(VpeChnl), MI_SUCCESS);
}


void Mdb_Integrate::VifDeinit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MDB_EXPECT_OK("ST_Vif_StopPort", strOut, ST_Vif_StopPort(0, 0), MI_SUCCESS);
    MDB_EXPECT_OK("ST_Vif_DisableDev", strOut, ST_Vif_DisableDev(0), MI_SUCCESS);
}

void Mdb_Integrate::VpeDeinit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VPE_CHANNEL VpeChnl = Atoi(inStrings[0]);
    MDB_EXPECT_OK("ST_Vpe_StopChannel", strOut, ST_Vpe_StopChannel(VpeChnl), MI_SUCCESS);
    MDB_EXPECT_OK("ST_Vpe_DestroyChannel", strOut, ST_Vpe_DestroyChannel(VpeChnl), MI_SUCCESS);
    MDB_EXPECT_OK("ST_Sys_Exit", strOut, ST_Sys_Exit(), MI_SUCCESS);
}

void Mdb_Integrate::VpeCreatePort(std::vector<std::string> &inStrings, std::string &strOut)
{
    ST_VPE_PortInfo_t stVpePortInfo;
    MI_SYS_ChnPort_t stVpeChnPort;
    MI_VPE_PORT VpePort = 0;
    MI_VPE_CHANNEL VpeChnl = Atoi(inStrings[0]);

    memset(&stVpeChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVpePortInfo, 0x0, sizeof(ST_VPE_PortInfo_t));
    stVpePortInfo.DepVpeChannel = VpeChnl;
    stVpePortInfo.ePixelFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[4]);
    stVpePortInfo.u16OutputWidth = Atoi(inStrings[2]); //venc hw limitation of width 32 pixel aligement.
    stVpePortInfo.u16OutputHeight = Atoi(inStrings[3]);
    VpePort = Atoi(inStrings[1]);
    MDB_EXPECT_OK("ST_Vpe_CreatePort", strOut, ST_Vpe_CreatePort(VpePort, &stVpePortInfo), MI_SUCCESS); //default support port0 --->>> vdisp
    stVpeChnPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnPort.u32DevId = 0;
    stVpeChnPort.u32ChnId = VpeChnl;
    stVpeChnPort.u32PortId = VpePort;
    MDB_EXPECT_OK("MI_SYS_SetChnOutputPortDepth", strOut, MI_SYS_SetChnOutputPortDepth(&stVpeChnPort, 0, 5), MI_SUCCESS);
}

void Mdb_Integrate:: VpeSetRotation(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VPE_CHANNEL VpeChannel = Atoi(inStrings[0]);
    MI_SYS_Rotate_e eRotateType =(MI_SYS_Rotate_e)Atoi(inStrings[1]);

    MDB_EXPECT_OK("MI_VPE_SetChannelRotation", strOut, MI_VPE_SetChannelRotation(VpeChannel, eRotateType),MI_SUCCESS);
}

void Mdb_Integrate::VpeDestroyPort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VPE_PORT VpePort = 0;
    MI_VPE_CHANNEL VpeChannel =0;
    
    VpePort = Atoi(inStrings[0]);
    VpeChannel = Atoi(inStrings[1]);
    MDB_EXPECT_OK("ST_Vpe_StopPort", strOut, ST_Vpe_StopPort(VpeChannel, VpePort), MI_SUCCESS);
}
void Mdb_Integrate::IqOpenServer(std::vector<std::string> &inStrings, std::string &strOut)
{
    int width = (int)Atoi(inStrings[0]);
    int height = (int)Atoi(inStrings[1]);
    MI_S32 ChnId = (int)Atoi(inStrings[2]);

    MDB_EXPECT_OK("MI_IQSERVER_Open", strOut, MI_IQSERVER_Open(width, height, ChnId), MI_SUCCESS);

}
void Mdb_Integrate::IqCloseServer(std::vector<std::string> &inStrings, std::string &strOut)
{
    MDB_EXPECT_OK("MI_IQSERVER_Close", strOut, MI_IQSERVER_Close(), MI_SUCCESS);
}
void Mdb_Integrate::IqLoadApiBin(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U32 channel = (int)Atoi(inStrings[0]);

    std::string str = inStrings[1];
    char *filepath = new char[str.length() + 1];
    strcpy(filepath, str.c_str());

    MI_U32 user_key = (int)Atoi(inStrings[2]);

    MDB_EXPECT_OK("MI_IQ_ISP_CmdLoadBinFile", strOut, MI_ISP_API_CmdLoadBinFile(channel, filepath, user_key), MI_SUCCESS);

    delete [] filepath;
}
void Mdb_Integrate::DivpInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_DIVP_ChnAttr_t stDivpChannelAttr;
    memset(&stDivpChannelAttr, 0x0, sizeof(stDivpChannelAttr));

    u8ChannelId = (MI_U8)Atoi(inStrings[0]);
    stDivpChannelAttr.u32MaxWidth = 1920;
    stDivpChannelAttr.u32MaxHeight = 1080;
    stDivpChannelAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChannelAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChannelAttr.eRotateType = (MI_SYS_Rotate_e)Atoi(inStrings[1]);
    stDivpChannelAttr.stCropRect.u16X = (MI_U16)Atoi(inStrings[2]);
    stDivpChannelAttr.stCropRect.u16Y = (MI_U16)Atoi(inStrings[3]);
    stDivpChannelAttr.stCropRect.u16Width = (MI_U16)Atoi(inStrings[4]);
    stDivpChannelAttr.stCropRect.u16Height =(MI_U16)Atoi(inStrings[5]);
    stDivpChannelAttr.bHorMirror = (MI_BOOL)Atoi(inStrings[6]);
    stDivpChannelAttr.bVerMirror = (MI_BOOL)Atoi(inStrings[7]);

    MDB_EXPECT_OK("MI_DIVP_CreateChn", strOut, MI_DIVP_CreateChn(u8ChannelId, &stDivpChannelAttr), MI_SUCCESS);

}

void Mdb_Integrate::DivpCreatePort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId =0;
    MI_DIVP_OutputPortAttr_t stDivpOutput;
    memset(&stDivpOutput, 0, sizeof(stDivpOutput));
    u8ChannelId = (MI_U8)Atoi(inStrings[0]);
    stDivpOutput.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stDivpOutput.ePixelFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[1]);
    stDivpOutput.u32Width = (MI_U16)Atoi(inStrings[2]);
    stDivpOutput.u32Height= (MI_U16)Atoi(inStrings[3]);

    MDB_EXPECT_OK("MI_DIVP_SetOutputPortAttr",strOut, MI_DIVP_SetOutputPortAttr(u8ChannelId, &stDivpOutput), MI_SUCCESS);
}

void Mdb_Integrate::DivpStart(std::vector<std::string> &inStrings, std::string &strOut)
{
     MI_U8 u8Channel =0;
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        MDB_EXPECT_OK("MI_DIVP_StartChn", strOut,MI_DIVP_StartChn(u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Integrate::DivpStop(std::vector<std::string> &inStrings, std::string &strOut)
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

void Mdb_Integrate::DivpDeInit(std::vector<std::string> &inStrings, std::string &strOut)
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
#if 0
void Mdb_Integrate::LdcCreat(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    MI_LDC_DEV ldc_devid =0;

    if (!inStrings.empty())
    {
        ldc_devid = (MI_LDC_DEV)Atoi(inStrings[0]);

        MDB_EXPECT_OK("MI_LDC_CreateDevice", strOut, MI_LDC_CreateDevice(ldc_devid), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}


void Mdb_Integrate::LdcInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Channel =0;
    MI_LDC_DEV ldc_devid =0;

    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);

        MDB_EXPECT_OK("MI_LDC_CreateChannel", strOut, MI_LDC_CreateChannel(ldc_devid, u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}


MI_S32 ST_ReadTableBin(const char *pConfigPath, void **tldc_bin, MI_U32 *pu32BinSize)
{
    struct stat statbuff;
    MI_U8 *pBufData = NULL;
    MI_S32 s32Fd = 0;
    MI_U32 u32Size = 0;

    if (pConfigPath == NULL)
    {
        ST_ERR("File path null!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    printf("Read file %s\n", pConfigPath);
    memset(&statbuff, 0, sizeof(struct stat));
    if(stat(pConfigPath, &statbuff) < 0)
    {
        ST_ERR("Bb table file not exit!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    else
    {
        if (statbuff.st_size == 0)
        {
            ST_ERR("File size is zero!\n");
            return MI_ERR_LDC_ILLEGAL_PARAM;
        }
        u32Size = statbuff.st_size;
    }
    s32Fd = open(pConfigPath, O_RDONLY);
    if (s32Fd < 0)
    {
        ST_ERR("Open file[%d] error!\n", s32Fd);
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    pBufData = (MI_U8 *)malloc(u32Size);
    if (!pBufData)
    {
        ST_ERR("Malloc error!\n");
        close(s32Fd);

        return MI_ERR_LDC_ILLEGAL_PARAM;
    }

    memset(pBufData, 0, u32Size);
    read(s32Fd, pBufData, u32Size);
    close(s32Fd);

    *tldc_bin = pBufData;
    *pu32BinSize = u32Size;

    printf("%d: read buffer %p \n",__LINE__, pBufData);
    printf("%d: &bin address %p, *binbuffer %p \n",__LINE__, tldc_bin, *tldc_bin);

    //free(pBufData);

    return MI_SUCCESS;
}


void Mdb_Integrate::LdcSetBin(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_LDC_DEV ldc_devid =0;
    MI_U8 u8Channel =0;
    
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        std::string str = inStrings[1];
        char *filepath = new char[str.length() + 1];
        strcpy(filepath, str.c_str());
        
        void *pLdcBinBuffer =NULL;
        MI_U32 u32LdcBinSize =0;
        
        ST_ReadTableBin(filepath, &pLdcBinBuffer, &u32LdcBinSize);
        MDB_EXPECT_OK("MI_LDC_SetChnBin",strOut,MI_LDC_SetConfig(ldc_devid, u8Channel, pLdcBinBuffer, u32LdcBinSize), MI_SUCCESS);
        free(pLdcBinBuffer);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Integrate::LdcStart(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_LDC_DEV ldc_devid =0;
    MI_U8 u8Channel =0;
    
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);

        MDB_EXPECT_OK("MI_LDC_StartChannel",strOut, MI_LDC_StartChannel(ldc_devid, u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Integrate::LdcStop(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_LDC_DEV ldc_devid =0;
    MI_U8 u8Channel =0;
    
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        MDB_EXPECT_OK("MI_LDC_StopChannel",strOut, MI_LDC_StopChannel(ldc_devid, u8Channel), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Integrate::LdcDeInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_LDC_DEV ldc_devid =0;
    MI_U8 u8Channel =0;
    
    if (!inStrings.empty())
    {
        u8Channel = (MI_U8)Atoi(inStrings[0]);
        MDB_EXPECT_OK("MI_LDC_DestroyChannel",strOut, MI_LDC_DestroyChannel(ldc_devid,u8Channel), MI_SUCCESS);
        MDB_EXPECT_OK("MI_LDC_DestroyDevice",strOut, MI_LDC_DestroyDevice(ldc_devid), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}
#endif
void Mdb_Integrate::VencWriteFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    int fd = 0;

    MDB_EXPECT_OK("ST_OpenDestFile", strOut, ST_OpenDestFile(inStrings[0].c_str(), &fd), MI_SUCCESS);
    MDB_EXPECT_OK("St_SkipAndWriteVencData", strOut, St_SkipAndWriteVencData(fd, Atoi(inStrings[1]), Atoi(inStrings[2]), Atoi(inStrings[3])), MI_SUCCESS);
    ST_CloseFile(fd);
}
void Mdb_Integrate::VencInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VENC_ChnAttr_t stVencAttr;
    MI_VENC_ParamJpeg_t stJpegPara;
    MI_U32 u32VencDevId = 0, s32Ret;
    MI_U32 u32Width = Atoi(inStrings[0]);
    MI_U32 u32Height = Atoi(inStrings[1]);
    MI_VENC_CHN VeChn =  Atoi(inStrings[2]);
    MI_U32 u32VenBitRate = 0;

    memset(&stVencAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    if (u32Width == 3840 && u32Height == 2160)
    {
        u32VenBitRate = 4 * 1024 * 1024;
    }
    else
    {
        u32VenBitRate = 2 * 1024 * 1024;
    }
    switch (Atoi(inStrings[3]))
    {
        case 0: //JPEG
        {
            stVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stVencAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stVencAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = stVencAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
            stVencAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = stVencAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
            stVencAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
        }
        break;
        case 1: //H264
        {
            stVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stVencAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stVencAttr.stVeAttr.stAttrH264e.u32PicWidth = stVencAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
            stVencAttr.stVeAttr.stAttrH264e.u32PicHeight = stVencAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
            stVencAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;

            stVencAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32BitRate = u32VenBitRate;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stVencAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
        }
        break;
        case 2: //H265
        {
            stVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stVencAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
            stVencAttr.stVeAttr.stAttrH265e.u32PicWidth = stVencAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
            stVencAttr.stVeAttr.stAttrH265e.u32PicHeight = stVencAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
            stVencAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

            stVencAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stVencAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
        }
        break;
        default:
            printf("error format!\n");
            return;
    }
    MDB_EXPECT_OK("MI_VENC_CreateChn", strOut, MI_VENC_CreateChn(VeChn, &stVencAttr), MI_SUCCESS);
    if(stVencAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        memset(&stJpegPara, 0, sizeof(MI_VENC_ParamJpeg_t));
        MI_VENC_GetJpegParam(0, &stJpegPara);
        stJpegPara.u32Qfactor = 30;
        MI_VENC_SetJpegParam(0, &stJpegPara);
    }
}

void Mdb_Integrate::VencSetInput(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VENC_CHN VeChn =  Atoi(inStrings[0]);
    MI_VENC_InputSourceConfig_t stSrcCfg;

    memset(&stSrcCfg, 0, sizeof(MI_VENC_InputSourceConfig_t));
    stSrcCfg.eInputSrcBufferMode = (MI_VENC_InputSrcBufferMode_e)Atoi(inStrings[1]);
    MDB_EXPECT_OK("MI_VENC_SetInputSourceConfig", strOut, MI_VENC_SetInputSourceConfig(VeChn, &stSrcCfg), MI_SUCCESS);
}
void Mdb_Integrate::VencStart(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VENC_CHN VeChn =  Atoi(inStrings[0]);

    MDB_EXPECT_OK("MI_VENC_StartRecvPic", strOut, MI_VENC_StartRecvPic(VeChn), MI_SUCCESS);
}
void Mdb_Integrate::VencStop(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VENC_CHN VeChn =  Atoi(inStrings[0]);

    MDB_EXPECT_OK("MI_VENC_StopRecvPic", strOut, MI_VENC_StopRecvPic(VeChn), MI_SUCCESS);
}
void Mdb_Integrate::VencDeinit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_VENC_CHN VeChn = 0;
    MI_U32 u32VencDevId = 0;

    VeChn = Atoi(inStrings[0]);
    MDB_EXPECT_OK("MI_VENC_DestroyChn", strOut, MI_VENC_DestroyChn(VeChn), MI_SUCCESS);

    return;
}
void Mdb_Integrate::VencInjectFrame(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_BufConf_t stBufConf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 u32YSize = 0;
    MI_U8 *pData = NULL, *pDataFrom = NULL, *pDataTo = NULL;
    MI_U16 u16Line = 0;
    MI_U16 u16InjectCnt = 0, i = 0;
    MI_SYS_ChnPort_t stChnVencChnPort;

    int fd = 0;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    memset(&stChnVencChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = Atoi(inStrings[1]);
    stBufConf.stFrameCfg.u16Height = Atoi(inStrings[2]);
    fd = open(inStrings[0].c_str(), O_RDONLY);
    if (fd <0 )
    {
        strOut += "Open file error!\n";
        return;
    }
    pData = ( MI_U8 *)malloc(stBufConf.stFrameCfg.u16Width * stBufConf.stFrameCfg.u16Height * 3 /2);
    ASSERT(pData);
    read(fd, pData, stBufConf.stFrameCfg.u16Width * stBufConf.stFrameCfg.u16Height * 3 /2);
    close(fd);

    pDataFrom = pData;
    stChnVencChnPort.u32ChnId = Atoi(inStrings[3]);
    stChnVencChnPort.u32PortId = 0;
    MDB_EXPECT_OK("MI_VENC_GetChnDevid", strOut, MI_VENC_GetChnDevid(stChnVencChnPort.u32ChnId, &stChnVencChnPort.u32DevId), MI_SUCCESS);
    stChnVencChnPort.eModId = E_MI_MODULE_ID_VENC;
    u16InjectCnt = Atoi(inStrings[4]);
    for (i = 0; i < u16InjectCnt; i++)
    {
        s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnVencChnPort, &stBufConf, &stBufInfo, &hHandle, 3000);
        if (s32Ret == MI_SUCCESS)
        {
            u32YSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0], u32YSize);
            MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[1], u32YSize >> 1);
            pDataTo = (MI_U8 *)stBufInfo.stFrameData.pVirAddr[0];
            for (u16Line = 0; u16Line < stBufInfo.stFrameData.u16Height; u16Line++)
            {
                memcpy(pDataTo, pDataFrom, stBufInfo.stFrameData.u16Width);
                pDataTo += stBufInfo.stFrameData.u32Stride[0];
                pDataFrom += stBufInfo.stFrameData.u16Width;
            }
            pDataTo = (MI_U8 *)stBufInfo.stFrameData.pVirAddr[1];
            for (u16Line = 0; u16Line < stBufInfo.stFrameData.u16Height / 2; u16Line++)
            {
                memcpy(pDataTo, pDataFrom, stBufInfo.stFrameData.u16Width);
                pDataTo += stBufInfo.stFrameData.u32Stride[0];
                pDataFrom += stBufInfo.stFrameData.u16Width;
            }
            MI_SYS_ChnInputPortPutBuf(hHandle, &stBufInfo , FALSE);
        }

    }
    free(pData);
}
void Mdb_Integrate::SetMmaConf(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_ModuleId_e eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
    MI_U32 u32DevId = (MI_U32)Atoi(inStrings[1]);
    MI_U32 u32ChnId = (MI_U32)Atoi(inStrings[2]);
    MI_U8 *pu8MMAHeapName = NULL;

    if(eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(u32ChnId, &u32DevId);
    }
    if (inStrings[3] == "NULL")
    {
        MI_U8 u8Empty = 0;

        MDB_EXPECT_OK("MI_SYS_SetChnMMAConf", strOut, MI_SYS_SetChnMMAConf(eModId, u32DevId, u32ChnId, &u8Empty), MI_SUCCESS);
    }
    else
    {
        pu8MMAHeapName = (MI_U8 *)inStrings[3].c_str();
        MDB_EXPECT_OK("MI_SYS_SetChnMMAConf", strOut, MI_SYS_SetChnMMAConf(eModId, u32DevId, u32ChnId, pu8MMAHeapName), MI_SUCCESS);
    }
}
void Mdb_Integrate::SetBindInfo(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrameRate =0, u32DstFrameRate =0;
    MI_U32 u32VencDevId = 0;

    memset(&stSrcChnPort, 0x0, sizeof(stSrcChnPort));
    memset(&stDstChnPort, 0x0, sizeof(stDstChnPort));

    stSrcChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
    stSrcChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
    stSrcChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);
    stSrcChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]); //Main stream
    u32SrcFrameRate =  (MI_U32)Atoi(inStrings[4]);
    stDstChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[5]);
    stDstChnPort.u32DevId = (MI_U32)Atoi(inStrings[6]);
    stDstChnPort.u32ChnId = (MI_U32)Atoi(inStrings[7]);
    stDstChnPort.u32PortId = (MI_U32)Atoi(inStrings[8]);
    u32DstFrameRate = (MI_U32)Atoi(inStrings[9]);

    if(stSrcChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stSrcChnPort.u32ChnId, &u32VencDevId);
        stSrcChnPort.u32DevId = u32VencDevId;
    }
    else if( stDstChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stDstChnPort.u32ChnId, &u32VencDevId);
        stDstChnPort.u32DevId = u32VencDevId;
    }
    printf("Src(mod %d, Dev %d, chn %d, port %d), Dev(mod %d, Dev %d, chn %d, port %d), FrameRate(%d, %d)\n",
        stSrcChnPort.eModId, stSrcChnPort.u32DevId, stSrcChnPort.u32ChnId, stSrcChnPort.u32PortId,
        stDstChnPort.eModId, stDstChnPort.u32DevId, stDstChnPort.u32ChnId, stDstChnPort.u32PortId, u32SrcFrameRate, u32DstFrameRate);

    MDB_EXPECT_OK("MI_SYS_BindChnPort",strOut, MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrameRate, u32DstFrameRate), MI_SUCCESS);

}
void Mdb_Integrate::SetBindInfo2(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrameRate =0, u32DstFrameRate =0;
    MI_U32 u32VencDevId = 0;
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam = 0;

    memset(&stSrcChnPort, 0x0, sizeof(stSrcChnPort));
    memset(&stDstChnPort, 0x0, sizeof(stDstChnPort));

    stSrcChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
    stSrcChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
    stSrcChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);
    stSrcChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]); //Main stream
    u32SrcFrameRate =  (MI_U32)Atoi(inStrings[4]);
    stDstChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[5]);
    stDstChnPort.u32DevId = (MI_U32)Atoi(inStrings[6]);
    stDstChnPort.u32ChnId = (MI_U32)Atoi(inStrings[7]);
    stDstChnPort.u32PortId = (MI_U32)Atoi(inStrings[8]);
    u32DstFrameRate = (MI_U32)Atoi(inStrings[9]);
    eBindType = (MI_SYS_BindType_e)Atoi(inStrings[10]);
    u32BindParam = (MI_U32)Atoi(inStrings[11]);

    if(stSrcChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stSrcChnPort.u32ChnId, &u32VencDevId);
        stSrcChnPort.u32DevId = u32VencDevId;
    }
    else if( stDstChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stDstChnPort.u32ChnId, &u32VencDevId);
        stDstChnPort.u32DevId = u32VencDevId;
    }
    printf("Src(mod %d, Dev %d, chn %d, port %d), Dev(mod %d, Dev %d, chn %d, port %d), FrameRate(%d, %d)\n",
        stSrcChnPort.eModId, stSrcChnPort.u32DevId, stSrcChnPort.u32ChnId, stSrcChnPort.u32PortId,
        stDstChnPort.eModId, stDstChnPort.u32DevId, stDstChnPort.u32ChnId, stDstChnPort.u32PortId, u32SrcFrameRate, u32DstFrameRate);

    MDB_EXPECT_OK("MI_SYS_BindChnPort",strOut, MI_SYS_BindChnPort2(&stSrcChnPort, &stDstChnPort, u32SrcFrameRate, u32DstFrameRate, eBindType, u32BindParam), MI_SUCCESS);
}
void Mdb_Integrate::SetUnBindInfo(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrameRate =0, u32DstFrameRate =0;
    MI_U32 u32VencDevId = 0;

    memset(&stSrcChnPort, 0x0, sizeof(stSrcChnPort));
    memset(&stDstChnPort, 0x0, sizeof(stDstChnPort));

    stSrcChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
    stSrcChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
    stSrcChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);;
    stSrcChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]);; //Main stream
    stDstChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[4]);;
    stDstChnPort.u32DevId = (MI_U32)Atoi(inStrings[5]);;
    stDstChnPort.u32ChnId = (MI_U32)Atoi(inStrings[6]);;
    stDstChnPort.u32PortId = (MI_U32)Atoi(inStrings[7]);;

    if(stSrcChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stSrcChnPort.u32ChnId, &u32VencDevId);
        stSrcChnPort.u32DevId = u32VencDevId;
    }
    else if( stDstChnPort.eModId == E_MI_MODULE_ID_VENC)
    {
        MI_VENC_GetChnDevid(stDstChnPort.u32ChnId, &u32VencDevId);
        stDstChnPort.u32DevId = u32VencDevId;
    }

    printf("Src(mod %d, Dev %d, chn %d, port %d), Dev(mod %d, Dev %d, chn %d, port %d)\n",
        stSrcChnPort.eModId, stSrcChnPort.u32DevId, stSrcChnPort.u32ChnId, stSrcChnPort.u32PortId,
        stDstChnPort.eModId, stDstChnPort.u32DevId, stDstChnPort.u32ChnId, stDstChnPort.u32PortId);

    MDB_EXPECT_OK("MI_SYS_UnBindChnPort",strOut, MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);

}

void Mdb_Integrate::SetDepth(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_U8 u8UserDepth = 0;
    MI_U8 u8TotalDepth = 0;

    memset(&stChnPort, 0x0, sizeof(stChnPort));

    if (!inStrings.empty())
    {
        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

        stChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
        stChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);
        stChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]);
        if (stChnPort.eModId == E_MI_MODULE_ID_VENC)
        {
            MDB_EXPECT_OK("MI_VENC_GetChnDevid", strOut, MI_VENC_GetChnDevid(stChnPort.u32ChnId, &stChnPort.u32DevId), MI_SUCCESS);
        }
        else
        {
            stChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
        }
        u8UserDepth = (MI_U8)Atoi(inStrings[4]);
        u8TotalDepth =(MI_U8)Atoi(inStrings[5]);

        printf("mod %d, dev %d, chnl %d, port %d,depth(%d, %d)\n", stChnPort.eModId, stChnPort.u32DevId, stChnPort.u32ChnId, stChnPort.u32PortId, u8UserDepth, u8TotalDepth);
        MDB_EXPECT_OK("MI_SYS_SetChnOutputPortDepth", strOut,MI_SYS_SetChnOutputPortDepth(&stChnPort, u8UserDepth, u8TotalDepth), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}
void Mdb_Integrate::ConfigPrivatePool(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;

    memset(&stGlobalPrivPoolConf, 0, sizeof(MI_SYS_GlobalPrivPoolConfig_t));
    stGlobalPrivPoolConf.bCreate = Atoi(inStrings[0]);
    stGlobalPrivPoolConf.eConfigType = (MI_SYS_InsidePrivatePoolType_e)Atoi(inStrings[1]);
    switch (stGlobalPrivPoolConf.eConfigType)
    {
        case E_MI_SYS_VPE_TO_VENC_PRIVATE_RING_POOL:
        {
            stGlobalPrivPoolConf.uConfig.stPreVpe2VencRingPrivPoolConfig.u32VencInputRingPoolStaticSize = (MI_U32)Atoi(inStrings[2]);
            strcpy((char *)stGlobalPrivPoolConf.uConfig.stPreVpe2VencRingPrivPoolConfig.u8MMAHeapName, inStrings[3].c_str());
        }
        break;
        case E_MI_SYS_PER_CHN_PRIVATE_POOL:
        {
            stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.eModule = (MI_ModuleId_e)Atoi(inStrings[2]);
            stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Devid = (MI_U32)Atoi(inStrings[3]);
            stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Channel = (MI_U32)Atoi(inStrings[4]);
            stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = (MI_U32)Atoi(inStrings[5]);
            strcpy((char *)stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u8MMAHeapName, inStrings[6].c_str());
        }
        break;
        case E_MI_SYS_PER_DEV_PRIVATE_POOL:
        {
            stGlobalPrivPoolConf.uConfig.stPreDevPrivPoolConfig.eModule = (MI_ModuleId_e)Atoi(inStrings[2]);
            stGlobalPrivPoolConf.uConfig.stPreDevPrivPoolConfig.u32Devid = (MI_U32)Atoi(inStrings[3]);
            stGlobalPrivPoolConf.uConfig.stPreDevPrivPoolConfig.u32PrivateHeapSize = (MI_U32)Atoi(inStrings[4]);
            strcpy((char *)stGlobalPrivPoolConf.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName, inStrings[5].c_str());
        }
        break;
        case E_MI_SYS_PER_CHN_PORT_OUTPUT_POOL:
        {
            stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.eModule = (MI_ModuleId_e)Atoi(inStrings[2]);
            stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.u32Devid = (MI_U32)Atoi(inStrings[3]);
            stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.u32Channel = (MI_U32)Atoi(inStrings[4]);
            stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.u32Port = (MI_U32)Atoi(inStrings[5]);
            stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.u32PrivateHeapSize = (MI_U32)Atoi(inStrings[6]);
            strcpy((char *)stGlobalPrivPoolConf.uConfig.stPreChnPortOutputPrivPool.u8MMAHeapName, inStrings[7].c_str());
        }
        break;
        default:
            return;
    }
    MDB_EXPECT_OK("MI_SYS_ConfigPrivateMMAPool", strOut, MI_SYS_ConfigPrivateMMAPool(&stGlobalPrivPoolConf), MI_SUCCESS);
}


#define STREAM_TITTLE "stream"
void* Mdb_Integrate::OpenStream(char const* szStreamName, void* arg)
{
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    char *pStr = (char *)szStreamName;
    std::string str;
    MI_VENC_CHN *pChnNum = NULL;

    if (strncmp(STREAM_TITTLE, szStreamName, strlen(STREAM_TITTLE)))
    {
        printf("Not found %s\n", STREAM_TITTLE);

        return NULL;
    }
    pStr += strlen(STREAM_TITTLE);
    str = *pStr;
    pChnNum = (MI_VENC_CHN *)malloc(sizeof(MI_VENC_CHN));
    ASSERT(pChnNum);
    *pChnNum = Atoi(str);
    s32Ret = MI_VENC_RequestIdr(*pChnNum, TRUE);
    if (MI_SUCCESS != s32Ret)
    {
        printf("request IDR fail, error:%x\n", s32Ret);

        return NULL;
    }
    printf("Open stream %s\n", szStreamName);

    return (void *)pChnNum;
}

int Mdb_Integrate::VideoReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[16];
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN chnNum = 0;

    memset(&stStream, 0, sizeof(stStream));
    memset(&stPack, 0, sizeof(stPack));

    ASSERT(handle);
    chnNum = *((MI_VENC_CHN *)handle);
    stStream.pstPack = stPack;
    stStream.u32PackCount = 1;
    s32Ret = MI_VENC_Query(chnNum, &stStat);
    if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
    {
        return 0;
    }
    stStream.u32PackCount = stStat.u32CurPacks;
    s32Ret = MI_VENC_GetStream(chnNum, &stStream, 40);
    if (MI_SUCCESS == s32Ret)
    {
        for (MI_U8 i = 0; i < stStream.u32PackCount; i++)
        {
            memcpy(ucpBuf + len, stStream.pstPack[i].pu8Addr, stStream.pstPack[i].u32Len);
            len += stStream.pstPack[i].u32Len;
        }
        MI_VENC_ReleaseStream(chnNum, &stStream);

        return len;
    }

    return 0;
}

int Mdb_Integrate::CloseStream(void* handle, void* arg)
{
    ASSERT(handle);
    printf("Close stream %s%d\n", STREAM_TITTLE, *((MI_VENC_CHN *)handle));
    free((MI_VENC_CHN *)handle);

    return 0;
}

void Mdb_Integrate::RtspServerStart(std::vector<std::string> &inStrings, std::string &strOut)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;
    int iRet = 0;
    char* urlPrefix = NULL;
    ServerMediaSession* mediaSession = NULL;
    ServerMediaSubsession* subSession = NULL;
    Live555RTSPServer *pRTSPServer = NULL;

    if (!strstr(inStrings[0].c_str(), STREAM_TITTLE))
    {
        printf("Not found %s\n", STREAM_TITTLE);

        return;
    }
    pRTSPServer = new Live555RTSPServer();
    if (pRTSPServer == NULL)
    {
        printf("malloc error\n");
        return;
    }

    iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    while (iRet < 0)
    {
        rtspServerPortNum++;

        if (rtspServerPortNum > 65535)
        {
            printf("Failed to create RTSP server: %s\n", pRTSPServer->getResultMsg());
            delete pRTSPServer;
            pRTSPServer = NULL;
            return;
        }

        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }

    urlPrefix = pRTSPServer->rtspURLPrefix();
    printf("=================URL===================\n");
    printf("%s%s\n", urlPrefix, inStrings[0].c_str());
    printf("=================URL===================\n");


    pRTSPServer->createServerMediaSession(mediaSession,
                                          inStrings[0].c_str(),
                                          NULL, NULL);
    if (Atoi(inStrings[1]) == 0) //MJPEG
    {
        subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                                    *(pRTSPServer->GetUsageEnvironmentObj()),
                                    inStrings[0].c_str(),
                                    OpenStream,
                                    VideoReadStream,
                                    CloseStream, 30);
    }
    else if (Atoi(inStrings[1]) == 1) // H264
    {
        subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                                    *(pRTSPServer->GetUsageEnvironmentObj()),
                                    inStrings[0].c_str(),
                                    OpenStream,
                                    VideoReadStream,
                                    CloseStream, 30);
    }
    else if (Atoi(inStrings[1]) == 2) // H265
    {
        subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                                    *(pRTSPServer->GetUsageEnvironmentObj()),
                                    inStrings[0].c_str(),
                                    OpenStream,
                                    VideoReadStream,
                                    CloseStream, 30);
    }
    else
    {
        Print(strOut, "Not support!\n", PRINT_COLOR_RED, PRINT_MODE_HIGHTLIGHT);

        return;
    }

    pRTSPServer->addSubsession(mediaSession, subSession);
    pRTSPServer->addServerMediaSession(mediaSession);


    pRTSPServer->Start();

    mapRTSPServer[inStrings[0]] = pRTSPServer;
    Print(strOut, "rtsp start ok!\n", PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);

}

void Mdb_Integrate::RtspServerStop(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::map<std::string, Live555RTSPServer*>::iterator iter;

    if (!strstr(inStrings[0].c_str(), STREAM_TITTLE))
    {
        printf("Not found %s\n", STREAM_TITTLE);

        return;
    }
    iter = mapRTSPServer.find(inStrings[0]);
    if(iter != mapRTSPServer.end())
    {
        iter->second->Join();
        delete iter->second;
        iter->second = NULL;
        mapRTSPServer.erase(iter);
        Print(strOut, "rtsp stop ok!\n", PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);
    }
    else
    {
        Print(strOut, "Not found string!\n", PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);
    }
}
void Mdb_Integrate::ExportFile(std::vector<std::string> &inStrings, std::string &strOut)
{
    char pBuf[50];
    int intWriteBytes = 0;
    int intReadBytes = 0;

    int fdFrom = open(inStrings[0].c_str(), O_RDONLY);
    if (fdFrom < 0)
    {
        perror("open");

        return;
    }
    int fdTo = open(inStrings[1].c_str(), O_WRONLY);
    if (fdTo < 0)
    {
        perror("open");

        return;
    }

    do
    {
        intReadBytes = read(fdFrom, pBuf, 50);
        if (intReadBytes)
        {
            write(fdTo, pBuf, intReadBytes);
            intWriteBytes += intReadBytes;
        }
    }while (intReadBytes == 50);
    close(fdFrom);
    close(fdTo);
    printf("######From %s dump 0x%x bytes to %s #######\n", inStrings[0].c_str(), intWriteBytes, inStrings[1].c_str());
}

void Mdb_Integrate::FbInit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MDB_EXPECT_OK("ST_Fb_Init", strOut, ST_Fb_Init(), MI_SUCCESS);
}
void Mdb_Integrate::FbFillRect(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_WindowRect_t stRect;
    MI_U32 u32Color = 0;

    stRect.u16X = (MI_U16)Atoi(inStrings[0]);
    stRect.u16Y = (MI_U16)Atoi(inStrings[1]);
    stRect.u16Width= (MI_U16)Atoi(inStrings[2]);
    stRect.u16Height= (MI_U16)Atoi(inStrings[3]);
    u32Color = (MI_U32)Atoi(inStrings[4]);

    MDB_EXPECT_OK("ST_Fb_FillRect", strOut, ST_Fb_FillRect(&stRect, u32Color), MI_SUCCESS);
}
void Mdb_Integrate::FbInitMouse(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_S32 s32MouseW = (MI_S32)Atoi(inStrings[1]);
    MI_S32 s32MouseH = (MI_S32)Atoi(inStrings[2]);
    MI_S32 s32BytePerPix = (MI_S32)Atoi(inStrings[3]);


    ST_Fb_InitMouse(s32MouseW, s32MouseH, s32BytePerPix, (MI_U8 *)inStrings[0].c_str()); //44 56 4
}
void Mdb_Integrate::FbSetMouse(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U32 u32X = (MI_U32)Atoi(inStrings[0]);
    MI_U32 u32Y = (MI_U32)Atoi(inStrings[1]);
    
    ST_Fb_MouseSet(u32X, u32Y);
}
void Mdb_Integrate::FbShow(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_BOOL bShow = (MI_BOOL)Atoi(inStrings[0]);;

    MDB_EXPECT_OK("ST_FB_Show", strOut, ST_FB_Show(bShow), MI_SUCCESS);
}
void Mdb_Integrate::FbFillRectTest(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_WindowRect_t stOldRect;
    MI_U32 u32OldColor = 0xFF;
    MI_SYS_WindowRect_t stRect;
    MI_U32 u32Color = 0xFF00;
    MI_U16 u16Pos = 0;
    MI_U32 u32Wait = (MI_U32)Atoi(inStrings[0]);
    MI_U32 u32MaxW = (MI_U32)Atoi(inStrings[1]);
    MI_U32 u32MaxH = (MI_U32)Atoi(inStrings[2]);
    MI_U32 u32RectW = (MI_U32)Atoi(inStrings[3]);
    MI_U16 u16Step = (MI_U16)Atoi(inStrings[4]);;


    while (u16Pos + u16Step + u32RectW < u32MaxW)
    {
        stOldRect.u16X = u16Pos;
        stOldRect.u16Y = 0;
        stOldRect.u16Width= u32RectW;
        stOldRect.u16Height= u32MaxH;
        
        stRect.u16X = u16Pos + u16Step;
        stRect.u16Y = 0;
        stRect.u16Width= u32RectW;
        stRect.u16Height= u32MaxH;
        ST_Fb_FillRect(&stOldRect, u32OldColor);
        ST_Fb_FillRect(&stRect, u32Color);
        u16Pos += u16Step;
        usleep(u32Wait * 1000);
    }
}
void Mdb_Integrate::FbSetColorKey(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U32 u32ColorKeyVal = (MI_U32)Atoi(inStrings[0]);

    MDB_EXPECT_OK("Fb_SetColorKey", strOut, ST_Fb_SetColorKey(u32ColorKeyVal), MI_SUCCESS);
}
void Mdb_Integrate::FbGetColorKey(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U32 u32ColorKeyVal = 0;
    std::stringstream ss;

    MDB_EXPECT_OK("Fb_GetColorKey", strOut, ST_Fb_GetColorKey(&u32ColorKeyVal), MI_SUCCESS);
    ss << "Color key val is 0x" << std::hex << u32ColorKeyVal << std::endl;
    strOut += ss.str();
}
