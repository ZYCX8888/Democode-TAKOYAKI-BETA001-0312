
#ifndef __MID_DLA_H__
#define __MID_DLA_H__

#ifdef ALIGN_UP
#undef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#else
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif

#define ST_NOP(fmt, args...)
#define ST_DBG(fmt, args...) \
    do { \
        printf(COLOR_GREEN "[DBG]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_WARN(fmt, args...) \
    do { \
        printf(COLOR_YELLOW "[WARN]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_INFO(fmt, args...) \
    do { \
        printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_ERR(fmt, args...) \
    do { \
        printf(COLOR_RED "[ERR]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)
#define MAX_DLA_RECT_NUMBER       20

typedef struct ST_Sys_Rect_s
{
    MI_U32 u32X;
    MI_U32 u32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} ST_Rect_T;

typedef struct
{
    MI_U32 u32X;
    MI_U32 u32Y;
} ST_Point_T;

typedef struct
{
    ST_Rect_T rect;
    char szObjName[64];
}ST_DlaRectInfo_T;

typedef struct
{
    char szModelFile[64];
    char szLabelFile[64];

    MI_DIVP_CHN divpChn;
    MI_SYS_PixelFormat_e enPixelFormat;
} ST_DLAInfo_T;


#if TARGET_CHIP_I6E

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <map>
#include <dirent.h>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "iout.h"
#include "face_recognize.h"
#include "mi_ipu.h"
#include "mi_ipu_datatype.h"
#include "FaceDatabase.h"
#include "mid_ipu_interface.h"

typedef struct
{
    MI_VPE_CHANNEL          vpeChn;
    MI_VPE_PORT             vpePort;
    MI_DIVP_CHN             divpChn;
    MI_IPU_CHN              ipuChn;
    MI_U32                  u32InBufDepth;
    MI_U32                  u32OutBufDepth;
    MI_S32                  s32Fd;
    MI_IPU_SubNet_InputOutputDesc_t     stNPUDesc;
    char                    *pModelFile;
    MI_U32                  u32Width;
    MI_U32                  u32Height;

    MI_SYS_BufInfo_t        stBufInfo;
    MI_SYS_BUF_HANDLE       stBufHandle;
} Model_Info_S;

/* IPU network types*/
typedef enum {
    E_NET_TYPE_INVALID = 0,
    E_NET_TYPE_CLASSIFICATION,
    E_NET_TYPE_DETECTION,
    E_NET_TYPE_RNN,
    E_NET_TYPE_DECONVOLUTION,
    E_NET_TYPE_DILATED_CONV,
    E_NET_TYPE_FACIAL_DETECTION,
    E_NET_TYPE_UNKNOWN,
    E_NET_TYPE_MAX,
} SGS_NET_TYPE_e;

typedef struct stFaceInfo_s
{
    char faceName[64];
    unsigned short xPos;
    unsigned short yPos;
    unsigned short faceW;
    unsigned short faceH;
    unsigned short winWid;
    unsigned short winHei;
}stFaceInfo_t;

typedef struct stCountName_s
{
    unsigned int Count;
    std::string Name;
} stCountName_t;

typedef struct stFdaOutputDataDesr_s
{
    float * pfBBox;
    float * pfLms;
    float * pfScores;
    float * pDectCount;

}stFdaOutputDataDesc_t;

#define DLA_VIDEO_NUM_FOR_I6E  2
BOOL GetDlaState(void);

class CDlaManager
{
public:
    static CDlaManager * instance();

    CDlaManager();
    ~CDlaManager();

    MI_S32 Init();
    MI_S32 UnInit();

    MI_S32 Start();
    MI_S32 Stop();
    MI_S32 SetInitInfo(const IPU_InitInfo_S *pstInitInfo);
    void Monitor1();
    CMidIPUInterface*       m_IPUInterface;
private:
    pthread_t               m_pthread;
    MI_BOOL                 m_bRun;
    IPU_InitInfo_S          m_stInitInfo;
    MI_BOOL                 m_bInit;
};

#define g_CDlaManager (CDlaManager::instance())

#endif

#endif /* __MID_DLA_H__ */

