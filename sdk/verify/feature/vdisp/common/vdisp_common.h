#ifndef __VDISP_COMMON_H__
#define __VDISP_COMMON_H__

#include <mi_sys_datatype.h>
#include <mi_sys.h>
#include <mi_disp_datatype.h>
#include <mi_disp.h>
#include <mi_vdisp_datatype.h>
#include <mi_vdisp.h>
#include <mi_gfx_datatype.h>
#include <mi_gfx.h>


#define VDISP_TEST_DATA_FILE_1280x720 "../data/yuv420sp_1280x720.yuv"
#define VDISP_TEST_DATA_FILE_960x540 "../data/yuv420sp_960x540.yuv"
#define VDISP_TEST_DATA_FILE_480x270 "../data/yuv420sp_480x270.yuv"

#define MAKE_YUYV_VALUE(y,u,v)    ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK                MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE                MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                  MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN                MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE                 MAKE_YUYV_VALUE(29,225,107)

typedef struct {
    MI_VDISP_InputChnAttr_t inAttr;
    MI_U16 inChn;
    MI_BOOL enable;
    MI_SYS_BUF_HANDLE bufHandle;
    MI_SYS_BufInfo_t bufInfo;
} vdispInputMeta;
typedef struct {
    MI_VDISP_OutputPortAttr_t outAttr;
    MI_U16 outPort;
    MI_SYS_BUF_HANDLE bufHandle;
    MI_SYS_BufInfo_t bufInfo;
} vdispOutputMeta;



MI_U32 _MI_VDISP_IMPL_CalFrameSize(MI_SYS_PixelFormat_e ePixelFmt, MI_U16 u16Width, MI_U16 u16Height);

void vdisp_func_420sp(
    MI_SYS_FrameData_t *pstOutFrame,
    MI_SYS_WindowRect_t *dstRect,
    MI_SYS_FrameData_t *pstInFrame,
    MI_SYS_WindowRect_t *srcRect);

void vdisp_configOutputputMeta(vdispOutputMeta *outPutMeta, MI_U8 fps, stTimingArray_t *pstTiming);

void vdisp_configInputMeta(vdispInputMeta *inPutMeta, MI_U8 cellNum, vdispOutputMeta *outPutMeta);

void  vdisp_readFile(void *data, FILE *fp, MI_VDISP_InputChnAttr_t *inAttr);

void vdisp_process_config(
    MI_SYS_FrameData_t *pstDstbuf,
    MI_SYS_WindowRect_t *pstDstRect,
    MI_SYS_FrameData_t *pstSrcBuf,
    MI_SYS_WindowRect_t *pstSrcRect,
    char *data,
    MI_U64 phyData,
    MI_SYS_FrameData_t *pstOutFrame,
    vdispInputMeta *inMeta,
    vdispOutputMeta *outMeta);

#endif


