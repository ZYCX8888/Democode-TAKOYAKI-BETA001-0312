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
#ifndef MDB_VPE
#define MDB_VPE
#include "Mdb_Base.h"

class Mdb_Vpe : public Mdb_Base
{
    public:
        Mdb_Vpe();
        virtual ~Mdb_Vpe();
    private:
        void CreateChannel(std::vector<std::string> &inStrings, std::string &strOut);
        void SetCrop(std::vector<std::string> &inStrings, std::string &strOut);
        void CreatePort(std::vector<std::string> &inStrings, std::string &strOut);
        void StartChannel(std::vector<std::string> &inStrings, std::string &strOut);
        void StopPort(std::vector<std::string> &inStrings, std::string &strOut);
        void StopChannel(std::vector<std::string> &inStrings, std::string &strOut);
        void DestroyChannel(std::vector<std::string> &inStrings, std::string &strOut);
        void WriteVpeFile(std::vector<std::string> &inStrings, std::string &strOut);
};
#endif
