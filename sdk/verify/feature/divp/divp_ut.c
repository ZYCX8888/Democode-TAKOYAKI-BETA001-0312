#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include "mi_sys.h"
#include "mi_divp.h"

#define CROP_TEST 0
#define STRETCH_BUFF_TEST 0
#if (STRETCH_BUFF_TEST == 1)
_Bool bStopStretchBufThread = false;
pthread_t StretchBuffThread;
#endif

#define divp_ut_dbg(fmt, args...) {do{printf("[%s][%d]"fmt,__FUNCTION__,__LINE__,##args);}while(0);}

#define MAX(a,b) ({\
        typeof(a) _a = a;\
        typeof(b) _b = b;\
        (_a) > (_b) ? (_a) : (_b);\
        })
#define MIN(a,b) ({\
        typeof(a) _a = a;\
        typeof(b) _b = b;\
        (_a) < (_b) ? (_a) : (_b);\
        })

#ifndef ALIGN_UP
#define ALIGN_UP(val, alignment) ((((val)+(alignment)-1)/(alignment))*(alignment))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif

#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        divp_ut_dbg("exec function failed\n");\
        return 1;\
    }\
    else\
    {\
        divp_ut_dbg("exec function pass\n");\
    }

#define DIVP_CAP_MAX_WIDTH 1920
#define DIVP_CAP_MAX_HEIGHT 1920

typedef struct STUB_StreamRes_s
{
    MI_U32 u32DevId;
    MI_U32 u32ChnId;
    MI_U32 u32PortId;
    pthread_t outTid;
    pthread_t inTid;
    MI_BOOL bInThreadRunning;
    MI_BOOL bOutThreadRunning;
    MI_S8 inputName[128];

    MI_U32 inputW;
    MI_U32 inputH;
    MI_U32 crop_x;
    MI_U32 crop_y;
    MI_U32 crop_w;
    MI_U32 crop_h;
    MI_U32 outputW;
    MI_U32 outputH;
    MI_BOOL bHMirror;
    MI_BOOL bVMirror;
    MI_SYS_Rotate_e eRotateType;
    MI_SYS_PixelFormat_e eInPixelFormat;
    MI_SYS_PixelFormat_e eOutPixelFormat;
} STUB_StreamRes_t;

#define STUB_DIVP_CHN_NUM 16

static STUB_StreamRes_t _stStubStreamRes[STUB_DIVP_CHN_NUM];
static MI_U32 _u32StreamCount = 1;
static MI_BOOL _bStopThread = FALSE;

static MI_SYS_PixelFormat_e parse_pixformat(char *str)
{
    MI_SYS_PixelFormat_e epixformat;

    if(strcmp(str, "yuv422") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    }
    else if(strcmp(str,"yuv420") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    }
    else if(strcmp(str,"mst420") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV_MST_420;
    }
    else if(strcmp(str,"argb888") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_ARGB8888;
    }
    else if(strcmp(str,"abgr888") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_ABGR8888;
    }
    else if(strcmp(str,"rgb565") == 0)
    {
        epixformat = E_MI_SYS_PIXEL_FRAME_RGB565;
    }
    else
    {
        divp_ut_dbg("using default pixformat\n");
        epixformat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    }
    return epixformat;
}

void test_saveSnapshot(char *buff, MI_SYS_BufInfo_t *pstBufInfo, char *name)
{
    char filename[64];
    struct timeval timestamp;

    gettimeofday(&timestamp, 0);
    sprintf(filename, "saveSnapshot_%dx%d_%d_%08d.%s",pstBufInfo->stFrameData.u16Width,pstBufInfo->stFrameData.u16Height,(int)timestamp.tv_sec,(int)timestamp.tv_usec,name);
    FILE *pfile = fopen(filename, "wb");

    if(NULL == pfile)
    {
        divp_ut_dbg("error: fopen %s failed\n", filename);
        return;
    }
    fwrite(buff, 1, pstBufInfo->stFrameData.u32BufSize, pfile);
    fflush(pfile);
    fclose(pfile);
    sync();
}

