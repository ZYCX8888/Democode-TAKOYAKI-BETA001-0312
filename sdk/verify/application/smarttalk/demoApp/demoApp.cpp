#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/syscall.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "st_common.h"
#include "st_ao.h"
#include "st_voice.h"
#include "st_disp.h"

#include "st_fb.h"
#include "st_gpio.h"
#include "mi_divp.h"

#include "mi_ao.h"
#include "mi_panel.h"
#include "mi_venc.h"
#include "mi_vdec_datatype.h"
#include "mi_gfx.h"

#include "utility.h"

#include "main_ui.h"

#ifdef UI_1024_600
#include "SAT070CP50_1024x600.h"
#else
//#include "FPGA_800x480_60.h"
#include "SAT070A_800x480.h"
#endif

#include "omron.h"
#include "cmdqueue.h"
#include "st_venc.h"
#include "st_stream.h"
#include "st_xmlprase.h"
#include "st_socket.h"
#include "st_vdec.h"
#include "st_system.h"

#include "app_config.h"

//#include "v4l2.h"

static MI_S32 g_s32Sys_Status = E_DEVICE_IDLE;
static bool g_bExit = FALSE;
static int g_AppRun = FALSE;
static int g_V4l2Run = FALSE;
static ST_Run_Para_S g_stRunPara;

static ST_OMRON_Mng_S g_stOMRONMng;
ST_OMRON_Osd_S g_stOMRONOsd[MAX_OSD_NUM];

static int g_VdecRun = FALSE;
static pthread_t g_VdeStream_tid = 0;

MsgQueue g_toAPP_msg_queue;
sem_t g_toAPP_sem;
extern MsgQueue g_SocketCall_Queue;
extern sem_t g_SocketCall_Sem;
extern MsgQueue g_SocketAnswerCall_Queue;
extern sem_t g_SocketAnswerCall_Sem;
extern MsgQueue g_toUI_msg_queue;
extern sem_t g_toUI_sem;
extern int g_faceDetect;

void ST_HandleSig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        ST_INFO("catch Ctrl + C, exit normally\n");

        g_bExit = TRUE;
    }
}
void ST_Flush(void)
{
    char c;
    while ((c = getchar()) != '\n'&&c!=EOF);
}
//==============================================================================
#include "mi_vdec.h"
FILE *g_pStreamFile[32] = {NULL};
#define NALU_PACKET_SIZE            256*1024
#define ADD_HEADER_ES

typedef struct
{
    int startcodeprefix_len;
    unsigned int len;
    unsigned int max_size;
    char *buf;
    unsigned short lost_packets;
} NALU_t;

static int info2 = 0, info3 = 0;

static int FindStartCode2 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 1))
        return 0;
    else
        return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
    if((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0) || (Buf[3] != 1))
        return 0;
    else
        return 1;
}

NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n;
    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    {
        printf("AllocNALU: n");
        exit(0);
    }
    n->max_size=buffersize;
    if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
    {
        free (n);
        printf ("AllocNALU: n->buf");
        exit(0);
    }
    return n;
}

void FreeNALU(NALU_t *n)
{
    if (n)
    {
        if (n->buf)
        {
            free(n->buf);
            n->buf=NULL;
        }
        free (n);
    }
}

int GetAnnexbNALU (NALU_t *nalu, MI_S32 chn)
{
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
        printf ("GetAnnexbNALU: Could not allocate Buf memory\n");
    nalu->startcodeprefix_len=3;
    if (3 != fread (Buf, 1, 3, g_pStreamFile[chn]))
    {
        free(Buf);
        return 0;
    }
    info2 = FindStartCode2 (Buf);
    if(info2 != 1)
    {
        if(1 != fread(Buf+3, 1, 1, g_pStreamFile[chn]))
        {
            free(Buf);
            return 0;
        }
        info3 = FindStartCode3 (Buf);
        if (info3 != 1)
        {
            free(Buf);
            return -1;
        }
        else
        {
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    }
    else
    {
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;
    while (!StartCodeFound)
    {
        if (feof (g_pStreamFile[chn]))
        {
            nalu->len = (pos-1)-nalu->startcodeprefix_len;
            memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            free(Buf);
            fseek(g_pStreamFile[chn], 0, 0);
            return pos-1;
        }
        Buf[pos++] = fgetc (g_pStreamFile[chn]);
        info3 = FindStartCode3(&Buf[pos-4]);
        if(info3 != 1)
            info2 = FindStartCode2(&Buf[pos-3]);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    rewind = (info3 == 1) ? -4 : -3;
    if (0 != fseek (g_pStreamFile[chn], rewind, SEEK_CUR))
    {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }
    nalu->len = (pos+rewind);
    memcpy (nalu->buf, &Buf[0], nalu->len);
    free(Buf);
    return (pos+rewind);
}

void dump(NALU_t *n)
{
    if (!n)
        return;
    //printf(" len: %d  ", n->len);
    //printf("nal_unit_type: %x\n", n->nal_unit_type);
}

void *ST_VdecSendStream(void *args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32TimeOutMs = 20, s32ChannelId = 0, s32TempHeight = 0;
    MI_S32 s32Ms = 30;
    MI_BOOL bVdecChnEnable;
    MI_U16 u16Times = 20000;

    MI_S32 s32ReadCnt = 0;
    FILE *readfp = NULL;
    MI_U8 *pu8Buf = NULL;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[32] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_VideoStream_t stVdecStream;

    MI_U32 u32FpBackLen = 0; // if send stream failed, file pointer back length

    char tname[32];
    memset(tname, 0, 32);

#ifndef ADD_HEADER_ES
    NALU_t *n;
    n = AllocNALU(2000000);
#endif

    vdecChn = UVC_VDEC_CHN;
    snprintf(tname, 32, "push_t_%u", vdecChn);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;//0 1 2 3
    stChnPort.u32PortId = 0;
    if ((access("720P25.h264", F_OK))!=-1)
    {
        readfp = fopen("720P25.h264", "rb"); //ES
        ST_DBG("open current dir es file\n");
    }
    else if ((access("/customer/720P25.h264", F_OK))!=-1)
    {
        readfp = fopen("/customer/alsa.conf720P25.h264", "rb"); //ES
        ST_DBG("open /customer dir es file\n");
    }
    if (!readfp)
    {
        ST_ERR("Open es file failed!\n");
        return NULL;
    }
    else
    {
        g_pStreamFile[vdecChn] = readfp;
    }

    // s32Ms = _stTestParam.stChannelInfo[s32VoChannel].s32PushDataMs;
    // bVdecChnEnable = _stTestParam.stChannelInfo[0].bVdecChnEnable;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = (MI_U8 *)malloc(NALU_PACKET_SIZE);

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 3);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth error, %X\n", s32Ret);
        return NULL;
    }

    s32Ms = 30;

    //printf("----------------------%d------------------\n", stChnPort.u32ChnId);
    while (g_VdecRun)
    {
#ifdef ADD_HEADER_ES
        memset(au8Header, 0, 16);
        u32Pos = fseek(readfp, 0, SEEK_CUR);
        s32Len = fread(au8Header, 1, 16, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }
        u32FrameLen = MI_U32VALUE(au8Header, 4);
        // printf("vdecChn:%d, u32FrameLen:%d, %d\n", vdecChn, u32FrameLen, NALU_PACKET_SIZE);
        if(u32FrameLen > NALU_PACKET_SIZE)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }
        s32Len = fread(pu8Buf, 1, u32FrameLen, readfp);
        if(s32Len <= 0)
        {
            fseek(readfp, 0, SEEK_SET);
            continue;
        }

        stVdecStream.pu8Addr = pu8Buf;
        stVdecStream.u32Len = s32Len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len + 16; //back length
#else
        GetAnnexbNALU(n, vdecChn);
        dump(n);
        stVdecStream.pu8Addr = (MI_U8 *)n->buf;
        stVdecStream.u32Len = n->len;
        stVdecStream.u64PTS = u64Pts;
        stVdecStream.bEndOfFrame = 1;
        stVdecStream.bEndOfStream = 0;

        u32FpBackLen = stVdecStream.u32Len; //back length
#endif

        if(0x00 == stVdecStream.pu8Addr[0] && 0x00 == stVdecStream.pu8Addr[1]
            && 0x00 == stVdecStream.pu8Addr[2] && 0x01 == stVdecStream.pu8Addr[3]
            && 0x65 == stVdecStream.pu8Addr[4] || 0x61 == stVdecStream.pu8Addr[4]
            || 0x26 == stVdecStream.pu8Addr[4] || 0x02 == stVdecStream.pu8Addr[4]
            || 0x41 == stVdecStream.pu8Addr[4])
        {
            usleep(s32Ms * 1000);
        }

        if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(vdecChn, &stVdecStream, s32TimeOutMs)))
        {
            //ST_ERR("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            fseek(readfp, - u32FpBackLen, SEEK_CUR);
        }

        u64Pts = u64Pts + ST_Sys_GetPts(30);

        if (0 == (s32ReadCnt++ % 30))
            ;// printf("vdec(%d) push buf cnt (%d)...\n", s32VoChannel, s32ReadCnt)
            ;//printf("###### ==> Chn(%d) push frame(%d) Frame Dec:%d  Len:%d\n", s32VoChannel, s32ReadCnt, stChnStat.u32DecodeStreamFrames, u32Len);
    }
    printf("\n\n");
    usleep(300000);
    free(pu8Buf);

    printf("End----------------------%d------------------End\n", stChnPort.u32ChnId);

    return NULL;
}

