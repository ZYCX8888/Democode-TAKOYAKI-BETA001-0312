#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>

#include "mi_sensor.h"

#include "st_common.h"
#include "st_vpe.h"

#include "vpe.h"

Vpe::Vpe()
{
}
Vpe::~Vpe()
{
}
void Vpe::Init()
{
    ST_VPE_ChannelInfo_T stVpeChannelInfo;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_BOOL bMirror = FALSE, bFlip = FALSE;
    stVpeOutInfo_t stVpeOut;
    std::map<unsigned int, stModOutputInfo_t>::iterator itVpeOut;
    ST_VPE_PortInfo_T stVpePortInfo;

    stVpeInfo.intHdrType = GetIniInt(stModDesc.modKeyString, "HDR_TYPE");
    stVpeInfo.intbUseSnrFmt = GetIniInt(stModDesc.modKeyString, "IS_USE_SNR_FMT");
    stVpeInfo.intRotation= GetIniInt(stModDesc.modKeyString, "ROT");
    stVpeInfo.int3dNrLevel= GetIniInt(stModDesc.modKeyString, "3DNR_LV");
    stVpeInfo.intRunningMode = GetIniInt(stModDesc.modKeyString, "R_MOD");

    if (stVpeInfo.intbUseSnrFmt)
    {
        stVpeInfo.intSensorId = GetIniInt(stModDesc.modKeyString, "SNR_ID");
        memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
        MI_SNR_GetPlaneInfo((MI_SNR_PAD_ID_e)stVpeInfo.intSensorId, 0, &stSnrPlane0Info);
        stVpeInfo.intInputFmt = (int)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);
    }
    else
    {
        stVpeInfo.intInputFmt = GetIniInt(stModDesc.modKeyString, "VID_FMT");
    }
    switch((MI_SYS_Rotate_e)stVpeInfo.intRotation)
    {
        case E_MI_SYS_ROTATE_NONE:
            bMirror= FALSE;
            bFlip = FALSE;
            break;
        case E_MI_SYS_ROTATE_90:
            bMirror = FALSE;
            bFlip = TRUE;
            break;
        case E_MI_SYS_ROTATE_180:
            bMirror = TRUE;
            bFlip = TRUE;
            break;
        case E_MI_SYS_ROTATE_270:
            bMirror = TRUE;
            bFlip = FALSE;
            break;
        default:
            bMirror= FALSE;
            bFlip = FALSE;
            break;
    }
    MI_SNR_SetOrien((MI_SNR_PAD_ID_e)stVpeInfo.intSensorId, bMirror, bFlip);
    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = stSnrPlane0Info.stCapRect.u16Width;
    stVpeChannelInfo.u16VpeMaxH = stSnrPlane0Info.stCapRect.u16Height;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = (MI_VPE_RunningMode_e)stVpeInfo.intRunningMode;
    stVpeChannelInfo.eFormat = (MI_SYS_PixelFormat_e)stVpeInfo.intInputFmt ;
    stVpeChannelInfo.e3DNRLevel = (MI_VPE_3DNR_Level_e)stVpeInfo.int3dNrLevel;
    stVpeChannelInfo.eHDRtype = (MI_VPE_HDRType_e)stVpeInfo.intHdrType;
    stVpeChannelInfo.bRotation = FALSE;
    ST_Vpe_CreateChannel((MI_VPE_CHANNEL)stModDesc.chnId, &stVpeChannelInfo);
    MI_VPE_SetChannelRotation((MI_VPE_CHANNEL)stModDesc.chnId, (MI_SYS_Rotate_e)stVpeInfo.intRotation);
    ST_Vpe_StartChannel((MI_VPE_CHANNEL)stModDesc.chnId);
    for(itVpeOut = mapModOutputInfo.begin(); itVpeOut != mapModOutputInfo.end(); itVpeOut++)
    {
        memset(&stVpeOut, 0, sizeof(stVpeOutInfo_t));
        stVpeOut.intVpeOutFmt = GetIniInt(itVpeOut->second.curIoKeyString, "VID_FMT");
        //printf("%s : VID_FMT %d\n", itVpeOut->second.curIoKeyString.c_str(), stVpeOut.intVpeOutFmt);
        stVpeOut.intVpeOutWidth= GetIniInt(itVpeOut->second.curIoKeyString, "VID_W");
        //printf("%s : VID_W %d\n", itVpeOut->second.curIoKeyString.c_str(), stVpeOut.intVpeOutWidth);
        stVpeOut.intVpeOutHeight= GetIniInt(itVpeOut->second.curIoKeyString, "VID_H");
        //printf("%s : VID_H %d\n", itVpeOut->second.curIoKeyString.c_str(), stVpeOut.intVpeOutHeight);
        stVpeOut.intPortId = itVpeOut->second.curPortId;
        memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));
        stVpePortInfo.DepVpeChannel = (MI_VPE_CHANNEL)stModDesc.chnId;
        stVpePortInfo.ePixelFormat = (MI_SYS_PixelFormat_e)stVpeOut.intVpeOutFmt;
        stVpePortInfo.u16OutputWidth = (MI_U16)stVpeOut.intVpeOutWidth;
        stVpePortInfo.u16OutputHeight = (MI_U16)stVpeOut.intVpeOutHeight;
        stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        ST_Vpe_StartPort(stVpeOut.intPortId, &stVpePortInfo);
        vVpeOutInfo.push_back(stVpeOut);
    }
}
void Vpe::Start()
{

}
void Vpe::Stop()
{
    std::vector<stVpeOutInfo_t>::iterator itVpeOutInfo;

    for(itVpeOutInfo = vVpeOutInfo.begin(); itVpeOutInfo != vVpeOutInfo.end(); itVpeOutInfo++)
    {
        ST_Vpe_StopPort((MI_VPE_CHANNEL)stModDesc.chnId, itVpeOutInfo->intPortId);
    }
    vVpeOutInfo.clear();
}

void Vpe::Deinit()
{
    ST_Vpe_StopChannel((MI_VPE_CHANNEL)stModDesc.chnId);
    ST_Vpe_DestroyChannel((MI_VPE_CHANNEL)stModDesc.chnId);
}

