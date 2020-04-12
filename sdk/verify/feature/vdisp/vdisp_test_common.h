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
#ifndef _VDISP_TEST_COM_H_
#define _VDISP_TEST_COM_H_

#define DBG_ERR(msg, ...) do{printf("usr VDISP ERR: %s-%d:" msg,__FUNCTION__, __LINE__,##__VA_ARGS__);fflush(stdout); }while(0)
#define DBG_INFO(msg, ...) do{printf("usr VDISP INF: %s-%d:" msg,__FUNCTION__, __LINE__,##__VA_ARGS__);fflush(stdout);}while(0)
#define BUG_ON(cond) do{if(cond){DBG_ERR("usr Bug %s\n", #cond);while(1);/**((char*)(0))=0;*/}}while(0)
typedef unsigned long long uint64_t;
typedef struct frameplane_s
{
    unsigned char* paddr;
    unsigned long long phy;
    int nwidth;
    int nheight;
    int nstride;
}frameplane_t;
typedef struct framebuf_s
{
    uint64_t u64pts;
    int nplanenum;
    frameplane_t plane[3];
    void *priv;
}framebuf_t;

typedef int (*pfn_get_inject_buf)(void *handle, framebuf_t *frame);
typedef void (*pfn_finish_inject_buf)(void *handle, framebuf_t *frame, int bvaliddata);
typedef int (*pfn_wait_sink_buf)(void *handle, int timeout);
typedef int (*pfn_get_sink_buf)(void *handle, framebuf_t *frame);
typedef void (*pfn_finish_sink_buf)(void *handle, framebuf_t *frame);

int vdisp_test_init(void);
int vdisp_test_deinit(void);
int vdisp_test_enable_inject(
    int inj_id,
    int file,
    int framerate,
    pfn_get_inject_buf pfngetbuf,
    pfn_finish_inject_buf pfnfinishbuf,
    void *handle
    );
int vdisp_test_disable_inject(
    int inj_id);
int vdisp_test_enable_sink(
    int file,
    pfn_wait_sink_buf pfnwaitbuf,
    pfn_get_sink_buf pfngetbuf,
    pfn_finish_sink_buf pfnfinishbuf,
    void *handle);
int vdisp_test_disable_sink(void);
int vdisp_test_start(void);
int vdisp_test_stop(void);
int vdisp_test_open_file(const char *pathname, int bwrite);
void vdisp_test_close_file(int f);
#endif
