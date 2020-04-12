#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<assert.h>

#include <errno.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp_datatype.h"
#include "mi_divp.h"

#define WAIT_TIMEOUT 6 //6s

#define TEST_ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))
#define TEST_ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

typedef enum
{
    TEST_CASE_H264,
    TEST_CASE_H265,
    TEST_CASE_JPEG,
    TEST_CASE_H264_H265,
    TEST_CASE_H264_H265_JPEG,
    TEST_CASE_H264_DIVP,
    TEST_CASE_H265_DIVP,
    TEST_CASE_SWITCH_DIVP,
    TEST_CASE_H264_H265_FRAMING,
}TEST_CASE_e;

#define VDEC_CHN_MAX (33)
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (1024 * 1024)



typedef struct chn_info_s
{
    MI_VDEC_CHN VdecChn;
    MI_U16      u16Width;
    MI_U16      u16Height;
    MI_U32      u32BufSize;
    MI_U32      u32FrmRate;
    MI_U32      u32RefFrameNum;
    MI_VDEC_CodecType_e eCodecType;
    MI_VDEC_VideoMode_e eVideoMode;
    MI_VDEC_DPB_BufMode_e eDpbBufMode;
    MI_U16      u16ScaleWidth;
    MI_U16      u16ScaleHeight;
    MI_U8       au8FilePath[128];
}chn_info_t;


typedef enum
{
    E_TEST_INITPARAM,
    E_TEST_VDEC,
    E_TEST_MAX
} Param_Type_e;

//file config
typedef struct test_cfg_s
{
    Param_Type_e eParamType;
    MI_U32   u32MaxChnNum;
    MI_BOOL  bDisableLowLatency;
    chn_info_t stChnInfo[VDEC_CHN_MAX];
}test_cfg_t;


static test_cfg_t stTestCfg;
static pthread_t _thrPush[VDEC_CHN_MAX];
static pthread_t _thrGet[VDEC_CHN_MAX];
static MI_BOOL _bRun[VDEC_CHN_MAX];
static FILE *_hFile[VDEC_CHN_MAX];
static MI_SYS_ChnPort_t _astChnPort;

static MI_BOOL _bStop;

#define _CHECKPOINT_ printf("xxxxxxxxx [%s][%d] xxxxxxxx\n", __FUNCTION__, __LINE__);

MI_U64 get_pts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

