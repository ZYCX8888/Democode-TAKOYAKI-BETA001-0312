/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#ifdef __cplusplus
extern "C"  {
#endif
#include "mi_sys.h"
#include "st_hdmi.h"
#include "st_common.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"
#include "mi_venc.h"
#include "st_fb.h"
//#include "st_warp.h"
#include "list.h"
#include "tem.h"

#include "i2c.h"
#include "mi_venc.h"
#include "mi_vpe.h"
#include "mi_vdisp.h"
#include "mi_divp.h"

#ifdef __cplusplus
}
#endif
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include "Live555RTSPServer.hh"
extern "C" {

void* OpenStream(char const* StreamName, void* arg);
int   CloseStream(void* handle, void* arg);
int   VideoReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg);
int   AACReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg);
int   WAVReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg);
int Live555RTSPServerSet(Live555RTSPServer* mLive555RTSPServer);

}
typedef struct
{
	int VencChn;
	MI_VENC_ModType_e VencType;
	pthread_t getStream_thread;
	unsigned char thread_ExitFlag;
	unsigned char* buf;	int iSumLen;
	int iUsedLen;
}StreamBuf;
Live555RTSPServer* mLive555RTSPServer;


pthread_t pt;
static MI_BOOL _bThreadRunning = FALSE;

static MI_BOOL g_subExit = FALSE;
static MI_BOOL g_bExit = FALSE;
static MI_U32 g_u32SubCaseIndex = 0;

static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;
static MI_S32 g_s32AdWorkMode = 0;

#define MAX_VIF_DEV_NUM 4
#define MAX_VIF_CHN_NUM 16

#define SUPPORT_VIDEO_ENCODE
#define SUPPORT_VIDEO_ENCODE_H264_H265
//#define SUPPORT_VIDEO_ENCODE_MJPEG
//#define SUPPORT_VIDEO_VPE2

//#define SUPPORT_WARP
#define SUPPORT_VIDEO_PREVIEW

//#define SUPPORT_WRITE_FILE

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_U32 u32MainWidth;
    MI_U32 u32MainHeight;
    MI_VENC_ModType_e eType;
    int vencFd;
} VENC_Attr_t;

typedef struct
{
    pthread_t ptGetEs;
    pthread_t ptFillYuv;
    VENC_Attr_t stVencAttr[MAX_VIF_CHN_NUM];
    MI_U32 u32ChnNum;
    MI_BOOL bRunFlag;
} Venc_Args_t;

Venc_Args_t g_stVencArgs[MAX_VIF_CHN_NUM];

typedef struct ST_ChnInfo_s
{
    MI_S32 s32VideoFormat; //720P 1080P ...
    MI_S32 s32VideoType; //CVI TVI AHD CVBS ...
    MI_S32 s32VideoLost;
    MI_U8 u8EnChannel;
    MI_U8 u8ViDev;
    MI_U8 u8ViChn;
    MI_U8 u8ViPort; //main or sub
} ST_ChnInfo_t;

typedef struct ST_TestInfo_s
{
    ST_ChnInfo_t stChnInfo[MAX_VIF_CHN_NUM];
} ST_TestInfo_t;

//Config logic chn trans to phy chn
ST_VifChnConfig_t stVifChnCfg[VIF_MAX_CHN_NUM] = {
    {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, //16main
    {0, 4, 0}, {0, 5, 0}, {0, 6, 0}, {0, 7, 0},
    {0, 8, 0}, {0, 9, 0}, {0, 10, 0}, {0, 11, 0},
    {0, 12, 0}, {0, 13, 0}, {0, 14, 0}, {0, 15, 0},
    {0, 0, 1}, {0, 1, 1}, {0, 2, 1}, {0, 3, 1}, //16sub
    {0, 4, 1}, {0, 5, 1}, {0, 6, 1}, {0, 7, 1},
    {0, 8, 1}, {0, 9, 1}, {0, 10, 1}, {0, 11, 1},
    {0, 12, 1}, {0, 13, 1}, {0, 14, 1}, {0, 15, 1},
};

extern ST_CaseDesc_t g_stVifCaseDesc[2];

void ST_VifUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 i = 0;

    for (i = 0; i < u32Size; i ++)
    {
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
    }
}

void ST_CaseSubUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 i = 0;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    for (i = 0; i < pstCaseDesc[u32CaseIndex].u32SubCaseNum; i ++)
    {
        printf("\t%d) %s\n", pstCaseDesc[u32CaseIndex].stSubDesc[i].u32CaseIndex + 1,
            pstCaseDesc[u32CaseIndex].stSubDesc[i].szDesc);
    }
}
static MI_SYS_ChnPort_t gstChnPort;
static struct pollfd gpfd[1] =
{
    {0, POLLIN | POLLERR},
};

#ifdef SUPPORT_WRITE_FILE

