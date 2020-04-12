#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <string>

#include "st_common.h"
#include "st_venc.h"

#include "venc.h"

Venc::Venc()
{
}
Venc::~Venc()
{
}
void Venc::Init()
{
    MI_VENC_ChnAttr_t stChnAttr;

    stVencInfo.intWidth = GetIniInt(stModDesc.modKeyString,"STREAM_W");
    stVencInfo.intHeight = GetIniInt(stModDesc.modKeyString,"STREAM_H");
    stVencInfo.intBitRate = GetIniInt(stModDesc.modKeyString,"BIT_RATE");
    stVencInfo.intEncodeType = GetIniInt(stModDesc.modKeyString,"EN_TYPE");

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    switch (stVencInfo.intEncodeType)
    {
        case E_MI_VENC_MODTYPE_H264E:
        {
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            //stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = 30;
            //stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            //stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;
            //stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
            //stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = ((MI_U32)stVencInfo.intBitRate) * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;

        }
        break;
        case E_MI_VENC_MODTYPE_H265E:
        {
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = ((MI_U32)stVencInfo.intBitRate) * 1024 * 1024;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
        }
        break;
        case E_MI_VENC_MODTYPE_JPEGE:
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = (MI_U32)stVencInfo.intWidth;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = (MI_U32)stVencInfo.intHeight;
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
        }
        break;
        default:
            assert(0);
    }
    stChnAttr.stVeAttr.eType = (MI_VENC_ModType_e)stVencInfo.intEncodeType;
    ST_Venc_CreateChannel((MI_VENC_CHN)stModDesc.chnId, &stChnAttr);
    ST_Venc_StartChannel((MI_VENC_CHN)stModDesc.chnId);
}
void Venc::Deinit()
{
    ST_Venc_StopChannel((MI_VENC_CHN)stModDesc.chnId);
    ST_Venc_DestoryChannel((MI_VENC_CHN)stModDesc.chnId);
}

