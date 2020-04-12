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
#include "mixer.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIMPLE_OUTPUT (1)
#define MAX_VENC_CHANNEL (16)
struct venc_channel_state
{
    MI_U32 u32DevId;
};
struct venc_channel_state _astChn[MAX_VENC_CHANNEL];

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

#define DEF_QP (45)
#define RcType "Cbr" //Cbr | Vbr | FixQp : case-sensitive
#define Bitrate (1000000)
#define VbrMinQp (20)
#define VbrMaxQp (40)
#define VENC_CH00_QP (20)
#define AbrMaxBitrate (500000)

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
    .rc = { .u32Gop = 25, .u8QpI = DEF_QP, .u8QpP = DEF_QP}
};

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

int venc_init_rc_config(int iChn, MI_VENC_ModType_e eModType, MI_U32 u32FrameRate, VENC_Rc_t *pstRc)
{
    MI_BOOL bErr = FALSE;
    char *szRcType;

    if(pstRc == NULL)
        return -1;

    memset(pstRc, 0, sizeof(VENC_Rc_t));

    szRcType = RcType;
    pstRc->u32SrcFrmRate = u32FrameRate;

    if(strcmp("Cbr", szRcType) == 0)
    {
        pstRc->u32Bitrate = Bitrate;
        ExecFunc(bErr, FALSE);
        switch (eModType)
        {
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
    else if (strcmp("Vbr", szRcType) == 0)
    {
        switch (eModType)
        {
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
        switch (eModType)
        {
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
#if 1
    s8Core = map_core_id(VencChannel, eModType);
    s32DevId = get_venc_dev_id(eModType, s8Core);
#else
    ExecFunc(MI_VENC_GetChnDevid(VencChannel, (MI_U32*)&s32DevId), MI_SUCCESS);
#endif
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
        printf("%02X ", ((char*)buf)[i]);
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

int mixer_path_create_vif_vpe_venc(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    MI_U32 u32FrameRate;
    MI_SYS_WindowRect_t stVpeInWin = {0};
    MI_SYS_WindowRect_t stVpeOutWin = {0};
    MI_VPE_CHANNEL u32VpeChn;
    MixerVideoChnInfo_t stMixerChnInfo;
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U8 u8ChnPath;
    MI_U32 u32DevId;
    MI_VIF_DEV u32VifDev = u32VifChn/MI_VIF_MAX_WAY_NUM_PER_DEV;
    MI_VIF_WorkMode_e eWorkMode = mixer_device_get_workmode(u32VifDev);
    MI_VENC_ModType_e eModType;
    int iCh;
    int nCh = 4;

    DBG_ENTER();
    u32VpeChn = u32VifChn*2 + u32VifPort;

    mixer_chn_get_video_info(u32VifChn,&stMixerChnInfo);
    if(stMixerChnInfo.eformat == 255)
        return -1;

    DBG_INFO("%d %d\n", u32VifChn, u32VifChn);

    ExecFunc(mixer_chn_vif_create(u32VifChn, u32VifPort, &stMixerChnInfo), 0);
    stVpeInWin.u16Width = ALIGN_UP(stMixerChnInfo.u16Width, 32);

    stVpeInWin.u16Height = stMixerChnInfo.u16Height;
    ExecFunc(mixer_chn_vpe_create(u32VpeChn,  &stVpeInWin), 0);

    if(eWorkMode == E_MI_VIF_WORK_MODE_1MULTIPLEX)
    {
        stVpeOutWin.u16Width  = 1920;
        stVpeOutWin.u16Height = 1088;
    }
    else if(eWorkMode == E_MI_VIF_WORK_MODE_2MULTIPLEX)
    {
        stVpeOutWin.u16Width  = 1920/2;
        stVpeOutWin.u16Height = 1080/4;
    }
    else
    {
        stVpeOutWin.u16Width  = 1920/4;
        stVpeOutWin.u16Height = 1080/4;
    }
    stVpeOutWin.u16Width = ALIGN_UP(stVpeOutWin.u16Width, 32);
    stVpeOutWin.u16Height = ALIGN_UP(stVpeOutWin.u16Height, 32);
    //ExecFunc(mixer_chn_vpe_config_port(u32VpeChn, 3, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV), 0);
    u32FrameRate = mixer_chn_get_video_fps(u32VifChn, 0);

    stSrcChnPort.eModId    = E_MI_MODULE_ID_VIF;
    stSrcChnPort.u32DevId  = 0;
    stSrcChnPort.u32ChnId  = u32VifChn;
    stSrcChnPort.u32PortId = u32VifPort;
    stDstChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stDstChnPort.u32DevId  = 0;
    stDstChnPort.u32ChnId  = u32VpeChn;
    stDstChnPort.u32PortId = 0;
    ExecFunc(mixer_chn_vpe_config_port(u32VpeChn, stDstChnPort.u32PortId, &stVpeOutWin, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420), 0);

    ExecFunc(mixer_bind_module(&stSrcChnPort,&stDstChnPort, u32FrameRate, u32FrameRate), 0);

    // VENC
    stSrcChnPort = stDstChnPort;
    u8ChnPath = mixer_chn_get_path(u32VifChn, u32VifPort);

    //DBG_INFO("Get path %c\n", u8ChnPath);
    stDstChnPort.eModId    = E_MI_MODULE_ID_VENC;
    stDstChnPort.u32PortId = 0;
    for(iCh = 0; iCh < nCh; ++iCh)
    {
        VENC_Rc_t stRc;
        switch (u8ChnPath)
        {
            case E_MIXER_CHN_PATH_VIF_VPE_VENC_H264:
                u32DevId = (iCh & 1) + 2;
                eModType = E_MI_VENC_MODTYPE_H264E;
                break;
            case E_MIXER_CHN_PATH_VIF_VPE_VENC_H265:
                eModType = E_MI_VENC_MODTYPE_H265E;
                u32DevId = iCh & 1;
                break;
            case E_MIXER_CHN_PATH_VIF_VPE_VENC_JPEG:
                eModType = E_MI_VENC_MODTYPE_JPEGE;
                u32DevId = 4;
                break;
            default:
                eModType = E_MI_VENC_MODTYPE_MAX;
                u32DevId = 2;
                break;
        }
        stDstChnPort.u32DevId  = u32DevId;
        stDstChnPort.u32ChnId  = (MI_U32)iCh;
        ExecFunc(venc_init_rc_config(iCh, eModType, u32FrameRate, &stRc), MI_SUCCESS);
        DBG_INFO("Create VENC CH%2d type:%d\n", iCh, eModType);
        ExecFunc(create_venc_channel(eModType, iCh, stVpeOutWin.u16Width, stVpeOutWin.u16Height, &stRc), MI_SUCCESS);
        ExecFunc(mixer_bind_module(&stSrcChnPort, &stDstChnPort, u32FrameRate, u32FrameRate), 0);
    }

    ExecFunc(mixer_chn_vpe_start(u32VpeChn), 0);
    for (iCh = 0; iCh < nCh; ++iCh)
    {
        ExecFunc(start_venc_channel(iCh), MI_SUCCESS);
    }

    return 0;
}

int mixer_path_destroy_vif_vpe_venc(MI_VIF_CHN u32VifChn , MI_VIF_PORT u32VifPort)
{
    return 0;
}

