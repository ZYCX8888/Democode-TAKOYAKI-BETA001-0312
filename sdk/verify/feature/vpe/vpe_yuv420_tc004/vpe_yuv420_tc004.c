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

#include "../mi_vpe_test.h"
#if 0
static test_vpe_Config stTest004 = {
    .inputFile  = TEST_VPE_CHNN_FILE420(004, 0, 960x540),
    .stSrcWin   = {0, 0, 960, 540},
    .stCropWin  = {0, 0, 960, 540},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(004, 0, 0, 480x288),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 480, 288},
        },
    },
};
#else
static test_vpe_Config stTest004 = {
    .inputFile  = TEST_VPE_CHNN_FILE420(004, 0, 960x540),
    .stSrcWin   = {0, 0, 960, 540},
    .stCropWin  = {64, 32, 640, 360},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(004, 0, 0, 480x288),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 480, 288},
        },
    },
};

#endif
#define BONDER_COLOR (180 | (12) << 8)
#define DRAW_FONT  (4)
#define MIN_WIDTH  (64)
#define MIN_HEIGHT (64)
static MI_SYS_WindowRect_t stCropLast = {0, 0, 0, 0};

MI_BOOL test_crop_ZoomOut(test_vpe_Config *pstTest, MI_SYS_WindowRect_t *pstCropInfo, MI_BOOL *pEnd)
{
    MI_BOOL bEnd = FALSE;
    if ((stCropLast.u16X == 0)
        && (stCropLast.u16Y == 0)
        && (stCropLast.u16Width== 0)
        && (stCropLast.u16Height== 0))
    {
        pstCropInfo->u16X = (pstTest->stSrcWin.u16Width - MIN_WIDTH)/2;
        pstCropInfo->u16Y = (pstTest->stSrcWin.u16Height - MIN_HEIGHT)/2;
        pstCropInfo->u16Width = MIN_WIDTH;
        pstCropInfo->u16Height = MIN_HEIGHT;
        memcpy(&stCropLast, pstCropInfo, sizeof(*pstCropInfo));
    }
    else
    {
        stCropLast.u16X -= 2;
        stCropLast.u16Y -= 1;
        stCropLast.u16Width += 3;
        stCropLast.u16Height += 3;

        if (!((stCropLast.u16X > 0) && (stCropLast.u16X < ((1 << 15) - 1) )))
        {
            stCropLast.u16X = 0;
        }

        //stCropLast.u16Y = (stCropLast.u16Y > 0) ? 0:stCropLast.u16Y;
        if (!((stCropLast.u16Y > 0) && (stCropLast.u16Y < ((1 << 15) - 1) )))
        {
            stCropLast.u16Y = 0;
        }


        if((stCropLast.u16X + stCropLast.u16Width) > pstTest->stSrcWin.u16Width)
        {
            stCropLast.u16Width = pstTest->stSrcWin.u16Width - stCropLast.u16X;
        }

        if((stCropLast.u16Y + stCropLast.u16Height) > pstTest->stSrcWin.u16Height)
        {
            stCropLast.u16Height = pstTest->stSrcWin.u16Height - stCropLast.u16Y;
        }

        memcpy(pstCropInfo, &stCropLast, sizeof(*pstCropInfo));

        if ((pstCropInfo->u16X == pstTest->stSrcWin.u16X)
            &&(pstCropInfo->u16Y == pstTest->stSrcWin.u16Y)
            &&(pstCropInfo->u16Height== pstTest->stSrcWin.u16Height)
            &&(pstCropInfo->u16Width == pstTest->stSrcWin.u16Width)
        )
        {
            bEnd = TRUE;
            memset(&stCropLast, 0, sizeof(stCropLast));
        }
    }

    *pEnd = bEnd;
    return TRUE;
}

MI_BOOL test_crop_Pos(test_vpe_Config *pstTest, MI_SYS_WindowRect_t *pstCropInfo, MI_BOOL *pEnd)
{
    MI_BOOL bEnd = FALSE;
    MI_U32 u32Width  = 100;
    MI_U32 u32Height = 80;

    if ((stCropLast.u16X == 0)
        && (stCropLast.u16Y == 0)
        && (stCropLast.u16Width== 0)
        && (stCropLast.u16Height== 0))
    {
        pstCropInfo->u16X = 0;
        pstCropInfo->u16Y = 0;
        pstCropInfo->u16Width  = u32Width;
        pstCropInfo->u16Height = u32Height;
        memcpy(&stCropLast, pstCropInfo, sizeof(*pstCropInfo));
    }
    else
    {
        stCropLast.u16X += 2;
        stCropLast.u16Y += 1;


        if((stCropLast.u16X + stCropLast.u16Width) > pstTest->stSrcWin.u16Width)
        {
            stCropLast.u16X = pstTest->stSrcWin.u16Width - stCropLast.u16Width;
        }

        if((stCropLast.u16Y + stCropLast.u16Height) > pstTest->stSrcWin.u16Height)
        {
            stCropLast.u16Y = pstTest->stSrcWin.u16Height - stCropLast.u16Height;
        }

        memcpy(pstCropInfo, &stCropLast, sizeof(*pstCropInfo));

        if ((pstCropInfo->u16X == (pstTest->stSrcWin.u16Width - stCropLast.u16Width))
            &&(pstCropInfo->u16Y == (pstTest->stSrcWin.u16Height - stCropLast.u16Height))
        )
        {
            bEnd = TRUE;
            memset(&stCropLast, 0, sizeof(stCropLast));
        }
    }

    *pEnd = bEnd;
    return TRUE;
}

