#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "st_common.h"
#include "st_vif.h"
#include "st_vpe.h"
#include "st_venc.h"

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "Live555RTSPServer.hh"

//#include "mi_rgn.h"
#include "mi_sensor.h"
#include "mi_sensor_datatype.h"
#include "mi_isp.h"
#include "mi_iqserver.h"
#include "mi_eptz.h"
#include "mi_ldc.h"
#include "mi_eptz.h"

#define ST_MAX_STREAM_NUM (4)
#define ST_LDC_MAX_VIEWNUM (4)
#define RTSP_LISTEN_PORT        554
static Live555RTSPServer *g_pRTSPServer = NULL;
#define STREAM_NAME "video0"
typedef struct ST_VpePortAttr_s
{
    MI_BOOL bUsed;
    MI_BOOL bMirror;
    MI_BOOL bFlip;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_SYS_WindowRect_t  stPortCrop;
    MI_SYS_WindowSize_t  stPortSize;
    MI_SYS_WindowSize_t  stOrigPortSize;
    MI_SYS_WindowRect_t  stOrigPortCrop;
}ST_VpePortAttr_t;

typedef struct ST_VpeChannelAttr_s
{
    ST_VpePortAttr_t        stVpePortAttr[ST_MAX_STREAM_NUM];
    MI_VPE_HDRType_e        eHdrType;
    MI_VPE_3DNR_Level_e     e3DNRLevel;
    MI_SYS_Rotate_e         eVpeRotate;
    MI_BOOL                 bChnMirror;
    MI_BOOL                 bChnFlip;
    MI_SYS_WindowRect_t     stVpeChnCrop;
    MI_SYS_WindowRect_t     stOrgVpeChnCrop;
    MI_BOOL                 bEnLdc;
    MI_U32                  u32ChnPortMode;

    char LdcCfgbin_Path[128];
    mi_eptz_config_param tconfig_para;
    MI_U32 u32ViewNum;
    LDC_BIN_HANDLE          ldcBinBuffer[ST_LDC_MAX_VIEWNUM];
    MI_U32                  u32LdcBinSize[ST_LDC_MAX_VIEWNUM];
    MI_S32  s32Rot[ST_LDC_MAX_VIEWNUM];

}ST_VpeChannelAttr_t;

static ST_VpeChannelAttr_t gstVpeChnattr;

void ST_Flush(void)
{
    char c;

    while((c = getchar()) != '\n' && c != EOF);
}

void *ST_OpenStream(char const *szStreamName, void *arg)
{
    MI_U32 i = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    for(i = 0; i < ST_MAX_STREAM_NUM; i ++)
    {
        if(!strncmp(szStreamName, STREAM_NAME,
                    strlen(szStreamName)))
        {
            break;
        }
    }

    if(i >= ST_MAX_STREAM_NUM)
    {
        ST_ERR("not found this stream, \"%s\"", szStreamName);
        return NULL;
    }

    s32Ret = MI_VENC_RequestIdr(0, TRUE);

    ST_DBG("open stream \"%s\" success, chn:%d\n", szStreamName, 0);

    if(MI_SUCCESS != s32Ret)
    {
        ST_WARN("request IDR fail, error:%x\n", s32Ret);
    }

    return (void *)0xffff;
}

MI_U32 u32GetCnt=0, u32ReleaseCnt=0;
int ST_VideoReadStream(void *handle, unsigned char *ucpBuf, int BufLen, struct timeval *p_Timestamp, void *arg)
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_U32 u32DevId = 0;
    MI_VENC_Stream_t stStream;
    MI_VENC_Pack_t stPack;
    MI_VENC_ChnStat_t stStat;
    MI_VENC_CHN vencChn = 0 ;

    s32Ret = MI_VENC_GetChnDevid(vencChn, &u32DevId);

    if(MI_SUCCESS != s32Ret)
    {
        ST_INFO("MI_VENC_GetChnDevid %d error, %X\n", vencChn, s32Ret);
    }

    memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
    memset(&stStream, 0, sizeof(stStream));
    memset(&stPack, 0, sizeof(stPack));
    stStream.pstPack = &stPack;
    stStream.u32PackCount = 1;
    s32Ret = MI_VENC_Query(vencChn, &stStat);

    if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
    {
        return 0;
    }

    s32Ret = MI_VENC_GetStream(vencChn, &stStream, 40);

    if(MI_SUCCESS == s32Ret)
    {
        u32GetCnt++;
        len = stStream.pstPack[0].u32Len;
        memcpy(ucpBuf, stStream.pstPack[0].pu8Addr, MIN(len, BufLen));

        s32Ret = MI_VENC_ReleaseStream(vencChn, &stStream);
        if(s32Ret != MI_SUCCESS)
        {
            ST_WARN("RELEASE venc buffer fail\n");
        }
        u32ReleaseCnt ++;
        return len;
    }

    return 0;
}

