/******************************************************
*
* Copyright (c) 2006-2015 SigmaStar Semiconductor, Inc.
* All rights reserved.
*
*******************************************************
* File name:  mid_VideoEncoder.cpp
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: mixer videoEncoder source file
*
*
*
* History:
*
*    1. Date:          2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
******************************************************/
#include <iostream>
using namespace std;

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <malloc.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <math.h>
#include <stdlib.h>

#include "mid_utils.h"
#include "module_common.h"
#include "mid_common.h"
#include "mi_venc.h"
#include "mi_divp.h"
#include "PacketModule.h"
#include "List.h"

#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
#include "mi_vdisp.h"
#include "mi_vdisp_datatype.h"
#endif
#include "mi_sys.h"
#include "mid_VideoEncoder.h"
#include "mid_video_type.h"
#include "mid_venc_type.h"
#include "mid_venc.h"
#include "module_record.h"
#include "mid_vif.h"
#include "module_record.h"


const static MI_U16 Video4kWidth = 3840;
const static MI_U16 Video4kHeight = 2160;
const static MI_U16 Video2kWidth = 2688;
const static MI_U16 Video2kHeight = 1520;
const static MI_U16 Video3mWidth169 = 2304;
const static MI_U16 Video3mHeight169 = 1296;
const static MI_U16 Video3mWidth43 = 2048;
const static MI_U16 Video3mHeight43 = 1520;
const static MI_U16 Video2mWidth43 = 1920;
const static MI_U16 Video2mHeight43 = 1080;
const static MI_U16 Video2mWidth11 = 1440;
const static MI_U16 Video2mHeight11 = 1440;
const static MI_U16 Video960pWidth = 1280;
const static MI_U16 Video960pHeight = 960;
const static MI_U16 Video720pWidth = 1280;
const static MI_U16 Video720pHeight = 720;

const static MI_U16 VideoD1WidthN = 704;
const static MI_U16 VideoD1HeightN = 480;
const static MI_U16 VideoD1WidthP = 704;
const static MI_U16 VideoD1HeightP = 576;

const static MI_U16 VideoVGAWidthN = 640;
const static MI_U16 VideoVGAHeightN = 360;
const static MI_U16 VideoVGAWidthP = 640;
const static MI_U16 VideoVGAHeightP = 480;

const static MI_U16 VideoCIFWidthN = 320;
const static MI_U16 VideoCIFHeightN = 240;
const static MI_U16 VideoCIFWidthP = 352;
const static MI_U16 VideoCIFHeightP = 288;

#define MAKE_YUYV_VALUE(y,u,v)    ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK                MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE                MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                  MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN                MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE                 MAKE_YUYV_VALUE(29,225,107)

const MI_U32 u32MaxConsumerCount = 5;

BOOL g_ShowFrameInterval = FALSE;

extern BOOL g_bBiLinear;
extern int g_videoNumber;
extern VencRcParamEx_t g_videoRcParam[MAX_VIDEO_NUMBER];


extern MI_S32 g_s32DivpChnIndex;
extern int gDebug_VideoStreamLog;
extern MI_S32 g_SuperFrameEnable;


#define JPEG_PATH                     "sanpshot"

#define DEFAULT_MAX_BUFFER_COUNT      1000
#define DEFAULT_BUFFER_SIZE           (2048 * 1024)
#define DEFAULT_MJPEG_QFACTOR         60

#define DEFAULT_VIDEO_STREAM_BUF_SIZE "2000000"

#define SHOW_FPS_INTERVAL             2 //second
#define MIXER_VIDEO_ENABLE_SELECT     1

MI_U64 MI_VideoEncoder::fStreamTimestamp = 0;
#if TARGET_CHIP_I6 || TARGET_CHIP_I5
MI_U32 MI_VideoEncoder::vifframeRate = 25;
MI_U32 MI_VideoEncoder::vpeframeRate = 25;
#else
MI_U32 MI_VideoEncoder::vifframeRate = 30;
MI_U32 MI_VideoEncoder::vpeframeRate = 30;
#endif

MI_SYS_ChnPort_t MI_VideoEncoder::VifChnPort = {E_MI_MODULE_ID_VIF, 0, 0, 0};
MI_SYS_ChnPort_t MI_VideoEncoder::VpeChnPortTop = {E_MI_MODULE_ID_VPE, 0, 0, 0};
MI_SYS_ChnPort_t MI_VideoEncoder::VpeChnPortBottom = {E_MI_MODULE_ID_VPE, 0, 0, 0};
MI_VPE_HDRType_e MI_VideoEncoder::eHDRType = E_MI_VPE_HDR_TYPE_OFF;
MI_VPE_3DNR_Level_e MI_VideoEncoder::e3DNRLevel = E_MI_VPE_3DNR_LEVEL_OFF;

#if TARGET_CHIP_I6B0
MI_U32 MI_VideoEncoder::u32RealDivpChn = 0xff;
BOOL MI_VideoEncoder::bRealDivpInit = FALSE;
#endif
#if TARGET_CHIP_I6B0 || TARGET_CHIP_I6 || TARGET_CHIP_I6E
BOOL MI_VideoEncoder::bVpePort2share = FALSE;
#endif

extern int g_bUseTimestampVideo;

extern int fps_display(VI_CHN viChn, char *string);
extern int gop_display(VI_CHN viChn, char *string);
extern int bitrate_display(VI_CHN viChn, char *string);
#if TARGET_CHIP_I6B0
MI_BOOL checkLastRealDivp(MI_U32 chn);
#endif


MI_VideoConsumer::MI_VideoConsumer()
{
    ConsumerCount = 0x0;

    INIT_LIST_HEAD(&mConsumerList);
}

MI_VideoConsumer::~MI_VideoConsumer()
{
    VideoConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, VideoConsumer, ConsumerList);
        if(NULL != tmp)
        {
            list_del(&tmp->ConsumerList);
            tmp->tChannel = NULL;
            free(tmp);
            tmp = NULL;
        }
    }
    m_Mutex.OnUnLock();
    INIT_LIST_HEAD(&mConsumerList);
    ConsumerCount = 0x0;
}

MI_BOOL MI_VideoConsumer::AddConsumer(VideoChannelFrame &tframe)
{
    if(ConsumerCount >= u32MaxConsumerCount)
    {
        MIXER_ERR("Consumer count[%d] should not more than %d!!!\n", ConsumerCount, u32MaxConsumerCount);
        return FALSE;
    }

    VideoConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    //
    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, VideoConsumer, ConsumerList);

        if(NULL != tmp && NULL != tmp->tChannel && tmp->tChannel == &tframe)
        {
            MIXER_ERR("%p has already register in VideoConsumer\n", &tframe);
            m_Mutex.OnUnLock();
            return TRUE;
        }
    }

    tmp = (VideoConsumer *)malloc(sizeof(VideoConsumer));
    if(NULL == tmp)
    {
        MIXER_ERR("can not malloc VideoConsumer\n");
        m_Mutex.OnUnLock();
        return FALSE;
    }
    INIT_LIST_HEAD(&tmp->ConsumerList);
    tmp->tChannel =(VideoChannelFrame *)&tframe;

    //debug info
    //MIXER_DBG("tChannel is %p\n", tmp->tChannel);

    list_add_tail(&tmp->ConsumerList, &mConsumerList);

    ConsumerCount++;
    m_Mutex.OnUnLock();
    return TRUE;
}

MI_BOOL MI_VideoConsumer::DelConsumer(VideoChannelFrame &tframe)
{
    VideoConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, VideoConsumer, ConsumerList);

        if(NULL != tmp && NULL != tmp->tChannel && tmp->tChannel == &tframe)
        {
            MIXER_DBG("fine it\n");

            list_del(&tmp->ConsumerList);
            tmp->tChannel = NULL;
            free(tmp);
            tmp = NULL;
            ConsumerCount--;
            break;
        }
    }
    m_Mutex.OnUnLock();

    return TRUE;
}

MI_U32 MI_VideoConsumer::GetConsumerCount()
{
    MI_U32 tmp=0x0;
    m_Mutex.OnLock();
    tmp = ConsumerCount;
    m_Mutex.OnUnLock();

    return tmp;
}

MI_U32 MI_VideoConsumer::Consumer(const FrameInf_t &pFrameInf)
{
    VideoConsumer *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    m_Mutex.OnLock();

    list_for_each_safe(pHead, ptmp, &mConsumerList)
    {
        tmp = list_entry(pHead, VideoConsumer, ConsumerList);
        //MIXER_DBG("tmp is %p\n", tmp);
        if(NULL != tmp && NULL != tmp->tChannel)
        {
            //MIXER_DBG("tChannel111 is %p\n", tmp->tChannel);
            tmp->tChannel->OnData(pFrameInf);
        }
    }
    m_Mutex.OnUnLock();

    return TRUE;
}

#if 0
static MI_U32 Get_Frame_Interval(MI_U32 u32FrameRate)
{
    MI_U32 getInterval = 0;
    MI_U32 den = 0;
    MI_U32 num = 0;

    if(u32FrameRate & 0xffff0000)
    {
        if(u32FrameRate& 0xffff0000)
        {
            getInterval = 1000000 / u32FrameRate & 0x0000ffff;
        }
        else
        {
            den = (u32FrameRate & 0xffff0000) >> 16;
            num = u32FrameRate & 0x0000ffff;
            getInterval = 1000000 * den / num;
        }
    }
    else
    {
        getInterval = 1000000 / u32FrameRate;
    }

    return getInterval;
}
#endif

MI_VideoEncoder* MI_VideoEncoder::createNew(const int streamId)
{
    MI_VideoEncoder *videoEncoder = NULL;
    do
    {
        videoEncoder = new MI_VideoEncoder(streamId);

        return videoEncoder;
    }while(0);

    return NULL;
}

