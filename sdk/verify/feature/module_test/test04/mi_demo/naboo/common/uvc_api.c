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

#include "mi_uvc.h"

#if 0
void UVC_GetTime()
{
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
   struct timex  txc;
   struct rtc_time tm;
   do_gettimeofday(&(txc.time));
   rtc_time_to_tm(txc.time.tv_sec,&tm);
   printk(“UTC time :%d-%d-%d %d:%d:%d /n”,tm.tm_year+1900,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
}
#endif

//#define UVC_DEBUG_ENABLE

static MI_UVC_Device_t *uvc;


int ret =0;

static void UVC_Fill_Streaming_Control(struct uvc_streaming_control *ctrl,
                                       MI_S32 iframe, MI_S32 iformat);

static MI_S32  _UVC_StartCapture(StreamEvent_e is_on)
{
    MI_UVC_OPS_t fops = uvc->ChnAttr.fops;
    if( IS_NULL(fops.UVC_StartCapture ) ||
        IS_NULL(fops.UVC_Inputdev_Init) )
        return -EINVAL;

    switch(is_on)
    {
       case STREAMING_SYSTEM_EVENT:
          if( 0 == UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) )
          {
              fops.UVC_Inputdev_Init(uvc);
              UVC_SET_STATUS(uvc->status,UVC_INTDEV_INITIAL);
          }
          break;
       case STREAMING_SETTING_EVENT:
          if( 0 == UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON) )
          {
              fops.UVC_StartCapture(uvc,uvc->stream_param);
              UVC_SET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
          }
          break;
       default:
          break;
    }
    return MI_SUCCESS;
};

static MI_S32  _UVC_FillBuffer(MI_U32 *length,void *buf)
{
   MI_UVC_OPS_t fops = uvc->ChnAttr.fops;
   if(IS_NULL(fops.UVC_DevFillBuffer))
       return -EINVAL;
   else
       fops.UVC_DevFillBuffer(uvc,length,buf);

    return MI_SUCCESS;
};

static void _UVC_StopCapture(StreamEvent_e is_on)
{
    printf("INFO:%s \n",__func__);
    MI_UVC_OPS_t fops = uvc->ChnAttr.fops;

    if(IS_NULL(fops.UVC_Inputdev_Deinit) ||
       IS_NULL(fops.UVC_StopCapture))
        return;

    switch(is_on)
    {
    case STREAMING_SYSTEM_EVENT:
       if( UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) )
       {
          fops.UVC_Inputdev_Deinit(uvc);
          UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_INITIAL);
       }
       break;
    case STREAMING_SETTING_EVENT:
       if( UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON) )
       {
           fops.UVC_StopCapture(uvc);
           UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
       }
       break;
    default:
       break;
    }
    return;
};

static MI_S8 UVC_Video_QBuf(void)
{
	MI_U32 i;
	MI_S32 s32Ret;
    
    if(IS_NULL(uvc)){
         printf(" %s Invalid Arguments\n",__func__);
         return -EINVAL;
    }

    MI_S32 uvc_fd = uvc->uvc_fd;
    MI_UVC_Setting_t set = uvc->ChnAttr.setting;
    struct buffer *mem = uvc->mem;

	for (i = 0; i < set.nbuf; ++i) {
		memset(&mem[i].buf, 0, sizeof(mem[i].buf));

		mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		mem[i].buf.memory = V4L2_MEMORY_MMAP;
		mem[i].buf.index = i;
       
        // fill uvc buffer  MaxBufSize 153600
        s32Ret = _UVC_FillBuffer(&mem[i].buf.bytesused,mem[i].start);
        if( s32Ret <0 )
        {
            printf(" %s some thing error in fill data\n",__func__);
            return s32Ret;
        }
        
        // queue  the uvc buffer
		s32Ret = ioctl(uvc_fd, VIDIOC_QBUF, &(mem[i].buf));
		if (s32Ret < 0) {
			printf("UVC: VIDIOC_QBUF failed : %s (%d).\n",
					strerror(errno), errno);
			return s32Ret;
		}
	}
	return MI_SUCCESS;
}

