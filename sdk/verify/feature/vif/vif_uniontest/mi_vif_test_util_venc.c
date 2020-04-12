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
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include "mi_vif_test_util.h"

typedef struct VENC_FPS_s {
    struct timeval stTimeStart, stTimeEnd;
    struct timezone stTimeZone;
    MI_U32 u32DiffUs;
    MI_U32 u32TotalBits;
    MI_U32 u32FrameCnt;
    MI_BOOL bRestart;
} VENC_FPS_t;
#define TIMEVAL_US_DIFF(start, end)     ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct VENC_Rc_s
{
    MI_VENC_RcMode_e eRcMode;
    MI_U32 u32SrcFrmRate;
    MI_U32 u32Bitrate; //u32MaxBitRate for VBR, u32BitRate for CBR, u32AvgBitRate for ABR
    MI_U32 u32AvrMaxBitrate; //u32MaxBitRate for ABR
    MI_U32 u32FixQp;
    MI_U32 u32VbrMinQp;
    MI_U32 u32VbrMaxQp;
} VENC_Rc_t;

typedef enum {
    E_VENC_DEV_MHE0,
    E_VENC_DEV_MHE1,
    E_VENC_DEV_MFE0,
    E_VENC_DEV_MFE1,
    E_VENC_DEV_JPGE,
    E_VENC_DEV_DUMMY,
    E_VENC_DEV_MAX
} VENC_Device_e;

static MI_BOOL venc_channel_port_stop[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
static pthread_t venc_channel_port_tid[MI_VIF_MAX_PHYCHN_NUM][MI_VIF_MAX_CHN_OUTPORT];
#define VENC_WRITE_FRAMES (500) //Set 0 to skip write
static const char OUT_PATH[] = "/tmp/";
//static int _argc = -1;
static int _argChn = 0;
//static const char** _argv = NULL;
MI_VENC_ModType_e _eModType = E_MI_VENC_MODTYPE_MAX;

#define SIMPLE_OUTPUT (1)
#define MHE_CORE (0)
#define VENC_ENC (0)
#define VENC_ENC_v2 (1)

struct venc_channel_state _astChn[MAX_VENC_CHANNEL];


//Common state for many different CODEC
struct venc_state
{
    struct rate_ctl
    {
        MI_U32 u32Gop;
        MI_U8 u8QpI;
        MI_U8 u8QpP;
    } rc;
} venc_state =
{
    .rc = { .u32Gop = 25, .u8QpI = 20, .u8QpP = 20}
};

//Set up common video encoder rate control parameters
int setup_venc_fixed_rc(MI_U32 u32Gop, MI_U8 u8QpI, MI_U8 u8QpP)
{
    venc_state.rc.u32Gop = u32Gop;
    venc_state.rc.u8QpI = u8QpI;
    venc_state.rc.u8QpP = u8QpP;
    return 0;
}

MI_S8 map_core_id(int iCh, MI_VENC_ModType_e eModType)
{
    switch (eModType) {
    case E_MI_VENC_MODTYPE_H265E:
    case E_MI_VENC_MODTYPE_H264E:
        return iCh & 1;
        break;
    case E_MI_VENC_MODTYPE_JPEGE:
        return 0;
        break;
    default:
        break;
    }
    DBG_ERR("Unsupported core map: ch:%d, mod:%d\n", iCh, (int) eModType);
    return -1;
}


//=============================================================================
//
//  Functions for Test cases
//
//=============================================================================
MI_VENC_ModType_e get_mod_from_argv(const char *szCodec)
{
    if(/*_argc > 0 && */szCodec)
    {
        if((strcmp("h264", szCodec) == 0) || (strcmp("264", szCodec) == 0))
            return E_MI_VENC_MODTYPE_H264E;
        if((strcmp("h265", szCodec) == 0) || (strcmp("265", szCodec) == 0))
            return E_MI_VENC_MODTYPE_H265E;
        if((strcmp("jpeg", szCodec) == 0) || (strcmp("jpe", szCodec) == 0))
            return E_MI_VENC_MODTYPE_JPEGE;
    }
    DBG_ERR("Module argument is not set.\n");
    return E_MI_VENC_MODTYPE_MAX;
}

int set_extra_argv(int argc, const char **argv)
{
    if(argc == 4)
    {
        printf("Got argv: %s %s\n", argv[2], argv[3]);
        //_argc = argc - 2;
        //_argv = argv + 2;
        sscanf(argv[2], "%d", &_argChn);
        _eModType = get_mod_from_argv(argv[3]);
        return 0;
    }
    DBG_INFO("\nUseage: %s TEST_CASE CHANNEL CODEC\n"
            "\tTEST_CASE 20-29 are for VENC test\n"
            "\tCAHNNEL are number of channels to be tested\n"
            "\tCODEC available: h264 h265 jpeg (or 264 265 jpg)\n",
            argv[0]);
    return -2;
}

//s8Core: <0: undefined,   0: core 0,   1: core 1,  >1: don't care
//sync with _MI_VENC_ChooseIP
MI_S32 get_venc_dev_id(MI_VENC_ModType_e eModType, MI_S8 s8Core)
{
    MI_S32 s32DevId = -1;

    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H265E:
            if(s8Core == 0)
                s32DevId = 0;//E_MI_VENC_DEV_MHE0

            if(s8Core == 1)
                s32DevId = 1;//E_MI_VENC_DEV_MHE1
            break;
        case E_MI_VENC_MODTYPE_H264E:
            if(s8Core == 0)
                s32DevId = 2;//E_MI_VENC_DEV_MFE0
            if(s8Core == 1)
                s32DevId = 3;//E_MI_VENC_DEV_MFE1
            break;
        case E_MI_VENC_MODTYPE_JPEGE:
            if(s8Core == 0)
                s32DevId = 4;//E_MI_VENC_DEV_MHE0
            break;
        default:
            break;
    }

    return s32DevId;
}

//int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_S8 s8Core, MI_U32 u32Width,
//                        MI_U32 u32Height)
int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_U32 u32Width,
                        MI_U32 u32Height, VENC_Rc_t *pstVencRc)
{
    MI_VENC_ChnAttr_t stChannelVencAttr;
    MI_SYS_ChnPort_t stSysChnOutPort;
    const MI_U32 u32QueueLen = 40;
    MI_S32 s32Ret;// = E_MI_ERR_FAILED;
    MI_S32 s32DevId;
    MI_S8 s8Core;

    if(NULL == pstVencRc)
    {
        return -1;
    }
    printf("CH%2d %dx%d mod:%d, rc:%d\n", VencChannel, u32Width, u32Height, eModType, pstVencRc->eRcMode);
    memset(&stChannelVencAttr, 0, sizeof(stChannelVencAttr));

    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
            break;

        case E_MI_VENC_MODTYPE_H265E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
            break;

        case E_MI_VENC_MODTYPE_JPEGE:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
            break;

        default:
            break;
    }

    stChannelVencAttr.stRcAttr.eRcMode = pstVencRc->eRcMode;
    switch(pstVencRc->eRcMode)
    {
        case E_MI_VENC_RC_MODE_H264FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H265FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H264CBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265CBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H264VBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265VBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32Gop = venc_state.rc.u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_MJPEGFIXQP:
            if(eModType != E_MI_VENC_MODTYPE_JPEGE)
            {
                DBG_ERR("JPEG RC is set but module is %d\n", eModType);
                return -1;
            }
            break;
        default:
            DBG_ERR("Unknown RC type:%d\n", pstVencRc->eRcMode);
            return -3;
            break;
    }

    ExecFunc(MI_VENC_CreateChn(VencChannel, &stChannelVencAttr), MI_SUCCESS);

    //Set port
    s8Core = map_core_id(VencChannel, eModType);
    s32DevId = get_venc_dev_id(eModType, s8Core);

    if(s32DevId >= 0)
    {
        _astChn[VencChannel].u32DevId = s32DevId;
        stSysChnOutPort.u32DevId = s32DevId;
    }
    else
    {
        DBG_INFO("[ERR] Can not find device ID %X\n", s32DevId);
        return -2;
    }

    stSysChnOutPort.eModId = E_MI_MODULE_ID_VENC;
    stSysChnOutPort.u32ChnId = VencChannel;
    stSysChnOutPort.u32PortId = 0;//port is always 0 in VENC
    //This was set to (5, 10) and might be too big for kernel
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stSysChnOutPort, 5, u32QueueLen);

    if(s32Ret != 0)
    {
        DBG_ERR("Unable to set output port depth %X\n", s32Ret);
        return -3;
    }

    ExecFunc(MI_VENC_StartRecvPic(VencChannel), MI_SUCCESS);
    //usleep(3 * 1000 * 1000);//wait for all channels are started

    return 0;
}