MI_BOOL test_crop_PosZoom(test_vpe_Config *pstTest, MI_SYS_WindowRect_t *pstCropInfo, MI_BOOL *pEnd)
{
    MI_BOOL bEnd = FALSE;

    if ((stCropLast.u16X == 0)
        && (stCropLast.u16Y == 0)
        && (stCropLast.u16Width== 0)
        && (stCropLast.u16Height== 0))
    {
        pstCropInfo->u16X = 0;
        pstCropInfo->u16Y = 0;
        pstCropInfo->u16Width = MIN_WIDTH;
        pstCropInfo->u16Height = MIN_HEIGHT;
        memcpy(&stCropLast, pstCropInfo, sizeof(*pstCropInfo));
    }
    else
    {

        if((stCropLast.u16X + stCropLast.u16Width) < pstTest->stSrcWin.u16Width)
        {
            stCropLast.u16X += 2;
            stCropLast.u16Width += 1;
        }

        if((stCropLast.u16Y + stCropLast.u16Height) < pstTest->stSrcWin.u16Height)
        {
            stCropLast.u16Height += 1;
            stCropLast.u16Y += 1;
        }


        if ((stCropLast.u16X + stCropLast.u16Width) > pstTest->stSrcWin.u16Width)
        {
            stCropLast.u16Width = pstTest->stSrcWin.u16Width - stCropLast.u16X;
            if (stCropLast.u16Width < MIN_WIDTH)
            {
                stCropLast.u16Width = MIN_WIDTH;
                stCropLast.u16X     = pstTest->stSrcWin.u16Width - stCropLast.u16Width;
            }
        }

        if ((stCropLast.u16Y + stCropLast.u16Height)> pstTest->stSrcWin.u16Height)
        {
            stCropLast.u16Height = pstTest->stSrcWin.u16Height - stCropLast.u16Y;
            if (stCropLast.u16Height < MIN_HEIGHT)
            {
                stCropLast.u16Height = MIN_HEIGHT;
                stCropLast.u16Y      = pstTest->stSrcWin.u16Height - stCropLast.u16Height;
            }
        }

        memcpy(pstCropInfo, &stCropLast, sizeof(*pstCropInfo));
        if (((stCropLast.u16X + stCropLast.u16Width) == pstTest->stSrcWin.u16Width)
            &&((stCropLast.u16Y + stCropLast.u16Height)== pstTest->stSrcWin.u16Height)
        )
        {
            bEnd = TRUE;
            memset(&stCropLast, 0, sizeof(stCropLast));
        }
    }

    *pEnd = bEnd;
    return TRUE;
}