static MI_S8 UVC_Video_ReqBufs(MI_S32 flags)
{
	struct v4l2_requestbuffers rb;
	MI_U32 i;
	MI_S32 s32Ret;

    struct buffer *mem = NULL;
    MI_UVC_Setting_t set = uvc->ChnAttr.setting;
    MI_S32 uvc_fd = uvc->uvc_fd;

	CLEAR(rb);

    if(1 == flags)
    {
        if( 0 == UVC_GET_STATUS(uvc->status,UVC_DEVICE_REQBUFS) )
        {
	        rb.count = set.nbuf;
            UVC_SET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
        } else
        {
           printf(" %s Release BUF First\n",__func__);
           return -1;
        }
    }
    else if(0 == flags)
    {
        if( UVC_GET_STATUS(uvc->status,UVC_DEVICE_REQBUFS) )
        {
            rb.count = 0;
            UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
        } else
        {
           printf(" %s Already Release Buf\n",__func__);
           return MI_SUCCESS;
        }
    }
    else
        return -EINVAL;

	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = V4L2_MEMORY_MMAP;

	s32Ret = ioctl(uvc_fd, VIDIOC_REQBUFS, &rb);
	if (s32Ret < 0) {
		if (s32Ret == -EINVAL)
			printf("UVC: does not support memory mapping\n");
		else
			printf("UVC: Unable to allocate buffers: %s (%d).\n",
					strerror(errno), errno);
		goto err;
	}

	if (!rb.count)
		return MI_SUCCESS;

	if (rb.count < 2) {
		printf("UVC: Insufficient buffer memory.\n");
		s32Ret = -EINVAL;
		goto err;
	}

	/* Map the buffers. */
	mem = (struct buffer*)calloc(rb.count, sizeof(mem[0]));
	if (!mem) {
		printf("UVC: Out of memory\n");
		s32Ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < rb.count; ++i) {
		memset(&mem[i].buf, 0, sizeof(mem[i].buf));

		mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		mem[i].buf.memory = V4L2_MEMORY_MMAP;
		mem[i].buf.index = i;

		s32Ret = ioctl(uvc_fd, VIDIOC_QUERYBUF, &(mem[i].buf));
		if (s32Ret < 0) {
			printf("UVC: VIDIOC_QUERYBUF failed for buf %d: "
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
			printf("[ %s UVC: Unable to map buffer %u: %s (%d). ]\n", __func__,i,
				strerror(errno), errno);
			mem[i].length = 0;
			s32Ret = -EINVAL;
			goto err_free;
		}

		mem[i].length = mem[i].buf.length;
#ifdef UVC_DEBUG_ENABLE
		printf("[ %s UVC: Buffer %u mapped at address %p mem length %d. ]\n",__func__, i,
				mem[i].start,mem[i].length);
#endif
	}

	uvc->ChnAttr.setting.nbuf = rb.count;
    uvc->mem = mem;
	printf("[ %s UVC: %u buffers allocated. ]\n",__func__, rb.count);
    
	return MI_SUCCESS;

err_free:
	free(mem);
err:
	return s32Ret;
}

static MI_S8 UVC_Video_Stream(MI_S32 fd,MI_S32 enable)
{
	MI_S32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	MI_S32 s32Ret;

	if (!enable) {
        if( 0 == UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON) )
           return MI_SUCCESS;
        else
        {
		   s32Ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
		   if (s32Ret < 0) {
		   	   printf("UVC: VIDIOC_STREAMOFF failed: %s (%d).\n",
			  		strerror(errno), errno);
			   return s32Ret;
		   }
           UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
        }
		printf("UVC: Stopping video stream.\n");
	    return MI_SUCCESS;
	} else {
        if( UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON) )
           return MI_SUCCESS;
        else
        {
           s32Ret = ioctl(fd, VIDIOC_STREAMON, &type);
	       if (s32Ret < 0) {
	  	       printf("UVC: Unable to start streaming %s (%d).\n",
		  	           strerror(errno), errno);
		       return s32Ret;
	       } 
           UVC_SET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
        }
        printf("[ UVC: Starting video stream.]\n");
        return MI_SUCCESS;
    }
	return MI_SUCCESS;
}

