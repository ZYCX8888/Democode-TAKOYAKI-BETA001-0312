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
#ifndef MDB_SYS
#define MDB_SYS
#include "Mdb_Base.h"

class Mdb_Sys : public Mdb_Base
{
    public:
        Mdb_Sys();
        virtual ~Mdb_Sys();
    private:
        void SetBindInfo(std::vector<std::string> &inStrings, std::string &strOut);
        void SetDepth(std::vector<std::string> &inStrings, std::string &strOut);
        void SetUnBindInfo(std::vector<std::string> &inStrings, std::string &strOut);
};

#endif