int destroy_venc_channel(MI_VENC_CHN VencChannel)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VENC_StopRecvPic(VencChannel), MI_SUCCESS);
    //printf("sleeping...");
    //usleep(2 * 1000 * 1000);//wait for stop channel
    //printf("sleep done\n");

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VENC_DestroyChn(VencChannel), MI_SUCCESS);

    return 0;
}

void print_es(char* title, void* buf, int iCh, int max_bytes)
{
    //char *data = (char *) buf;
#if SIMPLE_OUTPUT
    (void)title;//unused parameter
    (void)iCh;//unused parameter
    (void)max_bytes;//unused parameter
    (void)buf;//unused parameter
    return;
    //printf("^^^^ User Get %5d Bytes:%02X %02X %02X %02X %02X %s\n", max_bytes,
    //       data[0], data[1], data[2], data[3], data[4], (data[4] == 0x67 || data[4] == 0x40) ? " I" : "");
#else
    int i;

    printf("%s\nCH%02d Offset(h) \n"
            "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
            "-----------------------------------------------", title, iCh);
    for (i = 0; i < 32; i++)
    {
        if (i % 16 == 0)
        {
            printf("\n");
        }
        printf("%02X ", data[i]);
    }
    printf("\n");
#endif
}

void *venc_channel_func(void* arg)
{
    MI_U32 u32Arg = (MI_U32)arg;
    MI_VENC_CHN VencChannel;
    MI_U32 VencDevId;
    MI_U32 VencPortId;
    MI_SYS_ChnPort_t stVencChn0OutputPort0;
    const unsigned int VENC_DUMP_FRAMES = VENC_WRITE_FRAMES;
    int fd = -1;
    int fdJpg;// = -1;

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret;
    //MI_BOOL bErr;
    MI_BOOL bSaveEach = FALSE;
    char szOutputPath[128];
    MI_U32 u32Seq = 0;
    VENC_FPS_t stFps = { .bRestart = TRUE, .u32TotalBits = 0, .u32DiffUs = 0, .u32TotalBits = 0 };
    //char *pData = NULL;

    VencChannel = u32Arg >> 8;
    VencDevId = _astChn[VencChannel].u32DevId;
    VencPortId = u32Arg & 0xFF;
    DBG_INFO("Start of Thread %d Getting.\n", VencChannel);

    memset(&stVencChn0OutputPort0, 0x0, sizeof(MI_SYS_ChnPort_t));
    stVencChn0OutputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0OutputPort0.u32DevId = VencDevId;
    stVencChn0OutputPort0.u32ChnId = VencChannel;
    stVencChn0OutputPort0.u32PortId = VencPortId;

    if(stVencChn0OutputPort0.u32DevId == E_VENC_DEV_JPGE/*2*/)
    {
        bSaveEach = TRUE;
    }
    if(VENC_DUMP_FRAMES)
    {
        snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%s/enc_d%dc%02d.es",
            OUT_PATH,
            VencDevId, VencChannel);
        fd = open(szOutputPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        printf("open %d %s\n", fd, szOutputPath);

        if(fd < 0)
        {
            printf("unable to open es\r\n");
        }
    }

    while(!venc_channel_port_stop[VencChannel][VencPortId])
    {
        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChn0OutputPort0, &stBufInfo, &hHandle)))
        {
            //DBG_INFO("[INFO] Get  %d bytes output\n", stBufInfo.stRawData.u32ContentSize);
            if(hHandle == NULL)
            {
                DBG_INFO("ch%02d NULL output port buffer handle.\n", VencChannel);
            }
            else if(stBufInfo.stRawData.pVirAddr == NULL)
            {
                DBG_INFO("ch%02d But unable to read buffer. VA==0\n", VencChannel);
            }
            else if(stBufInfo.stRawData.u32ContentSize >= stBufInfo.stRawData.u32BufSize)  //MAX_OUTPUT_ES_SIZE in KO
            {
                DBG_INFO("ch%02d buffer overflow %d >= %d\n", VencChannel, stBufInfo.stRawData.u32ContentSize,
                        stBufInfo.stRawData.u32BufSize);
            }
            else
            {
                //flush is should not be needed.
                //MI_SYS_FlushInvCache(stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
                if(stBufInfo.stRawData.u32ContentSize > 0 && stBufInfo.stRawData.u32ContentSize < 10)
                {
                    DBG_INFO("data:%s\n", (char*)stBufInfo.stRawData.pVirAddr);
                }
                else
                {
                    print_es("[USER SPACE]", stBufInfo.stRawData.pVirAddr, VencChannel, stBufInfo.stRawData.u32ContentSize);
                }

                if(stFps.bRestart)
                {
                    stFps.bRestart = FALSE;
                    stFps.u32TotalBits = 0;
                    stFps.u32FrameCnt = 0;
                    gettimeofday(&stFps.stTimeStart, &stFps.stTimeZone);
                }
                gettimeofday(&stFps.stTimeEnd, &stFps.stTimeZone);
                stFps.u32DiffUs = TIMEVAL_US_DIFF(stFps.stTimeStart, stFps.stTimeEnd);
                stFps.u32TotalBits += stBufInfo.stRawData.u32ContentSize * 8;
                stFps.u32FrameCnt++;
                if (stFps.u32DiffUs > 1000000 && stFps.u32FrameCnt > 1)
                {
                    printf("<%5ld.%06ld> CH %02d get %.2f fps, %d kbps\n",
                            stFps.stTimeEnd.tv_sec, stFps.stTimeEnd.tv_usec,
                            VencChannel, (float) (stFps.u32FrameCnt-1) * 1000000.0f / stFps.u32DiffUs,
                            stFps.u32TotalBits * 1000 / stFps.u32DiffUs);
                    //printf("VENC bps: %d kbps\n", nBitrateCalcTotalBit * 1000 / nBitrateCalcUsDiff);
                    stFps.bRestart = TRUE;
                }

                if((VENC_DUMP_FRAMES > 0) && (fd > 0))
                {
                    ssize_t ssize;
                    ssize = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
                    if(ssize < 0)
                    {
                        DBG_INFO("\n==== file too big ===\n\n");
                        close(fd);
                        fd = 0;
                    }
                }

                if(bSaveEach && u32Seq < VENC_DUMP_FRAMES)
                {
                    snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%sd%dc%02d_%03d.jpg",
                        OUT_PATH,
                        VencDevId, VencChannel, u32Seq);
                    fdJpg = open(szOutputPath,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    printf("open %d %s\n", fdJpg, szOutputPath);
                    if(fdJpg < 0)
                    {
                        printf("unable to open jpeg\r\n");
                        bSaveEach = FALSE;
                    }
                    else
                    {
                        ssize_t ssize;
                        ssize = write(fdJpg, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
                        if(ssize < 0)
                        {
                            printf("\n==== file too big ===\n\n");
                        }
                        close(fdJpg);
                        fdJpg = 0;
                    }
                }
                u32Seq++;
            }

            MI_SYS_ChnOutputPortPutBuf(hHandle);

            if(VENC_DUMP_FRAMES && fd > 0 && u32Seq >= VENC_DUMP_FRAMES)
            {
                DBG_INFO("\n==== ch%2d %d frames wr done\n\n", VencChannel, u32Seq);
                close(fd);
                fd = 0;
            }

            if(stBufInfo.bEndOfStream)
            {
                DBG_INFO("\n==== EOS ====\n\n");
                break;
            }
        }
        else
        {
            if((((MI_U32)s32Ret) & 0xFF) == E_MI_ERR_NOBUF)//0x0D://E_MI_ERR_NOBUF
            {
                //DBG_INFO("malloc buf fail for CH%d output port%d\n", VencChannel, VencPortId);
            }
            usleep(1 * 1000);//sleep 1 ms
        }
    }

    if(VENC_DUMP_FRAMES && fd > 0)
    {
        close(fd);
        //fd = 0;//not used
    }

    MI_SYS_SetChnOutputPortDepth(&stVencChn0OutputPort0, 0, 3);
    DBG_INFO("Thread Getting %02d exited.\n", VencChannel);

    return NULL;
}

