/*
* cycle_buffer.cpp- Sigmastar
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
* File name:  mid_common.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: common module source file
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
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <execinfo.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <errno.h>


#include "mid_common.h"
#include "mid_utils.h"
#include "mi_sys_datatype.h"

#include "mi_md.h"



static int g_watchdog = 0;
static pthread_t pthreadWatchdog;
int g_dbglevel = MIXER_DBGLV_DEBUG;
static BOOL g_bHdr = FALSE;
static BOOL g_ShowFrameInterval = FALSE;
static BOOL g_displayOsd = FALSE;
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
MI_S32 Mixer_get_thread_policy(pthread_attr_t *attr)
{
    int policy;
    int rs = pthread_attr_getschedpolicy(attr, &policy);
    assert (rs == 0);
    switch (policy)
    {
        case SCHED_FIFO:
            MIXER_DBG("policy = SCHED_FIFO\n");
            break;
        case SCHED_RR:
            MIXER_DBG("policy = SCHED_RR");
            break;
        case SCHED_OTHER:
            MIXER_DBG("policy = SCHED_OTHER\n");
            break;
        default:
            MIXER_DBG("policy = UNKNOWN\n");
            break; 
    }
    return policy;

}
void Mixer_show_thread_priority(pthread_attr_t *attr,int policy)
{
    int priority = sched_get_priority_max(policy);
    assert (priority != -1);
    MIXER_DBG("max_priority = %d\n", priority);
    priority = sched_get_priority_min (policy);
    assert (priority != -1);
    MIXER_DBG("min_priority = %d\n", priority);
}

MI_S32 Mixer_get_thread_priority(pthread_attr_t *attr)
{
    struct sched_param param;
    int rs = pthread_attr_getschedparam(attr, &param);
    assert (rs == 0);
    MIXER_DBG("priority = %d\n", param.__sched_priority);
    return param.__sched_priority;
}

void Mixer_set_thread_policy(pthread_attr_t *attr,int policy)
{
    MI_S32 rs = pthread_attr_setschedpolicy(attr, policy);
    assert (rs == 0);
    Mixer_get_thread_policy(attr);
}

PlatformType_e mixerGetPlatformType()
{
    PlatformType_e type = PLATFORM_TYPE_318;
    char platformType[128];
    FILE *pFd = NULL;

    pFd = fopen("/sys/devices/soc0/machine", "r");

    if(NULL == pFd)
    {
        printf("%s: warning, open /sys/devices/soc0/machine failed\n", __func__);
        return type;
    }

    platformType[0] = '\0';
    fgets(platformType, 128, pFd);

    if(strstr(platformType, "64M"))
    {
        type = PLATFORM_TYPE_313E;
    }

    fclose(pFd);
    pFd = NULL;

    printf("%s: platform type is %d\n", __func__, type);
    return type;
}


MI_S32 Mixer_Sys_Bind(Mixer_Sys_BindInfo_T *pstBindInfo)
{
#if TARGET_CHIP_I5
    printf("%s:%d bind src(%d,%d,%d,%d), dst(%d,%d,%d,%d), SrcFps=%d, DstFps=%d\n",__func__,__LINE__,
                           pstBindInfo->stSrcChnPort.eModId, pstBindInfo->stSrcChnPort.u32DevId,
                           pstBindInfo->stSrcChnPort.u32ChnId, pstBindInfo->stSrcChnPort.u32PortId,
                           pstBindInfo->stDstChnPort.eModId, pstBindInfo->stDstChnPort.u32DevId,
                           pstBindInfo->stDstChnPort.u32ChnId, pstBindInfo->stDstChnPort.u32PortId,
                           pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate);

    ExecFuncWARNING(MI_SYS_BindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort,   \
                                 pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate), \
                                 MI_SUCCESS);
#elif TARGET_CHIP_I6
    printf("%s:%d bind src: (%d,%d,%d,%d), dst:(%d,%d,%d,%d), SrcFps=%d DstFps=%d, BindType=%d BindParam=%d\n", __func__,__LINE__,
                       pstBindInfo->stSrcChnPort.eModId, pstBindInfo->stSrcChnPort.u32DevId,
                       pstBindInfo->stSrcChnPort.u32ChnId, pstBindInfo->stSrcChnPort.u32PortId,
                       pstBindInfo->stDstChnPort.eModId, pstBindInfo->stDstChnPort.u32DevId,
                       pstBindInfo->stDstChnPort.u32ChnId, pstBindInfo->stDstChnPort.u32PortId,
                       pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate,
                       pstBindInfo->eBindType, pstBindInfo->u32BindParam);

    ExecFuncWARNING(MI_SYS_BindChnPort2(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
                                 pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate, \
                                 pstBindInfo->eBindType, pstBindInfo->u32BindParam),     \
                                 MI_SUCCESS);
#elif TARGET_CHIP_I6E
        printf("%s:%d bind src: (%d,%d,%d,%d), dst:(%d,%d,%d,%d), SrcFps=%d DstFps=%d, BindType=%d BindParam=%d\n", __func__,__LINE__,
                       pstBindInfo->stSrcChnPort.eModId, pstBindInfo->stSrcChnPort.u32DevId,
                       pstBindInfo->stSrcChnPort.u32ChnId, pstBindInfo->stSrcChnPort.u32PortId,
                       pstBindInfo->stDstChnPort.eModId, pstBindInfo->stDstChnPort.u32DevId,
                       pstBindInfo->stDstChnPort.u32ChnId, pstBindInfo->stDstChnPort.u32PortId,
                       pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate,
                       pstBindInfo->eBindType, pstBindInfo->u32BindParam);

    ExecFuncWARNING(MI_SYS_BindChnPort2(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
                                 pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate, \
                                 pstBindInfo->eBindType, pstBindInfo->u32BindParam),     \
                                 MI_SUCCESS);
#elif TARGET_CHIP_I6B0
        printf("%s:%d bind src: (%d,%d,%d,%d), dst:(%d,%d,%d,%d), SrcFps=%d DstFps=%d, BindType=%d BindParam=%d\n", __func__,__LINE__,
                       pstBindInfo->stSrcChnPort.eModId, pstBindInfo->stSrcChnPort.u32DevId,
                       pstBindInfo->stSrcChnPort.u32ChnId, pstBindInfo->stSrcChnPort.u32PortId,
                       pstBindInfo->stDstChnPort.eModId, pstBindInfo->stDstChnPort.u32DevId,
                       pstBindInfo->stDstChnPort.u32ChnId, pstBindInfo->stDstChnPort.u32PortId,
                       pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate,
                       pstBindInfo->eBindType, pstBindInfo->u32BindParam);

    ExecFuncWARNING(MI_SYS_BindChnPort2(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
                                 pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate, \
                                 pstBindInfo->eBindType, pstBindInfo->u32BindParam),     \
                                 MI_SUCCESS);
#endif

    return MI_SUCCESS;
}

MI_S32 Mixer_Sys_UnBind(Mixer_Sys_BindInfo_T *pstBindInfo)
{
    printf("%s:%d unbind src(%d,%d,%d,%d), dst(%d,%d,%d,%d), SrcFps=%d, DstFps=%d\n", __func__, __LINE__,
                           pstBindInfo->stSrcChnPort.eModId, pstBindInfo->stSrcChnPort.u32DevId,
                           pstBindInfo->stSrcChnPort.u32ChnId, pstBindInfo->stSrcChnPort.u32PortId,
                           pstBindInfo->stDstChnPort.eModId, pstBindInfo->stDstChnPort.u32DevId,
                           pstBindInfo->stDstChnPort.u32ChnId, pstBindInfo->stDstChnPort.u32PortId,
                           pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate);

    ExecFuncWARNING(MI_SYS_UnBindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_U64 Mixer_Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

MI_S32 Mixer_Sys_Init(void)
{
    MI_U64 u64Pts = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_Version_t stVersion;
    struct timeval curStamp;


    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    ExecFunc(MI_SYS_GetVersion(&stVersion), MI_SUCCESS);
    MIXER_INFO("Get MI_SYS_Version:0x%llx\n", (MI_U64)stVersion.u8Version);


    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    ExecFunc(MI_SYS_InitPtsBase(u64Pts), MI_SUCCESS);

    gettimeofday(&curStamp, NULL);
    u64Pts = (curStamp.tv_sec) * 1000000ULL + curStamp.tv_usec;
    ExecFunc(MI_SYS_SyncPts(u64Pts), MI_SUCCESS);

    ExecFunc(MI_SYS_GetCurPts(&u64Pts), MI_SUCCESS);
    printf("%s:%d Get MI_SYS_CurPts:0x%llx(%ds,%dus)\n", __func__, __LINE__,
                  u64Pts, (int)curStamp.tv_sec, (int)curStamp.tv_usec);

    mixer_property_init();

    return s32Ret;
}

MI_S32 Mixer_Sys_Exit(void)
{
    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    mixer_property_uninit();

    return MI_SUCCESS;
}

#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
int Mixer_coverVi2Vpe(int viChn)
{
    MI_S32 vpeChn = -1;

    if(viChn < MAX_VPE_PORT_NUMBER)
    {
        vpeChn = viChn;
    }
    else
    {
        vpeChn = VPE_SUB_PORT;
    }

    return vpeChn;
}

MI_SYS_BindType_e coverMixerBindMode(Mixer_Venc_Bind_Mode_E eMixerBindMode)
{
    MI_SYS_BindType_e ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE;

    switch(eMixerBindMode)
    {
        case Mixer_Venc_Bind_Mode_FRAME:    ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE; break;
        case Mixer_Venc_Bind_Mode_REALTIME: ebindMode = E_MI_SYS_BIND_TYPE_REALTIME; break;
#if TARGET_CHIP_I6B0
        case Mixer_Venc_Bind_Mode_HW_HALF_RING:  ebindMode = E_MI_SYS_BIND_TYPE_HW_RING; break;
#endif
        case Mixer_Venc_Bind_Mode_HW_RING:  ebindMode = E_MI_SYS_BIND_TYPE_HW_RING; break;
        default: ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE; break;
    }

    return ebindMode;
}

MI_U32 Mixer_getIntFramerate(int u32FrameRate)
{
    MI_U32 getInterval = 0;
    MI_U32 den = 0;
    MI_U32 num = 0;

    if(u32FrameRate & 0xffff0000)
    {
        den = (u32FrameRate & 0xffff0000) >> 16;
        num = u32FrameRate & 0x0000ffff;
        getInterval = num/den;
        if(getInterval==0) getInterval = 1;
    }
    else
    {
        getInterval = u32FrameRate;
    }

    return getInterval;
}
#elif TARGET_CHIP_I6E
int Mixer_coverVi2Vpe(int viChn)
{
    MI_S32 vpeChn = -1;

    if(viChn < MAX_VPE_PORT_NUMBER)
    {
        vpeChn = viChn;
    }
    else
    {
        vpeChn = VPE_SUB_PORT;
    }

    return vpeChn;
}

MI_SYS_BindType_e coverMixerBindMode(Mixer_Venc_Bind_Mode_E eMixerBindMode)
{
    MI_SYS_BindType_e ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE;

    switch(eMixerBindMode)
    {
        case Mixer_Venc_Bind_Mode_FRAME:    ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE; break;
        case Mixer_Venc_Bind_Mode_REALTIME: ebindMode = E_MI_SYS_BIND_TYPE_REALTIME; break;
        case Mixer_Venc_Bind_Mode_HW_RING:  ebindMode = E_MI_SYS_BIND_TYPE_HW_RING; break;
        default: ebindMode = E_MI_SYS_BIND_TYPE_FRAME_BASE; break;
    }

    return ebindMode;
}

MI_U32 Mixer_getIntFramerate(int u32FrameRate)
{
    MI_U32 getInterval = 0;
    MI_U32 den = 0;
    MI_U32 num = 0;

    if(u32FrameRate & 0xffff0000)
    {
        den = (u32FrameRate & 0xffff0000) >> 16;
        num = u32FrameRate & 0x0000ffff;
        getInterval = num/den;
        if(getInterval==0) getInterval = 1;
    }
    else
    {
        getInterval = u32FrameRate;
    }

    return getInterval;
}

#endif

void* WatchDog_Task(void *argu)
{
    printf("[%s] + g_watchdog=%d\n", __FUNCTION__, g_watchdog);
    int wdt_fd = -1;
    wdt_fd = open("/dev/watchdog", O_WRONLY);

    if(wdt_fd == -1)
    {
        printf("open /dev/watchdog failed\n");
        return NULL;
    }

    int timeout = 10;
    if(-EINVAL == ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout))
    {
        printf("%s %d ioctl return -EINVAL\n", __func__, __LINE__);
    }

    while(g_watchdog)
    {
        if(-EINVAL == ioctl(wdt_fd, WDIOC_KEEPALIVE, 0))
        {
            printf("%s %d ioctl return -EINVAL\n", __func__, __LINE__);
        }
        printf("WDIOC_KEEPALIVE\n");
        sleep(5);
    }

    int option = WDIOS_DISABLECARD;
    int ret = ioctl(wdt_fd, WDIOC_SETOPTIONS, &option);
    printf("[%s] WDIOC_SETOPTIONS %d WDIOS_DISABLECARD=%d\n", __FUNCTION__, ret, option);

    if(wdt_fd != -1)
    {
        close(wdt_fd);
        wdt_fd = -1;
    }

    printf("[%s] -\n", __FUNCTION__);

    return NULL;
}

void watchDogInit()
{
    //  g_watchdog = readPropValueToInt((char *)"mixer.prop.watchdog");
    g_watchdog = 1;

    if(g_watchdog > 0)
    {
        pthread_create(&pthreadWatchdog, NULL, WatchDog_Task, NULL);
        pthread_setname_np(pthreadWatchdog , "WatchDog_Task");
    }
}

void watchDogUninit()
{
    if(g_watchdog > 0)
    {
        g_watchdog = 0;
        printf("[%s] thread join+\n", __FUNCTION__);
        pthread_join(pthreadWatchdog, NULL);
        printf("[%s] thread join-\n", __FUNCTION__);
    }
}

static void print_backtrace(void)
{
    void *bt[16];
    int bt_size;
    char **bt_syms;
    int i;

    bt_size = backtrace(bt, 16);
    bt_syms = backtrace_symbols(bt, bt_size);
    printf("backtrace() returned %d addresses\n", bt_size);

    printf("BACKTRACE ------------\n");

    for(i = 1; i < bt_size; i++)
    {
        printf("%s\n", bt_syms[i]);
    }

    printf("----------------------\n");
    free(bt_syms);
}

void segfault_sigaction(int signo, siginfo_t *si, void *arg)
{
    printf("Caught \"%s\"\n", strsignal(signo));
    system("cat /proc/self/maps");
    print_backtrace();

    signal(signo, SIG_DFL);
    raise(signo);
}
void mixer_setHdrValue(BOOL value)
{
  g_bHdr = value;
}
BOOL mixer_GetHdrValue(void)
{
  return g_bHdr;
}

 void Mixer_SetShowFrameIntervalState(BOOL state)
{
    g_ShowFrameInterval = state;
}
 BOOL Mixer_GetShowFrameIntervalState(void)
{
  return g_ShowFrameInterval;
}
 void Mixer_SetFdState(BOOL state)
{

}
BOOL Mixer_GetFdState(void)
{
  return TRUE;
}
void Mixer_SetHdState(BOOL state)
{

}
BOOL Mixer_GetHdState(void)
{
    return TRUE;
}
void Mixer_SetHcState(BOOL state)
{
}
BOOL Mixer_GetHcState(void)
{
  return TRUE;
}
void Mixer_SetMdState(BOOL state)
{
}
 BOOL Mixer_GetOdState(void)
{
  return TRUE;
}
void Mixer_SetIeLogState(BOOL state)
{
}
BOOL Mixer_GetIeLogState(void)
{
    return TRUE;
}
void Mixer_SetVgState(BOOL state)
{
}
 BOOL Mixer_GetVgState(void)
{
  return TRUE;
}
void Mixer_SetDlaState(BOOL state)
{
}
 BOOL Mixer_GetDlaState(void)
{
  return TRUE;
}
void Mixer_SetOsdState(BOOL state)
{
  g_displayOsd = state;
}
 BOOL Mixer_GetOsdState(void)
{
  return g_displayOsd;
}
void Mixer_SetVideoNum(MI_U16 num)
{

}
MI_U16 Mixer_GetVideoNum(void)
{
  return TRUE;
}
void mixer_SetRotaValue(MI_U32 value)
{

}
MI_U32 mixer_GetRotaValue(void)
{
  return TRUE;
}

MI_U16 CheckAlign(MI_U16 mb_size, MI_U32 stride)
{
    MI_U16 u16BufHAlignment;
    //MI_U16 u16BufVAlignment;
    MDMB_MODE_e mbSize = (MDMB_MODE_e)mb_size;
    switch(mbSize)
    {
#ifdef CONFIG_SIGMASTAR_CHIP_I2
        case MDMB_MODE_MB_8x8:
        u16BufHAlignment = 8 * 16;   /* width */
        //u16BufVAlignment = 8;        /* height */
        break;

        case MDMB_MODE_MB_16x16:
        u16BufHAlignment = 16 * 16;   /* width */
        //u16BufVAlignment = 16;        /* height */
        break;

        case MDMB_MODE_MB_4x4:
        default:
        u16BufHAlignment = 4 * 16;   /* width */
        //u16BufVAlignment = 4;        /* height */
        break;
#else
        case MDMB_MODE_MB_8x8:
        u16BufHAlignment = 16;   /* width */
        //u16BufVAlignment = 8;        /* height */
        break;

        case MDMB_MODE_MB_16x16:
        u16BufHAlignment = 16;   /* width */
        //u16BufVAlignment = 16;        /* height */
        break;

        case MDMB_MODE_MB_4x4:
        default:
        u16BufHAlignment = 16;   /* width */
        //u16BufVAlignment = 4;        /* height */
        break;
#endif
    }

   if(stride % u16BufHAlignment)
       {
           MIXER_ERR("stride(%u) not align up (%u), please check input param\n", stride, u16BufHAlignment);
        return -1;
       }
   return 0;
}


