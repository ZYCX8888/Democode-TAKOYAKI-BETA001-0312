#ifndef __SYS__H__
#define __SYS__H__
#include <string>
#include <vector>
#include <map>
#include <assert.h>

#include "tem.h"

#include "iniparser.h"

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

typedef struct stModDesc_s
{
    std::string modKeyString;
    unsigned int modId;
    unsigned int chnId;
    unsigned int devId;
}stModDesc_t;
typedef struct stModIoInfo_s
{
    std::string modKeyString;
    unsigned int portId;
    unsigned int frmRate;
}stModIoInfo_t;
typedef struct stModInputInfo_s
{
    std::string curIoKeyString;
    unsigned int curPortId;
    unsigned int curFrmRate;
    stModIoInfo_t stPrev;
}stModInputInfo_t;
typedef struct stModOutputInfo_s
{
    std::string curIoKeyString;
    unsigned int curPortId;
    unsigned int curFrmRate;
    std::vector<stModIoInfo_t> vectNext;
}stModOutputInfo_t;
typedef void (*DeliveryRecFp)(void *, unsigned int, void *);
typedef void (*DeliveryState)(void *);
typedef enum
{
    E_SENDER_STATE_START,
    E_SENDER_STATE_STOP,
    E_SENDER_STATE_MAX
}E_SENDER_STATE;

typedef enum
{
    E_SYS_MOD_DISP = E_MI_MODULE_ID_DISP,
    E_SYS_MOD_VENC = E_MI_MODULE_ID_VENC,
    E_SYS_MOD_VPE = E_MI_MODULE_ID_VPE,
    E_SYS_MOD_VIF = E_MI_MODULE_ID_VIF,
    E_SYS_MOD_DIVP = E_MI_MODULE_ID_DIVP,
    E_SYS_MOD_VDISP = E_MI_MODULE_ID_VDISP,
    E_SYS_MOD_VDEC = E_MI_MODULE_ID_VDEC,
    E_SYS_MOD_LDC = E_MI_MODULE_ID_LDC,
    E_SYS_MOD_EXT = E_MI_MODULE_ID_MAX,
    E_SYS_MOD_RTSP,
    E_SYS_MOD_DLA,
    E_SYS_MOD_FDFR,
    E_SYS_MOD_UI,
    E_SYS_MOD_IQ,
    E_SYS_MOD_FILE,
    E_SYS_MOD_MAX
}E_SYS_MOD;


class Sys
{
    public:
        typedef struct stReceiverPortDesc_s
        {
            unsigned char bStart;
            DeliveryRecFp fpRec;
            DeliveryState fpStateStart;
            DeliveryState fpStateStop;
            void *pUsrData;
        }stReceiverPortDesc_t;
        typedef struct stReceiverDesc_s
        {
            Sys * pSysClass;
            unsigned int uintPort;
            pthread_mutex_t stDeliveryMutex;
            unsigned int uintRefsCnt;
            std::map<std::string, stReceiverPortDesc_t> mapPortDesc;
        }stReceiverDesc_t;
        Sys(){}
        virtual ~Sys(){}
        static void InitSys(std::string strIniPath, std::map<std::string, unsigned int> &mapModId);
        static void DeinitSys();
        void GetModDesc(stModDesc_t &stDesc){stDesc = stModDesc;}

    protected:
        //Modules flow: Init->BindBlock->Start;Stop->UnBindBlock->Deinit
        virtual void BindBlock(stModInputInfo_t & stIn);
        virtual void UnBindBlock(stModInputInfo_t & stIn);
        virtual void Init() = 0;
        virtual void Deinit() = 0;
        virtual void Start();
        virtual void Stop();

        //Delivery api
        virtual int CreateSender(unsigned int outPortId);
        virtual int DestroySender(unsigned int outPortId);
        virtual int StartSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc);
        virtual int StopSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc);

        //Delivery internal api
        int Send(unsigned int outPortId, void *pData, unsigned int intDataSize);
        int State(unsigned int outPortId, E_SENDER_STATE eState, stReceiverPortDesc_t &stRecPortDesc);
        int CreateReceiver(unsigned int inPortId, DeliveryRecFp funcRecFp, DeliveryState funcStart, DeliveryState funcStop, void *pUsrData);
        int DestroyReceiver(unsigned int inPortId);
        int StartReceiver(unsigned int inPortId);
        int StopReceiver(unsigned int inPortId);

        static Sys* GetInstance(std::string &strKey){return connectMap[strKey];}

        //INI operation
        static int GetIniInt(std::string section, std::string key);
        static unsigned int GetIniUnsignedInt(std::string section, std::string key);
        static char *GetIniString(std::string section, std::string key);

        std::map<unsigned int, stModInputInfo_t> mapModInputInfo;
        std::map<unsigned int, stModOutputInfo_t> mapModOutputInfo;
        stModDesc_t stModDesc;
        //Delivery data
        std::map<unsigned int, stReceiverDesc_t> mapRecevier;

    private:

        //Module's tree implement
        template<class SYSCHILD>
        class SysChild
        {
            public:
                explicit SysChild(std::string &strKey)
                {
                    Sys *pClass;

                    pClass = new (std::nothrow) SYSCHILD;
                    assert(pClass);
                    pClass->SetCurInfo(strKey);
                }
                virtual ~SysChild(){};
        };
        void SetCurInfo(std::string &strKey);
        void BuildModTree();
        static void CreateConnection();
        static void DestroyConnection();
        static void Implement(std::string &strKey);
        static bool FindBlock(std::string &strKey){return connectMap.find(strKey) != connectMap.end();}
        static unsigned int FindBlockId(std::string &strKey)
        {
            std::string strModName;

            strModName = GetIniString(strKey, "MOD");

            return (connectIdMap.find(strModName) != connectIdMap.end()) ? connectIdMap[strModName] : -1;
        }
        static void *SenderState(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData);
        static void *SenderMonitor(ST_TEM_BUFFER stBuf);

        static std::map<std::string, Sys *> connectMap;
        static std::vector<Sys *> connectOrder;
        static std::map<std::string, unsigned int> connectIdMap;
        static dictionary *m_pstDict;

};

#endif