int start_venc_channel(MI_VENC_CHN VencChn)
{
    pthread_t stThread;
    MI_U32 VencPortId = 0;

    DBG_INFO("VencChn = %d VencPort = %d\n", VencChn, VencPortId);
    venc_channel_port_stop[VencChn][VencPortId] = FALSE;
    pthread_create(&stThread, NULL, venc_channel_func, (void*)((VencChn << 8) | VencPortId));
    venc_channel_port_tid[VencChn][VencPortId] = stThread;
    return 0;
}

int stop_venc_channel(MI_VENC_CHN VencChn)
{
    //DBG_ENTER();
    venc_channel_port_stop[VencChn][0] = TRUE;
    return pthread_join(venc_channel_port_tid[VencChn][0], NULL);
}

//=============================================================================
//
//  Test cases
//
//=============================================================================

#define PRINT_CRASH_WARNING DBG_INFO("This test case is expected to be unstoppable or crash.\n")
#define ENABLE_VENC (0) //disable this section for DZ checking, and create_vif_channel has been changed.

int init_venc_sensor(MI_VIF_DEV u32VifDev, MI_U32 u32SensorMode, MI_SYS_WindowRect_t *pstVpeInWin, MI_VPE_CHANNEL *pu32VpeChn)
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_VIF_CHN u32VifChn;
    //MI_VPE_CHANNEL u32VpeChn;

    //MI_SYS_WindowRect_t stVpeInWin = {0};
    if(NULL==pstVpeInWin)
        return -1;
    ExecFunc(create_vif_dev(u32VifDev, u32SensorMode), 0);

    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    *pu32VpeChn = u32VifChn;
    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("===> vi:%d vpe:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, *pu32VpeChn, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);

    pstVpeInWin->u16Width = ALIGN_UP(u32Width, 32);
    pstVpeInWin->u16Height = u32Height;
    ExecFunc(create_vpe_channel(*pu32VpeChn, pstVpeInWin), 0);
    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn,   0, u32FrameRate,
                         E_MI_MODULE_ID_VPE, 0, *pu32VpeChn, 0, u32FrameRate), 0);

    return MI_SUCCESS;
}

int config_vpe_outport_depth(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstDispWin,
                             MI_SYS_PixelFormat_e ePixelFormat, MI_U32 u32UserBuffers, MI_U32 u32KernelBuffers)
{
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_ChnPort_t stVpeChnOutputPort;

    memset(&stVpeMode, 0, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    stVpeMode.ePixelFormat = ePixelFormat;
    stVpeMode.u16Width = pstDispWin->u16Width;
    stVpeMode.u16Height = pstDispWin->u16Height;
    ExecFunc(MI_VPE_SetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);

    //ExecFunc(MI_VPE_GetPortMode(VpeChannel, VpePort, &stVpeMode), MI_VPE_OK);
    //printf("%s %d %d\n",__FUNCTION__,__LINE__,stVpeMode.ePixelFormat);

    stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChnOutputPort.u32DevId = 0;
    stVpeChnOutputPort.u32ChnId = VpeChannel;
    stVpeChnOutputPort.u32PortId = VpePort;
    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, u32UserBuffers, u32KernelBuffers), 0);

    ExecFunc(MI_VPE_EnablePort(VpeChannel, VpePort), MI_VPE_OK);
    return MI_VPE_OK;
}

#define DEF_QP (45)
#define RcType "Cbr" //Cbr | Vbr | FixQp : case-sensitive
#define Bitrate (1000000)
#define VbrMinQp (20)
#define VbrMaxQp (40)
#define VENC_CH00_QP (20)
#define AbrMaxBitrate (500000)
int venc_init_rc_config(int iChn, MI_VENC_ModType_e eModType, VENC_Rc_t *pstRc)
{
    MI_BOOL bErr = FALSE;
    char *szRcType;

    if(pstRc == NULL)
        return -1;

    memset(pstRc, 0, sizeof(VENC_Rc_t));

    szRcType = RcType;
    pstRc->u32SrcFrmRate = 25;

    if(strcmp("Cbr", szRcType) == 0)
    {
        pstRc->u32Bitrate = Bitrate;
        ExecFunc(bErr, FALSE);
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            break;
        default:
            return -2;
            break;
        }
    }
    else if(strcmp("Vbr", szRcType) == 0)
    {
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264VBR;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            break;
        default:
            return -2;
            break;
        }
        pstRc->u32Bitrate = Bitrate;
        ExecFunc(bErr, FALSE);
        pstRc->u32VbrMinQp = VbrMinQp;
        ExecFunc(bErr, FALSE);
        pstRc->u32VbrMaxQp = VbrMaxQp;
        ExecFunc(bErr, FALSE);
    }
    else if(strcmp("FixQp", szRcType) == 0)
    {
        switch (eModType) {
        case E_MI_VENC_MODTYPE_H264E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            break;
        case E_MI_VENC_MODTYPE_H265E:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            break;
        case E_MI_VENC_MODTYPE_JPEGE:
            pstRc->eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            break;
        default:
            return -2;
            break;
        }
        pstRc->u32FixQp = VENC_CH00_QP + iChn;
    }
    else if(strcmp("Abr", szRcType) == 0)
    {
        pstRc->u32AvrMaxBitrate = AbrMaxBitrate;
        ExecFunc(bErr, FALSE);
    }
    else
    {
        DBG_ERR("Unknown rcType %s.\n", szRcType);
        return -3;
    }

    return MI_SUCCESS;
}

//verification: 10-JAN-2018 works
int test_fhd_n_venc(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC_v2
    MI_SYS_WindowRect_t stVpeWin = { 0 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, 1920, 1088 };
    MI_VPE_CHANNEL VpeChn;
    int iCh;
    MI_U32 u32VpeOutPort = 0;

    DBG_INFO("\n\n"
             "1-N channels VIF->VPE->VENC, 1 sensor\n"
             "\tVIF 4,0--VPE 4,0--VENC 0\n"
             "\t                `-VENC 1\n"
             "\t                `-VENC 2...\n"
            );
    PRINT_CRASH_WARNING;
    if (nCh < 1 || nCh > 10)
    {
        DBG_ERR("The number of channel must be 1-10 now.\n");
        return -1;
    }
    printf("%d channels test\n", nCh);
    ExecFunc(init_venc_sensor(1, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn), MI_SUCCESS);
    ExecFunc(config_vpe_outport(VpeChn, u32VpeOutPort, &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
#if 0
    {
        MI_SYS_ChnPort_t stVpeChnOutputPort;
        stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort.u32DevId = 0;
        stVpeChnOutputPort.u32ChnId = VpeChn;
        stVpeChnOutputPort.u32PortId = u32VpeOutPort;
        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, 0, 3), 0);
        //stVpeChnOutputPort.eModId = E_MI_MODULE_ID_VIF;
        //ExecFunc(MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort, 0, 3), 0);
    }
#endif

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        VENC_Rc_t stRc;
        ExecFunc(venc_init_rc_config(iCh, eModType, &stRc), MI_SUCCESS);
        ExecFunc(create_venc_channel(eModType, iCh, stEncWin.u16Width, stEncWin.u16Height, &stRc), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(VpeChn,0,  iCh), MI_SUCCESS);
        //ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
    ExecFunc(start_vpe_channel(VpeChn, 0), 0);
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}

