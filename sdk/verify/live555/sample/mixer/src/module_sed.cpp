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
#if MIXER_SED_ENABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "module_common.h"
#include "mid_common.h"
#include "mid_vpe.h"
#include "mi_sys_datatype.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"

#include "mid_divp.h"

#include "mi_sed_datatype.h"
#include "mid_sed.h"
#include "mi_common_datatype.h"
#define RAW_W           352
#define RAW_H           288
#define VPE_OUTPUT_RAW_W  704
#define VPE_OUTPUT_RAW_H 576

#define SED_FRAMERATE   10
const static MI_S32 g_ieViChn=2;
extern MI_S32 g_EnableSed;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
extern int g_videoNumber;
static int g_sedchn_share = 0;

MI_S32 g_SED_Open = 0;
static MI_S32 sedDivpChn = 0;
static int sed_vpe_divp_init()
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_SYS_ChnPort_t stChnPort;
    MI_VPE_PortMode_t stVpeMode;
    Mixer_VPE_PortInfo_T stVpePortInfo;
    unsigned short port2_width=RAW_W,port2_height=RAW_H;
    memset(&stVpeMode, 0x00, sizeof(MI_VPE_PortMode_t));
	sedDivpChn = Mixer_Divp_GetChannleNum();
    ExecFunc(MI_VPE_GetPortMode(VpeCh, g_ieViChn, &stVpeMode), MI_VPE_OK);
    MIXER_DBG("%s %d :Benis g_ieViChn %d\n",__func__,__LINE__,g_ieViChn);
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId  = 0;
    stChnPort.u32ChnId  = VpeCh;
    stChnPort.u32PortId = g_ieViChn;

    if(stVpeMode.u16Width==0 && stVpeMode.u16Height==0) // not need enable vpe port 2
    {
        memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpePortInfo.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.u16VpeOutputHeight = VPE_OUTPUT_RAW_H;
        stVpePortInfo.u16VpeOutputWidth  = VPE_OUTPUT_RAW_W;
        stVpePortInfo.VpeChnPort.eModId = E_MI_MODULE_ID_VPE;
        stVpePortInfo.VpeChnPort.u32DevId = 0x0;
        stVpePortInfo.VpeChnPort.u32PortId  = g_ieViChn;
        stVpePortInfo.VpeChnPort.u32ChnId = VpeCh;


        Mixer_Vpe_StartPort(VpeCh, &stVpePortInfo);
        MIXER_DBG("%s %d :first time start vpe port %d\n\n\n\n",__func__,__LINE__,g_ieViChn);
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 3));
        g_sedchn_share = 0;
    }
    else
    {
        g_sedchn_share = 1;
        port2_width=stVpeMode.u16Width;
        port2_height=stVpeMode.u16Height;
        MIXER_DBG("the IE port(ChnId=%d, PortId=%d) of VPE has been initialed!\n", VpeCh, g_ieViChn);
    }
    // create divp
    MI_SYS_WindowRect_t stCropRect;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    MI_SYS_ChnPort_t destChnPort;
    memset(&stDivpPortAttr, 0x00, sizeof(MI_DIVP_OutputPortAttr_t));
    memset(&stCropRect, 0x00, sizeof(stCropRect));
    stCropRect.u16X = 0;
    stCropRect.u16Y = 0;
    stCropRect.u16Width  = port2_width;
    stCropRect.u16Height = port2_height;

    stDivpPortAttr.u32Width  = RAW_W;
    stDivpPortAttr.u32Height = RAW_H;
    stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stDivpPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(sedDivpChn, E_MI_SYS_ROTATE_NONE, &stCropRect));
    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(sedDivpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(sedDivpChn));
    destChnPort.eModId   = E_MI_MODULE_ID_DIVP;
    destChnPort.u32ChnId = sedDivpChn;
    destChnPort.u32DevId = 0;
    destChnPort.u32PortId = 0;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&destChnPort, 2, 4), MI_SUCCESS);//HC and HD will keep double user buffer
    // bind VPE to divp
    Mixer_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId    = stChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = stChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = stChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = stChnPort.u32PortId;

    stBindInfo.stDstChnPort.eModId    = destChnPort.eModId;
    stBindInfo.stDstChnPort.u32DevId  = destChnPort.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = destChnPort.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = destChnPort.u32PortId;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;

    stBindInfo.u32SrcFrmrate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
    stBindInfo.u32DstFrmrate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    return 0;
}
static int sed_vpe_divp_uninit()
{
    MI_VPE_CHANNEL VpeCh = 0;
    Mixer_Sys_BindInfo_T stBindInfo;

    //un vivp
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = VpeCh;
    stBindInfo.stSrcChnPort.u32PortId = g_ieViChn;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = sedDivpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;
    stBindInfo.u32DstFrmrate = (int)g_videoParam[0].stVifInfo.sensorFrameRate;

    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    MIXERCHECKRESULT(Mixer_Divp_StopChn(sedDivpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(sedDivpChn));

    // un vpe
    if(g_sedchn_share==0)
    {
        MI_VPE_DisablePort(VpeCh, g_ieViChn);
    }
	Mixer_Divp_PutChannleNum(sedDivpChn);
    g_sedchn_share = 0;
    return 0;
}

int sed_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen)
{
    MI_SED_CHN sed_chn = 1;
    switch(id)
    {
            static MI_S32 sed_ieWidth = RAW_W;
            static MI_S32 sed_ieHeight = RAW_H;

        case CMD_SED_OPEN:
        {
            int ret;
            int i =0;
            if(g_SED_Open == 0)
            {
                MI_S32 iqparam[3];
                memcpy(iqparam, param, sizeof(iqparam));
                MI_SED_DetectorAttr_t sedStAttr;
                sed_vpe_divp_init();
                MIXER_DBG("CMD_SED_OPEN vpe port=%d\n", g_ieViChn);
                if (0 == iqparam[0])
                {
                    sedStAttr.stAlgoAttr.eType = E_MI_IVEOBJDETECT_ALGOPARAM ;
                }
                else if (1 == iqparam[0])
                {
                 #if TARGET_CHIP_I6E
                    sedStAttr.stAlgoAttr.eType = E_MI_CNNOBJDETECT_ALGOPARAM;
					sedStAttr.stAlgoAttr.stCNNObjDetectAlgo.u32MaxVariableBufSize = 2097152;
					sedStAttr.stAlgoAttr.stCNNObjDetectAlgo.u32IPUChnId = 0;
					sedStAttr.stAlgoAttr.stMotionObjDetectAlgo.iveHandle = 0; 
				#else
				    MIXER_DBG("just have ipu dev support!\n");
				#endif
                }
                else if (2 == iqparam[0])
                {
                    sedStAttr.stAlgoAttr.eType = E_MI_MOTIONDETECT_ALGOPARAM;
                    sedStAttr.stAlgoAttr.stMotionObjDetectAlgo.iveHandle = 0;
                }
                else
                {
                    sedStAttr.stAlgoAttr.eType = E_MI_IVEOBJDETECT_ALGOPARAM ;
                }

                //TODO Default enable video 0
                g_videoNumber = 1;
                g_videoParam[0].u8SedEnable = 1;

                sedStAttr.stAlgoAttr.stIveObjDetectAlgo.u32VdfChn  = 4; //0formd 1forod 2forvg
                sedStAttr.stAlgoAttr.stIveObjDetectAlgo.u8Sensitivity = 80;
                sedStAttr.stTargetAttr.s32RltQp = -7;
                sedStAttr.stInputAttr.u32Width = sed_ieWidth;
                sedStAttr.stInputAttr.u32Height = sed_ieHeight;
                sedStAttr.stInputAttr.u32FrameRateDen = 1;
                sedStAttr.stInputAttr.u32FrameRateNum = SED_FRAMERATE;
                sedStAttr.stInputAttr.stInputPort.eModId = E_MI_MODULE_ID_DIVP;
                sedStAttr.stInputAttr.stInputPort.u32ChnId = sedDivpChn;
                sedStAttr.stInputAttr.stInputPort.u32DevId = 0;
                sedStAttr.stInputAttr.stInputPort.u32PortId = 0;

                MIXER_DBG("check QP = %d\n", sedStAttr.stTargetAttr.s32RltQp);
                ret = MI_SED_CreateChn(sed_chn,&sedStAttr);
                if(ret != MI_SUCCESS)
                {
                    MIXER_ERR("MI_SED_CreateChn return %d\n",ret);
                    break;
                }
                for(i=0; i<g_videoNumber; i++)
                {
                    if(g_videoParam[i].u8SedEnable)
                    {
                        MIXER_ERR("MI_SED_AttachToChn %d  \n",i);
                        ret = MI_SED_AttachToVencChn(sed_chn,(MI_SED_TARGET_CHN)i);
                        if(ret != MI_SUCCESS)
                        {
                            MIXER_ERR("MI_SED_AttachToChn %d  return %d\n",i,ret);
                        }
                    }
                }
                ret = MI_SED_StartDetector(sed_chn);
                if(ret != MI_SUCCESS)
                {
                    MIXER_ERR("MI_SED_StartDetector return %d\n",ret);
                }
				if(1 == iqparam[0])
				{
				  	startSedCnnobjDetect(sed_chn);
				}
                //TODO Wait Sed_API Code Review Will Enable
#if 0
                if (1 == iqparam[1])
                {
                   MI_SED_SetDbgLevel(MI_DBG_ERR);
                }
#endif
                g_SED_Open = 1;
            }
        }
        break;

        case CMD_SED_CLOSE:
        {
            int ret;
            int i;
            if(g_SED_Open)
            {
                stopSedCnnobjDetect();
                for(i=0; i<g_videoNumber; i++)
                {
                    if(g_videoParam[i].u8SedEnable)
                    {
                        ret = MI_SED_DetachFromVencChn(sed_chn,(MI_SED_TARGET_CHN)i);
                        if(ret != MI_SUCCESS)
                        {
                            MIXER_ERR("MI_SED_DetachFromChn %d  return %d\n",i,ret);
                        }
                    }
                }
                ret = MI_SED_DestroyChn(sed_chn);
                if(ret != MI_SUCCESS)
                {
                    MIXER_ERR("MI_SED_DestroyChn return %d\n",ret);
                }
                sed_vpe_divp_uninit();
                g_SED_Open = 0;
            }
        }
        break;
        default:
            break;
    }

    return 0;
}
#endif
