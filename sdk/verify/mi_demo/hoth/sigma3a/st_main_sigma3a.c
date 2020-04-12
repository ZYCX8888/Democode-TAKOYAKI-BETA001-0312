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
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

//#include "../sigma3a/isp_3a_if.h"
#include "mi_sys.h"
#include "mi_vpe.h"
//#include "mi_venc.h"
//#include "mi_alsa.h"
//#include "mi_aio.h"
//#include "mi_aio_datatype.h"
//#include "st_uvc.h" //infinity5
#include "st_common.h"
//#include "st_hdmi.h" //infinity5
//#include "st_disp.h" //infinity5
#include "st_vpe.h"
//#include "st_vif.h"
//#include "st_fb.h" //infinity5
//#include "st_warp.h" //infinity5
//#include "st_sd.h" //infinity5
//#include "list.h" //infinity5
//#include "mi_rgn.h"
//#include "tem.h" //infinity5
#include "mi_isp.h"
#include "mi_vpe.h"
#include "isp_cus3a_if.h"
#include "sigma_isp_general.h"
#include "isp_sigma3a_ext.h"
#include "isp_3a_if.h"

#define UVC_SUPPORT_MMAP
//#define UVC_SUPPORT_LL
#ifndef UVC_SUPPORT_MMAP
    #define UVC_MEMORY_MODE UVC_MEMORY_USERPTR
#else
    #define UVC_MEMORY_MODE UVC_MEMORY_MMAP
#endif
//#define SUPPORT_WRITE_FILE
#define ENABLE_PUTES_TO_UVC 0
#define RD_OR_WR 1
#define ENABLE_DUMPCIF_PORT1 0

#define UVC_FUNCTION_EN (0) //infinity5


typedef enum
{
    E_UVC_TIMMING_4K2K_JPG,
    E_UVC_TIMMING_2560X1440P_JPG,
    E_UVC_TIMMING_1920X1080P_JPG,
    E_UVC_TIMMING_1280X720P_JPG,
    E_UVC_TIMMING_640X480P_JPG,
    E_UVC_TIMMING_320X240P_JPG,
    E_UVC_TIMMING_4K2K_H264,
    E_UVC_TIMMING_2560X1440P_H264,
    E_UVC_TIMMING_1920X1080P_H264,
    E_UVC_TIMMING_1280X720P_H264,
    E_UVC_TIMMING_640X480P_H264,
    E_UVC_TIMMING_320X240P_H264,
    E_UVC_TIMMING_4K2K_H265,
    E_UVC_TIMMING_2560X1440P_H265,
    E_UVC_TIMMING_1920X1080P_H265,
    E_UVC_TIMMING_1280X720P_H265,
    E_UVC_TIMMING_640X480P_H265,
    E_UVC_TIMMING_320X240P_H265,
    E_UVC_TIMMING_1920X1080_NV12,
    E_UVC_TIMMING_1280X720_NV12,
    E_UVC_TIMMING_640X480_NV12,
    E_UVC_TIMMING_320X240_NV12,
    E_UVC_TIMMING_1920X1080_YUV422_YUYV,
    E_UVC_TIMMING_1280X720_YUV422_YUYV,
    E_UVC_TIMMING_640X480_YUV422_YUYV,
    E_UVC_TIMMING_320X240_YUV422_YUYV,
} UVC_FormatTimming_e;

typedef struct ST_ModuleState_s
{
    MI_BOOL bEnableVpe;
    MI_BOOL bEnableWarp;
    MI_BOOL bEnableCevaVx;
    MI_BOOL bEnableSd;
    MI_BOOL bEnableVenc;
} ST_ModuleState_t;

extern MI_U32 Mif_Syscfg_GetTime0();
static MI_U32 curtime[5];
static MI_U32 u32BufSize = 0;;
static pthread_mutex_t _gTime = PTHREAD_MUTEX_INITIALIZER;

static MI_S32 St_BaseModuleDeinit(void)
{

    return MI_SUCCESS;
}
static void do_fix_mantis(int mantis)
{
    printf("To fix mantis 1574080!\n");
    return;
}

unsigned int poll_isp(int fd, int timeout)
{
    struct pollfd pfd;
    int ret = 0;
    unsigned int flag = 0;

    pfd.fd = fd;
    pfd.events = POLLIN/* | POLLRDNORM*/;

    ret = poll(&pfd, 1, timeout);
    if(ret > 0)
    {
        read(fd, (void*)&flag, sizeof(flag));
    }
    return flag;
}

