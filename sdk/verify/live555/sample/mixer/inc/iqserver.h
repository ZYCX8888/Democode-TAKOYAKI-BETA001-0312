/*
* iqserver.h- Sigmastar
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
#ifndef _IQSERVER_MAIN_H
#define _IQSERVER_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
int IQSERVER_Open(int v_width, int v_height, int serverMode);
int IQSERVER_Close();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
