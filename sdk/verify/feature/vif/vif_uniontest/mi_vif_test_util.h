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
#ifndef _MI_VIF_TEST_UTIL_H_
#define _MI_VIF_TEST_UTIL_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>



#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_divp.h"
#include "mi_disp.h"
#include "mi_hdmi.h"
#include "../i2c.h"
#include "../AddaSysApi.h"

typedef enum
{
    VIF_TEST_DBG_NONE = 0,
    VIF_TEST_DBG_ERR,
    VIF_TEST_DBG_WRN,
    VIF_TEST_DBG_INFO,
    VIF_TEST_DBG_DEBUG,
    VIF_TEST_DBG_ALL
} VIF_DBG_LEVEL_e;

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

extern VIF_DBG_LEVEL_e vif_test_debug_level;
extern int vif_test_func_trace;

#define DBG_DEBUG(fmt, args...)     ({do{if(vif_test_debug_level>=VIF_TEST_DBG_DEBUG){printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define DBG_INFO(fmt, args...)     ({do{if(vif_test_debug_level>=VIF_TEST_DBG_INFO){printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define DBG_WRN(fmt, args...)      ({do{if(vif_test_debug_level>=VIF_TEST_DBG_WRN){printf(ASCII_COLOR_YELLOW"[APP WRN ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define DBG_ERR(fmt, args...)      ({do{if(vif_test_debug_level>=VIF_TEST_DBG_ERR){printf(ASCII_COLOR_RED"[APP ERR ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define DBG_EXIT_ERR(fmt, args...) ({do{printf(ASCII_COLOR_RED"<<<%s[%d] " fmt ASCII_COLOR_END,__FUNCTION__,__LINE__,##args);}while(0);})
#define DBG_ENTER()                ({do{if(vif_test_func_trace){printf(ASCII_COLOR_BLUE">>>%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})
#define DBG_EXIT_OK()              ({do{if(vif_test_func_trace){printf(ASCII_COLOR_BLUE"<<<%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})

#define MAX_VENC_CHANNEL (16)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__,__LINE__);\
        return 1;\
    }


enum
{
    /* For Hi3531 or Hi3532 */
    SAMPLE_VI_MODE_1_D1 = 0,
    SAMPLE_VI_MODE_16_D1,
    SAMPLE_VI_MODE_16_960H,
    SAMPLE_VI_MODE_4_720P,
    SAMPLE_VI_MODE_1_1080P,
    /* For Hi3521 */
    SAMPLE_VI_MODE_8_D1,
    SAMPLE_VI_MODE_1_720P,
    SAMPLE_VI_MODE_16_Cif,
    SAMPLE_VI_MODE_16_2Cif,
    SAMPLE_VI_MODE_16_D1Cif,
    SAMPLE_VI_MODE_1_D1Cif,
    /*For Hi3520A*/
    SAMPLE_VI_MODE_4_D1,
    SAMPLE_VI_MODE_8_2Cif,
};

struct venc_channel_state
{
    MI_U32 u32DevId;
};
#define FUNC_ENTRY2()   do{fprintf(stderr, "Enter %s line: %d\n",__FUNCTION__,__LINE__);}while(0)

int my_write_yuv_file(FILE_HANDLE filehandle, MI_SYS_FrameData_t framedata);
MI_U32 vif_get_time();
int vif_dh9931_Init(int device , VD_PORT_MUX_MODE_E enVdPortMuxMode, VD_PORT_EDGE_E enVdportEdge, VD_PORT_CLK_E enVdportClk);
int vif_init_hdmi(void);
MI_S32 vif_deInit_hdmi(void);
MI_S32 create_disp_channel(MI_DISP_CHN DispChn, MI_SYS_PixelFormat_e ePixFormat);
int config_disp_port(MI_U32 u32InputPort , MI_U16 u16Width , MI_U16 u16Height);
int vif_get_vid_fmt(MI_VIF_CHN VifChn , MI_U32* pu32Width, MI_U32* pu32Height , MI_U32* pu32FrameRate , MI_SYS_FrameScanMode_e* peScanMode);
int init_vif_vpe(MI_VIF_CHN u32VifChn , MI_VPE_CHANNEL u32VpeChn , MI_VIF_FrameRate_e eFrameRate);
int destroy_disp_channel(MI_DIVP_CHN DivpChn);
MI_U32 vif_cal_fps(MI_U32 u32FrameRate, MI_VIF_FrameRate_e eFrameRate);
int vif_init_video_info(void);


int stop_vif_1_1(void);
int stop_vif_1_4(void);
int stop_vif_4_1(void);
int stop_vif_4_4(void);

int start_vif_1_1D1(void);
int start_vif_1_4D1(void);
int start_vif_4_1D1(void);
int start_vif_4_4D1(void);
int start_vif_1_1FHD(void);
int start_vif_4_1FHD(void);