static void * ST_DoWaitData(ST_TEM_BUFFER stBuffer, pthread_mutex_t *pMutex)
{
    MI_SYS_ChnPort_t *pstChnPort = NULL;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32WriteSize = 0;
    int rval = 0;
    int fd= 0;
    int write_len= 0;

    pstChnPort = &gstChnPort;
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    rval = poll(gpfd, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        return 0;
    }
    if(rval == 0)
    {
        printf("get fd time out!\n");
        return 0;
    }
    if((gpfd[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        return 0;
    }
    if(MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("GetBuf fail\n");
        return NULL;
    }
    fd = *((int *)stBuffer.pTemBuffer);
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            u32WriteSize = stBufInfo.stRawData.u32ContentSize;
            write_len = write(fd, stBufInfo.stRawData.pVirAddr, u32WriteSize);
            ASSERT(write_len == u32WriteSize);
        }
        break;
        case E_MI_SYS_BUFDATA_FRAME:
        {
            u32WriteSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write_len = write(fd, stBufInfo.stFrameData.pVirAddr[0], u32WriteSize);
            ASSERT(write_len == u32WriteSize);
            u32WriteSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] / 2;
            write_len = write(fd, stBufInfo.stFrameData.pVirAddr[0], u32WriteSize);
            ASSERT(write_len == u32WriteSize);
        }
        break;
        default:
            ASSERT(0);
    }

    MI_SYS_ChnOutputPortPutBuf(stBufHandle);

    return NULL;
}
static void ST_TemDestroy(void)
{
    int fd;

    TemGetBuffer("Wait data", &fd);
    TemClose("Wait data");
    close(fd);

}
static void ST_TemCreate(void)
{
    ST_TEM_ATTR stAttr;
    pthread_attr_t m_SigMonThreadAttr;
    MI_U32 u32DevId = 0;
    MI_S32 s32Ret = 0;
    ST_TEM_USER_DATA stUserData;
    int fd = 0;

    fd = open("./venc_data", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0)
    {
        perror("open");
        return;
    }
    PTH_RET_CHK(pthread_attr_init(&m_SigMonThreadAttr));
    memset(&stAttr, 0, sizeof(ST_TEM_ATTR));
    stAttr.fpThreadDoSignal = NULL;
    stAttr.fpThreadWaitTimeOut = ST_DoWaitData;
    stAttr.thread_attr = m_SigMonThreadAttr;
    stAttr.u32ThreadTimeoutMs = 33;
    stAttr.bSignalResetTimer = 0;
    stAttr.stTemBuf.pTemBuffer = (void *)&fd;
    stAttr.stTemBuf.u32TemBufferSize = sizeof(int);
    TemOpen("Wait data", stAttr);

    TemStartMonitor("Wait data");
}
#endif
static MI_U32 ST_DoGetData(MI_SYS_ChnPort_t * pstChnPort, void *pData)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;
    MI_U32 u32RetSize = 0;
    MI_U16 i = 0;
    MI_U8 *u8CopyData = (MI_U8 *)pData;
    int rval = 0;

    ASSERT(pstChnPort);
    ASSERT(pData);

    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufHandle, 0, sizeof(MI_SYS_BUF_HANDLE));
    rval = poll(gpfd, 1, 200);
    if(rval < 0)
    {
        printf("poll error!\n");
        return 0;
    }
    if(rval == 0)
    {
        printf("get fd time out!\n");
        return 0;
    }
    if((gpfd[0].revents & POLLIN) != POLLIN)
    {
        printf("error !\n");
        return 0;
    }
    if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(pstChnPort, &stBufInfo, &stBufHandle))
    {
        printf("Get buffer error!\n");
        return 0;
    }
    switch(stBufInfo.eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            u32RetSize = stBufInfo.stRawData.u32ContentSize;
            //no need to do flushCache here..
            //MI_SYS_FlushInvCache(stBufInfo.stRawData.pVirAddr, u32RetSize);
            memcpy(pData, stBufInfo.stRawData.pVirAddr, u32RetSize);
        }
        break;
        case E_MI_SYS_BUFDATA_FRAME:
        {
            u32RetSize = stBufInfo.stFrameData.u16Height * (stBufInfo.stFrameData.u32Stride[0] +  stBufInfo.stFrameData.u32Stride[1] / 2);
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0]);
            u8CopyData += stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            memcpy(u8CopyData, stBufInfo.stFrameData.pVirAddr[1], stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[1] /2);
        }
        break;
        default:
            ASSERT(0);
    }
    MI_SYS_ChnOutputPortPutBuf(stBufHandle);

    return u32RetSize;
}
static void ST_ConfigData(void)
{
    MI_S32 s32Ret = 0;

    memset(&gstChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    if (g_s32AdWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VENC)
    {
        gstChnPort.eModId = E_MI_MODULE_ID_VENC;

        s32Ret = MI_VENC_GetChnDevid((MI_VENC_CHN)0, (MI_U32 *)&gstChnPort.u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
        }
        gstChnPort.u32ChnId = 0;
        gstChnPort.u32PortId = 0;
    }
    else if (g_s32AdWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VPE)
    {
#ifdef SUPPORT_WARP
        gstChnPort.eModId = E_MI_MODULE_ID_WARP;
        gstChnPort.u32DevId = 0;
        gstChnPort.u32ChnId = 0;
        gstChnPort.u32PortId = 0;
#else
        gstChnPort.eModId = E_MI_MODULE_ID_VPE;
        gstChnPort.u32DevId = 0;
        gstChnPort.u32ChnId = 0;
        gstChnPort.u32PortId = MAIN_VENC_PORT;
#endif
    }
}

void *ST_VencGetEsBufferProc(void *args)
{
    Venc_Args_t *pArgs = (Venc_Args_t *)args;
	//Live555RTSPServer* mLive555RTSPServer;

	MI_SYS_ChnPort_t stVencChnInputPort;
	char szFileName[128];
	//int fd = -1;
	FILE *fd =NULL;
	MI_SYS_BUF_HANDLE hHandle;
	MI_SYS_BufInfo_t stBufInfo;
	MI_S32 s32Ret = MI_SUCCESS;
	MI_S32 len = 0;
	MI_U32 u32DevId = 0;
	MI_VENC_Stream_t stStream;
	MI_VENC_Pack_t stPack0;
	MI_VENC_ChnStat_t stStat;

	stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
	printf("Thread ST_VencGetEsBufferProc create successful\n");
	s32Ret = MI_VENC_GetChnDevid(pArgs->stVencAttr[0].vencChn, &u32DevId);
	if (MI_SUCCESS != s32Ret)
	{
		printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n",
			__func__, __LINE__, pArgs->stVencAttr[0].vencChn, s32Ret);
	}
	stVencChnInputPort.u32DevId = u32DevId;
	stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
	stVencChnInputPort.u32PortId = 0;

	memset(szFileName, 0, sizeof(szFileName));
	len = snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%dx%d.",
		stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
		pArgs->stVencAttr[0].u32MainWidth, pArgs->stVencAttr[0].u32MainHeight);
	if (pArgs->stVencAttr[0].eType == E_MI_VENC_MODTYPE_H264E)
	{
		snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h264");
	}
	else if (pArgs->stVencAttr[0].eType == E_MI_VENC_MODTYPE_H265E)
	{
		snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h265");
	}
	else
	{
		snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "mjpeg");
	}

	fd = fopen(szFileName, "wb");
	if (fd <= 0)
	{
		printf("%s %d create %s error\n", __func__, __LINE__, szFileName);
		return NULL;
	}

	printf("%s %d create %s success\n", __func__, __LINE__, szFileName);

	while (1)
	{
		hHandle = MI_HANDLE_NULL;
		memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
		memset(&stStream, 0, sizeof(stStream));
		memset(&stPack0, 0, sizeof(stPack0));
		stStream.pstPack = &stPack0;
		stStream.u32PackCount = 1;
		s32Ret = MI_VENC_Query(pArgs->stVencAttr[0].vencChn, &stStat);
		if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
		{
			usleep(10 * 1000);
			continue;
		}
		s32Ret = MI_VENC_GetStream(pArgs->stVencAttr[0].vencChn, &stStream, 40);

		if (MI_SUCCESS == s32Ret)
		{
			//printf("%s %d, chn:%d write frame len=%d offset = %d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
					  //stStream.pstPack[0].u32Len, stStream.pstPack[0].u32Offset);
			len = fwrite(stStream.pstPack[0].pu8Addr, 1,stStream.pstPack[0].u32Len,fd);
			if (len != stStream.pstPack[0].u32Len)
			{
				printf("fwrite es buffer fail.\n");
			}
			if (MI_SUCCESS != (s32Ret = MI_VENC_ReleaseStream(pArgs->stVencAttr[0].vencChn, &stStream)))
			{
				printf("%s %d, MI_VENC_ReleaseStream error, %X\n", __func__, __LINE__, s32Ret);
			}
		}
		else
		{

			if ((MI_ERR_VENC_NOBUF != s32Ret) && (MI_ERR_SYS_NOBUF != s32Ret))
			{
				printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X %x\n", __func__, __LINE__, s32Ret, MI_ERR_VENC_BUF_EMPTY);
			}
			usleep(10 * 1000);
		}
	}

    fclose(fd);
}
int venc_jpegSnap()
{
	MI_SYS_ChnPort_t stVencChnInputPort;
	char szFileName[128];
	//int fd = -1;
	FILE *fd =NULL;
	MI_SYS_BUF_HANDLE hHandle;
	MI_SYS_BufInfo_t stBufInfo;
	MI_S32 s32Ret = MI_SUCCESS;
	MI_S32 len = 0;
	MI_U32 u32DevId = 0;
	MI_VENC_Stream_t stStream;
	MI_VENC_Pack_t stPack0;
	MI_VENC_ChnStat_t stStat;
	int vencChn=2;   //venc 2
	static int snap_number=0;
	stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
	s32Ret = MI_VENC_GetChnDevid(vencChn, &u32DevId);
	if (MI_SUCCESS != s32Ret)
	{
		printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n",
			__func__, __LINE__, vencChn, s32Ret);
	}
	stVencChnInputPort.u32DevId = u32DevId;
	stVencChnInputPort.u32ChnId = vencChn;
	stVencChnInputPort.u32PortId = 0;
	memset(szFileName, 0, sizeof(szFileName));
	snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%d.jpeg",
		stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,snap_number);
	fd = fopen(szFileName, "wb");
	if (fd <= 0)
	{
		printf("%s %d create %s error\n", __func__, __LINE__, szFileName);
		return NULL;
	}

	printf("%s %d create %s success\n", __func__, __LINE__, szFileName);

	hHandle = MI_HANDLE_NULL;
	memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
	memset(&stStream, 0, sizeof(stStream));
	memset(&stPack0, 0, sizeof(stPack0));
	stStream.pstPack = &stPack0;
	stStream.u32PackCount = 1;
	s32Ret = MI_VENC_Query(vencChn, &stStat);
	if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
	{
		printf("venc_jpegSnap MI_VENC_Query error\n");
		goto error;
	}
	s32Ret = MI_VENC_GetStream(vencChn, &stStream, 40);

	if (MI_SUCCESS == s32Ret)
	{

		len = fwrite(stStream.pstPack[0].pu8Addr, 1,stStream.pstPack[0].u32Len,fd);
		if (len != stStream.pstPack[0].u32Len)
		{
			printf("fwrite es buffer fail.\n");
		}
		if (MI_SUCCESS != (s32Ret = MI_VENC_ReleaseStream(vencChn, &stStream)))
		{
			printf("%s %d, MI_VENC_ReleaseStream error, %X\n", __func__, __LINE__, s32Ret);
		}
	}
	else
	{

		if ((MI_ERR_VENC_NOBUF != s32Ret) && (MI_ERR_SYS_NOBUF != s32Ret))
		{
			printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X %x\n", __func__, __LINE__, s32Ret, MI_ERR_VENC_BUF_EMPTY);
		}
	}
