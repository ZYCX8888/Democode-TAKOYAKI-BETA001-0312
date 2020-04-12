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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include "mi_sys.h"
#include "mi_vdisp.h"
#include "mi_vpe.h"
#include "vdisp_test_common.h"

typedef struct
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_ChnPort_t sysport;
}vdisp_inject_handle_t;
typedef struct
{
    MI_SYS_BUF_HANDLE bufhandle;
    MI_SYS_BufInfo_t stBufInfo;
}inject_frame_priv_t;

typedef struct
{
    int fd;
    MI_SYS_ChnPort_t sysport;
}vdisp_sink_handle_t;
typedef struct
{
    MI_SYS_BUF_HANDLE bufhandle;
    MI_SYS_BufInfo_t stBufInfo;
}sink_frame_priv_t;
static MI_SYS_PixelFormat_e test_pixel_fmt=E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;//E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
int vdisp_get_inject_buf_yuv422(void *handle, framebuf_t *frame)
{
    int ret=-1;
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    inject_frame_priv_t *priv;
    inject->stBufConf.u64TargetPts=frame->u64pts;
    frame->priv=NULL;
    priv=malloc(sizeof(inject_frame_priv_t));
    if(!priv)
        goto exit;
    memset(priv, 0, sizeof(inject_frame_priv_t));
    if(MI_SUCCESS!=MI_SYS_ChnInputPortGetBuf(&inject->sysport,
                                                                     &inject->stBufConf,
                                                                     &priv->stBufInfo,
                                                                     &priv->bufhandle, 0) ){
        //DBG_INFO("get input port buf fail\n");
        goto free_priv;
    }
    BUG_ON(priv->stBufInfo.eBufType!=E_MI_SYS_BUFDATA_FRAME);
    BUG_ON(priv->stBufInfo.stFrameData.ePixelFormat!=E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);
    frame->priv=priv;
    frame->nplanenum=1;
    frame->plane[0].paddr=priv->stBufInfo.stFrameData.pVirAddr[0];
    frame->plane[0].nwidth=priv->stBufInfo.stFrameData.u16Width*2;
    frame->plane[0].nheight=priv->stBufInfo.stFrameData.u16Height;
    frame->plane[0].nstride=priv->stBufInfo.stFrameData.u32Stride[0];
    frame->plane[0].phy=priv->stBufInfo.stFrameData.phyAddr[0];
    ret=0;
    goto exit;
free_priv:
    free(priv);
exit:
    return ret;
}
int vdisp_get_inject_buf_yuv420(void *handle, framebuf_t *frame)
{
    int ret=-1;
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    inject_frame_priv_t *priv;
    inject->stBufConf.u64TargetPts=frame->u64pts;
    frame->priv=NULL;
    priv=malloc(sizeof(inject_frame_priv_t));
    if(!priv)
        goto exit;
    memset(priv, 0, sizeof(inject_frame_priv_t));
    if(MI_SUCCESS!=MI_SYS_ChnInputPortGetBuf(&inject->sysport,
                                                                     &inject->stBufConf,
                                                                     &priv->stBufInfo,
                                                                     &priv->bufhandle, 0) ){
        //DBG_INFO("get input port buf fail\n");
        goto free_priv;
    }
    BUG_ON(priv->stBufInfo.eBufType!=E_MI_SYS_BUFDATA_FRAME);
    BUG_ON(priv->stBufInfo.stFrameData.ePixelFormat!=E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420);
    frame->priv=priv;
    frame->nplanenum=2;
    frame->plane[0].paddr=priv->stBufInfo.stFrameData.pVirAddr[0];
    frame->plane[0].nwidth=priv->stBufInfo.stFrameData.u16Width;
    frame->plane[0].nheight=priv->stBufInfo.stFrameData.u16Height;
    frame->plane[0].nstride=priv->stBufInfo.stFrameData.u32Stride[0];
    frame->plane[0].phy=priv->stBufInfo.stFrameData.phyAddr[0];

    frame->plane[1].paddr=priv->stBufInfo.stFrameData.pVirAddr[1];
    frame->plane[1].nwidth=priv->stBufInfo.stFrameData.u16Width;
    frame->plane[1].nheight=priv->stBufInfo.stFrameData.u16Height/2;
    frame->plane[1].nstride=priv->stBufInfo.stFrameData.u32Stride[1];
    frame->plane[1].phy=priv->stBufInfo.stFrameData.phyAddr[1];
    ret=0;
    goto exit;
free_priv:
    free(priv);
exit:
    return ret;
}
int vdisp_get_inject_buf(void *handle, framebuf_t *frame)
{
    int ret=0;
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    BUG_ON(inject->stBufConf.eBufType!=E_MI_SYS_BUFDATA_FRAME);
    if(inject->stBufConf.stFrameCfg.eFormat==E_MI_SYS_PIXEL_FRAME_YUV422_YUYV){
        ret=vdisp_get_inject_buf_yuv422(handle, frame);
    }else if(inject->stBufConf.stFrameCfg.eFormat==E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420){
        ret=vdisp_get_inject_buf_yuv420(handle, frame);
    }
    return ret;
}

