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
/*************************************************
*
* File name: module_vif.cpp
* Author:     gavin.ran@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: mixer vif test file
*
*
*************************************************/
#include "module_common.h"
#include "mid_VideoEncoder.h"
#include "mid_vif.h"
#include <math.h>


extern int g_openIQServer;


MI_U8 g_u8SnrCurResIdx = 0;
typedef struct _SnrInfo_t
{
    MI_U32 idx;
    MI_U16 u16SnrW;
    MI_U16 u16SnrH;
    MI_U32 u32SnrMaxFps;
    MI_U32 u32SnrMinFps;
} SnrInfo_t;

extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
static pthread_t GetVifRawDateThread;
MI_S32 videoCreateSnrVif(VideoParam_t *pstVideoParam)
{
    MI_U8 u8ResIndex = 0;
    MI_U32 u32CurFps = 0;
    MI_U32 u32SnrResCount;
    MI_VIF_DEV VifDev = pstVideoParam->sensorPad*2;
    MI_VIF_PORT VifPort = 0;
    MI_VIF_CHN vifChn = VifDev*4;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SNR_Res_t stSensorCurRes;
    MI_SNR_PADInfo_t  stSnrPadInfo;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    SnrInfo_t stSnrInfo[MAX_VIDEO_NUMBER];
    Mixer_VIF_PortInfo_T stVifPortInfo;
    MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;

    MixerCmdId id = pstVideoParam->id;
    MI_U32 u32SnrRes = pstVideoParam->u32SnrRes;
    MI_U32 u32SnrFps = pstVideoParam->u32SnrFps;
    MI_U32 u32NewWidth = pstVideoParam->u32VideoWidth;
    MI_U32 u32NewHeight = pstVideoParam->u32VideoHeight;


    memset(&stSnrPadInfo, 0x00, sizeof(MI_SNR_PADInfo_t));
    memset(&stVifPortInfo, 0x00, sizeof(Mixer_VIF_PortInfo_T));
    memset(&stSnrInfo, 0x00, sizeof(SnrInfo_t) * MAX_VIDEO_NUMBER);

    if((E_MI_VIF_HDR_TYPE_VC == (MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType) ||
       (E_MI_VIF_HDR_TYPE_DOL== (MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType) ||
       (E_MI_VIF_HDR_TYPE_LI == (MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType))
    {
        eVifHdrType = (MI_VIF_HDRType_e)MI_VideoEncoder::eHDRType;
        ExecFunc(MI_SNR_SetPlaneMode(pstVideoParam->sensorPad, TRUE), MI_SUCCESS);
    }
    else if((E_MI_VIF_HDR_TYPE_VC == pstVideoParam->eHdrType) ||
            (E_MI_VIF_HDR_TYPE_DOL== pstVideoParam->eHdrType) ||
            (E_MI_VIF_HDR_TYPE_LI == pstVideoParam->eHdrType))
    {
        eVifHdrType = (MI_VIF_HDRType_e)pstVideoParam->eHdrType;
        ExecFunc(MI_SNR_SetPlaneMode(pstVideoParam->sensorPad, TRUE), MI_SUCCESS);
    }
    else
    {
        eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
        ExecFunc(MI_SNR_SetPlaneMode(pstVideoParam->sensorPad, FALSE), MI_SUCCESS);
    }

    if((CMD_VIDEO_SWITCH_HDR_LINAER_MODE == id) || (CMD_VIF_INIT == id))
    {
        ExecFunc(MI_SNR_QueryResCount(pstVideoParam->sensorPad, &u32SnrResCount), MI_SUCCESS);
        for(u8ResIndex = 0; u8ResIndex < u32SnrResCount; u8ResIndex++)
        {
            memset(&stSensorCurRes, 0x00, sizeof(MI_SNR_Res_t));
            if(MI_SUCCESS != MI_SNR_GetRes(pstVideoParam->sensorPad, u8ResIndex, &stSensorCurRes))
            {
                printf("%s:%d Get sensor resolution index %d error!\n", __func__, __LINE__, u8ResIndex);
                continue;
            }

            printf("%s:%d index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",__func__, __LINE__,
                          u8ResIndex, stSensorCurRes.stCropRect.u16X, stSensorCurRes.stCropRect.u16Y,
                          stSensorCurRes.stCropRect.u16Width, stSensorCurRes.stCropRect.u16Height,
                          stSensorCurRes.stOutputSize.u16Width, stSensorCurRes.stOutputSize.u16Height,
                          stSensorCurRes.u32MaxFps, stSensorCurRes.u32MinFps, stSensorCurRes.strResDesc);

            stSnrInfo[u8ResIndex].idx = u8ResIndex;
            stSnrInfo[u8ResIndex].u32SnrMaxFps = stSensorCurRes.u32MaxFps;
            stSnrInfo[u8ResIndex].u32SnrMinFps = stSensorCurRes.u32MinFps;
            stSnrInfo[u8ResIndex].u16SnrW = stSensorCurRes.stCropRect.u16Width;
            stSnrInfo[u8ResIndex].u16SnrH = stSensorCurRes.stCropRect.u16Height;
        }

        for(u8ResIndex = 0; u8ResIndex < u32SnrResCount; u8ResIndex++)
        {
            SnrInfo_t stSnrInfoTmp;
            for(MI_U32 u32ResIndexCmp = u8ResIndex + 1; u32ResIndexCmp < u32SnrResCount; u32ResIndexCmp++)
            {
                if((stSnrInfo[u8ResIndex].u16SnrW > stSnrInfo[u32ResIndexCmp].u16SnrW)||
                   ((stSnrInfo[u8ResIndex].u16SnrW == stSnrInfo[u32ResIndexCmp].u16SnrW) &&
                    (stSnrInfo[u8ResIndex].u16SnrH > stSnrInfo[u32ResIndexCmp].u16SnrH))||
                   ((stSnrInfo[u8ResIndex].u16SnrW == stSnrInfo[u32ResIndexCmp].u16SnrW) &&
                    (stSnrInfo[u8ResIndex].u16SnrH == stSnrInfo[u32ResIndexCmp].u16SnrH) &&
                    (stSnrInfo[u8ResIndex].u32SnrMaxFps > stSnrInfo[u32ResIndexCmp].u32SnrMaxFps)))
                {
                    memcpy(&stSnrInfoTmp, &stSnrInfo[u8ResIndex], sizeof(SnrInfo_t));
                    memcpy(&stSnrInfo[u8ResIndex], &stSnrInfo[u32ResIndexCmp], sizeof(SnrInfo_t));
                    memcpy(&stSnrInfo[u32ResIndexCmp], &stSnrInfoTmp, sizeof(SnrInfo_t));
                }
            }
        }
    //    MI_U16 lastIndex = 0x0;
        BOOL  statue = false;
        MI_U8  minDiffFps= 250;
    //    MI_U8  i = 0;
        MI_U32 minDiffRes = 0xffffffff;
        MI_U8 vifFps = SENSOR_FPS_UP2_INT(MI_VideoEncoder::vifframeRate);
        if(CMD_VIF_INIT == id)
        {
          vifFps =SENSOR_FPS_UP2_INT(u32SnrFps);
        }
        for(u8ResIndex = 0; u8ResIndex < u32SnrResCount; u8ResIndex++)
        {
            MI_U16 u16SnrOutW = stSnrInfo[u8ResIndex].u16SnrW;
            MI_U16 u16SnrOutH = stSnrInfo[u8ResIndex].u16SnrH;

            MIXER_DBG(" u32NewWidth=%d, u32NewHeight=%d.u16SnrOutW=%d, u16SnrOutH=%d\n", u32NewWidth, u32NewHeight, u16SnrOutW, u16SnrOutH);

    //        MIXER_DBG("stSnrInfo[u8ResIndex].u32SnrMaxFps=%d, MI_VideoEncoder::vifframeRate =%d\n", stSnrInfo[u8ResIndex].u32SnrMaxFps,MI_VideoEncoder::vifframeRate);
            if((u32NewWidth == u16SnrOutW) && (u32NewHeight == u16SnrOutH))
            {
              if(0 == minDiffFps)
              {
                u32SnrRes = stSnrInfo[u8ResIndex].idx;
                break;
              }
              else if((abs((MI_S32)(stSnrInfo[u8ResIndex].u32SnrMaxFps-vifFps))) < minDiffFps)
              {
                     minDiffFps = abs((MI_S32)(stSnrInfo[u8ResIndex].u32SnrMaxFps-MI_VideoEncoder::vifframeRate));
                  u32SnrRes = stSnrInfo[u8ResIndex].idx;
                  statue = true;
              }
            }
            else if(abs((MI_S32)(u32NewWidth*u32NewHeight - u16SnrOutH*u16SnrOutW)) < minDiffRes)
            {
              if(false == statue)
              {
                  minDiffRes = abs((MI_S32)(u32NewWidth*u32NewHeight - u16SnrOutH*u16SnrOutW));
                  u32SnrRes = stSnrInfo[u8ResIndex].idx;
              }
            }
        }
    }

    ExecFunc(MI_SNR_SetRes(pstVideoParam->sensorPad, u32SnrRes), MI_SUCCESS);
    if(E_MI_VIF_SNRPAD_ID_0 == (MI_VIF_SNRPad_e)pstVideoParam->sensorPad)
    {
      g_u8SnrCurResIdx = u32SnrRes;
      MIXER_DBG(" g_u8SnrCurResIdx=%d.\n", u32SnrRes);
    }
    memset(&stSensorCurRes, 0x00, sizeof(MI_SNR_Res_t));
    ExecFunc(MI_SNR_GetRes(pstVideoParam->sensorPad, u32SnrRes, &stSensorCurRes), MI_SUCCESS);

    if(stSensorCurRes.u32MinFps <= u32SnrFps && u32SnrFps <= stSensorCurRes.u32MaxFps)
    {
        ExecFunc(MI_SNR_SetFps(pstVideoParam->sensorPad, u32SnrFps), MI_SUCCESS);
        printf("%s:%d current sensor fps(min:%d, max:%d), Set new sensor fps:%d u32SnrRes=%d\n", __func__, __LINE__,
            stSensorCurRes.u32MinFps, stSensorCurRes.u32MaxFps, u32SnrFps,u32SnrRes);
    }

    ExecFunc(MI_SNR_Enable(pstVideoParam->sensorPad), MI_SUCCESS);

    if(CMD_VIF_INIT == id)
    {
        memset(&stSensorCurRes, 0x00, sizeof(MI_SNR_Res_t));
        ExecFunc(MI_SNR_GetRes(pstVideoParam->sensorPad, u32SnrRes, &stSensorCurRes), MI_SUCCESS);

        ExecFunc(MI_SNR_GetFps(pstVideoParam->sensorPad, &u32CurFps), MI_SUCCESS);
        printf("%s:%d  Set sensor fps:%d, current sensor fps:%d(min:%d, max:%d) u32SnrRes=%d\n", __func__, __LINE__,
                       u32SnrFps, u32CurFps, stSensorCurRes.u32MinFps, stSensorCurRes.u32MaxFps,u32SnrRes);
    }

    ExecFunc(MI_SNR_GetPadInfo(pstVideoParam->sensorPad, &stSnrPadInfo), MI_SUCCESS);
    ExecFunc(MI_SNR_GetPlaneInfo(pstVideoParam->sensorPad, 0, &stSnrPlane0Info), MI_SUCCESS);


   if((CMD_VIDEO_SWITCH_HDR_LINAER_MODE == id) || (CMD_VIF_INIT == id) || \
    (CMD_VIDEO_SET_SENSOR_RESOLUTION == id) || (CMD_VIDEO_SET_RESOLUTION == id))
    {
       MIXERCHECKRESULT(Mixer_Vif_EnableDev(VifDev, eVifHdrType, &stSnrPadInfo));
    }
#if TARGET_CHIP_I5
    if(g_videoParam[0].stVifInfo.HdrType)
    {
        ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(E_MI_SYS_DATA_PRECISION_10BPP, E_MI_SYS_PIXEL_BAYERID_RG);
    }
    else
#endif
    {
        ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    }
    printf("stSnrPlane0Info.ePixPrecision=%d stSnrPlane0Info.eBayerId=%d, epixformat:%d\n",\
                            stSnrPlane0Info.ePixPrecision,\
                            stSnrPlane0Info.eBayerId, \
                            ePixFormat);
    stVifPortInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
    stVifPortInfo.u32RectWidth  = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.u32RectHeight = stSnrPlane0Info.stCapRect.u16Height;
    stVifPortInfo.u32DestWidth  = stSnrPlane0Info.stCapRect.u16Width;
    stVifPortInfo.u32DestHeight = stSnrPlane0Info.stCapRect.u16Height;
    stVifPortInfo.s32FrameRate  = E_MI_VIF_FRAMERATE_FULL;
    stVifPortInfo.ePixFormat    = ePixFormat;
    if(2 == VifDev)
    {
      stVifPortInfo.ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    }
    for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
    {
        MIXERCHECKRESULT(Mixer_Vif_CreatePort(vifChn, VifPort, &stVifPortInfo));
    /*    if((CMD_VIDEO_SWITCH_HDR_LINAER_MODE == id) || ((CMD_VIF_INIT == id)&&(1 < Mixer_vifDevNumberGet())) || \
        (CMD_VIDEO_SET_SENSOR_RESOLUTION == id) ||(CMD_VIDEO_SET_RESOLUTION == id))*/
        {
          MIXERCHECKRESULT(Mixer_Vif_StartPort(VifDev, vifChn, VifPort));
        }
        printf("Mixer_Vif_CreatePort VifDev=%d  vifChn=%d  VifPort=%d\n",VifDev,vifChn,VifPort);
    }
    return MI_SUCCESS;
}
void *getvifRawData(void* arg)
{
    MI_U64 phyAddr;
    MI_SYS_ChnPort_t stChnPort;
    MI_U8 u32DevId = Mixer_GetSensorNum();
    MI_U8 vifChn = u32DevId*4;
    MI_U8 vifPort = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 errCode = 0;
    MI_S32 s32Ret = -1;
    //MI_U32 pPhysAddr;
    //void *pVirtAddr = NULL;
    MI_S32 ret = -1;
    MI_U32 nReqSize =  (MI_U32)arg;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = u32DevId;
    stChnPort.u32ChnId = vifChn;
    stChnPort.u32PortId = vifPort;
    Mixer_Vif_StopPort(vifChn, vifPort);
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 6);
    s32Ret = Mixer_Vif_StartPort(u32DevId, vifChn, vifPort);
    while(Mixer_GetSensor1GetRawDataThrState())
    {
         ret = MI_SYS_MMA_Alloc(NULL,nReqSize,&phyAddr);
         if(0 == ret && (TRUE == Mixer_GetSensor1GetRawDataThrState()))
         {
              if(MI_SUCCESS == (errCode = MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle)))
              {
                 if(stBufInfo.stFrameData.phyAddr[0])
                 {
                   s32Ret = MI_SYS_MemcpyPa(phyAddr,stBufInfo.stFrameData.phyAddr[0],stBufInfo.stFrameData.u32BufSize);
                   if(MI_SUCCESS == s32Ret)
                   {
                        printf("MI_SYS_MemcpyPa is ok!\n");
                   }
                   else
                   {
                      printf("MI_SYS_MemcpyPa is error=%0x!\n",s32Ret);
                   }
                 }
                 printf("nReqSize=%d stBufInfo.stFrameData.u32BufSize =%d\n",nReqSize,stBufInfo.stFrameData.u32BufSize);
                 errCode = MI_SYS_ChnOutputPortPutBuf(hHandle);
                 if(MI_SUCCESS != errCode)
                 {
                   printf("MI_SYS_ChnOutputPortPutBuf is error=%0x",errCode);
                 }
                 else
                 {
                    printf("MI_SYS_ChnOutputPortPutBuf is ok!\n");
                 }
              }
             else
             {
                printf("u32DevId=%d u32ChnId=%d u32PortId=%d MI_SYS_ChnOutputPortGetBuf is error=%0x!\n",u32DevId,vifChn,vifPort,errCode);
             }
             ret = MI_SYS_MMA_Free(phyAddr);
             if(0 != ret)
             {
               printf(" MI_SYS_MMA_Free is error=%0x state=%d\n",ret, Mixer_GetSensor1GetRawDataThrState());
             }
             else
             {
               printf(" MI_SYS_MMA_Free is ok!\n");
             }
             sleep(3);
         }
         else if(0 != ret)
         {
           printf("mma alloc is error= %0x\n",ret);
           sleep(5);
         }
         else
         {
             ret = MI_SYS_MMA_Free(phyAddr);
             if(0 != ret)
             {
               printf(" MI_SYS_MMA_Free is error=%0x state=%d\n",ret, Mixer_GetSensor1GetRawDataThrState());
             }
             else
             {
               printf(" MI_SYS_MMA_Free is ok!\n");
               break;
             }
         }
   }
  return NULL;
}
void GetSensor1RawData(MI_U32 mmaSize)
{
    MI_S32 s32Ret = pthread_create(&GetVifRawDateThread, NULL, getvifRawData, (void*)((MI_U32)mmaSize));
    if(0 == s32Ret)
    {
        pthread_setname_np(GetVifRawDateThread, "GetSensor1RawData_Task");
    }
    else if(0 != s32Ret)
    {
      Mixer_SetSensor1GetRawDataThrState(FALSE);
      printf("pthread_create getvifRawData thrd is failed! s32Ret=%d\n",s32Ret);
    }
}
MI_S32 vif_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    switch(id)
    {
        case CMD_VIF_INIT:
            {
                MI_S32 vifparam[6];
                VideoParam_t stVideoParam;

                memcpy(vifparam, param, paramLen);
                Mixer_vifDevNumberSet(vifparam[0]);

                stVideoParam.id = CMD_VIF_INIT;
                stVideoParam.u32SnrRes = 0;
                stVideoParam.u32SnrFps = vifparam[1];
                stVideoParam.u32VideoWidth  = vifparam[2];
                stVideoParam.u32VideoHeight = vifparam[3];
                stVideoParam.eHdrType = (MI_VIF_HDRType_e)vifparam[4];
                stVideoParam.e3DNRLevel = (MI_VPE_3DNR_Level_e)vifparam[5];

                stVideoParam.sensorPad = E_MI_SNR_PAD_ID_0;
                videoCreateSnrVif(&stVideoParam);
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
               if(1 < Mixer_GetSensorNum())
                   {
                     stVideoParam.eHdrType = (MI_VIF_HDRType_e)0;
                   stVideoParam.e3DNRLevel = (MI_VPE_3DNR_Level_e)0;
                     Mixer_SetSensor1GetRawDataThrState(TRUE);
                     printf("======================open NO.2 sensor=================\n");
                     MIXERCHECKRESULT(Mixer_Sys_Init());
                  stVideoParam.sensorPad = E_MI_SNR_PAD_ID_1;
                  videoCreateSnrVif(&stVideoParam);
                  GetSensor1RawData(stVideoParam.u32VideoWidth* stVideoParam.u32VideoHeight*2);
                   }
#endif
            }
            break;

        case CMD_VIF_UNINIT:
            {
                MI_VIF_DEV VifDev = 0;
                MI_VIF_CHN VifChn = 0;
                MI_VIF_PORT VifPort = 0;

                for(VifChn = 0; VifChn < Mixer_vifDevNumberGet(); VifChn++)
                {
                    MIXERCHECKRESULT(Mixer_Vif_StopPort(VifChn, VifPort));
                }

                MIXERCHECKRESULT(Mixer_Vif_DisableDev(VifDev));
                MIXERCHECKRESULT(MI_SNR_Disable(E_MI_SNR_PAD_ID_0));
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
               if(1 < Mixer_GetSensorNum())
                   {
                  Mixer_SetSensor1GetRawDataThrState(FALSE);
                  pthread_join(GetVifRawDateThread, NULL);
                     printf("======================closing NO.2 sensor=================\n");
                     VifDev = Mixer_GetSensorNum();
                  VifChn = VifDev*4;
                  MIXERCHECKRESULT(Mixer_Vif_StopPort(VifChn, VifPort));
                  MIXERCHECKRESULT(Mixer_Vif_DisableDev(VifDev));
                  MIXERCHECKRESULT(MI_SNR_Disable(E_MI_SNR_PAD_ID_1));
                   }
#endif
                MI_U16  data[1] = {0x0};
                data[0] = CMD_VIF_UNINIT;
                if(0x0 != mixer_return_cmd_result(CMD_VIF_UNINIT, (MI_S8*)data, sizeof(data)))
                {
                    MIXER_ERR("return cmd result err.\n");
                }
            }
            break;

        default:
            break;
    }
    return 0;
}
