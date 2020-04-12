/*
* OnvifWraper.h- Sigmastar
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
#ifndef _ONVIF_WRAPER_H_
#define _ONVIF_WRAPER_H_


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

int MST_ONVIF_Init();
int MST_ONVIF_DeInit();
int MST_ONVIF_StartTask();
int MST_ONVIF_StopTask();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //_ONVIF_WRAPER_H_
