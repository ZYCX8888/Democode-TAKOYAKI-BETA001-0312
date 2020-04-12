#ifndef _MID_IPU_HC_H_
#define _MID_IPU_HC_H_

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <map>
#include <dirent.h>

#include "mid_ipu_interface.h"
#include "FaceDatabase.h"
#include "mid_dla.h"
#include "MyMutex.h"

#define SETUPUNIT   (2)
#define SETUPCOUNT (10)
#define WidthSetup (16)
#define  HeightSetup (8)
#define  XSetup (16)
#define  YSetup (8)
#define  WAVE_FLITER_QP 16
#define INNER_MOST_ALIGNMENT (8)
#define fdfr_ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

typedef struct _hcRect_t_
{
    MI_S16 x;
    MI_S16 y;
    MI_U16 width;
    MI_U16 height;
    MI_U64 pts;

}HcRect_t;

#define HcRectAlgoArray 11
class CHcRectAlgo
{
public:
    CHcRectAlgo();
    ~CHcRectAlgo();

    MI_U16 Init();
    MI_U16 UnInit();

    MI_U16 PushRect(const HcRect_t &mHcRectSize);
    MI_U16 GetAlgoRect(HcRect_t &mHcRectSize);

    MI_U16 m_CurXWThreshold;
    MI_U16 m_CurYHThreshold;
private:
    static volatile MI_U16 rCount;
    static volatile MI_U16 wCount;

    static HcRect_t * m_dataRects;

    MyMutex  HcRectLock;
};

class CMidIPUHc: public CMidIPUInterface
{
public:
    CMidIPUHc(IPU_InitInfo_S &stInitInfo);
    ~CMidIPUHc();

public:
    int InitResource();
    int ReleaseResource();
    void DealDataProcess();
	MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop);
    MI_S8 DataWaveFliter(HcRect_t &tRectSize,MI_U8 dataLen,MI_U8 fliterType);
    void Hc2Vpe_Task1();
    int GetVpeCropRect(MI_SYS_WindowRect_t &stVpeCropRect);

private:
    int InitStreamSource(Model_Info_S &stModelInfo);
    int ReleaseStreamSource(Model_Info_S &stModelInfo);

    int DoHc(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);

    void UpdateResult(const HcRect_t &mHcRectSize);
    int CalcHcRectSize(const MI_IPU_TensorVector_t &Vector4Affine, HcRect_t &HcRectSize);
    int GetTargetSize( HcRect_t &mHcRectSize);
    std::vector<DetBBox> SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> &detboxes);
    void DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData);
    void DoCalcRectPoint(const HcRect_t &mTargetRectSize, \
                                     const MI_SYS_WindowRect_t &mCurRectSize, \
                                     MI_SYS_WindowRect_t &mResultRect);
private:

    Model_Info_S                m_stHcModel;
    MI_S32                      m_s32MaxFd;
    MI_U8                       m_dataReady;
    //std::vector<DetBBox>  m_detboxes;
   // std::vector<HcRect_t>  m_dataRects;
   CHcRectAlgo m_dataRects;
    IOUTracker                  m_BBoxTracker;
    std::vector<Track>          m_HcTrackBBoxs;

    MI_SYS_WindowRect_t m_stVpeCropWinGet;
    MI_U16 m_MaxIndex;
    MI_U16 m_MinIndex;
    MI_U16 m_CurIndex;

    pthread_t g_stPthreadHc2Vpe;
    MI_BOOL m_bThread;

    HcRect_t mCurHcPosition;
};

#endif // _MID_IPU_FDFR_H_


