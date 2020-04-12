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
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "Mdb_Base.h"
#include "Mdb_Region.h"
#include "Mdb_Integrate.h"
#include "Mdb_Vif.h"
#include "Mdb_Vpe.h"
#include "Mdb_Sys.h"
#include "Mdb_Divp.h"


Mdb_Base *Mdb_Base::pInstance = NULL;
std::map<std::string, std::string> Mdb_Base::mapModule;

template<class MDBCHILD>
Mdb_Base::Mdb_Setup<MDBCHILD>::Mdb_Setup(const char *pModuleName)
{
    pInstance = new (std::nothrow) MDBCHILD;
    assert(pInstance);
    pInstance->strMdbBaseSubName = pModuleName;
}

Mdb_Base::Mdb_Base()
{
    bExit = 0;
    pthread_mutex_init(&mutexProcess, NULL);

    //START_PROCESS;
}
Mdb_Base::~Mdb_Base()
{
    printf("%s\n", __FUNCTION__);
    //STOP_PROCESS;
    pInstance = NULL;
}

void Mdb_Base::PrepareModule(std::string &strOut)
{
    PREPARE_MODULE(MDB_MODULE_RGN, MDB_MODULE_RGN_NAME);
    PREPARE_MODULE(MDB_MODULE_INTEGRATE, MDB_MODULE_INTEGRATE_NAME);
    PREPARE_MODULE(MDB_MODULE_VIF, MDB_MODULE_VIF_NAME);
    PREPARE_MODULE(MDB_MODULE_VPE, MDB_MODULE_VPE_NAME);
    PREPARE_MODULE(MDB_MODULE_SYS, MDB_MODULE_SYS_NAME);
    PREPARE_MODULE(MDB_MODULE_DIVP, MDB_MODULE_DIVP_NAME);

    return;
}
void Mdb_Base::GetName(std::string &strOut)
{
    strOut = strMdbBaseSubName;
    return;
}
unsigned int Mdb_Base::GetTime(void)
{
    struct timespec ts;
    unsigned int ms;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ms = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    if(ms == 0)
    {
        ms = 1;
    }
    return ms;
}
unsigned int Mdb_Base::GetTimeDiff(unsigned int intOldTime)
{
    return GetTime() - intOldTime;
}
void Mdb_Base::DumpCmd(std::string &strOut)
{
    std::map<std::string, MDB_Cmd_t>::iterator it;

    for (it = mapMdbBaseCmd.begin(); it != mapMdbBaseCmd.end(); ++it)
    {
        strOut += it->first;
        strOut += "/";
    }
}
void Mdb_Base::SetPara(std::string &strCmd, std::vector<std::string> &strInStrings)
{
    strMdbBaseSubCmd = strCmd;
    strMdbBaseInStrings = strInStrings;
}
void Mdb_Base::Create(std::string &strModuleName)
{
    switch(Mdb_Base::Atoi(strModuleName))
    {
        case MDB_MODULE_RGN:
        {
            Mdb_Setup<Mdb_Region> Mdb_Region(MDB_MODULE_RGN_NAME);
        }
        break;
        case MDB_MODULE_INTEGRATE:
        {
            Mdb_Setup<Mdb_Integrate> Mdb_Integrate(MDB_MODULE_INTEGRATE_NAME);
        }
        break;
        case MDB_MODULE_VIF:
        {
            Mdb_Setup<Mdb_Vif> Mdb_Vif(MDB_MODULE_VIF_NAME);
        }
        break;
        case MDB_MODULE_VPE:
        {
            Mdb_Setup<Mdb_Vpe> Mdb_Vpe(MDB_MODULE_VPE_NAME);
        }
        break;
        case MDB_MODULE_SYS:
        {
            Mdb_Setup<Mdb_Sys> Mdb_Sys(MDB_MODULE_SYS_NAME);
        }
        break;
        case MDB_MODULE_DIVP:
        {
            Mdb_Setup<Mdb_Divp> Mdb_Divp(MDB_MODULE_DIVP_NAME);
        }
        break;
        default:
        {
            if (mapModule.find(strModuleName) != mapModule.end())
            {
                printf("module find end \n");

                Create(mapModule[strModuleName]);
            }
        }
        break;
    }
}
unsigned int Mdb_Base::Atoi(std::string &strOut)
{
    const char * pStr = strOut.c_str();
    int intStrLen = strOut.length();
    unsigned short bUseHex = 0;
    unsigned int intRetNumber = 0;

    if (pStr == NULL)
    {
        return 0xFFFFFFFF;
    }

    if (intStrLen > 2)
    {
        if (pStr[0] == '0' &&(pStr[1] == 'X' || pStr[1] == 'x'))
        {
            bUseHex = 1;
            pStr += 2;
        }
    }
    if (bUseHex == 1)
    {
        for (int i = 0; i < intStrLen - 2; i++)
        {
            if ((pStr[i] > '9' || pStr[i] < '0')    \
                && (pStr[i] > 'f' || pStr[i] < 'a') \
                && (pStr[i] > 'F' || pStr[i] < 'A'))
            {
                return 0xFFFFFFFF;
            }
        }
        sscanf(pStr, "%x", &intRetNumber);
    }
    else
    {
        for (int i = 0; i < intStrLen; i++)
        {
            if (pStr[i] > '9' || pStr[i] < '0')
            {
                return 0xFFFFFFFF;
            }
        }
        intRetNumber =  atoi(strOut.c_str());
    }
    return intRetNumber;
}
void Mdb_Base::Print(std::string &strOut, std::string strContainer, PRINT_COLOR enColor, PRINT_MODE enMode)
{

    if (enColor == PRINT_COLOR_NORMAL && enMode == PRINT_MODE_NORMAL)
    {
        strOut += strContainer;
    }
    else
    {
        std::stringstream sstr;

        sstr<< "\033[" << enMode << ';' << enColor << 'm' << strContainer.c_str() << "\033[0m";
        strOut += sstr.str();
    }
}

