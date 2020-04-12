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
#define VESFILE_READER_BATCH (128 * 1024)


static pthread_t _thrPush[VDEC_CHN_MAX];
static pthread_t _thrGet[VDEC_CHN_MAX];
static MI_BOOL _bRun[VDEC_CHN_MAX];
static FILE *_hFile[VDEC_CHN_MAX];
static MI_SYS_ChnPort_t _astChnPort;

static MI_U8  _u8ChnNum  = 1;
static MI_U8  _aeCodecType[VDEC_CHN_MAX][8];
static MI_U32 _u32PicWidth = 1920;
static MI_U32 _u32PicHeight = 1080;
static MI_U8  _u8FrameRate = 30;
static MI_U8  au8FilePath[256] = "";
static MI_VDEC_DPB_BufMode_e _eDpbBufMode = E_MI_VDEC_DPB_MODE_NORMAL;
static MI_U16 _out_w = 0;
static MI_U16 _out_h = 0;

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

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)(u32Param >> 16);
    MI_VDEC_VideoMode_e eVideoMode = (MI_VDEC_VideoMode_e)(u32Param & 0xFF);

    pu8Buf = malloc(VESFILE_READER_BATCH);
    printf("chn(%d) %s\n", VdecChn, (E_MI_VDEC_VIDEO_MODE_STREAM == eVideoMode) ? "stream" : "frame");

    while (_bRun[VdecChn])
    {
        usleep(1000 / _u8FrameRate * 1000);

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

void *get_meta_data(void* args)
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

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 3, 5);
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

            printf("Chn(%d), Pixel:%d, phyAddr:0x%llx, pVirAddr:%p, W:%d, H:%d, BufSize:%u.\n",
                VdecChn,
                stBufInfo.stFrameData.ePixelFormat,
                stBufInfo.stFrameData.phyAddr[0],
                stBufInfo.stFrameData.pVirAddr[0],
                stBufInfo.stFrameData.u16Width,
                stBufInfo.stFrameData.u16Height,
                stBufInfo.stFrameData.u32BufSize);

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
        printf("get frame count:%d\n", u32Frmcnt[u32Chn]++);
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
        usleep(1000 / _u8FrameRate * 1000);
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

void create_push_stream_thread(MI_VDEC_CHN VdecChn, MI_BOOL bCreateGetThead, MI_VDEC_ChnAttr_t *pstChnAttr)
{
    FILE *readfp = NULL;

    _bRun[VdecChn] = 1;

    printf("\033[1;32m""fppen file:%s\n""\033[0m", au8FilePath);

    readfp = fopen(au8FilePath, "rb");
    if (readfp)
    {
        _hFile[VdecChn] = readfp;
        if (pthread_create(&_thrPush[VdecChn], NULL, push_stream, ((VdecChn << 16) | pstChnAttr->eVideoMode)))
        {
            assert(0);
        }

        if (!bCreateGetThead)
        {
            return;
        }

        if (pthread_create(&_thrGet[VdecChn], NULL, get_meta_data, VdecChn))
        {
            assert(0);
        }
    }
    else
    {
        printf("\033[1;31m""VdecChn %d fopen File:%s Error\n""\033[0m", VdecChn, au8FilePath);
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
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn = 0;
    MI_BOOL bCreateGetThead = TRUE;

    memset(_thrPush,    0x0, sizeof(_thrPush));
    memset(_thrGet,     0x0, sizeof(_thrGet));
    memset(_bRun,       0x0, sizeof(_bRun));

    for (VdecChn = 0; VdecChn < _u8ChnNum; VdecChn++)
    {
        memset(&stChnAttr, 0x0, sizeof(MI_VDEC_ChnAttr_t));

        if(0 == strncasecmp(_aeCodecType[VdecChn], "h264", strlen("h264")))
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
        }
        else if(0 == strncasecmp(_aeCodecType[VdecChn], "h265", strlen("h265")))
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
        }
        else if(0 == strncasecmp(_aeCodecType[VdecChn], "jpeg", strlen("jpeg")))
        {
            stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
        }
        else
        {
            printf("VdecChn(%d) CodecType(%s) err!\n", VdecChn, _aeCodecType[VdecChn]);
            return;
        }

        printf("create chn(%d) codec:%d\n", VdecChn, stChnAttr.eCodecType);

        stChnAttr.stVdecVideoAttr.u32RefFrameNum = 1;
        stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
        stChnAttr.u32BufSize = 1 * 1024 * 1024;
        stChnAttr.u32PicWidth = _u32PicWidth;
        stChnAttr.u32PicHeight = _u32PicHeight;
        stChnAttr.eDpbBufMode = _eDpbBufMode;
        stChnAttr.u32Priority = 0;
        ExecFunc(MI_VDEC_CreateChn(VdecChn, &stChnAttr), MI_SUCCESS);
        ExecFunc(MI_VDEC_StartChn(VdecChn), MI_SUCCESS);
        if(_out_w != 0 && _out_h != 0)
        {
            MI_VDEC_OutputPortAttr_t stOutputPortAttr;
            stOutputPortAttr.u16Width = _out_w;
            stOutputPortAttr.u16Height = _out_h;

            ExecFunc(MI_VDEC_SetOutputPortAttr(VdecChn, &stOutputPortAttr), MI_SUCCESS);
        }
        create_push_stream_thread(VdecChn, bCreateGetThead, &stChnAttr);
    }
}