MI_VideoEncoder::MI_VideoEncoder(const int streamId)
	: m_thread_exit(FALSE)
	, m_bDivpInit(FALSE)
	, m_bDivpFixed(FALSE)
	, m_bUseDivp(FALSE)
	, m_eVpeOutportPixelFormat(E_MI_SYS_PIXEL_FRAME_FORMAT_MAX)
	, m_veChn(-1)
	, m_width(0)
	, m_height(0)
	, m_widthmax(1920)
	, m_heightmax(1080)
	, m_VpeOutputWidth(1920)
	, m_VpeOutputHeight(1080)
	, m_vpeBufUsrDepth(2)
	, m_vpeBufCntQuota(5)
	, m_vencBufUsrDepth(0)
	, m_vencBufCntQuota(0)
	, m_divpBufUsrDepth(0)
	, m_divpBufCntQuota(0)
	, m_DivpSclUpChnId(-1)
	, m_DivpSclDownChnId(-1)
	, m_snapDynamicallyMode(FALSE)
	, m_bChangeRes(FALSE)
{
    m_qfactor = DEFAULT_MJPEG_QFACTOR;
    m_videoBufferMaxCount = DEFAULT_MAX_BUFFER_COUNT;
    m_viChnStatus = VI_ILLEGAL;
    m_initRotate = E_MI_SYS_ROTATE_NONE;
    m_pipCfg = 0x00;
    m_startGetFrame = TRUE;
    m_pipRectX = 0x00;
    m_pipRectY = 0x00;
    m_pipRectH = 0x00;
    m_pipRectW = 0x00;
    m_bindMode = Mixer_Venc_Bind_Mode_NUM;
    m_MaxQfactor = 0x00;
    m_virtualIInterval = 0;
    m_64mMemoryMode = 0x00;
    m_minPQp = 0x0;
    m_maxIQp = 0x00;
    m_gop = 0x0;
    m_rateCtlType = 0x00;
    m_virtualIEnable = FALSE;
    m_snapInterval = 0x00;
    m_IPQPDelta = 0x00;
    m_snapLastTimeStamp = 0x00;
    m_MinQfactor = 0x00;
    m_maxPQp = 0x00;
    m_minIQp = 0x00;
    m_bitRate = 0x00;
    m_disposable = FALSE;
    m_snapNumber = 0x00;
    #if TARGET_CHIP_I6 || TARGET_CHIP_I5
            m_VideoProfile = 1;
    #else
        m_VideoProfile = 2;
    #endif
    m_encoderType = VE_TYPE_MAX;
    m_getStreamThread = (pthread_t)(-1);
    m_videoInfoCaculator = new VideoInfoCalculator();
    if(NULL == m_videoInfoCaculator)
    {
        MIXER_ERR("err, can not alloc m_videoInfoCaculator\n");
    }

    ClearThreadState();
    m_vencGetStreamStatus = FALSE;
    m_vencframeRate = 0;

    memset(&m_VpeChnPort,  0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&m_VencChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
    memset(&m_DivpChnPort, 0x00, sizeof(MI_SYS_ChnPort_t));
	m_DivpChnPort.u32ChnId = 0xff;

    if(m_encoderType == VE_JPG || (m_encoderType == VE_MJPEG && m_64mMemoryMode == TRUE))
    {
        m_snapDynamicallyMode = FALSE;
    }
}

MI_VideoEncoder::~MI_VideoEncoder()
{
    if(m_videoInfoCaculator != NULL)
    {
        delete m_videoInfoCaculator;
        m_videoInfoCaculator = NULL;
    }
}

void* Stream_Task(void *argv)
{
    MI_VideoEncoder* m_videoEncoder = (MI_VideoEncoder*)argv;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack[3];
    MI_VENC_ChnStat_t stStat;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 i = 0x0;
    struct timeval curTimestamp;
    //frame rate test start
    struct timeval curStamp, lastStamp,curYuvStamp;
    struct timeval lastFpsTimestamp = {0};
    MI_U64 lastBufferTime = 0, curBufferTime = 0;
//    MI_U32 oldFrameRate = m_videoEncoder->m_vencframeRate;

    MI_U8 *pStreamAddr[10] = {0x0};
    MI_U32 StreamLen[10] = {0x0};
    MI_U32 tmpStreamLen=0x0;

    FILE* pframeSpacedevioutfile = NULL;
    FILE* pFramePTSFile = NULL;

    MI_U64 lastFramePTS = 0;
    MI_U64 currentPTS = 0;
    MI_U32 frameType = 0;
    MI_S32 streamType = 0;
    MI_U64 varianceRealTimeSum = 0;
    MI_U64 variancePtsTimeSum = 0;
    MI_U32 uFrameTimeBase = 0;
    MI_U64 uTimeSum = 0;
    MI_U64 k = 0;
    MI_SYS_BufInfo_t stYuvBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 s32YuvRet = -1;
     MI_SYS_ChnPort_t stYuvChnOutputPort;
     void *pIspBuffer = NULL;
#if MIXER_VIDEO_ENABLE_SELECT
    struct timeval TimeoutVal;
    MI_S32 VideoFd = -1;
    fd_set read_fds;
#endif
    memset(&stYuvBufInfo,0,sizeof(MI_SYS_BufInfo_t));

     if((VE_YUV420 == m_videoEncoder->m_encoderType))
     {
        //if(m_videoEncoder->m_VpeChnPort.u32PortId >= (MAX_VPE_PORT_NUMBER-1))    //yuv from divp
        if(m_videoEncoder->m_bDivpInit)
        {
            stYuvChnOutputPort.eModId = m_videoEncoder->m_DivpChnPort.eModId;
            stYuvChnOutputPort.u32DevId =m_videoEncoder->m_DivpChnPort.u32DevId;
            stYuvChnOutputPort.u32ChnId = m_videoEncoder->m_DivpChnPort.u32ChnId;
            stYuvChnOutputPort.u32PortId = m_videoEncoder->m_DivpChnPort.u32PortId;
        }
        else        //yuv from vpe port
        {
            stYuvChnOutputPort.eModId = m_videoEncoder->m_VpeChnPort.eModId;
            stYuvChnOutputPort.u32DevId =m_videoEncoder->m_VpeChnPort.u32DevId;
            stYuvChnOutputPort.u32ChnId = m_videoEncoder->m_VpeChnPort.u32ChnId;
            stYuvChnOutputPort.u32PortId = m_videoEncoder->m_VpeChnPort.u32PortId;
        }
        MI_VPE_Alloc_IspDataBuf(sizeof(IspFrameMetaInfo_t), &pIspBuffer);
    }

    //Frame spacing is calculated based on frame rate
    uFrameTimeBase = (1000 * 1000) / m_videoEncoder->m_vencframeRate;

    if(FALSE == m_videoEncoder->m_vencGetStreamStatus)
    {
        m_videoEncoder->m_vencGetStreamStatus = TRUE;
    }
    else if(TRUE == m_videoEncoder->m_vencGetStreamStatus)
    {
        printf("%s: VideoEncoder Stream_Task%d is already running\n", __func__, m_videoEncoder->m_veChn);
        pthread_exit(NULL);
        return NULL;
    }

    if(TRUE == m_videoEncoder->m_vencGetStreamStatus)
    {
        switch(m_videoEncoder->m_encoderType)
        {
            case VE_AVC:
                printf("VideoEncoder Stream_Task%d:encoderType %d(H264)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            case VE_H265:
                printf("VideoEncoder Stream_Task%d:encoderType %d(H265)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            case VE_MJPEG:
                printf("VideoEncoder Stream_Task%d:encoderType %d(MJPEG)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            case VE_YUV420:
                printf("VideoEncoder Stream_Task%d:encoderType %d(YUV420)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            case VE_JPG:
                printf("VideoEncoder Stream_Task%d:encoderType %d(JPG)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            case VE_JPG_YUV422:
                printf("VideoEncoder Stream_Task%d:encoderType %d(JPG_YUV422)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                break;
            default:
                printf("VideoEncoder Stream_Task%d:encoderType %d(Not recognized)\n", m_videoEncoder->m_veChn, m_videoEncoder->m_encoderType);
                return NULL;
        }
    }

    //A frame with 25 FPS inter frame spacing of 40*1000us
    uFrameTimeBase = (40 * 1000 * m_videoEncoder->m_vencframeRate) / 25;

    gettimeofday(&lastStamp, 0);
#if MIXER_VIDEO_ENABLE_SELECT
    if(VE_YUV420 != m_videoEncoder->m_encoderType)
    {

        VideoFd = MI_VENC_GetFd(m_videoEncoder->m_veChn);
        if(VideoFd <= 0)
        {
            MIXER_ERR("VideoEncoder Stream_Task%d unable to get vencFd:%d\n", m_videoEncoder->m_veChn, VideoFd);
            return NULL;
        }
        else
        {
            printf("VideoEncoder Stream_Task%d get vencFd:%d\n", m_videoEncoder->m_veChn, VideoFd);
        }

    }
    else  //get yuv fd from sys
    {
        if(MI_SUCCESS != MI_SYS_GetFd(&stYuvChnOutputPort, &VideoFd))
        {
            MIXER_ERR("VideoEncoder Stream_Task%d unable to get VideoFd:%d\n", m_videoEncoder->m_veChn, VideoFd);
                   return NULL;
        }
        else
            {
                printf("VideoEncoder Stream_Task%d get VideoFd:%d\n", m_videoEncoder->m_veChn, VideoFd);
            }
    }
#endif

    if(g_SuperFrameEnable &&(m_videoEncoder->m_encoderType != VE_YUV420))
    {
        MI_VENC_SuperFrameCfg_t stSuperFrameCfg;

        stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE;
        stSuperFrameCfg.u32SuperIFrmBitsThr = (m_videoEncoder->m_bitRate / SUPERIPFRMBITDEN) * SUPERIFRMBITMOL;
        stSuperFrameCfg.u32SuperPFrmBitsThr = (m_videoEncoder->m_bitRate / SUPERIPFRMBITDEN) * SUPERPFRMBITMOL;
        stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;

        s32Ret = MI_VENC_SetSuperFrameCfg(m_videoEncoder->m_veChn, &stSuperFrameCfg);
        if(s32Ret)
        {
            printf("%s %d MI_VENC_SetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
        }
        else
        {
            printf("MI_VENC_SetSuperFrameCfg Success, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                            stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
        }
    }

    while(m_videoEncoder->m_vencGetStreamStatus)
    {
        if((g_ShowFrameInterval) && (NULL == pFramePTSFile)&&(VE_YUV420 != m_videoEncoder->m_encoderType) && (VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
        {
            char frameRateName[256];
            char frameSpacedeviName[256];

            sprintf(frameRateName, "framerate%d.txt", m_videoEncoder->m_veChn);
            pFramePTSFile = fopen(frameRateName, "wb");

            sprintf(frameSpacedeviName, "frameSpacedevi%d.txt", m_videoEncoder->m_veChn);
            pframeSpacedevioutfile = fopen(frameSpacedeviName, "wb");
            MIXER_INFO("Stream_Task: outfile = %p pframeSpacedevioutfile:%p \n", pFramePTSFile, pframeSpacedevioutfile);
        }

#if MIXER_VIDEO_ENABLE_SELECT
        //if((VE_YUV420 != m_videoEncoder->m_encoderType) && (VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
        {
            FD_ZERO(&read_fds);
            FD_SET(VideoFd, &read_fds);

            //gettimeofday(&TimeoutVal, 0); //select() no need to get current time
            TimeoutVal.tv_sec  = 0;
            TimeoutVal.tv_usec = 500*1000;//select() wait for 300ms

            s32Ret = select(VideoFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
            if (s32Ret < 0)
            {
                MIXER_ERR("VideoEncoder Stream_Task%d select failed\n", m_videoEncoder->m_veChn);
                MySystemDelay(5);
                continue;
            }
            else if (0 == s32Ret)
            {
                //printf("VideoEncoder Stream_Task%d select timeout\n", m_videoEncoder->m_veChn);
                //MySystemDelay(5);
                 continue;
            }
            else if(!FD_ISSET(VideoFd, &read_fds))
            {
                printf("VideoEncoder Stream_Task%d not detect venc envent!\n", m_videoEncoder->m_veChn);
                //MySystemDelay(5);
                continue;
            }

            FD_CLR(VideoFd, &read_fds);
        }

#endif

        switch(m_videoEncoder->m_encoderType)
        {
            case VE_JPG:
         case VE_MJPEG:
            case VE_AVC:
            case VE_H265:
                {
                    memset(&stStat, 0, sizeof(stStat));
                    s32Ret = MI_VENC_Query(m_videoEncoder->m_veChn, &stStat);
                    if((s32Ret != MI_SUCCESS) || (0 == stStat.u32CurPacks))
                    {
                        MySystemDelay(5);
                        continue;
                    }

                        pthread_mutex_lock(&m_videoEncoder->m_stopMutex);
                         stStream.u32PackCount = stStat.u32CurPacks ;
                        stStream.pstPack = stPack;
                        s32Ret = MI_VENC_GetStream(m_videoEncoder->m_veChn, &stStream, FALSE);

                    if(MI_SUCCESS == s32Ret)
                    {
                        //pthread_mutex_lock(&m_videoEncoder->m_stopMutex);
                        gettimeofday(&curStamp, NULL);

                        curBufferTime = stStream.pstPack[0].u64PTS;

                        if(gDebug_VideoStreamLog)
                        {
                            printf("[F]%x=%d enc:%d len:%d\n", (unsigned int)m_videoEncoder, m_videoEncoder->m_veChn, \
                                                                m_videoEncoder->m_encoderType, stStream.pstPack[0].u32Len);
                        }

                        if(g_bUseTimestampVideo)
                        {
                            gettimeofday(&curTimestamp, NULL);
                        }
                        else
                        {
                            if(0 == lastFramePTS)
                            {
                                lastFramePTS = stStream.pstPack[0].u64PTS;
                                gettimeofday(&curTimestamp, NULL);
                                currentPTS = (curTimestamp.tv_sec * 1000000LL) + curTimestamp.tv_usec;
                                MIXER_DBG("%s, %lu.%03ld, currentPTS<%llu>, lastFramePTS<%llu>\r\n", __func__, curTimestamp.tv_sec, curTimestamp.tv_usec, currentPTS, lastFramePTS);
                            }
                            else
                            {
                                MI_S64 diffPTS = stStream.pstPack[0].u64PTS- lastFramePTS;

                                if(0 > diffPTS)
                                {
                                    gettimeofday(&curTimestamp, NULL);
                                    currentPTS = (curTimestamp.tv_sec * 1000000LL) + curTimestamp.tv_usec;
                                }
                                else
                                {
                                    currentPTS += diffPTS;
                                    curTimestamp.tv_sec = currentPTS / 1000000LL;
                                    curTimestamp.tv_usec = currentPTS % 1000000LL;
                                }

                                lastFramePTS = stStream.pstPack[0].u64PTS;
                              /*  if(0 == m_videoEncoder->getStreamId())
                                {
                                    MIXER_DBG("%s, %lu.%06ld, currentPTS<%llu>, lastFramePTS<%llu>, diffPTS<%lld>\r\n", __func__, curTimestamp.tv_sec, curTimestamp.tv_usec, currentPTS, lastFramePTS, diffPTS);
                                }*/
                            }
                        }
                        if(g_bUseTimestampVideo)
                        {
                            MIXER_DBG("video chn = %d, s: %ld, us: %ld\n", m_videoEncoder->m_veChn, curTimestamp.tv_sec, curTimestamp.tv_usec);
                        }

                        switch(m_videoEncoder->m_encoderType)
                        {
                            case VE_AVC:
                                frameType = VIDEO_PLAY_LOAD_H264;
                                streamType = stStream.pstPack[0].stDataType.eH264EType;
                                break;
                            case VE_H265:
                                frameType = VIDEO_PLAY_LOAD_H265;
                                streamType = stStream.pstPack[0].stDataType.eH265EType;
                                break;
                            case VE_MJPEG:
                            case VE_JPG:
                                  frameType = VIDEO_PLAY_LOAD_JPEG;
                                streamType = stStream.pstPack[0].stDataType.eJPEGEType;
                                break;
                            default:
                                break;
                        }

                        tmpStreamLen = 0x0;
                        memset(&pStreamAddr[0], 0, sizeof(pStreamAddr[0]) * 10);
                        memset(StreamLen, 0, sizeof(StreamLen));
                        for(i = 0x0; i < stStream.u32PackCount; i++)
                        {
                          pStreamAddr[i] = stStream.pstPack[i].pu8Addr;
                          StreamLen[i] = stStream.pstPack[i].u32Len;
                          tmpStreamLen += StreamLen[i] ;
                        }

                     FrameInf_t  FrameVideo;
                     if(0 < tmpStreamLen)
                     {
                            CPacket * pPacket = NULL;
                           //PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());

                            pPacket = (CPacket *)g_PacketModule->MallocPacket(tmpStreamLen);
                            if(NULL == pPacket)
                            {
                                MIXER_ERR("can not alloc pPacket, size(%d)\n", tmpStreamLen);
                                MI_VENC_ReleaseStream(m_videoEncoder->m_veChn, &stStream);
                                pthread_mutex_unlock(&m_videoEncoder->m_stopMutex);
                                continue;
                            }
                            pPacket->AddRef();

                            //memset(pPacket->GetBuffer(), 0x0, tmpStreamLen);

                            //MIXER_ERR("packet(%p) alloc size(%d)\n", pPacket->GetBuffer() , tmpStreamLen);
                            MI_U32 offset = 0x0;
                            for(i=0x0; i<stStream.u32PackCount; i++)
                            {
                                memcpy(pPacket->GetBuffer() + offset, stStream.pstPack[i].pu8Addr, StreamLen[i]);
                                 offset += StreamLen[i];

                    //MIXER_ERR("packet(%p) alloc size(%d), Streadm[%d]:%d\n", pPacket->GetBuffer() , tmpStreamLen , i , StreamLen[i]);
                            }

                          FrameVideo.nVideoPlayLoad = (VIDEO_PLAY_LOAD_E)frameType;

                          if(VIDEO_PLAY_LOAD_H264 == frameType)
                          {
                            if(E_MI_VENC_H264E_NALU_ISLICE == stStream.pstPack[0].stDataType.eH264EType)
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_I;
                            else
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_P;
                          }
                          else if(VIDEO_PLAY_LOAD_H265 == frameType)
                          {
                            if(E_MI_VENC_H265E_NALU_ISLICE == stStream.pstPack[0].stDataType.eH265EType)
                            {
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_I;
                            }
                            else{
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_P;
                            }
                          }
                          else if(VIDEO_PLAY_LOAD_JPEG == frameType)
                          {
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_JPEG;
                          }
                          else
                          {
                                FrameVideo.nFrameType = VIDEO_FRAME_TYPE_YUV;
                          }

                          FrameVideo.nLen = tmpStreamLen;
                          FrameVideo.Ch = m_videoEncoder->m_veChn;

                          if(0x0 == m_videoEncoder->m_veChn)
                            FrameVideo.StreamType = VIDEO_MAIN_STREAM;
                          else if(0x01 == m_videoEncoder->m_veChn)
                            FrameVideo.StreamType = VIDEO_SUB_STREAM;
                          else if(0x02 == m_videoEncoder->m_veChn)
                            FrameVideo.StreamType = VIDEO_THRD_STREAM;
                          else if(0x03 == m_videoEncoder->m_veChn)
                              FrameVideo.StreamType = VIDEO_SNAP_STREAM;
                          else if(0x04 == m_videoEncoder->m_veChn)
                          FrameVideo.StreamType = VIDEO_YUV_STREAM;

                          FrameVideo.nPts = stStream.pstPack[0].u64PTS;
                          FrameVideo.pPacketAddr = (void*)pPacket;

                        #if 0
                                  //m_videoEncoder->GetLiveChannelFrameNode().OnData(FrameVideo);
                        #else
                        /*MIXER_DBG("ch(%d), consumer count is %d\n", m_videoEncoder->m_veChn,\
                                                                m_videoEncoder->GetLiveChannelConsumer().GetConsumerCount());*/
                        m_videoEncoder->GetLiveChannelConsumer().Consumer(FrameVideo);
                        #endif
                        pPacket->Release();
                        }
                //if(m_videoEncoder->m_veChn == 1)
                //MIXER_DBG("ch(%d), streamtype is %d, eH265EType:%d\n", m_videoEncoder->m_veChn, streamType, stStream.pstPack[0].stDataType.eH264EType);
                        m_videoEncoder->m_videoInfoCaculator->IncFrmCnt(tmpStreamLen, streamType, m_videoEncoder->m_encoderType,curBufferTime);
                        MI_VENC_ReleaseStream(m_videoEncoder->m_veChn, &stStream);
                        pthread_mutex_unlock(&m_videoEncoder->m_stopMutex);
                    }
                    else
                    {
                        pthread_mutex_unlock(&m_videoEncoder->m_stopMutex);

                       /* if(VE_JPG != m_videoEncoder->m_encoderType)
                        {
                            MySystemDelay(5);
                        }*/
                        float realFPS = (float)(m_videoEncoder->m_vencframeRate & 0xFFFF) / (m_videoEncoder->m_vencframeRate >> 16 == 0 ? 1 : m_videoEncoder->m_vencframeRate >> 16);

                        if(realFPS > 1)
                        {
                            MIXER_INFO(" %s: get stream timeout e=%d vechn=%d\n", __func__ , m_videoEncoder->m_encoderType, m_videoEncoder->m_veChn);
                        }
                    }
                }
                break;
            case VE_YUV420:
            case VE_JPG_YUV422:
              {
                FrameInf_t  FrameVideo;
                MI_S32 ret = -1;
                tmpStreamLen = 0x0;
                CPacket * pPacket = NULL;
                gettimeofday(&curYuvStamp, NULL);
                s32YuvRet = MI_SYS_ChnOutputPortGetBuf(&stYuvChnOutputPort , &stYuvBufInfo, &hHandle);
                if(MI_SUCCESS == s32YuvRet)
                {
                    if(gDebug_VideoStreamLog)
                    {
                        printf("getbuf sucess, size(%dx%d), stride(%d, %d, %d), Pixel %d, viraddr(%p, %p, %p)\n", stYuvBufInfo.stFrameData.u16Width, stYuvBufInfo.stFrameData.u16Height,
                                stYuvBufInfo.stFrameData.u32Stride[0], stYuvBufInfo.stFrameData.u32Stride[1], stYuvBufInfo.stFrameData.u32Stride[2], stYuvBufInfo.stFrameData.ePixelFormat,
                                stYuvBufInfo.stFrameData.pVirAddr[0], stYuvBufInfo.stFrameData.pVirAddr[1], stYuvBufInfo.stFrameData.pVirAddr[2]);
                    }
               }
               if(MI_SUCCESS == s32YuvRet)
               {
                  ret =  pthread_mutex_trylock(&m_videoEncoder->m_stopMutex);
                 if(ret == EBUSY)
                 {
                    ret = -1;
                 }
                 else if(ret != 0)
                 {
                    ret = -1;
                  }
                 else
                 {
                    memset(&FrameVideo,0,sizeof(FrameInf_t));
                    frameType = VIDEO_PLAY_LOAD_YUV;
                               //PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());
                    StreamLen[0] = stYuvBufInfo.stFrameData.u32Stride[0]*stYuvBufInfo.stFrameData.u16Height;
                    if((VE_YUV420 == m_videoEncoder->m_encoderType))
                    {
                        StreamLen[1] = StreamLen[0]/2;
                    }
                    else
                    {
                        StreamLen[1] = StreamLen[0];
                    }
                    for(i = 0; i < 2; i++)
                    {
                         tmpStreamLen += StreamLen[i];
                    }
                    pPacket = (CPacket *)g_PacketModule->MallocPacket(tmpStreamLen);
                    if(NULL == pPacket)
                    {
                        MIXER_ERR("can not alloc pPacket, size(%d)\n", tmpStreamLen);
                        if(MI_SUCCESS == s32YuvRet)
                        {
                          MI_SYS_ChnOutputPortPutBuf(hHandle);
                        }
                        pthread_mutex_unlock(&m_videoEncoder->m_stopMutex);
                        continue;
                    }
                    pPacket->AddRef();
                    memset(pPacket->GetBuffer(), 0x0, tmpStreamLen);
                    //MIXER_ERR("packet(%p) alloc size(%d)\n", pPacket->GetBuffer() , tmpStreamLen);
                    for(i=0x0; i<2; i++)
                    {
                      memcpy(pPacket->GetBuffer() + i*StreamLen[0], (MI_U8*)stYuvBufInfo.stFrameData.pVirAddr[i], StreamLen[i]);
                      //  MIXER_ERR("packet(%p) alloc size(%d), Streadm[%d]:%d\n", stYuvBufInfo.stFrameData.pVirAddr[i], tmpStreamLen , i , StreamLen[i]);
                    }
                    FrameVideo.nPts = (curYuvStamp.tv_sec * 1000000LL) + curYuvStamp.tv_usec;
                    if(TRUE == g_RecordManager->bRecOpen[m_videoEncoder->m_veChn] && VE_YUV420 == m_videoEncoder->m_encoderType && NULL != pIspBuffer)
                    {
                        int nGet = 30;
                        MI_S32 s32Rete = MI_ISP_OK;
                        do
                        {
                            ((IspFrameMetaInfo_t*)pIspBuffer)->u64Pts = stYuvBufInfo.u64Pts;
                            s32Rete = MI_ISP_GetFrameMetaInfo(m_videoEncoder->m_VpeChnPort.u32ChnId, (IspFrameMetaInfo_t*)pIspBuffer);
                             if(MI_ISP_OK == s32Rete)
                            {
                                MIXER_DBG("get frame meta info ok, pts: %llu, u32Shutter = %u, u32SensorGain = %u, u32ColorTmp = %u, curr pts: %llu, get times: %d\n", ((IspFrameMetaInfo_t*)pIspBuffer)->u64Pts, ((IspFrameMetaInfo_t*)pIspBuffer)->u32Shutter, ((IspFrameMetaInfo_t*)pIspBuffer)->u32SensorGain, ((IspFrameMetaInfo_t*)pIspBuffer)->u32ColorTmp, FrameVideo.nPts, 30 - nGet);
                                break;
                            }
                            usleep(10000);
                        }while(nGet-- > 0);
                        if(MI_ISP_OK != s32Rete)
                            MIXER_ERR("get frame meta info failed, ret: %x, pts: %llu, m_videoEncoder->m_veChn: %d, curr pts: %llu\n", s32Rete, ((IspFrameMetaInfo_t*)pIspBuffer)->u64Pts, m_videoEncoder->m_VpeChnPort.u32ChnId, FrameVideo.nPts);

                    }
                    FrameVideo.nVideoPlayLoad = (VIDEO_PLAY_LOAD_E)frameType;
                    FrameVideo.nFrameType = VIDEO_FRAME_TYPE_YUV;
                    FrameVideo.nLen = tmpStreamLen;
                    FrameVideo.Ch = m_videoEncoder->m_veChn;

                    FrameVideo.StreamType = VIDEO_YUV_STREAM;

                    FrameVideo.pPacketAddr = (void*)pPacket;
                    m_videoEncoder->GetLiveChannelConsumer().Consumer(FrameVideo);
                    pPacket->Release();
                    MI_SYS_ChnOutputPortPutBuf(hHandle);
                    pthread_mutex_unlock(&m_videoEncoder->m_stopMutex);
                     MySystemDelay(10);
                 }
               }
               else
               {
                 MySystemDelay(5);
               }
              break;
             }
            default:
                break;
        }

        if((MI_SUCCESS == s32Ret)&&(VE_YUV420 != m_videoEncoder->m_encoderType)&&(VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
        {
            if((g_ShowFrameInterval) && (pFramePTSFile))
            {
                MI_S64 realDiffTime = 0;
                MI_S64 bufTSDiffTime = 0;
                MI_U32 realDiffTimeTem = 0;
                MI_U32 bufTSDiffTimeTem = 0;
                float fps = 0;

                fprintf(pFramePTSFile, "realDiff: %d \n bufTSDiff: %llu\n", (int)(((curStamp.tv_sec - lastStamp.tv_sec) * 1000000L
                        + curStamp.tv_usec) - lastStamp.tv_usec), curBufferTime - lastBufferTime);

                if(0 == lastFpsTimestamp.tv_sec && 0 == lastFpsTimestamp.tv_usec)
                {
                    lastFpsTimestamp.tv_sec = curStamp.tv_sec;
                    lastFpsTimestamp.tv_usec = curStamp.tv_usec;
                }

                if(curStamp.tv_sec - lastFpsTimestamp.tv_sec >= SHOW_FPS_INTERVAL)
                {
                    fps = m_videoEncoder->m_videoInfoCaculator->GetFps();

                    if(fps > 0)
                    {
                        fprintf(pFramePTSFile, "fps: %f\n", fps);
                    }

                    lastFpsTimestamp.tv_sec = curStamp.tv_sec;
                    lastFpsTimestamp.tv_usec = curStamp.tv_usec;
                }

                if(lastBufferTime != 0)
                {
                    realDiffTime = (((curStamp.tv_sec - lastStamp.tv_sec) * 1000000L + curStamp.tv_usec) - lastStamp.tv_usec);
                    bufTSDiffTime = (curBufferTime  - lastBufferTime);

                    //Difference absolute value between realDiffTime and uFrameTimeBase
                    realDiffTimeTem = abs(realDiffTime - uFrameTimeBase);
                    bufTSDiffTimeTem = abs(bufTSDiffTime - uFrameTimeBase);

                    //Sum of variance
                    varianceRealTimeSum += (realDiffTimeTem * realDiffTimeTem);
                    variancePtsTimeSum += (bufTSDiffTimeTem * bufTSDiffTimeTem);

                    uTimeSum += realDiffTime;
                    k++;

                    //The standard deviation within the 1s output to the file
                    if(uTimeSum > 1000000L)
                    {
                        fprintf(pframeSpacedevioutfile, "realDiff standard deviation: %.2f\n bufTSDiff standard deviation: %.2f\n", (sqrt(varianceRealTimeSum / k)) / 1000, (sqrt(variancePtsTimeSum / k)) / 1000);
                        varianceRealTimeSum = 0;
                        variancePtsTimeSum = 0;
                        k = 0;
                        uTimeSum = 0;
                    }
                }

                lastStamp.tv_sec = curStamp.tv_sec;
                lastStamp.tv_usec = curStamp.tv_usec;
                lastBufferTime = curBufferTime;
            }

        }

        //frame rate test start
        if((FALSE == g_ShowFrameInterval) && (NULL != pFramePTSFile))
        {
            if(NULL != pFramePTSFile)
            {
                fflush(pFramePTSFile);
                fclose(pFramePTSFile);
                pFramePTSFile = NULL;
            }

            if(NULL != pframeSpacedevioutfile)
            {
                fflush(pframeSpacedevioutfile);
                fclose(pframeSpacedevioutfile);
                pframeSpacedevioutfile = NULL;
            }
        }
    }


#if MIXER_VIDEO_ENABLE_SELECT
#if TARGET_CHIP_I5
    if((VE_YUV420 != m_videoEncoder->m_encoderType)&&(VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
    {
      if(MI_SUCCESS != (MI_VENC_CloseFd(m_videoEncoder->m_veChn)))
      {
        MIXER_ERR("close fd err, venc %d\n", m_videoEncoder->m_veChn);
      }
    }
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    if((VE_YUV420 != m_videoEncoder->m_encoderType)&&(VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
    {
      if(MI_SUCCESS != (MI_VENC_CloseFd(m_videoEncoder->m_veChn)))
      {
        MIXER_ERR("close fd err, venc %d\n", m_videoEncoder->m_veChn);
      }
    }
    else if((VE_YUV420 == m_videoEncoder->m_encoderType) && MI_SUCCESS != (MI_SYS_CloseFd(VideoFd)))
    {
          MIXER_ERR("close yuv fd err, channel: %d\n", m_videoEncoder->m_veChn);
    }
#endif
    //FD_CLR(vencFd, &read_fds);
    if(/*(VE_YUV420 != m_videoEncoder->m_encoderType)&&*/(VE_JPG_YUV422 != m_videoEncoder->m_encoderType))
    {
      FD_ZERO(&read_fds);
    }
#endif

    //frame rate test start
    if((g_ShowFrameInterval) && (pFramePTSFile))
    {
        if(NULL != pFramePTSFile)
        {
            fflush(pFramePTSFile);
            fclose(pFramePTSFile);
            pFramePTSFile = NULL;
        }

        if(NULL != pframeSpacedevioutfile)
        {
            fflush(pframeSpacedevioutfile);
            fclose(pframeSpacedevioutfile);
            pframeSpacedevioutfile = NULL;
        }
    }

    //m_videoEncoder->ClearThreadState();
    if((VE_YUV420 == m_videoEncoder->m_encoderType))
    {
        MI_VPE_Free_IspDataBuf(pIspBuffer);
    }

    MIXER_DBG("%s %d veChn = %d exit\n", __func__, __LINE__, m_videoEncoder->m_encoderType);
    pthread_exit(NULL);
    return NULL;
}

int isCharParam(char *op)
{
    int isChar = 0;

    if(0 == strcmp(op, "MI_VENC_InsertUserData"))
    {
        isChar = 1;
    }

    return isChar;
}

int paserParam(char* cmd, char *op, int *param, char** charParam)
{
    char *sub = NULL;
    int paramNum = 0;
    int isStr = 0;
    sub = strtok(cmd, ",");

    if(NULL == sub)
    {
        printf(" %s: get operation error\n", __func__);
        paramNum = -1;
        return paramNum;
    }

    strncpy(op, sub, 50);
    printf(" %s: op = %s\n", __func__, op);

    isStr = isCharParam(op);

    sub = strtok(NULL, ",");

    while(sub)
    {
        if(isStr)
        {
            charParam[paramNum] = sub;
            printf(" %s: charParam[%d] = %s\n", __func__, paramNum, charParam[paramNum]);
        }
        else
        {
            param[paramNum] = atoi(sub);
            printf(" %s: param[%d] = %d\n", __func__, paramNum, param[paramNum]);
        }

        paramNum++;
        sub = strtok(NULL, ",");
    }

    return paramNum;
}


/***************************************************
*
* s32Mode = 0, unbind vdisp/divp/vpe to venc
* s32Mode = 1, bind   vdisp/divp/vpe to venc
*
***************************************************/
MI_S32 MI_VideoEncoder::initDivpAndVdisp(MI_S32 s32Mode)
{
    MI_S32 result = -1;
    MI_SYS_WindowRect_t stCropRect;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;

    if(TRUE == m_bDivpInit)
    {
        MIXER_ERR("Video-%d: The Divp has been initialed, please call uninitVideo() first!\n", m_veChn);
        return result;
    }
#if TARGET_CHIP_I6B0
    if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && m_DivpChnPort.u32ChnId == MI_VideoEncoder::u32RealDivpChn && MI_VideoEncoder::bRealDivpInit == TRUE)
    {
        //dont need create divp and bind divp.
        if(1 == s32Mode)
        {
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_DivpChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_DivpChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_DivpChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = vpeframeRate;//m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;

            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }
        MIXER_DBG("Video-%d Not init Divp and vdisp,because it is real mode.\n", m_veChn);
        m_bDivpInit = TRUE;
        return 0;
    }
#endif
 #if TARGET_CHIP_I5 || TARGET_CHIP_I6E
    if(m_pipCfg)
    {
        /* PIP flow introduction:

                             /-> DIVP CHNL0 \
            SNR -> VIF -> VPE                -> VDISP
                             \-> DIVP CHNL1 /

           DIVP CHNL0 is used for crop and scale up.
           DIVP CHNL1 is used for scale down.
        */

        /************************************************
          Step1:  create divp for scale up PIP image
        *************************************************/
        stCropRect.u16X = ALIGN_2xUP(m_pipRectX);
        stCropRect.u16Y = ALIGN_2xUP(m_pipRectY);
        stCropRect.u16Width  = ALIGN_2xUP(m_pipRectW);
        stCropRect.u16Height = ALIGN_2xUP(m_pipRectH);

        stDivpPortAttr.u32Width  = ALIGN_2xUP(m_width);
        stDivpPortAttr.u32Height = ALIGN_2xUP(m_height);
        stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpPortAttr.ePixelFormat = m_eVpeOutportPixelFormat;

        m_DivpSclUpChnId = Mixer_Divp_GetChannleNum();

        MIXER_DBG("Divp chnId:%d Crop_X:%d Crop_Y:%d  Crop_W:%d Crop_H:%d,  Dst_W:%d Dst_H:%d\n", m_DivpSclUpChnId,
            stCropRect.u16X,stCropRect.u16Y, stCropRect.u16Width,stCropRect.u16Height, stDivpPortAttr.u32Width, stDivpPortAttr.u32Height);

        MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_DivpSclUpChnId, E_MI_SYS_ROTATE_NONE, &stCropRect));
        MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_DivpSclUpChnId, &stDivpPortAttr));
        MIXERCHECKRESULT(Mixer_Divp_StartChn(m_DivpSclUpChnId));

        MI_SYS_ChnPort_t stDivpChnPort;

        memset(&stDivpChnPort, 0, sizeof(MI_SYS_ChnPort_t));
        stDivpChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stDivpChnPort.u32DevId = 0;
        stDivpChnPort.u32ChnId = m_DivpSclUpChnId;
        stDivpChnPort.u32PortId = 0;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stDivpChnPort, m_divpBufUsrDepth, m_divpBufCntQuota), MI_SUCCESS);

        // bind VPE to divp
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = stDivpChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId  = stDivpChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = stDivpChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = stDivpChnPort.u32PortId;

#if TARGET_CHIP_I6E
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif

        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


        /************************************************
          Step2:  create divp for scale down orign image
        *************************************************/
        stCropRect.u16X = 0;
        stCropRect.u16Y = 0;
        stCropRect.u16Width  = ALIGN_2xUP(m_width);
        stCropRect.u16Height = ALIGN_2xUP(m_height);

        stDivpPortAttr.u32Width  = ALIGN_2xUP(m_pipRectW);
        stDivpPortAttr.u32Height = ALIGN_2xUP(m_pipRectH);
        stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpPortAttr.ePixelFormat = m_eVpeOutportPixelFormat;

        m_DivpSclDownChnId = Mixer_Divp_GetChannleNum();

        MIXER_DBG("Divp chnId:%d Crop_X:%d Crop_Y:%d  Crop_W:%d Crop_H:%d,  Dst_W:%d Dst_H:%d\n", m_DivpSclDownChnId,
            stCropRect.u16X,stCropRect.u16Y, stCropRect.u16Width,stCropRect.u16Height, stDivpPortAttr.u32Width, stDivpPortAttr.u32Height);

        MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_DivpSclDownChnId, E_MI_SYS_ROTATE_NONE, &stCropRect));
        MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_DivpSclDownChnId, &stDivpPortAttr));
        MIXERCHECKRESULT(Mixer_Divp_StartChn(m_DivpSclDownChnId));

        stDivpChnPort.u32ChnId = m_DivpSclDownChnId;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stDivpChnPort, m_divpBufUsrDepth, m_divpBufCntQuota), MI_SUCCESS);

        // bind VPE to divp
        stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclDownChnId;

#if TARGET_CHIP_I6E
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


        /************************************************
          Step3:  init vdisp for PIP
        *************************************************/
    #if TARGET_CHIP_I5
        MI_VDISP_InputPortAttr_t stInputPortAttr;
    #elif TARGET_CHIP_I6E
        MI_VDISP_InputChnAttr_t stInputPortAttr;
    #endif

        MI_VDISP_OutputPortAttr_t stOutputPortAttr;
        MI_SYS_ChnPort_t ChnPort;

        ExecFunc(MI_VDISP_Init(), MI_SUCCESS);
        ExecFunc(MI_VDISP_OpenDevice(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);

        //initial vdisp inputport Attr for PIP
     #if TARGET_CHIP_I5
        memset(&stInputPortAttr, 0x00, sizeof(MI_VDISP_InputPortAttr_t));
    #elif TARGET_CHIP_I6E
        memset(&stInputPortAttr, 0x00, sizeof(MI_VDISP_InputChnAttr_t));
    #endif
        stInputPortAttr.u32OutX = 0;
        stInputPortAttr.u32OutY = 0;
        stInputPortAttr.u32OutWidth  = ALIGN_2xUP(m_width);
        stInputPortAttr.u32OutHeight = ALIGN_2xUP(m_height);
        printf("%s:%d =====>In0_X:%d In0_Y:%d  In0_W:%d In0_H:%d\n",__func__,__LINE__, stInputPortAttr.u32OutX,stInputPortAttr.u32OutY,
                            stInputPortAttr.u32OutWidth,stInputPortAttr.u32OutHeight);

        stInputPortAttr.s32IsFreeRun = TRUE;
    #if TARGET_CHIP_I5
        ExecFunc(MI_VDISP_SetInputPortAttr(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP, &stInputPortAttr), MI_SUCCESS);
    #elif TARGET_CHIP_I6E
       ExecFunc(MI_VDISP_SetInputChannelAttr(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP, &stInputPortAttr), MI_SUCCESS);
    #endif
        //initial vdisp inputport Attr for origin image
#if TARGET_CHIP_I5
        memset(&stInputPortAttr, 0x00, sizeof(MI_VDISP_InputPortAttr_t));
#elif TARGET_CHIP_I6E
        memset(&stInputPortAttr, 0x00, sizeof(MI_VDISP_InputChnAttr_t));
#endif
        stInputPortAttr.u32OutWidth  = ALIGN_2xUP(stDivpPortAttr.u32Width);
        stInputPortAttr.u32OutHeight = ALIGN_2xUP(stDivpPortAttr.u32Height);
        stInputPortAttr.u32OutX = ALIGN_2xUP(m_width  - stInputPortAttr.u32OutWidth);
        stInputPortAttr.u32OutY = ALIGN_2xUP(m_height - stInputPortAttr.u32OutHeight);
        printf("%s:%d =====>In16_X:%d In16_Y:%d  In16_W:%d In16_H:%d\n",__func__,__LINE__, stInputPortAttr.u32OutX,stInputPortAttr.u32OutY,
                            stInputPortAttr.u32OutWidth,stInputPortAttr.u32OutHeight);

        stInputPortAttr.s32IsFreeRun = TRUE;
    #if TARGET_CHIP_I5
        ExecFunc(MI_VDISP_SetInputPortAttr(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_VDISP, &stInputPortAttr), MI_SUCCESS);
    #elif TARGET_CHIP_I6E
        ExecFunc(MI_VDISP_SetInputChannelAttr(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_CHNID_FOR_VDISP, &stInputPortAttr), MI_SUCCESS);
    #endif
        //initial vdisp outputport Attr
        memset(&stOutputPortAttr, 0x00, sizeof(MI_VDISP_OutputPortAttr_t));
        stOutputPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stOutputPortAttr.u32BgColor = YUYV_BLACK;
        stOutputPortAttr.u32FrmRate = 0; //m_vencframeRate; //0:vdisp use sys for frame rate control

        stOutputPortAttr.u32Width  = ALIGN_2xUP(m_width);
        stOutputPortAttr.u32Height = ALIGN_2xUP(m_height);
        ExecFunc(MI_VDISP_SetOutputPortAttr(MIXER_VDISP_DEVID_FOR_PIP, 0, &stOutputPortAttr), MI_SUCCESS);

        ChnPort.eModId    = E_MI_MODULE_ID_VDISP;
        ChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
        ChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
        ChnPort.u32PortId = 0; //mainPort;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&ChnPort, m_divpBufUsrDepth, m_divpBufCntQuota), MI_SUCCESS);


        /************************************************
          Step4:  bind divp to vdisp
        *************************************************/
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId  = 0;
        stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclUpChnId;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
        stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
        stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
        stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;
        stBindInfo.u32SrcFrmrate = m_vencframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
#if TARGET_CHIP_I6E
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

        stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclDownChnId;

    #if TARGET_CHIP_I5
        stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
        stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
    #elif TARGET_CHIP_I6E
        stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_VDISP;
        stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
    #endif
#if TARGET_CHIP_I6E
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif

        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    #if TARGET_CHIP_I5
        ExecFunc(MI_VDISP_EnableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
        ExecFunc(MI_VDISP_EnableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_VDISP), MI_SUCCESS);
    #elif TARGET_CHIP_I6E
        ExecFunc(MI_VDISP_EnableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
        ExecFunc(MI_VDISP_EnableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_CHNID_FOR_VDISP), MI_SUCCESS);
    #endif
        ExecFunc(MI_VDISP_StartDev(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);

        if(1 == s32Mode)
        {
             memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stSrcChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32PortId = 0; //MIXER_VDISP_PORTID_FOR_PIP;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
#if TARGET_CHIP_I6E
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif

            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }

        m_bDivpInit = TRUE;
    }
    else if((TRUE == g_bBiLinear) && ((m_width <= 720) || (m_height <= 576)) &&
            (0 < m_veChn) && (E_MI_MODULE_ID_DISP != m_DivpChnPort.eModId) && (FALSE == m_bDivpFixed))
    {
        m_DivpChnPort.eModId    = E_MI_MODULE_ID_DIVP;
        m_DivpChnPort.u32DevId  = 0;
		if(0xff == m_DivpChnPort.u32ChnId)
        	m_DivpChnPort.u32ChnId  = Mixer_Divp_GetChannleNum();
        m_DivpChnPort.u32PortId = 0;
        m_divpBufUsrDepth = MIXER_DIVP_BUFUSRDEPTH_DEF;
        m_divpBufCntQuota = MIXER_DIVP_BUFCNTQUOTA_DEF;

        stCropRect.u16X = 0;
        stCropRect.u16Y = 0;
        stCropRect.u16Width  = 1280; //ALIGN_2xUP(m_width);
        stCropRect.u16Height = 720; //ALIGN_2xUP(m_height);

        stDivpPortAttr.u32Width  = ALIGN_2xUP(m_width);
        stDivpPortAttr.u32Height = ALIGN_2xUP(m_height);
        stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpPortAttr.ePixelFormat = m_eVpeOutportPixelFormat;

        MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_DivpChnPort.u32ChnId, E_MI_SYS_ROTATE_NONE, &stCropRect));
        MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_DivpChnPort.u32ChnId, &stDivpPortAttr));
        MIXERCHECKRESULT(Mixer_Divp_StartChn(m_DivpChnPort.u32ChnId));

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&m_DivpChnPort, m_divpBufUsrDepth, m_divpBufCntQuota), MI_SUCCESS);

        // bind VPE to divp
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = m_DivpChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId  = m_DivpChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = m_DivpChnPort.u32PortId;

        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

        if(1 == s32Mode)
        {
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_DivpChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_DivpChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_DivpChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }

        m_bDivpInit = TRUE;
    }
    else if((TRUE == g_bBiLinear) && ((m_width > 720) || (m_height > 576)) &&
            (0 < m_veChn) && (E_MI_MODULE_ID_DIVP == m_DivpChnPort.eModId) && (FALSE == m_bDivpFixed))
    {
        m_DivpChnPort.eModId    = E_MI_MODULE_ID_MAX;
        m_DivpChnPort.u32DevId  = 0;
        m_DivpChnPort.u32ChnId  = 0;
        m_DivpChnPort.u32PortId = 0;
        m_divpBufUsrDepth = 0;
        m_divpBufCntQuota = 0;

        // bind VPE to venc
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

        m_bDivpInit = FALSE;
    }
    else
#endif
    if(m_bUseDivp)
    {
    	if(0xff == m_DivpChnPort.u32ChnId)
        	m_DivpChnPort.u32ChnId  = Mixer_Divp_GetChannleNum();
         memset(&stCropRect, 0x0, sizeof(stCropRect));
        //stCropRect.u16X = 0;
        //stCropRect.u16Y = 0;
#if TARGET_CHIP_I5
        if((TRUE == g_bBiLinear) && ((m_width <= 720) || (m_height <= 576)))
        {
            stCropRect.u16Width  = 1280; //ALIGN_2xUP(m_width);
            stCropRect.u16Height = 720; //ALIGN_2xUP(m_height);
        }
        else
#endif
        {
            //mervyn:without crop,it is unless.
            //stCropRect.u16Width  = ALIGN_2xUP(m_width);
            //stCropRect.u16Height = ALIGN_2xUP(m_height);
        }

        stDivpPortAttr.u32Width  = ALIGN_2xUP(m_width);
        stDivpPortAttr.u32Height = ALIGN_2xUP(m_height);
#if TARGET_CHIP_I6E || TARGET_CHIP_I6 || TARGET_CHIP_I6B0
        stDivpPortAttr.u32Width  = ALIGN_16xUP(m_width);
#endif
        stDivpPortAttr.eCompMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stDivpPortAttr.ePixelFormat = m_eVpeOutportPixelFormat;

        MIXER_DBG("divp chnid: %d. m_width=%d  m_height=%d stCropRect.u16Width=%d stCropRect.u16Height=%d\n", m_DivpChnPort.u32ChnId,m_width,m_height,stCropRect.u16Width,stCropRect.u16Height);

	    MIXER_DBG("input stDivpPortAttr u32Width=%d  u32Height=%d\n",stDivpPortAttr.u32Width, stDivpPortAttr.u32Height);

		MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_DivpChnPort.u32ChnId, E_MI_SYS_ROTATE_NONE, &stCropRect));
        MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_DivpChnPort.u32ChnId, &stDivpPortAttr));
        MIXERCHECKRESULT(Mixer_Divp_StartChn(m_DivpChnPort.u32ChnId));

        if(VE_YUV420 == m_encoderType)
        {
            m_divpBufUsrDepth = m_divpBufCntQuota = MIXER_DIVP_BUFCNTQUOTA_DEF;
        }
        else
        {
            m_divpBufUsrDepth = MIXER_DIVP_BUFUSRDEPTH_DEF;
            m_divpBufCntQuota = MIXER_DIVP_BUFCNTQUOTA_DEF;
        }

        MI_SYS_ChnPort_t stDivpChnPort;

        memset(&stDivpChnPort, 0, sizeof(MI_SYS_ChnPort_t));
        stDivpChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stDivpChnPort.u32DevId = 0;
        stDivpChnPort.u32ChnId = m_DivpChnPort.u32ChnId;
        stDivpChnPort.u32PortId = 0;

        ExecFunc(MI_SYS_SetChnOutputPortDepth(&stDivpChnPort, m_divpBufUsrDepth, m_divpBufCntQuota), MI_SUCCESS);

        // bind VPE to divp
        memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = stDivpChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId  = stDivpChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = stDivpChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = stDivpChnPort.u32PortId;


