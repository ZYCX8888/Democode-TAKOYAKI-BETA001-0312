
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "cus02_video_api.h"

#define MAX_FRAME_RATE 25

#define RAW_W 640
#define RAW_H 480

#define DETECTION_VENC_CHN 2

#define ALIGN(base, align)  (((base) + (align) - 1) & ~((align) - 1))
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

uint8_t g_smart_codec_on = 0;
#define DEBUG_PRINTF_FRAME_TYPE 0

typedef struct strm_para_s
{
    int strm_id;
    MI_U32 u32InputChn;
    MI_U32 u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_Stream_t vencStream;
    MI_VENC_Pack_t stPack;
    MI_U32 enInput;
}STRM_PARAM_S;

STRM_PARAM_S g_strm_param[STRM_ID_VIDEO_LAST] = {0};

int getTime_delay_ms(struct timespec *pTimeSpecPre,int delay_ms)
{
    int delayReached = 0;
    struct timespec ts;
    float time = 0;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time = ts.tv_sec*1000+ts.tv_nsec/1000000-pTimeSpecPre->tv_sec*1000-pTimeSpecPre->tv_nsec/1000000;
    if (fabs(time) >= delay_ms)
    {
        delayReached = 1;
        pTimeSpecPre->tv_sec = ts.tv_sec;
        pTimeSpecPre->tv_nsec = ts.tv_nsec;
    }
    return delayReached;
}

int GetVpeChn(TP_STRM_ID_E strm_id)
{
    return g_strm_param[strm_id].u32InputChn;
}

int GetVpePort(TP_STRM_ID_E strm_id)
{
    return g_strm_param[strm_id].u32InputPort;
}

int GetVencChn(TP_STRM_ID_E strm_id)
{
    return g_strm_param[strm_id].vencChn;
}

int GetEnInput(TP_STRM_ID_E strm_id)
{
    return g_strm_param[strm_id].enInput;
}

static void StrmParamInit(void)
{
    memset(g_strm_param, 0, sizeof(STRM_PARAM_S) * STRM_ID_VIDEO_LAST);

    g_strm_param[STRM_ID_MAIN].strm_id = STRM_ID_MAIN;
    g_strm_param[STRM_ID_MAIN].u32InputChn =0;
#ifdef TARGET_CHIP_I6
    g_strm_param[STRM_ID_MAIN].u32InputPort = 1;
#else
    g_strm_param[STRM_ID_MAIN].u32InputPort = 0;
#endif
    g_strm_param[STRM_ID_MAIN].vencChn = 0;
    g_strm_param[STRM_ID_MAIN].enInput = ST_Sys_Input_VPE;

    g_strm_param[STRM_ID_MINOR].strm_id = STRM_ID_MINOR;
    g_strm_param[STRM_ID_MINOR].u32InputChn = 0;
#ifdef TARGET_CHIP_I6
    g_strm_param[STRM_ID_MINOR].u32InputPort = 2;
    g_strm_param[STRM_ID_MINOR].enInput = ST_Sys_Input_DIVP;
#else
    g_strm_param[STRM_ID_MINOR].u32InputPort = 1;
    g_strm_param[STRM_ID_MINOR].enInput = ST_Sys_Input_VPE;
#endif
    g_strm_param[STRM_ID_MINOR].vencChn = 1;

#ifdef THIRD_STREAM_ENABLE
    g_strm_param[STRM_ID_THIRD].strm_id = STRM_ID_THIRD;
    g_strm_param[STRM_ID_THIRD].u32InputChn = 0;
    g_strm_param[STRM_ID_THIRD].u32InputPort = 3;
    g_strm_param[STRM_ID_THIRD].vencChn = 3;
#endif
}

static void StrmParamDeInit(void)
{
    memset(g_strm_param, 0, sizeof(STRM_PARAM_S)*STRM_ID_VIDEO_LAST);
}