int stop_vpe_1_1(void);
int stop_vpe_1_4(void);
int stop_vpe_4_1(void);
int stop_vpe_4_4(void);

int start_vpe_1_1D1(void);
int start_vpe_1_4D1(void);
int start_vpe_4_1D1(void);
int start_vpe_4_4D1(void);
int start_vpe_1_1FHD(void);
int start_vpe_4_1FHD(void);


int start_test_20(void);
int stop_test_20(void);
int start_test_21(void);
int stop_test_21(void);
int start_test_22(void);
int stop_test_22(void);
int start_test_23(void);
int stop_test_23(void);
int start_test_24(void);
int stop_test_24(void);
int start_test_25(void);
int stop_test_25(void);
int start_test_26(void);
int stop_test_26(void);
int start_test_27(void);
int stop_test_27(void);
int start_test_28(void);
int stop_test_28(void);
int start_test_29(void);
int stop_test_29(void);


int start_disp_1_D1(void);
int start_test_disp_4D1(void);
int start_test_disp_1FHD(void);
int start_test_disp_4FHD(void);

int stop_test_disp_1(void);
int stop_test_disp_4(void);

int start_disp_1_1D1(void);
int start_disp_1_4D1(void);
int start_disp_4_1D1(void);
int start_disp_4_4D1(void);
int start_disp_1_1FHD(void);
int start_disp_4_1FHD(void);

int start_divp_1_1D1_no_hvsp(void);
int start_divp_1_1D1_hvsp(void);
int stop_divp_1_1D1(void);

int start_test_34(void);
int stop_test_34(void);
int start_test_35(void);
int stop_test_35(void);
int start_test_36(void);
int stop_test_36(void);
int start_test_37(void);
int stop_test_37(void);
int start_test_38(void);
int stop_test_38(void);
int start_test_39(void);
int stop_test_39(void);

int bind_module(MI_ModuleId_e eSrcModId, MI_U32 u32SrcDevId, MI_U32 u32SrcChnId, MI_U32 u32SrcPortId, MI_U32 u32SrcFrmrate,
                MI_ModuleId_e eDstModId, MI_U32 u32DstDevId, MI_U32 u32DstChnId, MI_U32 u32DstPortId  , MI_U32 u32DstFrmrate);
int unbind_module(MI_ModuleId_e eSrcModId, MI_U32 u32SrcDevId, MI_U32 u32SrcChnId, MI_U32 u32SrcPortId,
                  MI_ModuleId_e eDstModId, MI_U32 u32DstDevId, MI_U32 u32DstChnId, MI_U32 u32DstPortId);
int deinit_vif_vpe(MI_VIF_CHN u32VifChn, MI_VPE_CHANNEL u32VpeChn);

int create_vif_dev(MI_VIF_DEV u32VifDev, MI_U32 u32Mode);
int destroy_vif_dev(MI_VIF_DEV u32VifDev);

int create_vif_channel(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort, MI_U32 u32Width, MI_U32 u32Height, MI_SYS_FrameScanMode_e eScanMode, MI_SYS_PixelFormat_e ePixFormat, MI_VIF_FrameRate_e eFrameRate);

int create_vpe_channel(MI_VPE_CHANNEL VpeChannel, MI_SYS_WindowRect_t *pstCropWin);
int config_vpe_outport(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstDispWin,
                       MI_SYS_PixelFormat_e ePixelFormat);
int init_vif_vpe_D1(MI_VIF_CHN u32VifChn , MI_VPE_CHANNEL u32VpeChn , MI_SYS_PixelFormat_e  ePixelFormat);
int init_vif_vpe_FHD(MI_VIF_CHN u32VifChn , MI_VPE_CHANNEL u32VpeChn, MI_SYS_PixelFormat_e  ePixelFormat);

int set_extra_argv(int argc, const char **argv);
int create_h264_channel(MI_VENC_CHN VencChannel, MI_U32 VencPortId, MI_U32 u32Width, MI_U32 u32Height);
int destroy_vif_channel(MI_VIF_CHN VifChn,    MI_VIF_PORT VifPort);
int destroy_vpe_channel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort);
int bind_vif_vpe(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort);
int bind_vpe_venc(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort, MI_VENC_CHN VencChn);
int unbind_vif_vpe(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort);
int unbind_vpe_venc(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort, MI_VENC_CHN VencChn);
int start_vpe_channel(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort);
int start_venc_channel(MI_VENC_CHN VencChn);
int stop_venc_channel(MI_VENC_CHN VencChn);
int stop_vpe_channel(MI_VPE_CHANNEL VpeChn, MI_VPE_PORT VpePort);
#endif // _MI_VIF_TEST_UTIL_H_
