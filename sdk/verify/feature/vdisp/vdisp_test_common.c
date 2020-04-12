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
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "vdisp_test_common.h"

#define VDISP_MAX_DEVICE_NUM 1
#define VDISP_MAX_CHN_NUM 1
#define VDISP_MAX_INPUTPORT_NUM 16
#define VDISP_MAX_OUTPUTPORT_NUM 1

typedef struct injectinfo_s
{
    int nenable;
    int file;
    int nframerate;
    uint64_t cur_pts;
    uint64_t next_stc;
    uint64_t pts_step;
    int (*get_inject_buf)(void *handle, framebuf_t *frame/*in/out*/);
    void (*finish_inject_buf)(void *handle, framebuf_t *frame/*in*/, int bvaliddata);
    void *handle;
    int frm_cnt;
}injectinfo_t;

typedef struct sinkinfo_s
{
    int nenable;
    int file;
    int (*wait_sink_buf)(void *handle, int timeout); //0: ok; -1: fail and another wait; timeout: ms
    int (*get_sink_buf)(void *handle, framebuf_t *frame/*out*/);
    void (*finish_sink_buf)(void *handle, framebuf_t *frame/*in*/);
    void *handle;
    int frm_cnt;
}sinkinfo_t;

#define INJECT_THREAD_FLG_STOP 0x01
#define INJECT_THREAD_FLG_PAUSE 0x02

#define SINK_THREAD_FLG_STOP 0x01
#define SINK_THREAD_FLG_PAUSE 0x02
typedef struct vdisp_test_module_s
{
    int inited;
    injectinfo_t inject[VDISP_MAX_INPUTPORT_NUM];
    sinkinfo_t sink;
    uint64_t next_stc;
    unsigned long inject_flag;
    pthread_t inject_thread;
    pthread_mutex_t inject_mtx;
    pthread_cond_t inject_cond;
    unsigned long sink_flag;
    pthread_t sink_thread;
    pthread_mutex_t sink_mtx;
    pthread_cond_t sink_cond;
    int inject_handled[VDISP_MAX_INPUTPORT_NUM];
}vdisp_test_module_t;

static pthread_mutex_t vdisp_mutex = PTHREAD_MUTEX_INITIALIZER;
static vdisp_test_module_t vdisp_test_module=
{
    .inject_mtx=PTHREAD_MUTEX_INITIALIZER,
    .inject_cond=PTHREAD_COND_INITIALIZER,
    .sink_mtx=PTHREAD_MUTEX_INITIALIZER,
    .sink_cond=PTHREAD_COND_INITIALIZER
};


static inline int _vdisp_cmp_stc(
    uint64_t stc,
    uint64_t dststc,
    uint64_t threshold)
{
    if(stc>=dststc){
        if((stc-dststc)<=threshold)
            return 0;
        else
            return 1;
    }else{
        if((dststc-stc)<=threshold)
            return 0;
        else
            return -1;
    }
}
static inline uint64_t vdisp_get_cur_time(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (tp.tv_sec*1000000+tp.tv_nsec/1000);
}
static inline void _vdisp_test_random_init(void)
{
    srandom((unsigned long)vdisp_get_cur_time());
}
static inline long _vdisp_test_get_random_number(void)
{
    long randNum;
    randNum=random();
    return randNum;
}
static inline void _vdisp_test_reset_handle_flag(vdisp_test_module_t *mod)
{
    int i;
    for(i=0;i<VDISP_MAX_INPUTPORT_NUM;i++){
        mod->inject_handled[i]=0;
    }
}
static int _vdisp_test_get_next_inject(vdisp_test_module_t *mod)
{
    int i,start;
    start=i=_vdisp_test_get_random_number()%(VDISP_MAX_INPUTPORT_NUM);
    do{
        if(mod->inject_handled[i]==0){
            mod->inject_handled[i]=1;
            return i;
        }
        i++;
        if(i>=VDISP_MAX_INPUTPORT_NUM)
            i=0;
    }while(i!=start);
    return -1;
}

static inline int vdisp_get_cur_pos(int f)
{
    off_t off_cur ;
    off_cur = lseek(f, 0, SEEK_CUR);
    return off_cur;
}
static int vdisp_check_and_rewind_file(int f)
{
    off_t off_cur ;
    off_t off_end ;
    off_cur = lseek(f, 0, SEEK_CUR);
    off_end = lseek(f, 0, SEEK_END);
    if(off_cur<0 || off_end<0)
        return 0;
    if(off_cur>=off_end){
        lseek(f, 0, SEEK_SET);
        return 1;
    }else{
        lseek(f, off_cur, SEEK_SET);
        return 0;
    }
}
static inline int _read_data(
    int f,
    unsigned char *buf,
    int len)
{
    unsigned char *p=buf;
    int left=len;
    while(left>0){
        int ret=read(f, p, left);
        if(ret<0){ //err
            if(errno==EINTR)
                continue;
            goto exit;
        }
        if(ret==0) //eof
            goto exit;
        p = p+ret;
        left -= ret;
    }
exit:
    return (len-left);
}
static inline int _write_data(
    int f,
    unsigned char *buf,
    int len)
{
    unsigned char *p=buf;
    int left=len;
    while(left>0){
        int ret=write(f, p, left);
        if(ret<0){ //err
            if(errno==EINTR)
                continue;
            goto exit;
        }
        if(ret==0)
            goto exit;
        p = p+ret;
        left -= ret;
    }
exit:
    return (len-left);
}

