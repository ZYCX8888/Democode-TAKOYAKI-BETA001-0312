/*
* module_video.cpp- Sigmastar
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
#include <sys/prctl.h>

#include "module_common.h"
#include "mid_utils.h"
#include "mi_venc.h"
#include "mid_vif.h"
#include "mid_VideoEncoder.h"
#include "mid_common.h"
//#include "mi_rgn.h"
#if TARGET_CHIP_I5
#include "mi_isp_pretzel.h"
#include "mi_isp_pretzel_datatype.h"
#elif TARGET_CHIP_I6
#include "mid_sys.h"
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6E
#include "mid_sys.h"
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6B0
#include "mid_sys.h"
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#endif
#include "mid_system_config.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"


#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#if LOAD_MIXER_CFG
#include "module_private_memory_pool.h"
#endif
#endif
extern MI_S32 g_EnablePIP;
extern MI_S32 gDebug_osdColorInverse;
extern MI_U32 g_videoNumber;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
extern MI_S32 g_openIQServer;
extern MI_S32 g_ieWidth;
extern MI_S32 g_ieHeight;
extern MI_U8 g_u8SnrCurResIdx;

#if TARGET_CHIP_I6E
static MI_BOOL gbLdcEnable = FALSE;
static char gszLdcCfgBinPath[MIXER_LDC_CFG_BIN_PATH_LENGTH] = {0};
#endif

MI_U32 g_bInitRotate = 0;
//MI_S32 g_s32DivpChnIndex = MIXER_DIVP_CHNID_FOR_VDISP + 1;
MI_S32 g_s32DivpChnIndex = MIXER_DIVP_CHNID_FOR_VDISP;
VencRcParamEx_t g_videoRcParam[MAX_VIDEO_NUMBER];
MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];

MI_S32 videoSetFps2RcParam(MI_U32 fps, VencRcParamEx_t *pRcParam);
MI_S32 videoSetGop2RcParam(MI_U32 gop, VencRcParamEx_t *pRcParam);
ViChnStatus _s32ToViChnStatus(MI_S32 num);
int video_SetRotate(MI_S8 *param, MI_U32 paramLen);
int video_SetRotateV2(MI_S8 *param, MI_U32 paramLen);

extern MI_S32 videoCreateSnrVif(VideoParam_t *pstVideoParam);

#if TARGET_CHIP_I6E
void mixerSetLdcStatus(MI_BOOL bEnable)
{
    gbLdcEnable = bEnable;
}

MI_BOOL mixerGetLdcStatus()
{
    return gbLdcEnable;
}

MI_BOOL mixerSetLdcBinPath(char *path)
{
    if(path == NULL)
    {
        MIXER_ERR("path is NULL!\n");
        return FALSE;
    }

    int len = 0;
    len = strlen(path);
    if (len > MIXER_LDC_CFG_BIN_PATH_LENGTH)
    {
        MIXER_ERR("path length(%d) is exceed %d!\n", len, MIXER_LDC_CFG_BIN_PATH_LENGTH);
    }

    strcpy(gszLdcCfgBinPath, path);

    return TRUE;
}

char *mixerGetLdcBinPath(void)
{
    return gszLdcCfgBinPath;
}
#endif
#if TARGET_CHIP_I6B0
MI_BOOL checkLastRealDivp(MI_U32 chn)
{
    MI_S32 i;
    MI_BOOL ret = TRUE;
    for(i=0;i<g_videoNumber;i++)
    {
        if(i==chn)continue;
        if(g_videoEncoderArray[i]!=NULL && g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && g_videoEncoderArray[i]->m_bDivpInit == TRUE)
        {
            ret = FALSE;
            break;
        }
    }
    return ret;
}
#endif
#if TARGET_CHIP_I6  || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
int videoSetHDR(Mixer_HDR_Mode_E hdr_flag)
{
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_VIF_DEV vifDev = 0;
    MI_VIF_CHN vifChn = 0;
    MI_VIF_PORT vifPort = 0;
    MI_U8 u8ResIdx =0;
    MI_SNR_Res_t stRes;
    MI_SNR_Res_t stHdrRes;
    MI_U32 stHdrFps;
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_U32 u32CurFps = 0;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    Mixer_VIF_PortInfo_T stVifPortInfo;
    MI_VPE_ChannelPara_t stVpeChParam;
    MI_U64 lastFramePTS = 0;
    MI_U64 currentPTS = 0;
	MI_U32 u32SnrResCount = 0;
    struct timeval curTimestamp;
    memset(&curTimestamp,0x00,sizeof(struct timeval));
    gettimeofday(&curTimestamp, NULL);
    currentPTS = (curTimestamp.tv_sec * 1000000LL) + curTimestamp.tv_usec;
    MIXER_DBG("%s, %lu.%03ld, currentPTS<%llu>\r\n", __func__, curTimestamp.tv_sec, curTimestamp.tv_usec, currentPTS);
    MIXERCHECKRESULT(Mixer_Vpe_StopChannel(0));
    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = vifDev;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = vifPort;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = SENSOR_FPS_UP2_INT(MI_VideoEncoder::vifframeRate);
    stBindInfo.u32DstFrmrate = SENSOR_FPS_UP2_INT(MI_VideoEncoder::vifframeRate);
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    /************************************************
    Step2:  disable VIF
    *************************************************/
    MIXERCHECKRESULT(Mixer_Vif_StopPort(vifChn, vifPort));
    MIXERCHECKRESULT(Mixer_Vif_DisableDev(vifDev));
    /************************************************
    Step3:  change SNR
    *************************************************/
    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    ExecFunc(MI_SNR_GetCurRes(eSnrPad, &u8ResIdx, &stRes), MI_VPE_OK);
    ExecFunc(MI_SNR_GetFps(eSnrPad, &u32CurFps), MI_VPE_OK);
    MI_SNR_Disable(eSnrPad);
    if(hdr_flag == Mixer_HDR_TYPE_OFF){
        MI_SNR_SetPlaneMode(eSnrPad, FALSE);
    }
    else {
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }
    printf("current use resolution idx=%d w=%d h=%d fps=%d \n",u8ResIdx,stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,u32CurFps);
    ExecFunc(MI_SNR_QueryResCount(eSnrPad, &u32SnrResCount), MI_SUCCESS);
    /* To find HDR resolution which matched Linear resolution*/
    for(u8ResIdx=0; u8ResIdx<u32SnrResCount; ++u8ResIdx)
    {
        printf("test res %d\n", u8ResIdx);
        if(MI_SNR_GetRes(eSnrPad, u8ResIdx, &stHdrRes)!=MI_SUCCESS)
        {
            printf("[%s] failed to change resolution.\n", __FUNCTION__);
            u8ResIdx = 0;
            break;
        }

        printf("%s:%d index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, curfps %d, ResDesc %s\n", __func__, __LINE__,
                              u8ResIdx,  stHdrRes.stCropRect.u16X, stHdrRes.stCropRect.u16Y,
                              stHdrRes.stCropRect.u16Width, stHdrRes.stCropRect.u16Height,
                              stHdrRes.stOutputSize.u16Width, stHdrRes.stOutputSize.u16Height,
                              stHdrRes.u32MaxFps, stHdrRes.u32MinFps, u32CurFps, stHdrRes.strResDesc);

        if( (stRes.stCropRect.u16Width == stHdrRes.stCropRect.u16Width) && (stRes.stCropRect.u16Height == stHdrRes.stCropRect.u16Height) && (u32CurFps <= stHdrRes.u32MaxFps))
        {
            break;
        }
    }

    //u8ResIdx = 2;
  //  printf("******** Change ResID to %d for HDR mode *********\n", u8ResIdx);

    ExecFunc(MI_SNR_SetRes(eSnrPad,u8ResIdx), MI_SUCCESS);
    ExecFunc(MI_SNR_GetCurRes(eSnrPad, &u8ResIdx, &stRes), MI_VPE_OK);
	g_u8SnrCurResIdx = u8ResIdx;
    //ExecFunc(MI_SNR_GetFps(eSnrPad, &u32CurFps), MI_VPE_OK);
 //   printf("HDR resolution idx=%d w=%d h=%d fps=%d \n",u8ResIdx,stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,u32CurFps);

    ExecFunc(MI_SNR_Enable(eSnrPad), MI_SUCCESS);
  //  ExecFunc(MI_SNR_SetRes(eSnrPad,u8ResIdx), MI_SUCCESS);

    stHdrFps = SENSOR_FPS_UP2_INT(u32CurFps);
    if(stHdrFps > stHdrRes.u32MaxFps)
        stHdrFps = stHdrRes.u32MaxFps;

    if(stHdrFps < stHdrRes.u32MinFps)
        stHdrFps = stHdrRes.u32MinFps;
    ExecFunc(MI_SNR_SetFps(eSnrPad, stHdrFps), MI_SUCCESS);

    ExecFunc(MI_SNR_GetPadInfo(eSnrPad, &stPad0Info), MI_SUCCESS);
    ExecFunc(MI_SNR_GetPlaneInfo(eSnrPad, 0, &stSnrPlane0Info), MI_SUCCESS);
    /************************************************
    Step4:  create VIF and vif port
    *************************************************/
    memset(&stVifPortInfo, 0x00, sizeof(Mixer_VIF_PortInfo_T));
    MIXERCHECKRESULT(Mixer_Vif_EnableDev(vifDev, (MI_VIF_HDRType_e)hdr_flag, &stPad0Info));
    stVifPortInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
    stVifPortInfo.s32FrameRate  = E_MI_VIF_FRAMERATE_FULL;
    stVifPortInfo.u32RectWidth  = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.u32RectHeight = stSnrPlane0Info.stCapRect.u16Height;
    stVifPortInfo.u32DestWidth  = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.u32DestHeight = stSnrPlane0Info.stCapRect.u16Height;
    stVifPortInfo.ePixFormat    = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    MIXERCHECKRESULT(Mixer_Vif_CreatePort(vifChn, vifPort, &stVifPortInfo));

    //request idr
    for(MI_U32 i = 0; i < g_videoNumber; i++)
    {
        if(g_videoEncoderArray[i])
        {
            g_videoEncoderArray[i]->requestIDR();
        }
    }

    /************************************************
    Step5:  init VPE (create one VPE)
    *************************************************/
    memset(&stVpeChParam, 0x0, sizeof(MI_VPE_ChannelPara_t));
    MIXERCHECKRESULT(MI_VPE_GetChannelParam(0, &stVpeChParam));
    stVpeChParam.eHDRType = (MI_VPE_HDRType_e)hdr_flag;
    MIXERCHECKRESULT(MI_VPE_SetChannelParam(0, &stVpeChParam));
    MIXERCHECKRESULT(Mixer_Vpe_StartChannel(0));

    MIXERCHECKRESULT(Mixer_Vif_StartPort(vifDev,vifChn,vifPort));
    /************************************************
    Step6:  bind vif and vpe
    *************************************************/
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = SENSOR_FPS_UP2_INT(u32CurFps);
    stBindInfo.u32DstFrmrate = SENSOR_FPS_UP2_INT(u32CurFps);
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    MI_VideoEncoder::eHDRType = (MI_VPE_HDRType_e)hdr_flag;

    gettimeofday(&curTimestamp, NULL);
    lastFramePTS = (curTimestamp.tv_sec * 1000000LL) + curTimestamp.tv_usec;
    MIXER_DBG("%s, %lu.%03ld, lastFramePTS<%llu>\r\n", __func__, curTimestamp.tv_sec, curTimestamp.tv_usec,lastFramePTS);
    MIXER_DBG("videoSetHDR spend time is %llu\n",lastFramePTS-currentPTS);
    return 0;
}
#endif

int videoDisableSnrVifVpe(VideoParam_t *pstVideoParam)
{
    Mixer_Sys_BindInfo_T stBindInfo;
    MixerCmdId id = pstVideoParam->id;
    MI_U32 u32NewWidth = pstVideoParam->u32VideoWidth;
    MI_U32 u32NewHeight = pstVideoParam->u32VideoHeight;
    MI_U32 u32CompareW = 0;
    MI_U32 u32CompareH = 0;
    MI_U32 tmp = 0;
    MI_U32 RotaState = (g_bInitRotate & 0xFFFF)%360;
    MI_SNR_Res_t stSensorCurRes;
    /************************************************
     Step1:  stop venc  Step2:  stop divp
    *************************************************/
    if(CMD_VIDEO_SET_SENSOR_RESOLUTION == id)
    {
      u32CompareW = u32NewWidth;
      u32CompareH = u32NewHeight;
    }
    else if(CMD_VIDEO_SET_RESOLUTION == id)
    {
      memset(&stSensorCurRes,0,sizeof(MI_SNR_Res_t));
      ExecFunc(MI_SNR_GetRes(pstVideoParam->sensorPad, pstVideoParam->u32SnrRes, &stSensorCurRes), MI_SUCCESS);
      u32CompareW = stSensorCurRes.stCropRect.u16Width;
      u32CompareH = stSensorCurRes.stCropRect.u16Height;
    }
    if(90 == RotaState || 270 == RotaState)
    {
       tmp = u32CompareW;
       u32CompareW = u32CompareH;
       u32CompareH = tmp;
    }

    for(MI_U32 i = 0; (i < g_videoNumber)&&(NULL != g_videoEncoderArray[i]); i++)
    {
        if((CMD_VIDEO_SET_SENSOR_RESOLUTION == id) || //CMD_VIDEO_SET_SENSOR_RESOLUTION must be reinit modules.
            (g_videoEncoderArray[i]->m_bChangeRes))
        {
            g_videoEncoderArray[i]->stopVideoEncoder();
            g_videoEncoderArray[i]->uninitVideo(MIXER_SYS_INPUT_VPE);
        }
        else
        {
#if TARGET_CHIP_I5
            g_videoEncoderArray[i]->uninitVideo(MIXER_SYS_INPUT_MAX);
#endif
        }
    }

    /************************************************
     Step3:  unbind VIF->VPE
    *************************************************/
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[0]->VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[0]->VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[0]->VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[0]->VifChnPort.u32PortId;
    stBindInfo.u32SrcFrmrate = g_videoEncoderArray[0]->vifframeRate;

    stBindInfo.stDstChnPort.eModId = g_videoEncoderArray[0]->VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[0]->VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[0]->VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->VpeChnPortTop.u32PortId;
    stBindInfo.u32DstFrmrate = g_videoEncoderArray[0]->vpeframeRate;

    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
#if TARGET_CHIP_I5
   if((90 == RotaState || 270 == RotaState) && (MIXER_SYS_INPUT_VPE == state))
   {
     if(RotaState)
     {
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.stDstChnPort.eModId = MI_VideoEncoder::VpeChnPortBottom.eModId;
        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortBottom.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortBottom.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortBottom.u32PortId;
        MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
       //disenable VPE TOP/BOTTOM mode
        if((CMD_VIDEO_SET_SENSOR_RESOLUTION != id) && (CMD_VIDEO_SET_RESOLUTION != id))
        {
           ExecFunc(MI_VPE_DisablePort(MI_VideoEncoder::VpeChnPortTop.u32ChnId,
                                    MI_VideoEncoder::VpeChnPortTop.u32PortId), MI_VPE_OK);
        }
        if(RotaState)    //enable VPE TOP/BOTTOM mode
        {
            ExecFunc(MI_VPE_StopChannel(MI_VideoEncoder::VpeChnPortTop.u32ChnId), MI_VPE_OK);
            ExecFunc(MI_VPE_DestroyChannel(MI_VideoEncoder::VpeChnPortTop.u32ChnId), MI_VPE_OK);
        }
        //when in realtime mode, VpeChnPortTop&VpeChnPortBottom have the same data
     }
       ExecFunc(MI_VPE_StopChannel(MI_VideoEncoder::VpeChnPortBottom.u32ChnId), MI_VPE_OK);
     ExecFunc(MI_VPE_DestroyChannel(MI_VideoEncoder::VpeChnPortBottom.u32ChnId), MI_VPE_OK);
    }
#endif
    ExecFunc(MI_VPE_StopChannel(MI_VideoEncoder::VpeChnPortTop.u32ChnId), MI_VPE_OK);
    for(MI_U32 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
        ExecFunc(MI_VIF_DisableChnPort(i, 0), MI_SUCCESS);
    }

    ExecFunc(MI_VIF_DisableDev(MI_VideoEncoder::VifChnPort.u32DevId), MI_SUCCESS);
    ExecFunc(MI_SNR_Disable(pstVideoParam->sensorPad), MI_SUCCESS);

    return 0;
}


