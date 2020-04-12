#ifndef __FDFR_H__
#define __FDFR_H__

#include "mi_sys.h"

#include <map>
#include <string>
#include <vector>

#include "tem.h"
#include "sys.h"

#include "mi_ipu.h"

#include "face_recognize.h"

#define  FDAFR_FUNC_INFO(fmt, args...)           //do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)
#define  TIME_FUNC_INFO(fmt, args...)           //do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)


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

typedef struct stFdaOutputDataDesr_s
{
    float * pfBBox;
    float * pfLms;
    float * pfScores;
    float * pDectCount;

}stFdaOutputDataDesc_t;

typedef struct stFdfrInputInfo_s
{
    //0: fd 1: fr
    unsigned int intInPort;
    unsigned int intPreMod;
    unsigned int intPreChn;
    unsigned int intPreOutPort;
    unsigned int intPreDev;
}stFdfrInputInfo_t;


typedef struct stCountName_s
{
    unsigned int Count;
    std::string Name;
} stCountName_t;


class Fdfr: public Sys
{
    public:
        Fdfr();
        virtual ~Fdfr();
    private:
        virtual void Init();
        virtual void Deinit();
        virtual void Start();
        virtual void Stop();
        virtual void BindBlock(stModInputInfo_t & stIn);
        virtual void UnBindBlock(stModInputInfo_t & stIn);
        virtual int CreateSender(unsigned int outPortId);
        virtual int DestroySender(unsigned int outPortId);
        virtual int StartSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc);
        virtual int StopSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc);

        int DoFd(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);
        void DoFr(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);
        void SendResult();
        stCountName_t SearchNameByID(std::map<int, stCountName_t> &mapLastIDName, int id);
        std::vector<DetBBox> &SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> & detboxes);
        void DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData);
        void DoRecognition(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *inputVector, MI_IPU_TensorVector_t *outputVector);
        void ReadFaceDB();

        int SetPrivatePool(int Channel, int HeapSize);
        int InferenceFDANetwork(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine);
        std::string getFaceNamebyFeature(void* feat1);

        MI_IPU_CHN FdaChn, FrChn;
        MI_IPU_SubNet_InputOutputDesc_t FdaDesc, FrDesc;
        MI_IPU_DevAttr_t stDevAttr;
        MI_IPUChnAttr_t FdaChnAttr, FrChnAttr;
        float Threshold;

        static void *DataMonitor(ST_TEM_BUFFER stBuf);
        static void DataReceiver(void *pData, unsigned int dataSize, void *pUsrData);
        static void * SenderSignal(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData);
        static void * SenderSignalEnd(ST_TEM_BUFFER stBuf);

        std::vector<stFdfrInputInfo_t> vectFdFrInput;
        std::vector<stFaceInfo_t> vectResult;
        std::map<int, stCountName_t> mapCurIDName;
        std::vector<Track> vecTrackBBoxs;
        std::vector<DetBBox> vecDetBBoxs;
        stFdaOutputDataDesc_t stFdaOutData;
        int intImgWidth;
        int intImgHeight;
        float fScoreThreshold;
        IOUTracker BBoxTracker;
        FaceRecognizeUtils faceRecognizer;
        FaceDatabase face_db;
        int nnncrop = 0;
        MI_SYS_PixelFormat_e   fd_ePixelFormat;
        MI_SYS_PixelFormat_e   fr_ePixelFormat;
};
#endif

