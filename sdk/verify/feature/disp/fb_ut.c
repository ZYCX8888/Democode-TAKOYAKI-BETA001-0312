#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <linux/fb.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mi_sys.h"
#include "mi_disp.h"
#include "mi_hdmi.h"
#include "mi_common.h"
#include "mi_vdec.h"
#include "mi_panel.h"

#include "init_panel_driveric.h"
#include "draw.h"
#include "ugui.h"

//#include "SAT070AT50_800x480.h"
//#include "SAT070AT40_800x480.h"
//#include "SAT070CP50_1024x600.h"
//#include "RM68200_720x1280_MIPI.h"
#include "SAT070BO30I21Y0_1024x600_MIPI.h"

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)
#define MAX(a,b)                    ((a) > (b) ? (a) : (b))

#define DISP_INPUT_PORT_MAX 16
#define DISP_LAYER_MAX 2
#define DISP_DEV_MAX 2

#define cus_print(fmt, args...) {do{printf("\033[32m");printf(fmt, ##args);printf("\033[0m");}while(0);}

#ifndef ALIGN_UP
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (( (val)/(alignment))*(alignment))
#endif

typedef struct stDispUtDev_s
{
    MI_BOOL bDevEnable;
    MI_BOOL bPanelInit;
    MI_BOOL bDevBindLayer[DISP_LAYER_MAX];
    MI_DISP_PubAttr_t stPubAttr;
    MI_PANEL_LinkType_e eLinkType;
}stDispUtDev_t;

typedef struct stDispUtLayer_s
{
    MI_BOOL bLayerEnable;
    MI_DISP_RotateMode_e eRotateMode;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
}stDispUtLayer_t;

typedef struct stDispUtPort_s
{
    MI_BOOL bPortEnable;
    MI_SYS_PixelFormat_e    ePixFormat;         /* Pixel format of the video layer */
    MI_DISP_VidWinRect_t stCropWin;                     /* rect of video out chn */
    MI_DISP_InputPortAttr_t stInputPortAttr;
    char FilePath[50];
}stDispUtPort_t;

