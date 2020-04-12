/*-------------------------------------------------------------------*/
/*  Copyright(C) 2011-2012 by OMRON Corporation                      */
/*  All Rights Reserved.                                             */
/*                                                                   */
/*   This source code is the Confidential and Proprietary Property   */
/*   of OMRON Corporation.  Any unauthorized use, reproduction or    */
/*   transfer of this software is strictly prohibited.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*
   Body Detection Library Ver.2
*/
#ifndef OMCVBDAPI_H__
#define OMCVBDAPI_H__

#define OMCV_API
#include    "CommonDef.h"
#include    "OkaoDef.h"
#include    "DetectionInfo.h"
#include    "DetectorComDef.h"


#ifndef OMCV_DEF_HBODY
#define OMCV_DEF_HBODY
typedef void * HBODY;          /* Body Detection handle  */
typedef void * HBDRESULT;      /* Body Detection Result handle */
#endif /* OMCV_DEF_HBODY */

#define DETECTOR_TYPE_SOFT_BD_V2   (DET_SOFT|DET_BD|DET_V2)  /* Body Detection V2 */

/* Search Density */
#define BD_DENSITY_HIGH_190         (190)
#define BD_DENSITY_HIGH_165         (165)
#define BD_DENSITY_NORMAL           (100)
#define BD_DENSITY_LOW_75            (75)
#define BD_DENSITY_LOW_50            (50)

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************/
/* Version infomation                                     */
/**********************************************************/
/* Gets Version */
OMCV_API INT32     OMCV_BD_GetVersion(UINT8 *pucMajor, UINT8 *pucMinor);


/**********************************************************/
/* Handle Creation/Deletion                               */
/**********************************************************/
/* Creates/Deletes Detection Handle */
OMCV_API HBODY     OMCV_BD_CreateHandle(INT32 nDetectionMode, INT32 nMaxDetectionCount);
OMCV_API INT32     OMCV_BD_DeleteHandle(HBODY hBD);

/* Creates/Deletes Detection Result Handle */
OMCV_API HBDRESULT OMCV_BD_CreateResultHandle(void);
OMCV_API INT32     OMCV_BD_DeleteResultHandle(HBDRESULT hBdResult);

/**********************************************************/
/* Excecution of Detection/Tracking                       */
/******************************************************** */
/* Executes Body detection */
OMCV_API INT32     OMCV_BD_Detect(HBODY hBD, RAWIMAGE *pImage, INT32 nWidth, INT32 nHeight,
                                  volatile INT32 *pnBreakFlag, HBDRESULT hBdResult);


/**********************************************************/
/* Gets Detection Result                                  */
/**********************************************************/
/* Gets the number of detected body */
OMCV_API INT32     OMCV_BD_GetResultCount(HBDRESULT hBdResult, INT32 *pnCount);

/* Gets the detection result for each body */
OMCV_API INT32     OMCV_BD_GetResultInfo(HBDRESULT hBdResult,
        INT32 nIndex, DETECTION_INFO *psDetectionInfo);


/*************************************************************/
/* Setting functions for Still and Movie mode (COMMON)       */
/*************************************************************/
/* Sets/Gets the min and max body size */
OMCV_API INT32     OMCV_BD_SetSizeRange(HBODY hBD, INT32 nMinSize, INT32 nMaxSize);
OMCV_API INT32     OMCV_BD_GetSizeRange(HBODY hBD, INT32 *pnMinSize, INT32 *pnMaxSize);

/* Sets/Gets the direction to be detected */
OMCV_API INT32     OMCV_BD_SetAngle(HBODY hBD, UINT32 nAngle);
OMCV_API INT32     OMCV_BD_GetAngle(HBODY hBD, UINT32 *pnAngle);

/* Sets/Gets the edge mask area */
OMCV_API INT32     OMCV_BD_SetEdgeMask(HBODY hBD, RECT rcEdgeMask);
OMCV_API INT32     OMCV_BD_GetEdgeMask(HBODY hBD, RECT *prcEdgeMask);

/* Sets/Gets search density */
OMCV_API INT32     OMCV_BD_SetSearchDensity(HBODY hBD, INT32 nSearchDensity);
OMCV_API INT32     OMCV_BD_GetSearchDensity(HBODY hBD, INT32 *pnSearchDensity);

/* Sets/Gets the Detection Threshold */
OMCV_API INT32     OMCV_BD_SetThreshold(HBODY hBD, INT32 nThreshold);
OMCV_API INT32     OMCV_BD_GetThreshold(HBODY hBD, INT32 *pnThreshold);


/*************************************************************/
/* functions for Movie mode only (MOVIE MODE only)           */
/*************************************************************/
/* Reset all tracking infomation and status */
OMCV_API INT32     OMCV_BD_MV_ResetTracking(HBODY hBD);

/* Remove a tracking object by ID */
OMCV_API INT32     OMCV_BD_MV_RemoveTrackingObject(HBODY hBD, INT32 nID);

/* Lock/Unlock tracking lock by ID */
OMCV_API INT32     OMCV_BD_MV_ToggleTrackingLock(HBODY hBD, INT32 nID);

/* Sets/Gets search cycle */
OMCV_API INT32     OMCV_BD_MV_SetSearchCycle(HBODY hBD,
        INT32 nInitialBodySearchCycle, INT32 nNewBodySearchCycle);
OMCV_API INT32     OMCV_BD_MV_GetSearchCycle(HBODY hBD,
        INT32 *pnInitialBodySearchCycle, INT32 *pnNewBodySearchCycle);

/* Sets/Gets Max Retry Count and Max Hold Count */
OMCV_API INT32     OMCV_BD_MV_SetLostParam(HBODY hBD, INT32 nMaxRetryCount, INT32 nMaxHoldCount);
OMCV_API INT32     OMCV_BD_MV_GetLostParam(HBODY hBD, INT32 *pnMaxRetryCount, INT32 *pnMaxHoldCount);

/* Sets/Gets the steadiness parameters for size and positon of the detection result */
OMCV_API INT32     OMCV_BD_MV_SetSteadinessParam(HBODY hBD,
        INT32 nPosSteadinessParam, INT32 nSizeSteadinessParam);
OMCV_API INT32     OMCV_BD_MV_GetSteadinessParam(HBODY hBD,
        INT32 *pnPosSteadinessParam, INT32 *pnSizeSteadinessParam);

/* Sets/Gets tracking swap parameter */
OMCV_API INT32     OMCV_BD_MV_SetTrackingSwapParam(HBODY hBD, INT32 nTrackingSwapParam);
OMCV_API INT32     OMCV_BD_MV_GetTrackingSwapParam(HBODY hBD, INT32 *pnTrackingSwapParam);


#ifdef  __cplusplus
}
#endif

#endif /* OMCVBDAPI_H__ */