static void test_mi_api()
{
    FILE *fd;
    MI_ISP_AE_HW_STATISTICS_t *pAe;
    MI_ISP_AWB_HW_STATISTICS_t *pAwb;
    MI_ISP_HISTO_HW_STATISTICS_t *pHisto;
    Cus3AEnable_t *pCus3AEnable;
    CusAEInitParam_t *pAeInitParam;
    CusAEInfo_t *pAeInfo;
    CusAEResult_t *pAeResult;
    CusAWBInfo_t *pAwbInfo;
    CusAWBResult_t *pAwbResult;

    /*AE avg statistics*/
    pAe = malloc(sizeof(MI_ISP_AE_HW_STATISTICS_t));
    MI_ISP_AE_GetAeHwAvgStats(0, pAe);
    fd = fopen("ae.bin", "w+b");
    fwrite(pAe->nAvg, sizeof(pAe->nAvg), 1, fd);
    fclose(fd);
    printf("Save ae data to file ae.bin\n");
    free(pAe);

    /*AWB avg statistics*/
    pAwb = malloc(sizeof(MI_ISP_AWB_HW_STATISTICS_t));
    MI_ISP_AWB_GetAwbHwAvgStats(0, pAwb);
    fd = fopen("awb.bin", "w+b");
    fwrite(pAwb->nAvg, sizeof(pAwb->nAvg), 1, fd);
    fclose(fd);
    printf("Save awb data to file ae.bin\n");
    free(pAwb);

    /*Histo0 avg statistics*/
    pHisto = malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto0HwStats(0, pHisto);
    fd = fopen("histo0.bin", "w+b");
    fwrite(pHisto->nHisto, sizeof(pHisto->nHisto), 1, fd);
    fclose(fd);
    printf("Save histo0 data to file ae.bin\n");
    free(pHisto);

    /*Histo1 avg statistics*/
    pHisto = malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
    MI_ISP_AE_GetHisto1HwStats(0, pHisto);
    fd = fopen("histo1.bin", "w+b");
    fwrite(pHisto->nHisto, sizeof(pHisto->nHisto), 1, fd);
    fclose(fd);
    printf("Save histo1 data to file ae.bin\n");
    free(pHisto);

    /*Check CUS3A*/
    pCus3AEnable = malloc(sizeof(Cus3AEnable_t));
    pCus3AEnable->bAE = 1;
    pCus3AEnable->bAWB = 1;
    pCus3AEnable->bAF = 0;
    MI_ISP_CUS3A_Enable(0, pCus3AEnable);
    free(pCus3AEnable);

    /*Check AE init param*/
    pAeInitParam = malloc(sizeof(CusAEInitParam_t));
    memset(pAeInitParam, 0, sizeof(CusAEInitParam_t));
    MI_ISP_CUS3A_GetAeInitStatus(0, pAeInitParam);
    printf("AeInitParam ,shutter=%d,shutter_step=%d,sensor_gain_min=%d,sensor_gain_max=%d\n",
           pAeInitParam->shutter,
           pAeInitParam->shutter_step,
           pAeInitParam->sensor_gain_min,
           pAeInitParam->sensor_gain_max
          );
    free(pAeInitParam);

    /*Check AE param*/
    pAeInfo = malloc(sizeof(CusAEInfo_t));
    memset(pAeInfo, 0, sizeof(CusAEInfo_t));
    MI_ISP_CUS3A_GetAeStatus(0, pAeInfo);
    printf("AeInitParam ,Shutter=%d,SensirGain=%d,IspGain=%d\n",
           pAeInfo->Shutter,
           pAeInfo->SensorGain,
           pAeInfo->IspGain
          );
    free(pAeInfo);

    /*Check AWB param*/
    pAwbInfo = malloc(sizeof(CusAWBInfo_t));
    memset(pAwbInfo, 0, sizeof(CusAWBInfo_t));
    MI_ISP_CUS3A_GetAwbStatus(0, pAwbInfo);
    printf("AwbInitParam ,Rgain=%d,Ggain=%d,Bgain=%d\n",
           pAwbInfo->CurRGain,
           pAwbInfo->CurGGain,
           pAwbInfo->CurBGain
          );
    free(pAwbInfo);

    /*Set AWB param*/
    pAwbResult = malloc(sizeof(CusAWBResult_t));
    memset(pAwbResult, 0, sizeof(CusAWBResult_t));
    pAwbResult->Size = sizeof(CusAWBResult_t);
    pAwbResult->R_gain = 4096;
    pAwbResult->G_gain = 1024;
    pAwbResult->B_gain = 1024;
    MI_ISP_CUS3A_SetAwbParam(0, pAwbResult);
    free(pAwbResult);

#if 1
    int isp_fe = open("/dev/isp_fe", O_RDWR);
    int n = 0;
    /*Check AE init param*/
    pAeResult = malloc(sizeof(CusAEResult_t));
    for(n = 0; n < 120; ++n)
    {
        unsigned int status = poll_isp(isp_fe, 500);
        //printf("ISP CH%d FE\n", status);
        memset(pAeResult, 0, sizeof(CusAEResult_t));
        pAeResult->Size = sizeof(CusAEResult_t);
        pAeResult->Change = 1;
        pAeResult->u4BVx16384 = 16384;
        pAeResult->HdrRatio = 10;
        pAeResult->ShutterHdrShort = 300;
        pAeResult->Shutter = (300 * n) % 30000;
        pAeResult->IspGain = 2048;
        pAeResult->SensorGain = 1024;
        pAeResult->SensorGainHdrShort = 4096;
        pAeResult->IspGainHdrShort = 1024;
        pAeResult->AvgY = 128;
        MI_ISP_CUS3A_SetAeParam(0, pAeResult);
    }
    free(pAeResult);
    close(isp_fe);
#endif
    pCus3AEnable = malloc(sizeof(Cus3AEnable_t));
    pCus3AEnable->bAE = 0;
    pCus3AEnable->bAWB = 0;
    pCus3AEnable->bAF = 0;
    MI_ISP_CUS3A_Enable(0, pCus3AEnable);
    free(pCus3AEnable);

    /*Unregister api agent*/
    //MI_ISP_RegisterIspApiAgent(0, NULL, NULL);
}

//*****************************************************************************************************/
// Customer 3A
//*****************************************************************************************************/
#define USING_NEW_AE_ALGO    (1)    // 0: using simple AE algorithm, 1: using complex AE algorithm
#define USING_NEW_AWB_ALGO    (1)    // 0: using simple AE algorithm, 1: using complex AE algorithm
#define AE_RUN_WITH_USE_CASE_1    (0)  // Implement ae algorithm function only in ae_run() callback function, don't need to create new thread
#define AE_RUN_WITH_USE_CASE_2    (1)  // Create new thread to run ae algorithm, get AvgY and get result, then send result to ae_run() callback function
#define AE_RUN_WITH_USE_CASE_3    (2)  // Create new thread to run ae algorithm, get AvgY, get result and set AE result in same thread, don't need to register ae_run() callback function
#define AE_RUN_WITH_USE_CASE    (AE_RUN_WITH_USE_CASE_1)

#define ENABLE_DOAE_MSG        (1)
#define ENABLE_DOAWB_MSG    (0)
#define ENABLE_DOAF_MSG        (0)

#define SHUTTER_GAIN_DELAY_TEST1   (0)
#define SHUTTER_GAIN_DELAY_TEST2   (0)
#define SHUTTER_TEST                               (0)
#define GAIN_TEST                                      (0)
#define AE_EXAMPLE                                   (1)

typedef struct
{
    AeHandle    hAe;
    AeOutput_t  tAeOutput;
    AEDEBUG_t   tAeDbg;
} CusAeInfo_t;

typedef struct
{
    AwbHandle   hAwb;
    AwbOutput_t tAwbOutput;
    AWBDEBUG_t  tAwbDbg;
} CusAwbInfo_t;