void vdisp_finish_inject_buf_yuv422(void *handle, framebuf_t *frame, int bvaliddata)
{
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    inject_frame_priv_t *priv=frame->priv;
    if(!priv)
        return;
    MI_SYS_ChnInputPortPutBuf(priv->bufhandle, &priv->stBufInfo, !bvaliddata);
    free(priv);
}
void vdisp_finish_inject_buf_yuv420(void *handle, framebuf_t *frame, int bvaliddata)
{
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    inject_frame_priv_t *priv=frame->priv;
    if(!priv)
        return;
    MI_SYS_ChnInputPortPutBuf(priv->bufhandle, &priv->stBufInfo, !bvaliddata);
    free(priv);
}
void vdisp_finish_inject_buf(void *handle, framebuf_t *frame, int bvaliddata)
{
    vdisp_inject_handle_t *inject=(vdisp_inject_handle_t *)handle;
    BUG_ON(inject->stBufConf.eBufType!=E_MI_SYS_BUFDATA_FRAME);
    if(inject->stBufConf.stFrameCfg.eFormat==E_MI_SYS_PIXEL_FRAME_YUV422_YUYV){
        vdisp_finish_inject_buf_yuv422(handle, frame, bvaliddata);
    }else if(inject->stBufConf.stFrameCfg.eFormat==E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420){
        vdisp_finish_inject_buf_yuv420(handle, frame, bvaliddata);
    }
}
int vdisp_wait_sink_buf(void *handle, int timeout)
{
    vdisp_sink_handle_t *sink=(vdisp_sink_handle_t *)handle;
    struct pollfd pfd[1] = {
            {sink->fd, POLLIN|POLLERR},
        };
    int rval = poll(pfd, 1, timeout);
    if(rval>0 && (pfd[0].revents&POLLIN))
        return 0;
    else
        return -1;
}
int vdisp_get_sink_buf(void *handle, framebuf_t *frame)
{
    int ret=-1;
    vdisp_sink_handle_t *sink=(vdisp_sink_handle_t *)handle;
    sink_frame_priv_t *priv;
    frame->priv=NULL;
    priv=malloc(sizeof(sink_frame_priv_t));
    if(!priv)
        goto exit;
    memset(priv, 0, sizeof(inject_frame_priv_t));
    if(MI_SUCCESS!=MI_SYS_ChnOutputPortGetBuf(&sink->sysport,
                                                                     &priv->stBufInfo,
                                                                     &priv->bufhandle) ){
        goto free_priv;
    }
    BUG_ON(priv->stBufInfo.eBufType!=E_MI_SYS_BUFDATA_FRAME);
    frame->priv=priv;
    if(priv->stBufInfo.stFrameData.ePixelFormat==E_MI_SYS_PIXEL_FRAME_YUV422_YUYV){
        frame->nplanenum=1;
        frame->plane[0].paddr=priv->stBufInfo.stFrameData.pVirAddr[0];
        frame->plane[0].nwidth=priv->stBufInfo.stFrameData.u16Width*2;
        frame->plane[0].nheight=priv->stBufInfo.stFrameData.u16Height;
        frame->plane[0].nstride=priv->stBufInfo.stFrameData.u32Stride[0];
        frame->plane[0].phy=priv->stBufInfo.stFrameData.phyAddr[0];
        //no need flush here.
        //MI_SYS_FlushInvCache(frame->plane[0].paddr, frame->plane[0].nstride*frame->plane[0].nheight);
    }else if(priv->stBufInfo.stFrameData.ePixelFormat==E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420){
        frame->nplanenum=2;
        frame->plane[0].paddr=priv->stBufInfo.stFrameData.pVirAddr[0];
        frame->plane[0].nwidth=priv->stBufInfo.stFrameData.u16Width;
        frame->plane[0].nheight=priv->stBufInfo.stFrameData.u16Height;
        frame->plane[0].nstride=priv->stBufInfo.stFrameData.u32Stride[0];
        frame->plane[0].phy=priv->stBufInfo.stFrameData.phyAddr[0];

        frame->plane[1].paddr=priv->stBufInfo.stFrameData.pVirAddr[1];
        frame->plane[1].nwidth=priv->stBufInfo.stFrameData.u16Width;
        frame->plane[1].nheight=priv->stBufInfo.stFrameData.u16Height/2;
        frame->plane[1].nstride=priv->stBufInfo.stFrameData.u32Stride[1];
        frame->plane[1].phy=priv->stBufInfo.stFrameData.phyAddr[1];
        //no need flushCache here.
        //MI_SYS_FlushInvCache(frame->plane[0].paddr, frame->plane[0].nstride*frame->plane[0].nheight);
        //MI_SYS_FlushInvCache(frame->plane[1].paddr, frame->plane[1].nstride*frame->plane[1].nheight);
    }else{
        BUG_ON(1);
    }
    ret=0;
    goto exit;
free_priv:
    free(priv);
exit:
    return ret;
}