void *push_stream(void* args)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_S32 Index = 0;
    MI_VDEC_CHN VdecChn;
    MI_U32 u32FrmRate = 0;
    MI_VDEC_VideoMode_e eVideoMode;
    MI_U8 *pu8Buf = NULL;
    MI_U32 u32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_U32 u32Param = (MI_U32)args;
    MI_VDEC_VideoStream_t stVdecStream;
    MI_S32 s32TimeOutMs = 20;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    Index      = u32Param >> 16;
    VdecChn    = stTestCfg.stChnInfo[Index].VdecChn;
    eVideoMode = (MI_VDEC_VideoMode_e)(u32Param & 0xFF);
    u32FrmRate = stTestCfg.stChnInfo[Index].u32FrmRate;

    pu8Buf = malloc(VESFILE_READER_BATCH);
    printf("chn(%d) %s\n", VdecChn, (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode) ? "stream" : "frame");

    while (_bRun[VdecChn])
    {
        usleep(1000 / u32FrmRate * 1000);

        if(_bStop)
        {
		    usleep(10 * 1000);
            continue;
        }

        if (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode)
        {
            ///stream mode, read 128k every time
            u32FrameLen = VESFILE_READER_BATCH;
            u32Pos = fseek(_hFile[VdecChn], 0L, SEEK_CUR);
        }
        else
        {
            ///frame mode, read one frame lenght every time
            memset(au8Header, 0, 16);
            u32Pos = fseek(_hFile[VdecChn], 0L, SEEK_CUR);
            u32Len = fread(au8Header, 16, 1, _hFile[VdecChn]);
            if(u32Len <= 0)
            {
                fseek(_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }

            u32FrameLen = MI_U32VALUE(au8Header, 4);
            if(u32FrameLen > VESFILE_READER_BATCH)
            {
                fseek(_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }
        }

        u32Len = fread(pu8Buf, u32FrameLen, 1, _hFile[VdecChn]);
        if(u32Len <= 0)
        {
            fseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = u32FrameLen;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len + 16; //back length

#if 0
        printf("%s, %d, VdecChn: %d, pu8Addr: %p, u32Len: %u, u64PTS: %llu.\n", __FUNCTION__, __LINE__,
            VdecChn,
            stVdecStream.pu8Addr,
            stVdecStream.u32Len,
            stVdecStream.u64PTS);
#endif

        if(MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(VdecChn, &stVdecStream, s32TimeOutMs)))
        {
            //ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(_hFile[VdecChn], - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + get_pts(30);

        memset(&stChnStat, 0x0, sizeof(stChnStat));
        MI_VDEC_GetChnStat(VdecChn, &stChnStat);
        //printf("Chn(%d)_%s_Codec:%d, Frame Dec:%d\n", VdecChn, _aeCodecType[VdecChn], stChnStat.eCodecType, stChnStat.u32DecodeStreamFrames);
    }

    free(pu8Buf);
    return NULL;
}

void *get_meta_data(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32CheckSum = 0;

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VdecChn;
    stChnPort.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort,2,4);
    while (_bRun[VdecChn])
    {
        hSysBuf = MI_HANDLE_NULL;
        usleep(30 * 1000);
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }

        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_META)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }
        else
        {
            MI_U32 i = 0;
            char *p = NULL;

            if(0)
            {
                printf("Chn(%d), Bufino(bEndOfStream:%d, bUsrBuf:%d, eBufType:%d, Pts:%lld)\n",
                    VdecChn,
                    stBufInfo.bEndOfStream,
                    stBufInfo.bUsrBuf,
                    stBufInfo.eBufType,
                    stBufInfo.u64Pts);
            }

            if(0)
            {
                printf("Chn(%d), eDataFromModule:%d, phyAddr:0x%llx, pVirAddr:0x%p, u32Size:%d\n",
                    VdecChn,
                    stBufInfo.stMetaData.eDataFromModule,
                    stBufInfo.stMetaData.phyAddr,
                    stBufInfo.stMetaData.pVirAddr,
                stBufInfo.stMetaData.u32Size);
            }

            p = (char *)(stBufInfo.stMetaData.pVirAddr);
            u32CheckSum = 0;
            for (i = 0; i < stBufInfo.stMetaData.u32Size; ++i)
            {
                u32CheckSum += p[i];
            }
            printf("Chn(%d), pts(%lld) u32CheckSum:%d\n", VdecChn, stBufInfo.u64Pts, u32CheckSum);
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

void save_yuv420_data(MI_U32 u32Chn, MI_SYS_FrameData_t *pstFrameData)
{
    FILE *fp = NULL;
    char aName[128];
    MI_U32 u32Width = pstFrameData->u16Width;
    MI_U32 u32Height = pstFrameData->u16Height;
    MI_U32 u32Pitch = pstFrameData->u32Stride[0];
    MI_U32 u32YLen = 0;
    MI_U32 u32UvLen = 0;
    static MI_U32 u32Frmcnt[33] = {0};

    if (u32Frmcnt[u32Chn] > 2)
    {
        //printf("Save frame finished, total: %d.\n", u32Frmcnt[u32Chn]);
        return;
    }
    memset(aName, 0x0, sizeof(aName));
    sprintf(aName, "/tmp/app_chn_%d_dump_vdec[%d_%d_%d]_%d_nv12.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);

    fp = fopen(aName, "wb+");
    if (!fp)
    {
        printf("Open File Faild\n");
        return;
    }

    // Y buffer
    if(pstFrameData->pVirAddr[0])
    {
        fseek(fp, 0, SEEK_SET);
        u32YLen = u32Pitch * u32Height;
        if(fwrite(pstFrameData->pVirAddr[0], 1, u32YLen, fp) != u32YLen)
        {
            printf("Chn %d Y buffer fwrite %s failed\n", u32Chn, aName);
            goto _END;
        }
    }
    else
    {
        printf("Chn %d Y buffer is NULL.\n", u32Chn);
    }

    // UV buffer
    if(pstFrameData->pVirAddr[1])
    {
        u32UvLen = u32Pitch * u32Height / 2;
        fseek(fp, u32YLen, SEEK_SET);
        if(fwrite(pstFrameData->pVirAddr[1], 1, u32UvLen, fp) != u32UvLen)
        {
            printf("Chn %d UV buffer fwrite %s failed\n", u32Chn, aName);
            goto _END;
        }
    }
    else
    {
        printf("Chn %d UV buffer is NULL.\n", u32Chn);
    }

    printf("dump file(%s) ok ..............[len:%d]\n", aName, u32YLen+u32UvLen);
    u32Frmcnt[u32Chn]++;

_END:
    fclose(fp);
    fp = NULL;
}

void *get_frame_data(void* args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32CheckSum = 0;

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)args;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VdecChn;
    stChnPort.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 4);
    while (_bRun[VdecChn])
    {
        hSysBuf = MI_HANDLE_NULL;
        usleep(2000);

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }

        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }

        {
            //MI_U32 i = 0;
            //char *p = NULL;

            //printf("Chn(%d), Bufino(bEndOfStream:%d, bUsrBuf:%d, eBufType:%d, Pts:%lld)\n",
            //    VdecChn,
            //    stBufInfo.bEndOfStream,
            //    stBufInfo.bUsrBuf,
            //    stBufInfo.eBufType,
            //    stBufInfo.u64Pts);

            if(0)
            {
                printf("Chn(%d), Pixel:%d, phyAddr:0x%llx, pVirAddr:%p, W:%d, H:%d, BufSize:%u.\n",
                    VdecChn,
                    stBufInfo.stFrameData.ePixelFormat,
                    stBufInfo.stFrameData.phyAddr[0],
                    stBufInfo.stFrameData.pVirAddr[0],
                    stBufInfo.stFrameData.u16Width,
                    stBufInfo.stFrameData.u16Height,
                    stBufInfo.stFrameData.u32BufSize);
            }

            if(0)
            {
                save_yuv420_data(VdecChn, &stBufInfo.stFrameData);
            }

            //p = (char *)(stBufInfo.stFrameData.pVirAddr);
            //u32CheckSum = 0;
            //for (i = 0; i < stBufInfo.stFrameData.u32BufSize; ++i)
            //{
            //    u32CheckSum += p[i];
            //}
            //printf("Chn(%d), pts(%lld) u32CheckSum:%d\n", VdecChn, stBufInfo.u64Pts, u32CheckSum);
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

void save_yuv422_data(MI_U8 *pYuv422Data, MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32Pitch, MI_U32 u32Chn)
{
    FILE *fp = NULL;
    char aName[128];
    MI_U32 u32Length = u32Width * u32Height * 2;
    static MI_U32 u32Frmcnt[33] = {0};

    if (u32Frmcnt[u32Chn] > 5)
    {
        printf("get frame count:%d\n", u32Frmcnt[u32Chn]);
        return;
    }
    memset(aName, 0x0, sizeof(aName));
    sprintf(aName, "/mnt/app_chn_%d_jpeg_dump_vdec[%d_%d_%d]_%d.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);

    fp = fopen(aName, "wb+");
    if (!fp)
    {
        printf("Open File Faild\n");
        return;
    }

    fseek(fp, 0, SEEK_SET);
    if(fwrite(pYuv422Data, 1, u32Length, fp) != u32Length)
    {
        printf("fwrite %s failed\n", aName);
        goto _END;
    }

    printf("dump file(%s) ok ..............[len:%d]\n", aName, u32Length);
    u32Frmcnt[u32Chn]++;

_END:
    fclose(fp);
    fp = NULL;
}

#if 0
void *get_frame_data(void* args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32CheckSum = 0;
    MI_VDEC_CHN VdecChn = 0;

    memcpy(&stChnPort, args, sizeof(MI_SYS_ChnPort_t));
    VdecChn = stChnPort.u32ChnId;
    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while (_bRun[VdecChn])
    {
        hSysBuf = MI_HANDLE_NULL;
        //usleep(1000 / _u8FrameRate * 1000);
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }

        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }
#if 0
        else
        {
            if(strncmp(_aeCodecType[VdecChn], "jpeg", 4) == 0)
            {
                MI_U32 u32BufSize = stBufInfo.stFrameData.u16Width * stBufInfo.stFrameData.u16Height * 2;
                //get buffer virtual address
                void *pvirFramAddr = NULL;
                if(MI_SYS_Mmap(stBufInfo.stFrameData.phyAddr[0], u32BufSize, &pvirFramAddr, TRUE))
                {
                    printf(" MI_SYS_Mmap failed!.\n");
                }
                else
                {
                    printf("pvirFramAddr = %p. \n", pvirFramAddr);
                }
                stBufInfo.stFrameData.pVirAddr[0] = pvirFramAddr;
            }
            save_yuv422_data(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0], VdecChn);
        }
#endif

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}
#endif

