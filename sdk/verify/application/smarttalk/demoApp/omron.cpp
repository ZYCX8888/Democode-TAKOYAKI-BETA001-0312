#include <poll.h>
#include <sys/syscall.h>

#include "omron.h"
//#include "st_rgn.h"
#include "st_venc.h"
#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "cmdqueue.h"
#include "st_common.h"
#include "st_vdec.h"

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

unsigned int madp_BD_OsCounterGetMs(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    unsigned int T = (1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000) / 1000 ;
    return T;
}

/* Initialization of time */
UINT32 OkaoExtraInitTime(void)
{
    return 0;
}

/* Get of Time (ms) from initialization */
UINT32 OkaoExtraGetTime(UINT32 glOkaoDtStartTime)
{
    return 0;
}
#ifdef __cplusplus
}
#endif	// __cplusplus

extern ST_OMRON_Osd_S g_stOMRONOsd[MAX_OSD_NUM];
extern MsgQueue g_toUI_msg_queue;
extern sem_t g_toUI_sem;

MI_S32 ST_OMRON_OSDAddDirtyArea(ST_OMRON_Osd_S *pstOMRONOsd, ST_Rect_T stRect)
{
#if 0
    ST_OMRON_Area_Node_S* pstAreaNode = NULL;

    if (pstOMRONOsd == NULL)
    {
        return 0;
    }

    pstAreaNode = (ST_OMRON_Area_Node_S *)malloc(sizeof(ST_OMRON_Area_Node_S));
    if (pstAreaNode == NULL)
    {
        ST_ERR("malloc error\n");
        return 0;
    }

    pstAreaNode->stRect = stRect;
    list_add_tail(&pstAreaNode->list, &pstOMRONOsd->dirtyAreaList);
#endif
    return 0;
}

MI_S32 ST_OMRON_OSDClearDirtyArea(ST_OMRON_Osd_S *pstOMRONOsd)
{
#if 0
    struct list_head *pHead = NULL;
    struct list_head *pListPos = NULL;
    struct list_head *pListPosN = NULL;
    ST_OMRON_Area_Node_S* pstAreaNode = NULL;

    if (pstOMRONOsd == NULL)
    {
        return 0;
    }

    if (list_empty(&pstOMRONOsd->dirtyAreaList))
    {
        return 0;
    }
    pHead = &pstOMRONOsd->dirtyAreaList;
    list_for_each_safe(pListPos, pListPosN, pHead)
    {
        pstAreaNode = list_entry(pListPos, ST_OMRON_Area_Node_S, list);
        list_del(pListPos);

        ST_OSD_ClearRectFast(pstOMRONOsd->hHandle, pstAreaNode->stRect);

        free(pstAreaNode);
        pstAreaNode = NULL;
    }
#endif
    return 0;
}

void ST_OMRON_DrawFDFRResult(ST_OMRON_Mng_S* pstOMRONMng, ST_OMRON_FR_Result_S stFrResult)
{
#if 0
    MI_U32 u32BaseWidth = 1;
    MI_U32 u32BaseHeight = 1;
    MI_U32 i = 0, j = 0;
    MI_RGN_CanvasInfo_t* pstCanvasInfo;
    MI_U8 u8BorderWidth = 4;
    ST_Rect_T stRc;
    MI_U32 u32Color = I4_RED;
    ST_OMRON_Osd_S *pstOMRONOsd = g_stOMRONOsd;

    if (pstOMRONMng == NULL)
    {
        return;
    }

    u32BaseWidth = pstOMRONMng->u32Width;
    u32BaseHeight = pstOMRONMng->u32Height;

    for (i = 0; i < MAX_OSD_NUM; i ++)
    {
        if ((pstOMRONOsd[i].hHandle == ST_OSD_HANDLE_INVALID) || (pstOMRONOsd[i].s32InitFlag == FALSE))
        {
            continue;
        }

        pthread_mutex_lock(&pstOMRONOsd[i].mutex);
        // get canvas info
        ST_OSD_GetCanvasInfo(pstOMRONOsd[i].hHandle, &pstCanvasInfo);

        // clear dirty area
        ST_OMRON_OSDClearDirtyArea(&pstOMRONOsd[i]);

        for (j = 0; j < stFrResult.u32Num; j ++)
        {
            memset(&stRc, 0, sizeof(ST_Rect_T));

            stRc.s32X = (pstOMRONOsd[i].u32Width * stFrResult.pos[j].s32X) / u32BaseWidth;
            stRc.s32Y = (pstOMRONOsd[i].u32Height * stFrResult.pos[j].s32Y) / u32BaseHeight;
            stRc.s16PicW = (pstOMRONOsd[i].u32Width * stFrResult.pos[j].s16PicW) / u32BaseWidth;
            stRc.s16PicH = (pstOMRONOsd[i].u32Height * stFrResult.pos[j].s16PicH) / u32BaseHeight;

            // ST_DBG("rgn:%d, %d,%d,%d,%d\n", i, stRc.u32X, stRc.u32Y, stRc.u16PicW, stRc.u16PicH);
            if (stFrResult.bValid[j] == TRUE)
            {
                u32Color = I4_GREEN;
            }
            else
            {
                u32Color = I4_RED;
            }
            // ST_OSD_DrawRect(pstOMRONOsd[i].hHandle, stRc, u8BorderWidth, I4_RED);
            ST_OSD_DrawRectFast(pstOMRONOsd[i].hHandle, stRc, u8BorderWidth, u32Color);

            // calc border width
            stRc.s16PicW += 2 * u8BorderWidth;
            stRc.s16PicH += 2 * u8BorderWidth;
            // add dirty area
            ST_OMRON_OSDAddDirtyArea(&pstOMRONOsd[i], stRc);
        }

        // update canvas info
        ST_OSD_Update(pstOMRONOsd[i].hHandle);

        pthread_mutex_unlock(&pstOMRONOsd[i].mutex);
        //for (i = 0; i < stFrResult.u32Num; i++)
        //{
        //    printf("FR num(%d) area (x-y-w-h)=(%d-%d-%d-%d)...\n", stFrResult.u32Num, stFrResult.pos[i].u32X, stFrResult.pos[i].u32Y,
        //        stFrResult.pos[i].u16PicW, stFrResult.pos[i].u16PicH);
        //}   
    }
#endif
}