int stop_test_unit()
{
    MI_VDEC_CHN VdecChn = 0;

    for (VdecChn = 0; VdecChn < _u8ChnNum; VdecChn++)
    {
        destroy_push_stream_thread(VdecChn);
        ExecFunc(MI_VDEC_StopChn(VdecChn), MI_SUCCESS);
        ExecFunc(MI_VDEC_DestroyChn(VdecChn), MI_SUCCESS);
    }
}


int main(int argc, const char *argv[])
{
    int i = 0;
    char InputCmd[256] = { 0 };
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    if(argc < 7 || argc > 10)
    {
        printf("Input err!!\n");
        printf("Usage: %s [ChnNum] [CodeType] [PicW] [PicH] [FrameRate] [FilePath] [DpbBufMode] [OutW] [OutH]\n", argv[0]);
        printf("Like: %s 4 h264 720 576 30 /mnt/file 0 720 576\n", argv[0]);
        return -1;
    }

    if(argv[1])
    {
        _u8ChnNum = atoi(argv[1]);
        printf("[%s][%d] ChnNum: %u\n", __FUNCTION__, __LINE__, _u8ChnNum);
    }
    if(argv[2])
    {
        for(i = 0; i < _u8ChnNum; i++)
        {
            strcpy(_aeCodecType[i], argv[2]);
            printf("[%s][%d] CodecType: %s\n", __FUNCTION__, __LINE__, _aeCodecType[i]);
        }
    }
    if(argv[3])
    {
        _u32PicWidth  = atoi(argv[3]);
        printf("[%s][%d] PicW[%u]\n", __FUNCTION__, __LINE__, _u32PicWidth);
    }
    if(argv[4])
    {
        _u32PicHeight = atoi(argv[4]);
        printf("[%s][%d] PicH[%u]\n", __FUNCTION__, __LINE__, _u32PicHeight);
    }
    if(argv[5])
    {
        _u8FrameRate  = atoi(argv[5]);
        printf("[%s][%d] FrameRate: %u\n", __FUNCTION__, __LINE__, _u8FrameRate);
    }
    if(argv[6])
    {
        strcpy(au8FilePath, argv[6]);
        printf("file path:%s\n", au8FilePath);
    }
    if(argv[7])
    {
        _eDpbBufMode = (MI_VDEC_DPB_BufMode_e)atoi(argv[7]);
        printf("DpbBufMode:%d\n", _eDpbBufMode);
    }
    if(argv[8])
    {
        _out_w = (MI_U16)atoi(argv[8]);
        printf("_out_w:%d\n", _out_w);
    }
    if(argv[9])
    {
        _out_h = (MI_U16)atoi(argv[9]);
        printf("_out_h:%d\n", _out_h);
    }

    start_test_unit();

    while (1)
    {
        setbuf(stdin, NULL);
        memset(InputCmd, 0, sizeof(InputCmd));
        printf("cmd:\n");
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
        if (0 == strncmp(InputCmd, "exit", strlen("exit"))
            || 0 == strncmp(InputCmd, "q", strlen("q")))
        {
            printf("prepare to exit!\n\n");
            goto EXIT_1;
            break;
        }
    }

EXIT_1:
    stop_test_unit();
    printf("vdec exit completed!\n\n");

    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    printf("sys exit completed!\n\n");

    return 0;
}