int ST_CloseStream(void *handle, void *arg)
{
    if(handle == NULL)
    {
        return -1;
    }

    ST_DBG("close \"%s\" success\n", STREAM_NAME);
    return 0;
}

MI_S32 ST_RtspServerStart(void)
{
    unsigned int rtspServerPortNum = RTSP_LISTEN_PORT;
    int iRet = 0;
    char *urlPrefix = NULL;
    int arraySize = 1;
    int i = 0;
    ServerMediaSession *mediaSession = NULL;
    ServerMediaSubsession *subSession = NULL;
    Live555RTSPServer *pRTSPServer = NULL;
    MI_VENC_ModType_e eType = E_MI_VENC_MODTYPE_H265E;

    pRTSPServer = new Live555RTSPServer();

    if(pRTSPServer == NULL)
    {
        ST_ERR("malloc error\n");
        return -1;
    }

    iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);

    while(iRet < 0)
    {
        rtspServerPortNum++;

        if(rtspServerPortNum > 65535)
        {
            ST_INFO("Failed to create RTSP server: %s\n", pRTSPServer->getResultMsg());
            delete pRTSPServer;
            pRTSPServer = NULL;
            return -2;
        }

        iRet = pRTSPServer->SetRTSPServerPort(rtspServerPortNum);
    }

    urlPrefix = pRTSPServer->rtspURLPrefix();

    for(i = 0; i < arraySize; i ++)
    {
        printf("=================URL===================\n");
        printf("%s%s\n", urlPrefix, STREAM_NAME);
        printf("=================URL===================\n");

        pRTSPServer->createServerMediaSession(mediaSession,
                                              STREAM_NAME,
                                              NULL, NULL);

        if(eType == E_MI_VENC_MODTYPE_H264E)
        {
            subSession = WW_H264VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             STREAM_NAME,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }
        else if(eType == E_MI_VENC_MODTYPE_H265E)
        {
            subSession = WW_H265VideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             STREAM_NAME,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }
        else if(eType == E_MI_VENC_MODTYPE_JPEGE)
        {
            subSession = WW_JPEGVideoFileServerMediaSubsession::createNew(
                             *(pRTSPServer->GetUsageEnvironmentObj()),
                             STREAM_NAME,
                             ST_OpenStream,
                             ST_VideoReadStream,
                             ST_CloseStream, 30);
        }

        pRTSPServer->addSubsession(mediaSession, subSession);
        pRTSPServer->addServerMediaSession(mediaSession);
    }

    pRTSPServer->Start();

    g_pRTSPServer = pRTSPServer;

    return 0;
}

MI_S32 ST_RtspServerStop(void)
{
    if(g_pRTSPServer)
    {
        g_pRTSPServer->Join();
        delete g_pRTSPServer;
        g_pRTSPServer = NULL;
    }

    return 0;
}