#if TARGET_CHIP_I6
        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#elif TARGET_CHIP_I6E
        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#elif TARGET_CHIP_I6B0
        if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
        {
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = vpeframeRate;            //when real mode DIVP ,vpe bind divp == (sensorfps:sensorfps)
        }
        else
        {
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
        }
        MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
#endif

        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

        if(1 == s32Mode)
        {
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;


#if TARGET_CHIP_I6
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#elif TARGET_CHIP_I6E
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#elif TARGET_CHIP_I6B0
            if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
            {
                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
            }
            else
            {
                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
            }
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }

        m_bDivpInit = TRUE;
#if TARGET_CHIP_I6B0
        if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && m_DivpChnPort.u32ChnId == MI_VideoEncoder::u32RealDivpChn)
        {
            MI_VideoEncoder::bRealDivpInit = TRUE;
        }
#endif
    }
    else
    {
        if(1 == s32Mode)
        {
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
#if TARGET_CHIP_I6
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            if( Mixer_Venc_Bind_Mode_HW_RING == m_bindMode && m_VpeChnPort.u32PortId == 1)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
            }
            else if( Mixer_Venc_Bind_Mode_REALTIME == m_bindMode && m_VpeChnPort.u32PortId == 0)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
            }
#elif TARGET_CHIP_I6E
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            if( Mixer_Venc_Bind_Mode_REALTIME == m_bindMode && m_VpeChnPort.u32PortId == 0)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
            }