void vidsp_finish_sink_buf(void *handle, framebuf_t *frame)
{
    vdisp_sink_handle_t *sink=(vdisp_sink_handle_t *)handle;
    sink_frame_priv_t *priv=frame->priv;
    if(!priv)
        return;
    MI_SYS_ChnOutputPortPutBuf(priv->bufhandle);
    free(priv);
}
static int in_w=160, in_h=90, out_w=640, out_h=360,frmrate=15;
int main_001(int argc, const char *argv[])
{
    int ret=-1;
    int injfd, sinkfd;
    MI_VDISP_InputChnAttr_t inputportattr;
    MI_VDISP_OutputPortAttr_t outputportattr;
    MI_SYS_ChnPort_t ChnPort;
    vdisp_inject_handle_t injhandle;
    vdisp_sink_handle_t sinkhandle;
    if(vdisp_test_init()<0){
        DBG_ERR("vdisp_test_init fail\n");
        goto exit;
    }
    DBG_INFO("\n");
    injfd=vdisp_test_open_file("/mnt/mstyuv-160x90@20.yuv", 0);
    sinkfd=vdisp_test_open_file("/mnt/mstoutput.yuv",1);
    if(injfd<0 || sinkfd<0){
        DBG_ERR("open file fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_SYS_Init();
    if(ret!=MI_SUCCESS){
        DBG_ERR("MI_SYS_Init fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_Init();
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_Init fail\n");
        goto exit_sys;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_OpenDevice(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_OpenDevice fail\n");
        goto exit_vdisp;
    }
    DBG_INFO("\n");
    inputportattr.s32IsFreeRun=1;
    inputportattr.u32OutX=0;
    inputportattr.u32OutY=0;
    inputportattr.u32OutWidth=in_w;
    inputportattr.u32OutHeight=in_h;
    ret=MI_VDISP_SetInputPortAttr(0, 0, &inputportattr);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_SetInputPortAttr fail\n");
        goto close_device;
    }
    DBG_INFO("\n");
    outputportattr.ePixelFormat=test_pixel_fmt;
    outputportattr.u32BgColor=0x00700070;
    outputportattr.u32FrmRate=frmrate;
    outputportattr.u32Width=out_w;
    outputportattr.u32Height=out_h;
    outputportattr.u64pts=0;
    ret=MI_VDISP_SetOutputPortAttr(0,0, &outputportattr);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_SetOutputPortAttr fail\n");
        goto close_device;
    }
    ChnPort.eModId = E_MI_MODULE_ID_VDISP;
    ChnPort.u32DevId = 0;
    ChnPort.u32ChnId = 0;
    ChnPort.u32PortId = 0;
    ret=MI_SYS_SetChnOutputPortDepth(&ChnPort,5,20);

    DBG_INFO("\n");
    ret=MI_VDISP_EnableInputPort(0,0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_EnableInputPort fail\n");
        goto close_device;
    }
    DBG_INFO("\n");
    injhandle.sysport.eModId=E_MI_MODULE_ID_VDISP;
    injhandle.sysport.u32DevId=0;
    injhandle.sysport.u32ChnId=0;
    injhandle.sysport.u32PortId=0;
    injhandle.stBufConf.eBufType=E_MI_SYS_BUFDATA_FRAME;
    injhandle.stBufConf.u32Flags=0;
    injhandle.stBufConf.u64TargetPts=0;
    injhandle.stBufConf.stFrameCfg.eFormat=test_pixel_fmt;
    injhandle.stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    injhandle.stBufConf.stFrameCfg.u16Width=in_w;
    injhandle.stBufConf.stFrameCfg.u16Height=in_h;
    ret=vdisp_test_enable_inject(0, injfd, frmrate, vdisp_get_inject_buf, vdisp_finish_inject_buf,&injhandle);
    if(ret<0){
        DBG_ERR("vdisp_test_enable_inject fail\n");
        goto disable_inputport;
    }
    DBG_INFO("\n");
    sinkhandle.sysport.eModId=E_MI_MODULE_ID_VDISP;
    sinkhandle.sysport.u32DevId=0;
    sinkhandle.sysport.u32ChnId=0;
    sinkhandle.sysport.u32PortId=0;
    ret=MI_SYS_GetFd(&sinkhandle.sysport, &sinkhandle.fd);
    if(MI_SUCCESS!=ret || sinkhandle.fd<0){
        DBG_ERR("MI_SYS_GetFd fail\n");
        goto disable_inject;
    }
    DBG_INFO("\n");
    ret=vdisp_test_enable_sink(sinkfd,vdisp_wait_sink_buf,vdisp_get_sink_buf,vidsp_finish_sink_buf,&sinkhandle);
    if(ret<0){
        DBG_ERR("vdisp_test_enable_sink fail\n");
        goto close_fd;
    }
    DBG_INFO("\n");
    ret=vdisp_test_start();
    if(ret<0){
        DBG_ERR("vdisp_test_start fail\n");
        goto disable_sink;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_StartDev(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_StartDev fail\n");
        goto stop_vdisp_test;
    }
    DBG_INFO("\n");
    while(1){
        int ch=getchar();
        if(ch == 'q'){
            break;
        }
    }
    if(injfd>=0){
        vdisp_test_close_file(injfd);
        injfd=-1;
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
        sinkfd=-1;
    }
stop_vdisp_dev:
    DBG_INFO("\n");
    MI_VDISP_StopDev(0);
stop_vdisp_test:
    DBG_INFO("\n");
    vdisp_test_stop();
disable_sink:
    DBG_INFO("\n");
    vdisp_test_disable_sink();
close_fd:
    DBG_INFO("\n");
    MI_SYS_CloseFd(sinkhandle.fd);
disable_inject:
    DBG_INFO("\n");
    vdisp_test_disable_inject(0);
disable_inputport:
    DBG_INFO("\n");
    MI_VDISP_DisableInputPort(0,0);
close_device:
    DBG_INFO("\n");
    MI_VDISP_CloseDevice(0);
exit_vdisp:
    DBG_INFO("\n");
    MI_VDISP_Exit();
exit_sys:
    DBG_INFO("\n");
    MI_SYS_Exit();
close_file:
    DBG_INFO("\n");
    if(injfd>=0){
        vdisp_test_close_file(injfd);
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
    }
exit_deinit:
    DBG_INFO("\n");
    vdisp_test_deinit();
exit:
    DBG_INFO("\n");
    return ret;
}

#define INJECT_NUM 16
static int in_x[INJECT_NUM]={0,160, 320,480, 0  ,160, 320,480, 0  ,160, 320,480, 0  ,160, 320,480};
static int in_y[INJECT_NUM]={0,   0,  0,   0,    90 , 90 , 90, 90, 180, 180, 180, 180,270,270,270,270};


int main_002(int argc, const char *argv[])
{
    int ret=-1;
    int injfd[INJECT_NUM], sinkfd;
    MI_VDISP_InputChnAttr_t inputportattr;
    MI_VDISP_OutputPortAttr_t outputportattr;
    MI_SYS_ChnPort_t ChnPort;
    vdisp_inject_handle_t injhandle[INJECT_NUM];
    vdisp_sink_handle_t sinkhandle;
    int i;
    if(vdisp_test_init()<0){
        DBG_ERR("vdisp_test_init fail\n");
        goto exit;
    }
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM;i++){
        injfd[i]=vdisp_test_open_file("/mnt/mstyuv-160x90@20.yuv", 0);
        BUG_ON(injfd[i]<0);
    }
    sinkfd=vdisp_test_open_file("/mnt/mstoutput.yuv",1);
    if(sinkfd<0){
        DBG_ERR("open file fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_SYS_Init();
    if(ret!=MI_SUCCESS){
        DBG_ERR("MI_SYS_Init fail\n");
        goto close_file;
    }

    DBG_INFO("\n");
    ret=MI_VDISP_Init();
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_Init fail\n");
        goto exit_sys;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_OpenDevice(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_OpenDevice fail\n");
        goto exit_vdisp;
    }
    for(i=0;i<INJECT_NUM;i++){
        DBG_INFO("\n");
        inputportattr.s32IsFreeRun=1;
        inputportattr.u32OutX=in_x[i];
        inputportattr.u32OutY=in_y[i];
        inputportattr.u32OutWidth=in_w;
        inputportattr.u32OutHeight=in_h;
        ret=MI_VDISP_SetInputPortAttr(0, i, &inputportattr);
        BUG_ON(MI_SUCCESS!=ret);
        /*if(ret<0){
            DBG_ERR("MI_VDISP_SetInputPortAttr fail\n");
            goto close_device;
        }*/
    }
    DBG_INFO("\n");
    outputportattr.ePixelFormat=test_pixel_fmt;
    outputportattr.u32BgColor=0x00700070;
    outputportattr.u32FrmRate=frmrate;
    outputportattr.u32Width=out_w;
    outputportattr.u32Height=out_h;
    outputportattr.u64pts=0;
    ret=MI_VDISP_SetOutputPortAttr(0,0, &outputportattr);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_SetOutputPortAttr fail\n");
        goto close_device;
    }
    ChnPort.eModId = E_MI_MODULE_ID_VDISP;
    ChnPort.u32DevId = 0;
    ChnPort.u32ChnId = 0;
    ChnPort.u32PortId = 0;
    ret=MI_SYS_SetChnOutputPortDepth(&ChnPort,5,20);

    for(i=0;i<INJECT_NUM;i++){
        DBG_INFO("\n");
        ret=MI_VDISP_EnableInputPort(0,i);
        BUG_ON(MI_SUCCESS!=ret);
        /*
        if(ret<0){
            DBG_ERR("MI_VDISP_EnableInputPort fail\n");
            goto close_device;
        }*/
        DBG_INFO("\n");
        injhandle[i].sysport.eModId=E_MI_MODULE_ID_VDISP;
        injhandle[i].sysport.u32DevId=0;
        injhandle[i].sysport.u32ChnId=0;
        injhandle[i].sysport.u32PortId=i;
        injhandle[i].stBufConf.eBufType=E_MI_SYS_BUFDATA_FRAME;
        injhandle[i].stBufConf.u32Flags=0;
        injhandle[i].stBufConf.u64TargetPts=0;
        injhandle[i].stBufConf.stFrameCfg.eFormat=test_pixel_fmt;
        injhandle[i].stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        injhandle[i].stBufConf.stFrameCfg.u16Width=in_w;
        injhandle[i].stBufConf.stFrameCfg.u16Height=in_h;
        ret=vdisp_test_enable_inject(i, injfd[i], frmrate, vdisp_get_inject_buf, vdisp_finish_inject_buf,&injhandle[i]);
        BUG_ON(ret<0);
        /*
        if(ret<0){
            DBG_ERR("vdisp_test_enable_inject fail\n");
            goto disable_inputport;
        }*/
    }
    DBG_INFO("\n");
    sinkhandle.sysport.eModId=E_MI_MODULE_ID_VDISP;
    sinkhandle.sysport.u32DevId=0;
    sinkhandle.sysport.u32ChnId=0;
    sinkhandle.sysport.u32PortId=0;
    ret=MI_SYS_GetFd(&sinkhandle.sysport, &sinkhandle.fd);
    if(MI_SUCCESS!=ret || sinkhandle.fd<0){
        DBG_ERR("MI_SYS_GetFd fail\n");
        goto disable_inject;
    }
    DBG_INFO("\n");
    ret=vdisp_test_enable_sink(sinkfd,vdisp_wait_sink_buf,vdisp_get_sink_buf,vidsp_finish_sink_buf,&sinkhandle);
    if(ret<0){
        DBG_ERR("vdisp_test_enable_sink fail\n");
        goto close_fd;
    }
    DBG_INFO("\n");
    ret=vdisp_test_start();
    if(ret<0){
        DBG_ERR("vdisp_test_start fail\n");
        goto disable_sink;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_StartDev(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_StartDev fail\n");
        goto stop_vdisp_test;
    }
    DBG_INFO("\n");
    while(1){
        int ch=getchar();
        if(ch == 'q'){
            break;
        }
    }
    for(i=0;i<INJECT_NUM;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
        injfd[i]=-1;
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
        sinkfd=-1;
    }
stop_vdisp_dev:
    DBG_INFO("\n");
    MI_VDISP_StopDev(0);
stop_vdisp_test:
    DBG_INFO("\n");
    vdisp_test_stop();
disable_sink:
    DBG_INFO("\n");
    vdisp_test_disable_sink();
close_fd:
    DBG_INFO("\n");
    MI_SYS_CloseFd(sinkhandle.fd);
disable_inject:
    for(i=0;i<INJECT_NUM;i++){
        DBG_INFO("\n");
        vdisp_test_disable_inject(i);
    }
disable_inputport:
    for(i=0;i<INJECT_NUM;i++){
        DBG_INFO("\n");
        MI_VDISP_DisableInputPort(0,i);
    }
close_device:
    DBG_INFO("\n");
    MI_VDISP_CloseDevice(0);
exit_vdisp:
    DBG_INFO("\n");
    MI_VDISP_Exit();
exit_sys:
    DBG_INFO("\n");
    MI_SYS_Exit();
close_file:
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
    }
exit_deinit:
    DBG_INFO("\n");
    vdisp_test_deinit();
exit:
    DBG_INFO("\n");
    return ret;
}



#define ExecFunc(_func_, _ret_, fail_label) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
        goto fail_label;\
    }
MI_S32 test_vpe_CreatChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpssAttr.u16MaxW = /*1920*/pstDispWin->u16Width;
    stChannelVpssAttr.u16MaxH = /*1080*/pstDispWin->u16Height;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = test_pixel_fmt;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK, exit);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK, destroy_ch);

    stChannelVpssAttr.bContrastEn = FALSE;
    stChannelVpssAttr.bNrEn = FALSE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK, destroy_ch);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK, destroy_ch);
    stCropWin.u16X = pstCropWin->u16X;
    stCropWin.u16Y = pstCropWin->u16Y;
    stCropWin.u16Width = pstCropWin->u16Width;
    stCropWin.u16Height = pstCropWin->u16Height;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK, destroy_ch);
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK, destroy_ch);
    stVpeMode.ePixelFormat = test_pixel_fmt;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height= pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK, destroy_ch);