error:
	snap_number++;
	fclose(fd);
}

void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS, s32VoChannel = 0;
    MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;
    FILE *fp = NULL;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = s32VoChannel;
    stChnPort.u32PortId = 2;
    printf("..st_GetOutputDataThread.s32VoChannel(%d)...\n", s32VoChannel);

    s32ReadCnt = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3); //Default queue frame depth--->20
    fp = fopen("vpe_2.yuv","wb");
    while (!_bThreadRunning)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == stBufInfo.stFrameData.ePixelFormat)
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u16Width;

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                        fwrite(stBufInfo.stFrameData.pVirAddr[1], size/2, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
            else
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            if(stBufInfo.bEndOfStream)
                break;
        }
        usleep(10*1000);
    }
    printf("\n\n");
    usleep(3000000);

    return NULL;
}

int ST_SplitWindow()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32LastWndNum = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 1;
    MI_U16 u16DispWidth = 0, u16DispHeight = 0;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    ST_Sys_BindInfo_t stBindInfo;
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stVpeRect;

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    /*
    VDEC->DIVP->VDISP->DISP

    (1) unbind VDEC DIVP
    (2) VDISP disable input port
    (3) divp set output port attr
    (4) vdisp set input port attr
    (5) bind vdec divp
    (6) enable vdisp input port

    VDEC->DIVP->VPE->VDISP->DISP
    (1) unbind vdec divp
    (2) unbind divp vpe
    (3) vdisp disable input port
    (4) divp set chn attr
    (5) vpe set port mode
    (6) vdisp set input port attr
    (7) bind vdec divp
    (8) bind divp vpe
    (9) enable vdisp input port
    */

    u32CurWndNum = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    u32LastWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    if (u32CurWndNum == u32LastWndNum)
    {
        printf("same wnd num, skip\n");
        return 0;
    }
    else
    {
        printf("split window from %d to %d\n", u32LastWndNum, u32CurWndNum);
    }

    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[u32CaseIndex].eDispoutTiming,
                &s32HdmiTiming, &s32DispTiming, &u16DispWidth, &u16DispHeight));

    printf("%s %d, u16DispWidth:%d,u16DispHeight:%d\n", __func__, __LINE__, u16DispWidth,
        u16DispHeight);

    // 1, unbind VDEC to DIVP
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    // 2, unbind DIVP to VPE
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    // 3, disable vdisp input port
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_VDISP_PORT vdispPort = 0;
    MI_VDISP_InputPortAttr_t stInputPortAttr;

#if 1
    // stop divp and vpe
    for (i = 0; i < u32LastWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StopChn(i));
    }

    for (i = 0; i < u32LastWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_DisablePort(vpeChn, 0));
    }
#endif

    for (i = 0; i < u32LastWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
    }

    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }

    // 4, divp set output port chn attr
#if 0
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    for (i = 0; i < u32CurWndNum; i++)
    {
        divpChn = i;
        STCHECKRESULT(MI_DIVP_GetOutputPortAttr(divpChn, &stOutputPortAttr));
        printf("change divp from %dx%d ", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        stOutputPortAttr.u32Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stOutputPortAttr.u32Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        printf("to %dx%d\n", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }
#endif

    // 5, set vpe port mode
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }

    // 6, set vdisp input port attribute
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16DispHeight / u32Square, 2);

        printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
             stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }

    // start divp and vpe
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StartChn(i));
    }

    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_EnablePort(vpeChn, 0));
    }

    // 9, enable vdisp input port
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
    }

    // 7, bind VDEC to DIVP
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    // 8, bind DIVP to VPE
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    pstCaseDesc[u32CaseIndex].u32ShowWndNum =
        pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;

    return 0;
}

MI_S32 ST_SubExit()
{
    MI_U32 u32WndNum = 0, u32ShowWndNum;
    MI_S32 i = 0;
    ST_Sys_BindInfo_t stBindInfo;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    u32WndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;
    u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].u32ShowWndNum;

	ST_DBG("u32ShowWndNum:%d\n", u32ShowWndNum);
    /************************************************
    step1:  unbind VIF to VPE
    *************************************************/
    for (i = 0; i < u32WndNum; i++)
    {
        STCHECKRESULT(ST_Vif_StopPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort));
    }
    for (i = 0; i < u32ShowWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = stVifChnCfg[i].u8ViDev;
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = stVifChnCfg[i].u8ViPort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step4:  unbind VPE to VDISP
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
    	memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;

	    stBindInfo.u32SrcFrmrate = 30;
	    stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step5:  destroy vif  vpe vdisp disp
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        STCHECKRESULT(ST_Vpe_StopPort(i, 0));
        STCHECKRESULT(ST_Vpe_StopChannel(i));
        STCHECKRESULT(ST_Vpe_DestroyChannel(i));
    }

    STCHECKRESULT(ST_Vif_DisableDev(0));//0 1 2 3?

    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0 , u32ShowWndNum)); //disp input port 0
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    STCHECKRESULT(ST_Fb_DeInit());
    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