void create_push_stream_thread(MI_S32 Index, MI_BOOL bCreateGetThead, MI_VDEC_ChnAttr_t *pstChnAttr)
{
    MI_VDEC_CHN VdecChn = 0;
    FILE *readfp = NULL;

    VdecChn = stTestCfg.stChnInfo[Index].VdecChn;

    _bRun[VdecChn] = 1;

    printf("\033[1;32m""fopen file:%s\n""\033[0m", stTestCfg.stChnInfo[Index].au8FilePath);

    readfp = fopen(stTestCfg.stChnInfo[Index].au8FilePath, "rb");
    if (readfp)
    {
        _hFile[VdecChn] = readfp;
        if (pthread_create(&_thrPush[VdecChn], NULL, push_stream, ((Index << 16) | pstChnAttr->eVideoMode)))
        {
            assert(0);
        }

        if (!bCreateGetThead)
        {
            return;
        }

        if(stTestCfg.bDisableLowLatency)
        {
            //if (pthread_create(&_thrGet[VdecChn], NULL, get_meta_data, VdecChn))
            //{
            //    assert(0);
            //}
        }
        else
        {
            //if (pthread_create(&_thrGet[VdecChn], NULL, get_frame_data, VdecChn))
            //{
            //    assert(0);
            //}
        }
    }
    else
    {
        printf("\033[1;31m""VdecChn %d fopen File:%s Error, errno: %s.\n""\033[0m",
            VdecChn, stTestCfg.stChnInfo[Index].au8FilePath, strerror(errno));
    }
}

