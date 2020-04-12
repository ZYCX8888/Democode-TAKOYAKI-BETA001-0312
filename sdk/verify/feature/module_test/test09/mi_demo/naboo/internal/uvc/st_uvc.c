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
#include "st_uvc.h"
#include "st_isp.h"

bool gbDbgPrint = false;
bool uvc_func_trace = 1;
UVC_DBG_LEVEL_e uvc_debug_level = UVC_DBG_ERR;

static ST_UVC_Device_t *uvc;
static void _UVC_Fill_Streaming_Control(struct uvc_streaming_control *ctrl,
                                      MI_S32 iframe, MI_S32 iformat);

struct setupbuf {
	char			valid;
	char			transfer_buffer[0x40];
	unsigned long   transfer_buffer_length;
};

struct setupbuf gSetupBuf = { 0, 0, 0 };
#if AIT_XU_DEF
struct usb_ctrlrequest gsUsbCtrlReq;
FW_RELEASE_VERSION    gbFwVersion = { 0x01, 0x00, FW_VER_BCD };	// Add FW version.

MI_U8 eu1_set_isp_val[EU1_SET_MMP_LEN];
MI_U8 eu1_get_isp_result_val[EU1_GET_ISP_RESULT_LEN];	
MI_U8 eu1_set_mmp_val[EU1_SET_MMP_LEN];
MI_U8 eu1_get_mmp_result_val[EU1_GET_MMP_RESULT_LEN];
MI_U8 eu1_set_isp_ex_val[EU1_SET_ISP_EX_LEN];
MI_U8 eu1_get_isp_ex_result_val[EU1_GET_ISP_EX_RESULT_LEN];
MI_U8 eu1_get_chip_info_val[EU1_GET_CHIP_INFO_LEN];
MI_U8 eu1_set_mmp_cmd16_val[EU1_SET_MMP_CMD16_LEN];
MI_U8 eu1_get_mmp_cmd16_result_val[EU1_GET_MMP_CMD16_RESULT_LEN];
MI_U8 eu1_get_data_32_val[EU1_GET_DATA_32_LEN];
MI_U8 eu1_set_data_32_val[EU1_SET_DATA_32_LEN];

unsigned long WriteMemAddr = 0;
unsigned long WriteMemCount = 0;
unsigned long ReadMemAddr = 0;
unsigned long ReadMemCount = 0;
#if (ISP_IQ_SERVER)
//extern MMPF_OS_FLAGID   SENSOR_Flag;
#endif




//
// XU1 : AIT internal ISP / MMP control
//
VC_CMD_CFG VC_XU_SET_ISP_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_ISP_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_ISP_CFG = {
	(CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_GET_SUPPORT),
	EU1_GET_ISP_RESULT_LEN,1,
	0,0,0,0,0
};


VC_CMD_CFG VC_XU_SET_FW_DATA_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_FW_DATA_LEN,1,
	0,0,0,0,0
};


VC_CMD_CFG VC_XU_SET_MMP_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_MMP_LEN,1,
	0,0,0,0,0
};


