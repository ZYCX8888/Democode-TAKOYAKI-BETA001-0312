#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

#include "mi_ai.h"
#include "mi_ao.h"

#include "st_common.h"
#include "st_stream.h"

#include "st_socket.h"
#include "app_config.h"
#include "st_venc.h"

#include "st_vdec.h"

typedef struct
{
    pthread_t tid_AudioSend;
    MI_BOOL g_AudioSendRun;
    unsigned long ServerIP;
} ST_Audio_Send_S;

typedef struct
{
    pthread_t tid_AudioRecv;
    MI_BOOL g_AudioRecvRun;
    MI_S32 s32Socket;
    MI_S32 s32AoDev;
    MI_S32 s32AoChn;
} ST_Audio_Recv_S;

static MI_S32 g_s32VideoNodeCnt = 0;
static VideoBufInfo_T g_VideoBuf;

static StreamSync_T g_StreamSync;
static st_TimeStamp_T g_stTimeStamp;

static pthread_t tid_VideoDeal;
static pthread_t tid_VideoRecv;

static ST_Audio_Send_S stAudioSend;
static ST_Audio_Recv_S stAudioRecv;

static MI_S32 g_s32VideoRecvSocket;

static void StreamDebug(MI_S32 s32Num)
{
    if (STREAM_DEBUG_FLAG)
    {
        printf("StreamDebug__(%d)__\n", s32Num);
    }
}

static void Stream_free(void *ptr, int line)
{
    if (NULL != ptr)
    {
        printf("_________FREE_________(%s):(%d)________\n", __FUNCTION__, line);
        free (ptr);
        ptr = NULL;
    }
}

MI_S32 Stream_AnalyzeRecvVideoPack(MI_S32 *s32FrameNo, MI_U8 *u8RecvBuf, MI_S32 s32BufLen)
{
    MI_S32 i,j;
    MI_S32 isFull;
    MI_S32 s32TotalPackage, s32FrameType;
    MI_S32 CurrPackage;
    MI_U16 u16TmpSeq;
    MI_U32 u32TmpTimeStamp;
    RTP_header rtp_header;

    memcpy(&rtp_header, u8RecvBuf, RTP_HEAD_SIZE);
    u16TmpSeq = rtp_header.seq_no << 8;

    u16TmpSeq |= ((rtp_header.seq_no & 0xff00) >> 8);
    rtp_header.seq_no = u16TmpSeq; //ntohs

    u32TmpTimeStamp = ((rtp_header.timestamp & 0x000000FF) << 24) + ((rtp_header.timestamp & 0x0000FF00)<< 8) +((rtp_header.timestamp  & 0x00FF0000) >> 8) + ((rtp_header.timestamp & 0xFF000000) >> 24);
    rtp_header.timestamp = u32TmpTimeStamp; //ntohl

    s32FrameType = 2;

    if (g_s32VideoNodeCnt >= MAXVIDEOPACKNUM)
    {
        g_s32VideoNodeCnt = MAXVIDEOPACKNUM;
        ST_DBG("Consumers already stopped!\n");
    }
    else
    {
        pthread_mutex_lock(&g_StreamSync.video_dec_lock);      
        isFull = 0;
        if (g_VideoBuf.u32TimeStamp != u32TmpTimeStamp)
        {
            if (g_VideoBuf.s32NaluPackSize > 0) //Process prev nalu 
            {
                g_VideoBuf.s32FrameType = s32FrameType;
                g_VideoBuf.s32FrameNo = *s32FrameNo;
                isFull = 1;
                for (i = 0; i < g_VideoBuf.TotalPackage; i++) //default  g_VideoBuf.TotalPackage == 10
                {
                    if (g_VideoBuf.CurrPackage[i] == 0)
                    {
                        isFull = 0;
                        ST_DBG("Drop frame %d~~\n", s32BufLen);
                        break;
                    }
                }
                for (i = 1;i<g_VideoBuf.TotalPackage;i++)
                {
                    if ((g_VideoBuf.CurrPackseq[i-1]+1) != g_VideoBuf.CurrPackseq[i])
                    {
                        isFull = 0;
                        ST_DBG("Packet sequence error, discard it\n");
                        break;
                    }
                }
                if (g_VideoBuf.s32NaluPackSize > MAXNALUSIZE)
                {
                    ST_DBG("g_VideoBuf too large len = %d\n", g_VideoBuf.s32NaluPackSize);
                }
                else
                {
                    if(isFull == 1)
                    {
                        if(g_VideoBuf.Mark == 1)
                        {
                            if(g_VideoBuf.VideoBuff[0] == 0x00 && g_VideoBuf.VideoBuff[1] == 0x00 &&
                                g_VideoBuf.VideoBuff[2] == 0x00 && g_VideoBuf.VideoBuff[3] == 0x01)
                            {
                                ST_SendVdecFrame(TALK_VDEC_CHN, g_VideoBuf.VideoBuff, g_VideoBuf.s32NaluPackSize);
                            }
                            else
                            {
                                ST_DBG("Not found nalu header\n");
                            }
                        }
                        else
                        {
                            ST_DBG("Seq(%d)Lost video slice, discard it\n", g_VideoBuf.s32FrameNo);
                            isFull = 0;
                        }
                    }
                }   
                *s32FrameNo++;
            }
            g_VideoBuf.s32NaluPackSize = 0; //Process Cur packet
            g_VideoBuf.u32TimeStamp = u32TmpTimeStamp;
            g_VideoBuf.seq_no = u16TmpSeq;
            g_VideoBuf.Mark = 0;

            for (i = 0; i < MAXVIDEOPACKNUM; i++)
            {
                g_VideoBuf.CurrPackage[i] = 0;
            }

            if ((s32BufLen - RTP_HEAD_SIZE) > UDP_MAX_PACKSIZE)
            {
                pthread_mutex_unlock(&g_StreamSync.video_dec_lock);
                return -1;
            }
            else
            {
                memcpy(g_VideoBuf.VideoBuff, u8RecvBuf + RTP_HEAD_SIZE, (s32BufLen - RTP_HEAD_SIZE));
            }

            CurrPackage = rtp_header.seq_no - g_VideoBuf.seq_no;
            g_VideoBuf.CurrPackage[CurrPackage] = 1;
            g_VideoBuf.CurrPackseq[CurrPackage] = rtp_header.seq_no;
            g_VideoBuf.TotalPackage = CurrPackage + 1;
            g_VideoBuf.s32NaluPackSize += (s32BufLen - RTP_HEAD_SIZE);

            if (rtp_header.marker ==1)
            {
                g_VideoBuf.Mark =rtp_header.marker;
            }
        }
        else
        {
            CurrPackage = rtp_header.seq_no - g_VideoBuf.seq_no;

            if (CurrPackage < 0)
            {
                CurrPackage = CurrPackage + 65536;
                ST_DBG("CurrPackage = %d\n", CurrPackage);
            }

            if ((g_VideoBuf.s32NaluPackSize + (s32BufLen - RTP_HEAD_SIZE)) <= MAXNALUSIZE)
            {
                memcpy(g_VideoBuf.VideoBuff + g_VideoBuf.s32NaluPackSize,
                    u8RecvBuf + RTP_HEAD_SIZE, (s32BufLen - RTP_HEAD_SIZE));
            }  
            else
            {
                for (i = 0; i < MAXVIDEOPACKNUM; i++)
                {
                    g_VideoBuf.CurrPackage[i] = 0;
                }
            }

            if ((s32BufLen - RTP_HEAD_SIZE) > UDP_MAX_PACKSIZE)
            {
                ST_DBG("Recv Udp packet too large (%d)\n", s32BufLen);
            }

            g_VideoBuf.CurrPackage[CurrPackage] = 1;
            g_VideoBuf.CurrPackseq[CurrPackage] = rtp_header.seq_no;
            g_VideoBuf.s32NaluPackSize += (s32BufLen - RTP_HEAD_SIZE);

            if (g_VideoBuf.TotalPackage < CurrPackage + 1)
            {
                g_VideoBuf.TotalPackage = CurrPackage + 1;
            }

            if (rtp_header.marker ==1)
            {
                g_VideoBuf.Mark =rtp_header.marker;
            }
        }
        pthread_mutex_unlock(&g_StreamSync.video_dec_lock);
    }
}