void destroy_push_stream_thread(MI_VDEC_CHN VdecChn)
{
    if (_bRun[VdecChn])
    {
        _bRun[VdecChn] = 0;
        if (_thrPush[VdecChn])
        {
            pthread_join(_thrPush[VdecChn], NULL);
        }

        if (_thrGet[VdecChn])
        {
            pthread_join(_thrGet[VdecChn], NULL);
        }
    }

    if (_hFile[VdecChn])
    {
        fclose(_hFile[VdecChn]);
        _hFile[VdecChn] = NULL;
    }
}

int start_test_unit()
{
    MI_S32 i = 0;
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn = 0;
    MI_BOOL bCreateGetThead = TRUE;
    MI_SYS_ChnPort_t stChnPort;
    MI_VDEC_InitParam_t stVdecInitParam;

    memset(_thrPush,    0x0, sizeof(_thrPush));
    memset(_thrGet,     0x0, sizeof(_thrGet));
    memset(_bRun,       0x0, sizeof(_bRun));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    memset(&stVdecInitParam, 0x0, sizeof(MI_VDEC_InitParam_t));

    stVdecInitParam.bDisableLowLatency = stTestCfg.bDisableLowLatency;
    MI_VDEC_InitDev(&stVdecInitParam);

    for (i = 0; i < stTestCfg.u32MaxChnNum; i++)
    {
        memset(&stChnAttr, 0x0, sizeof(MI_VDEC_ChnAttr_t));

        VdecChn = stTestCfg.stChnInfo[i].VdecChn;
        stChnAttr.eCodecType    = stTestCfg.stChnInfo[i].eCodecType;
        stChnAttr.u32PicWidth   = stTestCfg.stChnInfo[i].u16Width;
        stChnAttr.u32PicHeight  = stTestCfg.stChnInfo[i].u16Height;
        stChnAttr.eVideoMode    = stTestCfg.stChnInfo[i].eVideoMode;
        stChnAttr.u32BufSize    = stTestCfg.stChnInfo[i].u32BufSize;
        stChnAttr.eDpbBufMode  = stTestCfg.stChnInfo[i].eDpbBufMode;
        stChnAttr.stVdecVideoAttr.u32RefFrameNum = stTestCfg.stChnInfo[i].u32RefFrameNum;
        stChnAttr.u32Priority = 0;

        printf("create chn(%d) codec:%d\n", VdecChn, stChnAttr.eCodecType);

        ExecFunc(MI_VDEC_CreateChn(VdecChn, &stChnAttr), MI_SUCCESS);
        ExecFunc(MI_VDEC_StartChn(VdecChn), MI_SUCCESS);
        if(stTestCfg.stChnInfo[i].u16ScaleWidth != 0 && stTestCfg.stChnInfo[i].u16ScaleHeight != 0)
        {
            MI_VDEC_OutputPortAttr_t stOutputPortAttr;
            stOutputPortAttr.u16Width = stTestCfg.stChnInfo[i].u16ScaleWidth;
            stOutputPortAttr.u16Height = stTestCfg.stChnInfo[i].u16ScaleHeight;

            ExecFunc(MI_VDEC_SetOutputPortAttr(VdecChn, &stOutputPortAttr), MI_SUCCESS);
        }

        stChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stChnPort.u32DevId = 0;
        stChnPort.u32ChnId = VdecChn;
        stChnPort.u32PortId = 0;

        MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 4);

        create_push_stream_thread(i, bCreateGetThead, &stChnAttr);
    }
}