MI_U32 ST_OMRON_QueryAlbum(void)
{
    FILE *pFile;
    struct stat statbuf;

    /* Set Album data in pucBuffer */
    if ( ( pFile = fopen( OMRON_ALBUM_FILE , "rb" ) ) == NULL )
    {
        ST_ERR( "%s : File Open Error!\n", OMRON_ALBUM_FILE);
        return 0;
    }

    if ( fstat(fileno(pFile), &statbuf) < 0 )
    {
        return 0;
    }

    ST_DBG( "%s : Size [%d]\n", OMRON_ALBUM_FILE, (UINT32)statbuf.st_size);

    fclose(pFile);

    return (MI_U32)statbuf.st_size;
}

HALBUM ST_OMRON_CreateOrLoadAlbum(MI_U32 u32BufSize)      /* Album data size */
{
    HALBUM hAL = NULL;          /* Album handle */
    UINT8 *pucBuffer = NULL;    /* Album data buffer */
    FR_ERROR nError;            /* Error code */

    UINT32 unSize;
    FILE *pFile;

    if (u32BufSize == 0 /* If Library is run for the first time */ )
    {
        hAL = OKAO_FR_CreateAlbumHandle(1000, 10);
    }
    else
    { /* Incase Album is stored only during executing the library */
        pucBuffer = (UINT8 *)malloc(u32BufSize);
        if ( pucBuffer == NULL )
        {
            /* Error Handling */
            ST_ERR("%s : Malloc Error!\n", OMRON_ALBUM_FILE);
            return NULL;
        }

        /* Set Album data in pucBuffer */
        if ( ( pFile = fopen( OMRON_ALBUM_FILE , "rb" ) ) == NULL )
        {
            ST_ERR("%s : File Open Error!\n", OMRON_ALBUM_FILE);
            free(pucBuffer);
            return NULL;
        }

        unSize = fread(pucBuffer, 1, u32BufSize, pFile);
        if ( unSize != u32BufSize )
        {
            ST_WARN("%s : Read Size Error [%d != %d]\n", OMRON_ALBUM_FILE, unSize, u32BufSize);
        }
        fclose(pFile);

        hAL = OKAO_FR_RestoreAlbum(pucBuffer, u32BufSize, &nError);
        ST_DBG("Load [%s] : OK\n", OMRON_ALBUM_FILE );

        free(pucBuffer);
    }

    return hAL;
}

MI_S32 ST_OMRON_InitLibrary(ST_OMRON_Mng_S* pstOMRONMng)
{
    MI_U32 u32AlbumSize = 0;

    u32AlbumSize = ST_OMRON_QueryAlbum();

    OMRON_SDK_CHECK(OKAO_CO_CreateHandle(), pstOMRONMng->hCo);
    OMRON_SDK_CHECK(OKAO_CreateDetection(), pstOMRONMng->hDT);
    OMRON_SDK_CHECK(OKAO_CreateDtResult(OMRON_MAX_FACE_NUM, 0), pstOMRONMng->hDtResult);
    OMRON_SDK_CHECK(OKAO_PT_CreateHandle(pstOMRONMng->hCo), pstOMRONMng->hPT);
    OMRON_SDK_CHECK(OKAO_PT_CreateResultHandle(pstOMRONMng->hCo), pstOMRONMng->hPtResult);
    OMRON_SDK_CHECK(OKAO_FR_CreateFeatureHandle(), pstOMRONMng->hFD);
    pstOMRONMng->hAL = ST_OMRON_CreateOrLoadAlbum(u32AlbumSize);
    if (NULL == pstOMRONMng->hAL)
    {
        ST_ERR("ST_OMRON_CreateOrLoadAlbum error\n");
        return -1;
    }

    return OMCV_NORMAL;
}

MI_S32 ST_OMRON_UnInitLibrary(ST_OMRON_Mng_S* pstOMRONMng)
{
    if (pstOMRONMng->hAL != NULL)
    {
        OKAO_FR_DeleteAlbumHandle(pstOMRONMng->hAL);
        pstOMRONMng->hAL = NULL;
    }

    if (pstOMRONMng->hFD != NULL)
    {
        OKAO_FR_DeleteFeatureHandle(pstOMRONMng->hFD);
        pstOMRONMng->hFD = NULL;
    }

    if (pstOMRONMng->hPtResult != NULL)
    {
        OKAO_PT_DeleteResultHandle(pstOMRONMng->hPtResult);
        pstOMRONMng->hPtResult = NULL;
    }

    if (pstOMRONMng->hPT != NULL)
    {
        OKAO_PT_DeleteHandle(pstOMRONMng->hPT);
        pstOMRONMng->hPT = NULL;
    }

    if (pstOMRONMng->hDtResult != NULL)
    {
        OKAO_DeleteDtResult(pstOMRONMng->hDtResult);
        pstOMRONMng->hDtResult = NULL;
    }

    if (pstOMRONMng->hDT != NULL)
    {
        OKAO_DeleteDetection(pstOMRONMng->hDT);
        pstOMRONMng->hDT = NULL;
    }

    if (pstOMRONMng->hCo != NULL)
    {
        OKAO_CO_DeleteHandle(pstOMRONMng->hCo);
        pstOMRONMng->hCo = NULL;
    }

    return OMCV_NORMAL;
}