/******************************************************************************
 * 函数名称:    tp_video_get_stream
 * 函数描述:    获取编码流
 * 输    入:    strm_id: 码流通道ID
 *              encode_type: 编码类型
 * 输    出:    pTpSteam: 编码信息结构体指针
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_get_stream(TP_STRM_ID_E strm_id,TP_ENCODE_TYPE_E encode_type, TP_VENC_STRM_S* pTpStream)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_VENC_Stream_t *pStStream = NULL;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn ;
    int writeLen = 0;

    vencChn = GetVencChn(strm_id);

    pStStream = &g_strm_param[strm_id].vencStream;
    memset(pStStream, 0, sizeof(MI_VENC_Stream_t));
    pStStream->pstPack = &g_strm_param[strm_id].stPack;
    memset(pStStream->pstPack, 0, sizeof(MI_VENC_Pack_t));
    pStStream->u32PackCount = 1;
    s32Ret = MI_VENC_Query(vencChn, &stStat);

    if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
    {
        pStStream->pstPack = NULL;
        return -1;
    }

    s32Ret = MI_VENC_GetStream(vencChn, pStStream, 0);

    if (MI_SUCCESS != s32Ret)
    {
        pStStream->pstPack = NULL;
        return -1;
    }

    pTpStream->pack[0].addr = pStStream->pstPack[0].pu8Addr;
    pTpStream->pack[0].len = pStStream->pstPack[0].u32Len;
    pTpStream->pack_cnt = 1;
    pTpStream->len = pStStream->pstPack[0].u32Len;
    pTpStream->pts = pStStream->pstPack[0].u64PTS;

    if (TP_ENCODE_TYPE_VIDEO_H265 ==  encode_type)
    {
#if DEBUG_PRINTF_FRAME_TYPE
		if (STRM_ID_MAIN == strm_id)
		{
			printf("pStStream->pstPack[0].stDataType.eH265EType %d\t"
				"pStStream->stH265Info.eRefType %d\n",
				pStStream->pstPack[0].stDataType.eH265EType,
				pStStream->stH265Info.eRefType);
		}
#endif
        switch(pStStream->stH265Info.eRefType)
        {
            case E_MI_VENC_BASE_IDR:
                pTpStream->frame_type = TP_FRAME_TYPE_I;
#ifdef VIDEO_AVBR_ENABLE
                if (STRM_ID_MAIN == strm_id)
                {
                    pTpStream->is_smart_codec = g_smart_codec_on;
                }
#endif
                break;
            case E_MI_VENC_BASE_P_REFTOIDR:
            case E_MI_VENC_BASE_P_REFBYBASE:
            case E_MI_VENC_BASE_P_REFBYENHANCE:
#ifdef VIDEO_AVBR_ENABLE
                /* 在不开启智能编码时，BASE层的P帧仍归类到普通P帧 */
                if (STRM_ID_MAIN == strm_id && g_smart_codec_on)
                {
                    pTpStream->frame_type = TP_FRAME_TYPE_VIRTUAL_I;
                    break;
                }
#endif
            case E_MI_VENC_ENHANCE_P_REFBYENHANCE:
            case E_MI_VENC_ENHANCE_P_NOTFORREF:
                pTpStream->frame_type = TP_FRAME_TYPE_P;
                break;
            default:
                break;
        }
#if DEBUG_PRINTF_FRAME_TYPE
		if (STRM_ID_MAIN == strm_id)
		{
			printf("pTpStream->frame_type %d\n", pTpStream->frame_type);
		}
#endif
    }
    else
    {
#if DEBUG_PRINTF_FRAME_TYPE
		if (STRM_ID_MAIN == strm_id)
		{
			printf("pStStream->pstPack[0].stDataType.eH264EType %d\t"
				"pStStream->stH264Info.eRefType %d\n",
				pStStream->pstPack[0].stDataType.eH264EType,
				pStStream->stH264Info.eRefType);
		}
#endif
        switch(pStStream->stH264Info.eRefType)
        {
            case E_MI_VENC_BASE_IDR:
                pTpStream->frame_type = TP_FRAME_TYPE_I;
#ifdef VIDEO_AVBR_ENABLE
                if (STRM_ID_MAIN == strm_id)
                {
                    pTpStream->is_smart_codec = g_smart_codec_on;
                }
#endif
                break;
            case E_MI_VENC_BASE_P_REFTOIDR:
            case E_MI_VENC_BASE_P_REFBYBASE:
            case E_MI_VENC_BASE_P_REFBYENHANCE:
#ifdef VIDEO_AVBR_ENABLE
                /* 在不开启智能编码时，BASE层的P帧仍归类到普通P帧 */
                if (STRM_ID_MAIN == strm_id && g_smart_codec_on)
                {
                    pTpStream->frame_type = TP_FRAME_TYPE_VIRTUAL_I;
                    break;
                }
#endif
            case E_MI_VENC_ENHANCE_P_REFBYENHANCE:
            case E_MI_VENC_ENHANCE_P_NOTFORREF:
                pTpStream->frame_type = TP_FRAME_TYPE_P;
                break;
            default:
                break;
        }
#if DEBUG_PRINTF_FRAME_TYPE
		if (STRM_ID_MAIN == strm_id)
		{
			printf("pTpStream->frame_type %d\n", pTpStream->frame_type);
		}
#endif
    }

    return 0;
}

/******************************************************************************
 * 函数名称:    tp_video_ReleaseStream
 * 函数描述:    释放编码流
 * 输    入:    strm_id: 码流通道ID
 *              pTpSteam: 编码信息结构体指针
 * 输    出:    N/A
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_release_stream(TP_STRM_ID_E strm_id, TP_VENC_STRM_S* pTpStream)
{
    MI_VENC_CHN vencChn;
    vencChn = GetVencChn(strm_id);

    MI_VENC_Stream_t* pStStream = &g_strm_param[strm_id].vencStream;

    if (pStStream->pstPack != NULL)
    {
        MI_VENC_ReleaseStream(vencChn, pStStream);
        memset(&g_strm_param[strm_id].stPack, 0, sizeof(MI_VENC_Pack_t));
        memset(pStStream, 0, sizeof(MI_VENC_Stream_t));
        memset(pTpStream, 0, sizeof(TP_VENC_STRM_S));
    }

    return 0;
}

/******************************************************************************
 * 函数名称:    tp_video_init
 * 函数描述:    VIDEO模块初始化
 * 输    入:    strm_num:   码流数量
 *              pAttr: 码流属性
 * 输    出:    N/A
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_init(int strm_num, TP_STREAM_ATTR_S pAttr[])
{
    int tt = -5;
    printf("%d\n",tt);

    int ret = -1;

    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;

    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
#ifdef SENSOR_HDR_MODE
     MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_DOL;
     MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_DOL;
#else
     MI_VIF_HDRType_e eVifHdrType = E_MI_VIF_HDR_TYPE_OFF;
     MI_VPE_HDRType_e eVpeHdrType = E_MI_VPE_HDR_TYPE_OFF;
#endif
    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U8 u8ChocieRes =0;
    MI_U32 maxFrameRate = MAX_FRAME_RATE;

    /* 支持的最大宽高为主码流最大宽高 */
    u32CapWidth = pAttr[STRM_ID_MAIN].strm_max_res.res_w;
    u32CapHeight = pAttr[STRM_ID_MAIN].strm_max_res.res_h;

    if (u32CapWidth <= 0 || u32CapHeight <= 0)
    {
        printf("max width or height is zero!!!\n");
        return -1;
    }

    StrmParamInit();

    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