MI_BOOL mi_divp_GetDivpInputFrameData420(FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = FALSE;
    MI_U32 u32LineNum = 0;
    MI_U32 u32BytesPerLine = 0;
    MI_U32 u32Index = 0;
    MI_U32 u32FrameDataSize = 0;
    MI_U32 u32YSize = 0;
    MI_U32 u32UVSize = 0;

    if (pInputFile == NULL)
    {
        divp_ut_dbg("create file error.\n");
        return bRet;
    }

    if(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height * 3 / 2;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;
    }
    else
    {
        divp_ut_dbg("UnSupport pixformat:%d\n", pstBufInfo->stFrameData.ePixelFormat);
        return bRet;
    }

    for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
    {
        u32YSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
    }

    for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
    {
        u32UVSize += fread(pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], 1, u32BytesPerLine, pInputFile);
    }

    if(u32YSize + u32UVSize == u32FrameDataSize)
    {
        bRet = TRUE;
    }
    else if(u32YSize + u32UVSize < u32FrameDataSize)
    {
        fseek(pInputFile, 0, SEEK_SET);
        u32YSize = 0;
        u32UVSize = 0;

        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height; u32Index ++)
        {
            u32YSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
        }

        for (u32Index = 0; u32Index < pstBufInfo->stFrameData.u16Height / 2; u32Index ++)
        {
            u32UVSize += fread(pstBufInfo->stFrameData.pVirAddr[1] + u32Index * pstBufInfo->stFrameData.u32Stride[1], 1, u32BytesPerLine, pInputFile);
        }

        if(u32YSize + u32UVSize == u32FrameDataSize)
        {
            bRet = TRUE;
        }
        else
        {
            divp_ut_dbg("read file error. u32YSize = %u, u32UVSize = %u. \n", u32YSize, u32UVSize);
            bRet = FALSE;
        }
    }

    return bRet;
}

MI_BOOL mi_divp_GetDivpInputFrameDataYuv422(FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = FALSE;
    MI_U32 u32ReadSize = 0;
    MI_U32 u32LineNum = 0;
    MI_U32 u32BytesPerLine = 0;
    MI_U32 u32Index = 0;
    MI_U32 u32FrameDataSize = 0;

    if (pInputFile == NULL)
    {
        divp_ut_dbg("create file error.\n");
        return bRet;
    }

    if(E_MI_SYS_PIXEL_FRAME_YUV422_YUYV == pstBufInfo->stFrameData.ePixelFormat)
    {
        u32LineNum = pstBufInfo->stFrameData.u16Height;
        u32BytesPerLine = pstBufInfo->stFrameData.u16Width * 2;
        u32FrameDataSize = u32BytesPerLine * u32LineNum;
        divp_ut_dbg("u16Width = %u, u16Height = %u, u32BytesPerLine = %u, u32LineNum = %u. u32FrameDataSize = %u \n",
            pstBufInfo->stFrameData.u16Width, pstBufInfo->stFrameData.u16Height, u32BytesPerLine, u32LineNum, u32FrameDataSize);
    }
    else
    {
        divp_ut_dbg("######  error ######. ePixelFormat = %u\n", pstBufInfo->stFrameData.ePixelFormat);
        return bRet;
    }

    for (u32Index = 0; u32Index < u32LineNum; u32Index ++)
    {
        u32ReadSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
    }

    if(u32ReadSize == u32FrameDataSize)
    {
        bRet = TRUE;
    }
    else if(u32ReadSize < u32FrameDataSize)
    {
        fseek(pInputFile, 0, SEEK_SET);
        u32ReadSize = 0;

        for(u32Index = 0; u32Index < u32LineNum; u32Index ++)
        {
            u32ReadSize += fread(pstBufInfo->stFrameData.pVirAddr[0] + u32Index * pstBufInfo->stFrameData.u32Stride[0], 1, u32BytesPerLine, pInputFile);
        }

        if(u32ReadSize == u32FrameDataSize)
        {
            bRet = TRUE;
        }
        else
        {
            divp_ut_dbg("read file error. u32ReadSize = %u. \n", u32ReadSize);
            bRet = FALSE;
        }
    }

    return bRet;
}

