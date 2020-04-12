#ifndef _ST_OMRON_H_
#define _ST_OMRON_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "st_common.h"
#include "linux_list.h"

//omron header
#include "OkaoCoDef.h"
#include "OkaoAPI.h"
#include "CommonDef.h"
#include "OkaoCoStatus.h"
#include "DetectionInfo.h"
#include "DetectorComDef.h"

#include "OkaoCoAPI.h"

#include "OkaoDtAPI.h"
#include "OkaoPtAPI.h"
#include "OkaoFrAPI.h"
#include "OmcvBdAPI.h"

#include "mi_sys.h"

#define OMRON_ALBUM_FILE        "album.dat"
#define OMRON_FEATURE_FILE      "feature.dat"

#define OMRON_MAX_FACE_NUM		(10)
#define OMRON_MAX_HD_NUM		(35)
#define RTSP_LISTEN_PORT        (554)
#define EDGEMASK_PIXEL 			(-1)
#define MAX_OSD_NUM				4


#ifndef OMRON_SDK_CHECK
#define OMRON_SDK_CHECK(_func_, value)\
    do{ \
        void *p = NULL; \
        p = _func_; \
        if (p == NULL)\
        { \
            printf("[%s %d]exec function failed\n", __func__, __LINE__); \
            return -1; \
        } \
        else \
        { \
            printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__); \
            value = p; \
        } \
    } while(0)
#endif //OMRON_SDK_CHECK

#ifndef OMRON_SDK_CHECK_V
#define OMRON_SDK_CHECK_V(_func_)\
    do{ \
        MI_S32 s32Ret = OMCV_NORMAL; \
        s32Ret = _func_; \
        if (s32Ret != OMCV_NORMAL)\
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__); \
        } \
    } while(0)
#endif //OMRON_SDK_CHECK_V

typedef struct
{
    MI_ModuleId_e enModule;
    MI_U32 u32Dev;
    MI_U32 u32Chn;
    MI_U32 u32Port;
} ST_OMRON_Source_S;

typedef struct
{
    MI_RGN_HANDLE hHandle;
    pthread_mutex_t mutex;

    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_S32 s32InitFlag;
    struct list_head dirtyAreaList;
} ST_OMRON_Osd_S;

typedef enum
{
    OMRON_IE_MODE_BD = 0x01,
    OMRON_IE_MODE_FDFR = 0x02,
    OMRON_IE_MODE_ALBUM = 0x04,
} ST_OMRON_IE_MODE_E;

typedef struct
{
    ST_Rect_T stRect;
    struct list_head list;
} ST_OMRON_Area_Node_S;

typedef struct
{
    MI_U32 u32Num;                          /* Number of Face Detected */
    ST_Rect_T pos[OMRON_MAX_FACE_NUM];      /* Face Postion */
    char name[OMRON_MAX_FACE_NUM][32];      /* Face ID */
    MI_BOOL bValid[OMRON_MAX_FACE_NUM];
} ST_OMRON_FR_Result_S;

typedef struct
{
    MI_U32 u32Num;                          /* Number of Face Detected */
    ST_Rect_T pos[OMRON_MAX_HD_NUM];        /* human Postion */
} ST_OMRON_HD_Result_S;

typedef struct
{
    MI_U32 u32Width;                // input YUV width
    MI_U32 u32Height;
    ST_OMRON_IE_MODE_E enIEMode;    //

    pthread_mutex_t dataMutex;      // Y data mutex
    unsigned char* pYData;          // Y data
    unsigned char* pUVData;
    MI_U32 u32MaxSize;              // buf max size

    ST_OMRON_Source_S stSource;     // which module to get data

    // can do many task at the same time?
    // human body detect
    HBODY hBody;
    HBDRESULT hBodyResult;

    // face detect
    HCOMMON hCo;
    HDETECTION hDT;         /* Face Detection Handle */
    HDTRESULT hDtResult;    /* Face Detection Result Handle */
    HPOINTER hPT;           /* Facial Parts Detection Handle */
    HPTRESULT hPtResult;    /* Facial Parts Detection Result Handle */
    HALBUM hAL;             /* Album Handle */
    HFEATURE hFD;           /* Feature Handle */

    MI_BOOL bAlbumProcess;  //

    MI_BOOL bTaskRun;
    pthread_t bdTaskThread;

    MI_BOOL bGetYUVRun;
    pthread_t getYUVThread;
    MI_BOOL bSaveYData;

    pthread_mutex_t condMutex;
    pthread_cond_t dataCond;
} ST_OMRON_Mng_S;

typedef struct
{
    MI_BOOL         bExit;
    MI_U32          u32CaseIndex;

    MI_BOOL         bSubExit;
    MI_U32          u32CurSubCaseIndex;

    MI_U32          u32LastSubCaseIndex;

    // update time osd
    MI_BOOL bOsdTimeRun;
    pthread_t osdTimeThread;
} ST_Run_Para_S;

MI_S32 ST_OMRON_InitLibrary(ST_OMRON_Mng_S* pstOMRONMng);
MI_S32 ST_OMRON_StartDetect(ST_OMRON_Mng_S* pstOMRONMng);
MI_S32 ST_OMRON_RegisterFaceID(ST_OMRON_Mng_S* pstOMRONMng);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_ST_OMRON_H_
