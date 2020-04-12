#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_shadow.h"

///#define MI_PRINT(fmt, args...) 
#define MI_PRINT printf


#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        MI_PRINT("[%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        MI_PRINT("(%d)exec function pass\n", __LINE__);\
    }

///////////////////////////////////////////////////////////////////
MI_SHADOW_RegisterDevParams_t _stIVERegDevInfo;
MI_SHADOW_HANDLE _hIVEDev = 0;

MI_S32 _MI_IVE_OnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)IVE Get On Input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_IVE_OnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)IVE Get On output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_IVE_OnUnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)IVE Get On un input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

MI_S32 _MI_IVE_OnUnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)IVE Get On Un Output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

int ive_process(void)
{
    MI_PRINT("(%d)IVE Test Start\n", __LINE__);
    memset(&_stIVERegDevInfo, 0x0, sizeof(MI_SHADOW_RegisterDevParams_t));
    _stIVERegDevInfo.stModDevInfo.eModuleId = E_MI_MODULE_ID_IVE;
    _stIVERegDevInfo.stModDevInfo.u32DevId = 0;
    _stIVERegDevInfo.stModDevInfo.u32DevChnNum = 16;
    _stIVERegDevInfo.stModDevInfo.u32InputPortNum = 1;
    _stIVERegDevInfo.stModDevInfo.u32OutputPortNum = 1;
    _stIVERegDevInfo.OnBindInputPort = _MI_IVE_OnBindInputPort;
    _stIVERegDevInfo.OnBindOutputPort = _MI_IVE_OnBindOutputPort;
    _stIVERegDevInfo.OnUnBindInputPort = _MI_IVE_OnUnBindInputPort;
    _stIVERegDevInfo.OnUnBindOutputPort = _MI_IVE_OnUnBindOutputPort;

    ExecFunc(MI_SHADOW_RegisterDev(&_stIVERegDevInfo, &_hIVEDev), MI_SUCCESS);
    MI_PRINT("(%d)_hIVEDev:0x%x\n", __LINE__, _hIVEDev);


    MI_U16 u32ChnId = 5;
    MI_U16 u32PortId = 1;
    MI_SYS_BufConf_t stBufConfig;
    MI_SYS_BufInfo_t stOutputBufInfo;
    MI_BOOL bBlockedByRateCtrl = FALSE;

    memset(&stBufConfig, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConfig.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConfig.u64TargetPts = MI_SYS_INVALID_PTS;
    stBufConfig.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stBufConfig.stFrameCfg.u16Width = 1920;
    stBufConfig.stFrameCfg.u16Height = 1080;

    memset(&stOutputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    MI_SYS_BUF_HANDLE hOutputBufHandle = MI_HANDLE_NULL;
    ExecFunc(MI_SHADOW_GetOutputPortBuf(_hIVEDev, u32ChnId, u32PortId, &stBufConfig, &bBlockedByRateCtrl, &stOutputBufInfo, &hOutputBufHandle), MI_SUCCESS);
    MI_PRINT("(%d)_hIVEDev:0x%x, Get Output u64PhyAddr:0x%llx\n", __LINE__, stOutputBufInfo.stFrameData.phyAddr[0]);

    MI_SYS_BufInfo_t stInputBufInfo;
    memset(&stInputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    MI_SYS_BUF_HANDLE hInputBufHandle = MI_HANDLE_NULL;
    ExecFunc(MI_SHADOW_GetInputPortBuf(_hIVEDev, u32ChnId, u32PortId, &stInputBufInfo, &hInputBufHandle), MI_SUCCESS);
    MI_PRINT("_hIVEDev:0x%x, Get Input u64PhyAddr:0x%llx\n", __LINE__, stInputBufInfo.stFrameData.phyAddr[0]);

    ExecFunc(MI_SHADOW_FinishBuf(_hIVEDev, hInputBufHandle), MI_SUCCESS);
    ExecFunc(MI_SHADOW_RewindBuf(_hIVEDev, hOutputBufHandle), MI_SUCCESS);

    ExecFunc(MI_SHADOW_WaitOnInputTaskAvailable(_hIVEDev, 1000), MI_SUCCESS);

    int counter = 0;
    while (1)
    {
        MI_PRINT("\nsleep counter:%d\n", counter++);
        sleep(3);
        if (counter > 10)
        {
            break;
        }
    }

    ExecFunc(MI_SHADOW_UnRegisterDev(_hIVEDev), MI_SUCCESS);
}

///////////////////////////////////////////////////////////////////
MI_SHADOW_RegisterDevParams_t _stVDFRegDevInfo;
MI_SHADOW_HANDLE _hVDFDev = 0;

MI_S32 _MI_VDF_OnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)VDF Get On Input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_VDF_OnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)VDF Get On output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_VDF_OnUnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("VDF Get On un input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

MI_S32 _MI_VDF_OnUnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_PRINT("(%d)VDF Get On Un Output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

int vdf_process(void)
{
    MI_PRINT("(%d)VDF Test Start\n", __LINE__);
    memset(&_stVDFRegDevInfo, 0x0, sizeof(MI_SHADOW_RegisterDevParams_t));
    _stVDFRegDevInfo.stModDevInfo.eModuleId = E_MI_MODULE_ID_VDF;
    _stVDFRegDevInfo.stModDevInfo.u32DevId = 0;
    _stVDFRegDevInfo.stModDevInfo.u32DevChnNum = 16;
    _stVDFRegDevInfo.stModDevInfo.u32InputPortNum = 1;
    _stVDFRegDevInfo.stModDevInfo.u32OutputPortNum = 1;
    _stVDFRegDevInfo.OnBindInputPort = _MI_VDF_OnBindInputPort;
    _stVDFRegDevInfo.OnBindOutputPort = _MI_VDF_OnBindOutputPort;
    _stVDFRegDevInfo.OnUnBindInputPort = _MI_VDF_OnUnBindInputPort;
    _stVDFRegDevInfo.OnUnBindOutputPort = _MI_VDF_OnUnBindOutputPort;

    ExecFunc(MI_SHADOW_RegisterDev(&_stVDFRegDevInfo, &_hVDFDev), MI_SUCCESS);
    MI_PRINT("(%d)_hVDFDev:0x%x\n", __LINE__, _hVDFDev);


    MI_U16 u32ChnId = 3;
    MI_U16 u32PortId = 1;
    MI_SYS_BufConf_t stBufConfig;
    MI_SYS_BufInfo_t stOutputBufInfo;
    MI_BOOL bBlockedByRateCtrl = FALSE;

    memset(&stBufConfig, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConfig.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConfig.u64TargetPts = MI_SYS_INVALID_PTS;
    stBufConfig.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stBufConfig.stFrameCfg.u16Width = 1920;
    stBufConfig.stFrameCfg.u16Height = 1080;

    memset(&stOutputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    MI_SYS_BUF_HANDLE hOutputBufHandle = MI_HANDLE_NULL;
    ExecFunc(MI_SHADOW_GetOutputPortBuf(_hVDFDev, u32ChnId, u32PortId, &stBufConfig, &bBlockedByRateCtrl, &stOutputBufInfo, &hOutputBufHandle), MI_SUCCESS);
    MI_PRINT("(%d)_hVDFDev:0x%x, Get Output u64PhyAddr:0x%llx\n", __LINE__, stOutputBufInfo.stFrameData.phyAddr[0]);

    MI_SYS_BufInfo_t stInputBufInfo;
    memset(&stInputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    MI_SYS_BUF_HANDLE hInputBufHandle = MI_HANDLE_NULL;
    ExecFunc(MI_SHADOW_GetInputPortBuf(_hVDFDev, u32ChnId, u32PortId, &stInputBufInfo, &hInputBufHandle), MI_SUCCESS);
    MI_PRINT("(%d)_hVDFDev:0x%x, Get Input u64PhyAddr:0x%llx\n", __LINE__, stInputBufInfo.stFrameData.phyAddr[0]);

    ExecFunc(MI_SHADOW_FinishBuf(_hVDFDev, hInputBufHandle), MI_SUCCESS);
    ExecFunc(MI_SHADOW_RewindBuf(_hVDFDev, hOutputBufHandle), MI_SUCCESS);

    ExecFunc(MI_SHADOW_WaitOnInputTaskAvailable(_hVDFDev, 1000), MI_SUCCESS);
    
    int counter = 0;
    while (1)
    {
        MI_PRINT("\nsleep counter:%d\n", counter++);
        sleep(3);
        if (counter > 10)
        {
            break;
        }
    }

    ExecFunc(MI_SHADOW_UnRegisterDev(_hVDFDev), MI_SUCCESS);
}

int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;

    if (0 == fork())
    {
        ive_process();
    }
    else
    {
        vdf_process();
    }
    return 0;
}