VC_CMD_CFG VC_XU_GET_MMP_CFG = {
	( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_GET_SUPPORT),
	EU1_GET_MMP_RESULT_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_ISP_EX_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_ISP_EX_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_ISP_EX_CFG = {
	( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_GET_SUPPORT),
	EU1_GET_ISP_EX_RESULT_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_READ_MMP_MEM_CFG = {
	( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_GET_SUPPORT),
	EU1_READ_MMP_MEM_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_MMP_MEM_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_WRITE_MMP_MEM_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_ACCESS_CUSTOMER_DATA_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_ACCESS_CUSTOMER_DATA_LEN,1,
	0,0,0,0,0
};



VC_CMD_CFG VC_XU_GET_CHIP_INFO_CFG = {
	( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_GET_SUPPORT),
	EU1_GET_CHIP_INFO_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_DATA_32_CFG = {
	(CAP_GET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_GET_DATA_32_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_DATA_32_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_DATA_32_LEN,1,
	0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_MMP_CMD16_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT),
	EU1_SET_MMP_CMD16_LEN,1,
	0,0,0,0,0
};

DataExchangeParam gDEParam = { 0,0,0,0 };

VC_CMD_CFG VC_XU_GET_MMP_CMD16_RESULT_CFG = {
	(CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
	(INFO_SET_SUPPORT | INFO_GET_SUPPORT),
	EU1_GET_MMP_CMD16_RESULT_LEN,1,
	0,0,0,0,0
};

#endif

static MI_S32 _UVC_FillBuffer(struct buffer *mem,UVC_IO_MODE_e io_mode)
{
   UVC_INFO("\n");

   ST_UVC_BufInfo_t BufInfo;
   ST_UVC_OPS_t fops = uvc->ChnAttr.fops;
retry:
   memset(&BufInfo,0x00,sizeof(ST_UVC_BufInfo_t));
   BufInfo.is_tail = 1;

   if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON)){
       if(UVC_MEMORY_MMAP==io_mode){
           if(IS_NULL(fops.m.UVC_DevFillBuffer))
               return -1;
           BufInfo.b.buf = mem->start;
           /* Fill BUF,set param if successfully */
           if(0 <= fops.m.UVC_DevFillBuffer(uvc,&BufInfo)){
               mem->is_tail = BufInfo.is_tail;
               mem->buf.bytesused = BufInfo.length;
           } else {
               goto retry;
           }
       } else
       if(UVC_MEMORY_USERPTR==io_mode){
           if(IS_NULL(fops.u.UVC_DevFillBuffer) ||
              IS_NULL(fops.u.UVC_FinishBuffer) )
               return -1;
           /* Finish Buf If Necessary */
           if(0 != mem->handle){
               fops.u.UVC_FinishBuffer(mem->handle);
               mem->handle = 0;
           }
           /* Fill BUF,set param if successfully */
           if(0 <= (fops.u.UVC_DevFillBuffer(uvc,&BufInfo,&(mem->handle)))){
               mem->is_tail = BufInfo.is_tail;
               mem->buf.m.userptr = BufInfo.b.start;
               mem->buf.bytesused = BufInfo.length;
           } else {
               goto retry;
           }
       }
   }
   /* check format */
   if(mem->buf.bytesused > mem->buf.length){
       UVC_WRN("Get Pic length is bigger than Buf size ,try again(%u %u)\n",
                       mem->buf.bytesused,mem->buf.length);
       goto retry;
   }
   return mem->buf.bytesused;
};

static MI_S8 _UVC_Video_QBuf(void)
{
	MI_U32 i;
	MI_S32 s32Ret;
    
    if(IS_NULL(uvc)){
         UVC_ERR("Invalid Arguments\n");
         return -EINVAL;
    }

    MI_S32 uvc_fd = uvc->uvc_fd;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
    struct buffer *mem = uvc->mem;

	for (i = 0; i < set.nbuf; ++i) {

        /* fill uvc buffer */
        s32Ret = _UVC_FillBuffer(&mem[i],set.io_mode);
        if(s32Ret <= 0)
            UVC_WRN("Fill a NULL Buf\n");
        
        /* queue the uvc buffer */
#ifdef UVC_SUPPORT_LOWLATENCY
        if(mem[i].is_tail)
	        s32Ret = ioctl(uvc_fd, UVCIOC_QBUF_TAIL, &(mem[i].buf));
        else
	        s32Ret = ioctl(uvc_fd, UVCIOC_QBUF, &(mem[i].buf));
#else
		s32Ret = ioctl(uvc_fd, VIDIOC_QBUF, &(mem[i].buf));
#endif
		if (s32Ret < 0) {
            UVC_ERR("UVC: VIDIOC_QBUF failed : %s (%d).\n",
                   strerror(errno), errno);
			return s32Ret;
		}
	}
	return ST_UVC_SUCCESS;
}

static MI_S8 _UVC_Video_ReqBufs(MI_S32 flags)
{
	struct v4l2_requestbuffers rb;
	MI_U32 i;
	MI_S32 s32Ret;

    struct buffer *mem = NULL;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;
    MI_S32 uvc_fd = uvc->uvc_fd;
	MI_S32 buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;

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
            /* userptr mode release buf if necessary */
            if(NULL!=fops.u.UVC_FinishBuffer){
                for(i = 0;i < rb.count;i++){
                    if(uvc->mem[i].handle != 0)
                        fops.u.UVC_FinishBuffer(uvc->mem[i].handle);
                    uvc->mem[i].handle = 0;
                }
            }
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

	s32Ret = ioctl(uvc_fd, VIDIOC_REQBUFS, &rb);
	if (s32Ret < 0) {
		if (s32Ret == -EINVAL)
            UVC_ERR("does not support memory mapping");
		else
            UVC_ERR("Unable to allocate buffers: %s (%d).\n",
                    strerror(errno), errno);
		goto err;
	}

	if (!rb.count)
		return ST_UVC_SUCCESS;

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

        /* USERPTR Mode just return */
        if(UVC_MEMORY_USERPTR == set.io_mode){
            mem[i].buf.length = uvc->stream_param.maxframesize;
            mem[i].buf.memory = V4L2_MEMORY_USERPTR;
            continue;
        }

	    /* MMAP Map the buffers. */
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

	uvc->ChnAttr.setting.nbuf = rb.count;
    uvc->mem = mem;
    UVC_INFO("UVC: %u buffers allocated. \n",rb.count);

	s32Ret = ST_UVC_SUCCESS;
    goto done;

err_free:
	free(mem);
err:
    UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_REQBUFS);
done:
	return s32Ret;
}

static MI_S8 _UVC_Uninit_Device(void)
{
	MI_U32 i;
	MI_S32 s32Ret;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting; 

    if(UVC_MEMORY_MMAP==set.io_mode){
	    for (i = 0; i < uvc->ChnAttr.setting.nbuf; ++i) {
		    s32Ret= munmap(uvc->mem[i].start, uvc->mem[i].length);
		    if (s32Ret< 0) {
			    UVC_ERR("munmap failed\n");
			    return s32Ret;
	    	}
	    }
	    free(uvc->mem);
    } else if(UVC_MEMORY_USERPTR == set.io_mode)
            ;
	return ST_UVC_SUCCESS;
}

static MI_S8 _UVC_Video_Stream_on_off(MI_S32 fd,MI_S32 enable)
{
    struct v4l2_format vformat;
	MI_S32 s32Ret;
	MI_S32 buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (!enable) {
        if(false==UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON))
           return ST_UVC_SUCCESS;
        else{
		   s32Ret = ioctl(fd, VIDIOC_STREAMOFF, &buftype);
		   if (s32Ret < 0){
		   	   UVC_ERR("VIDIOC_STREAMOFF failed: %s (%d).\n",
			  		strerror(errno), errno);
			   return s32Ret;
		   }
           UVC_UNSET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
           _UVC_Uninit_Device();
           _UVC_Video_ReqBufs(0);
        }
        UVC_INFO("UVC: Stopping video stream.\n");
	    return ST_UVC_SUCCESS;
	} else {
        if(true==UVC_GET_STATUS(uvc->status,UVC_DEVICE_STREAMON))
           return ST_UVC_SUCCESS;
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
		        return s32Ret;

           /* Queue buffers to UVC domain and start streaming. */
	        s32Ret = _UVC_Video_QBuf();
	        if (s32Ret < 0)
		        return s32Ret;

           /* And now Stream On the V4l2 Device */ 
           s32Ret = ioctl(fd, VIDIOC_STREAMON, &buftype);
	       if (s32Ret < 0) {
	  	       UVC_ERR("Unable to start streaming %s (%d).\n",
		  	           strerror(errno), errno);
		       return s32Ret;
	       } 
           UVC_SET_STATUS(uvc->status,UVC_DEVICE_STREAMON);
        }
        UVC_INFO("UVC: Starting video stream.\n");
        return ST_UVC_SUCCESS;
    }
	return ST_UVC_SUCCESS;
}

static void _UVC_SYS_Exit(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return;

    MI_S32 s32Ret = -1;
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

    MI_S32 s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;
    if(IS_NULL(fops.UVC_StopCapture))
        return;
 
/* Stop Input streaming... */
    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
    {
        s32Ret = fops.UVC_StopCapture(uvc);
        if(0 > s32Ret)
            return;
        UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
    }
/* Stream Off the V4l3 Device */
    _UVC_Video_Stream_on_off(uvc->uvc_fd,0);
}