#ifdef TARGET_CHIP_I6
    /* 固定给SDK使用的内存，防止切换编码因为内存碎片，异常退出 */
    STCHECKRESULT(ST_Fix_mem(u32CapWidth, u32CapHeight));
#endif

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));
    memset(&stSnrPlane0Info, 0x0, sizeof(MI_SNR_PlaneInfo_t));
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));
#ifdef SENSOR_HDR_MODE
    MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, TRUE);
#else
    MI_SNR_SetPlaneMode(E_MI_SNR_PAD_ID_0, FALSE);
#endif

    MI_SNR_QueryResCount(E_MI_SNR_PAD_ID_0, &u32ResCount);
    for(u8ResIndex = 0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(E_MI_SNR_PAD_ID_0, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
            u8ResIndex,
            stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
            stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
            stRes.u32MaxFps,stRes.u32MinFps,
            stRes.strResDesc);
    }

    printf("choice which resolution use, cnt %d\n", u32ResCount);

    if (u32ResCount <= 0)
    {
        printf("sensor resolution count error: %d\n", u32ResCount);
        return -1;
    }

    u8ChocieRes = u32ResCount - 1;

    printf("You select %d res\n", u8ChocieRes);

    MI_SNR_SetRes(E_MI_SNR_PAD_ID_0,u8ChocieRes);
    MI_SNR_Enable(E_MI_SNR_PAD_ID_0);

    MI_SNR_SetFps(E_MI_SNR_PAD_ID_0, maxFrameRate);

    MI_SNR_GetPadInfo(E_MI_SNR_PAD_ID_0, &stPad0Info);
    MI_SNR_GetPlaneInfo(E_MI_SNR_PAD_ID_0, 0, &stSnrPlane0Info);

    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);

    STCHECKRESULT(ST_Vif_EnableDev(0, eVifHdrType, &stPad0Info));

    ST_VIF_PortInfo_T stVifPortInfoInfo;
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    /* STCHECKRESULT(ST_Vif_StartPort(0, 0, 0)); */

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    ST_VPE_ChannelInfo_T stVpeChannelInfo;
    ST_Sys_BindInfo_T stBindInfo;

    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_T));
    stVpeChannelInfo.u16VpeMaxW = u32CapWidth;
    stVpeChannelInfo.u16VpeMaxH = u32CapHeight;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 0;
    stVpeChannelInfo.u16VpeCropH = 0;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = ePixFormat;//E_MI_SYS_PIXEL_FRAME_RGB_BAYER_12BPP_GR;
    stVpeChannelInfo.eHDRtype = eVpeHdrType;

    stVpeChannelInfo.bRotation = 0;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));
    /* STCHECKRESULT(ST_Vif_StartPort(0, 0, 0)); */

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
#ifdef TARGET_CHIP_I6
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif

    stBindInfo.u32SrcFrmrate = MAX_FRAME_RATE;
    stBindInfo.u32DstFrmrate = MAX_FRAME_RATE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, 0, 0));

#ifndef TARGET_CHIP_I6
//    tp_video_start_algo_stream();
#endif

#ifdef IQSERVER_SUPPORT
    MI_IQSERVER_Open(1920, 1080, 0);
#endif

    return 0;
}