MI_BOOL mi_divp_GetDivpInputFrameData(MI_SYS_PixelFormat_e ePixelFormat, FILE *pInputFile, MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_BOOL bRet = TRUE;
    if((NULL == pInputFile) || (NULL == pstBufInfo))
    {
        bRet = FALSE;
        divp_ut_dbg("read input frame data failed! pInputFile = %p, pstBufInfo = %p.\n", pInputFile, pstBufInfo);
    }
    else
    {
        if(ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
        {
            if(!mi_divp_GetDivpInputFrameData420(pInputFile, pstBufInfo))
            {
                bRet = FALSE;
                divp_ut_dbg("read input frame data failed!.\n");
            }
        }
        else if(ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
        {
            if(!mi_divp_GetDivpInputFrameDataYuv422(pInputFile, pstBufInfo))
            {
                bRet = FALSE;
                divp_ut_dbg("read input frame data failed!.\n");
            }
        }
    }
    return bRet;
}
#if (STRETCH_BUFF_TEST == 1)
#define SRC_WIDTH 1280
#define SRC_HEIGHT 720
#define SRC_BUFF_STRIDE (ALIGN_UP(SRC_WIDTH,16))
#define SRC_BUFF_SIZE (SRC_BUFF_STRIDE*SRC_HEIGHT*3/2)

#define DST_WIDTH 640
#define DST_HEIGHT 480
#define DST_BUFF_STRIDE (ALIGN_UP(DST_WIDTH,16))
#define DST_BUFF_SIZE (DST_BUFF_STRIDE*DST_HEIGHT*3/2)

#define CROP_X 200
#define CROP_Y 100
#define CROP_W 68
#define CROP_H 48

static int FillSrcBuf(const char* FilePath,MI_DIVP_DirectBuf_t *pstDirectSrcBuf)
{
    int ret = 0;
    FILE *fp;
    void *pVirSrcBufAddr = NULL;
    int LineIdx = 0;
    int ReadSize = 0;

    fp = fopen(FilePath,"r");
    if(!fp)
    {
        divp_ut_dbg("open file[%s] failed\n",FilePath);
        ret = -1;
        goto EXIT;
    }

    MI_SYS_Mmap(pstDirectSrcBuf->phyAddr[0], SRC_BUFF_SIZE, &pVirSrcBufAddr, FALSE);
    if(!pVirSrcBufAddr)
    {
        divp_ut_dbg("mmap dst buff failed\n");
        ret = -1;
        goto EXIT;
    }

    for(LineIdx = 0; LineIdx < SRC_HEIGHT*3/2; LineIdx++)
    {
        ReadSize += fread(pVirSrcBufAddr+LineIdx*SRC_BUFF_STRIDE, 1, SRC_WIDTH, fp);
    }
    if(ReadSize < SRC_WIDTH*SRC_HEIGHT*3/2)
    {
        fseek(fp, 0, SEEK_SET);
        ReadSize = 0;
        for(LineIdx = 0; LineIdx < SRC_HEIGHT*3/2; LineIdx++)
        {
            ReadSize += fread(pVirSrcBufAddr+LineIdx*SRC_BUFF_STRIDE, 1, SRC_WIDTH, fp);
        }
        if(ReadSize < SRC_WIDTH*SRC_HEIGHT*3/2)
        {
            divp_ut_dbg("read file failed, read size:%d\n",ReadSize);
            ret = -1;
            goto EXIT;
        }
    }

EXIT:
    if(fp)
        fclose(fp);
    if(pVirSrcBufAddr)
        MI_SYS_Munmap(pVirSrcBufAddr, SRC_BUFF_SIZE);

    return ret;
}

static int DumpDstBuf(MI_DIVP_DirectBuf_t *pstDirectDstBuf)
{
    int ret = 0;
    FILE *fp;
    void *pVirDstBufAddr = NULL;
    int LineIdx = 0;
    int WriteSize = 0;
    char outputfile[128];
    struct timeval timestamp;

    gettimeofday(&timestamp, 0);
    sprintf(outputfile, "output_%dx%d_%d_%08d.yuv",pstDirectDstBuf->u32Width,pstDirectDstBuf->u32Height,(int)timestamp.tv_sec,(int)timestamp.tv_usec);

    fp = fopen(outputfile,"w+");
    if(!fp)
    {
        divp_ut_dbg("open file[%s] failed\n",outputfile);
        ret = -1;
        goto EXIT;
    }

    MI_SYS_Mmap(pstDirectDstBuf->phyAddr[0], DST_BUFF_SIZE, &pVirDstBufAddr, FALSE);
    if(!pVirDstBufAddr)
    {
        divp_ut_dbg("mmap dst buff failed\n");
        ret = -1;
        goto EXIT;
    }

    for(LineIdx = 0; LineIdx < DST_HEIGHT*3/2; LineIdx++)
    {
        WriteSize += fwrite(pVirDstBufAddr+LineIdx*DST_BUFF_STRIDE, 1, DST_WIDTH, fp);
    }
    if(WriteSize < DST_WIDTH*DST_HEIGHT*3/2)
    {
        divp_ut_dbg("write file failed, write size:%d\n",WriteSize);
    }
    fflush(fp);
    sync();
    divp_ut_dbg("save stretch dst buff to[%s]\n",outputfile);

EXIT:
    if(fp)
        fclose(fp);
    if(pVirDstBufAddr)
        MI_SYS_Munmap(pVirDstBufAddr, DST_BUFF_SIZE);

    return 0;
}

void *divp_StretchBuf_Thread(void* p)
{
    MI_PHY phySrcBufAddr = 0;
    MI_PHY phyDstBufAddr = 0;
    MI_DIVP_DirectBuf_t stSrcBuf;
    MI_DIVP_DirectBuf_t stDstBuf;
    MI_SYS_WindowRect_t stSrcCrop;

    while(!bStopStretchBufThread)
    {
        MI_SYS_MMA_Alloc(NULL, SRC_BUFF_SIZE, &phySrcBufAddr);
        if(!phySrcBufAddr)
        {
            divp_ut_dbg("alloc src buff failed\n");
            return NULL;
        }

        MI_SYS_MMA_Alloc(NULL, DST_BUFF_SIZE, &phyDstBufAddr);
        if(!phyDstBufAddr)
        {
            divp_ut_dbg("alloc dst buff failed\n");
            return NULL;
        }

        stSrcBuf.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stSrcBuf.u32Width = SRC_WIDTH;
        stSrcBuf.u32Height = SRC_HEIGHT;
        stSrcBuf.u32Stride[0] = SRC_BUFF_STRIDE;
        stSrcBuf.u32Stride[1] = SRC_BUFF_STRIDE;
        stSrcBuf.phyAddr[0] = phySrcBufAddr;
        stSrcBuf.phyAddr[1] = stSrcBuf.phyAddr[0] + SRC_BUFF_STRIDE*SRC_HEIGHT;

        stDstBuf.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stDstBuf.u32Width = DST_WIDTH;
        stDstBuf.u32Height = DST_HEIGHT;
        stDstBuf.u32Stride[0] = DST_BUFF_STRIDE;
        stDstBuf.u32Stride[1] = DST_BUFF_STRIDE;
        stDstBuf.phyAddr[0] = phyDstBufAddr;
        stDstBuf.phyAddr[1] = stDstBuf.phyAddr[0] + DST_BUFF_STRIDE*DST_HEIGHT;

        stSrcCrop.u16X = CROP_X;
        stSrcCrop.u16Y = CROP_Y;
        stSrcCrop.u16Width = CROP_W;
        stSrcCrop.u16Height = CROP_H;

        if(FillSrcBuf("./1280x720_yuv420.yuv", &stSrcBuf))
            return NULL;
        if(MI_SUCCESS == MI_DIVP_StretchBuf(&stSrcBuf, &stSrcCrop, &stDstBuf))
        {
            if(DumpDstBuf(&stDstBuf))
                return NULL;
        }
        else
        {
            divp_ut_dbg("MI_DIVP_StretchBuf failed\n");
        }

        MI_SYS_MMA_Free(phySrcBufAddr);
        MI_SYS_MMA_Free(phyDstBufAddr);

        usleep(100*1000);
    }
    return NULL;
}
#endif

void *divp_channel_input_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stInputBufConf;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHdl = 0;
    MI_U8 u8ChnId = 0;
    FILE *pInputFile = NULL;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    u8ChnId = pstStubStreamRes->u32ChnId;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = pstStubStreamRes->u32DevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32ChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32PortId;

    stInputBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stInputBufConf.u32Flags = 0x80000000;
    stInputBufConf.u64TargetPts = 0x12340000;
    stInputBufConf.stFrameCfg.u16Width = pstStubStreamRes->inputW;
    stInputBufConf.stFrameCfg.u16Height = pstStubStreamRes->inputH;
    stInputBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stInputBufConf.stFrameCfg.eFormat = pstStubStreamRes->eInPixelFormat;

    pInputFile =fopen(pstStubStreamRes->inputName, "rb");
    if(pInputFile == NULL)
    {
        divp_ut_dbg("chn[%d] open input file error. pchFileName = %s\n",u8ChnId,pstStubStreamRes->inputName);
        return NULL;
    }

    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,5);
    while(pstStubStreamRes->bInThreadRunning)
    {
        if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stChnPort,&stInputBufConf,&stInputBufInfo,&hInputBufHdl, 1))
        {
            if(!mi_divp_GetDivpInputFrameData(pstStubStreamRes->eInPixelFormat, pInputFile, &stInputBufInfo))
            {
                fclose(pInputFile);
                pInputFile = 0;
                divp_ut_dbg("chn[%d] read input frame data failed!.\n",u8ChnId);
                return NULL;
            }
            if(MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hInputBufHdl,  &stInputBufInfo, false))
            {
                divp_ut_dbg("chn[%d] MI_SYS_ChnInputPortPutBuf fail.\n",u8ChnId);
            }
        }
        else
        {
            divp_ut_dbg("chn[%d] MI_SYS_ChnInputPortGetBuf fail.\n",u8ChnId);
        }
        usleep(100*1000);
    }

    if(NULL != pInputFile)
    {
        fclose(pInputFile);
    }
    return NULL;
}

