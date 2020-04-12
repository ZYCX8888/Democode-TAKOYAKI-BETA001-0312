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

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp_datatype.h"
#include "mi_divp.h"

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
#define VESFILE_READER_BATCH (1024*1024)


static pthread_t _thrPush[VDEC_CHN_MAX];
static pthread_t _thrGet[VDEC_CHN_MAX];
static MI_BOOL _bRun[VDEC_CHN_MAX];
static MI_S32 _hFile[VDEC_CHN_MAX];
static MI_SYS_ChnPort_t _astChnPort;
static MI_U8 _aeCodecType[VDEC_CHN_MAX][8];
static TEST_CASE_e _eTestCase = TEST_CASE_H264_DIVP;
static MI_U8 au8FilePath[256] = "/mnt";

static MI_U8 _u8TestNum  = 1;
static MI_U8 _u8TestFrameRate = 30;
static MI_U32 _u32PicWidth = 1920;
static MI_U32 _u32PicHeight = 1080;

#define _CHECKPOINT_ printf("xxxxxxxxx [%s][%d] xxxxxxxx\n", __FUNCTION__, __LINE__);
MI_U64 get_pts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

void *push_stream(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;

    MI_U8 *pu8Buf = NULL;
    MI_U32 u32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_U32 u32Param = (MI_U32)p;

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)(u32Param >> 16);
    MI_VDEC_VideoMode_e eVideoMode = (MI_VDEC_VideoMode_e)(u32Param & 0xFF);
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VdecChn;
    stChnPort.u32PortId = 0;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = malloc(VESFILE_READER_BATCH);
    printf("chn(%d) %s\n", VdecChn, (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode) ? "stream" : "frame");
    while (_bRun[VdecChn])
    {
        usleep(1000 / _u8TestFrameRate * 1000);

        if (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode)
        {
            ///stream mode, read 128k every time
            u32FrameLen = VESFILE_READER_BATCH;
            u32Pos = lseek(_hFile[VdecChn], 0L, SEEK_CUR);
        }
        else
        {
            ///frame mode, read one frame lenght every time
            memset(au8Header, 0, 16);
            u32Pos = lseek(_hFile[VdecChn], 0L, SEEK_CUR);
            u32Len = read(_hFile[VdecChn], au8Header, 16);
            if(u32Len <= 0)
            {
                lseek(_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }

            u32FrameLen = MI_U32VALUE(au8Header, 4);
            if(u32FrameLen > VESFILE_READER_BATCH)
            {
                lseek(_hFile[VdecChn], 0, SEEK_SET);
                continue;
            }
        }

        u32Len = read(_hFile[VdecChn], pu8Buf, u32FrameLen);
        if(u32Len <= 0)
        {
            lseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        stBufConf.stRawCfg.u32Size = u32Len;

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        hSysBuf = MI_HANDLE_NULL;
        if (MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf , 0))
        {
            lseek(_hFile[VdecChn], u32Pos, SEEK_SET);
            continue;
        }

        memcpy(stBufInfo.stRawData.pVirAddr, pu8Buf, u32Len);

        stBufInfo.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufInfo.bEndOfStream = FALSE;
        stBufInfo.u64Pts = u64Pts;
        stBufInfo.stRawData.bEndOfFrame = TRUE;
        stBufInfo.stRawData.u32ContentSize = u32Len;
        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, FALSE))
        {
            lseek(_hFile[VdecChn], u32Pos, SEEK_SET);
            continue;
        }

        u64Pts = u64Pts + get_pts(30);

        memset(&stChnStat, 0x0, sizeof(stChnStat));
        MI_VDEC_GetChnStat(VdecChn, &stChnStat);
        printf("Chn(%d)_%s_Codec:%d, Frame Dec:%d\n", VdecChn, _aeCodecType[VdecChn], stChnStat.eCodecType, stChnStat.u32DecodeStreamFrames);
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

    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
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
#if (0)
            printf("Chn(%d), Bufino(bEndOfStream:%d, bUsrBuf:%d, eBufType:%d, Pts:%lld)\n",
                VdecChn,
                stBufInfo.bEndOfStream,
                stBufInfo.bUsrBuf,
                stBufInfo.eBufType,
                stBufInfo.u64Pts);

            printf("Chn(%d), eDataFromModule:%d, phyAddr:0x%llx, pVirAddr:0x%p, u32Size:%d\n",
                VdecChn,
                stBufInfo.stMetaData.eDataFromModule,
                stBufInfo.stMetaData.phyAddr,
                stBufInfo.stMetaData.pVirAddr,
                stBufInfo.stMetaData.u32Size);

