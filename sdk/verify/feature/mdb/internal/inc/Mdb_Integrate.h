/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef MDB_INTEGRATE
#define MDB_INTEGRATE
#include "Mdb_Base.h"
#include "Live555RTSPServer.hh"


class Mdb_Integrate : public Mdb_Base
{
    public:
        Mdb_Integrate();
        virtual ~Mdb_Integrate();
        virtual void ShowWelcome(std::string &strOut);
    private:
        void VifInit(std::vector<std::string> &inStrings, std::string &strOut);
        void VifDeinit(std::vector<std::string> &inStrings, std::string &strOut);

        void VpeInit(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeDeinit(std::vector<std::string> &inStrings, std::string &strOut);
        void YuvWriteFile(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeCreatePort(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeDestroyPort(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeSetRotation(std::vector<std::string> &inStrings, std::string &strOut);

        void DivpInit(std::vector<std::string> &inStrings, std::string &strOut);
        void DivpCreatePort(std::vector<std::string> &inStrings, std::string &strOut);
        void DivpStart(std::vector<std::string> &inStrings, std::string &strOut);
        void DivpStop(std::vector<std::string> &inStrings, std::string &strOut);
        void DivpDeInit(std::vector<std::string> &inStrings, std::string &strOut);
#if 0
        void LdcCreat(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcInit(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcSetBin(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcCreatePort(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcStart(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcStop(std::vector<std::string> &inStrings, std::string &strOut);
        void LdcDeInit(std::vector<std::string> &inStrings, std::string &strOut);
#endif
        void VencInit(std::vector<std::string> &inStrings, std::string &strOut);
        void VencDeinit(std::vector<std::string> &inStrings, std::string &strOut);
        void VencWriteFile(std::vector<std::string> &inStrings, std::string &strOut);
        void VencInjectFrame(std::vector<std::string> &inStrings, std::string &strOut);
        void VencStart(std::vector<std::string> &inStrings, std::string &strOut);
        void VencStop(std::vector<std::string> &inStrings, std::string &strOut);
        void VencSetInput(std::vector<std::string> &inStrings, std::string &strOut);

        void IqOpenServer(std::vector<std::string> &inStrings, std::string &strOut);
        void IqCloseServer(std::vector<std::string> &inStrings, std::string &strOut);
        void IqLoadApiBin(std::vector<std::string> &inStrings, std::string &strOut);

        void SetBindInfo(std::vector<std::string> &inStrings, std::string &strOut);
        void SetBindInfo2(std::vector<std::string> &inStrings, std::string &strOut);
        void SetDepth(std::vector<std::string> &inStrings, std::string &strOut);
        void SetUnBindInfo(std::vector<std::string> &inStrings, std::string &strOut);
        void SetMmaConf(std::vector<std::string> &inStrings, std::string &strOut);
        void ConfigPrivatePool(std::vector<std::string> &inStrings, std::string &strOut);

        void RtspServerStart(std::vector<std::string> &inStrings, std::string &strOut);
        void RtspServerStop(std::vector<std::string> &inStrings, std::string &strOut);

        void ExportFile(std::vector<std::string> &inStrings, std::string &strOut);

        void FbInit(std::vector<std::string> &inStrings, std::string &strOut);
        void FbFillRect(std::vector<std::string> &inStrings, std::string &strOut);
        void FbInitMouse(std::vector<std::string> &inStrings, std::string &strOut);
        void FbSetMouse(std::vector<std::string> &inStrings, std::string &strOut);
        void FbShow(std::vector<std::string> &inStrings, std::string &strOut);

        void FbFillRectTest(std::vector<std::string> &inStrings, std::string &strOut);
        void FbSetColorKey(std::vector<std::string> &inStrings, std::string &strOut);
        void FbGetColorKey(std::vector<std::string> &inStrings, std::string &strOut);
        static void* OpenStream(char const* szStreamName, void* arg);
        static int VideoReadStream(void* handle, unsigned char* ucpBuf, int BufLen, struct timeval *p_Timestamp, void* arg);
        static int CloseStream(void* handle, void* arg);

        static std::map<std::string, Live555RTSPServer*> mapRTSPServer;
        static Live555RTSPServer *pRTSPServer;
};
#endif