int videoRecreateSnrVifVpe(VideoParam_t *pstVideoParam)
{
#if TARGET_CHIP_I5
    MI_U32 u32SnrOutWidth = 0;
    MI_U32 u32SnrOutHeight = 0;
    Mixer_VPE_ChannelInfo_T stVpeChannelInfo;
#endif
    MI_SYS_ChnPort_t stChnPort;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SNR_Res_t stSnrRes;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    Mixer_VPE_PortInfo_T stVpePortInfo;

    MixerCmdId id = pstVideoParam->id;
    MI_U32 u32NewWidth  = pstVideoParam->u32VideoWidth;
    MI_U32 u32NewHeight = pstVideoParam->u32VideoHeight;
    MI_U32 u32CompareH = 0;
    MI_U32 u32CompareW = 0;
    MI_U32 RotaState = (g_bInitRotate & 0xFFFF)%360;
    MI_U32 tmp = 0;

    memset(&stSnrPlane0Info, 0x00, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
    memset(&stChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));

    ExecFunc(MI_SNR_GetPlaneInfo(pstVideoParam->sensorPad, 0, &stSnrPlane0Info), MI_SUCCESS);

    ExecFunc(MI_SNR_GetRes(pstVideoParam->sensorPad, g_u8SnrCurResIdx, &stSnrRes), MI_SUCCESS);
    printf("Get Current Sensor resolution: index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                              g_u8SnrCurResIdx,  stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                              stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                              stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                              stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);
     MI_SNR_GetCurRes(pstVideoParam->sensorPad, &g_u8SnrCurResIdx, &stSnrRes);

    MI_VideoEncoder::vifframeRate = stSnrRes.u32MaxFps;
    MI_VideoEncoder::vpeframeRate = stSnrRes.u32MaxFps;

#if TARGET_CHIP_I5
    u32SnrOutWidth  = stSnrPlane0Info.stCapRect.u16Width;
    u32SnrOutHeight = stSnrPlane0Info.stCapRect.u16Height;
#endif
    if(CMD_VIDEO_SET_SENSOR_RESOLUTION == id)
    {
      u32CompareW = u32NewWidth;
      u32CompareH = u32NewHeight;
    }
    else if(CMD_VIDEO_SET_RESOLUTION == id)
    {
      u32CompareW = stSnrPlane0Info.stCapRect.u16Width;
      u32CompareH = stSnrPlane0Info.stCapRect.u16Height;
    }

    if(CMD_VIDEO_SWITCH_HDR_LINAER_MODE == id)
    {
      MI_SNR_PADInfo_t stPad0Info;
      memset(&stPad0Info, 0x00, sizeof(MI_SNR_PADInfo_t));
      ExecFunc(MI_SNR_GetPadInfo(pstVideoParam->sensorPad, &stPad0Info), MI_SUCCESS);
      Mixer_Vif_EnableDev(0,(MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType, &stPad0Info);
    }
    else
    {
          ExecFunc(MI_VIF_EnableDev(MI_VideoEncoder::VifChnPort.u32DevId), MI_SUCCESS);
    }
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
       ExecFunc(MI_VIF_EnableChnPort(i,  MI_VideoEncoder::VifChnPort.u32PortId), MI_SUCCESS);
    }
    if(CMD_VIDEO_SWITCH_HDR_LINAER_MODE == id)
    {
        MI_VPE_ChannelPara_t stVpeChParam;
        memset(&stVpeChParam,0x00,sizeof(MI_VPE_ChannelPara_t));
        ExecFunc(MI_VPE_GetChannelParam(0, &stVpeChParam),MI_SUCCESS);
        stVpeChParam.eHDRType = MI_VideoEncoder::eHDRType;
        ExecFunc(MI_VPE_SetChannelParam(0, &stVpeChParam),MI_SUCCESS);
    }
    //bind vif0--vpe0
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

    stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

    stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;
    stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif

    printf(" MI_VideoEncoder::vifframeRate=%d MI_VideoEncoder::vpeframeRate=%d\n", MI_VideoEncoder::vifframeRate ,MI_VideoEncoder::vpeframeRate);
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    if(90 == RotaState || 270 == RotaState)
    {
          tmp = u32CompareW;
        u32CompareW = u32CompareH;
        u32CompareH = tmp;
    }
    MIXERCHECKRESULT(Mixer_Vpe_StartChannel(MI_VideoEncoder::VpeChnPortTop.u32ChnId));
#if TARGET_CHIP_I5
    if(RotaState)  //enable Bottom mode
    {
        memset(&stVpeChannelInfo, 0x00, sizeof(Mixer_VPE_ChannelInfo_T));
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpePortInfo.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.u16OutputHeight = u32SnrOutHeight;
        stVpePortInfo.u16OutputWidth  = u32SnrOutWidth;
        stVpePortInfo.VpeChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stVpePortInfo.VpeChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stVpePortInfo.VpeChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stVpePortInfo.VpeChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;
        MIXERCHECKRESULT(Mixer_Vpe_StartPort(MI_VideoEncoder::VpeChnPortTop.u32ChnId, &stVpePortInfo));

        stChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;
        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort,
                                              g_videoEncoderArray[0]->m_vpeBufUsrDepth,
                                              g_videoEncoderArray[0]->m_vpeBufCntQuota), MI_SUCCESS);

        //create vpe channel_1
        stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_BOTTOM_MODE;
        stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpeChannelInfo.u16VpeCropW = 0;
        stVpeChannelInfo.u16VpeCropH = 0;
        stVpeChannelInfo.bRotation = TRUE;
        stVpeChannelInfo.u32X = 0;
        stVpeChannelInfo.u32Y = 0;
        stVpeChannelInfo.u16VpeMaxW  = stSnrPlane0Info.stCapRect.u16Width;
        stVpeChannelInfo.u16VpeMaxH  = stSnrPlane0Info.stCapRect.u16Height;
        stVpeChannelInfo.eHDRtype = MI_VideoEncoder::eHDRType;
        stVpeChannelInfo.e3DNRLevel = MI_VideoEncoder::e3DNRLevel;
        stVpeChannelInfo.eFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
         stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;

        MIXERCHECKRESULT(Mixer_Vpe_CreateChannel(MI_VideoEncoder::VpeChnPortBottom.u32ChnId, &stVpeChannelInfo));
        MIXERCHECKRESULT(Mixer_Vpe_StartChannel(MI_VideoEncoder::VpeChnPortBottom.u32ChnId));

        //bind vpe0--vpe1
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortBottom.eModId;
        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortBottom.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortBottom.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortBottom.u32PortId;

        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    }
#endif
    if((CMD_VIDEO_SET_SENSOR_RESOLUTION == id)|| (CMD_VIDEO_SET_RESOLUTION == id))
    {
        memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
        for(MI_U32 i = 0; (i < g_videoNumber)&&(NULL != g_videoEncoderArray[i]); i++)
        {
            stVpePortInfo.VpeChnPort.eModId    = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
            stVpePortInfo.VpeChnPort.u32DevId  = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
            stVpePortInfo.VpeChnPort.u32ChnId  = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
            stVpePortInfo.VpeChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;
            stVpePortInfo.eCompressMode        = E_MI_SYS_COMPRESS_MODE_NONE;
            stVpePortInfo.ePixelFormat         = g_videoEncoderArray[i]->m_eVpeOutportPixelFormat;

            if((CMD_VIDEO_SET_SENSOR_RESOLUTION == id) || //CMD_VIDEO_SET_SENSOR_RESOLUTION must be reinit modules.
                (g_videoEncoderArray[i]->m_bChangeRes))
            {
                if(CMD_VIDEO_SET_SENSOR_RESOLUTION == id)
                {
                    /* Here need to think about the case of the original vpe port output resolution less than the new resolution,
                    which don't need to change anyting! */
                    if((g_videoEncoderArray[i]->m_widthmax >= u32CompareW) || (g_videoEncoderArray[i]->m_heightmax >= u32CompareH))
                    {
                        g_videoEncoderArray[i]->m_VpeOutputWidth = u32CompareW;
                        g_videoEncoderArray[i]->m_VpeOutputHeight = u32CompareH;
                        g_videoEncoderArray[i]->m_width = u32CompareW;
                        g_videoEncoderArray[i]->m_height = u32CompareH;
                    }
                }

#if TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                if(VE_MJPEG == g_videoEncoderArray[i]->m_encoderType || VE_JPG ==  g_videoEncoderArray[i]->m_encoderType)
                {
                    g_videoEncoderArray[i]->m_height  = ALIGN_16xUP(g_videoEncoderArray[i]->m_height);
                    g_videoEncoderArray[i]->m_width = ALIGN_16xUP(g_videoEncoderArray[i]->m_width);
                    g_videoEncoderArray[i]->m_VpeOutputWidth = ALIGN_16xUP(g_videoEncoderArray[i]->m_VpeOutputWidth);
                    g_videoEncoderArray[i]->m_VpeOutputHeight = ALIGN_16xUP(g_videoEncoderArray[i]->m_VpeOutputHeight);
                }
#endif
                if(g_videoEncoderArray[i]->m_widthmax < g_videoEncoderArray[i]->m_width)
                {
                    g_videoEncoderArray[i]->m_widthmax  =   g_videoEncoderArray[i]->m_width;
                }

                if(g_videoEncoderArray[i]->m_heightmax <  g_videoEncoderArray[i]->m_height)
                {
                   g_videoEncoderArray[i]->m_heightmax =  g_videoEncoderArray[i]->m_height;
                }

                stVpePortInfo.u16OutputHeight = g_videoEncoderArray[i]->m_VpeOutputHeight;
                stVpePortInfo.u16OutputWidth  = g_videoEncoderArray[i]->m_VpeOutputWidth;
                stVpePortInfo.u16VpeOutputHeight = g_videoEncoderArray[i]->m_VpeOutputHeight;
                stVpePortInfo.u16VpeOutputWidth = g_videoEncoderArray[i]->m_VpeOutputWidth;

                if(g_videoEncoderArray[i]->m_bChangeRes)
                {
                   g_videoEncoderArray[i]->m_bChangeRes = FALSE;
                }

                g_videoEncoderArray[i]->initVideoInput(&stVpePortInfo);
                g_videoEncoderArray[i]->initVideoEncoder();
                g_videoEncoderArray[i]->startVideoEncoder();

            }
            else
            {

#if TARGET_CHIP_I5
                stVpePortInfo.u16OutputHeight      = g_videoEncoderArray[i]->m_height;
                stVpePortInfo.u16OutputWidth       = g_videoEncoderArray[i]->m_width;

                g_videoEncoderArray[i]->initDivpAndVdisp(1);
                MIXERCHECKRESULT(Mixer_Vpe_StartPort(0, &stVpePortInfo));
                g_videoEncoderArray[i]->startVideoEncoder();
#endif
            }
            if((g_EnablePIP) && (stSnrRes.u32MaxFps < g_videoEncoderArray[i]->m_vencframeRate))
            {
                g_videoEncoderArray[i]->setFrameRate(stSnrRes.u32MaxFps);
            }
            //pthread_mutex_unlock(&g_videoEncoderArray[i]->m_stopMutex);
        }
    }
    else
    {
        printf("%s:%d can't match the cmdid=0x%x\n", __func__, __LINE__, id);
        return -1;
    }
    return 0;
}


int videoOpen(MixerVideoParam *videoParam, int videoNumber)
{
    MI_U32 u32MaxW = 1920;
    MI_U32 u32MaxH = 1080;
    MI_U32 u32CurFps = MIXER_DEFAULT_FPS;
    MI_SNR_Res_t stSnrRes;
    MI_SNR_PADInfo_t stSnrPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_SYS_ChnPort_t stChnPort;
    Mixer_Sys_BindInfo_T stBindInfo;
    Mixer_VPE_PortInfo_T stVpePortInfo;
    Mixer_VPE_ChannelInfo_T stVpeChannelInfo;

    MI_VideoEncoder::eHDRType = (MI_VPE_HDRType_e)videoParam[0].stVifInfo.HdrType;
    MI_VideoEncoder::e3DNRLevel = (MI_VPE_3DNR_Level_e)videoParam[0].stVifInfo.level3DNR;

    memset(&stVpeChannelInfo, 0x00, sizeof(Mixer_VPE_ChannelInfo_T));
    memset(&stSnrPlane0Info, 0x00, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    memset(&stSnrPad0Info, 0x00, sizeof(MI_SNR_PADInfo_t));
    memset(&stChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));

    // get sensor hdr type mode
    ExecFunc(MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stSnrPad0Info), MI_SUCCESS);
    if(mixer_GetHdrValue() && (stSnrPad0Info.eHDRMode != (MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType))
    {
        printf("%s:%d Reset HDR mode = %d(user set mode = %d)\n", __func__, __LINE__,
                      (MI_U32)stSnrPad0Info.eHDRMode, (MI_U32)MI_VideoEncoder::eHDRType);
        MI_VideoEncoder::eHDRType = (MI_VPE_HDRType_e)stSnrPad0Info.eHDRMode;
    }

    ExecFunc(MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info), MI_SUCCESS);

    ExecFunc(MI_SNR_GetFps(E_MI_SNR_PAD_ID_0, &u32CurFps), MI_SUCCESS);

    ExecFunc(MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, g_u8SnrCurResIdx, &stSnrRes), MI_SUCCESS);
    printf("%s:%d index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, curfps %d, ResDesc %s\n", __func__, __LINE__,
                              g_u8SnrCurResIdx,  stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                              stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                              stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                              stSnrRes.u32MaxFps, stSnrRes.u32MinFps, u32CurFps, stSnrRes.strResDesc);



    u32MaxW = stSnrPlane0Info.stCapRect.u16Width;
    u32MaxH = stSnrPlane0Info.stCapRect.u16Height;

    if(u32CurFps <= 30)
    {
        videoParam[0].stVifInfo.vifframeRate = (MI_U32)SENSOR_FPS_UP2_INT(u32CurFps);
        videoParam[0].vpeframeRate = (MI_U32)SENSOR_FPS_UP2_INT(u32CurFps);
    }
    else if(SENSOR_FPS_UP2_INT(u32CurFps)/1000)
    {
        videoParam[0].stVifInfo.vifframeRate = (MI_U32)SENSOR_FPS_UP2_INT(u32CurFps)/1000;
        videoParam[0].vpeframeRate = (MI_U32)SENSOR_FPS_UP2_INT(u32CurFps)/1000;
    }
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeMaxW  = u32MaxW;
    stVpeChannelInfo.u16VpeMaxH  = u32MaxH;
    stVpeChannelInfo.u16VpeCropW = u32MaxW;
    stVpeChannelInfo.u16VpeCropH = u32MaxH;
    stVpeChannelInfo.eHDRtype = MI_VideoEncoder::eHDRType;     //E_MI_VPE_HDR_TYPE_OFF;
    stVpeChannelInfo.e3DNRLevel = MI_VideoEncoder::e3DNRLevel; //E_MI_VPE_3DNR_LEVEL3;
    stVpeChannelInfo.eFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    MIXER_DBG("vpe.eFormat: %d\n", stVpeChannelInfo.eFormat);
    if(1)
    {
        MI_VideoEncoder::vifframeRate = (MI_U32)videoParam[0].stVifInfo.vifframeRate;
        MI_VideoEncoder::vpeframeRate = (MI_U32)videoParam[0].vpeframeRate;
          //The global variables of the class have been initialized in mid_VideoEncoder.cpp
        MI_VideoEncoder::VifChnPort.eModId    = E_MI_MODULE_ID_VIF; //videoParam[0].stVifChnPort.eModId;
        MI_VideoEncoder::VifChnPort.u32DevId  = 0; //videoParam[0].stVifChnPort.u32DevId;
        MI_VideoEncoder::VifChnPort.u32ChnId  = 0; //videoParam[0].stVifChnPort.u32ChnId;
        MI_VideoEncoder::VifChnPort.u32PortId = 0; //videoParam[0].stVifChnPort.u32PortId;


        MI_VideoEncoder::VpeChnPortTop.eModId    = E_MI_MODULE_ID_VPE;
        MI_VideoEncoder::VpeChnPortTop.u32DevId  = 0;
        MI_VideoEncoder::VpeChnPortTop.u32ChnId  = 0;
        MI_VideoEncoder::VpeChnPortTop.u32PortId = 0;

        MI_VideoEncoder::VpeChnPortBottom.eModId    = E_MI_MODULE_ID_VPE;
        MI_VideoEncoder::VpeChnPortBottom.u32DevId  = 0;
        MI_VideoEncoder::VpeChnPortBottom.u32ChnId  = 0;
        MI_VideoEncoder::VpeChnPortBottom.u32PortId = 0;

#if TARGET_CHIP_I5
        if(g_bInitRotate & 0xFF000000)
        {
            MI_VideoEncoder::VpeChnPortBottom.u32ChnId  = 1;
        }
#endif
    }