#endif
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

void save_yuv422_data(MI_U8 *pYuv422Data, MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32Pitch, MI_U32 u32Chn)
{
    FILE *fp = NULL;
    char aName[128];
    MI_U32 u32Length = u32Width * u32Height * 2;
    static MI_U32 u32Frmcnt[33] = {0};

    if (u32Frmcnt[u32Chn] > 5)
    {
        printf("get frame count:%d\n", u32Frmcnt[u32Chn]++);
        return;
    }
    memset(aName, 0x0, sizeof(aName));
    if (_eTestCase == TEST_CASE_H264_DIVP)
    {
        sprintf(aName, "/mnt/app_chn_%d_dump_h264_divp[%d_%d_%d]_%d.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);
    }
    else if (_eTestCase == TEST_CASE_H265_DIVP)
    {
        sprintf(aName, "/mnt/app_chn_%d_dump_h265_divp[%d_%d_%d]_%d.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);
    }
    else
    {
        sprintf(aName, "/mnt/app_chn_%d_jpeg_dump_vdec[%d_%d_%d]_%d.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);
    }
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

void *get_frame_data(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32CheckSum = 0;
    MI_VDEC_CHN VdecChn = 0;

    memcpy(&stChnPort, p, sizeof(MI_SYS_ChnPort_t));
    VdecChn = stChnPort.u32ChnId;
    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,20);
    while (_bRun[VdecChn])
    {
        hSysBuf = MI_HANDLE_NULL;
        usleep(30 * 1000);
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }

        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }
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

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

void create_push_stream_thread(MI_VDEC_CHN VdecChn, MI_VDEC_CodecType_e eCodecType, MI_BOOL bCreateGetThead, MI_VDEC_VideoMode_e eVideoMode)
{
    MI_U8 pu8FileName[64];
    _bRun[VdecChn] = 1;
    if (eCodecType == E_MI_VDEC_CODEC_TYPE_H264)
    {
        if (eVideoMode == E_MI_VDEC_VIDEO_MODE_STREAM)
        {
            sprintf(pu8FileName, "%s/h264_chn_%d.stream", au8FilePath, VdecChn);
        }
        else
        {
            sprintf(pu8FileName, "%s/h264_chn_%d.es", au8FilePath, VdecChn);
        }
        sprintf(_aeCodecType[VdecChn], "h264");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_H265)
    {
        if (eVideoMode == E_MI_VDEC_VIDEO_MODE_STREAM)
        {
            sprintf(pu8FileName, "%s/h265_chn_%d.stream", au8FilePath, VdecChn);
        }
        else
        {
            sprintf(pu8FileName, "%s/h265_chn_%d.es", au8FilePath, VdecChn);
        }
        sprintf(_aeCodecType[VdecChn], "h265");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_JPEG)
    {
        if (eVideoMode == E_MI_VDEC_VIDEO_MODE_STREAM)
        {
            sprintf(pu8FileName, "%s/jpeg_chn_%d.stream", au8FilePath, VdecChn);
        }
        else
        {
            sprintf(pu8FileName, "%s/jpeg_chn_%d.es", au8FilePath, VdecChn);
        }
        sprintf(_aeCodecType[VdecChn], "jpeg");
    }
    else
    {
        return;
    }

    printf("\033[1;32m""Open File:%s\n""\033[0m", pu8FileName);
    _hFile[VdecChn] = open(pu8FileName, O_RDONLY, 0);
    if (_hFile[VdecChn] >= 0)
    {
        if (pthread_create(&_thrPush[VdecChn], NULL, push_stream, ((VdecChn << 16) | eVideoMode)))
        {
            assert(0);
        }
        if (!bCreateGetThead)
        {
            return;
        }

        if (eCodecType == E_MI_VDEC_CODEC_TYPE_JPEG)
        {
            memset(&_astChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            _astChnPort.eModId = E_MI_MODULE_ID_VDEC;
            _astChnPort.u32DevId = 0;
            _astChnPort.u32ChnId = VdecChn;
            _astChnPort.u32PortId = 0;
            if (pthread_create(&_thrGet[VdecChn], NULL, get_frame_data, &_astChnPort))
            {
                assert(0);
            }
        }
        else
        {
            if (pthread_create(&_thrGet[VdecChn], NULL, get_meta_data, VdecChn))
            {
                assert(0);
            }
        }
    }
    else
    {
        printf("\033[1;31m""Open File:%s Error\n""\033[0m", pu8FileName);
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

    if (_hFile[VdecChn] >= 0)
    {
        close(_hFile[VdecChn]);
    }
}

int start_test_unit(MI_U32 u32TestCase)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn = 0;
    MI_U32 u32TestCaseNum = 0;
    MI_BOOL bCreateGetThead = TRUE;

    memset(_thrPush, 0x0, sizeof(_thrPush));
    memset(_thrGet, 0x0, sizeof(_thrGet));
    memset(_bRun, 0x0, sizeof(_bRun));

    _eTestCase = (TEST_CASE_e)u32TestCase;
    switch (u32TestCase)
    {
        case TEST_CASE_JPEG:
        case TEST_CASE_H264:
        case TEST_CASE_H265:
        case TEST_CASE_H264_DIVP:
        case TEST_CASE_H265_DIVP:
        {
            u32TestCaseNum = 1;
        }
        break;

        case TEST_CASE_H264_H265:
        case TEST_CASE_H264_H265_FRAMING:
        {
            u32TestCaseNum = 2;
        }
        break;

        case TEST_CASE_H264_H265_JPEG:
        {
            u32TestCaseNum = 3;
        }
        break;

        case TEST_CASE_SWITCH_DIVP:
        {
            u32TestCaseNum = 4;
        }
        break;

        default:
            return;
    }

    u32TestCaseNum = _u8TestNum;

    for (VdecChn = 0; VdecChn < u32TestCaseNum; VdecChn++)
    {
        memset(&stChnAttr, 0x0, sizeof(MI_VDEC_ChnAttr_t));
        if (u32TestCase == TEST_CASE_JPEG)
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
        }
        else if ((u32TestCase == TEST_CASE_H264) || (u32TestCase == TEST_CASE_H264_DIVP))
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
        }
        else if ((u32TestCase == TEST_CASE_H265) || (u32TestCase == TEST_CASE_H265_DIVP))
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
        }
        else
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
            if (VdecChn == 1)
            {
                stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
            }
            else if (VdecChn == 2)
            {
                stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
            }
        }

        if ((u32TestCase == TEST_CASE_H264_DIVP) || (u32TestCase == TEST_CASE_H265_DIVP) || (u32TestCase == TEST_CASE_SWITCH_DIVP))
        {
            bCreateGetThead = FALSE;
        }

        if (u32TestCase == TEST_CASE_SWITCH_DIVP)
        {
            if ((VdecChn % 2))
            {
                stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
            }
            else
            {
                stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
            }
        }

        printf("create chn(%d) codec:%d ,case:%d\n", VdecChn, stChnAttr.eCodecType, u32TestCase);
        stChnAttr.stVdecVideoAttr.u32RefFrameNum = 1;
        if (TEST_CASE_H264_H265_FRAMING != u32TestCase)
        {
            stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
        }
        else
        {
            stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_STREAM;
        }

        stChnAttr.u32BufSize = 1024 * 1024;
        stChnAttr.u32PicWidth = _u32PicWidth;
        stChnAttr.u32PicHeight = _u32PicHeight;
        stChnAttr.u32Priority = 0;
        ExecFunc(MI_VDEC_CreateChn(VdecChn, &stChnAttr), MI_SUCCESS);
        ExecFunc(MI_VDEC_StartChn(VdecChn), MI_SUCCESS);
        create_push_stream_thread(VdecChn, stChnAttr.eCodecType, bCreateGetThead, stChnAttr.eVideoMode);
    }
}