MI_S32 ST_ReadLdcTableBin(const char *pConfigPath, LDC_BIN_HANDLE *tldc_bin, MI_U32 *pu32BinSize)
{
    struct stat statbuff;
    MI_U8 *pBufData = NULL;
    MI_S32 s32Fd = 0;
    MI_U32 u32Size = 0;

    if (pConfigPath == NULL)
    {
        ST_ERR("File path null!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    printf("Read file %s\n", pConfigPath);
    memset(&statbuff, 0, sizeof(struct stat));
    if(stat(pConfigPath, &statbuff) < 0)
    {
        ST_ERR("Bb table file not exit!\n");
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    else
    {
        if (statbuff.st_size == 0)
        {
            ST_ERR("File size is zero!\n");
            return MI_ERR_LDC_ILLEGAL_PARAM;
        }
        u32Size = statbuff.st_size;
    }
    s32Fd = open(pConfigPath, O_RDONLY);
    if (s32Fd < 0)
    {
        ST_ERR("Open file[%d] error!\n", s32Fd);
        return MI_ERR_LDC_ILLEGAL_PARAM;
    }
    pBufData = (MI_U8 *)malloc(u32Size);
    if (!pBufData)
    {
        ST_ERR("Malloc error!\n");
        close(s32Fd);

        return MI_ERR_LDC_ILLEGAL_PARAM;
    }

    memset(pBufData, 0, u32Size);
    read(s32Fd, pBufData, u32Size);
    close(s32Fd);

    *tldc_bin = pBufData;
    *pu32BinSize = u32Size;

    printf("%d: read buffer %p \n",__LINE__, pBufData);
    printf("%d: &bin address %p, *binbuffer %p \n",__LINE__, tldc_bin, *tldc_bin);

    //free(pBufData);

    return MI_SUCCESS;
}

MI_S32 ST_GetLdcCfgViewNum(mi_LDC_MODE eLdcMode)
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

MI_S32 ST_Parse_LibarayCfgFilePath(char *pLdcLibCfgPath, mi_eptz_config_param *ptconfig_para)
{
    mi_eptz_err err_state = MI_EPTZ_ERR_NONE;

    printf("cfg file path %s\n", pLdcLibCfgPath);
    //check cfg file, in out path with bin position

    err_state = mi_eptz_config_parse(pLdcLibCfgPath, ptconfig_para);
    if (err_state != MI_EPTZ_ERR_NONE)
    {
        printf("confile file read error: %d\n", err_state);
        return err_state;
    }

    printf("ldc mode %d \n", ptconfig_para->ldc_mode);
    return 0;
}
    
MI_S32 ST_Libaray_CreatBin(MI_S32 s32ViewId, mi_eptz_config_param *ptconfig_para, LDC_BIN_HANDLE *ptldc_bin, MI_U32 *pu32LdcBinSize, MI_S32 s32Rot)
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

    //Free bin buffer
    /*
    err_state = mi_eptz_buffer_free((LDC_BIN_HANDLE)tldc_bin);
    if (err_state != MI_EPTZ_ERR_NONE)
    {
        printf("[MI EPTZ ERR] =  %d !! \n", err_state);
    }*/
    //release working buffer
    free(pWorkingBuffer);

    return 0;
}
    
MI_S32 ST_GetLdcBinBuffer(MI_S32 s32Cfg_Param, char *pCfgFilePath, mi_eptz_config_param *ptconfig_para, MI_U32 *pu32ViewNum, LDC_BIN_HANDLE *ptldc_bin, MI_U32 *pu32LdcBinSize, MI_S32 *ps32Rot)
{
    MI_U8 i =0;
    MI_U32 u32ViewNum = 0;

    if(s32Cfg_Param == 0)
    {
        ST_Parse_LibarayCfgFilePath(pCfgFilePath, ptconfig_para);
        u32ViewNum = ST_GetLdcCfgViewNum(ptconfig_para->ldc_mode);
        for(i=0; i<u32ViewNum; i++)
        {
            ST_Libaray_CreatBin(i, ptconfig_para, &ptldc_bin[i], &pu32LdcBinSize[i], ps32Rot[i]);
        }
        *pu32ViewNum = u32ViewNum;
    }
    else
    {
        ST_ReadLdcTableBin(pCfgFilePath, ptldc_bin, pu32LdcBinSize);
        *pu32ViewNum = 1;
    }

    return 0;
}


void ST_SetArgs()
{
    MI_S32 s32bEnLdc = TRUE;
    MI_U8 i=0;

    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;

    if(s32bEnLdc == TRUE)
    {
        printf("set Ldc libaray cfg path:  ");
        scanf("%s", pstVpeChnattr->LdcCfgbin_Path);
        ST_Flush();
    }

    pstVpeChnattr->e3DNRLevel = E_MI_VPE_3DNR_LEVEL2;
    pstVpeChnattr->eHdrType = E_MI_VPE_HDR_TYPE_OFF;
    pstVpeChnattr->eVpeRotate = E_MI_SYS_ROTATE_NONE;
    pstVpeChnattr->bChnMirror = FALSE;
    pstVpeChnattr->bChnFlip = FALSE;

    pstVpeChnattr->stOrgVpeChnCrop.u16X = 0;
    pstVpeChnattr->stOrgVpeChnCrop.u16Y = 0;
    pstVpeChnattr->stOrgVpeChnCrop.u16Width = 0;
    pstVpeChnattr->stOrgVpeChnCrop.u16Height = 0;
    pstVpeChnattr->bEnLdc = s32bEnLdc;
    pstVpeChnattr->u32ChnPortMode = 0;
    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        pstVpeChnattr->stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        pstVpeChnattr->stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        pstVpeChnattr->stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
        pstVpeChnattr->stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
    }
    else
    {
        pstVpeChnattr->stVpeChnCrop.u16X = pstVpeChnattr->stOrgVpeChnCrop.u16X;
        pstVpeChnattr->stVpeChnCrop.u16Y = pstVpeChnattr->stOrgVpeChnCrop.u16Y;
        pstVpeChnattr->stVpeChnCrop.u16Width = pstVpeChnattr->stOrgVpeChnCrop.u16Width;
        pstVpeChnattr->stVpeChnCrop.u16Height = pstVpeChnattr->stOrgVpeChnCrop.u16Height;
    }

    if(pstVpeChnattr->bEnLdc == TRUE)
    {
        MI_U32 u32Cfg_Param = 0;
        pstVpeChnattr->s32Rot[0] = 0;
        pstVpeChnattr->s32Rot[1] = 90;
        pstVpeChnattr->s32Rot[2] = 180;
        pstVpeChnattr->s32Rot[3] = 270;
        ST_GetLdcBinBuffer(u32Cfg_Param, pstVpeChnattr->LdcCfgbin_Path, &pstVpeChnattr->tconfig_para, &pstVpeChnattr->u32ViewNum, pstVpeChnattr->ldcBinBuffer, pstVpeChnattr->u32LdcBinSize, pstVpeChnattr->s32Rot);
    }

    MI_S32 s32PortW=pstVpeChnattr->tconfig_para.in_width, s32PortH=pstVpeChnattr->tconfig_para.in_height;


    ST_VpePortAttr_t  *pstVpePortAttr = &pstVpeChnattr->stVpePortAttr[i];
/*
    printf("port %d port width:", i);
    scanf("%d", &s32PortW);
    ST_Flush();

    printf("port %d port height:", i);
    scanf("%d", &s32PortH);
    ST_Flush();
*/
    pstVpePortAttr->bMirror = FALSE;
    pstVpePortAttr->bFlip = FALSE;
    pstVpePortAttr->stOrigPortCrop.u16X = 0;
    pstVpePortAttr->stOrigPortCrop.u16Y = 0;
    pstVpePortAttr->stOrigPortCrop.u16Width = 0;
    pstVpePortAttr->stOrigPortCrop.u16Height = 0;

    pstVpePortAttr->stOrigPortSize.u16Width = s32PortW;
    pstVpePortAttr->stOrigPortSize.u16Height = s32PortH;

    if(pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_90
        || pstVpeChnattr->eVpeRotate == E_MI_SYS_ROTATE_270)
    {
        pstVpePortAttr->stPortSize.u16Width = pstVpePortAttr->stOrigPortSize.u16Height;
        pstVpePortAttr->stPortSize.u16Height = pstVpePortAttr->stOrigPortSize.u16Width;

        pstVpePortAttr->stPortCrop.u16X = pstVpePortAttr->stOrigPortCrop.u16Y;
        pstVpePortAttr->stPortCrop.u16Y = pstVpePortAttr->stOrigPortCrop.u16X;
        pstVpePortAttr->stPortCrop.u16Width = pstVpePortAttr->stOrigPortCrop.u16Height;
        pstVpePortAttr->stPortCrop.u16Height = pstVpePortAttr->stOrigPortCrop.u16Width;
    }
    else
    {
        pstVpePortAttr->stPortSize.u16Width = pstVpePortAttr->stOrigPortSize.u16Width;
        pstVpePortAttr->stPortSize.u16Height = pstVpePortAttr->stOrigPortSize.u16Height;

        pstVpePortAttr->stPortCrop.u16X = pstVpePortAttr->stOrigPortCrop.u16X;
        pstVpePortAttr->stPortCrop.u16Y = pstVpePortAttr->stOrigPortCrop.u16Y;
        pstVpePortAttr->stPortCrop.u16Width = pstVpePortAttr->stOrigPortCrop.u16Width;
        pstVpePortAttr->stPortCrop.u16Height = pstVpePortAttr->stOrigPortCrop.u16Height;
    }
    pstVpePortAttr->ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pstVpePortAttr->bUsed = TRUE;

    pstVpeChnattr->u32ChnPortMode = 0;
    printf("chnport mode %d \n", pstVpeChnattr->u32ChnPortMode);

}


MI_S32 ST_GetLdcOutputData()
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32  s32Portid = 0;
    MI_S32  s32DumpBuffNum =0;
    MI_U32  u32SkipBuffNum = 5;
    FILE *fp = NULL;
    char FilePath[256]={0};
    time_t stTime = 0;
    char sFilePath[128];
    memset(&stTime, 0, sizeof(stTime));

    printf("Dump Buffer Num:");
    scanf("%d", &s32DumpBuffNum);
    ST_Flush();

    printf("write file path:\n");
    scanf("%s", sFilePath);
    ST_Flush();

    MI_LDC_DEV LdcDevId = 0;
    MI_LDC_CHN LdcChn = 0;
    MI_LDC_OutputPortAttr_t stLdcOutPutAttr;
    memset(&stLdcOutPutAttr, 0x0, sizeof(MI_LDC_OutputPortAttr_t));

    MI_LDC_GetOutputPortAttr(LdcDevId, LdcChn, &stLdcOutPutAttr);

    sprintf(FilePath, "%s/ldcport%d_%dx%d_pixel%d_%ld.raw", sFilePath, s32Portid, stLdcOutPutAttr.u16Width, stLdcOutPutAttr.u16Height, stLdcOutPutAttr.ePixelFmt, time(&stTime));

    fp = fopen(FilePath,"wb");
    if(fp == NULL)
    {
        printf("file %s open fail\n", FilePath);
        return 0;
    }

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_LDC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = LdcChn;
    stChnPort.u32PortId = 0;

    while (s32DumpBuffNum+u32SkipBuffNum > 0)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            if(u32SkipBuffNum > 0)
            {
                u32SkipBuffNum--;
                printf("=======Skip port %d id %d =======\n", s32Portid, u32SkipBuffNum);
            }
            else
            {
                s32DumpBuffNum--;
                printf("=======begin writ port %d file id %d, file path %s, bufsize %d, stride %d, height %d\n", s32Portid, s32DumpBuffNum, FilePath, 
                    stBufInfo.stFrameData.u32BufSize,stBufInfo.stFrameData.u32Stride[0], stBufInfo.stFrameData.u16Height);
                /*
                MI_U32  u32Offset =0;
                MI_U32  u32LineOffset=0;
                MI_U32  u32LineSize=0;
                MI_U32  u32LineNum=0;
                u32LineSize = stBufInfo.stFrameData.u16Width;
                u32LineNum = stBufInfo.stFrameData.u16Height;
                if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
                    u32LineSize = u32LineSize;
                else if(stBufInfo.stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
                    u32LineSize = u32LineSize*2;
                else
                    u32LineSize = u32LineSize*4;

                if(stBufInfo.stFrameData.pVirAddr[0] != NULL)
                {
                    u32LineOffset = stBufInfo.stFrameData.u32Stride[0];
                    printf("stride %d, linesize %d, line num %d, ", u32LineOffset, u32LineSize, u32LineNum);
                    ST_WriteOneFrame(fp, u32Offset, (char *)stBufInfo.stFrameData.pVirAddr[0], u32LineOffset, u32LineSize, u32LineNum);
                    u32Offset += u32LineNum *u32LineOffset;
                    printf("write size %d \n", u32Offset);
                }

                if(stBufInfo.stFrameData.pVirAddr[1] != NULL)
                {
                    u32LineOffset = stBufInfo.stFrameData.u32Stride[1];
                    u32LineNum = u32LineNum/2;
                    printf("stride %d, linesize %d, line num %d, ", u32LineOffset, u32LineSize, u32LineNum);
                    ST_WriteOneFrame(fp, u32Offset, (char *)stBufInfo.stFrameData.pVirAddr[0], u32LineOffset, u32LineSize, u32LineNum);
                    u32Offset += u32LineNum *u32LineOffset;
                    printf("write size %d \n", u32Offset);
                }*/
                
                fwrite(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize, 1, fp);
                printf("=======end   writ port %d file id %d, file path %s \n", s32Portid, s32DumpBuffNum, FilePath);
            }
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        usleep(10*1000);
    }
    fclose(fp);
    
    //ExecFunc(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 4), 0);
    printf("\n\n");

    return 0;
}