MI_S32 ST_OMRON_BDSetParams(ST_OMRON_Mng_S* pstOMRONMng)
{
    RECT stEdgeMask;

    /* Sets the Minimum and Maximum Human body sizes to be detected */
    OMRON_SDK_CHECK_V(OMCV_BD_SetSizeRange(pstOMRONMng->hBody, 40, 8192));

    /* Sets the Angle settings for Human body detection */
    OMRON_SDK_CHECK_V(OMCV_BD_SetAngle(pstOMRONMng->hBody, ROLL_ANGLE_UP));

    /* Sets the Human body detection Rectangular Mask */
    stEdgeMask.left = EDGEMASK_PIXEL;
    stEdgeMask.top = EDGEMASK_PIXEL;
    stEdgeMask.right = (EDGEMASK_PIXEL == -1) ? EDGEMASK_PIXEL : ( pstOMRONMng->u32Width - (EDGEMASK_PIXEL + 1) );
    stEdgeMask.bottom = (EDGEMASK_PIXEL == -1) ? EDGEMASK_PIXEL : ( pstOMRONMng->u32Height - (EDGEMASK_PIXEL + 1) );
    OMRON_SDK_CHECK_V(OMCV_BD_SetEdgeMask(pstOMRONMng->hBody, stEdgeMask));

    /* Sets the Human body detection Threshold */
    OMRON_SDK_CHECK_V(OMCV_BD_SetThreshold(pstOMRONMng->hBody, 750));

    /* Sets the search density */
    OMRON_SDK_CHECK_V(OMCV_BD_SetSearchDensity(pstOMRONMng->hBody, BD_DENSITY_HIGH_190));

    /* Sets search cycle */
    OMRON_SDK_CHECK_V(OMCV_BD_MV_SetSearchCycle(pstOMRONMng->hBody, 3, 15));

    /* Sets Max Retry Count and Max Hold Count */
    OMRON_SDK_CHECK_V(OMCV_BD_MV_SetLostParam(pstOMRONMng->hBody, 2, 2));

    /* Sets the steadiness parameters for size and positon of the detection result */
    OMRON_SDK_CHECK_V(OMCV_BD_MV_SetSteadinessParam(pstOMRONMng->hBody, 10, 10));

    /* Sets tracking swap parameter */
    OMRON_SDK_CHECK_V(OMCV_BD_MV_SetTrackingSwapParam(pstOMRONMng->hBody, 150));

    return OMCV_NORMAL;
}

MI_S32 ST_OMRON_FDFRSetParams(ST_OMRON_Mng_S* pstOMRONMng)
{
    RECT rcStillArea;
    RECT rcMotionArea;
    UINT32 anAngle[POSE_TYPE_COUNT];
    INT32 nMotionAngleExtension;

    /* Sets the Face Detection Mode */
    OMRON_SDK_CHECK_V(OKAO_SetDtMode(pstOMRONMng->hDT, DT_MODE_MOTION1));

    /* Sets the Minimum and Maximum face sizes to be detected */
    OMRON_SDK_CHECK_V(OKAO_SetDtFaceSizeRange(pstOMRONMng->hDT, 40, pstOMRONMng->u32Width));

    /* Sets the Angle settings for face detection */
    anAngle[POSE_FRONT] = ANGLE_0;
    anAngle[POSE_HALF_PROFILE] = ANGLE_NONE;
    anAngle[POSE_PROFILE] = ANGLE_NONE;
    nMotionAngleExtension = ANGLE_ROTATION_EXT1 | ANGLE_POSE_EXT1 | DETECT_HEAD_USE;
    OMRON_SDK_CHECK_V(OKAO_SetDtAngle(pstOMRONMng->hDT, anAngle, nMotionAngleExtension));

    /* Sets the Direction Mask for Motion mode */
    OMRON_SDK_CHECK_V(OKAO_SetDtDirectionMask(pstOMRONMng->hDT, TRUE));

    /* Sets the timeout time for OKAO_Detection() */
    OMRON_SDK_CHECK_V(OKAO_SetDtTimeout(pstOMRONMng->hDT, 0, 0));

    /* Sets the Face Detction Rectangular Mask */
    rcStillArea.left = 5;
    rcStillArea.top = 5;
    rcStillArea.right = pstOMRONMng->u32Width - 5;
    rcStillArea.bottom = pstOMRONMng->u32Height - 5;
    rcMotionArea.left = 5;
    rcMotionArea.top = 5;
    rcMotionArea.right = pstOMRONMng->u32Width - 5;
    rcMotionArea.bottom = pstOMRONMng->u32Height - 5;
    OMRON_SDK_CHECK_V(OKAO_SetDtRectangleMask(pstOMRONMng->hDT, rcStillArea, rcMotionArea));

    /* Sets the Face Detection Threshold */
    OMRON_SDK_CHECK_V(OKAO_SetDtThreshold(pstOMRONMng->hDT, 500, 500));

    /* Sets the search density coefficient for face detection */
    OMRON_SDK_CHECK_V(OKAO_SetDtStep(pstOMRONMng->hDT, 33, 33));

    /* Sets Motion Face Detection Refresh Count for each Motion mode */
    OMRON_SDK_CHECK_V(OKAO_SetDtRefreshCount(pstOMRONMng->hDT, DT_MODE_MOTION1, 15));

    /* Sets Motion Face Detection Retry Count, Motion Head Detection Retry Count and Hold Count at lost time */
    OMRON_SDK_CHECK_V(OKAO_SetDtLostParam(pstOMRONMng->hDT, 2, 3, 2));

    /* Sets the motion mode face detection position correction parameter */
    OMRON_SDK_CHECK_V(OKAO_SetDtModifyMoveRate(pstOMRONMng->hDT, 4));

    return OMCV_NORMAL;
}

