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
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <string.h>

//#include "mi_rgn.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"

#define TOTAL_SENSOR_NUM (2)

int main(int argc, char **argv)
{
    MI_U8 u8SnrId= 0;
    char s8InsmodString[TOTAL_SENSOR_NUM][128];
    char s8lsmodName[TOTAL_SENSOR_NUM][128];
    char s8SnrPath[50] = "/config/modules/4.9.84/";
    //char s8SnrPath[50] = "/customer/";
    memset(s8InsmodString, 0x0, sizeof(TOTAL_SENSOR_NUM*128));
    memset(s8lsmodName, 0x0, sizeof(TOTAL_SENSOR_NUM*128));
    
    sprintf(s8InsmodString[1], "insmod %s/%s", s8SnrPath, "imx307_MIPI.ko chmap=1");
    sprintf(s8InsmodString[0], "insmod %s/%s", s8SnrPath, "imx291_MIPI.ko chmap=1");

    sprintf(s8lsmodName[1], "rmmod %s", "drv_ms_cus_imx307_MIPI");
    sprintf(s8lsmodName[0], "rmmod %s", "drv_ms_cus_imx291_MIPI");

    do{
        system(s8InsmodString[u8SnrId]);
        if(MI_SNR_Enable(E_MI_SNR_PAD_ID_0) == MI_SUCCESS)
        {
           MI_SNR_Disable(E_MI_SNR_PAD_ID_0);
           return 0;
        }
        else
        {
            system(s8lsmodName[u8SnrId]);
        }
        u8SnrId++;
    }while(u8SnrId < TOTAL_SENSOR_NUM);

    return 0;
}