MI_S32 ST_CreateVdec4Uvc(MI_S32 s32VdecChn, MI_S32 s32DivpChn, MI_U32 u32VdecW, MI_U32 u32VdecH, MI_S32 s32CodecType)
{
    ST_Rect_T stCrop= {0, 0, 0, 0};
    ST_CreateVdecChannel(s32VdecChn, s32CodecType, u32VdecW, u32VdecH, VIDEO_DISP_W, VIDEO_DISP_H);
    ST_CreateDivpChannel(s32DivpChn, 1920, 1080, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV, stCrop);
    ST_ModuleBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                E_MI_MODULE_ID_DIVP, 0, s32DivpChn, 0); //VDEC->DIVP
    return MI_SUCCESS;
}
MI_S32 ST_DestroyVdec4Uvc(MI_S32 s32VdecChn, MI_S32 s32DivpChn)
{
    ST_ModuleUnBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                    E_MI_MODULE_ID_DIVP, 0, s32DivpChn, 0); //VDEC->DIVP
    ST_DestroyVdecChannel(s32VdecChn);
    ST_DestroyDivpChannel(s32DivpChn);

    return MI_SUCCESS;
}
#if 0
void *GetVucBuffer(void *args)
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSnapFace;
    MI_SYS_ChnPort_t stSysChnPort;
    DeviceContex_t *ctx = NULL;
    Packet pkt;
    MI_S32 s32Ret;
    //FILE *savefp = NULL;
    if((access("/dev/video0", F_OK))!=-1)
    {
        ST_DBG("found uvc device\n");
    }
    else
    {
        ST_ERR("uvc device not found\n");
        return NULL;
    }
    v4l2_dev_init(&ctx, "/dev/video0");
    //savefp = fopen("/var/tmp.es", "w+");

    v4l2_dev_set_fmt(ctx, V4L2_PIX_FMT_H264, 640, 480);
    v4l2_read_header(ctx);
    while (g_V4l2Run)
    {
        s32Ret = v4l2_read_packet(ctx, &pkt);
        if(s32Ret == -EAGAIN) {
            usleep(10000);
            continue;
        }
        #if 0
       MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
       stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
       stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
       stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
       stBufConf.stFrameCfg.u16Width = 640;
       stBufConf.stFrameCfg.u16Height = 480;

        stSysChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stSysChnPort.u32DevId = 0;
        stSysChnPort.u32ChnId = UVC_DIVP_CHN;
        stSysChnPort.u32PortId = 0;
        s32Ret = MI_SYS_ChnInputPortGetBuf(&stSysChnPort, &stBufConf, &stBufInfo, &hSnapFace, 1000);
        if (MI_SUCCESS != s32Ret)
        {
            //ST_ERR("GetInputPortBuf Fail...\n");
            v4l2_read_packet_end(ctx, &pkt);
            continue;
        }
        memcpy(stBufInfo.stFrameData.pVirAddr[0], pkt.data, 640*480);
        memcpy(stBufInfo.stFrameData.pVirAddr[1], pkt.data+640*480, (640*480) >> 1);
        s32Ret = MI_SYS_ChnInputPortPutBuf(hSnapFace ,&stBufInfo , FALSE);
        if (MI_SUCCESS != s32Ret)
        {
            ST_ERR("Inject UVC divp Buffer fail...\n");
            v4l2_read_packet_end(ctx, &pkt);
            continue;
        }
        #endif
        if (s32Ret >= 0)
        {
            if (pkt.size > 4)
            {
                if (MI_SUCCESS != ST_SendVdecFrame(UVC_VDEC_CHN, pkt.data, pkt.size))
                {
                    ST_ERR("Send UVC stream to vdec, return fail, len = %d\n", pkt.size);
                }
            }
            //printf("Get Pkt size%d\n", pkt.size);
            //save_file(pkt.data, pkt.size, 0);
            //if (savefp)
            //{
            //    fwrite(pkt.data, pkt.size, 1, savefp);
            //}
            v4l2_read_packet_end(ctx, &pkt);
        }
        //usleep(30*1000);
    }
    v4l2_read_close(ctx);

    v4l2_dev_deinit(ctx);
}
#endif
static MI_S32 ST_VideoModuleInit()
{
    MI_U32 u32VencDevID = 0;
    MI_SYS_ChnPort_t stChnPort;
    ST_Rect_T stCrop = {0, 0, 0, 0};

    ST_CreateVdec4Uvc(UVC_VDEC_CHN, UVC_DIVP_CHN, 1920, 1080, E_MI_VDEC_CODEC_TYPE_H264);

    //ST_CreateDivpChannel(MAINVENC_DIVP_CHN, VIDEO_DISP_W, 480, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, stCrop);
    ST_CreateDivpChannel(LOCAL_DISP_DIVP_CHN, VIDEO_DISP_W, VIDEO_DISP_H, E_MI_SYS_PIXEL_FRAME_YUV422_YUYV, stCrop);
    ST_CreateDivpChannel(FD_DIVP_CHN, VIDEO_DISP_W, VIDEO_DISP_H, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, stCrop);

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = LOCAL_DISP_DIVP_CHN;
    stChnPort.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 4);

    //=====================Create Venc Chn==================================
    //ST_VencCreateChannel(MAIN_STREAM_VENC, E_ST_H264, VIDEO_DISP_W, 480, 30);
    //MI_VENC_GetChnDevid(MAIN_STREAM_VENC, &u32VencDevID);

    //ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
    //            E_MI_MODULE_ID_DIVP, 0, MAINVENC_DIVP_CHN, 0); //VENC: DIVP->DIVP
    ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
                E_MI_MODULE_ID_DIVP, 0, LOCAL_DISP_DIVP_CHN, 0); //DISP: DIVP->DIVP
    ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
                E_MI_MODULE_ID_DIVP, 0, FD_DIVP_CHN, 0); //FD: DIVP->DIVP

    //ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, MAINVENC_DIVP_CHN, 0,
    //            E_MI_MODULE_ID_VENC, u32VencDevID, MAIN_STREAM_VENC, 0); //DIVP->VENC

    //ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, LOCAL_DISP_DIVP_CHN, 0,
    //            E_MI_MODULE_ID_DISP, 0, 0, 0); //DIVP->VENC
    return 0;
}

