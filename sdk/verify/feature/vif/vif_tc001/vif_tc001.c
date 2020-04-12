/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "mi_vif.h"
#include "mi_sys.h"
#include "../i2c.h"
#include "../memmap.h"
#include "../AddaSysApi.h"
#include "../adda_resource.h"

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("Test [%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("Test [%d]exec function pass\n", __LINE__);\
    }

    
static void adda_config(int dev, int max_chs, int edge, int pclk , ADS_RES_S* pAddaInitRes)
{
    unsigned int ChToI2c[] = {0x60, 0x62, 0x64, 0x66}; //channel to device i2c slave address
    unsigned int tmux_mode = 1;

    switch(max_chs)
    {
        case 1:
            tmux_mode = VD_PORT_MUX_MODE_1MUX;
            break;

        case 2:
            tmux_mode = VD_PORT_MUX_MODE_2MUX;
            break;

        case 4:
            tmux_mode = VD_PORT_MUX_MODE_4MUX;
            break;

        default:
            break;
    }

    int pclk_sel = 0;

    switch(pclk)
    {
        case 0:
            pclk_sel = VD_PORT_CKL_AUTO;
            break;
        case 1:
            pclk_sel = VD_PORT_CKL_27M;
            break;
        case 2:
            pclk_sel = VD_PORT_CKL_74_25M;
            break;
        case 3:
            pclk_sel = VD_PORT_CKL_148_5M;
            break;
        case 4:
            pclk_sel = VD_PORT_CKL_36M;
        default:
            break;
    }

    pAddaInitRes->iCount = 1;
    pAddaInitRes->iChipTypeNum = 1;

    pAddaInitRes->AdRes[0].iBusId = 0;
    pAddaInitRes->AdRes[0].iChipAddr = ChToI2c[dev];
    pAddaInitRes->AdRes[0].ucChnCount = 4; //max channels

    pAddaInitRes->AdRes[0].stChnMap[0].ucLogicChn = dev*4;
    pAddaInitRes->AdRes[0].stChnMap[0].ucPhyChn = 0;
    pAddaInitRes->AdRes[0].stChnMap[1].ucLogicChn = dev*4 + 1;
    pAddaInitRes->AdRes[0].stChnMap[1].ucPhyChn = 1;
    pAddaInitRes->AdRes[0].stChnMap[2].ucLogicChn = dev*4 + 2;
    pAddaInitRes->AdRes[0].stChnMap[2].ucPhyChn = 2;
    pAddaInitRes->AdRes[0].stChnMap[3].ucLogicChn = dev*4 + 3;
    pAddaInitRes->AdRes[0].stChnMap[3].ucPhyChn = 3;

    pAddaInitRes->AdRes[0].enEqMode = EQ_MODE_IN;/*使用内部EQ*/

    pAddaInitRes->AdRes[0].stVdPortDev.ucCount = 1; /*One port config*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdportClk = pclk_sel;
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdportEdge = (edge == 0) ? VD_PORT_EDGE_UP : VD_PORT_EDGE_DUAL;
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdPortMuxMode = tmux_mode; /*复用模式*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
    pAddaInitRes->AdRes[0].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/
    pAddaInitRes->AdRes[0].enDriverPower = DRIVER_POWER_18V;/*BT656的逻辑电平*/
}
uint8_t i2c_dummy_write(uint8_t i2c_addr, uint16_t reg_addr, uint8_t val, uint8_t bus_id)
{
    vif_i2c_write(i2c_addr, reg_addr, val, I2C_FMT_A16D8);
    return 0;
}

uint8_t i2c_dummy_read(uint8_t i2c_addr, uint16_t reg_addr, uint8_t bus_id)
{
    uint16_t reg_val = 0;
    vif_i2c_read(i2c_addr, reg_addr, &reg_val, I2C_FMT_A16D8);
    return (uint8_t)(reg_val & 0xFF);
}

void reset_dummy(void)
{
    printf("adda hardware reset successfully!!\n");

    return ;
}


int vif_dh9931_Init(int VifDev , int channel, int pclk, int edge)
{
    unsigned short regVal = 0;
    int i;
    int ChnnaleOffset = 0, ChannelRange = 0;

    pid_t pid;
    AD_DETECT_STATUS_S ad_status;
    ADS_RES_S AddaInitRes;
    MI_U32 count = 3;

    printf("dh9931 dev %d init chn:%d pclk %d begin!\n", VifDev, channel, pclk);
    while(count--)
    {
        vif_i2c_read(0x60 + 2 * VifDev, 0x0507, &regVal, I2C_FMT_A16D8);

        if ((regVal != 0x1c) && (0 == count))
        {
            printf("dev %d i2c status err %u\n", VifDev, regVal);
            //return -1;
        }
        usleep(1000*1000);
    }
    pid = vfork();

    if(pid == -1)
    {
        printf("create adda process fail!\n");
        return -1;
    }
    else if(pid == 0)
    {
        vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);

        Adda_I2COpsRegister(i2c_dummy_write, i2c_dummy_read);
        Adda_HWResetOpsRegister(reset_dummy);

        memset(&AddaInitRes, 0, sizeof(AddaInitRes));

        adda_config(VifDev, channel, edge, pclk , &AddaInitRes);
        Adda_Resource_Init(&AddaInitRes);
        Adda_Init();
        if(VifDev == 2)
        {
            unsigned short regval =0;
            vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x23, I2C_FMT_A16D8);

            vif_i2c_read(0x60 + 2 * VifDev, 0x0802, &regval, I2C_FMT_A16D8);

            printf("dev2 read reg is 0x%x\n", regval);
        }
        else
            vif_i2c_write(0x60 + 2 * VifDev, 0x0802, 0x30, I2C_FMT_A16D8);

        printf("dh9931 dev %d init chn:%d pclk %d line:%d!\n", VifDev, channel, pclk, __LINE__);
        ChnnaleOffset = MI_VIF_MAX_WAY_NUM_PER_DEV * VifDev;
        if (pclk == 3)
        {
            ChannelRange = channel;
        }
        else if (pclk == 1)
        {
            ChannelRange = channel;
        }
        else if (2 == pclk)
        {
            if (4 == channel)
            {
                ChannelRange = 4;
            }
            else
            {
                ChannelRange = 1;
            }
        }
        else
        {
            ChannelRange = channel;
        }
        for(i = ChnnaleOffset; i < ChnnaleOffset + ChannelRange; i++)
        {
            count = 12;
            while (count--)
            {
                usleep(500*1000);
                Adda_GetVideoInStatus(i, &ad_status);
                printf("\nchn %d vfmt: %d, lost: %d, report fmt: %d, sigtype: %d\n", i,
                    ad_status.iVideoFormat, ad_status.iLostStatus, ad_status.iReportFormat, ad_status.iVideoSignalType);
                if (1 == ad_status.iLostStatus)
                {
                    printf("Video lost Chn(%d)!!!\n", i);
                    continue;
                }

                if (((AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat) || (AD_VIDEO_IN_SD_PAL == ad_status.iVideoFormat))
                    && (VIDEO_TYPE_CVBS == ad_status.iVideoSignalType))
                {
                    printf("Channel(%d) Detect video format CVBS(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else if ((AD_VIDEO_IN_HD_1080P_25HZ == ad_status.iVideoFormat) && (VIDEO_TYPE_HDCVI == ad_status.iVideoSignalType))
                {
                    printf("Channel(%d) Detect video format HDCVI format(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else if (AD_VIDEO_IN_HD_720P_25HZ == ad_status.iVideoFormat)
                {
                    printf("Channel(%d) Detect video format 720P(%d)\n", i, ad_status.iVideoFormat);
                    goto detect_ok;
                }
                else
                {
                    printf("Detect video format incorrect!!!\n");
                    continue;
                }
            }
  detect_ok:
            count = 0;
        }
        exit(0);
    }
    if(pid == wait(NULL))
    {
        printf("vfork =================== %d ======================\n", pid);
    }

    return 0;
}

MI_BOOL test_vif_OpenDestFile(const char *pFileName, int *pDestFd)
{
    int dest_fd = open(pFileName, O_WRONLY|O_CREAT, 0777);
    if (dest_fd < 0)
    {
        printf("dest_file: %s.\n", pFileName);
        perror("open");
        return -1;
    }

    *pDestFd = dest_fd;

    return TRUE;
}

MI_S32 test_vif_WriteOneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;
    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for (i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset*i;
        yuvSize = line_size;
        do {
            if (yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }

            size = write(dstFd, pData, size);
            if (size == 0)
            {
                break;
            }
            else if (size < 0)
            {
                break;
            }
            pData += size;
            yuvSize -= size;
        } while (yuvSize > 0);
    }

    return 0;
}

MI_BOOL bThreadRun = TRUE;
void *test_vif_getoutputbuffer(void *args)
{
    MI_SYS_ChnPort_t stVifChn0Port0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 u32Stride =0;
    MI_U16 u16Width =0;
    MI_U16 u16Height =0;
    
    MI_U32 u32FrameSize = 0;
    int DestFd = *((int *)args);
    MI_U32 u32Offset =0;
    
    stVifChn0Port0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn0Port0.u32DevId = 0;
    stVifChn0Port0.u32ChnId = 0;
    stVifChn0Port0.u32PortId = 0;

    while(bThreadRun)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVifChn0Port0 , &stBufInfo, &hHandle))
        {
            // Add user write buffer to file
            u16Width  = stBufInfo.stFrameData.u16Width;
            u16Height = stBufInfo.stFrameData.u16Height;
            u32Stride = stBufInfo.stFrameData.u32Stride[0];
            u32FrameSize = u16Height*u32Stride;
            // put frame
            test_vif_WriteOneFrame(DestFd,u32Offset,stBufInfo.stFrameData.pVirAddr[0], u32Stride, u16Width, u16Height);
            u32Offset += u32FrameSize;
            test_vif_WriteOneFrame(DestFd,u32Offset,stBufInfo.stFrameData.pVirAddr[1], u32Stride, u16Width, u16Height/2);

            /*test_vpe_PutOneFrame(stTest001.stOutPort[0].dest_fd, stTest001.stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
            stTest001.stOutPort[0].dest_offset += frame_size;*/
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
    }
    return NULL;
}

