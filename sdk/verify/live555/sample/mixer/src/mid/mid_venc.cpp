/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_venc.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: venc module source file
*
*
*
* History:
*
*    1. Date  :        2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mid_venc.h"
#include "mid_common.h"
#include "mi_venc.h"
#include "mid_VideoEncoder.h"


extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];


MI_S32 Mixer_Venc_CreateChannel(MI_VENC_CHN VencChn, MI_VENC_ChnAttr_t *pstAttr)
{
    MI_U32 u32DevId = -1;

    if (pstAttr == NULL)
    {
        MIXER_ERR("invalid param\n");
        return -1;
    }

    ExecFunc(MI_VENC_CreateChn(VencChn, pstAttr), MI_SUCCESS);

    if (pstAttr->stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        MI_VENC_ParamJpeg_t stParamJpeg;

        memset(&stParamJpeg, 0x00, sizeof(stParamJpeg));
        ExecFunc(MI_VENC_GetJpegParam(VencChn, &stParamJpeg), MI_SUCCESS);
        MIXER_INFO("Get u32Qfactor:%d\n", stParamJpeg.u32Qfactor);
#if TARGET_CHIP_I5
        stParamJpeg.u32Qfactor = 50;
#elif TARGET_CHIP_I6
        stParamJpeg.u32Qfactor = 30;
#elif TARGET_CHIP_I6E
        stParamJpeg.u32Qfactor = 50;
#elif TARGET_CHIP_I6B0
        stParamJpeg.u32Qfactor = 50;
#endif
        ExecFunc(MI_VENC_SetJpegParam(VencChn, &stParamJpeg), MI_SUCCESS);
    }

    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

    g_videoEncoderArray[VencChn]->m_VencChnPort.eModId    = E_MI_MODULE_ID_VENC;
    g_videoEncoderArray[VencChn]->m_VencChnPort.u32DevId  = u32DevId;
    g_videoEncoderArray[VencChn]->m_VencChnPort.u32ChnId  = VencChn;
    g_videoEncoderArray[VencChn]->m_VencChnPort.u32PortId = 0;

#if TARGET_CHIP_I5
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&g_videoEncoderArray[VencChn]->m_VencChnPort,
                                           g_videoEncoderArray[VencChn]->m_vencBufUsrDepth,
                                           g_videoEncoderArray[VencChn]->m_vencBufCntQuota), MI_SUCCESS);
#endif
    return MI_SUCCESS;
}

MI_S32 Mixer_Venc_DestoryChannel(MI_VENC_CHN VencChn)
{
    ExecFunc(MI_VENC_DestroyChn(VencChn), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Venc_StartChannel(MI_VENC_CHN VencChn)
{
    ExecFunc(MI_VENC_StartRecvPic(VencChn), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Venc_StopChannel(MI_VENC_CHN VencChn)
{
    ExecFunc(MI_VENC_StopRecvPic(VencChn), MI_SUCCESS);

    return MI_SUCCESS;
}