static MI_S32 ST_VideoModuleDeInit()
{
    ST_DispChnInfo_t stDispChnInfo;
    //MI_U32 u32VencDevID = 0;

    //ST_ModuleUnBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
    //            E_MI_MODULE_ID_DIVP, 0, MAINVENC_DIVP_CHN, 0); //VENC: DIVP->DIVP
    ST_ModuleUnBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
                E_MI_MODULE_ID_DIVP, 0, LOCAL_DISP_DIVP_CHN, 0); //DISP: DIVP->DIVP
    ST_ModuleUnBind(E_MI_MODULE_ID_DIVP, 0, UVC_DIVP_CHN, 0,
                E_MI_MODULE_ID_DIVP, 0, FD_DIVP_CHN, 0); //FD: DIVP->DIVP

    ST_CreateVdec4Uvc(UVC_VDEC_CHN, UVC_DIVP_CHN, 1920, 1080, E_MI_VDEC_CODEC_TYPE_H264);
    ST_Disp_DeInit(0, 0, 1);

    //ST_DestroyDivpChannel(MAINVENC_DIVP_CHN);
    ST_DestroyDivpChannel(LOCAL_DISP_DIVP_CHN);
    ST_DestroyDivpChannel(FD_DIVP_CHN);

    //ST_VencDestroyChannel(MAIN_STREAM_VENC);

    return 0;
}

MI_S32 ST_CreateVdec2DispPipe(MI_S32 s32VdecChn, MI_S32 s32DivpChn, MI_U32 u32VdecW, MI_U32 u32VdecH, MI_S32 s32CodecType)
{
    ST_Rect_T stCrop= {0, 0, 0, 0};
    #ifdef UI_1024_600
        ST_CreateVdecChannel(s32VdecChn, s32CodecType, u32VdecW, u32VdecH, VIDEO_DISP_1024_W, VIDEO_DISP_1024_H);
    #else
        ST_CreateVdecChannel(s32VdecChn, s32CodecType, u32VdecW, u32VdecH, VIDEO_DISP_W, VIDEO_DISP_H);
    #endif
    ST_Disp_EnableChn(0, 0);
    ST_ModuleBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                    E_MI_MODULE_ID_DISP, 0, 0, 0); //DIVP->DISP

    return MI_SUCCESS;
}

MI_S32 ST_DestroyVdec2DispPipe(     MI_S32 s32VdecChn, MI_S32 s32DivpChn)
{
    ST_ModuleUnBind(E_MI_MODULE_ID_VDEC, 0, s32VdecChn, 0,
                    E_MI_MODULE_ID_DISP, 0, 0, 0); //DIVP->DISP
    ST_DestroyVdecChannel(s32VdecChn);
    ST_Disp_DisableChn(0, 0);

    return MI_SUCCESS;
}

void ST_LocalCameraDisp(MI_S32 s32Disp)
{
    if (s32Disp)
    {
        ST_CreateVdec2DispPipe(UVC_VDEC_CHN, 0, 1280, 720, E_MI_VDEC_CODEC_TYPE_H264);
        g_VdecRun = TRUE;
        pthread_create(&g_VdeStream_tid, NULL, ST_VdecSendStream, NULL);
        if ((access("8K_16bit_MONO.wav", F_OK))!=-1)
        {
            StartPlayAudioFile("8K_16bit_MONO.wav", 2);
            ST_DBG("open current dir wav file\n");
        }
        else if ((access("/customer/8K_16bit_MONO.wav", F_OK))!=-1)
        {
            StartPlayAudioFile("/customer/8K_16bit_MONO.wav", 2);
            ST_DBG("open /custome dir wav file\n");
        }
        ST_DBG("Bind ST_LocalCameraDisp...\n");
    }
    else
    {
        g_VdecRun = FALSE;
        pthread_join(g_VdeStream_tid, NULL);
        ST_DestroyVdec2DispPipe(UVC_VDEC_CHN, 0);
        if ((access("8K_16bit_MONO.wav", F_OK))!=-1)
        {
            StopPlayAudioFile();
            ST_DBG("open current dir wav file\n");
        }
        else if ((access("/customer/8K_16bit_MONO.wav", F_OK))!=-1)
        {
            StopPlayAudioFile();
            ST_DBG("open /customer dir wav file\n");
        }
        ST_DBG("UnBind ST_LocalCameraDisp...\n");
    }

    return;
}