#if 0
int test_fhd_2_vif_n_venc(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC_v2
    MI_SYS_WindowRect_t stVpeWin = { 0 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, 1920, 1088 };
    MI_VPE_CHANNEL VpeChn[2] = {0};
    int iCh;
    MI_U32 u32VpeOutPort[4] = {0, 2, 0, 2};
    int iUsedChn;
    const int SPEC_CHN = 16;
    //const int VPE_DEPTH = SPEC_CHN / 4 * 3;// (/4) max 4 vpe port, (*3) 3 times of frames

    DBG_INFO("\n\n"
             "1-N channels VIF->VPE->VENC, 2 sensor\n"
             "\tVIF 4,0--VPE 4,0x-VENC 0\n"
             "\t                x-VENC 1\n"
             "\tVIF 8,0--VPE 8,0x-VENC 2...\n"
            );
    PRINT_CRASH_WARNING;
    if(nCh < 1 || nCh > SPEC_CHN * 2)
    {
        DBG_ERR("The number of channel must be 1-%d now.\n", SPEC_CHN * 2);
        return -1;
    }
    if(nCh > SPEC_CHN)
    {
        DBG_ERR("%d channels are OVERSPEC.\n", nCh);
    }
    printf("%d channels test\n", nCh);
    ExecFunc(init_venc_sensor(1, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[0]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[0], VpeChn[0]);
    ExecFunc(config_vpe_outport(VpeChn[0], u32VpeOutPort[0], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(VpeChn[0], u32VpeOutPort[1], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(init_venc_sensor(2, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[1]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[1], VpeChn[1]);
    ExecFunc(config_vpe_outport(VpeChn[1], u32VpeOutPort[0], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(VpeChn[1], u32VpeOutPort[1], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWin.u16Width, stEncWin.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(VpeChn[(iCh%4)/2],u32VpeOutPort[(iCh%4)%2],  iCh), MI_SUCCESS);
        printf("VPE%dp%d-VENC%d\n", VpeChn[(iCh%4)/2], u32VpeOutPort[(iCh%4)%2], iCh);
        //ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
    iUsedChn = ((nCh-1)%4)/2;
    for (iCh = 0; iCh <= iUsedChn; ++iCh)
    {
        ExecFunc(start_vpe_channel(VpeChn[iCh],u32VpeOutPort[(iCh%4)%2]), 0);
    }
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}

int test_hd_2_vif_n_venc(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC_v2
    MI_SYS_WindowRect_t stVpeWin = { 0 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, 1280, 720 };
    MI_VPE_CHANNEL VpeChn[2] = {0};
    int iCh;
    MI_U32 u32VpeOutPort[4] = {0, 2, 0, 2};
    int iUsedChn;

    DBG_INFO("\n\n"
             "1-N channels VIF->VPE->VENC, 2 sensor\n"
             "\tVIF 4,0--VPE 4,0 x VENC 0\n"
             "\t                 x VENC 1\n"
             "\tVIF 8,0--VPE 8,0 x VENC 2...\n"
            );
    PRINT_CRASH_WARNING;
    if(nCh < 1 || nCh > 10)
    {
        DBG_ERR("The number of channel must be 1-10 now.\n");
        return -1;
    }
    if(nCh>8)
    {
        DBG_ERR("%d is OVERSPEC\n", nCh);
    }
    printf("%d channels test\n", nCh);
    ExecFunc(init_venc_sensor(1, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[0]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[0], VpeChn[0]);
    ExecFunc(config_vpe_outport(VpeChn[0], u32VpeOutPort[0], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(VpeChn[0], u32VpeOutPort[1], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(init_venc_sensor(2, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[1]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[1], VpeChn[1]);
    ExecFunc(config_vpe_outport(VpeChn[1], u32VpeOutPort[0], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(VpeChn[1], u32VpeOutPort[1], &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWin.u16Width, stEncWin.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(VpeChn[(iCh%4)/2],u32VpeOutPort[(iCh%4)%2],  iCh), MI_SUCCESS);
        printf("VPE%dp%d-VENC%d\n", VpeChn[(iCh%4)/2], u32VpeOutPort[(iCh%4)%2], iCh);
        //ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
    iUsedChn = ((nCh-1)%4)/2;
    for (iCh = 0; iCh <= iUsedChn; ++iCh)
    {
        ExecFunc(start_vpe_channel(VpeChn[iCh],u32VpeOutPort[(iCh%4)%2]), 0);
    }
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}
#endif

int test_2_vif_n_venc(int nCh, MI_VENC_ModType_e eModType, MI_U32 u32W, MI_U32 u32H, MI_U32 u32MaxChn)
{
#if VENC_ENC_v2
    MI_SYS_WindowRect_t stVpeWin = { 0 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, u32W, u32H };
    MI_VPE_CHANNEL VpeChn[2] = {0};
    int iCh;
    MI_U32 u32VpeOutPort[4] = {0, 2, 0, 2};
    int iUsedChn;
    const int SPEC_CHN = u32MaxChn;
    const int VPE_DEPTH = SPEC_CHN / 4 * 3;// (/4) max 4 vpe port, (*3) 3 times of frames
    MI_U32 u32MaxChnForTest;

    DBG_INFO("\n\n"
             "1-%d channels VIF->VPE->VENC, 2 sensor\n"
             "\tVIF 4,0--VPE 4,0 x VENC 0\n"
             "\t                 x VENC 1\n"
             "\tVIF 8,0--VPE 8,0 x VENC 2...\n",
            SPEC_CHN);
    printf("%d channels %dx%d test\n", nCh, u32W, u32H);
    PRINT_CRASH_WARNING;
    u32MaxChnForTest = SPEC_CHN * 2;
    if(u32MaxChnForTest > MI_VIF_MAX_PHYCHN_NUM)
        u32MaxChnForTest = MI_VIF_MAX_PHYCHN_NUM;
    if(nCh < 1 || nCh > u32MaxChnForTest)
    {
        DBG_ERR("The number of channel must be 1-%d now.\n", u32MaxChnForTest);
        return -1;
    }
    if(nCh > SPEC_CHN)
    {
        DBG_ERR("%d channels are OVERSPEC.\n", nCh);
    }
    ExecFunc(init_venc_sensor(1, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[0]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[0], VpeChn[0]);
    ExecFunc(config_vpe_outport_depth(VpeChn[0], u32VpeOutPort[0],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(config_vpe_outport_depth(VpeChn[0], u32VpeOutPort[1],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(init_venc_sensor(2, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[1]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[1], VpeChn[1]);
    ExecFunc(config_vpe_outport_depth(VpeChn[1], u32VpeOutPort[0],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(config_vpe_outport_depth(VpeChn[1], u32VpeOutPort[1], &stEncWin,
            E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        VENC_Rc_t stRc;
        ExecFunc(venc_init_rc_config(iCh, eModType, &stRc), MI_SUCCESS);
        ExecFunc(create_venc_channel(eModType, iCh, stEncWin.u16Width, stEncWin.u16Height, &stRc), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(VpeChn[(iCh%4)/2],u32VpeOutPort[(iCh%4)%2],  iCh), MI_SUCCESS);
        printf("VPE%dp%d-VENC%d\n", VpeChn[(iCh%4)/2], u32VpeOutPort[(iCh%4)%2], iCh);
        //ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
    iUsedChn = ((nCh-1)%4)/2;
    for (iCh = 0; iCh <= iUsedChn; ++iCh)
    {
        ExecFunc(start_vpe_channel(VpeChn[iCh],u32VpeOutPort[(iCh%4)%2]), 0);
    }
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}
#if 0
int test_d1_2_vif_n_venc(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC_v2
    MI_SYS_WindowRect_t stVpeWin = { 0 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, 736, 480 };
    MI_VPE_CHANNEL VpeChn[2] = {0};
    int iCh;
    MI_U32 u32VpeOutPort[4] = {0, 2, 0, 2};
    int iUsedChn;
    const int SPEC_CHN = 16;
    const int VPE_DEPTH = SPEC_CHN / 4 * 3;// (/4) max 4 vpe port, (*3) 3 times of frames

    DBG_INFO("\n\n"
             "1-N channels VIF->VPE->VENC, 2 sensor\n"
             "\tVIF 4,0--VPE 4,0 x VENC 0\n"
             "\t                 x VENC 1\n"
             "\tVIF 8,0--VPE 8,0 x VENC 2...\n"
            );
    PRINT_CRASH_WARNING;
    if(nCh < 1 || nCh > SPEC_CHN * 2)
    {
        DBG_ERR("The number of channel must be 1-%d now.\n", SPEC_CHN * 2);
        return -1;
    }
    if(nCh > SPEC_CHN)
    {
        DBG_ERR("%d channels are OVERSPEC.\n", nCh);
    }
    printf("%d channels test\n", nCh);
    ExecFunc(init_venc_sensor(1, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[0]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[0], VpeChn[0]);
    ExecFunc(config_vpe_outport_depth(VpeChn[0], u32VpeOutPort[0],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(config_vpe_outport_depth(VpeChn[0], u32VpeOutPort[1],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(init_venc_sensor(2, SAMPLE_VI_MODE_1_1080P, &stVpeWin, &VpeChn[1]), MI_SUCCESS);
    printf("VIF%d-VPE%d\n", VpeChn[1], VpeChn[1]);
    ExecFunc(config_vpe_outport_depth(VpeChn[1], u32VpeOutPort[0],
            &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);
    ExecFunc(config_vpe_outport_depth(VpeChn[1], u32VpeOutPort[1], &stEncWin,
            E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, 0, VPE_DEPTH), MI_SUCCESS);

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWin.u16Width, stEncWin.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(VpeChn[(iCh%4)/2],u32VpeOutPort[(iCh%4)%2],  iCh), MI_SUCCESS);
        printf("VPE%dp%d-VENC%d\n", VpeChn[(iCh%4)/2], u32VpeOutPort[(iCh%4)%2], iCh);
        //ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
    iUsedChn = ((nCh-1)%4)/2;
    for (iCh = 0; iCh <= iUsedChn; ++iCh)
    {
        ExecFunc(start_vpe_channel(VpeChn[iCh],u32VpeOutPort[(iCh%4)%2]), 0);
    }
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}
#endif
//one channel VIF->VPE->VENC
int start_test_20(void)
{
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin = {0, 0, 480, 288};

    DBG_INFO("\n========= start_test_20 ========\n"
             "1 channel VIF->VPE->VENC\n"
             "\tVIF 0,0--VPE 0,0--VENC.264 0,0\n");
    PRINT_CRASH_WARNING;

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode,
            E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, eFrameRate), 0)
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
             E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
    ExecFunc(create_vpe_channel(0,  &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), 0);
    //ExecFunc(create_h264_channel(0, 0, stEncWin.u16Width, stEncWin.u16Height), 0);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H264E, 0, 0, stEncWin.u16Width, stEncWin.u16Height), 0);

    ExecFunc(bind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(bind_vpe_venc(0, 0, 0), 0);
    ExecFunc(start_vpe_channel(0, 0), 0);
    ExecFunc(start_venc_channel(0), 0);
#endif
    return 0;
}

int stop_test_20(void)
{
    DBG_INFO("========= stop_test_20 ========\n");
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), 0);
    ExecFunc(stop_vpe_channel(0, 0), 0);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(unbind_vpe_venc(0, 0, 0), 0);
    ExecFunc(destroy_vif_channel(0, 0), 0);
    ExecFunc(destroy_vpe_channel(0, 0), 0);
    ExecFunc(destroy_venc_channel(0), 0);
#endif
    return 0;
}


//2 channels VIF->VPE->VENC, 2 sensors
/*   VIF 0,0--VPE 0,0--VENC 0,0
 *   VIF 1,0--VPE 1,0--VENC 1,0          */
int start_test_21(void)
{
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160, 96};

    DBG_INFO("\n========= start_test_21 ========\n"
             "2 channels VIF->VPE->VENC, 2 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.264 0,0\n"
             "\tVIF 1,0--VPE 1,0--VENC.264 1,0\n");
    PRINT_CRASH_WARNING;

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);

    ExecFunc(create_vpe_channel(0, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    //ExecFunc(create_h264_channel(0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H264E, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);

    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    ExecFunc(start_venc_channel(0), MI_SUCCESS);

    ExecFunc(create_vif_dev(1, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(1, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(1, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(1, 0, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    //ExecFunc(create_h264_channel(1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H264E, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(1,0,  1,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(1,0,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(1, 0), 0);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int stop_test_21(void)
{
    DBG_INFO("========= stop_test_21 ========\n");
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);

    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(1, 0, 1, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(1, 0, 1), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(1, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(1, 0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

//2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors
/*   VIF 0,0--VPE 0,0--VENC 0,0
 *          `-VPE 0,1--VENC 1,0          */
int start_test_22(void)
{
#if ENABLE_VENC
#if 1
    DBG_INFO("\n========= start_test_22 ========\n"
             "2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.264 0,0\n"
             "\t       `-VPE 0,1--VENC.264 1,0\n");
    PRINT_CRASH_WARNING;

    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160,  96};

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 1, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    //ExecFunc(create_h264_channel(0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    //ExecFunc(create_h264_channel(1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H264E, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H264E, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,1,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    printf("\nVPE started\n\n");
    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#else
    MI_SYS_WindowRect_t stCropWin = {0, 0, 720, 240};
    //MI_SYS_WindowRect_t stDispWin0 = {0, 0, 240, 144};//NG resolution, output shearing images
    //original setting 480x288, 240x144:
    //360x240 + 480x288 cause kernel crash: MI_SYS_IMPL_MmaAlloc fail
    //360x240 + 288x192, the same bug
    //MI_SYS_WindowRect_t stDispWin0 = {0, 0, 360, 240};
    MI_SYS_WindowRect_t stDispWin0 = {0, 0, 480, 288};
    //MI_SYS_WindowRect_t stDispWin1 = {0, 0, 480, 288};
    //MI_SYS_WindowRect_t stDispWin1 = {0, 0, 288, 192};
    MI_SYS_WindowRect_t stDispWin1 = {0, 0, 160, 96};
    ExecFunc(create_vif_channel(0, 0, stCropWin.u16Width, stCropWin.u16Height, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    bDelayStart = TRUE;
    ExecFunc(create_vpe_channel(0, 0, &stCropWin, &stDispWin0), MI_SUCCESS);
    ExecFunc(config_vpe_port(0, 1, &stDispWin1), MI_SUCCESS);
    bDelayStart = FALSE;
    ExecFunc(create_h264_channel(0, 0, stDispWin0.u16Width, stDispWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_h264_channel(1, 0, stDispWin1.u16Width, stDispWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0, 0, 0), MI_SUCCESS); //display could only use 3
    ExecFunc(bind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(MI_VPE_StartChannel(0), MI_VPE_OK);
    printf("\nVPE started\n\n");
    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
#endif
    return 0;
}

int stop_test_22(void)
{
    DBG_INFO("========= stop_test_22 ========\n");
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(MI_VPE_DisablePort(0, 1), MI_VPE_OK);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}


int test_fhd_1_chn(MI_VENC_ModType_e eModType)
{
    MI_SYS_WindowRect_t stEncWin = {0, 0, 1920, 1088};

    MI_VIF_DEV u32VifDev = 0;
    MI_VIF_CHN u32VifChn;
    MI_VPE_CHANNEL u32VpeChn;
    MI_VENC_CHN    u32VencChn;
    //MI_U32 bDump = 0;

    DBG_ENTER();

    u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    u32VpeChn = u32VifChn;
    u32VencChn = u32VpeChn;

    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};
    VENC_Rc_t stRc;

    DBG_INFO("\n========= start_test_23 ========\n"
             "1 channel VIF->VPE->VENC\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n");

    ExecFunc(create_vif_dev(u32VifDev, SAMPLE_VI_MODE_1_1080P), 0);

    ExecFunc(vif_get_vid_fmt(u32VifChn, &u32Width, &u32Height, &u32FrameRate, &eScanMode), 0);
    DBG_INFO("===> vi:%d vpe:%d venc:%d w:%u h:%u fps:%u scan:%u\n", u32VifChn, u32VpeChn, u32VencChn, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(u32VifChn, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);


    stVpeInWin.u16Width = ALIGN_UP(u32Width, 32);
    stVpeInWin.u16Height = u32Height;
    ExecFunc(create_vpe_channel(u32VpeChn,  &stVpeInWin), 0);

    stVpeOutWin.u16Width  = 1920;
    stVpeOutWin.u16Height = 1080;
    ExecFunc(config_vpe_outport(u32VpeChn, 0, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), 0);

    ExecFunc(venc_init_rc_config(u32VencChn, eModType, &stRc), MI_SUCCESS);
    ExecFunc(create_venc_channel(eModType, u32VencChn, stEncWin.u16Width, stEncWin.u16Height, &stRc), 0);

    u32FrameRate = vif_cal_fps(u32FrameRate, E_MI_VIF_FRAMERATE_FULL);

    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, u32VifChn, 0, 50,
                         E_MI_MODULE_ID_VPE, 0, u32VpeChn, 0, 50), 0);

    ExecFunc(bind_vpe_venc(u32VpeChn, 0, u32VencChn), 0);

    ExecFunc(start_vpe_channel(u32VpeChn, 0), 0);

    ExecFunc(start_venc_channel(u32VencChn), 0);

    return 0;
}

int test_FHD_VIF_VPE_VENCxN(int nCh, MI_VENC_ModType_e eModType)
{
    MI_SYS_WindowRect_t stEncWin = {0, 0, 1920, 1088};

    MI_VIF_DEV u32VifDev = 0;
    VENC_Rc_t stRc;
    //MI_VIF_CHN u32VifChn;
    //MI_VPE_CHANNEL u32VpeChn;
    //MI_VENC_CHN    u32VencChn;
    //MI_U32 bDump = 0;
    int iCh = 0;

    DBG_ENTER();

    //u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    //u32VpeChn = u32VifChn;
    //u32VencChn = u32VpeChn;

    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};

    DBG_INFO("\n========= test_FHD_VIF_VPE_VENCxN ========\n"
             "1 channel VIF->VPE->VENCxN\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n");

    for (iCh = 0; iCh < nCh; ++iCh)
    {
        if (iCh == 0)
        {
            ExecFunc(create_vif_dev(iCh, SAMPLE_VI_MODE_1_1080P), MI_SUCCESS);

            ExecFunc(vif_get_vid_fmt(iCh, &u32Width, &u32Height, &u32FrameRate, &eScanMode), MI_SUCCESS);
            DBG_INFO("===> vi:%d vpe:%d venc:%d w:%u h:%u fps:%u scan:%u\n", iCh, iCh, iCh, u32Width, u32Height, u32FrameRate, eScanMode);

            ExecFunc(create_vif_channel(iCh, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);

            stVpeInWin.u16Width = ALIGN_UP(u32Width, 32);
            stVpeInWin.u16Height = u32Height;
            ExecFunc(create_vpe_channel(iCh,  &stVpeInWin), MI_SUCCESS);
            stVpeOutWin.u16Width  = 1920;
            stVpeOutWin.u16Height = 1080;
            ExecFunc(config_vpe_outport(iCh, 0, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
        }

        ExecFunc(venc_init_rc_config(iCh, eModType, &stRc), MI_SUCCESS);
        ExecFunc(create_venc_channel(eModType, iCh, stEncWin.u16Width, stEncWin.u16Height, &stRc), MI_SUCCESS);

        u32FrameRate = vif_cal_fps(u32FrameRate, E_MI_VIF_FRAMERATE_FULL);

        if (u32VifDev == 0)
        {
            ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, 0, 0, 50,
                                 E_MI_MODULE_ID_VPE, 0, iCh, 0, 50), MI_SUCCESS);
        }

        ExecFunc(bind_vpe_venc(0, 0, iCh), MI_SUCCESS);

        if (iCh == 0)
        {
            ExecFunc(start_vpe_channel(iCh, 0), MI_SUCCESS);
        }

        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }

    return 0;
}

int test_FHD_VIF_VPE_VENCx2_1core(int nCh, MI_VENC_ModType_e eModType)
{
    MI_SYS_WindowRect_t stEncWin = {0, 0, 1920, 1088};
    VENC_Rc_t stRc;

    //MI_VIF_DEV u32VifDev = 0;
    //MI_VIF_CHN u32VifChn;
    //MI_VPE_CHANNEL u32VpeChn;
    //MI_VENC_CHN    u32VencChn;
    //MI_U32 bDump = 0;
    int iCh = 0;


    DBG_ENTER();

    //u32VifChn = MI_VIF_MAX_WAY_NUM_PER_DEV * u32VifDev;
    //u32VpeChn = u32VifChn;
    //u32VencChn = u32VpeChn;

    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_U32 u32FrameRate;
    MI_SYS_FrameScanMode_e eScanMode;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};

    DBG_INFO("\n========= test_FHD_VIF_VPE_VENCxN ========\n"
             "1 channel VIF->VPE->VENCxN\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n");


    ExecFunc(create_vif_dev(iCh, SAMPLE_VI_MODE_1_1080P), MI_SUCCESS);

    ExecFunc(vif_get_vid_fmt(iCh, &u32Width, &u32Height, &u32FrameRate, &eScanMode), MI_SUCCESS);
    DBG_INFO("===> vi:%d vpe:%d venc:%d w:%u h:%u fps:%u scan:%u\n", iCh, iCh, iCh, u32Width, u32Height, u32FrameRate, eScanMode);

    ExecFunc(create_vif_channel(iCh, 0, u32Width, u32Height, eScanMode, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);

    stVpeInWin.u16Width = ALIGN_UP(u32Width, 32);
    stVpeInWin.u16Height = u32Height;
    ExecFunc(create_vpe_channel(iCh, &stVpeInWin), MI_SUCCESS);
    stVpeOutWin.u16Width = 1920;
    stVpeOutWin.u16Height = 1080;
    ExecFunc(config_vpe_outport(iCh, 0, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);


    ExecFunc(venc_init_rc_config(0, eModType, &stRc), MI_SUCCESS);
    ExecFunc(create_venc_channel(eModType, 0, stEncWin.u16Width, stEncWin.u16Height, &stRc), MI_SUCCESS);

    ExecFunc(venc_init_rc_config(2, eModType, &stRc), MI_SUCCESS);
    ExecFunc(create_venc_channel(eModType, 2, stEncWin.u16Width, stEncWin.u16Height, &stRc), MI_SUCCESS);

    u32FrameRate = vif_cal_fps(u32FrameRate, E_MI_VIF_FRAMERATE_FULL);


    ExecFunc(bind_module(E_MI_MODULE_ID_VIF, 0, 0, 0, 50,
            E_MI_MODULE_ID_VPE, 0, iCh, 0, 50), MI_SUCCESS);


    ExecFunc(bind_vpe_venc(0, 0, 0), MI_SUCCESS);

    ExecFunc(bind_vpe_venc(0, 0, 2), MI_SUCCESS);


    ExecFunc(start_vpe_channel(iCh, 0), MI_SUCCESS);


    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(2), MI_SUCCESS);


    return 0;
}

//one channel VIF->VPE->VENC.265
int start_test_23(void)
{
    DBG_ENTER();
    //MI_VENC_ModType_e eModType = get_mod_from_argv();
    if (E_MI_VENC_MODTYPE_MAX == _eModType)
        return -1;

    ExecFunc(test_fhd_n_venc(_argChn, _eModType), MI_SUCCESS);
    //test_FHD_VIF_VPE_VENCxN(4, E_MI_VENC_MODTYPE_H265E);
    //test_FHD_VIF_VPE_VENCx2_1core(2, E_MI_VENC_MODTYPE_H265E);

#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin = {0, 0, 480, 288};

    DBG_INFO("\n========= start_test_23 ========\n"
             "1 channel VIF->VPE->VENC\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n");

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0,  &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), 0);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H265E, 0, MHE_CORE, stEncWin.u16Width, stEncWin.u16Height), 0);
    ExecFunc(bind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(bind_vpe_venc(0, 0, 0), 0);
    ExecFunc(start_vpe_channel(0, 0), 0);
    ExecFunc(start_venc_channel(0), 0);
#endif
    return 0;
}

int stop_test_23(void)
{
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), 0);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(unbind_vpe_venc(0, 0, 0), 0);
    ExecFunc(destroy_vif_channel(0, 0), 0);
    ExecFunc(destroy_vpe_channel(0, 0), 0);
    ExecFunc(destroy_venc_channel(0), 0);
#endif
    return 0;
}

//int test_2_sensor_
//2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors
/*   VIF 0,0--VPE 0,0--VENC 0,0
 *          `-VPE 0,1--VENC 1,0          */
int start_test_24(void)
{
    DBG_ENTER();
    //MI_VENC_ModType_e eModType = get_mod_from_argv();
    if (E_MI_VENC_MODTYPE_MAX == _eModType)
        return -1;

    //ExecFunc(test_fhd_2_vif_n_venc(_argChn, _eModType), MI_SUCCESS);
    ExecFunc(test_2_vif_n_venc(_argChn, _eModType, 1920, 1088, 4), MI_SUCCESS);
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = { 0, 0, 720, 240 };
    MI_SYS_WindowRect_t stVpeWin = { 0, 0, 736, 240 };
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160, 96};

    DBG_INFO("\n========= start_test_24 ========\n"
             "2 channels VIF->VPE->VENC, 2 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n"
             "\tVIF 1,0--VPE 1,0--VENC.265 1,0\n");
    PRINT_CRASH_WARNING;

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);

    ExecFunc(create_vpe_channel(0, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    //ExecFunc(create_h264_channel(0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H265E, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);

    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    ExecFunc(start_venc_channel(0), MI_SUCCESS);

    ExecFunc(create_vif_dev(1, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(1, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(1, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(1, 0, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H265E, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(1,0,  1,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(1,0,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(1, 0), 0);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int stop_test_24(void)
{

#if ENABLE_VENC
    ExecFunc(stop_vpe_channel(0, 0), 0);
    ExecFunc(stop_vpe_channel(0, 1), 0);
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(MI_VPE_DisablePort(0, 1), MI_VPE_OK);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int start_test_25(void)
{
    DBG_ENTER();
    //MI_VENC_ModType_e eModType = get_mod_from_argv();
    if (E_MI_VENC_MODTYPE_MAX == _eModType)
        return -1;

    //ExecFunc(test_hd_2_vif_n_venc(_argChn, _eModType), MI_SUCCESS);
    ExecFunc(test_2_vif_n_venc(_argChn, _eModType, 1280, 720, 8), MI_SUCCESS);

#if ENABLE_VENC
    DBG_INFO("\n========= start_test_25 ========\n"
             "2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.265 0,0\n"
             "\t       `-VPE 0,1--VENC.265 1,0\n");
    PRINT_CRASH_WARNING;

    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160,  96};

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 1, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H265E, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_H265E, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,1,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    printf("\nVPE started\n\n");
    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int stop_test_25(void)
{
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(MI_VPE_DisablePort(0, 1), MI_VPE_OK);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int start_test_26(void)
{
    DBG_ENTER();
    //MI_VENC_ModType_e eModType = get_mod_from_argv();
    if (E_MI_VENC_MODTYPE_MAX == _eModType)
        return -1;

    //ExecFunc(test_d1_2_vif_n_venc(_argChn, _eModType), MI_SUCCESS);
    ExecFunc(test_2_vif_n_venc(_argChn, _eModType, 736, 480, 16), MI_SUCCESS);
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin = {0, 0, 480, 288};

    DBG_INFO("\n========= start_test_26 ========\n"
             "1 channel VIF->VPE->VENC\n"
             "\tVIF 0,0--VPE 0,0--VENC.jpe 0,0\n");
    PRINT_CRASH_WARNING;

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), 0);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
             E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), 0);
    ExecFunc(create_vpe_channel(0,  &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), 0);
    //ExecFunc(create_h264_channel(0, 0, stEncWin.u16Width, stEncWin.u16Height), 0);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_JPEGE, 0, 0, stEncWin.u16Width, stEncWin.u16Height), 0);

    ExecFunc(bind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(bind_vpe_venc(0, 0, 0), 0);
    ExecFunc(start_vpe_channel(0, 0), 0);
    ExecFunc(start_venc_channel(0), 0);
#endif
    return 0;
}

int stop_test_26(void)
{
    DBG_INFO("========= stop_test_26 ========\n");
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), 0);
    ExecFunc(stop_vpe_channel(0, 0), 0);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), 0);
    ExecFunc(unbind_vpe_venc(0, 0, 0), 0);
    ExecFunc(destroy_vif_channel(0, 0), 0);
    ExecFunc(destroy_vpe_channel(0, 0), 0);
    ExecFunc(destroy_venc_channel(0), 0);
#endif
    return 0;
}

int start_test_27(void)
{
    DBG_INFO("\n========= start_test_27 ========\n"
             "2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.jpe 0,0\n"
             "\t       `-VPE 0,1--VENC.jpe 1,0\n");
    PRINT_CRASH_WARNING;
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160,  96};

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 1, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_JPEGE, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_JPEGE, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,1,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    printf("\nVPE started\n\n");
    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int stop_test_27(void)
{
    DBG_INFO("========= stop_test_27 ========\n");
#if ENABLE_VENC
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(MI_VPE_DisablePort(0, 1), MI_VPE_OK);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int start_test_28(void)
{
    DBG_INFO("\n========= start_test_28 ========\n"
             "2 channels VIF->1 VPE 2 ports->2 x VENC, 1 sensors\n"
             "\tVIF 0,0--VPE 0,0--VENC.jpe 0,0\n"
             "\t       `-VPE 0,1--VENC.jpe 1,0\n");
    PRINT_CRASH_WARNING;
#if ENABLE_VENC
    MI_SYS_WindowRect_t stVifWin = {0, 0, 720, 240};
    MI_SYS_WindowRect_t stVpeWin = {0, 0, 736, 240};
    MI_SYS_WindowRect_t stEncWin0 = {0, 0, 480, 288};
    MI_SYS_WindowRect_t stEncWin1 = {0, 0, 160,  96};

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_D1), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
                    E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), 0);
    ExecFunc(config_vpe_outport(0, 0, &stEncWin0, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 1, &stEncWin1, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_JPEGE, 0, 0, stEncWin0.u16Width, stEncWin0.u16Height), MI_SUCCESS);
    ExecFunc(create_venc_channel(E_MI_VENC_MODTYPE_JPEGE, 1, 0, stEncWin1.u16Width, stEncWin1.u16Height), MI_SUCCESS);
    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,0,  0), MI_SUCCESS);
    ExecFunc(bind_vpe_venc(0,1,  1), MI_SUCCESS);
    ExecFunc(start_vpe_channel(0, 0), 0);
    printf("\nVPE started\n\n");
    ExecFunc(start_venc_channel(0), MI_SUCCESS);
    ExecFunc(start_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}

int stop_test_28(void)
{
    DBG_INFO("========= stop_test_28 ========\n");
#if 0
    ExecFunc(stop_venc_channel(0), MI_SUCCESS);
    ExecFunc(stop_venc_channel(1), MI_SUCCESS);
    ExecFunc(unbind_vif_vpe(0, 0, 0, 0), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 1, 1), MI_SUCCESS);
    ExecFunc(unbind_vpe_venc(0, 0, 0), MI_SUCCESS);
    ExecFunc(destroy_vif_channel(0, 0), MI_SUCCESS);
    ExecFunc(destroy_vpe_channel(0, 0), MI_SUCCESS);
    ExecFunc(MI_VPE_DisablePort(0, 1), MI_VPE_OK);
    ExecFunc(destroy_venc_channel(0), MI_SUCCESS);
    ExecFunc(destroy_venc_channel(1), MI_SUCCESS);
#endif
    return 0;
}


//nCh number of channels
//verification: 10-JAN-2018 works but 2nd channel would have green glitches.
int test_fhd_n_vif(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC
    MI_SYS_WindowRect_t stVifWin = { 0, 0, 1920, 1080 };
    MI_SYS_WindowRect_t stVpeWin = { 0, 0, 1920, 1088 };
    MI_SYS_WindowRect_t stEncWin = { 0, 0, 1920, 1088 };
    int iCh;
    const int iVpeMul = 8;

    DBG_INFO("\n\n"
             "n channels VIF->VPE->VENC, 1-3 sensors into each daughter board.\n"
             "\tVIF 0,0--VPE 0,0--VENC 0\n"
             "\tVIF 1,0--VPE 1,0--VENC 1\n"
             "\tVIF 2,0--VPE 2,0--VENC 2\n"
            );
    PRINT_CRASH_WARNING;
    if (nCh < 1 || nCh > 3)
    {
        DBG_ERR("The number of channel must be 1-3 now.\n");
        return -1;
    }
    printf("%d channels test\n", nCh);

    for (iCh = 0; iCh < nCh; ++iCh) {
        ExecFunc(create_vif_dev(iCh, SAMPLE_VI_MODE_1_1080P), MI_SUCCESS);
        ExecFunc(create_vif_channel(iCh*4, 0, stVifWin.u16Width, stVifWin.u16Height,
                E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);

        ExecFunc(create_vpe_channel(iCh*iVpeMul, &stVpeWin), MI_SUCCESS);
        ExecFunc(config_vpe_outport(iCh*iVpeMul, 0, &stEncWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWin.u16Width, stEncWin.u16Height), MI_SUCCESS);

        ExecFunc(bind_vif_vpe(iCh*4,0,  iCh*iVpeMul,0), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(iCh,0,  iCh), MI_SUCCESS);
        ExecFunc(start_vpe_channel(iCh, 0), 0);
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}

//special arrangement for full specification.
//verification:
int test_fhd_30x4(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC
    MI_SYS_WindowRect_t stVifWin = { 0, 0, 1920, 1080 };
    MI_SYS_WindowRect_t stVpeWin = { 0, 0, 1920, 1088 };
    MI_SYS_WindowRect_t stEncWinL = { 0, 0, 1920, 1088 };
    MI_SYS_WindowRect_t stEncWinS = { 0, 0, 736, 480 };
    int iCh;

    //Let input FHDx25 FPS
    //each core would be FHD x 50 FPS + HD x 25 FPS
    //~= FHD x 61 FPS
    DBG_INFO("\n\n"
             "n channels VIF->VPE->VENC, 1 sensors\n"
             "\tVIF 0,0--VPE 0,0 FHD--FHD VENC 0\n" //core 0
             "\t       `-VPE 0,1  HD--FHD VENC 1\n"
             "\t                    `-FHD VENC 2\n" //core 0
             "\t                    `-FHD VENC 3\n"
             "\t                    `- HD VENC 4\n" //core 0
             "\t                    `- HD VENC 5\n"
            );
    PRINT_CRASH_WARNING;
    if (nCh < 1 || nCh > 3)
    {
        DBG_ERR("The number of channel must be 1-3 now.\n");
        return -1;
    }
    printf("%d channels test\n", nCh);

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_1080P), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
            E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 0, &stEncWinL, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 1, &stEncWinS, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);

    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    for (iCh = 0; iCh <= 3; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWinL.u16Width, stEncWinL.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(0,0,  iCh), MI_SUCCESS);
    }
    for (iCh = 4; iCh <= 5; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWinS.u16Width, stEncWinS.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(0,1,  iCh), MI_SUCCESS);
    }
    ExecFunc(start_vpe_channel(0, 0), 0);
    for (iCh = 0; iCh <= 5; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}

int test_hd_30x8(int nCh, MI_VENC_ModType_e eModType)
{
#if VENC_ENC
    MI_SYS_WindowRect_t stVifWin = { 0, 0, 1920, 1080 };
    MI_SYS_WindowRect_t stVpeWin = { 0, 0, 736, 480 };
    MI_SYS_WindowRect_t stEncWinS = { 0, 0, 736, 480 };
    int iCh;

    DBG_INFO("\n\n"
             "n channels VIF->VPE->VENC, 1 FHD sensors\n"
             "\tVIF 0,0--VPE 0,0  HD-- HD VENC 0,2,4,6\n" //core 0
             "\t                    `- HD VENC 1,3,5,7\n"
            );
    PRINT_CRASH_WARNING;
    if (nCh < 1 || nCh > 8)
    {
        DBG_ERR("The number of channel must be 1-8 now.\n");
        return -1;
    }
    printf("%d channels test\n", nCh);

    ExecFunc(create_vif_dev(0, SAMPLE_VI_MODE_1_1080P), MI_SUCCESS);
    ExecFunc(create_vif_channel(0, 0, stVifWin.u16Width, stVifWin.u16Height,
            E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, E_MI_VIF_FRAMERATE_FULL), MI_SUCCESS);
    ExecFunc(create_vpe_channel(0, &stVpeWin), MI_SUCCESS);
    ExecFunc(config_vpe_outport(0, 0, &stEncWinS, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), MI_SUCCESS);

    ExecFunc(bind_vif_vpe(0,0,  0,0), MI_SUCCESS);
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(create_venc_channel(eModType, iCh, map_core_id(iCh, eModType), stEncWinS.u16Width, stEncWinS.u16Height), MI_SUCCESS);
        ExecFunc(bind_vpe_venc(0,0,  iCh), MI_SUCCESS);
    }
    ExecFunc(start_vpe_channel(0, 0), 0);
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }
#endif
    return 0;
}

int start_test_29(void)
{
    DBG_ENTER();
    //ExecFunc(test_fhd_30x4(3, E_MI_VENC_MODTYPE_H264E), MI_SUCCESS);
    //ExecFunc(test_hd_30x8(4, E_MI_VENC_MODTYPE_H265E), MI_SUCCESS);
    //ExecFunc(test_fhd_n_vif(2, E_MI_VENC_MODTYPE_H264E), MI_SUCCESS);
    ExecFunc(test_fhd_n_venc(8, E_MI_VENC_MODTYPE_H264E), MI_SUCCESS);

    return 0;
}
int stop_test_29(void)  {return 0;}