static MI_S8 UVC_Uninit_Device(void)
{
	MI_U32 i;
	MI_S32 ret;

	for (i = 0; i < uvc->ChnAttr.setting.nbuf; ++i) {
		ret = munmap(uvc->mem[i].start, uvc->mem[i].length);
		if (ret < 0) {
			printf("UVC: munmap failed\n");
			return ret;
		}
	}
	free(uvc->mem);

	return MI_SUCCESS;
}

/*flags: 0: free buf 1: request buf */
static void UVC_Handle_Streamoff_Event(MI_S32 is_on)
{
    MI_S32 uvc_fd = uvc->uvc_fd;
 
/* Stop Input streaming... */
    _UVC_StopCapture(is_on);

/* ... and now UVC streaming.. */
    if (UVC_OUTPUT_ISENABLE(uvc->status)) {
        UVC_Video_Stream(uvc_fd,0);
        UVC_Uninit_Device();
        UVC_Video_ReqBufs(0);
    }
}

static MI_S8 UVC_Handle_Streamon_Event(StreamEvent_e is_on)
{
	MI_S32 s32Ret;

printf("%s\n",__func__);
    if(IS_NULL(uvc))
        return -EINVAL;

    MI_S32 uvc_fd = uvc->uvc_fd;
/* IS SYSTEM Setting Event ? */
    if(!is_on)
    {
        _UVC_StartCapture(STREAMING_SYSTEM_EVENT);
        return MI_SUCCESS;
    }
           
/* Before Streamon ,First Streamoff */
    UVC_Handle_Streamoff_Event(STREAMING_SETTING_EVENT);

/* Start Input Video capturing now. */
    s32Ret = _UVC_StartCapture(STREAMING_SETTING_EVENT);
	if (s32Ret < 0)
		 goto err;

/* Request UVC buffers & mmap. */
    s32Ret = UVC_Video_ReqBufs(1);
    if (s32Ret < 0)
		goto err;

/* Queue buffers to UVC domain and start streaming. */
	s32Ret = UVC_Video_QBuf();
	if (s32Ret < 0)
		 goto err;
    UVC_Video_Stream(uvc_fd,1);

	return MI_SUCCESS;
err:
	return s32Ret;
}