#if TARGET_CHIP_I5
    if(g_bInitRotate & 0xFF000000)
    {
        stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_TOP_MODE;
        stVpeChannelInfo.bRotation = TRUE;

        //create vpe channel_0
        MIXERCHECKRESULT(Mixer_Vpe_CreateChannel(0, &stVpeChannelInfo));
        MIXERCHECKRESULT(Mixer_Vpe_StartChannel(0));

        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stVpePortInfo.ePixelFormat  = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpePortInfo.u16OutputHeight = u32MaxH;
        stVpePortInfo.u16OutputWidth  = u32MaxW;
        stVpePortInfo.VpeChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stVpePortInfo.VpeChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stVpePortInfo.VpeChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stVpePortInfo.VpeChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;
        MIXERCHECKRESULT(Mixer_Vpe_StartPort(0, &stVpePortInfo));

        stChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, videoParam[0].vpeBufUsrDepth, videoParam[0].vpeBufCntQuota));


        //bind vif0--vpe0
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VifChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


        //create vpe channel_1
        stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_BOTTOM_MODE;
        stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stVpeChannelInfo.u16VpeCropW = 0;
        stVpeChannelInfo.u16VpeCropH = 0;
        MIXERCHECKRESULT(Mixer_Vpe_CreateChannel(1, &stVpeChannelInfo));
        MIXERCHECKRESULT(Mixer_Vpe_StartChannel(1));

        //bind vpe0--vpe1
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortBottom.eModId;
        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortBottom.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortBottom.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortBottom.u32PortId;

        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    }
    else
#endif
    {
        stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
        stVpeChannelInfo.bRotation = FALSE;

        //create vpe channel_0
    #if TARGET_CHIP_I6E
        MI_BOOL bLdcEnable = FALSE;
		MI_SYS_Rotate_e eRotateType = E_MI_SYS_ROTATE_NONE;

        bLdcEnable = mixerGetLdcStatus();

        stVpeChannelInfo.bLdcEnable = bLdcEnable;

        MI_U32 u32ChnPortMode = 0;

        if(bLdcEnable)
        {
            u32ChnPortMode = E_MI_VPE_ZOOM_LDC_PORT0 | E_MI_VPE_ZOOM_LDC_PORT1 | E_MI_VPE_ZOOM_LDC_PORT2;

        }
        else
        {
            u32ChnPortMode = 0;
        }
        stVpeChannelInfo.u32ChnPortMode = u32ChnPortMode;

        if((g_bInitRotate & 0xFF000000) && (g_bInitRotate & 0xFFFF))
        {
            MI_U32 u32RotAnle = g_bInitRotate & 0xFFFF;

            switch(u32RotAnle)
            {
                case 0: eRotateType = E_MI_SYS_ROTATE_NONE;
                break;

                case 90: eRotateType = E_MI_SYS_ROTATE_90;
                break;

                case 180: eRotateType = E_MI_SYS_ROTATE_180;
                break;

                case 270: eRotateType = E_MI_SYS_ROTATE_270;
                break;

                default:
                    eRotateType = E_MI_SYS_ROTATE_NONE;

            }

            stVpeChannelInfo.eRotateType = eRotateType;
            MIXER_DBG("eRotateType:%d\n", (int)eRotateType);
        }
    #endif
        MIXERCHECKRESULT(Mixer_Vpe_CreateChannel(0, &stVpeChannelInfo));

   #if TARGET_CHIP_I6E
        if(bLdcEnable)
        {
            char *pLdcBinPath = NULL;
            pLdcBinPath = mixerGetLdcBinPath();

            MI_VideoEncoder::initLdc(0, pLdcBinPath);
        }
   #endif

        MIXERCHECKRESULT(Mixer_Vpe_StartChannel(0));

        //bind vif0--vpe0
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VifChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;
        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    }

#if TARGET_CHIP_I6    || TARGET_CHIP_I6E || TARGET_CHIP_I6B0

    MIXERCHECKRESULT(Mixer_Vif_StartPort(MI_VideoEncoder::VifChnPort.u32DevId,
                                         MI_VideoEncoder::VifChnPort.u32ChnId,
                                         MI_VideoEncoder::VifChnPort.u32PortId));
#endif
#if TARGET_CHIP_I6B0
    MI_VideoEncoder::u32RealDivpChn=0xff;
#endif
#if TARGET_CHIP_I6B0 || TARGET_CHIP_I6 || TARGET_CHIP_I6E
    {
        MI_U32 port2_chn = 0;
        for(int i = 0; i < videoNumber; i++)
        {
            if(videoParam[i].stVpeChnPort.u32PortId == 2)port2_chn++;
        }
        if(port2_chn>1)
            MI_VideoEncoder::bVpePort2share = TRUE;
        else
            MI_VideoEncoder::bVpePort2share = FALSE;
    }
#endif
    for(int i = 0; i < videoNumber; i++)
    {
        MI_U16 u16VideoWidth  = videoParam[i].width;
        MI_U16 u16VideoHeight = videoParam[i].height;
        MI_U32 u32MaxHeight = videoParam[i].MaxHeight;
        MI_U32 u32MaxWidth = videoParam[i].MaxWidth;
        if((0 == i) && g_openIQServer)
        {
            u16VideoWidth  = u32MaxW;
            u16VideoHeight = u32MaxH;
        }

        if(videoParam[i].encoderType == VE_YUV420 || videoParam[i].encoderType == VE_JPG_YUV422)
        {
            if(videoParam[i].FromPort >= (MAX_VPE_PORT_NUMBER -1 ))    //yuv from divp
            {
            //save yuv frame  of max size 1280*720
            if(u16VideoHeight > 720)
                u16VideoHeight = 720;
            if(u16VideoWidth > 1280)
                u16VideoWidth = 1280;
            u32MaxHeight = 720;
            u32MaxWidth = 1280;
            }
        }

        memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
#if TARGET_CHIP_I5
        if(videoParam[i].encoderType != VE_JPG)
        {
         u16VideoWidth  = ALIGN_8xUP(u16VideoWidth);
         u16VideoHeight = ALIGN_2xUP(u16VideoHeight);
          u32MaxWidth = ALIGN_8xUP(u32MaxWidth);
          u32MaxHeight = ALIGN_2xUP(u32MaxHeight);
        }
        else
        {
          u16VideoWidth = ALIGN_8xDOWN(u16VideoWidth);
          u16VideoHeight = ALIGN_2xUP(u16VideoHeight);

          u32MaxWidth = ALIGN_8xDOWN(u32MaxWidth);
          u32MaxHeight = ALIGN_2xUP(u32MaxHeight);
        }
#elif TARGET_CHIP_I6
        if(videoParam[i].encoderType != VE_JPG)
        {
          u16VideoWidth  = ALIGN_32xUP(u16VideoWidth);
          u16VideoHeight = ALIGN_2xUP(u16VideoHeight);

          u32MaxWidth = ALIGN_32xUP(u32MaxWidth);
          u32MaxHeight = ALIGN_2xUP(u32MaxHeight);
        }
        else
        {
          u16VideoWidth = ALIGN_16xDOWN(u16VideoWidth);
          u16VideoHeight = ALIGN_2xUP(u16VideoHeight);

          u32MaxWidth = ALIGN_16xDOWN(u32MaxWidth);
          u32MaxHeight = ALIGN_2xUP(u32MaxHeight);
        }

#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0

    u16VideoWidth  = ALIGN_8xUP(u16VideoWidth);
    u16VideoHeight = ALIGN_2xUP(u16VideoHeight);

    u32MaxWidth = ALIGN_8xUP(u32MaxWidth);
    u32MaxHeight = ALIGN_2xUP(u32MaxHeight);

#endif
        stVpePortInfo.u16OutputHeight = u16VideoHeight;
        stVpePortInfo.u16OutputWidth  = u16VideoWidth;

        g_videoEncoderArray[i] = MI_VideoEncoder::createNew(i);
        if(g_videoEncoderArray[i])
        {
            g_videoEncoderArray[i]->m_initRotate = E_MI_SYS_ROTATE_NONE;
            g_videoEncoderArray[i]->m_veChn  = i;
            g_videoEncoderArray[i]->m_width  = u16VideoWidth;
            g_videoEncoderArray[i]->m_height = u16VideoHeight;
            g_videoEncoderArray[i]->m_widthmax  = u32MaxWidth;
            g_videoEncoderArray[i]->m_heightmax =  u32MaxHeight;
            g_videoEncoderArray[i]->m_bitRate = videoParam[i].bitrate;

            g_videoEncoderArray[i]->m_vencframeRate = (MI_U32)videoParam[i].vencframeRate;
            g_videoEncoderArray[i]->m_vpeBufUsrDepth  = (MI_U32)videoParam[i].vpeBufUsrDepth;
            g_videoEncoderArray[i]->m_vpeBufCntQuota  = (MI_U32)videoParam[i].vpeBufCntQuota;
            g_videoEncoderArray[i]->m_vencBufUsrDepth = (MI_U32)videoParam[i].vencBufUsrDepth;
            g_videoEncoderArray[i]->m_vencBufCntQuota = (MI_U32)videoParam[i].vencBufCntQuota;
            g_videoEncoderArray[i]->m_divpBufUsrDepth = (MI_U32)videoParam[i].divpBufUsrDepth;
            g_videoEncoderArray[i]->m_divpBufCntQuota = (MI_U32)videoParam[i].divpBufCntQuota;
            g_videoEncoderArray[i]->m_encoderType = (Mixer_EncoderType_e)videoParam[i].encoderType;
            g_videoEncoderArray[i]->m_rateCtlType = (MI_U32)videoParam[i].u8RateCtlType;
            g_videoEncoderArray[i]->m_gop = videoParam[i].gop;
            g_videoEncoderArray[i]->m_maxIQp = videoParam[i].maxIQp;
            g_videoEncoderArray[i]->m_minIQp = videoParam[i].minIQp;
            g_videoEncoderArray[i]->m_maxPQp = videoParam[i].maxPQp;
            g_videoEncoderArray[i]->m_minPQp = videoParam[i].minPQp;
            g_videoEncoderArray[i]->m_IPQPDelta = videoParam[i].IPQPDelta;
            g_videoEncoderArray[i]->m_qfactor = videoParam[i].s8Qfactor;
            g_videoEncoderArray[i]->m_MaxQfactor = videoParam[i].u8MaxQfactor;
            g_videoEncoderArray[i]->m_MinQfactor = videoParam[i].u8MinQfactor;
            g_videoEncoderArray[i]->m_virtualIInterval = (videoParam[i].virtualIInterval > 0) ? videoParam[i].virtualIInterval : 0;
            g_videoEncoderArray[i]->m_virtualIEnable = videoParam[i].virtualIEnable;
#if TARGET_CHIP_I5
            g_videoEncoderArray[i]->m_eVpeOutportPixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
#elif TARGET_CHIP_I6
            g_videoEncoderArray[i]->m_eVpeOutportPixelFormat = videoParam[i].ePixelFormat;
            g_videoEncoderArray[i]->m_bindMode = videoParam[i].eBindMode;
#elif TARGET_CHIP_I6E
            g_videoEncoderArray[i]->m_eVpeOutportPixelFormat = videoParam[i].ePixelFormat;
            g_videoEncoderArray[i]->m_bindMode = videoParam[i].eBindMode;
#elif TARGET_CHIP_I6B0
            g_videoEncoderArray[i]->m_eVpeOutportPixelFormat = videoParam[i].ePixelFormat;
            g_videoEncoderArray[i]->m_bindMode = videoParam[i].eBindMode;
#endif
            g_videoEncoderArray[i]->m_viChnStatus = VI_ENABLE;
            g_videoEncoderArray[i]->m_pipCfg   = videoParam[i].pipCfg;
            g_videoEncoderArray[i]->m_pipRectX = videoParam[i].pipRectX;
            g_videoEncoderArray[i]->m_pipRectY = videoParam[i].pipRectY;
            g_videoEncoderArray[i]->m_pipRectW = videoParam[i].pipRectW;
            g_videoEncoderArray[i]->m_pipRectH = videoParam[i].pipRectH;

            g_videoEncoderArray[i]->m_bChangeRes = FALSE;

            if(videoParam[i].encoderType != VE_JPG)
            {
#if TARGET_CHIP_I5
              g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xUP(videoParam[i].u16VpeOutWidth);
#elif TARGET_CHIP_I6
              g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_32xUP(videoParam[i].u16VpeOutWidth);
#elif TARGET_CHIP_I6E
            g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xUP(videoParam[i].u16VpeOutWidth);
#elif TARGET_CHIP_I6B0
            g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xUP(videoParam[i].u16VpeOutWidth);
#endif
              g_videoEncoderArray[i]->m_VpeOutputHeight = ALIGN_2xUP(videoParam[i].u16VpeOutHeight);
            }
            else
            {
#if TARGET_CHIP_I5
              g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xDOWN(videoParam[i].u16VpeOutWidth);
#elif  TARGET_CHIP_I6
              g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_16xDOWN(videoParam[i].u16VpeOutWidth);
#elif  TARGET_CHIP_I6E
             g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xUP(videoParam[i].u16VpeOutWidth);
#elif  TARGET_CHIP_I6B0
             g_videoEncoderArray[i]->m_VpeOutputWidth  = ALIGN_8xUP(videoParam[i].u16VpeOutWidth);
#endif
              g_videoEncoderArray[i]->m_VpeOutputHeight = ALIGN_2xUP(videoParam[i].u16VpeOutHeight);
            }

    #ifdef TARGET_CHIP_I6
            g_videoEncoderArray[i]->m_VpeChnPort.eModId    = videoParam[i].stVpeChnPort.eModId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32DevId  = videoParam[i].stVpeChnPort.u32DevId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId  = videoParam[i].stVpeChnPort.u32ChnId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32PortId = videoParam[i].stVpeChnPort.u32PortId;
    #elif TARGET_CHIP_I6E
            g_videoEncoderArray[i]->m_VpeChnPort.eModId    = videoParam[i].stVpeChnPort.eModId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32DevId  = videoParam[i].stVpeChnPort.u32DevId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId  = videoParam[i].stVpeChnPort.u32ChnId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32PortId = videoParam[i].stVpeChnPort.u32PortId;
    #elif TARGET_CHIP_I6B0
            g_videoEncoderArray[i]->m_VpeChnPort.eModId    = videoParam[i].stVpeChnPort.eModId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32DevId  = videoParam[i].stVpeChnPort.u32DevId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId  = videoParam[i].stVpeChnPort.u32ChnId;
            g_videoEncoderArray[i]->m_VpeChnPort.u32PortId = videoParam[i].stVpeChnPort.u32PortId;
    #else
            memcpy(&g_videoEncoderArray[i]->m_VpeChnPort, &MI_VideoEncoder::VpeChnPortBottom, sizeof(MI_SYS_ChnPort_t));
         g_videoEncoderArray[i]->m_VpeChnPort.u32PortId = videoParam[i].stVpeChnPort.u32PortId;
    #endif


            g_videoEncoderArray[i]->m_VencChnPort.eModId    = (MI_ModuleId_e)0;
            g_videoEncoderArray[i]->m_VencChnPort.u32DevId  = 0;
            g_videoEncoderArray[i]->m_VencChnPort.u32ChnId  = 0;
            g_videoEncoderArray[i]->m_VencChnPort.u32PortId = 0;

            if(0 != videoParam[i].s8DivpEnable)
            {

                g_videoEncoderArray[i]->m_bUseDivp = TRUE;
                printf("%s i %d, m_bUseDivp %d  true\n", __func__, i, g_videoEncoderArray[i]->m_bUseDivp);

                g_videoEncoderArray[i]->m_DivpChnPort.eModId    = E_MI_MODULE_ID_DIVP;
                g_videoEncoderArray[i]->m_DivpChnPort.u32DevId  = 0;
#if TARGET_CHIP_I6B0
                if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
                {
                    if(MI_VideoEncoder::u32RealDivpChn == 0xff)
                    {
                        MI_VideoEncoder::u32RealDivpChn = g_s32DivpChnIndex;
                        g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId    = g_s32DivpChnIndex++;
                    }
                    else
                    {
                        g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId = MI_VideoEncoder::u32RealDivpChn;
                    }
                }
                else
                {
                     g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId  = g_s32DivpChnIndex++;
                }
#else
                g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId = Mixer_Divp_GetChannleNum();
#endif
                g_videoEncoderArray[i]->m_DivpChnPort.u32PortId = 0;
                g_videoEncoderArray[i]->m_bDivpFixed = TRUE;

            }
            else
            {
                g_videoEncoderArray[i]->m_DivpChnPort.eModId    = E_MI_MODULE_ID_MAX;
                g_videoEncoderArray[i]->m_DivpChnPort.u32DevId  = 0;
                g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId  = 0;
                g_videoEncoderArray[i]->m_DivpChnPort.u32PortId = 0;
                g_videoEncoderArray[i]->m_bDivpFixed = FALSE;

                g_videoEncoderArray[i]->m_bUseDivp = FALSE;
            }

#if TARGET_CHIP_I5
            /*if(0 != videoParam[i].s8DivpEnable)
            {
                g_videoEncoderArray[i]->m_DivpChnPort.eModId   = E_MI_MODULE_ID_DIVP;
                g_videoEncoderArray[i]->m_DivpChnPort.u32DevId = 0;
                g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId = g_s32DivpChnIndex++;
                g_videoEncoderArray[i]->m_DivpChnPort.u32PortId = 0;
            }*/

            if(videoParam[i].stVpeChnPort.u32ChnId == 0)
            {
                g_videoEncoderArray[i]->m_eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
            }
            else if(videoParam[i].stVpeChnPort.u32ChnId == 1)
            {
                g_videoEncoderArray[i]->m_eRunningMode = E_MI_VPE_RUN_REALTIME_BOTTOM_MODE;
            }
            else
            {
                printf("%s:%d vpe%d chnport info: DevId=%d, ChnId=%d, PortId=%d\n", __func__, __LINE__, i, videoParam[i].stVpeChnPort.u32DevId,
                                             videoParam[i].stVpeChnPort.u32ChnId, videoParam[i].stVpeChnPort.u32PortId);
            }
#endif

            stVpePortInfo.VpeChnPort.eModId    = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
            stVpePortInfo.VpeChnPort.u32DevId  = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
            stVpePortInfo.VpeChnPort.u32ChnId  = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
            stVpePortInfo.VpeChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;
            stVpePortInfo.ePixelFormat  = g_videoEncoderArray[i]->m_eVpeOutportPixelFormat;

            stVpePortInfo.u16VpeOutputWidth  = g_videoEncoderArray[i]->m_VpeOutputWidth;
            stVpePortInfo.u16VpeOutputHeight = g_videoEncoderArray[i]->m_VpeOutputHeight;

    //    MIXER_DBG("curW:%d, curH:%d.\n", videoParam[i].u16VpeOutWidth, videoParam[i].u16VpeOutHeight);
        MIXER_DBG("vpe_w(%d), h(%d),  vpe_out_w(%d), vpe_out_h(%d), %d, %d, %d, %d\n", videoParam[i].width, \
                                                                videoParam[i].height,\
                                                                videoParam[i].u16VpeOutWidth,\
                                                                videoParam[i].u16VpeOutHeight,\
                                                                stVpePortInfo.u16VpeOutputWidth,\
                                                                stVpePortInfo.u16VpeOutputHeight,\
                                                                g_videoEncoderArray[i]->m_width,\
                                                                g_videoEncoderArray[i]->m_height);

            g_videoEncoderArray[i]->initVideoInput(&stVpePortInfo);
        }
    }


    // initVideoEncoder after all initVideoInput finished
    for(int i = 0; i < videoNumber; i++)
    {
        g_videoEncoderArray[i]->initVideoEncoder();
        g_videoEncoderArray[i]->startVideoEncoder();
        if(VI_DISABLE == videoParam[i].viChnStatus || VI_DISABLE_DEPTHLY == videoParam[i].viChnStatus)
        {
            //g_videoEncoderArray[i]->setChnStatus(videoParam[i].viChnStatus);
        }
        else
        {
            g_videoEncoderArray[i]->m_viChnStatus = videoParam[i].viChnStatus;
        }
    }

    if((g_bInitRotate & 0xFF000000) && (g_bInitRotate & 0xFFFF))
    {
        MI_U32 Rotate = g_bInitRotate & 0xFFFF;
        MySystemDelay(1000); //makesure  getstream thread is running.
        video_SetRotate((MI_S8 *)&Rotate, (MI_U32)sizeof(Rotate));
    }

    return 0;
}

