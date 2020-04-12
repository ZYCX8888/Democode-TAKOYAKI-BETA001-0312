#ifndef _MID_IPU_DETECT_H_
#define _MID_IPU_DETECT_H_

#include "mid_ipu_interface.h"
#include "mid_dla.h"

#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#define LABEL_CLASS_COUNT       (1200)
#define LABEL_NAME_MAX_SIZE     (60)

using namespace std;

struct DetectionBBoxInfo
{
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int   classID;
};

class CMidIPUDetect : public CMidIPUInterface
{
public:
    CMidIPUDetect(IPU_InitInfo_S &stInitInfo);
    virtual ~CMidIPUDetect();

    int InitResource();
    int ReleaseResource();
    void DealDataProcess();

private:
    int InitStreamSource();
    int ReleaseStreamSource();
    int DoDetect(MI_SYS_BufInfo_t* pstBufInfo);
    MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop);
    int ScaleToModelSize(MI_SYS_BufInfo_t* pstBufInfo, MI_IPU_TensorVector_t* pstInputTensorVector);
    std::vector<DetectionBBoxInfo > GetDetections(float *pfBBox, float *pfClass, float *pfScore, float *pfDetect);
    void PrintResult(MI_IPU_TensorVector_t* pstOutputTensorVector);
private:
    MI_VPE_CHANNEL          m_vpeChn;
    MI_VPE_PORT             m_vpePort;
    MI_DIVP_CHN             m_divpChn;
    MI_IPU_CHN              m_ipuChn;
    MI_U32                  m_u32InBufDepth;
    MI_U32                  m_u32OutBufDepth;
    MI_U32                  m_u32Width;
    MI_U32                  m_u32Height;
    MI_S32                  m_s32Fd;
    MI_IPU_SubNet_InputOutputDesc_t     m_stNPUDesc;
    char                    m_szLabelName[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
    MI_S32                  m_s32LabelCount;
	std::vector<Track>      m_DetectTrackBBoxs;
	IOUTracker				m_DetectBBoxTracker;
   
};

#endif // _MID_IPU_DETECT_H_

