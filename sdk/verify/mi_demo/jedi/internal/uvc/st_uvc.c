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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <assert.h>
#include "st_uvc.h"
#include "st_uvc_xu.h"

bool uvc_func_trace = 1;
UVC_DBG_LEVEL_e uvc_debug_level = UVC_DBG_ERR;

#define DOWN() \
    do \
    {\
        pthread_mutex_lock(&mm_lock);\
    } while(0)


#define UP() \
    do \
    {\
        pthread_mutex_unlock(&mm_lock);\
    } while(0)

pthread_mutex_t mm_lock;
static ST_UVC_Device_t *uvc;
static struct uvc_streaming_control
_UVC_Fill_Streaming_Control(struct uvc_streaming_control *ctrl,int32_t iframe, int32_t iformat);

static int32_t _UVC_FillBuffer(struct buffer *mem,UVC_IO_MODE_e io_mode)
{
    ST_UVC_BufInfo_t BufInfo;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;

retry:
    if(false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
        return -1;

    memset(&BufInfo,0x00,sizeof(ST_UVC_BufInfo_t));
    BufInfo.is_tail = true;

    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON)){
        if(UVC_MEMORY_MMAP==io_mode){
            if(IS_NULL(fops.m.UVC_DevFillBuffer))
                return -1;
            BufInfo.b.buf = mem->start;
            /* Fill BUF,set param if successfully */
            if(0 <= fops.m.UVC_DevFillBuffer(uvc, &BufInfo)){
                mem->is_tail = BufInfo.is_tail;
                mem->buf.bytesused = BufInfo.length;
            } else {
                usleep(1000);
                goto retry;
            }
        } else
        if(UVC_MEMORY_USERPTR==io_mode){
            if(IS_NULL(fops.u.UVC_DevFillBuffer))
                return -1;
            BufInfo.b.buf = mem->start;
            /* Fill BUF,set param if successfully */
            if(0 <= (fops.u.UVC_DevFillBuffer(uvc, &BufInfo))){
                mem->is_tail = BufInfo.is_tail;
                mem->buf.m.userptr = (u_int32_t)BufInfo.b.buf;
                mem->buf.bytesused = BufInfo.length;
                printf("length %lu\n", BufInfo.length);
            } else {
                usleep(1000);
                goto retry;
            }
        }
    } else {
        UVC_WRN("Input device is no ready");
        return -1;
    }

    /* check format */
    if(mem->buf.bytesused > mem->buf.length){
        UVC_WRN("bytesused is bigger than buf length ,Drop it\n");
        goto retry;
    }
    return mem->buf.bytesused;
}

static int8_t _UVC_Get_User_Buf_Count(buffer_status_e status)
{
    if(NULL == uvc->mem)
        return 0;

    int8_t  i ,count = 0;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;

    for(i = 0; i < set.nbuf; i++) {
        if(uvc->mem[i].status == status) {
            count++;
        }
    }
    return count;
}