void *Stream_VideoRecvProcess(void *args)
{
    struct sockaddr_in c_addr;
    socklen_t addr_len;
    MI_S32 s32VideoRecvSocket;
    MI_S32 s32VideoRecvLen, s32Ret;
    MI_U8 u8FromIP[20];
    MI_U8 *u8Buf = NULL;
    MI_S32 s32FrameNo = 1;
    
    fd_set readfd;
    unsigned long maxfd = 0;
    struct timeval timeout;

    addr_len = sizeof(c_addr);
    s32VideoRecvSocket = ST_CreateVideoRecvSocket();
    StreamDebug(200);
    u8Buf = (MI_U8 *)malloc(MAXNALUSIZE);
    if (u8Buf == NULL)
    {
        close(s32VideoRecvSocket);
        ST_ERR("Malloc video recv buffer fail\n");
        return NULL;
    }
    while (g_StreamSync.s32DecodeFlag)
    {
        s32VideoRecvLen = 0;
        StreamDebug(201);
        if (s32VideoRecvSocket < 0)
        {
            s32VideoRecvLen = 0;
            ST_ERR("socket(%d) is closed\n", s32VideoRecvSocket);
            //goto exit
        }
        else
        {
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            FD_ZERO(&readfd);
            FD_SET(s32VideoRecvSocket, &readfd);
            maxfd = (s32VideoRecvSocket + 1) > maxfd?(s32VideoRecvSocket+1):maxfd;
            StreamDebug(202);
            s32Ret = select(maxfd,&readfd, NULL,  NULL, &timeout);
            if (s32Ret)
            {
                StreamDebug(203);
                if (FD_ISSET(s32VideoRecvSocket,&readfd))
                {
                    StreamDebug(204);
                    memset(u8Buf, 0x0, MAXNALUSIZE);
                    s32VideoRecvLen = recvfrom(s32VideoRecvSocket, u8Buf, (MAXNALUSIZE - 1), 0,(struct sockaddr *) &c_addr, &addr_len);
                }
            }
            s32Ret = 0;
        }
        //save es , buf+12
        
        if ((s32VideoRecvLen < MAXNALUSIZE) && (s32VideoRecvLen >= 12))
        {
            u8Buf[s32VideoRecvLen] = '\0';
            strcpy((char *)u8FromIP, inet_ntoa(c_addr.sin_addr));
            //ST_DBG("Video rcvfrom %s:%d:len = %d\n\r", inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), s32VideoRecvLen);
            
            Stream_AnalyzeRecvVideoPack(&s32FrameNo, u8Buf, s32VideoRecvLen);
        }
        else if (s32VideoRecvLen >= MAXNALUSIZE)
        {
            StreamDebug(205);
            ST_ERR("Recv video pack size larger than %d\n", MAXNALUSIZE);
            u8Buf[s32VideoRecvLen] = '\0';
        }
    }
    if (s32VideoRecvSocket)
    {
        StreamDebug(206);
        close(s32VideoRecvSocket);
        ST_DBG("close video recv sokcet(%d)..\n", s32VideoRecvSocket);
    }
    if (u8Buf)
    {
        free(u8Buf);
        u8Buf = NULL;
    }
    ST_DBG("Video recv thread  exit!!!\n");

    return NULL;
}