void *divp_channel_output_func(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 count = 0;
    MI_U8 u8ChnId = 0;

    STUB_StreamRes_t *pstStubStreamRes = (STUB_StreamRes_t *)p;
    u8ChnId = pstStubStreamRes->u32ChnId;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = pstStubStreamRes->u32DevId;
    stChnPort.u32ChnId = pstStubStreamRes->u32ChnId;
    stChnPort.u32PortId = pstStubStreamRes->u32PortId;

    while(pstStubStreamRes->bOutThreadRunning)
    {
        count++;
        hSysBuf = MI_HANDLE_NULL;
        usleep(30 * 1000);

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        while(1)
        {
            if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
            {
                usleep(1000);
                continue;
            }
            else
            {
                break;
            }
        }

        divp_ut_dbg("chn[%d] divp getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p) size %u\n",
                u8ChnId, stBufInfo.stFrameData.u16Width,
                stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0],
                stBufInfo.stFrameData.u32Stride[1], stBufInfo.stFrameData.u32Stride[2],
                stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.pVirAddr[0],
                stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.pVirAddr[2],
                stBufInfo.stFrameData.u32BufSize);

        if(count >= 2)
        {
            test_saveSnapshot(stBufInfo.stFrameData.pVirAddr[0], &stBufInfo, "yuv");
            count = 0;
        }
        if(MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            divp_ut_dbg("chn[%d] MI_SYS_ChnOutputPortPutBuf failed\n",u8ChnId);
            continue;
        }
    }

    return NULL;
}

