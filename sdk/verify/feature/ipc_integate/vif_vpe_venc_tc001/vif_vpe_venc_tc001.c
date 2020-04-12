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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mi_sys.h"
#include "../common/st_common.h"
#include "../common/st_vpe.h"
#include "../common/st_vif.h"

#include "mi_venc.h"
#include "../common/env.h"
#include "../common/venc_common.h"

#define BUFNUM 10
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
    const char *pDestFile;
    MI_U16 u16Width;
    MI_U16 u16Height; 
    MI_U16 u16PortId ;
    MI_SYS_PixelFormat_e ePixelFormat;
    int sd;
}Test_Parames_t;

static MI_S32 St_WriteFile(Test_Parames_t *pstParamer)
{
    MI_SYS_ChnPort_t stVpeChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U16  u16BufInfoStride =0;
    MI_U16  u16BufInfoHeight =0;
    MI_U32  u32FrameSize =0;
    int offset =0;
    MI_U8 u8BufNum = BUFNUM;

    if(pstParamer->cnt > u8BufNum)
        pstParamer->cnt = u8BufNum;
    
    stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort.u32DevId = 0;
    stVpeChnOutputPort.u32ChnId = 0;
    stVpeChnOutputPort.u32PortId = pstParamer->u16PortId;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u64TargetPts = 0x1234;
    stBufConf.stFrameCfg.eFormat = pstParamer->ePixelFormat;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = pstParamer->u16Width;
    stBufConf.stFrameCfg.u16Height = pstParamer->u16Height;

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

            if(pstParamer->cnt >= u8BufNum)
            {
                printf("begin write file\n");
                STCHECKRESULT(ST_Write_OneFrame(pstParamer->sd, offset, stBufInfo.stFrameData.pVirAddr[0], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight));
                offset += u32FrameSize;

                if(pstParamer->ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
                {
                    STCHECKRESULT(ST_Write_OneFrame(pstParamer->sd, offset, stBufInfo.stFrameData.pVirAddr[1], u16BufInfoStride, u16BufInfoStride, u16BufInfoHeight/2));
                    offset += u32FrameSize/2;
                }
            }
            STCHECKRESULT(MI_SYS_ChnOutputPortPutBuf(hHandle));
            printf("put buf %d done\n", u8BufNum);
            u8BufNum--;
        }
    }while(u8BufNum > 0);

    return MI_SUCCESS;
}


int _VENC_GetRcConfig(int iChn, MI_VENC_ModType_e eModType, VENC_Rc_t *pstRc)
{
    int iApplyCfg;
    MI_BOOL bErr;
    char *szRcType;
    char szQp[64];

    if(pstRc == NULL)
        return -1;
    if(eModType == E_MI_VENC_MODTYPE_MAX)
    {
        pstRc->eRcMode = E_MI_VENC_RC_MODE_MAX;
        return MI_SUCCESS;
    }
    memset(pstRc, 0, sizeof(VENC_Rc_t));

    pstRc->u32Gop = get_cfg_int("VENC_GOP", &bErr);
    iApplyCfg = get_cfg_int("VENC_RC", &bErr);

    if(iApplyCfg == 0)//not by cfg system, use default value
    {
        szRcType = "FixQp";
    }
    else
    {
        szRcType = get_cfg_str("RcType", &bErr);
        if(bErr)
        {
            DBG_ERR("Can not find RcType config.\n");
            return -2;
        }
    }
    pstRc->u32SrcFrmRateNum = 30;
    pstRc->u32SrcFrmRateDen = 1;

    if(strcmp("Cbr", szRcType) == 0)
    {
        pstRc->u32Bitrate = get_cfg_int("Bitrate", &bErr);
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            break;
        default:
            return -2;
            break;
        }
    }
    else if(strcmp("Vbr", szRcType) == 0)
    {
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264VBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            break;
        default:
            return -2;
            break;
        }
        pstRc->u32Bitrate = get_cfg_int("Bitrate", &bErr);
        pstRc->u32VbrMinQp = get_cfg_int("VbrMinQp", &bErr);
        pstRc->u32VbrMaxQp = get_cfg_int("VbrMaxQp", &bErr);
    }
    else if(strcmp("FixQp", szRcType) == 0)
    {
        switch (eModType)
        {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            break;
        case E_MI_VENC_MODTYPE_JPEGE:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            break;
        default:
            return -2;
            break;
        }
        if(iApplyCfg)
        {
            snprintf(szQp, sizeof(szQp)-1, "VENC_CH%02d_QP", iChn);
            pstRc->u32FixQp = get_cfg_int(szQp, &bErr) + iChn;
            if (bErr)
            {
                pstRc->u32FixQp = DEF_QP + iChn;
            }
        }
        else
        {
            pstRc->u32FixQp = /*venc_state.rc.u8QpI*/ DEF_QP + iChn;
        }
    }
    else if(strcmp("Abr", szRcType) == 0)
    {
        pstRc->u32AvrMaxBitrate = get_cfg_int("AbrMaxBitrate", &bErr);
    }
    else
    {
        DBG_ERR("Unknown rcType %s.\n", szRcType);
        return -3;
    }

    return MI_SUCCESS;
}

static Chn_t _stChn;