MI_S32 Stream_StartPlayVideo(void)
{
    int i, j;
    int EncodeType;
    int width, height;

    g_VideoBuf.s32NaluPackSize = 0;
    g_VideoBuf.u32TimeStamp = 0x1000;
    g_VideoBuf.seq_no = 0;
    //g_VideoBuf.VideoBuff = (MI_U8 *)malloc(64*1024);
    ST_DBG("Stream_StartPlayVideo xxxxx %p\n", g_VideoBuf.VideoBuff);
    StreamDebug(100);

    if (NULL == g_VideoBuf.VideoBuff)
    {
        ST_ERR("Malloc VideoBuff fail!!!\n");
        return -1;
    }

    //Stream_InitVideoList(g_VideoNode_H);

    StreamDebug(101);
    //Stream_DeleteAllVideoNode(&g_VideoNode_H);
    StreamDebug(102);
    g_s32VideoNodeCnt = 0;

    if (pthread_mutex_init (&g_StreamSync.video_dec_lock, NULL) == -1)
    {
        ST_ERR("Cannot init g_StreamSync.video_dec_lock!!!\n");
    }
    g_StreamSync.s32DecodeFlag = TRUE;
    StreamDebug(103);
    pthread_create(&tid_VideoRecv, NULL, Stream_VideoRecvProcess, NULL);
    if (tid_VideoRecv <= 0 )
    {
        ST_ERR("Create video recv thread fail !!!\n");
        g_StreamSync.s32DecodeFlag = FALSE;

        return -1;
    }
    StreamDebug(104);
}

MI_S32 Stream_StopPlayVideo(void)
{
    g_StreamSync.s32DecodeFlag = FALSE;
    StreamDebug(200);
    if (tid_VideoRecv)
    {
        if (0 == pthread_join(tid_VideoRecv, NULL))
        {
            ST_DBG("tid_VideoRecv thread join OK...\n");
            tid_VideoRecv = -1;
        }
    }
    StreamDebug(201);
    StreamDebug(202);
    pthread_mutex_destroy(&g_StreamSync.video_dec_lock);
    ST_DBG("Stream_StopPlayVideo 1111\n");
    StreamDebug(203);
    g_s32VideoNodeCnt = 0;
    if (g_VideoBuf.VideoBuff)
    {
        ST_DBG("Stream_StopPlayVideo 2222\n");
        memset(g_VideoBuf.VideoBuff, 0x0, 64*1024);
        if (g_VideoBuf.VideoBuff)
        {
            ST_DBG("Stream_StopPlayVideo aaaa %p\n", g_VideoBuf.VideoBuff);
            //Stream_free(g_VideoBuf.VideoBuff, __LINE__);
            ST_DBG("Stream_StopPlayVideo bbbb\n");
        }
        //g_VideoBuf.VideoBuff = NULL;
        ST_DBG("Stream_StopPlayVideo 3333\n");
    }
    else
    {
        ST_DBG("Stream_StopPlayVideo 44444\n");
    }
    ST_DBG("Stream_StopPlayVideo ccccc\n");

    return 0;
}

MI_S32 Stream_StartSendVideo(unsigned long IPaddr)
{
#if 0
    VencRunParam_T stVencRunPara;

    stVencRunPara.s32Socket = ST_CreateSendVideoSocket();
    stVencRunPara.s32SaveFileFlag = FALSE;
    stVencRunPara.IPaddr = IPaddr;
    stVencRunPara.s32VencChn = MAIN_STREAM_VENC;
    ST_DBG("___________Stream_StartSendVideo  socket = %d ... send ip:0x%x\n", stVencRunPara.s32Socket, IPaddr);
    if (MI_SUCCESS != ST_VencStartGetStream(&stVencRunPara))
    {
        ST_ERR("Start send video fail socket=%d\n", stVencRunPara.s32Socket);
        return -1;
    }
#endif
    return MI_SUCCESS;
}