typedef struct
{
    CusAeInfo_t tCusAeInfo;
    CusAwbInfo_t tCusAwbInfo;
} Cus3A_Priv_Data_t;

Cus3A_Priv_Data_t g_tCus3APrivData[4]; //max 4 isp channel
int ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    AeInitParam_t tAeInit;
    AeInput_t tAeInput;

    if(pdata == NULL)
    {
        printf("ae_init error!!! NULL Pointer!!!\n");
        return -1;
    }
    printf("[ae_init] isp_gain_max = 0x%x, sgl_min = 0x%x, sgl_max = 0x%x, sgs_min  0x%x, sgs_max = 0x%x\n",
           (int)init_state->isp_gain_max,
           (int)init_state->sensor_gain_min,
           (int)init_state->sensor_gain_max,
           (int)init_state->sensor_gainHDRShort_min,
           (int)init_state->sensor_gainHDRShort_max
          );
    printf("[ae_init], shl_min = %d us, shl_max = %d us, shl_step = %d us, shs_min = %d us, shs_max = %d us, shs_step = %d us\n",
           (int)init_state->shutter_min,
           (int)init_state->shutter_max,
           (int)init_state->shutter_step,
           (int)init_state->shutterHDRShort_min,
           (int)init_state->shutterHDRShort_max,
           (int)init_state->shutterHDRShort_step
          );

    pAe = &pstCus3a->tCusAeInfo;
    tAeInit.FNx10           = init_state->FNx10;
    tAeInit.Size            = sizeof(AeInitParam_t);
    tAeInit.fps             = init_state->fps;
    tAeInit.isp_gain        = init_state->isp_gain;
    tAeInit.isp_gain_max    = init_state->isp_gain_max;

    tAeInit.sensor_gain     = init_state->sensor_gain;
    tAeInit.sensor_gain_min = 1 * 1024;          // 1024 * 1
    tAeInit.sensor_gain_max = init_state->sensor_gain_max;          // 1024 * 177
    tAeInit.sensor_id[0]    = init_state->sensor_id[0];
    tAeInit.shutter         = init_state->shutter;
    tAeInit.shutter_min     = init_state->shutter_min;    // 14814 * 2  //conversion driver's ns to AE's us
    tAeInit.shutter_max     = init_state->shutter_max;    // 1000000000 / 3   //conversion driver's ns to AE's us
    tAeInit.shutter_step    = init_state->shutter_step;     // 14814   //conversion driver's ns to AE's ns

    // Init AE DEBUG Setting.
    pAe->tAeDbg.AE_DEBUG_ENABLE_AE = 1;
    pAe->tAeDbg.AE_DEBUG_LEVEL = 0;

    tAeInput.tAEDebug = &pAe->tAeDbg;

    /*CUS3A v1.1*/
    tAeInput.AvgBlkX = init_state->AvgBlkX;
    tAeInput.AvgBlkY = init_state->AvgBlkY;

    // Init. AE Output parameters.
    pAe->tAeOutput.FNx10         = 18;
    pAe->tAeOutput.CurYx10       = 100;
    pAe->tAeOutput.Shutter       = 300;
    pAe->tAeOutput.SensorGain    = 1024;
    pAe->tAeOutput.IspGain       = 1024;
    pAe->tAeOutput.ShutterHDR    = 30;
    pAe->tAeOutput.SensorGainHDR = 1024;
    pAe->tAeOutput.IspGainHDR    = 1024;

    pstCus3a->tCusAeInfo.hAe = AeInit(NULL, &tAeInit, &tAeInput, &pAe->tAeOutput);

    return 0;
}

void ae_release(void* pdata)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;

    if(pdata == NULL)
    {
        printf("ae_release error!!! NULL Pointer!!!\n");
        return;
    }

    AeRelease(pstCus3a->tCusAeInfo.hAe);
    printf("************* ae_release *************\n");
}

void ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    AeInput_t  tAeInput;
    AeInput_t *ptAeInput;
    static int period = 1;
    static unsigned int fcount = 0;

    //printf("AE run\n");

    if(pdata == NULL || info == NULL)
    {
        printf("ae_run error!!! NULL Pointer!!! pdata=0x%X, info=0x%X\n", (u32) pdata, (u32) info);
        return;
    }

    pAe = &pstCus3a->tCusAeInfo;
    result->Size              = sizeof(ISP_AE_RESULT);
    result->Change             = 0;
    result->u4BVx16384         = pAe->tAeOutput.i4BVx16384;
    result->HdrRatio           = 0;      /*TODO: ??*/
    result->Shutter            = pAe->tAeOutput.Shutter;
    result->SensorGain         = pAe->tAeOutput.SensorGain;
    result->IspGain            = pAe->tAeOutput.IspGain;
    result->ShutterHdrShort    = pAe->tAeOutput.ShutterHDR;
    result->SensorGainHdrShort = pAe->tAeOutput.SensorGainHDR;
    result->IspGainHdrShort    = pAe->tAeOutput.IspGainHDR;
    result->AvgY               = pAe->tAeOutput.CurYx10 / 10;

    if(++fcount % period == 0)
    {
        ptAeInput = &tAeInput;
        ptAeInput->AvgBlkX        = info->AvgBlkX;
        ptAeInput->AvgBlkY        = info->AvgBlkY;
        ptAeInput->PreAvgYx10     = info->PreAvgY*10;
        ptAeInput->Shutter        = info->Shutter;
        ptAeInput->SensorGain     = info->SensorGain;
        ptAeInput->IspGain        = info->IspGain;
        ptAeInput->ShutterHDR     = info->ShutterHDRShort;
        ptAeInput->SensorGainHDR  = info->SensorGainHDRShort;
        ptAeInput->IspGainHDR     = info->IspGainHDRShort;

        ptAeInput->Size           = sizeof(AeInput_t);
        ptAeInput->avgs           = (ISPAESample_t *)info->avgs;
        ptAeInput->avgs1          = NULL;
        ptAeInput->hist1          = (ISPHistX_t *)info->hist1;
        ptAeInput->hist2          = (ISPHistX_t *)info->hist2;
        ptAeInput->tAEDebug       = &pAe->tAeDbg;

        /*CUS3A V1.1*/
        ptAeInput->PreCurYx10     = pAe->tAeOutput.CurYx10;//info->PreWeightY*10;/**< Previous frame brightness*/
        ptAeInput->HDRMode        = info->HDRCtlMode;/**< 0 = Separate shutter/sensor gain settings; */
                                                  /**< 1 = Separate shutter & Share sensor gain settings */
        ptAeInput->CurFPS         = info->CurFPS; /**Current sensor FPS */
        ptAeInput->FNx10          = info->FNx10;

#if 0
        printf("Blk X=%d, Y=%d, PreWeightY=%d, PreAvgY=%d, HdrCtlMode=%d, FN=%d\n",
                info->AvgBlkX,info->AvgBlkY,
                info->PreWeightY,info->PreAvgY,
                info->HDRCtlMode,
                info->FNx10
                );
#endif

        /*Customer AE*/
        DoAe(pAe->hAe, ptAeInput, &pAe->tAeOutput);

        result->Size         = sizeof(ISP_AE_RESULT);
        result->Change       = pAe->tAeOutput.Change;
        result->u4BVx16384   = pAe->tAeOutput.i4BVx16384;
        result->HdrRatio     = 0;      /*TODO: ??*/
        result->ShutterHdrShort = pAe->tAeOutput.ShutterHDR;
        result->Shutter      = pAe->tAeOutput.Shutter;
        result->IspGain      = pAe->tAeOutput.IspGain;
        result->SensorGain   = pAe->tAeOutput.SensorGain;
        result->SensorGainHdrShort = pAe->tAeOutput.SensorGainHDR;
        result->IspGainHdrShort = pAe->tAeOutput.IspGainHDR;
        result->AvgY         = pAe->tAeOutput.AvgYx10 / 10;

        /*CUS3A V1.1*/
        result->FNx10        = pAe->tAeOutput.FNx10;    /**< F number * 10*/
        result->DebandFPS    = pAe->tAeOutput.DebandFPS;/** Target fps when running auto debanding**/
        result->WeightY      = pAe->tAeOutput.CurY1x10 / 10;
#if 0
        {
            int x=0,y=0;
            unsigned int avg = 0;
            //printf("avg buf = 0x%X, BlkX=%d BlkY=%d\n", info->avgs, info->AvgBlkX, info->AvgBlkY);
            for(y=0;y<info->AvgBlkY;++y)
            {
                for(x=0;x<info->AvgBlkX;++x)
                {
                    avg += info->avgs[y*info->AvgBlkX + x].y;
                }
            }
            printf("Frame-%d AE avg=%u\n",avg/(info->AvgBlkX*info->AvgBlkY));
        }

        if(fcount%30==0)
        {
            result->IspGain      = 1024;
            result->SensorGain   = 4096;
            result->Shutter      = 30000;
        }
#endif

#if (ENABLE_DOAE_MSG)
        printf("DoAE--> Change:%02u, AvgY:%06u, u4BVx16384:%06d, IspGain:%06u, sensor gain:%06u, Shutter:%06u\n",
               (int)result->Change,
               (int)result->AvgY,
               (int)result->u4BVx16384,
               (int)result->IspGain,
               (int)result->SensorGain,
               (int)result->Shutter);
#endif
    }
}

int ae_ctrl(void *pdata, ISP_AE_CTRL_CMD cmd, void* param)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    MI_CUS3A_CtrlCmd_t *pstCus3aCtrlCmd = (MI_CUS3A_CtrlCmd_t *)param;

    if(pdata == NULL)
    {
        printf("ae_ctrl pdata error!!! NULL Pointer!!!\n");
        return -1;
    }

    if(param == NULL)
    {
        printf("ae_ctrl param error!!! NULL Pointer!!!\n");
        return -1;
    }

    pAe = &pstCus3a->tCusAeInfo;

    printf("[ae_ctrl] CtrlID = %d, Dir = %d\n", (int)cmd, pstCus3aCtrlCmd->u32Dir);
    if(pstCus3aCtrlCmd->u32Dir)
    {
        pstCus3aCtrlCmd->stApiHeader.s32Ret = DrvAlgo_IF_ApiGet(cmd, pAe->hAe, NULL, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (void*)pstCus3aCtrlCmd->pData);
    }
    else
    {
        pstCus3aCtrlCmd->stApiHeader.s32Ret = DrvAlgo_IF_ApiSet(cmd, pAe->hAe, NULL, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (void*)pstCus3aCtrlCmd->pData);
    }

    return 0;
}

int awb_init(void *pdata)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    AwbInput_t tAwbInput;

    pstCus3a->tCusAwbInfo.tAwbDbg.AWB_DEBUG_AWB_ENABLE   = 1;
    pstCus3a->tCusAwbInfo.tAwbDbg.AWB_DEBUG_LEVEL        = 0;
    tAwbInput.tAWBDebug = &pstCus3a->tCusAwbInfo.tAwbDbg;
    // Run AWB_Init function
    pstCus3a->tCusAwbInfo.hAwb = AwbInit(NULL, &tAwbInput, &pstCus3a->tCusAwbInfo.tAwbOutput);
    printf("************ awb_init **********\n");
    return 0;
}

void awb_release(void *pdata)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;

    if(pdata == NULL)
    {
        printf("awb_release error!!! NULL Pointer!!!\n");
        return;
    }

    AwbRelease(pstCus3a->tCusAwbInfo.hAwb);
    printf("************ awb_release **********\n");
}

void awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAeInfo_t *pAe;
    CusAwbInfo_t *pAwb;
    AwbInput_t  tAwbInput;
    AwbInput_t  *ptAwbInput;
    static int period = 3;
    static unsigned int fcount = 0;

    pAe = &pstCus3a->tCusAeInfo;
    pAwb = &pstCus3a->tCusAwbInfo;

    result->Size = sizeof(ISP_AWB_RESULT);
    result->Change = 0;
    result->ColorTmp = pAwb->tAwbOutput.nColorTmp;
    result->R_gain = pAwb->tAwbOutput.tGain.nRgain;
    result->G_gain = pAwb->tAwbOutput.tGain.nGrgain;
    result->B_gain = pAwb->tAwbOutput.tGain.nBgain;

    if(fcount++ % period == 0)
    {
        ptAwbInput = &tAwbInput;
        ptAwbInput->nSize                     = sizeof(AwbInput_t);
        ptAwbInput->nBV                       = pAe->tAeOutput.i4BVx16384;
        ptAwbInput->CurYx10                   = pAe->tAeOutput.CurYx10;
        ptAwbInput->tBlock.u2BlkNum_x         = AWB_WIN_MAX_WIDTH;
        ptAwbInput->tBlock.u2BlkNum_y         = AWB_WIN_MAX_HEIGHT;
        ptAwbInput->tBlock.u2BlkVaild_x       = AWB_BLOCK_MIN_VALID_X;
        ptAwbInput->tBlock.u2BlkVaild_y       = AWB_BLOCK_MIN_VALID_Y;
        ptAwbInput->tBlock.u2BlkValidLowLux_x = 10;
        ptAwbInput->tBlock.u2BlkValidLowLux_y = 8;
        ptAwbInput->tCrop.x_start             = 1;
        ptAwbInput->tCrop.y_start             = 1;
        ptAwbInput->tCrop.width               = 1920;
        ptAwbInput->tCrop.height              = 1080;
        ptAwbInput->pAwbStatis                = (IspAwbStatis_t*)info->avgs;
        ptAwbInput->tAWBDebug                 = &pAwb->tAwbDbg;
        ptAwbInput->nFrameCount               = 0;
        ptAwbInput->nColorToGrayFlag          = 0;
        ptAwbInput->nGrayToColorFlag          = 0;

        ptAwbInput->HDRMode = AWB_MODE_NORMAL;  //TODO : get current mode from kernel
        ptAwbInput->pAwbStatisShort = 0;    //TODO: get statis from kernel

        ptAwbInput->tCrop.width = 1920; //TODO : read from kernel
        ptAwbInput->tCrop.height = 1080; //TODO : read from kernel

        ptAwbInput->nFrameCount = fcount; //TODO : read from kernel
        ptAwbInput->nColorToGrayFlag = 0; //TODO : read from kernel IQ
        ptAwbInput->nColorToGrayFlag = 0; //TODO : read from kernel IQ

        /*CUS3A V1.1*/
        ptAwbInput->HDRMode          = info->HDRMode; /**< Noramal or HDR mode*/
        ptAwbInput->pAwbStatisShort  = info->pAwbStatisShort; /**<awb statis for HDR short Shutter AWB statistic data */
        ptAwbInput->nBV              = info->u4BVx16384;      /**< From AE output, Bv * 16384 in APEX system, EV = Av + Tv = Sv + Bv */
        ptAwbInput->CurYx10          = info->WeightY;
        printf("DoAWB, WeightY=%d, Bv=%d \n",ptAwbInput->CurYx10,ptAwbInput->nBV);
        DoAwb(pAwb->hAwb, ptAwbInput, &pAwb->tAwbOutput);

        result->Size = sizeof(ISP_AWB_RESULT);
        result->Change = 1;
        result->ColorTmp = pAwb->tAwbOutput.nColorTmp;
        result->R_gain = pAwb->tAwbOutput.tGain.nRgain;
        result->G_gain = pAwb->tAwbOutput.tGain.nGrgain;
        result->B_gain = pAwb->tAwbOutput.tGain.nBgain;

#if (ENABLE_DOAWB_MSG)
        printf("=== DoAWB ===\n");
        printf("RGain         :   %04hu\n", pAwb->tAwbOutput.tGain.nRgain);
        printf("GrGain        :   %04hu\n", pAwb->tAwbOutput.tGain.nGrgain);
        printf("GbGain        :   %04hu\n", pAwb->tAwbOutput.tGain.nGbgain);
        printf("BGain         :   %04hu\n", pAwb->tAwbOutput.tGain.nBgain);
        printf("nColorTmp     :  %05hu\n", pAwb->tAwbOutput.nColorTmp);
#endif
    }
}

int awb_ctrl(void *pdata, ISP_AE_CTRL_CMD cmd, void* param)
{
    Cus3A_Priv_Data_t *pstCus3a = (Cus3A_Priv_Data_t *)pdata;
    CusAwbInfo_t *pAwb;
    MI_CUS3A_CtrlCmd_t *pstCus3aCtrlCmd = (MI_CUS3A_CtrlCmd_t *)param;

    if(pdata == NULL)
    {
        printf("awb_ctrl pdata error!!! NULL Pointer!!!\n");
        return -1;
    }

    if(param == NULL)
    {
        printf("awb_ctrl param error!!! NULL Pointer!!!\n");
        return -1;
    }

    pAwb = &pstCus3a->tCusAwbInfo;

    printf("[awb_ctrl] CtrlID = %d, Dir = %d\n", (int)cmd, pstCus3aCtrlCmd->u32Dir);
    if(pstCus3aCtrlCmd->u32Dir)
    {
        pstCus3aCtrlCmd->stApiHeader.s32Ret = DrvAlgo_IF_ApiGet(cmd, NULL, pAwb->hAwb, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (void*)pstCus3aCtrlCmd->pData);
    }
    else
    {
        pstCus3aCtrlCmd->stApiHeader.s32Ret = DrvAlgo_IF_ApiSet(cmd, NULL, pAwb->hAwb, NULL, NULL, pstCus3aCtrlCmd->stApiHeader.u32DataLen, (const void*)pstCus3aCtrlCmd->pData);
    }

    return 0;
}