#if 0
void *ST_OMRON_GetYUVDataProc(void *args)
{
    ST_OMRON_Mng_S* pstOMRONMng = (ST_OMRON_Mng_S *)args;
    MI_SYS_ChnPort_t stChnPort;
    MI_S32 s32Ret = MI_SUCCESS;
    struct pollfd fds = {0, POLLIN | POLLERR};
    int rval = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE bufHandle;
    MI_U32 u32Count = 0;
    char szFileName[64] = {0,};
    int fd = 0;

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = pstOMRONMng->stSource.enModule;
    stChnPort.u32DevId = pstOMRONMng->stSource.u32Dev;
    stChnPort.u32ChnId = pstOMRONMng->stSource.u32Chn;
    stChnPort.u32PortId = pstOMRONMng->stSource.u32Port;
    s32Ret = MI_SYS_GetFd(&stChnPort, (MI_S32 *)&fds.fd);
    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_GetFd 0, error, %X\n", s32Ret);
        return NULL;
    }

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 3);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_SetChnOutputPortDepth err:%x, chn:%d,port:%d\n", s32Ret,
            stChnPort.u32ChnId, stChnPort.u32PortId);
        return NULL;
    }

    ST_DBG("pid=%d, %d\n", getpid(), syscall(SYS_gettid));

    while(pstOMRONMng->bGetYUVRun)
    {
        rval = poll(&fds, 1, 200);
        if(rval < 0)
        {
            ST_DBG("poll error!\n");
            continue;
        }
        if(rval == 0)
        {
            //ST_DBG("get fd time out!\n");
            continue;
        }
        if((fds.revents & POLLIN) != POLLIN)
        {
            ST_DBG("error !\n");
            continue;
        }

        memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &bufHandle))
        {
            ST_ERR("Get buffer error!\n");
            continue;
        }

        if (pstOMRONMng->bSaveYData)
        {
            // save Y data, luma
            memset(szFileName, 0, sizeof(szFileName) - 1);
            snprintf(szFileName, sizeof(szFileName) - 1, "divp%d_port%d_%dx%d_%04d_luma.yuv", pstOMRONMng->stSource.u32Chn,
                pstOMRONMng->stSource.u32Port, pstOMRONMng->u32Width, pstOMRONMng->u32Height, u32Count ++);
            fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd > 0)
            {
                write(fd, stBufInfo.stFrameData.pVirAddr[0],
                    stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
                write(fd, stBufInfo.stFrameData.pVirAddr[1],
                    stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] >> 1);
                close(fd);
                fd = -1;
            }
            pstOMRONMng->bSaveYData = 0;
        }

        pthread_mutex_lock(&pstOMRONMng->dataMutex);
        if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == stBufInfo.stFrameData.ePixelFormat)
        {
            memcpy(pstOMRONMng->pYData, stBufInfo.stFrameData.pVirAddr[0], pstOMRONMng->u32MaxSize);
            memcpy(pstOMRONMng->pUVData, stBufInfo.stFrameData.pVirAddr[1], (pstOMRONMng->u32MaxSize >> 1));
            //printf("stride = %d stride1 = %d...\n", stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u32Stride[1]);
        }
        else
        {
            ST_DBG("Please config vpe fdfr output port pixelformat == NV12\n");
        }
        MI_SYS_ChnOutputPortPutBuf(bufHandle);
        pthread_mutex_unlock(&pstOMRONMng->dataMutex);

        pthread_cond_signal(&pstOMRONMng->dataCond);
    }

    return NULL;
}

MI_S32 ST_OMRON_SDKInit(MI_S32 s32DepVpeChannel, ST_OMRON_Source_S *pstOmronSrcChn)
{
    ST_OMRON_Mng_S* pstOMRONMng = &g_stOMRONMng;
    ST_OMRON_Osd_S *pstOMRONOsd = g_stOMRONOsd;

    ST_VPE_PortInfo_T stPortInfo;

    // init params
    pstOMRONMng->u32Width = VIDEO_DISP_W;
    pstOMRONMng->u32Height = VIDEO_DISP_H;
    pstOMRONMng->enIEMode = OMRON_IE_MODE_FDFR;
    pstOMRONMng->u32MaxSize = pstOMRONMng->u32Width * pstOMRONMng->u32Height; // yuv420
    pstOMRONMng->pYData = (unsigned char *)malloc(pstOMRONMng->u32MaxSize);
    pstOMRONMng->pUVData = (unsigned char *)malloc(pstOMRONMng->u32MaxSize>>1);
    if ((pstOMRONMng->pYData == NULL) || (pstOMRONMng->pUVData == NULL))
    {
        ST_ERR("malloc error\n");
        return -1;
    }
    pthread_mutex_init(&pstOMRONMng->dataMutex, NULL);
    pstOMRONMng->stSource.enModule = pstOmronSrcChn->enModule;
    pstOMRONMng->stSource.u32Dev = pstOmronSrcChn->u32Dev;
    pstOMRONMng->stSource.u32Chn = pstOmronSrcChn->u32Chn;
    pstOMRONMng->stSource.u32Port = pstOmronSrcChn->u32Port;
    pstOMRONMng->hBody = NULL;
    pstOMRONMng->hBodyResult = NULL;
    pstOMRONMng->hCo = NULL;
    pstOMRONMng->hDT = NULL;
    pstOMRONMng->hDtResult = NULL;
    pstOMRONMng->hPT = NULL;
    pstOMRONMng->hPtResult = NULL;
    pstOMRONMng->hAL = NULL;
    pstOMRONMng->hFD = NULL;
    pstOMRONMng->bAlbumProcess = FALSE;
    pstOMRONMng->bTaskRun = FALSE;
    pstOMRONMng->bGetYUVRun = FALSE;
    pstOMRONMng->bSaveYData = FALSE;
    pthread_cond_init(&pstOMRONMng->dataCond, NULL);
    pthread_mutex_init(&pstOMRONMng->condMutex, NULL);

    // create thread to get YUV
    pstOMRONMng->bGetYUVRun = TRUE;
    pthread_create(&pstOMRONMng->getYUVThread, NULL, ST_OMRON_GetYUVDataProc, pstOMRONMng);

    ST_OMRON_StartDetect(pstOMRONMng);

    return OMCV_NORMAL;
}

void ST_OSD_VarInit(void)
{
    MI_U32 i = 0;
    ST_OMRON_Osd_S *pstOMRONOsd = g_stOMRONOsd;

    for (i = 0; i < MAX_OSD_NUM; i ++)
    {
        pstOMRONOsd[i].hHandle = ST_OSD_HANDLE_INVALID;
        pstOMRONOsd[i].u32Width = 0;
        pstOMRONOsd[i].u32Height = 0;
        pstOMRONOsd[i].s32InitFlag = FALSE;
        pthread_mutex_init(&pstOMRONOsd[i].mutex, NULL);

        INIT_LIST_HEAD(&pstOMRONOsd[i].dirtyAreaList);
    }
}

