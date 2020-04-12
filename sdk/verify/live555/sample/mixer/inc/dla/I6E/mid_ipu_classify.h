#ifndef _MID_IPU_CLASSIFY_H_
#define _MID_IPU_CLASSIFY_H_

#include "mid_ipu_interface.h"

#define LABEL_CLASS_COUNT       (1200)
#define LABEL_NAME_MAX_SIZE     (60)

class CMidIPUClassify : public CMidIPUInterface
{
public:
    CMidIPUClassify(IPU_InitInfo_S &stInitInfo);
    ~CMidIPUClassify();

    int InitResource();
    int ReleaseResource();
    void DealDataProcess();
	MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop);

private:
    int InitStreamSource();
    int ReleaseStreamSource();
    int DoClassify(MI_SYS_BufInfo_t* pstBufInfo);
    MI_BOOL GetTopN(float aData[], int dataSize, int aResult[], int TopN);

    void PrintResult(MI_IPU_TensorVector_t* pstOutputTensorVector);

private:
    MI_VPE_CHANNEL          m_vpeChn;
    MI_VPE_PORT             m_vpePort;
    MI_DIVP_CHN             m_divpChn;
    MI_IPU_CHN              m_ipuChn;
    MI_U32                  m_u32OutBufDepth;
    MI_U32                  m_u32InBufDepth;
    MI_S32                  m_s32Fd;
    MI_IPU_SubNet_InputOutputDesc_t     m_stNPUDesc;
    char                    m_szLabelName[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
};

#endif // _MID_IPU_CLASSIFY_H_

