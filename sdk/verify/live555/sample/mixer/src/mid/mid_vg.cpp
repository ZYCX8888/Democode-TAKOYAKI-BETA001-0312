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
#include <math.h>

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

static pthread_t g_pthread_vg;
static MI_VDF_ChnAttr_t g_stAttr[1] = { {(MI_VDF_WorkMode_e)0,}, };
static MI_VDF_CHANNEL g_VdfChn[1] = { 0 };
static BOOL g_vgExit = FALSE;

extern int g_ieWidth;
extern int g_ieHeight;
extern MI_S32 g_ieVpePort;
extern MI_S32 g_ieFrameInterval;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
extern int VGtoRECT(int chn, MI_VDF_VgAttr_t *pVgAttr, MI_VG_Result_t *pVgResult);

int mid_vg_VDF_Set_Attr(MI_VDF_ChnAttr_t* pstAttr)
{
    pstAttr->enWorkMode = E_MI_VDF_WORK_MODE_VG;
    pstAttr->stVgAttr.u8VgBufCnt  = 4;
    pstAttr->stVgAttr.u8VDFIntvl  = 0;

    pstAttr->stVgAttr.height = g_ieHeight;
    pstAttr->stVgAttr.width = g_ieWidth;
    pstAttr->stVgAttr.stride = g_ieWidth;   //ALIGN64(g_ieWidth);

    pstAttr->stVgAttr.object_size_thd = VG_SENSITIVELY_HIGH;
    pstAttr->stVgAttr.indoor = 1;
    pstAttr->stVgAttr.function_state = VG_VIRTUAL_GATE;
    pstAttr->stVgAttr.line_number = 2;

    if( pstAttr->stVgAttr.function_state == VG_VIRTUAL_GATE )
    {
        if( pstAttr->stVgAttr.line_number >= 1 )
        {
            //First Line   384 * 288
            pstAttr->stVgAttr.line[0].px.x = 60;
            pstAttr->stVgAttr.line[0].px.y = 150;
            pstAttr->stVgAttr.line[0].py.x = 160;
            pstAttr->stVgAttr.line[0].py.y = 150;
            pstAttr->stVgAttr.line[0].pdx.x = 110;
            pstAttr->stVgAttr.line[0].pdx.y = 190;
            pstAttr->stVgAttr.line[0].pdy.x = 110;
            pstAttr->stVgAttr.line[0].pdy.y = 120;
        }

        if( pstAttr->stVgAttr.line_number == 2 )
        {
            //Second Line
            pstAttr->stVgAttr.line[1].px.x = 230;
            pstAttr->stVgAttr.line[1].px.y = 220;
            pstAttr->stVgAttr.line[1].py.x = 330;
            pstAttr->stVgAttr.line[1].py.y = 220;
            pstAttr->stVgAttr.line[1].pdx.x = 280;
            pstAttr->stVgAttr.line[1].pdx.y = 190;
            pstAttr->stVgAttr.line[1].pdy.x = 280;
            pstAttr->stVgAttr.line[1].pdy.y = 250;
        }
    }
    else  //VG_REGION_INVASION
    {
        pstAttr->stVgAttr.vg_region.p_one.x = 15;
        pstAttr->stVgAttr.vg_region.p_one.y = 20;
        pstAttr->stVgAttr.vg_region.p_two.x = 115;
        pstAttr->stVgAttr.vg_region.p_two.y = 20;
        pstAttr->stVgAttr.vg_region.p_three.x = 115;
        pstAttr->stVgAttr.vg_region.p_three.y = 95;
        pstAttr->stVgAttr.vg_region.p_four.x = 15;
        pstAttr->stVgAttr.vg_region.p_four.y = 95;

        //Set region direction
        pstAttr->stVgAttr.vg_region.region_dir = VG_REGION_ENTER;
        //vg_region.region_dir = VG_REGION_LEAVING;
        //vg_region.region_dir = VG_REGION_CROSS;
    }
    printf("VG line_number=%d, function_state=%d\n", pstAttr->stVgAttr.line_number, pstAttr->stVgAttr.function_state);
    return 0;
}

int mid_vg_Bind_DIVP_To_VDF(void)
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
    stDstChnPort.u32ChnId = g_VdfChn[0];
    stDstChnPort.u32PortId = 0;

//  printf("OD g_ieVpePort=%d,u32ChnId=%d,srcFps=%d,dstFps=%d\n", g_ieVpePort, u32ChnId, srcFps, dstFps);
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, srcFps, dstFps), MI_SUCCESS);

    return 0;
}

int mid_vg_UnBind_DIVP_To_VDF(void)
{

    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = MIXER_DIVP_CHNID_FOR_VDF;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = g_VdfChn[0];
    stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);

    return 0;
}

