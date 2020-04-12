#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "mi_vpe.h"

#define ExecFunc(func, _ret_) \
    printf("%d Start test: %s\n", __LINE__, #func);\
    if (func != _ret_)\
    {\
        printf("DISP_TEST [%d] %s exec function failed\n",__LINE__, #func);\
        return 1;\
    }\
    else\
    {\
        printf("DISP_TEST [%d] %s  exec function pass\n", __LINE__, #func);\
    }\
    printf("%d End test: %s\n", __LINE__, #func);

#if 0
MI_S32 test_show(void)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_VPE_WinRect_t stCropWin;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_S32 s32Ret = MI_OK;

    VpeChannel = 0;
    VpePort = 0;

    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bESEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUVInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    s32Ret = MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    stChannelVpssAttr.bContrastEn = TRUE;
    stChannelVpssAttr.bNrEn = TRUE;
    s32Ret = MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_GetChannelCrop(VpeChannel, &stCropWin);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }
    stCropWin.u16X = 20;
    stCropWin.u16Y = 40;
    stCropWin.u16Width = 1920;
    stCropWin.u16Height = 1080;
    s32Ret = MI_VPE_SetChannelCrop(VpeChannel, &stCropWin);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_EnablePort(VpeChannel, VpePort);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_StartChannel (VpeChannel);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    s32Ret = MI_VPE_StopChannel (VpeChannel);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_DisablePort(VpeChannel, VpePort);
    {
        return s32Ret;
    }

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    s32Ret = MI_VPE_DestroyChannel(VpeChannel);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

}
#endif
MI_S32 test_vpe_flow(void)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_S32 s32Ret = MI_VPE_OK;
    MI_U32 u32Dly = 0;


    VpeChannel = 0;
    VpePort = 0;

    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = TRUE;
    stChannelVpssAttr.bNrEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = 20;
    stCropWin.u16Y = 40;
    stCropWin.u16Width = 1920;
    stCropWin.u16Height = 1080;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    u32Dly = 6;
    while(u32Dly--)
    {
        printf("%d.\n", u32Dly);
        sleep(1);
    }

    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);

    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);
    u32Dly = 6;
    while(u32Dly--)
    {
        printf("%d.\n", u32Dly);
        sleep(1);
    }

    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VPE_StopChannel (VpeChannel), MI_VPE_OK);

    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);

}

MI_S32 test_vpe_CreatChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    MI_VPE_ChannelAttr_t stChannelVpssAttr;
    MI_SYS_WindowRect_t stCropWin;
    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    ExecFunc(MI_VPE_CreateChannel(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = TRUE;
    stChannelVpssAttr.bNrEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpeChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = 20;
    stCropWin.u16Y = 40;
    stCropWin.u16Width = 1920;
    stCropWin.u16Height = 1080;
    ExecFunc(MI_VPE_SetChannelCrop(VpeChannel, &stCropWin), MI_VPE_OK);

    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);

    ExecFunc(MI_VPE_StartChannel (VpeChannel), MI_VPE_OK);

}

MI_S32 test_vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VPE_StopChannel (VpeChannel), MI_VPE_OK);

    ExecFunc(MI_VPE_DisablePort(VpeChannel, VpePort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpeChannel), MI_VPE_OK);
}
static void printfData(MI_SYS_BufInfo_t *pstBufInfo)
{
    int i = 0;
    if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_RAW)
    {
        //printf("%s \n", pstBufInfo->stRawData.pVirAddr);
        printf("u32ContentSize : %d \n",pstBufInfo->stRawData.u32ContentSize);
    }
    else if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_FRAME)
    {
        printf("height :%d  width : %d \n", pstBufInfo->stFrameData.u16Height , pstBufInfo->stFrameData.u16Width);
    }
    else if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_META)
    {
        printf("From Module :%d \n",pstBufInfo->stMetaData.eDataFromModule);
    }
    else
        printf("error \n");
}

#include <time.h>