MI_S32 ST_BaseModuleInit(MI_SNR_PAD_ID_e eSnrPad, MI_VIF_DEV s32vifDev)
{
    MI_U32 u32CapWidth = 0, u32CapHeight = 0;
    MI_VIF_FrameRate_e eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    MI_SYS_PixelFormat_e ePixFormat;
    MI_SNR_PADInfo_t  stPad0Info;
    MI_SNR_PlaneInfo_t stSnrPlane0Info;
    MI_SNR_PAD_ID_e eSnrPadId = eSnrPad;
    MI_VIF_DEV vifDev = s32vifDev;
    MI_VIF_CHN vifChn = s32vifDev*4;
    MI_VPE_CHANNEL vpechn = s32vifDev;
    
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    MI_U8 i=0;

    memset(&stPad0Info, 0x0, sizeof(MI_SNR_PADInfo_t));

     if(E_MI_VPE_HDR_TYPE_OFF== pstVpeChnattr->eHdrType 
        || E_MI_VPE_HDR_TYPE_EMBEDDED == pstVpeChnattr->eHdrType
        || E_MI_VPE_HDR_TYPE_LI== pstVpeChnattr->eHdrType)
    {
        MI_SNR_SetPlaneMode(eSnrPad, FALSE);
    }
    else
    {
        MI_SNR_SetPlaneMode(eSnrPad, TRUE);
    }

    MI_U32 u32ResCount =0;
    MI_U8 u8ResIndex =0;
    MI_SNR_Res_t stRes;
    MI_U8 u8ChocieRes =0;
    MI_S32 s32Input =0;
    memset(&stRes, 0x0, sizeof(MI_SNR_Res_t));

    MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
    for(u8ResIndex=0; u8ResIndex < u32ResCount; u8ResIndex++)
    {
        MI_SNR_GetRes(eSnrPadId, u8ResIndex, &stRes);
        printf("index %d, Crop(%d,%d,%d,%d), outputsize(%d,%d), maxfps %d, minfps %d, ResDesc %s\n",
        u8ResIndex,
        stRes.stCropRect.u16X, stRes.stCropRect.u16Y, stRes.stCropRect.u16Width,stRes.stCropRect.u16Height,
        stRes.stOutputSize.u16Width, stRes.stOutputSize.u16Height,
        stRes.u32MaxFps,stRes.u32MinFps,
        stRes.strResDesc);
    }

    printf("choice which resolution use, cnt %d\n", u32ResCount);
    do
    {
        scanf("%d", &s32Input);
        u8ChocieRes = (MI_U8)s32Input;
        ST_Flush();
        MI_SNR_QueryResCount(eSnrPadId, &u32ResCount);
        if(u8ChocieRes >= u32ResCount)
        {
            printf("choice err res %d > =cnt %d\n", u8ChocieRes, u32ResCount);
        }
    }while(u8ChocieRes >= u32ResCount);

    printf("You select %d res\n", u8ChocieRes);

    MI_SNR_SetRes(eSnrPadId,u8ChocieRes);
    MI_SNR_Enable(eSnrPadId);

    MI_SNR_GetPadInfo(eSnrPadId, &stPad0Info);
    MI_SNR_GetPlaneInfo(eSnrPadId, 0, &stSnrPlane0Info);

    u32CapWidth = stSnrPlane0Info.stCapRect.u16Width;
    u32CapHeight = stSnrPlane0Info.stCapRect.u16Height;
    eFrameRate = E_MI_VIF_FRAMERATE_FULL;
    //stSnrPlane0Info.eBayerId = E_MI_SYS_PIXEL_BAYERID_BG;
    ePixFormat = (MI_SYS_PixelFormat_e)RGB_BAYER_PIXEL(stSnrPlane0Info.ePixPrecision, stSnrPlane0Info.eBayerId);;
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    /************************************************
    Step2:  init VIF(for IPC, only one dev)
    *************************************************/
    MI_VIF_DevAttr_t stDevAttr;
    memset(&stDevAttr, 0x0, sizeof(MI_VIF_DevAttr_t));

    stDevAttr.eIntfMode = stPad0Info.eIntfMode;
    stDevAttr.eWorkMode = E_MI_VIF_WORK_MODE_RGB_REALTIME;
    stDevAttr.eHDRType = (MI_VIF_HDRType_e)pstVpeChnattr->eHdrType;
    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        stDevAttr.eClkEdge = stPad0Info.unIntfAttr.stBt656Attr.eClkEdge;
    else
        stDevAttr.eClkEdge = E_MI_VIF_CLK_EDGE_DOUBLE;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_MIPI)
        stDevAttr.eDataSeq =stPad0Info.unIntfAttr.stMipiAttr.eDataYUVOrder;
    else
        stDevAttr.eDataSeq = E_MI_VIF_INPUT_DATA_YUYV;

    if(stDevAttr.eIntfMode == E_MI_VIF_MODE_BT656)
        memcpy(&stDevAttr.stSyncAttr, &stPad0Info.unIntfAttr.stBt656Attr.stSyncAttr, sizeof(MI_VIF_SyncAttr_t));

    stDevAttr.eBitOrder = E_MI_VIF_BITORDER_NORMAL;

    ExecFunc(MI_VIF_SetDevAttr(vifDev, &stDevAttr), MI_SUCCESS);
    ExecFunc(MI_VIF_EnableDev(vifDev), MI_SUCCESS);

    ST_VIF_PortInfo_T stVifPortInfoInfo;
    memset(&stVifPortInfoInfo, 0, sizeof(ST_VIF_PortInfo_T));
    stVifPortInfoInfo.u32RectX = stSnrPlane0Info.stCapRect.u16X;
    stVifPortInfoInfo.u32RectY = stSnrPlane0Info.stCapRect.u16Y;
    stVifPortInfoInfo.u32RectWidth = u32CapWidth;
    stVifPortInfoInfo.u32RectHeight = u32CapHeight;
    stVifPortInfoInfo.u32DestWidth = u32CapWidth;
    stVifPortInfoInfo.u32DestHeight = u32CapHeight;
    stVifPortInfoInfo.eFrameRate = eFrameRate;
    stVifPortInfoInfo.ePixFormat = ePixFormat;
    STCHECKRESULT(ST_Vif_CreatePort(vifChn, 0, &stVifPortInfoInfo));
    STCHECKRESULT(ST_Vif_StartPort(0, vifChn, 0));

    /************************************************
    Step3:  init VPE (create one VPE)
    *************************************************/
    MI_VPE_ChannelAttr_t stChannelVpeAttr;
    MI_VPE_ChannelPara_t stChannelVpeParam;
    
    memset(&stChannelVpeAttr, 0, sizeof(MI_VPE_ChannelAttr_t));
    memset(&stChannelVpeParam, 0x00, sizeof(MI_VPE_ChannelPara_t));
    
    stChannelVpeParam.eHDRType = pstVpeChnattr->eHdrType;
    stChannelVpeParam.e3DNRLevel = pstVpeChnattr->e3DNRLevel;
    stChannelVpeParam.bMirror = pstVpeChnattr->bChnMirror;
    stChannelVpeParam.bFlip = pstVpeChnattr->bChnFlip;
    MI_VPE_SetChannelParam(vpechn, &stChannelVpeParam);

    stChannelVpeAttr.u16MaxW = u32CapWidth;
    stChannelVpeAttr.u16MaxH = u32CapHeight;
    stChannelVpeAttr.ePixFmt = ePixFormat;
    stChannelVpeAttr.eRunningMode = E_MI_VPE_RUN_REALTIME_MODE;
    stChannelVpeAttr.eSensorBindId = E_MI_VPE_SENSOR0;
    stChannelVpeAttr.bEnLdc = 0;
    stChannelVpeAttr.u32ChnPortMode = pstVpeChnattr->u32ChnPortMode;
    ExecFunc(MI_VPE_CreateChannel(vpechn, &stChannelVpeAttr), MI_VPE_OK);

    STCHECKRESULT(MI_VPE_SetChannelRotation(vpechn, pstVpeChnattr->eVpeRotate));
    STCHECKRESULT(MI_VPE_SetChannelCrop(vpechn, &pstVpeChnattr->stVpeChnCrop));

    STCHECKRESULT(ST_Vpe_StartChannel(vpechn));

    for(i=0; i<ST_MAX_STREAM_NUM-1; i++)
    {
        MI_VPE_PortMode_t stVpeMode;
        memset(&stVpeMode, 0, sizeof(stVpeMode));

        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            MI_VPE_SetPortCrop(vpechn, i, &pstVpeChnattr->stVpePortAttr[i].stPortCrop);

            stVpeMode.u16Width = pstVpeChnattr->stVpePortAttr[i].stPortSize.u16Width;
            stVpeMode.u16Height = pstVpeChnattr->stVpePortAttr[i].stPortSize.u16Height;
            stVpeMode.ePixelFormat = pstVpeChnattr->stVpePortAttr[i].ePixelFormat;
            stVpeMode.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
            stVpeMode.bMirror =pstVpeChnattr->stVpePortAttr[i].bMirror;
            stVpeMode.bFlip = pstVpeChnattr->stVpePortAttr[i].bFlip;

            ExecFunc(MI_VPE_SetPortMode(vpechn, i, &stVpeMode), MI_VPE_OK);

            ExecFunc(MI_VPE_EnablePort(vpechn, i), MI_VPE_OK);
        }
    }

    MI_LDC_DEV LdcDevid = 0;
    MI_LDC_CHN LdcChnId = 0;

    MI_LDC_CreateDevice(LdcDevid);
    MI_LDC_CreateChannel(LdcDevid, LdcChnId);
    for(i=0; i< pstVpeChnattr->u32ViewNum; i++)
    {
        MI_LDC_SetConfig(LdcDevid, LdcChnId, pstVpeChnattr->ldcBinBuffer[i], pstVpeChnattr->u32LdcBinSize[i]);
        
        //free(pstVpeChnattr->ldcBinBuffer);
        if(mi_eptz_buffer_free(pstVpeChnattr->ldcBinBuffer[i]) != MI_EPTZ_ERR_NONE)
        {
            printf("[MI EPTZ ERR]   %d !! \n", __LINE__);
        }
    }
    MI_LDC_StartChannel(LdcDevid, LdcChnId);

    /************************************************
    Step1:  bind VIF->VPE
    *************************************************/
    ST_Sys_BindInfo_T stBindInfo;
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = vifChn;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_REALTIME;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_LDC;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = LdcChnId;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    MI_LDC_OutputPortAttr_t stLdcOutputAttr;
    memset(&stLdcOutputAttr, 0x0, sizeof(MI_LDC_OutputPortAttr_t));

    MI_LDC_GetOutputPortAttr(LdcDevid, LdcChnId, &stLdcOutputAttr);

    MI_VENC_ChnAttr_t stChnAttr;
    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));

    MI_U32 u32VenBitRate = 1024 * 1024 * 4;
    MI_U32 u32VencW  = stLdcOutputAttr.u16Width;
    MI_U32 u32VencH  = stLdcOutputAttr.u16Height;
    MI_U32 VencChn =0, u32DevId =0;

    stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = u32VencW;
    stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = u32VencH;
    stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32VencW;
    stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32VencH;
    stChnAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
    
    stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = u32VenBitRate;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = 30;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = 30;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
    stChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
    stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;

    STCHECKRESULT(ST_Venc_CreateChannel(VencChn, &stChnAttr));
    
    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_LDC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = LdcChnId;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
    stBindInfo.u32BindParam = 0;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    
    STCHECKRESULT(ST_Venc_StartChannel(VencChn));

    return MI_SUCCESS;
}