int mid_vg_VDF_Config()
{
    // initial VDF_HDL(OD)
    g_VdfChn[0] = 2;

    mid_vg_VDF_Set_Attr(&g_stAttr[0]);
    if(0 != CheckAlign((MI_U16)MDMB_MODE_BUTT, g_stAttr[0].stVgAttr.stride))
    {
         return -1;
    }

    //VG_line or VG_region do no exceed Ie width and height
    if( g_stAttr[0].stVgAttr.function_state == VG_VIRTUAL_GATE )  //VG_LINE
    {
        if( g_stAttr[0].stVgAttr.line_number >= 1 && (g_stAttr[0].stVgAttr.width <= g_stAttr[0].stVgAttr.line[0].py.x
            || g_stAttr[0].stVgAttr.height <= g_stAttr[0].stVgAttr.line[0].pdx.y))
        {
            MIXER_ERR("line number = %d,Ie width(%d) height(%d) must be larger than frist Line width(%d) height(%d), please check input param\n",
                g_stAttr[0].stVgAttr.line_number,g_stAttr[0].stVgAttr.width,g_stAttr[0].stVgAttr.height,g_stAttr[0].stVgAttr.line[0].py.x,g_stAttr[0].stVgAttr.line[0].pdx.y);
            return -1;
        }

        if( g_stAttr[0].stVgAttr.line_number == 2 && (g_stAttr[0].stVgAttr.width <= g_stAttr[0].stVgAttr.line[1].py.x
            || g_stAttr[0].stVgAttr.height <= g_stAttr[0].stVgAttr.line[1].pdy.y))
        {
            MIXER_ERR("line number = %d, Ie width(%d) height(%d) must be larger than second Line width(%d) height(%d), please check input param\n",
                g_stAttr[0].stVgAttr.line_number,g_stAttr[0].stVgAttr.width,g_stAttr[0].stVgAttr.height,g_stAttr[0].stVgAttr.line[1].py.x,g_stAttr[0].stVgAttr.line[1].pdy.y);
            return -1;
        }
    }
    else  //VG_REGION_INVASION
    {
        if( g_stAttr[0].stVgAttr.width <= g_stAttr[0].stVgAttr.vg_region.p_three.x
            && g_stAttr[0].stVgAttr.height <= g_stAttr[0].stVgAttr.vg_region.p_three.y)
        {
            MIXER_ERR("Ie width(%d) height(%d) must be larger than VG_Region width(%d) height(%d), please check input param\n",
                g_stAttr[0].stVgAttr.width,g_stAttr[0].stVgAttr.height,g_stAttr[0].stVgAttr.vg_region.p_three.x,g_stAttr[0].stVgAttr.vg_region.p_three.y);
            return -1;
        }
    }

    MI_VDF_CreateChn(g_VdfChn[0], &g_stAttr[0]);

    //MI_VDF_GetLibVersion(g_VdfChn[0], u32VDFVersion);

    MI_VDF_Run(E_MI_VDF_WORK_MODE_VG);

    sleep(1);

    mid_vg_Bind_DIVP_To_VDF();

    MI_VDF_EnableSubWindow(g_VdfChn[0], 0, 0, 1);

    return 0;
}

void* mid_vg_Task(void *argu)
{
    int chn = 0;
    int alarmCnt = 0;
    MI_S32 ret = 0;
    MI_VDF_Result_t stVdfResult = {(MI_VDF_WorkMode_e)0 };
//    struct timespec ts_pre = { 0 };
//    struct timespec ts_cur = { 0 };
//    float time = 0;
    MI_VG_Result_t stVgResult;
    //MI_VG_Result_t stVgResultTmp;
    BOOL blEnter = FALSE;
    BOOL blDiff = FALSE;

    printf("enter vg task loop\n");
    do{     // wait osd init ok
        usleep(100*1000);
    }while(!g_vgExit && ret++ < 30);
    memset(&stVgResult,0x00,sizeof(MI_VG_Result_t));
    stVdfResult.stVgResult.alarm[0] = 1;
    stVdfResult.stVgResult.alarm[1] = 1;
    VGtoRECT(chn, &g_stAttr[0].stVgAttr, &stVdfResult.stVgResult);
    while(!g_vgExit)
    {
        usleep(1000*10);
        blEnter = FALSE;
        blDiff = FALSE;
        memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
        stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_VG;
        ret = MI_VDF_GetResult(g_VdfChn[0], &stVdfResult, 0);
        if(MI_SUCCESS == ret)
        {
            for(int i = 0; i < g_stAttr[0].stVgAttr.line_number; i++)
            {
                if(1 == stVdfResult.stVgResult.alarm[i]) blEnter = TRUE;
                if(stVgResult.alarm[i] != stVdfResult.stVgResult.alarm[i]) blDiff = TRUE;
            }
            memcpy(&stVgResult, &stVdfResult.stVgResult, sizeof(stVgResult));
            MI_VDF_PutResult(g_VdfChn[0], &stVdfResult);
            if(blEnter)
            {
                if(blDiff)
                {
                    usleep(1000*10);
                    VGtoRECT(chn, NULL, NULL);
                    usleep(1000*10);
                }
                VGtoRECT(chn, &g_stAttr[0].stVgAttr, &stVgResult);
                alarmCnt = 10;
                continue;
            }
            else if(alarmCnt-- < 1)
            {
                VGtoRECT(chn, NULL, NULL);
            }
        }
    }

    return NULL;
}

int mid_vg_Initial(int param)
{
    g_vgExit = FALSE;
    printf("mid_vg_Initial\n");
    if(0 !=  mid_vg_VDF_Config())
        return -1;
    pthread_create(&g_pthread_vg, NULL, mid_vg_Task, NULL);
    pthread_setname_np(g_pthread_vg , "mid_vg_Task");
    return 0;
}

int mid_vg_Uninitial(void)
{
    //void *ptExt;

    MI_VDF_Stop(E_MI_VDF_WORK_MODE_VG);
    g_vgExit = TRUE;
    pthread_join(g_pthread_vg, NULL);
    mid_vg_UnBind_DIVP_To_VDF();
    MI_VDF_DestroyChn(g_VdfChn[0]);
    return 0;
}