int af_init(void *pdata, ISP_AF_INIT_PARAM *param)
{
    MI_U32 u32ch = 0;
    MI_U8 u8win_idx = 16;
    CusAFRoiMode_t taf_roimode;

    printf("************ af_init **********\n");

    //Init Normal mode setting
    taf_roimode.mode = AF_ROI_MODE_NORMAL;
    taf_roimode.u32_vertical_block_number = 1;
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {
        { 0, {   0,    0,  255,  255}},
        { 1, { 256,    0,  511,  255}},
        { 2, { 512,    0,  767,  255}},
        { 3, { 768,    0, 1023,  255}},
        { 4, {   0,  256,  255,  511}},
        { 5, { 256,  256,  511,  511}},
        { 6, { 512,  256,  767,  511}},
        { 7, { 768,  256, 1023,  511}},
        { 8, {   0,  512,  255,  767}},
        { 9, { 256,  512,  511,  767}},
        {10, { 512,  512,  767,  767}},
        {11, { 768,  512, 1023,  767}},
        {12, {   0,  768,  255, 1023}},
        {13, { 256,  768,  511, 1023}},
        {14, { 512,  768,  767, 1023}},
        {15, { 768,  768, 1023, 1023}}
    };
    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }

    static CusAFFilter_t affilter =
    {
        //filter setting with sign value
        //{63, -126, 63, -109, 48, 0, 320, 0, 1023},
        //{63, -126, 63, 65, 55, 0, 320, 0, 1023}

        //convert to hw format (sign bit with msb)
        63, 126 + 1024, 63, 109 + 128, 48, 0, 320, 0, 1023,
        63, 126 + 1024, 63, 65, 55, 0, 320, 0, 1023,

    };

    MI_ISP_CUS3A_SetAFFilter(0, &affilter);
    return 0;
}

void af_release(void *pdata)
{
    printf("************ af_release **********\n");
}

void af_run(void *pdata, const ISP_AF_INFO *af_info, ISP_AF_RESULT *result)
{
#if (ENABLE_DOAF_MSG)
    int i = 0, x = 0;

    printf("\n\n");

    //print row0 16wins
    x = 0;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }

    //print row15 16wins
    x = 15;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }
#endif
}

int af_ctrl(void *pdata, ISP_AF_CTRL_CMD cmd, void* param)
{
    return 0;
}

#define BUFFER_SIZE_OUT_16BITS(w) ((((w + 7) / 8) * 8) * 2) //unsigned char
#define BUFFER_SIZE_IN_12BITS(w) ((((w + 31) / 32) * 32) * 12 / 8) // (12bits/ pixel) -> (12 / 8) (bytes / pixel)
#define BUFFER_SIZE_IN_10BITS(w) ((((w + 63) / 64) * 64) * 10 / 8)  // (10bits/ pixel) -> (10 / 8) (bytes / pixel)
#define BUFFER_SIZE_IN_8BITS(w) ((((w + 15) / 16) * 16))   // (8bits/ pixel) -> (8 / 8) (bytes / pixel)

void Convert12BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 4) & 0x0ff0) + (((unsigned short)pInbuf[1] << 12) & 0xf000);
    pOutbuf[1] = (((unsigned short)pInbuf[1]) & 0x00f0) + (((unsigned short)pInbuf[2] << 8) & 0xff00);
    pOutbuf[2] = (((unsigned short)pInbuf[3] << 4) & 0x0ff0) + (((unsigned short)pInbuf[4] << 12) & 0xf000);
    pOutbuf[3] = (((unsigned short)pInbuf[4]) & 0x00f0) + (((unsigned short)pInbuf[5] << 8) & 0xff00);
    *pu32InSize = 6;
    *pu32OutSize = 4;
}

void Convert10BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 6) & 0x3fc0) + (((unsigned short)pInbuf[1] << 14) & 0xc000);
    pOutbuf[1] = (((unsigned short)pInbuf[1] << 4) & 0x0fc0) + (((unsigned short)pInbuf[2] << 12) & 0xf000);
    pOutbuf[2] = (((unsigned short)pInbuf[2] << 2) & 0x03c0) + (((unsigned short)pInbuf[3] << 10) & 0xfc00);
    pOutbuf[3] = (((unsigned short)pInbuf[3]) & 0x00c0) + (((unsigned short)pInbuf[4] << 8) & 0xff00);
    *pu32InSize = 5;
    *pu32OutSize = 4;
}

void Convert8BitsTo16Bits(unsigned char *pInbuf, unsigned short *pOutbuf, unsigned long *pu32InSize, unsigned long *pu32OutSize)
{
    pOutbuf[0] = (((unsigned short)pInbuf[0] << 8) & 0xff00);
    pOutbuf[1] = (((unsigned short)pInbuf[1] << 8) & 0xff00);
    pOutbuf[2] = (((unsigned short)pInbuf[2] << 8) & 0xff00);
    pOutbuf[3] = (((unsigned short)pInbuf[3] << 8) & 0xff00);
    *pu32InSize = 4;
    *pu32OutSize = 4;
}

