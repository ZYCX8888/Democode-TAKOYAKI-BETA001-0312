#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#include <mi_disp_datatype.h>
#include <mi_disp.h>
#include <mi_vdisp_datatype.h>
#include <mi_vdisp.h>
#include <mi_gfx_datatype.h>
#include <mi_gfx.h>
#include <sys/time.h>

#include "../common/sstardisp.h"
#include "../common/vdisp_common.h"

#define VDISP_TEST_DATA_FILE VDISP_TEST_DATA_FILE_960x540
#define VDISP_TEST_DATA_FILE_W 960
#define VDISP_TEST_DATA_FILE_H 540

int main(int argc, char **argv)
{

    if(argc < 3) {
        printf("usage: prog <cell num(1-16)> <fps> \n");
        exit(-1);
    }


    MI_U8 cellNum = atoi(argv[1]);
    MI_U8 fps = atoi(argv[2]);


    MI_VDISP_DEV vdisp_dev = 0;
    MI_SYS_ChnPort_t stSrcChnPort, stDstChnPort;
    MI_DISP_PubAttr_t  stDispPubAttr;


    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_1080P60;
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
    sstar_disp_init(&stDispPubAttr);
    stTimingArray_t *pstTiming = gettiming(stDispPubAttr.eIntfSync);


    vdispInputMeta *inPutMeta = calloc(sizeof(vdispInputMeta), cellNum);
    vdispOutputMeta *outPutMeta = calloc(sizeof(vdispOutputMeta), 1);
    vdisp_configOutputputMeta(outPutMeta, fps, pstTiming);
    vdisp_configInputMeta(inPutMeta, cellNum, outPutMeta);

    char fileName[128];
    memset(fileName, 0, 128);
    sprintf(fileName, "../data/yuv420sp_%dx%d.yuv", inPutMeta[0].inAttr.u32OutWidth, \
            inPutMeta[0].inAttr.u32OutHeight);

    FILE *fp = fopen(fileName, "r");

    if(fp == NULL) {
        printf("%s %d open %s fail\n", __FUNCTION__, __LINE__, fileName);
        free(inPutMeta);
        free(outPutMeta);
        return -1;
    }

    //cellNum = 1;

    MI_VDISP_Init();
    MI_VDISP_OpenDevice(0);
    //inPutMeta[1].inAttr.u32OutY = 540;
    //inPutMeta[0].inAttr.u32OutX = 0;
    //inPutMeta[0].inAttr.u32OutWidth = 960;
    //inPutMeta[0].inAttr.u32OutHeight = 540;


    for(int i = 0; i < cellNum; i++) {
        inPutMeta[i].enable = TRUE;
        MI_VDISP_SetInputChannelAttr(vdisp_dev, inPutMeta[i].inChn, &inPutMeta[i].inAttr);
    }

    MI_VDISP_SetOutputPortAttr(vdisp_dev, outPutMeta->outPort, &outPutMeta->outAttr);

    for(int i = 0; i < cellNum; i++) {
        MI_VDISP_EnableInputChannel(vdisp_dev, inPutMeta[i].inChn);
    }

    stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32DevId = vdisp_dev;
    stSrcChnPort.u32PortId = outPutMeta->outPort;
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
                           _MI_VDISP_IMPL_CalFrameSize(outPutMeta->outAttr.ePixelFormat, \
                                   inPutMeta[0].inAttr.u32OutWidth, inPutMeta[0].inAttr.u32OutHeight), \
                           &u64PhyAddr);

    if(ret != MI_SUCCESS) {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
    }

    ret = MI_SYS_Mmap(u64PhyAddr, \
                      _MI_VDISP_IMPL_CalFrameSize(outPutMeta->outAttr.ePixelFormat, \
                              inPutMeta[0].inAttr.u32OutWidth, inPutMeta[0].inAttr.u32OutHeight), \
                      (&pVirtualAddress), FALSE);

    if(ret != MI_SUCCESS) {
        printf("%s %d fail\n", __FUNCTION__, __LINE__);
        usleep(300 * 1000);
        goto allocdupbuf;
    }

    int cnt = 0;
    int cnt1= 0;
    
    struct timeval tv1;
    struct timeval tv0;
    
    while(1) {
        MI_SYS_ChnPort_t stChnPort;
        MI_SYS_BufConf_t stBufConf;
        MI_SYS_BufInfo_t stBufInfo;
        MI_SYS_BufInfo_t stBufInfo1;
        MI_SYS_BUF_HANDLE bufHandle;
        MI_SYS_BUF_HANDLE bufHandle1;
        MI_S32 s32TimeOutMs = 10 ;
        size_t n = 0;
        int k = 0;
        vdisp_readFile(pVirtualAddress, fp, &inPutMeta[0].inAttr);

        memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
        stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
        stBufConf.stFrameCfg.eFormat = outPutMeta->outAttr.ePixelFormat;
        stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
        stBufConf.stFrameCfg.u16Height = inPutMeta[0].inAttr.u32OutHeight;
        stBufConf.stFrameCfg.u16Width = inPutMeta[0].inAttr.u32OutWidth;
        stBufConf.u64TargetPts = 0;
        stBufConf.u32Flags = MI_SYS_MAP_VA;

        printf("%s %d :",__FUNCTION__,__LINE__);
        for(int i = 0; i < cellNum; i++) {
            memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
            stChnPort.eModId = E_MI_MODULE_ID_VDISP;
            stChnPort.u32ChnId = inPutMeta[i].inChn;
            stChnPort.u32DevId = vdisp_dev;
            stChnPort.u32PortId = 0;
            if(!inPutMeta[i].enable)
                continue;
            if(MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, \
                    &inPutMeta[i].bufInfo, &inPutMeta[i].bufHandle, s32TimeOutMs)) {
                    
                //printf("%s %d %d fail\n", __FUNCTION__, __LINE__,i);
                k++;
                usleep(1000);
                i--;
                continue;
            }
            printf("%d ",k);
            k = 0;
        }
        printf("\n");
        
        MI_SYS_FrameData_t outFrame;
        outFrame = inPutMeta[0].bufInfo.stFrameData;
        outFrame.phyAddr[1] = outFrame.phyAddr[0] + outPutMeta->outAttr.u32Height * outPutMeta->outAttr.u32Width;
        outFrame.pVirAddr[1] = outFrame.pVirAddr[0] + (outFrame.phyAddr[1] - outFrame.phyAddr[0]);
        //printf("%s %d :",__FUNCTION__,__LINE__);
        gettimeofday(&tv0, NULL);

        for(int i = 0; i < cellNum ; i++) {
            MI_SYS_WindowRect_t stDstRect[3];
            MI_SYS_FrameData_t stSrcBuf;
            MI_SYS_WindowRect_t stSrcRect[3];
            MI_SYS_FrameData_t stDstBuf;

            if(!inPutMeta[i].enable)
                continue;
            vdisp_process_config(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect, \
                                 pVirtualAddress, u64PhyAddr, &outFrame, \
                                 inPutMeta + i, outPutMeta);

            vdisp_func_420sp(&stDstBuf, stDstRect, &stSrcBuf, stSrcRect);
            MI_SYS_ChnInputPortPutBuf(inPutMeta[i].bufHandle, &inPutMeta[i].bufInfo, FALSE);
            
           // printf("%d ",i);
        }
        //printf("\n");
        gettimeofday(&tv1, NULL);
        //printf("%ld ms\n", (tv1.tv_sec - tv0.tv_sec) * 1000 + (tv1.tv_usec - tv0.tv_usec) / 1000);

        if(cnt > 10) {
            cnt = 0;
            printf("%s %d :",__FUNCTION__,__LINE__);
            for(int i = 1; i < cellNum; i++) {
                int j = 1;
                MI_VDISP_EnableInputChannel(vdisp_dev, i);
                inPutMeta[i].enable  =TRUE;
                srand(time(NULL)+i);

                if(rand() & 1) {
                    inPutMeta[i].enable  =FALSE;
                    MI_VDISP_DisableInputChannel(vdisp_dev, i);
                    j=0;
                }
                if(j)
                    printf("%d ",i);
            }
            printf("\n");
        }
        
/*
        if(cnt1>100)
        {
            MI_VDISP_StopDev(vdisp_dev);
            printf("MI_VDISP_StopDev\n");
            while(cnt1--)
            {
                usleep(100*1000);
                
            }
            MI_VDISP_StartDev(vdisp_dev);
            printf("MI_VDISP_StartDev\n");
        }
*/
        cnt1++;
        cnt++;
        usleep(20 * 1000);
    }

    fclose(fp);

    return 0;
}
