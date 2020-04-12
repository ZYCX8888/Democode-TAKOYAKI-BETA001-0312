/*
* module_config.h- Sigmastar
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
#ifndef _MODULE_CONFIG_H_
#define _MODULE_CONFIG_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"


#define MIXER_I5_ENABLE         1

#define MIXER_OSD_ENABLE        1
#define MIXER_IE_ENABLE         1
#define MIXER_AED_ENABLE        0
#define MIXER_LSD_ENABLE        0

#if TARGET_CHIP_I6E
#define MIXER_UVC_ENABLE        1
#else
#define MIXER_UVC_ENABLE        0
#endif

#define MIXER_VIF_ENABLE        1
#define MIXER_VPE_ENABLE        1
#define MIXER_VENC_ENABLE       1
#define MIXER_VIDEO_ENABLE      1
#define MIXER_IRCUT_ENABLE      1
#define MIXER_ISP_ENABLE        0
#define MIXER_IQSERVER_ENABLE   1
#define MIXER_ONVIF_ENABLE      1
#define MIXER_LIVE555_ENABLE    1
#define MIXER_PWM_MOTO_ENABLE   0
#define MIXER_CUS3A_ENABLE      1

#if TARGET_CHIP_I6B0
    #define MIXER_SUPPORT_DIVP_BIND_CHANGE      1
#else
    #define MIXER_SUPPORT_DIVP_BIND_CHANGE      0
#endif
#endif //#define _MODULE_CONFIG_H_
