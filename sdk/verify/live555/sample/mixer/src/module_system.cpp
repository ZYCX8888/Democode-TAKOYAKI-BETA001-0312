/*
* module_ie.cpp- Sigmastar
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
/*************************************************
*
* File name:  module_system.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: Mixer system module source file
*
*
*
* History:
*
*    1. Date  :        2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <execinfo.h>

#include "module_common.h"
#include "module_config.h"
#include "mid_common.h"

#if TARGET_CHIP_I5
#include "mi_isp_pretzel.h"
#include "mi_isp_pretzel_datatype.h"
#include "mi_iqserver_pretzel.h"
#elif TARGET_CHIP_I6
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#include "mi_iqserver.h"
#elif TARGET_CHIP_I6E
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#include "mi_iqserver.h"
#elif TARGET_CHIP_I6B0
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#include "mi_iqserver.h"
#endif

#include "mi_sys.h"
#include "ircut.h"
#include "OnvifWraper.h"
#include "mid_VideoEncoder.h"
#include "live555.h"
#include "mi_sensor.h"
#include "pwm_moto.h"
#include "isp_cus3a_if.h"

#include "module_cus3a.h"

#define  WATCHDOG_IOCTL_BASE          'W'
#define  WDIOC_SETTIMEOUT             _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define  WDIOC_KEEPALIVE              _IOR(WATCHDOG_IOCTL_BASE, 5, int)


live555 *g_live555Server = NULL;

extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];
extern MI_U32 g_videoNumber;

extern MI_U32 g_ircut_daynight;
extern MI_U32 g_ircut_set_done;

//*****************************************************************************************************/
// Customer 3A
//*****************************************************************************************************/
#if MIXER_CUS3A_ENABLE

#define ENABLE_DOAE_MSG        (0)
#define ENABLE_DOAWB_MSG    (0)
#define ENABLE_DOAF_MSG        (0)

#define SHUTTER_GAIN_DELAY_TEST1   (0)
#define SHUTTER_GAIN_DELAY_TEST2   (0)
#define SHUTTER_TEST                               (0)
#define GAIN_TEST                                      (0)
#define AE_EXAMPLE                                   (1)

int mod_isp_ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state);
void mod_isp_ae_release(void* pdata);
void mod_isp_ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result);
int mod_isp_awb_init(void *pdata);
void mod_isp_awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result);
void mod_isp_awb_release(void *pdata);

int af_init(void *pdata, ISP_AF_INIT_PARAM *param)
{
    MI_U32 u32ch = 0;
    MI_U8 u8win_idx = 16;
    CusAFRoiMode_t taf_roimode;

    printf("************ af_init **********\n");

#if 1

    //Init Normal mode setting
    taf_roimode.mode = AF_ROI_MODE_NORMAL;
    taf_roimode.u32_vertical_block_number = 1;
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {
        { 0, {   0,    0,  255,  255}},
        { 1, { 256,    0,  511,  255}},
        { 2, { 512,    0,  767,  255}},
        { 3, { 768,    0, 1023,  255}},
        { 4, {   0,  256,  255,  511}},
        { 5, { 256,  256,  511,  511}},
        { 6, { 512,  256,  767,  511}},
        { 7, { 768,  256, 1023,  511}},
        { 8, {   0,  512,  255,  767}},
        { 9, { 256,  512,  511,  767}},
        {10, { 512,  512,  767,  767}},
        {11, { 768,  512, 1023,  767}},
        {12, {   0,  768,  255, 1023}},
        {13, { 256,  768,  511, 1023}},
        {14, { 512,  768,  767, 1023}},
        {15, { 768,  768, 1023, 1023}}
    };
    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }

#else
    //Init Matrix mode setting
    taf_roimode.mode = AF_ROI_MODE_MATRIX;
    taf_roimode.u32_vertical_block_number = 16; //16xN, N=16
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {

        //full image, equal divide to 16x16
        {0, {0, 0, 63, 63}},
        {1, {64, 64, 127, 127}},
        {2, {128, 128, 191, 191}},
        {3, {192, 192, 255, 255}},
        {4, {256, 256, 319, 319}},
        {5, {320, 320, 383, 383}},
        {6, {384, 384, 447, 447}},
        {7, {448, 448, 511, 511}},
        {8, {512, 512, 575, 575}},
        {9, {576, 576, 639, 639}},
        {10, {640, 640, 703, 703}},
        {11, {704, 704, 767, 767}},
        {12, {768, 768, 831, 831}},
        {13, {832, 832, 895, 895}},
        {14, {896, 896, 959, 959}},
        {15, {960, 960, 1023, 1023}}

        /*
        //use two row only => 16x2 win
        {0, {0, 0, 63, 63}},
        {1, {64, 64, 127, 127}},
        {2, {128, 0, 191, 2}},        //win2 v_str, v_end doesn't use, set to (0, 2)
        {3, {192, 0, 255, 2}},
        {4, {256, 0, 319, 2}},
        {5, {320, 0, 383, 2}},
        {6, {384, 0, 447, 2}},
        {7, {448, 0, 511, 2}},
        {8, {512, 0, 575, 2}},
        {9, {576, 0, 639, 2}},
        {10, {640, 0, 703, 2}},
        {11, {704, 0, 767, 2}},
        {12, {768, 0, 831, 2}},
        {13, {832, 0, 895, 2}},
        {14, {896, 0, 959, 2}},
        {15, {960, 0, 1023, 2}}
        */
    };

    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }
