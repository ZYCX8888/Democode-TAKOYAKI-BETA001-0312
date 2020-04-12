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


static pthread_t g_pthread_md;
static MI_VDF_ChnAttr_t g_stAttr[1] = { {(MI_VDF_WorkMode_e)0,}, };

extern int g_ieWidth;
extern int g_ieHeight;
extern MI_S32 g_ieFrameInterval;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];

static BOOL g_mdExit = FALSE;
//static pthread_mutex_t g_VDFChnMutex = PTHREAD_MUTEX_INITIALIZER;
extern int MultiMDtoRECT_SAD(int chn, MI_U8* pu8MdRstData, int col, int row, int enable);
extern int MultiMDtoRECT_FG(int chn, MDOBJ_t* pstRegion, int regionNum, int enable);
extern int MultiMDtoRECT_FG2(int chn, MI_U8* pu8MdRstData, int col, int row, int enable);
static MI_U8 MdUseVdfChannel = 0;
MI_U8 GetMdUseVdfChannelValue()
{
  return MdUseVdfChannel;
}
void SetMdUseVdfChannelValue(MI_U8 value)
{
   MdUseVdfChannel = value;
}

int mid_md_VDF_Set_Attr(MI_VDF_ChnAttr_t* pstAttr,int param)
{
    pstAttr->enWorkMode = E_MI_VDF_WORK_MODE_MD;
    pstAttr->stMdAttr.u8Enable      = 1;
    pstAttr->stMdAttr.u8MdBufCnt  = 4;
    pstAttr->stMdAttr.u8VDFIntvl  = 0;
    pstAttr->stMdAttr.ccl_ctrl.u16InitAreaThr = 8;
    pstAttr->stMdAttr.ccl_ctrl.u16Step = 2;
    pstAttr->stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    pstAttr->stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    pstAttr->stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    pstAttr->stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;
    pstAttr->stMdAttr.stMdStaticParamsIn.width     = g_ieWidth;//720;//g_width;
    pstAttr->stMdAttr.stMdStaticParamsIn.height  = g_ieHeight;//576;//g_height;
    pstAttr->stMdAttr.stMdStaticParamsIn.stride  = g_ieWidth;//ALIGN64(g_ieWidth);
    pstAttr->stMdAttr.stMdStaticParamsIn.color     = 1;
    pstAttr->stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
    if(param == 2){
        pstAttr->stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_FG;
    }else{
        pstAttr->stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD;
    }
    pstAttr->stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x = 0;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y = 0;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x = g_ieWidth - 1;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y = 0;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x = g_ieWidth - 1;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y = g_ieHeight - 1;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x = 0;
    pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y = g_ieHeight - 1;

    printf("MD width:%d,height:%d, 0(%dx%d), 1(%dx%d), 2(%dx%d), 3(%dx%d)\n",
            pstAttr->stMdAttr.stMdStaticParamsIn.width,
            pstAttr->stMdAttr.stMdStaticParamsIn.height,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x,
            pstAttr->stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y);

    return 0;
}

int mid_md_Bind_DIVP_To_VDF(void)
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
    stDstChnPort.u32ChnId = GetMdUseVdfChannelValue();
    stDstChnPort.u32PortId = 0;

//  printf("MD g_ieVpePort=%d,u32ChnId=%d,srcFps=%d,dstFps=%d\n", g_ieVpePort, u32ChnId, srcFps, dstFps);
    ExecFunc(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, srcFps, dstFps), MI_SUCCESS);

    return 0;
}

int mid_md_UnBind_DIVP_To_VDF(void)
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = MIXER_DIVP_CHNID_FOR_VDF;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_VDF;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = GetMdUseVdfChannelValue();
    stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort), MI_SUCCESS);
    return 0;
}

int mid_md_VDF_Config(int param)
{
    int ret;

    mid_md_VDF_Set_Attr(&g_stAttr[0],param);
    if(0 != CheckAlign(g_stAttr[0].stMdAttr.stMdStaticParamsIn.mb_size, g_stAttr[0].stMdAttr.stMdStaticParamsIn.stride))
    {
         return -1;
    }

    ret = MI_VDF_CreateChn(GetMdUseVdfChannelValue(), &g_stAttr[0]);
    if(ret != MI_SUCCESS)
    {
        printf("MI_VDF_CreateChn ret=0x%x\n", ret);
    }

    MI_VDF_Run(E_MI_VDF_WORK_MODE_MD);

    sleep(1);

    mid_md_Bind_DIVP_To_VDF();

    MI_VDF_EnableSubWindow(GetMdUseVdfChannelValue(), 0, 0, 1);

    return 0;
}

void mid_md_MdRstData_display(MI_U8 *pu8MdRstData, int buffer_size, int col, int row)
{
    int i,j;

    printf("row %d, col %d---------------\n", row, col);
    for(i = 0; i < row; i++)
    {
        for(j = 0; j < col; j++)
        {
            printf("%02d ",pu8MdRstData[i*col+j]);
        }
        printf("\n");
    }
}