//    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK, destroy_ch);
//    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK, disable_port);
    return 0;
//disable_port:
//    MI_VPE_DisablePort(VpeChannel, VpePort);
destroy_ch:
    MI_VPE_DestroyChannel(VpeChannel);
exit:
    return -1;
}

MI_S32 test_vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
//    MI_VPE_StopChannel (VpeChannel);
//    MI_VPE_DisablePort(VpeChannel, VpePort);
    MI_VPE_DestroyChannel(VpeChannel);
    return 0;
}
MI_S32 test_vpe_StartChannel(MI_VPE_CHANNEL VpeChannel,MI_VPE_PORT VpePort)
{
    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK, exit);
    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK, disable_port);
    return 0;
disable_port:
    MI_VPE_DisablePort(VpeChannel, VpePort);
exit:
    return -1;
}
MI_S32 test_vpe_StopChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    MI_VPE_StopChannel (VpeChannel);
    MI_VPE_DisablePort(VpeChannel, VpePort);
}

#define INJECT_NUM_003 INJECT_NUM
int main_003(int argc, const char *argv[])
{
    int ret=-1;
    int injfd[INJECT_NUM_003], sinkfd;
    MI_VDISP_InputChnAttr_t inputportattr;
    MI_VDISP_OutputPortAttr_t outputportattr;
    MI_SYS_ChnPort_t ChnPort;
    vdisp_inject_handle_t injhandle[INJECT_NUM_003];
    vdisp_sink_handle_t sinkhandle;
    MI_SYS_WindowRect_t cropWin,dispWin;
    int i;
    if(vdisp_test_init()<0){
        DBG_ERR("vdisp_test_init fail\n");
        goto exit;
    }
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_003;i++){
        injfd[i]=vdisp_test_open_file("/mnt/mstyuv-160x90@20.yuv", 0);
        BUG_ON(injfd[i]<0);
    }
    sinkfd=vdisp_test_open_file("/mnt/mstoutput.yuv",1);
    if(sinkfd<0){
        DBG_ERR("open file fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_SYS_Init();
    if(ret!=MI_SUCCESS){
        DBG_ERR("MI_SYS_Init fail\n");
        goto close_file;
    }

    cropWin.u16X=0;
    cropWin.u16Y=0;
    cropWin.u16Width=in_w;
    cropWin.u16Height=in_h;
    dispWin.u16X=0;
    dispWin.u16Y=0;
    dispWin.u16Width=in_w;
    dispWin.u16Height=in_h;
    for(i=0;i<INJECT_NUM_003;i++){
        ret=test_vpe_CreatChannel(i, 0, &cropWin, &dispWin);
        BUG_ON(ret<0);
    }

    DBG_INFO("\n");
    ret=MI_VDISP_Init();
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_Init fail\n");
        goto exit_sys;
    }

    DBG_INFO("\n");
    ret=MI_VDISP_OpenDevice(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_OpenDevice fail\n");
        goto exit_vdisp;
    }
    for(i=0;i<INJECT_NUM_003;i++){
        DBG_INFO("\n");
        inputportattr.s32IsFreeRun=1;
        inputportattr.u32OutX=in_x[i];
        inputportattr.u32OutY=in_y[i];
        inputportattr.u32OutWidth=in_w;
        inputportattr.u32OutHeight=in_h;
        ret=MI_VDISP_SetInputPortAttr(0, i, &inputportattr);
        BUG_ON(MI_SUCCESS!=ret);
        /*if(ret<0){
            DBG_ERR("MI_VDISP_SetInputPortAttr fail\n");
            goto close_device;
        }*/
    }
    DBG_INFO("\n");
    outputportattr.ePixelFormat=test_pixel_fmt;
    outputportattr.u32BgColor=0x00700070;
    outputportattr.u32FrmRate=frmrate;
    outputportattr.u32Width=out_w;
    outputportattr.u32Height=out_h;
    outputportattr.u64pts=0;
    ret=MI_VDISP_SetOutputPortAttr(0,0, &outputportattr);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_SetOutputPortAttr fail\n");
        goto close_device;
    }
    DBG_INFO("\n");
    ChnPort.eModId = E_MI_MODULE_ID_VDISP;
    ChnPort.u32DevId = 0;
    ChnPort.u32ChnId = 0;
    ChnPort.u32PortId = 0;
    ret=MI_SYS_SetChnOutputPortDepth(&ChnPort,5,20);
    //////////////////////////////////////////////////
    //////////bind vpe outputport 0 to vdisp input port//////////////////////
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_003;i++){
        MI_SYS_ChnPort_t srcport, dstport;
        srcport.eModId = E_MI_MODULE_ID_VPE;
        srcport.u32DevId = 0;
        srcport.u32ChnId = i;
        srcport.u32PortId = 0;

        dstport.eModId = E_MI_MODULE_ID_VDISP;
        dstport.u32DevId = 0;
        dstport.u32ChnId = 0;
        dstport.u32PortId = i;
        ret=MI_SYS_BindChnPort(&srcport, &dstport ,frmrate,  frmrate);
        BUG_ON(ret!=MI_SUCCESS);
    }
    //////////////////////////////////////////////////

    for(i=0;i<INJECT_NUM_003;i++){
        DBG_INFO("\n");
        ret=MI_VDISP_EnableInputPort(0,i);
        BUG_ON(MI_SUCCESS!=ret);
        /*
        if(ret<0){
            DBG_ERR("MI_VDISP_EnableInputPort fail\n");
            goto close_device;
        }*/
    }
    for(i=0;i<INJECT_NUM_003;i++){
        DBG_INFO("\n");
        injhandle[i].sysport.eModId=E_MI_MODULE_ID_VPE;
        injhandle[i].sysport.u32DevId=0;
        injhandle[i].sysport.u32ChnId=i;
        injhandle[i].sysport.u32PortId=0;
        injhandle[i].stBufConf.eBufType=E_MI_SYS_BUFDATA_FRAME;
        injhandle[i].stBufConf.u32Flags=0;
        injhandle[i].stBufConf.u64TargetPts=0;
        injhandle[i].stBufConf.stFrameCfg.eFormat=test_pixel_fmt;
        injhandle[i].stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        injhandle[i].stBufConf.stFrameCfg.u16Width=in_w;
        injhandle[i].stBufConf.stFrameCfg.u16Height=in_h;
        ret=vdisp_test_enable_inject(i, injfd[i], frmrate, vdisp_get_inject_buf, vdisp_finish_inject_buf,&injhandle[i]);
        BUG_ON(ret<0);
        /*
        if(ret<0){
            DBG_ERR("vdisp_test_enable_inject fail\n");
            goto disable_inputport;
        }*/
    }
    DBG_INFO("\n");
    sinkhandle.sysport.eModId=E_MI_MODULE_ID_VDISP;
    sinkhandle.sysport.u32DevId=0;
    sinkhandle.sysport.u32ChnId=0;
    sinkhandle.sysport.u32PortId=0;
    ret=MI_SYS_GetFd(&sinkhandle.sysport, &sinkhandle.fd);
    if(MI_SUCCESS!=ret || sinkhandle.fd<0){
        DBG_ERR("MI_SYS_GetFd fail\n");
        goto disable_inject;
    }
    DBG_INFO("\n");
    ret=vdisp_test_enable_sink(sinkfd,vdisp_wait_sink_buf,vdisp_get_sink_buf,vidsp_finish_sink_buf,&sinkhandle);
    if(ret<0){
        DBG_ERR("vdisp_test_enable_sink fail\n");
        goto close_fd;
    }
    DBG_INFO("\n");
    ret=vdisp_test_start();
    if(ret<0){
        DBG_ERR("vdisp_test_start fail\n");
        goto disable_sink;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_StartDev(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_StartDev fail\n");
        goto stop_vdisp_test;
    }
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_003;i++){
        ret=test_vpe_StartChannel(i, 0);
        BUG_ON(ret<0);
    }
    DBG_INFO("\n");
    while(1){
        int ch=getchar();
        if(ch == 'q'){
            break;
        }
    }
    for(i=0;i<INJECT_NUM_003;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
        injfd[i]=-1;
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
        sinkfd=-1;
    }
    for(i=0;i<INJECT_NUM_003;i++){
        ret=test_vpe_StopChannel(i, 0);
    }
