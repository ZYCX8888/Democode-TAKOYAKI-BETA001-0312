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
static test_vpe_Config stTest008 = {
    .inputFile  = TEST_VPE_CHNN_FILE420(008, 0, 720x240),
    .stSrcWin   = {0, 0, 720, 240},
    .stCropWin  = {0, 0, 720, 240},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(008, 0, 0, 720x240),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 240},
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
    printf("\n Entry your choice: 0 ~ 31.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendMotionWei = (MI_U8)(id & 0x1f);
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
    printf("\n Entry your choice: 0 ~ 31.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendOtherWei = (MI_U8)(id & 0x1f);
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
    printf("\n Entry your choice: 0 ~ 31.\n");
    id = getChoice();
    // TODO: 0 ~ 31 need check.
    stPara.u8NryBlendStillWei = (MI_U8)(id & 0x1f);
    ExecFunc(MI_VPE_SetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    return E_VPE_TEST_IQ_RET_PASS;
}

static int test_vpe_IqSetEdgeGain(int data)
{
    MI_VPE_CHANNEL VpeCh = 0;
    MI_VPE_ChannelPara_t stPara;
    int id = 0;
    // TODO: just for test
    return 0;
    memset(&stPara, 0, sizeof(stPara));
    ExecFunc(MI_VPE_GetChannelParam(VpeCh, &stPara), MI_VPE_OK);
    printf("\n Entry your choice: 0~255.\n");
    //id = getChoice();
    // TODO: 0~255 need check.
    //stPara.u8EdgeGain[6] = (MI_U8)(id & 0xff);
    scanf("%d %d %d %d %d %d.\n", &stPara.u8EdgeGain[0], &stPara.u8EdgeGain[1], &stPara.u8EdgeGain[2], &stPara.u8EdgeGain[3],&stPara.u8EdgeGain[4], &stPara.u8EdgeGain[5]);
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

static void * test_vpe_TestIq (void *pData)
{
    MI_BOOL *pbNeedStop = pData;
    MI_SYS_WindowRect_t stWinRect[4];
    MI_U32   u32LumaData[4];
    MI_VPE_RegionInfo_t stRegionInfo;
    int i = 0;
    int index = 0;
    int opt = 0;
    int max = 0;

    test_vpe_TestIqMenu_t *pstMenu = NULL;
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

    _gbStop = TRUE;
    return 0;
}

//int test_vpe_TestCase008_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    time_t stTime = 0;
    char src_file[256];
    char dest_file[256];
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    int cnt = 0;
    pthread_t thread;
    pthread_attr_t attr;
    int s;

    s = pthread_attr_init(&attr);
    if (s != 0)
        perror("pthread_attr_init");

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 2)
    {
        printf("%s <test_dir>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_008_DESC);
        printf("Channel: %d.\n", 0);
        printf("InputFile: %s.\n", stTest008.inputFile);
        printf("OutputFile: %s.\n", stTest008.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s\n", argv[0], argv[1]);
    pbaseDir = argv[1];
    sprintf(src_file, "%s/%s", pbaseDir, stTest008.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest008.src_fd), TRUE);
    sprintf(dest_file, "%s/%s", pbaseDir, stTest008.stOutPort[0].outputFile);
    ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest008.stOutPort[0].dest_fd), TRUE);
    stTest008.count = 0;
    stTest008.src_offset = 0;
    stTest008.src_count  = 0;
    stTest008.stOutPort[0].dest_offset = 0;
    stTest008.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest008.stCropWin, &stTest008.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420);

    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stVpeMode.u16Width = stTest008.stOutPort[0].stPortWin.u16Width;
    stVpeMode.u16Height= stTest008.stOutPort[0].stPortWin.u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    // set vpe port buffer depth
    stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnInputPort0.u32DevId = 0;
    stVpeChnInputPort0.u32ChnId = 0;
    stVpeChnInputPort0.u32PortId = 0;

    stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort0.u32DevId = 0;
    stVpeChnOutputPort0.u32ChnId = 0;
    stVpeChnOutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 1, 3);
    pthread_create(&thread, &attr, test_vpe_TestIq, &_gbStop);
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    struct timeval stTv;
    int y_size = 0;
    int uv_size = 0;

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        gettimeofday(&stTv, NULL);
        stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest008.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest008.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            #if 0
            // Start user put int buffer
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            //test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrame(stTest008.src_fd, stTest008.src_offset, stBufInfo.stFrameData.pVirAddr[0], frame_size))
            {
                stTest008.src_offset += frame_size;
                MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0], frame_size);
                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest008.src_offset = 0;
                stTest008.src_count = 0;
                test_vpe_FdRewind(stTest008.src_fd);
            }
            #else
            // Start user put int buffer
            width   = stBufInfo.stFrameData.u32Stride[0];
            height  = stBufInfo.stFrameData.u16Height;
            y_size  = width*height;
            width   = stBufInfo.stFrameData.u32Stride[1];
            uv_size  = width*height/2;
            test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
            if (1 == test_vpe_GetOneFrameYUV420(stTest008.src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size))
            {
                stTest008.src_offset += y_size + uv_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                stTest008.src_offset = 0;
                stTest008.src_count = 0;
                test_vpe_FdRewind(stTest008.src_fd);
            }
            #endif
        }

        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = 0;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_VPE_TEST_DBG("%s()@line%d: Start Get output buffer.\n", __func__, __LINE__);

        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
        {
            MI_VPE_TEST_DBG("get output buf ok\n");
            MI_VPE_TEST_DBG("@@@--->buf type %d\n", stBufInfo.eBufType);
#if 0
            // Add user write buffer to file
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            // put frame

            //test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(stTest008.stOutPort[0].dest_fd, stTest008.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
            stTest008.stOutPort[0].dest_offset += frame_size;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
#else
            // Add user write buffer to file
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(stTest008.stOutPort[0].dest_fd, stTest008.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width, height);
            stTest008.stOutPort[0].dest_offset += frame_size;
            test_vpe_PutOneFrame(stTest008.stOutPort[0].dest_fd, stTest008.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u32Stride[1], width, height/2);
            stTest008.stOutPort[0].dest_offset += frame_size/2;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);

#endif
            MI_VPE_TEST_DBG("Test %d Pass.\n", cnt++);
        }
    }while (_gbStop == FALSE);
    pthread_join(thread, NULL);
    s = pthread_attr_destroy(&attr);
    if (s != 0)
        perror("pthread_attr_destroy");

    test_vpe_CloseFd(stTest008.src_fd);
    test_vpe_CloseFd(stTest008.stOutPort[0].dest_fd);
    sync();

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