#endif

#if TARGET_CHIP_I6E
    //set AF Filter
    static CusAFFilter_t affilter =
    {
        //filter setting with sign value
        //{s9, s10, s9, s7, s7}
        //high: 37, 0, -37, 83, 40; 37, 0, -37, -54, 34; 32, 0, -32, 14, 0
        //low:  15, 0, -15, -79, 44; 15, 0, -15, -115, 55; 14, 0, -14, -91, 37

        //convert to hw format (sign bit with msb)
        37, 0, 37+512, 83, 40, 0, 1023, 0, 1023,    //high
        15, 0, 15+512, 79+128, 44, 0, 1023, 0, 1023,    //low
        1, 37, 0, 37+512,  54+128, 34, 1, 32, 0, 32+512, 14, 0,    //high-e1, e2
        1, 15, 0, 15+512, 115+128, 55, 1, 14, 0, 14+512, 91+128, 37            //low-e1, e2
    };
#else
    //set AF Filter
    static CusAFFilter_t affilter =
    {
        //filter setting with sign value
        //{63, -126, 63, -109, 48, 0, 320, 0, 1023},
        //{63, -126, 63, 65, 55, 0, 320, 0, 1023}

        //convert to hw format (sign bit with msb)
        63, 126 + 1024, 63, 109 + 128, 48, 0, 320, 0, 1023,
        63, 126 + 1024, 63, 65, 55, 0, 320, 0, 1023,
    };
#endif

    MI_ISP_CUS3A_SetAFFilter(0, &affilter);

    //set AF Sq
/*    CusAFFilterSq_t sq = {
        .bSobelYSatEn = 0,
        .u16SobelYThd = 1023,
        .bIIRSquareAccEn = 1,
        .bSobelSquareAccEn = 0,
        .u16IIR1Thd = 0,
        .u16IIR2Thd = 0,
        .u16SobelHThd = 0,
        .u16SobelVThd = 0,
        .u8AFTblX = {6,7,7,6,6,6,7,6,6,7,6,6,},
        .u16AFTblY = {0,1,53,249,431,685,1023,1999,2661,3455,5487,6749,8191},
    };

    MI_ISP_CUS3A_SetAFFilterSq(0, &sq);
*/
    return 0;
}

void af_release(void *pdata)
{
    printf("************ af_release **********\n");
}

void af_run(void *pdata, const ISP_AF_INFO *af_info, ISP_AF_RESULT *result)
{
#if (ENABLE_DOAF_MSG)
    int i = 0, x = 0;

    printf("\n\n");

    //print row0 16wins
    x = 0;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }

    //print row15 16wins
    x = 15;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }
#endif
}

int af_ctrl(void *pdata, ISP_AF_CTRL_CMD cmd, void* param)
{
    return 0;
}
//*****************************************************************************************************/
// Customer 3A END
//*****************************************************************************************************/
#endif //end of MIXER_CUS3A_ENABLE
MI_S32 system_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
  //  static int bCus3AEn = 0;
    switch(id)
    {
        case CMD_SYSTEM_INIT:
            MIXERCHECKRESULT(Mixer_Sys_Init());
            break;

        case CMD_SYSTEM_UNINIT:
            MIXERCHECKRESULT(Mixer_Sys_Exit());
            break;

        case CMD_SYSTEM_WATCHDOG_OPEN:
            watchDogInit();
            break;

        case CMD_SYSTEM_WATCHDOG_CLOSE:
            watchDogUninit();
            break;

#if MIXER_IRCUT_ENABLE
        case CMD_SYSTEM_IRCUT_OPEN:
            {
                int iqparam[3];
                memcpy(iqparam, param, sizeof(iqparam));
                ircut_open((float)iqparam[0] / 1000.0, iqparam[1], iqparam[2]);
                printf("ircut_open fps:%d mode:%d\n", iqparam[0], iqparam[1]);
            }
            break;

        case CMD_SYSTEM_IRCUT_CLOSE:
            ircut_close();
            break;

        case CMD_SYSTEM_IRCUT_WHITE:
            g_ircut_daynight = 1;
            g_ircut_set_done = 0;
            printf("set ircut daynight:%d\n", g_ircut_daynight);
            break;

        case CMD_SYSTEM_IRCUT_BLACK:
            g_ircut_daynight = 0;
            g_ircut_set_done = 0;
            printf("set ircut daynight:%d\n", g_ircut_daynight);
            break;
#endif

#if MIXER_IQSERVER_ENABLE
        case CMD_SYSTEM_IQSERVER_OPEN:
            {
                int iqparam[1];
                int width = 0;
                int height = 0;
                MI_S32 ChnId = 0;
                MI_SNR_PlaneInfo_t stSnrPlane0Info;

                memcpy(iqparam, param, sizeof(iqparam));

                memset(&stSnrPlane0Info, 0x00, sizeof(MI_SNR_PlaneInfo_t));
                ExecFunc(MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info), MI_SUCCESS);
                width  = stSnrPlane0Info.stCapRect.u16Width;
                height = stSnrPlane0Info.stCapRect.u16Height;
                ChnId  = iqparam[0];
                printf("width:%d, height:%d, vpeChn:%d\n", width,height, ChnId);

                ExecFunc(MI_IQSERVER_Open(width, height, ChnId), MI_SUCCESS);
            }
            break;

        case CMD_SYSTEM_IQSERVER_CLOSE:
            MI_IQSERVER_Close();
            break;
