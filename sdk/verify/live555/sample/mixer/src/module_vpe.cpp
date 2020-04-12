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
* File name:  module_vpe.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: vpe module source file
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <execinfo.h>
#include <linux/watchdog.h>

#include "module_common.h"
#include "mi_common.h"
#include "mid_vpe.h"



MI_S32 vpe_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    switch(id)
    {
        case CMD_VPE_CREATECHANNEL:
            break;

        case CMD_VPE_DESTROYCHANNEL:
            break;

        case CMD_VPE_SETCHANNELATTR:
            break;

        case CMD_VPE_GETCHANNELATTR:
            break;

        case CMD_VPE_STARTCHANNEL:
            break;

        case CMD_VPE_STOPCHANNEL:
            break;

        case CMD_VPE_SETCHANNELPARAM:
            break;

        case CMD_VPE_GETCHANNELPARAM:
            break;

        case CMD_VPE_SETCHANNELCROP:
            break;

        case CMD_VPE_GETCHANNELCROP:
            break;

        case CMD_VPE_GETCHANNELREGIONLUMA:
            break;

        case CMD_VPE_SETCHANNELROTATION:
            break;

        case CMD_VPE_GETCHANNELROTATION:
            break;

        case CMD_VPE_ENABLEPORT:
            break;

        case CMD_VPE_DISABLEPORT:
            break;

        case CMD_VPE_SETPORTMODE:
            break;

        case CMD_VPE_GETPORTMODE:
            break;

        default:
            break;
    }

    return 0;
}