int stop_test_unit()
{
    MI_S32 i = 0;

    for (i = 0; i < stTestCfg.u32MaxChnNum; i++)
    {
        MI_VDEC_CHN VdecChn = stTestCfg.stChnInfo[i].VdecChn;

        destroy_push_stream_thread(VdecChn);
        ExecFunc(MI_VDEC_StopChn(VdecChn), MI_SUCCESS);
        ExecFunc(MI_VDEC_DestroyChn(VdecChn), MI_SUCCESS);
    }
}

void st_save_param_type1(test_cfg_t *pstTestCfg, MI_U8 **pstCfg)
{
    MI_S32 j = 0;

    //bDisableLowLatency
    if(strlen(pstCfg[j++]))
    {
        pstTestCfg->bDisableLowLatency = (MI_BOOL)atoi(pstCfg[j-1]);
    }
    else
    {
        pstTestCfg->bDisableLowLatency = FALSE;
    }

    return;
}

void st_save_param_type2(test_cfg_t *pstTestCfg, MI_U8 **pstCfg)
{
    MI_S32 j = 0;
    MI_S32 k = 0;
    MI_S8 *ps8Arg = NULL;
    MI_U16 u16ChnStart = 0;
    MI_U16 u16ChnEnd = 0;
    MI_U32 u32Index = 0;

    //parse VdecChn
    if(strlen(pstCfg[j++]))
    {
        ps8Arg = strtok(pstCfg[j-1], "-");
        if(!ps8Arg)
        {
            u16ChnStart = (MI_U16)atoi(pstCfg[j-1]);
            u16ChnEnd = u16ChnStart;
        }
        else
        {
            u16ChnStart = (MI_U16)atoi(ps8Arg);
            ps8Arg = strtok(NULL, "-");
            if(!ps8Arg)
            {
                u16ChnEnd = u16ChnStart;
            }
            else
            {
                u16ChnEnd = (MI_U16)atoi(ps8Arg);
                if(u16ChnEnd < u16ChnStart)
                {
                    printf("[%s][%d]: vdec chn config error.\n", __FUNCTION__, __LINE__);
                    return;
                }
            }
        }
    }
    else
    {
        printf("[%s][%d]: vdec chn not config.\n", __FUNCTION__, __LINE__);
        return;
    }

    for(k=u16ChnStart; k<=u16ChnEnd; k++)
    {
        j = 1;
        u32Index = pstTestCfg->u32MaxChnNum;

        //VdecChn
        pstTestCfg->stChnInfo[u32Index].VdecChn = k;

        //Type
        if(0 == strncasecmp(pstCfg[j++], "h264", strlen("h264")))
        {
            pstTestCfg->stChnInfo[u32Index].eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
        }
        else if(0 == strncasecmp(pstCfg[j-1], "h265", strlen("h265")))
        {
            pstTestCfg->stChnInfo[u32Index].eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
        }
        else
        {
            pstTestCfg->stChnInfo[u32Index].eCodecType = (MI_VDEC_CodecType_e)atoi(pstCfg[j-1]);
        }

        //Width
        pstTestCfg->stChnInfo[u32Index].u16Width = (MI_U16)atoi(pstCfg[j++]);

        //Height
        pstTestCfg->stChnInfo[u32Index].u16Height = (MI_U16)atoi(pstCfg[j++]);

        //VideoMode
        if(strlen(pstCfg[j++]))
        {
            if(0 == strncasecmp(pstCfg[j-1], "frame", strlen("frame")))
            {
                pstTestCfg->stChnInfo[u32Index].eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
            }
            else if(0 == strncasecmp(pstCfg[j-1], "stream", strlen("stream")))
            {
                pstTestCfg->stChnInfo[u32Index].eVideoMode = E_MI_VDEC_VIDEO_MODE_STREAM;
            }
            else
            {
                pstTestCfg->stChnInfo[u32Index].eVideoMode = (MI_VDEC_VideoMode_e)atoi(pstCfg[j-1]);
            }
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
        }

        //u32BufSize
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].u32BufSize = (MI_U32)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].u32BufSize = 1048576;
        }

        //eDpbBufMode
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].eDpbBufMode = (MI_VDEC_DPB_BufMode_e)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].eDpbBufMode = E_MI_VDEC_DPB_MODE_NORMAL;
        }

        //maxRefNum
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].u32RefFrameNum = (MI_U32)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].u32RefFrameNum = 2;
        }

        //FrameRate
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].u32FrmRate = (MI_U32)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].u32FrmRate = 30;
        }

        //ScaleW
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].u16ScaleWidth = (MI_U16)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].u16ScaleWidth = 0;
        }

        //ScaleH
        if(strlen(pstCfg[j++]))
        {
            pstTestCfg->stChnInfo[u32Index].u16ScaleHeight = (MI_U16)atoi(pstCfg[j-1]);
        }
        else //default value
        {
            pstTestCfg->stChnInfo[u32Index].u16ScaleHeight = 0;
        }

        //Path
        if(strlen(pstCfg[j++]))
        {
            snprintf(pstTestCfg->stChnInfo[u32Index].au8FilePath, 128, "%s", pstCfg[j-1]);
        }
        else //default value
        {
            printf("[%s][%d]: vdec chn %d not config stream file path.\n", __FUNCTION__, __LINE__, k);
            return;
        }

        pstTestCfg->u32MaxChnNum ++;
    }

    return;
}

