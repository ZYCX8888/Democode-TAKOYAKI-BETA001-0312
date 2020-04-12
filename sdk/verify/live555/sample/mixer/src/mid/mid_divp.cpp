/*
* mid_divp.cpp- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "mi_sys.h"
#include "mid_divp.h"

static MI_BOOL abDivpChannelUse[MIXER_DIVP_CHANNEL_NUM] = {FALSE};
static pthread_mutex_t mGetDivpChNum = PTHREAD_MUTEX_INITIALIZER;;

MI_DIVP_CHN Mixer_Divp_GetChannleNum()
{
    pthread_mutex_lock(&mGetDivpChNum);

    int i = 0;

    while(i < MIXER_DIVP_CHANNEL_NUM)
    {
        if(abDivpChannelUse[i] == FALSE)
        {
            abDivpChannelUse[i] = TRUE;

            pthread_mutex_unlock(&mGetDivpChNum);

            MIXER_DBG("get divp channel:%d\n", i);
            return i;
        }

        ++i;
    }

    pthread_mutex_unlock(&mGetDivpChNum);

    MIXER_ERR("No available DIVP channel!!!");
    return MIXER_INVALID_CHANNEL_ID;
}

void Mixer_Divp_PutChannleNum(MI_DIVP_CHN DivpChn)
{
    if((DivpChn >= MIXER_DIVP_CHANNEL_NUM) || (DivpChn == MIXER_INVALID_CHANNEL_ID))
    {
        MIXER_ERR("Invalid divp channel:%d, Max channel num is %d\n", DivpChn, MIXER_DIVP_CHANNEL_NUM);
        return;
    }

    if(!abDivpChannelUse[DivpChn])
    {
        MIXER_WARN("%d channle is not be used!\n", DivpChn);
        return;
    }

    MIXER_DBG("put divp channel:%d\n", DivpChn);

    abDivpChannelUse[DivpChn] = FALSE;
}

MI_S32 Mixer_Divp_CreatChannel(MI_DIVP_CHN DivpChn, MI_SYS_Rotate_e eRoate, MI_SYS_WindowRect_t *pstCropWin)
{
    MI_DIVP_ChnAttr_t stDivpChnAttr;

    MIXER_DBG("divp channel id:%d\n", DivpChn);
    memset(&stDivpChnAttr, 0x00, sizeof(MI_DIVP_ChnAttr_t));

    stDivpChnAttr.bHorMirror = FALSE;
    stDivpChnAttr.bVerMirror = FALSE;
    stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType = eRoate;
    stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X = pstCropWin->u16X;
    stDivpChnAttr.stCropRect.u16Y = pstCropWin->u16Y;
    stDivpChnAttr.stCropRect.u16Width = pstCropWin->u16Width;
    stDivpChnAttr.stCropRect.u16Height = pstCropWin->u16Height;
    stDivpChnAttr.u32MaxWidth = pstCropWin->u16Width;
    stDivpChnAttr.u32MaxHeight = pstCropWin->u16Height;
    ExecFunc(MI_DIVP_CreateChn(DivpChn, &stDivpChnAttr), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_SetOutputAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t *stDivpPortAttr)
{
 /*   stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stDivpPortAttr.u32Width  = stDivpPortAttr.u32Width;
    stDivpPortAttr.u32Height = stDivpPortAttr.u32Height;*/
    if(NULL == stDivpPortAttr)
    {
        MIXER_ERR("param err\n");
        return -1;
    }
    ExecFunc(MI_DIVP_SetOutputPortAttr(DivpChn, stDivpPortAttr), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_GetOutputAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t *pstDivpPortAttr)
{
    Mixer_API_ISVALID_POINT(pstDivpPortAttr);
    ExecFunc(MI_DIVP_GetOutputPortAttr(DivpChn, pstDivpPortAttr), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_GetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t *pstDivpChnAttr)
{
    Mixer_API_ISVALID_POINT(pstDivpChnAttr);
    ExecFunc(MI_DIVP_GetChnAttr(DivpChn, pstDivpChnAttr), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_SetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t stDivpChnAttr)
{
    ExecFunc(MI_DIVP_SetChnAttr(DivpChn, &stDivpChnAttr), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_StartChn(MI_DIVP_CHN DivpChn)
{
    ExecFunc(MI_DIVP_StartChn(DivpChn), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_StopChn(MI_DIVP_CHN DivpChn)
{
    ExecFunc(MI_DIVP_StopChn(DivpChn), MI_SUCCESS);
    return MI_SUCCESS;
}

MI_S32 Mixer_Divp_DestroyChn(MI_DIVP_CHN DivpChn)
{
    ExecFunc(MI_DIVP_DestroyChn(DivpChn), MI_SUCCESS);
    return MI_SUCCESS;
}