#endif

#if MIXER_ONVIF_ENABLE
        case CMD_SYSTEM_ONVIF_OPEN:
            MST_ONVIF_Init();
            MST_ONVIF_StartTask();
            break;

        case CMD_SYSTEM_ONVIF_CLOSE:
            MST_ONVIF_StopTask();
            MST_ONVIF_DeInit();
            break;
#endif

        case CMD_SYSTEM_LIVE555_OPEN:
            {
                char streamName[8];

                g_live555Server = live555::createNew();

                if(g_live555Server)
                {
                    //sample: "video0" "video1"
                    for(MI_U32 i = 0; i < g_videoNumber ; i++)
                    {
                        if((g_videoEncoderArray[i]->m_encoderType < VE_JPG_YUV422) && (g_videoEncoderArray[i]->m_encoderType != VE_YUV420))
                        {
                            sprintf(streamName, "video%d", i);

                            if(MI_AudioEncoder::g_s32AudioInNum > i)
                            {
                                g_live555Server->createServerMediaSession(g_videoEncoderArray[i], MI_AudioEncoder::g_pAudioEncoderArray[i], streamName, i);
                            }
                            else
                            {
                                g_live555Server->createServerMediaSession(g_videoEncoderArray[i], NULL, streamName, i);
                            }
                        }
                    }

                    g_live555Server->startLive555Server();
                }
            }
            break;

        case CMD_SYSTEM_LIVE555_SET_FRAMERATE:
            {
                int framerate;
                memcpy(&framerate, param, sizeof(framerate));

                if(g_live555Server)
                {
                    g_live555Server->setFrameRate(framerate);
                }
            }
            break;

        case CMD_SYSTEM_LIVE555_STOP_SUBMEDIASESSION:
            {
                MI_U32 veChn = *param;
                if(g_live555Server && veChn < g_videoNumber)
                {
                    g_live555Server->stopVideoSubMediaSession(veChn, g_videoEncoderArray[veChn]);
                }
            }
            break;
        case CMD_SYSTEM_LIVE555_SET_ENCODETYPE:
            {
                MI_U32 veChn = *param;
                if(g_live555Server && veChn < g_videoNumber)
                {
                    g_live555Server->RestartVideoSubMediaSession(veChn, g_videoEncoderArray[veChn],MI_AudioEncoder::g_pAudioEncoderArray[veChn]);
                }
            }
            break;

        case CMD_SYSTEM_LIVE555_CLOSE:
            {
                if(g_live555Server)
                {
                    g_live555Server->stopLive555Server();
                    delete g_live555Server;
                }
            }
            break;

        case CMD_SYSTEM_CORE_BACKTRACE:
            {
                struct sigaction sa;

                memset(&sa, 0, sizeof(sa));
                sigemptyset(&sa.sa_mask);
                sa.sa_sigaction = segfault_sigaction;
                sa.sa_flags = SA_SIGINFO;

                sigaction(SIGSEGV, &sa, NULL);
                sigaction(SIGILL, &sa, NULL);
                sigaction(SIGABRT, &sa, NULL);
                sigaction(SIGBUS, &sa, NULL);
            }
            break;
#if MIXER_PWM_MOTO_ENABLE
        case CMD_SYSTEM_PWM_MOTO_GROUP:
            pwm_config_group_in(*param);
            break;
        case CMD_SYSTEM_PWM_MOTO_PARAM:
            int setparam[18];
            memcpy(setparam,param,sizeof(setparam));
            pwm_config_param(setparam[17],(char *)param);
            break;
        case CMD_SYSTEM_PWM_MOTO_ENABLE:
            int eparam[2];
            memcpy(eparam,param,sizeof(eparam));
            group_mode_enable_disable(eparam[0],eparam[1]);
            break;
        case CMD_SYSTEM_PWM_MOTO_HSTOP:
            int hsparam[3];
            memcpy(hsparam,param,sizeof(hsparam));
            if(hsparam[1] == 0)
                group_mode_hold(hsparam[0],hsparam[2]);
            else if(hsparam[1] == 1)
                group_mode_stop(hsparam[0],1);
            break;
#endif

        default:
            break;
    }

    return 0;
}