int st_save_param(test_cfg_t *pstTestCfg, MI_U8 **pstCfg)
{
    if(!pstTestCfg)
    {
        printf("%s[%d]: pstTestCfg is NULL.\n", __FUNCTION__, __LINE__);
    }
    if(!pstCfg)
    {
        printf("%s[%d]: pstCfg is NULL.\n", __FUNCTION__, __LINE__);
    }

    //parse header
    if(E_TEST_INITPARAM == pstTestCfg->eParamType)
    {
        st_save_param_type1(pstTestCfg, pstCfg);
    }
    else if(E_TEST_VDEC == pstTestCfg->eParamType)
    {
        st_save_param_type2(pstTestCfg, pstCfg);
    }

    return 0;
}

//去除首部空格
char *ltrim(char *str)
{
    if (!str || *str == '\0')
    {
        return str;
    }

    int len = 0;
    char *p = str;
    while (*p != '\0' && isspace(*p))
    {
        ++p;
        ++len;
    }

    memmove(str, p, strlen(str) - len + 1);

    return str;
}

//去掉尾部的空格
char *rtrim(char *str)
{
    if ( !str || *str == '\0')
    {
        return str;
    }

    int len = strlen(str);
    char *p = str + len - 1;
    while (p >= str  && isspace(*p))
    {
        *p = '\0';
        --p;
    }

    return str;
}