int ST_ChangeDisplayTiming()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    ST_DispoutTiming_e eLastDispoutTiming = E_ST_TIMING_MAX;
    ST_DispoutTiming_e eCurDispoutTiming = E_ST_TIMING_MAX;
    MI_HDMI_TimingType_e s32LastHdmiTiming, s32CurHdmiTiming;
    MI_DISP_OutputTiming_e s32LastDispTiming, s32CurDispTiming;
    MI_U16 u16LastDispWidth = 0, u16LastDispHeight = 0;
    MI_U16 u16CurDispWidth = 0, u16CurDispHeight = 0;
    MI_S32 u32CurWndNum = 0;
    MI_U32 u32TotalWnd = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 0;
    ST_Sys_BindInfo_t stBindInfo;

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    eCurDispoutTiming = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].eDispoutTiming;
    eLastDispoutTiming = pstCaseDesc[u32CaseIndex].eDispoutTiming;

    if (eCurDispoutTiming == eLastDispoutTiming)
    {
        printf("the same timing, skip\n");
        return 0;
    }

    u32CurWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    STCHECKRESULT(ST_GetTimingInfo(eCurDispoutTiming,
                (MI_S32*)&s32CurHdmiTiming, (MI_S32*)&s32CurDispTiming, (MI_U16*)&u16CurDispWidth, (MI_U16*)&u16CurDispHeight));

    STCHECKRESULT(ST_GetTimingInfo(eLastDispoutTiming,
                (MI_S32*)&s32LastHdmiTiming, (MI_S32*)&s32LastDispTiming, (MI_U16*)&u16LastDispWidth, (MI_U16*)&u16LastDispHeight));

    printf("change from %dx%d to %dx%d\n", u16LastDispWidth, u16LastDispHeight, u16CurDispWidth,
                u16CurDispHeight);

    /*
    (1), stop HDMI
    (2), stop VDISP
    (3), stop DISP
    (4), set vpe port mode
    (5), set vdisp input port chn attr
    (6), start disp
    (7), start vdisp
    (8), star HDMI
    */

    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }

    // stop hdmi
    ExecFunc(MI_HDMI_Stop(E_MI_HDMI_ID_0), MI_SUCCESS);

    // stop vdisp
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_S32 s32FrmRate = 30;
    MI_S32 s32OutputPort = 0;
    MI_VDISP_PORT vdispPort = 0;

    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Vdisp_StopDevice(vdispDev));

    // stop disp
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0, DISP_MAX_CHN));

    // set vpe port mode
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16CurDispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }
    // start vdisp
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
    }
    ST_Rect_t stdispRect, stRect;
    stdispRect.u16PicW = u16CurDispWidth;
    stdispRect.u16PicH = u16CurDispHeight;
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev, s32OutputPort, &stdispRect, s32FrmRate, 1));

    // set vdisp input port chn attr
    MI_VDISP_InputPortAttr_t stInputPortAttr;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16CurDispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16CurDispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16CurDispHeight / u32Square, 2);

        // printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
        //     stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
    }

    STCHECKRESULT(ST_Vdisp_StartDevice(vdispDev));

    // start disp
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32CurDispTiming));
    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    // start HDMI
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32CurHdmiTiming));

    pstCaseDesc[u32CaseIndex].eDispoutTiming = eCurDispoutTiming;

    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;
}

void ST_WaitSubCmd(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    char szCmd[16];
    MI_U32 index = 0;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;

    while (!g_subExit)
    {
        ST_CaseSubUsage();

        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        index = atoi(szCmd);

        if (index <= 0 || index > u32SubCaseSize)
        {
            continue;
        }

        g_u32CurSubCaseIndex = index - 1;

        if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
        {
            ST_ChangeDisplayTiming(); //change timing
        }
        else
        {
            if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
            {
                ST_SubExit();
                return;
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
            {
            }
            else
            {
                if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
                {
                    ST_SplitWindow(); //switch screen
                }
            }
        }
    }
}

MI_S32 ST_VifToDisp(MI_S32 s32CaseIndex)
{
    MI_VIF_DEV VifDevId = 0;
    MI_U16 u16DispLayerW = 1920;
    MI_U16 u16DispLayerH = 1080;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    ST_Rect_t stdispRect = {0, 0, 1920, 1080};
    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_TimingType_e eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
    MI_DISP_OutputTiming_e eDispoutTiming = E_MI_DISP_OUTPUT_1080P60;
    MI_S32 s32CapChnNum = 0, s32DispChnNum = 0, i = 0;
    MI_S32 s32DevNum = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32Square = 0;
    MI_U32 u32SubCaseSize = pstCaseDesc[s32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    ST_Rect_t stDispWndRect[16] = {0};
    MI_S32 s32AdId[4];
    VIF_AD_WORK_MODE_E s32AdWorkMode ;

    s32DispChnNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    s32CapChnNum = s32DispChnNum;
    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[s32CaseIndex].eDispoutTiming, (MI_S32 *)&eHdmiTiming,
        (MI_S32 *)&eDispoutTiming, (MI_U16*)&stdispRect.u16PicW, (MI_U16*)&stdispRect.u16PicH));
    for (i = 0; i < 4; i++)
    {
        s32AdId[i] = 0;
    }
    if (u32WndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32WndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32WndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32WndNum <= 16)
    {
        u32Square = 4;
    }

    if (6 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 3, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 3, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 2, 2},
            {2, 0, 1, 1},
            {2, 1, 1, 1},
            {0, 2, 1, 1},
            {1, 2, 1, 1},
            {2, 2, 1, 1},
        };//3x3 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
    }
    else if (8 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 4, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 4, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 3, 3},
            {3, 0, 1, 1},
            {3, 1, 1, 1},
            {3, 2, 1, 1},
            {0, 3, 1, 1},
            {1, 3, 1, 1},
            {2, 3, 1, 1},
            {3, 3, 1, 1},
        };//4x4 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
    }
    else
    {
        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X    = ALIGN_BACK((stdispRect.u16PicW / u32Square) * (i % u32Square), 2);
            stDispWndRect[i].s32Y    = ALIGN_BACK((stdispRect.u16PicH / u32Square) * (i / u32Square), 2);
            stDispWndRect[i].u16PicW = ALIGN_BACK((stdispRect.u16PicW / u32Square), 2);
            stDispWndRect[i].u16PicH = ALIGN_BACK((stdispRect.u16PicH / u32Square), 2);
        }
    }
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    // ExecFunc(vif_i2c_init(), 0);
    /************************************************
    Step2:  init HDMI
    *************************************************/
    // STCHECKRESULT(ST_Hdmi_Init());

    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, eDispoutTiming)); //Dispout timing

    /************************************************
    Step3:  init VIF
    *************************************************/
    s32DevNum = s32CapChnNum / 4;

    /*
        case 0: AD_A CHN0 1*FHD
        case 1: AD_B CHN0 1*FHD
        case 2: AD_C CHN0 1*FHD
        case 3: AD_D CHN0 1*FHD
        case 4: AD_A-AD_D CHN0/CHN4/CHN8/CHN12
        case 5: AD_A 4*D1 CHN0/CHN4/CHN8/CHN12
    */
    switch (s32CaseIndex)
    {
        case 0:
        case 1:
            s32AdId[0] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 0;
            stVifChnCfg[1].u8ViDev = 0;
            stVifChnCfg[1].u8ViChn = 0;
            s32AdWorkMode = SAMPLE_VI_MODE_MIPI_1_1080P_VENC;
            break;
        default:
            ST_DBG("Unkown test case(%d)!\n", s32CaseIndex);
            return 0;
    }

    g_s32AdWorkMode = s32AdWorkMode;

    MI_SYS_ChnPort_t stChnPort;
    STCHECKRESULT(ST_Vif_CreateDev(0, s32AdWorkMode));

    memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 3840;
    stVifPortInfoInfo.u32RectHeight = 2160;
    stVifPortInfoInfo.u32DestWidth = 3840;
    stVifPortInfoInfo.u32DestHeight = 2160;
    stVifPortInfoInfo.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vif_CreatePort(0, 0, &stVifPortInfoInfo));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3);
    STCHECKRESULT(ST_Vif_StartPort(0, 0));