void ConvertOneLineTo16Bits(unsigned char *pbufin, unsigned short *pbufout, unsigned long ulinbufsize, unsigned long uloutbufsize, unsigned short usbitwidth)
{
    unsigned long u32InSize, u32OutSize;
    unsigned long InCnt = 0;
    unsigned long OutCnt = 0;

    memset((void *)pbufout, 0x0, uloutbufsize);

    u32InSize = 0;
    u32OutSize = 0;
    while ((u32InSize < ulinbufsize) && (u32OutSize < uloutbufsize))
    {
        if (usbitwidth == 12)
        {
            Convert12BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        else if (usbitwidth == 10)
        {
            Convert10BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        else if (usbitwidth == 8)
        {
            Convert8BitsTo16Bits(pbufin, pbufout, &InCnt, &OutCnt);
        }
        pbufin += InCnt;
        pbufout += OutCnt;
        u32InSize += InCnt;
        u32OutSize += OutCnt;
    }
}

void ConvertRawImageTo16Bits(void *pBufin, void *pBufout, unsigned long u32Width, unsigned long u32Height, unsigned long u32BitDepth)
{
    unsigned char *pbufin;
    unsigned short *pbufout;
    unsigned long buf_size_in, buf_size_out;
    unsigned long linecnt = 0;
    unsigned short bitwidth = 0;

    if (u32BitDepth == 0)  // 8-bits
    {
        buf_size_in = BUFFER_SIZE_IN_8BITS(u32Width);
        bitwidth = 8;
    }
    else if (u32BitDepth == 1)  // 10 bits
    {
        buf_size_in = BUFFER_SIZE_IN_10BITS(u32Width);
        bitwidth = 10;
    }
    else if (u32BitDepth == 3)  // 12-bits
    {
        buf_size_in = BUFFER_SIZE_IN_12BITS(u32Width);
        bitwidth = 12;
    }
    else
    {
        printf("Error! Don't need to convert image!!!u32BitDepth = %lu\n", u32BitDepth);
        return;
    }

    buf_size_out = (BUFFER_SIZE_OUT_16BITS(u32Width) >> 1);  // by unsigned short

    linecnt = 0;
    pbufin = (unsigned char *)pBufin;
    pbufout = (unsigned short *)pBufout;
    while (linecnt < u32Height)
    {
        ConvertOneLineTo16Bits(pbufin, pbufout, buf_size_in, buf_size_out, bitwidth);
        linecnt++;
        pbufin += buf_size_in;
        pbufout += buf_size_out;
    }
}

MI_S32 main(int argc, char **argv)
{
    char cmd = 0;
    int i = 0;
    u32 nCh = 0;
    if (signal(SIGINT, do_fix_mantis) != 0)
    {
        perror("signal");
        return -1;
    }
    if (signal(SIGTSTP, do_fix_mantis) != 0)
    {
        perror("signal");
        return -1;
    }

    while(1)
    {
        printf("Type \"q\" to exit,\n");
        printf("\"i\" to test mi api,\n");
        printf("\"c\" to start CusAE,\n");
        printf("\"d\" to dump ISP out YUV image\n");
        printf("\"D\" to dump ISP in Bayer raw image\n");
        printf("\"H\" to dump HDR P1 Raw image\n");
        printf("\"j\" pause AE\n");
        printf("\"k\" resume AE\n");
        printf("Type \"r\" to dump IR image\n");

        cmd = getchar();
        if (cmd == 'q')
        {
            /*Register api agent*/
            MI_ISP_RegisterIspApiAgent(0, NULL, NULL);

            CUS3A_RegInterface(0, NULL, NULL, NULL);
            CUS3A_Release();
            break;
        }
        else if(cmd == 'i')
        {
            test_mi_api();
        }
        else if(cmd == 'j')
        {
            MI_ISP_SM_STATE_TYPE_e eData = SS_ISP_STATE_PAUSE;
            MI_ISP_AE_SetState(0, &eData);
        }
        else if(cmd == 'k')
        {
            MI_ISP_SM_STATE_TYPE_e eData = SS_ISP_STATE_NORMAL;
            MI_ISP_AE_SetState(0, &eData);
        }
        else if(cmd == 'c')
        {
            ISP_AE_INTERFACE tAeIf;
            ISP_AWB_INTERFACE tAwbIf;
            ISP_AF_INTERFACE tAfIf;

            CUS3A_Init();

            /*AE*/
            tAeIf.ctrl = ae_ctrl;
            tAeIf.pdata = &g_tCus3APrivData[nCh];
            tAeIf.init = ae_init;
            tAeIf.release = ae_release;
            tAeIf.run = ae_run;

            /*AWB*/
            tAwbIf.ctrl = awb_ctrl;
            tAwbIf.pdata =  &g_tCus3APrivData[nCh];
            tAwbIf.init = awb_init;
            tAwbIf.release = awb_release;
            tAwbIf.run = awb_run;

            /*AF*/
            tAfIf.pdata = NULL;
            tAfIf.init = af_init;
            tAfIf.release = af_release;
            tAfIf.run = af_run;
            tAfIf.ctrl = af_ctrl;
            CUS3A_RegInterface(nCh, &tAeIf, &tAwbIf, &tAfIf);

            /*Register api agent*/
            MI_ISP_RegisterIspApiAgent(0, Cus3A_SetIspApiData, Cus3A_GetIspApiData);
        }
        else if((cmd == 'd') || (cmd == 'D') || (cmd == 'H') || (cmd == 'r'))
        {
            void *pImgAddrSrc, *pImgAddrDst;
            u32  u32BufSizeSrc, u32BufSizeDst;
            u32 u32PhysAddrSrc = 0, u32PhysAddrDst = 0;
            u32 u32MiuAddrSrc = 0, u32MiuAddrDst = 0;
            MI_U32 u32Channel;
            MI_U32 u32isp_out_image_count = 0, u32timeout = 0;
            CusImageResolution_t timage_resolution;
            CusISPOutImage_t tisp_out_image;
            CameraRawStoreNode_e eNode;
            CusHdrRawImage_t thdr_raw_image;

            if(cmd == 'd')
            {
                eNode = eRawStoreNode_ISPOUT;
            }
            else if(cmd == 'D')
            {
                eNode = eRawStoreNode_P0TAIL;
            }
            else if(cmd == 'H')
            {
                eNode = eRawStoreNode_P1HEAD;
            }
            else if(cmd == 'r')
            {
                eNode = eRawStoreNode_RGBIR_IR_ONLY;
            }

            //Get width and height from isp api.
            u32Channel = 0;
            timage_resolution.u32Node = (MI_U32)eNode;
            MI_ISP_CUS3A_GetImageResolution(u32Channel, &timage_resolution);
            printf("Node:%d, width=%d, height=%d, depth=%d\n", eNode, timage_resolution.u32image_width, timage_resolution.u32image_height, timage_resolution.u32PixelDepth);
            /*
            // data precision
            {
                ISP_DATAPRECISION_8 = 0,
                ISP_DATAPRECISION_10 = 1,
                ISP_DATAPRECISION_16 = 2,
                ISP_DATAPRECISION_12 = 3,
            }
            */
            u32BufSizeSrc = (eNode == eRawStoreNode_RGBIR_IR_ONLY) ?
                            timage_resolution.u32image_width * timage_resolution.u32image_height / 4 :
                            timage_resolution.u32image_width * timage_resolution.u32image_height * 2;

            do
            {
                if(u32BufSizeSrc == 0)
                {
                    printf("Error! Buffer size is 0.\n");
                    break;
                }

                pImgAddrSrc = (void *)pAllocDmaBuffer("ISP_OUT_SRC", u32BufSizeSrc, &u32PhysAddrSrc, &u32MiuAddrSrc, FALSE);
                if(pImgAddrSrc == NULL)
                {
                    printf("Error! Allocate buffer Error!!!\n");
                    break;
                }

                if(eNode == eRawStoreNode_P1HEAD)
                {
                    thdr_raw_image.u32enable = 1;
                    thdr_raw_image.u32image_width = timage_resolution.u32image_width;
                    thdr_raw_image.u32image_height = timage_resolution.u32image_height;
                    thdr_raw_image.u32physical_address = u32MiuAddrSrc;
                    thdr_raw_image.u32Node = (MI_U32)eNode;
                    thdr_raw_image.u32PixelDepth = (MI_U32)timage_resolution.u32PixelDepth;
                    MI_ISP_CUS3A_CaptureHdrRawImage(u32Channel, &thdr_raw_image);

                    u32timeout = 100;
                    do
                    {
                        usleep(1000 * 10);
                        MI_ISP_CUS3A_GetISPOutImageCount(u32Channel, &u32isp_out_image_count);
                    }
                    while((u32isp_out_image_count < 2) && (--u32timeout > 0));
                    printf("HDR raw image count:%d, time:%d ms.\n", u32isp_out_image_count, (100 - u32timeout) * 10);

                    thdr_raw_image.u32enable = 0;
                    MI_ISP_CUS3A_CaptureHdrRawImage(u32Channel, &thdr_raw_image);

                    if(thdr_raw_image.u32PixelDepth != 2)  // not  ISP_DATAPRECISION_16
                    {
                        u32BufSizeDst = thdr_raw_image.u32image_width * thdr_raw_image.u32image_height * 2;
                        pImgAddrDst = (void *)pAllocDmaBuffer("ISP_OUT_DST", u32BufSizeDst, &u32PhysAddrDst, &u32MiuAddrDst, FALSE);
                        if(pImgAddrDst == NULL)
                        {
                            printf("Error! Allocate buffer Error!!!\n");
                            break;
                        }
                        ConvertRawImageTo16Bits(pImgAddrSrc, pImgAddrDst, thdr_raw_image.u32image_width, thdr_raw_image.u32image_height, thdr_raw_image.u32PixelDepth);
                    }
                    else  // for ISP_DATAPRECISION_16
                    {
                        pImgAddrDst = pImgAddrSrc;
                        u32BufSizeDst = u32BufSizeSrc;
                    }
                }
                else
                {
                    tisp_out_image.u32enable = 1;
                    tisp_out_image.u32image_width = timage_resolution.u32image_width;
                    tisp_out_image.u32image_height = timage_resolution.u32image_height;
                    tisp_out_image.u32physical_address = u32MiuAddrSrc;
                    tisp_out_image.u32Node = (MI_U32)eNode;
                    MI_ISP_CUS3A_EnableISPOutImage(u32Channel, &tisp_out_image);

                    u32timeout = 100;
                    do
                    {
                        usleep(1000 * 10);
                        MI_ISP_CUS3A_GetISPOutImageCount(u32Channel, &u32isp_out_image_count);
                    }
                    while((u32isp_out_image_count < 2) && (--u32timeout > 0));
                    printf("ISP out image count:%d, time:%d ms.\n", u32isp_out_image_count, (100 - u32timeout) * 10);

                    tisp_out_image.u32enable = 0;
                    MI_ISP_CUS3A_EnableISPOutImage(u32Channel, &tisp_out_image);
                    pImgAddrDst = pImgAddrSrc;
                    u32BufSizeDst = u32BufSizeSrc;
                }

                //Save image file.
                {
                    char filename[64];
                    //struct timeval timestamp;

                    //gettimeofday(&timestamp, 0);
                    //printf("saveSnapshot%d_%08d.%s buffLen=%d\n", (int)timestamp.tv_sec, (int)timestamp.tv_usec, name, buffLen);

                    memset(filename, 0x00, sizeof(filename));
                    if(eNode == eRawStoreNode_ISPOUT)
                        sprintf(filename, "dump_ispout_yuv422.raw");
                    else if(eNode == eRawStoreNode_RGBIR_IR_ONLY)
                        sprintf(filename, "dump_RGBIR_IR_only_raw8.raw");
                    else if(eNode == eRawStoreNode_P1HEAD)
                    {
                        switch(thdr_raw_image.u32PixelDepth)
                        {
                            case 0: // ISP_DATAPRECISION_8
                                sprintf(filename, "dump_hdr_p1_raw8_to_raw16.raw");
                                break;
                            case 1: // ISP_DATAPRECISION_10
                                sprintf(filename, "dump_hdr_p1_raw10_to_raw16.raw");
                                break;
                            case 2: // ISP_DATAPRECISION_16
                                sprintf(filename, "dump_hdr_p1_raw16.raw");
                                break;
                            case 3: // ISP_DATAPRECISION_12
                                sprintf(filename, "dump_hdr_p1_raw12_to_raw16.raw");
                                break;
                            default:
                                printf("data precision error!! fmt: %d\n", thdr_raw_image.u32PixelDepth);
                                break;
                        }
                    }
                    else
                        sprintf(filename, "dump_ispin_p0_raw16.raw");

                    FILE *pfile = fopen(filename, "wb");
                    if(NULL == pfile)
                    {
                        printf("error: fopen %s failed\n", filename);
                        return -1;
                    }

                    fwrite(pImgAddrDst, 1, u32BufSizeDst, pfile);

                    fflush(pfile);
                    fclose(pfile);
                    sync();
                }

                FreeDmaBuffer("ISP_OUT_SRC", u32MiuAddrSrc, pImgAddrSrc, u32BufSizeSrc);
                if((eNode == eRawStoreNode_P1HEAD) && (thdr_raw_image.u32PixelDepth != 2))
                {
                    FreeDmaBuffer("ISP_OUT_DST", u32MiuAddrDst, pImgAddrDst, u32BufSizeDst);
                }
            }
            while(0);
        }
        pthread_mutex_lock(&_gTime);
        for (i = 0; i < 5; i++)
        {
            printf("time [%d] = %d\n", i, curtime[i]);
        }
        printf("Buf size is %d\n", u32BufSize);
        pthread_mutex_unlock(&_gTime);
    }
    St_BaseModuleDeinit();
    return 0;
}
