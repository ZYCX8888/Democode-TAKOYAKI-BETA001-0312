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
#include "mi_vif_test_util.h"

extern struct venc_channel_state _astChn[MAX_VENC_CHANNEL];

int bind_module(MI_ModuleId_e eSrcModId, MI_U32 u32SrcDevId, MI_U32 u32SrcChnId, MI_U32 u32SrcPortId, MI_U32 u32SrcFrmrate,
                MI_ModuleId_e eDstModId, MI_U32 u32DstDevId, MI_U32 u32DstChnId, MI_U32 u32DstPortId  , MI_U32 u32DstFrmrate)
{
    MI_SYS_ChnPort_t stSrcputPort;
    MI_SYS_ChnPort_t stDstputPort;

    memset(&stSrcputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcputPort.eModId = eSrcModId;
    stSrcputPort.u32DevId = u32SrcDevId;
    stSrcputPort.u32ChnId = u32SrcChnId;
    stSrcputPort.u32PortId = u32SrcPortId;
    memset(&stDstputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stDstputPort.eModId = eDstModId;
    stDstputPort.u32DevId = u32DstDevId;
    stDstputPort.u32ChnId = u32DstChnId;
    stDstputPort.u32PortId = u32DstPortId;

    MI_SYS_SetChnOutputPortDepth(&stSrcputPort, 0, 3);
    //MI_SYS_SetChnOutputPortDepth(&stDstputPort, 0, 3);

    ExecFunc(MI_SYS_BindChnPort(&stSrcputPort, &stDstputPort, u32SrcFrmrate, u32DstFrmrate), MI_SUCCESS);
    return 0;
}

int unbind_module(MI_ModuleId_e eSrcModId, MI_U32 u32SrcDevId, MI_U32 u32SrcChnId, MI_U32 u32SrcPortId,
                  MI_ModuleId_e eDstModId, MI_U32 u32DstDevId, MI_U32 u32DstChnId, MI_U32 u32DstPortId)
{
    MI_SYS_ChnPort_t stSrcputPort;
    MI_SYS_ChnPort_t stDstputPort;

    memset(&stSrcputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stSrcputPort.eModId = eSrcModId;
    stSrcputPort.u32DevId = u32SrcDevId;
    stSrcputPort.u32ChnId = u32SrcChnId;
    stSrcputPort.u32PortId = u32SrcPortId;
    memset(&stDstputPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stDstputPort.eModId = eDstModId;
    stDstputPort.u32DevId = u32DstDevId;
    stDstputPort.u32ChnId = u32DstChnId;
    stDstputPort.u32PortId = u32DstPortId;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcputPort, &stDstputPort), MI_SUCCESS);
    return 0;
}


int unbind_vif_vpe(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort, MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort)
{
    return unbind_module(E_MI_MODULE_ID_VIF, 0, VifChn, VifPort,
                         E_MI_MODULE_ID_VPE, 0, VpeChn, VpePort);
}

int bind_vpe_venc(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort, MI_VENC_CHN VencChn)
{
DBG_INFO("bind venc d%d c%d\n", _astChn[VencChn].u32DevId, VencChn);
    return bind_module(E_MI_MODULE_ID_VPE, 0, VpeChn, VpePort, 50,
                       E_MI_MODULE_ID_VENC, _astChn[VencChn].u32DevId, VencChn, 0, 50);
}

int unbind_vpe_venc(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort, MI_VENC_CHN VencChn)
{
    return unbind_module(E_MI_MODULE_ID_VPE, 0, VpeChn, VpePort,
                         E_MI_MODULE_ID_VENC, 0, VencChn, 0);
}
