/*
* debug.h- Sigmastar
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

//#include "mi_venc.h"
#ifndef _IRCUT_H_
#define _IRCUT_H_


#ifdef __cplusplus

extern "C"
{

#endif /* __cplusplus */

void ircut_open(float frameRate, int option, int b64MMode);
void ircut_close();

#ifdef __cplusplus

}

#endif /* __cplusplus */

#endif //_IRCUT_H_
