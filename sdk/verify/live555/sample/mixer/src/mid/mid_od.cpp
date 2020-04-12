/*************************************************
*
*       Author: summer.guo@sigmastar.com.cn
*
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "mid_utils.h"
#include "mid_common.h"
#include "mid_md.h"
#include "mi_md.h"
#include "mi_od.h"
#include "mi_vg.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"
#include "mi_sys_datatype.h"
#include "mid_video_type.h"
#include "module_common.h"
#include <errno.h>

static pthread_t g_OdPthread;
static MI_VDF_ChnAttr_t g_stAttr[1] = { (MI_VDF_WorkMode_e)0 };
//static MI_VDF_CHANNEL g_VdfChn[1] = { 0 };

extern int g_ieWidth;
extern int g_ieHeight;
extern MI_S32 g_ieVpePort;
extern MI_S32 g_ieFrameInterval;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];

extern int ODtoRECT(int chn, MI_OD_Result_t stOdResult);

static BOOL g_odExit = FALSE;
static MI_U8 OdUseVdfChannel = 0;
MI_U8 GetOdUseVdfChannelValue()
{
  return OdUseVdfChannel;
}
void SetOdUseVdfChannelValue(MI_U8 value)
{
   OdUseVdfChannel = value;
}

int mid_od_VDF_Set_Attr(MI_VDF_ChnAttr_t* pstAttr)
{
    pstAttr->enWorkMode = E_MI_VDF_WORK_MODE_OD;
    pstAttr->stOdAttr.u8OdBufCnt  = 4;
    pstAttr->stOdAttr.u8VDFIntvl  = 0;

    pstAttr->stOdAttr.stOdDynamicParamsIn.thd_tamper     = 3;
    pstAttr->stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
    pstAttr->stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

    pstAttr->stOdAttr.stOdStaticParamsIn.inImgW = g_ieWidth;
    pstAttr->stOdAttr.stOdStaticParamsIn.inImgH = g_ieHeight;
    pstAttr->stOdAttr.stOdStaticParamsIn.inImgStride = g_ieWidth;//ALIGN64(g_ieWidth);
    pstAttr->stOdAttr.stOdStaticParamsIn.nClrType = OD_Y;
    pstAttr->stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
    pstAttr->stOdAttr.stOdStaticParamsIn.alpha = 2;
    pstAttr->stOdAttr.stOdStaticParamsIn.M = 120;
    pstAttr->stOdAttr.stOdStaticParamsIn.MotionSensitivity = 0;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.num = 4;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x = 0;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y = 0;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x = g_ieWidth - 1;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y = 0;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x = g_ieWidth - 1;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y = g_ieHeight - 1;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x = 0;
    pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y = g_ieHeight - 1;

    printf("OD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
           pstAttr->stMdAttr.stMdStaticParamsIn.width,
           pstAttr->stMdAttr.stMdStaticParamsIn.height,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x,
           pstAttr->stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y);

    return 0;
}

int mid_od_Bind_DIVP_To_VDF(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    int srcFps = g_videoParam[0].vpeframeRate;
    int dstFps = g_ieFrameInterval;

    stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = MIXER_DIVP_CHNID_FOR_VDF;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = GetOdUseVdfChannelValue();
    stDstChnPort.u32PortId = 0;

//  printf("OD g_ieVpePort=%d,u32ChnId=%d,srcFps=%d,dstFps=%d\n", g_ieVpePort, u32ChnId, srcFps, dstFps);
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, srcFps, dstFps), MI_SUCCESS);

    return 0;
}

int mid_od_UnBind_DIVP_To_VDF(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = MIXER_DIVP_CHNID_FOR_VDF;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = GetOdUseVdfChannelValue();
    stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);

    return 0;
}

int mid_od_VDF_Config()
{
    // initial VDF_HDL(OD)
    mid_od_VDF_Set_Attr(&g_stAttr[0]);
    if(0 != CheckAlign((MI_U16)MDMB_MODE_BUTT, g_stAttr[0].stOdAttr.stOdStaticParamsIn.inImgStride))
    {
         return -1;
    }
    MI_VDF_CreateChn(GetOdUseVdfChannelValue(), &g_stAttr[0]);

    //MI_VDF_GetLibVersion(g_VdfChn[0], u32VDFVersion);

    MI_VDF_Run(E_MI_VDF_WORK_MODE_OD);

    sleep(1);

    mid_od_Bind_DIVP_To_VDF();
    MI_VDF_EnableSubWindow(GetOdUseVdfChannelValue(), 0, 0, 1);

    return 0;
}

void* mid_od_Task(void *arg)
{
    MI_S32 ret = 0;
    static int sOdFlag = 0;
    static int sShakeDelay = 0;
    MI_VDF_Result_t stVdfResult = { (MI_VDF_WorkMode_e)0 };
    int chn = 0;

    printf("enter od task loop\n");
    while(!g_odExit)
    {
        MySystemDelay(10);
        sShakeDelay++;

        memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
        stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
        ret = MI_VDF_GetResult(GetOdUseVdfChannelValue(), &stVdfResult, 0);
        if((0 == ret) && (1 == stVdfResult.stOdResult.u8Enable))
        {
            MI_VDF_PutResult(GetOdUseVdfChannelValue(), &stVdfResult);

            ODtoRECT(chn, stVdfResult.stOdResult);
            sOdFlag = 1;
            sShakeDelay = 0;
        }
        else
        {
            if(sShakeDelay > 40)
            {
                if(sOdFlag){
                    stVdfResult.stOdResult.u8Enable = 0;
                    ODtoRECT(chn, stVdfResult.stOdResult);
                    sOdFlag = 0;
                }
                sShakeDelay = 0;
            }
        }
    }

    memset(&stVdfResult, 0x00, sizeof(stVdfResult));
    ODtoRECT(chn, stVdfResult.stOdResult);

    return NULL;
}


int mid_od_Initial(int param)
{
    SetOdUseVdfChannelValue(1);
    g_odExit = FALSE;
    printf("mid_od_Initial\n");
    if(0 != mid_od_VDF_Config())
        return -1;
      MI_S32 s32Ret = pthread_create(&g_OdPthread, NULL, mid_od_Task, NULL);
    if(0 == s32Ret)
    {
      pthread_setname_np(g_OdPthread , "Mid_Od_Task");
    }
    else
    {
       g_OdPthread = -1;
       printf("mid_od_Initial, create g_pthread_od=%ld is err[%d]!\n %s\n",g_OdPthread,s32Ret,strerror(s32Ret));
    }
    return 0;
}

int mid_od_Uninitial(void)
{
  //  void *ptExt;

    MI_VDF_Stop(E_MI_VDF_WORK_MODE_OD);
    g_odExit = TRUE;
    if(-1 != (MI_S32)g_OdPthread)
    {
      pthread_join(g_OdPthread, NULL);
    }
    mid_od_UnBind_DIVP_To_VDF();
    MI_VDF_DestroyChn(GetOdUseVdfChannelValue());
    return 0;
}

