/*
* mid_divp.h- Sigmastar
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
#ifndef _MID_DIVP_H
#define _MID_DIVP_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_divp.h"
#include "mid_utils.h"
#include "mid_common.h"

#define MIXER_DIVP_CHANNEL_NUM 7
//divp channel

MI_DIVP_CHN Mixer_Divp_GetChannleNum(void);
void Mixer_Divp_PutChannleNum(MI_DIVP_CHN DivpChn);

MI_S32 Mixer_Divp_CreatChannel(MI_DIVP_CHN DivpChn, MI_SYS_Rotate_e eRoate, MI_SYS_WindowRect_t *pstCropWin);
MI_S32 Mixer_Divp_StartChn(MI_DIVP_CHN DivpChn);
MI_S32 Mixer_Divp_SetOutputAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t *stDivpPortAttr);
MI_S32 Mixer_Divp_GetOutputAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t *pstDivpPortAttr);
MI_S32 Mixer_Divp_GetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t *pstDivpChnAttr);
MI_S32 Mixer_Divp_SetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t stDivpChnAttr);

MI_S32 Mixer_Divp_StopChn(MI_DIVP_CHN DivpChn);
MI_S32 Mixer_Divp_DestroyChn(MI_DIVP_CHN DivpChn);

#endif//_ST_DIVP_H
