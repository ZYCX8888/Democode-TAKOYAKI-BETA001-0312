#include <stdio.h>
#include <sys/time.h>
#include "st_common.h"

MI_S32 ST_Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    STCHECKRESULT(MI_SYS_Init());

    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    STCHECKRESULT(MI_SYS_GetVersion(&stVersion));
    ST_INFO("u8Version:0x%llx\n", stVersion.u8Version);

    STCHECKRESULT(MI_SYS_GetCurPts(&u64Pts));
    ST_INFO("u64Pts:0x%llx\n", u64Pts);

    u64Pts = 0xF1237890F1237890;
    STCHECKRESULT(MI_SYS_InitPtsBase(u64Pts));

    u64Pts = 0xE1237890E1237890;
    STCHECKRESULT(MI_SYS_SyncPts(u64Pts));

    return MI_SUCCESS;
}

MI_S32 ST_Sys_Exit(void)
{
    STCHECKRESULT(MI_SYS_Exit());

    return MI_SUCCESS;
}

MI_U64 ST_Sys_GetCurTimeU64()
{
    struct timeval tm;
    gettimeofday(&tm,NULL);

    return tm.tv_sec*1000000 + tm.tv_usec; //unit:us
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


MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

MI_S32 ST_GetTimingInfo(MI_S32 s32ApTiming, MI_S32 *ps32HdmiTiming, MI_S32 *ps32DispTiming, MI_U16 *pu16DispW, MI_U16 *pu16DispH)
{
    switch (s32ApTiming)
    {
        case E_ST_TIMING_720P_50:
            *ps32DispTiming = E_MI_DISP_OUTPUT_720P50;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_720_50P;
            *pu16DispW = 1280;
            *pu16DispH = 720;
            break;
        case E_ST_TIMING_720P_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_720P60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_720_60P;
            *pu16DispW = 1280;
            *pu16DispH = 720;
            break;
        case E_ST_TIMING_1080P_50:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1080P50;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1080_50P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_1080P_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1080P60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_1600x1200_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1600x1200_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1600x1200_60P;
            *pu16DispW = 1600;
            *pu16DispH = 1200;
            break;
        case E_ST_TIMING_1440x900_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1440x900_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1440x900_60P;
            *pu16DispW = 1440;
            *pu16DispH = 900;
            break;
        case E_ST_TIMING_1280x1024_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1280x1024_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1280x1024_60P;
            *pu16DispW = 1280;
            *pu16DispH = 1024;
            break;
        case E_ST_TIMING_1024x768_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_1024x768_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1024x768_60P;
            *pu16DispW = 1024;
            *pu16DispH = 768;
            break;
        case E_ST_TIMING_2560x1440_30:
            *ps32DispTiming = E_MI_DISP_OUTPUT_2560x1440_30;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_1440_30P;
            *pu16DispW = 2560;
            *pu16DispH = 1440;
            break;
        case E_ST_TIMING_3840x2160_30:
            *ps32DispTiming = E_MI_DISP_OUTPUT_3840x2160_30;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_4K2K_30P;
            *pu16DispW = 1920;
            *pu16DispH = 1080;
            break;
        case E_ST_TIMING_3840x2160_60:
            *ps32DispTiming = E_MI_DISP_OUTPUT_3840x2160_60;
            *ps32HdmiTiming = E_MI_HDMI_TIMING_4K2K_60P;
            *pu16DispW = 3840;
            *pu16DispH = 2160;
            break;
        default:
            ST_WARN("Unspport Ap timing (%d)\n", s32ApTiming);
            return -1;
    }

    return MI_SUCCESS;
}

