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
* File name:  module_venc.c
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <execinfo.h>
#include <linux/watchdog.h>

#include "module_common.h"
#include "mi_common.h"
#include "mid_common.h"
#include "mid_VideoEncoder.h"
#include "mid_venc.h"


extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];


MI_S32 venc_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    switch(id)
    {
        case CMD_VENC_GETCHNDEVID:
            break;

        case CMD_VENC_SETMODPARAM:
            break;

        case CMD_VENC_GETMODPARAM:
            break;

        case CMD_VENC_CREATECHN:
            break;
        case CMD_VENC_DESTROYCHN:
            break;

        case CMD_VENC_RESETCHN:
            break;

        case CMD_VENC_STARTRECVPIC:
            break;

        case CMD_VENC_STARTRECVPICEX:
            break;

        case CMD_VENC_STOPTRECVPIC:
            break;

        case CMD_VENC_QUERY:
            break;

        case CMD_VENC_SETCHNATTR:
            break;

        case CMD_VENC_GETCHNATTR:
            break;

        case CMD_VENC_GETSTREAM:
            break;

        case CMD_VENC_RELEASESTREAM:
            break;

        case CMD_VENC_INSERTUSERDATA:
            break;

        case CMD_VENC_SETMAXSTREAMCNT:
            break;

        case CMD_VENC_GETMAXSTREAMCNT:
            break;

        case CMD_VENC_REQUESTIDR:
            break;

        case CMD_VENC_ENABLEIDR:
            break;

        case CMD_VENC_SETH264IDRPICID:
            break;

        case CMD_VENC_GETH264IDRPICID:
            break;

        case CMD_VENC_GETFD:
            break;

        case CMD_VENC_CLOSEFD:
            break;

        case CMD_VENC_SETROICFG:
            break;

        case CMD_VENC_GETROICFG:
            break;

        case CMD_VENC_SETROIBGFRAMERATE:
            break;

        case CMD_VENC_SETH264SLICESPLIT:
            int veChn;
            MI_U32 videoParam[2];

            memcpy(videoParam, param, paramLen);
            veChn = videoParam[0];

            if((0 <= veChn) && (veChn < MAX_VIDEO_NUMBER) && (NULL != g_videoEncoderArray[veChn]))
            {
                switch(g_videoEncoderArray[veChn]->m_encoderType)
                {
                    case VE_AVC:
                        {
                            MI_VENC_ParamH264SliceSplit_t stH264SliceSplit;

                            if(0 == videoParam[1])
                            {
                                stH264SliceSplit.bSplitEnable = 0;
                                stH264SliceSplit.u32SliceRowCount = 0;
                                ExecFunc(MI_VENC_SetH264SliceSplit(g_videoEncoderArray[veChn]->m_veChn, &stH264SliceSplit), MI_SUCCESS);
                            }
                            else
                            {
                                stH264SliceSplit.bSplitEnable = 1;
                                stH264SliceSplit.u32SliceRowCount = MIN(videoParam[1], 5);
                                ExecFunc(MI_VENC_SetH264SliceSplit(g_videoEncoderArray[veChn]->m_veChn, &stH264SliceSplit), MI_SUCCESS);
                            }
                            break;
                        }
                    case VE_H265:
                        {
                            MI_VENC_ParamH265SliceSplit_t stH265SliceSplit;

                            if(0 == videoParam[1])
                            {
                                stH265SliceSplit.bSplitEnable = 0;
                                stH265SliceSplit.u32SliceRowCount = 0;
                                ExecFunc(MI_VENC_SetH265SliceSplit(g_videoEncoderArray[veChn]->m_veChn, &stH265SliceSplit), MI_SUCCESS);
                            }
                            else
                            {
                                stH265SliceSplit.bSplitEnable = 1;
                                stH265SliceSplit.u32SliceRowCount = MIN(videoParam[1], 5);
                                ExecFunc(MI_VENC_SetH265SliceSplit(g_videoEncoderArray[veChn]->m_veChn, &stH265SliceSplit), MI_SUCCESS);
                            }
                            break;
                        }
                    default: break;
                }
            }

        default:
            break;
    }

    return 0;
}
