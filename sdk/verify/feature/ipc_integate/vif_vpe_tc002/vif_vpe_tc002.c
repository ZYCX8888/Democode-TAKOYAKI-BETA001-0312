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
#include "mi_sys.h"
#include "../common/st_common.h"
#include "../common/st_vpe.h"
#include "../common/st_vif.h"

#define FRAME_NUME 8

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

typedef struct Test_Parames_s
{
    int cnt;
    char stFileName[50];
    MI_U16 u16Width;
    MI_U16 u16Height; 
    MI_U16 u16PortId ;
    MI_SYS_PixelFormat_e ePixelFormat;
    int sd;
    MI_U32 u32offset;
    MI_BOOL bEnable;
    MI_U16  u16FrameNum;
}Test_Parames_t;

static MI_S32 St_WriteFile(Test_Parames_t     *pstParamer)
{
    MI_SYS_ChnPort_t stVpeChnOutputPort[4];
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle =0;
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U32  u32FrameSize =0;
    MI_U8  u8PortId = 0;

    memset(stVpeChnOutputPort, 0x0, sizeof(MI_SYS_ChnPort_t)*4);
    memset(&stBufInfo, 0x0, sizeof(stBufInfo));
    memset(&stBufConf, 0x0, sizeof(stBufConf));
    do{
        for(u8PortId=0; u8PortId<4; u8PortId++)
        {
            if(pstParamer[u8PortId].bEnable == TRUE)
            {

                if(pstParamer[u8PortId].u16FrameNum > 0 )
                {
                    printf("[%d]cnt %d begin\n",u8PortId, pstParamer[u8PortId].u16FrameNum);
                }
                else
                {
                    printf("[%d]cnt %d have done\n",u8PortId,pstParamer[u8PortId].u16FrameNum );
                    continue;
                }
                
                stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                stBufConf.u64TargetPts = 0x1234;
                stBufConf.stFrameCfg.eFormat = pstParamer[u8PortId].ePixelFormat;
                stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                stBufConf.stFrameCfg.u16Width = pstParamer[u8PortId].u16Width;
                stBufConf.stFrameCfg.u16Height = pstParamer[u8PortId].u16Height;

                stVpeChnOutputPort[u8PortId].eModId = E_MI_MODULE_ID_VPE;
                stVpeChnOutputPort[u8PortId].u32DevId = 0;
                stVpeChnOutputPort[u8PortId].u32ChnId = 0;
                stVpeChnOutputPort[u8PortId].u32PortId = pstParamer[u8PortId].u16PortId;
                
                if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort[u8PortId] , &stBufInfo,&hHandle))
                {
                    // Add user write buffer to file
                    u16BufInfoStride  = stBufInfo.stFrameData.u32Stride[0];
                    u16BufInfoHeight = stBufInfo.stFrameData.u16Height;
                    u32FrameSize = u16BufInfoStride*u16BufInfoHeight;
                    // put frame
                    printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height,
                    stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2], stBufInfo.stFrameData.ePixelFormat,
                    stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2]);
                    if(pstParamer[u8PortId].cnt>= pstParamer[u8PortId].u16FrameNum)
                    {
                        printf("port %d, begin write file %d\n",u8PortId, pstParamer[u8PortId].u16FrameNum);
                        STCHECKRESULT(ST_Write_OneFrame(pstParamer[u8PortId].sd, pstParamer[u8PortId].u32offset, stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight));
                        pstParamer[u8PortId].u32offset += u32FrameSize;

                        if(pstParamer[u8PortId].ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
                        {
                            STCHECKRESULT(ST_Write_OneFrame(pstParamer[u8PortId].sd, pstParamer[u8PortId].u32offset, stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2));
                            pstParamer[u8PortId].u32offset += u32FrameSize/2;
                        }
                    }
                    
                    STCHECKRESULT(MI_SYS_ChnOutputPortPutBuf(hHandle));
                    printf("port[%d]put buf %d done\n",u8PortId,pstParamer[u8PortId].u16FrameNum);
                    if(pstParamer[u8PortId].u16FrameNum > 0)
                    {
                        pstParamer[u8PortId].u16FrameNum--;
                    }
                    else 
                    {
                        pstParamer[u8PortId].u16FrameNum = 0;
                    }

                    if(pstParamer[u8PortId].u16FrameNum > FRAME_NUME)
                    {
                        pstParamer[u8PortId].u16FrameNum = 0;
                    }
                }
            }
            else
            {
                pstParamer[u8PortId].u16FrameNum = 0;
            }
        }

            if(pstParamer[0].u16FrameNum || pstParamer[1].u16FrameNum || pstParamer[2].u16FrameNum || pstParamer[3].u16FrameNum)
            {
                printf("[0]cnt %d, [1]cnt %d, [2]cnt %d, [3]cnt %d\n", pstParamer[0].u16FrameNum, pstParamer[1].u16FrameNum, pstParamer[2].u16FrameNum, pstParamer[3].u16FrameNum);
            }
            else
            {
                printf("[0]cnt %d, [1]cnt %d, [2]cnt %d, [3]cnt %d\n", pstParamer[0].u16FrameNum, pstParamer[1].u16FrameNum, pstParamer[2].u16FrameNum, pstParamer[3].u16FrameNum);
                break;
            }
    }while(1);

    return MI_SUCCESS;
}