MI_S32 Stream_StopSendVideo()
{
#if 0
    if (MI_SUCCESS != ST_VencStopGetStream())
    {
        ST_ERR("Stop send video fail !!!\n");
        return -1;
    }
#endif
    return MI_SUCCESS;
}

//========================== Add play audui file ============================
#define MI_AUDIO_SAMPLE_PER_FRAME 1024
#define DMA_BUF_SIZE_8K     (8000)
#define DMA_BUF_SIZE_16K    (16000)
#define DMA_BUF_SIZE_32K    (32000)
#define DMA_BUF_SIZE_48K    (48000)

typedef struct WAVE_FORMAT
{
    signed short wFormatTag;
    signed short wChannels;
    unsigned int dwSamplesPerSec;
    unsigned int dwAvgBytesPerSec;
    signed short wBlockAlign;
    signed short wBitsPerSample;
} WaveFormat_t;

typedef struct WAVEFILEHEADER
{
    char chRIFF[4];
    unsigned int  dwRIFFLen;
    char chWAVE[4];
    char chFMT[4];
    unsigned int  dwFMTLen;
    WaveFormat_t wave;
    char chDATA[4];
    unsigned int  dwDATALen;
} WaveFileHeader_t;

static MI_S32 g_AoReadFd = -1;
static MI_S32 g_AoDevId = 0;
static MI_S32 g_AoChn = 0;
static WaveFileHeader_t g_stWavHeaderInput;
static MI_S32 g_s32NeedSize = 0;
static pthread_t tid_playaudio;
static MI_BOOL bAoExit;
MI_U8 u8TempBuf[MI_AUDIO_SAMPLE_PER_FRAME * 2];

void* aoSendFrame(void* data)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_Frame_t stAoSendFrame;
    if (g_AoReadFd < 0)
    {
        return NULL;
    }
    bAoExit = FALSE;
    while(FALSE == bAoExit)
    {
        s32Ret = read(g_AoReadFd, &u8TempBuf, g_s32NeedSize);
        if(s32Ret != g_s32NeedSize)
        {
            lseek(g_AoReadFd, sizeof(WaveFileHeader_t), SEEK_SET);
            s32Ret = read(g_AoReadFd, &u8TempBuf, g_s32NeedSize);
            if (s32Ret < 0)
            {
                printf("Input file does not has enough data!!!\n");
                break;
            }
        }
        
        memset(&stAoSendFrame, 0x0, sizeof(MI_AUDIO_Frame_t));
        stAoSendFrame.u32Len = s32Ret;
        stAoSendFrame.apVirAddr[0] = u8TempBuf;
        stAoSendFrame.apVirAddr[1] = NULL;

        do{
            s32Ret = MI_AO_SendFrame(g_AoDevId, g_AoChn, &stAoSendFrame, -1);
        }while(s32Ret == MI_AO_ERR_NOBUF);

        if(s32Ret != MI_SUCCESS)
        {
            printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n", s32Ret);
        }
    }
    if (g_AoReadFd > 0)
    {
        close(g_AoReadFd);
    }
    return NULL;
}

MI_S32 StopPlayAudioFile(void)
{
    bAoExit = TRUE;
    pthread_join(tid_playaudio, NULL);
    ExecFunc(MI_AO_DisableChn(g_AoDevId, g_AoChn), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(g_AoDevId), MI_SUCCESS);

    return 0;
}

