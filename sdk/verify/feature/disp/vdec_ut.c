#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
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
#include "mi_common.h"
#include "mi_vdec.h"

#include "mi_panel.h"
#include "init_panel_driveric.h"
#include "SAT070AT50_800x480.h"
//#include "SAT070AT40_800x480.h"
//#include "SAT070CP50_1024x600.h"
//#include "RM68200_720x1280.h"

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

typedef struct stDispTestPubBuffThreadArgs_s
{
    pthread_t pt;
    pthread_t ptsnap;
    char FileName[50];
    MI_BOOL bRunFlag;
    MI_DISP_LAYER DispLayer;
    MI_U32 u32PortId;
    MI_U16 u16BuffWidth;
    MI_U16 u16BuffHeight;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_SYS_ChnPort_t stSysChnPort;
}stDispTestPutBuffThreadArgs_t;

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
typedef struct ST_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
} ST_Sys_BindInfo_T;

typedef struct ST_Sys_Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_S16 s16PicW;
    MI_S16 s16PicH;
} ST_Rect_T;

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

static MI_BOOL g_PushDataExit[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX] = {FALSE};
static MI_BOOL g_PushDataStop[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX] = {FALSE};
stDispTestPutBuffThreadArgs_t gastDispTestPutBufThread[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX];

stDispUtDev_t _gastDispUtDev[DISP_DEV_MAX];
stDispUtLayer_t _gastDispUtLayer[DISP_LAYER_MAX];
stDispUtPort_t _gastDispUtPort[DISP_LAYER_MAX][DISP_INPUT_PORT_MAX];
FILE *g_pStreamFile[32] = {NULL};
static char g_filepath[128];
static int g_VdecRun = FALSE;
static pthread_t g_VdeStream_tid = 0;

static MI_S32 Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;

    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));

    MI_SYS_Init();

    MI_SYS_GetVersion(&stVersion);
    MI_SYS_GetCurPts(&u64Pts);

    u64Pts = 0xF1237890F1237890;
    MI_SYS_InitPtsBase(u64Pts);

    u64Pts = 0xE1237890E1237890;
    MI_SYS_SyncPts(u64Pts);

    return MI_SUCCESS;
}

static MI_S32 Sys_Exit(void)
{
    MI_SYS_Exit();
    return MI_SUCCESS;
}

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

MI_S32 ST_ModuleBind(MI_S32 s32SrcMod, MI_S32 s32SrcDev, MI_S32 s32SrcChn,MI_S32 s32SrcPort,
    MI_S32 s32DstMod,MI_S32 s32DstDev,MI_S32 s32DstChn,MI_S32 s32DstPort)
{
    ST_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(stBindInfo));

    stBindInfo.stSrcChnPort.eModId = (MI_ModuleId_e)s32SrcMod;
    stBindInfo.stSrcChnPort.u32DevId = s32SrcDev;
    stBindInfo.stSrcChnPort.u32ChnId = s32SrcChn;
    stBindInfo.stSrcChnPort.u32PortId = s32SrcPort;

    stBindInfo.stDstChnPort.eModId = (MI_ModuleId_e)s32DstMod;
    stBindInfo.stDstChnPort.u32DevId = s32DstDev;
    stBindInfo.stDstChnPort.u32ChnId = s32DstChn;
    stBindInfo.stDstChnPort.u32PortId = s32DstPort;

    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    printf("xxxxxxxModule bind src(%d-%d-%d-%d) dst(%d-%d-%d-%d)...\n", s32SrcMod, s32SrcDev, s32SrcChn, s32SrcPort,
        s32DstMod, s32DstDev, s32DstChn, s32DstPort);
    ExecFunc(MI_SYS_BindChnPort(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort, \
        stBindInfo.u32SrcFrmrate, stBindInfo.u32DstFrmrate), MI_SUCCESS);

    return 0;
}