void *ST_OMRON_BD_Process(void *args)
{
    ST_OMRON_Mng_S* pstOMRONMng = (ST_OMRON_Mng_S *)args;
    INT32 nBreakFlag;               /* Cancellation flag */
    INT32 nRet = OMCV_NORMAL;  		/* Return code */
    INT32 nResultCount;             /* The number of detected body */
    INT32 nIndex;                   /* Index of face detection */
    DETECTION_INFO stDetectionInfo; /* Human body Result */
    ST_Rect_T stRect;
    ST_OMRON_HD_Result_S stHdResult;

    ST_DBG("pid=%d, %d\n", getpid(), syscall(SYS_gettid));

    stRect.s32X = 0;
    while(pstOMRONMng->bTaskRun)
    {
        pthread_mutex_lock(&pstOMRONMng->condMutex);
        pthread_cond_wait(&pstOMRONMng->dataCond, &pstOMRONMng->condMutex);
        pthread_mutex_unlock(&pstOMRONMng->condMutex);

        // ST_DBG("oooo, get Y data\n");
        /************************/
        /* Human body detection */
        /************************/
        /* Initialize Cancellation flag */
        nBreakFlag = FLAG_OFF;

        /* Executes Human body Detection */
        nRet = OMCV_BD_Detect(pstOMRONMng->hBody, pstOMRONMng->pYData,
        pstOMRONMng->u32Width, pstOMRONMng->u32Height, &nBreakFlag, pstOMRONMng->hBodyResult);
        if ((nRet != OMCV_NORMAL) && (nRet != OKAO_BREAK))
        {
        ST_ERR( "OMCV_BD_Detect() Error : %d\n", nRet );
        continue;
        }

        /* Gets the number of detected Human body */
        nRet = OMCV_BD_GetResultCount(pstOMRONMng->hBodyResult, &nResultCount);
        if (nRet != OMCV_NORMAL)
        {
        ST_ERR( "OMCV_BD_GetResultCount() Error : %d\n", nRet );
        continue;
        }

        if(0 < nResultCount)
        {
        // ST_DBG("OMCV_BD_GetResultCount() OK : nResultCount = %d\n", nResultCount);
        }
        else
        {
        // ST_DBG("detect nothing\n");
        }

        memset(&stHdResult, 0, sizeof(ST_OMRON_HD_Result_S));
        stHdResult.u32Num = 0;
        for(nIndex = 0; nIndex < nResultCount; nIndex++)       /*** Human body Loop ***/
        {
            /* Gets the detection result for each Human body */
            memset(&stDetectionInfo, 0, sizeof(DETECTION_INFO));
            nRet = OMCV_BD_GetResultInfo(pstOMRONMng->hBodyResult, nIndex, &stDetectionInfo);
            if (nRet != OMCV_NORMAL)
            {
                ST_ERR( "OMCV_BD_GetResultInfo(%d) Error : %d\n", nIndex, nRet );
                break;
            }

#if 0
            printf("   <NO.%d>  (Confidence=%d, Center=[%d,%d], Width=%d, Height=%d, Angle=%d, ID=%d, DetectionMethod=%d, HoldCount=%d)\n",
            nIndex, stDetectionInfo.nConfidence,
            stDetectionInfo.ptCenter.x, stDetectionInfo.ptCenter.y,
            stDetectionInfo.nWidth, stDetectionInfo.nHeight,
            stDetectionInfo.nAngle, stDetectionInfo.nID,
            stDetectionInfo.nDetectionMethod, stDetectionInfo.nHoldCount);
#endif

            stHdResult.pos[nIndex].s32X = (stDetectionInfo.ptCenter.x > (stDetectionInfo.nWidth >> 1)) ?
                stDetectionInfo.ptCenter.x - (stDetectionInfo.nWidth >> 1) : 0;
            stHdResult.pos[nIndex].s32Y = (stDetectionInfo.ptCenter.y > (stDetectionInfo.nHeight >> 1)) ?
                stDetectionInfo.ptCenter.y - (stDetectionInfo.nHeight >> 1) : 0;
            stHdResult.pos[nIndex].s16PicW = stDetectionInfo.nWidth;
            stHdResult.pos[nIndex].s16PicH = stDetectionInfo.nHeight;

            stHdResult.u32Num++;

        }

        //ST_OMRON_DrawHDResult(pstOMRONMng, stHdResult);
        // showfps();

        // DEBUG
#if 0
        stRect.u32X += 20;
        if (stRect.u32X > 1200)
        stRect.u32X = 0;
        stRect.u32Y = 20;
        stRect.u16PicW = 100;
        stRect.u16PicH = 100;

        ST_OMRON_UpdateHDResult(pstOMRONMng, stRect);
#endif
        // DEBUG
    }

    ST_DBG("pid=%d, %d, exit\n", getpid(), syscall(SYS_gettid));

    return NULL;
}

MI_S32 ST_OMRON_ALBUMRegisterData(ST_OMRON_Mng_S* pstOMRONMng, INT32 nUserID, INT32 nDataID)
{
    BOOL bRegist;       /* Registration confirmation flag */

    OMRON_SDK_CHECK_V(OKAO_FR_IsRegistered(pstOMRONMng->hAL, nUserID, nDataID, &bRegist));

    if (bRegist)
    {
        /* The specified nUid, nDid are already registered. If registered again,
            it will over-write the previous registration. */
    }
    else
    {
        /* Specified nUid, nDid are not registered. If registered,
         it will be a new registration. */
    }

    OMRON_SDK_CHECK_V(OKAO_FR_RegisterData(pstOMRONMng->hAL, pstOMRONMng->hFD, nUserID, nDataID));

    return OKAO_NORMAL;
}

MI_S32 ST_OMRON_ALBUMBackupAlbum(ST_OMRON_Mng_S* pstOMRONMng)                /* Album handle */
{
    INT32 nRet;                 /* Error code */

    UINT32 unSize;              /* Album size */
    UINT8 *pucBuffer = NULL;    /* Local Pointer to store Album data*/
    FILE *pFile;

    OMRON_SDK_CHECK_V(OKAO_FR_GetSerializedAlbumSize(pstOMRONMng->hAL, &unSize));

    pucBuffer = (UINT8 *)malloc(unSize);
    if ( pucBuffer == NULL )
    {
        /* Error handling */
        ST_ERR("%s : Malloc Error!\n", OMRON_ALBUM_FILE);
        return -1;
    }

    nRet = OKAO_FR_SerializeAlbum(pstOMRONMng->hAL, pucBuffer, unSize);
    if ( nRet != OKAO_NORMAL )
    {
        /* Error handling */
        ST_ERR("OKAO_FR_SerializeAlbum() Error : %d\n", nRet);
        free(pucBuffer);
        return -1;
    }

    if((pFile = fopen(OMRON_ALBUM_FILE , "wb")) == NULL)
    {
        ST_ERR( "%s : File Open Error!\n", OMRON_ALBUM_FILE);
        free(pucBuffer);
        return -1;
    }

    fwrite(pucBuffer, 1, unSize, pFile);
    fclose(pFile);
    ST_DBG( "Save [%s] : OK\n", OMRON_ALBUM_FILE);

    free(pucBuffer);
    return TRUE;
}