typedef struct stTimingArray_s
{
    char desc[50];
    MI_DISP_OutputTiming_e eOutputTiming;
    MI_HDMI_TimingType_e eHdmiTiming;
    MI_U16 u16Width;
    MI_U16 u16Height;
}stTimingArray_t;

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("[%s %d]exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_)\
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = (MI_S32)_func_; \
        if (s32Ret != MI_SUCCESS)\
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__); \
        } \
    } while(0)
#endif

#define STDBG_ENTER() \
    printf("\n"); \
    printf("[IN] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define STDBG_LEAVE() \
    printf("\n"); \
    printf("[OUT] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define ST_RUN() \
    printf("\n"); \
    printf("[RUN] ok [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define COLOR_NONE          "\033[0m"
#define COLOR_BLACK         "\033[0;30m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_YELLOW        "\033[1;33m"
#define COLOR_WHITE         "\033[1;37m"

#define ST_NOP(fmt, args...)
#define ST_DBG(fmt, args...) \
    do { \
        printf(COLOR_GREEN "[DBG]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_WARN(fmt, args...) \
    do { \
        printf(COLOR_YELLOW "[WARN]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_INFO(fmt, args...) \
    do { \
        printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_ERR(fmt, args...) \
    do { \
        printf(COLOR_RED "[ERR]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define NALU_PACKET_SIZE            256*1024
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

stDispUtDev_t _gastDispUtDev[DISP_DEV_MAX];
stDispUtLayer_t _gastDispUtLayer[DISP_LAYER_MAX];
stDispUtPort_t _gastDispUtPort[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX];

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
static char *framebuffer = NULL;

stTimingArray_t astTimingArray[] = {
    {
        .desc = "user",
        .eOutputTiming = E_MI_DISP_OUTPUT_USER,
        .eHdmiTiming = E_MI_HDMI_TIMING_MAX,
    },
    {
        .desc = "480p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_480P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_480_60P,
        .u16Width = 640,.u16Height = 480
    },
    {
        .desc = "576p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_576P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_576_50P,
        .u16Width = 720,.u16Height = 576
    },
    {
        .desc = "720p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_720P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_720_50P,
            .u16Width = 1280,.u16Height = 720
    },
    {
        .desc = "720p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_720P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_720_60P,
            .u16Width = 1280,.u16Height = 720
    },
    {
        .desc = "1024x768_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1024x768_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1024x768_60P,
            .u16Width = 1024,.u16Height = 768
    },
    {
        .desc = "1080p24",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P24,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_24P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p25",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P25,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_25P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p30",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P30,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_30P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p50",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P50,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_50P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1080p60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1080P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_60P,
            .u16Width = 1920,.u16Height = 1080
    },
    {
        .desc = "1280x800_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1280x800_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1280x800_60P,
            .u16Width = 1280,.u16Height = 800
    },
    {
        .desc = "1280x1024_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1280x1024_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1280x1024_60P,
            .u16Width = 1280,.u16Height = 1024
    },
    {
        .desc = "1366x768_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1366x768_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1366x768_60P,//HDMI don't support this timing
            .u16Width = 1366,.u16Height = 768
    },
    {
        .desc = "1440x900_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1440x900_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1440x900_60P,
            .u16Width = 1440,.u16Height = 900
    },
    {
        .desc = "1680x1050_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1680x1050_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1680x1050_60P,//HDMI don't support this timing
            .u16Width = 1680,.u16Height = 1050
    },
    {
        .desc = "1600x1200_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_1600x1200_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1600x1200_60P,
            .u16Width = 1600,.u16Height = 1200
    },
    {
        .desc = "2560x1440_30",
        .eOutputTiming = E_MI_DISP_OUTPUT_2560x1440_30,
         .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1440
    },
    {
        .desc = "2560x1440_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_MAX,//not defined
         .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1440
    },
    {
        .desc = "2560x1600_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_2560x1600_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_MAX,//HDMI don't support this timing
            .u16Width = 2560,.u16Height = 1600
    },
    {
        .desc = "3840x2160_30",
        .eOutputTiming = E_MI_DISP_OUTPUT_3840x2160_30,
        .eHdmiTiming = E_MI_HDMI_TIMING_4K2K_30P,
            .u16Width = 3840,.u16Height = 2160
    },
    {
        .desc = "3840x2160_60",
        .eOutputTiming = E_MI_DISP_OUTPUT_3840x2160_60,
        .eHdmiTiming = E_MI_HDMI_TIMING_4K2K_60P,
            .u16Width = 3840,.u16Height = 2160
    },
};

static MI_BOOL disp_ut_setdev(MI_DISP_DEV DispDev)
{
    MI_DISP_PubAttr_t stPubAttr;

    memcpy(&stPubAttr, &_gastDispUtDev[DispDev].stPubAttr, sizeof(MI_DISP_PubAttr_t));
    //set disp pub
    stPubAttr.u32BgColor = YUYV_BLACK;
    MI_DISP_SetPubAttr(DispDev,  &stPubAttr);
    MI_DISP_Enable(DispDev);
    _gastDispUtDev[DispDev].bDevEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_setlayer(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer)
{
#if 0  //i2m unneeded
    MI_DISP_VideoLayerAttr_t stLayerAttr;

    memcpy(&stLayerAttr, &_gastDispUtLayer[DispLayer].stLayerAttr, sizeof(MI_DISP_VideoLayerAttr_t));
    MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
#endif
    MI_DISP_BindVideoLayer(DispLayer,DispDev);
    MI_DISP_EnableVideoLayer(DispLayer);
    _gastDispUtDev[DispDev].bDevBindLayer[DispLayer] = TRUE;
    _gastDispUtLayer[DispLayer].bLayerEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_setport(MI_DISP_LAYER DispLayer, MI_U32 DispInport)
{
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_DISP_RotateConfig_t stRotateConfig;

    memcpy(&stInputPortAttr, &_gastDispUtPort[DispLayer][DispInport].stInputPortAttr, sizeof(MI_DISP_InputPortAttr_t));
    printf("%s:%d layer:%d port:%d srcwidth:%d srcheight:%d x:%d y:%d outwidth:%d outheight:%d\n",__FUNCTION__,__LINE__,
        DispLayer,DispInport,
        stInputPortAttr.u16SrcWidth,stInputPortAttr.u16SrcHeight,
        stInputPortAttr.stDispWin.u16X,stInputPortAttr.stDispWin.u16Y,
        stInputPortAttr.stDispWin.u16Width,stInputPortAttr.stDispWin.u16Height);

    stRotateConfig.eRotateMode = _gastDispUtLayer[DispLayer].eRotateMode;
    MI_DISP_SetInputPortAttr(DispLayer, DispInport, &stInputPortAttr);
    MI_DISP_SetVideoLayerRotateMode(DispLayer, &stRotateConfig);
    MI_DISP_EnableInputPort(DispLayer, DispInport);
    MI_DISP_SetInputPortSyncMode(DispLayer, DispInport, E_MI_DISP_SYNC_MODE_FREE_RUN);

    _gastDispUtPort[DispLayer][DispInport].bPortEnable = TRUE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_disabledev(MI_DISP_DEV DispDev)
{
    MI_DISP_Disable(DispDev);
    _gastDispUtDev[DispDev].bDevEnable = FALSE;

    return MI_SUCCESS;
}
static MI_BOOL disp_ut_disablelayer(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer)
{
    MI_DISP_DisableVideoLayer(DispLayer);
    MI_DISP_UnBindVideoLayer(DispLayer, DispDev);
    _gastDispUtDev[DispDev].bDevBindLayer[DispLayer] = FALSE;
    _gastDispUtLayer[DispLayer].bLayerEnable = FALSE;

    return MI_SUCCESS;
}

static MI_BOOL disp_ut_disableport(MI_DISP_LAYER DispLayer, MI_U32 DispInport)
{
    MI_DISP_DisableInputPort(DispLayer, DispInport);
    _gastDispUtPort[DispLayer][DispInport].bPortEnable = FALSE;
    return MI_SUCCESS;
}

static void GetLayerDisplaySize(MI_DISP_OutputTiming_e eOutputTiming, MI_U32 *LayerDisplayWidth, MI_U32 *LayerDisplayHeight)
{
    if(eOutputTiming == E_MI_DISP_OUTPUT_USER)
    {
        *LayerDisplayWidth = stPanelParam.u16Width;
        *LayerDisplayHeight = stPanelParam.u16Height;
        return;
    }
    else
        printf("invalid output timing\n");
}

static MI_DISP_OutputTiming_e parse_outputtiming(char *str)
{
    MI_DISP_OutputTiming_e eOutputTiming;

    int timingNum = sizeof(astTimingArray)/sizeof(stTimingArray_t);
    int i = 0;
    for(i = 0;i<timingNum;i++)
    {
        if(strcasecmp(str,astTimingArray[i].desc)==0)
        {
            return astTimingArray[i].eOutputTiming;
        }
    }
    printf("using default outputtiming 1080p60\n");
    return E_MI_DISP_OUTPUT_1080P60;
}
/**
 *dump fix info of Framebuffer
 */
void printFixedInfo ()
{
    printf ("Fixed screen info:\n"
            "\tid: %s\n"
            "\tsmem_start: 0x%lx\n"
            "\tsmem_len: %d\n"
            "\ttype: %d\n"
            "\ttype_aux: %d\n"
            "\tvisual: %d\n"
            "\txpanstep: %d\n"
            "\typanstep: %d\n"
            "\tywrapstep: %d\n"
            "\tline_length: %d\n"
            "\tmmio_start: 0x%lx\n"
            "\tmmio_len: %d\n"
            "\taccel: %d\n"
            "\n",
            finfo.id, finfo.smem_start, finfo.smem_len, finfo.type,
            finfo.type_aux, finfo.visual, finfo.xpanstep, finfo.ypanstep,
            finfo.ywrapstep, finfo.line_length, finfo.mmio_start,
            finfo.mmio_len, finfo.accel);
}

/**
 *dump var info of Framebuffer
 */
void printVariableInfo ()
{
    printf ("Variable screen info:\n"
            "\txres: %d\n"
            "\tyres: %d\n"
            "\txres_virtual: %d\n"
            "\tyres_virtual: %d\n"
            "\tyoffset: %d\n"
            "\txoffset: %d\n"
            "\tbits_per_pixel: %d\n"
            "\tgrayscale: %d\n"
            "\tred: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tgreen: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tblue: offset: %2d, length: %2d, msb_right: %2d\n"
            "\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tnonstd: %d\n"
            "\tactivate: %d\n"
            "\theight: %d\n"
            "\twidth: %d\n"
            "\taccel_flags: 0x%x\n"
            "\tpixclock: %d\n"
            "\tleft_margin: %d\n"
            "\tright_margin: %d\n"
            "\tupper_margin: %d\n"
            "\tlower_margin: %d\n"
            "\thsync_len: %d\n"
            "\tvsync_len: %d\n"
            "\tsync: %d\n"
            "\tvmode: %d\n"
            "\n",
            vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
            vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel,
            vinfo.grayscale, vinfo.red.offset, vinfo.red.length,
            vinfo.red.msb_right, vinfo.green.offset, vinfo.green.length,
            vinfo.green.msb_right, vinfo.blue.offset, vinfo.blue.length,
            vinfo.blue.msb_right, vinfo.transp.offset, vinfo.transp.length,
            vinfo.transp.msb_right, vinfo.nonstd, vinfo.activate,
            vinfo.height, vinfo.width, vinfo.accel_flags, vinfo.pixclock,
            vinfo.left_margin, vinfo.right_margin, vinfo.upper_margin,
            vinfo.lower_margin, vinfo.hsync_len, vinfo.vsync_len,
            vinfo.sync, vinfo.vmode);
}

UG_GUI _gUGUI;

static void pset(UG_S16 x, UG_S16 y, UG_COLOR color)
{
    drawPixel(x, y, color, &vinfo, &finfo, framebuffer);
}

static MI_BOOL fb_ut(MI_U8 u8LayerId, MI_U8 u8PortId)
{
    MI_DISP_DEV DispDev = 0;
    const char *devfile = "/dev/fb0";
    int fbFd = 0;
    long int screensize = 0;
    MI_SYS_ChnPort_t stSysChnPort;

    if(_gastDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD)
    {
        _gastDispUtDev[DispDev].eLinkType = stPanelParam.eLinkType;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vact = stPanelParam.u16Height;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth + stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hact = stPanelParam.u16Width;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth + stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvact = 0;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvbb = 0;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Bvfb = 0;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
        _gastDispUtDev[DispDev].stPubAttr.stSyncInfo.u32FrameRate = stPanelParam.u16DCLK*1000000/(stPanelParam.u16HTotal*stPanelParam.u16VTotal);
    }
    //set disp pub
    disp_ut_setdev(DispDev);

    //set panel config
    if((_gastDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD) &&
            ((_gastDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_LVDS)
            || (_gastDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
            || (_gastDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_TTL)))
    {
        MI_PANEL_Init(_gastDispUtDev[DispDev].eLinkType);
        if(_gastDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_TTL)
        {
            //init_panel_LX50FWB();
        }
        MI_PANEL_SetPanelParam(&stPanelParam);
        if(_gastDispUtDev[DispDev].eLinkType == E_MI_PNL_LINK_MIPI_DSI)
        {
            MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
        }
        _gastDispUtDev[DispDev].bPanelInit = TRUE;
    }

    fbFd = open (devfile, O_RDWR);
    if(fbFd == -1)
    {
        perror ("Error: cannot open framebuffer device");
        exit(1);
    }
    //get fb_fix_screeninfo
    if(ioctl(fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }
    printFixedInfo();
    //get fb_var_screeninfo
    if(ioctl(fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }
    printVariableInfo();

    screensize = finfo.smem_len;

    /* Map the device to memory */
    framebuffer = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fbFd, 0);
    if(framebuffer == MAP_FAILED)
    {
        perror ("Error: Failed to map framebuffer device to memory");
        exit (4);
    }
#if 1 
    //draw red rectangle
    drawRect (vinfo.xres / 8, vinfo.yres / 8,
             vinfo.xres / 4, vinfo.yres / 4, 0xffff0000, &vinfo, &finfo, framebuffer);

    //draw DarkGoldenrod rectangle
    drawRect (vinfo.xres * 3 / 8, vinfo.yres * 3 / 8,
                vinfo.xres / 4, vinfo.yres / 4, 0xffb8860b, &vinfo, &finfo, framebuffer);

    //draw blue rectanble
    drawRect (vinfo.xres * 5 / 8, vinfo.yres * 5 / 8,
             vinfo.xres / 4, vinfo.yres / 4, 0xff0000ff, &vinfo, &finfo, framebuffer);

#endif
#if 0
    UG_Init(&_gUGUI,pset,stPanelParam.u16Width,stPanelParam.u16Height);
    UG_FontSelect(&FONT_12X16);
    UG_DrawCircle(320,500,60,C_YELLOW);
    UG_DrawCircle(320,630,60,C_BLUE);
    UG_DrawCircle(320,760,60,C_RED);
    UG_DrawCircle(395,565,60,C_VIOLET);
    UG_DrawCircle(395,695,60,C_GREEN);
#endif
    printf ("Done.\n");

    munmap (framebuffer, screensize);

    return MI_SUCCESS;
}

int main(int argc, char **argv)
{
    MI_BOOL ret;
    int opt;
    char buf[50];
    fd_set stFdSet;
    MI_DISP_Interface_e eInterface = E_MI_DISP_INTF_LCD;
    MI_DISP_OutputTiming_e eOutputTiming = E_MI_DISP_OUTPUT_USER;
    MI_U8 u8LayerId = 0;
    MI_U8 u8PortId = 0;
    MI_U8 u8DevIndex = 0;
    MI_U8 u8LayerIndex = 0;
    MI_U8 u8PortIndex = 0;

    memset(_gastDispUtDev, 0, sizeof(_gastDispUtDev));
    memset(_gastDispUtLayer, 0, sizeof(_gastDispUtLayer));
    memset(_gastDispUtPort, 0, sizeof(_gastDispUtPort));

    while ((opt = getopt(argc, argv, "t:o:")) != -1) {
        switch(opt){
            case 't':
                if(strcmp("lcd",optarg) == 0)
                    eInterface = E_MI_DISP_INTF_LCD;
                else if(strcmp("hdmi",optarg) == 0)
                    eInterface = E_MI_DISP_INTF_HDMI;
                break;
            case 'o':
                eOutputTiming = parse_outputtiming(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-t interface] [-o outputtiming]\n",argv[0]);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
    }

    _gastDispUtDev[0].stPubAttr.eIntfSync = eOutputTiming;
    _gastDispUtDev[0].stPubAttr.eIntfType = eInterface;
    ret = fb_ut(u8LayerId, u8PortId);

    FD_ZERO(&stFdSet);
    FD_SET(0,&stFdSet);
    for(;;)
    {
        cus_print("input 'q' exit\n");
        select(1, &stFdSet, NULL, NULL, NULL);
        if(FD_ISSET(0, &stFdSet))
        {
            int i = read(0, buf, sizeof(buf));
            if(i>0 && (buf[0] == 'q'))
            {
                break;
            }
        }
    }
    for(u8DevIndex = 0; u8DevIndex < DISP_DEV_MAX; u8DevIndex++)
    {
        if(_gastDispUtDev[u8DevIndex].bDevEnable == TRUE)
        {
            for(u8LayerIndex = 0; u8LayerIndex < DISP_LAYER_MAX; u8LayerIndex++)
            {
                if(_gastDispUtLayer[u8LayerIndex].bLayerEnable == TRUE)
                {
                    for(u8PortIndex = 0; u8PortIndex < DISP_INPUT_PORT_MAX; u8PortIndex++)
                    {
                        if(_gastDispUtPort[u8LayerIndex][u8PortIndex].bPortEnable == TRUE)
                        {
                            disp_ut_disableport(u8LayerIndex, u8PortIndex);
                            cus_print("disable layerid:%d portid:%d\n",u8LayerIndex,u8PortIndex);
                        }
                    }
                    disp_ut_disablelayer(u8DevIndex, u8LayerIndex);
                    cus_print("disable layerid:%d\n",u8LayerIndex);
                }
            }
            disp_ut_disabledev(u8DevIndex);
            cus_print("disable devid:%d\n",u8DevIndex);
        }
        if( _gastDispUtDev[u8DevIndex].bPanelInit == TRUE)
        {
            MI_PANEL_DeInit();
            cus_print("deinit panel\n");
        }
    }

    printf("--------EXIT--------\n");
    return 0;
}