/******************************************************************************
 * 函数名称:    tp_video_deinit
 * 函数描述:    VIDEO模块去初始化
 * 输    入:    strm_num: 码流数量
 * 输    出:    N/A
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_deinit(int strm_num)
{
    ST_Sys_BindInfo_T stBindInfo;
    FILE *fp = NULL;

#ifdef IQSERVER_SUPPORT
    MI_IQSERVER_Close();
#endif

#ifndef TARGET_CHIP_I6
//    tp_video_stop_algo_stream();
#endif

    /************************************************
    Step1:  unbind VIF->VPE
    *************************************************/
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    /************************************************
    Step2:  destory VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));

    MI_SNR_Disable(E_MI_SNR_PAD_ID_0);

#ifdef TARGET_CHIP_I6
    ST_Unfix_mem();
#endif

    /************************************************
    Step4:  destory SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Exit());

    StrmParamDeInit();

    fp = fopen("/tmp/video_stopped", "w");
    if (fp)
    {
        close(fp);
    }

    return 0;
}

/******************************************************************************
 * 函数名称:    tp_video_stop
 * 函数描述:    关闭码流通道
 * 输    入:    strm_id: 码流通道ID
 * 输    出:    N/A
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_stop(TP_STRM_ID_E strm_id)
{
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 VpeChn = 0;
    MI_U32 VpePort = 0;
    MI_VENC_CHN VencChn = 0;

    ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;
    MI_U32 EnInput = 0;

    VpeChn = GetVpeChn(strm_id);
    VpePort = GetVpePort(strm_id);
    VencChn = GetVencChn(strm_id);
    EnInput = GetEnInput(strm_id);
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

    /************************************************
    Step3:  stop VENC
    *************************************************/
    STCHECKRESULT(ST_Venc_StopChannel(VencChn));

    if (ST_Sys_Input_DIVP == EnInput)
    {
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = VpeChn;
        stBindInfo.stSrcChnPort.u32PortId = VpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_SCALE;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = MAX_FRAME_RATE;
        stBindInfo.u32DstFrmrate = MAX_FRAME_RATE;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = DIVP_CHN_FOR_SCALE;
        stBindInfo.stSrcChnPort.u32PortId = 0;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = MAX_FRAME_RATE;
        stBindInfo.u32DstFrmrate = MAX_FRAME_RATE;
        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
        STCHECKRESULT(ST_Venc_StopChannel(VencChn));
        STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
        ExecFunc(MI_DIVP_StopChn(DIVP_CHN_FOR_SCALE), MI_SUCCESS);
        ExecFunc(MI_DIVP_DestroyChn(DIVP_CHN_FOR_SCALE), MI_SUCCESS);
        STCHECKRESULT(ST_Vpe_StopPort(VpeChn, VpePort));
    }
    else
    {
        /************************************************
        Step2:  unbind and stop VPE port
        *************************************************/
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = VpeChn;
        stBindInfo.stSrcChnPort.u32PortId = VpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

        STCHECKRESULT(ST_Vpe_StopPort(VpeChn, VpePort));

        /************************************************
        Step1:  destory VENC
        *************************************************/
        STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));
    }

    return 0;
}

/******************************************************************************
 * 函数名称:    tp_video_Start
 * 函数描述:    启动码流通道
 * 输    入:    strm_id: 码流通道ID
 * 输    出:    N/A
 * 返 回 值:    参见TP_RET_E
*******************************************************************************/
int tp_video_start(TP_STRM_ID_E strm_id, TP_STREAM_ATTR_S* pAttr)
{
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 VpeChn = 0;
    MI_U32 VpePort = 0;
    MI_VENC_CHN VencChn = 0;
    MI_S32 s32Ret = MI_SUCCESS;
    ST_VPE_PortInfo_T stVpePortInfo;
    MI_U32 u32DevId = -1;

    MI_U32 framerate = 0;
    MI_U32 bitrate = 0;
    MI_U32 gopfactor = 0;
    MI_U32 gop = 0;
    MI_U32 EnInput = 0;

    /************************************************
    Step4:  init VENC
    *************************************************/
    MI_VENC_ChnAttr_t stChnAttr;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

    framerate = pAttr->framerate;
    bitrate = pAttr->bitrate;
    gopfactor = pAttr->gopfactor;
    gop = framerate * gopfactor;

    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_VENC;

#ifdef TARGET_CHIP_I6
    if (STRM_ID_MAIN == strm_id)
    {
        if (TP_ENCODE_TYPE_VIDEO_H264 == pAttr->encode_type)
            stChnAttr.stVeAttr.stAttrH264e.u32BufSize = pAttr[strm_id].strm_max_res.res_w * pAttr[strm_id].strm_max_res.res_h * 0.4;
        else if (TP_ENCODE_TYPE_VIDEO_H265 == pAttr->encode_type)
            stChnAttr.stVeAttr.stAttrH265e.u32BufSize = pAttr[strm_id].strm_max_res.res_w * pAttr[strm_id].strm_max_res.res_h * 0.4;
    }
#endif

    switch (pAttr->encode_type)
    {
        case TP_ENCODE_TYPE_VIDEO_H264:
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = pAttr->strm_res.res_h;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = pAttr->strm_res.res_h;
            //stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 0;
            //stChnAttr.stVeAttr.stAttrH264e.u32RefNum = 1;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            /* h264 1代表是main profile，0代表baseline */
            stChnAttr.stVeAttr.stAttrH264e.u32Profile = 1;

            if(TP_BITRATE_TYPE_CBR == pAttr->bitrate_type)
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = bitrate;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = gop;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = framerate;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            }
#ifdef VIDEO_AVBR_ENABLE
            else if(TP_BITRATE_TYPE_AVBR == pAttr->bitrate_type)
            {
                if (TP_SMART_CODEC_CBR == pAttr->smart_codec_type)
                {
                    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = bitrate;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = framerate * 12;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = framerate;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 12;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 48;
                }
                else if (TP_SMART_CODEC_VBR == pAttr->smart_codec_type)
                {
                    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = bitrate;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = framerate * 12;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = framerate;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 12;
                    stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 48;
                }
                else
                {
                    SLOG(SLOG_ERROR, "set strm id %d unknow smart encode type\n", strm_id);
                    return MI_FAIL;
                }
            }
#endif
            else
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = bitrate;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = gop;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = framerate;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 12;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 48;
            }

            break;
        case TP_ENCODE_TYPE_VIDEO_H265:
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = pAttr->strm_res.res_h;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = pAttr->strm_res.res_h;
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            /* h265 0代表main profile，目前只支持0 */
            stChnAttr.stVeAttr.stAttrH265e.u32Profile = 0;

            if(TP_BITRATE_TYPE_CBR == pAttr->bitrate_type)
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = bitrate;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = framerate;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = gop;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            }
#ifdef VIDEO_AVBR_ENABLE
            else if(TP_BITRATE_TYPE_AVBR == pAttr->bitrate_type)
            {
                if (TP_SMART_CODEC_CBR == pAttr->smart_codec_type)
                {
                    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitrate;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = framerate * 12;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = framerate;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 12;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
                }
                else if (TP_SMART_CODEC_VBR == pAttr->smart_codec_type)
                {
                    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitrate;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = framerate * 12;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = framerate;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 12;
                    stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
                }
                else
                {
                    SLOG(SLOG_ERROR, "set strm id %d unknow smart encode type\n", strm_id);
                    return MI_FAIL;
                }
            }
