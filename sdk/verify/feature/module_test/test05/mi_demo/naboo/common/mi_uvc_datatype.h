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
#ifndef _MI_UVC_DATATYPE_H_
#define _MI_UVC_DATATYPE_H_

#include <linux/usb/ch9.h>
#include <linux/videodev2.h>
#include <pthread.h>

#include "video.h"
#include "uvc.h"

#include "stdio.h"
typedef  FILE* FILE_HANDLE;

typedef int  MI_S32;
typedef unsigned char MI_U8;
typedef unsigned int MI_U32;
typedef unsigned int size_t;
typedef MI_U32 MI_UVC_CHANNEL;
typedef MI_U32 MI_UVC_PORT;
 
#define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5') /* add claude.rao */

#define MaxFrameSize    640*480*2   //define in kernel

#define ARRAY_SIZE(a)                  ((sizeof(a) / sizeof(a[0])))
#define CLEAR(x)                       memset(&(x), 0, sizeof (x))
#define FrameInterval2FrameRate(val)   ((int)(1.0/val*10000000))
#define FrameRate2FrameInterval(val)   ((int)(1.0/val*10000000))

#define clamp(val, min, max) ({                 \
        typeof(val) __val = (val);              \
        typeof(min) __min = (min);              \
        typeof(max) __max = (max);              \
        (void) (&__val == &__min);              \
        (void) (&__val == &__max);              \
        __val = __val < __min ? __min: __val;   \
        __val > __max ? __max: __val; })

#define true 1
#define false 0
#define IS_NULL(val)   ((val==NULL)?true:false)
#define IS_TRUE(val)   ((val!=0)?true:false)
#define IS_ZERO(val)   ((val==0)?true:false)
#define IS_NOZERO(val) ((val!=0)?true:false)
#define IS_EQUAL(vala,valb) ((vala==valb)?true:false)

/*FOR UVC DEVICE STATUS*/
#define BIT_1 (1<<0)
#define BIT_2 (1<<1)
#define BIT_3 (1<<2)
#define BIT_4 (1<<3)
#define BIT_5 (1<<4)
#define BIT_6 (1<<5)
#define BIT_7 (1<<6)
#define BIT_8 (1<<7)
#define UVC_DEVICE_MASK  0
#define UVC_DEVICE_INITIAL        (BIT_8 << UVC_DEVICE_MASK)
#define UVC_DEVICE_ENUMURATED     (BIT_7 << UVC_DEVICE_MASK)
#define UVC_DEVICE_REQBUFS        (BIT_6 << UVC_DEVICE_MASK)
#define UVC_DEVICE_STREAMON       (BIT_5 << UVC_DEVICE_MASK)
#define UVC_INTDEV_INITIAL        (BIT_4 << UVC_DEVICE_MASK)
#define UVC_INTDEV_STARTED        (BIT_3 << UVC_DEVICE_MASK)
#define UVC_INTDEV_STREAMON       (BIT_2 << UVC_DEVICE_MASK)
#define UVC_INTDEV_UNDEFINE       (BIT_1 << UVC_DEVICE_MASK)

#define UVC_CHANGE_STATUS(val,mask,bit) \
         do{val= bit ? (mask | val) : ( val & (~mask));}while(0)
#define UVC_SET_STATUS(val,mask)    UVC_CHANGE_STATUS(val,mask,1)
#define UVC_UNSET_STATUS(val,mask)  UVC_CHANGE_STATUS(val,mask,0)
#define UVC_GET_STATUS(val,mask)    IS_TRUE((val & mask))
#define UVC_INPUT_ISENABLE(val)   ( UVC_GET_STATUS(val,UVC_INTDEV_INITIAL ))
#define UVC_OUTPUT_ISENABLE(val)  ( UVC_GET_STATUS(val,UVC_DEVICE_INITIAL )     && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_ENUMURATED )  && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_REQBUFS )     && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_STREAMON ))
#define UVC_DEVICE_ISREADY(val)   ( UVC_GET_STATUS(val,UVC_DEVICE_INITIAL ) &&\
                                    UVC_GET_STATUS(val,UVC_INTDEV_INITIAL ) &&\
                                    UVC_GET_STATUS(val,UVC_INTDEV_STARTED ))

