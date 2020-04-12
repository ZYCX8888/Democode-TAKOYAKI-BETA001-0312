/*
* module_ie.cpp- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "module_common.h"
#include "mid_common.h"
#include "mi_venc.h"
#include "mid_vpe.h"
#include "mid_md.h"

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#include "ssnn.h"
#endif

#include "mi_md.h"
#include "mi_od.h"
#include "mi_vg.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_divp_datatype.h"
#include "mid_video_type.h"
#include "mid_VideoEncoder.h"
#include "mid_divp.h"
#include "mid_hc.h"

MI_DIVP_CHN gDivpChn4Vdf = 0;

extern MI_S32 g_ieVpePort;
extern MI_S32 g_ieWidth;
extern MI_S32 g_ieHeight;
extern MI_U32 g_bInitRotate;
extern MI_S32 g_fdParam;
extern MI_S32 g_hchdParam;
extern MI_S32 g_mdParam;
extern MI_S32 g_odParam;
extern MI_S32 g_vgParam;
extern MI_S32 g_dlaParam;
extern int g_ieFrameInterval;
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];
extern const char * g_ChipType;
extern MI_U32 g_videoNumber;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
MI_S32 g_MD_InitDone = FALSE;
int g_IE_Open = 0;
static int g_IE_MD_Open = 0;
static int g_IE_OD_Open = 0;
static int g_IE_FD_Open = 0;
static int g_IE_VG_Open = 0;
static int g_IE_DLA_Open = 0;
static int g_IE_HCHD_Open = 0;

//static MI_S32 IeViChn  = 0;
//static MI_S32 IeWidth  = RAW_W;
//static MI_S32 IeHeight = RAW_H;
#if TARGET_CHIP_I6B0
static MI_U32 g_VpePort = VPE_REALMODE_SUB_PORT;
#else
static MI_U32 g_VpePort = g_ieVpePort;
#endif
#define DIV_W_FULL_REGION   10
#define DIV_H_FULL_REGION   12
#define DIV_W_MULTI_REGION  1
#define DIV_H_MULTI_REGION  1
#define Region_Num_MAX      8
#define Region_Num          8

extern int mid_md_Initial(int param);
extern int mid_md_Uninitial(void);
extern int mid_od_Initial(int param);
extern int mid_od_Uninitial(void);
extern int mid_fdfr_Initial(int param);
extern int mid_fdfr_Uninitial(void);
extern int mid_vg_Initial(int param);
extern int mid_vg_Uninitial(void);
extern int mid_dla_Initial(int param);
extern int mid_dla_Uninitial(void);
extern int mid_dla_SetParam(const MI_S8* param);
extern int mid_dla_SetInitInfo(const MI_S8* param);
extern int mid_hchd_Initial(int param);
extern int mid_hchd_Uninitial(void);
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];


static int ie_vpe_divp_init(MI_S32 *ieParm)
{
    MI_SYS_ChnPort_t stChnPort;
    Mixer_VPE_PortInfo_T stVpePortInfo;
    MI_VPE_CHANNEL VpeCh = 0;

    if((g_bInitRotate & 0xFF000000)&0xffff)
    {
    #if TARGET_CHIP_I5
        VpeCh = 1;
    #endif
    }
    if(ieParm)
    {
      MIXER_DBG("ieParm[0]=%d ieParm[1]=%d ieParm[2]=%d\n",ieParm[0],ieParm[1],ieParm[2]);
    }
	else
	{
	  MIXER_DBG("ie_vpe_divp_init parm is NULL\n");
	  return -1;
	}
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId  = 0;
    stChnPort.u32ChnId  = VpeCh;
    stChnPort.u32PortId = ieParm[0];

    int i = 0;
    BOOL ieVpePortEn = FALSE;

    for(i=0; i<MAX_VIDEO_NUMBER; i++)
    {
      if(g_videoEncoderArray[i] == NULL)
      {
            continue;
      }
      if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == (MI_U32)ieParm[0])
      {
        ieVpePortEn = TRUE;
        break;
      }
    }

    if(FALSE == ieVpePortEn) // not enable vpe
    {
        MIXER_DBG("The IE port(ChnId=%d, PortId=%d) of VPE is will init!\n", VpeCh, ieParm[0]);
        memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpePortInfo.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
#if TARGET_CHIP_I6B0
        stVpePortInfo.u16OutputHeight = ieParm[2];//g_ieHeight;
        stVpePortInfo.u16OutputWidth  = ieParm[1];//g_ieWidth;
        stVpePortInfo.u16VpeOutputHeight = ieParm[2];//g_ieHeight;
        stVpePortInfo.u16VpeOutputWidth = ieParm[1];//g_ieWidth;
#else
         MI_VPE_PortMode_t stVpeMode;
         memset(&stVpeMode, 0x00, sizeof(stVpeMode));
         ExecFunc(MI_VPE_GetPortMode(VpeCh, ieParm[0], &stVpeMode), MI_VPE_OK);
         stVpePortInfo.u16OutputHeight =  1080;//stVpeMode.u16Height;
         stVpePortInfo.u16OutputWidth  = 1920;//stVpeMode.u16Width;
         stVpePortInfo.u16VpeOutputHeight = 1080;//stVpeMode.u16Height;
         stVpePortInfo.u16VpeOutputWidth = 1920;//stVpeMode.u16Width;
#endif
        stVpePortInfo.VpeChnPort.u32ChnId  = VpeCh;
        stVpePortInfo.VpeChnPort.u32PortId = ieParm[0];

        Mixer_Vpe_StartPort(VpeCh, &stVpePortInfo);
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, g_videoEncoderArray[0]->m_vpeBufCntQuota));
    }
    else
    {
        MIXER_DBG("the IE port(ChnId=%d, PortId=%d) of VPE has been initialed!\n", VpeCh, ieParm[0]);
    }
#if TARGET_CHIP_I6B0
    ieVpePortEn = FALSE;
    for(i=0; i<MAX_VIDEO_NUMBER; i++)
    {
      if(g_videoEncoderArray[i] == NULL)
      {
            continue;
      }
      if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == 2 ) //I6B0:port 3 res is actually set by port 2.
      {
        ieVpePortEn = TRUE;
        break;
      }
    }
    if(FALSE == ieVpePortEn)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpeMode.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpeMode.u16Width = ieParm[1];
        stVpeMode.u16Height = ieParm[2];
        ExecFunc(MI_VPE_SetPortMode(VpeCh, 2 , &stVpeMode), MI_VPE_OK);
    }
#endif

    gDivpChn4Vdf = Mixer_Divp_GetChannleNum();

    MI_VPE_PortMode_t stVpeMode;
    MI_DIVP_CHN stDivpChn = MIXER_DIVP_CHNID_FOR_VDF;

    #if TARGET_CHIP_I5
    if(g_dlaParam)
    {
       stDivpChn = MIXER_DIVP_CHNID_FOR_DLA;
    }
    #endif

    // create divp
    MI_SYS_WindowRect_t stCropRect;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    MI_SYS_ChnPort_t destChnPort;

    memset(&stDivpPortAttr, 0x00, sizeof(MI_DIVP_OutputPortAttr_t));
    memset(&stCropRect, 0x00, sizeof(stCropRect));
    stCropRect.u16X = 0;
    stCropRect.u16Y = 0;
    ExecFunc(MI_VPE_GetPortMode(VpeCh, ieParm[0], &stVpeMode), MI_VPE_OK);
	MI_U32 Rotate = g_bInitRotate & 0xFFFF;

#if TARGET_CHIP_I6B0 || TARGET_CHIP_I6E
    stDivpPortAttr.u32Width  = ieParm[1];
    stDivpPortAttr.u32Height = ieParm[2];
#endif
    stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
#if TARGET_CHIP_I6E
    MIXER_DBG("stDivpChn[%d] Rotate=%d g_ieWidth=%d g_ieHeight=%d stVpeMode.u16Width=%d,stVpeMode.u16Height=%d\n",stDivpChn,Rotate,g_ieWidth,g_ieHeight,stVpeMode.u16Width,stVpeMode.u16Height);
    if((ieParm[1]*6 < stVpeMode.u16Width) || (ieParm[2]*4 < stVpeMode.u16Height ))
    {
        MIXER_DBG("Because of  divp width of in/out is large than 6 or divp height of in/out is large than 4,ie width&heigth[%d-%d] change to[%d-%d] \n",ieParm[1],ieParm[2],stVpeMode.u16Width/6,stVpeMode.u16Height/4);
		g_ieWidth = ALIGN_2xUP(stVpeMode.u16Width/6);
        g_ieHeight = ALIGN_2xUP(stVpeMode.u16Height/4);

        stDivpPortAttr.u32Width  = g_ieWidth;
        stDivpPortAttr.u32Height = g_ieHeight;
    }
#endif
    #if TARGET_CHIP_I5
    if(g_dlaParam == 1)
        stDivpPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_ABGR8888;
    else
    #endif
        stDivpPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(stDivpChn, E_MI_SYS_ROTATE_NONE, &stCropRect));
#if TARGET_CHIP_I5
    if(g_dlaParam == 1)
    {
        MIXERCHECKRESULT(MI_SYS_SetChnMMAConf(E_MI_MODULE_ID_DIVP, 0, stDivpChn, (MI_U8 *)"mma_heap_name1"));
    }
#endif
    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(stDivpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(stDivpChn));

    destChnPort.eModId   = E_MI_MODULE_ID_DIVP;
    destChnPort.u32ChnId = stDivpChn;
    destChnPort.u32DevId = 0;
    destChnPort.u32PortId = 0;
#if TARGET_CHIP_I6E	|| TARGET_CHIP_I5
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&destChnPort, 2, g_videoEncoderArray[0]->m_divpBufCntQuota>2?(g_videoEncoderArray[0]->m_divpBufCntQuota):2), MI_SUCCESS);
#else
	ExecFunc(MI_SYS_SetChnOutputPortDepth(&destChnPort, 2, g_videoEncoderArray[0]->m_divpBufCntQuota>2?(g_videoEncoderArray[0]->m_divpBufCntQuota+1):3), MI_SUCCESS);
#endif
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
#if TARGET_CHIP_I6
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#elif TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#elif TARGET_CHIP_I6B0
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
    stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;

    #if TARGET_CHIP_I5
    if(g_fdParam || g_dlaParam || g_hchdParam)
    #else
     if(g_fdParam || g_hchdParam)
    #endif
    {
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;    //framerate control by vdf or hchd
    }
    else
    {
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
    }
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    return 0;
}

static int ie_vpe_divp_uninit()
{
    MI_VPE_CHANNEL VpeCh = 0;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_DIVP_CHN stDivpChn = MIXER_DIVP_CHNID_FOR_VDF;
    MI_U8 i = 0;

#if TARGET_CHIP_I5
    if(g_dlaParam)
        stDivpChn = MIXER_DIVP_CHNID_FOR_DLA;
#endif

    if(g_bInitRotate & 0xFF000000)
    {
    #if TARGET_CHIP_I5
        VpeCh = 1;
    #endif
    }

    //unbind vpe->divp
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = VpeCh;
    stBindInfo.stSrcChnPort.u32PortId = g_VpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = stDivpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = g_videoEncoderArray[0]->m_vencframeRate;

    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    MIXERCHECKRESULT(Mixer_Divp_StopChn(stDivpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(stDivpChn));

    Mixer_Divp_PutChannleNum(stDivpChn);

    //
    for(i = 0; i < g_videoNumber; i++)
    {
      if(stBindInfo.stSrcChnPort.u32PortId == g_videoParam[i].stVpeChnPort.u32PortId)
      {
        break;
      }
    }
    if(i >= g_videoNumber)
    {
          if(MI_SUCCESS != MI_VPE_DisablePort(VpeCh, g_VpePort)) //port2可能除了绑定vdf外还可能绑定venc，所以需要做判断，否则会影响其他路正常工�?
          {
            MIXER_ERR("can not disable vpe port.\n");
        }
    }
    return 0;
}
int ie_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    MI_S32 ieParam[3];
    memset(ieParam,0x00,sizeof(ieParam));
    switch(id)
    {
        case CMD_IE_OPEN:
            {
                memcpy(ieParam, param, paramLen);
             //   if(g_IE_Open == 0)
                {
                    //g_VpePort = ieParam[0];
                    //IeWidth   = ieParam[1];
                    //IeHeight  = ieParam[2];

                    ie_vpe_divp_init(ieParam);
                    printf("CMD_IE_OPEN vpe port=%d\n", g_ieVpePort);
              //      if(g_mdParam || g_odParam || g_vgParam)
                    {
                        MI_VDF_Init();
                    }
                    g_IE_Open = 1;
                }
            }
            break;

        case CMD_IE_CLOSE:
            if(g_IE_Open && g_IE_MD_Open == 0 && g_IE_OD_Open == 0 && g_IE_FD_Open == 0 && g_IE_VG_Open == 0 && g_IE_DLA_Open == 0)
            {
           //     if(g_mdParam || g_odParam || g_vgParam)
                {
                    printf("CMD_IE_CLOSE vpe port=%d\n", g_ieVpePort);
                    MI_VDF_Uninit();
                }
                ie_vpe_divp_uninit();
                g_IE_Open = 0;
             }
            break;

        case CMD_IE_MD_OPEN:
            if(g_IE_Open && g_IE_MD_Open == 0)
            {
                memcpy(ieParam,param,paramLen);
                if(0 == mid_md_Initial(ieParam[0]))
                {
                    g_MD_InitDone = TRUE;
                    g_IE_MD_Open = 1;
                }
            }
            break;

        case CMD_IE_OD_OPEN:
            if(g_IE_Open && g_IE_OD_Open == 0)
            {
                if(0 == mid_od_Initial(1))
                    g_IE_OD_Open = 1;
            }
            break;

        case CMD_IE_MD_CLOSE:
            if(g_IE_Open && g_IE_MD_Open)
            {
                mid_md_Uninitial();
                g_MD_InitDone = FALSE;
                g_IE_MD_Open = 0;
            }
            break;

        case CMD_IE_OD_CLOSE:
            if(g_IE_Open && g_IE_OD_Open)
            {
                mid_od_Uninitial();
                g_IE_OD_Open = 0;
            }
            break;

        case CMD_IE_MD_CHANGE:
            if(g_IE_Open && g_IE_MD_Open)
            {
                mid_md_Param_Change(param, paramLen);
            }
            break;
        case CMD_IE_FD_OPEN:
            if(g_IE_Open && g_IE_FD_Open == 0)
            {
#ifndef BUILD_UCLIBC_PROG
                #if TARGET_CHIP_I5
                        mid_fdfr_Initial(*param);
                #endif
#else
                #if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                mid_hchd_Initial(3);
                #endif
#endif

                g_IE_FD_Open = 1;
            }
            break;
        case CMD_IE_FD_CLOSE:
            if(g_IE_FD_Open)
            {
#ifndef BUILD_UCLIBC_PROG
            #if TARGET_CHIP_I5
                mid_fdfr_Uninitial();
            #endif
#else
            #if    TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                 mid_hchd_Uninitial();
            #endif
#endif
                g_IE_FD_Open = 0;
            }
            break;
        case CMD_IE_FD_CHANGE:
            if(g_IE_FD_Open)
            {

            }
            break;
        case CMD_IE_HCHD_OPEN:
            if(g_IE_Open && 0 == g_IE_HCHD_Open)
            {
#if    TARGET_CHIP_I6 ||TARGET_CHIP_I6E || TARGET_CHIP_I6B0
              mid_hchd_Initial(*param);
            //  g_IE_Open = 0;
              g_IE_HCHD_Open = 1;
#endif
            }
            break;
        case CMD_IE_HCHD_CLOSE:
            if(1 == g_IE_HCHD_Open)
            {
#if    TARGET_CHIP_I6 ||TARGET_CHIP_I6E || TARGET_CHIP_I6B0
              mid_hchd_Uninitial();
              g_IE_HCHD_Open = 0;
#endif
            }
            break;
        case CMD_IE_VG_OPEN:
            if(g_IE_Open && g_IE_VG_Open == 0)
            {
                if(0 == mid_vg_Initial(*param))
                    g_IE_VG_Open = 1;
            }
            break;
        case CMD_IE_VG_CLOSE:
            if(g_IE_Open && g_IE_VG_Open)
            {
                mid_vg_Uninitial();
                g_IE_VG_Open = 0;
            }
            break;

        case CMD_IE_DLA_OPEN:
            if((0x0 != memcmp(g_ChipType, "i5", 2)) && (0x0 != memcmp(g_ChipType, "i6e", 3)))
            {
                printf("Only I5/I6E support DLA!!!\n");
                break;
            }
            if(g_IE_Open && g_IE_DLA_Open == 0)
            {
                #if TARGET_CHIP_I5 || TARGET_CHIP_I6E
                mid_dla_Initial(*param);
                #endif
                g_IE_DLA_Open = 1;
            }
            break;

        case CMD_IE_DLA_CLOSE:
            if(g_IE_Open && g_IE_DLA_Open)
            {
                #if TARGET_CHIP_I5 || TARGET_CHIP_I6E
                mid_dla_Uninitial();
                #endif
                g_IE_DLA_Open = 0;
            }
            break;

        case CMD_IE_DLA_SETPARAM:
            if(g_IE_Open && g_IE_DLA_Open)
            {
                #if TARGET_CHIP_I6E
                    mid_dla_SetParam(param);
                #endif
            }
            break;

        case CMD_IE_DLA_SETINITINFO:
            if(g_IE_Open && g_IE_DLA_Open)
            {
                #if TARGET_CHIP_I6E
                    mid_dla_SetInitInfo(param);
                #endif
            }
            break;

        default:
            break;
    }

    return 0;
}