#ifdef SUPPORT_VIDEO_VPE2
	memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
    stVifPortInfoInfo.u32RectX = 0;
    stVifPortInfoInfo.u32RectY = 0;
    stVifPortInfoInfo.u32RectWidth = 1920;
    stVifPortInfoInfo.u32RectHeight = 1080;
    stVifPortInfoInfo.u32DestWidth = 1920;
    stVifPortInfoInfo.u32DestHeight = 1080;
    stVifPortInfoInfo.ePixFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vif_CreatePort(/*0*/1, 0, &stVifPortInfoInfo));
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VIF;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = /*0*/1;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3);
    STCHECKRESULT(ST_Vif_StartPort(/*0*/1,0));
#endif
    /************************************************
    Step4:  init VPE channel 0
    *************************************************/
    memset(&stVpeChannelInfo, 0, sizeof(ST_VPE_ChannelInfo_t));
    stVpeChannelInfo.u16VpeMaxW = 3840;
    stVpeChannelInfo.u16VpeMaxH = 2160;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 3840;
    stVpeChannelInfo.u16VpeCropH = 2160;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vpe_CreateChannel(0, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(0));
#ifdef SUPPORT_VIDEO_VPE2
	/************************************************
    Step4:  init VPE channel 1
    *************************************************/
    stVpeChannelInfo.u16VpeMaxW = 1920;
    stVpeChannelInfo.u16VpeMaxH = 1080;
    stVpeChannelInfo.u32X = 0;
    stVpeChannelInfo.u32Y = 0;
    stVpeChannelInfo.u16VpeCropW = 1920;
    stVpeChannelInfo.u16VpeCropH = 1080;
    stVpeChannelInfo.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;//E_MI_VPE_RUN_REALTIME_MODE;
    stVpeChannelInfo.eFormat = E_MI_SYS_PIXEL_FRAME_RGB_BAYER_10BPP_RG;
    STCHECKRESULT(ST_Vpe_CreateChannel(1, &stVpeChannelInfo));
    STCHECKRESULT(ST_Vpe_StartChannel(1));
#endif

#ifdef SUPPORT_VIDEO_PREVIEW
	stPortInfo.DepVpeChannel = 0;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stPortInfo.u16OutputWidth = stDispWndRect[0].u16PicW;
    stPortInfo.u16OutputHeight = stDispWndRect[0].u16PicH;
    STCHECKRESULT(ST_Vpe_CreatePort(DISP_PORT, &stPortInfo)); //default support port0 --->>> vdisp
#endif

#ifdef SUPPORT_VIDEO_ENCODE
	stPortInfo.DepVpeChannel = 0;
    stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    if (s32AdWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VPE)
    {
        stPortInfo.u16OutputWidth = 320;
        stPortInfo.u16OutputHeight = 240;
    }
    else
    {
        // stPortInfo.u16OutputWidth = ALIGN_N(stDispWndRect[i].u16PicW, 32);
        // stPortInfo.u16OutputHeight = ALIGN_N(stDispWndRect[i].u16PicH, 8);
        stPortInfo.u16OutputWidth = 1920;
        stPortInfo.u16OutputHeight = 1080;
    }

    STCHECKRESULT(ST_Vpe_CreatePort(MAIN_VENC_PORT, &stPortInfo)); //default support port2 --->>> venc

    MI_S32 s32Ret = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;
    MI_U32 u32DevId = 0;
    MI_VENC_SuperFrameCfg_t stSuperFrameCfg;
    MI_VENC_ParamFrameLost_t stFrameLost;
    MI_VENC_RcPriority_e eRcPriority;
    MI_VENC_RoiBgFrameRate_t stRoiBgFrameRate;
    MI_VENC_RoiCfg_t stRoiCfg;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stSuperFrameCfg, 0, sizeof(MI_VENC_SuperFrameCfg_t));
    memset(&stFrameLost, 0, sizeof(MI_VENC_ParamFrameLost_t));
    memset(&stRoiBgFrameRate, 0, sizeof(MI_VENC_RoiBgFrameRate_t));
    memset(&stRoiCfg, 0, sizeof(MI_VENC_RoiCfg_t));
    memset(&stRect, 0, sizeof(MI_VENC_Rect_t));
    if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
        stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1920;
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1080;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1920;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1080;
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
	#if 0
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate =
                pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.s32FrmRate;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

        stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
        stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
	#else

		stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
		stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum= 30;
        stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen= 1;
		stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = 30;
		stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate=3 * 1024*1024;
		stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp=48;
		stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp=20;
	#endif

    }
	else if(E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
	{
		stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
        stChnAttr.stVeAttr.stAttrH265e.u32BFrameNum = 2; // not support B frame
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1920;
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 1080;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1920;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 1080;
        stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;

		stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
		stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum= 30;
        stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
		stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = 30;
		stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = 2 * 1024 * 1024;
		stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp=48;
		stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp=20;
	}
    else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1920;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 1080;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1920;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 1080;
    }
    s32Ret = MI_VENC_CreateChn(0, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }

    s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }

    ST_DBG("u32DevId:%d\n", u32DevId);
    stVencChnOutputPort.u32DevId = u32DevId;
    stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnOutputPort.u32ChnId = 0;
    stVencChnOutputPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }

    stSuperFrameCfg.eSuperFrmMode = E_MI_VENC_SUPERFRM_REENCODE;
    stSuperFrameCfg.u32SuperIFrmBitsThr = 50 * 1024 * 8; //bit
    stSuperFrameCfg.u32SuperPFrmBitsThr = 30 * 1024 * 8;
    //STCHECKRESULT(MI_VENC_SetSuperFrameCfg(0, &stSuperFrameCfg));

    stFrameLost.bFrmLostOpen = TRUE;
    stFrameLost.eFrmLostMode = E_MI_VENC_FRMLOST_NORMAL;
    stFrameLost.u32EncFrmGaps = 1;
    stFrameLost.u32FrmLostBpsThr = 3*1024*1024;
    //STCHECKRESULT(MI_VENC_SetFrameLostStrategy(0, &stFrameLost));

    eRcPriority = E_MI_VENC_RC_PRIORITY_FRAMEBITS_FIRST;
    //STCHECKRESULT(MI_VENC_SetRcPriority(0, &eRcPriority));

    s32Ret = MI_VENC_StartRecvPic(0);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, 0, s32Ret);
    }

    stRoiCfg.bEnable = TRUE;
    stRoiCfg.stRect.u32Left = 320;
    stRoiCfg.stRect.u32Top = 320;
    stRoiCfg.stRect.u32Height = 560;
    stRoiCfg.stRect.u32Width = 720;
    stRoiCfg.u32Index = 0;
    stRoiCfg.s32Qp = 0;
    stRoiCfg.bAbsQp = FALSE;
    STCHECKRESULT(MI_VENC_SetRoiCfg(0, &stRoiCfg));

    stRoiBgFrameRate.s32DstFrmRate = 1;
    stRoiBgFrameRate.s32SrcFrmRate = 30;
    STCHECKRESULT(MI_VENC_SetRoiBgFrameRate(0, &stRoiBgFrameRate));


#ifdef SUPPORT_VIDEO_ENCODE_H264_H265
	stPortInfo.DepVpeChannel = 0;
	stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    stPortInfo.u16OutputWidth = 1280;
    stPortInfo.u16OutputHeight = 720;
    STCHECKRESULT(ST_Vpe_CreatePort(/*MAIN_VENC_PORT*/1, &stPortInfo)); //default support port2 --->>> venc

    //MI_S32 s32Ret = 0;
    //MI_VENC_ChnAttr_t stChnAttr;
    //MI_SYS_ChnPort_t stVencChnOutputPort;
    //MI_U32 u32DevId = 0;
    s32Ret = 0;
	u32DevId = 0;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