MI_S32 ST_OMRON_OSD_Init(MI_U16 u16DispW, MI_U16 u16DispH)
{
    // create RGN
    ST_OSD_Attr_T stOsdAttr;
    MI_RGN_HANDLE hRgnHandle;
    ST_OMRON_Osd_S *pstOMRONOsd = g_stOMRONOsd;
    hRgnHandle = MAX_OSD_NUM - 1;

    ST_OSD_VarInit();

    memset(&stOsdAttr, 0, sizeof(ST_OSD_Attr_T));
    stOsdAttr.stRect.s32X = 0;
    stOsdAttr.stRect.s32Y = 0;
    stOsdAttr.stRect.s16PicW = u16DispW;
    stOsdAttr.stRect.s16PicH = u16DispH;
    stOsdAttr.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stOsdAttr.stRgnChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stOsdAttr.stRgnChnPort.s32DevId = 0;
    stOsdAttr.stRgnChnPort.s32ChnId = LOCAL_DISP_DIVP_CHN;
    stOsdAttr.stRgnChnPort.s32OutputPortId = 0;
    ExecFunc(ST_OSD_Init(hRgnHandle, &stOsdAttr), MI_RGN_OK);

    pstOMRONOsd[hRgnHandle].hHandle = hRgnHandle;
    pstOMRONOsd[hRgnHandle].u32Width = u16DispW;
    pstOMRONOsd[hRgnHandle].u32Height = u16DispH;
    pstOMRONOsd[hRgnHandle].s32InitFlag = TRUE;

    return MI_SUCCESS;
}
#endif
static void TalkingMediaStart(unsigned long DstIP)
{
    MI_S32 s32VolumeValue = 0;
    MI_BOOL bMute = FALSE;

    ST_XmlParseDevVolumeCfg((const MI_U8*)"SpeakerVolume", &s32VolumeValue, &bMute);
    if (s32VolumeValue)
        s32VolumeValue = s32VolumeValue * (MAX_ADJUST_AO_VOLUME - MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
    else
        s32VolumeValue = MIN_AO_VOLUME;

    printf("Func=%s\n", __FUNCTION__);
    ST_CreateVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN, VIDEO_STREAM_W, VIDEO_STREAM_H, E_MI_VDEC_CODEC_TYPE_H264);
    Stream_StartPlayVideo();
    Stream_StartRecvAudio(s32VolumeValue, bMute);
    //Stream_StartSendVideo(DstIP);
    Stream_StartSendAudio(DstIP);
}

static void TalkingMediaStop()
{
    printf("Func=%s\n", __FUNCTION__);
    Stream_StopRecvAudio();
    //Stream_StopSendVideo();
    Stream_StopSendAudio();
    Stream_StopPlayVideo();
    ST_DestroyVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN);
    //ST_Disp_ClearChn(0,0);
}

static void TalkingMediaStartForDoor(unsigned long DstIP)
{
    MI_S32 s32VolumeValue = 0;
    MI_BOOL bMute = FALSE;

    ST_XmlParseDevVolumeCfg((const MI_U8*)"SpeakerVolume", &s32VolumeValue, &bMute);
    if (s32VolumeValue)
        s32VolumeValue = s32VolumeValue * (MAX_ADJUST_AO_VOLUME - MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
    else
        s32VolumeValue = MIN_AO_VOLUME;

    printf("Func=%s\n", __FUNCTION__);
    ST_CreateVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN, VIDEO_STREAM_W, VIDEO_STREAM_H, E_MI_VDEC_CODEC_TYPE_H264);
    Stream_StartPlayVideo();
    Stream_StartRecvAudio(s32VolumeValue, bMute);
    Stream_StartSendAudio(DstIP);
}

static void TalkingMediaStopForDoor()
{
    printf("Func=%s\n", __FUNCTION__);
    Stream_StopRecvAudio();
    Stream_StopPlayVideo();
    Stream_StopSendAudio();
    ST_DestroyVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN);
    //ST_Disp_ClearChn(0,0);
}

static void MonitorMediaStart()
{
    MI_S32 s32VolumeValue = 0;
    MI_BOOL bMute = FALSE;

    ST_XmlParseDevVolumeCfg((const MI_U8*)"SpeakerVolume", &s32VolumeValue, &bMute);
    if (s32VolumeValue)
        s32VolumeValue = s32VolumeValue * (MAX_ADJUST_AO_VOLUME - MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
    else
        s32VolumeValue = MIN_AO_VOLUME;

    printf("Func=%s\n", __FUNCTION__);
    ST_CreateVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN, VIDEO_STREAM_W, VIDEO_STREAM_H, E_MI_VDEC_CODEC_TYPE_H264);
    Stream_StartPlayVideo();
    Stream_StartRecvAudio(s32VolumeValue, bMute);
}

static void MonitorMediaStop()
{
    printf("Func=%s\n", __FUNCTION__);
    ST_DestroyVdec2DispPipe(TALK_VDEC_CHN, TALK_DIVP_CHN);
    Stream_StopPlayVideo();
    Stream_StopRecvAudio();
    //ST_Disp_ClearChn(0,0);
}

static void MonitoredMediaStart(unsigned long DstIP)
{
    printf("Func=%s\n", __FUNCTION__);
    Stream_StartSendVideo(DstIP);
    Stream_StartSendAudio(DstIP);
}

static void MonitoredMediaStop()
{
    printf("Func=%s\n", __FUNCTION__);
    Stream_StopSendVideo();
    Stream_StopSendAudio();
    //ST_Disp_ClearChn(0,0);
}