static MI_S8 _UVC_SYS_Init(void)
{
    if(IS_NULL(uvc))
        return -EINVAL;

    MI_S32 s32Ret = -1;
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

static MI_S8 _UVC_StartCapture(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;

	MI_S32 s32Ret = -1;
    ST_UVC_OPS_t fops = uvc->ChnAttr.fops;

    if(IS_NULL(fops.UVC_StartCapture))
        return -EINVAL;

/* Start Input Video capturing now. */
    if(false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STREAMON))
    {
        s32Ret = fops.UVC_StartCapture(uvc,uvc->stream_param);
	    if (s32Ret < 0)
		    goto err;
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
    } else {
        _UVC_StopCapture();
        s32Ret = fops.UVC_StartCapture(uvc,uvc->stream_param);
	    if (s32Ret < 0)
		    goto err;
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_STREAMON);
    }
/* Stream On V4l2 Device */
    _UVC_Video_Stream_on_off(uvc->uvc_fd,1);

	return ST_UVC_SUCCESS;
err:
	return s32Ret;
}

// process xu isp command.
void usb_vc_eu1_isp_cmd(MI_U8 *cmd)//cmd -> 8byte
{
	//cmd[2] = 0x01;
	//cmd[3] = 0x02;
	//cmd[4] = 0x03;
	//cmd[5] = 0x04;
	//cmd[6] = 0x05;
	//cmd[7] = 0x06;
	eu1_get_isp_result_val[0] = EU_ISP_CMD_OK;

    printf("ISP cmd:%x %x %x %x %x %x %x %x\r\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);
	switch (cmd[0])
	{
		case 11:	//get FW ver
		{
			eu1_get_isp_result_val[2] = (gbFwVersion.major >> 8) & 0xFF;
			eu1_get_isp_result_val[3] = gbFwVersion.major & 0xFF;
			eu1_get_isp_result_val[4] = (gbFwVersion.minor >> 8) & 0xFF;
			eu1_get_isp_result_val[5] = gbFwVersion.minor & 0xFF;
			eu1_get_isp_result_val[6] = (gbFwVersion.build >> 8) & 0xFF;
			eu1_get_isp_result_val[7] = gbFwVersion.build & 0xFF;
		}
		break;

		default:
			break;
	}
}

static MI_S8 _UVC_Events_Process_Control(ST_UVC_Device_t *uvc,MI_U8 req,MI_U8 cs,
                                         MI_U8 entity_id,MI_U8 len, 
                                         struct uvc_request_data *resp)
{
	uvc->control.stype = UVC_CONTROL_SELECTOR;
 //   uvc->control.control = cs;
	//printf("Process Control: bRequest: %x, CS: %x entity_id %x wLength %x \n",
	//	req, cs, entity_id, len);

	switch (entity_id) {
	case UVC_VC_DESCRIPTOR_UNDEFINED:
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
	case UVC_VC_INPUT_TERMINAL:
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
	case UVC_VC_PROCESSING_UNIT:
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
#if AIT_XU_DEF
    case UVC_VC_EXTENSION_UNIT:		//eu1:
		//printf("UVC_VC_EXTENSION_UNIT: req:%x,cs:%x\r\n",req, cs);
		usb_vc_eu1_cs(cs, req, resp);
		break;
#endif
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

static MI_S8 _UVC_Events_Process_Streaming(ST_UVC_Device_t *uvc,MI_U8 req, MI_U8 cs,
                                            struct uvc_request_data *resp)
{
	struct uvc_streaming_control *ctrl;

	if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL)
		return -EINVAL;

	ctrl = (struct uvc_streaming_control *)&resp->data;
	resp->length = sizeof *ctrl;
	uvc->control.stype = UVC_STREAMING_SELECTOR;

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

static MI_S8 _UVC_Events_Process_Class(ST_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                        struct uvc_request_data *resp)
{
	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return -EINVAL;
	memcpy(&gsUsbCtrlReq, ctrl, sizeof(struct usb_ctrlrequest));

    UVC_SET_STATUS(uvc->status,UVC_DEVICE_ENUMURATED);
    //printf("bRequest %x wValue %x wIndex %x wLength %x \n",
    //                ctrl->bRequest,ctrl->wValue >> 8,ctrl->wIndex >> 8,ctrl->wLength);
	switch (ctrl->wIndex & 0xff) {
	case UVC_INTF_CONTROL:
		_UVC_Events_Process_Control(uvc,
                        ctrl->bRequest,		// req
			    	    ctrl->wValue >> 8,	// CS
					    ctrl->wIndex >> 8,	// entity_id
						ctrl->wLength,		// Length.
						resp);				// resp.
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

static MI_S8 _UVC_Events_Process_Standard(ST_UVC_Device_t *uvc,struct usb_ctrlrequest *ctrl,
                                           struct uvc_request_data *resp)
{
	(void)ctrl;
	(void)resp;
    return ST_UVC_SUCCESS;
}

static MI_S8 _UVC_Events_Process_Setup(struct usb_ctrlrequest *ctrl,
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

#define RAW_STORE_ONE_IMG      0x33

#define I2C_MODE_1A1D 1
#define I2C_MODE_2A1D 0

int usb_vc_eu1_mmp_cmd(unsigned char* cmd)//cmd -> 8byte
{
	int ret = 0;
#if (ISP_IQ_SERVER)
	PCAM_IQSVR_HANDLE *iqsvr;
#endif

	printf("[mmp_cmd]: [%x] [%x] [%x] [%x] [%x] [%x] [%x] [%x]\r\n",
		cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

	//eu1_get_mmp_result_val[0] = EU_ISP_CMD_OK;

	switch (cmd[0]) 
	{
	case DOWNLOAD_MMP_FW_CMD:
#if (ISP_IQ_SERVER)
		iqsvr = ISP_usb_get_iqsvr_handle();
#endif
		//FW burning,burn the firmware in task instead of ISR
		switch (cmd[1]) 
		{
		case 0:
#if (ISP_IQ_SERVER)
			/* Tool is going to send command data */
			if (iqsvr->ubCmdHandling == false) 
			{
				iqsvr->usCmdLen = (unsigned short) (cmd[4] | (cmd[5] << 8));
				printf("[Mess] ISP SVR Len: %d\r\n", iqsvr->usCmdLen);
				iqsvr->usCmdBufCurPos = 0;  // reset buffer write ptr
				if (cmd[3] == 6) {
					iqsvr->usCmdType = IQ_XU_CMD_SETAPI;
				}
				else if (cmd[3] == 7) {
					iqsvr->usCmdType = IQ_XU_CMD_GETAPI;
					iqsvr->usDataBufCurPos = 0;
				}
				iqsvr->ubCmdHandling = true;
				iqsvr->ubCmdWaitAck = false;
			}
			else {
				// Pend acking this request until the previous command is handled.
				iqsvr->ubCmdWaitAck = true;
				return -1;
			}
#else
			gbUVCDSCCommand = 0x80;
			MMPF_OS_SetFlags(USB_OP_Flag, USB_FLAG_UVC, MMPF_OS_FLAG_SET);
#endif
			break;
		case 1:
#if (ISP_IQ_SERVER)
			//Tool sent command data completely
			printf("[Mess]all command data received\n");
#else
			gbUVCDSCCommand = 0x81;
			gbUVCPara[0] = cmd[3];  // 0: old FW Package, 1: new ROM Package
			MMPF_OS_SetFlags(USB_OP_Flag, USB_FLAG_UVC, MMPF_OS_FLAG_SET);
#endif
			break;
		case 0x10:
#if (ISP_IQ_SERVER)
			//Tool is going to get data back
			if (iqsvr->usCmdType == IQ_XU_CMD_GETAPI) 
			{
				// Pend acking this request until command is handled by sensor task.
				if (iqsvr->ubCmdHandling) {
					iqsvr->ubCmdHandling = false;
					iqsvr->ubCmdWaitAck = true;
					return -1;
				}
			}
#endif
			break;
		default:
			break;
		}
		break;
	default:
		eu1_get_mmp_result_val[0] = EU_ISP_CMD_NOT_SUPPORT;
		break;
	}
	return ret;
}

// To process XU data.
MI_S8 usb_vc_eu1_cs(MI_U8 cs, MI_U8 req, struct uvc_request_data *resp)
{
	switch (cs)
	{
		case EU1_SET_ISP:
		{
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
			if(gbDbgPrint)
				printf("EU1_SET_ISP\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_SET_ISP_CFG, 0, resp);
			break;
		}
		case EU1_GET_ISP_RESULT:
		{
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
			if (gbDbgPrint)
				printf("EU1_GET_ISP\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_GET_ISP_CFG, (unsigned long) eu1_get_isp_result_val, resp);
			break;
		}
		case EU1_SET_FW_DATA:
		{
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
			VC_XU_SET_ISP_CFG.dwCurVal = 0;//(unsigned long)resVal ;
			if (gbDbgPrint)
				printf("EU1_SET_FW_DATA\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_SET_FW_DATA_CFG, 0, resp);
			break;
		}
		case EU1_SET_MMP:
		{
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwMinVal = 0;
			VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;

			//printf("EU1_SET_MMP\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_SET_MMP_CFG, 0, resp);
			break;
		}
		case EU1_GET_MMP_RESULT:
		{
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
			//printf("EU1_GET_MMP_RESULT\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_GET_MMP_CFG, (unsigned long) eu1_get_mmp_result_val, resp);
			break;
		}
		case EU1_SET_ISP_EX:
		{
			VC_XU_SET_ISP_EX_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_EX_CFG.dwMinVal = 0;//(unsigned long)maxVal
			VC_XU_SET_ISP_EX_CFG.dwResVal = 0;//(unsigned long)resVal ;
			//printf("EU1_SET_ISP_EX\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_SET_ISP_EX_CFG, 0, resp);
			break;
		}
		case EU1_GET_ISP_EX_RESULT:
		{
			VC_XU_SET_ISP_EX_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_EX_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_ISP_EX_CFG.dwResVal = 0;//(unsigned long)resVal ;
			//printf("EU1_GET_ISP_EX\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_GET_ISP_EX_CFG, (unsigned long) eu1_get_isp_ex_result_val, resp);
			break;
		}

		case EU1_READ_MMP_MEM:
		{
			unsigned char buf[EU1_READ_MMP_MEM_LEN];
			if (req == UVC_GET_CUR)
			{
				unsigned char n;
				unsigned char rlen;

				if (ReadMemCount>EU1_READ_MMP_MEM_LEN)
					rlen = EU1_READ_MMP_MEM_LEN;
				else
					rlen = ReadMemCount;

				ReadMemCount -= rlen;
				for (n = 0; n<16; ++n)
				{
					if (n<rlen)
						buf[n] = ((char*) ReadMemAddr)[n];
					else
						buf[n] = 0;
				}
				ReadMemAddr += rlen;
			}
			VC_XU_READ_MMP_MEM_CFG.dwMaxVal = 0;
			VC_XU_READ_MMP_MEM_CFG.dwMinVal = 0;
			VC_XU_READ_MMP_MEM_CFG.dwResVal = 0;
			usb_vc_cmd_cfg(req, &VC_XU_READ_MMP_MEM_CFG, (unsigned long) buf, resp);
			break;
		}
		case EU1_WRITE_MMP_MEM:
		{
			VC_XU_SET_MMP_MEM_CFG.dwMaxVal = 0;
			VC_XU_SET_MMP_MEM_CFG.dwMinVal = 0;
			VC_XU_SET_MMP_MEM_CFG.dwResVal = 0;
			usb_vc_cmd_cfg(req, &VC_XU_READ_MMP_MEM_CFG, 0, resp);      
			break;
		}

		case EU1_GET_CHIP_INFO:
		{
			unsigned char i;

			VC_XU_GET_CHIP_INFO_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_GET_CHIP_INFO_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_GET_CHIP_INFO_CFG.dwResVal = 0;//(unsigned long)resVal ;

			printf("EU1_GET_CHIP_INFO\r\n");
			eu1_get_chip_info_val[0] = '8';
			eu1_get_chip_info_val[1] = '4';
			eu1_get_chip_info_val[2] = '5';
			eu1_get_chip_info_val[3] = '5';
			eu1_get_chip_info_val[4] = 0;
			eu1_get_chip_info_val[5] = 0;;
			eu1_get_chip_info_val[6] = 0;
			eu1_get_chip_info_val[7] = 0;
			eu1_get_chip_info_val[8] = 0;
			eu1_get_chip_info_val[9] = 0;
			eu1_get_chip_info_val[10] = 0;
			eu1_get_chip_info_val[11] = 0;
			eu1_get_chip_info_val[12] = 0;
			eu1_get_chip_info_val[13] = 0;
			eu1_get_chip_info_val[14] = 0;
			eu1_get_chip_info_val[15] = 0;
			usb_vc_cmd_cfg(req, &VC_XU_GET_CHIP_INFO_CFG, (unsigned long) &eu1_get_chip_info_val[0], resp);
			break;
		}

		case EU1_GET_DATA_32:
		{
			unsigned char curVal[EU1_GET_DATA_32_LEN];

			unsigned char maxVal[EU1_GET_DATA_32_LEN];
			unsigned char resVal[EU1_GET_DATA_32_LEN];
			unsigned char i;
#if (ISP_IQ_SERVER)
			PCAM_IQSVR_HANDLE *iqsvr;
#else
#if defined(SUPPORT_UVC_ISP_EZMODE_FUNC) && (SUPPORT_UVC_ISP_EZMODE_FUNC) 
			MMP_USHORT usSz;
#endif
#endif

			for (i = 0; i<EU1_GET_DATA_32_LEN; i++) {
				maxVal[i] = 0xFF;
				resVal[i] = 0x00;
			}
			resVal[0] = 0x01;
			VC_XU_GET_DATA_32_CFG.dwMaxVal = (unsigned long) maxVal;
			VC_XU_GET_DATA_32_CFG.dwResVal = (unsigned long) resVal;
#if (ISP_IQ_SERVER)
			if (req == UVC_GET_CUR)
			{
				unsigned char i;
				iqsvr = ISP_usb_get_iqsvr_handle();
				//here we send back data from IQ server
				if ((iqsvr->usDataBufCurPos + EU1_GET_DATA_32_LEN) <= IQ_XU_DATABUF_LEN)
				{
					unsigned char *ptemp = (unsigned char*) (iqsvr->pubDataBuf + iqsvr->usDataBufCurPos);
					//if(gbDbgPrint)
					printf("EU1_GET_DATA_32: memcpy address:0x%x,0x%x\r\n", iqsvr->usDataBufCurPos, ptemp[0]);
	
					memcpy(eu1_get_data_32_val, 
						iqsvr->pubDataBuf + iqsvr->usDataBufCurPos, 
						EU1_GET_DATA_32_LEN);
				}
				else {
					memcpy(eu1_get_data_32_val, 0xFF, EU1_GET_DATA_32_LEN);
					printf("[Err]: getting size is over than data buf size\n");
				}
				
#if 0			/* For debug only */
				printf("Pos: %x\n", iqsvr->usDataBufCurPos);
				{
					for (i = 0; i < EU1_GET_DATA_32_LEN; i++) {
						if ((i % 0xf) == 0)
							printf("\n");
						printf(" %02x", eu1_get_data_32_val[i]);
					}
				}
#endif
				iqsvr->usDataBufCurPos += EU1_GET_DATA_32_LEN;
			}
#else
			if (req == UVC_GET_CUR)
			{
				if (gDEParam.cur_offset < gDEParam.data_len)
				{
					unsigned char i, len;
					len = (gDEParam.data_len - gDEParam.cur_offset>EU1_GET_DATA_32_LEN) ? EU1_GET_DATA_32_LEN : (gDEParam.data_len - gDEParam.cur_offset);
					for (i = 0; i<len; ++i)
					{
						curVal[i] = gDEParam.ptr[gDEParam.cur_offset + i];
					}
					gDEParam.cur_offset += len;
				}
				VC_XU_GET_DATA_32_CFG.dwCurVal = (unsigned long) curVal;
			}
#endif //(ISP_IQ_SERVER)
			//usb_vc_cmd_cfg(req, &VC_XU_GET_DATA_32_CFG, VC_XU_GET_DATA_32_CFG.dwCurVal, resp);
			usb_vc_cmd_cfg(req, &VC_XU_GET_DATA_32_CFG, (unsigned long) eu1_get_data_32_val, resp);
			break;
		}
		case EU1_SET_DATA_32:
		{
			VC_XU_SET_DATA_32_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_DATA_32_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
			VC_XU_SET_DATA_32_CFG.dwResVal = 0;//(unsigned long)resVal ;
			VC_XU_SET_DATA_32_CFG.dwCurVal = 0;//(unsigned long)maxVal ;        

			//printf("EU1_SET_FW_DATA32\r\n");
			usb_vc_cmd_cfg(req, &VC_XU_SET_DATA_32_CFG, 0, resp);
			break;
		}

		case EU1_SET_MMP_CMD16:
		{
			VC_XU_SET_MMP_CMD16_CFG.dwMinVal = 0;
			VC_XU_SET_MMP_CMD16_CFG.dwMaxVal = 0;
			VC_XU_SET_MMP_CMD16_CFG.dwResVal = 0;
			usb_vc_cmd_cfg(req, &VC_XU_SET_MMP_CMD16_CFG, 0, resp);
			break;
		}
		case EU1_GET_MMP_CMD16_RESULT:
		{
			VC_XU_GET_MMP_CMD16_RESULT_CFG.dwMinVal = 0;
			VC_XU_GET_MMP_CMD16_RESULT_CFG.dwMaxVal = 0;
			VC_XU_GET_MMP_CMD16_RESULT_CFG.dwResVal = 0;
			usb_vc_cmd_cfg(req, &VC_XU_GET_MMP_CMD16_RESULT_CFG, (unsigned long) eu1_get_mmp_cmd16_result_val, resp);
			break;
		}

		case XU_CONTROL_UNDEFINED:
		default:
			// un-support
			goto EU1_CS_FAIL;
			break;
	}

	uvc->request_error_code.data[0] = 0x00;
	uvc->request_error_code.length = 1;
	return ST_UVC_SUCCESS;

EU1_CS_FAIL:
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

	return ST_UVC_SUCCESS;
}

MI_S8 usb_vc_eu1_cs_out(struct uvc_request_data *data)
{
	unsigned char cs, len;
	unsigned short i;
	unsigned char entity_id;
#if (ISP_IQ_SERVER)
	PCAM_IQSVR_HANDLE *iqsvr;
#endif
	cs = gsUsbCtrlReq.wValue >> 8;
	len = gsUsbCtrlReq.wLength;
	entity_id = gsUsbCtrlReq.wIndex >> 8;

	switch (cs)
	{
	case EU1_SET_ISP:
		usb_vc_eu1_isp_cmd(data->data);
		break;
	case EU1_GET_ISP_RESULT:
		break;
	case EU1_SET_FW_DATA:
		//set fw data
		//Move to USB task
#if (ISP_IQ_SERVER)
		iqsvr = ISP_usb_get_iqsvr_handle();
		if ((iqsvr->usCmdBufCurPos + len) <= IQ_XU_CMDBUF_LEN) 
		{
			if (iqsvr->usCmdBufCurPos < iqsvr->usCmdLen) 
			{
				char *cmd = data->data;
				printf("EU1_SET_FW_DATA::Len:0x%x, cmd:%x %x %x %x %x %x %x %x\r\n", len,
					cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

				//USB_Class_Get_TransferBuf(iqsvr->pubCmdBuf + iqsvr->usCmdBufCurPos, len);
				memcpy(iqsvr->pubCmdBuf + iqsvr->usCmdBufCurPos, data->data, len);
				iqsvr->usCmdBufCurPos += len;
				// Wake up IQ server to handle request from tool
				if (iqsvr->usCmdBufCurPos >= iqsvr->usCmdLen) {
					//MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_PCAMOP, MMPF_OS_FLAG_SET);
					printf("[Mes] ready to process ISP data\r\n");
				}
			}
			else {
				// show error
				printf("[Err] Tool sends an over-size command (%d)\n", iqsvr->usCmdLen);
			}
		}
		else 
		{
			printf("[Err] Tool sends an over max size command (%d, %d)\n", iqsvr->usCmdLen, IQ_XU_CMDBUF_LEN);
		}
#else
		USB_Class_Get_TransferBuf(eu1_set_data_32_val, EU1_SET_DATA_32_LEN);
#if defined(SUPPORT_UVC_ISP_EZMODE_FUNC) && (SUPPORT_UVC_ISP_EZMODE_FUNC) 
#if (EN_EZMODE_TEMP_DBG_LOG)
		printf("SET32 status =0x%x\r\n",usb_uvc_SetData32CFG()->bCurSts); 
#endif
		if (usb_uvc_SetData32CFG()->bCurSts == CS_SET_DATA_32_STS_OUT_CALI) {
			usb_uvc_DwnldDat32(eu1_set_data_32_val, EU1_SET_DATA_32_LEN);
			eu1_get_data_32_val[0] = EU_ISP_CMD_OK;
		}
		else  if (usb_uvc_SetData32CFG()->bCurSts == CS_SET_DATA_32_STS_OUT_CALI) {
			usb_uvc_DwnldDat32(eu1_set_data_32_val, EU1_SET_DATA_32_LEN);
			eu1_get_data_32_val[0] = EU_ISP_CMD_OK;
		}
		else
#endif
		{
			eu1_get_data_32_val[0] = EU_ISP_CMD_NOT_SUPPORT;
		}
#endif //(ISP_IQ_SERVER)
		break;
	case EU1_SET_MMP:
		
		usb_vc_eu1_mmp_cmd(data->data);
		break;
	case EU1_GET_DATA_32:
	{
		MI_U8 maxVal[EU1_GET_DATA_32_LEN];
		MI_U8 resVal[EU1_GET_DATA_32_LEN];
		MI_U8 i;

		printf("usb_vc_eu1_cs_out(): EU1_GET_DATA_32\r\n" );
		for (i = 0; i < EU1_GET_DATA_32_LEN; i++) {
			maxVal[i] = 0xFF;
			resVal[i] = 0x00;
		}
		VC_XU_GET_DATA_32_CFG.dwMaxVal = (MI_U32) maxVal;
		VC_XU_GET_DATA_32_CFG.dwResVal = (MI_U32) resVal;
	}
	break;
	default:
		printf("[War]usb_vc_out_data:: not support.\r\n");
		break;
	}
	return ST_UVC_SUCCESS;
}

void usb_vc_cmd_cfg(MI_U8 req, VC_CMD_CFG *cfg, unsigned long cur_val, struct uvc_request_data *resp)
{
	int print = gbDbgPrint;
	int ret_stall = 0;

	if (print) 
	{
		 printf("vc.req : %x\r\n",req); 
		 printf("vc.val : %x\r\n",cur_val); 
		 printf("cmd cap : %x,info cap : %x\r\n",cfg->bCmdCap,cfg->bInfoCap );
	}

	switch (req) 
	{
	case UVC_SET_CUR:
		if (cfg->bCmdCap & CAP_SET_CUR_CMD) {
			if (cfg->bInfoCap & INFO_AUTO_MODE_SUPPORT) {

			}
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_CUR:
		if (cfg->bCmdCap & CAP_GET_CUR_CMD) {				
			memcpy(resp->data, cur_val, cfg->bCmdLen);
			resp->length = cfg->bCmdLen;
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_INFO:
		if (cfg->bCmdCap & CAP_GET_INFO_CMD) {
			if (print) {
				printf("vc.bInfoCap : %x\r\n", cfg->bInfoCap);
			}
#if 0	// to debug.
			memcpy(resp->data, cfg->bInfoCap, sizeof(char));
#else
			resp->data[0] = (INFO_SET_SUPPORT | INFO_GET_SUPPORT);
#endif
			resp->length = sizeof(char);
		}
		else {
			goto invalid_req;
		}
		break;

	case UVC_GET_DEF:
		if (cfg->bCmdCap & CAP_GET_DEF_CMD) {
			if (print) {
				printf("vc.def : %x\r\n",cfg->dwDefVal); 
			}
#if 0
			memcpy(resp->data, cfg->dwDefVal, sizeof(unsigned short));
#else
			resp->data[0] = 0x0;
			resp->data[1] = 0x0;
#endif
			resp->length = sizeof(unsigned short);
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_MAX:
		if (cfg->bCmdCap & CAP_GET_MAX_CMD) {
			if (print) {
				printf("vc.max : %x\r\n", cfg->dwMaxVal);
			}
#if 0
			memcpy(resp->data, cfg->dwMaxVal, sizeof(unsigned short));
#else			
			resp->data[0] = 0x1;
			resp->data[1] = 0x0;
#endif
			resp->length = sizeof(unsigned short);
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_MIN:
		if (cfg->bCmdCap & CAP_GET_MIN_CMD) {
			if (print) {
				printf("vc.mix : %x\r\n", cfg->dwMinVal);
			}
#if 0
			memcpy(resp->data, cfg->dwMinVal, sizeof(char));
#else
			resp->data[0] = 0x0;
#endif
			resp->length = sizeof(char);
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_RES:
		if (cfg->bCmdCap & CAP_GET_RES_CMD) {
			if (print) {
				printf("vc.res : %x\r\n", cfg->dwResVal);
			}
#if 0
			memcpy(resp->data, cfg->dwResVal, sizeof(unsigned short));
#else			
			resp->data[0] = 0x1;
			resp->data[1] = 0x0;
#endif
			resp->length = sizeof(unsigned short);
		}
		else {
			goto invalid_req;
		}
		break;
	case UVC_GET_LEN:
		if (cfg->bCmdCap & CAP_GET_LEN_CMD) {
			unsigned short cmdLen = cfg->bCmdLen;
			memcpy(resp->data, (unsigned char *) &cmdLen, sizeof(unsigned short));
			resp->length = sizeof(unsigned short);
		}
		else {
			goto invalid_req;
		}
		break;
	default:
	invalid_req:
		if (print) {
			printf("Cfg.Err\r\n");
		}
		//UsbWriteEp0CSR(SET_EP0_SENDSTALL);
		//gbVCERRCode = CONTROL_INVALID_REQUEST;
		break;
	}

	if(print)
	    printf("Cfg.End\r\n");
}
//*/
// process PU, CT, XU job.
MI_S8 usb_vc_out_data(struct uvc_request_data * data)
{
	unsigned char id;

	id = (/*UsbRequestPayload_wIndex*/gsUsbCtrlReq.wIndex >> 8);
	switch (id)
	{
	case UVC_VC_EXTENSION_UNIT:
		usb_vc_eu1_cs_out(data);
		break;
	default:
		break;
	}
	
	return ST_UVC_SUCCESS;
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
	MI_S32 s32Ret;

    if(UVC_STREAMING_SELECTOR == uvc->control.stype){
	    switch (uvc->control.control) {
	    case UVC_VS_PROBE_CONTROL:
            UVC_INFO(" probe control, length = %d \n",data->length);
	        target = &uvc->probe;
            break;

        case UVC_VS_COMMIT_CONTROL:
            UVC_INFO(" commit control, length = %d \n",data->length);
            target = &uvc->commit;
            break;

        default:
            UVC_INFO(" unknown control, length = %d \n",data->length);
            return 0;
        }
	}
	else if(UVC_CONTROL_SELECTOR == uvc->control.stype)
	{	// XU data.
		if(gbDbgPrint)
			printf("_UVC_Events_Process_Data:, length = %d,data=%x\n", data->length, (MI_S8 *) data->data);
		usb_vc_out_data(data);
		goto done;
    } else {
            return 0;//todo
    }

	ctrl = (struct uvc_streaming_control *)&data->data;
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
		target->dwMaxVideoFrameSize = frame->width*frame->height * 2.0;
		break;
	case V4L2_PIX_FMT_NV12:
		target->dwMaxVideoFrameSize = frame->width*frame->height * 1.5;
		break;
	case V4L2_PIX_FMT_MJPEG:
		target->dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/6;
        break;
	case V4L2_PIX_FMT_H264:
		target->dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/7;
        break;
	case V4L2_PIX_FMT_H265:
		target->dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/8;
		break;
	}
	target->dwFrameInterval = *interval;
	
    if (uvc->ChnAttr.setting.mode == USB_BULK_MODE)
	   target->dwMaxPayloadTransferSize = target->dwMaxVideoFrameSize;
	else
	/*
     * ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) *
     *                      (dev->mult + 1) * (dev->burst + 1);
     */
       target->dwMaxPayloadTransferSize = 1024 * (2 + 1) * (0 + 1);

UVC_INFO("MaxVideoFrameSize %u wMaxPayloadTransferSize %u \n",
         target->dwMaxVideoFrameSize,target->dwMaxPayloadTransferSize);

	if (uvc->control.control == UVC_VS_COMMIT_CONTROL) {
        uvc->stream_param.fcc       = format->fcc ;
        uvc->stream_param.iformat   = iformat_tmp;
        uvc->stream_param.iframe    = iframe_tmp;
		uvc->stream_param.width     = frame->width;
		uvc->stream_param.height    = frame->height;
        uvc->stream_param.frameRate = FrameInterval2FrameRate(target->dwFrameInterval);
        uvc->stream_param.maxframesize = uvc->commit.dwMaxVideoFrameSize;
        UVC_INFO(" (UVC_VS_COMMIT_CONTROL)(iformat %d iframe %d width %d height %d FrameRate %f)\n",
                        uvc->stream_param.iformat,uvc->stream_param.iframe,uvc->stream_param.width,
                        uvc->stream_param.height,uvc->stream_param.frameRate);
    } else
        goto done;

    if(uvc->ChnAttr.setting.mode == USB_BULK_MODE)
    {
        s32Ret = _UVC_StartCapture();
	    if (s32Ret < 0)
        {
			goto err;
        }
    }

done:
	return ST_UVC_SUCCESS;
err:
	return s32Ret;
}

static MI_S8 _UVC_Events_Process()
{
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (struct uvc_event *)&v4l2_event.u.data;
	struct uvc_request_data resp;
    MI_S32 uvc_fd = uvc->uvc_fd;
	MI_S32 s32Ret;
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

static MI_S8 _UVC_Video_Process(void)
{
	struct v4l2_buffer ubuf;
	MI_S32 s32Ret;
    MI_S32 uvc_fd     = uvc->uvc_fd;
    ST_UVC_Setting_t set = uvc->ChnAttr.setting;
	MI_S32 buftype = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    struct buffer *mem = NULL;

   /*
    * Return immediately if UVC video output device has not started
    * streaming yet.
    */
   if(false==UVC_INPUT_ISENABLE(uvc->status) || 
      false==UVC_OUTPUT_ISENABLE(uvc->status) )
   {
       usleep(1000);
       return -1;
   }
	/* Prepare a v4l2 buffer to be dequeued from UVC domain. */
	CLEAR(ubuf);
	ubuf.type   = buftype;
    if(UVC_MEMORY_MMAP == set.io_mode)
	    ubuf.memory = V4L2_MEMORY_MMAP;
    else if(UVC_MEMORY_USERPTR == set.io_mode)
	    ubuf.memory = V4L2_MEMORY_USERPTR;

	s32Ret = ioctl(uvc_fd, VIDIOC_DQBUF, &ubuf);
	if (s32Ret < 0) {
        UVC_WRN("VIDIOC_DQBUF Fail  s32Ret: %d \n",s32Ret);
		return s32Ret;
    }
    mem = &(uvc->mem[ubuf.index]);
    mem->buf = ubuf;

    s32Ret = _UVC_FillBuffer(mem,set.io_mode);
    if( s32Ret <= 0 )
        UVC_WRN("Fill a NULL Buf\n");

#ifdef UVC_SUPPORT_LOWLATENCY
    if(mem->is_tail)
	    s32Ret = ioctl(uvc_fd, UVCIOC_QBUF_TAIL, &(mem->buf));
    else
	    s32Ret = ioctl(uvc_fd, UVCIOC_QBUF, &(mem->buf));
#else
	s32Ret = ioctl(uvc_fd, VIDIOC_QBUF, &(mem->buf));
#endif
	if (s32Ret < 0) {
		UVC_ERR("Unable to queue buffer: %s (%d).\n",
				strerror(errno), errno);
		return s32Ret;
	}
	return s32Ret;
}

static void _UVC_Fill_Streaming_Control(struct uvc_streaming_control *ctrl,
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
		ctrl->dwMaxVideoFrameSize = frame->width*frame->height * 2.0;
		break;
    case V4L2_PIX_FMT_NV12:
		ctrl->dwMaxVideoFrameSize = frame->width*frame->height * 1.5;
		break;
	case V4L2_PIX_FMT_MJPEG:
		ctrl->dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/6;
        break;
	case V4L2_PIX_FMT_H264:
		ctrl->dwMaxVideoFrameSize = (frame->width*frame->height*2.0)/7;
        break;
	case V4L2_PIX_FMT_H265:
		ctrl->dwMaxVideoFrameSize = (frame->width*frame->height *2.0)/8;
		break;
	}

    if (uvc->ChnAttr.setting.mode == USB_BULK_MODE)
	    ctrl->dwMaxPayloadTransferSize = ctrl->dwMaxVideoFrameSize;
	else
	/*
     * ctrl->dwMaxPayloadTransferSize = (dev->maxpkt) *
     *                      (dev->mult + 1) * (dev->burst + 1);
     */
        ctrl->dwMaxPayloadTransferSize = 1024 * (2 + 1) * (0 + 1);

	ctrl->bmFramingInfo = 3;
	ctrl->bPreferedVersion = 1;
	ctrl->bMaxVersion = 1;
}

static void * UVC_Event_Task(void *arg)
{
     MI_S32 s32Ret = -1;
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
         s32Ret = select(uvc_fd + 1, NULL,&dfds, &efds, &timeout);

         if (-1 == s32Ret) {
             UVC_ERR("select error %d, %s\n",errno, strerror (errno));
              if (EINTR == errno)
                  return NULL;
         }

         if (0 == s32Ret){
             UVC_WRN("select timeout, device status %x \n",uvc->status);
             continue;
         }

          if (FD_ISSET(uvc_fd, &efds))
              _UVC_Events_Process();

          if (FD_ISSET(uvc_fd, &dfds))
              _UVC_Video_Process();
    }
    return NULL;
}
 
MI_S32 ST_UVC_Init(char *uvc_name)
{
    UVC_INFO("\n");

    MI_S32 s32Ret = -1;
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

    _UVC_Fill_Streaming_Control(&uvc->probe,uvc->stream_param.iformat,uvc->stream_param.iframe);
    _UVC_Fill_Streaming_Control(&uvc->commit,uvc->stream_param.iformat,uvc->stream_param.iframe);
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
    close(uvc->uvc_fd);
    uvc->uvc_fd = open(uvc_name, O_RDWR | O_NONBLOCK);

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

MI_S32 ST_UVC_Uninit(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return ST_UVC_SUCCESS;

    if(true==UVC_GET_STATUS(uvc->status,UVC_DEVICE_INITIAL))
    { 
        close(uvc->uvc_fd);
        free(uvc);
    }

    return ST_UVC_SUCCESS;
}

MI_S32 ST_UVC_CreateDev(const ST_UVC_ChnAttr_t* pstAttr)
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
       (IS_NULL(fops.u.UVC_DevFillBuffer)||
        IS_NULL(fops.u.UVC_FinishBuffer)))
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
    return ST_UVC_SUCCESS;
}

MI_S32 ST_UVC_DestroyDev()
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
    return ST_UVC_SUCCESS;
};

/* Some Attr todo */
MI_S32 ST_UVC_SetChnAttr(const ST_UVC_ChnAttr_t* pstAttr)
{
    UVC_INFO("\n");

    return ST_UVC_SUCCESS;
};

/* Some Attr todo */
MI_S32 ST_UVC_GetChnAttr(const ST_UVC_ChnAttr_t* pstAttr)
{
    UVC_INFO("\n");

    return ST_UVC_SUCCESS;
};

MI_S32 ST_UVC_StartDev(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;

    pthread_t *pThread = &uvc->Thread;

    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_INITIAL) &&
       false==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_SET_STATUS(uvc->status,UVC_INTDEV_STARTED);
        pthread_create(pThread, NULL, UVC_Event_Task, NULL);
        pthread_setname_np(*pThread , "UVC_Event_Task");
#if ISP_IQ_SERVER
		ISP_usb_iqsvr_open();
#endif
        return ST_UVC_SUCCESS;
    } else 
        return -1;
};

MI_S32 ST_UVC_StopDev(void)
{
    UVC_INFO("\n");

    if(IS_NULL(uvc))
        return -EINVAL;
    pthread_t pThread = uvc->Thread;
    if(true==UVC_GET_STATUS(uvc->status,UVC_INTDEV_STARTED))
    {
        UVC_UNSET_STATUS(uvc->status,UVC_INTDEV_STARTED);
#if ISP_IQ_SERVER
		ISP_usb_iqsvr_close();
#endif
    }
    pthread_join(pThread, NULL);
    return ST_UVC_SUCCESS;
};

void uvc_save_file(void *buf,MI_U32 length,char type)
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

char* uvc_get_format(MI_U32 fcc)
{
    switch(fcc)
    {
    case V4L2_PIX_FMT_YUYV:
        return "YUYV";
    case V4L2_PIX_FMT_NV12:
        return "NV12";
    case V4L2_PIX_FMT_MJPEG:
        return "MJPEG";
    case V4L2_PIX_FMT_H264:
        return "H264";
    case V4L2_PIX_FMT_H265:
        return "H265";
    default:
        return "unkonown";
    }
}


void USB_Set_TransferBuf(unsigned char* buf, unsigned long len) {
	//if(gSetupBuf.valid == 0) {
	memcpy((void *) (gSetupBuf.transfer_buffer), buf, len);
	gSetupBuf.transfer_buffer_length = len;
	//gSetupBuf.valid = 1;
	//}
}
/*
void USB_Driver_Get_TransferBuf(USB_VAR *gUSBStruct, unsigned char ep) {
	//if(gSetupBuf.valid) {
	gUSBStruct->otgUSB_EP[ep].transfer_buffer = (unsigned long) gSetupBuf.transfer_buffer;
	gUSBStruct->otgUSB_EP[ep].transfer_buffer_length = gSetupBuf.transfer_buffer_length;
	//gSetupBuf.valid = 0;
	//}
}//*/