#if 0
	stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
	stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = 1280;
	stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = 720;
	stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = 1280;
	stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = 720;

	stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = 2 * 1024 * 1024;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum =
        pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.s32MainFrmRate;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
#endif

	stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
    stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
    stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1280;
    stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 720;
    stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1280;
    stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 720;
#if 0
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
    stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate =
            pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.s32FrmRate;
    stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

    stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
    stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
#else
	stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum=
            pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.s32FrmRate;
    stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen= 1;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = 30;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate=1024*1024;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp=45;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp=25;
#endif

    s32Ret = MI_VENC_CreateChn(/*0*/1, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, /*0*/1, s32Ret);
    }

    s32Ret = MI_VENC_GetChnDevid(/*0*/1, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/1, s32Ret);
    }

    ST_DBG("u32DevId:%d\n", u32DevId);
    stVencChnOutputPort.u32DevId = u32DevId;
    stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnOutputPort.u32ChnId = /*0*/1;
    stVencChnOutputPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, /*0*/1, s32Ret);
    }
    s32Ret = MI_VENC_StartRecvPic(/*0*/1);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, /*0*/1, s32Ret);
    }
#endif
#ifdef SUPPORT_VIDEO_ENCODE_MJPEG
	stPortInfo.DepVpeChannel = 0;
	stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    stPortInfo.u16OutputWidth = 1280;
    stPortInfo.u16OutputHeight = 720;
    STCHECKRESULT(ST_Vpe_CreatePort(/*MAIN_VENC_PORT*/3, &stPortInfo)); //default support port2 --->>> venc
    s32Ret = 0;
	u32DevId = 0;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

	stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = 1280;
    stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = 720;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = 1280;
    stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = 720;

    s32Ret = MI_VENC_CreateChn(/*0*/2, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, /*0*/2, s32Ret);
    }

    s32Ret = MI_VENC_GetChnDevid(/*0*/2, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/2, s32Ret);
    }

    ST_DBG("u32DevId:%d\n", u32DevId);
    stVencChnOutputPort.u32DevId = u32DevId;
    stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnOutputPort.u32ChnId = /*0*/2;
    stVencChnOutputPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, /*0*/2, s32Ret);
    }
    s32Ret = MI_VENC_StartRecvPic(/*0*/2);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, /*0*/2, s32Ret);
    }
#endif
#ifdef SUPPORT_VIDEO_VPE2
	stPortInfo.DepVpeChannel = 1;
	stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
	stPortInfo.u16OutputWidth = 1920;
	stPortInfo.u16OutputHeight = 1080;
	STCHECKRESULT(ST_Vpe_CreatePort(0, &stPortInfo)); //default support port0

	s32Ret = 0;
	u32DevId = 0;
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
    stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
    stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = 1920;
    stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = 1080;
    stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = 1920;
    stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = 1080;

	stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate=
            pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.s32FrmRate;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = 30;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate=1024*1024;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp=45;
	stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp=25;

    s32Ret = MI_VENC_CreateChn(/*0*/3, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, /*0*/3, s32Ret);
    }

    s32Ret = MI_VENC_GetChnDevid(/*0*/3, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/3, s32Ret);
    }

    ST_DBG("u32DevId:%d\n", u32DevId);
    stVencChnOutputPort.u32DevId = u32DevId;
    stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnOutputPort.u32ChnId = /*0*/3;
    stVencChnOutputPort.u32PortId = 0;
    //This was set to (5, 10) and might be too big for kernel

    s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, /*0*/3, s32Ret);
    }
    s32Ret = MI_VENC_StartRecvPic(/*0*/3);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, /*0*/3, s32Ret);
    }
#endif
#endif

#ifdef SUPPORT_VIDEO_ENCODE
#ifdef SUPPORT_WARP
    /************************************************
    Step6:  init Warp
    *************************************************/
    if (s32AdWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VPE)
    {
        STCHECKRESULT(ST_Warp_Init(E_WARP_320_240_NV12)); //default support port2 --->>> venc
    }
    else
    {
        STCHECKRESULT(ST_Warp_Init(E_WARP_1280_720_NV12)); //default support port2 --->>> venc
    }
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_WARP;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = 0;
    stChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3);
    STCHECKRESULT(ST_Warp_CreateChannel(0)); //default support port2 --->>> venc
#endif
#endif
    /************************************************
    Step7:  init DISP
    *************************************************/
    // STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, eDispoutTiming)); //Dispout timing

#ifdef SUPPORT_VIDEO_PREVIEW
    ST_DispChnInfo_t stDispChnInfo;
    stDispChnInfo.InputPortNum = s32DispChnNum;
    stDispChnInfo.stInputPortAttr[0].u32Port = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16X = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Y = 0;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Width = 1920;
    stDispChnInfo.stInputPortAttr[0].stAttr.stDispWin.u16Height = 1080;
    STCHECKRESULT(ST_Disp_ChnInit(0, &stDispChnInfo));
#endif

    // must init after disp
    // ST_Fb_Init();
    // ST_FB_Show(FALSE);

    /************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#ifdef SUPPORT_VIDEO_VPE2
	/************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
    stBindInfo.stSrcChnPort.u32ChnId = /*0*/1;
    stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 1;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif

#ifdef SUPPORT_VIDEO_ENCODE //Create Video encode Channel
#ifdef SUPPORT_WARP
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_WARP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif
#endif

    /************************************************
    Step8:  Bind VPE->VDISP
    *************************************************/
#ifdef SUPPORT_VIDEO_PREVIEW
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif

#ifdef SUPPORT_VIDEO_ENCODE
    if (s32AdWorkMode == SAMPLE_VI_MODE_MIPI_1_1080P_VENC)
    {
#ifdef SUPPORT_WARP
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_WARP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = 0;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
#else
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = 0;
        stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
#endif
        s32Ret = MI_VENC_GetChnDevid(0, &u32DevId);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, 0, s32Ret);
        }
        stBindInfo.stDstChnPort.u32DevId = u32DevId;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
#ifdef SUPPORT_VIDEO_ENCODE_H264_H265
	stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = /*MAIN_VENC_PORT*/1;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
	s32Ret = MI_VENC_GetChnDevid(/*0*/1, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/1, s32Ret);
    }
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId =/*0*/1;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif
#ifdef SUPPORT_VIDEO_ENCODE_MJPEG
	stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = /*MAIN_VENC_PORT*/2;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
	s32Ret = MI_VENC_GetChnDevid(/*0*/2, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/2, s32Ret);
    }
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId =/*0*/2;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif
#ifdef SUPPORT_VIDEO_VPE2
	stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 1;
    stBindInfo.stSrcChnPort.u32PortId = /*MAIN_VENC_PORT*/0;
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
	s32Ret = MI_VENC_GetChnDevid(/*0*/3, &u32DevId);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n", __func__, __LINE__, /*0*/3, s32Ret);
    }
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId =/*0*/3;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
#endif
#endif