#endif
            else
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitrate;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = gop;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = framerate;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 12;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            }

            break;
        case TP_ENCODE_TYPE_SNAPSHOT_JPEG:
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = pAttr->strm_res.res_h;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = pAttr->strm_res.res_w;
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = pAttr->strm_res.res_h;
            break;
        default:
            printf("encode_type not supported");
            break;
    }

    if (stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_VENC)
    {
        return -1;
    }

    VpeChn = GetVpeChn(strm_id);
    VpePort = GetVpePort(strm_id);
    VencChn = GetVencChn(strm_id);
    EnInput = GetEnInput(strm_id);
    
    STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));

    /************************************************
    Step3:  start VPE
    *************************************************/
    memset(&stVpePortInfo, 0, sizeof(ST_VPE_PortInfo_T));

    stVpePortInfo.DepVpeChannel = VpeChn;
    if (ST_Sys_Input_DIVP == EnInput && STRM_ID_MINOR == strm_id)
    {
        /* 直接使用子码流最大的分辨率 */
        stVpePortInfo.u16OutputWidth = 704;
        stVpePortInfo.u16OutputHeight = 576;
    }
    else
    {
        stVpePortInfo.u16OutputWidth = pAttr->strm_res.res_w;
        stVpePortInfo.u16OutputHeight = pAttr->strm_res.res_h;
    }
    stVpePortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVpePortInfo.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;

#ifdef TARGET_CHIP_I6
    if (STRM_ID_MINOR == strm_id)
    {
        STCHECKRESULT(ST_Vpe_StartPort(VpePort, &stVpePortInfo, 3));
    }
    else
    {
        STCHECKRESULT(ST_Vpe_StartPort(VpePort, &stVpePortInfo, 5));
    }
#else
    STCHECKRESULT(ST_Vpe_StartPort(VpePort, &stVpePortInfo, 5));
#endif

    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);

    if (ST_Sys_Input_DIVP == EnInput)
    {
        MI_DIVP_ChnAttr_t stDivpChnAttr;
        MI_DIVP_OutputPortAttr_t stOutputPortAttr;
        memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
        stDivpChnAttr.bHorMirror            = FALSE;
        stDivpChnAttr.bVerMirror            = FALSE;
        stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
        stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X       = 0;
        stDivpChnAttr.stCropRect.u16Y       = 0;
        stDivpChnAttr.stCropRect.u16Width   = 0;
        stDivpChnAttr.stCropRect.u16Height  = 0;
        stDivpChnAttr.u32MaxWidth           = pAttr->strm_res.res_w;
        stDivpChnAttr.u32MaxHeight          = pAttr->strm_res.res_h;
        ExecFunc(MI_DIVP_CreateChn(DIVP_CHN_FOR_SCALE, &stDivpChnAttr), MI_SUCCESS);
        ExecFunc(MI_DIVP_StartChn(DIVP_CHN_FOR_SCALE), MI_SUCCESS);
        memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
        stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
        stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stOutputPortAttr.u32Width           = pAttr->strm_res.res_w;
        stOutputPortAttr.u32Height          = pAttr->strm_res.res_h;
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DIVP_CHN_FOR_SCALE, &stOutputPortAttr));

        // bind VPE to divp
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = VpeChn;
        stBindInfo.stSrcChnPort.u32PortId = VpePort;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = DIVP_CHN_FOR_SCALE;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = MAX_FRAME_RATE;
        stBindInfo.u32DstFrmrate = framerate;
#ifdef TARGET_CHIP_I6
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
        STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&(stBindInfo.stDstChnPort), 0, 2));
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

        // bind divp to venc
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = DIVP_CHN_FOR_SCALE;
        stBindInfo.stSrcChnPort.u32PortId = 0;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = framerate;
        stBindInfo.u32DstFrmrate = framerate;
