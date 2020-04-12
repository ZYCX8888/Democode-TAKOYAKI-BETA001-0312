#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <string>

#include "mi_sensor.h"

#include "st_common.h"
#include "st_vif.h"

#include "vif.h"

Vif::Vif()
{
}
Vif::~Vif()
{
}
void Vif::Init()
{
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_SNR_Res_t stRes;
    MI_U8 u8ResIndex =0;
    MI_U32 u32ResCount =0;
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    ST_VIF_PortInfo_T stVifPortInfoInfo;

    stVifInfo.intHdrType = GetIniInt(stModDesc.modKeyString,"HDR_TYPE");
    stVifInfo.intSensorId = GetIniInt(stModDesc.modKeyString,"SNR_ID");
    stVifInfo.intSensorRes = GetIniInt(stModDesc.modKeyString,"SNR_RES");

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    if(stVifInfo.intHdrType > 0)
        MI_SNR_SetPlaneMode((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, TRUE);
    else
        MI_SNR_SetPlaneMode((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, FALSE);

    MI_SNR_QueryResCount((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }
    if(stVifInfo.intSensorRes >= (int)u32ResCount)
    {
        printf("choice err res %d > =cnt %d\n", stVifInfo.intSensorRes, u32ResCount);
        assert(0);
    }
    MI_SNR_SetRes((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, (MI_U32)stVifInfo.intSensorRes);
    MI_SNR_Enable((MI_SNR_PAD_ID_e)stVifInfo.intSensorId);
    MI_SNR_GetPadInfo((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, &stPad0Info);
    MI_SNR_GetPlaneInfo((MI_SNR_PAD_ID_e)stVifInfo.intSensorId, 0, &stSnrPlane0Info);
    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    ST_Vif_EnableDev((MI_VIF_DEV)stModDesc.devId, (MI_VIF_HDRType_e)stVifInfo.intHdrType, &stPad0Info);
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    ST_Vif_CreatePort((MI_VIF_CHN)stModDesc.chnId, 0, &stVifPortInfoInfo);
    ST_Vif_StartPort((MI_VIF_DEV)stModDesc.devId, 0, 0);
}
void Vif::Deinit()
{
    ST_Vif_StopPort((MI_VIF_CHN)stModDesc.chnId, 0);
    ST_Vif_DisableDev((MI_VIF_DEV)stModDesc.devId);
}

