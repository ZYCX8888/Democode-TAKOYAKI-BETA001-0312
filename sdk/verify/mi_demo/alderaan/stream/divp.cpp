#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>

#include "mi_common.h"
#include "mi_sys.h"
#include "mi_divp.h"

#include "divp.h"

Divp::Divp()
{
}
Divp::~Divp()
{
}
void Divp::Init()
{
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutAttr;
    stDivpOutInfo_t stDivpOutInfo;
    std::map<unsigned int, stModOutputInfo_t>::iterator itDivpOut;
    char *pMmaName = NULL;

    stDivpInfo.intCropWidth = GetIniInt(stModDesc.modKeyString, "CROP_W");
    stDivpInfo.intCropHeight = GetIniInt(stModDesc.modKeyString, "CROP_H");
    stDivpInfo.intCropX = GetIniInt(stModDesc.modKeyString, "CROP_X");
    stDivpInfo.intCropY = GetIniInt(stModDesc.modKeyString, "CROP_Y");

    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = (MI_U16)stDivpInfo.intCropX;
    stDivpChnAttr.stCropRect.u16Y       = (MI_U16)stDivpInfo.intCropY;
    stDivpChnAttr.stCropRect.u16Width   = (MI_U16)stDivpInfo.intCropWidth;
    stDivpChnAttr.stCropRect.u16Height  = (MI_U16)stDivpInfo.intCropHeight;
    stDivpChnAttr.u32MaxWidth           = (MI_U16)stDivpInfo.intCropWidth;
    stDivpChnAttr.u32MaxHeight          = (MI_U16)stDivpInfo.intCropHeight;
    MI_DIVP_CreateChn((MI_DIVP_CHN)stModDesc.chnId, &stDivpChnAttr);
    pMmaName = GetIniString(stModDesc.modKeyString, "MMA_CONF");
    if (pMmaName)
    {
        MI_SYS_SetChnMMAConf((MI_ModuleId_e)stModDesc.modId, (MI_ModuleId_e)stModDesc.devId, (MI_DIVP_CHN)stModDesc.chnId, (MI_U8 *)(pMmaName));
    } 
    for (itDivpOut = mapModOutputInfo.begin(); itDivpOut != mapModOutputInfo.end(); itDivpOut++)
    {
        memset(&stDivpOutInfo, 0, sizeof(stDivpOutInfo_t));
        stDivpOutInfo.intDivpOutFmt = GetIniInt(itDivpOut->second.curIoKeyString, "VID_FMT");
        //printf("%s : VID_FMT %d\n", itDivpOut->second.curIoKeyString.c_str(), stDivpOutInfo.intDivpOutFmt);
        stDivpOutInfo.intDivputWidth = GetIniInt(itDivpOut->second.curIoKeyString, "VID_W");
        //printf("%s : VID_W %d\n", itDivpOut->second.curIoKeyString.c_str(), stDivpOutInfo.intDivputWidth);
        stDivpOutInfo.intDivpOutHeight = GetIniInt(itDivpOut->second.curIoKeyString, "VID_H");
        //printf("%s : VID_H %d\n", itDivpOut->second.curIoKeyString.c_str(), stDivpOutInfo.intDivpOutHeight);
        stDivpOutInfo.intPortId = itDivpOut->second.curPortId;
        memset(&stOutAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));
        stOutAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stOutAttr.ePixelFormat = (MI_SYS_PixelFormat_e)stDivpOutInfo.intDivpOutFmt;
        stOutAttr.u32Width = (MI_U32)stDivpOutInfo.intDivputWidth;
        stOutAttr.u32Height= (MI_U32)stDivpOutInfo.intDivpOutHeight;
        MI_DIVP_SetOutputPortAttr((MI_DIVP_CHN)stModDesc.chnId, &stOutAttr);
        MI_DIVP_StartChn((MI_DIVP_CHN)stModDesc.chnId);
        vDivpOutInfo.push_back(stDivpOutInfo);
    }
}
void Divp::Deinit()
{
    vDivpOutInfo.clear();
    MI_DIVP_StopChn((MI_DIVP_CHN)stModDesc.chnId);
    MI_DIVP_DestroyChn((MI_DIVP_CHN)stModDesc.chnId);
}