#ifdef TARGET_CHIP_I6
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
    else
    {
        // vpe port 2 can not attach osd, so use divp
        memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = VpeChn;
        stBindInfo.stSrcChnPort.u32PortId = VpePort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = VencChn;
        stBindInfo.stDstChnPort.u32PortId = 0;

        stBindInfo.u32SrcFrmrate = MAX_FRAME_RATE;
        stBindInfo.u32DstFrmrate = framerate;
#ifdef TARGET_CHIP_I6
        if (STRM_ID_MAIN == strm_id)
        {
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
            stBindInfo.u32BindParam = pAttr->strm_res.res_h;
        }
        else
        {
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        }
#endif
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

#ifdef VIDEO_AVBR_ENABLE
	if (STRM_ID_MAIN == strm_id)
	{
		MI_VENC_ParamRef_t lrt_param ={0};
		if(TP_BITRATE_TYPE_AVBR == pAttr->bitrate_type)
		{
			s32Ret = MI_VENC_GetRefParam(VencChn, &lrt_param);
			if(MI_SUCCESS != s32Ret)
			{
				printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
				goto avbr_out;
			}
	
			lrt_param.u32Base = 1;
			lrt_param.u32Enhance = framerate * 2 - 1;
			lrt_param.bEnablePred = FALSE;
	
			s32Ret = MI_VENC_SetRefParam(VencChn, &lrt_param);
			if(MI_SUCCESS != s32Ret)
			{
				printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
				goto avbr_out;
			}

			g_smart_codec_on = 1;
		}
		else
		{
			s32Ret = MI_VENC_GetRefParam(VencChn, &lrt_param);
			if(MI_SUCCESS != s32Ret)
			{
				printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
				goto avbr_out;
			}

			lrt_param.u32Base = 1;
			lrt_param.u32Enhance = 0;
			lrt_param.bEnablePred = FALSE;
	
			s32Ret = MI_VENC_SetRefParam(VencChn, &lrt_param);
			if(MI_SUCCESS != s32Ret)
			{
				printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
				goto avbr_out;
			}

			g_smart_codec_on = 0;

			s32Ret = MI_VENC_GetRefParam(VencChn, &lrt_param);
			if(MI_SUCCESS != s32Ret)
			{
				printf("%s %d, MI_VENC_SetRefParam error, %X\n", __func__, __LINE__, s32Ret);
				goto avbr_out;
			}

			printf("u32Base %d, u32Enhance %d, bEnablePred %d\n",
				lrt_param.u32Base, lrt_param.u32Enhance, lrt_param.bEnablePred);
		}

	}

avbr_out:
#endif

	if (stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H264E)
	{
		MI_VENC_ParamH264Vui_t vui_264 = {0};
		s32Ret = MI_VENC_GetH264Vui(VencChn, &vui_264);
		if (s32Ret == MI_SUCCESS)
		{
			vui_264.stVuiTimeInfo.u8TimingInfoPresentFlag = 1;
			vui_264.stVuiTimeInfo.u8FixedFrameRateFlag = 1;
			vui_264.stVuiTimeInfo.u32NumUnitsInTick = 1;
			vui_264.stVuiTimeInfo.u32TimeScale = framerate * 2;
			vui_264.stVuiVideoSignal.u8ColourDescriptionPresentFlag = 0;
			s32Ret = MI_VENC_SetH264Vui(VencChn, &vui_264);
		}
	}
	else if (stChnAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_H265E)
	{
		MI_VENC_ParamH265Vui_t vui_265 = {0};
		s32Ret = MI_VENC_GetH265Vui(VencChn, &vui_265);
		if (s32Ret == MI_SUCCESS)
		{
			vui_265.stVuiTimeInfo.u8TimingInfoPresentFlag = 1;
			//vui_265.stVuiTimeInfo.u8FixedFrameRateFlag = 1;
			vui_265.stVuiTimeInfo.u32NumUnitsInTick = 1;
			vui_265.stVuiTimeInfo.u32TimeScale = framerate;
			vui_265.stVuiVideoSignal.u8ColourDescriptionPresentFlag = 0;
			s32Ret = MI_VENC_SetH265Vui(VencChn, &vui_265);
		}
	}

    /************************************************
    Step3:  start VENC
    *************************************************/
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    STCHECKRESULT(ST_Venc_StartChannel(VencChn));

    /* start ROI */
    //VENCROIAttrSet(VencChn, (TP_STREAM_ROI_S*)&(pAttr->roi_info));

    return 0;
}

int tp_video_getFD(TP_STRM_ID_E strm_id)
{
    int vencChn;
    vencChn = GetVencChn(strm_id);

    return MI_VENC_GetFd(vencChn);
}

