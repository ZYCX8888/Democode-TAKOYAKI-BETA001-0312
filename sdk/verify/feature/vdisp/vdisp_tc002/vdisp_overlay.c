#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <mi_sys_datatype.h>
#include <mi_sys.h>
#include <mi_disp_datatype.h>
#include <mi_disp.h>
#include <mi_vdisp_datatype.h>
#include <mi_vdisp.h>
#include <mi_gfx_datatype.h>
#include <mi_gfx.h>
#include <sys/time.h>

#include "../common/sstardisp.h"
#include "../common/vdisp_common.h"

#include <signal.h>

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}



int main(int argc, char **argv)
{

    MI_VDISP_DEV vdisp_dev = 0;
    MI_SYS_ChnPort_t stSrcChnPort, stDstChnPort;
    MI_DISP_PubAttr_t  stDispPubAttr;
    vdispOutputMeta outMeta;
    vdispInputMeta MainInChnMeta;
    vdispInputMeta OverlayInChnMeta;

    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
    sstar_disp_init(&stDispPubAttr);
    stTimingArray_t *pstTiming = gettiming(stDispPubAttr.eIntfSync);

    MI_VDISP_OutputPortAttr_t stOutPortAttr;
    stOutPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutPortAttr.u32BgColor = YUYV_BLUE;
    stOutPortAttr.u32FrmRate = 30;
    stOutPortAttr.u32Height = pstTiming->u16Height;
    stOutPortAttr.u32Width = pstTiming->u16Width;
    stOutPortAttr.u64pts = 0 ;

    MI_VDISP_InputChnAttr_t stMainInportAttr;
    stMainInportAttr.s32IsFreeRun = TRUE;
    stMainInportAttr.u32OutHeight = pstTiming->u16Height / 2;
    stMainInportAttr.u32OutWidth = pstTiming->u16Width / 2;
    stMainInportAttr.u32OutX = 0;
    stMainInportAttr.u32OutY = 0;

    MI_VDISP_InputChnAttr_t stOverlayInportAttr;

    stOverlayInportAttr.s32IsFreeRun = TRUE;
    stOverlayInportAttr.u32OutHeight = pstTiming->u16Height / 2;
    stOverlayInportAttr.u32OutWidth = pstTiming->u16Width / 2;
    stOverlayInportAttr.u32OutX = 100;
    stOverlayInportAttr.u32OutY = 100;

    outMeta.outAttr = stOutPortAttr;
    MainInChnMeta.inAttr = stMainInportAttr;
    OverlayInChnMeta.inAttr = stOverlayInportAttr;

    char fileName[128];
    memset(fileName, 0, 128);
    sprintf(fileName, "../data/yuv420sp_%dx%d.yuv", stMainInportAttr.u32OutWidth, \
            stMainInportAttr.u32OutHeight);

    FILE *fp = fopen(fileName, "r");

    if(fp == NULL) {
        printf("%s %d open %s fail\n", __FUNCTION__, __LINE__, fileName);
        return -1;
    }
    signal(SIGINT, intHandler);

    MI_VDISP_Init();
    MI_VDISP_OpenDevice(0);

    MI_VDISP_SetInputChannelAttr(vdisp_dev, 0, &stMainInportAttr);
    MI_VDISP_SetInputChannelAttr(vdisp_dev, VDISP_OVERLAYINPUTCHNID, &stOverlayInportAttr);
    MI_VDISP_SetOutputPortAttr(vdisp_dev, 0, &stOutPortAttr);

    MI_VDISP_EnableInputChannel(vdisp_dev, 0);
    MI_VDISP_EnableInputChannel(vdisp_dev, VDISP_OVERLAYINPUTCHNID);

    stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32DevId = vdisp_dev;
    stSrcChnPort.u32PortId = 0;
    stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32PortId = 0;

    MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, 30, 30);
    
    MI_SYS_SetChnOutputPortDepth(&stSrcChnPort, 0, 5);
    MI_VDISP_StartDev(0);


    MI_U64 u64PhyAddr;
    void *pVirtualAddress;
    MI_U32 ret;
