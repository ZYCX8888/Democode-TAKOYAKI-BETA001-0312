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

#include "st_vpe.h"
#include "st_common.h"

MI_S32 ST_Vpe_CreateChannel(MI_VPE_CHANNEL VpeChannel, ST_VPE_ChannelInfo_T *pstChannelInfo)
{
    MI_VPE_ChannelAttr_t stChannelVpeAttr;
    MI_SYS_WindowRect_t stCropWin;
    MI_VPE_ChannelPara_t stChannelVpeParam;

    memset(&stChannelVpeAttr, 0, sizeof(MI_VPE_ChannelAttr_t));
    memset(&stCropWin, 0, sizeof(MI_SYS_WindowRect_t));
    memset(&stChannelVpeParam, 0x00, sizeof(MI_VPE_ChannelPara_t));

    stChannelVpeParam.eHDRType = pstChannelInfo->eHDRtype;
    stChannelVpeParam.e3DNRLevel = pstChannelInfo->e3DNRLevel;
    MI_VPE_SetChannelParam(VpeChannel, &stChannelVpeParam);

    stChannelVpeAttr.u16MaxW = pstChannelInfo->u16VpeMaxW;
    stChannelVpeAttr.u16MaxH = pstChannelInfo->u16VpeMaxH;
    stChannelVpeAttr.bNrEn= FALSE;
    stChannelVpeAttr.bEdgeEn= FALSE;
    stChannelVpeAttr.bEsEn= FALSE;
    stChannelVpeAttr.bContrastEn= FALSE;
    stChannelVpeAttr.bUvInvert= FALSE;
    stChannelVpeAttr.ePixFmt = pstChannelInfo->eFormat;
    stChannelVpeAttr.eRunningMode = pstChannelInfo->eRunningMode;
    stChannelVpeAttr.bRotation = pstChannelInfo->bRotation;
    stChannelVpeAttr.eHDRType  = pstChannelInfo->eHDRtype;
    stChannelVpeAttr.eSensorBindId = pstChannelInfo->eBindSensorId;
    printf("beal.......mode %d %d  .....\n", E_MI_VPE_RUN_REALTIME_MODE, pstChannelInfo->eRunningMode);
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);
#if 0
    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);

    stChannelVpeAttr.bContrastEn = TRUE;
    stChannelVpeAttr.bNrEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpeAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = pstChannelInfo->u32X;
    stCropWin.u16Y = pstChannelInfo->u32Y;
    stCropWin.u16Width = pstChannelInfo->u16VpeCropW;
    stCropWin.u16Height = pstChannelInfo->u16VpeCropH;

    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
#endif
#if 0
    MI_VPE_ChannelPara_t stVpeParam; //default 3dnr Para
    stVpeParam.u8NrcSfStr = 80;
    stVpeParam.u8NrySfStr = 80;
    stVpeParam.u8NrcTfStr = 60;
    stVpeParam.u8NryTfStr = 60;
    stVpeParam.u8NryBlendMotionTh = 4;
    stVpeParam.u8NryBlendMotionWei = 16;
    stVpeParam.u8NryBlendOtherWei = 4;
    stVpeParam.u8NryBlendStillTh = 8;
    stVpeParam.u8NryBlendStillWei = 0;
    stVpeParam.u8Contrast = 128;
    stVpeParam.u8EdgeGain[0] = 0;
    stVpeParam.u8EdgeGain[1] = 20;
    stVpeParam.u8EdgeGain[2] = 80;
    stVpeParam.u8EdgeGain[3] = 120;
    stVpeParam.u8EdgeGain[4] = 160;
    stVpeParam.u8EdgeGain[5] = 160;

    ExecFunc(MI_VPE_SetChannelParam(VpeChannel, &stVpeParam), MI_VPE_OK);
#endif

    return MI_SUCCESS;
}

MI_VPE_RunningMode_e ST_Vpe_GetRunModeByVIFMode(VIF_WORK_MODE_E enWorkMode)
{
#if 0
    switch (e_WorkMode)
    {
        case SAMPLE_VI_MODE_1_D1:
        case SAMPLE_VI_MODE_16_D1:
        case SAMPLE_VI_MODE_16_960H:
        case SAMPLE_VI_MODE_2_720P:
        case SAMPLE_VI_MODE_1_1080P:
        case SAMPLE_VI_MODE_8_D1:
        case SAMPLE_VI_MODE_1_720P:
        case SAMPLE_VI_MODE_16_Cif:
        case SAMPLE_VI_MODE_16_2Cif:
        case SAMPLE_VI_MODE_16_D1Cif:
        case SAMPLE_VI_MODE_1_D1Cif:
        case SAMPLE_VI_MODE_4_D1:
        case SAMPLE_VI_MODE_8_2Cif:
            return E_MI_VPE_RUNNING_MODE_FRAMEBUF_DVR_MODE;
        case SAMPLE_VI_MODE_MIPI_1_1080P_VPE:
        case SAMPLE_VI_MODE_MIPI_1_1080P_VENC:
            return E_MI_VPE_RUNNING_MODE_REALTIME_MODE;
        default:
            ST_ERR("not support this work mode, mode:%d\n", e_WorkMode);
            return E_MI_VPE_RUNNING_MODE_INVALID;
    }
#endif

    return E_MI_VPE_RUN_REALTIME_MODE;
}

MI_S32 ST_Vpe_StartChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Vpe_StopChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_StopChannel(VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel)
{
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Vpe_StartPort(MI_VPE_PORT VpePort, ST_VPE_PortInfo_T *pstPortInfo)
{
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_ChnPort_t stChnPort;
    printf("xxxxxxxxxxx........ST_Vpe_StartPort......%d  %d....\n", pstPortInfo->DepVpeChannel, VpePort);

    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(pstPortInfo->DepVpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stVpeMode.ePixelFormat = pstPortInfo->ePixelFormat;
    stVpeMode.u16Width = pstPortInfo->u16OutputWidth;
    stVpeMode.u16Height= pstPortInfo->u16OutputHeight;
    ExecFunc(MI_VPE_SetPortMode(pstPortInfo->DepVpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = pstPortInfo->DepVpeChannel;
    stChnPort.u32PortId = VpePort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 5), 0);

    ExecFunc(MI_VPE_EnablePort(pstPortInfo->DepVpeChannel, VpePort), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Vpe_StopPort(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    return MI_SUCCESS;
}