static MI_S32 St_BaseModuleInit(Test_Parames_t      *pstParamer)
{
    MI_SYS_ChnPort_t stChnPort[4];
    ST_Sys_BindInfo_t stBindInfo;
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VPE_PortInfo_t stPortInfo[4];
    MI_U8 u8PortID =0;
    
    memset(stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t)*4);
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    memset(&stVpeChannelInfo, 0x0, sizeof(ST_VPE_ChannelInfo_t));
    memset(stPortInfo, 0x0, sizeof(ST_VPE_PortInfo_t)*4);
    
    STCHECKRESULT(ST_Sys_Init());
    STCHECKRESULT(ST_Vif_CreateDev(0, SAMPLE_VI_MODE_MIPI_1_1080P_REALTIME));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 1920;
    stVifPortInfoInfo.u32RectHeight = 1080;
    stVifPortInfoInfo.u32DestWidth = 1920;
    stVifPortInfoInfo.u32DestHeight = 1080;
    stVifPortInfoInfo.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_RG;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    printf("vif port(%dx%d), pixel %d\n", stVifPortInfoInfo.u32DestWidth, stVifPortInfoInfo.u32DestHeight, stVifPortInfoInfo.ePixFormat);
    /************************************************
    Step4:  init VPE
    *************************************************/
    stVpeChannelInfo.u16VpeMaxW = 1920;
    stVpeChannelInfo.u16VpeMaxH = 1080;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_RG;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));

    for(u8PortID=0; u8PortID<4; u8PortID++)
    {
        if(pstParamer[u8PortID].bEnable == TRUE)
        {
            stPortInfo[u8PortID].DepVpeChannel = 0;
            stPortInfo[u8PortID].ePixelFormat = pstParamer[u8PortID].ePixelFormat;
            stPortInfo[u8PortID].u16OutputWidth = pstParamer[u8PortID].u16Width;
            stPortInfo[u8PortID].u16OutputHeight = pstParamer[u8PortID].u16Height;
            STCHECKRESULT(ST_Vpe_CreatePort(pstParamer[u8PortID].u16PortId, &stPortInfo[u8PortID])); //default support port0 --.>> vdisp
            printf("vpe port%d, (%dx%d), pixel %d, runmode %d\n", pstParamer[u8PortID].u16PortId, stPortInfo[u8PortID].u16OutputWidth, 
            stPortInfo[u8PortID].u16OutputHeight, stPortInfo[u8PortID].ePixelFormat, stVpeChannelInfo.eRunningMode);

            stChnPort[u8PortID].eModId = E_MI_MODULE_ID_VPE;
            stChnPort[u8PortID].u32DevId = 0;
            stChnPort[u8PortID].u32ChnId = 0;
            stChnPort[u8PortID].u32PortId = pstParamer[u8PortID].u16PortId;
            MI_SYS_SetChnOutputPortDepth(&stChnPort[u8PortID], 2, 4);
        }
    }
    STCHECKRESULT(ST_Vif_StartPort(0, 0));

    /************************************************
    Step7:  Bind VIF.VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    return MI_SUCCESS;
}

static MI_S32 St_BaseModuleDeinit(Test_Parames_t      *pstParamer)
{
    ST_Sys_BindInfo_t stBindInfo;
    MI_U8 u8PortId =0;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Vpe_StopChannel(0));
    for(u8PortId =0; u8PortId< 4; u8PortId++)
    {
        if(pstParamer[u8PortId].bEnable == TRUE)
        {
            STCHECKRESULT(ST_Vpe_StopPort(0, pstParamer[u8PortId].u16PortId));
        }
    }
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));
    STCHECKRESULT(ST_Sys_Exit());
    return MI_SUCCESS;
}



MI_S32 main(int argc, char **argv)
{
    Test_Parames_t  stParamer[4];
    const char *dest_path = NULL;
    MI_U8 u8pixel =0;
    MI_U8 u8Cnt =0;
    MI_U8 u8PortId =0;
    MI_U8 u8PortNum =0;
    memset(stParamer, 0x0, sizeof(Test_Parames_t) * 4);
    if(argc < 5)
    {
       printf("paramer cnt %d < 5\n", argc);
       printf("PortNum[1], pixel[2](YUV422YUYV:0, YUV420:10, YUV422SP:9,ARGB8888:1, BGRA8888:3), filepath[3], cnt[4]\n");
       return 1;
    }

    u8PortNum = atoi(argv[1]);
    u8pixel =  atoi(argv[2]);
    dest_path = argv[3];
    u8Cnt = atoi(argv[4]);

    printf("pixel %d, filename %s, cnt %d\n",u8pixel, dest_path, u8Cnt);

    for(u8PortId=0; u8PortId< u8PortNum; u8PortId++)
    {
        stParamer[u8PortId].u16Width = 352;
        stParamer[u8PortId].u16Height = 288;
        stParamer[u8PortId].bEnable = TRUE;
        stParamer[u8PortId].u16FrameNum = 0;
    }

    stParamer[1].bEnable = TRUE;
    stParamer[2].bEnable = TRUE;
    stParamer[3].bEnable = TRUE;
    
    for(u8PortId=0; u8PortId<4; u8PortId++)
    {
        if(stParamer[u8PortId].bEnable == TRUE)
        {
            stParamer[u8PortId].ePixelFormat = u8pixel;
            sprintf(stParamer[u8PortId].stFileName, "%s/vpe_outport%d.yuv", dest_path, u8PortId);
            stParamer[u8PortId].cnt = u8Cnt;
            stParamer[u8PortId].u16PortId = u8PortId;
            stParamer[u8PortId].u16FrameNum = FRAME_NUME;
            STCHECKRESULT(ST_OpenDestFile(stParamer[u8PortId].stFileName, &stParamer[u8PortId].sd));
        }
    }

    STCHECKRESULT(St_BaseModuleInit(stParamer));
    
    STCHECKRESULT(St_WriteFile(stParamer));
    
    STCHECKRESULT(St_BaseModuleDeinit(stParamer));
    return 0;
}
