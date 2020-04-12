/*
* module_motor.cpp - Sigmastar
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

#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "module_motor.h"
#include "module_common.h"
#include "mid_utils.h"

int AFZoomFocusOpen(void)
{
    mAfFd = open("/dev/AN41908A", 0);
    if(mAfFd<0)
    {
        printf("AFzoomOpen /dev/AN41908A error!\n");
        return -1;
    }
    return 0;
}

int AFZoomFocusClose(void)
{
    if (mAfFd > 0)
    {
        return close(mAfFd);
    }

    return -1;
}

int AFgetFocusCurPos(void)
{
    if (mAfFd > 0)
    {
        if(ioctl(mAfFd, AN41908A_FOCUS_CUR_POS, &ZoomFocus)<(int)0)
		{
			MIXER_ERR("IOCTL faile!\n");
		}
        return ZoomFocus.Motor[MOTOR_FOCUS].pos;
    }
    else
    {
        printf("AFgetFocusCurPos error! %d\n", __LINE__);
        exit(1);
    }
    return -1;
}

int AFsetFocusSteps(int steps, int foscusDir, int loop)
{
    int loopCnt = loop;
    int ret = 0x0;
    MOTOR_INFO  *pFocus = &ZoomFocus.Motor[MOTOR_FOCUS];

    pFocus->dir = foscusDir;

    if (!pFocus->dir)
    {
        pFocus->dir = 1;
    }

    if (foscusDir)
    {
        pFocus->dir = foscusDir;
    }

    if (mAfFd > 0)
    {
        while(loopCnt)
        {
            loopCnt--;
            //focus.focus_steps = MAX_STEP_LONG;
            pFocus->steps = steps;
            ret = ioctl(mAfFd, AN41908A_FOCUS_CMD, &ZoomFocus);
            if(ret < 0)
            {
                printf("[%s] [%d] set focus step is error\n",__func__,__LINE__);
                return -1;
            }

            if ((pFocus->pos <= 0) || (pFocus->pos >= FOCUS_MAX_STEPS))
            {
                pFocus->dir *= -1;
                //printf("next focus dir : %d ; cur pos :%d ; %d\n", focus.focus_dir, focus.focus_pos,__LINE__);
                return -1;
            }

            if (loopCnt >= 0)
            {
                usleep(35000); /* 35ms */
            }
        }

    }
    else
    {
        printf("AFsetZoomSteps error! %d\n", __LINE__);
        exit(1);
    }

    return pFocus->pos;

}

int AFsetFocusPos(int postion)
{
    int focusPosTemp = 0 ;
    int cycles = 0;
    int remainSteps = 0;
    int diffSteps = 0;
    int dirTemp = 0;

    focusPosTemp = AFgetFocusCurPos();
    dirTemp = (focusPosTemp < postion)? FORWARD_DIR: BACKWARD_DIR;
    diffSteps = ABS(focusPosTemp - postion);
    cycles = diffSteps/MAX_STEP_LONG;
    remainSteps = diffSteps%MAX_STEP_LONG;

    if (cycles)
    {
        AFsetFocusSteps(MAX_STEP_LONG, dirTemp, cycles);
    }
    if (remainSteps)
    {
        AFsetFocusSteps(remainSteps, dirTemp, 1);
    }

    return 0;
}



void AFtestFocusToLimit(void)
{
    int focusCurPos = 0;

    focusCurPos = AFgetFocusCurPos();


//    while(focusCurPos != 0)
//    {
//        printf("AFgetFocusCurPos = %d %d\n",focusCurPos,  __LINE__);
//        AFsetFocusSteps(MAX_STEP_LONG, -1, 1);
//        focusCurPos = AFgetFocusCurPos();
//    }
    while(focusCurPos != FOCUS_MAX_STEPS)
    {
        printf("AFgetFocusCurPos = %d %d\n",focusCurPos,  __LINE__);
        AFsetFocusSteps(MAX_STEP_LONG, 1, 1);
        focusCurPos = AFgetFocusCurPos();
    }
    return;
}

int AFocusReset(void)
{
    int ret = -1;

    if (mAfFd > 0)
    {
        ret = ioctl(mAfFd, AN41908A_FOCUS_RESET, &ZoomFocus);
    }
    else
    {
        printf("AFfocusReset mFocusFd error!    %d\n", __LINE__);
    }


    return ret;
}