void* mid_md_Task(void *arg)
{
    int sMdSADFlag = 0;
    int sMdFGFlag = 0;
    MI_U32 col, row, buffer_size;
    int sShakeDelay = 0;

    int chn = 0;
    MI_MD_static_param_t *pstMdStaticParamsIn = &g_stAttr[0].stMdAttr.stMdStaticParamsIn;
    MI_U8 *stSadDataArry = NULL;
    MI_U8 *fgDataArry = NULL;
    if(pstMdStaticParamsIn->mb_size == MDMB_MODE_MB_4x4)
    {
        col = g_ieWidth >> 2;
        row = g_ieHeight >> 2;
    }
    else if(pstMdStaticParamsIn->mb_size == MDMB_MODE_MB_8x8)
    {
        col = g_ieWidth >> 3;    // 48
        row = g_ieHeight >> 3;    // 36
    }
    else    // MDMB_MODE_MB_16x16
    {
        col = g_ieWidth >> 4;
        row = g_ieHeight >> 4;
    }

    buffer_size = col * row;    // MDSAD_OUT_CTRL_8BIT_SAD
    if(pstMdStaticParamsIn->sad_out_ctrl == MDSAD_OUT_CTRL_16BIT_SAD){
        buffer_size *= 2;
    }
    stSadDataArry = (MI_U8 *)malloc(buffer_size + 1);
    if(NULL == stSadDataArry)
    {
        MIXER_ERR("can not malloc stSadDataArry buf.\n");
        goto exit;
    }
    memset(stSadDataArry, 0x0, buffer_size + 1);

    fgDataArry = (MI_U8 *)malloc(col*row + 1);
    if(NULL == fgDataArry)
    {
        MIXER_ERR("can not malloc fgDataArry buf.\n");
        goto exit;
    }
    memset(fgDataArry, 0x0, col*row + 1);

    printf("enter md task loop\n");
    while(!g_mdExit)
    {
        MI_S32 ret = 0;
        MI_U8* pu8MdRstData = NULL;
        MI_VDF_Result_t stVdfResult = { (MI_VDF_WorkMode_e)0 };

        MySystemDelay(10);
        sShakeDelay++;

        memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
        stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
        ret = MI_VDF_GetResult(GetMdUseVdfChannelValue(), &stVdfResult, 0);
        if(MI_SUCCESS == ret)
        {
            if(1 == stVdfResult.stMdResult.u8Enable)
            {
                if( stVdfResult.stMdResult.pstMdResultSad != NULL &&
                    stVdfResult.stMdResult.pstMdResultSad->enOutCtrl == MDSAD_OUT_CTRL_8BIT_SAD &&
                    g_stAttr[0].stMdAttr.stMdStaticParamsIn.md_alg_mode == MDALG_MODE_SAD )
                {
                    pu8MdRstData = (MI_U8*)stVdfResult.stMdResult.pstMdResultSad->paddr;
                    memcpy(stSadDataArry, pu8MdRstData, buffer_size);
                    sMdSADFlag = 1;
                }
                else if(stVdfResult.stMdResult.pstMdResultObj != NULL &&
                        g_stAttr[0].stMdAttr.stMdStaticParamsIn.md_alg_mode == MDALG_MODE_FG)
                {
                    /*
                    fgRegionNum = stVdfResult.stMdResult.pstMdResultObj->u8RegionNum;
                    memcpy(stRegionArry, stVdfResult.stMdResult.pstMdResultObj->astRegion,
                            sizeof(MDOBJ_t) * fgRegionNum);
                    printf("MD fgRectNum=%d\n", fgRegionNum);
                    */
                    // value = 0 or 255
                    memcpy(fgDataArry, stVdfResult.stMdResult.pstMdResultStatus->paddr, col*row);
                    sMdFGFlag = 1;
                }

                MI_VDF_PutResult(GetMdUseVdfChannelValue(), &stVdfResult);

                if(sMdSADFlag && g_stAttr[0].stMdAttr.stMdStaticParamsIn.md_alg_mode == MDALG_MODE_SAD){
                    ////mid_md_MdRstData_display(stSadDataArry, buffer_size, col, row);
                    MultiMDtoRECT_SAD(chn, stSadDataArry, col, row, 1);
                }

                if(sMdFGFlag && g_stAttr[0].stMdAttr.stMdStaticParamsIn.md_alg_mode == MDALG_MODE_FG){
                    ////mid_md_MdRstData_display(fgDataArry, col*row, col, row);
                    ////MultiMDtoRECT_FG(chn, stRegionArry, fgRegionNum, 1);
                    MultiMDtoRECT_FG2(chn, fgDataArry, col, row, 1);
                }
                sShakeDelay = 0;
            }
            else
            {
                MI_VDF_PutResult(GetMdUseVdfChannelValue(), &stVdfResult);
            }
        }
        else
        {
            if(sShakeDelay > 40)
            {
                if(sMdSADFlag){
                    MultiMDtoRECT_SAD(chn, stSadDataArry, col, row, 0);
                    sMdSADFlag = 0;
                }
                if(sMdFGFlag){
                    ////MultiMDtoRECT_FG(chn, stRegionArry, fgRegionNum, 0);
                    MultiMDtoRECT_FG2(chn, fgDataArry, col, row, 0);
                    sMdFGFlag = 0;
                }
                sShakeDelay = 0;
            }
        }
    }

    if(sMdSADFlag){
        memset(stSadDataArry, 0x00, buffer_size+1);
        MultiMDtoRECT_SAD(chn, stSadDataArry, col, row, 1);
        sMdSADFlag = 0;
    }
    if(sMdFGFlag){
        memset(fgDataArry, 0x00, col*row + 1);
        ////MultiMDtoRECT_FG(chn, stRegionArry, fgRegionNum, 0);
        MultiMDtoRECT_FG2(chn, fgDataArry, col, row, 1);
        sMdFGFlag = 0;
    }

exit:
    if(NULL != stSadDataArry)
    {
        free(stSadDataArry);
        stSadDataArry = NULL;
    }

    if(NULL != fgDataArry)
    {
        free(fgDataArry);
        fgDataArry = NULL;
    }

    printf("exit md task loop\n");

    return NULL;
}