static int vdisp_read_to_buf(
    int f, unsigned char *buf,
    int nwidth, int nheight,
    int nstride)
{
    unsigned char *p=buf;
    int i;
    for(i=0;i<nheight; i++){
        if(_read_data(f, p, nwidth) != nwidth)
            return -1;
        p += nstride;
    }
    return 0;
}

static int vdisp_write_from_buf(
    int f, unsigned char *buf,
    int nwidth, int nheight,
    int nstride)
{
    unsigned char *p=buf;
    int i;
    for(i=0;i<nheight; i++){
        if(_write_data(f, p, nwidth) != nwidth)
            return -1;
        p += nstride;
    }
    return 0;
}
static int vdisp_read_to_frame(
    int f,
    framebuf_t *frame)
{
    int i;
    for(i=0;i<frame->nplanenum;i++){
        if(vdisp_read_to_buf(f, frame->plane[i].paddr,
            frame->plane[i].nwidth, frame->plane[i].nheight,
            frame->plane[i].nstride)<0)
            return -1;
    }
    return 0;
}
static int vdisp_write_from_frame(
    int f,
    framebuf_t *frame)
{
    int i;
    for(i=0;i<frame->nplanenum;i++){
        if(vdisp_write_from_buf(f, frame->plane[i].paddr,
            frame->plane[i].nwidth, frame->plane[i].nheight,
            frame->plane[i].nstride)<0)
            return -1;
    }
    return 0;
}