MI_S32 ST_ModuleUnBind(MI_S32 s32SrcMod, MI_S32 s32SrcDev, MI_S32 s32SrcChn,MI_S32 s32SrcPort,
    MI_S32 s32DstMod,MI_S32 s32DstDev,MI_S32 s32DstChn,MI_S32 s32DstPort)
{
    ST_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(stBindInfo));

    stBindInfo.stSrcChnPort.eModId = (MI_ModuleId_e)s32SrcMod;
    stBindInfo.stSrcChnPort.u32DevId = s32SrcDev;
    stBindInfo.stSrcChnPort.u32ChnId = s32SrcChn;
    stBindInfo.stSrcChnPort.u32PortId = s32SrcPort;

    stBindInfo.stDstChnPort.eModId = (MI_ModuleId_e)s32DstMod;
    stBindInfo.stDstChnPort.u32DevId = s32DstDev;
    stBindInfo.stDstChnPort.u32ChnId = s32DstChn;
    stBindInfo.stDstChnPort.u32PortId = s32DstPort;

    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    printf("xxxxxxxModule Unbind src(%d-%d-%d-%d) dst(%d-%d-%d-%d)...\n", s32SrcMod, s32SrcDev, s32SrcChn, s32SrcPort,
        s32DstMod, s32DstDev, s32DstChn, s32DstPort);
    ExecFunc(MI_SYS_UnBindChnPort(&stBindInfo.stSrcChnPort, &stBindInfo.stDstChnPort), MI_SUCCESS);

    return 0;
}

MI_S32 ST_CreateVdecChannel(MI_S32 s32VdecChn, MI_S32 s32CodecType,
    MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32OutWidth, MI_U32 u32OutHeight)
{
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_OutputPortAttr_t stOutputPortAttr;

    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
    stVdecChnAttr.u32PicWidth   = u32Width;
    stVdecChnAttr.u32PicHeight  = u32Height;
    stVdecChnAttr.u32Priority   = 0;
    stVdecChnAttr.eCodecType    = s32CodecType;
    stVdecChnAttr.eDpbBufMode = 0;

    STCHECKRESULT(MI_VDEC_CreateChn(s32VdecChn, &stVdecChnAttr));
    STCHECKRESULT(MI_VDEC_StartChn(s32VdecChn));
    if (u32OutWidth > u32Width)
    {
        u32OutWidth = u32Width;
    }
    if (u32OutHeight > u32Height)
    {
        u32OutHeight = u32Height;
    }
    stOutputPortAttr.u16Width = u32OutWidth;
    stOutputPortAttr.u16Height = u32OutHeight;
    MI_VDEC_SetOutputPortAttr(s32VdecChn, &stOutputPortAttr);

    return MI_SUCCESS;
}

MI_S32 ST_DestroyVdecChannel(MI_S32 s32VdecChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_VDEC_StopChn(s32VdecChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VdecChn, s32Ret);
    }
    s32Ret |= MI_VDEC_DestroyChn(s32VdecChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VdecChn, s32Ret);
    }

    return s32Ret;
}

void *ST_VdecSendStream(void *args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32TimeOutMs = 20, s32ChannelId = 0, s32TempHeight = 0;
    MI_S32 s32Ms = 30;
    MI_BOOL bVdecChnEnable;
    MI_U16 u16Times = 20000;

    MI_S32 s32ReadCnt = 0;
    FILE *readfp = NULL;
    MI_U8 *pu8Buf = NULL;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[32] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_VideoStream_t stVdecStream;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    char tname[32];
    memset(tname, 0, 32);

    vdecChn = 0;
    snprintf(tname, 32, "push_t_%u", vdecChn);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;//0 1 2 3
    stChnPort.u32PortId = 0;
    if ((access(g_filepath, F_OK))!=-1)
    {
        readfp = fopen(g_filepath, "rb"); //ES
        ST_DBG("open current dir es file\n");
    }
    else
    {
    ST_ERR("Open es file failed!\n");
        return NULL;
    }

    if (!readfp)
    {
        ST_ERR("Open es file failed!\n");
        return NULL;
    }
    else
    {
        g_pStreamFile[vdecChn] = readfp;
    }

    // s32Ms = _stTestParam.stChannelInfo[s32VoChannel].s32PushDataMs;
    // bVdecChnEnable = _stTestParam.stChannelInfo[0].bVdecChnEnable;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = (MI_U8 *)malloc(NALU_PACKET_SIZE);

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 3);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth error, %X\n", s32Ret);
        return NULL;
    }

    s32Ms = 30;

    //printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (g_VdecRun)
    {
        memset(au8Header, 0, 16);
        u32Pos = fseek(readfp, 0, SEEK_CUR);
        s32Len = fread(au8Header, 1, 16, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }
        u32FrameLen = MI_U32VALUE(au8Header, 4);
        // printf("vdecChn:%d, u32FrameLen:%d, %d\n", vdecChn, u32FrameLen, NALU_PACKET_SIZE);
        if(u32FrameLen > NALU_PACKET_SIZE)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }
        s32Len = fread(pu8Buf, 1, u32FrameLen, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }

        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = s32Len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len + 16; //back length

        if(0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1]
            && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]
            && 0x65 == stVdecStream.pu8Addr[4] || 0x61 == stVdecStream.pu8Addr[4]
            || 0x26 == stVdecStream.pu8Addr[4] || 0x02 == stVdecStream.pu8Addr[4]
            || 0x41 == stVdecStream.pu8Addr[4])
        {
            usleep(s32Ms * 1000);
        }

        if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(vdecChn, &stVdecStream, s32TimeOutMs)))
        {
            //ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(readfp, - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + ST_Sys_GetPts(30);

        if (0 == (s32ReadCnt++ % 30))
            ;// printf("vdec(%d) push buf cnt (%d)...\n", s32VoChannel, s32ReadCnt)
            ;//printf("###### ==> Chn(%d) push frame(%d) Frame Dec:%d  Len:%d\n", s32VoChannel, s32ReadCnt, stChnStat.u32DecodeStreamFrames, u32Len);
    }
    printf("\n\n");
    usleep(300000);
    free(pu8Buf);

    printf("End----------------------%d------------------End\n", stChnPort.u32ChnId);

    return NULL;
}