int radom_video_param(TP_STRM_ID_E strm_id, TP_STREAM_ATTR_S *pAttr)
{
    //主码流
    static STRM_RES_S main_resolution_list[] = {{1280,960},{1280,720},{1920,1080}};
    static TP_ENCODE_TYPE_E main_encode_type_list[] = {TP_ENCODE_TYPE_VIDEO_H264,TP_ENCODE_TYPE_VIDEO_H265};
    static uint32_t main_frame_rate_list[] = {1,5,10,15,20,25};
    static TP_BITRATE_TYPE main_bitrate_type_list[] = {TP_BITRATE_TYPE_VBR,TP_BITRATE_TYPE_CBR};
    static uint32_t main_bitrate_limit_list[] = {4096,3072,2048,1024,768,512,384,256};
    //子码流
    static STRM_RES_S minor_resolution_list[] = {{704,576},{640,480},{352,288},{320,240}};
    static TP_ENCODE_TYPE_E minor_encode_type_list[] = {TP_ENCODE_TYPE_VIDEO_H264,TP_ENCODE_TYPE_VIDEO_H265};
    static uint32_t minor_frame_rate_list[] = {1,5,10,15,20,25};
    static TP_BITRATE_TYPE minor_bitrate_type_list[] = {TP_BITRATE_TYPE_VBR,TP_BITRATE_TYPE_CBR};
    static uint32_t minor_bitrate_limit_list[] = {1024,768,512,384,256,192,128,96,64};
    static int testCase = 0;
    int randomNum = 0;

    switch(testCase)
    {
        case 0:
            if(strm_id == STRM_ID_MAIN) {
                randomNum = random() % ARRAY_SIZE(main_resolution_list);
                pAttr->strm_res.res_w = main_resolution_list[randomNum].res_w;
                pAttr->strm_res.res_h = main_resolution_list[randomNum].res_h;
            }else{
                randomNum = random() % ARRAY_SIZE(minor_resolution_list);
                pAttr->strm_res.res_w = minor_resolution_list[randomNum].res_w;
                pAttr->strm_res.res_h = minor_resolution_list[randomNum].res_h;
            }
            testCase = 1;
            printf("%s[resolution]: strm_id = %d, res_w = %d, res_h = %d\n",__FUNCTION__, strm_id, pAttr->strm_res.res_w, pAttr->strm_res.res_h);
            break;

        case 1:
            if(strm_id == STRM_ID_MAIN) {
                randomNum = random() % ARRAY_SIZE(main_encode_type_list);
                pAttr->encode_type = main_encode_type_list[randomNum];
            }else{
                randomNum = random() % ARRAY_SIZE(minor_encode_type_list);
                pAttr->encode_type = minor_encode_type_list[randomNum];
            }
            testCase = 2;
            printf("%s[encode_type]: strm_id = %d, encode_type = %s\n",__FUNCTION__, strm_id, (pAttr->encode_type == TP_ENCODE_TYPE_VIDEO_H264)?"H264":"H265");
            break;

        case 2:
            if(strm_id == STRM_ID_MAIN) {
                randomNum = random() % ARRAY_SIZE(main_frame_rate_list);
                pAttr->framerate = main_frame_rate_list[randomNum];
            }else{
                randomNum = random() % ARRAY_SIZE(minor_frame_rate_list);
                pAttr->framerate = minor_frame_rate_list[randomNum];
            }
            testCase = 3;
            printf("%s[frame_rate]: strm_id = %d, frame_rate = %d\n",__FUNCTION__, strm_id, pAttr->framerate);
            break;

        case 3:
            if(strm_id == STRM_ID_MAIN) {
                randomNum = random() % ARRAY_SIZE(main_bitrate_type_list);
                pAttr->bitrate_type = main_bitrate_type_list[randomNum];
            }else{
                randomNum = random() % ARRAY_SIZE(minor_bitrate_type_list);
                pAttr->bitrate_type = minor_bitrate_type_list[randomNum];
            }
            testCase = 4;
            printf("%s[bitrate_type]: strm_id = %d, bitrate_type = %s\n",__FUNCTION__, strm_id, (pAttr->bitrate_type==TP_BITRATE_TYPE_VBR)?"VBR":"CBR");
            break;
            
        case 4:
            if(strm_id == STRM_ID_MAIN) {
                randomNum = random() % ARRAY_SIZE(main_bitrate_limit_list);
                pAttr->bitrate = main_bitrate_limit_list[randomNum]*1000;
            }else{
                randomNum = random() % ARRAY_SIZE(minor_bitrate_limit_list);
                pAttr->bitrate = minor_bitrate_limit_list[randomNum]*1000;
            }
            testCase = 0;
            printf("%s[bitrate]: strm_id = %d, bitrate = %d\n",__FUNCTION__, strm_id, pAttr->bitrate);
            break;
    }

     return 0;
}

void* video_stream_proc(void *argv)
{
    struct timeval TimeoutVal;
    MI_S32 vencMainFd = -1;
    MI_S32 vencMinorFd = -1;
    MI_S32 vencFdMax = -1;
    MI_S32 s32Ret;
    fd_set read_fds;
    TP_STREAM_ATTR_S *pStreamAttr = (TP_STREAM_ATTR_S *)argv;
    TP_VENC_STRM_S mainTpStream;
    TP_VENC_STRM_S minorTpStream;

    FD_ZERO(&read_fds);
    vencMainFd = MI_VENC_GetFd(GetVencChn(STRM_ID_MAIN));
    FD_SET(vencMainFd, &read_fds);
    vencMinorFd = MI_VENC_GetFd(GetVencChn(STRM_ID_MINOR));
    FD_SET(vencMinorFd, &read_fds);

    TimeoutVal.tv_sec  = 0;
    TimeoutVal.tv_usec = 300*1000;//select() wait for 300ms
    vencFdMax = (vencMainFd < vencMinorFd) ? vencMinorFd : vencMainFd;

    s32Ret = select(vencFdMax + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        printf("select fail!\n");
        usleep(1000 * 1);
        return NULL;
    }
    else if (0 == s32Ret)
    {
        //printf("time out!\n");
        usleep(1000 * 1);
        return NULL;
    }

    if(FD_ISSET(vencMainFd, &read_fds))
    {
        memset(&mainTpStream, 0, sizeof(mainTpStream));
        tp_video_get_stream(STRM_ID_MAIN, pStreamAttr[STRM_ID_MAIN].encode_type, &mainTpStream);
        ;;
        tp_video_release_stream(STRM_ID_MAIN, &mainTpStream);
    }

    if(FD_ISSET(vencMainFd, &read_fds))
    {
        memset(&minorTpStream, 0, sizeof(minorTpStream));
        tp_video_get_stream(STRM_ID_MINOR, pStreamAttr[STRM_ID_MINOR].encode_type, &minorTpStream);
        ;;
        tp_video_release_stream(STRM_ID_MINOR, &minorTpStream);
    }

    return 0;
}