MI_S32 mi_divp_test_CreateChn(MI_DIVP_CHN DivpChn, MI_U32 u32MaxWidth, MI_U32 u32MaxHeight, MI_DIVP_DiType_e eDiType)
{
    MI_S32 s32Ret = -1;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    memset(&stDivpChnAttr, 0, sizeof(stDivpChnAttr));

    stDivpChnAttr.bHorMirror = false;
    stDivpChnAttr.bVerMirror = false;
    stDivpChnAttr.eDiType = eDiType;
    stDivpChnAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.u32MaxWidth = u32MaxWidth;
    stDivpChnAttr.u32MaxHeight = u32MaxHeight;

    s32Ret = MI_DIVP_CreateChn(DivpChn, &stDivpChnAttr);
    divp_ut_dbg("\n s32Ret = %d", s32Ret);

    return s32Ret;
}

MI_S32 mi_divp_test_SetChnAttr(MI_DIVP_CHN DivpChn, MI_DIVP_ChnAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_SetChnAttr(DivpChn, pstAttr));

    return s32Ret;
}

MI_S32 mi_divp_test_SetOutputPortAttr(MI_DIVP_CHN DivpChn, MI_DIVP_OutputPortAttr_t* pstOutputPortAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(DivpChn, pstOutputPortAttr));

    return s32Ret;
}

MI_S32 mi_divp_test_StartChn(MI_DIVP_CHN DivpChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    STCHECKRESULT(MI_DIVP_StartChn(DivpChn));
    return s32Ret;
}

