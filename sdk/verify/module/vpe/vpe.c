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
    MI_VPE_CHANNEL VpssChannel;
    MI_VPE_PORT VpssPort;
    MI_S32 s32Ret = MI_OK;

    VpssChannel = 0;
    VpssPort = 0;

    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bESEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUVInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    s32Ret = MI_VPE_CreateChannel(VpssChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_GetChannelAttr(VpssChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    stChannelVpssAttr.bContrastEn = TRUE;
    stChannelVpssAttr.bNrEn = TRUE;
    s32Ret = MI_VPE_SetChannelAttr(VpssChannel, &stChannelVpssAttr);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_GetChannelCrop(VpssChannel, &stCropWin);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }
    stCropWin.u16X = 20;
    stCropWin.u16Y = 40;
    stCropWin.u16Width = 1920;
    stCropWin.u16Height = 1080;
    s32Ret = MI_VPE_SetChannelCrop(VpssChannel, &stCropWin);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_EnablePort(VpssChannel, VpssPort);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_StartChannel (VpssChannel);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    s32Ret = MI_VPE_StopChannel (VpssChannel);
    if(s32Ret != MI_OK)
    {
        return s32Ret;
    }

    s32Ret = MI_VPE_DisablePort(VpssChannel, VpssPort);
    {
        return s32Ret;
    }

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    s32Ret = MI_VPE_DestroyChannel(VpssChannel);
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
    MI_VPE_CHANNEL VpssChannel;
    MI_VPE_PORT VpssPort;
    MI_S32 s32Ret = MI_VPE_OK;
    MI_U32 u32Dly = 0;

    VpssChannel = 0;
    VpssPort = 0;

    stChannelVpssAttr.u16MaxW = 1920;
    stChannelVpssAttr.u16MaxH = 1080;
    stChannelVpssAttr.bNrEn= FALSE;
    stChannelVpssAttr.bEdgeEn= FALSE;
    stChannelVpssAttr.bEsEn= FALSE;
    stChannelVpssAttr.bContrastEn= FALSE;
    stChannelVpssAttr.bUvInvert= FALSE;
    stChannelVpssAttr.ePixFmt = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    ExecFunc(MI_VPE_CreateChannel(VpssChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelAttr(VpssChannel, &stChannelVpssAttr), MI_VPE_OK);

    stChannelVpssAttr.bContrastEn = TRUE;
    stChannelVpssAttr.bNrEn = TRUE;
    ExecFunc(MI_VPE_SetChannelAttr(VpssChannel, &stChannelVpssAttr), MI_VPE_OK);

    ExecFunc(MI_VPE_GetChannelCrop(VpssChannel, &stCropWin), MI_VPE_OK);
    stCropWin.u16X = 20;
    stCropWin.u16Y = 40;
    stCropWin.u16Width = 1920;
    stCropWin.u16Height = 1080;
    ExecFunc(MI_VPE_SetChannelCrop(VpssChannel, &stCropWin), MI_VPE_OK);
    u32Dly = 6;
    while(u32Dly--)
    {
        printf("%d.\n", u32Dly);
        sleep(1);
    }

    ExecFunc(MI_VPE_EnablePort(VpssChannel, VpssPort), MI_VPE_OK);

    ExecFunc(MI_VPE_StartChannel (VpssChannel), MI_VPE_OK);
    u32Dly = 6;
    while(u32Dly--)
    {
        printf("%d.\n", u32Dly);
        sleep(1);
    }

    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VPE_StopChannel (VpssChannel), MI_VPE_OK);

    ExecFunc(MI_VPE_DisablePort(VpssChannel, VpssPort), MI_VPE_OK);

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VPE_DestroyChannel(VpssChannel), MI_VPE_OK);

}



int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    MI_U32 u32Delay    = 60;
    MI_BOOL bLayerTest = FALSE;
    MI_BOOL bPortTest  = FALSE;

    test_vpe_flow();

    printf("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}