int AFocusStop(void)
{
    if (mAfFd > 0)
    {
        if(ioctl( mAfFd, AN41908A_FOCUS_STOP, &ZoomFocus )<(int)0)
        {
			MIXER_ERR("IOCTL faile!\n");
		}
    }
    else
    {
        return -1;
    }

    return 0;
}



int AFgetZoomCurPos(void)
{
    if (mAfFd > 0)
    {
        if(ioctl( mAfFd, AN41908A_ZOOM_CUR_POS, &ZoomFocus )<(int)0)
        {
			MIXER_ERR("IOCTL faile!\n");
		}
        return ZoomFocus.Motor[MOTOR_ZOOM].pos;
    }
    else
    {
        return -1;
    }
}

int AFsetZoomSteps(int steps, int zoomDir)
{
    int loopCnt = steps/MAX_STEP_LONG;
    int subSteps = steps%MAX_STEP_LONG;
    MOTOR_INFO *Motor = &ZoomFocus.Motor[MOTOR_ZOOM];

    Motor->dir = zoomDir;

    if (mAfFd > 0)
    {
        while(loopCnt)
        {
            loopCnt--;
            Motor->steps = MAX_STEP_LONG;
            if(ioctl(mAfFd, AN41908A_ZOOM_CMD, &ZoomFocus)<(int)0)
            {
				MIXER_ERR("IOCTL faile!\n");
			}
            printf("AFsetZoomSteps zoom dir : %d ; pos : %d ; %d\n", Motor->dir, Motor->pos, __LINE__);

            if (loopCnt >= 0)
            {
                usleep(35000); /* 30ms */
            }
        }

        Motor->steps = subSteps;
        ioctl(mAfFd, AN41908A_ZOOM_CMD, &ZoomFocus);
        usleep(35000); /* 30ms */
    }
    else
    {
        printf("AFsetZoomSteps error! %d\n", __LINE__);
        exit(1);
    }

    return Motor->pos;

}

int AFzoomSetSpeed(const int speedLevel)
{
    int ret = -1;

    if (mAfFd > 0)
    {
        ZoomFocus.Motor[MOTOR_ZOOM].speed_level = speedLevel;
        ret = ioctl(mAfFd, AN41908A_ZOOM_SET_SPEED, &ZoomFocus);
        if(ret < 0)
            printf("[%s] [%d] AFzoomSetSpeed is error\n",__func__,__LINE__);
    }
    else
    {
        printf("AFzoomSetSpeed mAfFd error! %d\n", __LINE__);
    }

    return ret;
}

int AFzoomStop(void)
{
    int ret  = -1;
    if (mAfFd > 0)
    {
        ret = ioctl( mAfFd, AN41908A_ZOOM_STOP, &ZoomFocus );
        if(ret < 0)
            printf("[%s] [%d] AFzoomStop is error\n",__func__,__LINE__);
    }
    else
    {
        return -1;
    }

    return 0;
}

void AFtestZoomToLimit(void)
{
    int ZoomCurPos = 0;

    ZoomCurPos = AFgetZoomCurPos();


    while(ZoomCurPos != 0)
    {
        printf("ZoomCurPos = %d %d\n", ZoomCurPos,  __LINE__);
        AFsetZoomSteps(MAX_STEP_LONG, -1);
        ZoomCurPos = AFgetZoomCurPos();
    }
    return ;
}

int AFzoomReset(void)
{
    int ret = -1;

    if (mAfFd > 0)
    {
        ret = ioctl(mAfFd, AN41908A_ZOOM_RESET, &ZoomFocus);
    }
    else
    {
        printf("AFzoomReset mAfFd error!    %d\n", __LINE__);
    }


    return ret;
}

int motor_init()
{
   if(AFZoomFocusOpen()<0)
   {
       printf("[%s] [%d] ERROR!! AFZoomFocusOpen failed!\n",__func__,__LINE__);
       return -1;
   }

   AFtestFocusToLimit();
   AFtestZoomToLimit();

   if(AFzoomReset()<0)
   {
       printf("[%s] [%d] ERROR!! AFzoomReset failed!\n",__func__,__LINE__);
       return -1;
   }

   if(AFocusReset()<0)
   {
       printf("[%s] [%d] ERROR!! AFocusReset failed!\n",__func__,__LINE__);
       return -1;
   }

   return 0;
}