int parse_script_file_and_save_param(MI_S8 *pFileName)
{
    MI_S32 i = 0;
    MI_S32 j = 0;
    MI_S32 ret =0;
    FILE *pFile = NULL;
    MI_U8 LineBufTmp[256];
    char  *pLineBufTmp =NULL;
    char  *pLineBuf =NULL;
    MI_U32 u32ParamIndex = 0;
    MI_U8  *pstCfg[12] = {NULL};

    pFile = fopen(pFileName, "r");
    if (!pFile)
    {
        printf("fopen file %s failed\n", pFileName);
        return -1;
    }

    memset(LineBufTmp, 0, 256);
    memset(&stTestCfg, 0, sizeof(test_cfg_t));

    while(NULL != fgets(LineBufTmp, 256, pFile))
    {
        char *p = NULL;
        char *q = NULL;

        // Remove a blank space from the beginning and end of a row
        pLineBufTmp = ltrim(LineBufTmp);
        pLineBuf = rtrim(pLineBufTmp);

        //skip blank line
        if (pLineBuf[0] == '\0' || pLineBuf[0] == '\n' || pLineBuf[0] == '\r' || 0 == strlen(pLineBuf))
        {
            memset(LineBufTmp, 0, 256);
            continue;
        }

        // ignore a line if there is a "//" or "#" at the beginning of a line
        if (strstr(pLineBuf, "//") || strstr(pLineBuf, "#") )
        {
            memset(LineBufTmp, 0, 256);
            continue;
        }

        //find "=" in a line
        if(NULL != (p = strtok(pLineBuf, "=")))
        {
            if(0 == strncasecmp(p, "VDEC", strlen("VDEC")))
            {
                stTestCfg.eParamType = E_TEST_VDEC;
            }
            else if(0 == strncasecmp(p, "INITPARAM", strlen("INITPARAM")))
            {
                stTestCfg.eParamType = E_TEST_INITPARAM;
            }
            else
            {
                printf("[%s][%d]: Script file config error.\n", __FUNCTION__, __LINE__);
                return -1;
            }

            p = strtok(NULL, "=");
            if(!p)
            {
                printf("[%s][%d]: Script file config error.\n", __FUNCTION__, __LINE__);
                return -1;
            }
        }
        else
        {
            printf("[%s][%d]: Script file config error.\n", __FUNCTION__, __LINE__);
            return -1;
        }

        q = strtok(p, ",");
        while(NULL != q)
        {
            char *tmp = NULL;

            tmp = ltrim(q);
            pstCfg[u32ParamIndex] = rtrim(tmp);

            q = strtok(NULL, ",");
            u32ParamIndex ++;
        }

        ret = st_save_param(&stTestCfg, pstCfg);
        if(ret < 0)
        {
            printf("[%s][%d]: vdec set param error.\n", __FUNCTION__, __LINE__);
            return -1;
        }

        u32ParamIndex = 0;
        memset(LineBufTmp, 0, 256);
    }

    return MI_SUCCESS;
}