#ifdef SUPPORT_VIDEO_ENCODE
#if 0
    /*for (i = 0; i < 1; i++) //init vif channel
    {
        g_stVencArgs[i].stVencAttr[0].vencChn = 0;
        g_stVencArgs[i].stVencAttr[0].u32MainWidth =1280;
        g_stVencArgs[i].stVencAttr[0].u32MainHeight =720;
        g_stVencArgs[i].stVencAttr[0].eType = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType;
        g_stVencArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(0);
        g_stVencArgs[i].bRunFlag = TRUE;
        ST_DBG("i:%d, eType:%d\n", i, g_stVencArgs[i].stVencAttr[0].eType);
        pthread_create(&g_stVencArgs[i].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[i]);
    }*/
    g_stVencArgs[0].stVencAttr[0].vencChn = 0;
    g_stVencArgs[0].stVencAttr[0].u32MainWidth =1280;
    g_stVencArgs[0].stVencAttr[0].u32MainHeight =720;
    g_stVencArgs[0].stVencAttr[0].eType = pstCaseDesc[s32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType;
    g_stVencArgs[0].stVencAttr[0].vencFd = MI_VENC_GetFd(0);
    g_stVencArgs[0].bRunFlag = TRUE;
    //ST_DBG("i:%d, eType:%d\n", i, g_stVencArgs[i].stVencAttr[0].eType);
    pthread_create(&g_stVencArgs[0].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[0]);
#ifdef SUPPORT_VIDEO_ENCODE_H264_H265
	g_stVencArgs[1].stVencAttr[0].vencChn = 1;
    g_stVencArgs[1].stVencAttr[0].u32MainWidth =1280;
    g_stVencArgs[1].stVencAttr[0].u32MainHeight =720;
    g_stVencArgs[1].stVencAttr[0].eType = E_MI_VENC_MODTYPE_H264E;
    g_stVencArgs[1].stVencAttr[0].vencFd = MI_VENC_GetFd(1);
    g_stVencArgs[1].bRunFlag = TRUE;
    //ST_DBG("i:%d, eType:%d\n", i, g_stVencArgs[i].stVencAttr[0].eType);
    pthread_create(&g_stVencArgs[1].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[1]);
#endif
#else

	mLive555RTSPServer = new Live555RTSPServer();
	if ( mLive555RTSPServer==NULL ) {
		return 0;
	}
	Live555RTSPServerSet(mLive555RTSPServer);
	mLive555RTSPServer->Start();

#endif
#endif

#ifdef SUPPORT_VIDEO_PREVIEW
    STCHECKRESULT(ST_Hdmi_Init());
    STCHECKRESULT(ST_Hdmi_Start(eHdmi, eHdmiTiming)); //Hdmi timing
#endif

    g_u32LastSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    g_u32CurSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[s32CaseIndex].u32ShowWndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    //pthread_create(&pt, NULL, st_GetOutputDataThread, NULL);

    return MI_SUCCESS;
}

void ST_DealCase(int argc, char **argv)
{
    MI_U32 u32Index = 0;
    MI_U32 u32SubIndex = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;

    if (argc != 3)
    {
        return;
    }

    u32Index = atoi(argv[1]);
    u32SubIndex = atoi(argv[2]);

    if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))//case num
    {
        printf("case index range (%d~%d)\n", 1, ARRAY_SIZE(g_stVifCaseDesc));
        return;
    }
    g_u32CaseIndex = u32Index - 1;//real array index

    if (u32SubIndex <= 0 || u32SubIndex > pstCaseDesc[g_u32CaseIndex].u32SubCaseNum)
    {
        printf("sub case index range (%d~%d)\n", 1, pstCaseDesc[g_u32CaseIndex].u32SubCaseNum);
        return;
    }

    g_u32LastSubCaseIndex = pstCaseDesc[g_u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[g_u32CaseIndex].u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;

    printf("case index %d, sub case %d-%d\n", g_u32CaseIndex, g_u32CurSubCaseIndex, g_u32LastSubCaseIndex);

    ST_VifToDisp(g_u32CaseIndex);

    g_u32CurSubCaseIndex = u32SubIndex - 1;//select subIndex

    if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
    {
        ST_ChangeDisplayTiming(); //change timing
    }
    else
    {
        if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
        {
            ST_SubExit();
            return;
        }
        else if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
        {
        }
        else
        {
            if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
            {
                ST_SplitWindow(); //switch screen
            }
        }
    }

    ST_WaitSubCmd();
}

MI_S32 test_main(int argc, char **argv)
{
    MI_S32 a = 0, b = 1;
    MI_S32 sum = 0;

    while (1)
    {
        sum = a + b;
    }

    return 0;
}

int Live555RTSPServerSet(Live555RTSPServer* mLive555RTSPServer)
{
	if ( mLive555RTSPServer==NULL ) {
		return -1;
	}
	unsigned int rtspServerPortNum = 555;  //如果不是554端口比如555端口，rtsp://192.168.8.104:555/video0  如果是554端口，rtsp://192.168.8.104/video0
	int iRet=mLive555RTSPServer->SetRTSPServerPort(rtspServerPortNum);
	//rtspServerPortNum = 1000;
	rtspServerPortNum++;
	while ( iRet<0 ){
		if ( rtspServerPortNum>65535 ) {
			printf("Failed to create RTSP server: %s\n", mLive555RTSPServer->getResultMsg());
			return -2;
		}
		iRet=mLive555RTSPServer->SetRTSPServerPort(rtspServerPortNum);
		rtspServerPortNum++;
	}

	char* urlPrefix = mLive555RTSPServer->rtspURLPrefix();
	printf("%s%s\n", urlPrefix, "video0");
	printf("%s%s\n", urlPrefix, "video1");
	ServerMediaSession* sms=NULL;

	// video0 H264
	mLive555RTSPServer->createServerMediaSession(sms, "video0", "video0", "video0 Stream");
	mLive555RTSPServer->addSubsession(sms, WW_H264VideoFileServerMediaSubsession::createNew(
										*(mLive555RTSPServer->GetUsageEnvironmentObj())
										, "venc0", OpenStream, VideoReadStream
										, CloseStream, 30));

	mLive555RTSPServer->addServerMediaSession(sms);

#if 0
	// video1 H265
	mLive555RTSPServer->createServerMediaSession(sms, "video1", "video1", "video1 Stream");
	mLive555RTSPServer->addSubsession(sms, WW_H265VideoFileServerMediaSubsession::createNew(
										*(mLive555RTSPServer->GetUsageEnvironmentObj())
										, "venc1", OpenStream, VideoReadStream
										, CloseStream, 30));
	mLive555RTSPServer->addServerMediaSession(sms);
#endif
	// video1 H264
	mLive555RTSPServer->createServerMediaSession(sms, "video1", "video1", "video1 Stream");
	mLive555RTSPServer->addSubsession(sms, WW_H264VideoFileServerMediaSubsession::createNew(
										*(mLive555RTSPServer->GetUsageEnvironmentObj())
										, "venc1", OpenStream, VideoReadStream
										, CloseStream, 30));
	mLive555RTSPServer->addServerMediaSession(sms);

	unsigned int httpPort=80;
	iRet = mLive555RTSPServer->setUpTunnelingOverHTTP(httpPort);
	// Dynamic Ports: (49152,65535)
	for ( httpPort = 49152; iRet!=0; httpPort++ )
	{
		if ( httpPort>65535 )
		{
			printf("(RTSP-over-HTTP tunneling is not available.)\n");
			return -6;
		}

		iRet = mLive555RTSPServer->setUpTunnelingOverHTTP(httpPort);
	}

	printf("httpServer Port=%d\n", mLive555RTSPServer->httpServerPortNum());
	return 0;
}