int motor_uninit()
{
    if(AFzoomStop() < 0)
    {
        printf("[%s] [%d] ERROR!! AFzoomStop failed!\n",__func__,__LINE__);
        return -1;
    }
    if(AFocusStop() < 0)
    {
        printf("[%s] [%d] ERROR!! AFocusStop failed!\n",__func__,__LINE__);
        return -1;
    }

    if(AFZoomFocusClose() < 0)
    {
        printf("[%s] [%d] ERROR!! AFZoomFocusClose failed!\n",__func__,__LINE__);
        return -1;
    }

    printf("[%s] [%d] motor_uninit success\n",__func__,__LINE__);
    return 0;
}


int motor_control(int conStatus)
{
   int ret = -1;
   switch(conStatus)
   {
       case 0:
            ret = ioctl(mAfFd, AN41908A_ZOOM_IN, &ZoomFocus);
            if(ret < 0)
                printf("[%s] [%d]set zoom in is error\n",__func__,__LINE__);
            break;

       case 1:
            ret = ioctl(mAfFd, AN41908A_ZOOM_OUT, &ZoomFocus);
            if(ret < 0)
                printf("[%s] [%d]set zoom out is error\n",__func__,__LINE__);
            break;

       case 2:
            ret = ioctl(mAfFd, AN41908A_FOCUS_FAR, &ZoomFocus);
            if(ret < 0)
               printf("[%s] [%d]set focus far is error\n",__func__,__LINE__);
            break;

       case 3:
            ret = ioctl(mAfFd, AN41908A_FOCUS_NEAR, &ZoomFocus);
            if(ret < 0)
                printf("[%s] [%d]set focus near is error\n",__func__,__LINE__);
            break;

       default:
            printf("the set control status [%d] is error,please input again!!!\n",conStatus);
            break;

   }

   /* printf("== zoom position : %d ; speed : %d;  focus pos:%d, speed:%d \n", \
                ZoomFocus.Motor[MOTOR_ZOOM].pos, ZoomFocus.Motor[MOTOR_ZOOM].speed,\
                ZoomFocus.Motor[MOTOR_FOCUS].pos, ZoomFocus.Motor[MOTOR_FOCUS].speed);  */

   MySystemDelay(Delayms);
   if(AFzoomStop() < 0)
   {
       printf("[%s] [%d] ERROR!! AFzoomStop failed!\n",__func__,__LINE__);
       return -1;
   }
   if(AFocusStop() < 0)
   {
       printf("[%s] [%d] ERROR!! AFocusStop failed!\n",__func__,__LINE__);
       return -1;
   }

   return ret;
}

int af_pos1 = 0;
int af_zoom1 = 0;

int motor_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen)
{
    switch(id)
    {
        case CMD_MOTOR_INIT:
            if(MotoInitFlag == 1)
            {
                printf("[%s][%d] AF moto is  init!!! don't init again!!\n",__func__,__LINE__);
                break;
            }
            if(motor_init() < 0)
            {
                printf("[%s][%d] AF moto is init error\n",__func__,__LINE__);
                break;
            }
            MotoInitFlag = 1;
            break;

        case CMD_MOTOR_UNINIT:
            if(MotoInitFlag == 0)
            {
                printf("[%s][%d] AF moto is not init!!! Please init AF moto first!!\n",__func__,__LINE__);
                break;
            }
            if(motor_uninit() < 0)
            {
                printf("[%s][%d] AF moto is uinit error\n",__func__,__LINE__);
                break;
            }
            MotoInitFlag = 0;
            break;

        case CMD_MOTOR_CONTROL:
            if(MotoInitFlag == 0)
            {
                //printf("[%s][%d] AF moto is not init!!! Please init AF moto first!!\n",__func__,__LINE__);
                break;
            }
            int controlStaus;
            memcpy(&controlStaus,param,paramLen);

            if(motor_control(controlStaus) < 0)
                ;//printf("[%s][%d] AF control AF moto is error\n,control status is %d\n",__func__,__LINE__,controlStaus);
            else
                ;//printf("[%s][%d] AF control AF moto is success\n,control status is %d\n",__func__,__LINE__,controlStaus);

            af_pos1 = AFgetZoomCurPos();
            af_zoom1 = AFgetFocusCurPos();

            //if (param[0]==2 || param[0]==3)
            printf("\n[moto]pos:%d,zoom:%d",af_pos1, af_zoom1);
            break;

         case CMD_MOTOR_DelayMs:
                memcpy(&Delayms,param,paramLen);
                //printf("[%s] [%d] set delayms is %d,when set step use it\n",__func__,__LINE__,Delayms);
            break;

         default:
            printf("[%s][%d] this AF moto command is not valid!!!!\n",__func__,__LINE__);
            break;
    }
    return 0;
}
