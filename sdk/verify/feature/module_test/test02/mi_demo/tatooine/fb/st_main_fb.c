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
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp.h"
#include "st_fb.h"
#include "st_common.h"
#include "st_disp.h"

MI_S32 main(MI_S32 argc, MI_U8 **argv)
{
    MI_SYS_WindowRect_t Rect;
    ST_Rect_t stVdispOutRect;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;

    ST_Sys_Init();

    // init hdmi
    STCHECKRESULT(ST_Hdmi_Init());
    STCHECKRESULT(ST_GetTimingInfo(E_ST_TIMING_1080P_60,
                    &s32HdmiTiming, &s32DispTiming, &stVdispOutRect.u16PicW, &stVdispOutRect.u16PicH));

    ST_Disp_DevInit(ST_DISP_DEV0, s32DispTiming);
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32HdmiTiming));

    ST_Fb_Init();
    Rect.u16X = 100;
    Rect.u16Y = 100;
    Rect.u16Width = 200;
    Rect.u16Height = 200;

    ST_Fb_FillRect(&Rect, 0xffff0000);
    Rect.u16X = 400;
    Rect.u16Y = 100;
    Rect.u16Width = 200;
    Rect.u16Height = 200;
    ST_Fb_FillRect(&Rect, 0xffb8860b);
    Rect.u16X = 800;
    Rect.u16Y = 100;
    Rect.u16Width = 200;
    Rect.u16Height = 200;
    ST_Fb_FillRect(&Rect, 0xff0000ff);
    sleep(2);
    printf("Yellow Yellow Key\n");
    sleep(2);
    printf("Yellow Yellow Key\n");
    sleep(2);
    ST_Fb_SetColorKey(0xffb8860b);

#if 1
    ST_Fb_InitMouse(44, 56, 4, "/mnt/cursor.raw");
    ST_Fb_MouseSet(500, 500);
    while (1)
    {
        ST_Fb_MouseSet(rand()%1000, rand()%1000);

        usleep(100*1000);
    }
#endif

    return 0;
}