MI_S32 ST_OMRON_SnapFace(MI_U8 *u8YData, MI_U8 *u8UVData, MI_S32 s32X, MI_S32 s32Y, MI_S16 s16FaceW, MI_S16 s16FaceH)
{
#if 0
    MI_BOOL bThdRun;
    MI_S32 s32Ret;
    MI_U32 u32Retry = 0, s32VencDevID = 0;
    MI_S32 s32FdStream = -1;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSnapFace;
    MI_SYS_ChnPort_t stSysChnPort;
    ST_Rect_T stCrop= {0, 0, 0, 0};
    MI_U64 u64Time;
    stCrop.s32X = s32X;
    stCrop.s32Y = s32Y;
    stCrop.s16PicW = s16FaceW;
    stCrop.s16PicH = s16FaceH;
    ST_CreateDivpChannel(JPEG_DIVP_CHN, s16FaceW, s16FaceH, E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420, stCrop);

    if (MI_SUCCESS != ST_VencCreateChannel(SNAPFACE_CHN, E_ST_JPEG, s16FaceW, s16FaceH, 1))
    {
        ST_ERR("CreateVencChannel (%d) Fail\n", SNAPFACE_CHN);
        return -1;
    }
    s32Ret = MI_VENC_GetChnDevid(SNAPFACE_CHN, &s32VencDevID);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("Get Device ID:%X\n :%d", s32Ret, s32VencDevID);
        return -1;
    }
    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = 640;
    stBufConf.stFrameCfg.u16Height = 480;

    ST_ModuleBind(E_MI_MODULE_ID_DIVP, 0, JPEG_DIVP_CHN, 0,
                E_MI_MODULE_ID_VENC, s32VencDevID, SNAPFACE_CHN, 0); //DIVP->VENC
    stSysChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stSysChnPort.u32DevId = 0;
    stSysChnPort.u32ChnId = JPEG_DIVP_CHN;
    stSysChnPort.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stSysChnPort, 2, 3);
    s32Ret = MI_SYS_ChnInputPortGetBuf(&stSysChnPort, &stBufConf, &stBufInfo, &hSnapFace, 1000);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("GetInputPortBuf Fail...\n");
        ST_VencDestroyChannel(SNAPFACE_CHN);
        return -1;
    }
    else
    {
        ST_DBG("Get Snap venc buf success\n");
    }
    memcpy(stBufInfo.stFrameData.pVirAddr[0], u8YData, 640*480);
    memcpy(stBufInfo.stFrameData.pVirAddr[1], u8UVData, 640*480 >> 1);
    s32Ret = MI_SYS_ChnInputPortPutBuf(hSnapFace ,&stBufInfo , FALSE);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("Inject Buffer fail...\n");
        ST_VencDestroyChannel(SNAPFACE_CHN);
        ST_DestroyDivpChannel(JPEG_DIVP_CHN);
        return -1;
    }
    else
    {
        ST_DBG("Inject Snap venc buf success\n");
    }

    s32FdStream = MI_VENC_GetFd(SNAPFACE_CHN);
    if(s32FdStream <= 0)
    {
        ST_ERR("Unable to get FD:%d for ch:%2d\n", s32FdStream, SNAPFACE_CHN);
        ST_VencDestroyChannel(SNAPFACE_CHN);
        ST_DestroyDivpChannel(JPEG_DIVP_CHN);
        return -1;
    }
    else
    {
        printf("CH%2d FD%d\n", SNAPFACE_CHN, s32FdStream);
    }
    bThdRun = FALSE;
    while(!bThdRun)
    {
        MI_VENC_Stream_t stStream;
        MI_VENC_Pack_t stPack0;
        MI_VENC_ChnStat_t stStat;

        fd_set fdsread;
        struct timeval tv;
        FD_ZERO(&fdsread);
        FD_SET(s32FdStream, &fdsread);

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        s32Ret = select(s32FdStream + 1, &fdsread, NULL, NULL, &tv);
        if(s32Ret < 0)
        {
            ST_ERR("select");
            continue;
        }
        else if(s32Ret == 0)
        {
            ST_ERR("timeout\n");
            sleep(1);
            continue;
        }
        else
        {
            //DBG_ERR("ok\n");
        }

        s32Ret = MI_VENC_Query(SNAPFACE_CHN, &stStat);
        if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
        {
            ST_ERR("Unexpected query state:%X curPacks:%d\n", s32Ret, stStat.u32CurPacks);
            continue;
        }

        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack0, 0, sizeof(stPack0));
        stStream.pstPack = &stPack0;
        stStream.u32PackCount = 1;
        s32Ret = MI_VENC_GetStream(SNAPFACE_CHN, &stStream, 0);

        if(s32Ret != MI_SUCCESS)
        {
            
            if(s32Ret != MI_ERR_VENC_NOBUF)
            {
                ST_ERR("GetStream Err:%X seq:%d\n\n", s32Ret, stStream.u32Seq);
            }
            u32Retry++;
            continue;
        }
        else
        {
            int iRet;

            if(u32Retry > 0)
            {
                ST_INFO("Retried %d times on CH%2d\n", u32Retry, SNAPFACE_CHN);
                u32Retry = 0;
            }
            printf("Get face jpeg success addr=%p len = %d\n", stStream.pstPack[0].pu8Addr, stStream.pstPack[0].u32Len);
            FILE *jpegfp = NULL;
            jpegfp = fopen("appres/faceR.jpg", "w+");
            if (jpegfp)
            {
                fwrite(stStream.pstPack[0].pu8Addr, 1, stStream.pstPack[0].u32Len, jpegfp);
                fclose(jpegfp);
            }
            s32Ret = MI_VENC_ReleaseStream(SNAPFACE_CHN, &stStream);
            if(s32Ret != MI_SUCCESS)
            {
                ST_ERR("Unable to put output %X\n", s32Ret);
            }
            bThdRun = TRUE;
        }
    }
    ST_ModuleUnBind(E_MI_MODULE_ID_DIVP, 0, JPEG_DIVP_CHN, 0,
                E_MI_MODULE_ID_VENC, s32VencDevID, SNAPFACE_CHN, 0); //DIVP->VENC
    ST_VencDestroyChannel(SNAPFACE_CHN);
    ST_DestroyDivpChannel(JPEG_DIVP_CHN);