static int8_t _UVC_QBuf(struct buffer *mem)
{
    int32_t s32Ret;
    int32_t uvc_fd     = uvc->uvc_fd;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;

    assert(mem);

    if(mem->status != BUFFER_DEQUEUE)
         return -EINVAL;

    mem->status = BUFFER_FILLING;

    s32Ret = _UVC_FillBuffer(mem,set.io_mode);
    if( s32Ret <= 0 ) {
        UVC_WRN("Failed to Get A Buf\n");
        goto fail;
    }

    /* if dev is already streamoff ,return -1 */
    if(false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
        goto fail;

    if(mem->is_tail)
        mem->buf.reserved = 0;
    else
        mem->buf.reserved = 1;

    s32Ret = ioctl(uvc_fd, VIDIOC_QBUF, &(mem->buf));
    if (s32Ret < 0) {
        UVC_WRN("Unable to queue buffer: %s (%d).\n",
                strerror(errno), errno);
        goto fail;
    }
    mem->status = BUFFER_QUEUE;
    UVC_INFO("Qbuf Success, buf index %d\n",mem->buf.index);
    return 0;

fail:
    mem->status = BUFFER_DEQUEUE;
    return s32Ret;
}

/* flags: 0 from v4l2 device , 1 from from user buf */
static struct buffer *_UVC_DQBuf(bool flags)
{
    int32_t s32Ret;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
    struct buffer *mem = NULL;

    if(NULL == uvc || NULL == uvc->mem)
        return NULL;

    if (!flags) {
        int32_t buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        int32_t uvc_fd     = uvc->uvc_fd;
        struct v4l2_buffer ubuf;

        CLEAR(ubuf);
        ubuf.type   = buftype;
        if(UVC_MEMORY_MMAP == set.io_mode)
            ubuf.memory = V4L2_MEMORY_MMAP;
        else if(UVC_MEMORY_USERPTR == set.io_mode)
            ubuf.memory = V4L2_MEMORY_USERPTR;

        s32Ret = ioctl(uvc_fd, VIDIOC_DQBUF, &ubuf);
        if (s32Ret < 0) {
            UVC_WRN("VIDIOC_DQBUF Fail s32Ret: %d \n",s32Ret);
            return NULL;
        }
        mem = &(uvc->mem[ubuf.index]);
        mem->buf = ubuf;
        mem->status = BUFFER_DEQUEUE;
    } else {
        int32_t i;

        for(i = 0; i < set.nbuf; i++) {
            if(uvc->mem[i].status == BUFFER_DEQUEUE) {
                mem = &uvc->mem[i];
                UVC_INFO("Get User buf,buf index %d \n",mem->buf.index);
                return mem;
            }
        }
        return NULL;
    }
    return mem;
}

static int8_t _UVC_Video_QBuf(void)
{
    uint32_t i;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
    struct buffer *mem = NULL;

    if(false==UVC_GET_STATUS(uvc->status, UVC_DEVICE_REQBUFS))
    {
        usleep(100);
        return -1;
    }
    DOWN();
    for (i = 0; i < set.nbuf; ++i) {
         mem = _UVC_DQBuf(1);
        if(NULL == mem)
            continue;

         _UVC_QBuf(mem);
    }
    UP();

    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Video_ReqBufs(int32_t flags)
{
    struct v4l2_requestbuffers rb;
    uint32_t i;
    int32_t s32Ret;

    struct buffer *mem = NULL;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
    int32_t uvc_fd = uvc->uvc_fd;
    int32_t buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    CLEAR(rb);

    if(1 == flags){
        if(false==UVC_GET_STATUS(uvc->status,UVC_DEVICE_REQBUFS)){
            rb.count = set.nbuf;
            UVC_SET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
        } else{
           UVC_ERR("Release BUF First \n");
           return -1;
        }
    }
    else if(0 == flags){
        if(true==UVC_GET_STATUS(uvc->status,UVC_DEVICE_REQBUFS)){
            rb.count = 0;
            UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
        } else{
           UVC_WRN("Already Release Buf\n");
           return ST_UVC_SUCCESS;
        }
    }
    else{
        UVC_WRN("Invalid Param \n");
        return -EINVAL;
    }

    rb.type   = buftype;
    if(UVC_MEMORY_MMAP == set.io_mode)
        rb.memory = V4L2_MEMORY_MMAP;
    else if(UVC_MEMORY_USERPTR == set.io_mode)
        rb.memory = V4L2_MEMORY_USERPTR;

    if (!rb.count)
    {
        mem = uvc->mem;
        for (i = 0; i < set.nbuf; ++i)
        {
            if(UVC_MEMORY_USERPTR == set.io_mode)
            {
                free(mem[i].start);
            } else
            if(UVC_MEMORY_MMAP == set.io_mode)
            {
                s32Ret= munmap(mem[i].start, mem[i].length);
                if (s32Ret< 0)
                {
                    UVC_ERR("munmap failed\n");
                    goto err;
                }
            }
        }

        for (i = 0; i < set.nbuf; ++i)
        {
            s32Ret = ioctl(uvc_fd, VIDIOC_REQBUFS, &rb);
            if (s32Ret < 0)
            {
                if (s32Ret == -EINVAL)
                    UVC_ERR("does not support memory mapping");
                else
                    UVC_ERR("Unable to free buffers: %s (%d).\n",
                        strerror(errno), errno);
                goto err;
            }
        }
        free(mem);
        mem = NULL;
    }

    if (rb.count)
    {
        s32Ret = ioctl(uvc_fd, VIDIOC_REQBUFS, &rb);
        if (s32Ret < 0)
        {
            if (s32Ret == -EINVAL)
                UVC_ERR("does not support memory mapping");
            else
                UVC_ERR("Unable to alloc buffers: %s (%d).\n",
                    strerror(errno), errno);
            goto err;
        }

        if (rb.count < 2) {
            UVC_ERR("Insufficient buffer memory.\n");
            s32Ret = -EINVAL;
            goto err;
        }

        mem = (struct buffer*)calloc(rb.count, sizeof(mem[0]));
        if (!mem) {
            UVC_ERR("Out of memory\n");
            s32Ret = -ENOMEM;
            goto err;
        }

        for (i = 0; i < rb.count; ++i) {
            memset(&mem[i].buf, 0, sizeof(mem[i].buf));
            mem[i].buf.type   = buftype;
            mem[i].buf.index  = i;
            mem[i].handle = 0;
            mem[i].is_tail = 0;
            mem[i].status = BUFFER_DEQUEUE;

            /* USERPTR Mode alloc usr buffer */
            if(UVC_MEMORY_USERPTR == set.io_mode){
                mem[i].buf.memory = V4L2_MEMORY_USERPTR;
                s32Ret = ioctl(uvc_fd, VIDIOC_QUERYBUF, &(mem[i].buf));
                if (s32Ret < 0) {
                    UVC_ERR("VIDIOC_QUERYBUF failed for buf %d:"
                       "%s (%d).\n", i, strerror(errno), errno);
                    s32Ret = -EINVAL;
                    goto err_free;
                }

                mem[i].start = malloc(uvc->stream_param.maxframesize);
                mem[i].buf.length = uvc->stream_param.maxframesize;
                continue;
            } else
            /* MMAP Map the buffers. */
            if(UVC_MEMORY_MMAP == set.io_mode)
            {
                mem[i].buf.memory = V4L2_MEMORY_MMAP;
                s32Ret = ioctl(uvc_fd, VIDIOC_QUERYBUF, &(mem[i].buf));
                if (s32Ret < 0) {
                    UVC_ERR("VIDIOC_QUERYBUF failed for buf %d:"
                       "%s (%d).\n", i, strerror(errno), errno);
                    s32Ret = -EINVAL;
                    goto err_free;
                }

                mem[i].start = mmap(NULL /* start anywhere */,
                               mem[i].buf.length,
                               PROT_READ | PROT_WRITE /* required */,
                               MAP_SHARED /* recommended */,
                               uvc_fd, mem[i].buf.m.offset);

                if (MAP_FAILED == mem[i].start) {
                    UVC_ERR("Unable to map buffer %u: %s (%d). \n",
                             i,strerror(errno), errno);
                    mem[i].length = 0;
                    s32Ret = -EINVAL;
                    goto err_free;
                }

                mem[i].length = mem[i].buf.length;
                UVC_INFO("UVC: Buffer %u mapped at address %p mem length %d. \n",i,mem[i].start,mem[i].length);
            }
        }
    }

    uvc->mem = mem;
    printf("UVC: %u buffers %s. \n", set.nbuf, rb.count?"alloc":"free");

    s32Ret = ST_UVC_SUCCESS;
    goto done;

err_free:
    free(mem);
err:
    UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
done:
    return s32Ret;
}

static int8_t _UVC_Video_Stream_on_off(int32_t fd,int32_t enable)
{
    struct v4l2_format vformat;
    int32_t s32Ret = ST_UVC_SUCCESS;
    int32_t buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    DOWN();
    if (!enable) {
        if(false==UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON))
            goto done;
        else{
           UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
           s32Ret = ioctl(fd, VIDIOC_STREAMOFF, &buftype);
           if (s32Ret < 0){
               UVC_ERR("VIDIOC_STREAMOFF failed: %s (%d).\n",
                    strerror(errno), errno);
               goto done;
           }
           while(_UVC_Get_User_Buf_Count(BUFFER_FILLING))
               usleep(1000);

           _UVC_Video_ReqBufs(0);
        }
        printf("UVC: Stop video stream Successfully.\n");
    } else {
        if(true==UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON))
           goto done;
        else{
            /* Get  V4l2 Format Param */
            vformat.type = buftype;
            s32Ret = ioctl(fd,VIDIOC_G_FMT,&vformat);
            if(s32Ret <0 )
                UVC_WRN("GET Format Failed ret= %d\n",s32Ret);
            /* then Set the v4l2 device Param */
            vformat.fmt.pix.pixelformat = uvc->stream_param.fcc;
            vformat.fmt.pix.width       = uvc->stream_param.width;
            vformat.fmt.pix.height      = uvc->stream_param.height;
            vformat.fmt.pix.sizeimage   = uvc->stream_param.maxframesize;
            s32Ret = ioctl(fd,VIDIOC_S_FMT,&vformat);
            if(s32Ret <0 ){
                UVC_ERR("Set V4l2 Param Failed ret=%d \n",s32Ret);
            }
            /* again to ensure format */
            s32Ret = ioctl(fd,VIDIOC_G_FMT,&vformat);
            if(s32Ret <0 )
                UVC_WRN("GET Format Failed ret= %d\n",s32Ret);
            else {
                UVC_INFO("mt->fmt.pix.width      =%u\n",vformat.fmt.pix.width);
                UVC_INFO("mt->fmt.pix.height     =%u\n",vformat.fmt.pix.height);
                UVC_INFO("mt->fmt.pix.sizeimage  =%u\n",vformat.fmt.pix.sizeimage);
            }
            /* Request UVC buffers & mmap. */
            s32Ret = _UVC_Video_ReqBufs(1);
            if (s32Ret < 0)
                goto done;

           /* And now Stream On the V4l2 Device */
           s32Ret = ioctl(fd, VIDIOC_STREAMON, &buftype);
           if (s32Ret < 0) {
               UVC_ERR("Unable to start streaming %s (%d).\n",
                       strerror(errno), errno);
               goto done;
           }
           UVC_SET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
        }
        printf("UVC: Start video stream Successfully.\n");
    }

done:
    UP();
    return ST_UVC_SUCCESS;
}