int cus02_test_main()
{
    int index = 0;
    int iVideoParamRandTest = 0;
    struct timespec timeSpecPre;
    TP_STREAM_ATTR_S attr[2];
    TP_STRM_ID_E strm_id[10] = {STRM_ID_MAIN,STRM_ID_MAIN,STRM_ID_MAIN,STRM_ID_MAIN,STRM_ID_MAIN,
                                STRM_ID_MINOR,STRM_ID_MINOR,STRM_ID_MINOR,STRM_ID_MINOR,STRM_ID_MINOR};
    TP_STRM_ID_E strm_cur = STRM_ID_MAIN;
    TP_STRM_ID_E strm_other = STRM_ID_MINOR;

    attr[STRM_ID_MAIN].bitrate = 4096*1000;
    attr[STRM_ID_MAIN].bitrate_type = TP_BITRATE_TYPE_CBR;
    attr[STRM_ID_MAIN].encode_type = TP_ENCODE_TYPE_VIDEO_H265;
    attr[STRM_ID_MAIN].framerate = 25;
    attr[STRM_ID_MAIN].gopfactor = 2;
    attr[STRM_ID_MAIN].smart_codec_type = TP_SMART_CODEC_CBR;   //ltr
    attr[STRM_ID_MAIN].strm_max_res.res_w = 1920;
    attr[STRM_ID_MAIN].strm_max_res.res_h = 1080;
    attr[STRM_ID_MAIN].strm_res.res_w = 1920;
    attr[STRM_ID_MAIN].strm_res.res_h = 1080;

    attr[STRM_ID_MINOR].bitrate = 2048*1000;
    attr[STRM_ID_MINOR].bitrate_type = TP_BITRATE_TYPE_CBR;
    attr[STRM_ID_MINOR].encode_type = TP_ENCODE_TYPE_VIDEO_H265;
    attr[STRM_ID_MINOR].framerate = 25;
    attr[STRM_ID_MINOR].gopfactor = 2;
    attr[STRM_ID_MINOR].smart_codec_type = TP_SMART_CODEC_CBR;   //ltr
    attr[STRM_ID_MINOR].strm_max_res.res_w = 704;
    attr[STRM_ID_MINOR].strm_max_res.res_h = 576;
    attr[STRM_ID_MINOR].strm_res.res_w = 704;
    attr[STRM_ID_MINOR].strm_res.res_h = 576;

    tp_video_init(2, attr);

    tp_video_start(STRM_ID_MAIN, &(attr[STRM_ID_MAIN]));
    tp_video_start(STRM_ID_MINOR, &(attr[STRM_ID_MINOR]));
    printf("video init ok!\n");

    printf("test video param random ? 0-no, 1-yes\n");
    scanf("%d", &iVideoParamRandTest);

    srandom();
    memset(&timeSpecPre, 0, sizeof(timeSpecPre));
    while(1)
    {
        // for STRM_ID_MAIN and STRM_ID_MINOR
        video_stream_proc(attr);

        if(iVideoParamRandTest)
        {
            // delay 3s to change video param
            if(getTime_delay_ms(&timeSpecPre, 3*1000))
            {
                strm_cur = strm_id[index];
                strm_other = (strm_cur == STRM_ID_MAIN)?STRM_ID_MINOR:STRM_ID_MAIN;
                if(random() % 2)
                {
                    printf("stop -> start %d first!\n", strm_cur);
                    tp_video_stop(strm_cur);
                    radom_video_param(strm_cur, &(attr[strm_cur]));
                    tp_video_start(strm_cur, &(attr[strm_cur]));
                    printf("stop -> start %d second!\n", strm_other);
                    tp_video_stop(strm_other);
                    tp_video_start(strm_other,&(attr[strm_other]));
                }else{
                    printf("stop -> start %d first!\n", strm_other);
                    tp_video_stop(strm_other);
                    tp_video_start(strm_other,&(attr[strm_other]));
                    printf("stop -> start %d second!\n", strm_cur);
                    tp_video_stop(strm_cur);
                    radom_video_param(strm_cur, &(attr[strm_cur]));
                    tp_video_start(strm_cur, &(attr[strm_cur]));
                }

                index ++;
                index %= ARRAY_SIZE(strm_id);
            }
        }

    }

    tp_video_deinit(2);
    return 0;
}