typedef enum {
    USB_ISOC_MODE,
    USB_BULK_MODE,
} TransferMode_e;

typedef enum {
    STREAMING_SYSTEM_EVENT,
    STREAMING_SETTING_EVENT,
} StreamEvent_e;

typedef enum {
    UVC_PROBE_CONTROL = 1,
    UVC_COMMIT_CONTROL,
} ControlType_e;

struct buffer {
    struct v4l2_buffer buf; 
    void   *start;
    size_t length;
};

struct uvc_frame_info {
    MI_U32 width;
    MI_U32 height;
    MI_U32 intervals[8];
};

struct uvc_format_info {
	MI_U32 fcc;
	const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_nv12[] = {
	{  320, 240, { 400000, 666666,  1000000, 0 }, },
	{  640, 360, { 666666, 1000000, 5000000, 0 }, },
	{  640, 480, { 400000, 1000000, 5000000, 0 }, },
//	{ 1280, 720, { 666666, 1000000, 5000000, 0 }, },
	{ 1920, 1080,{ 666666, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
	{  640, 360, { 666666, 1000000, 5000000, 0 }, },
	{ 1280, 720, { 5000000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
    {  640, 360, { 333333,  666666, 1000000, 0 }, },
    {  640, 480, { 333333,  666666, 1000000, 0 }, },
    { 1280, 720, { 333333,  666666, 1000000, 0 }, },
    { 1920,1080, { 400000,  666666, 1000000, 0 }, },
    { 1920,1088, { 333333,  400000, 1000000, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h264[] = {
    {  480, 360, { 333333, 666666, 1000000,  0 }, },
    {  640, 480, { 333333, 666666, 1000000,  0 }, },
    { 1280, 720, { 333333, 666666, 1000000,  0 }, },
    { 1920,1080, { 333333, 666666, 1000000,  0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h265[] = {
    {  480, 360, { 333333, 666666, 1000000,  0 }, },
    {  640, 480, { 333333, 666666, 1000000,  0 }, },
    { 1280, 720, { 333333, 666666, 1000000,  0 }, },
    { 1920,1080, { 333333, 666666, 1000000,  0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
    { V4L2_PIX_FMT_NV12, uvc_frames_nv12 },
//	{ V4L2_PIX_FMT_YUYV, uvc_frames_yuyv },
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
    { V4L2_PIX_FMT_H264, uvc_frames_h264 },
    { V4L2_PIX_FMT_H265, uvc_frames_h265 },
};

typedef struct Stream_Params_s {
    /* video format framerate */
    MI_U32 fcc;
    MI_U32 iformat;
    MI_U32 iframe;
    MI_U32 width;
    MI_U32 height;
    double frameRate;
} Stream_Params_t;

typedef struct MI_UVC_Setting_s {
    /* buffer related*/
    MI_U8 nbuf;

    /* transfer mode : bulk or isoc */
    TransferMode_e mode;

} MI_UVC_Setting_t;

typedef struct MI_UVC_OPS_s {
    MI_S32  (* UVC_Inputdev_Init)  (void *uvc);
    MI_S32  (* UVC_Inputdev_Deinit)(void *uvc);
    MI_S32  (* UVC_DevFillBuffer)  (void *uvc,MI_U32 *length,void *buf);
    MI_S32  (* UVC_StartCapture)   (void *uvc,Stream_Params_t format);
    MI_S32  (* UVC_StopCapture)    (void *uvc);
} MI_UVC_OPS_t;

typedef struct MI_UVC_ChnAttr_s {
    MI_UVC_Setting_t setting;
    MI_UVC_OPS_t fops;
} MI_UVC_ChnAttr_t;

typedef struct MI_UVC_Device_s {
    MI_S32 uvc_fd;
     
    /* UVC thread Releated */
    pthread_t Thread;  
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    /* UVC Setting */
    MI_UVC_ChnAttr_t ChnAttr;

    /* Input Video Specific */
    void *Input_Device;

    /* UVC Stream Control Specific */
    Stream_Params_t stream_param;
    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;

    /* UVC Control Request Specific */
    ControlType_e control;
    struct uvc_request_data request_error_code;

    /* UVC Device buffer */
    struct buffer *mem;

    /* UVC Specific flags */
    unsigned char status;
} MI_UVC_Device_t;

#endif //_MI_UVC_DATATYPE_H_