static MI_S32 STUB_BaseModuleInit(void)
{
    MI_U32 i = 0;
    MI_DIVP_CHN u32ChnId = 0;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;
    MI_U32 u32DivpDevId;
    MI_U32 u32DivpChnId;
    MI_U32 u32DivpPortId;

    MI_SYS_Init();

    for(i = 0; i < _u32StreamCount; i++)
    {
        memset(&stDivpChnAttr, 0, sizeof(stDivpChnAttr));
        memset(&stDivpOutputPortAttr, 0, sizeof(stDivpOutputPortAttr));

        u32ChnId = i;
        stDivpChnAttr.bHorMirror =  _stStubStreamRes[i].bHMirror;
        stDivpChnAttr.bVerMirror = _stStubStreamRes[i].bVMirror;
        stDivpChnAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
        stDivpChnAttr.eRotateType = _stStubStreamRes[i].eRotateType;
        stDivpChnAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
        stDivpChnAttr.stCropRect.u16X = _stStubStreamRes[i].crop_x;
        stDivpChnAttr.stCropRect.u16Y = _stStubStreamRes[i].crop_y;
        stDivpChnAttr.stCropRect.u16Width = _stStubStreamRes[i].crop_w;
        stDivpChnAttr.stCropRect.u16Height = _stStubStreamRes[i].crop_h;
        stDivpChnAttr.u32MaxWidth = DIVP_CAP_MAX_WIDTH;
        stDivpChnAttr.u32MaxHeight = DIVP_CAP_MAX_HEIGHT;

        stDivpOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpOutputPortAttr.ePixelFormat = _stStubStreamRes[i].eOutPixelFormat;
        stDivpOutputPortAttr.u32Width = _stStubStreamRes[i].outputW;
        stDivpOutputPortAttr.u32Height = _stStubStreamRes[i].outputH;

        mi_divp_test_CreateChn(u32ChnId, DIVP_CAP_MAX_WIDTH, DIVP_CAP_MAX_HEIGHT, E_MI_DIVP_DI_TYPE_OFF);
        mi_divp_test_SetChnAttr(u32ChnId, &stDivpChnAttr);
        mi_divp_test_SetOutputPortAttr(u32ChnId, &stDivpOutputPortAttr);
        mi_divp_test_StartChn(u32ChnId);

        u32DivpDevId = 0;
        u32DivpChnId = i;
        u32DivpPortId = 0;
        _stStubStreamRes[i].u32DevId = u32DivpDevId;
        _stStubStreamRes[i].u32ChnId = u32DivpChnId;
        _stStubStreamRes[i].u32PortId = u32DivpPortId;
        _stStubStreamRes[i].bInThreadRunning = TRUE;
        _stStubStreamRes[i].bOutThreadRunning = TRUE;

        pthread_create(&_stStubStreamRes[i].outTid, NULL, divp_channel_output_func, &_stStubStreamRes[i]);
        pthread_create(&_stStubStreamRes[i].inTid, NULL, divp_channel_input_func, &_stStubStreamRes[i]);
    }
#if (CROP_TEST == 1)
    {
        unsigned int crop_w = 128;
        unsigned int crop_h = 96;
        unsigned int tmpc_x = 0;
        unsigned int tmpc_y = 0;
        unsigned int tmpc_w = 0;
        unsigned int tmpc_h = 0;
        int loop_cnt = 50;
        MI_DIVP_ChnAttr_t stDivpChnAttr;
        MI_DIVP_OutputPortAttr_t stDivpOutputPortAttr;

        srand((unsigned int)time(NULL));
        for(;loop_cnt > 0; loop_cnt--){

            MI_DIVP_GetChnAttr(0, &stDivpChnAttr);
            tmpc_x = MIN((_stStubStreamRes[0].inputW-crop_w),ALIGN_DOWN(rand()%(_stStubStreamRes[0].inputW),2));
            tmpc_y = MIN((_stStubStreamRes[0].inputH-crop_h),ALIGN_DOWN(rand()%(_stStubStreamRes[0].inputH),2));
            tmpc_w = ALIGN_DOWN(rand()%(_stStubStreamRes[0].inputW - tmpc_x),2);
            tmpc_h = ALIGN_DOWN(rand()%(_stStubStreamRes[0].inputH - tmpc_y),2);
            stDivpChnAttr.stCropRect.u16X = tmpc_x;
            stDivpChnAttr.stCropRect.u16Y = tmpc_y;
            stDivpChnAttr.stCropRect.u16Width = MAX(crop_w,tmpc_w);
            stDivpChnAttr.stCropRect.u16Height = MAX(crop_h,tmpc_h);
            MI_DIVP_SetChnAttr(0, &stDivpChnAttr);

            MI_DIVP_GetOutputPortAttr(0, &stDivpOutputPortAttr);
            stDivpOutputPortAttr.u32Width = ALIGN_UP(stDivpChnAttr.stCropRect.u16Width,16);
            stDivpOutputPortAttr.u32Height = ALIGN_UP(stDivpChnAttr.stCropRect.u16Height,16);
            MI_DIVP_SetOutputPortAttr(0, &stDivpOutputPortAttr);

            divp_ut_dbg("loop_cnt:%d input(%d_%d),crop(%d_%d_%d_%d),output(%d_%d)\n",loop_cnt,
                    _stStubStreamRes[0].inputW,
                    _stStubStreamRes[0].inputH,
                    stDivpChnAttr.stCropRect.u16X,
                    stDivpChnAttr.stCropRect.u16Y,
                    stDivpChnAttr.stCropRect.u16Width,
                    stDivpChnAttr.stCropRect.u16Height,
                    stDivpOutputPortAttr.u32Width,
                    stDivpOutputPortAttr.u32Height);

            usleep(1000*1000*3);
        }
    }
#endif

#if (STRETCH_BUFF_TEST == 1)
    pthread_create(&StretchBuffThread, NULL, divp_StretchBuf_Thread, NULL);
#endif
    return MI_SUCCESS;
}