#elif TARGET_CHIP_I6B0
            if( Mixer_Venc_Bind_Mode_HW_RING == m_bindMode && m_VpeChnPort.u32PortId == 1)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
                stBindInfo.u32BindParam = m_height;
            }
            else if(Mixer_Venc_Bind_Mode_HW_HALF_RING == m_bindMode && m_VpeChnPort.u32PortId == 1)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_HW_RING;
                stBindInfo.u32BindParam = m_height/2;
            }
            else if( Mixer_Venc_Bind_Mode_REALTIME == m_bindMode && m_VpeChnPort.u32PortId == 0)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
                stBindInfo.u32BindParam = 0;
            }
            else
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
                stBindInfo.u32BindParam = 0;
            }
#endif
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }

        MIXER_DBG("Video-%d Not init Divp and Vdisp\n", m_veChn);
    }

    return result;
}

#if TARGET_CHIP_I6E
MI_S32 LdcParseLibarayCfgFilePath(char *pLdcLibCfgPath, mi_eptz_config_param *ptconfig_para)
{
    mi_eptz_err err_state = MI_EPTZ_ERR_NONE;

    err_state = mi_eptz_config_parse(pLdcLibCfgPath, ptconfig_para);
    if (err_state != MI_EPTZ_ERR_NONE)
    {
        printf("confile file read error: %d\n", err_state);
        return err_state;
    }

    printf("ldc mode %d \n", ptconfig_para->ldc_mode);
    return 0;
}

MI_S32 LdcGetCfgViewNum(mi_LDC_MODE eLdcMode)
{
    MI_U32 u32ViewNum = 0;

    switch(eLdcMode)
    {
        case LDC_MODE_1R:
        case LDC_MODE_1P_CM:
        case LDC_MODE_1P_WM:
        case LDC_MODE_1O:
        case LDC_MODE_1R_WM:
            u32ViewNum = 1;
            break;
        case LDC_MODE_2P_CM:
        case LDC_MODE_2P_DM:
            u32ViewNum = 2;
            break;
        case LDC_MODE_4R_CM:
        case LDC_MODE_4R_WM:
            u32ViewNum = 4;
            break;
        default:
            printf("########### ldc mode %d err \n", eLdcMode);
            break;
    }

    printf("view num %d \n", u32ViewNum);
    return u32ViewNum;
}


MI_S32 LdcLibarayCreatBin(MI_S32 s32ViewId, mi_eptz_config_param *ptconfig_para,
                          LDC_BIN_HANDLE *ptldc_bin, MI_U32 *pu32LdcBinSize, MI_S32 s32Rot)
{
    unsigned char* pWorkingBuffer;
    int working_buf_len = 0;
    mi_eptz_err err_state = MI_EPTZ_ERR_NONE;
    EPTZ_DEV_HANDLE eptz_handle = NULL;

    mi_eptz_para teptz_para;
    memset(&teptz_para, 0x0, sizeof(mi_eptz_para));

    printf("view %d rot %d\n", s32ViewId, s32Rot);

    working_buf_len = mi_eptz_get_buffer_info(ptconfig_para);
    pWorkingBuffer = (unsigned char*)malloc(working_buf_len);
    if (pWorkingBuffer == NULL)
    {
        printf("buffer allocate error\n");
        return MI_EPTZ_ERR_MEM_ALLOCATE_FAIL;
    }

   // printf("%s:%d working_buf_len %d \n", __FUNCTION__, __LINE__, working_buf_len);

    //EPTZ init
    teptz_para.ptconfig_para = ptconfig_para; //ldc configure

  //  printf("%s:%d ptconfig_para %p, pWorkingBuffer %p, working_buf_len %d\n", __FUNCTION__, __LINE__, teptz_para.ptconfig_para,
    //    pWorkingBuffer, working_buf_len);

    eptz_handle =  mi_eptz_runtime_init(pWorkingBuffer, working_buf_len, &teptz_para);
    if (eptz_handle == NULL)
    {
        printf("EPTZ init error\n");
        return MI_EPTZ_ERR_NOT_INIT;
    }

    teptz_para.pan = 0;
    teptz_para.tilt = -60;
    if(ptconfig_para->ldc_mode == 1)
        teptz_para.tilt = 60;
    teptz_para.rotate = s32Rot;
    teptz_para.zoom = 150.00;
    teptz_para.out_rot = 0;
    teptz_para.view_index = s32ViewId;

    //Gen bin files from 0 to 360 degree
    switch (ptconfig_para->ldc_mode)
    {
        case LDC_MODE_4R_CM:  //LDC_MODE_4R_CM/Desk, if in desk mount mode, tilt is nagetive.
            teptz_para.view_index = s32ViewId;
            teptz_para.pan = 0;
            teptz_para.tilt = -50; //In CM mode, tilt is positive, but in desk mode, tilt is negative.
            teptz_para.rotate = s32Rot;
            teptz_para.zoom = 150;
            teptz_para.out_rot = 0;
            err_state = (mi_eptz_err)mi_eptz_runtime_map_gen(eptz_handle,(mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;
        case LDC_MODE_4R_WM:  //LDC_MODE_4R_WM
            teptz_para.view_index = s32ViewId;
            teptz_para.pan = 0;
            teptz_para.tilt = 50; //In CM mode, tilt is positive, but in desk mode, tilt is negative.
            teptz_para.rotate = s32Rot;
            teptz_para.zoom = 150;
            teptz_para.out_rot = 0;
            err_state = (mi_eptz_err)mi_eptz_runtime_map_gen(eptz_handle,(mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;
        case LDC_MODE_1R:  //LDC_MODE_1R CM/Desk,  if in desk mount mode, tilt is negative.
            teptz_para.view_index = s32ViewId;
            teptz_para.pan = 0;
            teptz_para.tilt = 0; //In CM mode, tilt is positive, but in desk mode, tilt is negative.
            teptz_para.rotate = s32Rot;
            teptz_para.zoom = 150;
            teptz_para.out_rot = 0;
            err_state = (mi_eptz_err)mi_eptz_runtime_map_gen(eptz_handle,(mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;
        case LDC_MODE_1R_WM:  //LDC_MODE_1R WM
            teptz_para.view_index = s32ViewId;
            teptz_para.pan = 0;
            teptz_para.tilt = 50; //In CM mode, tilt is positive, but in desk mode, tilt is negative.
            teptz_para.rotate = s32Rot;
            teptz_para.zoom = 150;
            teptz_para.out_rot = 0;

            err_state = (mi_eptz_err)mi_eptz_runtime_map_gen(eptz_handle,(mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;

        case LDC_MODE_2P_CM:  //LDC_MODE_2P_CM
        case LDC_MODE_2P_DM:  //LDC_MODE_2P_DM
        case LDC_MODE_1P_CM:  //LDC_MODE_1P_CM
            //Set the input parameters for donut mode
            if(s32Rot > 180)
            {
                //Degree 180 ~ 360
                teptz_para.view_index = s32ViewId;
                teptz_para.r_inside = 550;
                teptz_para.r_outside = 10;
                teptz_para.theta_start = s32Rot;
                teptz_para.theta_end = s32Rot+360;
            }
            else
            {
                //Degree 180 ~ 0
                teptz_para.view_index = s32ViewId;
                teptz_para.r_inside = 10;
                teptz_para.r_outside = 550;
                teptz_para.theta_start = s32Rot;
                teptz_para.theta_end = s32Rot+360;
            }
            err_state = (mi_eptz_err)mi_donut_runtime_map_gen(eptz_handle, (mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;
        case LDC_MODE_1P_WM:  //LDC_MODE_1P wall mount.
            teptz_para.view_index = s32ViewId;
            teptz_para.pan = 0;
            teptz_para.tilt = 0;
            teptz_para.zoom_h = 100;
            teptz_para.zoom_v = 100;
            err_state = (mi_eptz_err)mi_erp_runtime_map_gen(eptz_handle,(mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[EPTZ ERR] =  %d !! \n", err_state);
            }
            break;
        case LDC_MODE_1O:    //bypass mode
            teptz_para.view_index = 0; //view index
            printf("begin mi_bypass_runtime_map_gen \n");
            err_state = (mi_eptz_err)mi_bypass_runtime_map_gen(eptz_handle, (mi_eptz_para*)&teptz_para, ptldc_bin, (int *)pu32LdcBinSize);
            if (err_state != MI_EPTZ_ERR_NONE)
            {
                printf("[MODE %d ERR] =  %d !! \n", LDC_MODE_1O, err_state);
                return err_state;
            }

            printf("end mi_bypass_runtime_map_gen\n");
            break;
        default :
             printf("********************err ldc mode %d \n", ptconfig_para->ldc_mode);
             return 0;
    }

    free(pWorkingBuffer);

    return 0;
}


MI_S32 LdcReadTableBin(const char *pConfigPath, LDC_BIN_HANDLE *tldc_bin, MI_U32 *pu32BinSize)
{
    struct stat statbuff;
    MI_U8 *pBufData = NULL;
    MI_S32 s32Fd = 0;
    MI_U32 u32Size = 0;

    if (pConfigPath == NULL)
    {
        MIXER_ERR("File path null!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    printf("Read file %s\n", pConfigPath);
    memset(&statbuff, 0, sizeof(struct stat));
    if(stat(pConfigPath, &statbuff) < 0)
    {
        MIXER_ERR("Bb table file not exit!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    else
    {
        if (statbuff.st_size == 0)
        {
            MIXER_ERR("File size is zero!\n");
            return MI_ERR_LDC_ILLEGAL_PARAM;
        }
        u32Size = statbuff.st_size;
    }
    s32Fd = open(pConfigPath, O_RDONLY);
    if (s32Fd < 0)
    {
        MIXER_ERR("Open file[%d] error!\n", s32Fd);
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    pBufData = (MI_U8 *)malloc(u32Size);
    if (!pBufData)
    {
        MIXER_ERR("Malloc error!\n");
        close(s32Fd);

        return MI_ERR_LDC_ILLEGAL_PARAM;
    }

    memset(pBufData, 0, u32Size);
    read(s32Fd, pBufData, u32Size);
    close(s32Fd);

    *tldc_bin = pBufData;
    *pu32BinSize = u32Size;

    printf("%d: &bin address %p, *binbuffer %p \n",__LINE__, tldc_bin, *tldc_bin);

    return MI_SUCCESS;
}


MI_S32 GetLdcBinBuffer(MI_BOOL bIsBinPath, char *pCfgFilePath, mi_eptz_config_param *pstCfgPara, MI_U32 *pu32ViewNum, LDC_BIN_HANDLE *phLdcBin, MI_U32 *pu32LdcBinSize, MI_S32 *ps32Rot)
{
    if(bIsBinPath)
    {
        MIXERCHECKRESULT(LdcReadTableBin(pCfgFilePath, phLdcBin, pu32LdcBinSize));
        *pu32ViewNum = 1;
    }
    else
    {
        MIXERCHECKRESULT(LdcParseLibarayCfgFilePath(pCfgFilePath, pstCfgPara));

        MI_U32 u32ViewNum = 0;

        u32ViewNum = LdcGetCfgViewNum(pstCfgPara->ldc_mode);
        for(MI_U32 i = 0; i < u32ViewNum; i++)
        {
            MIXERCHECKRESULT(LdcLibarayCreatBin(i, pstCfgPara, &phLdcBin[i], &pu32LdcBinSize[i], ps32Rot[i]));
        }

        *pu32ViewNum = u32ViewNum;
    }

    return 0;
}

MI_S32 LdcCheckBinPath(char *pLdcCfgPath, MI_BOOL &bIsBinPath)
{
    MIXER_DBG("pLdcCfgPath: %s\n", pLdcCfgPath);
    char *pstr = NULL;

    pstr = strstr(pLdcCfgPath, ".bin");
    if(pstr != NULL)
    {
        bIsBinPath = TRUE;
        return MI_SUCCESS;
    }

    pstr = strstr(pLdcCfgPath, ".cfg");
    if(pstr != NULL)
    {
        bIsBinPath = FALSE;
        return MI_SUCCESS;
    }

    return MI_ERR_LDC_ILLEGAL_PARAM;
}


MI_S32 MI_VideoEncoder::initLdc(MI_VPE_CHANNEL VpeChannel, char *pLdcBinPath)
{
    MI_BOOL bIsBinPath = FALSE;
    mi_eptz_config_param stCfgPara;
    MI_U32 u32ViewNum = 0;
    LDC_BIN_HANDLE hLdcBin[MIXER_LDC_MAX_VIEWNUM] = {NULL};
    MI_U32 u32LdcBinSize[MIXER_LDC_MAX_VIEWNUM] = {0};
    MI_S32  s32Rot[MIXER_LDC_MAX_VIEWNUM] = {0, 90, 180, 270};

    memset(&stCfgPara, 0, sizeof(mi_eptz_config_param));

    MIXERCHECKRESULT(LdcCheckBinPath(pLdcBinPath, bIsBinPath));
    MIXER_DBG("bIsBinPath: %d\n", (int)bIsBinPath);

    MIXERCHECKRESULT(GetLdcBinBuffer(bIsBinPath, pLdcBinPath, &stCfgPara,
        &u32ViewNum, hLdcBin, u32LdcBinSize, s32Rot));

    MIXERCHECKRESULT(MI_VPE_LDCBegViewConfig(VpeChannel));

    for(MI_U32 i = 0; i < u32ViewNum; i++)
    {
        MIXERCHECKRESULT(MI_VPE_LDCSetViewConfig(VpeChannel, hLdcBin[i], u32LdcBinSize[i]));

        if(mi_eptz_buffer_free(hLdcBin[i]) != MI_EPTZ_ERR_NONE)
        {
            MIXER_ERR("[MI EPTZ ERR]   %d !! \n", __LINE__);
        }
    }

    MIXERCHECKRESULT(MI_VPE_LDCEndViewConfig(VpeChannel));

    return MI_SUCCESS;
}
#endif


MI_S32 MI_VideoEncoder::initVideoEncoder()
{
    MI_VENC_ChnAttr_t stChnAttr;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_VENC_ParamRef_t stParamRef;
    MI_U32 u32DevId = -1;
    MI_U32 tFnum = 0x0, tFden = 0x0;
#if TARGET_CHIP_I6B0
    MI_VENC_InputSourceConfig_t stInputSource;
#endif

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    memset(&stChnAttr,  0x00, sizeof(MI_VENC_ChnAttr_t));
    memset(&stParamRef, 0x00, sizeof(MI_VENC_ParamRef_t));

    if(m_encoderType == VE_YUV420 || m_encoderType == VE_JPG_YUV422)
    {
        MIXER_DBG("initvideo viChn=%d,yuv data, skip init video encoder channel\n", m_veChn);
        return 0;
    }

    #if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    tFnum = m_vencframeRate & 0xffff;
    tFden = (m_vencframeRate>>16) & 0xffff;
    if(0x0 == tFden)
    {
        tFden = 1;
    }
    #else
    tFnum = m_vencframeRate;
    tFden = 1;
    #endif
    switch(m_encoderType)
    {
        case VE_AVC:
#if TARGET_CHIP_I5
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth  = ALIGN_8xUP(m_width);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth  = ALIGN_8xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth  = ALIGN_32xUP(m_width);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth  = ALIGN_32xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6E
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth  = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6B0
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth    = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#endif
            stChnAttr.stVeAttr.stAttrH264e.u32Profile = m_VideoProfile ; //1 //0:baseline, 1:main
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2;
            stChnAttr.stVeAttr.stAttrH264e.bByFrame = true;
            if((m_widthmax <= 704) && (m_heightmax <= 576))
            {
#if TARGET_CHIP_I5
                stChnAttr.stVeAttr.stAttrH264e.u32BufSize = ALIGN_8xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6
                stChnAttr.stVeAttr.stAttrH264e.u32BufSize = ALIGN_32xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6E
        stChnAttr.stVeAttr.stAttrH264e.u32BufSize = ALIGN_16xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6B0
        stChnAttr.stVeAttr.stAttrH264e.u32BufSize = ALIGN_16xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#endif
                stChnAttr.stVeAttr.stAttrH264e.u32BufSize = ALIGN_64xUP(stChnAttr.stVeAttr.stAttrH264e.u32BufSize);
            }

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = m_bitRate;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = m_gop;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = tFnum;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = tFden;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            break;

        case VE_H265:
#if TARGET_CHIP_I5
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth  = ALIGN_8xUP(m_width);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth  = ALIGN_8xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth  = ALIGN_32xUP(m_width);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth  = ALIGN_32xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6E
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth  = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6B0
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth  = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#endif
            stChnAttr.stVeAttr.stAttrH265e.bByFrame = true;
            if((m_widthmax <= 704) && (m_heightmax <= 576))
            {
#if TARGET_CHIP_I5
                stChnAttr.stVeAttr.stAttrH265e.u32BufSize = ALIGN_8xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6
                stChnAttr.stVeAttr.stAttrH265e.u32BufSize = ALIGN_32xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6E
                stChnAttr.stVeAttr.stAttrH265e.u32BufSize = ALIGN_16xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6B0
                stChnAttr.stVeAttr.stAttrH265e.u32BufSize = ALIGN_16xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
#endif
               stChnAttr.stVeAttr.stAttrH265e.u32BufSize = ALIGN_64xUP(stChnAttr.stVeAttr.stAttrH265e.u32BufSize);
            }

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = m_bitRate;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = tFnum;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = tFden;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = m_gop;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            break;

        case VE_JPG:
     case VE_MJPEG:
#if TARGET_CHIP_I5
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth  = ALIGN_8xDOWN(m_width);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth  = ALIGN_8xDOWN(m_widthmax);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth  = ALIGN_16xDOWN(m_width);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth  = ALIGN_16xDOWN(m_widthmax);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6E
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth  = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#elif TARGET_CHIP_I6B0
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth  = ALIGN_16xUP(m_width);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_2xUP(m_height);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth  = ALIGN_16xUP(m_widthmax);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = ALIGN_2xUP(m_heightmax);
#endif
            stChnAttr.stVeAttr.stAttrJpeg.bByFrame = true;
            if((m_widthmax <= 704) && (m_heightmax <= 576))
            {
                stChnAttr.stVeAttr.stAttrJpeg.u32BufSize = ALIGN_8xUP(m_widthmax) * ALIGN_2xUP(m_heightmax);
                stChnAttr.stVeAttr.stAttrJpeg.u32BufSize = ALIGN_64xUP(stChnAttr.stVeAttr.stAttrJpeg.u32BufSize);
            }

            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor = m_qfactor;
            stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateNum = tFnum;
            stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateDen = tFden;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            break;

        case VE_YUV420:
        case VE_JPG_YUV422:
            /*if(fShmSrc)
            {
                shmdt(fShmSrc);
                shmctl(fShmId, IPC_RMID, NULL);
                fShmSrc = NULL;
            }*/
            break;

        default :
            MIXER_ERR("init video error, error video encoder type:%d\n", m_encoderType);
            return -1;
    }

    MIXERCHECKRESULT(Mixer_Venc_CreateChannel(m_veChn, &stChnAttr));

    if(m_encoderType == VE_AVC || m_encoderType == VE_H265)
    {
        if(m_virtualIEnable == TRUE && m_virtualIInterval > 0)
        {
            //Reference I frame
            stParamRef.u32Base = 1;
            stParamRef.bEnablePred = FALSE;
            stParamRef.u32Enhance = m_virtualIInterval - 1;
        }
        else if(m_virtualIEnable == FALSE && m_virtualIInterval > 0)
        {
            //Reference P frame
            stParamRef.u32Base = 1;
            stParamRef.bEnablePred = TRUE;
            stParamRef.u32Enhance = m_virtualIInterval - 1;
        }
        else
        {
            // disable LTR
            stParamRef.u32Base = 1;
            stParamRef.bEnablePred = FALSE;
            stParamRef.u32Enhance = 0;
        }

        printf("%s:%d Set LTR para: veChn=%d, Base=%d, bEnablePred=%s, Enhance=%d\n", __func__, __LINE__, m_veChn,
                                  stParamRef.u32Base, stParamRef.bEnablePred ? "TRUE" : "FALSE", stParamRef.u32Enhance);

        ExecFunc(MI_VENC_SetRefParam(m_veChn, &stParamRef), MI_SUCCESS);
        printf("%s:%d Set LTR info: veChn=%d, m_virtualIEnable=%d, m_virtualIInterval=%d\n",
                                   __func__, __LINE__, m_veChn, m_virtualIEnable, m_virtualIInterval);

        memset(&stParamRef, 0x00, sizeof(MI_VENC_ParamRef_t));
        ExecFunc(MI_VENC_GetRefParam(m_veChn, &stParamRef), MI_SUCCESS);
        printf("%s:%d Get LTR para: veChn=%d, Base=%d, bEnablePred=%s, Enhance=%d\n", __func__, __LINE__, m_veChn,
                                  stParamRef.u32Base, stParamRef.bEnablePred ? "TRUE" : "FALSE", stParamRef.u32Enhance);
    }

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    if(m_bDivpInit == TRUE)
    {
        if(m_pipCfg)
        {
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stSrcChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
        #if TARGET_CHIP_I6E
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            MIXER_DBG("eBindType is %d\n", stBindInfo.eBindType);
        #endif
        }
        else
        {
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#if TARGET_CHIP_I6B0
            if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && m_DivpChnPort.u32ChnId == MI_VideoEncoder::u32RealDivpChn)
                stBindInfo.u32SrcFrmrate = vpeframeRate;
#endif
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            stBindInfo.u32BindParam = 0;
#endif
        }
    }
    else
    {
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;
        stBindInfo.u32SrcFrmrate = vpeframeRate;

    #if (TARGET_CHIP_I6 || TARGET_CHIP_I6E)
        stBindInfo.u32BindParam = 0;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
        if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
        {
            stBindInfo.u32BindParam = m_height;
        }
        else
        {
            stBindInfo.u32BindParam = 0;
        }
        MIXER_DBG("m_veChn=%d type is %d, mode is %d m_encoderType=%d\n",m_veChn,stBindInfo.eBindType, m_bindMode,m_encoderType);
    #elif TARGET_CHIP_I6B0
        stBindInfo.u32BindParam = 0;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
        if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
        {
            if(m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)stBindInfo.u32BindParam = m_height/2;
            else stBindInfo.u32BindParam = m_height;
        }
        else
        {
            stBindInfo.u32BindParam = 0;
        }
        MIXER_DBG("m_veChn=%d type is %d, mode is %d m_encoderType=%d\n",m_veChn,stBindInfo.eBindType, m_bindMode,m_encoderType);
    #endif

    }
#if TARGET_CHIP_I6B0
    if(m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)
    {
        memset(&stInputSource, 0, sizeof(stInputSource));
        stInputSource.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_RING_HALF_FRM;
        MI_VENC_SetInputSourceConfig(m_veChn, &stInputSource);
    }
    else if(m_bindMode == Mixer_Venc_Bind_Mode_HW_RING)
    {
        memset(&stInputSource, 0, sizeof(stInputSource));
        stInputSource.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_RING_ONE_FRM;
        MI_VENC_SetInputSourceConfig(m_veChn, &stInputSource);
    }
    else if(m_bindMode == Mixer_Venc_Bind_Mode_FRAME)
    {
        memset(&stInputSource, 0, sizeof(stInputSource));
        stInputSource.eInputSrcBufferMode = E_MI_VENC_INPUT_MODE_NORMAL_FRMBASE;
        MI_VENC_SetInputSourceConfig(m_veChn, &stInputSource);
    }
#endif
    ExecFunc(MI_VENC_GetChnDevid(m_veChn, &u32DevId), MI_SUCCESS);

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId  = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId  = m_veChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32DstFrmrate = m_vencframeRate;
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    MIXERCHECKRESULT(MI_VENC_SetMaxStreamCnt(m_veChn, 20));
#endif

    return 0;
}

MI_S32 MI_VideoEncoder::initVideoInput(Mixer_VPE_PortInfo_T *pstMixerVpePortInfo)
{
    MI_SYS_WindowRect_t stCropRect;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;

    memset(&stDivpPortAttr, 0x00, sizeof(MI_DIVP_OutputPortAttr_t));
    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
    memset(&stCropRect, 0x00, sizeof(stCropRect));

    m_bDivpInit = FALSE;

    MIXERCHECKRESULT(Mixer_Vpe_StartPort(m_VpeChnPort.u32ChnId, pstMixerVpePortInfo));

#if TARGET_CHIP_I5
    if((m_encoderType <  VE_JPG_YUV422) && (m_encoderType != VE_YUV420))
    {
        // user no need set buffer
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, m_vpeBufUsrDepth, m_vpeBufCntQuota));
    }
    else
    {
        // dump yuv to file, user need set buffer
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, 2/*m_vpeBufUsrDepth*/, m_vpeBufCntQuota));
    }
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    if(((VE_JPG_YUV422 > m_encoderType) &&(VE_YUV420 != m_encoderType)) || pstMixerVpePortInfo->VpeChnPort.u32PortId >= (MAX_VPE_PORT_NUMBER-1))
    {
        MIXER_DBG("eModId(%d), DevId(%d),  ChnId(%d), PortId(%d)\n", m_VpeChnPort.eModId,\
                                         m_VpeChnPort.u32DevId,\
                                         m_VpeChnPort.u32ChnId,\
                                         m_VpeChnPort.u32PortId);

         if(0 >= m_vpeBufCntQuota)
         {
           m_vpeBufCntQuota = 3;
         }
		 if((pstMixerVpePortInfo->VpeChnPort.u32PortId >= (MAX_VPE_PORT_NUMBER-1)) && ((VE_YUV420 == m_encoderType) || (VE_JPG_YUV422 == m_encoderType)))
		 {
		    if(!m_bDivpInit)//port2==>yuv
		   	{
              MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, 2, 4));
		   	}
			else//port2=>divp->yuv
			{
			   MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, 0, m_vpeBufCntQuota));
			}
		 }
	 	 else
		 {
	       MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, 0, m_vpeBufCntQuota));
		 }
    }
    else
    {
        if(0 >= m_vpeBufCntQuota)
        {
          m_vpeBufCntQuota = 5;
        }
        if(0 >= m_vpeBufUsrDepth)
        {
          m_vpeBufUsrDepth = 2;
        }
        MIXERCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&m_VpeChnPort, m_vpeBufUsrDepth, m_vpeBufCntQuota));  // dump yuv to file, user need set buffer
    }
#endif

    initDivpAndVdisp(0);

    return 0;
}
MI_S32 MI_VideoEncoder::uninitVideoV2(MI_U32 s32Mode)
{
    MI_S32 result = 0;
    Mixer_Sys_Input_E enInput;
    //MI_SYS_WindowRect_t stCropRect;
    Mixer_Sys_BindInfo_T stBindInfo;
    //MI_DIVP_OutputPortAttr_t stDivpPortAttr;

    if(m_encoderType == VE_JPG_YUV422)
    {
        MIXER_WARN("The VideoStream%d encoderType is JPG_YUV422!\n", m_veChn);
        return result;
    }

#if TARGET_CHIP_I5
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_MAX ? s32Mode:MIXER_SYS_INPUT_MAX);
#elif TARGET_CHIP_I6
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);

#elif    TARGET_CHIP_I6E
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);
#elif    TARGET_CHIP_I6B0
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);