int stop_test_unit(MI_U32 u32TestCase)
{
    MI_U32 u32TestCaseNum = 0;
    MI_VDEC_CHN VdecChn = 0;

    switch (u32TestCase)
    {
        case TEST_CASE_JPEG:
        case TEST_CASE_H264:
        case TEST_CASE_H265:
        case TEST_CASE_H264_DIVP:
        case TEST_CASE_H265_DIVP:
        {
            u32TestCaseNum = 1;
        }
        break;

        case TEST_CASE_H264_H265:
        case TEST_CASE_H264_H265_FRAMING:
        {
            u32TestCaseNum = 2;
        }
        break;

        case TEST_CASE_H264_H265_JPEG:
        {
            u32TestCaseNum = 3;
        }
        break;

        case TEST_CASE_SWITCH_DIVP:
        {
            u32TestCaseNum = 4;
        }
        break;

        default:
            return;
    }

    for (VdecChn = 0; VdecChn < u32TestCaseNum; VdecChn++)
    {
        destroy_push_stream_thread(VdecChn);
        ExecFunc(MI_VDEC_StopChn(VdecChn), MI_SUCCESS);
        ExecFunc(MI_VDEC_DestroyChn(VdecChn), MI_SUCCESS);
    }
}

void start_test_case_jpeg(void)
{
    start_test_unit(TEST_CASE_JPEG);
}

