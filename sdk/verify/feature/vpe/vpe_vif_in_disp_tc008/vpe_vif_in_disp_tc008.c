/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "../mi_vpe_test.h"
#include "mi_vif.h"
static test_vpe_Config stTest008 = {
    //.inputFile  = TEST_VPE_CHNN_FILE420(008, 0, 736x240),
    .stSrcWin   = {0, 0, 1920, 1080},
    .stCropWin  = {0, 0, 1920, 1080},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(008, 0, 0, 1920x1080),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 1920, 1080},
        },
    },
};
static MI_BOOL _gbStop = FALSE;
static int test_vpe_IqEnd(int id)
{
    return E_VPE_TEST_IQ_RET_EXIT;
}
static int getChoice(void)
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), stdin);
    return atoi(buffer);
}
static int test_vpe_IqNrEnable(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    int id;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    printf("\n Entry your choice:\n\t0: OFF, 1: ON.\n");
    id = getChoice();
    if (id == 0)
    {
        stVpeChAttr.bNrEn = FALSE;
    }
    else
    {
        stVpeChAttr.bNrEn = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqEdgeEnable(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    int id;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    printf("\n Entry your choice:\n\t0: OFF, 1: ON.\n");
    id = getChoice();
    if (id == 0)
    {
        stVpeChAttr.bEdgeEn = FALSE;
    }
    else
    {
        stVpeChAttr.bEdgeEn = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqEsEnable(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    int id;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    printf("\n Entry your choice:\n\t0: OFF, 1: ON.\n");
    id = getChoice();
    if (id == 0)
    {
        stVpeChAttr.bEsEn = FALSE;
    }
    else
    {
        stVpeChAttr.bEsEn = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqContrastEnable(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    int id;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    printf("\n Entry your choice:\n\t0: OFF, 1: ON.\n");
    id = getChoice();
    if (id == 0)
    {
        stVpeChAttr.bContrastEn = FALSE;
    }
    else
    {
        stVpeChAttr.bContrastEn = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqUvInvertEnable(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    int id;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    printf("\n Entry your choice:\n\t0: OFF, 1: ON.\n");
    id = getChoice();
    if (id == 0)
    {
        stVpeChAttr.bUvInvert = FALSE;
    }
    else
    {
        stVpeChAttr.bUvInvert = TRUE;
    }
    ExecFunc(MI_VPE_SetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNrcSfStr(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 255.\n");
    id = getChoice();
    // TODO: 0 ~ 255; need check.
    stPara.u8NrcSfStr = (MI_U8)(id & 0xff);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNrcTfStr(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 255.\n");
    id = getChoice();
    // TODO: 0 ~ 255 need check.
    stPara.u8NrcTfStr = (MI_U8)(id & 0xff);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNrySfStr(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 255.\n");
    id = getChoice();
    // TODO: 0 ~ 255 need check.
    stPara.u8NrySfStr = (MI_U8)(id & 0xff);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryTfStr(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 255.\n");
    id = getChoice();
    // TODO: 0 ~ 255 need check.
    stPara.u8NryTfStr = (MI_U8)(id & 0xff);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryBlendMotionTh(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 15.\n");
    id = getChoice();
    // TODO: 0 ~ 15 need check.
    stPara.u8NryBlendMotionTh = (MI_U8)(id & 0x0f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryBlendStillTh(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 15.\n");
    id = getChoice();
    // TODO: 0 ~ 15 need check.
    stPara.u8NryBlendStillTh = (MI_U8)(id & 0x0f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryBlendMotionWei(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 15.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendMotionWei = (MI_U8)(id & 0x0f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryBlendOtherWei(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 15.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendOtherWei = (MI_U8)(id & 0x0f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetNryBlendStillWei(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0 ~ 15.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendStillWei = (MI_U8)(id & 0x0f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetEdgeGain(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0,i=0;
    // TODO: just for test
    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    for(i=0;i<6;i++)
    {
        printf("\n u8EdgeGain[%d] Entry your choice: 0~255.\n",i);
        id = getChoice();
        if(id < 255)
          stPara.u8EdgeGain[i] = (MI_U8)(id & 0xff);
    }
    // TODO: 0~255 need check.
    //scanf("%d %d %d %d %d %d.\n", &stPara.u8EdgeGain[0], &stPara.u8EdgeGain[1], &stPara.u8EdgeGain[2], &stPara.u8EdgeGain[3],&stPara.u8EdgeGain[4], &stPara.u8EdgeGain[5]);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetContrast(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0~255.\n");
    id = getChoice();
    // TODO: 0~255 need check.
    stPara.u8Contrast = (MI_U8)(id & 0xff);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqShowPara(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;

    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n");
    printf("IQ Parameters:\n");
    printf("\t.NrcSfStr         : %d.\n", stPara.u8NrcSfStr);
    printf("\t.NrcTfStr         : %d.\n", stPara.u8NrcTfStr);
    printf("\t.NrySfStr         : %d.\n", stPara.u8NrySfStr);
    printf("\t.NryTfStr         : %d.\n", stPara.u8NryTfStr);
    printf("\t.NryBlendMotionTh : %d.\n", stPara.u8NryBlendMotionTh);
    printf("\t.NryBlendStillTh  : %d.\n", stPara.u8NryBlendStillTh);
    printf("\t.NryBlendMotionWei: %d.\n", stPara.u8NryBlendMotionWei);
    printf("\t.NryBlendOtherWei : %d.\n", stPara.u8NryBlendOtherWei);
    printf("\t.NryBlendStillWei : %d.\n", stPara.u8NryBlendStillWei);
    printf("\t.EdgeGain[6]      : {%d, %d, %d, %d, %d, %d}.\n", stPara.u8EdgeGain[0], stPara.u8EdgeGain[1], stPara.u8EdgeGain[2],
        stPara.u8EdgeGain[3], stPara.u8EdgeGain[4], stPara.u8EdgeGain[5]);
    printf("\t.Contrast         : %d.\n", stPara.u8Contrast);
    printf("\n");

    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqShowOnOff(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelAttr_t stVpeChAttr;
    memset(&stVpeChAttr, 0, sizeof(stVpeChAttr));
    ExecFunc(MI_VPE_GetChannelAttr(VpeCh, &stVpeChAttr), MI_VPE_OK);

    printf("\n");
    printf("IQ ON/OFF:\n");
    printf("\t.u16MaxW          : %d.\n", stVpeChAttr.u16MaxW);
    printf("\t.u16MaxH          : %d.\n", stVpeChAttr.u16MaxH);
    printf("\t.ePixFmt          : %d.\n", stVpeChAttr.ePixFmt);
    printf("\t.bNrEn            : %d.\n", stVpeChAttr.bNrEn);
    printf("\t.bEdgeEn          : %d.\n", stVpeChAttr.bEdgeEn);
    printf("\t.bEsEn            : %d.\n", stVpeChAttr.bEsEn);
    printf("\t.bContrastEn      : %d.\n", stVpeChAttr.bContrastEn);
    printf("\t.bUvInvert        : %d.\n", stVpeChAttr.bUvInvert);
    printf("\n");

    return E_VPE_TEST_IQ_RET_PASS;
}


static int test_vpe_IqShowAll(int data)
{
    test_vpe_IqShowOnOff(data);
    test_vpe_IqShowPara(data);

    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetCrop(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_SYS_WindowRect_t stCropInfo;
    memset(&stCropInfo, 0, sizeof(stCropInfo));
    printf("x y width height.\n");
    //scanf("%d %d %d %d", &stCropInfo.u16X, &stCropInfo.u16Y, &stCropInfo.u16Width, &stCropInfo.u16Height);
    printf("Entry X:\n");
    stCropInfo.u16X = getChoice();
    printf("Entry Y:\n");
    stCropInfo.u16Y = getChoice();
    printf("Entry Width:\n");
    stCropInfo.u16Width = getChoice();
    printf("Entry Height:\n");
    stCropInfo.u16Height= getChoice();

    ExecFunc(MI_VPE_SetChannelCrop(VpeCh,  &stCropInfo), MI_VPE_OK);
    printf("\n");
    printf("Set Ch %d CropInfo:\n", VpeCh);
    printf("\t.u16X          : %d.\n", stCropInfo.u16X);
    printf("\t.u16Y          : %d.\n", stCropInfo.u16Y);
    printf("\t.u16Width      : %d.\n", stCropInfo.u16Width);
    printf("\t.u16Height     : %d.\n", stCropInfo.u16Height);
    printf("\n");

    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqGetCrop(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_SYS_WindowRect_t stCropInfo;
    memset(&stCropInfo, 0, sizeof(stCropInfo));
    ExecFunc(MI_VPE_GetChannelCrop(VpeCh,  &stCropInfo), MI_VPE_OK);
    printf("\n");
    printf("Get Ch %d CropInfo:\n", VpeCh);
    printf("\t.u16X          : %d.\n", stCropInfo.u16X);
    printf("\t.u16Y          : %d.\n", stCropInfo.u16Y);
    printf("\t.u16Width      : %d.\n", stCropInfo.u16Width);
    printf("\t.u16Height     : %d.\n", stCropInfo.u16Height);
    printf("\n");
    return E_VPE_TEST_IQ_RET_PASS;
}

static test_vpe_TestIqMenu_t _gTestVpeIq[] = {
    // Main menu
    [E_VPE_TEST_IQ_IQ_ON_OFF]               = {"Test IQ ON/OFF", NULL, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_IQ_PARAM]                = {"Test IQ Param",  NULL, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_CROP]                = {"Test IQ Crop",   NULL, E_VPE_TEST_IQ_SET_CROP_INFO, FALSE},
    [E_VPE_TEST_IQ_IQ_SHOW_ALL]             = {"Show all IQ Attribution and Param",  test_vpe_IqShowAll, E_VPE_TEST_IQ_IQ_ON_OFF, TRUE},

    // IQ ON/OFF
    [E_VPE_TEST_IQ_ENABLE_NR]                = {"NR ON/OFF", test_vpe_IqNrEnable, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_EDGE]              = {"Edge ON/OFF", test_vpe_IqEdgeEnable, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_ES]                = {"Es ON/OFF", test_vpe_IqEsEnable, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_CONTRAST]          = {"Contrast ON/OFF", test_vpe_IqContrastEnable, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_UVINVERT]          = {"Uv invert ON/OFF", test_vpe_IqUvInvertEnable, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_SHOW_ON_OFF]       = {"Show IQ Attribution", test_vpe_IqShowOnOff, E_VPE_TEST_IQ_ENABLE_NR, FALSE},
    [E_VPE_TEST_IQ_ENABLE_RET]               = {"Return", NULL, E_VPE_TEST_IQ_IQ_ON_OFF, TRUE},

    // IQ Param
    [E_VPE_TEST_IQ_SET_NRC_SF_STR]           = {"Set NrcSfStr -- 0 ~ 255", test_vpe_IqSetNrcSfStr, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRC_TF_STR]           = {"Set NrcTfStr -- 0 ~ 255", test_vpe_IqSetNrcTfStr, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_SF_STR]           = {"Set NrySfStr -- 0 ~ 255", test_vpe_IqSetNrySfStr, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_TF_STR]           = {"Set NryTfStr -- 0 ~ 255", test_vpe_IqSetNryTfStr, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_BLEND_MOTION_TH]  = {"Set NryBlendMotionTh -- 0 ~ 15", test_vpe_IqSetNryBlendMotionTh, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_BLEND_STILL_TH]   = {"Set NryBlendStillTh -- 0 ~ 15", test_vpe_IqSetNryBlendStillTh, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_BLEND_MOTION_WEI] = {"Set NryBlendMotionWei -- 0 ~ 31", test_vpe_IqSetNryBlendMotionWei, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_BLEND_OTHER_WEI]  = {"Set NryBlendOtherWei -- 0 ~ 31", test_vpe_IqSetNryBlendOtherWei, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_NRY_BLEND_STILL_WEI]  = {"Set NryBlendStillWei -- 0 ~ 31", test_vpe_IqSetNryBlendStillWei, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_EDGE_GAIN]            = {"Set EdgeGain[6] -- 0~255", test_vpe_IqSetEdgeGain, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_CONTRAST]             = {"Set Contrast -- 0~255", test_vpe_IqSetContrast, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_SHOW_PARA]            = {"Show IQ Parameters", test_vpe_IqShowPara, E_VPE_TEST_IQ_SET_NRC_SF_STR, FALSE},
    [E_VPE_TEST_IQ_SET_RET]                  = {"Return", NULL, E_VPE_TEST_IQ_IQ_ON_OFF, TRUE},

    [E_VPE_TEST_IQ_SET_CROP_INFO]            = {"Set crop info: x y w h", test_vpe_IqSetCrop, E_VPE_TEST_IQ_SET_CROP_INFO, FALSE},
    [E_VPE_TEST_IQ_GET_CROP_INFO]            = {"Get crop info", test_vpe_IqGetCrop, E_VPE_TEST_IQ_SET_CROP_INFO, FALSE},
    [E_VPE_TEST_IQ_CROP_RET]                 = {"Return", NULL, E_VPE_TEST_IQ_IQ_ON_OFF, TRUE},
};

static int showMenu(int index)
{
    int i = 0;
    test_vpe_TestIqMenu_t *pstMenu = NULL;
    printf("\n");
    do
    {
        pstMenu = &_gTestVpeIq[index + i];
        printf("[%d]: %s.\n", i, pstMenu->desc);
        i++;
        if (pstMenu->bEnd == TRUE)
        {
            break;
        }
    }while(1);
    printf("\n[%d]: %s.\n", i, "Quit");
    printf("Enter you Choice: \n");
    return i;
}

//int test_vpe_TestCase008_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    char src_file[256];
    const char *pbaseDir = NULL;
    int cnt = 0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    MI_DISP_INPUTPORT LayerInputPort = 0;
    int index = 0;
    int opt = 0;
    int max = 0;
    test_vpe_TestIqMenu_t *pstMenu = NULL;
    int outputPixel = 0;
    MI_SYS_PixelFormat_e eOutputPixelFmt = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;

    MI_SYS_WindowRect_t stCanvas = {0, 0, 1920, 1080};
    MI_SYS_WindowRect_t stDispWin = {0, 0, 1920, 1080};

    if (argc < 2)
    {
        printf("%s <PixelFormat>(0: YUV422 1: MST420).\n", argv[0]);
        printf("%s.\n", VPE_TEST_008_DESC);
        printf("Channel: %d.\n", 0);
        return 0;
    }

    printf("%s %s\n", argv[0], argv[1]);
    outputPixel = atoi(argv[1]);
    switch(outputPixel)
    {
    case 0:
        eOutputPixelFmt = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    break;
    case 1:
        eOutputPixelFmt = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
    break;
    default:
        {
            printf("%s <test_dir> <PixelFormat>(0: YUV422 1: MST420).\n", argv[0]);
            printf("%s.\n", VPE_TEST_008_DESC);
            printf("Channel: %d.\n", 0);
            printf("InputFile: %s.\n", stTest008.inputFile);
            return 0;
        }
    }
    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 3;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest008.stCropWin, &stTest008.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = eOutputPixelFmt;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    // set vpe port buffer depth
    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = VpeChannel;
    stVpeChnInputPort0.u32PortId = 0;

    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = VpeChannel;
    stVpeChnOutputPort0.u32PortId = VpePort;
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 0, 3);

    stCanvas.u16X = stTest008.stOutPort[0].stPortWin.u16X;
    stCanvas.u16Y = stTest008.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Width = stTest008.stOutPort[0].stPortWin.u16Width;
    stCanvas.u16Height= stTest008.stOutPort[0].stPortWin.u16Height;

    stDispWin.u16X = stTest008.stOutPort[0].stPortWin.u16X;
    stDispWin.u16Y = stTest008.stOutPort[0].stPortWin.u16Y;
    stDispWin.u16Width = 1920;//stTest008.stOutPort[0].stPortWin.u16Width;
    stDispWin.u16Height= 1080;//stTest008.stOutPort[0].stPortWin.u16Height;

    // init VIF
    test_vpe_InitVif(0, 0, 0, &stTest008.stSrcWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL);

    // VPE binder to VIF
    test_vpeBinderFromVif(0, VpeChannel);

    test_vpe_InitDisp(DispDev, DispLayer, LayerInputPort, &stCanvas, &stDispWin, eOutputPixelFmt);
    test_vpeBinderDisp(VpePort, 0);

    printf("Welcome: IQ test.\n");
    while(1)
    {
        max = showMenu(index);
        opt = getChoice();
        if ((opt < 0) || (opt > max))
        {
            printf("Invalid input option !!.\n");
            continue;
        }
        if (opt == max)
        {
            break;
        }
        pstMenu = &_gTestVpeIq[index + opt];
        if (pstMenu->func != NULL)
        {
            pstMenu->func(opt);
        }
        index = pstMenu->next_index;
    }

    // VPE unbinder VIF
    test_vpeUnBinderFromVif(0, VpeChannel);

    test_vpeUnBinderDisp(VpePort, 0);
    test_vpe_DestroyChannel(VpeChannel, VpePort);

    // VPE Deinit VIF
    test_vpe_DeinitVif(0, 0, 0);

    test_vpe_DeinitDisp(DispDev, DispLayer, LayerInputPort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