void *msg_toAPPcmd_process(void *args)
{
    MI_S32 s32Ret = 0;
    Msg* pMsg = NULL;
    unsigned long recvmsg[4];
    unsigned long sendmsg[4];
    unsigned long remoteIp = 0xFFFFFFFF;
    unsigned long remoteDeviceType = E_ST_DEV_ROOM;
    unsigned long localcallIp = 0xFFFFFFFF;

    while (g_AppRun)
    {
        memset(recvmsg,0,sizeof(recvmsg));
        sem_wait(&g_toAPP_sem);
        pMsg = g_toAPP_msg_queue.get_message();
        if(pMsg){
            s32Ret = cmd_parse_msg(pMsg, recvmsg);
            if(s32Ret < 0)
                break;
            if(s32Ret == 0)
                continue;
            switch (recvmsg[0])
            {
                case MSG_TYPE_LOCAL_VIDEO_DISP:
                    ST_LocalCameraDisp(recvmsg[1]);
                    break;
                case MSG_TYPE_CONFIRM_REGSITER_FACE:
                    ST_OMRON_RegisterFaceID(&g_stOMRONMng);
                    printf("register face !!!\n");
                    break;
                case MSG_TYPE_START_SEND_VIDEO:
                    if ((access("8K_16bit_MONO.wav", F_OK))!=-1)
                    {
                        StartPlayAudioFile("8K_16bit_MONO.wav", 2);
                        ST_DBG("open current dir wav file\n");
                    }
                    else if ((access("/customer/8K_16bit_MONO.wav", F_OK))!=-1)
                    {
                        StartPlayAudioFile("/customer/8K_16bit_MONO.wav", 2);
                        ST_DBG("open /customer dir wav file\n");
                    }
                    continue;
                case MSG_TYPE_STOP_SEND_VIDEO:
                    if ((access("8K_16bit_MONO.wav", F_OK))!=-1)
                    {
                        StopPlayAudioFile();
                        ST_DBG("open current dir wav file\n");
                    }
                    else if ((access("/customer/8K_16bit_MONO.wav", F_OK))!=-1)
                    {
                        StopPlayAudioFile();
                        ST_DBG("open /customer dir wav file\n");
                    }
                    continue;
                case MSG_TYPE_CREATE_XMLCFG:
                    ST_XmlAddSmartBDCfg(NULL);
                    printf("MSG_TYPE_CREATE_XMLCFG !!!\n");
                    break;
                case MSG_TYPE_PRASE_XMLCFG:
                {
                    ST_Rect_T stUiItemArea[16];
                    ST_XmlPraseUiCfg((MI_U8*)"smartLayout.xml", (MI_U8*)"Main_LAYOUT", stUiItemArea);
                    ST_XmlPraseUiCfg((MI_U8*)"smartLayout.xml", (MI_U8*)"CallWindow_LAYOUT", stUiItemArea);
                    ST_XmlPraseUiCfg((MI_U8*)"smartLayout.xml", (MI_U8*)"MonitorWindow_LAYOUT", stUiItemArea);
                    #if 0
                    MI_U8 u8IPaddr[16];
                    memset(u8IPaddr, 0, 16);
                    ST_XmlPraseBDCfg((MI_U8 *)DEVICE_CFG_FILE, (MI_U8 *)"01010101", u8IPaddr);
                    printf("01010101 u8IPaddr == %s\n", u8IPaddr);
                    memset(u8IPaddr, 0, 16);
                    ST_XmlPraseBDCfg((MI_U8 *)DEVICE_CFG_FILE, (MI_U8 *)"01010102", u8IPaddr);
                    printf("01010102 u8IPaddr == %s\n", u8IPaddr);
                    memset(u8IPaddr, 0, 16);
                    ST_XmlPraseBDCfg((MI_U8 *)DEVICE_CFG_FILE, (MI_U8 *)"01010103", u8IPaddr);
                    printf("01010103 u8IPaddr == %s\n", u8IPaddr);
                    #endif
                    break;
                }
                case MSG_TYPE_LOCAL_START_MONITOR:
                {
                    MI_U8 u8IPaddr[16];
                    MI_U8 DeviceID[16];
                    ST_XmlPraseBD_MyInfo((MI_U8 *)DEVICE_CFG_FILE, DeviceID);
                    if (0 == strncmp((const char*)recvmsg[1], (const char*)DeviceID, 8))
                    {
                        ST_ERR("Can not monitor myself\n");
                        break;
                    }
                    memset(u8IPaddr, 0, 16);
                    if (1 == ST_XmlPraseBDCfg((MI_U8 *)DEVICE_CFG_FILE, (MI_U8 *)recvmsg[1], u8IPaddr))
                    {
                        ST_DBG("u8IPaddr == %s  DeviceID = %s\n", u8IPaddr, DeviceID);
                        localcallIp = inet_addr((const char*)u8IPaddr);
                    }
                    else
                    {
                        ST_ERR("Failure to match CallID\n");
                        break;
                    }
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_CONNECT_REMOTE;
                    if (localcallIp != 0xFFFFFFFF)
                    {
                        sendmsg[1] = localcallIp; //inet_addr((const char*)u8IPaddr);
                    }
                    else
                    {
                        break;
                    }
                    sendmsg[2] = 0;
                    ST_DBG("UI send MSG_TYPE_LOCAL_START_MONITOR 0x%x\n", sendmsg[1]);
                    g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    usleep(100*1000);
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD;
                    sendmsg[1] = localcallIp;//inet_addr((const char*)u8IPaddr);
                    sendmsg[2] = ROOM_STARTMONT;
                    g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_UI_CREATE_MONITOR_WIN;
                    sendmsg[1] = 0;
                    sendmsg[2] = 0;
                    g_toUI_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toUI_sem);
                    g_s32Sys_Status = E_DEVICE_MONITOR;
                    MonitorMediaStart();
                    break;
                }
                case MSG_TYPE_LOCAL_STOP_MONITOR:
                {
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD;
                    if (localcallIp != 0xFFFFFFFF)
                    {
                        sendmsg[1] = localcallIp;
                        sendmsg[2] = ROOM_STOPMONT;
                        g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    }
                    MonitorMediaStop();
                    localcallIp = 0xFFFFFFFF;
                    remoteIp = 0xFFFFFFFF;
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    break;
                }
                case MSG_TYPE_RECV_MONITOR_CMD: //net recv
                {
                    if (TRUE == recvmsg[2]) //remote stat monitor, init local
                    {
                        ST_DBG("net recv MSG_TYPE_RECV_MONITOR_CMD start\n");
                        if (E_DEVICE_IDLE == g_s32Sys_Status)
                        {
                            MonitoredMediaStart(recvmsg[3]);
                            memset(sendmsg, 0, 16);
                            sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                            sendmsg[1] = recvmsg[3];
                            sendmsg[2] = ROOM_MONTACK;
                            g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                            g_s32Sys_Status = E_DEVICE_MONITORED;
                        }
                        else
                        {
                            ST_DBG("send Busy cmd to remote\n");
                            memset(sendmsg, 0, 16);
                            sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                            sendmsg[1] = recvmsg[3];
                            sendmsg[2] = ROOM_BUSY;
                            g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                        }
                    }
                    else //
                    {
                        ST_DBG("net recv MSG_TYPE_RECV_MONITOR_CMD stop\n");
                        g_s32Sys_Status = E_DEVICE_IDLE;
                        MonitoredMediaStop();
                    }
                    break;
                }
                case MSG_TYPE_RECV_IDLE_CMD: //net recv
                {
                    if (E_DEVICE_CALLROOM == g_s32Sys_Status)
                    {
                        ST_DBG("recv MSG_TYPE_RECV_IDLE_CMD status(%d)\n", g_s32Sys_Status);
                    }
                    remoteIp = recvmsg[3];
                    break;
                }
                case MSG_NET_RECV_ROOM_MONTACK_CMD: //net recv
                {
                    if (E_DEVICE_MONITOR == g_s32Sys_Status)
                    {
                        ST_DBG("recv MSG_NET_RECV_ROOM_MONTACK_CMD status(%d)\n", g_s32Sys_Status);
                    }
                    remoteIp = recvmsg[3];
                    break;
                }
                case MSG_TYPE_RECV_ROOM_BUSY: //net recv
                {
                    ST_DBG("net recv MSG_TYPE_RECV_ROOM_BUSY\n");
                    remoteIp = recvmsg[3];
                    break;
                }
                case MSG_TYPE_LOCAL_CALL_ROOM:
                {
                    MI_U8 u8IPaddr[16];
                    MI_U8 DeviceID[16];
                    remoteDeviceType = E_ST_DEV_ROOM;
                    ST_XmlPraseBD_MyInfo((MI_U8 *)DEVICE_CFG_FILE, DeviceID);
                    if (0 == strncmp((const char*)recvmsg[1], (const char*)DeviceID, 8))
                    {
                        ST_ERR("Can not call myself\n");
                        break;
                    }
                    memset(u8IPaddr, 0, 16);
                    if (1 == ST_XmlPraseBDCfg((MI_U8 *)DEVICE_CFG_FILE, (MI_U8 *)recvmsg[1], u8IPaddr))
                    {
                        ST_DBG("u8IPaddr == %s  DeviceID = %s\n", u8IPaddr, DeviceID);
                        localcallIp = inet_addr((const char*)u8IPaddr);
                    }
                    else
                    {
                        ST_ERR("Failure to match CallID\n");
                        break;
                    }
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_CONNECT_REMOTE;
                    sendmsg[1] = localcallIp;
                    sendmsg[2] = 0;
                    ST_DBG("UI send MSG_TYPE_LOCAL_CALLROOM 0x%x\n", sendmsg[1]);
                    g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    usleep(100*1000);
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD;
                    sendmsg[1] = localcallIp;
                    sendmsg[2] = ROOM_CALLROOM;
                    g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    g_s32Sys_Status = E_DEVICE_CALLROOM;
                    remoteIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_TYPE_RECV_REMOTE_CALL_ROOM: //net recv
                {
                    ST_DBG("recv remote call me 0x%x\n", recvmsg[3]);
                    if (E_DEVICE_IDLE == g_s32Sys_Status)
                    {
                        //ST_CreateVdec2DispPipe(0, 0, VIDEO_DISP_W, 480, E_MI_VDEC_CODEC_TYPE_H264);
                        //Stream_StartPlayVideo();
                        if (ROOM_CALLROOM == recvmsg[1])
                        {
                            remoteDeviceType = E_ST_DEV_ROOM;
                        }
                        else if (DOOR_CALLROOM == recvmsg[1])
                        {
                            remoteDeviceType = E_ST_DEV_DOOR;
                        }
                        ST_DBG("start play video\n");
                        memset(sendmsg, 0, 16);
                        sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                        sendmsg[1] = recvmsg[3];
                        sendmsg[2] = ROOM_IDLE;
                        g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                        memset(sendmsg, 0, 16);
                        sendmsg[0] = MSG_TYPE_REMOTE_CALL_ME;
                        sendmsg[1] = 0;
                        sendmsg[2] = 0;
                        g_toUI_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toUI_sem);
                        g_s32Sys_Status = E_DEVICE_CALLED;
                        remoteIp = recvmsg[3];
                    }
                    else
                    {
                        ST_DBG("send Busy cmd to remote\n");
                        memset(sendmsg, 0, 16);
                        sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                        sendmsg[1] = recvmsg[3];
                        sendmsg[2] = ROOM_BUSY;
                        g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                    }
                    localcallIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_TYPE_TALK_CALLED_HOLDON: //UI action
                {
                    ST_DBG("local MSG_TYPE_TALK_CALLED_HOLDON\n");
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                    if (0xFFFFFFFF != remoteIp)
                    {
                        sendmsg[1] = remoteIp;
                    }
                    else
                    {
                        ST_DBG("Discard remote ip\n");
                    }
                    sendmsg[2] = ROOM_HOLDON;
                    g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                    if (E_ST_DEV_ROOM == remoteDeviceType)
                    {
                        TalkingMediaStart(remoteIp);
                    }
                    else if (E_ST_DEV_DOOR == remoteDeviceType)
                    {
                        TalkingMediaStartForDoor(remoteIp);
                    }
                    ST_DBG("Send mediastream to 0x%x\n", remoteIp);
                    g_s32Sys_Status = E_DEVICE_TALKING;
                    break;
                }
                case MSG_TYPE_TALK_CALLED_HANGUP: //UI action
                {
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD;
                    if (0xFFFFFFFF != remoteIp)
                    {
                        sendmsg[1] = remoteIp;
                    }
                    else
                    {
                        ST_DBG("Discard remote ip\n");
                    }
                    sendmsg[2] = ROOM_HANGUP;
                    g_SocketAnswerCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketAnswerCall_Sem);
                    ST_DBG("local MSG_TYPE_TALK_CALLED_HANGUP\n");
                    remoteIp = 0xFFFFFFFF;
                    if (g_s32Sys_Status == E_DEVICE_TALKING)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStop();
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStopForDoor();
                        }
                    }
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    localcallIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_TYPE_TALK_CALLER_HANGUP: //UI action
                {
                    ST_DBG("local MSG_TYPE_TALK_CALLER_HANGUP\n");
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD;
                    if (localcallIp != 0xFFFFFFFF)
                    {
                        sendmsg[1] = localcallIp;
                    }
                    sendmsg[2] = ROOM_HANGUP;
                    if (g_s32Sys_Status != E_DEVICE_IDLE)
                    {
                        ST_DBG("local MSG_TYPE_TALK_CALLER_HANGUP 2222\n");
                        g_SocketCall_Queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_SocketCall_Sem);
                    }
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    if (g_s32Sys_Status == E_DEVICE_TALKING)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStop();
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStopForDoor();
                        }
                    }
                    localcallIp = 0xFFFFFFFF;
                    remoteIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_NET_TALK_CALLED_HOLDON:
                {
                    if (localcallIp != 0xFFFFFFFF)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStart(localcallIp);
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStartForDoor(localcallIp);
                        }
                    }
                    else
                    {
                        TalkingMediaStart(0xAC1813AC);
                    }
                    g_s32Sys_Status = E_DEVICE_TALKING;
                    break;
                }
                case MSG_NET_TALK_CALLED_HANGUP:
                {
                    ST_DBG("Called Remote Hangup process\n");
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_UI_CALLED_HANGUP;
                    sendmsg[1] = 0;
                    sendmsg[2] = 0;
                    g_toUI_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toUI_sem);
                    if (g_s32Sys_Status == E_DEVICE_TALKING)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStop();
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStopForDoor();
                        }
                    }
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    localcallIp = 0xFFFFFFFF;
                    remoteIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_NET_TALK_CALLER_HANGUP:
                {
                    ST_DBG("Caller Remote Hangup process\n");
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_UI_CALLER_HANGUP;
                    sendmsg[1] = 0;
                    sendmsg[2] = 0;
                    g_toUI_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toUI_sem);
                    if (g_s32Sys_Status == E_DEVICE_TALKING)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStop();
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStopForDoor();
                        }
                    }
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    localcallIp = 0xFFFFFFFF;
                    remoteIp = 0xFFFFFFFF;
                    break;
                }
                case MSG_TIMEOUT_HOLDON_HANGUP:
                {
                    if (g_s32Sys_Status == E_DEVICE_TALKING)
                    {
                        if (E_ST_DEV_ROOM == remoteDeviceType)
                        {
                            TalkingMediaStop();
                        }
                        else if (E_ST_DEV_DOOR == remoteDeviceType)
                        {
                            TalkingMediaStopForDoor();
                        }
                    }
                }
                case MSG_TIMEOUT_NO_HOLDON_HANGUP:
                {
                    memset(sendmsg, 0, 16);
                    sendmsg[0] = MSG_UI_CALLER_HANGUP;
                    sendmsg[1] = 0;
                    sendmsg[2] = 0;
                    g_toUI_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toUI_sem);
                    g_s32Sys_Status = E_DEVICE_IDLE;
                    localcallIp = 0xFFFFFFFF;
                    remoteIp = 0xFFFFFFFF;
                    break;
                }
                default:
                    break;
            }
        }
    }

    return NULL;
}