#endif
    return 0;
}

int g_faceSnap = 0;
int g_faceDetect = 1;
int NV12CutImage(unsigned char * src, unsigned char * dest, int srcW, int srcH, int x0, int y0, int dstw, int dsth)
{
    int i = 0;
    int j = 0;
    int k = 0;

    int x1;
    int y1;
    x1 = dstw+x0-1;
    y1 = dsth+y0-1;
    for (i = y0; i <= y1; i++) //h
    {
        for (j = x0; j <= x1; j++)
        {
            dest[((i - y0) / 2 * (dstw / 2) + (j - x0) / 2) * 2 + 0] = src[((i / 2) * (srcW / 2) + (j / 2)) * 2 + 0];

            dest[((i - y0) / 2 * (dstw / 2) + (j - x0) / 2) * 2 + 1] = src[((i / 2) * (srcW / 2) + (j / 2)) * 2 + 1];

        }
    }
    return 0;
}


void *ST_OMRON_FDFR_Process(void *args)
{
    ST_OMRON_Mng_S* pstOMRONMng = (ST_OMRON_Mng_S *)args;
    INT32 i;
    INT32 nCount;       /* Number of faces detected */
    INT32 nIndex;       /* Index Number */
    FACEINFO info;      /* Detection Result */
    INT32 anConf[PT_POINT_KIND_MAX];
    POINT aptPoint[PT_POINT_KIND_MAX];
    INT32 nValidPT = TRUE;
    INT32 nValidNum = 0;/* Number of face detected with valid range */
    INT32 nUid[3];      /* User ID list */
    INT32 nScore[3];    /* Score List */
    INT32 nCount2;      /* Number of outputs of Face recognition */
    MI_U8 *pTempYData, *pTempUVData;
    char szFileName[64] = {0,};
    int fd = -1;
    MI_S32 s32Ret = -1;

    ST_OMRON_FR_Result_S stFrResult;

    ST_DBG("pid=%d, %d\n", getpid(), syscall(SYS_gettid));
    
    //static int saveface = 0;
    pTempYData = NULL;
    pTempUVData = NULL;
    pTempYData = (MI_U8*)malloc(640*480);
    pTempUVData = (MI_U8*)malloc(640*240);
    if ((!pTempYData) || (!pTempUVData))
    {
        ST_ERR("malloc temp buffer fail\n");
        return NULL;
    }
    while (pstOMRONMng->bTaskRun)
    {
        if (!g_faceDetect)
        {
            usleep(30*1000);
            continue;
        }   
        pthread_mutex_lock(&pstOMRONMng->condMutex);
        pthread_cond_wait(&pstOMRONMng->dataCond, &pstOMRONMng->condMutex);

        //ST_DBG("oooo, get Y data\n");
        //continue;
        // DEBUG
        // save Y data, luma
#if 0
        memset(szFileName, 0, sizeof(szFileName) - 1);
        snprintf(szFileName, sizeof(szFileName) - 1, "divp%d_port%d_%dx%d_luma.yuv", pstOMRONMng->stSource.u32Chn,
        pstOMRONMng->stSource.u32Port, pstOMRONMng->u32Width, pstOMRONMng->u32Height);
        if (fd < 0)
        {
        fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }

        if (fd > 0)
        {
        write(fd, pstOMRONMng->pYData, pstOMRONMng->u32MaxSize);
        }
#endif
        // DEBUG

        OKAO_Detection(pstOMRONMng->hDT, pstOMRONMng->pYData, pstOMRONMng->u32Width, pstOMRONMng->u32Height,
            ACCURACY_NORMAL, pstOMRONMng->hDtResult);

        OKAO_GetDtFaceCount(pstOMRONMng->hDtResult, &nCount);

        memset(&stFrResult, 0, sizeof(ST_OMRON_FR_Result_S));
        nValidNum = 0;
        for (nIndex = 0; nIndex < nCount; nIndex++)
        {
            /* Gets the detection result for each face */
            OKAO_GetDtFaceInfo(pstOMRONMng->hDtResult, nIndex, &info);
            if ( (info.ptLeftTop.x < 0) || (info.ptLeftTop.y < 0) ||
                ((info.ptRightTop.x >= pstOMRONMng->u32Width) ||
                (info.ptLeftBottom.y >= pstOMRONMng->u32Width)))
            {
                continue;
            }

            OKAO_PT_SetPositionFromHandle(pstOMRONMng->hPT, pstOMRONMng->hDtResult, nIndex);
            OKAO_PT_DetectPoint_GRAY(pstOMRONMng->hPT, pstOMRONMng->pYData, pstOMRONMng->u32Width,
            pstOMRONMng->u32Height, GRAY_ORDER_Y0Y1Y2Y3, pstOMRONMng->hPtResult);

            /* Gets Facial Parts Position Result */
            OKAO_PT_GetResult(pstOMRONMng->hPtResult, PT_POINT_KIND_MAX, aptPoint, anConf);
            nValidPT = TRUE;
            for (i = 0; i < PT_POINT_KIND_MAX; i++)
            {
                if ((aptPoint[i].x < 0) || (aptPoint[i].y < 0) )
                {
                    nValidPT = FALSE;
                    break;
                }
            }
            if (!nValidPT)
            {
                continue;
            }

            stFrResult.pos[nValidNum].s32X = info.ptLeftTop.x;
            stFrResult.pos[nValidNum].s32Y = info.ptLeftTop.y;
            stFrResult.pos[nValidNum].s16PicW = info.ptRightTop.x - info.ptLeftTop.x + 1;
            stFrResult.pos[nValidNum].s16PicH = info.ptLeftBottom.y - info.ptLeftTop.y + 1;

#if 0
            ST_DBG("   <NO.%d>  (Confidence=%d, [%d,%d](%d,%d), ID=%d)\n",
            nIndex, info.nConfidence,
            stFrResult.pos[nValidNum].u32X, stFrResult.pos[nValidNum].u32Y,
            stFrResult.pos[nValidNum].u16PicW, stFrResult.pos[nValidNum].u16PicH,
            info.nID);

            ST_DBG("      eOkaoPt OK (Left Eye  [%d,%d]:%d - [%d,%d]:%d)\n",
            aptPoint[PT_POINT_LEFT_EYE_IN].x, aptPoint[PT_POINT_LEFT_EYE_IN].y, anConf[PT_POINT_LEFT_EYE_IN],
            aptPoint[PT_POINT_LEFT_EYE_OUT].x, aptPoint[PT_POINT_LEFT_EYE_OUT].y, anConf[PT_POINT_LEFT_EYE_OUT]);
            ST_DBG("                 (Right Eye [%d,%d]:%d - [%d,%d]:%d)\n",
            aptPoint[PT_POINT_RIGHT_EYE_IN].x, aptPoint[PT_POINT_RIGHT_EYE_IN].y, anConf[PT_POINT_RIGHT_EYE_IN],
            aptPoint[PT_POINT_RIGHT_EYE_OUT].x, aptPoint[PT_POINT_RIGHT_EYE_OUT].y, anConf[PT_POINT_RIGHT_EYE_OUT]);
            ST_DBG("                 (Mouth     [%d,%d]:%d - [%d,%d]:%d)\n",
            aptPoint[PT_POINT_MOUTH_LEFT].x, aptPoint[PT_POINT_MOUTH_LEFT].y, anConf[PT_POINT_MOUTH_LEFT],
            aptPoint[PT_POINT_MOUTH_RIGHT].x, aptPoint[PT_POINT_MOUTH_RIGHT].y, anConf[PT_POINT_MOUTH_RIGHT]);
#endif
            OKAO_FR_ExtractFeature(pstOMRONMng->hFD, pstOMRONMng->pYData, pstOMRONMng->u32Width, pstOMRONMng->u32Height,
            PT_POINT_KIND_MAX, aptPoint, anConf);

            OKAO_FR_Identify(pstOMRONMng->hFD, pstOMRONMng->hAL, 1, nUid, nScore, &nCount2);

            for (i = 0; i < nCount2; i++)
            {
                if (nScore[i] > 500)
                {
                    // ST_DBG( "eOkaoFr Identify Face_%d (UserID[%d] = %d, Score[%d] = %d)\n", nIndex, i, nUid[i], i, nScore[i] );
                }
            }

            if (nScore[0] > 500)
            {
                /* Index Number = nI represents the person nUid[0] */
                // printf( "      This person(Face_%d) is same as UserID:%d\n", nIndex, nUid[0]);
                sprintf(stFrResult.name[nValidNum], "UID %d", nUid[0]);
                stFrResult.bValid[nValidNum] = TRUE;
            }
            else
            {
                /* Index Number = nI is not one registered person */
                //printf( "      This person(Face_%d) is not one registered person\n", nI );
                stFrResult.name[nValidNum][0] = '\0';
                stFrResult.bValid[nValidNum] = FALSE;
            }

            nValidNum++;
        }
        //ST_DBG("oooo, end Y data process\n");

        stFrResult.u32Num = nValidNum;
        for (i = 0; i < stFrResult.u32Num; i++)
        {
            //printf("Face area:(%d-%d-%d-%d)...\n", stFrResult.pos[i].s32X, stFrResult.pos[i].s32Y,
            //    stFrResult.pos[i].s16PicW, stFrResult.pos[i].s16PicH);
            stFrResult.pos[i].s32X -= 64;
            stFrResult.pos[i].s32Y -= 64;
            stFrResult.pos[i].s16PicW += 128;
            stFrResult.pos[i].s16PicH += 128;

            if (stFrResult.pos[i].s32X < 0)
                stFrResult.pos[i].s32X = 0;
            if (stFrResult.pos[i].s32Y < 0)
                stFrResult.pos[i].s32Y = 0;
            if (stFrResult.pos[i].s32X + stFrResult.pos[i].s16PicW > pstOMRONMng->u32Width)
                stFrResult.pos[i].s16PicW = pstOMRONMng->u32Width - stFrResult.pos[i].s32X;
            if (stFrResult.pos[i].s32Y + stFrResult.pos[i].s16PicH > pstOMRONMng->u32Height)
                stFrResult.pos[i].s16PicW = pstOMRONMng->u32Height - stFrResult.pos[i].s32Y;

            stFrResult.pos[i].s32X = ALIGN_UP(stFrResult.pos[i].s32X, 2);
            stFrResult.pos[i].s32Y = ALIGN_UP(stFrResult.pos[i].s32Y, 2);
            stFrResult.pos[i].s16PicW = ALIGN_UP(stFrResult.pos[i].s16PicW, VENC_JPEG_ALIGN_W);
            stFrResult.pos[i].s16PicH = ALIGN_UP(stFrResult.pos[i].s16PicH, VENC_JPEG_ALIGN_H);
            if (stFrResult.pos[i].s32X + stFrResult.pos[i].s16PicW > pstOMRONMng->u32Width)
                stFrResult.pos[i].s16PicW -= VENC_JPEG_ALIGN_W;
            if (stFrResult.pos[i].s32Y + stFrResult.pos[i].s16PicH > pstOMRONMng->u32Height)
                stFrResult.pos[i].s16PicW -= VENC_JPEG_ALIGN_H;
            if (g_faceSnap)
            {
                #if 0
                FILE *yuvfp = NULL;
                int m;
                yuvfp = fopen("appres/face.yuv", "wb");
                FILE *yuvallfp = NULL;
                yuvallfp = fopen("appres/all.yuv", "wb");
                printf("stFrResult.u32Num = %d face area (x-y-w-h)=(%d-%d-%d-%d)...\n", stFrResult.u32Num,
                    stFrResult.pos[i].s32X, stFrResult.pos[i].s32Y, stFrResult.pos[i].s16PicW, stFrResult.pos[i].s16PicH);
                //sleep(1);
                if (yuvfp)
                {
                    for (m = 0; m < stFrResult.pos[i].s16PicH; m++)
                    {
                        memcpy(pTempYData+m*stFrResult.pos[i].s16PicW, pstOMRONMng->pYData + ((stFrResult.pos[i].s32Y + m)*pstOMRONMng->u32Width + stFrResult.pos[i].s32X),
                            stFrResult.pos[i].s16PicW);
                    }
                    fwrite(pTempYData, 1, stFrResult.pos[i].s16PicW*stFrResult.pos[i].s16PicH, yuvfp);
                    //for (m = 0; m < stFrResult.pos[i].s16PicH; m++)
                    {
                        //memcpy(pTempUVData+m*stFrResult.pos[i].s16PicW/2, pstOMRONMng->pUVData + ((stFrResult.pos[i].s32Y/2 + m)*pstOMRONMng->u32Width/2 + stFrResult.pos[i].s32X/2),
                        //    stFrResult.pos[i].s16PicW/2);
                        NV12CutImage(pstOMRONMng->pUVData, pTempUVData, pstOMRONMng->u32Width, pstOMRONMng->u32Height, stFrResult.pos[i].s32X, stFrResult.pos[i].s32Y,
                            stFrResult.pos[i].s16PicW, stFrResult.pos[i].s16PicH);
                        fwrite(pstOMRONMng->pUVData, 1, stFrResult.pos[i].s16PicW*stFrResult.pos[i].s16PicH/2, yuvfp);
                    }
                    fclose(yuvfp);
                    yuvfp = NULL;
                    if (yuvallfp)
                    {
                        fwrite(pstOMRONMng->pYData, 1, pstOMRONMng->u32Width*pstOMRONMng->u32Height, yuvallfp);
                        fwrite(pstOMRONMng->pUVData, 1, pstOMRONMng->u32Width*pstOMRONMng->u32Height >> 1, yuvallfp);
                        fclose(yuvallfp);
                        yuvallfp = NULL;
                    }
                }
                #endif
                ST_OMRON_SnapFace(pstOMRONMng->pYData, pstOMRONMng->pUVData, stFrResult.pos[i].s32X, stFrResult.pos[i].s32Y, stFrResult.pos[i].s16PicW, stFrResult.pos[i].s16PicH);
                g_faceDetect = 0;
                unsigned long msg[4];
                memset(msg, 0, 16);
                msg[0] = MSG_TYPE_DISP_DETECT_FACE;
                msg[1] = 0;
                msg[2] = 0;
                msg[3] = 0;
                g_toUI_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toUI_sem);
                g_faceSnap = 0;
            }
        }
        pthread_mutex_unlock(&pstOMRONMng->condMutex);

        ST_OMRON_DrawFDFRResult(pstOMRONMng, stFrResult);
        // showfps();
    }

    if (pTempUVData)
        free(pTempUVData);
    if (pTempYData)
        free(pTempYData);
    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }

    ST_DBG("pid=%d, %d, exit\n", getpid(), syscall(SYS_gettid));

    return NULL;
}