static void _UVC_SYS_Exit(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return;

    int32_t s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;

    if(IS_NULL(fops.UVC_Inputdev_Deinit))
        return;

    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) )
    {
       s32Ret = fops.UVC_Inputdev_Deinit(uvc);
       if(0>s32Ret)
           return;
       UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_INITIAL);
    }
}

static void _UVC_StopCapture(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return;

    int32_t s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;
    if(IS_NULL(fops.UVC_StopCapture))
        return;

/* Stop Input streaming... */
    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
    {
        UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
        s32Ret = fops.UVC_StopCapture(uvc);
        if(0 > s32Ret)
        {
            UVC_ERR("Failed to Stop Stream, s32Ret: %d\n", s32Ret);
            return;
        }
    }
/* Stream Off the V4l2 Device */
    _UVC_Video_Stream_on_off(uvc->uvc_fd,0);
}

static int8_t _UVC_SYS_Init(void)
{
    if(IS_NULL(uvc))
        return -EINVAL;

    int32_t s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;
    if(IS_NULL(fops.UVC_Inputdev_Init))
        return -EINVAL;

    if(false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL))
    {
        s32Ret = fops.UVC_Inputdev_Init(uvc);
        if(s32Ret < 0)
            return -EINVAL;
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_INITIAL);
    }
    return ST_UVC_SUCCESS;
}

static int8_t _UVC_StartCapture(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;

    int32_t s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;

    if(IS_NULL(fops.UVC_StartCapture))
        return -EINVAL;

/* Start Input Video capturing now. */
    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
    {
        _UVC_StopCapture();
    }

    s32Ret = fops.UVC_StartCapture(uvc,uvc->stream_param);
    if (s32Ret < 0)
    {
        UVC_ERR("Failed to Start Stream, s32Ret: %d\n", s32Ret);
        goto err;
    }
    UVC_SET_STATUS(uvc->status,UVC_INTDEV_STREAMON);

/* Stream On V4l2 Device */
    _UVC_Video_Stream_on_off(uvc->uvc_fd,1);

    return ST_UVC_SUCCESS;
err:
    return s32Ret;
}

static int8_t _UVC_Events_Process_Control(ST_UVC_Device_t *uvc,uint8_t req,uint8_t cs,
                                         uint8_t entity_id,uint8_t len,
                                         struct uvc_request_data *resp)
{
    uvc->control.ctype = UVC_CONTROL_INTERFACE;
    uvc->control.entity = entity_id;
    uvc->control.control = cs;
    uvc->control.length = len;