MI_S32 ST_BaseModuleUnInit(void)
{
    ST_VpeChannelAttr_t *pstVpeChnattr = &gstVpeChnattr;
    MI_U32 i = 0;
    ST_Sys_BindInfo_T stBindInfo;
    MI_U32 VencChn=0, u32DevId =0;

    ExecFunc(MI_VENC_GetChnDevid(VencChn, &u32DevId), MI_SUCCESS);
    
    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_LDC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
    stBindInfo.stDstChnPort.u32DevId = u32DevId;
    stBindInfo.stDstChnPort.u32ChnId = VencChn;
    stBindInfo.stDstChnPort.u32PortId = 0;
    
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Venc_DestoryChannel(VencChn));

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;
    
    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_LDC;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    MI_LDC_StopChannel(0, 0);
    MI_LDC_DestroyChannel(0, 0);
    MI_LDC_DestroyDevice(0);

    memset(&stBindInfo, 0x0, sizeof(ST_Sys_BindInfo_T));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;

    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    for(i = 0; i < ST_MAX_STREAM_NUM; i ++)
    {
        if(pstVpeChnattr->stVpePortAttr[i].bUsed == TRUE)
        {
            STCHECKRESULT(ST_Vpe_StopPort(0, i));
        }
    }

    /************************************************
    Step2:  destory VPE
    *************************************************/
    STCHECKRESULT(ST_Vpe_StopChannel(0));
    STCHECKRESULT(ST_Vpe_DestroyChannel(0));

    /************************************************
    Step3:  destory VIF
    *************************************************/
    STCHECKRESULT(ST_Vif_StopPort(0, 0));
    STCHECKRESULT(ST_Vif_DisableDev(0));

    MI_SNR_Disable(E_MI_SNR_PAD_ID_0);
    /************************************************
    Step4:  destory SYS
    *************************************************/

    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

int main(int argc, char **argv)
{
    MI_SNR_PAD_ID_e eSnrPad = E_MI_SNR_PAD_ID_0;
    MI_VIF_DEV vifDev = 0;
    MI_BOOL bExit = FALSE;

    ST_SetArgs();
    STCHECKRESULT(ST_BaseModuleInit(eSnrPad, vifDev));

    ST_RtspServerStart();

    while(!bExit)
    {
        MI_U32 u32Select = 0xff;

        printf("select 6: Get port buffer\n");
        printf("select 11: exit\n");
        scanf("%d", &u32Select);
        ST_Flush();

        if(u32Select == 6)
        {
            bExit =ST_GetLdcOutputData();
        }
        else if(u32Select == 11)
        {
            bExit = TRUE;
        }

       usleep(100 * 1000);
    }

    ST_RtspServerStop();
    
}