int main(int argc, const char* args[])
{
    MI_S32 s32DispTiming = 0;
    ST_Rect_T stDispOutRect;
    pthread_t t_app_msg, t_v4l2_vpe;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_DEV dispDev = ST_DISP_DEV0;
    MI_DISP_LAYER dispLay = ST_DISP_LAYER0;
    ST_DispChnInfo_t stDispChnInfo;

    // pull gpio4 high (BL)
//#if 0
//    system("echo 4 > /sys/class/gpio/export");
//    system("echo out > /sys/class/gpio/gpio4/direction");
//    system("echo 1 > /sys/class/gpio/gpio4/value");
//#else
//    char *pGpioValue = "0";
//    ST_Gpio_Export(4);
//    ST_Gpio_SetDirection(4, 1);
//    ST_Gpio_SetValue(4, (MI_S8*)pGpioValue, strlen(pGpioValue));
//#endif


    stPubAttr.stSyncInfo.u16Vact = stPanelParam.u16Height;
    stPubAttr.stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
    stPubAttr.stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth + stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
    stPubAttr.stSyncInfo.u16Hact = stPanelParam.u16Width;
    stPubAttr.stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
    stPubAttr.stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth + stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
    stPubAttr.stSyncInfo.u16Bvact = 0;
    stPubAttr.stSyncInfo.u16Bvbb = 0;
    stPubAttr.stSyncInfo.u16Bvfb = 0;
    stPubAttr.stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
    stPubAttr.stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
    stPubAttr.stSyncInfo.u32FrameRate = stPanelParam.u16DCLK*1000000/(stPanelParam.u16HTotal*stPanelParam.u16VTotal);
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stPubAttr.u32BgColor = YUYV_BLACK;
    ST_Disp_DevInit(dispDev, stPubAttr);

    stLayerAttr.stVidLayerSize.u16Width  = stPanelParam.u16Width;
    stLayerAttr.stVidLayerSize.u16Height = stPanelParam.u16Height;

    stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stLayerAttr.stVidLayerDispWin.u16X      = 0;
    stLayerAttr.stVidLayerDispWin.u16Y      = 0;
    stLayerAttr.stVidLayerDispWin.u16Width  = stPanelParam.u16Width;
    stLayerAttr.stVidLayerDispWin.u16Height = stPanelParam.u16Height;
    ST_Disp_LayerInit(dispDev, dispLay, stLayerAttr);

    memset(&stDispChnInfo, 0, sizeof(ST_DispChnInfo_t));

    stDispChnInfo.InputPortNum = 1;
    stDispChnInfo.stInputPortAttr[0].u32Port = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X =
        ALIGN_BACK(VIDEO_DISP_X, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y =
        ALIGN_BACK(VIDEO_DISP_Y, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width =
        ALIGN_BACK(VIDEO_DISP_W, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height =
        ALIGN_BACK(VIDEO_DISP_H, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.u16SrcWidth = VIDEO_DISP_W;
    stDispChnInfo.stInputPortAttr[0].stAttr.u16SrcHeight = VIDEO_DISP_H;
    #ifdef UI_1024_600
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X =
        ALIGN_BACK(VIDEO_DISP_1024_X, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y =
        ALIGN_BACK(VIDEO_DISP_1024_Y, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width =
        ALIGN_BACK(VIDEO_DISP_1024_W, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height =
        ALIGN_BACK(VIDEO_DISP_1024_H, 2);
    stDispChnInfo.stInputPortAttr[0].stAttr.u16SrcWidth = VIDEO_DISP_1024_W;
    stDispChnInfo.stInputPortAttr[0].stAttr.u16SrcHeight = VIDEO_DISP_1024_H;
    #endif
    STCHECKRESULT(ST_Disp_ChnInit(ST_DISP_LAYER0, &stDispChnInfo));

    MI_PANEL_Init(E_MI_PNL_LINK_TTL);
    MI_PANEL_SetPanelParam(&stPanelParam);
    MI_GFX_Open();//for minigui
    ST_System_InitCfg();
    ST_InitMiniGui(argc, args);
    //ST_VideoModuleInit();
    //ST_CreateVdec2DispPipe(UVC_VDEC_CHN, 0, VIDEO_STREAM_W, VIDEO_STREAM_H, E_MI_VDEC_CODEC_TYPE_H264);
#if 0
    ST_OMRON_Source_S stOmronSrcChn;
    stOmronSrcChn.enModule = E_MI_MODULE_ID_DIVP;//FD FR src chn
    stOmronSrcChn.u32Dev = 0;
    stOmronSrcChn.u32Chn = FD_DIVP_CHN;
    stOmronSrcChn.u32Port = 0;
    ST_OMRON_OSD_Init(VIDEO_DISP_W, VIDEO_DISP_H);
    ST_OMRON_SDKInit(0, &stOmronSrcChn);
#endif
    ST_Socket_CmdProcessInit();
    sem_init(&g_toAPP_sem,0,0);
    g_AppRun = TRUE;
    pthread_create(&t_app_msg, NULL, msg_toAPPcmd_process, NULL);
    g_V4l2Run = TRUE;
    //pthread_create(&t_v4l2_vpe, NULL, ST_VdecSendStream, NULL);
    //pthread_create(&t_v4l2_vpe, NULL, GetVucBuffer, NULL);
    system("/config/riu_w 0x101e 0x9 0x10");

	ST_CreateMainWindow_New(MAINWND_W, MAINWND_H);
    ST_DeinitMiniGui(0);
    ST_DBG("Exit main program\n");
    ST_VideoModuleDeInit();

    g_AppRun = FALSE;
    g_V4l2Run = FALSE;
    if (0 == pthread_join(t_app_msg, NULL))
    {
        ST_DBG("Join t_app_msg(%d) success\n", t_app_msg);
    }
    if (0 == pthread_join(t_v4l2_vpe, NULL))
    {
        ST_DBG("Join t_v4l2_vpe(%d) success\n", t_v4l2_vpe);
    }

    return 0;
}