#if 1
static MI_S8 _UVC_Events_Process_Control(MI_UVC_Device_t *uvc,MI_U8 req,MI_U8 cs,
                                         MI_U8 entity_id,MI_U8 len, 
                                         struct uvc_request_data *resp)
{
	switch (entity_id) {
	case 0:
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
	case 1:
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
	case 2:
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
	//			memcpy(&resp->data[0], &brightness_val,
	//					resp->length);
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
			//	resp->data[0] = PU_BRIGHTNESS_MAX_VAL;
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
			//	memcpy(&resp->data[0], &brightness_val,
			//			resp->length);
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
			//	resp->data[0] = PU_BRIGHTNESS_DEFAULT_VAL;
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
			//	resp->data[0] = PU_BRIGHTNESS_STEP_SIZE;
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

#ifdef UVC_DEBUG_ENABLE
	printf("[ %s control request (req %02x cs %02x) ]\n", __func__,req, cs);
#endif
	return MI_SUCCESS;
}

static MI_S8 _UVC_Events_Process_Streaming(MI_UVC_Device_t *uvc,MI_U8 req, MI_U8 cs,
                                            struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
		return -EINVAL;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof *ctrl;

	switch (req) {
	case UVC_SET_CUR:
		uvc->control =(ControlType_e)cs;
		resp->length = 34;
		break;

	case UVC_GET_CUR:
		if (cs == UVC_VS_PROBE_CONTROL)
			memcpy(ctrl, &uvc->probe, sizeof *ctrl);
		else
			memcpy(ctrl, &uvc->commit, sizeof *ctrl);
#ifdef UVC_DEBUG_ENABLE
        printf("Format Index  : %d\n",ctrl->bFormatIndex);  
        printf("Frame  Index  : %d\n",ctrl->bFrameIndex);
        printf("Frame  Interval   : %d\n",ctrl->dwFrameInterval);
        printf("Frame  Rate   : %d\n",ctrl->wPFrameRate);
#endif
		break;

	case UVC_GET_MIN:
	case UVC_GET_MAX:
	case UVC_GET_DEF:
		UVC_Fill_Streaming_Control(ctrl, req == UVC_GET_MAX ? -1 : 0,
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
	return MI_SUCCESS;
}

static MI_S8 _UVC_Events_Process_Class(MI_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                        struct uvc_request_data *resp)
{
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return -EINVAL;

#ifdef UVC_DEBUG_ENABLE
    printf("bRequest %x wValue %x wIndex %x wLength %x \n",
            ctrl->bRequest,ctrl->wValue >> 8,ctrl->wIndex >> 8,ctrl->wLength);
#endif
	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		_UVC_Events_Process_Control(uvc,
                                   ctrl->bRequest,
			    	     		   ctrl->wValue >> 8,
					        	   ctrl->wIndex >> 8,
						           ctrl->wLength, resp);
        UVC_SET_STATUS(uvc->status,UVC_DEVICE_ENUMURATED);
		break;

	case UVC_INTF_STREAMING:
		_UVC_Events_Process_Streaming(uvc,
                                     ctrl->bRequest,
						             ctrl->wValue >> 8, resp);
		break;

	default:
		break;
	}
    return MI_SUCCESS;
}

static MI_S8 _UVC_Events_Process_Standard(MI_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                           struct uvc_request_data *resp)
{
	(void)ctrl;
	(void)resp;
    return MI_SUCCESS;
}

static MI_S8 _UVC_Events_Process_Setup(struct usb_ctrlrequest *ctrl,
                                        struct uvc_request_data *resp)
{
#ifdef UVC_DEBUG_ENABLE
	printf("\n[ %s bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
		"wLength %04x ]\n",__func__, ctrl->bRequestType, ctrl->bRequest,
		ctrl->wValue, ctrl->wIndex, ctrl->wLength);
#endif

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
    return MI_SUCCESS;
}


static MI_S8 _UVC_Events_Process_Data(struct uvc_request_data *data)
{
	struct uvc_streaming_control *target = NULL ;
	struct uvc_streaming_control *ctrl = NULL;
	const struct uvc_format_info *format = NULL ;
	const struct uvc_frame_info *frame = NULL ;
	const MI_U32 *interval = NULL;
	MI_U32 iformat_tmp, iframe_tmp;
    MI_U32 nframes;
	MI_S32 ret;

	switch (uvc->control) {
	case UVC_VS_PROBE_CONTROL:
		printf("[ %s setting probe control, length = %d ]\n", __func__,data->length);
		target = &uvc->probe;
		break;

	case UVC_VS_COMMIT_CONTROL:
		printf("[ %s setting commit control, length = %d ]\n", __func__,data->length);
		target = &uvc->commit;
		break;

	default:
		printf("[ %s setting unknown control, length = %d ]\n", __func__,data->length);
    }

	ctrl = (struct uvc_streaming_control *)&data->data;
#if 1
if(uvc->control ==UVC_VS_PROBE_CONTROL)
   printf("UVC_VS_PROBE_CONTROL %u %u %u \n",ctrl->bFormatIndex,ctrl->bFrameIndex,ctrl->dwFrameInterval);
else
   printf("UVC_VS_COMMIT_CONTROL %u %u %u \n",ctrl->bFormatIndex,ctrl->bFrameIndex,ctrl->dwFrameInterval);
#endif
	iformat_tmp = clamp((unsigned int)ctrl->bFormatIndex, 1U,
			(unsigned int)ARRAY_SIZE(uvc_formats));

	format = &uvc_formats[iformat_tmp-1];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	iframe_tmp = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
	frame = &format->frames[iframe_tmp-1];
	interval = frame->intervals;

	while (interval[0] < ctrl->dwFrameInterval && interval[1])
		++interval;
	target->bFormatIndex = iformat_tmp;
	target->bFrameIndex = iframe_tmp;
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		target->dwMaxVideoFrameSize = frame->width * frame->height * 2.0;
		break;
	case V4L2_PIX_FMT_NV12:
		target->dwMaxVideoFrameSize = frame->width * frame->height * 1.5;
		break;
	case V4L2_PIX_FMT_MJPEG:
	case V4L2_PIX_FMT_H264:
	case V4L2_PIX_FMT_H265:
		target->dwMaxVideoFrameSize = ( frame->width * frame->height * 1.5 )/2;
		break;
	}
	target->dwFrameInterval = *interval;
	
    if (uvc->ChnAttr.setting.mode == USB_BULK_MODE)
	   target->dwMaxPayloadTransferSize = ctrl->dwMaxVideoFrameSize;
	else
	/*
     * ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) *
     *                      (dev->mult + 1) * (dev->burst + 1);
     */
       target->dwMaxPayloadTransferSize = 1024 * (0 + 1) * (0 + 1);

	if (uvc->control == UVC_VS_COMMIT_CONTROL) {
        uvc->stream_param.fcc       = format->fcc ;
        uvc->stream_param.iformat   = iformat_tmp;
        uvc->stream_param.iframe    = iframe_tmp;
		uvc->stream_param.width     = frame->width;
		uvc->stream_param.height    = frame->height;
        uvc->stream_param.frameRate = FrameInterval2FrameRate(target->dwFrameInterval);
#ifdef UVC_DEBUG_ENABLE
        printf("[ %s UVC_VS_COMMIT_CONTROL ]\n",__func__);
        printf("[ %s iformat %d iframe %d width %d height %d FrameRate %f ]\n",__func__,
                        uvc->stream_param.iformat,uvc->stream_param.iframe,uvc->stream_param.width,
                        uvc->stream_param.height,uvc->stream_param.frameRate);
#endif
    } else
        goto done;

    if(uvc->ChnAttr.setting.mode == USB_BULK_MODE)
    {
#ifdef UVC_DEBUG_ENABLE
        time_get_interval("uvc_handle_streamon_event");
#endif
	 	ret = UVC_Handle_Streamon_Event(STREAMING_SETTING_EVENT);
#ifdef UVC_DEBUG_ENABLE
        time_get_interval("uvc_handle_streamon_event");
#endif
	    if (ret < 0)
        {
			goto err;
        }
    }

done:
	return MI_SUCCESS;

err:
	return ret;
}

static MI_S8 UVC_Events_Process()
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
	struct uvc_request_data resp;
    MI_S32 uvc_fd = uvc->uvc_fd;
	MI_S32 ret;
printf("%s\n",__func__);
	ret = ioctl(uvc_fd, VIDIOC_DQEVENT, &v4l2_event);
	if (ret < 0) {
		printf("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
			errno);
		return ret;
	}