    switch (entity_id) {
    case UVC_VC_SELECTOR_UNIT_ID:
        switch (cs) {
        case UVC_VC_REQUEST_ERROR_CODE_CONTROL:
            /* Send the request error code last prepared. */
            resp->data[0] = uvc->request_error_code.data[0];
            resp->length  = uvc->request_error_code.length;
            break;

        default:
            /*
             * If we were not supposed to handle this
             * 'cs', prepare an error code response.
             */
            uvc->request_error_code.data[0] = 0x06;
            uvc->request_error_code.length = 1;
            break;
        }
        break;

    /* Camera terminal unit 'UVC_VC_INPUT_TERMINAL'. */
    case UVC_VC_INPUT_TERMINAL_ID:
        switch (cs) {
        /*
         * We support only 'UVC_CT_AE_MODE_CONTROL' for CAMERA
         * terminal, as our bmControls[0] = 2 for CT. Also we
         * support only auto exposure.
         */
        case UVC_CT_AE_MODE_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                /* Incase of auto exposure, attempts to
                 * programmatically set the auto-adjusted
                 * controls are ignored.
                 */
                resp->data[0] = 0x01;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;

            case UVC_GET_INFO:
                /*
                 * TODO: We support Set and Get requests, but
                 * don't support async updates on an video
                 * status (interrupt) endpoint as of
                 * now.
                 */
                resp->data[0] = 0x03;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;

            case UVC_GET_CUR:
            case UVC_GET_DEF:
            case UVC_GET_RES:
                /* Auto Mode â?? auto Exposure Time, auto Iris. */
                resp->data[0] = 0x02;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            default:
                /*
                 * We don't support this control, so STALL the
                 * control ep.
                 */
                resp->length = -EL2HLT;
                /*
                 * For every unsupported control request
                 * set the request error code to appropriate
                 * value.
                 */
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
            break;
        case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                resp->data[0] = 0x64;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_CUR:
                resp->data[0] = 0x64;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_INFO:
                resp->data[0] = 0x0f;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = 0x64;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = 0x60;
                resp->data[1] = 0x09;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = 0x64;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_DEF:
                resp->data[0] = 0x2c;
                resp->data[1] = 0x01;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
            default:
                resp->length = -EL2HLT;
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
            break;
        case UVC_CT_IRIS_ABSOLUTE_CONTROL:
                resp->length = -EL2HLT;
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
            break;
        case UVC_CT_ZOOM_ABSOLUTE_CONTROL:
            switch (req) {
            case UVC_GET_INFO:
                resp->data[0] = 0x0b;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = 0x00;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = 0x1;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = 0x1;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_SET_CUR:
            case UVC_GET_CUR:
            case UVC_GET_DEF:
                resp->data[0] = 0x00;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
            default:
                resp->length = -EL2HLT;
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
            break;
        default:
            /*
             * We don't support this control, so STALL the control
             * ep.
             */
            resp->length = -EL2HLT;
            /*
             * If we were not supposed to handle this
             * 'cs', prepare a Request Error Code response.
             */
            uvc->request_error_code.data[0] = 0x06;
            uvc->request_error_code.length = 1;
            break;
        }
        break;

    /* processing unit 'UVC_VC_PROCESSING_UNIT' */
    case UVC_VC_PROCESSING_UNIT_ID:
        switch (cs) {
        /*
         * We support only 'UVC_PU_BRIGHTNESS_CONTROL' for Processing
         * Unit, as our bmControls[0] = 1 for PU.
         */
        case UVC_PU_BACKLIGHT_COMPENSATION_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                resp->data[0] = 0x0;
                resp->length = len;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = 0x0;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = 0x1;
                resp->data[1] = 0x0;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_CUR:
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_INFO:
                /*
                 * We support Set and Get requests and don't
                 * support async updates on an interrupt endpt
                 */
                resp->data[0] = 0x03;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_DEF:
                resp->data[0] = 0x2;
                resp->data[1] = 0x0;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = 0x1;
                resp->data[1] = 0x0;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            default:
                /*
                 * We don't support this control, so STALL the
                 * default control ep.
                 */
                resp->length = -EL2HLT;
                /*
                 * For every unsupported control request
                 * set the request error code to appropriate
                 * code.
                 */
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
        case UVC_PU_BRIGHTNESS_CONTROL:
            switch (req) {
            case UVC_SET_CUR:
                resp->data[0] = 0x0;
                resp->length = len;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                //resp->data[0] = PU_BRIGHTNESS_MIN_VAL;
                resp->data[0] = 0;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
            //  resp->data[0] = PU_BRIGHTNESS_MAX_VAL;
                resp->data[0] = 255;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_CUR:
                resp->length = 2;
            //  memcpy(&resp->data[0], &brightness_val,
            //          resp->length);
                /*
                 * For every successfully handled control
                 * request set the request error code to no
                 * error
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_INFO:
                /*
                 * We support Set and Get requests and don't
                 * support async updates on an interrupt endpt
                 */
                resp->data[0] = 0x03;
                resp->length = 1;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_DEF:
            //  resp->data[0] = PU_BRIGHTNESS_DEFAULT_VAL;
                resp->data[0] = 127;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
            //  resp->data[0] = PU_BRIGHTNESS_STEP_SIZE;
                resp->data[0] = 1;
                resp->length = 2;
                /*
                 * For every successfully handled control
                 * request, set the request error code to no
                 * error.
                 */
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            default:
                /*
                 * We don't support this control, so STALL the
                 * default control ep.
                 */
                resp->length = -EL2HLT;
                /*
                 * For every unsupported control request
                 * set the request error code to appropriate
                 * code.
                 */
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
            break;

case UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
            switch (req) {
            case UVC_GET_INFO:
                resp->data[0] = 0x0f;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MIN:
                resp->data[0] = 0xf0;
                resp->data[0] = 0x0a;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_MAX:
                resp->data[0] = 0x64;
                resp->data[1] = 0x19;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_GET_RES:
                resp->data[0] = 0x3a;
                resp->data[1] = 0x07;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            case UVC_SET_CUR:
            case UVC_GET_CUR:
            case UVC_GET_DEF:
                resp->data[0] = 0x19;
                resp->data[0] = 0x64;
                resp->length = len;
                uvc->request_error_code.data[0] = 0x00;
                uvc->request_error_code.length = 1;
                break;
            default:
                resp->length = -EL2HLT;
                uvc->request_error_code.data[0] = 0x07;
                uvc->request_error_code.length = 1;
                break;
            }
            break;
        default:
            /*
             * We don't support this control, so STALL the control
             * ep.
             */
            resp->length = -EL2HLT;
            /*
             * If we were not supposed to handle this
             * 'cs', prepare a Request Error Code response.
             */
            uvc->request_error_code.data[0] = 0x06;
            uvc->request_error_code.length = 1;
            break;
        }
        break;
    /* vendor unit 'UVC_VC_EXTENSION_UNIT' */
    case UVC_VC_EXTENSION1_UNIT_ID:
        if (0 == usb_vc_eu1_cs(cs, req, resp))
        {
            resp->length = len;
            uvc->request_error_code.data[0] = 0x00;
            uvc->request_error_code.length = 1;
        } else
        {
            resp->length = -EL2HLT;
            uvc->request_error_code.data[0] = 0x06;
            uvc->request_error_code.length = 1;
        }
        break;
    case UVC_VC_EXTENSION2_UNIT_ID:
        resp->length = -EL2HLT;
        uvc->request_error_code.data[0] = 0x06;
        uvc->request_error_code.length = 1;
        break;
    /* output unit 'UVC_VC_OUTPUT_TERMINAL' */
    case UVC_VC_OUTPUT_TERMINAL_ID:
        resp->length = -EL2HLT;
        uvc->request_error_code.data[0] = 0x06;
        uvc->request_error_code.length = 1;
        break;
    default:
        /*
         * If we were not supposed to handle this
         * 'cs', prepare a Request Error Code response.
         */
        uvc->request_error_code.data[0] = 0x06;
        uvc->request_error_code.length = 1;
        break;

    }
    if(resp->length < 0)
    {
        resp->data[0] = 0x5;
        resp->length = len;
    }

    UVC_INFO("control request (req %02x cs %02x) ]\n",req, cs);
    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Events_Process_Streaming(ST_UVC_Device_t *uvc,uint8_t req, uint8_t cs,
                                            struct uvc_request_data *resp)
{
    struct uvc_streaming_control *ctrl;

    if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
        return -EINVAL;

    ctrl = (struct uvc_streaming_control *)&resp->data;
    resp->length = sizeof *ctrl;
    uvc->control.ctype = UVC_STREAMING_INTERFACE;

    switch (req) {
    case UVC_SET_CUR:
        uvc->control.control = cs;
        resp->length = 19;
        break;

    case UVC_GET_CUR:
        if (cs == UVC_VS_PROBE_CONTROL)
            memcpy(ctrl, &uvc->probe, sizeof *ctrl);
        else
            memcpy(ctrl, &uvc->commit, sizeof *ctrl);
        break;

    case UVC_GET_MIN:
    case UVC_GET_MAX:
    case UVC_GET_DEF:
        _UVC_Fill_Streaming_Control(ctrl, req == UVC_GET_MAX ? -1 : 0,
                       req == UVC_GET_MAX ? -1 : 0);
        break;

    case UVC_GET_RES:
        CLEAR(ctrl);
        break;

    case UVC_GET_LEN:
        resp->data[0] = 0x00;
        resp->data[1] = 0x22;
        resp->length = 2;
        break;

    case UVC_GET_INFO:
        resp->data[0] = 0x03;
        resp->length = 1;
        break;
    }
    return ST_UVC_SUCCESS;
}
/* ctrl request:
 * wValue: the wValue field contains the Control Selector (CS) in the high byte
 * wIndex: high byte for entity or zero,low byte for endpoint or interface
 *
 * */
static int8_t _UVC_Events_Process_Class(ST_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                        struct uvc_request_data *resp)
{
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
        return -EINVAL;

    UVC_SET_STATUS(uvc->status,UVC_DEVICE_ENUMURATED);
    UVC_INFO("bRequest %x wValue %x wIndex %x wLength %x \n",
                    ctrl->bRequest,ctrl->wValue >> 8,ctrl->wIndex >> 8,ctrl->wLength);
    switch (ctrl->wIndex & 0xff) {
    case UVC_INTF_CONTROL:
        _UVC_Events_Process_Control(uvc,
                        ctrl->bRequest,     // req
                        ctrl->wValue >> 8,  // CS
                        ctrl->wIndex >> 8,  // entity_id
                        ctrl->wLength,      // Length.
                        resp);              // resp.
        break;

    case UVC_INTF_STREAMING:
        _UVC_Events_Process_Streaming(uvc,
                        ctrl->bRequest,
                        ctrl->wValue >> 8, resp);
        break;

    default:
        break;
    }
    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Events_Process_Standard(ST_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                           struct uvc_request_data *resp)
{
    (void)ctrl;
    (void)resp;
    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Events_Process_Setup(struct usb_ctrlrequest *ctrl,
                                        struct uvc_request_data *resp)
{
    UVC_INFO("( bRequestType %02x bRequest %02x wValue %04x wIndex %04x wLength %04x )\n",
                  ctrl->bRequestType, ctrl->bRequest,ctrl->wValue, ctrl->wIndex, ctrl->wLength);

    switch (ctrl->bRequestType & USB_TYPE_MASK) {
    case USB_TYPE_STANDARD:
        _UVC_Events_Process_Standard(uvc,ctrl, resp);
        break;

    case USB_TYPE_CLASS:
        _UVC_Events_Process_Class(uvc,ctrl, resp);
        break;

    default:
        break;
    }
    return ST_UVC_SUCCESS;
}

// process PU, CT, XU job.
int8_t usb_vc_out_data(uint8_t entity, uint8_t cs, uint32_t len, struct uvc_request_data * data)
{
    switch (entity)
    {
    case UVC_VC_EXTENSION1_UNIT_ID:
        usb_vc_eu1_cs_out(entity, cs, len, data);
        break;
    case UVC_VC_EXTENSION2_UNIT_ID:
        printf("Enter UVC_VC_EXTENSION2_UNIT_ID\n");
        break;
    default:
        break;
    }

    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Events_Process_Data(struct uvc_request_data *data)
{
    struct uvc_streaming_control *ctrl = NULL;

    if(UVC_STREAMING_INTERFACE == uvc->control.ctype){
        switch (uvc->control.control) {
        case UVC_VS_PROBE_CONTROL:
            UVC_INFO(" Probe control, length = %d \n",data->length);
            ctrl = (struct uvc_streaming_control *)&data->data;
            uvc->probe = _UVC_Fill_Streaming_Control(ctrl,0,0);
            break;

        case UVC_VS_COMMIT_CONTROL:
            UVC_INFO(" Commit control, length = %d \n",data->length);
            ctrl = (struct uvc_streaming_control *)&data->data;
            uvc->commit = _UVC_Fill_Streaming_Control(ctrl,0,0);
            break;

        default:
            UVC_INFO(" unknown control, length = %d \n",data->length);
            break;
        }
    } else
    if(UVC_CONTROL_INTERFACE == uvc->control.ctype)
    {
        usb_vc_out_data(uvc->control.entity, uvc->control.control, uvc->control.length, data);
    } else /* Control Class Interface */
    {
        UVC_INFO(" unknown control type: %d\n", uvc->control.ctype);
    }

    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Events_Process()
{
    struct v4l2_event v4l2_event;
    struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
    struct uvc_request_data resp;
    int32_t uvc_fd = uvc->uvc_fd;
    int32_t s32Ret;
    s32Ret= ioctl(uvc_fd, VIDIOC_DQEVENT, &v4l2_event);
    if (s32Ret< 0) {
        UVC_ERR("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
            errno);
        return s32Ret;
    }

    memset(&resp, 0, sizeof resp);
    resp.length = -EL2HLT;

    switch (v4l2_event.type) {
    case UVC_EVENT_CONNECT:
        uvc->exit_request = 0;
        UVC_INFO("Possible USB  requested from "
                        "Host, seen via UVC_EVENT_CONNECT\n");
        return ST_UVC_SUCCESS;

    case UVC_EVENT_DISCONNECT:
        uvc->exit_request = 1;
        UVC_INFO("Possible USB shutdown requested from "
                "Host, seen via UVC_EVENT_DISCONNECT\n");
        return ST_UVC_SUCCESS;

    case UVC_EVENT_SETUP:
        _UVC_Events_Process_Setup(&uvc_event->req, &resp);
        break;

    case UVC_EVENT_DATA:
        s32Ret= _UVC_Events_Process_Data(&uvc_event->data);
        if (s32Ret< 0)
            break;
        return ST_UVC_SUCCESS;

    case UVC_EVENT_STREAMON:
        /* Only Isoc mode can be here */
        _UVC_StartCapture();
        return ST_UVC_SUCCESS;

    case UVC_EVENT_STREAMOFF:
        /* Stop Input streaming... */
        _UVC_StopCapture();
        return ST_UVC_SUCCESS;
    }

    s32Ret= ioctl(uvc_fd, UVCIOC_SEND_RESPONSE, &resp);
    if (s32Ret< 0) {
        UVC_ERR("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno),errno);
        return s32Ret;
    }

    return ST_UVC_SUCCESS;
}

static int8_t _UVC_Video_Process(void)
{
    int32_t s32Ret = ST_UVC_SUCCESS;
    struct buffer *mem = NULL;

    /*
     * Return immediately if UVC video output device has not started
     * streaming yet.
     */
    if(false==UVC_INPUT_ISENABLE(uvc->status)  ||
       false==UVC_OUTPUT_ISENABLE(uvc->status) )
    {
        usleep(100);
        return -1;
    }

    DOWN();
    mem = _UVC_DQBuf(0);
    if(NULL == mem)
    {
         s32Ret = -1;
    }

    if(ST_UVC_SUCCESS == s32Ret)
    {
         s32Ret = _UVC_QBuf(mem);
    }

    UP();
    return s32Ret;
}

/* when uvc_streaming_control is no NULL ,use c for ctrl setting
 *   iframe   : 0 - max
 *   ifrormat : 0 - max
 * */
static struct uvc_streaming_control
_UVC_Fill_Streaming_Control(struct uvc_streaming_control *c,int32_t iframe, int32_t iformat)
{
    int32_t nframes = 0;
    const uint32_t *interval = NULL;
    const struct uvc_format_info *format = NULL;
    const struct uvc_frame_info *frame = NULL;
    struct uvc_streaming_control ctrl;

    memset(&ctrl, 0, sizeof ctrl);
    /* Get Stream iFormat */
    if (NULL!=c) {
        iformat = clamp((unsigned int)c->bFormatIndex, 1U,
            (unsigned int)ARRAY_SIZE(uvc_formats)) - 1;
    }

    if (iformat < 0)
        iformat = ARRAY_SIZE(uvc_formats) + iformat;

    if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats)){
        UVC_ERR("No Support Format\n");
        return ctrl;
    }
    format = &uvc_formats[iformat];

    /* Get Stream iFrame */
    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    if (NULL!=c) {
        iframe = clamp((unsigned int)c->bFrameIndex, 1U, nframes) - 1;
    }

    if (iframe < 0)
        iframe = nframes + iframe;

    if (iframe < 0 || iframe >= (int)nframes){
        UVC_ERR("No Support Format\n");
        return ctrl;
    }
    frame = &format->frames[iframe];

    /* Get Frame Interval */
    interval = frame->intervals;
    if (NULL!=c) {
        while (interval[0] < c->dwFrameInterval && interval[1])
            ++interval;
    } else {
        interval += 0;
    }

    /* Commit the Ctrl data */
    ctrl.bmHint = 1;
    ctrl.bFormatIndex = iformat + 1;
    ctrl.bFrameIndex = iframe + 1;
    ctrl.dwFrameInterval = *interval;
    switch (format->fcc) {
    case V4L2_PIX_FMT_YUYV:
        ctrl.dwMaxVideoFrameSize = frame->width*frame->height * 2.0;
        break;
    case V4L2_PIX_FMT_NV12:
        ctrl.dwMaxVideoFrameSize = frame->width*frame->height * 1.5;
        break;
    case V4L2_PIX_FMT_MJPEG:
        ctrl.dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/6;
        break;
    case V4L2_PIX_FMT_H264:
        ctrl.dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/7;
        break;
    case V4L2_PIX_FMT_H265:
        ctrl.dwMaxVideoFrameSize = (frame->width*frame->height *2.0)/8;
        break;
    }

    if (uvc->ChnAttr.setting.mode == USB_BULK_MODE)
        ctrl.dwMaxPayloadTransferSize = ctrl.dwMaxVideoFrameSize;
    else
    /*
     * ctrl.dwMaxPayloadTransferSize = (dev->maxpkt) *
     *                      (dev->mult + 1) * (dev->burst + 1);
     */
        ctrl.dwMaxPayloadTransferSize = 1024 * (2 + 1) * (0 + 1);

    ctrl.bmFramingInfo = 3;
    ctrl.bPreferedVersion = 1;
    ctrl.bMaxVersion = 1;

    /* Finally Set the Strem Param For the Device */
    uvc->stream_param.fcc       = format->fcc ;
    uvc->stream_param.iformat   = iformat;
    uvc->stream_param.iframe    = iframe;
    uvc->stream_param.width     = frame->width;
    uvc->stream_param.height    = frame->height;
    uvc->stream_param.frameRate = FrameInterval2FrameRate(ctrl.dwFrameInterval);
    uvc->stream_param.maxframesize = ctrl.dwMaxVideoFrameSize;
    UVC_INFO(" (iformat %d iframe %d width %d height %d FrameRate %f)\n",
                uvc->stream_param.iformat,uvc->stream_param.iframe,uvc->stream_param.width,
                uvc->stream_param.height,uvc->stream_param.frameRate);

    if( uvc->control.control == UVC_VS_COMMIT_CONTROL &&
        uvc->ChnAttr.setting.mode == USB_BULK_MODE )
    {
        if( _UVC_StartCapture() < 0)
            UVC_ERR("Some err Occur at _UVC_StartCapture\n");
    }
    return ctrl;
}

static void * UVC_Event_Handle_Task(void *arg)
{
     int32_t s32Ret = -1;
     fd_set fdsu;
     int32_t uvc_fd = uvc->uvc_fd;
     struct timeval timeout;

     while(UVC_DEVICE_ISREADY(uvc->status))
     {
         FD_ZERO(&fdsu);

         /* We want both setup and data events on UVC interface.. */
         FD_SET(uvc_fd, &fdsu);
         fd_set efds = fdsu;

         timeout.tv_sec  = 2;
         timeout.tv_usec = 0;
         s32Ret = select(uvc_fd + 1, NULL, NULL, &efds, &timeout);

         if (-1 == s32Ret) {
             UVC_ERR("select error %d, %s\n",errno, strerror (errno));
              if (EINTR == errno)
                  return NULL;
         }

         if (0 == s32Ret){
             UVC_INFO("select timeout, device status %x \n",uvc->status);
             continue;
         }

         if (FD_ISSET(uvc_fd, &efds))
             _UVC_Events_Process();
    }
    return NULL;
}

static void * UVC_Video_Process_Task(void *arg)
{
     int32_t s32Ret = -1;
     fd_set fdsu;
     int32_t uvc_fd = uvc->uvc_fd;
     struct timeval timeout;

     while(UVC_DEVICE_ISREADY(uvc->status))
     {
         /* queue buf from user */
         _UVC_Video_QBuf();

         FD_ZERO(&fdsu);

         /* We want both setup and data events on UVC interface.. */
         FD_SET(uvc_fd, &fdsu);
         fd_set dfds = fdsu;

         timeout.tv_sec  = 2;
         timeout.tv_usec = 0;
         s32Ret = select(uvc_fd + 1, NULL,&dfds, NULL, &timeout);

         if (-1 == s32Ret) {
             UVC_ERR("select error %d, %s\n",errno, strerror (errno));
              if (EINTR == errno)
                  return NULL;
         }

         if (FD_ISSET(uvc_fd, &dfds))
             _UVC_Video_Process();
    }
    return NULL;
}

int32_t ST_UVC_Init(char *uvc_name)
{
    UVC_INFO("\n");

    int32_t s32Ret = -1;
    struct v4l2_event_subscription sub;
    struct v4l2_capability cap;

/* Malloc A UVC DEVICE */
    if(IS_NULL(uvc))
        uvc = (ST_UVC_Device_t *)malloc(sizeof(ST_UVC_Device_t));
    else
    {
        UVC_WRN("UVC_Device already Init\n");
        return ST_UVC_SUCCESS;
    }

/* Set default Streaming control */
    uvc->stream_param.iframe  = 1;
    uvc->stream_param.iformat = 1;
    uvc->stream_param.fcc = V4L2_PIX_FMT_MJPEG;

    uvc->control.control = UVC_VS_PROBE_CONTROL;

    uvc->probe  = _UVC_Fill_Streaming_Control(NULL,
                    uvc->stream_param.iformat,uvc->stream_param.iframe);
    uvc->commit = _UVC_Fill_Streaming_Control(NULL,
                    uvc->stream_param.iformat,uvc->stream_param.iframe);
    UVC_INFO("( probe iformat %d iframe %d )\n",
                    uvc->probe.bFormatIndex,uvc->probe.bFrameIndex);
    UVC_INFO("( Probe dwFrameInterval %d frameRate %f )\n",
                    uvc->probe.dwFrameInterval,uvc->stream_param.frameRate);

/* Init UVC Specific */
    CLEAR(uvc->status);

/* Start Init the UVC DEVICE */
   /* open the uvc device */
    if(IS_NULL(uvc_name))
    {
        UVC_ERR("the uvc can't be opened \n");
        goto err1;
    }

    /* It seems strange,but you have to double open it for reset signal */
    uvc->uvc_fd = open(uvc_name, O_RDWR | O_NONBLOCK);
  //  close(uvc->uvc_fd);
  //  uvc->uvc_fd = open(uvc_name, O_RDWR | O_NONBLOCK);

    if (uvc->uvc_fd < 0) {
        UVC_ERR("device open failed: %s (%d).\n",
               strerror(errno), errno);
        goto err1;
    }
   /* query uvc device */
    s32Ret = ioctl(uvc->uvc_fd, VIDIOC_QUERYCAP, &cap);
    if (s32Ret < 0) {
        UVC_ERR("unable to query uvc device: %s (%d)\n",
                strerror(errno), errno);
        goto err2;
    }
   /* check the device type */
    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        UVC_ERR("%s is no video output device\n", uvc_name);
        goto err2;
    }
    UVC_INFO("device is %s on bus %s\n", cap.card, cap.bus_info);
    UVC_INFO("open succeeded, file descriptor = %d\n", uvc->uvc_fd);

    uvc->exit_request = 0;

   /* Set default Function Operations */
    memset(&(uvc->ChnAttr.fops),0x00,sizeof(uvc->ChnAttr.fops));

   /* add the subscribe event to the uvc */
    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_CONNECT;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DISCONNECT;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);

    UVC_SET_STATUS(uvc->status,UVC_DEVICE_INITIAL);
    return ST_UVC_SUCCESS;

err2:
   close(uvc->uvc_fd);
err1:
   return -1;
}

int32_t ST_UVC_Uninit(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return ST_UVC_SUCCESS;

    if(true==UVC_GET_STATUS(uvc->status,UVC_DEVICE_INITIAL))
    {
        close(uvc->uvc_fd);
        free(uvc);
        uvc = NULL;
    }

    return ST_UVC_SUCCESS;
}

int32_t ST_UVC_CreateDev(const ST_UVC_ChnAttr_t* pstAttr)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
    {
        UVC_ERR("Please Init UVC Device First\n");
        return -EINVAL;
    }


    ST_UVC_Setting_t setting = pstAttr->setting;
    ST_UVC_OPS_t fops = pstAttr->fops;

    if(IS_NULL(fops.UVC_Inputdev_Init)   ||
       IS_NULL(fops.UVC_Inputdev_Deinit) ||
       IS_NULL(fops.UVC_StartCapture)    ||
       IS_NULL(fops.UVC_StopCapture))
    {
        if(UVC_MEMORY_MMAP==setting.io_mode)

        UVC_ERR("Err: Invalid Param \n");
        return -EINVAL;
    }

    if(UVC_MEMORY_USERPTR==setting.io_mode &&
       (IS_NULL(fops.u.UVC_DevFillBuffer)))
    {
        UVC_ERR("Err: Invalid Param \n");
        return -EINVAL;
    }
    else if(UVC_MEMORY_MMAP==setting.io_mode &&
            IS_NULL(fops.m.UVC_DevFillBuffer)){
        UVC_ERR("Err: Invalid Param \n");
        return -EINVAL;
    }

    uvc->ChnAttr.fops = fops;
    uvc->ChnAttr.setting = setting;

    /* Init the Video Input SYSTEM */
    _UVC_SYS_Init();

    pthread_mutex_init(&mm_lock, NULL);
    return ST_UVC_SUCCESS;
}

int32_t ST_UVC_DestroyDev()
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
    {
        UVC_ERR("Please Init UVC Device First\n");
        return -EINVAL;
    }
    /* Stop Device First*/
    _UVC_StopCapture();

    /* Uninit Device */
    _UVC_SYS_Exit();

    pthread_mutex_destroy(&mm_lock);
    return ST_UVC_SUCCESS;
};

/* Some Attr todo */
int32_t ST_UVC_SetChnAttr(const ST_UVC_ChnAttr_t* pstAttr)
{
    UVC_INFO("\n");

    return ST_UVC_SUCCESS;
};

/* Some Attr todo */
int32_t ST_UVC_GetChnAttr(const ST_UVC_ChnAttr_t* pstAttr)
{
    UVC_INFO("\n");

    return ST_UVC_SUCCESS;
};

int32_t ST_UVC_StartDev(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;

    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) &&
       false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_STARTED);