#endif
    if(m_bDivpInit == TRUE)
    {
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
        if(m_pipCfg)
        {
            //MI_VDISP_InputPortAttr_t stInputPortAttr;
            //MI_VDISP_OutputPortAttr_t stOutputPortAttr;

            /************************************************
              Step1:  unbind Venc to vdisp & destroy Venc
            *************************************************/
            if(m_encoderType != VE_YUV420)
            {
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VDISP;
                stBindInfo.stSrcChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
                stBindInfo.stSrcChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
                stBindInfo.stSrcChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;

                stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
                stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                if (enInput == MIXER_SYS_INPUT_VPE)
                {
                MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
                }
                else
                {
                MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
                }
            }

            /**********************************************************
              Step2:  unbind divp(pip&vdisp) to vdisp & destroy vdisp
            **********************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclDownChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
#if TARGET_CHIP_I5
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
#elif TARGET_CHIP_I6E
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_VDISP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
#endif

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

        #if TARGET_CHIP_I5
            ExecFunc(MI_VDISP_DisableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_DisableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_VDISP), MI_SUCCESS);
        #elif TARGET_CHIP_I6E
            ExecFunc(MI_VDISP_DisableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_DisableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_CHNID_FOR_VDISP), MI_SUCCESS);
        #endif
            ExecFunc(MI_VDISP_StopDev(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_CloseDevice(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_Exit(), MI_SUCCESS);


            /****************************************************
              Step3:  unbind VPE to divp(for pip) & destroy divp
            *****************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            // stop & destroy divp(for pip)
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpSclUpChnId));
            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpSclUpChnId));
            Mixer_Divp_PutChannleNum(m_DivpSclUpChnId);


            /******************************************************
              Step4:  unbind VPE to divp(for vdisp) & destroy divp
            *******************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclDownChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            // stop & destroy divp(for vdisp)
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpSclDownChnId));
            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpSclDownChnId));
            Mixer_Divp_PutChannleNum(m_DivpSclDownChnId);
        }
        else // if((m_bDivpInit == TRUE) && (m_pipCfg == FALSE))
#endif
        if(m_bDivpInit == TRUE)
        {
            // unbind divp to venc
            if(m_encoderType != VE_YUV420)
            {
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId    = m_DivpChnPort.eModId;
                stBindInfo.stSrcChnPort.u32DevId  = m_DivpChnPort.u32DevId;
                stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
                stBindInfo.stSrcChnPort.u32PortId = m_DivpChnPort.u32PortId;

                stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
                stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                if (enInput == MIXER_SYS_INPUT_VPE)
                {
                MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
                }
                else
                {
                MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
                }
            }

            // unbind VPE to divp
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            // stop & destroy divp
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpChnPort.u32ChnId));
            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpChnPort.u32ChnId));
            Mixer_Divp_PutChannleNum(m_DivpChnPort.u32ChnId);
			m_DivpChnPort.u32ChnId = 0xff;
            m_bDivpInit = FALSE;
        }
    }
    else
    {
        if(m_encoderType != VE_YUV420)
        {
            if (enInput == MIXER_SYS_INPUT_VPE)
            {
                MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
            }
            else
            {
               MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
            }
        }
    }

    if (enInput == MIXER_SYS_INPUT_VPE)
    {
        MIXERCHECKRESULT(Mixer_Vpe_StopPort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId));
    }

    return result;
}
MI_S32 MI_VideoEncoder::uninitVideo(MI_U32 s32Mode)
{
    MI_S32 result = 0;
    Mixer_Sys_Input_E enInput;
    //MI_SYS_WindowRect_t stCropRect;
    Mixer_Sys_BindInfo_T stBindInfo;
    //MI_DIVP_OutputPortAttr_t stDivpPortAttr;

    if(/*m_encoderType == VE_YUV420 ||*/ m_encoderType == VE_JPG_YUV422)
    {
        MIXER_WARN("The VideoStream%d encoderType is JPG_YUV422!\n", m_veChn);
        return result;
    }

#if TARGET_CHIP_I5
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_MAX ? s32Mode:MIXER_SYS_INPUT_MAX);
#elif TARGET_CHIP_I6
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);

#elif    TARGET_CHIP_I6E
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);
#elif    TARGET_CHIP_I6B0
    enInput = (Mixer_Sys_Input_E)(s32Mode < MIXER_SYS_INPUT_BUTT ? s32Mode:MIXER_SYS_INPUT_BUTT);

#endif
    if(m_bDivpInit == TRUE)
    {
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
        if(m_pipCfg)
        {
            //MI_VDISP_InputPortAttr_t stInputPortAttr;
            //MI_VDISP_OutputPortAttr_t stOutputPortAttr;

            /************************************************
              Step1:  unbind Venc to vdisp & destroy Venc
            *************************************************/
            if(m_encoderType != VE_YUV420)
            {
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VDISP;
                stBindInfo.stSrcChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
                stBindInfo.stSrcChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
                stBindInfo.stSrcChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;

                stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
                stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                if (enInput == MIXER_SYS_INPUT_VPE)
                {
                MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
                }
                else
                {
                MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
                }
            }

            /**********************************************************
              Step2:  unbind divp(pip&vdisp) to vdisp & destroy vdisp
            **********************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclDownChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;

#if TARGET_CHIP_I5
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
#elif TARGET_CHIP_I6E
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_VDISP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;
#endif
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

        #if TARGET_CHIP_I5
            ExecFunc(MI_VDISP_DisableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_DisableInputPort(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_VDISP), MI_SUCCESS);
        #elif TARGET_CHIP_I6E
            ExecFunc(MI_VDISP_DisableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_PORTID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_DisableInputChannel(MIXER_VDISP_DEVID_FOR_PIP, MIXER_VDISP_CHNID_FOR_VDISP), MI_SUCCESS);
        #endif
            ExecFunc(MI_VDISP_StopDev(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_CloseDevice(MIXER_VDISP_DEVID_FOR_PIP), MI_SUCCESS);
            ExecFunc(MI_VDISP_Exit(), MI_SUCCESS);


            /****************************************************
              Step3:  unbind VPE to divp(for pip) & destroy divp
            *****************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            // stop & destroy divp(for pip)
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpSclUpChnId));
            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpSclUpChnId));
            Mixer_Divp_PutChannleNum(m_DivpSclUpChnId);


            /******************************************************
              Step4:  unbind VPE to divp(for vdisp) & destroy divp
            *******************************************************/

            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclDownChnId;

            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            // stop & destroy divp(for vdisp)
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpSclDownChnId));
            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpSclDownChnId));
            Mixer_Divp_PutChannleNum(m_DivpSclDownChnId);


        }
        else // if((m_bDivpInit == TRUE) && (m_pipCfg == FALSE))
