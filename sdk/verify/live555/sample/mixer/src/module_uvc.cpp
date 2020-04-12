/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  module_uvc.c
* Author:     claude.rao@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/8/20
* Description: Mixer system module source file
*
*
*
* History:
*
*    1. Date  :        2018/8/20
*       Author:        claude.roa@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <sys/socket.h>
#include <sys/shm.h>
#include <linux/netlink.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "module_common.h"
#include "mid_uvc.h"

#include "CUvc.h"

extern MI_U32 g_videoNumber;
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];

static CUvc *pCUvc = NULL;
static const int VideoId = 0;

static MI_S32 UVC_Init(void *uvc)
{
    return 0;
}

static MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}

static MI_S32 UVC_MM_FillBuffer(void *uvc_dev, MI_UVC_BufInfo_t *bufInfo)
{
    MI_UVC_Device_t *uvc = (MI_UVC_Device_t*)uvc_dev;
    unsigned long *length = &bufInfo->length;
    void *buf = bufInfo->b.buf;

    switch(uvc->stream_param.fcc)
    {
        case V4L2_PIX_FMT_H264:
        case V4L2_PIX_FMT_H265:
        case V4L2_PIX_FMT_MJPEG:
            *length = pCUvc->GetFrameData(buf);
            if(*length == 0)
                return -EINVAL;
            break;
        case V4L2_PIX_FMT_NV12:
            break;
        default:
            break;
    }

    return 0;
}

static MI_S32 UVC_StartCapture(void *uvc_dev,Stream_Params_t format)
{
    MI_UVC_Device_t *uvc = (MI_UVC_Device_t *)uvc_dev;

    /* Init input device */
    switch(uvc->stream_param.fcc)
    {
        case V4L2_PIX_FMT_YUYV:
            MIXER_DBG("[ Video Format is V4L2_PIX_FMT_YUYV ]\n");
            break;
        case V4L2_PIX_FMT_NV12:
            MIXER_DBG("[ Video Format is V4L2_PIX_FMT_NV12 ]\n");
            break;
        case V4L2_PIX_FMT_MJPEG:
            break;
        case V4L2_PIX_FMT_H264:
            break;
        case V4L2_PIX_FMT_H265:
            break;
        default:
            return -EINVAL;
    }

    pCUvc = new CUvc(VideoId, g_videoEncoderArray[VideoId]);
    pCUvc->RegisterVideoEncoder();
    pCUvc->RequestIDR();

    return 0;
}

static MI_S32 UVC_StopCapture(void *uvc)
{
    pCUvc->UnRegisterVideoEncoder();
    pCUvc->FlushVideoBuff();

    return MI_SUCCESS;
}

static MI_S32 uvc_task_init()
{
    MI_UVC_Setting_t pstSet={4,UVC_MEMORY_MMAP,USB_ISOC_MODE};
    MI_UVC_OPS_t fops = {
            UVC_Init ,
            UVC_Deinit,
            UVC_MM_FillBuffer,
            UVC_StartCapture,
            UVC_StopCapture
    };

    MI_UVC_ChnAttr_t pstAttr ={pstSet,fops};

    char device_name[] = "/dev/video0";
    MI_UVC_Init(device_name);
    MI_UVC_CreateDev(&pstAttr);
    MI_UVC_StartDev();

    return MI_SUCCESS;
}


static void uvc_task_deinit()
{
    MI_UVC_StopDev();
    MI_UVC_DestroyDev();
    MI_UVC_Uninit();
}

static MI_S32 alsa_capture_task_init()
{
    return MI_SUCCESS;
}
static MI_S32 alsa_capture_task_deinit()
{
    return MI_SUCCESS;
}

static MI_S32 alsa_playback_task_init()
{
    return MI_SUCCESS;
}

static MI_S32 alsa_playback_task_deinit()
{
    return MI_SUCCESS;
}

int uvc_process_cmd(MixerCmdId id, MI_S8 *Param, MI_U32 ParamLen)
{
    switch(id)
    {
        case CMD_UVC_INIT:
            uvc_task_init();
            switch(Param[0]) {
                case 1:
                    alsa_capture_task_init();
                    MIXER_INFO("Open UVC & Alsa Capture\n");
                    break;
                case 2:
                    alsa_playback_task_init();
                    MIXER_INFO("Open UVC & Alsa Playback\n");
                    break;
                case 3:
                    alsa_capture_task_init();
                    alsa_playback_task_init();
                    MIXER_INFO("Open UVC & Alsa Capture + Playback\n");
                    break;
                case 0:
                default:
                    MIXER_INFO("Open UVC ONLY\n");
                    break;
            }
            break;

        case CMD_UVC_CLOSE:
            uvc_task_deinit();
            switch(Param[0]) {
                case 1:
                    MIXER_INFO("Close UVC & Alsa Capture\n");
                    alsa_capture_task_deinit();
                    break;
                case 2:
                    MIXER_INFO("Close UVC & Alsa playback\n");
                    alsa_playback_task_deinit();
                    break;
                case 3:
                    MIXER_INFO("Close UVC & Alsa Capture + Playback\n");
                    alsa_capture_task_deinit();
                    alsa_playback_task_deinit();
                    break;
                case 0:
                default:
                    MIXER_INFO("Close UVC ONLY\n");
                    break;
            }
            break;

        default:
            break;
    }
    return 0;
}