static MI_S32 STUB_BaseModuleDeinit(void)
{
    MI_U32 i = 0;
    for(i = 0; i < _u32StreamCount; i++)
    {
        _stStubStreamRes[i].bInThreadRunning = FALSE;
        _stStubStreamRes[i].bOutThreadRunning = FALSE;
        pthread_join(_stStubStreamRes[i].inTid, NULL);
        pthread_join(_stStubStreamRes[i].outTid, NULL);
#if (STRETCH_BUFF_TEST == 1)
        bStopStretchBufThread = true;
        pthread_join(StretchBuffThread, NULL);
#endif
        _stStubStreamRes[i].inTid = 0;
        _stStubStreamRes[i].outTid = 0;

        STCHECKRESULT(MI_DIVP_StopChn(i));
        STCHECKRESULT(MI_DIVP_DestroyChn(i));
    }

    STCHECKRESULT(MI_SYS_Exit());
    divp_ut_dbg("module exit\n");

    return MI_SUCCESS;
}

static void display_help()
{
    printf("Usage: prog [-n channel count] [-f filename] [-i input] [-c crop] [-o output] [-pi input pixfmt] [-po output pixfmt]\n\n");
    printf("\t\t-n  : channel count\n");
    printf("\t\t-f  : file path\n");
    printf("\t\t-i  : input resolution\n");
    printf("\t\t-c  : crop window\n");
    printf("\t\t-o  : output resolution\n");
    printf("\t\t-pi : input pixformat\n");
    printf("\t\t-po : output pixformat\n");
    printf("\t\tfor example : ./divp_ut -n 1 -f test.yuv -i 640_480 -c 640_480 -o 640_480 -pi yuv420 -po yuv420\n");
}

