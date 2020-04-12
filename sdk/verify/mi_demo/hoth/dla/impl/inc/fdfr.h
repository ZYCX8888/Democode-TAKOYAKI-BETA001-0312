#ifndef __FDFR_H__
#define __FDFR_H__

#include "mi_sys.h"

#include <map>
#include <string>
#include <vector>

#include "tem.h"

#include "sys.h"

typedef struct stFdfrInputInfo_s
{
    //0: fd 1: fr
    unsigned int intInPort;
    unsigned int intPreMod;
    unsigned int intPreChn;
    unsigned int intPreOutPort;
    unsigned int intPreDev;
}stFdfrInputInfo_t;

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

        void DoFd(MI_SYS_BufInfo_t &stBufInfo);
        void DoFr(MI_SYS_BufInfo_t &stBufInfo);
        void SendResult();

        static void *DataMonitor(ST_TEM_BUFFER stBuf);
        static void DataReceiver(void *pData, unsigned int dataSize, void *pUsrData);
        static void * SenderSignal(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData);
        static void * SenderSignalEnd(ST_TEM_BUFFER stBuf);

        std::vector<stFdfrInputInfo_t> vectFdFrInput;
        std::vector<stFaceInfo_t> vectResult;
};
#endif