MI_S32 StartPlayAudioFile(const char *WavAudioFile, MI_S32 s32AoVolume)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_Attr_t stAoSetAttr, stAoGetAttr;
    MI_AO_AdecConfig_t stAoSetAdecConfig, stAoGetAdecConfig;
    MI_AO_VqeConfig_t stAoSetVqeConfig, stAoGetVqeConfig;
    MI_S32 s32AoGetVolume;
    MI_AO_ChnParam_t stAoChnParam;
    MI_U32 u32DmaBufSize;
    MI_AUDIO_SampleRate_e eAoInSampleRate = E_MI_AUDIO_SAMPLE_RATE_INVALID;

    g_AoReadFd = open((const char *)WavAudioFile, O_RDONLY, 0666);
    if(g_AoReadFd <= 0)
    {
        printf("Open input file failed:%s \n", WavAudioFile);
        printf("error:%s", strerror(errno));
        return -1;
    }

    s32Ret = read(g_AoReadFd, &g_stWavHeaderInput, sizeof(WaveFileHeader_t));
    if (s32Ret < 0)
    {
        printf("Read wav header failed!!!\n");
        return -1;
    }
    memset(&stAoSetAttr, 0x0, sizeof(MI_AUDIO_Attr_t));
    stAoSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAoSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoSetAttr.WorkModeSetting.stI2sConfig.bSyncClock = FALSE;
    stAoSetAttr.WorkModeSetting.stI2sConfig.eFmt = E_MI_AUDIO_I2S_FMT_I2S_MSB;
    stAoSetAttr.WorkModeSetting.stI2sConfig.eMclk = E_MI_AUDIO_I2S_MCLK_0;
    stAoSetAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;
    stAoSetAttr.u32ChnCnt = g_stWavHeaderInput.wave.wChannels;

    if(stAoSetAttr.u32ChnCnt == 2)
    {
        stAoSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    }
    else if(stAoSetAttr.u32ChnCnt == 1)
    {
        stAoSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    }

    stAoSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)g_stWavHeaderInput.wave.dwSamplesPerSec;
    eAoInSampleRate = (MI_AUDIO_SampleRate_e)g_stWavHeaderInput.wave.dwSamplesPerSec;

    stAoSetVqeConfig.bAgcOpen = FALSE;
    stAoSetVqeConfig.bAnrOpen = FALSE;
    stAoSetVqeConfig.bEqOpen = FALSE;
    stAoSetVqeConfig.bHpfOpen = FALSE;
    stAoSetVqeConfig.s32FrameSample = 128;
    stAoSetVqeConfig.s32WorkSampleRate = eAoInSampleRate;
    //memcpy(&stAoSetVqeConfig.stAgcCfg, &stAgcCfg, sizeof(MI_AUDIO_AgcConfig_t));
    //memcpy(&stAoSetVqeConfig.stAnrCfg, &stAnrCfg, sizeof(MI_AUDIO_AnrConfig_t));
    //memcpy(&stAoSetVqeConfig.stEqCfg, &stEqCfg, sizeof(MI_AUDIO_EqConfig_t));
    //memcpy(&stAoSetVqeConfig.stHpfCfg, &stHpfCfg, sizeof(MI_AUDIO_HpfConfig_t));

    ExecFunc(MI_AO_SetPubAttr(g_AoDevId, &stAoSetAttr), MI_SUCCESS);
    ExecFunc(MI_AO_GetPubAttr(g_AoDevId, &stAoGetAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(g_AoDevId), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(g_AoDevId, g_AoChn), MI_SUCCESS);

    if(FALSE)
    {
        ExecFunc(MI_AO_SetVqeAttr(g_AoDevId, g_AoChn, &stAoSetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_GetVqeAttr(g_AoDevId, g_AoChn, &stAoGetVqeConfig), MI_SUCCESS);
        ExecFunc(MI_AO_EnableVqe(g_AoDevId, g_AoChn), MI_SUCCESS);
    }


    ExecFunc(MI_AO_SetVolume(g_AoDevId, s32AoVolume), MI_SUCCESS);
    ExecFunc(MI_AO_GetVolume(g_AoDevId, &s32AoGetVolume), MI_SUCCESS);


    g_s32NeedSize = MI_AUDIO_SAMPLE_PER_FRAME * 2 * (stAoSetAttr.u32ChnCnt);
    if (E_MI_AUDIO_SAMPLE_RATE_8000 == stAoSetAttr.eSamplerate)
    {
        u32DmaBufSize = DMA_BUF_SIZE_8K;;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_16000 == stAoSetAttr.eSamplerate)
    {
        u32DmaBufSize = DMA_BUF_SIZE_16K;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_32000 == stAoSetAttr.eSamplerate)
    {
        u32DmaBufSize = DMA_BUF_SIZE_32K;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_48000 == stAoSetAttr.eSamplerate)
    {
        u32DmaBufSize = DMA_BUF_SIZE_48K;
    }

    if (stAoSetAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)
    {
        if (g_s32NeedSize > (u32DmaBufSize / 4))
        {
            g_s32NeedSize = u32DmaBufSize / 4;
        }
    }
    else if (stAoSetAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
    {
        if (g_s32NeedSize > (u32DmaBufSize / 8))
        {
            g_s32NeedSize = u32DmaBufSize / 8;
        }
    }

    pthread_create(&tid_playaudio, NULL, aoSendFrame, NULL);
    printf("create ao thread.\n");
}

MI_S32 Stream_EnableAiChn()
{
    MI_S32 s32Ret = MI_SUCCESS, i;

    //Ai
    MI_AUDIO_DEV AiDevId = 0;//1
    MI_AI_CHN AiChn = 0;
    MI_AUDIO_Attr_t stAiSetAttr;
    MI_SYS_ChnPort_t stAiChn0OutputPort0;
    MI_AI_VqeConfig_t stAiVqeConfig;
    MI_U32 u32AecSupfreq[6] = {20,40,60,80,100,120};
    MI_U32 u32AecSupIntensity[7] = {4,4,4,4,6,6,6};
    MI_S16 s16CompressionRatioInput[5] = {-70, -60, -30, 0, 0};
    MI_S16 s16CompressionRatioOutput[5] = {-70, -45, -18, 0, 0};

    //set ai attr
    memset(&stAiSetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAiSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAiSetAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAiSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAiSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    //stAiSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_SLAVE;
    stAiSetAttr.u32ChnCnt = 2;
    stAiSetAttr.u32FrmNum = 16;
    stAiSetAttr.u32PtNumPerFrm = 1024;

    //set ai output port depth
    memset(&stAiChn0OutputPort0, 0, sizeof(MI_SYS_ChnPort_t));
    stAiChn0OutputPort0.eModId = E_MI_MODULE_ID_AI;
    stAiChn0OutputPort0.u32DevId = AiDevId;
    stAiChn0OutputPort0.u32ChnId = AiChn;
    stAiChn0OutputPort0.u32PortId = 0;

    //ai vqe
    memset(&stAiVqeConfig, 0, sizeof(MI_AI_VqeConfig_t));
    stAiVqeConfig.bHpfOpen = FALSE;
    stAiVqeConfig.bAnrOpen = FALSE;
    stAiVqeConfig.bAgcOpen = TRUE;
    stAiVqeConfig.bEqOpen = FALSE;
    stAiVqeConfig.bAecOpen = TRUE;

    stAiVqeConfig.stAecCfg.bComfortNoiseEnable = FALSE;
    memcpy(stAiVqeConfig.stAecCfg.u32AecSupfreq, u32AecSupfreq, sizeof(u32AecSupfreq));
    memcpy(stAiVqeConfig.stAecCfg.u32AecSupIntensity,  u32AecSupIntensity, sizeof(u32AecSupIntensity));

    stAiVqeConfig.s32FrameSample = 128;
    stAiVqeConfig.s32WorkSampleRate = E_MI_AUDIO_SAMPLE_RATE_16000;

    //Hpf
    stAiVqeConfig.stHpfCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
    stAiVqeConfig.stHpfCfg.eHpfFreq = E_MI_AUDIO_HPF_FREQ_120;

    //Anr
    stAiVqeConfig.stAnrCfg.eMode= E_MI_AUDIO_ALGORITHM_MODE_USER;
    stAiVqeConfig.stAnrCfg.eNrSpeed = E_MI_AUDIO_NR_SPEED_LOW;
    stAiVqeConfig.stAnrCfg.u32NrIntensity = 5;            //[0, 30]
    stAiVqeConfig.stAnrCfg.u32NrSmoothLevel = 10;          //[0, 10]

    //Agc
    stAiVqeConfig.stAgcCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
    stAiVqeConfig.stAgcCfg.s32NoiseGateDb = -50;           //[-80, 0], NoiseGateDb disable when value = -80
    stAiVqeConfig.stAgcCfg.s32TargetLevelDb =   0;       //[-80, 0]
    stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit = 1;  //[-20, 30]
    stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax =  15; //[0, 30]
    stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin = -5; //[-20, 30]
    stAiVqeConfig.stAgcCfg.u32AttackTime = 1;              //[1, 20]
    memcpy(stAiVqeConfig.stAgcCfg.s16Compression_ratio_input, s16CompressionRatioInput, sizeof(s16CompressionRatioInput));
    memcpy(stAiVqeConfig.stAgcCfg.s16Compression_ratio_output, s16CompressionRatioOutput, sizeof(s16CompressionRatioOutput));
    stAiVqeConfig.stAgcCfg.u32DropGainMax = 60;            //[0, 60]
    stAiVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb = 10;  //[0, 100]
    stAiVqeConfig.stAgcCfg.u32ReleaseTime = 10;             //[1, 20]

    //Eq
    stAiVqeConfig.stEqCfg.eMode = E_MI_AUDIO_ALGORITHM_MODE_USER;
    for (i = 0; i < sizeof(stAiVqeConfig.stEqCfg.s16EqGainDb) / sizeof(stAiVqeConfig.stEqCfg.s16EqGainDb[0]); i++)
    {
       stAiVqeConfig.stEqCfg.s16EqGainDb[i] = 5;
    }

    ExecFunc(MI_AI_SetPubAttr(AiDevId, &stAiSetAttr), MI_SUCCESS);
    ExecFunc(MI_AI_Enable(AiDevId), MI_SUCCESS);
    ExecFunc(MI_AI_EnableChn(AiDevId, AiChn), MI_SUCCESS);

    ExecFunc(MI_AI_SetVqeVolume(AiDevId, 0, 9), MI_SUCCESS);

    s32Ret = MI_AI_SetVqeAttr(AiDevId, AiChn, 0, 0, &stAiVqeConfig);
    if (s32Ret != MI_SUCCESS)
    {
        ST_ERR("%#x\n", s32Ret);
    }
    ExecFunc(MI_AI_EnableVqe(AiDevId, AiChn), MI_SUCCESS);

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAiChn0OutputPort0,4,8), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Stream_DisableAiChn()
{
    MI_AUDIO_DEV AiDevId = 0;
    MI_AI_CHN AiChn = 0;

    ExecFunc(MI_AI_DisableVqe(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_DisableChn(AiDevId, AiChn), MI_SUCCESS);
    ExecFunc(MI_AI_Disable(AiDevId), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Stream_EnableAoChn()
{
    MI_S32 s32Ret = MI_SUCCESS;

    //Ao
    MI_AUDIO_DEV AoDevId = 0;
    MI_AI_CHN AoChn = 0;
    MI_AUDIO_Attr_t stAoSetAttr;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;

    //set ao attr
    memset(&stAoSetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stAoSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stAoSetAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_16000;
    stAoSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    stAoSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stAoSetAttr.u32ChnCnt = 1;
    stAoSetAttr.u32FrmNum = 6;
    stAoSetAttr.u32PtNumPerFrm = 1024;

    //set ao output port depth
    memset(&stAoChn0OutputPort0, 0, sizeof(MI_SYS_ChnPort_t));
    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    ExecFunc(MI_AO_SetPubAttr(AoDevId, &stAoSetAttr), MI_SUCCESS);
    ExecFunc(MI_AO_Enable(AoDevId), MI_SUCCESS);
    ExecFunc(MI_AO_EnableChn(AoDevId, AoChn), MI_SUCCESS);

    //ExecFunc(MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 2, 4), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 Stream_SetAoVolume(MI_S32 s32VolValue, MI_BOOL bMute)
{
    MI_AUDIO_DEV AoDevId = 0;
    MI_AO_SetVolume(AoDevId, s32VolValue);
    MI_AO_SetMute(AoDevId, bMute);
}

MI_S32 Stream_DiableAoChn()
{
    MI_AUDIO_DEV AoDevId = 0;
    MI_AI_CHN AoChn = 0;

    ExecFunc(MI_AO_DisableChn(AoDevId, AoChn), MI_SUCCESS);
    ExecFunc(MI_AO_Disable(AoDevId), MI_SUCCESS);
    return MI_SUCCESS;
}

static void *ST_ProcessAudioSend(void *pdata)
{
    MI_AUDIO_DEV AiDevId = 0;
    MI_AI_CHN AiChn = 0;
    MI_AUDIO_Frame_t stAudioFrame;
    MI_AUDIO_AecFrame_t stAecFrm;
    MI_S32 s32ToTalSize = 0, s32SendSocket = -1;
    MI_S32 s32Ret = 0;
    FILE *pFile = NULL;
    char szFileName[64] = {0,};

    MI_SYS_ChnPort_t stChnPort;
    MI_S32 s32Fd = -1;
    fd_set read_fds;
    struct timeval TimeoutVal;

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_AI;
    stChnPort.u32DevId = AiDevId;
    stChnPort.u32ChnId = AiChn;
    stChnPort.u32PortId = 0;
    s32Ret = MI_SYS_GetFd(&stChnPort, &s32Fd);
    s32SendSocket = ST_CreateSendAudioSocket();

    if(MI_SUCCESS != s32Ret)
    {
        ST_ERR("MI_SYS_GetFd err:%x, chn:%d\n", s32Ret, AiChn);
        return NULL;
    }
    //FILE * savefp = NULL;
    //savefp = fopen("/var/audiow.data", "w+");
    while(stAudioSend.g_AudioSendRun)
    {
        FD_ZERO(&read_fds);
        FD_SET(s32Fd, &read_fds);

        TimeoutVal.tv_sec  = 0;
        TimeoutVal.tv_usec = 200*1000;
        s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            ST_ERR("select failed!\n");
            //  usleep(10 * 1000);
            continue;
        }
        else if (s32Ret == 0)
        {
            ST_ERR("get audio in frame time out\n");
            //usleep(10 * 1000);
            continue;
        }
        else
        {
            if(FD_ISSET(s32Fd, &read_fds))
            {
                if (MI_SUCCESS == MI_AI_GetFrame(AiDevId, AiChn, &stAudioFrame, &stAecFrm, 128))//1024 / 8000 = 128ms
                {
                    if (0 == stAudioFrame.u32Len)
                    {
                        usleep(10 * 1000);
                        continue;
                    }
                    if (s32SendSocket > 0)
                    {
                        ST_Socket_UdpSend(s32SendSocket, stAudioSend.ServerIP, RECV_AUDIO_PORT,
                            (MI_U8 *)stAudioFrame.apVirAddr[0], stAudioFrame.u32Len);
                        //printf("socket(%d)...ip(0x%x)...port(%d) len(%d)\n", s32SendSocket,
                        //    stAudioSend.ServerIP, RECV_AUDIO_PORT, stAudioFrame.u32Len);
                        //if (savefp)
                        //{
                        //    fwrite(stAudioFrame.apVirAddr[0], 1, stAudioFrame.u32Len, savefp);
                        //}
                    }
                    MI_AI_ReleaseFrame(AiDevId,  AiChn, &stAudioFrame, NULL);
                }
            }
        }
    }
    if (s32SendSocket)
    {
        close(s32SendSocket);
        ST_DBG("close audio send socket(%d)..\n", s32SendSocket);
        s32SendSocket = -1;
    }
    //if (savefp)
    //{
    //    fclose(savefp);
    //}
    ST_DBG("Audio send thread  exit!!!\n");

    return NULL;
}

MI_S32 Stream_StartSendAudio(unsigned long IPaddr)
{
    stAudioSend.g_AudioSendRun = TRUE;
    Stream_EnableAiChn();
    stAudioSend.ServerIP = IPaddr;

    pthread_create(&stAudioSend.tid_AudioSend, NULL, ST_ProcessAudioSend, NULL);
    return MI_SUCCESS;
}

MI_S32 Stream_StopSendAudio()
{
    stAudioSend.g_AudioSendRun = FALSE;
    if (0 == pthread_join(stAudioSend.tid_AudioSend, NULL))
    {
        ST_DBG("Join send audio OK\n");
    }
    usleep(500*1000);
    Stream_DisableAiChn();
    stAudioSend.ServerIP = 0xFFFFFFFF;
    stAudioSend.tid_AudioSend = -1;

    return MI_SUCCESS;
}

static void *ST_ProcessAudioRecv(void *pdata)
{
    struct sockaddr_in c_addr;
    socklen_t addr_len;
    MI_S32 s32AudioRecvSocket;
    MI_S32 s32AudioRecvLen, s32Ret;
    MI_U8 *u8Buf = NULL;

    fd_set readfd;
    unsigned long maxfd = 0;
    struct timeval timeout;
    MI_AUDIO_Frame_t stAudioFrame;

    addr_len = sizeof(c_addr);
    s32AudioRecvSocket = ST_CreateAudioRecvSocket();
    StreamDebug(900);
    ST_DBG("Audio recv thread  enter!!!\n");
    u8Buf = (MI_U8 *)malloc(4096);
    if (u8Buf == NULL)
    {
        ST_ERR("Alloc audio recv buffer fail\n");
        close(s32AudioRecvSocket);
        return NULL;
    }
    //FILE * savefp = NULL;
    //savefp = fopen("/var/audior.data", "w+");
    while (stAudioRecv.g_AudioRecvRun)
    {
        s32AudioRecvLen = 0;
        StreamDebug(901);
        if (s32AudioRecvSocket < 0)
        {
            s32AudioRecvLen = 0;
            ST_ERR("socket(%d) is closed\n", s32AudioRecvSocket);
            usleep(100*1000);
            //goto exit
        }
        else
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 400*1000;
            FD_ZERO(&readfd);
            FD_SET(s32AudioRecvSocket, &readfd);
            maxfd = (s32AudioRecvSocket + 1) > maxfd?(s32AudioRecvSocket+1):maxfd;
            StreamDebug(902);
            s32Ret = select(maxfd,&readfd, NULL,  NULL, &timeout);
            if (s32Ret)
            {
                StreamDebug(903);
                if (FD_ISSET(s32AudioRecvSocket,&readfd))
                {
                    StreamDebug(904);
                    s32AudioRecvLen = recvfrom(s32AudioRecvSocket, u8Buf, 4095, 0,(struct sockaddr *) &c_addr, &addr_len);
                    if ((s32AudioRecvLen < 4096) && (s32AudioRecvLen > 0))
                    {
                        u8Buf[s32AudioRecvLen] = '\0';
                        //strcpy((char *)u8FromIP, inet_ntoa(c_addr.sin_addr));
                        //ST_DBG("Audio rcvfrom %s:%d:len = %d\n\r", inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), s32AudioRecvLen);
                        stAudioFrame.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
                        stAudioFrame.apVirAddr[0] = u8Buf;
                        stAudioFrame.u32Len = s32AudioRecvLen;
                        s32Ret = MI_AO_SendFrame(stAudioRecv.s32AoDev, stAudioRecv.s32AoChn, &stAudioFrame, 1);
                        //if (savefp)
                        //{
                        //    fwrite(stAudioFrame.apVirAddr[0], 1, stAudioFrame.u32Len, savefp);
                        //}
                    }
                    else if (s32AudioRecvLen >= 4096)
                    {
                        StreamDebug(905);
                        ST_ERR("Recv audio pack size larger than %d\n", 4096);
                        u8Buf[s32AudioRecvLen] = '\0';
                    }
                }
            }
            s32Ret = 0;
        }
        //save es , buf+12
    }
    if (s32AudioRecvSocket)
    {
        StreamDebug(906);
        close(s32AudioRecvSocket);
        ST_DBG("close audio recv socket(%d)\n", s32AudioRecvSocket);
    }
    //if (savefp)
    //{
    //    fclose(savefp);
    //}
    if (u8Buf)
    {
        free(u8Buf);
        u8Buf = NULL;
    }
    ST_DBG("Audio recv thread  exit!!!\n");

    return NULL;
}

MI_S32 Stream_StartRecvAudio(MI_S32 s32VolValue, MI_BOOL bMute)
{
    static int flag = 0;

    stAudioRecv.g_AudioRecvRun = TRUE;
    stAudioRecv.s32AoDev = 0;
    stAudioRecv.s32AoChn = 0;
    //if (!flag)
    {
        Stream_EnableAoChn();
        Stream_SetAoVolume(s32VolValue, bMute);
        flag = 1;
    }
    pthread_create(&stAudioRecv.tid_AudioRecv, NULL, ST_ProcessAudioRecv, NULL);
    return MI_SUCCESS;
}

MI_S32 Stream_StopRecvAudio()
{
    stAudioRecv.g_AudioRecvRun = FALSE;
    stAudioRecv.s32AoDev = 0;
    stAudioRecv.s32AoChn = 0;
    if (0 == pthread_join(stAudioRecv.tid_AudioRecv, NULL))
    {
        ST_DBG("Join Recv audio OK\n");
    }
    stAudioRecv.tid_AudioRecv  = -1;
    usleep(500*1000);
    Stream_DiableAoChn();

    return MI_SUCCESS;
}