MI_S32 ST_OMRON_RegisterFaceID(ST_OMRON_Mng_S* pstOMRONMng)
{
    MI_S32 s32Ret = -1;
    INT32 nUserID = 0;              /* Album Data User ID */
    INT32 nDataID = 0;              /* DataID Number */

    OKAO_FR_GetRegisteredUserNum(pstOMRONMng->hAL, &nUserID);
    nDataID = 0;

    ST_DBG("nUserID=%d, nDataID=%d\n", nUserID, nDataID);
    s32Ret = ST_OMRON_ALBUMRegisterData(pstOMRONMng, nUserID, nDataID);
    if (OKAO_NORMAL != s32Ret)
    {
        ST_ERR("ST_OMRON_ALBUMRegisterData error\n");
        goto END;
    }

    ST_OMRON_ALBUMBackupAlbum(pstOMRONMng);
END:

    return 0;
}

MI_S32 ST_OMRON_StartDetect(ST_OMRON_Mng_S* pstOMRONMng)
{
    // create thread to deal result
    if (OMRON_IE_MODE_BD == pstOMRONMng->enIEMode)
    {
        OMRON_SDK_CHECK(OMCV_BD_CreateHandle(DETECTION_MODE_MOVIE, OMRON_MAX_HD_NUM), pstOMRONMng->hBody);
        OMRON_SDK_CHECK(OMCV_BD_CreateResultHandle(), pstOMRONMng->hBodyResult);
        ST_OMRON_BDSetParams(pstOMRONMng);

        pstOMRONMng->bTaskRun = TRUE;
        pthread_create(&pstOMRONMng->bdTaskThread, NULL, ST_OMRON_BD_Process, pstOMRONMng);
    }
    else if (OMRON_IE_MODE_FDFR == pstOMRONMng->enIEMode)
    {
        ST_OMRON_InitLibrary(pstOMRONMng);

        ST_OMRON_FDFRSetParams(pstOMRONMng);

        pstOMRONMng->bTaskRun = TRUE;
        pthread_create(&pstOMRONMng->bdTaskThread, NULL, ST_OMRON_FDFR_Process, pstOMRONMng);
    }

    return OMCV_NORMAL;
}

MI_S32 ST_OMRON_StopDetect(ST_OMRON_Mng_S* pstOMRONMng)
{
    if (OMRON_IE_MODE_BD == pstOMRONMng->enIEMode)
    {
        pstOMRONMng->bTaskRun = FALSE;

        // wait thread exit
        sleep(1);

        if (pstOMRONMng->hBody != NULL)
        {
            OMCV_BD_DeleteHandle(pstOMRONMng->hBody);
            pstOMRONMng->hBody = NULL;
        }

        /* Deletes Human body detection result handle */
        if (pstOMRONMng->hBodyResult != NULL)
        {
            OMCV_BD_DeleteResultHandle(pstOMRONMng->hBodyResult);
            pstOMRONMng->hBodyResult = NULL;
        }
    }   
    else if (OMRON_IE_MODE_FDFR == pstOMRONMng->enIEMode)
    {
        pstOMRONMng->bTaskRun = FALSE;

        // wait thread exit
        sleep(1);

        ST_OMRON_UnInitLibrary(pstOMRONMng);
    }

    return OMCV_NORMAL;
}