static MI_S32 St_BaseModuleInit(Test_Parames_t     *pstParamer)
{
    MI_SYS_ChnPort_t stChnPort;
    ST_Sys_BindInfo_t stBindInfo;
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VPE_PortInfo_t stPortInfo;
    
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    memset(&stVpeChannelInfo, 0x0, sizeof(ST_VPE_ChannelInfo_t));
    memset(&stPortInfo, 0x0, sizeof(ST_VPE_PortInfo_t));
    
    STCHECKRESULT(ST_Sys_Init());
    STCHECKRESULT(ST_Vif_CreateDev(0, SAMPLE_VI_MODE_DIGITAL_CAM_1_1080P_REALTIME));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 1920;
    stVifPortInfoInfo.u32RectHeight = 1080;
    stVifPortInfoInfo.u32DestWidth = 1920;
    stVifPortInfoInfo.u32DestHeight = 1080;
    stVifPortInfoInfo.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, 0));
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
    stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));
    stPortInfo.DepVpeChannel = 0;
    stPortInfo.ePixelFormat = pstParamer->ePixelFormat;
    stPortInfo.u16OutputWidth = pstParamer->u16Width;
    stPortInfo.u16OutputHeight = pstParamer->u16Height;
    STCHECKRESULT(ST_Vpe_CreatePort(pstParamer->u16PortId, &stPortInfo)); //default support port0 --->>> vdisp
    printf("vpe port%d, (%dx%d), pixel %d, runmode %d\n", pstParamer->u16PortId, stPortInfo.u16OutputWidth, 
        stPortInfo.u16OutputHeight, stPortInfo.ePixelFormat, stVpeChannelInfo.eRunningMode);
    
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = pstParamer->u16PortId;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5);
    printf("depth portid %d\n", pstParamer->u16PortId);
    /************************************************
    Step7:  Bind VIF->VPE
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

    /************************************************
    Step8:  init VENC
    *************************************************/
    MI_U32 u32Cnt;
    STCHECKRESULT(MI_VENC_GetMaxStreamCnt(0, &u32Cnt));//dummy function to connect module
    get_cfg_from_env();

    VENC_Rc_t stRc;
    memset(&stRc, 0x0, sizeof(stRc));

    MI_U32 u32VencChn = 0;
    MI_VENC_ModType_e eModType = E_MI_VENC_MODTYPE_H264E;
    STCHECKRESULT(_VENC_GetRcConfig(u32VencChn, eModType, &stRc));

    MI_U32 u32PicWidth = pstParamer->u16Width;
    MI_U32 u32PicHeight = pstParamer->u16Height;

    create_venc_channel(eModType, u32VencChn, u32PicWidth, u32PicHeight, &stRc);

    MI_VENC_StartRecvPic(u32VencChn);


    /************************************************
    Step9:  Bind VPE->VENC
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VPE dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = 1;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
  
    _stChn.eModType = eModType;
    _stChn.u32ChnId = u32VencChn;
    pthread_create(&_stChn.tid, NULL, venc_channel_func, &_stChn);

    
    return MI_SUCCESS;
}

static MI_S32 St_BaseModuleDeinit(Test_Parames_t     * pstParamer)
{
    ST_Sys_BindInfo_t stBindInfo;

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VPE dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = 1;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    
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
    destroy_venc_channel(0);
 

    STCHECKRESULT(ST_Vpe_StopPort(0, pstParamer->u16PortId));
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));
    STCHECKRESULT(ST_Sys_Exit());
    return MI_SUCCESS;
}

int checkParame(Test_Parames_t     *pstParamer)
{
    int cnt =pstParamer->cnt;
    const char *pDestFile = pstParamer->pDestFile;
    MI_U16 u16Width=pstParamer->u16Width, u16Height=pstParamer->u16Height, u16PortId =pstParamer->u16PortId;
    MI_SYS_PixelFormat_e ePixelFormat = pstParamer->ePixelFormat;

    if(u16PortId <0 || u16PortId > 3)
    {
        printf("portid %d err\n", u16PortId);
        return 1;
    }

    if(u16Width < 0 || u16Width > 1920 || u16Height < 0 || u16Height > 1080)
    {
        printf("width %d, height %d err \n", u16Width, u16Height);
        return 1;
    }

    if(ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV 
        && ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 
        && ePixelFormat !=E_MI_SYS_PIXEL_FRAME_ARGB8888)
    {
        printf("pixel %d err \n", ePixelFormat);
        return 1;
    }

    if(pDestFile == NULL || cnt ==0)
    {
        printf("destfill is %p, cnt %d \n", pDestFile, cnt);
        return 1;
    }

    return MI_SUCCESS;
}

MI_S32 main(int argc, char **argv)
{
    Test_Parames_t  stParamer;
    memset(&stParamer, 0x0, sizeof(Test_Parames_t));
    
    if(argc < 7)
    {
       printf("paramer cnt %d < 7 \n", argc);
       printf("portid[1], width[2], height[3], pixel[4](YUV422:0, YUV420:9, ARGB8888:1), filepath[5], cnt[6]\n");
       return 1;
    }
    
    stParamer.u16PortId = atoi(argv[1]);
    stParamer.u16Width = atoi(argv[2]);
    stParamer.u16Height = atoi(argv[3]);
    stParamer.ePixelFormat = atoi(argv[4]);
    stParamer.pDestFile = argv[5];
    stParamer.cnt = atoi(argv[6]);

    printf("portid %d, width %d, height %d, pixel %d, filename %s, cnt %d\n", stParamer.u16PortId, 
        stParamer.u16Width, stParamer.u16Height,stParamer.ePixelFormat, stParamer.pDestFile,stParamer.cnt);
    
    STCHECKRESULT(checkParame(&stParamer));
    STCHECKRESULT(ST_OpenDestFile(stParamer.pDestFile, &stParamer.sd));
    STCHECKRESULT(St_BaseModuleInit(&stParamer));

    pthread_join(_stChn.tid, NULL);
    
    
    //STCHECKRESULT(St_WriteFile(&stParamer));
    STCHECKRESULT(St_BaseModuleDeinit(&stParamer));
    return 0;
}