int mid_md_Initial(int param)
{
    SetMdUseVdfChannelValue(0);
    g_mdExit = FALSE;
    printf("mid_md_Initial param=%d\n", param);
    if(0 != mid_md_VDF_Config(param))
        return -1;
       MI_S32 s32Ret = pthread_create(&g_pthread_md, NULL, mid_md_Task, NULL);
    if(0 == s32Ret)
    {
        pthread_setname_np(g_pthread_md, "mid_md_Task");
    }
    else
    {
       g_pthread_md = -1;
       printf("mid_md_Initial,g_pthread_md=%ld is error[%d]\n%s\n", g_pthread_md,s32Ret,strerror(s32Ret));
    }
    return 0;
}

int mid_md_Uninitial(void)
{
    //void *ptExit;

    ExecFunc(MI_VDF_Stop(E_MI_VDF_WORK_MODE_MD), MI_SUCCESS);
    g_mdExit = TRUE;
    if(-1 != (MI_S32)g_pthread_md)
    {
      pthread_join(g_pthread_md, NULL);
    }
    ExecFunc(MI_VDF_DestroyChn(GetMdUseVdfChannelValue()), MI_SUCCESS);
    return 0;
}

int mid_md_Param_Change(MI_S8 *param, MI_S32 paramLen)
{
    MI_S8 mdParam[8];
    MI_U16 tmp[2]={0x00,0x00};
    MI_VDF_ChnAttr_t stAttr;
    int vdfchn = 0x0;

    memcpy((char *)mdParam, param, paramLen);
    tmp[0] = mdParam[6];
    tmp[1] = mdParam[7];
    if(mdParam[6] < 0)
    {
      tmp[0] = mdParam[6]+256;
    }
    if(mdParam[7] < 0)
    {
      tmp[1] = mdParam[7]+256;
    }
    //vdfchn = mdParam[1];
    if(mdParam[0] > 0)
    {
        ExecFunc(MI_VDF_GetChnAttr(GetMdUseVdfChannelValue(), &stAttr), MI_SUCCESS);

        stAttr.stMdAttr.u8Enable = mdParam[2];
#if TARGET_CHIP_I5
        stAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max    = mdParam[3]+128;
        stAttr.stMdAttr.stMdDynamicParamsIn.md_thr        = mdParam[4]+128;
#else
          stAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max    = mdParam[3];
        stAttr.stMdAttr.stMdDynamicParamsIn.md_thr        = mdParam[4];
#endif
        stAttr.stMdAttr.stMdDynamicParamsIn.sensitivity    = mdParam[5];

        stAttr.stMdAttr.stMdDynamicParamsIn.learn_rate    = (tmp[0] << 8) | (tmp[1]);

        printf("open state=%d g_VdfChn[%d]=%d,obj_num_max=%d,md_thr=%d,sensitivity=%d,learn_rate=%d\n",
                stAttr.stMdAttr.u8Enable,vdfchn,GetMdUseVdfChannelValue(), stAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max,
                                                                stAttr.stMdAttr.stMdDynamicParamsIn.md_thr,mdParam[5],
                                                                    stAttr.stMdAttr.stMdDynamicParamsIn.learn_rate);
        stAttr.enWorkMode = E_MI_VDF_WORK_MODE_MD;
        ExecFunc(MI_VDF_SetChnAttr(GetMdUseVdfChannelValue(), &stAttr), MI_SUCCESS);

        if(stAttr.stMdAttr.u8Enable){
            ExecFunc(MI_VDF_Run(stAttr.enWorkMode), MI_SUCCESS);
        }else{
            ExecFunc(MI_VDF_Stop(stAttr.enWorkMode), MI_SUCCESS);
        }
    }

    return 0;
}