int videoClose()
{
    Mixer_Sys_BindInfo_T stBindInfo;

    /************************************************
    Step1:  stop venc
    *************************************************/
        for(MI_U32 i = 0; i < g_videoNumber; i++)
    {
        printf("stopVideoEncoder exit %d\n", i);

        g_videoEncoderArray[i]->stopVideoEncoder();
        g_videoEncoderArray[i]->uninitVideo(MIXER_SYS_INPUT_VPE);
        delete g_videoEncoderArray[i];
     g_videoEncoderArray[i] = NULL;
    }

    /************************************************
    Step2:  stop vpe
    *************************************************/
#if TARGET_CHIP_I5
    if((g_bInitRotate & 0xFF000000) && (NULL != g_videoEncoderArray[0]))
    {
        memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = MI_VideoEncoder::VpeChnPortTop.eModId;
        stBindInfo.stSrcChnPort.u32DevId = MI_VideoEncoder::VpeChnPortTop.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

        stBindInfo.stDstChnPort.eModId = g_videoEncoderArray[0]->m_VpeChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId = g_videoEncoderArray[0]->m_VpeChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->m_VpeChnPort.u32PortId;

        MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    }

    if(g_bInitRotate & 0xFF000000)
    {
        MIXERCHECKRESULT(Mixer_Vpe_StopPort(0, 0));
        MIXERCHECKRESULT(Mixer_Vpe_StopChannel(1));

    }

#endif
    MIXERCHECKRESULT(Mixer_Vpe_StopChannel(0));
    /************************************************
    Step3:  unbind VIF->VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = MI_VideoEncoder::VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId = MI_VideoEncoder::VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId = MI_VideoEncoder::VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

    stBindInfo.stDstChnPort.eModId = MI_VideoEncoder::VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId = MI_VideoEncoder::VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

      /************************************************
    Step4: destroy vpe
    *************************************************/

#if TARGET_CHIP_I5
    if(g_bInitRotate & 0xFF000000)
    {
        MIXERCHECKRESULT(Mixer_Vpe_DestroyChannel(1));
    }
#endif
    MIXERCHECKRESULT(Mixer_Vpe_DestroyChannel(0));

    return 0;
}


MI_S32 videoSetGop2RcParam(MI_U32 gop, VencRcParamEx_t *pRcParam)
{
    if(0 == gop || NULL == pRcParam)
    {
        return -1;
    }

    if(Video_ControlRateConstant == pRcParam->controlRate)
    {
        pRcParam->type.h264Cbr.gop = gop;
    }
    else if(Video_ControlRateVariable == pRcParam->controlRate)
    {
        pRcParam->type.h264Vbr.gop = gop;
    }
    else if(Video_ControlRateConstantQP == pRcParam->controlRate)
    {
        pRcParam->type.h264FixQp.gop = gop;
    }

    return 0;
}

ViChnStatus _s32ToViChnStatus(MI_S32 num)
{
    if(0 == num)
    {
        return VI_DISABLE;
    }
    else if(1 == num)
    {
        return VI_DISABLE_DEPTHLY;
    }
    else if(2 == num)
    {
        return VI_ENABLE;
    }
    else
    {
        return VI_ILLEGAL;
    }
}

MI_S32 GetSnrResInfo(MI_U16 *pu8SnrResIdx, MI_U32 length)
{
    if(NULL == pu8SnrResIdx || 0x0 == length)
    {
        MIXER_ERR("pu8SnrResIdx is null or 0x0 == length, err\n");
        return -1;
    }

    MI_SNR_Res_t stSnrRes;
    MI_U16 tmp[100] = {0x0};
    MI_U8 u8SnrResIndex = 0;
    MI_U32 u32SnrResCount = 0;
    MI_S32 ret = -1;

    ExecFunc(MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32SnrResCount), MI_SUCCESS);
    tmp[0] = CMD_VIDEO_GET_SENSOR_RESOLUTION;
    tmp[1] = u32SnrResCount;
    for(u8SnrResIndex = 0; u8SnrResIndex < u32SnrResCount; u8SnrResIndex++)
    {
        memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));
        if(MI_SUCCESS != (ret = MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8SnrResIndex, &stSnrRes)))
        {
            printf("%s:%d call MI_SNR_GetRes() fail!\n", __func__, __LINE__);
            continue;
        }
        if((7 + u8SnrResIndex*5) < (100-3))
        {
            tmp[3 + u8SnrResIndex*5] = u8SnrResIndex;
            tmp[4 + u8SnrResIndex*5] = stSnrRes.stCropRect.u16Width;
            tmp[5 + u8SnrResIndex*5] = stSnrRes.stCropRect.u16Height;
            tmp[6 + u8SnrResIndex*5] = stSnrRes.u32MaxFps;
            tmp[7 + u8SnrResIndex*5] = stSnrRes.u32MinFps;

            printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                              u8SnrResIndex,  stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                              stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                              stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                              stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);
        }
    }
    tmp[2] = ret;
    memcpy(pu8SnrResIdx, tmp, length > (MI_U16)((8 + u8SnrResIndex*5)*sizeof(MI_U16)) ? (MI_U16)((8 + u8SnrResIndex*5)*sizeof(MI_U16)) : length);
    return 0;

}

