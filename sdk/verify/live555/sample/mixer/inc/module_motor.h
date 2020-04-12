/*
* module_motor.h - Sigmastar
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



#ifndef _MODULE_MOTOR_H
#define _MODULE_MOTOR_H

#include "an41908.h"
int motor_init();
int motor_uninit();
int motor_control();


int mAfFd = -1;
MFZ_LENS ZoomFocus = {0};
#define ABS(x) ((x)<0 ? -(x) : (x))
int zoomCurPos = 0;
int MotoInitFlag = 0;
int Delayms = 500;
#endif