//int test_vpe_TestCase004_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VPE_CHANNEL VpeChannel;
    MI_VPE_PORT VpePort;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    time_t stTime = 0;
    char src_file[256];
    char dest_file[256];
    int cnt = 0;
    int source_cnt = 0;
    const char *pbaseDir = NULL;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    MI_SYS_WindowRect_t stCropInfo;
    int font;
    MI_BOOL bEnd = FALSE;
    MI_BOOL bSwitch = FALSE;
    int step = 0;
    int y_size = 0;
    int uv_size = 0;
    int totalCnt = 0;

    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_004_DESC);
        printf("Channel: %d.\n", i);
        printf("InputFile: %s.\n", stTest004.inputFile);
        printf("OutputFile: %s.\n", stTest004.stOutPort[0].outputFile);
        return 0;
    }

    printf("%s %s %s\n", argv[0], argv[1], argv[2]);
    pbaseDir = argv[1];
    totalCnt = atoi(argv[2]);
    cnt = 0;// atoi(argv[2]);
    sprintf(src_file, "%s/%s", pbaseDir, stTest004.inputFile);
    ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest004.src_fd), TRUE);
    sprintf(dest_file, "%s/%s", pbaseDir, stTest004.stOutPort[0].outputFile);
    ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest004.stOutPort[0].dest_fd), TRUE);
    stTest004.count = 0;
    stTest004.src_offset = 0;
    stTest004.src_count  = 0;
    stTest004.stOutPort[0].dest_offset = 0;
    stTest004.product = 0;

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);

    // Create VPE channel
    VpeChannel = 0;
    VpePort = 0;
    test_vpe_CreatChannel(VpeChannel, VpePort, &stTest004.stCropWin, &stTest004.stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

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

    do {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;
        if (cnt > totalCnt)
        {
            break;
        }
        memset(&stBufConf , 0, sizeof(stBufConf));
        MI_VPE_TEST_DBG("%s()@line: Start get input buffer.\n", __func__, __LINE__);
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.u64TargetPts = time(&stTime);
        stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Width = stTest004.stSrcWin.u16Width;
        stBufConf.stFrameCfg.u16Height = stTest004.stSrcWin.u16Height;
        if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle , 0))
        {
            if (stBufInfo.stFrameData.pVirAddr[0] != NULL)
            {
                // Start user put int buffer
                width   = stBufInfo.stFrameData.u16Width;
                height  = stBufInfo.stFrameData.u16Height;
                y_size  = width*height;
                uv_size  = width*height/2;
                test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
                if (1 == test_vpe_GetOneFrameYUV420(stTest004.src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size))
                {
                    memset(&stCropInfo, 0, sizeof(stCropInfo));
                    stTest004.src_offset += y_size + uv_size;

                    if (stCropInfo.u16Width >4 && stCropInfo.u16Height > 4)
                    {
                        font = DRAW_FONT;
                    }
                    else
                    {
                        font = 1;
                    }
                    #if 0
                    bSwitch = FALSE;
                    switch (step)
                    {
                        case 0:
                        {
                            test_crop_ZoomOut(&stTest004, &stCropInfo, &bSwitch);
                            if (bSwitch == TRUE)
                            {
                                step++;
                            }
                        }
                        break;
                        case 1:
                        {
                            test_crop_Pos(&stTest004, &stCropInfo, &bSwitch);
                            stTest004.src_offset = 0;
                            stTest004.src_count = 0;
                            test_vpe_FdRewind(stTest004.src_fd);

                            if (bSwitch == TRUE)
                            {
                                step = 3;
                            }
                        }
                        break;
                        case 2:
                        {
                            test_crop_PosZoom(&stTest004, &stCropInfo, &bSwitch);
                            if (bSwitch == TRUE)
                            {
                                step++;
                            }
                        }
                        break;
                        case 3:
                        {
                            test_crop_ZoomOut(&stTest004, &stCropInfo, &bSwitch);
                            if (bSwitch == TRUE)
                            {
                                step++;
                            }
                        }
                        break;
                        default:
                            bEnd = TRUE;
                        break;
                    }
                    #endif
                    font = DRAW_FONT;
                    stCropInfo.u16X = 64;
                    stCropInfo.u16Y = 32;
                    stCropInfo.u16Width = 640;
                    stCropInfo.u16Height = 360;
                    //test_vpe_WriteBonderYuv422(BONDER_COLOR, font, &stCropInfo, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0]);
                    test_vpe_WriteBonderYuv420Luma(BONDER_COLOR, font, &stCropInfo, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0]);
                    //test_vpe_WriteBonderYuv420Chrom(BONDER_COLOR, font, &stCropInfo, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u32Stride[1]);

                    MI_VPE_SetChannelCrop(VpeChannel, &stCropInfo);
                    MI_VPE_TEST_INFO("Step: %d cropWin = {x: %u, y: %u, width: %u, height: %u}.\n", step,
                        stCropInfo.u16X, stCropInfo.u16Y, stCropInfo.u16Width, stCropInfo.u16Height);

                    MI_VPE_TEST_DBG("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                }
                else
                {
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                    stTest004.src_offset = 0;
                    stTest004.src_count = 0;
                    test_vpe_FdRewind(stTest004.src_fd);
                }
            }
            else
            {
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                MI_VPE_TEST_INFO("Get VA is NULL !!!!.\n");
            }
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
            // Add user write buffer to file
            width  = stBufInfo.stFrameData.u16Width;
            height = stBufInfo.stFrameData.u16Height;
            frame_size = width*height*YUV422_PIXEL_PER_BYTE;
            // put frame

            test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
            test_vpe_PutOneFrame(stTest004.stOutPort[0].dest_fd, stTest004.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
            stTest004.stOutPort[0].dest_offset += frame_size;

            MI_VPE_TEST_DBG("%s()@line%d: Start put output buffer.\n", __func__, __LINE__);
            MI_SYS_ChnOutputPortPutBuf(hHandle);

            MI_VPE_TEST_INFO("Test %d Pass.\n", cnt);
            cnt++;
        }
    }while (bEnd == FALSE);

    test_vpe_CloseFd(stTest004.src_fd);
    test_vpe_CloseFd(stTest004.stOutPort[0].dest_fd);
    sync();

    test_vpe_DestroyChannel(VpeChannel, VpePort);
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