void stop_test_case_jpeg(void)
{
    stop_test_unit(TEST_CASE_JPEG);
}

void start_test_case_h264(void)
{
    start_test_unit(TEST_CASE_H264);
}

void stop_test_case_h264(void)
{
    stop_test_unit(TEST_CASE_H264);
}

void start_test_case_h265(void)
{
    start_test_unit(TEST_CASE_H265);
}

void stop_test_case_h265(void)
{
    stop_test_unit(TEST_CASE_H265);
}

void start_test_case_h264_h265(void)
{
    start_test_unit(TEST_CASE_H264_H265);
}

void stop_test_case_h264_h265(void)
{
    stop_test_unit(TEST_CASE_H264_H265);
}

void start_test_case_h264_h265_jpeg(void)
{
    start_test_unit(TEST_CASE_H264_H265_JPEG);
}

void stop_test_case_h264_h265_jpeg(void)
{
    stop_test_unit(TEST_CASE_H264_H265_JPEG);
}

void start_divp(void)
{
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_DIVP_ChnAttr_t stAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_SYS_ChnPort_t stDivpInputPort;
    MI_SYS_ChnPort_t stDivpOutputPort;
    MI_U32 u32ChnId = 0;

    stAttr.bHorMirror = FALSE;
    stAttr.bVerMirror = FALSE;
    stAttr.eDiType = E_MI_DIVP_DI_TYPE_OFF;
    stAttr.eRotateType = E_MI_SYS_ROTATE_NONE;
    stAttr.eTnrLevel = E_MI_DIVP_TNR_LEVEL_OFF;
    stAttr.stCropRect.u16X = 0;
    stAttr.stCropRect.u16Y = 0;
    stAttr.stCropRect.u16Width = 1920;
    stAttr.stCropRect.u16Height = 1080;
    stAttr.u32MaxWidth = 1920;
    stAttr.u32MaxHeight = 1080;
    MI_DIVP_CreateChn(0, &stAttr);
    ///MI_DIVP_SetChnAttr(0, &stAttr);

    stOutputPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stOutputPortAttr.u32Width = 1920;
    stOutputPortAttr.u32Height = 1080;

    MI_DIVP_SetOutputPortAttr(0, &stOutputPortAttr);
    MI_DIVP_StartChn(0);

    stDivpOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpOutputPort.u32DevId = 0;
    stDivpOutputPort.u32ChnId = u32ChnId;
    stDivpOutputPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stDivpOutputPort, 10, 20);

    stDivpInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpInputPort.u32DevId = 0;
    stDivpInputPort.u32ChnId = u32ChnId;
    stDivpInputPort.u32PortId = 0;

    memcpy(&_astChnPort, &stDivpInputPort, sizeof(MI_SYS_ChnPort_t));
    if (pthread_create(&_thrGet[stDivpInputPort.u32ChnId], NULL, get_frame_data, &_astChnPort))
    {
        assert(0);
    }
}

