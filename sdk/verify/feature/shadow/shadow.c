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

#include "mi_shadow.h"
#include "mi_sys.h"

//////////////////////////
///vdec create
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

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

#define VDEC_CHN_MAX (33)
#define VDF_CHN_MAX (16)
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (1024*1024)


static pthread_t _thrPush[VDEC_CHN_MAX];
static pthread_t _thrGet[VDEC_CHN_MAX];
static MI_BOOL _bRun[VDEC_CHN_MAX];
static MI_S32 _hFile[VDEC_CHN_MAX];
static MI_SYS_ChnPort_t _astChnPort;
static MI_U8 _aeCodecType[VDEC_CHN_MAX][8];


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

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VdecChn;
    stChnPort.u32PortId = 0;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = malloc(VESFILE_READER_BATCH);
    while (_bRun[VdecChn])
    {
        usleep(30 * 1000);

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

        u32Len = read(_hFile[VdecChn], pu8Buf, u32FrameLen);
        if(u32Len <= 0)
        {
            lseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        stBufConf.stRawCfg.u32Size = u32Len;

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        hSysBuf = MI_HANDLE_NULL;
        if (MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf,0))
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
        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo,FALSE))
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
            save_yuv422_data(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0], VdecChn);
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

void create_push_stream_thread(MI_VDEC_CHN VdecChn, MI_VDEC_CodecType_e eCodecType, MI_BOOL bCreateGetThead)
{
    MI_U8 pu8FileName[64];
    _bRun[VdecChn] = 1;
    if (eCodecType == E_MI_VDEC_CODEC_TYPE_H264)
    {
        sprintf(pu8FileName, "/mnt/h264_chn_%d.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "h264");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_H265)
    {
        sprintf(pu8FileName, "/mnt/h265_chn_%d.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "h265");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_JPEG)
    {
        sprintf(pu8FileName, "/mnt/jpeg_chn_%d.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "jpeg");
    }
    else
    {
        return;
    }

    printf("%s\n", pu8FileName);
    _hFile[VdecChn] = open(pu8FileName, O_RDONLY, 0);
    if (_hFile[VdecChn] >= 0)
    {
        if (pthread_create(&_thrPush[VdecChn], NULL, push_stream, VdecChn))
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

int StartTestCaseJPEG(MI_BOOL bCreateVdecDumpThread)
{
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn = 0;
    MI_U32 u32TestCaseNum = 0;
    MI_BOOL bCreateGetThead = TRUE;

    memset(_thrPush, 0x0, sizeof(_thrPush));
    memset(_thrGet, 0x0, sizeof(_thrGet));
    memset(_bRun, 0x0, sizeof(_bRun));

    stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
    stChnAttr.stVdecVideoAttr.u32RefFrameNum = 7;
    stChnAttr.eVideoMode = E_MI_VDEC_VIDEO_MODE_FRAME;
    stChnAttr.u32BufSize = 1024 * 1024;
    stChnAttr.u32PicWidth = 720;
    stChnAttr.u32PicHeight = 576;
    stChnAttr.u32Priority = 0;
    ExecFunc(MI_VDEC_CreateChn(0, &stChnAttr), MI_SUCCESS);
    ExecFunc(MI_VDEC_StartChn(0), MI_SUCCESS);
    create_push_stream_thread(0, stChnAttr.eCodecType, bCreateVdecDumpThread);
}

int StopTestCaseJPEG(void)
{
    destroy_push_stream_thread(0);
    ExecFunc(MI_VDEC_StopChn(0), MI_SUCCESS);
    ExecFunc(MI_VDEC_DestroyChn(0), MI_SUCCESS);

}

///vdec create end
////////////////////////////////////////////

////////////////////////////////////////////
///shadow
MI_SHADOW_RegisterDevParams_t _stVDFRegDevInfo;
MI_SHADOW_HANDLE _hVDFDev = 0;

MI_S32 _MI_VDF_OnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    printf("(%d)VDF Get On Input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_VDF_OnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    printf("(%d)VDF Get On output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return E_MI_ERR_FAILED;
}

MI_S32 _MI_VDF_OnUnBindInputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    printf("VDF Get On un input Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

MI_S32 _MI_VDF_OnUnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    printf("(%d)VDF Get On Un Output Event, cur(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) peer(eModId:%d, u32DevId:%d, u32ChnId:%d, u32PortId:%d) pUsrData:%p\n",
        __LINE__,
        pstChnCurryPort->eModId,
        pstChnCurryPort->u32DevId,
        pstChnCurryPort->u32ChnId,
        pstChnCurryPort->u32PortId,
        pstChnPeerPort->eModId,
        pstChnPeerPort->u32DevId,
        pstChnPeerPort->u32ChnId,
        pstChnPeerPort->u32PortId,
        pUsrData);
    return MI_SUCCESS;
}

int create_vdf_channel(void)
{
    MI_U16 u32ChnId = 0;
    MI_U16 u32PortId = 0;

    memset(&_stVDFRegDevInfo, 0x0, sizeof(MI_SHADOW_RegisterDevParams_t));
    _stVDFRegDevInfo.stModDevInfo.eModuleId = E_MI_MODULE_ID_VDF;
    _stVDFRegDevInfo.stModDevInfo.u32DevId = 0;
    _stVDFRegDevInfo.stModDevInfo.u32DevChnNum = 16;
    _stVDFRegDevInfo.stModDevInfo.u32InputPortNum = 1;
    _stVDFRegDevInfo.stModDevInfo.u32OutputPortNum = 0;
    _stVDFRegDevInfo.OnBindInputPort = _MI_VDF_OnBindInputPort;
    _stVDFRegDevInfo.OnBindOutputPort = _MI_VDF_OnBindOutputPort;
    _stVDFRegDevInfo.OnUnBindInputPort = _MI_VDF_OnUnBindInputPort;
    _stVDFRegDevInfo.OnUnBindOutputPort = _MI_VDF_OnUnBindOutputPort;
    ExecFunc(MI_SHADOW_RegisterDev(&_stVDFRegDevInfo, &_hVDFDev), MI_SUCCESS);

    for (u32ChnId = 0; u32ChnId < VDF_CHN_MAX; u32ChnId++) {
        ExecFunc(MI_SHADOW_EnableChannel(_hVDFDev, u32ChnId), MI_SUCCESS);
        ExecFunc(MI_SHADOW_EnableInputPort(_hVDFDev, u32ChnId, u32PortId), MI_SUCCESS);
    }
}

int bind_vdec_to_vdf()
{
    MI_U16 u32ChnId = 0;
    MI_U16 u32PortId = 0;
    MI_SYS_ChnPort_t stOutputChnPort;
    MI_SYS_ChnPort_t stInputChnPort;

    stOutputChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stOutputChnPort.u32DevId = 0;
    stOutputChnPort.u32ChnId = u32ChnId;
    stOutputChnPort.u32PortId = u32PortId;

    for (u32ChnId = 0; u32ChnId < VDF_CHN_MAX; u32ChnId++) {
        stInputChnPort.eModId = E_MI_MODULE_ID_VDF;
        stInputChnPort.u32DevId = 0;
        stInputChnPort.u32ChnId = u32ChnId;
        stInputChnPort.u32PortId = u32PortId;
        ExecFunc(MI_SYS_BindChnPort(&stOutputChnPort, &stInputChnPort, 30, 30), MI_SUCCESS);
    }


}

void vdf_get_frame_data()
{
    MI_U16 u32ChnId = 0;
    MI_U16 u32PortId = 0;
    MI_SYS_BufInfo_t stInputBufInfo;
    MI_SYS_BUF_HANDLE hInputBufHandle = MI_HANDLE_NULL;
    memset(&stInputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

    while (TRUE)
    {
        if (MI_SUCCESS != MI_SHADOW_WaitOnInputTaskAvailable(_hVDFDev, 20))
        {
            printf("wait buffer time out\n");
            continue;
        }

        for (u32ChnId = 0; u32ChnId < VDF_CHN_MAX; u32ChnId++) {
            if (MI_SUCCESS != MI_SHADOW_GetInputPortBuf(_hVDFDev, u32ChnId, u32PortId, &stInputBufInfo, &hInputBufHandle))
            {
                printf("get buffer failed\n");
                continue;
            }
            
            save_yuv422_data(stInputBufInfo.stFrameData.pVirAddr[0], stInputBufInfo.stFrameData.u16Width, stInputBufInfo.stFrameData.u16Height, stInputBufInfo.stFrameData.u32Stride[0], u32ChnId);
            if (TRUE)
            {
                MI_SHADOW_FinishBuf(_hVDFDev, hInputBufHandle);
            }
            else
            {
                MI_SHADOW_RewindBuf(_hVDFDev, hInputBufHandle);
            }
        }
    }


}

void vdf_destroy()
{
    MI_SHADOW_UnRegisterDev(_hVDFDev);
}
////shadow end
/////////////////////////////////////////////

int main(int argc, const char *argv[])
{
    char InputCmd[256] = { 0 };
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    while (1)
    {
        printf("shadow wait for command:\n");
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);
        if (strncmp(InputCmd, "exit", 4) == 0)
        {
            printf("prepare to exit!\n\n");
            goto EXIT_1;
            break;
        }
        else if (strncmp(InputCmd, "start_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            StartTestCaseJPEG(FALSE);
            create_vdf_channel();
            bind_vdec_to_vdf();
            vdf_get_frame_data();
        }
        else if (strncmp(InputCmd, "stop_jpeg", (strlen(InputCmd) - 1)) == 0)
        {
            StopTestCaseJPEG();
        }
    }

EXIT_1:

    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);
    return 0;
}