#endif
        {
            // unbind divp to venc
            if(m_encoderType != VE_YUV420)
            {
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId    = m_DivpChnPort.eModId;
                stBindInfo.stSrcChnPort.u32DevId  = m_DivpChnPort.u32DevId;
                stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
                stBindInfo.stSrcChnPort.u32PortId = m_DivpChnPort.u32PortId;

                stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
                stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                if (enInput == MIXER_SYS_INPUT_VPE)
                {
                    MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
                }
                else
                {
                    MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
                }
            }

#if TARGET_CHIP_I6B0
            if(m_VpeChnPort.u32PortId != VPE_REALMODE_SUB_PORT || (m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && checkLastRealDivp(m_veChn) == TRUE))
            {
                // stop & destroy divp
                MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpChnPort.u32ChnId));

                // unbind VPE to divp
                memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
                stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
                stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
                stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
                stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

                stBindInfo.stDstChnPort.eModId    = m_DivpChnPort.eModId;
                stBindInfo.stDstChnPort.u32DevId  = m_DivpChnPort.u32DevId;
                stBindInfo.stDstChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
                stBindInfo.stDstChnPort.u32PortId = m_DivpChnPort.u32PortId;

                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpChnPort.u32ChnId));
            }
#else
            // stop & destroy divp
            MIXERCHECKRESULT(Mixer_Divp_StopChn(m_DivpChnPort.u32ChnId));

            // unbind VPE to divp
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_DivpChnPort.u32ChnId));

            Mixer_Divp_PutChannleNum(m_DivpChnPort.u32ChnId);
			m_DivpChnPort.u32ChnId = 0xff;
#endif
        }

        m_bDivpInit = FALSE;

#if TARGET_CHIP_I6B0
        if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT && m_DivpChnPort.u32ChnId == MI_VideoEncoder::u32RealDivpChn && checkLastRealDivp(m_veChn) == TRUE)
        {
            MI_VideoEncoder::bRealDivpInit = FALSE;
        }
#endif
    }
    else //if(m_bDivpInit == TRUE)
    {
        if(m_encoderType != VE_YUV420)
        {
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;
            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            if (enInput == MIXER_SYS_INPUT_VPE)
            {
            MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(m_veChn));
            }
            else
            {
            MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));
            }
        }
    }

    if (enInput == MIXER_SYS_INPUT_VPE)
    {
        MIXERCHECKRESULT(Mixer_Vpe_StopPort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId));
    }

    return result;
}


MI_S32 MI_VideoEncoder::startVideoEncoder()
{
    if(m_encoderType != VE_YUV420 && m_encoderType != VE_JPG_YUV422)
    {
        MIXER_DBG("m_VeChn is %d\n", m_veChn);
        MIXERCHECKRESULT(Mixer_Venc_StartChannel(m_veChn));
        m_thread_exit = FALSE;

        startGetStreamThread();

        //resume recording
        //MIXER_DBG("veChn %d\n", m_veChn);
    }
    else if(m_encoderType == VE_YUV420)
    {
        MIXER_DBG("yuv m_VeChn is %d\n", m_veChn);
        m_thread_exit = FALSE;
        startGetStreamThread();
    }
    if(0 > g_RecordManager->Resume(m_veChn))
    {
        MIXER_ERR("can not Resume recorder\n");
    }
        //end
    return 0;
}

MI_S32 MI_VideoEncoder::stopVideoEncoder()
{
     MI_VENC_CHN VencChn = m_veChn;

     //pause recording
     //MIXER_DBG("~~~~~~~~~~~~~veChn %d\n", m_veChn);
      if(0 > g_RecordManager->Pause(m_veChn))
      {
         MIXER_ERR("can not Pause recorder\n");
      }
     //end
     if(/*m_encoderType != VE_YUV420 &&*/ m_encoderType != VE_JPG_YUV422)
     {
      stopGetStreamThread();
      if(m_encoderType != VE_YUV420)
              MIXERCHECKRESULT(Mixer_Venc_StopChannel(VencChn));
        //MIXERCHECKRESULT(Mixer_Venc_DestoryChannel(VencChn));
        }
    return 0;
}

void MI_VideoEncoder::requestIDR()
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(m_encoderType == VE_YUV420)
    {
        MIXER_INFO("video %d  m_encoderType %d, not support request idr\n", m_veChn, m_encoderType);
     return;
    }

    MIXER_DBG("in function:%s\n", __FUNCTION__);
    MI_VENC_RequestIdr(m_veChn, TRUE);
    if (MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("request IDR fail, error:%x\n", s32Ret);
    }
}

void MI_VideoEncoder::startGetFrame()
{
    m_startGetFrame = TRUE;
    MI_VENC_ChnAttr_t attr;
    MI_U32 mi_err;

    mi_err = MI_VENC_GetChnAttr(m_veChn, &attr);
    if(MI_SUCCESS != mi_err)
    {
        printf("[%s:%d] call MI_VENC_GetChnAttr error.\n", __func__, __LINE__);
        return;
    }

    startVideoEncoder();
}

void MI_VideoEncoder::stopGetFrame()
{
    m_startGetFrame = FALSE;
    MI_VENC_ChnAttr_t attr;
    MI_U32 mi_err;

    mi_err = MI_VENC_GetChnAttr(m_veChn, &attr);
    if(MI_SUCCESS != mi_err)
    {
        printf("[%s:%d] call MI_VENC_GetChnAttr error.\n", __func__, __LINE__);
        return;
    }

    stopVideoEncoder();
}

BOOL MI_VideoEncoder::canGetFrame()
{
    return m_startGetFrame;
}

void MI_VideoEncoder::startGetStreamThread()
{
    MI_S32 s32Ret = 0;
    MI_S8 u8TaskName[64];

    if(0 != GetThreadState())
    {
        MIXER_ERR("%s veChn: %d alread start\n", __func__, m_veChn);
        return;
    }
    s32Ret = pthread_create(&GetThreadState(), NULL, Stream_Task, this);
    if(0 == s32Ret)
    {
        memset((char*)u8TaskName, 0x00, sizeof(u8TaskName));
        sprintf((char *)u8TaskName, "Stream%d_Task", m_veChn);
        pthread_setname_np(GetThreadState() , (const char *)u8TaskName);
    }
    else
    {
        MIXER_ERR("%s veChn: %d pthread_create failed\n", __func__, m_veChn);
    }

    MIXER_DBG("%s veChn: %d\n", __func__, m_veChn);
    return;
}

void MI_VideoEncoder::stopGetStreamThread()
{
    printf("%s veChn = %d, in\n", __func__, m_veChn);

    if(0 != GetThreadState() && TRUE == m_vencGetStreamStatus)
    {
        //wait for signal
    //MySystemDelay(2000);

    //end

        m_vencGetStreamStatus = FALSE;

        printf("%s wait for VideoEncoder Stream_Task%d out\n", __func__, m_veChn);

        pthread_join(GetThreadState(), NULL);
        ClearThreadState();
        printf("%s veChn = %d, out\n", __func__, m_veChn);
    }
    else
    {
        printf("%s veChn = %d already stop\n", __func__, m_veChn);
    }

    return;
}

MI_S32 MI_VideoEncoder::setResolution(MI_U32 width, MI_U32 height)
{
    MI_S32 ret = -1;

    if( m_encoderType == VE_YUV420 || m_encoderType == VE_JPG_YUV422)
    {
        MIXER_ERR("input param err, dump yuv not support setResolution\n");
        return ret;
    }

    if(m_pipCfg && (m_pipRectX + m_pipRectW > ALIGN_2xUP(width)))
    {
        printf("%s:%d set PIP width(=%d) is larger then the dest resolution width(=%d)\n", __func__, __LINE__, m_pipRectW, width);
        return ret;
    }
    else if(m_pipCfg && (m_pipRectY + m_pipRectH > ALIGN_2xUP(height)))
    {
        printf("%s:%d set PIP height(=%d) is larger then the dest resolution height(=%d)\n", __func__, __LINE__, m_pipRectH, height);
        return ret;
    }

    stopVideoEncoder();
    ExecFunc(MI_VPE_DisablePort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId), MI_VPE_OK);
    if(_setResolution(width, height))
    {
        printf("%s:%d run _setResolution(%d, %d) error!)\n", __func__, __LINE__, width, height);
    }
    ExecFunc(MI_VPE_EnablePort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId), MI_VPE_OK);

    startVideoEncoder();
    ret = 0;
    return ret;
}



MI_S32 MI_VideoEncoder::_setResolution(MI_U32 width, MI_U32 height)
{

    MI_VENC_ChnAttr_t stVencChnAttr;


    printf("setResolution: m_veChn:%d  m_width:%d  m_height: %d\n", m_veChn, width, height);


#if TARGET_CHIP_I5
    uninitVideo(MIXER_SYS_INPUT_MAX);
#elif TARGET_CHIP_I6
    uninitVideo(MIXER_SYS_INPUT_BUTT);
#elif TARGET_CHIP_I6E
    uninitVideo(MIXER_SYS_INPUT_BUTT);
#elif TARGET_CHIP_I6B0
    uninitVideo(MIXER_SYS_INPUT_BUTT);
#endif

#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
    MI_VPE_PortMode_t stVpeMode;
    memset(&stVpeMode, 0x00, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);

    stVpeMode.u16Width  = ALIGN_2xUP(width);
    stVpeMode.u16Height = ALIGN_2xUP(height);
    m_VpeOutputWidth = stVpeMode.u16Width;
    m_VpeOutputHeight = stVpeMode.u16Height;
    ExecFunc(MI_VPE_SetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);
#elif TARGET_CHIP_I6E
    if(m_VpeChnPort.u32PortId != 2)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);

        stVpeMode.u16Width  = ALIGN_2xUP(width);
        stVpeMode.u16Height = ALIGN_2xUP(height);
        m_VpeOutputWidth = stVpeMode.u16Width;
        m_VpeOutputHeight = stVpeMode.u16Height;
        ExecFunc(MI_VPE_SetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);
    }
#elif TARGET_CHIP_I6B0
    if(m_VpeChnPort.u32PortId != 2 || m_DivpChnPort.eModId == E_MI_MODULE_ID_MAX)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);

        stVpeMode.u16Width  = ALIGN_2xUP(width);
        stVpeMode.u16Height = ALIGN_2xUP(height);
        m_VpeOutputWidth = stVpeMode.u16Width;
        m_VpeOutputHeight = stVpeMode.u16Height;
        ExecFunc(MI_VPE_SetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_SUCCESS);
    }
#endif

    memset(&stVencChnAttr, 0x00, sizeof(MI_VENC_ChnAttr_t));
    ExecFunc(MI_VENC_GetChnAttr(m_veChn, &stVencChnAttr), MI_SUCCESS);

    switch(m_encoderType)
    {
        case VE_AVC:
            stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth  = ALIGN_8xUP(width);
            stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight = ALIGN_2xUP(height);
            m_width = stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth;
            m_height = stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight;
            break;

        case VE_H265:
            stVencChnAttr.stVeAttr.stAttrH265e.u32PicWidth  = ALIGN_8xUP(width);
            stVencChnAttr.stVeAttr.stAttrH265e.u32PicHeight = ALIGN_2xUP(height);

            m_width = stVencChnAttr.stVeAttr.stAttrH265e.u32PicWidth;
            m_height = stVencChnAttr.stVeAttr.stAttrH265e.u32PicHeight;
            break;
        case VE_MJPEG:
        case VE_JPG:
            stVencChnAttr.stVeAttr.stAttrJpeg.u32PicWidth  = ALIGN_8xUP(width);
            stVencChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = ALIGN_2xUP(height);

            m_width = stVencChnAttr.stVeAttr.stAttrJpeg.u32PicWidth;
            m_height = stVencChnAttr.stVeAttr.stAttrJpeg.u32PicHeight;
            break;

        default :
            MIXER_ERR("init video error, error video encoder type\n");
            break;
    }

    ExecFunc(MI_VENC_SetChnAttr(m_veChn, &stVencChnAttr), MI_SUCCESS);

    initDivpAndVdisp(1);

  //  MIXERCHECKRESULT(Mixer_Venc_StartChannel(m_veChn));
    return 0;
}
MI_S32 MI_VideoEncoder::setRotateV2(MI_SYS_Rotate_e eSetType)
{
    MI_VPE_PortMode_t stVpeMode;
     MI_U32 temp = 0;
    if(((eSetType % 2) != (m_initRotate % 2)))
    {
        temp = m_width;
        m_width  = m_height;
        m_height = temp;

        temp = m_widthmax;
        m_widthmax  = m_heightmax;
        m_heightmax = temp;

        temp = m_VpeOutputWidth;
        m_VpeOutputWidth = m_VpeOutputHeight;
        m_VpeOutputHeight = temp;
		if(m_pipCfg)
		{
		   temp = m_pipRectX;
		   m_pipRectX = m_pipRectY;
		   m_pipRectY = temp;

		   temp = m_pipRectH;
		   m_pipRectH = m_pipRectW;
		   m_pipRectW = temp;
		}
#if !TARGET_CHIP_I6B0
        m_VpeOutputWidth = (m_VpeOutputWidth >= m_width ? m_VpeOutputWidth:m_width);
        m_VpeOutputHeight = (m_VpeOutputHeight >= m_height ? m_VpeOutputHeight:m_height);
        m_widthmax = (m_VpeOutputWidth > m_widthmax ? m_VpeOutputWidth:m_widthmax);
        m_heightmax = (m_VpeOutputHeight > m_heightmax ? m_VpeOutputHeight:m_heightmax);
#endif
    }
    memset(&stVpeMode, 0x00, sizeof(stVpeMode));
    ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);
    //stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
    //stVpeMode.ePixelFormat  = pstPortInfo->ePixelFormat;
    if((eSetType % 2))
    {
#if TARGET_CHIP_I5
        stVpeMode.u16Width  = ALIGN_8xUP(m_VpeOutputWidth);
        stVpeMode.u16Height = ALIGN_2xUP(m_VpeOutputHeight);
#elif TARGET_CHIP_I6
        if(0x0 == m_VpeChnPort.u32PortId)
        {
            stVpeMode.u16Width  = ALIGN_16xDOWN(m_VpeOutputWidth);
            stVpeMode.u16Height = ALIGN_2xUP(m_VpeOutputHeight);
        }
        else
        {
            stVpeMode.u16Width  = ALIGN_32xUP(m_VpeOutputWidth);
            stVpeMode.u16Height = ALIGN_2xUP(m_VpeOutputHeight);
        }
#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        {
            stVpeMode.u16Width  = ALIGN_16xUP(m_VpeOutputWidth);
            stVpeMode.u16Height = ALIGN_2xUP(m_VpeOutputHeight);
        }
#endif
    }
    else
    {
        stVpeMode.u16Width  = m_VpeOutputWidth;
            stVpeMode.u16Height = m_VpeOutputHeight;
    }
    ExecFunc(MI_VPE_SetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);
        MIXER_DBG("Set VpeMode-%d (w:%d h:%d PixelFormat:%d CompressMode:%d))\n", m_veChn,
                    stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat, stVpeMode.eCompressMode);
    m_initRotate = eSetType;
    ExecFunc(MI_VPE_EnablePort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId), MI_VPE_OK);
    return MI_SUCCESS;
}
MI_S32 MI_VideoEncoder::setRotate(MI_SYS_Rotate_e eSetType)
{
    Mixer_Sys_BindInfo_T stBindInfo;

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
   if(((eSetType % 2) != (m_initRotate % 2)))
#endif
    {
        stopVideoEncoder();
        uninitVideo(MIXER_SYS_INPUT_VPE);
        for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
        {
            ExecFunc(MI_VIF_DisableChnPort(i, 0), MI_SUCCESS);
        }
        stBindInfo.stSrcChnPort.eModId = VifChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = VifChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = VifChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = VifChnPort.u32PortId;
        stBindInfo.u32SrcFrmrate = vifframeRate;

        stBindInfo.stDstChnPort.eModId = VpeChnPortTop.eModId;
        stBindInfo.stDstChnPort.u32DevId  = VpeChnPortTop.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = VpeChnPortTop.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = VpeChnPortTop.u32PortId;
        stBindInfo.u32DstFrmrate = vpeframeRate;
        MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));
    }
#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
    else if(m_bDivpInit == TRUE)
    {
#if TARGET_CHIP_I5
        uninitVideo(MIXER_SYS_INPUT_MAX);
#elif TARGET_CHIP_I6
        uninitVideo(MIXER_SYS_INPUT_BUTT);
#elif TARGET_CHIP_I6E
        uninitVideo(MIXER_SYS_INPUT_BUTT);
#elif TARGET_CHIP_I6B0
        uninitVideo(MIXER_SYS_INPUT_BUTT);
#endif
    }
    if(((eSetType % 2) != (m_initRotate % 2)))
#endif
    {
        MI_S32 temp = 0;
        MI_VPE_PortMode_t stVpeMode;

        if(((eSetType % 2) != (m_initRotate % 2)))
        {
            temp = m_width;
            m_width  = m_height;
            m_height = temp;

            temp = m_widthmax;
            m_widthmax  = m_heightmax;
            m_heightmax = temp;

        }
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);
        //stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        //stVpeMode.ePixelFormat  = pstPortInfo->ePixelFormat;
        if((eSetType % 2))
        {
#if TARGET_CHIP_I5
            stVpeMode.u16Width  = ALIGN_8xUP(m_width);
            stVpeMode.u16Height = ALIGN_2xUP(m_height);
#elif TARGET_CHIP_I6
            if(0x0 == m_VpeChnPort.u32PortId)
            {
                stVpeMode.u16Width  = ALIGN_16xDOWN(m_width);
                stVpeMode.u16Height = ALIGN_2xUP(m_height);
            }
            else
            {
                stVpeMode.u16Width  = ALIGN_32xUP(m_width);
                stVpeMode.u16Height = ALIGN_2xUP(m_height);
            }
#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0
            {
                stVpeMode.u16Width  = ALIGN_16xUP(m_width);
                stVpeMode.u16Height = ALIGN_2xUP(m_height);
            }
#endif
        }
        else
        {
            stVpeMode.u16Width  = m_width;
                stVpeMode.u16Height = m_height;
        }
#if 0 //TARGET_CHIP_I6
        MI_SYS_WindowRect_t  pstOutCropInfo;
        memset(&pstOutCropInfo, 0x00, sizeof(pstOutCropInfo));

        pstOutCropInfo.u16Width = m_width;
        pstOutCropInfo.u16Height = m_height;
        pstOutCropInfo.u16X = 0;
        pstOutCropInfo.u16Y = 0;
        if(0 == MI_VideoEncoder::VifChnPort.u32PortId)
           {
          ExecFunc(MI_VPE_SetPortCrop(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &pstOutCropInfo),MI_VPE_OK);
           }
#endif

#if TARGET_CHIP_I6E
        MI_SYS_WindowRect_t stPortCrop;
        memset(&stPortCrop, 0, sizeof(MI_SYS_WindowRect_t));

        ExecFunc(MI_VPE_GetPortCrop(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stPortCrop), MI_VPE_OK);

        stPortCrop.u16Width  = m_width;
        stPortCrop.u16Height = m_height;
        MIXER_DBG("set port crop:(x:%d y:%d w:%d h:%d)\n", stPortCrop.u16X, stPortCrop.u16Y, stPortCrop.u16Width, stPortCrop.u16Height);
        ExecFunc(MI_VPE_SetPortCrop(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stPortCrop), MI_VPE_OK);