        pthread_create(&uvc->event_handle_thread, NULL, UVC_Event_Handle_Task, NULL);
        pthread_setname_np(uvc->event_handle_thread , "UVC_Event_Handle_Task");

        pthread_create(&uvc->video_process_thread, NULL, UVC_Video_Process_Task, NULL);
        pthread_setname_np(uvc->video_process_thread , "UVC_Video_Process_Task");
        return ST_UVC_SUCCESS;
    } else
        return -1;
};

int32_t ST_UVC_StopDev(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;

    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STARTED);
    }
    pthread_join(uvc->event_handle_thread, NULL);
    pthread_join(uvc->video_process_thread, NULL);
    return ST_UVC_SUCCESS;
};

void uvc_save_file(void *buf,uint32_t length,char type)
{
    if(NULL == buf || 0 >= length)
       return;

    FILE *UVCFile = NULL;
    char FileName[120];

    switch(type){
    case 0:
        snprintf(FileName, sizeof(FileName) - 1, "myusb.img");
        UVCFile = fopen(FileName, "w+");
       break;
    case 1:
        snprintf(FileName, sizeof(FileName) - 1, "myusb.es");
        UVCFile = fopen(FileName, "a+");
       break;
    default:
       break;
    }
    fwrite(buf,length,1, UVCFile);
    fclose(UVCFile);
}

void uvc_get_format(uint32_t fcc)
{
    switch(fcc)
    {
    case V4L2_PIX_FMT_YUYV:
        printf("Format is YUYV\n");
        break;
    case V4L2_PIX_FMT_NV12:
        printf("Format is NV12\n");
        break;
    case V4L2_PIX_FMT_MJPEG:
        printf("Format is MJPEG\n");
        break;
    case V4L2_PIX_FMT_H264:
        printf("Format is H264\n");
        break;
    case V4L2_PIX_FMT_H265:
        printf("Format is H265\n");
        break;
    default:
        printf("unkonown format\n");
        break;
    }
}