int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_U32 u32Dly = 0;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_U32 u32VpeOutput2Frmrate= 30,  u32DispInput2Frmrate = 30;
    MI_U32 u32VpeOutput0Frmrate= 30,  u32DispInput0Frmrate = 30;
    MI_SYS_ChnPort_t stVpeChnOutputPort1;
    MI_U32 u32VpeOutput1Frmrate= 30,  u32DispInput1Frmrate = 30;
    MI_SYS_ChnPort_t stVpeChnOutputPort2;
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    time_t stTime = 0;
    memset(&stTime, 0, sizeof(stTime));

    // init MI_SYS
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort);

    // set vpe port buffer depth
    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = 0;
    stVpeChnInputPort0.u32PortId = 0;

    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 0;

    stVpeChnOutputPort1.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort1.u32DevId = 0;
    stVpeChnOutputPort1.u32ChnId = 0;
    stVpeChnOutputPort1.u32PortId = 1;
    stVpeChnOutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort2.u32DevId = 0;
    stVpeChnOutputPort2.u32ChnId = 0;
    stVpeChnOutputPort2.u32PortId = 2;
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0,5,20);
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort1,5,20);
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort2,5,20);

    // set sys PTS
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    ExecFunc(MI_SYS_GetVersion(&stVersion), MI_SUCCESS);
    printf("(%d) u8Version:0x%llx\n", __LINE__, stVersion.u8Version);

    ExecFunc(MI_SYS_GetCurPts(&u64Pts), MI_SUCCESS);
    printf("(%d) u64Pts:0x%llx\n", __LINE__, u64Pts);

    u64Pts = 0xF1237890F1237890;
    ExecFunc(MI_SYS_InitPtsBase(u64Pts), MI_SUCCESS);

    u64Pts = 0xE1237890E1237890;
    ExecFunc(MI_SYS_SyncPts(u64Pts), MI_SUCCESS);

    // Start user put int buffer

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U8 u8Pattern;
    MI_U32 alloc_size;

    alloc_size = 0x100000;
    while(1)
    {
        u8Pattern++;
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0,&stBufInfo,&hHandle))
        {
            printfData(&stBufInfo);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        MI_SYS_BufConf_t stBufConf;
        memset(&stBufConf ,  0 , sizeof(stBufConf));

        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u32Flags = 0x80000000;// MI_SYS_MAP_VA;
        stBufConf.u64TargetPts = time(&stTime);
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = 1280;
        stBufConf.stFrameCfg.u16Width = 720;
        //printf("@@@--->samson try alloc raw data %08x\n", stBufConf.stRawCfg.u32Size);
        if(MI_SUCCESS  != MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle ,-1))
         {
            //printf("@@@--->failed alloc raw data %08x\n", stBufConf.stRawCfg.u32Size);
            sleep(1);
            continue;
          }

        if(stBufInfo.stRawData.u32BufSize != stBufConf.stRawCfg.u32Size)
        {
            printf("@@@--->failed request alloc %08x, really return %08x\n", stBufConf.stRawCfg.u32Size, stBufInfo.stRawData.u32BufSize );
            continue;
        }

        //printf("@@@--->samson try mem set %p, %08x   %02x\n", stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize,u8Pattern);

        memset(stBufInfo.stMetaData.pVirAddr , u8Pattern, stBufInfo.stRawData.u32BufSize);
        //printf("after memory set\n");
        stBufInfo.stRawData.u32ContentSize = stBufConf.stRawCfg.u32Size;
        stBufInfo.bEndOfStream = 0;

        MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo);

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
        {
            printf("get output buf ok\n");
            printf("@@@--->buf type %d\n", stBufInfo.eBufType);
            if(stBufInfo.eBufType == E_MI_SYS_BUFDATA_META)
            {

              printf("@@@---> size %d, extradata%d, module%d,  content %p\n",
              stBufInfo.stMetaData.u32Size, stBufInfo.stMetaData.u32ExtraData , stBufInfo.stMetaData.eDataFromModule,    stBufInfo.stMetaData.pVirAddr);
              printf("@@@--->%d\n",   ((char*)stBufInfo.stMetaData.pVirAddr)[0]);
            }
            usleep(500);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            printf("@@@--->put output buf \n");
        }
        else
        {
            printf("@@@--->failed to get output buf\n", stBufConf.stRawCfg.u32Size, stBufInfo.stRawData.u32BufSize );
            continue;
        }

        sleep(1);
    }

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(MI_SYS_DeInit(), MI_SUCCESS);

    printf("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}

