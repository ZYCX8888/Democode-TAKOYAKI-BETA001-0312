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

#ifndef _ST_UVC_DATATYPE_H_
#define _ST_UVC_DATATYPE_H_

#include <linux/usb/ch9.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "video.h"
#include "uvc.h"

#define UVC_SUPPORT_DEBUG

#define UVC_VC_EXTENSION1_UNIT_ID   6 //ISP
#define UVC_VC_EXTENSION2_UNIT_ID   2 //customer
#define UVC_VC_OUTPUT_TERMINAL_ID   3
#define UVC_VC_PROCESSING_UNIT_ID   4
#define UVC_VC_INPUT_TERMINAL_ID    1
#define UVC_VC_SELECTOR_UNIT_ID     0xfe

#define ST_UVC_SUCCESS 0

typedef unsigned int size_t;

#define V4L2_PIX_FMT_H265     v4l2_fourcc('H', '2', '6', '5') /* add claude.rao */

#define MaxFrameSize    320*240*2   //define in kernel

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)                  ((sizeof(a) / sizeof(a[0])))
#endif

#define CLEAR(x)                       memset(&(x), 0, sizeof (x))
#define FrameInterval2FrameRate(val)   ((int)(1.0/val*10000000))
#define FrameRate2FrameInterval(val)   ((int)(1.0/val*10000000))

#define clamp(val, min, max) ({                 \
        typeof(val) __val = (val);              \
        typeof(min) __min = (min);              \
        typeof(max) __max = (max);              \
        __val = __val < __min ? __min: __val;   \
        __val > __max ? __max: __val; })

#define IS_NULL(val)   ((val==NULL)?true:false)
#define IS_TRUE(val)   ((val!=0)?true:false)
#define IS_ZERO(val)   ((val==0)?true:false)
#define IS_NOZERO(val) ((val!=0)?true:false)
#define IS_EQUAL(vala,valb) ((vala==valb)?true:false)

/*FOR UVC DEVICE STATUS*/
#define UVC_DEVICE_MASK  0
#define UVC_DEVICE_INITIAL        ((1<<7) << UVC_DEVICE_MASK)
#define UVC_DEVICE_ENUMURATED     ((1<<6) << UVC_DEVICE_MASK)
#define UVC_DEVICE_REQBUFS        ((1<<5) << UVC_DEVICE_MASK)
#define UVC_DEVICE_STREAMON       ((1<<4) << UVC_DEVICE_MASK)
#define UVC_INTDEV_INITIAL        ((1<<3) << UVC_DEVICE_MASK)
#define UVC_INTDEV_STARTED        ((1<<2) << UVC_DEVICE_MASK)
#define UVC_INTDEV_STREAMON       ((1<<1) << UVC_DEVICE_MASK)
#define UVC_INTDEV_UNDEFINE       ((1<<0) << UVC_DEVICE_MASK)

#define UVC_CHANGE_STATUS(val,mask,bit) \
         do{val= bit ? (mask | val) : ( val & (~mask));}while(0)
#define UVC_SET_STATUS(val,mask)    UVC_CHANGE_STATUS(val,mask,1)
#define UVC_UNSET_STATUS(val,mask)  UVC_CHANGE_STATUS(val,mask,0)
#define UVC_GET_STATUS(val,mask)    IS_TRUE((val & mask))
#define UVC_INPUT_ISENABLE(val)   ( UVC_GET_STATUS(val,UVC_INTDEV_INITIAL  )     &&\
                                    UVC_GET_STATUS(val,UVC_INTDEV_STARTED  )     && \
                                    UVC_GET_STATUS(val,UVC_INTDEV_STREAMON ))
#define UVC_OUTPUT_ISENABLE(val)  ( UVC_GET_STATUS(val,UVC_DEVICE_INITIAL )      && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_ENUMURATED )   && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_REQBUFS )      && \
                                    UVC_GET_STATUS(val,UVC_DEVICE_STREAMON ))
#define UVC_DEVICE_ISREADY(val)   ( UVC_GET_STATUS(val,UVC_DEVICE_INITIAL )      &&\
                                    UVC_GET_STATUS(val,UVC_INTDEV_INITIAL )      &&\
                                    UVC_GET_STATUS(val,UVC_INTDEV_STARTED ))