	memset(&resp, 0, sizeof resp);
	resp.length = -EL2HLT;

	switch (v4l2_event.type) {
	case UVC_EVENT_CONNECT:
		return MI_SUCCESS;

	case UVC_EVENT_DISCONNECT:
		CLEAR(uvc->status);
		printf("UVC: Possible USB shutdown requested from "
				"Host, seen via UVC_EVENT_DISCONNECT\n");
		return MI_SUCCESS;

	case UVC_EVENT_SETUP:
		_UVC_Events_Process_Setup(&uvc_event->req, &resp);
        break;

	case UVC_EVENT_DATA:
		ret = _UVC_Events_Process_Data(&uvc_event->data);
		if (ret < 0)
			break;
		return MI_SUCCESS;

	case UVC_EVENT_STREAMON:
        /* Only Isoc mode can be here */
printf("STream on\n");
		UVC_Handle_Streamon_Event(STREAMING_SETTING_EVENT);
		return MI_SUCCESS;

	case UVC_EVENT_STREAMOFF:
		/* Stop Input streaming... */
        printf(" UVC_EVENT_STREAMOFF \n");
		UVC_Handle_Streamoff_Event(STREAMING_SETTING_EVENT);
		return MI_SUCCESS;
	}

    ret = ioctl(uvc_fd, UVCIOC_SEND_RESPONSE, &resp);
    if (ret < 0) {
        printf("UVCIOC_S_EVENT failed: %s (%d)\n", strerror(errno),errno);
        return ret;
    }   