static void _inject_process(vdisp_test_module_t *mod)
{
    int i;
    int ret;
    injectinfo_t *injectinfo;
    framebuf_t frame;
    if(mod->next_stc!=0 && mod->next_stc>vdisp_get_cur_time())
        return;
    mod->next_stc=0;
    _vdisp_test_reset_handle_flag(mod);
    while((i=_vdisp_test_get_next_inject(mod))>=0){
        int cmpres;
        int bvaliddata=0;
        int filepos=0;
        injectinfo = &mod->inject[i];
        pthread_mutex_lock(&mod->inject_mtx);
        if( !injectinfo->nenable )
            goto continue_inject;

        cmpres=_vdisp_cmp_stc(injectinfo->next_stc , vdisp_get_cur_time(), injectinfo->pts_step);
        if(cmpres>0)
            goto calc_next_stc;
        frame.u64pts=injectinfo->cur_pts;
        ret=injectinfo->get_inject_buf(injectinfo->handle, &frame);
        if(ret<0)
            goto after_fill_buf;
        filepos = vdisp_get_cur_pos(injectinfo->file);
        ret=vdisp_read_to_frame(injectinfo->file, &frame);
        if(ret<0)
            goto finish_inj_buf;
        bvaliddata=1;
finish_inj_buf:
        injectinfo->finish_inject_buf(injectinfo->handle, &frame, bvaliddata);
        injectinfo->frm_cnt++;
after_fill_buf:
        injectinfo->cur_pts+=injectinfo->pts_step;
        injectinfo->next_stc += injectinfo->pts_step;
calc_next_stc:
        if(mod->next_stc==0 || injectinfo->next_stc<mod->next_stc)
            mod->next_stc=injectinfo->next_stc;
continue_inject:
        pthread_mutex_unlock(&mod->inject_mtx);
    }
    usleep(1000);
}
static void *inject_thread(void *data)
{
    vdisp_test_module_t *mod=(vdisp_test_module_t *)data;
    prctl(PR_SET_NAME,"inject_thread");
    while(1){
loop_again:
        if(mod->inject_flag&INJECT_THREAD_FLG_STOP){
            DBG_INFO("inject thread exit\n");
            break;
        }
        if(mod->inject_flag&INJECT_THREAD_FLG_PAUSE){
            DBG_INFO("inject thread pause\n");
            pthread_mutex_lock(&mod->inject_mtx);
            while(mod->inject_flag&INJECT_THREAD_FLG_PAUSE){
                pthread_cond_wait(&mod->inject_cond, &mod->inject_mtx);
            }
            pthread_mutex_unlock(&mod->inject_mtx);
            DBG_INFO("inject thread resume\n");
            goto loop_again;
        }
        _inject_process(mod);
    }
    return NULL;
}
static void _sink_process(vdisp_test_module_t *mod)
{
    sinkinfo_t *sink=&mod->sink;
    int ret;
    framebuf_t frame;
    pthread_mutex_lock(&mod->sink_mtx);
    if(!sink->nenable)
        goto exit;
    ret=sink->wait_sink_buf(sink->handle, 1);
    if(ret<0)
        goto exit;
    ret=sink->get_sink_buf(sink->handle, &frame);
    if(ret<0)
        goto exit;
    ret=vdisp_write_from_frame(sink->file, &frame);
    sink->finish_sink_buf(sink->handle, &frame);
    sink->frm_cnt++;
exit:
    pthread_mutex_unlock(&mod->sink_mtx);
    usleep(1000);
}
static void *sink_thread(void *data)
{
    vdisp_test_module_t *mod=(vdisp_test_module_t *)data;
    prctl(PR_SET_NAME,"sink_thread");
    while(1){
loop_again:
        if(mod->sink_flag&SINK_THREAD_FLG_STOP){
            DBG_INFO("sink thread exit\n");
            break;
        }
        if(mod->sink_flag&SINK_THREAD_FLG_PAUSE){
            DBG_INFO("sink thread pause\n");
            pthread_mutex_lock(&mod->sink_mtx);
            while(mod->sink_flag&SINK_THREAD_FLG_PAUSE){
                pthread_cond_wait(&mod->sink_cond, &mod->sink_mtx);
            }
            pthread_mutex_unlock(&mod->sink_mtx);
            DBG_INFO("sink thread resume\n");
            goto loop_again;
        }
        _sink_process(mod);
    }
    return NULL;
}
static void _vdisp_pause_inject_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.inject_mtx);
    vdisp_test_module.inject_flag |=INJECT_THREAD_FLG_PAUSE;
    pthread_mutex_unlock(&vdisp_test_module.inject_mtx);
}
static void _vdisp_pause_sink_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.sink_mtx);
    vdisp_test_module.sink_flag |=SINK_THREAD_FLG_PAUSE;
    pthread_mutex_unlock(&vdisp_test_module.sink_mtx);
}
static void _vdisp_resume_inject_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.inject_mtx);
    vdisp_test_module.inject_flag &=~INJECT_THREAD_FLG_PAUSE;
    pthread_cond_signal(&vdisp_test_module.inject_cond);
    pthread_mutex_unlock(&vdisp_test_module.inject_mtx);
}
static void _vdisp_resume_sink_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.sink_mtx);
    vdisp_test_module.sink_flag &=~SINK_THREAD_FLG_PAUSE;
    pthread_cond_signal(&vdisp_test_module.sink_cond);
    pthread_mutex_unlock(&vdisp_test_module.sink_mtx);
}
static void _vdisp_stop_inject_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.inject_mtx);
    vdisp_test_module.inject_flag |= INJECT_THREAD_FLG_STOP;
    vdisp_test_module.inject_flag &=~INJECT_THREAD_FLG_PAUSE;
    pthread_cond_signal(&vdisp_test_module.inject_cond);
    pthread_mutex_unlock(&vdisp_test_module.inject_mtx);
    pthread_join(vdisp_test_module.inject_thread, NULL);
}
static void _vdisp_stop_sink_thread(void)
{
    pthread_mutex_lock(&vdisp_test_module.sink_mtx);
    vdisp_test_module.sink_flag |=SINK_THREAD_FLG_STOP;
    vdisp_test_module.sink_flag &=~SINK_THREAD_FLG_PAUSE;
    pthread_cond_signal(&vdisp_test_module.sink_cond);
    pthread_mutex_unlock(&vdisp_test_module.sink_mtx);
    pthread_join(vdisp_test_module.sink_thread, NULL);
}
int vdisp_test_open_file(const char *pathname, int bwrite)
{
    return open(pathname, bwrite?(O_WRONLY|O_CREAT|O_TRUNC):O_RDONLY);
}
void vdisp_test_close_file(int f)
{
    close(f);
}
int vdisp_test_init(void)
{
    int ret=-1;
    pthread_mutex_lock(&vdisp_mutex);
    if(vdisp_test_module.inited)
        goto exit;
    memset(&vdisp_test_module.inject,0, sizeof(vdisp_test_module.inject));
    memset(&vdisp_test_module.sink,0, sizeof(vdisp_test_module.sink));
    vdisp_test_module.inject_flag=INJECT_THREAD_FLG_PAUSE;
    vdisp_test_module.sink_flag=SINK_THREAD_FLG_PAUSE;
    vdisp_test_module.next_stc=0;
    if(pthread_create(&vdisp_test_module.inject_thread, NULL, inject_thread, &vdisp_test_module)<0){
        vdisp_test_module.inject_thread=0;
        goto exit;
    }
    if(pthread_create(&vdisp_test_module.sink_thread, NULL, sink_thread, &vdisp_test_module)<0){
        vdisp_test_module.sink_thread=0;
        goto stop_inject_thread;
    }
    _vdisp_test_random_init();
    vdisp_test_module.inited=1;
    ret=0;
    goto exit;
stop_inject_thread:
    if(vdisp_test_module.inject_thread){
        _vdisp_stop_inject_thread();
    }
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return ret;
}
int vdisp_test_deinit(void)
{
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    if(vdisp_test_module.inject_thread){
        _vdisp_stop_inject_thread();
    }
    if(vdisp_test_module.sink_thread){
        _vdisp_stop_sink_thread();
    }
    vdisp_test_module.inited=0;
exit:
    pthread_mutex_unlock(&vdisp_mutex);
}
int vdisp_test_enable_inject(
    int inj_id,
    int file,
    int framerate,
    pfn_get_inject_buf pfngetbuf,
    pfn_finish_inject_buf pfnfinishbuf,
    void *handle
    )
{
    int ret=-1;
    injectinfo_t *inject;
    if(inj_id>=VDISP_MAX_INPUTPORT_NUM)
        return -1;
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    pthread_mutex_lock(&vdisp_test_module.inject_mtx);
    inject=&vdisp_test_module.inject[inj_id];
    if(inject->nenable)
        goto exit_inj;
    inject->file=file;
    inject->nframerate=framerate;
    inject->cur_pts=0;
    inject->next_stc=vdisp_get_cur_time();
    inject->pts_step=framerate?1000000/framerate:1000000/30;
    inject->get_inject_buf=pfngetbuf;
    inject->finish_inject_buf=pfnfinishbuf;
    inject->handle=handle;
    inject->frm_cnt=0;
    inject->nenable=1;
    ret=0;
exit_inj:
    pthread_mutex_unlock(&vdisp_test_module.inject_mtx);
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return ret;
}
int vdisp_test_disable_inject(
    int inj_id)
{
    int ret=-1;
    injectinfo_t *inject;
    if(inj_id>=VDISP_MAX_INPUTPORT_NUM)
        return -1;
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    pthread_mutex_lock(&vdisp_test_module.inject_mtx);
    inject=&vdisp_test_module.inject[inj_id];
    if(!inject->nenable)
        goto exit_inj;
    inject->nenable=0;
    ret=0;
exit_inj:
    pthread_mutex_unlock(&vdisp_test_module.inject_mtx);
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return ret;
}
int vdisp_test_enable_sink(
    int file,
    pfn_wait_sink_buf pfnwaitbuf,
    pfn_get_sink_buf pfngetbuf,
    pfn_finish_sink_buf pfnfinishbuf,
    void *handle)
{
    int ret=-1;
    sinkinfo_t *sink;
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    pthread_mutex_lock(&vdisp_test_module.sink_mtx);
    sink=&vdisp_test_module.sink;
    if(sink->nenable)
        goto exit_sink;
    sink->file=file;
    sink->get_sink_buf=pfngetbuf;
    sink->finish_sink_buf=pfnfinishbuf;
    sink->wait_sink_buf=pfnwaitbuf;
    sink->handle=handle;
    sink->frm_cnt=0;
    sink->nenable=1;
    ret=0;
exit_sink:
    pthread_mutex_unlock(&vdisp_test_module.sink_mtx);
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return ret;
}

int vdisp_test_disable_sink(void)
{
    int ret=-1;
    sinkinfo_t *sink;
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    pthread_mutex_lock(&vdisp_test_module.sink_mtx);
    sink=&vdisp_test_module.sink;
    if(!sink->nenable)
        goto exit_sink;
    sink->nenable=0;
    ret=0;
exit_sink:
    pthread_mutex_unlock(&vdisp_test_module.sink_mtx);
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return ret;

}
int vdisp_test_stop(void)
{
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    if(vdisp_test_module.inject_thread){
        _vdisp_pause_inject_thread();
    }
    if(vdisp_test_module.sink_thread){
        _vdisp_pause_sink_thread();
    }
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return 0;
}
int vdisp_test_start(void)
{
    pthread_mutex_lock(&vdisp_mutex);
    if(!vdisp_test_module.inited)
        goto exit;
    if(vdisp_test_module.inject_thread){
        _vdisp_resume_inject_thread();
    }
    if(vdisp_test_module.sink_thread){
        _vdisp_resume_sink_thread();
    }
exit:
    pthread_mutex_unlock(&vdisp_mutex);
    return 0;

}

#if 0
int main(int argc, const char *argv[])
{
}
#endif