#endif

        ExecFunc(MI_VPE_SetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);
        MIXER_DBG("Set VpeMode-%d (w:%d h:%d PixelFormat:%d CompressMode:%d))\n", m_veChn,
                    stVpeMode.u16Width, stVpeMode.u16Height, stVpeMode.ePixelFormat, stVpeMode.eCompressMode);
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
#endif
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        for(MI_U8 i = 0; i < Mixer_vifDevNumberGet(); i++)
         {
          MIXERCHECKRESULT(Mixer_Vif_StartPort(i,MI_VideoEncoder::VifChnPort.u32ChnId,
                                             MI_VideoEncoder::VifChnPort.u32PortId));
         }
        initDivpAndVdisp(0);
        initVideoEncoder();
        startVideoEncoder();
    }
#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
    else if(E_MI_MODULE_ID_DIVP == m_DivpChnPort.eModId)
#endif
    {
#if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
        initDivpAndVdisp(1);
        MIXERCHECKRESULT(Mixer_Venc_StartChannel(m_veChn));
#endif
    }

    m_initRotate = eSetType;

    ExecFunc(MI_VPE_EnablePort(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId), MI_VPE_OK);

    return MI_SUCCESS;
}

MI_S32 MI_VideoEncoder::setFrameRate(MI_U32 frameRate, MI_U32 bChangeBitrate)
{
    if( m_encoderType == VE_YUV420 || m_encoderType == VE_JPG_YUV422)
    {
        MIXER_ERR("input param err, yuv not support setFrameRate\n");
        return -1;
    }

    MI_VENC_ChnAttr_t stChnAttr;
    MI_VENC_RcMode_e eRcModeType = E_MI_VENC_RC_MODE_MAX;
    MI_VENC_ModType_e enType = E_MI_VENC_MODTYPE_VENC;
    MI_U32 oldvencFrameRate = m_vencframeRate;
    //float realFPS = 0.0;
    Mixer_Sys_BindInfo_T stBindInfo;

    if(0x0 == (frameRate>>16))
    {
            if(frameRate == 0x0)
        {
            MIXER_ERR("you can not set venc fps == 0. return err.\n");
            return -1;
        }
    }
    else
    {
        if(0x0 == (frameRate & 0xffff))
        {
            MIXER_ERR("you can not set venc fps == 0. return err.\n");
            return -1;
        }
    }

    MIXER_DBG("the venc framerate:%d\n", frameRate);
    m_vencframeRate = frameRate;

    MIXERCHECKRESULT(Mixer_Venc_StopChannel(m_veChn));

    if(!m_bDivpInit && (m_vencframeRate != oldvencFrameRate))
    {
        memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
        stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
        stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
        stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
        stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

        stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
        stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
        stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
        stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;


#if (TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0 )
        stBindInfo.u32BindParam = 0;
        stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
        stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
        if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
        {
#if TARGET_CHIP_I6B0
            if(m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)stBindInfo.u32BindParam = m_height/2;
            else stBindInfo.u32BindParam = m_height;
#else
            stBindInfo.u32BindParam = m_height;
#endif
        }
        else
        {
            stBindInfo.u32BindParam = 0;
        }
#endif
        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = oldvencFrameRate;
        MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

        stBindInfo.u32SrcFrmrate = vpeframeRate;
        stBindInfo.u32DstFrmrate = m_vencframeRate;
        MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
    }

    if(m_bDivpInit && (m_vencframeRate != oldvencFrameRate))
    {
 #if TARGET_CHIP_I5 || TARGET_CHIP_I6E
        if(m_pipCfg)
        {
            /************************************************
              Step1:  re-bind Venc to vdisp
            *************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stSrcChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stSrcChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;

            stBindInfo.u32SrcFrmrate = oldvencFrameRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


            /**********************************************************
              Step2:  re-bind divp(pip&vdisp) to vdisp
            **********************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_PIP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_PIP;

			stBindInfo.u32SrcFrmrate = oldvencFrameRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
			MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

			stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpSclDownChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_VDISP;
            stBindInfo.stDstChnPort.u32DevId  = MIXER_VDISP_DEVID_FOR_PIP;
            stBindInfo.stDstChnPort.u32ChnId  = MIXER_VDISP_CHNID_FOR_VDISP;
            stBindInfo.stDstChnPort.u32PortId = MIXER_VDISP_PORTID_FOR_VDISP;

			stBindInfo.u32SrcFrmrate = oldvencFrameRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
			MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


            /****************************************************
              Step3:  re-bind VPE to divp(for pip)
            *****************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclUpChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

			stBindInfo.u32SrcFrmrate = oldvencFrameRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
			MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));


            /******************************************************
              Step4:  re-bind VPE to divp(for vdisp)
            *******************************************************/
            memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId  = 0;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpSclDownChnId;
            stBindInfo.stDstChnPort.u32PortId = 0;

			stBindInfo.u32SrcFrmrate = oldvencFrameRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
			MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
        }
        else
#endif
        {
            memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = m_VpeChnPort.eModId;
            stBindInfo.stSrcChnPort.u32DevId  = m_VpeChnPort.u32DevId;
            stBindInfo.stSrcChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = m_VpeChnPort.u32PortId;

            stBindInfo.stDstChnPort.eModId    = m_DivpChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_DivpChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_DivpChnPort.u32PortId;


        #if (TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0)
            stBindInfo.u32BindParam = 0;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
        #if TARGET_CHIP_I6B0
            if(m_VpeChnPort.u32PortId == VPE_REALMODE_SUB_PORT)
            {
                stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
            }
        #endif
            if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
            {
        #if TARGET_CHIP_I6B0
                if(m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)stBindInfo.u32BindParam = m_height/2;
                else stBindInfo.u32BindParam = m_height;
        #else
                stBindInfo.u32BindParam = m_height;
        #endif
            }
            else
            {
                stBindInfo.u32BindParam = 0;
            }
        #endif
#if TARGET_CHIP_I6B0
            // vpe port 3 bind real mode DIVP ==(sensorfps:sensorfps)
            if(m_VpeChnPort.u32PortId != VPE_REALMODE_SUB_PORT)
            {
                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = oldvencFrameRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
            }
#else
            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = vpeframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
#endif
            memset(&stBindInfo, 0x0, sizeof(Mixer_Sys_BindInfo_T));
            stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId  = 0;
            stBindInfo.stSrcChnPort.u32ChnId  = m_DivpChnPort.u32ChnId;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId    = m_VencChnPort.eModId;
            stBindInfo.stDstChnPort.u32DevId  = m_VencChnPort.u32DevId;
            stBindInfo.stDstChnPort.u32ChnId  = m_VencChnPort.u32ChnId;
            stBindInfo.stDstChnPort.u32PortId = m_VencChnPort.u32PortId;


        #if (TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0)
            stBindInfo.u32BindParam = 0;
            stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
            stBindInfo.eBindType = coverMixerBindMode(m_bindMode);
            if(E_MI_SYS_BIND_TYPE_HW_RING == stBindInfo.eBindType)
            {
        #if TARGET_CHIP_I6B0
                if(m_bindMode == Mixer_Venc_Bind_Mode_HW_HALF_RING)stBindInfo.u32BindParam = m_height/2;
                else stBindInfo.u32BindParam = m_height;
        #else
                stBindInfo.u32BindParam = m_height;
        #endif
            }
            else
            {
                stBindInfo.u32BindParam = 0;
            }
        #endif
#if TARGET_CHIP_I6B0
            // vpe port 3 bind real mode DIVP ==(sensorfps:sensorfps)
            if(m_VpeChnPort.u32PortId != VPE_REALMODE_SUB_PORT)
            {
                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = oldvencFrameRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                stBindInfo.u32SrcFrmrate = m_vencframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
            }
            else
            {
                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = oldvencFrameRate;
                MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

                stBindInfo.u32SrcFrmrate = vpeframeRate;
                stBindInfo.u32DstFrmrate = m_vencframeRate;
                MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
            }
#else
            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = oldvencFrameRate;
            MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

            stBindInfo.u32SrcFrmrate = m_vencframeRate;
            stBindInfo.u32DstFrmrate = m_vencframeRate;
            MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));
#endif
        }
    }

    memset(&stChnAttr, 0x00, sizeof(MI_VENC_ChnAttr_t));
    ExecFunc(MI_VENC_GetChnAttr(m_veChn, &stChnAttr), MI_SUCCESS);

    eRcModeType = stChnAttr.stRcAttr.eRcMode;
    enType = stChnAttr.stVeAttr.eType;
    MIXER_DBG("the bitrate is %d\n", stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate);
    MI_U32 tFnum = 0x0, tFden = 0x0;

    #if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    tFnum = m_vencframeRate & 0xffff;
    tFden = (m_vencframeRate>>16) & 0xffff;
    if(0x0 == tFden)
    {
        tFden = 1;
    }
    #else
    tFnum = m_vencframeRate;
    tFden = 1;
    #endif

    switch(enType)
    {
        case E_MI_VENC_MODTYPE_H264E:
            if(E_MI_VENC_RC_MODE_H264CBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = tFden;


            }
            else if(E_MI_VENC_RC_MODE_H264VBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = tFden;

            }
            else
            {
                MIXER_ERR("not init rcmode  type, eRcModeType:%d\n", eRcModeType);
            }
            break;

        case E_MI_VENC_MODTYPE_H265E:
            if(E_MI_VENC_RC_MODE_H265CBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = tFden;
            }
            else if(E_MI_VENC_RC_MODE_H265VBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = tFden;
            }
            else
            {
                MIXER_ERR("not init rcmode  type, eRcModeType:%d\n", eRcModeType);
            }
            break;

        case E_MI_VENC_MODTYPE_JPEGE:
            break;

        default :
            MIXER_ERR("not init video type, error video encoder type:%d\n", enType);
            break;
    }
    if(bChangeBitrate)
    {
        MI_U32 bitRate;
        MI_U32 tmpFrameRate = m_vencframeRate;
        float newFrameRate = 0.0;
        if(0 != (tmpFrameRate & 0xffff0000))
        {
            newFrameRate = (tmpFrameRate & 0xffff) * 1.0 / ((tmpFrameRate & 0xffff0000) >> 16);
        }
        else
        {
            newFrameRate = tmpFrameRate * 1.0;
        }
        if(E_MI_VENC_MODTYPE_H264E == enType)
        {
            if(E_MI_VENC_RC_MODE_H264CBR == eRcModeType)
            {
                bitRate = stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate;
                if(0 == (oldvencFrameRate & 0xffff0000))
                {
                    bitRate *= newFrameRate / oldvencFrameRate;
                }
                else
                {
                    bitRate *= newFrameRate / ((oldvencFrameRate & 0xffff) * 1.0 / ((oldvencFrameRate & 0xffff0000) >> 16));
                }
                stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = bitRate;
            }
            else if(E_MI_VENC_RC_MODE_H264VBR == eRcModeType)
            {
                bitRate = stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate;
                if(0 == (oldvencFrameRate & 0xffff0000))
                {
                    bitRate *= newFrameRate / oldvencFrameRate;
                }
                else
                {
                    bitRate *= newFrameRate / ((oldvencFrameRate & 0xffff) * 1.0 / ((oldvencFrameRate & 0xffff0000) >> 16));
                }
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate= bitRate;
            }
            else
            {
                MIXER_ERR("not init rcmode type, eRcModeType:%d\n", eRcModeType);
            }
        }
        else if(E_MI_VENC_MODTYPE_H265E == enType)
        {
            if(E_MI_VENC_RC_MODE_H265CBR == eRcModeType)
            {
                bitRate = stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate;
                if(0 == (oldvencFrameRate & 0xffff0000))
                {
                    bitRate *= newFrameRate / oldvencFrameRate;
                }
                else
                {
                    bitRate *= newFrameRate / ((oldvencFrameRate & 0xffff) * 1.0 / ((oldvencFrameRate & 0xffff0000) >> 16));
                }
                stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = bitRate;
            }
            else if(E_MI_VENC_RC_MODE_H265VBR == eRcModeType)
            {
                bitRate = stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate;
                if(0 == (oldvencFrameRate & 0xffff0000))
                {
                    bitRate *= newFrameRate / oldvencFrameRate;
                }
                else
                {
                    bitRate *= newFrameRate / ((oldvencFrameRate & 0xffff) * 1.0 / ((oldvencFrameRate & 0xffff0000) >> 16));
                }
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitRate;
            }
            else
            {
                MIXER_ERR("not init rcmode type, eRcModeType:%d\n", eRcModeType);
            }
        }
        else if(E_MI_VENC_MODTYPE_JPEGE == enType)
        {
        }
        else
        {
            MIXER_ERR("not init video type, enType:%d\n", enType);
        }
    }

    if(MI_SUCCESS == MI_VENC_SetChnAttr(m_veChn, &stChnAttr))
    {
        //setGop(m_vencframeRate * 2);
    }

    MIXERCHECKRESULT(Mixer_Venc_StartChannel(m_veChn));

    return 0;
}

MI_S32 MI_VideoEncoder::changeCodec(int * param)
{
    Mixer_VPE_PortInfo_T stVpePortInfo;
    int videoParam[9];
    //MI_VPE_PortMode_t stVpeMode;
    memcpy(videoParam, param, sizeof(videoParam));

    //MI_S32 videoIdx = videoParam[0];
    Mixer_EncoderType_e type = (Mixer_EncoderType_e)videoParam[1];
    MI_S32 qfactor = videoParam[2];
    MI_S32 videoWidth = videoParam[3];
    MI_S32 videoHeight = videoParam[4];
    MI_U32 vencframeRate = videoParam[5];
    MI_U32 videoGop = videoParam[6];
    MI_U32 VideoProfile = videoParam[7];
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    MI_U32 bindtype = videoParam[8];
#endif

    /*
     * param[0] = vIndex;      // video index
     * param[1] = codecType;   // video codec type
     * param[2] = qfactor;     //
     * param[3] = size.width;  // video new resolution
     * param[4] = size.height; // video new resolution
     * param[5] = frameparam;  // video new fps(divp,vdisp,venc)
     * param[6] = vgop;        // video new gop
    */

    if(VE_AVC != type && VE_H265 != type && VE_MJPEG != type)
    {
        MIXER_ERR("%s: type %d not support\n", __FUNCTION__, type);
        return -1;
    }

    if(type == m_encoderType)
    {
        MIXER_WARN("%s: The current codetype:%d is same as the old one \n", __FUNCTION__, type);
        //return -1;
        //may be not change encoder,it can be change bind type or resolution.
    }

    //step 1: venc stop receive pic
    stopVideoEncoder();

    //step 2: destroy divp,vdisp & venc
    uninitVideo(MIXER_SYS_INPUT_VPE);

    switch(type)
    {

        case 1:
            m_encoderType = VE_H265;
        break;
        case 2:
            m_encoderType = VE_JPG;
        break;
     case 0:
     default:
             m_encoderType = VE_AVC;
        break;
    }

    if((VE_JPG == m_encoderType) || (VE_MJPEG == m_encoderType))
    {
        if(qfactor < 20 || qfactor > 90)
        {
            MIXER_WARN("qfactor must between 1 and 100.\n");
            return -1;
        }
        m_qfactor = qfactor;
    }

    m_gop = videoGop;

    m_width = videoWidth;
    m_height = videoHeight;
    m_vencframeRate = vencframeRate;
    m_VideoProfile = VideoProfile;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    m_bindMode = (Mixer_Venc_Bind_Mode_E)bindtype;
#endif
    memset(&stVpePortInfo, 0x00, sizeof(Mixer_VPE_PortInfo_T));
    stVpePortInfo.VpeChnPort.eModId    = m_VpeChnPort.eModId;
    stVpePortInfo.VpeChnPort.u32DevId  = m_VpeChnPort.u32DevId;
    stVpePortInfo.VpeChnPort.u32ChnId  = m_VpeChnPort.u32ChnId;
    stVpePortInfo.VpeChnPort.u32PortId = m_VpeChnPort.u32PortId;
    stVpePortInfo.eCompressMode        = E_MI_SYS_COMPRESS_MODE_NONE;
    stVpePortInfo.ePixelFormat         = m_eVpeOutportPixelFormat;

    stVpePortInfo.u16OutputHeight      = m_height;
    stVpePortInfo.u16OutputWidth       = m_width;
    stVpePortInfo.u16VpeOutputWidth = videoWidth;
    stVpePortInfo.u16VpeOutputHeight =  videoHeight;

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    //change resolution: do not change vpe port2 output resolution,only change divp output resolution and venc resolution.
    if((2 == m_VpeChnPort.u32PortId) && (TRUE == m_bUseDivp))
    {
        MI_VPE_PortMode_t stVpeMode;
        stVpePortInfo.u16OutputHeight      = m_VpeOutputHeight;
        stVpePortInfo.u16OutputWidth       = m_VpeOutputWidth;
        stVpePortInfo.u16VpeOutputWidth = m_VpeOutputWidth;
        stVpePortInfo.u16VpeOutputHeight =  m_VpeOutputHeight;
        memset(&stVpeMode, 0x00, sizeof(stVpeMode));
        ExecFunc(MI_VPE_GetPortMode(m_VpeChnPort.u32ChnId, m_VpeChnPort.u32PortId, &stVpeMode), MI_VPE_OK);
        if((stVpeMode.u16Width > stVpePortInfo.u16OutputWidth)||(stVpeMode.u16Height > stVpePortInfo.u16OutputWidth))
        {
            stVpePortInfo.u16OutputHeight      = stVpeMode.u16Height;
            stVpePortInfo.u16OutputWidth       = stVpeMode.u16Width;
        }
        if((stVpeMode.u16Width > stVpePortInfo.u16VpeOutputWidth)||(stVpeMode.u16Height > stVpePortInfo.u16VpeOutputHeight))
        {
           stVpePortInfo.u16VpeOutputWidth = stVpeMode.u16Width;
           stVpePortInfo.u16VpeOutputHeight =  stVpeMode.u16Height;
        }
    }
    else if(2 == m_VpeChnPort.u32PortId && MI_VideoEncoder::bVpePort2share==TRUE)
    {
        stVpePortInfo.u16OutputHeight      = m_VpeOutputHeight;
        stVpePortInfo.u16OutputWidth       = m_VpeOutputWidth;
        stVpePortInfo.u16VpeOutputWidth = m_VpeOutputWidth;
        stVpePortInfo.u16VpeOutputHeight =  m_VpeOutputHeight;
    }
#endif

    //step 3: create divp & vdisp
    initVideoInput(&stVpePortInfo);

    //step 4: create & enable venc
    initVideoEncoder();
    startVideoEncoder();

    return 0;
}

