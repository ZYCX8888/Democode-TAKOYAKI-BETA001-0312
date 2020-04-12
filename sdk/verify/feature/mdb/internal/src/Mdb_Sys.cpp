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
#include "Mdb_Sys.h"

Mdb_Sys::Mdb_Sys()
{
    printf("%s\n", __FUNCTION__);
    PREPARE_COMMAND("setbind", &Mdb_Sys::SetBindInfo, 10);
    PREPARE_COMMAND("setdepth", &Mdb_Sys::SetDepth, 6);
    PREPARE_COMMAND("setunbind", &Mdb_Sys::SetUnBindInfo, 8);
}

Mdb_Sys::~Mdb_Sys()
{
    printf("%s\n", __FUNCTION__);
}

void Mdb_Sys::SetBindInfo(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrameRate =0, u32DstFrameRate =0;

    memset(&stSrcChnPort, 0x0, sizeof(stSrcChnPort));
    memset(&stDstChnPort, 0x0, sizeof(stDstChnPort));

    if (!inStrings.empty())
    {
        stSrcChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
        stSrcChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
        stSrcChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);;
        stSrcChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]);; //Main stream
        stDstChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[4]);;
        stDstChnPort.u32DevId = (MI_U32)Atoi(inStrings[5]);;
        stDstChnPort.u32ChnId = (MI_U32)Atoi(inStrings[6]);;
        stDstChnPort.u32PortId = (MI_U32)Atoi(inStrings[7]);;

        u32SrcFrameRate = (MI_U32)Atoi(inStrings[8]);
        u32DstFrameRate = (MI_U32)Atoi(inStrings[9]);

        printf("Src(mod %d, Dev %d, chn %d, port %d), Dev(mod %d, Dev %d, chn %d, port %d), FrameRate(%d, %d)\n",
            stSrcChnPort.eModId, stSrcChnPort.u32DevId, stSrcChnPort.u32ChnId, stSrcChnPort.u32PortId,
            stDstChnPort.eModId, stDstChnPort.u32DevId, stDstChnPort.u32ChnId, stDstChnPort.u32PortId, u32SrcFrameRate, u32DstFrameRate);

        MDB_EXPECT_OK("MI_SYS_BindChnPort",strOut, MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, u32SrcFrameRate, u32DstFrameRate), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }
}

void Mdb_Sys::SetUnBindInfo(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrameRate =0, u32DstFrameRate =0;

    memset(&stSrcChnPort, 0x0, sizeof(stSrcChnPort));
    memset(&stDstChnPort, 0x0, sizeof(stDstChnPort));

    if (!inStrings.empty())
    {
        stSrcChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
        stSrcChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
        stSrcChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);;
        stSrcChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]);; //Main stream
        stDstChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[4]);;
        stDstChnPort.u32DevId = (MI_U32)Atoi(inStrings[5]);;
        stDstChnPort.u32ChnId = (MI_U32)Atoi(inStrings[6]);;
        stDstChnPort.u32PortId = (MI_U32)Atoi(inStrings[7]);;

        printf("Src(mod %d, Dev %d, chn %d, port %d), Dev(mod %d, Dev %d, chn %d, port %d)\n",
            stSrcChnPort.eModId, stSrcChnPort.u32DevId, stSrcChnPort.u32ChnId, stSrcChnPort.u32PortId,
            stDstChnPort.eModId, stDstChnPort.u32DevId, stDstChnPort.u32ChnId, stDstChnPort.u32PortId);

        MDB_EXPECT_OK("MI_SYS_UnBindChnPort",strOut, MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    }
    else
    {
        printf("instring Empty\n");
    }

}

void Mdb_Sys::SetDepth(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_U8 u8UserDepth = 0;
    MI_U8 u8TotalDepth = 0;

    memset(&stChnPort, 0x0, sizeof(stChnPort));

    if (!inStrings.empty())
    {
        memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));

        stChnPort.eModId = (MI_ModuleId_e)Atoi(inStrings[0]);
        stChnPort.u32DevId = (MI_U32)Atoi(inStrings[1]);
        stChnPort.u32ChnId = (MI_U32)Atoi(inStrings[2]);
        stChnPort.u32PortId = (MI_U32)Atoi(inStrings[3]);
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