stop_vdisp_dev:
    DBG_INFO("\n");
    MI_VDISP_StopDev(0);
stop_vdisp_test:
    DBG_INFO("\n");
    vdisp_test_stop();
disable_sink:
    DBG_INFO("\n");
    vdisp_test_disable_sink();
close_fd:
    DBG_INFO("\n");
    MI_SYS_CloseFd(sinkhandle.fd);
disable_inject:
    for(i=0;i<INJECT_NUM_003;i++){
        DBG_INFO("\n");
        vdisp_test_disable_inject(i);
    }
disable_inputport:
    for(i=0;i<INJECT_NUM_003;i++){
        DBG_INFO("\n");
        MI_VDISP_DisableInputPort(0,i);
    }
unbind_port:
    //////////////////////////////////////////////////
    //////////bind vpe outputport 0 to vdisp input port//////////////////////
    for(i=0;i<INJECT_NUM_003;i++){
        MI_SYS_ChnPort_t srcport, dstport;
        srcport.eModId = E_MI_MODULE_ID_VPE;
        srcport.u32DevId = 0;
        srcport.u32ChnId = i;
        srcport.u32PortId = 0;

        dstport.eModId = E_MI_MODULE_ID_VDISP;
        dstport.u32DevId = 0;
        dstport.u32ChnId = 0;
        dstport.u32PortId = i;
        ret=MI_SYS_UnBindChnPort(&srcport, &dstport);
        BUG_ON(ret!=MI_SUCCESS);
    }
