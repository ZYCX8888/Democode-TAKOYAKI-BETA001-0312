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

static MI_BOOL bHaveDisplay = FALSE;


int mixer_path_set_all_eframerate(char * ps8Path)
{
    MI_VIF_CHN u32VifChn;
    MI_VIF_PORT u32VifPort;

    DBG_DEBUG("%s\n",ps8Path);

    if(strlen(ps8Path) > MI_VIF_MAX_PHYCHN_NUM*MI_VIF_MAX_CHN_OUTPORT)
    {
        return -1;
    }

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        for(u32VifPort = 0 ; u32VifPort < MI_VIF_MAX_CHN_OUTPORT ; u32VifPort ++)
        {
            if(u32VifChn*2 + u32VifPort >= strlen(ps8Path))
                break;
             mixer_chn_set_framerate(u32VifChn,u32VifPort,ps8Path[u32VifChn*2 + u32VifPort] - '3');
        }
    }

    DBG_EXIT_OK();
    return 0;
}

int mixer_path_set_all(char * ps8Path)
{

    MI_VIF_CHN u32VifChn;
    MI_VIF_PORT u32VifPort;

    DBG_DEBUG("%s\n",ps8Path);

    if(strlen(ps8Path) > MI_VIF_MAX_PHYCHN_NUM*MI_VIF_MAX_CHN_OUTPORT)
    {
        return -1;
    }

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        for(u32VifPort = 0 ; u32VifPort < MI_VIF_MAX_CHN_OUTPORT ; u32VifPort ++)
        {
            if(u32VifChn*2 + u32VifPort >= strlen(ps8Path))
                break;
             mixer_chn_set_path(u32VifChn,u32VifPort,ps8Path[u32VifChn*2 + u32VifPort]);
        }
    }

    DBG_EXIT_OK();
    return 0;
}


int mixer_path_create_all()
{
    MI_VIF_CHN u32VifChn;
    MI_VIF_PORT u32VifPort;
    MI_U8 u8ChnPath;
    MI_BOOL bHaveDisplay;
    DBG_ENTER();

    bHaveDisplay = mixer_path_get_display();

    if(bHaveDisplay)
    {
        mixer_init_disp();
    }

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        for(u32VifPort = 0 ; u32VifPort < MI_VIF_MAX_CHN_OUTPORT ; u32VifPort ++)
        {
            u8ChnPath = mixer_chn_get_path(u32VifChn,u32VifPort);
            DBG_INFO("chn:%d port:%d path:%u %c\n",u32VifChn,u32VifPort,u8ChnPath, u8ChnPath);

            if(u8ChnPath == E_MIXER_CHN_PATH_NONE)
                continue;

            switch (u8ChnPath)
            {
                case E_MIXER_CHN_PATH_VIF:
                {
                    mixer_path_create_vif(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE:
                {
                    mixer_path_create_vif_vpe(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE_DISP:
                {
                	mixer_path_create_vif_vpe_disp(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_DIVP_DISP:
                {
                    mixer_path_create_vif_divp_disp(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE_VENC_H264:
                case E_MIXER_CHN_PATH_VIF_VPE_VENC_H265:
                case E_MIXER_CHN_PATH_VIF_VPE_VENC_JPEG:
                case E_MIXER_CHN_PATH_VIF_VPE_VENC:
                {
                    mixer_path_create_vif_vpe_venc(u32VifChn,u32VifPort);
                }
                break;
                default:
                    break;
            }
        }
    }

    if(bHaveDisplay)
    {
        mixer_init_hdmi();
    }
    return 0;
}

int mixer_path_destroy_all()
{
    MI_VIF_CHN u32VifChn;
    MI_VIF_PORT u32VifPort;
    MI_U8 u8ChnPath;

    for(u32VifChn = 0 ; u32VifChn < MI_VIF_MAX_PHYCHN_NUM ; u32VifChn ++)
    {
        for(u32VifPort = 0 ; u32VifPort < MI_VIF_MAX_CHN_OUTPORT ; u32VifPort ++)
        {
            u8ChnPath = mixer_chn_get_path(u32VifChn,u32VifPort);
            if(u8ChnPath == E_MIXER_CHN_PATH_NONE)
                continue;

            switch (u8ChnPath)
            {
                case E_MIXER_CHN_PATH_VIF:
                {
                    mixer_path_destroy_vif(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE:
                {
                    mixer_path_destroy_vif_vpe(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE_DISP:
                {
                    mixer_path_destroy_vif_vpe_disp(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_DIVP_DISP:
                {
                    mixer_path_destroy_vif_divp_disp(u32VifChn,u32VifPort);
                }
                break;
                case E_MIXER_CHN_PATH_VIF_VPE_VENC:
                {
                    mixer_path_destroy_vif_vpe_venc(u32VifChn,u32VifPort);
                }
                break;
                default:
                    break;
            }
        }
    }


    mixer_deinit_disp();
    return 0;
}


void mixer_path_set_display(MI_BOOL bDisplay)
{
    bHaveDisplay = bDisplay;
}

MI_BOOL mixer_path_get_display(void)
{
    return bHaveDisplay;
}