int main(int argc, const char *argv[])
{
     char cmd = 0;
    MI_VIF_DEV VifDev =0;
    MI_VIF_CHN VifChn =0;
    MI_VIF_PORT VifPort = 0;
    /*0: Auto, 1: 27M, 2: 74.25M, 3:148.5M, 4: 36M*/
    MI_S32 pclk = 3;
    MI_S32 channel = 1;
    /*0: up, 1: double*/
    MI_S32 edge = 0;

    char dest_file[256] = "vif_port0_1920x1080yuv420.yuv";
    int destFd =0;
    test_vif_OpenDestFile(dest_file, &destFd);

    vif_i2c_init();
    ExecFunc(vif_dh9931_Init(VifDev, channel, pclk, edge), 0);

    MI_VIF_DevAttr_t stDevAttr;

    stDevAttr.eIntfMode = E_MI_VIF_MODE_BT656;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_1MULTIPLEX;
    stDevAttr.au32CompMask[0] =0xFF000000;
    stDevAttr.au32CompMask[1] =0x0;
    stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_SINGLE_UP;
    stDevAttr.as32AdChnId[0] = -1;
    stDevAttr.as32AdChnId[1] = -1;
    stDevAttr.as32AdChnId[2] = -1;
    stDevAttr.as32AdChnId[3] = -1;
    stDevAttr.eDataSeq =E_MI_VIF_INPUT_DATA_YUYV;
    stDevAttr.stSynCfg.eVsync = E_MI_VIF_VSYNC_FIELD;
    stDevAttr.stSynCfg.eVsyncNeg = E_MI_VIF_VSYNC_NEG_HIGH;
    stDevAttr.stSynCfg.eHsync = E_MI_VIF_HSYNC_VALID_SINGNAL;
    stDevAttr.stSynCfg.eHsyncNeg = E_MI_VIF_HSYNC_NEG_HIGH;
    stDevAttr.stSynCfg.eVsyncValid = E_MI_VIF_VSYNC_VALID_SINGAL;
    stDevAttr.stSynCfg.eVsyncValidNeg = E_MI_VIF_VSYNC_VALID_NEG_HIGH;
    stDevAttr.stSynCfg.stTimingBlank.u32HsyncAct =0;
    stDevAttr.stSynCfg.stTimingBlank.u32HsyncHbb =0;
    stDevAttr.stSynCfg.stTimingBlank.u32HsyncHfb =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVact =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVbact =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVbb =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVbbb =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVbfb =0;
    stDevAttr.stSynCfg.stTimingBlank.u32VsyncVfb =0;
    stDevAttr.bDataRev = FALSE;
   
    ExecFunc(MI_VIF_SetDevAttr(VifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(VifDev), MI_SUCCESS);

    MI_VIF_ChnPortAttr_t stChnPortAttr;
    stChnPortAttr.stCapRect.u16X = 0;
    stChnPortAttr.stCapRect.u16Y = 0;
    stChnPortAttr.stCapRect.u16Width = 1920;
    stChnPortAttr.stCapRect.u16Height = 1080;
    stChnPortAttr.stDestSize.u16Width = 1920;
    stChnPortAttr.stDestSize.u16Height = 1080;
    stChnPortAttr.eCapSel = E_MI_SYS_FIELDTYPE_BOTH;
    stChnPortAttr.eScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stChnPortAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stChnPortAttr.bMirror = FALSE;
    stChnPortAttr.bFlip = FALSE;
    stChnPortAttr.eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    stChnPortAttr.u32FrameModeLineCount = 0;

    ExecFunc(MI_VIF_SetChnPortAttr(VifChn, VifPort, &stChnPortAttr), MI_SUCCESS);

    ExecFunc(MI_VIF_EnableChnPort(VifChn, VifPort), MI_SUCCESS);

    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VifChn;
    stChnPort.u32PortId = VifPort;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4);
    pthread_t pt_WriteFile;
    pthread_attr_t attr;
    int s;
    s = pthread_attr_init(&attr);
    if (s != 0)
        perror("pthread_attr_init");
    pthread_create(&pt_WriteFile, &attr, test_vif_getoutputbuffer, (void *)(&destFd));

    while(1)
    {
        printf("Type \"q\" to exit\n");
        cmd = getchar();
        if (cmd == 'q')
        {
            bThreadRun = FALSE;
            break;
        }
    }
    
    pthread_join(pt_WriteFile, NULL);
    s = pthread_attr_destroy(&attr);
    if (s != 0)
        perror("pthread_attr_destroy");

     close(destFd);
     ExecFunc(MI_VIF_DisableChnPort(VifChn, VifPort), MI_SUCCESS);
     ExecFunc(MI_VIF_DisableDev(VifDev), MI_SUCCESS);
     
    return MI_SUCCESS;
}