typedef enum
{
    UVC_DBG_NONE = 0,
    UVC_DBG_ERR,
    UVC_DBG_WRN,
    UVC_DBG_INFO,
    UVC_DBG_DEBUG,
    UVC_DBG_ALL
} UVC_DBG_LEVEL_e;

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

extern UVC_DBG_LEVEL_e uvc_debug_level;
extern bool  uvc_func_trace;

#ifdef UVC_SUPPORT_DEBUG
#define UVC_DEBUG(fmt, args...) ({do{if(uvc_debug_level>=UVC_DBG_DEBUG)\
                {printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define UVC_INFO(fmt, args...)     ({do{if(uvc_debug_level>=UVC_DBG_INFO)\
                {printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define UVC_WRN(fmt, args...)      ({do{if(uvc_debug_level>=UVC_DBG_WRN)\
                {printf(ASCII_COLOR_YELLOW"[APP WRN ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define UVC_ERR(fmt, args...)      ({do{if(uvc_debug_level>=UVC_DBG_ERR)\
                {printf(ASCII_COLOR_RED"[APP ERR ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define UVC_EXIT_ERR(fmt, args...) ({do\
                {printf(ASCII_COLOR_RED"<<<%s[%d] " fmt ASCII_COLOR_END,__FUNCTION__,__LINE__,##args);}while(0);})
#define UVC_ENTER()                ({do{if(uvc_func_trace)\
                {printf(ASCII_COLOR_BLUE">>>%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})
#define UVC_EXIT_OK()              ({do{if(uvc_func_trace)\
                {printf(ASCII_COLOR_BLUE"<<<%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})
#else
#define UVC_DEBUG(fmt, args...) NULL
#define UVC_ERR(fmt, args...)   NULL
#define UVC_INFO(fmt, args...)  NULL
#define UVC_WRN(fmt, args...)   NULL
#endif

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__,__LINE__);\
        return -1;\
    }
#endif

typedef enum {
    USB_ISOC_MODE,
    USB_BULK_MODE,
} Transfer_Mode_e;

typedef enum {
    UVC_CONTROL_INTERFACE = 1,
    UVC_STREAMING_INTERFACE,
} Control_Type_e;

typedef struct UVC_Control_s {
    Control_Type_e ctype;
    unsigned char control;
    unsigned char entity;
    unsigned char length;
} UVC_Control_t;

typedef enum {
    BUFFER_FREE,
    BUFFER_DEQUEUE,
    BUFFER_FILLING,
    BUFFER_QUEUE,
} buffer_status_e;

struct buffer {
    struct v4l2_buffer buf;
    void   *start;
/* for buff to finish, differ from mmap and userptr */
    int    handle;
/* for mmap */
    size_t length;
/* buffer tail flags,default tail(1) */
    buffer_status_e status;
    bool is_tail;
};

struct uvc_frame_info {
    uint32_t width;
    uint32_t height;
    uint32_t intervals[8];
};

struct uvc_format_info {
    uint32_t fcc;
    const struct uvc_frame_info *frames;
};

static const struct uvc_frame_info uvc_frames_yuyv[] = {
    {  320, 240, { 333333, 666666, 1000000, 0 }, },
    {  640, 480, { 333333, 666666, 1000000, 0 }, },
    { 1280, 720, { 333333, 666666, 1000000, 0 }, },
    { 1920, 1080,{ 333333, 666666, 1000000, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_nv12[] = {
    {  320, 240, { 333333, 666666, 1000000, 0 }, },
    {  640, 480, { 333333, 666666, 1000000, 0 }, },
    { 1280, 720, { 333333, 666666, 1000000, 0 }, },
    { 1920, 1080,{ 333333, 666666, 1000000, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
    { 320,  240, { 333333,  666666, 1000000, 0 }, },
    { 640,  480, { 333333,  666666, 1000000, 0 }, },
    { 1280, 720, { 333333,  666666, 1000000, 0 }, },
    { 1920,1080, { 333333,  666666, 1000000, 0 }, },
    { 2560,1440, { 333333,  666666, 1000000, 0 }, },
    { 3840,2160, { 333333,  666666, 1000000, 0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h264[] = {
    {  320, 240, { 333333, 666666, 1000000,  0 }, },
    {  640, 480, { 333333, 666666, 1000000,  0 }, },
    { 1280, 720, { 333333, 666666, 1000000,  0 }, },
    { 1920,1080, { 333333, 666666, 1000000,  0 }, },
    { 2560,1440, { 333333, 666666, 1000000,  0 }, },
    { 3840,2160, { 333333, 666666, 1000000,  0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h265[] = {
    {  320, 240, { 333333, 666666, 1000000,  0 }, },
    {  640, 480, { 333333, 666666, 1000000,  0 }, },
    { 1280, 720, { 333333, 666666, 1000000,  0 }, },
    { 1920,1080, { 333333, 666666, 1000000,  0 }, },
    { 2560,1440, { 333333, 666666, 1000000,  0 }, },
    { 3840,2160, { 333333, 666666, 1000000,  0 }, },
    { 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
//    { V4L2_PIX_FMT_YUYV,  uvc_frames_yuyv },
    { V4L2_PIX_FMT_NV12,  uvc_frames_nv12 },
    { V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg },
    { V4L2_PIX_FMT_H264,  uvc_frames_h264 },
    { V4L2_PIX_FMT_H265,  uvc_frames_h265 },
};

typedef struct Stream_Params_s {
    /* video format framerate */
    uint32_t fcc;
    uint32_t iformat;
    uint32_t iframe;
    uint32_t width;
    uint32_t height;
    double frameRate;
    uint32_t maxframesize;
} Stream_Params_t;

typedef enum {
    UVC_MEMORY_MMAP,
    UVC_MEMORY_USERPTR,
} UVC_IO_MODE_e;

typedef struct ST_UVC_Setting_s {
    /* buffer related*/
    uint8_t nbuf;
    /* v4l2 memory type */
    UVC_IO_MODE_e io_mode;
    /* transfer mode : bulk or isoc */
    Transfer_Mode_e mode;
} ST_UVC_Setting_t;

typedef struct ST_UVC_BufInfo_s {
    unsigned long length;
    union {
        unsigned long start; // for userptr
        void *buf;           // for mmap
    } b;
    bool is_tail;
}ST_UVC_BufInfo_t;

typedef struct ST_UVC_USERPTR_BufOpts_s {
    int32_t  (* UVC_DevFillBuffer)(void *uvc,ST_UVC_BufInfo_t *bufInfo);
} ST_UVC_USERPTR_BufOpts_t;

typedef struct ST_UVC_MMAP_BufOpts_s {
    int32_t  (* UVC_DevFillBuffer)(void *uvc,ST_UVC_BufInfo_t *bufInfo);
} ST_UVC_MMAP_BufOpts_t;

typedef struct ST_UVC_OPS_s {
    int32_t  (* UVC_Inputdev_Init)  (void *uvc);
    int32_t  (* UVC_Inputdev_Deinit)(void *uvc);
    union {
        ST_UVC_MMAP_BufOpts_t m;
        ST_UVC_USERPTR_BufOpts_t u;
    };
    int32_t  (* UVC_StartCapture)   (void *uvc,Stream_Params_t format);
    int32_t  (* UVC_StopCapture)    (void *uvc);
} ST_UVC_OPS_t;

typedef struct ST_UVC_ChnAttr_s {
    ST_UVC_Setting_t setting;
    ST_UVC_OPS_t fops;
} ST_UVC_ChnAttr_t;

typedef struct ST_UVC_Device_s {
    int32_t uvc_fd;

    /* UVC thread Releated */
    pthread_t event_handle_thread;
    pthread_t video_process_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    /* UVC Setting */
    ST_UVC_ChnAttr_t ChnAttr;

    /* Input Video Specific */
    void *Input_Device;

    /* UVC Stream Control Specific */
    Stream_Params_t stream_param;
    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;

    /* UVC Control Request Specific */
    UVC_Control_t control;
    struct uvc_request_data request_error_code;

    /* UVC Device buffer */
    struct buffer *mem;

    /* UVC Specific flags */
    unsigned char status;

    int exit_request;
} ST_UVC_Device_t;
#endif //_ST_UVC_DATATYPE_H_
