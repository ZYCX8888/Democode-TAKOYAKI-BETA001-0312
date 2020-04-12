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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pwm_moto.h"

extern int pwm_groud[11];

void group_mode_in_out(int pwm_id,int flag)
{
    char cmd[100] = {0};

    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_mode",pwm_id,flag);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_begin(int pwm_id,int begin_value)
{
    char cmd[100] = {0};

    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_begin",pwm_id,begin_value);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_end(int pwm_id,int end_value)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_end",pwm_id,end_value);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_period(int pwm_id,int period_value)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_period",pwm_id,period_value);
    printf("%s\n",cmd);
    system(cmd);
    return;

}


void group_mode_polority(int pwm_id,int enable)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_polarity",pwm_id,enable);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_round(int group_id,int round_value)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_round",group_id,round_value);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_enable_disable(int group_id,int enable)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_enable",group_id,enable);
    printf("%s\n",cmd);
    system(cmd);
    return;

}

void group_mode_stop(int group_id,int enable)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d > /sys/devices/virtual/mstar/motor/group_stop",group_id);
    printf("%s\n",cmd);
    system(cmd);
    return;
}

void group_mode_hold(int group_id,int enable)
{
    char cmd[100] = {0};
    sprintf(cmd,"echo %d %d > /sys/devices/virtual/mstar/motor/group_hold",group_id,enable);
    printf("%s\n",cmd);
    system(cmd);
    return;
}


void pwm_config_group_in(int          group_id)
{
    int i =0;
    if(group_id == 0)
    {

        for(i=0; i<4; i++)
            if(pwm_groud[i] == 1)
               group_mode_in_out(i,1);
            else if (pwm_groud[i] == 0)
               group_mode_in_out(i,0);
    }
    else if(group_id == 1)
    {

        for(i=4; i<8; i++)
            if(pwm_groud[i] == 1)
               group_mode_in_out(i,1);
            else if (pwm_groud[i] == 0)
               group_mode_in_out(i,0);
    }
    else if(group_id == 2)
    {

        for(i=8; i<11; i++)
            if(pwm_groud[i] == 1)
               group_mode_in_out(i,1);
            else if (pwm_groud[i] == 0)
               group_mode_in_out(i,0);
    }
    return;
}

void pwm_config_param(int group_id,char *param)
{
   int config_param[17],i=0;

   memset(config_param,0,sizeof(config_param));

   memcpy(config_param,param,sizeof(config_param));

   printf("group_id:%d\n",group_id);
   if(group_id == 0)
   {
       int current_pwm = 0;
       for(i=0;i<4;i++)
       {
        if(pwm_groud[i] == 1)
            {
                 group_mode_period(i,config_param[current_pwm]);
                 group_mode_polority(i,config_param[current_pwm+1]);
                 group_mode_begin(i,config_param[current_pwm+2]);
                 group_mode_end(i,config_param[current_pwm+3]);
                 current_pwm = current_pwm + 4;
            }
       }
   }


   if(group_id == 1)
   {
       int current_pwm = 0;
       for(i=4;i<8;i++)
       {
        if(pwm_groud[i] == 1)
            {
                 group_mode_period(i,config_param[current_pwm]);
                 group_mode_polority(i,config_param[current_pwm+1]);
                 group_mode_begin(i,config_param[current_pwm+2]);
                 group_mode_end(i,config_param[current_pwm+3]);
                 current_pwm = current_pwm + 4;
            }
        }
   }

   if(group_id == 2)
   {
       int current_pwm = 0;
       for(i=8;i<11;i++)
       {
        if(pwm_groud[i] == 1)
            {
                 group_mode_period(i,config_param[current_pwm]);
                 group_mode_polority(i,config_param[current_pwm+1]);
                 group_mode_begin(i,config_param[current_pwm+2]);
                 group_mode_end(i,config_param[current_pwm+3]);
                 current_pwm = current_pwm + 4;
            }
       }
   }

   group_mode_round(group_id,config_param[16]);
   return;
}