    return MI_SUCCESS;
}

static MI_S8 UVC_Video_Process(void)
{
	struct v4l2_buffer ubuf;
	MI_S32 s32Ret;
    MI_S32 uvc_fd     = uvc->uvc_fd;

	/*
	 * Return immediately if UVC video output device has not started
	 * streaming yet.
	 */
	if ( !UVC_INPUT_ISENABLE(uvc->status) || 
         !UVC_OUTPUT_ISENABLE(uvc->status) )
		return MI_SUCCESS;
	/* Prepare a v4l2 buffer to be dequeued from UVC domain. */
	CLEAR(ubuf);

	ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ubuf.memory = V4L2_MEMORY_MMAP;

	s32Ret = ioctl(uvc_fd, VIDIOC_DQBUF, &ubuf);
	if (s32Ret < 0)
    {
        printf("[ %s VIDIOC_DQBUF Fail  ret : %d ]\n",__func__,s32Ret);
		return s32Ret;
    }
#ifdef UVC_DEBUG_ENABLE
	printf("DeQueued buffer at UVC side = %d\n", ubuf.index);
#endif

    _UVC_FillBuffer(&ubuf.bytesused,uvc->mem[ubuf.index].start);
    if( s32Ret < 0 )
    {
        printf(" %s some thing error in fill data\n",__func__);
        return s32Ret;
    }
#ifdef UVC_DEBUG_ENABLE
    time_get_interval("UVC_FillBuffer");
#endif

	s32Ret = ioctl(uvc_fd, VIDIOC_QBUF, &ubuf);
	if (s32Ret < 0) {
		printf("UVC: Unable to queue buffer: %s (%d).\n",
				strerror(errno), errno);
		return s32Ret;
	}

#ifdef UVC_DEBUG_ENABLE
	printf("ReQueueing buffer at UVC side = %d\n", ubuf.index);
#endif

	return s32Ret;
}
#endif
static void UVC_Fill_Streaming_Control(struct uvc_streaming_control *ctrl,
                                       MI_S32 iframe, MI_S32 iformat)
{
	const struct uvc_format_info *format;
	const struct uvc_frame_info *frame;
    MI_S32 nframes;

	if (iformat < 0)
		iformat = ARRAY_SIZE(uvc_formats) + iformat;
	if (iformat < 0 || iformat >= (int)ARRAY_SIZE(uvc_formats))
		return;
	format = &uvc_formats[iformat];

	nframes = 0;
	while (format->frames[nframes].width != 0)
		++nframes;

	if (iframe < 0)
		iframe = nframes + iframe;
	if (iframe < 0 || iframe >= (int)nframes)
		return;
	frame = &format->frames[iframe];

	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = iformat + 1;
	ctrl->bFrameIndex = iframe + 1;
	ctrl->dwFrameInterval = frame->intervals[0];
	switch (format->fcc) {
	case V4L2_PIX_FMT_YUYV:
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2.0;
		break;
    case V4L2_PIX_FMT_NV12:
		ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 1.5;
		break;
	case V4L2_PIX_FMT_MJPEG:
	case V4L2_PIX_FMT_H264:
	case V4L2_PIX_FMT_H265:
		ctrl->dwMaxVideoFrameSize = ( frame->width * frame->height * 1.5 ) / 2;
		break;
	}

    if (uvc->ChnAttr.setting.mode == USB_BULK_MODE)
	    ctrl->dwMaxPayloadTransferSize = ctrl->dwMaxVideoFrameSize;
	else
	/*
     * ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) *
     *                      (dev->mult + 1) * (dev->burst + 1);
     */
        ctrl->dwMaxPayloadTransferSize = 1024 * (0 + 1) * (0 + 1);


	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;
}