VOID MI_VideoEncoder::setVirGop(MI_U32 enhance, MI_U32 virtualIEnable)
{
    MI_VENC_ParamRef_t stRefParam;
    m_virtualIInterval = enhance;
    //m_virtualIEnable = virtualIEnable; //if start is ref I,now it still ref I,if start is ref P,now it still ref P.
    if(m_encoderType == VE_AVC || m_encoderType == VE_H265)
    {
        MI_VENC_GetRefParam(m_veChn, &stRefParam);
        if(m_virtualIEnable == TRUE && m_virtualIInterval > 0)
        {
             //Reference I frame
          stRefParam.u32Base = 1;
          stRefParam.bEnablePred = FALSE;
          stRefParam.u32Enhance = m_virtualIInterval-1;
         }else if(m_virtualIEnable == FALSE && m_virtualIInterval > 0)
         {
              //Reference P frame
           stRefParam.u32Base = 1;
           stRefParam.bEnablePred = TRUE;
           stRefParam.u32Enhance = m_virtualIInterval-1;
         }
         else
         {
             // disable LTR
             stRefParam.u32Base = 1;
             stRefParam.bEnablePred = FALSE;
             stRefParam.u32Enhance = 0;
         }
         MI_VENC_SetRefParam(m_veChn, &stRefParam);
    }
    else {
        printf("[%s]encoder Type error\n",__func__);
    }
    printf("set virtualInterval gop enhance[%d] ok\n",enhance);
}

void MI_VideoEncoder::setGop(MI_U32 gop)
{
    MI_VENC_ChnAttr_t stChnAttr;
    MI_VENC_RcMode_e eRcModeType = E_MI_VENC_RC_MODE_MAX;
    MI_VENC_ModType_e enType = E_MI_VENC_MODTYPE_VENC;

    if(MI_SUCCESS != MI_VENC_GetChnAttr(m_veChn, &stChnAttr))
    {
    MIXER_ERR("venc:%d can not get gop\n", m_veChn);
    return;
    }

    eRcModeType = stChnAttr.stRcAttr.eRcMode;
    enType = stChnAttr.stVeAttr.eType;
    switch(enType)
    {
        case E_MI_VENC_MODTYPE_H264E:
            if(E_MI_VENC_RC_MODE_H264CBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = gop;
            }
            else if(E_MI_VENC_RC_MODE_H264VBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = gop;
            }
            else if(E_MI_VENC_RC_MODE_H264FIXQP == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop= gop;
            }
            else
            {
                MIXER_ERR("not init rcmode  type, eRcModeType:%d\n", eRcModeType);
            }
            break;

        case E_MI_VENC_MODTYPE_H265E:
            if(E_MI_VENC_RC_MODE_H265CBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = gop;
            }
            else if(E_MI_VENC_RC_MODE_H265VBR == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = gop;
            }
            else if(E_MI_VENC_RC_MODE_H265FIXQP == eRcModeType)
            {
                stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = gop;
            }
            else
            {
                MIXER_ERR("not init rcmode type, eRcModeType:%d\n", eRcModeType);
            }
            break;

        default :
            MIXER_ERR("not init video type, error video encoder type:%d (E_MI_VENC_MODTYPE_JPEGE)\n", enType);
            return;
    }

    if(MI_SUCCESS != MI_VENC_SetChnAttr(m_veChn, &stChnAttr))
    {
      MIXER_ERR("venc:%d can not set gop\n", m_veChn);
      return;
    }
    printf("Venc channel %d new gop is %d\n",m_veChn,gop);
    m_gop = gop;
}

MI_S32 MI_VideoEncoder::SetRoiCfg(MI_VENC_RoiCfg_t *pstRoiCfg)
{
    MI_VENC_RoiCfg_t stvencRoiCfg;
    MI_U32 index =-1;
    MI_U32 u32Left =0;
    MI_U32 u32Top = 0;
    MI_U32 u32Width =0;
    MI_U32 u32Height =0;

    if(VE_AVC == m_encoderType)
    {
        u32Top  = ALIGN_16xUP(pstRoiCfg->stRect.u32Top);
        u32Left = ALIGN_16xUP(pstRoiCfg->stRect.u32Left);
        u32Width  = ALIGN_16xUP(pstRoiCfg->stRect.u32Width);
        u32Height = ALIGN_16xUP(pstRoiCfg->stRect.u32Height);
    }
    else if(VE_H265 == m_encoderType)
    {
        u32Top  = ALIGN_32xUP(pstRoiCfg->stRect.u32Top);
        u32Left = ALIGN_32xUP(pstRoiCfg->stRect.u32Left);
        u32Width  = ALIGN_32xUP(pstRoiCfg->stRect.u32Width);
        u32Height = ALIGN_32xUP(pstRoiCfg->stRect.u32Height);
    }
    else
    {
        MIXER_ERR("only support H264 && H265, so setfail, current coderType: %d\n", m_encoderType);
        return -1;
    }

    Mixer_API_ISVALID_POINT(pstRoiCfg);
    memset(&stvencRoiCfg, 0, sizeof(MI_VENC_RoiCfg_t));
    index = pstRoiCfg->u32Index;

    ExecFunc(MI_VENC_GetRoiCfg(m_veChn,index,&stvencRoiCfg), MI_SUCCESS);

    stvencRoiCfg.bEnable = pstRoiCfg->bEnable;
    stvencRoiCfg.bAbsQp     = pstRoiCfg->bAbsQp;
    stvencRoiCfg.s32Qp = pstRoiCfg->s32Qp;
    stvencRoiCfg.stRect.u32Left = u32Left;
    stvencRoiCfg.stRect.u32Top = u32Top;
    stvencRoiCfg.stRect.u32Width = u32Width;
    stvencRoiCfg.stRect.u32Height = u32Height;
    stvencRoiCfg.u32Index = index;
    ExecFunc(MI_VENC_SetRoiCfg(m_veChn,&stvencRoiCfg), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 MI_VideoEncoder::setRcParamEx(MI_S32* pRcParam)
{
    MI_S32 crtl = -1;
    MI_VENC_RcParam_t stRcParam;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_VENC_ChnAttr_t stChnAttrGet;
    MI_VENC_SuperFrameCfg_t stSuperFrameCfg;

    MI_U32 tFnum = 0x0, tFden = 0x0;
    memset(&stSuperFrameCfg,0x00,sizeof(MI_VENC_SuperFrameCfg_t));
    #if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    tFnum = m_vencframeRate & 0xffff;
    tFden = (m_vencframeRate>>16) & 0xffff;
    if(0x0 == tFden)
    {
        tFden = 1;
    }
    #else
    tFnum = m_vencframeRate;
    tFden = 1;
    #endif

    if((VE_H265 != m_encoderType) && (VE_AVC != m_encoderType) && (VE_JPG != m_encoderType) && (VE_MJPEG != m_encoderType))
    {
        MIXER_ERR("%s: not support rc param, encoder type = %d\n", __func__, m_encoderType);
        return -1;
    }

    Mixer_API_ISVALID_POINT(pRcParam);

    memset(&stChnAttr, 0x00, sizeof(MI_VENC_ChnAttr_t));
    memset(&stChnAttrGet, 0x00, sizeof(MI_VENC_ChnAttr_t));

    ExecFunc(MI_VENC_GetChnAttr(m_veChn, &stChnAttrGet), MI_SUCCESS);
    memcpy(&stChnAttr.stVeAttr, &stChnAttrGet.stVeAttr, sizeof(MI_VENC_Attr_t));

    crtl = pRcParam[1];
    switch (stChnAttr.stVeAttr.eType)
    {
        case E_MI_VENC_MODTYPE_H265E:
            if(0 == crtl) // cbr
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pRcParam[2];
                stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = tFden;
                //stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
                //stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            }
            else if(1 == crtl) // vbr
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pRcParam[2];
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = pRcParam[4];
                stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = tFden;
                //stChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
                //stChnAttr.stRcAttr.stAttrH265Vbr.u32FluctuateLevel = 0;
            }
            else if(2 == crtl) // fixqp
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp =pRcParam[2];
                stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = tFden;
            }
          else if(3 == crtl) //avbr
              {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265AVBR;
                stChnAttr.stRcAttr.stAttrH265Avbr.u32MaxBitRate = pRcParam[2];
                stChnAttr.stRcAttr.stAttrH265Avbr.u32MaxQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH265Avbr.u32MinQp = pRcParam[4];
                stChnAttr.stRcAttr.stAttrH265Avbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH265Avbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH265Avbr.u32SrcFrmRateDen = tFden;
          }
            else
            {
                MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
            }
            break;

        case E_MI_VENC_MODTYPE_H264E:
            if(0 == crtl) // cbr
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pRcParam[2];
                stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = tFden;
                //stChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
                //stChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            }
            else if(1 == crtl) // vbr
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate= pRcParam[2];
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = pRcParam[4];
                stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = tFden;
                //stChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
                //stChnAttr.stRcAttr.stAttrH264Vbr.u32FluctuateLevel = 0;
            }
            else if(2 == crtl) // fixqp
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = pRcParam[2];
                stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = tFden;
            }
        else if(3 == crtl)
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264AVBR;
                stChnAttr.stRcAttr.stAttrH264Avbr.u32MaxBitRate= pRcParam[2];
                stChnAttr.stRcAttr.stAttrH264Avbr.u32MaxQp = pRcParam[3];
                stChnAttr.stRcAttr.stAttrH264Avbr.u32MinQp = pRcParam[4];
                stChnAttr.stRcAttr.stAttrH264Avbr.u32Gop = m_gop;
                stChnAttr.stRcAttr.stAttrH264Avbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrH264Avbr.u32SrcFrmRateDen = tFden;
        }
            else
            {
                MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
            }
            break;

        case E_MI_VENC_MODTYPE_JPEGE:
            if(0 == crtl) // cbr
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGCBR;
                stChnAttr.stRcAttr.stAttrMjpegCbr.u32BitRate = pRcParam[2];
                stChnAttr.stRcAttr.stAttrMjpegCbr.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrMjpegCbr.u32SrcFrmRateDen = tFden;
            }
            else if(2 == crtl) // fixqp
            {
                stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
                stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor =pRcParam[2];
                stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateNum = tFnum;
                stChnAttr.stRcAttr.stAttrMjpegFixQp.u32SrcFrmRateDen = tFden;
            }
            else
            {
                MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
            }
            break;

        default:
            break;
    }

    ExecFunc(MI_VENC_SetChnAttr(m_veChn, &stChnAttr), MI_SUCCESS);

    if(2 != crtl)
    {
        memset(&stRcParam, 0x00, sizeof(MI_VENC_RcParam_t));
        ExecFunc(MI_VENC_GetRcParam(m_veChn, &stRcParam), MI_SUCCESS);

        switch (stChnAttr.stVeAttr.eType)
        {
            case E_MI_VENC_MODTYPE_H265E:
                if(0 == crtl) // cbr
                {
                    stRcParam.stParamH265Cbr.u32MaxQp = pRcParam[3];
                    stRcParam.stParamH265Cbr.u32MinQp = pRcParam[4];
                    stRcParam.stParamH265Cbr.u32MaxIQp = pRcParam[5];
                    stRcParam.stParamH265Cbr.u32MinIQp = pRcParam[6];
                    stRcParam.stParamH265Cbr.s32IPQPDelta = pRcParam[7];
                }
                else if(1 == crtl) // vbr
                {
                    stRcParam.stParamH265Vbr.u32MaxIQp    = pRcParam[5];
                    stRcParam.stParamH265Vbr.u32MinIQp    = pRcParam[6];
                    stRcParam.stParamH265Vbr.s32IPQPDelta = pRcParam[7];
                    stRcParam.stParamH265Vbr.s32ChangePos = pRcParam[8];
                }
                else if(3 == crtl)     //avbr
                {
                    stRcParam.stParamH265Avbr.u32MaxIQp    = pRcParam[5];
                    stRcParam.stParamH265Avbr.u32MinIQp    = pRcParam[6];
                    stRcParam.stParamH265Avbr.s32IPQPDelta = pRcParam[7];
                    stRcParam.stParamH265Avbr.s32ChangePos = pRcParam[8];
                    stRcParam.stParamH265Avbr.u32MinStillPercent    = pRcParam[9];
                    stRcParam.stParamH265Avbr.u32MaxStillQp = pRcParam[10];
                    stRcParam.stParamH265Avbr.u32MotionSensitivity = pRcParam[11];
                    MIXER_DBG("changepos:%d, percent:%d,u32MaxStillQp:%d,u32MotionSensitivity:%d.\n",
                        pRcParam[8], pRcParam[9],pRcParam[10],pRcParam[11]);
                }
                else
                {
                    MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
                }
                break;

            case E_MI_VENC_MODTYPE_H264E:
                if(0 == crtl) // cbr
                {
                    stRcParam.stParamH264Cbr.u32MaxQp  = pRcParam[3];
                    stRcParam.stParamH264Cbr.u32MinQp  = pRcParam[4];
                    stRcParam.stParamH264Cbr.u32MaxIQp = pRcParam[5];
                    stRcParam.stParamH264Cbr.u32MinIQp = pRcParam[6];
                    stRcParam.stParamH264Cbr.s32IPQPDelta = pRcParam[7];
                }
                else if(1 == crtl) // vbr
                {
                    stRcParam.stParamH264VBR.u32MaxIQp    = pRcParam[5];
                    stRcParam.stParamH264VBR.u32MinIQp    = pRcParam[6];
                    stRcParam.stParamH264VBR.s32IPQPDelta = pRcParam[7];
                    stRcParam.stParamH264VBR.s32ChangePos = pRcParam[8];
                }
          else if(3 == crtl) //avbr
                {
                    stRcParam.stParamH264Avbr.u32MaxIQp    = pRcParam[5];
                    stRcParam.stParamH264Avbr.u32MinIQp    = pRcParam[6];
                    stRcParam.stParamH264Avbr.s32IPQPDelta = pRcParam[7];
                    stRcParam.stParamH264Avbr.s32ChangePos = pRcParam[8];
                    stRcParam.stParamH264Avbr.u32MinStillPercent    = pRcParam[9];
                    stRcParam.stParamH264Avbr.u32MaxStillQp = pRcParam[10];
                    stRcParam.stParamH264Avbr.u32MotionSensitivity = pRcParam[11];
                    MIXER_DBG("changepos:%d, percent:%d,u32MaxStillQp:%d,u32MotionSensitivity:%d.\n",
                        pRcParam[8], pRcParam[9],pRcParam[10],pRcParam[11]);
                }
                else
                {
                    MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
                }
                break;

            case E_MI_VENC_MODTYPE_JPEGE:
                if(0 == crtl) // cbr
                {
                    stRcParam.stParamMjpegCbr.u32MaxQfactor = pRcParam[3];
                    stRcParam.stParamMjpegCbr.u32MinQfactor = pRcParam[4];
                }
                else
                {
                    MIXER_ERR("%s: not support crtl = %d\n", __func__, crtl);
                }
                break;

            default:
                break;
        }

        ExecFunc(MI_VENC_SetRcParam(m_veChn, &stRcParam), MI_SUCCESS);
    }

#if 0
    if(g_SuperFrameEnable)
    {
        if(0 == crtl) // cbr
        {
            stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE;
            stSuperFrameCfg.u32SuperIFrmBitsThr = pRcParam[2];
            stSuperFrameCfg.u32SuperPFrmBitsThr = (pRcParam[2] / SUPERIPFRMBITDEN) * SUPERPFRMBITMOL;
            stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;
        }
        else if(1 == crtl) // vbr
        {
            stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE;
            stSuperFrameCfg.u32SuperIFrmBitsThr = (pRcParam[2] / SUPERIPFRMBITDEN) * SUPERIFRMBITMOL;
            stSuperFrameCfg.u32SuperPFrmBitsThr = (pRcParam[2] / SUPERIPFRMBITDEN) * SUPERPFRMBITMOL;
            stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;
        }

        if(2 != crtl)
        {
            s32Ret = MI_VENC_SetSuperFrameCfg(m_veChn, &stSuperFrameCfg);
            if(s32Ret)
            {
                printf("%s %d MI_VENC_SetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
            }
            else
            {
                printf("MI_VENC_SetSuperFrameCfg Success, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                        stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
            }
        }
    }
#endif
    return 0;
}

#if TARGET_CHIP_I5
MI_S32 MI_VideoEncoder::setSuperFrm(MI_U32 superIFrmBitsThr, MI_U32 superPFrmBitsThr)
{
    MI_S32 s32Ret = -1;
    MI_VENC_SuperFrameCfg_t stSuperFrameCfg;


    memset(&stSuperFrameCfg, 0, sizeof(MI_VENC_SuperFrameCfg_t));
    s32Ret = MI_VENC_GetSuperFrameCfg(m_veChn, &stSuperFrameCfg);
    if(s32Ret)
    {
        printf("%s %d MI_VENC_GetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
    }
    else
    {
        printf("MI_VENC_GetSuperFrameCfg Success, venChId=%d, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                        m_veChn, stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
    }

    if(0 < superIFrmBitsThr)
    {
        stSuperFrameCfg.u32SuperIFrmBitsThr = superIFrmBitsThr;
    }

    if(0 < superPFrmBitsThr)
    {
        stSuperFrameCfg.u32SuperPFrmBitsThr = superPFrmBitsThr;
    }

    if((0 < superIFrmBitsThr) || (0 < superPFrmBitsThr))
    {
        stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE;
        stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;

        s32Ret = MI_VENC_SetSuperFrameCfg(m_veChn, &stSuperFrameCfg);
        if(s32Ret)
        {
            printf("%s %d MI_VENC_SetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
        }
        else
        {
            printf("MI_VENC_SetSuperFrameCfg Success, venChId=%d, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                            m_veChn, stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
        }
    }

    return 0;
}
#elif (TARGET_CHIP_I6 || TARGET_CHIP_I6B0 || TARGET_CHIP_I6E)
MI_S32 MI_VideoEncoder::setSuperFrm(MI_U32 superIFrmBitsThr, MI_U32 superPFrmBitsThr)
{
    MI_S32 s32Ret = -1;
    MI_VENC_SuperFrameCfg_t stSuperFrameCfg;


    memset(&stSuperFrameCfg, 0, sizeof(MI_VENC_SuperFrameCfg_t));
    s32Ret = MI_VENC_GetSuperFrameCfg(m_veChn, &stSuperFrameCfg);
    if(s32Ret)
    {
        printf("%s %d MI_VENC_GetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
    }
    else
    {
        printf("MI_VENC_GetSuperFrameCfg Success, venChId=%d, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                        m_veChn, stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
    }

    if(0 < superIFrmBitsThr)
    {
        stSuperFrameCfg.u32SuperIFrmBitsThr = superIFrmBitsThr;
    }

    if(0 < superPFrmBitsThr)
    {
        stSuperFrameCfg.u32SuperPFrmBitsThr = superPFrmBitsThr;
    }

    if((0 < superIFrmBitsThr) || (0 < superPFrmBitsThr))
    {
        //stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_DISCARD;  superfrm mode is set by console command superframemode
        stSuperFrameCfg.u32SuperBFrmBitsThr = SUPERBFRMBITSTHR;

        s32Ret = MI_VENC_SetSuperFrameCfg(m_veChn, &stSuperFrameCfg);
        if(s32Ret)
        {
            printf("%s %d MI_VENC_SetSuperFrameCfg error, %X\n", __func__, __LINE__, s32Ret);
        }
        else
        {
            printf("MI_VENC_SetSuperFrameCfg Success, venChId=%d, SuperIFrmBitsThr=%d, SuperPFrmBitsThr=%d\n",
                            m_veChn, stSuperFrameCfg.u32SuperIFrmBitsThr, stSuperFrameCfg.u32SuperPFrmBitsThr);
        }
    }

    return 0;
}

#endif