void* OpenStream(char const* szStreamName, void* arg)
{
	StreamBuf* strStreamBuf = new StreamBuf();
	strStreamBuf->iUsedLen = 0;
	strStreamBuf->iSumLen = 0;

	/*strStreamBuf->buf = new unsigned char[100];
	if ( strStreamBuf->buf==NULL ) {
		return NULL;
	}
	memset(strStreamBuf->buf, 0, 100);*/
	strStreamBuf->buf = NULL;
	strStreamBuf->thread_ExitFlag=0;
	if(strcmp("venc0", szStreamName)==0)
	{
		strStreamBuf->VencChn = 0;
		strStreamBuf->VencType = E_MI_VENC_MODTYPE_H264E;
	}
	else if(strcmp("venc1", szStreamName)==0)
	{
		strStreamBuf->VencChn = 1;
		strStreamBuf->VencType = E_MI_VENC_MODTYPE_H265E;
	}
	else
	{
		ST_ERR("what the fuck venc name!not support!\n");
		return NULL;
	}

	ST_DBG("open stream %s success\n", szStreamName);

	MI_VENC_RequestIdr(strStreamBuf->VencChn, TRUE);

	return strStreamBuf;
}

int CloseStream(void* handle, void* arg)
{
	if ( handle==NULL) {
		return -1;
	}
	StreamBuf* strStreamBuf = (StreamBuf*)handle;
	if (strStreamBuf->buf)delete[] strStreamBuf->buf;
	if (strStreamBuf)delete strStreamBuf;
	strStreamBuf->thread_ExitFlag=1;
	printf("CloseStream successful\n\n\n\n\n\n");
	return 0;
}

int VideoReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg)
{
	MI_SYS_ChnPort_t stVencChnInputPort;
	MI_SYS_BUF_HANDLE hHandle;
	MI_SYS_BufInfo_t stBufInfo;
	MI_S32 s32Ret = MI_SUCCESS;
	MI_S32 len = 0;
	MI_U32 u32DevId = 0;
	MI_VENC_Stream_t stStream;
	MI_VENC_Pack_t stPack0;
	MI_VENC_ChnStat_t stStat;
	int  VencChn ;
	int writeLen = 0;

	if ( handle==NULL) {
		return -1;  //rtsp 断开。
	}
	StreamBuf* strStreamBuf = (StreamBuf*)handle;
#if 0
	StreamBuf* strStreamBuf = (StreamBuf*)handle;
	char* OneVideoData=NULL;
	int OneVideoDataLen=0;
	char NALUHead=0;
	int iRet=WW_H265VideoFileServerMediaSubsession::GetVideoFrame((char*)strStreamBuf->buf+strStreamBuf->iUsedLen, strStreamBuf->iSumLen-strStreamBuf->iUsedLen
			, &OneVideoData, &OneVideoDataLen, &NALUHead);
	//printf("OneVideoDataLen=%d\n", OneVideoDataLen);
	if (iRet==0) {
		memcpy(ucpBuf, OneVideoData, OneVideoDataLen);
		strStreamBuf->iUsedLen += OneVideoDataLen;
		return OneVideoDataLen;
	} else {
		return -1;
	}
#endif

	VencChn = strStreamBuf->VencChn;

	stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
	s32Ret = MI_VENC_GetChnDevid(VencChn, &u32DevId);
	if (MI_SUCCESS != s32Ret)
	{
		printf("%s %d, MI_VENC_GetChnDevid %d error, %X\n",__func__, __LINE__, VencChn, s32Ret);
	}
	stVencChnInputPort.u32DevId = u32DevId;
	stVencChnInputPort.u32ChnId = VencChn;
	stVencChnInputPort.u32PortId = 0;
	hHandle = MI_HANDLE_NULL;
	memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
	memset(&stStream, 0, sizeof(stStream));
	memset(&stPack0, 0, sizeof(stPack0));
	stStream.pstPack = &stPack0;
	stStream.u32PackCount = 1;
	s32Ret = MI_VENC_Query(VencChn, &stStat);

	if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
	{
		return 0;
	}
	s32Ret = MI_VENC_GetStream(VencChn, &stStream, 40);
	if (MI_SUCCESS == s32Ret)
	{
		//len = fwrite(stStream.pstPack[0].pu8Addr, 1,stStream.pstPack[0].u32Len,fd);
		len = stStream.pstPack[0].u32Len;
		memcpy(ucpBuf,stStream.pstPack[0].pu8Addr,len);

#if 0
		if (g_fd != NULL)
			// writeLen = write(g_fd, stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
			writeLen = fwrite(stStream.pstPack[0].pu8Addr, 1, stStream.pstPack[0].u32Len, g_fd);

		if (writeLen != stStream.pstPack[0].u32Len)
			printf("fwrite es buffer fail.\n");
#endif

		MI_VENC_ReleaseStream(VencChn, &stStream);
		return len;
	}
	return 0;

}

int AACReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg)
{
	if ( handle==NULL) {
		return -1;
	}

	StreamBuf* strStreamBuf = (StreamBuf*)handle;
	char* OneAacData=NULL;
	int OneAacDataLen=0;
	int iRet=WW_ADTSAudioFileServerMediaSubsession::GetADTSFrame((char*)strStreamBuf->buf+strStreamBuf->iUsedLen, strStreamBuf->iSumLen-strStreamBuf->iUsedLen
			, &OneAacData, &OneAacDataLen);
	//printf("OneAacDataLen=%d\n", OneAacDataLen);
	if (iRet==0) {
		memcpy(ucpBuf, OneAacData, OneAacDataLen);
		strStreamBuf->iUsedLen += OneAacDataLen;
		return OneAacDataLen;
	} else {
		return -1;
	}
}

int WAVReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg)
{
	if ( handle==NULL) {
		return -1;
	}
	if ( BufLen<=0 ) {
		return -2;
	}
	StreamBuf* strStreamBuf = (StreamBuf*)handle;

	if ( strStreamBuf->iUsedLen>=strStreamBuf->iSumLen ) {
		return -3;
	} else if ( (strStreamBuf->iUsedLen+BufLen)>strStreamBuf->iSumLen ) {
		int iWriteLen = strStreamBuf->iSumLen-strStreamBuf->iUsedLen;
		memcpy(ucpBuf, strStreamBuf->buf+strStreamBuf->iUsedLen, iWriteLen);
		strStreamBuf->iUsedLen += iWriteLen;
		return iWriteLen;
	} else {
		memcpy(ucpBuf, strStreamBuf->buf+strStreamBuf->iUsedLen, BufLen);
		strStreamBuf->iUsedLen += BufLen;
		return BufLen;
	}
}







MI_S32 main(int argc, char **argv)
{
    char szCmd[16];
    MI_U32 u32Index = 0;
	char ch;
	//Live555RTSPServer* mLive555RTSPServer;
    //return test_main(argc, argv);

    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    signal(SIGCHLD, SIG_IGN);

    ST_DealCase(argc, argv);

    while (!g_bExit)
    {
#if 0
        ST_VifUsage();//case usage
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        u32Index = atoi(szCmd);

        if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))
        {
            continue;
        }
        g_u32CaseIndex = u32Index - 1;
        if (0 == (strncmp(g_stVifCaseDesc[g_u32CaseIndex].stDesc.szDesc, "exit", 4)))
        {
            return MI_SUCCESS;
        }
#endif
		g_u32CaseIndex = 0;
        ST_VifToDisp(g_u32CaseIndex);
        //ST_WaitSubCmd();
        while(1)
        {
			ch = getchar();
			if('q' == ch || 'Q' == ch)
			{
				g_bExit = TRUE;
				ST_SubExit();
				break;

			}
			else if('k' == ch) //get mjpeg stream save as .jpeg
			{
				printf("******************snap*************\n");
				venc_jpegSnap();
			}
		}

    }
#if 1
	if (mLive555RTSPServer) {
		mLive555RTSPServer->Join();
		delete mLive555RTSPServer;
	}
	mLive555RTSPServer = NULL;
#endif
    return 0;
}
