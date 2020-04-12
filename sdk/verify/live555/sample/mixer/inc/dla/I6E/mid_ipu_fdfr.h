#ifndef _MID_IPU_FDFR_H_
#define _MID_IPU_FDFR_H_

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <map>
#include <dirent.h>

#include "mid_ipu_interface.h"
#include "FaceDatabase.h"
#include "mid_dla.h"
#define INNER_MOST_ALIGNMENT 8
class CMidIPUFdFr : public CMidIPUInterface
{
public:
    CMidIPUFdFr(IPU_InitInfo_S &stInitInfo);
    ~CMidIPUFdFr();

public:
    int InitResource();
    int ReleaseResource();
    void DealDataProcess();
 	MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop);
	MI_S32 GetIeParam(IeParamInfo &tmp);
private:
    int InitStreamSource(Model_Info_S &stModelInfo);
    int ReleaseStreamSource(Model_Info_S &stModelInfo);
    void LoadFaceDb();
    int ScaleToModelSize(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t* pstInputTensorVector,
                            Model_Info_S &stModelInfo);
    int DoFd(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);
    int DoFr(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);
    void DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData);
    std::vector<DetBBox> SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> &detboxes);
    MI_S8 DoRecognition(const MI_SYS_BufInfo_t &stBufInfo,
                        MI_IPU_TensorVector_t *inputVector,
                        MI_IPU_TensorVector_t *outputVector);

    stCountName_t SearchNameByID(std::map<int, stCountName_t> &mapLastIDName, int id);
    std::string getFaceNamebyFeature(void* feat1);

    void PrintResult();

private:
    FaceDatabase                m_faceDB;
    FaceRecognizeUtils          m_faceRecognizer;

    Model_Info_S                m_stFdModel;
    Model_Info_S                m_stFrModel;
    MI_S32                      m_s32MaxFd;
    MI_U8                       m_dataReady;

    std::vector<DetBBox>        m_vecDetBBoxs;
    std::map<int, stCountName_t> m_mapCurIDName;
    std::vector<Track>          m_vecTrackBBoxs;
    IOUTracker                  m_BBoxTracker;
    std::vector<stFaceInfo_t>   m_vectResult;
};

#endif // _MID_IPU_FDFR_H_