//int read_input_timeout(int fd, unsigned int wait_seconds)
int read_input_timeout(int maxfdp, unsigned int wait_seconds)
{
    int ret = 0;

    if(wait_seconds > 0)
    {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(maxfdp, &read_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do
        {
            ret=select(maxfdp+1, &read_fdset, NULL, NULL, &timeout);
        }while(ret < 0 && errno == EINTR);

        // check timeout
        if(ret == 0)
        {
            ret=-1;
            errno= ETIMEDOUT;
        }
        else if(ret==1)
        {
            ret= 0;
        }
    }

    return ret;
 }

int test_dyn_SetOutputPortAttr()
{
    int i = 0;
    int j = 0;
    char InputCmd[256] = {0};

    MI_VDEC_CHN VdecChn = 0;
    MI_U16 u16WAlign = 0;
    MI_U16 u16HAlign = 0;
    MI_VDEC_OutputPortAttr_t stOutputPortAttr;

    memset(&stOutputPortAttr, 0, sizeof(MI_VDEC_OutputPortAttr_t));

    while(1)
    {
        j++;

        setbuf(stdin, NULL);
        memset(InputCmd, 0, sizeof(InputCmd));
        printf("[Stop dynamic scaling?<y/Y>]:");
        if(0 == read_input_timeout(fileno(stdin), WAIT_TIMEOUT))
        {
            fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
            if (0 == strncasecmp(InputCmd, "y", strlen("y")) || 0 == strncasecmp(InputCmd, "Y", strlen("Y")))
            {
                printf("Stop dynamic scaling!\n\n");
                break;
            }
            else
            {
                usleep(1*1000*1000);
            }
        }

        for (i = 0; i < stTestCfg.u32MaxChnNum; i++)
        {
            VdecChn = stTestCfg.stChnInfo[i].VdecChn;

            if(E_MI_VDEC_CODEC_TYPE_H264 == stTestCfg.stChnInfo[i].eCodecType)
            {
                u16WAlign = 16;
                u16HAlign = 16;
            }
            else if(E_MI_VDEC_CODEC_TYPE_H265 == stTestCfg.stChnInfo[i].eCodecType)
            {
                u16WAlign = 32;
                u16HAlign = 32;
            }

            if(stTestCfg.stChnInfo[i].u16ScaleWidth != 0 && stTestCfg.stChnInfo[i].u16ScaleHeight != 0)
            {
                stOutputPortAttr.u16Width = stTestCfg.stChnInfo[i].u16ScaleWidth + j * 10;
                stOutputPortAttr.u16Height = stTestCfg.stChnInfo[i].u16ScaleHeight + j * 10;

                if(stOutputPortAttr.u16Width > stTestCfg.stChnInfo[i].u16Width)
                {
                    stOutputPortAttr.u16Width = TEST_ALIGN_UP(stTestCfg.stChnInfo[i].u16Width, u16WAlign) >> 3;
                }
                if(stOutputPortAttr.u16Height > stTestCfg.stChnInfo[i].u16Height)
                {
                    stOutputPortAttr.u16Height = TEST_ALIGN_UP(stTestCfg.stChnInfo[i].u16Height, u16HAlign) >> 3;
                }

                if(MI_SUCCESS != MI_VDEC_SetOutputPortAttr(VdecChn, &stOutputPortAttr))
                {
                    printf("[%d]exec MI_VDEC_SetOutputPortAttr failed\n", __LINE__);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void print_menu1()
{
    printf("*****************************************************\n");
    printf("    q/Q    exit \t\n");
    printf("    dyn    enable dynamic scaling \t\n");
    printf("   stop    Stop sending stream \t\n");
    printf("  start    Start sending stream \t\n");
    printf("*****************************************************\n");
    printf("cmd:");
}

int _stop_all_chn()
{
    int i = 0;
    for (i = 0; i < stTestCfg.u32MaxChnNum; i++)
    {
        ExecFunc(MI_VDEC_StopChn(i), MI_SUCCESS);
    }
}

int _start_all_chn()
{
    int i = 0;
    for (i = 0; i < stTestCfg.u32MaxChnNum; i++)
    {
        ExecFunc(MI_VDEC_StartChn(i), MI_SUCCESS);
    }
}

int main(int argc, const char *argv[])
{
    int i = 0;
    int ret = 0;
    char InputCmd[256] = { 0 };
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    if(argc < 2)
    {
        printf("Input err!!\n");
        printf("Usage: %s [xxx_config.txt].\n", argv[0]);
        return -1;
    }

    // parse script file
    ret = parse_script_file_and_save_param((MI_S8 *)argv[1]);
    if(ret < 0)
    {
        printf("[%s][%d]: parse_script_file_and_save_param failed.\n", __FUNCTION__, __LINE__);
        return -1;
    }

    start_test_unit();
    printf("Start decoding!\n");

    while (1)
    {
        setbuf(stdin, NULL);
        memset(InputCmd, 0, sizeof(InputCmd));
        print_menu1();
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
        if (0 == strncmp(InputCmd, "Q", strlen("Q"))
            || 0 == strncmp(InputCmd, "q", strlen("q")))
        {
            printf("prepare to exit!\n\n");
            goto EXIT_1;
            break;
        }
        else if(0 == strncmp(InputCmd, "dyn", strlen("dyn")))
        {
            printf("Start dynamic scaling!\n\n");
            test_dyn_SetOutputPortAttr();
        }
        else if(0 == strncmp(InputCmd, "stop", strlen("stop")))
        {
            printf("Stop sending stream!\n\n");
            _bStop = TRUE;
            _stop_all_chn();
        }
        else if(0 == strncmp(InputCmd, "start", strlen("start")))
        {
            printf("Start sending stream!\n\n");
            _bStop = FALSE;
            _start_all_chn();
        }
    }

EXIT_1:
    stop_test_unit();
    printf("vdec exit completed!\n\n");

    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    printf("sys exit completed!\n\n");

    return 0;
}
