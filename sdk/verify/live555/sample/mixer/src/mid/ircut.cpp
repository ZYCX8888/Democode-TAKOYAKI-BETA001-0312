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
#include <pthread.h>


#include "ircut.h"
#include "mid_common.h"
#include "mid_sys.h"
#include "mid_utils.h"

#if TARGET_CHIP_I5
#include "mi_isp_pretzel.h"
#include "mi_isp_pretzel_datatype.h"
#elif TARGET_CHIP_I6
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6E
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6B0
#include "mi_isp.h"
#include "mi_isp_datatype.h"

#endif

static pthread_t g_pthread_ircut;
static BOOL g_ircutExit = FALSE;
int g_ircut_daynight = 1;  // 1 day  0 night
int g_ircut_set_done = 1;  // 1 done  0 no-done

#if TARGET_CHIP_I5
    #define PAD_SAR_GPIO0 145
    #define PAD_SAR_GPIO1 146
    #define PAD_PM_PMIN   120
    #define MIN_LV10X_VALUE 8
    #define MAX_LV10X_VALUE 16

#elif TARGET_CHIP_I6
    #define PAD_SAR_GPIO0 78
    #define PAD_SAR_GPIO1 79
    #define PAD_PM_PMIN   61
    #define MIN_LV10X_VALUE 8
    #define MAX_LV10X_VALUE 16

#elif TARGET_CHIP_I6E
    #define PAD_SAR_GPIO0 26
    #define PAD_SAR_GPIO1 25
    #define PAD_PM_PMIN   24
    #define MIN_LV10X_VALUE 25
    #define MAX_LV10X_VALUE 50

#elif TARGET_CHIP_I6B0
    #define PAD_SAR_GPIO0 78
    #define PAD_SAR_GPIO1 79
    #define PAD_PM_PMIN   61
    #define MIN_LV10X_VALUE 25
    #define MAX_LV10X_VALUE 50
#endif

#define MIXER_CHANGE_IRCUTIN 0

void set_gpio_value(int port,int value)
{
    char cmd[100] = {0};
    char strvalue[2] = {0},strport[4] = {0};

    sprintf(strport,"%d",port);

    sprintf(strvalue,"%d",value);
    printf("ircut_setGPIO port = %s, value = %s\n",strport,strvalue);

    sprintf(cmd,"echo %s > /sys/class/gpio/export",strport);
    system(cmd);
    sprintf(cmd,"echo out > /sys/class/gpio/gpio%s/direction",strport);
    system(cmd);
    sprintf(cmd,"echo %s > /sys/class/gpio/gpio%s/value",strvalue,strport);
    system(cmd);
    sprintf(cmd,"echo %s > /sys/class/gpio/unexport",strport);
    system(cmd);
    return;
}

unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    FILE *fp;
    fp = fopen(path, "r");
    if(fp == NULL)
        return filesize;
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    fclose(fp);
    return filesize;
}


int readFileValue(char *filename,char *out)
{
    if(filename == NULL || out == NULL)
        return -1;

    int filesize = (int)get_file_size(filename);
    if(filesize <= 0)
        return -1;

    FILE *fp = fopen(filename,"rb");
    if(!fp)
        return -1;
    int readSize = fread(out, 1, filesize, fp);

    fclose(fp);
    return readSize;

}
int get_gpio_value(int port)
{
    char cmd[100] = {0};
    char strvalue[2] = {0},strport[4] = {0};
    int value = 0;

    sprintf(strport,"%d",port);

    sprintf(cmd,"echo %s > /sys/class/gpio/export",strport);
    system(cmd);
    sprintf(cmd,"echo in > /sys/class/gpio/gpio%s/direction",strport);
    system(cmd);
    sprintf(cmd,"/sys/class/gpio/gpio%s/value",strport);
    if(readFileValue(cmd,strvalue) <= 0)
    {
        printf("read value fail\n");
        return -1;
    }
    value = atoi(strvalue);

    sprintf(cmd,"echo %s > /sys/class/gpio/unexport",strport);
    system(cmd);

  //  printf("ircut_getGPIO port %d,value %d\n",port,value);
    return value;
}



void* ircut_Task(void *argu)
{

    {  // defautl set black -> white
      set_gpio_value(PAD_SAR_GPIO0,1);
      MySystemDelay(800);
      set_gpio_value(PAD_SAR_GPIO0,0);
      MySystemDelay(200);
      set_gpio_value(PAD_SAR_GPIO1,1);
      MySystemDelay(800);
      set_gpio_value(PAD_SAR_GPIO1,0);
    }

#if (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 0)
    MI_ISP_AE_EXPO_INFO_TYPE_t ExpInfo;
#elif (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 1)
    int irIn_Value = 0;
#endif
    while(!g_ircutExit)
    {
#if (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 0)
        MI_ISP_AE_QueryExposureInfo(0, &ExpInfo);
#elif (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 1)
    irIn_Value = get_gpio_value(PAD_PM_PMIN);
#endif

#if (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 0)
        if ((g_ircut_daynight == 0 && g_ircut_set_done == 0) \
            || (ExpInfo.u32LVx10 < MIN_LV10X_VALUE && g_ircut_daynight == 1 && (int)argu == 1))
#elif (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 1)
        if ((g_ircut_daynight == 0 && g_ircut_set_done == 0) \
            || (irIn_Value == 1 && g_ircut_daynight == 1))
#endif
        {
            set_gpio_value(PAD_SAR_GPIO0,1);
            MySystemDelay(800);
            set_gpio_value(PAD_SAR_GPIO0,0);
            g_ircut_daynight = 0;
            g_ircut_set_done = 1;
        }
#if (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 0)
        else if ((g_ircut_daynight == 1 && g_ircut_set_done == 0)    \
        || (ExpInfo.u32LVx10 > MAX_LV10X_VALUE && g_ircut_daynight == 0 && (int)argu == 1))
#elif (defined(MIXER_CHANGE_IRCUTIN) && MIXER_CHANGE_IRCUTIN == 1)
        else if ((g_ircut_daynight == 1 && g_ircut_set_done == 0)    \
        || (irIn_Value == 0 && g_ircut_daynight == 0))
#endif
        {
            set_gpio_value(PAD_SAR_GPIO1,1);
            MySystemDelay(800);
            set_gpio_value(PAD_SAR_GPIO1,0);
            g_ircut_daynight = 1;
            g_ircut_set_done = 1;
        }
        MySystemDelay(400);
    }

    return NULL;
}


void ircut_open(float frameRate, int option, int b64MMode)
{
    g_ircutExit = FALSE;
    printf("ircut_open frameRate=%f option=%d  b64MMode=%d\n",frameRate,option,b64MMode);
    pthread_create(&g_pthread_ircut, NULL, ircut_Task, (void *)b64MMode);
    pthread_setname_np(g_pthread_ircut , "ircut_Task");

    return;
}

void ircut_close()
{
    void *ptExit;
    g_ircutExit = TRUE;

    pthread_join(g_pthread_ircut, &ptExit);
    return;
}