static void * UVC_Event_Task(void *arg)
{
     MI_S32 ret = -1;
     fd_set fdsu;
     MI_S32 uvc_fd = uvc->uvc_fd;
     struct timeval timeout;

     while(UVC_DEVICE_ISREADY(uvc->status))
     {
         FD_ZERO(&fdsu);

         /* We want both setup and data events on UVC interface.. */
         FD_SET(uvc_fd, &fdsu);
         fd_set efds = fdsu;
         fd_set dfds = fdsu;

         timeout.tv_sec  = 2; 
         timeout.tv_usec = 0; 
         ret = select(uvc_fd + 1, NULL,&dfds, &efds, &timeout);

         if (-1 == ret) {
             printf("select error %d, %s\n",errno, strerror (errno));
              if (EINTR == errno)
                  return NULL;
          }

          if (0 == ret) {
              continue;
          }

          if (FD_ISSET(uvc_fd, &efds))
          {
              UVC_Events_Process();
          }

          if (FD_ISSET(uvc_fd, &dfds))
          {
              UVC_Video_Process();
          }
    }
    return NULL;
}
 
MI_S32 MI_UVC_Init(char *uvc_name)
{
    MI_S32 ret = -1;
    struct v4l2_event_subscription sub;
    struct v4l2_capability cap;
         
/* Malloc A UVC DEVICE */
    if(IS_NULL(uvc))
        uvc = (MI_UVC_Device_t *)malloc(sizeof(MI_UVC_Device_t));
    else
    {
        printf("UVC_Device already Init\n");
        return MI_SUCCESS;
    }

/* Set default Streaming control */
    uvc->stream_param.iframe  = 1;
    uvc->stream_param.iformat = 1;
    uvc->stream_param.fcc = V4L2_PIX_FMT_MJPEG;
    uvc->control = UVC_PROBE_CONTROL;
    UVC_Fill_Streaming_Control(&uvc->probe,uvc->stream_param.iformat,uvc->stream_param.iframe);
    UVC_Fill_Streaming_Control(&uvc->commit,uvc->stream_param.iformat,uvc->stream_param.iframe);
#ifdef UVC_DEBUG_ENABLE
    printf("[ %s probe iformat %d iframe %d dwFrameInterval %d frameRate %f ]\n",__func__,
           uvc->probe.bFormatIndex,uvc->probe.bFrameIndex,uvc->probe.dwFrameInterval,uvc->stream_param.frameRate);
#endif

/* Init UVC Specific */
   CLEAR(uvc->status);
 
/* Start Init the UVC DEVICE */
   /* open the uvc device */
    if(IS_NULL(uvc_name))
    {
        printf("[ %s the uvc can't be opened ]\n",__func__);
        goto err1;
    }
    /* It seems strange,but you have to double open it for reset signal */
    uvc->uvc_fd = open(uvc_name, O_RDWR | O_NONBLOCK);
    close(uvc->uvc_fd);
    uvc->uvc_fd = open(uvc_name, O_RDWR | O_NONBLOCK);

    if (uvc->uvc_fd < 0) {
        printf("UVC: device open failed: %s (%d).\n",
               strerror(errno), errno);
        goto err1; 
    }
   /* query uvc device */
    ret = ioctl(uvc->uvc_fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("UVC: unable to query uvc device: %s (%d)\n",
                strerror(errno), errno);
        goto err2;
    }
   /* check the device type */
    if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        printf("UVC: %s is no video output device\n", uvc_name);
        goto err2;
    }
#ifdef UVC_DEBUG_ENABLE
    printf("uvc device is %s on bus %s\n", cap.card, cap.bus_info);
    printf("uvc open succeeded, file descriptor = %d\n", uvc->uvc_fd);