close_device:
    DBG_INFO("\n");
    MI_VDISP_CloseDevice(0);
exit_vdisp:
    DBG_INFO("\n");
    MI_VDISP_Exit();
exit_sys:
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_003;i++){
        test_vpe_DestroyChannel(i, 0);
    }
    DBG_INFO("\n");
    MI_SYS_Exit();
close_file:
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_003;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
    }
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
    }
exit_deinit:
    DBG_INFO("\n");
    vdisp_test_deinit();
exit:
    DBG_INFO("\n");
    return ret;
}

#define INJECT_NUM_004 4
static int in_x_004[INJECT_NUM_004]={0,160, 320,480};
static int in_y_004[INJECT_NUM_004]={0,90 ,  180,270};
static int overlay_x=80,overlay_y=60;
int main_004(int argc, const char *argv[])
{
    int ret=-1;
    int injfd[INJECT_NUM_004], injfd_overlay, sinkfd;
    MI_VDISP_InputChnAttr_t inputportattr;
    MI_VDISP_OutputPortAttr_t outputportattr;
    MI_SYS_ChnPort_t ChnPort;
    vdisp_inject_handle_t injhandle[INJECT_NUM_004],injhandle_overlay;
    vdisp_sink_handle_t sinkhandle;
    int i;
    if(vdisp_test_init()<0){
        DBG_ERR("vdisp_test_init fail\n");
        goto exit;
    }
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_004;i++){
        injfd[i]=vdisp_test_open_file("/mnt/mstyuv-160x90@20.yuv", 0);
        BUG_ON(injfd[i]<0);
    }
    injfd_overlay=vdisp_test_open_file("/mnt/mstyuv-160x90@20.yuv", 0);
    BUG_ON(injfd_overlay<0);

    sinkfd=vdisp_test_open_file("/mnt/mstoutput.yuv",1);
    if(sinkfd<0){
        DBG_ERR("open file fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_SYS_Init();
    if(ret!=MI_SUCCESS){
        DBG_ERR("MI_SYS_Init fail\n");
        goto close_file;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_Init();
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_Init fail\n");
        goto exit_sys;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_OpenDevice(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_OpenDevice fail\n");
        goto exit_vdisp;
    }
    for(i=0;i<INJECT_NUM_004;i++){
        DBG_INFO("\n");
        inputportattr.s32IsFreeRun=1;
        inputportattr.u32OutX=in_x_004[i];
        inputportattr.u32OutY=in_y_004[i];
        inputportattr.u32OutWidth=in_w;
        inputportattr.u32OutHeight=in_h;
        ret=MI_VDISP_SetInputPortAttr(0, i, &inputportattr);
        BUG_ON(MI_SUCCESS!=ret);
        /*if(ret<0){
            DBG_ERR("MI_VDISP_SetInputPortAttr fail\n");
            goto close_device;
        }*/
    }
    DBG_INFO("\n");
    inputportattr.s32IsFreeRun=1;
    inputportattr.u32OutX=overlay_x;
    inputportattr.u32OutY=overlay_y;
    inputportattr.u32OutWidth=in_w;
    inputportattr.u32OutHeight=in_h;
    ret=MI_VDISP_SetInputPortAttr(0, VDISP_OVERLAYINPUTPORTID, &inputportattr);
    BUG_ON(MI_SUCCESS!=ret);

    DBG_INFO("\n");
    outputportattr.ePixelFormat=test_pixel_fmt;
    outputportattr.u32BgColor=0x00700070;
    outputportattr.u32FrmRate=frmrate;
    outputportattr.u32Width=out_w;
    outputportattr.u32Height=out_h;
    outputportattr.u64pts=0;
    ret=MI_VDISP_SetOutputPortAttr(0,0, &outputportattr);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_SetOutputPortAttr fail\n");
        goto close_device;
    }
    ChnPort.eModId = E_MI_MODULE_ID_VDISP;
    ChnPort.u32DevId = 0;
    ChnPort.u32ChnId = 0;
    ChnPort.u32PortId = 0;
    ret=MI_SYS_SetChnOutputPortDepth(&ChnPort,5,20);

    for(i=0;i<INJECT_NUM_004;i++){
        DBG_INFO("\n");
        ret=MI_VDISP_EnableInputPort(0,i);
        BUG_ON(MI_SUCCESS!=ret);
        /*
        if(ret<0){
            DBG_ERR("MI_VDISP_EnableInputPort fail\n");
            goto close_device;
        }*/
        DBG_INFO("\n");
        injhandle[i].sysport.eModId=E_MI_MODULE_ID_VDISP;
        injhandle[i].sysport.u32DevId=0;
        injhandle[i].sysport.u32ChnId=0;
        injhandle[i].sysport.u32PortId=i;
        injhandle[i].stBufConf.eBufType=E_MI_SYS_BUFDATA_FRAME;
        injhandle[i].stBufConf.u32Flags=0;
        injhandle[i].stBufConf.u64TargetPts=0;
        injhandle[i].stBufConf.stFrameCfg.eFormat=test_pixel_fmt;
        injhandle[i].stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        injhandle[i].stBufConf.stFrameCfg.u16Width=in_w;
        injhandle[i].stBufConf.stFrameCfg.u16Height=in_h;
        ret=vdisp_test_enable_inject(i, injfd[i], i<2?10:30, vdisp_get_inject_buf, vdisp_finish_inject_buf,&injhandle[i]);
        BUG_ON(ret<0);
        /*
        if(ret<0){
            DBG_ERR("vdisp_test_enable_inject fail\n");
            goto disable_inputport;
        }*/
    }

    DBG_INFO("\n");
    ret=MI_VDISP_EnableInputPort(0,VDISP_OVERLAYINPUTPORTID);
    BUG_ON(MI_SUCCESS!=ret);
    DBG_INFO("\n");
    injhandle_overlay.sysport.eModId=E_MI_MODULE_ID_VDISP;
    injhandle_overlay.sysport.u32DevId=0;
    injhandle_overlay.sysport.u32ChnId=0;
    injhandle_overlay.sysport.u32PortId=VDISP_OVERLAYINPUTPORTID;
    injhandle_overlay.stBufConf.eBufType=E_MI_SYS_BUFDATA_FRAME;
    injhandle_overlay.stBufConf.u32Flags=0;
    injhandle_overlay.stBufConf.u64TargetPts=0;
    injhandle_overlay.stBufConf.stFrameCfg.eFormat=test_pixel_fmt;
    injhandle_overlay.stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    injhandle_overlay.stBufConf.stFrameCfg.u16Width=in_w;
    injhandle_overlay.stBufConf.stFrameCfg.u16Height=in_h;
    ret=vdisp_test_enable_inject(INJECT_NUM_004, injfd_overlay, 15, vdisp_get_inject_buf, vdisp_finish_inject_buf,&injhandle_overlay);
    BUG_ON(ret<0);

    DBG_INFO("\n");
    sinkhandle.sysport.eModId=E_MI_MODULE_ID_VDISP;
    sinkhandle.sysport.u32DevId=0;
    sinkhandle.sysport.u32ChnId=0;
    sinkhandle.sysport.u32PortId=0;
    ret=MI_SYS_GetFd(&sinkhandle.sysport, &sinkhandle.fd);
    if(MI_SUCCESS!=ret || sinkhandle.fd<0){
        DBG_ERR("MI_SYS_GetFd fail\n");
        goto disable_inject;
    }
    DBG_INFO("\n");
    ret=vdisp_test_enable_sink(sinkfd,vdisp_wait_sink_buf,vdisp_get_sink_buf,vidsp_finish_sink_buf,&sinkhandle);
    if(ret<0){
        DBG_ERR("vdisp_test_enable_sink fail\n");
        goto close_fd;
    }
    DBG_INFO("\n");
    ret=vdisp_test_start();
    if(ret<0){
        DBG_ERR("vdisp_test_start fail\n");
        goto disable_sink;
    }
    DBG_INFO("\n");
    ret=MI_VDISP_StartDev(0);
    if(MI_SUCCESS!=ret){
        DBG_ERR("MI_VDISP_StartDev fail\n");
        goto stop_vdisp_test;
    }
    DBG_INFO("\n");
    while(1){
        int ch=getchar();
        if(ch == 'q'){
            break;
        }
    }
    for(i=0;i<INJECT_NUM_004;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
        injfd[i]=-1;
    }
    if(injfd_overlay>=0)
        vdisp_test_close_file(injfd_overlay);
    injfd_overlay=-1;

    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
        sinkfd=-1;
    }
stop_vdisp_dev:
    DBG_INFO("\n");
    MI_VDISP_StopDev(0);
stop_vdisp_test:
    DBG_INFO("\n");
    vdisp_test_stop();
disable_sink:
    DBG_INFO("\n");
    vdisp_test_disable_sink();
close_fd:
    DBG_INFO("\n");
    MI_SYS_CloseFd(sinkhandle.fd);
disable_inject:
    for(i=0;i<INJECT_NUM_004;i++){
        DBG_INFO("\n");
        vdisp_test_disable_inject(i);
    }
    DBG_INFO("\n");
    vdisp_test_disable_inject(INJECT_NUM_004);
disable_inputport:
    for(i=0;i<INJECT_NUM_004;i++){
        DBG_INFO("\n");
        MI_VDISP_DisableInputPort(0,i);
    }
    MI_VDISP_DisableInputPort(0, VDISP_OVERLAYINPUTPORTID);
close_device:
    DBG_INFO("\n");
    MI_VDISP_CloseDevice(0);
exit_vdisp:
    DBG_INFO("\n");
    MI_VDISP_Exit();
exit_sys:
    DBG_INFO("\n");
    MI_SYS_Exit();
close_file:
    DBG_INFO("\n");
    for(i=0;i<INJECT_NUM_004;i++){
        if(injfd[i]>=0)
            vdisp_test_close_file(injfd[i]);
    }
    if(injfd_overlay>=0)
        vdisp_test_close_file(injfd_overlay);
    injfd_overlay=-1;
    if(sinkfd>=0){
        vdisp_test_close_file(sinkfd);
    }
exit_deinit:
    DBG_INFO("\n");
    vdisp_test_deinit();
exit:
    DBG_INFO("\n");
    return ret;
}

#define MAX_TEST 4
int main(int argc, const char *argv[])
{
    int ret;
    int test_index=1;
    if(argc>=2){
        ret=sscanf(argv[1], "%d", &test_index);
        if(ret<=0)
            test_index=1;
    }
    if(test_index<1 || test_index>MAX_TEST)
        test_index=1;
    if(test_index==1){
        DBG_INFO("Start Test 001:\n");
        ret=main_001(argc, argv);
        DBG_INFO("End Test 001\n");
    }else if(test_index==2){
        DBG_INFO("Start Test 002:\n");
        ret=main_002(argc, argv);
        DBG_INFO("End Test 002\n");
    }else if(test_index==3){
        DBG_INFO("Start Test 003:\n");
        ret=main_003(argc, argv);
        DBG_INFO("End Test 003\n");
    }else if(test_index==4){
        DBG_INFO("Start Test 004:\n");
        ret=main_004(argc, argv);
        DBG_INFO("End Test 004\n");
    }else{
        DBG_INFO("Start Test 001:\n");
        ret=main_001(argc, argv);
        DBG_INFO("End Test 001\n");
    }
    return ret;
}