#if 0
MI_S32 SelectSnrResolution(MI_U8 *pu8SnrResIdx, MI_U32 *pu32SnrOutWidth, MI_U32 *pu32SnrOutHeight, MI_U32 *pu32SnrMaxFps, MI_U32 *pu32SnrMinFps)
{
    MI_SNR_Res_t stSnrRes;
    MI_U8 u8SnrResIndex = 0;
    MI_U32 u32SnrResCount = 0;
    MI_U32 s32SelectSnrIdx = 0;
    MI_S32 s32SelectSnrTimes = 5;

    ExecFunc(MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32SnrResCount), MI_SUCCESS);

    for(u8SnrResIndex = 0; u8SnrResIndex < u32SnrResCount; u8SnrResIndex++)
    {
        memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));
        if(MI_SUCCESS != MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8SnrResIndex, &stSnrRes))
        {
            printf("%s:%d call MI_SNR_GetRes() fail!\n", __func__, __LINE__);
            continue;
        }

        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                          u8SnrResIndex,  stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                          stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                          stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                          stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);
    }

    do
    {
        printf("Please choose which kind of resolution to be set (total count %d) :", u32SnrResCount);
        scanf("%d", &s32SelectSnrIdx);

        if(s32SelectSnrIdx >= u32SnrResCount)
        {
            printf("%s:%d choose Sensor resolution index(%d) is out of range(%d)\n", __func__,__LINE__, s32SelectSnrIdx, u32SnrResCount);
            s32SelectSnrIdx = 0xFF;
            continue;
        }

        memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));
        if(MI_SUCCESS != MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, s32SelectSnrIdx, &stSnrRes))
        {
            printf("%s:%d call MI_SNR_GetRes() fail!\n", __func__, __LINE__);
            continue;
        }

        if(g_bHdr)
        {
            if(((15 < stSnrRes.u32MaxFps) && (3840 <= stSnrRes.stOutputSize.u16Width) && (2160 <= stSnrRes.stOutputSize.u16Width)) ||
               ((30 < stSnrRes.u32MaxFps) && (1920 < stSnrRes.stOutputSize.u16Width) && (1080 < stSnrRes.stOutputSize.u16Width)))
            {
                printf("%s:%d I5 not support sensor fps:%d, Output width:%d height:%d\n", __func__,__LINE__,
                                 stSnrRes.u32MaxFps, stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height);
                s32SelectSnrIdx = 0xFF;
                continue;
            }
        }
        else
        {
            if(((20 < stSnrRes.u32MaxFps) && (3840 <= stSnrRes.stOutputSize.u16Width) && (2160 <= stSnrRes.stOutputSize.u16Width)) ||
               ((30 < stSnrRes.u32MaxFps) && (1920 < stSnrRes.stOutputSize.u16Width) && (1080 < stSnrRes.stOutputSize.u16Width)))
            {
                printf("%s:%d I5 not support sensor fps:%d, Output width:%d height:%d\n", __func__,__LINE__,
                                 stSnrRes.u32MaxFps, stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height);
                s32SelectSnrIdx = 0xFF;
                continue;
            }
        }

        if(s32SelectSnrIdx > u32SnrResCount)
        {
            printf("choice error idx %d, which is larger than the total count %d\n", s32SelectSnrIdx, u32SnrResCount);
            if(s32SelectSnrTimes)
            {
                printf("choice which resolution to use (total count %d) :", u32SnrResCount);
            }
        }
    }while((--s32SelectSnrTimes) && (s32SelectSnrIdx > u32SnrResCount));

    if((s32SelectSnrTimes <= 0) && (s32SelectSnrIdx > u32SnrResCount))
    {
        *pu8SnrResIdx = ~(0);
        *pu32SnrOutWidth  = 0;
        *pu32SnrOutHeight = 0;
        *pu32SnrMaxFps = 0;
        *pu32SnrMinFps = 0;
        return -1;
    }

    memset(&stSnrRes, 0x00, sizeof(MI_SNR_Res_t));
    u8SnrResIndex = (MI_U8)s32SelectSnrIdx;
    ExecFunc(MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8SnrResIndex, &stSnrRes), MI_SUCCESS);

    printf("You select index %d resolution, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n\n",
                          u8SnrResIndex, stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                          stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                          stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                          stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);

    *pu8SnrResIdx = u8SnrResIndex;
    *pu32SnrOutWidth  = stSnrRes.stCropRect.u16Width;
    *pu32SnrOutHeight = stSnrRes.stCropRect.u16Height;
    *pu32SnrMaxFps = stSnrRes.u32MaxFps;
    *pu32SnrMinFps = stSnrRes.u32MinFps;

    return 0;
}
#endif
int video_SetRotateV2(MI_S8 *param, MI_U32 paramLen)
{
    MI_U32 veChn = 0;
    MI_S32 videoParam[2];
    MI_S32 rotation = 0;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_Rotate_e eSetType = E_MI_SYS_ROTATE_NONE;
    memset(videoParam,0,sizeof(videoParam));
    memset(&stBindInfo,0x00,sizeof(Mixer_Sys_BindInfo_T));
    memcpy(videoParam, param, paramLen);
    rotation = videoParam[0];
    rotation %= 360;
    switch(rotation)
    {
        case 0:
            eSetType = E_MI_SYS_ROTATE_NONE;
            break;
        case 90:
            eSetType = E_MI_SYS_ROTATE_90;
            break;
        case 180:
            eSetType = E_MI_SYS_ROTATE_180;
            break;
        case 270:
            eSetType = E_MI_SYS_ROTATE_270;
            break;
        default:
            eSetType = E_MI_SYS_ROTATE_NONE;
            MIXER_ERR("%s: rotate param err, rotation=%d\n", __func__, rotation);
            return -1;
    }
    if(eSetType ==  g_videoEncoderArray[0]->m_initRotate)
    {
        MIXER_WARN("rotation state is same\n");
        if(0 == g_videoEncoderArray[0]->m_initRotate)
        {
          return -1;
        }
        return 0;
    }
    for(veChn=0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {

           g_videoEncoderArray[veChn]->stopVideoEncoder();
#if 1
        g_videoEncoderArray[veChn]->uninitVideo(MIXER_SYS_INPUT_VPE);
#else
        if(g_videoEncoderArray[veChn]->m_encoderType != VE_YUV420 && (FALSE == g_videoEncoderArray[veChn]->m_bDivpInit))
        {
            stBindInfo.stSrcChnPort.eModId    = g_videoEncoderArray[veChn]->m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[veChn]->m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[veChn]->m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[veChn]->m_VpeChnPort.u32PortId;
            stBindInfo.stDstChnPort.eModId    = g_videoEncoderArray[veChn]->m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[veChn]->m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[veChn]->m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[veChn]->m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = g_videoEncoderArray[veChn]->vpeframeRate;
            stBindInfo.u32DstFrmrate = g_videoEncoderArray[veChn]->m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
        }
#endif
    }
    ExecFunc(MI_VPE_StopChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
        ExecFunc(MI_VIF_DisableChnPort(i, 0), MI_SUCCESS);
    }
#if TARGET_CHIP_I6
    struct mixer_rotation_t new_sensor_state;
     MI_S8 ret = 0;
    memset(&new_sensor_state,0,sizeof(mixer_rotation_t));
    ret = GetMixerRotState(&new_sensor_state, eSetType, GetMirrorFlipLevel());
    MIXER_DBG("====GetMixerRotState=%d===new=bFlip=%d===mirror=%d==\n",ret,new_sensor_state.bFlip,new_sensor_state.bMirror);
    if(0 == ret)
    {
      ExecFunc(MI_SNR_SetOrien(E_MI_SNR_PAD_ID_0, new_sensor_state.bMirror, new_sensor_state.bFlip), MI_VPE_OK);
    }
#endif
    ExecFunc(MI_VPE_SetChannelRotation(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId, eSetType), MI_SUCCESS);
#if 0
    for(veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {
      g_videoEncoderArray[veChn]->uninitVideoV2(MIXER_SYS_INPUT_VPE);
    }
#endif

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[0]->VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[0]->VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[0]->VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[0]->VifChnPort.u32PortId;
    stBindInfo.u32SrcFrmrate = g_videoEncoderArray[0]->vifframeRate;

    stBindInfo.stDstChnPort.eModId = g_videoEncoderArray[0]->VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[0]->VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[0]->VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->VpeChnPortTop.u32PortId;
    stBindInfo.u32DstFrmrate = g_videoEncoderArray[0]->vpeframeRate;

    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    for(veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {
        if(((eSetType % 2) != (g_videoEncoderArray[veChn]->m_initRotate % 2)))
        {
           g_videoEncoderArray[veChn]->setRotateV2(eSetType);
        }
    }

    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
          MIXERCHECKRESULT(Mixer_Vif_StartPort(i,MI_VideoEncoder::VifChnPort.u32ChnId,
                                                     MI_VideoEncoder::VifChnPort.u32PortId));
    }
    for(veChn = 0; (veChn < g_videoNumber)&&(veChn < MAX_VIDEO_NUMBER)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {
        g_videoEncoderArray[veChn]->initDivpAndVdisp(0);
        g_videoEncoderArray[veChn]->initVideoEncoder();
        g_videoEncoderArray[veChn]->startVideoEncoder();
        g_videoEncoderArray[veChn]->m_initRotate = eSetType;
        ExecFunc(MI_VPE_EnablePort(g_videoEncoderArray[veChn]->m_VpeChnPort.u32ChnId,  g_videoEncoderArray[veChn]->m_VpeChnPort.u32PortId), MI_VPE_OK);

    }
    /*
#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
    if(E_MI_MODULE_ID_DIVP == g_videoEncoderArray[veChn]->m_DivpChnPort.eModId)
    {
        g_videoEncoderArray[veChn]->initDivpAndVdisp(1);
        MIXERCHECKRESULT(Mixer_Venc_StartChannel(g_videoEncoderArray[veChn]->m_veChn));
    }
#endif
*/
    ExecFunc(MI_VPE_StartChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);

    return MI_SUCCESS;
}
int video_SetRotate(MI_S8 *param, MI_U32 paramLen)
{
   return video_SetRotateV2(param,paramLen);
#if 0
    MI_U32 veChn = 0;
    MI_S32 videoParam[2];
    MI_S32 rotation = 0;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_Rotate_e eSetType = E_MI_SYS_ROTATE_NONE;


    memcpy(videoParam, param, paramLen);
    rotation = videoParam[0];
    rotation %= 360;

    if(0 != rotation && 90 != rotation && 180 != rotation && 270 != rotation)
    {
        MIXER_ERR("%s: rotation %d not support\n", __func__, rotation);
        g_bInitRotate = 0x00;
        return -1;
    }
    //eSetType = GetRotaType(rotation);
    //MIXER_DBG("===eSetType==%d==\n",eSetType);
    switch(rotation)
    {
        case 0:
            eSetType = E_MI_SYS_ROTATE_NONE;
            break;
        case 90:
            eSetType = E_MI_SYS_ROTATE_90;
            break;
        case 180:
            eSetType = E_MI_SYS_ROTATE_180;
            break;
        case 270:
            eSetType = E_MI_SYS_ROTATE_270;
            break;
        default:
            eSetType = E_MI_SYS_ROTATE_NONE;
            MIXER_ERR("%s: rotate param err, rotation=%d\n", __func__, rotation);
            g_bInitRotate = 0x00;
            break;
     }

    if(eSetType ==  g_videoEncoderArray[0]->m_initRotate)
    {
        MIXER_WARN("rotation state is same\n");
        if(0 == g_videoEncoderArray[0]->m_initRotate)
        {
          g_bInitRotate = 0x00;
        }
        return 0;
    }

    MIXER_DBG("%s:%d m_initRotate=%d, target rotation=%d, %d\n", __func__, __LINE__,
                  g_videoEncoderArray[0]->m_initRotate, rotation, ((rotation / 90) % 2));
    stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[0]->VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[0]->VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[0]->VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[0]->VifChnPort.u32PortId;
    stBindInfo.u32SrcFrmrate = g_videoEncoderArray[0]->vifframeRate;

    stBindInfo.stDstChnPort.eModId = g_videoEncoderArray[0]->VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[0]->VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[0]->VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->VpeChnPortTop.u32PortId;
    stBindInfo.u32DstFrmrate = g_videoEncoderArray[0]->vpeframeRate;

    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
        ExecFunc(MI_VIF_DisableChnPort(i, 0), MI_SUCCESS);
    }
    ExecFunc(MI_VPE_StopChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);
#if TARGET_CHIP_I6
    struct mixer_rotation_t new_sensor_state;
     MI_S8 ret = 0;
    memset(&new_sensor_state,0,sizeof(mixer_rotation_t));
    ret = GetMixerRotState(&new_sensor_state, eSetType, GetMirrorFlipLevel());
    MIXER_DBG("====GetMixerRotState=%d===new=bFlip=%d===mirror=%d==\n",ret,new_sensor_state.bFlip,new_sensor_state.bMirror);
    if(0 == ret)
    {
      ExecFunc(MI_SNR_SetOrien(E_MI_SNR_PAD_ID_0, new_sensor_state.bMirror, new_sensor_state.bFlip), MI_VPE_OK);
    }
#endif
    ExecFunc(MI_VPE_SetChannelRotation(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId, eSetType), MI_SUCCESS);
    for(veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {
      g_videoEncoderArray[veChn]->setRotate(eSetType);
    }
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
          MIXERCHECKRESULT(Mixer_Vif_StartPort(i,MI_VideoEncoder::VifChnPort.u32ChnId,
                                     MI_VideoEncoder::VifChnPort.u32PortId));
    }
    ExecFunc(MI_VPE_StartChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);

#endif
    return MI_SUCCESS;
}

//divpCnt (in/out):   in   sizeof divpStreamIdx
//                           out stream count that has divp
MI_U32 video_getDivpStreamNum(MI_U32 *pDivpStreamIdx, MI_U32 *pDivpCnt)
{
    MI_U32 i = 0, maxCnt = *pDivpCnt, divpCnt = 0;
    *pDivpCnt = 0;
    for(i = 0; i < g_videoNumber && i < maxCnt; i++)
    {
        if(g_videoEncoderArray[i]->m_bUseDivp)
        {
            pDivpStreamIdx[divpCnt] = i;
            divpCnt++;
        }
    }

    *pDivpCnt = divpCnt;
    return divpCnt;
}

//bindType:  bind type divp change to
//ret:           0 can not change divp bind type
//                1 can change divp bind type
MI_S32 videoCheckDivpBindType(MI_S32 bindType)
{
    MI_U32 streamInfo[MAX_VIDEO_NUMBER] = {0},  divpCnt = MAX_VIDEO_NUMBER, vpePort = 0;
    if(0 == video_getDivpStreamNum(streamInfo, &divpCnt))
    {
        MIXER_DBG("no divp stream, can not change\n");
        return 0;
    }

    if(1 != divpCnt)
    {
        MIXER_DBG("only support one divp stream now, divpCnt = %d\n", divpCnt);
        return 0;
    }

    vpePort = g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32PortId;
    if((1 == divpCnt && (MI_U32)VPE_REALMODE_SUB_PORT == vpePort && 0 == bindType) ||
		(1 == divpCnt && VPE_SUB_PORT == vpePort && 1 == bindType))
    {
        return 1;
    }

    MIXER_DBG("only support change divp bind type in this case.\n");
    return 0;
}

MI_S32 video_SetDivpBindTypeAll(MI_S32 bindtype)
{
    MI_U32 veChn = 0;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_U32 streamInfo[MAX_VIDEO_NUMBER] = {0},  divpCnt = MAX_VIDEO_NUMBER;
    memset(&stBindInfo,0x00,sizeof(Mixer_Sys_BindInfo_T));

    if(0 == video_getDivpStreamNum(streamInfo, &divpCnt))
    {
        MIXER_DBG("no divp stream\n");
        return -1;
    }

    for(veChn=0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {

        g_videoEncoderArray[veChn]->stopVideoEncoder();
        g_videoEncoderArray[veChn]->uninitVideo(MIXER_SYS_INPUT_VPE);

    }
    ExecFunc(MI_VPE_StopChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
        ExecFunc(MI_VIF_DisableChnPort(i, 0), MI_SUCCESS);
    }

    stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[0]->VifChnPort.eModId;
    stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[0]->VifChnPort.u32DevId;
    stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[0]->VifChnPort.u32ChnId;
    stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[0]->VifChnPort.u32PortId;
    stBindInfo.u32SrcFrmrate = g_videoEncoderArray[0]->vifframeRate;

    stBindInfo.stDstChnPort.eModId = g_videoEncoderArray[0]->VpeChnPortTop.eModId;
    stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[0]->VpeChnPortTop.u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[0]->VpeChnPortTop.u32ChnId;
    stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->VpeChnPortTop.u32PortId;
    stBindInfo.u32DstFrmrate = g_videoEncoderArray[0]->vpeframeRate;

    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;

    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    //set divp port
#if TARGET_CHIP_I6B0
    g_videoEncoderArray[streamInfo[0]]->bRealDivpInit = FALSE;
    g_videoEncoderArray[streamInfo[0]]->u32RealDivpChn = 0xFF;
#endif
    g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32PortId = (0 == bindtype) ? VPE_SUB_PORT : VPE_REALMODE_SUB_PORT;

    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
          MIXERCHECKRESULT(Mixer_Vif_StartPort(i,MI_VideoEncoder::VifChnPort.u32ChnId,
                                                     MI_VideoEncoder::VifChnPort.u32PortId));
    }
    for(veChn = 0; (veChn < g_videoNumber)&&(veChn < MAX_VIDEO_NUMBER)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
    {
        g_videoEncoderArray[veChn]->initDivpAndVdisp(0);
        g_videoEncoderArray[veChn]->initVideoEncoder();
        g_videoEncoderArray[veChn]->startVideoEncoder();
        ExecFunc(MI_VPE_EnablePort(g_videoEncoderArray[veChn]->m_VpeChnPort.u32ChnId,  g_videoEncoderArray[veChn]->m_VpeChnPort.u32PortId), MI_VPE_OK);
    }

    ExecFunc(MI_VPE_StartChannel(g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId), MI_VPE_OK);

    return 0;
}

MI_S32 video_SetDivpBindType(MI_S32 bindtype)
{
    //MI_U32 veChn = 0;
    MI_U32 streamInfo[MAX_VIDEO_NUMBER] = {0},  divpCnt = MAX_VIDEO_NUMBER;

    if(0 == video_getDivpStreamNum(streamInfo, &divpCnt))
    {
        MIXER_DBG("no divp stream\n");
        return -1;
    }

    //can only on divp now
    g_videoEncoderArray[streamInfo[0]]->stopVideoEncoder();
    ExecFunc(MI_VPE_DisablePort(g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32ChnId, g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32PortId), MI_VPE_OK);
    g_videoEncoderArray[streamInfo[0]]->uninitVideo(MIXER_SYS_INPUT_BUTT);

     //set divp port
#if TARGET_CHIP_I6B0
    g_videoEncoderArray[streamInfo[0]]->bRealDivpInit = FALSE;
    g_videoEncoderArray[streamInfo[0]]->u32RealDivpChn = 0xFF;
#endif
    g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32PortId = (0 == bindtype) ? VPE_SUB_PORT : VPE_REALMODE_SUB_PORT;

    g_videoEncoderArray[streamInfo[0]]->initDivpAndVdisp(1);
    ExecFunc(MI_VPE_EnablePort(g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32ChnId, g_videoEncoderArray[streamInfo[0]]->m_VpeChnPort.u32PortId), MI_VPE_OK);

    g_videoEncoderArray[streamInfo[0]]->startVideoEncoder();

    return 0;
}

MI_S8  changeVencParamIsOpSensor(MI_U32 pixe,MI_U16 data[],MI_U16 datalen)
{
  MI_S32 ret = -1;
  MI_U32 sensorWidth[5] = {0};
  MI_U32 sensorHeight[5] = {0};
//  MI_U16 sensorFrameRate[5] = {0};
  MI_U32 sensorPixe[5] = {0};
  MI_U8 workIndex = g_u8SnrCurResIdx;
  MI_U8 i=0;
  ret = GetSnrResInfo(data, datalen);;
  sensorWidth[0]  = data[4 + 0 * 5];
  sensorHeight[0] = data[5 + 0 * 5];
  //sensorPixe[0] = sensorWidth[0]*sensorHeight[0];
  if((0x00 == data[1]) || (1 == data[1]))
  {
    return -1;//no opt sensor
  }
  for(i=0; i<=data[1]; i++)
  {
    sensorWidth[i]  = data[4 + i * 5];
    sensorHeight[i] = data[5 + i * 5];
    sensorPixe[i] = sensorWidth[i]*sensorHeight[i];
  }
  if(0 == ret)
  {
     printf("data[1]=%d workIndex = %d  pixe=%d\n",data[1],workIndex,pixe);
     if(pixe <= sensorPixe[workIndex] && pixe > MIXER_CHANGE_SENSOR_RES_GATE)
     {
         return -1;//no opt sensor
     }
     else if(pixe < sensorPixe[workIndex] && sensorPixe[workIndex] > MIXER_CHANGE_SENSOR_RES_GATE)
     {
         for(i=0; i<data[1]; i++)
         {
            if(pixe <= sensorPixe[i] && pixe < MIXER_CHANGE_SENSOR_RES_GATE)
               {
                 return i;
               }
         }
     }
     else if(pixe > sensorPixe[workIndex] && sensorPixe[workIndex] < MIXER_CHANGE_SENSOR_RES_GATE)
     {
         for(i=0; i<data[1]; i++)
         {
            if(pixe >= sensorPixe[i] && pixe > MIXER_CHANGE_SENSOR_RES_GATE)
               {
                 return i;
               }
         }
     }
  }
  return -1;//cannot opt sensor
}
static void cmdSetVideoReslution(MI_S8 param[],MI_U8 paramLen)
{
    Size_t size;
    MI_U8 veChn;
    MI_S32 s32Ret = 0;
    MI_S8 setSensorIndex = -1;
    MI_U16 data[100] ={0x00};
    MI_U32 pixe = 0;
    MI_U32 videoParam[4]={0};
	memset(videoParam,0,sizeof(videoParam));
    memcpy(videoParam, param, paramLen);
    veChn = videoParam[0];
    size.width = videoParam[1];
    size.height = videoParam[2];

    if(veChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[veChn])
    {
        MIXER_ERR("venc channel [%d] error\n", veChn);
        s32Ret = -1;
        goto set_resolution_exit;
    }
    pixe = g_videoEncoderArray[veChn]->m_height * g_videoEncoderArray[veChn]->m_width /** g_videoEncoderArray[videoParam[0]]->m_vencframeRate*/;
    setSensorIndex = changeVencParamIsOpSensor(pixe,data,sizeof(data)/sizeof(data[0]));
    MIXER_DBG("setSensorIndex=%d\n",setSensorIndex);
    if(0 >= setSensorIndex)
    {
      MIXER_DBG("new reslution is width[%d] height[%d]  chn=%d! paramLen=%d\n",size.width,size.height,veChn,paramLen);
      g_videoEncoderArray[veChn]->setResolution(size.width, size.height);
	  s32Ret = 0;
    }
    else if(0 <= setSensorIndex)//change sensor resIndex and sensor fps
    {
      VideoParam_t pstVideoParam;
      memset(&pstVideoParam,0,sizeof(VideoParam_t));
      pstVideoParam.id = CMD_VIDEO_SET_RESOLUTION;
      pstVideoParam.u32SnrRes = setSensorIndex;
      pstVideoParam.u32SnrFps = data[6 + setSensorIndex * 5];
      pstVideoParam.u32VideoHeight = size.height;
      pstVideoParam.u32VideoWidth = size.width;
      pstVideoParam.sensorPad = E_MI_SNR_PAD_ID_0;
      if((size.height == g_videoEncoderArray[veChn]->m_height) && (size.width == g_videoEncoderArray[veChn]->m_width))
      {
        MIXER_DBG("your input venc[%d] reslution[w:%d,h:%d] have been set!\n",veChn,size.width,size.height);
        return;
      }
      g_videoEncoderArray[veChn]->m_bChangeRes = TRUE;
      MIXER_DBG("new video reslution is width[%d] height[%d],but need change sensor index=%d! sensorfps=%d\n",size.width,size.height,setSensorIndex, pstVideoParam.u32SnrFps);
      s32Ret = videoDisableSnrVifVpe(&pstVideoParam);
      if(0 != s32Ret)
      {
          MIXER_DBG("%s:%d  call videoDisableSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
          goto set_resolution_exit;
      }
      MIXER_DBG("setSensorIndex=%d==\n",setSensorIndex);
      s32Ret = videoCreateSnrVif(&pstVideoParam);
      if(0 != s32Ret)
      {
          MIXER_DBG("%s:%d  call videoCreateSnrVif() fail(%d)\n", __func__, __LINE__, s32Ret);
           goto set_resolution_exit;
      }
      s32Ret = videoRecreateSnrVifVpe(&pstVideoParam);
      if(0 != s32Ret)
      {
            MIXER_DBG("%s:%d  call videoRecreateSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
            goto set_resolution_exit;
      }
    }
    set_resolution_exit:
    if(0 == s32Ret)
    {
      if(0 <= setSensorIndex)
      {
        MIXER_DBG("set sensor and video[%d] resolution is ok!\n",veChn);
      }
      else
      {
        MIXER_DBG("set video[%d] resolution is ok!\n",veChn);
      }
    }
    else
    {
      if(veChn < MAX_VIDEO_NUMBER)
      {
        if(NULL != g_videoEncoderArray[veChn])
         g_videoEncoderArray[veChn]->m_veChn = veChn;
      }
      if(0 <= setSensorIndex)
      {
        MIXER_DBG("set sensor and video[%d] resolution is failed!\n",veChn);
      }
      else
      {
        MIXER_DBG("set video[%d] resolution is failed!\n",veChn);
      }
    }
	data[0] = CMD_VIDEO_SET_RESOLUTION;
	data[1] = 0;
	data[2] = veChn;
	if(0x0 != mixer_return_cmd_result(CMD_VIDEO_SET_RESOLUTION, (MI_S8*)data, sizeof(data)))
    {
        MIXER_ERR("return cmd result err.\n");
    }
}
static void cmdSetSensorReslution(MI_S8 param[],MI_U8 paramLen)
{
        MI_S32 s32Ret = -1;
        MI_U32 veChn = 0;
        MI_U32 videoParam[4];
        MI_U8 u8SnrResIdx = 0;
        MI_U32 u8SnrFps = 0;
        MI_U32 u32OldWidth = 0;
        MI_U32 u32OldHeight = 0;
        MI_U32 u32NewWidth = 0;
        MI_U32 u32NewHeight = 0;
		MI_U32 u32SnrResCount = 0;
        MI_SNR_Res_t stSnrRes;
        VideoParam_t stVideoParam;

        memcpy(videoParam, param, paramLen);
        u8SnrResIdx = videoParam[0];
        u32NewWidth = videoParam[1];
        u32NewHeight = videoParam[2];
        u8SnrFps = videoParam[3];
        stVideoParam.sensorPad = E_MI_SNR_PAD_ID_0;
		ExecFunc(MI_SNR_QueryResCount(stVideoParam.sensorPad, &u32SnrResCount), MI_SUCCESS);
		if(u8SnrResIdx >= u32SnrResCount)
		{
		   MIXER_DBG("set sensor res[%d] out of ringe[0-%d]\n",u8SnrResIdx,u32SnrResCount-1);
		   goto reselect_Snr_resolution_exit;
		}
        s32Ret = MI_SNR_GetRes(stVideoParam.sensorPad, g_u8SnrCurResIdx, &stSnrRes);
        if(MI_SUCCESS == s32Ret)
        {
          MIXER_DBG("Get Current Sensor resolution: index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                                  g_u8SnrCurResIdx, stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                                  stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                                  stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                                  stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);
        }
        else
        {
           MIXER_DBG("Get Current Sensor count[%d] is failed err[%d]!\n",u32SnrResCount,s32Ret);
		 //  goto reselect_Snr_resolution_exit;
        }
		
        u32OldWidth  = stSnrRes.stCropRect.u16Width;
        u32OldHeight = stSnrRes.stCropRect.u16Height;
        printf("%s:%d  ====>Old_Resolution(%d, %d), New_Resolution(%d, %d), SnrResIdx=%d\n",
                      __func__, __LINE__, u32OldWidth, u32OldHeight, u32NewWidth, u32NewHeight, u8SnrResIdx);


        memset(&stVideoParam, 0x00, sizeof(stVideoParam));
        stVideoParam.id = CMD_VIDEO_SET_SENSOR_RESOLUTION;
        stVideoParam.u32SnrRes = u8SnrResIdx;
        stVideoParam.u32SnrFps = u8SnrFps;
        stVideoParam.u32VideoWidth  = u32NewWidth;
        stVideoParam.u32VideoHeight = u32NewHeight;
        stVideoParam.sensorPad = E_MI_SNR_PAD_ID_0;

        s32Ret = videoDisableSnrVifVpe(&stVideoParam);
        if(0 != s32Ret)
        {
            printf("%s:%d  call videoDisableSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
            goto reselect_Snr_resolution_exit;
        }

        s32Ret = videoCreateSnrVif(&stVideoParam);
        if(0 != s32Ret)
        {
            printf("%s:%d  call videoCreateSnrVif() fail(%d)\n", __func__, __LINE__, s32Ret);
            goto reselect_Snr_resolution_exit;
        }
        s32Ret = videoRecreateSnrVifVpe(&stVideoParam);
        if(0 != s32Ret)
        {
            printf("%s:%d  call videoRecreateSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
            goto reselect_Snr_resolution_exit;
        }

   reselect_Snr_resolution_exit:

    MI_U16  data[100] = {0x0};
            data[0] = CMD_VIDEO_SET_SENSOR_RESOLUTION;
            data[1] = g_videoNumber;
            data[99] = s32Ret;

            for(veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
            {
                data[2 + veChn*4] = veChn;
                data[3 + veChn*4] = g_videoEncoderArray[veChn]->m_width;
                data[4 + veChn*4] = g_videoEncoderArray[veChn]->m_height;
                data[5 + veChn*4] = ((veChn == 0) ? 1 : 0);
            }

    if(0x0 != mixer_return_cmd_result(CMD_VIDEO_SET_SENSOR_RESOLUTION, (MI_S8*)data, sizeof(data)))
    {
        MIXER_ERR("return cmd result err.\n");
    }

}
int video_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    MI_U16  data[100] = {0x0};
    switch(id)
    {
        case CMD_VIDEO_SET_ROTATE_ENABLE:
            memcpy(&g_bInitRotate, param, sizeof(int));
            break;

        case CMD_VIDEO_OPEN:
            videoOpen((MixerVideoParam *)param, g_videoNumber);
            break;

        case CMD_VIDEO_CLOSE:
            videoClose();
            break;

        case CMD_VIDEO_START_FRAME_CAPTUR:
            {
                int veChn;
                memcpy(&veChn, param, sizeof(veChn));

                if(veChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[veChn])
                {
                    MIXER_ERR("venc channel [%d] error\n", veChn);
                   break;
                }

                g_videoEncoderArray[veChn]->startGetFrame();
            }
            break;

        case CMD_VIDEO_STOP_FRAME_CAPTUR:
            {
                int veChn;
                memcpy(&veChn, param, sizeof(veChn));

                if(veChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[veChn])
                {
                    MIXER_ERR("venc channel [%d] error\n", veChn);
                    break;
                }

                g_videoEncoderArray[veChn]->stopGetFrame();
            }
            break;

        case CMD_VIDEO_SET_SENSOR_FRAMERATE:
            {
                MI_U32 u32CurFps = 0;
                MI_U32 u32NewFps = 0;
                MI_U32 videoParam[4];
                MI_S32 s32SetTimes = 5;
                MI_SNR_Res_t stSensorCurRes;

                memcpy(videoParam, param, paramLen);

                ExecFunc(MI_SNR_GetFps(E_MI_SNR_PAD_ID_0, &u32CurFps), MI_VPE_OK);

                ExecFunc(MI_SNR_GetCurRes(E_MI_SNR_PAD_ID_0, &g_u8SnrCurResIdx, &stSensorCurRes), MI_VPE_OK);
                u32NewFps = videoParam[1];
                if((0 <= s32SetTimes) && (stSensorCurRes.u32MinFps <= u32NewFps) && (u32NewFps <= stSensorCurRes.u32MaxFps))
                {
                    ExecFunc(MI_SNR_SetFps(E_MI_SNR_PAD_ID_0, u32NewFps), MI_VPE_OK);
                    printf("%s:%d current sensor fps:%d(min:%d, max:%d), Set new sensor fps:%d\n", __func__, __LINE__,
                                             u32CurFps, stSensorCurRes.u32MinFps, stSensorCurRes.u32MaxFps, u32NewFps);
               //     if(MI_VideoEncoder::vifframeRate < stSensorCurRes.u32MaxFps)
                    {
                        Mixer_Sys_BindInfo_T stBindInfo;
                        MI_S32 u32DevId = -1;

                        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                        stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VifChnPort.eModId;
                        stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
                        stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
                        stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

                        stBindInfo.stDstChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
                        stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
                        stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
                        stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

                        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;
                        stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
                        MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                        for(MI_U32 i = 0; i < g_videoNumber; i++)
                        {
                            if((0 == g_videoEncoderArray[i]->m_pipCfg) && E_MI_MODULE_ID_DIVP == g_videoEncoderArray[i]->m_DivpChnPort.eModId)
                            {
                                // unbind divp to venc
                                if(g_videoEncoderArray[i]->m_encoderType != VE_YUV420)
                                {
                                    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                                    stBindInfo.stSrcChnPort.eModId    = g_videoEncoderArray[i]->m_DivpChnPort.eModId;
                                    stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[i]->m_DivpChnPort.u32DevId;
                                    stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId;
                                    stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_DivpChnPort.u32PortId;

                                    stBindInfo.stDstChnPort.eModId    = g_videoEncoderArray[i]->m_VencChnPort.eModId;
                                    stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[i]->m_VencChnPort.u32DevId;
                                    stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_VencChnPort.u32ChnId;
                                    stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[i]->m_VencChnPort.u32PortId;

                                    stBindInfo.u32SrcFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                                    stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                                    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
                                }

                                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                                stBindInfo.stSrcChnPort.eModId    = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
                                stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
                                stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
                                stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;

                                stBindInfo.stDstChnPort.eModId    = g_videoEncoderArray[i]->m_DivpChnPort.eModId;
                                stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[i]->m_DivpChnPort.u32DevId;
                                stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId;
                                stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[i]->m_DivpChnPort.u32PortId;

                                stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                                stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
                            }
                            else if(g_videoEncoderArray[i]->m_encoderType != VE_YUV420 && (0 == g_videoEncoderArray[i]->m_pipCfg))
                            {
                                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                                stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
                                stBindInfo.stSrcChnPort.u32ChnId = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
                                stBindInfo.stSrcChnPort.u32DevId = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
                                stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;
                                stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;

                                ExecFunc(MI_VENC_GetChnDevid(g_videoEncoderArray[i]->m_veChn, (MI_U32*)&u32DevId), MI_SUCCESS);
                                stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VENC;
                                stBindInfo.stDstChnPort.u32DevId  = u32DevId;
                                stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_veChn;
                                stBindInfo.stDstChnPort.u32PortId = 0;
                                stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
                            }
                        }

                    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                    stBindInfo.stSrcChnPort.eModId = MI_VideoEncoder::VifChnPort.eModId;
                    stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
                    stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
                    stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;

                    stBindInfo.stDstChnPort.eModId = MI_VideoEncoder::VpeChnPortTop.eModId;
                    stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
                    stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
                    stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

                    MI_VideoEncoder::vifframeRate = u32NewFps;
                    MI_VideoEncoder::vpeframeRate = u32NewFps;
                    stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;
                    stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
                    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

                    for(MI_U32 i = 0; i < g_videoNumber; i++)
                    {
                      if(E_MI_MODULE_ID_DIVP == g_videoEncoderArray[i]->m_DivpChnPort.eModId && (0 == g_videoEncoderArray[i]->m_pipCfg))
                      {
                        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                        stBindInfo.stSrcChnPort.eModId    = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
                        stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
                        stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
                        stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;

                        stBindInfo.stDstChnPort.eModId    = g_videoEncoderArray[i]->m_DivpChnPort.eModId;
                        stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[i]->m_DivpChnPort.u32DevId;
                        stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId;
                        stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[i]->m_DivpChnPort.u32PortId;

                        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                        stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
                        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#elif TARGET_CHIP_I6B0
                        if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
                        {
                            stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                            stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;        //port 3 bind real divp == (sensorfps:sensorfps)
                            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
                        }
                        else
                        {
                            stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                            stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                        }
#endif
                        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
                        if(g_videoEncoderArray[i]->m_encoderType != VE_YUV420)
                        {
                            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                            stBindInfo.stSrcChnPort.eModId    = g_videoEncoderArray[i]->m_DivpChnPort.eModId;
                            stBindInfo.stSrcChnPort.u32DevId  = g_videoEncoderArray[i]->m_DivpChnPort.u32DevId;
                            stBindInfo.stSrcChnPort.u32ChnId  = g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId;
                            stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_DivpChnPort.u32PortId;
                            stBindInfo.u32SrcFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                            stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
                            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                            stBindInfo.u32BindParam = 0;
#elif TARGET_CHIP_I6B0
                            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                            stBindInfo.u32BindParam = 0;
                            if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
                            {
                                stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                                stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                            }
#endif
                            ExecFunc(MI_VENC_GetChnDevid(g_videoEncoderArray[i]->m_veChn, (MI_U32*)&u32DevId), MI_SUCCESS);
                            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VENC;
                            stBindInfo.stDstChnPort.u32DevId  = u32DevId;
                            stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_veChn;
                            stBindInfo.stDstChnPort.u32PortId = 0;

                            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
                        }
                      }
                      else if(g_videoEncoderArray[i]->m_encoderType != VE_YUV420 && (0 == g_videoEncoderArray[i]->m_pipCfg))
                      {
                        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

                        stBindInfo.stSrcChnPort.eModId = g_videoEncoderArray[i]->m_VpeChnPort.eModId;
                        stBindInfo.stSrcChnPort.u32ChnId = g_videoEncoderArray[i]->m_VpeChnPort.u32ChnId;
                        stBindInfo.stSrcChnPort.u32DevId = g_videoEncoderArray[i]->m_VpeChnPort.u32DevId;
                        stBindInfo.stSrcChnPort.u32PortId = g_videoEncoderArray[i]->m_VpeChnPort.u32PortId;
                        stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
                        stBindInfo.eBindType = coverMixerBindMode(g_videoEncoderArray[i]->m_bindMode);
                        if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
                        {
                    #if TARGET_CHIP_I6B0
                            if(g_videoEncoderArray[i]->m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)stBindInfo.u32BindParam = g_videoEncoderArray[i]->m_height/2;
                             else stBindInfo.u32BindParam = g_videoEncoderArray[i]->m_height;
                    #else
                            stBindInfo.u32BindParam = g_videoEncoderArray[i]->m_height;
                    #endif
                        }
                        else
                        {
                          stBindInfo.u32BindParam = 0;
                        }
#elif TARGET_CHIP_I6E
                        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                        stBindInfo.u32BindParam = 0;
#endif
                        ExecFunc(MI_VENC_GetChnDevid(g_videoEncoderArray[i]->m_veChn, (MI_U32*)&u32DevId), MI_SUCCESS);
                        stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VENC;
                        stBindInfo.stDstChnPort.u32DevId  = u32DevId;
                        stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[i]->m_veChn;
                        stBindInfo.stDstChnPort.u32PortId = 0;
                        stBindInfo.u32DstFrmrate = g_videoEncoderArray[i]->m_vencframeRate;
                        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
                      }
                    }


#if TARGET_CHIP_I5
                        if((g_bInitRotate & 0xFF000000) && (NULL != g_videoEncoderArray[0]))
                        {
                            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                            stBindInfo.stSrcChnPort.eModId    = MI_VideoEncoder::VpeChnPortTop.eModId;
                            stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
                            stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
                            stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;

                            stBindInfo.stDstChnPort.eModId    = g_videoEncoderArray[0]->m_VpeChnPort.eModId;
                            stBindInfo.stDstChnPort.u32DevId  = g_videoEncoderArray[0]->m_VpeChnPort.u32DevId;
                            stBindInfo.stDstChnPort.u32ChnId  = g_videoEncoderArray[0]->m_VpeChnPort.u32ChnId;
                            stBindInfo.stDstChnPort.u32PortId = g_videoEncoderArray[0]->m_VpeChnPort.u32PortId;

                            stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vpeframeRate;
                            stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
                            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
                        }
#endif
                      }

             //for mantis:1679583, 1679744
                        if((g_EnablePIP) && (u32NewFps < g_videoEncoderArray[0]->m_vencframeRate))
                        {
                            g_videoEncoderArray[0]->setFrameRate(u32NewFps);
                        }
                    }
                    else
                    {
                       printf("%s:%d Set the sensor frame rate error[%d,%d]  fps:%d\n",__func__, __LINE__,stSensorCurRes.u32MinFps,stSensorCurRes.u32MaxFps,u32NewFps);
                    }
            }
            break;

        case CMD_VIDEO_SET_RESOLUTION:
            {
               cmdSetVideoReslution(param,paramLen);
            }
            break;

        case CMD_VIDEO_GET_SENSOR_RESOLUTION:
            {
                //MI_S32 ret = -1;
               /* MI_U8 u8SnrResIdx = 0;
                MI_U32 u32SnrOutWidth = 0;
                MI_U32 u32SnrOutHeight = 0;
                MI_U32 u32SnrMaxFps = 0;
                MI_U32 u32SnrMinFps = 0;*/

                //ret = SelectSnrResolution(&u8SnrResIdx, &u32SnrOutWidth, &u32SnrOutHeight, &u32SnrMaxFps, &u32SnrMinFps);
                MI_U16 data[100]={0x0};

              if(0x0 == GetSnrResInfo(data, sizeof(data)/sizeof(data[0])))
                  mixer_return_cmd_result(CMD_VIDEO_GET_SENSOR_RESOLUTION, (MI_S8*)data, sizeof(data));
              else
		        {
		            MIXER_ERR("can not ger snr res.\n");
		        }
            }
            break;

        case CMD_VIDEO_SET_SENSOR_RESOLUTION:
            {
               cmdSetSensorReslution(param,paramLen);
            }
            break;

        case CMD_VIDEO_SET_ENCODER:
        case CMD_VIDEO_SET_FRAMERATE:
            {
                MI_S32 videoParam[2];
                MI_U32 frameRate = 0;

                memcpy(videoParam, param, sizeof(videoParam));
                frameRate = videoParam[1];

                if(videoParam[0] >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[videoParam[0]])
                {
                    MIXER_ERR("video channel [%d] error\n", videoParam[0]);
                    break;
                }
                g_videoEncoderArray[videoParam[0]]->setFrameRate(frameRate);
            }
            break;

        case CMD_VIDEO_SET_SUPERFRAME:
            {
        #if TARGET_CHIP_I5 || TARGET_CHIP_I6B0 || TARGET_CHIP_I6 || TARGET_CHIP_I6E
                int videoParam[3];
                memcpy(videoParam, param, sizeof(videoParam));
                int veChn = videoParam[0];

                if(veChn >= MAX_VIDEO_NUMBER)
                {
                    MIXER_ERR("venc channel [%d] error\n", veChn);
                    break;
                }


                if((NULL != g_videoEncoderArray[videoParam[0]]) && (0 != videoParam[1] || 0 != videoParam[2]))
                {
                    g_videoEncoderArray[videoParam[0]]->setSuperFrm(videoParam[1], videoParam[2]);
                }
        #else
                  printf("This device not support the function!\n");
        #endif
            }
            break;

        case CMD_VIDEO_SET_SUPERFRAME_MODE:
            {
                MI_S32 s32Ret;
                MI_S32 veChn;
                MI_S32 videoParam[2];
                MI_VENC_SuperFrameCfg_t stSuperFrameCfg;

                memcpy(videoParam, param, sizeof(videoParam));
                veChn = videoParam[0];

                stSuperFrameCfg.eSuperFrmMode = (MI_VENC_SuperFrmMode_e)videoParam[1]; //E_MI_VENC_SUPERFRM_REENCODE;
                stSuperFrameCfg.u32SuperIFrmBitsThr = (g_videoEncoderArray[veChn]->m_bitRate / SUPERIPFRMBITDEN) * SUPERIFRMBITMOL;
                stSuperFrameCfg.u32SuperPFrmBitsThr = (g_videoEncoderArray[veChn]->m_bitRate / SUPERIPFRMBITDEN) * SUPERPFRMBITMOL;
                stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;

                s32Ret = MI_VENC_SetSuperFrameCfg(g_videoEncoderArray[veChn]->m_veChn, &stSuperFrameCfg);
                if(s32Ret)
                {
                    printf("%s %d MI_VENC_SetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
                }
                else
                {
                    printf("MI_VENC_SetSuperFrameCfg Success, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                                    stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
                }
            }
            break;
        case CMD_VIDEO_SET_IMAGE_QUALITY:
        case CMD_VIDEO_GET_ENCODER:
        case CMD_VIDEO_GET_RESOLUTION:
        case CMD_VIDEO_GET_FRAMERATE:
        case CMD_VIDEO_GET_BITRATE:
        case CMD_VIDEO_GET_BITRATE_CONTROL:
        case CMD_VIDEO_GET_IMAGE_QUALITY:
            break;

        case CMD_VIDEO_SET_GOP:
            {
                int videoParam[2];
                memcpy(videoParam, param, sizeof(videoParam));
                int veChn = videoParam[0];
                int gop = videoParam[1];

                if(veChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[veChn])
                {
                    MIXER_ERR("venc channel [%d] error\n", veChn);
                    break;
                }

                if((g_videoEncoderArray[veChn]->m_encoderType == VE_AVC) || (g_videoEncoderArray[veChn]->m_encoderType == VE_H265))
                {
                    g_videoEncoderArray[veChn]->setGop(gop);
                }
                else
                {
                  printf("The encoderType[%d] not have gop!\n",g_videoEncoderArray[veChn]->m_encoderType);
                }
            }
            break;

        case CMD_VIDEO_SET_SAO:
            break;

        case CMD_VIDEO_SET_BITRATE_CONTROL:
            {
                VENC_CHN chn = -1 ;
                int videoParam[16];
                if(paramLen > sizeof(videoParam))
                {
                    MIXER_ERR("CMD_VIDEO_SET_BITRATE_CONTROL OverFlow paramLen:%d\n", paramLen);
                    break;
                }
                memcpy(videoParam, param, paramLen);
                chn = videoParam[0];

                if(chn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[chn])
                {
                    MIXER_ERR("venc channel [%d] error\n", chn);
                    break;
                }
               if((VE_YUV420 != g_videoEncoderArray[chn]->m_encoderType)&&(VE_JPG_YUV422 != g_videoEncoderArray[chn]->m_encoderType))
               {
                 g_videoEncoderArray[chn]->setRcParamEx(videoParam);
               }
            }
            break;

        case CMD_VIDEO_REQUEST_IDR:
            {
                int videoParam[1];
                MI_S32 s32Ret = -1;
                memcpy(videoParam, param, sizeof(videoParam));
                s32Ret = MI_VENC_RequestIdr(videoParam[0], TRUE);
                if(0 == s32Ret)
                {
                  printf("set venc[%d] Request Idr ok!\n",videoParam[0]);
                }
                else
                {
                   printf("set venc[%d] Request Idr fail!\n",videoParam[0]);
                }
            }
            break;

        case CMD_VIDEO_CROP:
            break;

        case CMD_VIDEO_SET_BITRATE_CONTROL_CREATE:
            break;

        case CMD_VIDEO_SET_VIRTUAL_IINTERVAL:
            {
                MI_S32 videoParam[3];
                MI_U32 enhance = 0;
                MI_U32 virtualIEnable = 0;

                memcpy(videoParam, param, sizeof(videoParam));
                enhance = videoParam[1];
                virtualIEnable  = videoParam[2];
                g_videoEncoderArray[videoParam[0]]->setVirGop(enhance, virtualIEnable);
            }
            break;
/*
        case CMD_VIDEO_SET_FRAMERATE_AND_BITRATE:
            {
                MI_S32 videoParam[2];
                MI_U32 frameRate = 0;

                memcpy(videoParam, param, sizeof(videoParam));
                frameRate = videoParam[1];

                if(videoParam[0] >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[videoParam[0]])
                {
                    MIXER_ERR("venc channel [%d] error\n", videoParam[0]);
                    break;
                }

                g_videoEncoderArray[videoParam[0]]->setFrameRate(frameRate, 1);
            }
            break;
*/
        case CMD_VIDEO_SET_STATUS:
            break;

        case CMD_VIDEO_SET_ROTATE:
                video_SetRotate(param, paramLen);
                data[0] = CMD_OSD_RESTART_RESOLUTION;
                data[1] = g_videoNumber;

                for(MI_U32 veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
                {
                    data[2 + veChn*4] = veChn;
                    data[3 + veChn*4] = g_videoEncoderArray[veChn]->m_width;
                    data[4 + veChn*4] = g_videoEncoderArray[veChn]->m_height;
                    data[5 + veChn*4] = ((veChn == 0) ? 1 : 0);
                }
                if(0x0 != mixer_return_cmd_result(CMD_VIDEO_SET_ROTATE, (MI_S8*)data, sizeof(data)))
                {
                    MIXER_ERR("return cmd result err.\n");
                }
            break;

        case CMD_VIDEO_SET_DIVP_BIND_TYPE:
            {
                //videoParam[0]   divp change to [0]frame/[1]realtime
                //videoParam[1]   [0]all destroy/[1]only divp destroy
                MI_S32 videoParam[3] = {0};

                memcpy(videoParam, param, sizeof(videoParam));

                do{
                    if(0 == videoCheckDivpBindType(videoParam[0]))
                    {
                        break;
                    }

                    if(1 == videoParam[1])
                    {
                        video_SetDivpBindType(videoParam[0]);
                    }
                    else if(0 == videoParam[1])
                    {
                        video_SetDivpBindTypeAll(videoParam[0]);
                    }
                }while(FALSE);


                data[0] = CMD_OSD_RESTART_RESOLUTION;
                data[1] = g_videoNumber;

                for(MI_U32 veChn = 0; (veChn < g_videoNumber)&&(NULL != g_videoEncoderArray[veChn]); veChn++)
                {
                    data[2 + veChn*4] = veChn;
                    data[3 + veChn*4] = g_videoEncoderArray[veChn]->m_width;
                    data[4 + veChn*4] = g_videoEncoderArray[veChn]->m_height;
                    data[5 + veChn*4] = ((veChn == 0) ? 1 : 0);
                }

                printf("CMD_VIDEO_SET_DIVP_BIND_TYPE  response\n");
                if(0x0 != mixer_return_cmd_result(CMD_VIDEO_SET_DIVP_BIND_TYPE, (MI_S8*)data, sizeof(data)))
                {
                    MIXER_ERR("return cmd result err.\n");
                }
            }
            break;

        case CMD_VIDEO_SET_SAVE_TASK_STATUS:
            {
                int videoParam[4];
                MI_U32 timeInterval = 0;
                memcpy(videoParam, param, sizeof(videoParam));

                if(videoParam[0] >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[videoParam[0]])
                {
                    MIXER_ERR("venc channel [%d] error\n", videoParam[0]);
                    break;
                }

                if((videoParam[1] != 0) && (g_videoEncoderArray[videoParam[0]]->m_encoderType == VE_JPG ||
                    g_videoEncoderArray[videoParam[0]]->m_encoderType == VE_MJPEG))
                {
                    if(videoParam[2] & 0xffff0000)
                    {
                        if(1 == ((videoParam[2] & 0xffff0000) >> 16))
                        {
                            MI_U32 tmp = videoParam[2] & 0x0000ffff;
                            timeInterval = tmp * 1000000;
                        }
                        else
                        {
                            MI_U32 den = (videoParam[2] & 0xffff0000) >> 16;
                            MI_U32 num = (videoParam[2] & 0x0000ffff);
                            timeInterval = (int)(num * 1.0 / den * 1000000);
                        }
                    }
                    else
                    {
                        MI_U32 tmp = videoParam[2];
                        timeInterval = tmp * 1000000;
                    }

                    MI_U32 timePerFrame = (MI_U32)(1000000 * 1.0 / g_videoEncoderArray[videoParam[0]]->m_vencframeRate);
                    if(timeInterval < timePerFrame)
                    {
                        MIXER_ERR("The interval of jpeg/Mjpeg snap is less than the time of frame interval, not support.\n");
                    }
                    else
                    {
                        g_videoEncoderArray[videoParam[0]]->m_snapInterval = timeInterval;
                        g_videoEncoderArray[videoParam[0]]->m_snapNumber = videoParam[3];
                        g_videoEncoderArray[videoParam[0]]->m_snapDynamicallyMode = TRUE;
                        g_videoEncoderArray[videoParam[0]]->m_snapLastTimeStamp = MI_OS_GetTime();
                    }
                }
            }
            break;

        case CMD_VIDEO_CHANGE_CODEC:
            {
                MI_S32 veChn = *param;

                /*
                 * param[0] = vIndex;      // video index
                 * param[1] = codecType;   // video codec type
                 * param[2] = qfactor;     //
                 * param[3] = size.width;  // video new resolution
                 * param[4] = size.height; // video new resolution
                 * param[5] = frameparam;  // video new fps(divp,vdisp,venc)
                 * param[6] = vgop;        // video new gop
                */
                if((MAX_VIDEO_NUMBER <= veChn) || (NULL == g_videoEncoderArray[veChn]))
                {
                    MIXER_ERR("venc channel [%d] error\n", veChn);
                    break;
                }

                g_videoEncoderArray[veChn]->changeCodec((int *)param);
            }
            break;

        case CMD_VIDEO_SET_CROP_STATUS:
            break;

        case CMD_VIDEO_SET_3DNR:
            {
                int videoParam[3];
                MI_SNR_PADInfo_t  stSnrPad0Info;
                MI_VPE_ChannelPara_t stVpeChnParam;
                MI_VPE_HDRType_e eHDRType = E_MI_VPE_HDR_TYPE_OFF;
                MI_VPE_3DNR_Level_e  e3DNRLevel = E_MI_VPE_3DNR_LEVEL_OFF;

                memset(&stSnrPad0Info, 0x00, sizeof(MI_SNR_PADInfo_t));
                memset(videoParam, 0x00, sizeof(videoParam));
                memcpy(videoParam, param, paramLen);

                eHDRType = (MI_VPE_HDRType_e)videoParam[0];
                e3DNRLevel = (MI_VPE_3DNR_Level_e)videoParam[1];
#if TARGET_CHIP_I6 | TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                if(E_MI_VPE_3DNR_LEVEL2 < e3DNRLevel)
                {
                   e3DNRLevel = E_MI_VPE_3DNR_LEVEL2;
                }
#endif
                // get sensor hdr type mode
                ExecFunc(MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stSnrPad0Info), MI_SUCCESS);
                if(stSnrPad0Info.eHDRMode != (MI_VIF_HDRType_e)eHDRType)
                {
                    printf("%s:%d Reset HDR mode = %d(user set mode = %d)\n", __func__, __LINE__,
                                  (MI_U32)stSnrPad0Info.eHDRMode, (MI_U32)MI_VideoEncoder::eHDRType);
                    eHDRType = (MI_VPE_HDRType_e)stSnrPad0Info.eHDRMode;
                }
                if((mixer_GetHdrValue()) && ((E_MI_VPE_HDR_TYPE_OFF < eHDRType) && (eHDRType < E_MI_VPE_HDR_TYPE_MAX)))
                {
                    memset(&stVpeChnParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
                    ExecFunc(MI_VPE_GetChannelParam(0, &stVpeChnParam), MI_VPE_OK);
                    stVpeChnParam.eHDRType = eHDRType;
                }

                if((E_MI_VPE_3DNR_LEVEL_OFF < e3DNRLevel) && (e3DNRLevel < E_MI_VPE_3DNR_LEVEL_NUM))
                {
                    memset(&stVpeChnParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
                    ExecFunc(MI_VPE_GetChannelParam(0, &stVpeChnParam), MI_VPE_OK);
                    stVpeChnParam.bWdrEn = TRUE;
                    stVpeChnParam.e3DNRLevel = e3DNRLevel;
                }
                else
                {
                    memset(&stVpeChnParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
                    ExecFunc(MI_VPE_GetChannelParam(0, &stVpeChnParam), MI_VPE_OK);
                    stVpeChnParam.bWdrEn = FALSE;
                    stVpeChnParam.e3DNRLevel = E_MI_VPE_3DNR_LEVEL_OFF;
                }
                //敹恍�芋撘?ubind vif->vpe :stop     vif port: set vpe chn param ;bind vif->vpe ;start vf port
#if 0  //摰Ｘ撣貊璅∪�� stopVideoEncoder ->uninitVideo->ubind vif->vpe :stop     vif port: set vpe chn param ;bind vif->vpe ;start vf port->initDivpAndVdisp->startVideoEncoder
                for(MI_U32 i = 0; (i < g_videoNumber)&&(NULL != g_videoEncoderArray[i]); i++)
                {
                    g_videoEncoderArray[i]->stopVideoEncoder();
                    // need do un-initial operation for all module(vpe,divp,vdisp,venc)
                    g_videoEncoderArray[i]->uninitVideo(MIXER_SYS_INPUT_BUTT);

                }
#endif
                Mixer_Sys_BindInfo_T stBindInfo;
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId = MI_VideoEncoder::VifChnPort.eModId;
                stBindInfo.stSrcChnPort.u32DevId  = MI_VideoEncoder::VifChnPort.u32DevId;
                stBindInfo.stSrcChnPort.u32ChnId  = MI_VideoEncoder::VifChnPort.u32ChnId;
                stBindInfo.stSrcChnPort.u32PortId = MI_VideoEncoder::VifChnPort.u32PortId;
                stBindInfo.u32SrcFrmrate = MI_VideoEncoder::vifframeRate;

                stBindInfo.stDstChnPort.eModId = MI_VideoEncoder::VpeChnPortTop.eModId;
                stBindInfo.stDstChnPort.u32DevId  = MI_VideoEncoder::VpeChnPortTop.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = MI_VideoEncoder::VpeChnPortTop.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = MI_VideoEncoder::VpeChnPortTop.u32PortId;
                stBindInfo.u32DstFrmrate = MI_VideoEncoder::vpeframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
                Mixer_Vif_StopPort(0, 0);

                ExecFunc(MI_VPE_StopChannel(0), MI_VPE_OK);
                ExecFunc(MI_VPE_SetChannelParam(0, &stVpeChnParam), MI_VPE_OK);
                if((E_MI_VPE_HDR_TYPE_OFF < stVpeChnParam.eHDRType) && (stVpeChnParam.eHDRType < E_MI_VPE_HDR_TYPE_MAX))
                {
                  mixer_setHdrValue(TRUE);
                }
                else
                {
                   mixer_setHdrValue(FALSE);
                }
                ExecFunc(MI_VPE_StartChannel(0), MI_VPE_OK);
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
                MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
                Mixer_Vif_StartPort(0,0, 0);
#if 0 //摰Ｘ撣貊璅∪��
                for(MI_U32 i = 0; (i < g_videoNumber)&&(NULL != g_videoEncoderArray[i]); i++)
                {
                    g_videoEncoderArray[i]->initDivpAndVdisp(1);
                    g_videoEncoderArray[i]->startVideoEncoder();
                }
#endif
            }
            break;

        case CMD_VIDEO_CHANGE_LTR_PARAM:
            {
                int videoParam[3];
                memcpy(videoParam, param, sizeof(videoParam));

                if(videoParam[0] >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[videoParam[0]])
                {
                    MIXER_ERR("venc channel [%d] error\n", videoParam[0]);
                    break;
                }

                g_videoEncoderArray[videoParam[0]]->m_virtualIEnable = videoParam[1] == 1 ? TRUE : FALSE;
                g_videoEncoderArray[videoParam[0]]->m_virtualIInterval = videoParam[2];
            }
            break;

        case CMD_VIDEO_SET_ROICFG:
            {
                MI_U32 videoParam[9];
                MI_VENC_RoiCfg_t stRoiCfg;
                memcpy(videoParam, param, sizeof(videoParam));
                int veChn = videoParam[0];

                memset(&stRoiCfg, 0, sizeof(MI_VENC_RoiCfg_t));
                stRoiCfg.u32Index = videoParam[1];
                stRoiCfg.bEnable = videoParam[2];
                stRoiCfg.bAbsQp = videoParam[3];
                stRoiCfg.s32Qp = videoParam[4];
                stRoiCfg.stRect.u32Left = videoParam[5];
                stRoiCfg.stRect.u32Top = videoParam[6];
                stRoiCfg.stRect.u32Width = videoParam[7];
                stRoiCfg.stRect.u32Height = videoParam[8];
                if(veChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[veChn])
                {
                    MIXER_ERR("venc channel [%d]  error \n", veChn);
              return 0;
                }

                MIXER_DBG("venc ch[%d] roicfg info index:%d enable:%d bAbsQp:%d qp:%d left:%d top=%d w=%d h=%d\n", veChn, \
                          videoParam[1], videoParam[2], videoParam[3], videoParam[4], videoParam[5], videoParam[6], videoParam[7], videoParam[8]);
                g_videoEncoderArray[veChn]->SetRoiCfg(&stRoiCfg);
            }
            break;

        case CMD_VIDEO_SET_MIRROR_FLIP:
            {
                MI_BOOL bMirror,bFlip;
                MI_VPE_PortMode_t stVpeMode;
                MI_U32 vpeChn =0;
                MI_U32 VpePortid =0;
                MI_U32 nVpeMirrorFlipparam[4];

                memcpy(nVpeMirrorFlipparam, param, paramLen);
                vpeChn = nVpeMirrorFlipparam[0];
                VpePortid = nVpeMirrorFlipparam[1];
                bMirror = nVpeMirrorFlipparam[2];
                bFlip = nVpeMirrorFlipparam[3];
                printf("set vpe Mirror && flip:vpeChn=%d VpePortid=%d bMirror=%d bFlip=%d\n", vpeChn, VpePortid, bMirror, bFlip);
                memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));

                MI_VPE_GetPortMode(vpeChn, VpePortid, &stVpeMode);

                stVpeMode.bMirror = bMirror;
                stVpeMode.bFlip = bFlip;
                MI_VPE_SetPortMode(vpeChn, VpePortid, &stVpeMode);
            }
            break;
        case CMD_VIDEO_SWITCH_HDR_LINAER_MODE:
             {
                 MI_S32 s32Ret = -1;
                 MI_U32 videoParam[8];

                 MI_VIF_HDRType_e eHdrType = E_MI_VIF_HDR_TYPE_OFF;

                 memcpy((char*)videoParam, param, paramLen);
                 //u8SnrCount = videoParam[0];

                 eHdrType = (MI_VIF_HDRType_e)videoParam[2];
                 if((MI_VIF_HDRType_e)Mixer_HDR_Mode_NUM <= eHdrType)
                  {
                      MIXER_ERR("sdk don't support this value\n");
                    return 0;
                  }
                 mixer_setHdrValue(((MI_VPE_HDRType_e)eHdrType > E_MI_VPE_HDR_TYPE_OFF ? TRUE : FALSE));
                //to do.  too complex for I5 to chang hdr. should discuss with I5 owner
#if TARGET_CHIP_I5
                VideoParam_t stVideoParam;
                MI_U32 u32OldWidth = 0;
                         MI_U32 u32OldHeight = 0;
                MI_SNR_Res_t stSnrRes;
                MI_VPE_3DNR_Level_e e3DNRLevel = E_MI_VPE_3DNR_LEVEL3;
                e3DNRLevel = (MI_VPE_3DNR_Level_e)videoParam[1];
                if(mixer_GetHdrValue() && (E_MI_VIF_HDR_TYPE_OFF != eHdrType))
                 {
                     MI_SNR_PADInfo_t  stSnrPadInfo;

                     memset(&stSnrPadInfo, 0x00, sizeof(MI_SNR_PADInfo_t));

                     // get sensor hdr type mode
                     ExecFunc(MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stSnrPadInfo), MI_SUCCESS);
                     if(stSnrPadInfo.eHDRMode != eHdrType)
                     {
                         printf("%s:%d Reset HDR mode = %d(user set mode = %d)\n", __func__, __LINE__,
                                       (MI_U32)stSnrPadInfo.eHDRMode, (MI_U32)eHdrType);
                         eHdrType = (MI_VIF_HDRType_e)stSnrPadInfo.eHDRMode;
                     }
                 }

                 MI_VideoEncoder::e3DNRLevel = e3DNRLevel;
                 MI_VideoEncoder::eHDRType = (MI_VPE_HDRType_e)eHdrType;

                 ExecFunc(MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, g_u8SnrCurResIdx, &stSnrRes), MI_SUCCESS);
                 printf("Get Current Sensor resolution: index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
                                           g_u8SnrCurResIdx, stSnrRes.stCropRect.u16X, stSnrRes.stCropRect.u16Y,
                                           stSnrRes.stCropRect.u16Width, stSnrRes.stCropRect.u16Height,
                                           stSnrRes.stOutputSize.u16Width, stSnrRes.stOutputSize.u16Height,
                                           stSnrRes.u32MaxFps, stSnrRes.u32MinFps, stSnrRes.strResDesc);

                 u32OldWidth  = stSnrRes.stCropRect.u16Width;
                 u32OldHeight = stSnrRes.stCropRect.u16Height;

                 memset(&stVideoParam, 0x00, sizeof(stVideoParam));
                 stVideoParam.id = CMD_VIDEO_SWITCH_HDR_LINAER_MODE;
                 stVideoParam.u32SnrRes = g_u8SnrCurResIdx;
                 stVideoParam.u32SnrFps = 0;
                 stVideoParam.u32VideoWidth  = u32OldWidth;
                 stVideoParam.u32VideoHeight = u32OldHeight;
                 stVideoParam.e3DNRLevel = (MI_VPE_3DNR_Level_e)e3DNRLevel;
                 stVideoParam.eHdrType = (MI_VIF_HDRType_e)eHdrType;
                 stVideoParam.sensorPad = E_MI_SNR_PAD_ID_0;
                 s32Ret = videoDisableSnrVifVpe(&stVideoParam);
                 if(0 != s32Ret)
                 {
                     printf("%s:%d  call videoDisableSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
                     goto switch_Hdr_Linear_mode_exit;
                 }

                 s32Ret = videoCreateSnrVif(&stVideoParam);
                 if(0 != s32Ret)
                 {
                     printf("%s:%d  call videoCreateSnrVif() fail(%d)\n", __func__, __LINE__, s32Ret);
                     goto switch_Hdr_Linear_mode_exit;
                 }

                 s32Ret = videoRecreateSnrVifVpe(&stVideoParam);
                 if(0 != s32Ret)
                 {
                     printf("%s:%d  call videoRecreateSnrVifVpe() fail(%d)\n", __func__, __LINE__, s32Ret);
                     goto switch_Hdr_Linear_mode_exit;
                 }
#else
            s32Ret = videoSetHDR((Mixer_HDR_Mode_E)eHdrType);
#endif
            #if TARGET_CHIP_I5
            switch_Hdr_Linear_mode_exit:
            #endif

            MI_U16  data[2] = {0x0};
            data[0] = CMD_VIDEO_SWITCH_HDR_LINAER_MODE;
            data[1] = s32Ret;
            if(0x0 != mixer_return_cmd_result(CMD_VIDEO_SWITCH_HDR_LINAER_MODE, (MI_S8*)data, sizeof(data)))
            {
                MIXER_ERR("return cmd result err.\n");
            }

           }
           break;

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        case CMD_VIDEO_MMA_ALLOC:
            {
                MI_S32 videoNum;

                memcpy((char*)&videoNum, param, sizeof(videoNum));
                printf("CMD_VIDEO_MMA_ALLOC %d\n", videoNum);
#if LOAD_MIXER_CFG
                g_PriMmaManager->Config(videoNum, TRUE);
#endif
            }
            break;
        case CMD_VIDEO_MMA_FREE:
            {
                MI_S32 videoNum;

                memcpy((char*)&videoNum, param, sizeof(videoNum));
#if LOAD_MIXER_CFG
                g_PriMmaManager->Config(videoNum,  FALSE);
#endif
            }
            break;
#endif

        default:
            break;
    }

    return 0;
}