void stop_divp(void)
{
    MI_U32 u32ChnId = 0;
    MI_DIVP_StopChn(u32ChnId);
    MI_DIVP_DestroyChn(u32ChnId);
}

void start_test_case_h264_divp(int h264)
{
    MI_SYS_ChnPort_t stDivpInputPort;
    MI_SYS_ChnPort_t stVdecOutputPort;
    MI_U32 u32ChnId = 0;

    if (h264)
    {
        start_test_unit(TEST_CASE_H264_DIVP);
    }
    else
    {
        start_test_unit(TEST_CASE_H265_DIVP);
    }

    start_divp();
    stDivpInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpInputPort.u32DevId = 0;
    stDivpInputPort.u32ChnId = 0;
    stDivpInputPort.u32PortId = u32ChnId;

    stVdecOutputPort.eModId = E_MI_MODULE_ID_VDEC;
    stVdecOutputPort.u32DevId = 0;
    stVdecOutputPort.u32ChnId = u32ChnId;
    stVdecOutputPort.u32PortId = 0;

    MI_SYS_BindChnPort(&stVdecOutputPort, &stDivpInputPort, 30, 30);
}

void stop_test_case_h264_divp(int h264)
{
    MI_SYS_ChnPort_t stDivpInputPort;
    MI_SYS_ChnPort_t stVdecOutputPort;
    MI_U32 u32ChnId = 0;

    stDivpInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpInputPort.u32DevId = 0;
    stDivpInputPort.u32ChnId = u32ChnId;
    stDivpInputPort.u32PortId = 0;

    stVdecOutputPort.eModId = E_MI_MODULE_ID_VDEC;
    stVdecOutputPort.u32DevId = 0;
    stVdecOutputPort.u32ChnId = u32ChnId;
    stVdecOutputPort.u32PortId = 0;
    MI_SYS_UnBindChnPort(&stVdecOutputPort, &stDivpInputPort);

    if (h264)
    {
        stop_test_unit(TEST_CASE_H264_DIVP);
    }
    else
    {
        stop_test_unit(TEST_CASE_H265_DIVP);
    }

    stop_divp();
}

void start_test_case_switch_divp(void)
{
    MI_SYS_ChnPort_t stDivpInputPort;
    MI_SYS_ChnPort_t stVdecOutputPort;
    MI_U32 u32ChnId = 0;
    MI_U8 i = 0;

    start_test_unit(TEST_CASE_SWITCH_DIVP);
    start_divp();

    stDivpInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpInputPort.u32DevId = 0;
    stDivpInputPort.u32ChnId = 0;
    stDivpInputPort.u32PortId = u32ChnId;

    stVdecOutputPort.eModId = E_MI_MODULE_ID_VDEC;
    stVdecOutputPort.u32DevId = 0;
    stVdecOutputPort.u32ChnId = u32ChnId;
    stVdecOutputPort.u32PortId = 0;

    MI_SYS_BindChnPort(&stVdecOutputPort, &stDivpInputPort, 30, 30);
    for (i = 0; i < (4 * 3); ++i)
    {
        usleep(500 * 1000);
        printf("test(%d) switch divp bind vdec chn(%d)\n", i, (i + 1) % 4);
        MI_SYS_UnBindChnPort(&stVdecOutputPort, &stDivpInputPort);
        stVdecOutputPort.eModId = E_MI_MODULE_ID_VDEC;
        stVdecOutputPort.u32DevId = 0;
        stVdecOutputPort.u32ChnId = (i + 1) % 4;
        stVdecOutputPort.u32PortId = 0;
        MI_SYS_BindChnPort(&stVdecOutputPort, &stDivpInputPort, 30, 30);
    }

    printf("test switch divp end\n");
}

