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
#include "mixer.h"

int mixer_path_create_vif(MI_VIF_CHN u32VifChn, MI_VIF_PORT u32VifPort)
{
    MixerVideoChnInfo_t stChnInfo;
    DBG_ENTER();

    mixer_chn_get_video_info(u32VifChn,&stChnInfo);
    if(stChnInfo.eformat == 255)
        return -1;

    ExecFunc(mixer_chn_vif_create(u32VifChn, u32VifPort, &stChnInfo), 0);

    if(stChnInfo.bDump[u32VifPort])
    {
        ExecFunc(mixer_chn_vif_start(u32VifChn, u32VifPort), 0);
    }
    DBG_EXIT_OK();
    return 0;
}

int mixer_path_destroy_vif(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MixerVideoChnInfo_t stChnInfo;
    DBG_ENTER();

    mixer_chn_get_video_info(u32VifChn,&stChnInfo);
    if(stChnInfo.eformat == 255)
        return -1;

    if(stChnInfo.bDump[u32VifPort])
    {
        ExecFunc(mixer_chn_vif_stop(u32VifChn, u32VifPort), 0);
    }

    ExecFunc(mixer_chn_vif_destroy(u32VifChn, u32VifPort), 0);

    return 0;
}

