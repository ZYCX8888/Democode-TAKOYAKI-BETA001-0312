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
#include "mi_sensor.h"

#include "mi_vif.h"

#include "Mdb_Vif.h"

Mdb_Vif::Mdb_Vif()
{
    printf("%s\n", __FUNCTION__);
    PREPARE_COMMAND("createdev", &Mdb_Vif::CreateDev, 3);
    PREPARE_COMMAND("createport", &Mdb_Vif::CreatePort, 8);
    PREPARE_COMMAND("startport", &Mdb_Vif::StartPort, 2);
    PREPARE_COMMAND("stopport", &Mdb_Vif::StopPort, 2);
    PREPARE_COMMAND("disabledev", &Mdb_Vif::DisableDev, 1);
}

Mdb_Vif::~Mdb_Vif()
{
    printf("%s\n", __FUNCTION__);
}

/*[0] Dev
  [1] 0:BT656
      1:Digital Cam
      4:MIPI
  [2] 3: RealTime
      4: Came Frame
*/
void Mdb_Vif::CreateDev(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8DevId =0;
    MI_VIF_DevAttr_t stDevAttr;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_SYS_PixelFormat_e ePixFormat;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));

    memset(&stDevAttr,0x0, sizeof(stDevAttr));

    if (!inStrings.empty())
    {
        u8DevId = (MI_U8)Atoi(inStrings[0]);
        
        MDB_EXPECT_OK("MI_SNR_SetPlaneMode",strOut, MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE),MI_SUCCESS);

        MDB_EXPECT_OK("MI_SNR_SetRes",strOut, MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,0),MI_SUCCESS);
        MDB_EXPECT_OK("MI_SNR_Enable",strOut, MI_SNR_Enable(E_MI_SNR_PAD_ID_0),MI_SUCCESS);

        MDB_EXPECT_OK("MI_SNR_GetPadInfo",strOut,MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info),MI_SUCCESS);
        MDB_EXPECT_OK("MI_SNR_GetPlaneInfo",strOut,MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info),MI_SUCCESS);

        u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
        u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
        ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

        stDevAttr.eIntfMode = stPad0Info.eIntfMode;
        stDevAttr.eWorkMode = (MI_VIF_WorkMode_e)Atoi(inStrings[2]);
        stDevAttr.eHDRType = E_MI_VIF_HDR_TYPE_OFF;
        if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
            stDevAttr.eClkEdge = stPad0Info.unIntfAttr.stBt656Attr.eClkEdge;
        else
            stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;
    
        if(stDevAttr.eIntfMode == E_MI_VIF_MODE_MIPI)
            stDevAttr.eDataSeq =stPad0Info.unIntfAttr.stMipiAttr.eDataYUVOrder;
        else
            stDevAttr.eDataSeq = E_MI_VIF_INPUT_DATA_YUYV;
    
        if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
            memcpy(&stDevAttr.stSyncAttr, &stPad0Info.unIntfAttr.stBt656Attr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));
    
        stDevAttr.eBitOrder = E_MI_VIF_BITORDER_NORMAL; 

        printf("Vif Dev %d, intf %d,  workmode %d\n", u8DevId, stDevAttr.eIntfMode, stDevAttr.eWorkMode);
        MDB_EXPECT_OK("MI_VIF_CreateDev", strOut, MI_VIF_SetDevAttr(u8DevId, &stDevAttr), MI_SUCCESS);
        MDB_EXPECT_OK("MI_VIF_EnableDev", strOut, MI_VIF_EnableDev(u8DevId), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}


/*
pixel 12bppGR: 23  207sensor
      10bppRG: 18  317sensor
FrameRate: 0:30
           1:15
*/
void Mdb_Vif::CreatePort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId=0, u8PortId=0;
    MI_VIF_ChnPortAttr_t stVifPortInfo;
    memset(&stVifPortInfo, 0x0, sizeof(MI_VIF_ChnPortAttr_t));
    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        u8PortId = (MI_U8)Atoi(inStrings[1]);
        stVifPortInfo.stCapRect.u16X = (MI_U16)Atoi(inStrings[2]);
        stVifPortInfo.stCapRect.u16Y = (MI_U16)Atoi(inStrings[3]);
        stVifPortInfo.stCapRect.u16Width = (MI_U16)Atoi(inStrings[4]);
        stVifPortInfo.stCapRect.u16Height = (MI_U16)Atoi(inStrings[5]);
        stVifPortInfo.stDestSize.u16Width = (MI_U16)Atoi(inStrings[4]);
        stVifPortInfo.stDestSize.u16Height = (MI_U16)Atoi(inStrings[5]);
        stVifPortInfo.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
        stVifPortInfo.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stVifPortInfo.ePixFormat = (MI_SYS_PixelFormat_e)Atoi(inStrings[6]);
        stVifPortInfo.eFrameRate = (MI_VIF_FrameRate_e)Atoi(inStrings[7]);

        printf("Vif channel %d, port %d, Rect(%d,%d,%d,%d), DestWH(%dx%d), ePixel %d, FrameRate %d\n", u8ChannelId, u8PortId,stVifPortInfo.stCapRect.u16X, stVifPortInfo.stCapRect.u16Y, stVifPortInfo.stCapRect.u16Width, stVifPortInfo.stCapRect.u16Height,
            stVifPortInfo.stDestSize.u16Width, stVifPortInfo.stDestSize.u16Height, stVifPortInfo.ePixFormat, stVifPortInfo.eFrameRate);
        MDB_EXPECT_OK("MI_VIF_SetChnPortAttr", strOut, MI_VIF_SetChnPortAttr(u8ChannelId, u8PortId, &stVifPortInfo), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vif::StartPort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId=0, u8PortId=0, u8DevId =0;

    if (!inStrings.empty())
    {
         u8ChannelId= (MI_U8)Atoi(inStrings[0]);
         u8PortId= (MI_U8)Atoi(inStrings[1]);

        printf("Vif channel %d, port %d\n", u8ChannelId, u8PortId);
        MDB_EXPECT_OK("MI_VIF_EnableChnPort", strOut, MI_VIF_EnableChnPort(u8ChannelId, u8PortId), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vif::StopPort(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8ChannelId=0, u8PortId=0;

    if (!inStrings.empty())
    {
        u8ChannelId = (MI_U8)Atoi(inStrings[0]);
        u8PortId = (MI_U8)Atoi(inStrings[1]);

        printf("Vif channel %d, port %d\n", u8ChannelId, u8PortId);
        MDB_EXPECT_OK("MI_VIF_DisableChnPort", strOut, MI_VIF_DisableChnPort(u8ChannelId, u8PortId), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Vif::DisableDev(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8Dev =0;

    if (!inStrings.empty())
    {
        u8Dev = (MI_U8)Atoi(inStrings[0]);

        printf("Vif dev %d\n",u8Dev);
        MDB_EXPECT_OK("MI_VIF_DisableDev", strOut, MI_VIF_DisableDev(u8Dev), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}