MI_S32 main(int argc, char **argv)
{
    MI_S32 result, i;
    fd_set stFdSet;
    char buf[50];
    MI_U32 width, height, crop_x, crop_y, crop_w, crop_h;
    MI_SYS_PixelFormat_e ePixelFormat;
    char *str = NULL;

    while((result = getopt(argc, argv, "n:f:i:c:o:pi:po:hvr:")) != -1)
    {
        switch(result)
        {
            case 'n':
                {
                    str = strtok(optarg, "_");
                    _u32StreamCount = atoi(str);
                }
                break;
            case 'f':
                {
                    for(i = 0; i < _u32StreamCount; i++)
                    {
                        strcpy(_stStubStreamRes[i].inputName, optarg);
                    }
                }
                break;
            case 'i':
                {
                    str = strtok(optarg, "_");
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        width = atoi(str);
                        str = strtok(NULL, "_");
                        height = atoi(str);
                        str = strtok(NULL, "_");
                        _stStubStreamRes[i].inputW = width;
                        _stStubStreamRes[i].inputH = height;
                    }
                }
                break;
            case 'c':
                {
                    str = strtok(optarg, "_");
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        crop_x = atoi(str);
                        str = strtok(NULL, "_");
                        crop_y = atoi(str);
                        str = strtok(NULL, "_");
                        crop_w = atoi(str);
                        str = strtok(NULL, "_");
                        crop_h = atoi(str);
                        str = strtok(NULL, "_");
                        _stStubStreamRes[i].crop_x = crop_x;
                        _stStubStreamRes[i].crop_y = crop_y;
                        _stStubStreamRes[i].crop_w = crop_w;
                        _stStubStreamRes[i].crop_h = crop_h;
                    }
                }
                break;
            case 'o':
                {
                    str = strtok(optarg, "_");
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        width = atoi(str);
                        str = strtok(NULL, "_");
                        height = atoi(str);
                        str = strtok(NULL, "_");
                        _stStubStreamRes[i].outputW = width;
                        _stStubStreamRes[i].outputH = height;
                    }
                }
                break;
            case 'p':
                {
                    result = getopt(argc, argv, "n:f:i:c:o:pi:po:");
                    if(result == 'i')
                    {
                        str = strtok(optarg, "_");
                        ePixelFormat = parse_pixformat(str);
                        for(i = 0; i < _u32StreamCount; i++)
                        {
                            _stStubStreamRes[i].eInPixelFormat = ePixelFormat;
                        }
                    }
                    else if(result == 'o')
                    {
                        str = strtok(optarg, "_");
                        ePixelFormat = parse_pixformat(str);
                        for(i = 0; i < _u32StreamCount; i++)
                        {
                            _stStubStreamRes[i].eOutPixelFormat = ePixelFormat;
                        }
                    }
                    else
                        divp_ut_dbg("pixformat UnSupport!!!\n");
                }
                break;
            case 'h':
                {
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        _stStubStreamRes[i].bHMirror = TRUE;
                    }
                }
                break;
            case 'v':
                {
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        _stStubStreamRes[i].bVMirror = TRUE;
                    }
                }
                break;
            case 'r':
                {
                    int rotate_angle = 0;
                    str = strtok(optarg, "_");
                    rotate_angle = atoi(str);
                    for(i = 0; i < _u32StreamCount && str; i++)
                    {
                        _stStubStreamRes[i].eRotateType = (rotate_angle == 90)?E_MI_SYS_ROTATE_90:\
                                                          (rotate_angle == 180)?E_MI_SYS_ROTATE_180:\
                                                          (rotate_angle == 270)?E_MI_SYS_ROTATE_270:\
                                                            E_MI_SYS_ROTATE_NONE;
                    }

                }
                break;
            default:
                display_help();
                break;
        }
    }

    for(i = 0; i < _u32StreamCount; i++)
    {
        divp_ut_dbg("input: %u*%u, crop: %u*%u*%u*%u, output: %u*%u, InPixFormat:%d OutPixFormat:%d inputfile:%s\n",
                _stStubStreamRes[i].inputW, _stStubStreamRes[i].inputH,
                _stStubStreamRes[i].crop_x, _stStubStreamRes[i].crop_y,
                _stStubStreamRes[i].crop_w, _stStubStreamRes[i].crop_h,
			    _stStubStreamRes[i].outputW, _stStubStreamRes[i].outputH,
                _stStubStreamRes[i].eInPixelFormat, _stStubStreamRes[i].eOutPixelFormat,
                _stStubStreamRes[i].inputName);
    }

    STCHECKRESULT(STUB_BaseModuleInit());

    FD_ZERO(&stFdSet);
    FD_SET(0,&stFdSet);
    for(;;)
    {
        divp_ut_dbg("input 'q' exit\n");
        select(1, &stFdSet, NULL, NULL, NULL);
        if(FD_ISSET(0, &stFdSet))
        {
            int i = read(0, buf, sizeof(buf));
            if(i>0 && (buf[0] == 'q'))
            {
                break;
            }
        }
    }

    STCHECKRESULT(STUB_BaseModuleDeinit());

    return 0;
}