#endif

   /* Set default Function Operations */
    uvc->ChnAttr.fops.UVC_Inputdev_Init    = NULL;
    uvc->ChnAttr.fops.UVC_Inputdev_Deinit  = NULL;
    uvc->ChnAttr.fops.UVC_DevFillBuffer    = NULL;
    uvc->ChnAttr.fops.UVC_StartCapture     = NULL;
    uvc->ChnAttr.fops.UVC_StopCapture      = NULL;
    
   /* add the subscribe event to the uvc */  
    memset(&sub, 0, sizeof sub);
    sub.type = UVC_EVENT_SETUP;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_DATA;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMON;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(uvc->uvc_fd, VIDIOC_SUBSCRIBE_EVENT, &sub);

    UVC_SET_STATUS(uvc->status,UVC_DEVICE_INITIAL);
    return MI_SUCCESS;

err2:
   close(uvc->uvc_fd);
err1:
   return -1;
}

MI_S32 MI_UVC_Uninit(void)
{
    if(IS_NULL(uvc))
        return MI_SUCCESS;

    if(UVC_GET_STATUS(uvc->status,UVC_DEVICE_INITIAL))
    { 
        close(uvc->uvc_fd);
        free(uvc);
    }

    return MI_SUCCESS;
}

MI_S32 MI_UVC_CreateDev(MI_UVC_CHANNEL Chn,MI_UVC_PORT PortID,const MI_UVC_ChnAttr_t* pstAttr)
{
    if(IS_NULL(uvc))
    {
        printf("Please Init UVC Device First\n");
        return -EINVAL;
    }
   
    MI_UVC_Setting_t setting = pstAttr->setting;
    MI_UVC_OPS_t fops = pstAttr->fops;

    if(IS_NULL(fops.UVC_Inputdev_Init)   ||
       IS_NULL(fops.UVC_Inputdev_Deinit) ||
       IS_NULL(fops.UVC_DevFillBuffer)   ||
       IS_NULL(fops.UVC_StartCapture)    ||
       IS_NULL(fops.UVC_StopCapture))
    {
        printf("Err: Invalid Param \n"); 
        return -EINVAL;
    }

   uvc->ChnAttr.fops = fops;
   uvc->ChnAttr.setting = setting;

   /* Init the Video Input SYSTEM */
   UVC_Handle_Streamon_Event(STREAMING_SYSTEM_EVENT);
   return MI_SUCCESS;
}

MI_S32 MI_UVC_DestroyDev(MI_UVC_CHANNEL Chn)
{
    if(IS_NULL(uvc))
    {
        printf("Please Init UVC Device First\n");
        return -EINVAL;
    }
    /* Uninit  Device */
	UVC_Handle_Streamoff_Event(STREAMING_SYSTEM_EVENT);
    return MI_SUCCESS;
};

/* Some Attr todo */
MI_S32 MI_UVC_SetChnAttr(MI_UVC_CHANNEL Chn, const MI_UVC_ChnAttr_t* pstAttr)
{
    return MI_SUCCESS;
};

/* Some Attr todo */
MI_S32 MI_UVC_GetChnAttr(MI_UVC_CHANNEL Chn, const MI_UVC_ChnAttr_t* pstAttr)
{
    return MI_SUCCESS;
};

MI_S32 MI_UVC_StartDev(void)
{
    if(IS_NULL(uvc))
        return -EINVAL;

    pthread_t *pThread = &uvc->Thread;

    if(UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) &&
       0 == UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_STARTED);
        pthread_create(pThread, NULL, UVC_Event_Task, NULL);
        pthread_setname_np(*pThread , "UVC_Event_Task");
        return MI_SUCCESS;
    } else 
        return -1;
};

MI_S32 MI_UVC_StopDev(void)
{
    if(IS_NULL(uvc))
        return -EINVAL;
    pthread_t pThread = uvc->Thread;
    if(UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STARTED);
    }
    pthread_join(pThread, NULL);
    return MI_SUCCESS;
};