void stop_test_case_switch_divp(void)
{
    MI_SYS_ChnPort_t stDivpInputPort;
    MI_SYS_ChnPort_t stVdecOutputPort;
    MI_U32 u32ChnId = 0;

    stDivpInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpInputPort.u32DevId = 0;
    stDivpInputPort.u32ChnId = u32ChnId;
    stDivpInputPort.u32PortId = 0;

    stVdecOutputPort.eModId = E_MI_MODULE_ID_VDEC;
    stVdecOutputPort.u32DevId = 0;
    stVdecOutputPort.u32ChnId = u32ChnId;
    stVdecOutputPort.u32PortId = 0;
    MI_SYS_UnBindChnPort(&stVdecOutputPort, &stDivpInputPort);

    stop_test_unit(TEST_CASE_SWITCH_DIVP);
    stop_divp();
}

void start_test_case_h264_h265_framing(void)
{
    start_test_unit(TEST_CASE_H264_H265_FRAMING);
}

void stop_test_case_h264_h265_framing(void)
{
    stop_test_unit(TEST_CASE_H264_H265_FRAMING);
}

int main(int argc, const char *argv[])
{
    char InputCmd[256] = { 0 };
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    printf("./prog $FilePath $ChnMax $PushEsFrameRate $PicWidth $PicHeight\n");
    if (argc > 1)
    {
        strcpy(au8FilePath, argv[1]);
    }
    else
    {
        strcpy(au8FilePath, "/mnt");
    }

    if(argc > 2)
    {
        _u8TestNum = atoi(argv[2]);
        printf("[%s][%d] _u8TestNum[%u]\n", __FUNCTION__, __LINE__, _u8TestNum);
    }

    if(argc > 3)
    {
        _u8TestFrameRate = atoi(argv[3]);
        printf("[%s][%d] _u8TestFrameRate[%u]\n", __FUNCTION__, __LINE__, _u8TestFrameRate);
    }

    if(argc > 5)
    {
        _u32PicWidth = atoi(argv[4]);
        _u32PicHeight = atoi(argv[5]);
        printf("[%s][%d] _u32PicWidth[%u] _u32PicHeight[%u]\n", __FUNCTION__, __LINE__, _u32PicWidth, _u32PicHeight);
    }

    printf("file path:%s\n", au8FilePath);
    while (1)
    {
        printf("wait for command:\n");
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
        if (strncmp(InputCmd, "exit", 4) == 0)
        {
            printf("prepare to exit!\n\n");
            goto EXIT_1;
            break;
        }
        else if (strncmp(InputCmd, "start_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_jpeg();
        }
        else if (strncmp(InputCmd, "stop_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_jpeg();
        }
        else if (strncmp(InputCmd, "start_h264", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264();
        }
        else if (strncmp(InputCmd, "stop_h264", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264();
        }
        else if (strncmp(InputCmd, "start_h265", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h265();
        }
        else if (strncmp(InputCmd, "stop_h265", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h265();
        }
        else if (strncmp(InputCmd, "start_h264_h265", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264_h265();
        }
        else if (strncmp(InputCmd, "stop_h264_h265", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264_h265();
        }
        else if (strncmp(InputCmd, "start_h264_h265_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264_h265_jpeg();
        }
        else if (strncmp(InputCmd, "stop_h264_h265_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264_h265_jpeg();
        }
        else if (strncmp(InputCmd, "start_h264_divp", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264_divp(TRUE);
        }
        else if (strncmp(InputCmd, "stop_h264_divp", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264_divp(TRUE);
        }
        else if (strncmp(InputCmd, "start_h265_divp", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264_divp(FALSE);
        }
        else if (strncmp(InputCmd, "stop_h265_divp", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264_divp(FALSE);
        }
        else if (strncmp(InputCmd, "start_switch_divp", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_switch_divp();
        }
        else if (strncmp(InputCmd, "stop_switch_divp", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_switch_divp();
        }
        else if (strncmp(InputCmd, "start_h264_h265_framing", (strlen(InputCmd) - 1)) == 0)
        {
            start_test_case_h264_h265_framing();
        }
        else if (strncmp(InputCmd, "stop_h264_h265_framing", (strlen(InputCmd) - 1)) == 0)
        {
            stop_test_case_h264_h265_framing();
        }
    }

EXIT_1:

    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    return 0;
}