MI_S32 ST_CreateVdec2DispPipe(MI_S32 s32VdecChn, MI_S32 s32DivpChn, MI_U32 u32VdecW, MI_U32 u32VdecH, MI_S32 s32CodecType, MI_S32 s32OutW, MI_S32 s32OutH)
{
    ST_Rect_T stCrop= {0, 0, 0, 0};
    ST_CreateVdecChannel(s32VdecChn, s32CodecType, u32VdecW, u32VdecH, s32OutW, s32OutH);
    ST_ModuleBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                    E_MI_MODULE_ID_DISP, 0, 0, 0); //DIVP->DISP

    return MI_SUCCESS;
}

MI_S32 ST_DestroyVdec2DispPipe(     MI_S32 s32VdecChn, MI_S32 s32DivpChn)
{
    ST_ModuleUnBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                    E_MI_MODULE_ID_DISP, 0, 0, 0); //DIVP->DISP
    ST_DestroyVdecChannel(s32VdecChn);

    return MI_SUCCESS;
}

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

static MI_BOOL disp_ut_001(MI_U8 u8LayerId, MI_U8 u8PortId) //single window
{
    MI_DISP_DEV DispDev = 0;
    MI_SYS_ChnPort_t stSysChnPort;

    MI_SYS_Init();

    if(_gastDispUtDev[DispDev].stPubAttr.eIntfType == E_MI_DISP_INTF_LCD)
    {
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
    //set layer
    disp_ut_setlayer(DispDev, u8LayerId);
    //set inputport
    disp_ut_setport(u8LayerId, u8PortId);

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

    //create put buff thread
    stSysChnPort.eModId = E_MI_MODULE_ID_DISP;
    stSysChnPort.u32DevId = DispDev;
    stSysChnPort.u32ChnId = (u8LayerId == 0)?u8PortId:16;
    stSysChnPort.u32PortId = 0;

    strcpy(gastDispTestPutBufThread[u8LayerId][u8PortId].FileName, _gastDispUtPort[u8LayerId][u8PortId].FilePath);
    gastDispTestPutBufThread[u8LayerId][u8PortId].bRunFlag = TRUE;
    gastDispTestPutBufThread[u8LayerId][u8PortId].DispLayer = u8LayerId;
    gastDispTestPutBufThread[u8LayerId][u8PortId].u32PortId= u8PortId;
    gastDispTestPutBufThread[u8LayerId][u8PortId].u16BuffWidth = _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.u16SrcWidth;
    gastDispTestPutBufThread[u8LayerId][u8PortId].u16BuffHeight = _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.u16SrcHeight;
    gastDispTestPutBufThread[u8LayerId][u8PortId].ePixelFormat = _gastDispUtPort[u8LayerId][u8PortId].ePixFormat;
    memcpy(&gastDispTestPutBufThread[u8LayerId][u8PortId].stSysChnPort, &stSysChnPort, sizeof(MI_SYS_ChnPort_t));

   // pthread_create(&gastDispTestPutBufThread[u8LayerId][u8PortId].pt, NULL, disp_test_PutBuffer, &gastDispTestPutBufThread[u8LayerId][u8PortId]);

    return MI_SUCCESS;
}

int main(int argc, char **argv)
{
    MI_BOOL ret;
    int opt;
    char filepath[256];
    char buf[50];
    fd_set stFdSet;
    MI_DISP_Interface_e eInterface = E_MI_DISP_INTF_LCD;
    MI_DISP_OutputTiming_e eOutputTiming = E_MI_DISP_OUTPUT_USER;
    MI_DISP_RotateMode_e eRotateMode = E_MI_DISP_ROTATE_NONE;
    MI_SYS_PixelFormat_e ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    MI_PANEL_LinkType_e eLinkType;
    MI_U8 u8LayerId = 0;
    MI_U8 u8PortId = 0;
    MI_U32 u32PortWidth;
    MI_U32 u32PortHeight;
    MI_U32 u32PortOut_x = 0;
    MI_U32 u32PortOut_y = 0;
    MI_U32 u32PortOutWidth = 0;
    MI_U32 u32PortOutHeight = 0;
    MI_U32 u32LayerDispWidth = 0;
    MI_U32 u32LayerDispHeight = 0;
    MI_U8 u8DevIndex = 0;
    MI_U8 u8LayerIndex = 0;
    MI_U8 u8PortIndex = 0;

    memset(_gastDispUtDev, 0, sizeof(_gastDispUtDev));
    memset(_gastDispUtLayer, 0, sizeof(_gastDispUtLayer));
    memset(_gastDispUtPort, 0, sizeof(_gastDispUtPort));

    while ((opt = getopt(argc, argv, "f:w:h:o:")) != -1) {
        switch(opt){
            case 'f':
                strcpy(g_filepath,optarg);
                break;
            case 'w':
                u32PortWidth = atoi(optarg);
                break;
            case 'h':
                u32PortHeight = atoi(optarg);
                break;
            case 'o':
                if(strcmp("ttl",optarg) == 0)
                    eLinkType = E_MI_PNL_LINK_TTL;
                else if(strcmp("mipi",optarg) == 0)
                    eLinkType = E_MI_PNL_LINK_MIPI_DSI;
                break;
            default:
                fprintf(stderr, "Usage: %s [-f filepath] [-w image_width] [-h image_height]\n",argv[0]);
        }
    }
    printf("argc=%d; FilePath=%d; width=%d; eLinkType=%d;\n", argc, filepath, u32PortWidth, u32PortHeight, eLinkType);

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
    }

    GetLayerDisplaySize(eOutputTiming, &u32LayerDispWidth, &u32LayerDispHeight);
    u32PortOut_x = (u32LayerDispWidth-u32PortWidth)/2;
    u32PortOut_y = (u32LayerDispHeight-u32PortHeight)/2;
    u32PortOutWidth = u32PortWidth;
    u32PortOutHeight = u32PortHeight;

    _gastDispUtDev[0].stPubAttr.eIntfSync = eOutputTiming;
    _gastDispUtDev[0].stPubAttr.eIntfType = eInterface;
    _gastDispUtDev[0].eLinkType = eLinkType;
    _gastDispUtPort[u8LayerId][u8PortId].stCropWin.u16X = 0;
    _gastDispUtPort[u8LayerId][u8PortId].stCropWin.u16Y = 0;
    _gastDispUtPort[u8LayerId][u8PortId].stCropWin.u16Width = u32PortWidth;
    _gastDispUtPort[u8LayerId][u8PortId].stCropWin.u16Height = u32PortHeight;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.u16SrcWidth = u32PortWidth;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.u16SrcHeight = u32PortHeight;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.stDispWin.u16X = u32PortOut_x;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.stDispWin.u16Y = u32PortOut_y;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.stDispWin.u16Width = u32PortOutWidth;
    _gastDispUtPort[u8LayerId][u8PortId].stInputPortAttr.stDispWin.u16Height = u32PortOutHeight;
    _gastDispUtPort[u8LayerId][u8PortId].ePixFormat = ePixelFormat;
    ret = disp_ut_001(u8LayerId, u8PortId);
    ST_CreateVdec2DispPipe(0, 0, 1920, 1080, E_MI_VDEC_CODEC_TYPE_H264, u32PortWidth, u32PortHeight);
    g_VdecRun = TRUE;
    pthread_create(&g_VdeStream_tid, NULL, ST_VdecSendStream, NULL);

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
    g_VdecRun = FALSE;
    pthread_join(g_VdeStream_tid, NULL);
    ST_DestroyVdec2DispPipe(0, 0);
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
                            g_PushDataExit[u8LayerIndex][u8PortIndex] = TRUE;
                            if(gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt)
                            {
                                pthread_join(gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt,NULL);
                                gastDispTestPutBufThread[u8LayerIndex][u8PortIndex].pt = 0;
                            }
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
    MI_SYS_Exit();

    printf("--------EXIT--------\n");
    return 0;
}