allocdupbuf:
    ret = MI_SYS_MMA_Alloc(NULL, \
                           _MI_VDISP_IMPL_CalFrameSize(stOutPortAttr.ePixelFormat, \
                                   stMainInportAttr.u32OutWidth, stMainInportAttr.u32OutHeight), \
                           &u64PhyAddr);

    if(ret != MI_SUCCESS) {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }

    ret = MI_SYS_Mmap(u64PhyAddr, \
                      _MI_VDISP_IMPL_CalFrameSize(stOutPortAttr.ePixelFormat, \
                              stMainInportAttr.u32OutWidth, stMainInportAttr.u32OutHeight), \
                      (&pVirtualAddress), FALSE);

    if(ret != MI_SUCCESS) {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        usleep(300 * 1000);
        goto allocdupbuf;
    }

    while(keepRunning) {
        MI_SYS_ChnPort_t stChnPort;
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32TimeOutMs = 10 ;
        vdisp_readFile(pVirtualAddress, fp, &stMainInportAttr);

        memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.stFrameCfg.eFormat = stOutPortAttr.ePixelFormat;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Height = stMainInportAttr.u32OutHeight;
        stBufConf.stFrameCfg.u16Width = stMainInportAttr.u32OutWidth;
        stBufConf.u64TargetPts = 0;
        stBufConf.u32Flags = MI_SYS_MAP_VA;


        memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stChnPort.u32ChnId = 0;
        stChnPort.u32DevId = vdisp_dev;
        stChnPort.u32PortId = 0;

getMainChnBuf:

        if(MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, \
                &MainInChnMeta.bufInfo, &MainInChnMeta.bufHandle, s32TimeOutMs)) {
            goto getMainChnBuf;
        }

        memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
        stChnPort.eModId = E_MI_MODULE_ID_VDISP;
        stChnPort.u32ChnId = VDISP_OVERLAYINPUTCHNID;
        stChnPort.u32DevId = vdisp_dev;
        stChnPort.u32PortId = 0;

getOverlayChnBuf:

        if(MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, \
                &OverlayInChnMeta.bufInfo, &OverlayInChnMeta.bufHandle, s32TimeOutMs)) {
            goto getOverlayChnBuf;
        }

        MI_SYS_FrameData_t outFrame;
        outFrame = MainInChnMeta.bufInfo.stFrameData;
        outFrame.phyAddr[1] = outFrame.phyAddr[0] + outMeta.outAttr.u32Height * outMeta.outAttr.u32Width;
        outFrame.pVirAddr[1] = outFrame.pVirAddr[0] + (outFrame.phyAddr[1] - outFrame.phyAddr[0]);

        MI_SYS_WindowRect_t stDstRect[3];
        MI_SYS_FrameData_t stSrcBuf;
        MI_SYS_WindowRect_t stSrcRect[3];
        MI_SYS_FrameData_t stDstBuf;

        vdisp_process_config(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect, \
                             pVirtualAddress, u64PhyAddr, &outFrame, \
                             &MainInChnMeta, &outMeta);

        vdisp_func_420sp(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect);
        MI_SYS_ChnInputPortPutBuf(MainInChnMeta.bufHandle, &MainInChnMeta.bufInfo, FALSE);
        /*
                vdisp_process_config(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect, \
                                     pVirtualAddress, u64PhyAddr, &outFrame, \
                                     &OverlayInChnMeta, &outMeta);

                vdisp_func_420sp(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect);
        */
        memcpy(OverlayInChnMeta.bufInfo.stFrameData.pVirAddr[0], pVirtualAddress, \
               OverlayInChnMeta.inAttr.u32OutWidth*OverlayInChnMeta.inAttr.u32OutHeight);
        memcpy(OverlayInChnMeta.bufInfo.stFrameData.pVirAddr[1], \
               pVirtualAddress + OverlayInChnMeta.inAttr.u32OutWidth*OverlayInChnMeta.inAttr.u32OutHeight, \
               OverlayInChnMeta.inAttr.u32OutWidth * OverlayInChnMeta.inAttr.u32OutHeight / 2);

        MI_SYS_ChnInputPortPutBuf(OverlayInChnMeta.bufHandle, &OverlayInChnMeta.bufInfo, FALSE);
        usleep(20 * 1000);
    }



    stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32DevId = vdisp_dev;
    stSrcChnPort.u32PortId = 0;
    stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32PortId = 0;

    MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort);

    sstar_disp_Deinit(&stDispPubAttr);

    MI_VDISP_DisableInputChannel(vdisp_dev, 0);
    MI_VDISP_DisableInputChannel(vdisp_dev,VDISP_OVERLAYINPUTCHNID);
    MI_VDISP_StopDev(vdisp_dev);
    MI_VDISP_CloseDevice(vdisp_dev);
    MI_VDISP_Exit();
    
    return 0;
}



