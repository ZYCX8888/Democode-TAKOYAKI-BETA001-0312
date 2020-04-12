#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"

#include "st_disp.h"

MI_S32 ST_Disp_DevInit(MI_DISP_DEV dispDev, MI_DISP_PubAttr_t stPubAttr)
{
    MI_DISP_SetPubAttr(dispDev, &stPubAttr);
    MI_DISP_Enable(dispDev);
    STDBG_LEAVE();

    return MI_SUCCESS;
}

MI_S32 ST_Disp_LayerInit(MI_DISP_DEV dispDev, MI_DISP_LAYER layer, MI_DISP_VideoLayerAttr_t stLayerAttr)
{
    MI_DISP_SetVideoLayerAttr(layer, &stLayerAttr);
    MI_DISP_BindVideoLayer(layer, dispDev);
    MI_DISP_EnableVideoLayer(layer);

    return MI_SUCCESS;
}

MI_S32 ST_Disp_ChnInit(MI_DISP_LAYER DispLayer, const ST_DispChnInfo_t *pstDispChnInfo)
{
    MI_U32 i = 0;
    MI_U32 u32InputPort = 0;
    MI_S32 InputPortNum = pstDispChnInfo->InputPortNum; //test use 0

    MI_DISP_InputPortAttr_t stInputPortAttr;

    STDBG_ENTER();
    for (i = 0; i < InputPortNum; i++)
    {
        memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

        u32InputPort = pstDispChnInfo->stInputPortAttr[i].u32Port;
        ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        stInputPortAttr.stDispWin.u16X      = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16X;
        stInputPortAttr.stDispWin.u16Y      = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Y;
        stInputPortAttr.stDispWin.u16Width  = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Width;
        stInputPortAttr.stDispWin.u16Height = pstDispChnInfo->stInputPortAttr[i].stAttr.stDispWin.u16Height;
        stInputPortAttr.u16SrcWidth = pstDispChnInfo->stInputPortAttr[i].stAttr.u16SrcWidth;
        stInputPortAttr.u16SrcHeight = pstDispChnInfo->stInputPortAttr[i].stAttr.u16SrcHeight;

        ExecFunc(MI_DISP_SetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_GetInputPortAttr(DispLayer, u32InputPort, &stInputPortAttr), MI_SUCCESS);
        ExecFunc(MI_DISP_EnableInputPort(DispLayer, u32InputPort), MI_SUCCESS);
        ExecFunc(MI_DISP_SetInputPortSyncMode(DispLayer, u32InputPort, E_MI_DISP_SYNC_MODE_FREE_RUN), MI_SUCCESS);
    }
    STDBG_LEAVE();

    return MI_SUCCESS;
}

MI_S32 ST_Disp_DeInit(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_S32 s32InputPortNum)
{
    MI_S32 s32InputPort = 0;
    for (s32InputPort = 0; s32InputPort < s32InputPortNum; s32InputPort++)
    {
        ExecFunc(MI_DISP_DisableInputPort(DispLayer, s32InputPort), MI_SUCCESS);
    }
    ExecFunc(MI_DISP_DisableVideoLayer(DispLayer), MI_SUCCESS);
    ExecFunc(MI_DISP_UnBindVideoLayer(DispLayer, DispDev), MI_SUCCESS);
    ExecFunc(MI_DISP_Disable(DispDev), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Disp_ShowStatus(MI_DISP_LAYER DispLayer, MI_S32 s32InputPortNum, MI_BOOL bIsShow)
{
    if (bIsShow)
    {
        ExecFunc(MI_DISP_ShowInputPort(DispLayer, s32InputPortNum), MI_SUCCESS);
    }
    else
    {
        ExecFunc(MI_DISP_HideInputPort(DispLayer, s32InputPortNum), MI_SUCCESS);

    }

    return MI_SUCCESS;
}

MI_S32 ST_Disp_ClearChn(MI_DISP_LAYER s32Layer, MI_S32 s32Port)
{
    ExecFunc(MI_DISP_ClearInputPortBuffer(s32Layer, s32Port, TRUE), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Disp_EnableChn(MI_DISP_LAYER s32Layer, MI_S32 s32Port)
{
    ExecFunc(MI_DISP_EnableInputPort(s32Layer, s32Port), MI_SUCCESS);

    return MI_SUCCESS;
}
MI_S32 ST_Disp_DisableChn(MI_DISP_LAYER s32Layer, MI_S32 s32Port)
{
    ExecFunc(MI_DISP_DisableInputPort(s32Layer, s32Port), MI_SUCCESS);

    return MI_SUCCESS;
}

