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
#ifndef MDB_BASE_H
#define MDB_BASE_H
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <map>

#include <iostream>
#include <sstream>

typedef enum
{
    MDB_MODULE_RGN = 1,
    MDB_MODULE_INTEGRATE = 2,
    MDB_MODULE_VIF = 3,
    MDB_MODULE_VPE = 4,
    MDB_MODULE_SYS = 5,
    MDB_MODULE_DIVP =6,
    MDB_MODULE_MAX
}MDB_MODULE;

#define MDB_MODULE_RGN_NAME "region"
#define MDB_MODULE_INTEGRATE_NAME "integrate"
#define MDB_MODULE_VIF_NAME "vif"
#define MDB_MODULE_VPE_NAME "vpe"
#define MDB_MODULE_SYS_NAME "sys"
#define MDB_MODULE_DIVP_NAME "divp"


#ifndef ASSERT
#define ASSERT(_x_)                                                                         \
    do  {                                                                                   \
        if ( ! ( _x_ ) )                                                                    \
        {                                                                                   \
            printf("ASSERT FAIL: %s %s %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__);     \
            abort();                                                                        \
        }                                                                                   \
    } while (0)
#endif
#ifndef PTH_RET_CHK
#define PTH_RET_CHK(_pf_) \
    ({ \
        int r = _pf_; \
        if ((r != 0) && (r != ETIMEDOUT)) \
            printf("[PTHREAD] %s: %d: %s: %s\n", __FILE__, __LINE__, #_pf_, strerror(r)); \
        r; \
    })
#endif

typedef enum
{
    PRINT_COLOR_NORMAL = 0,
    PRINT_COLOR_BLACK = 30,
    PRINT_COLOR_RED ,
    PRINT_COLOR_GREEN,
    PRINT_COLOR_YELLOW,
    PRINT_COLOR_BLUE,
    PRINT_COLOR_FUNCHSIN,
    PRINT_COLOR_CYAN,
    PRINT_COLOR_WHITE
}PRINT_COLOR;
typedef enum
{
    PRINT_MODE_NORMAL = 0,
    PRINT_MODE_HIGHTLIGHT = 1,
    PRINT_MODE_UNDERLINE = 4,
    PRINT_MODE_FLICK = 5,
    PRINT_MODE_INVERT = 7,
}PRINT_MODE;

#define PREPARE_MODULE(module_enum, module_name) do{ \
    std::stringstream sstr; \
    sstr << '[' << module_enum << "] " << module_name << std::endl; \
    Print(strOut, sstr.str(), PRINT_COLOR_YELLOW, PRINT_MODE_HIGHTLIGHT);    \
    mapModule[module_name] = module_enum + '0';    \
}while (0);
#define PREPARE_COMMAND(cmdStr, fpFunction, max_para)    do{ \
    mapMdbBaseCmd[cmdStr].fpFunc = (SUBCMD_FUNC)(fpFunction);    \
    mapMdbBaseCmd[cmdStr].maxPara = max_para;    \
}while (0);
#define START_PROCESS   do{    \
    pthread_create(&tid, NULL, MdbProcess, this); \
}while(0);
#define STOP_PROCESS    do{    \
    bExit = 1;  \
    pthread_join(tid, NULL); \
}while(0);

#define INSTALL_PROCESS(processName, fpFunction, pAttr)    do{ \
    pthread_mutex_lock(&mutexProcess);  \
    mapCmdProcess[processName].fpFunc = (SUBPROCESS_FUNC)fpFunction;    \
    mapCmdProcess[processName].pArg = pAttr;    \
    pthread_mutex_unlock(&mutexProcess);  \
}while (0);

#define UNINSTALL_PROCESS(processName)    do{ \
    std::map<std::string, MDB_Process_t>::iterator iter;    \
    pthread_mutex_lock(&mutexProcess);  \
    iter = mapCmdProcess.find(processName);     \
    if (iter != mapCmdProcess.end()) \
    {   \
        mapCmdProcess.erase(iter);    \
    }   \
    pthread_mutex_unlock(&mutexProcess);  \
}while (0);

#define MDB_EXPECT(testcaseName, testcaseStr, testcaseExp) do {    \
        std::string tmpStr;    \
        char number[20];    \
        tmpStr += testcaseName;    \
        unsigned int intCurrentTime = GetTime();    \
        if (testcaseExp)    \
        {   \
            tmpStr += " : Test ok ";    \
            sprintf(number, "T: %d\n", GetTimeDiff(intCurrentTime));    \
            tmpStr += number;   \
            Print(testcaseStr, tmpStr, PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);    \
        }   \
        else    \
        {   \
            tmpStr += " : Test fail!\n";    \
            Print(testcaseStr, tmpStr, PRINT_COLOR_RED, PRINT_MODE_HIGHTLIGHT);    \
        }   \
    }while (0);
#define MDB_EXPECT_ERRCASE(testcaseName, testcaseStr, testcaseExp, errorcase) do {    \
        std::string tmpStr;    \
        char number[20];    \
        tmpStr += testcaseName;    \
        unsigned int intCurrentTime = GetTime();    \
        if (testcaseExp)    \
        {   \
            tmpStr += " : Test ok ";    \
            sprintf(number, "T: %d\n", GetTimeDiff(intCurrentTime));    \
            tmpStr += number;   \
            Print(testcaseStr, tmpStr, PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);    \
        }   \
        else    \
        {   \
            tmpStr += " : Test fail!\n";    \
            Print(testcaseStr, tmpStr, PRINT_COLOR_RED, PRINT_MODE_HIGHTLIGHT);    \
            {errorcase;};   \
        }   \
    }while (0);

#define MDB_EXPECT_OK(testcaseName, testcaseStr, ret, expval) MDB_EXPECT(testcaseName, testcaseStr, (ret == expval))
#define MDB_EXPECT_FAIL(testcaseName, testcaseStr, ret, expfailval) MDB_EXPECT(testcaseName, testcaseStr, (ret != expfailval))
#define MDB_EXPECT_OK_ERRCASE(testcaseName, testcaseStr, ret, expval, errcase) MDB_EXPECT_ERRCASE(testcaseName, testcaseStr, (ret == expval), errcase)
#define MDB_EXPECT_FAIL_ERRCASE(testcaseName, testcaseStr, ret, expfailval, errcase) MDB_EXPECT_ERRCASE(testcaseName, testcaseStr, (ret != expfailval), errcase)
#define MDB_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MDB_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MDB_ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#define MDB_ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))


class Mdb_Base
{


    public:
        Mdb_Base();
        virtual ~Mdb_Base();
        static Mdb_Base *GetInstance()
        {
            return pInstance;
        }
        static void Create(std::string &strModuleName);
        static void PrepareModule(std::string &strOut);
        static unsigned int Atoi(std::string &strOut);
        static void Print(std::string &strOut, std::string strContainer, PRINT_COLOR enColor = PRINT_COLOR_NORMAL, PRINT_MODE enMode = PRINT_MODE_NORMAL);
        void SetPara(std::string &strCmd, std::vector<std::string> &strInStrings);
        void GetName(std::string &strOut); //Get module name
        void DumpCmd(std::string &strOut);

        typedef void (Mdb_Base::*SUBCMD_FUNC)(std::vector<std::string> &, std::string &);
        typedef void (Mdb_Base::*SUBPROCESS_FUNC)(void *);

        typedef struct MDB_Cmd_s
        {
            SUBCMD_FUNC fpFunc;
            unsigned char maxPara;
        }MDB_Cmd_t;


        typedef struct MDB_Process_s
        {
            SUBPROCESS_FUNC fpFunc;
            void * pArg;
        }MDB_Process_t;

        typedef struct MDB_PushedCmds_s
        {
            std::vector<std::string> strInStrings;
            std::string strSubCmd;
        }MDB_PushedCmds_t;

        void DoCmd(std::string &strOut)
        {
            std::map<std::string, MDB_Cmd_t>::iterator it;
            if (strMdbBaseSubCmd == "push")
            {
                MDB_PushedCmds_t stPushedCmd;
                std::vector<std::string>::iterator itVectInstr;
                std::vector<std::string>::iterator itVectStr;

                if (strMdbBaseInStrings.size() == 1)
                {
                    for (itVectStr = strPushedStrings.begin(); itVectStr != strPushedStrings.end(); ++itVectStr)
                    {
                        if (*itVectStr == strMdbBaseSubCmd)
                        {
                            break;
                        }
                    }
                    if (itVectStr == strPushedStrings.end())
                        strPushedStrings.push_back(strMdbBaseInStrings[0]);
                }
            }
            else if (strMdbBaseSubCmd == "pop")
            {
                if (strPushedStrings.size())
                {
                    std::vector<std::string>::iterator itVectStr;
                    std::vector<MDB_PushedCmds_t>::iterator itVectCmds;
                    std::string ignoreStr;

                    if (strMdbBaseInStrings.size() == 1)
                    {
                        for (itVectStr = strPushedStrings.begin(); itVectStr != strPushedStrings.end(); ++itVectStr)
                        {
                            if (*itVectStr == strMdbBaseInStrings[0])
                            {
                                for (itVectCmds = mapPushedData[*itVectStr].begin(); itVectCmds != mapPushedData[*itVectStr].end(); ++itVectCmds)
                                {
                                    //printf("pushed data size %d sub cmd %s\n", mapPushedData[*itVectStr].size(), itVectCmds->strSubCmd.c_str());
                                    it = mapMdbBaseCmd.find(itVectCmds->strSubCmd);
                                    if (it != mapMdbBaseCmd.end())
                                    {
                                        if (it->second.maxPara != itVectCmds->strInStrings.size())
                                        {
                                            Print(strOut, "Cmd Para error!\n", PRINT_COLOR_YELLOW, PRINT_MODE_HIGHTLIGHT);
                                        }
                                        else
                                        {
                                            (this->*(it->second.fpFunc))(itVectCmds->strInStrings, ignoreStr);
                                        }
                                    }

                                }
                                break;
                            }
                        }
                        if (itVectStr != strPushedStrings.end())
                        {
                            if (mapPushedData[*itVectStr].begin() != mapPushedData[*itVectStr].end())
                            {
                                mapPushedData.erase(*itVectStr);
                            }
                            strPushedStrings.erase(itVectStr);
                        }
                    }
                }
            }
            else
            {
                if (strPushedStrings.size())
                {
                    std::vector<std::string>::iterator itVectStr;
                    MDB_PushedCmds_t pushedCmds;

                    for (itVectStr = strPushedStrings.begin(); itVectStr != strPushedStrings.end(); ++itVectStr)
                    {
                        if (*itVectStr == strMdbBaseSubCmd && strMdbBaseInStrings.size())
                        {
                            pushedCmds.strSubCmd = strMdbBaseInStrings[0];
                            pushedCmds.strInStrings = strMdbBaseInStrings;
                            pushedCmds.strInStrings.erase(pushedCmds.strInStrings.begin());
                            mapPushedData[*itVectStr].push_back(pushedCmds);

                            break;
                        }
                    }
                }
                else
                {
                    it = mapMdbBaseCmd.find(strMdbBaseSubCmd);
                    if (it != mapMdbBaseCmd.end())
                    {
                        if (it->second.maxPara != strMdbBaseInStrings.size())
                        {
                            Print(strOut, "Cmd Para error!\n", PRINT_COLOR_YELLOW, PRINT_MODE_HIGHTLIGHT);
                            return;
                        }
                        (this->*(it->second.fpFunc))(strMdbBaseInStrings, strOut);
                    }
                }
            }
        }
        virtual void ShowWelcome(std::string &strOut)
        {
            strOut+= "Welcome!\n";
        }
        void Destroy(void)
        {
            printf("%s\n", __FUNCTION__);
            delete this;
        }
        template<class MDBCHILD>
        class Mdb_Setup
        {
            public:
                explicit Mdb_Setup(const char *pModuleName);
                ~Mdb_Setup()
                {
                    ;//printf("%s\n", __FUNCTION__);
                }
        };
    protected:
        static void *MdbProcess(void *arg)
        {
            Mdb_Base *pIns = (Mdb_Base *)arg;
            std::map<std::string, MDB_Process_t>::iterator iter;
            if (pIns)
            {
                while (pIns->bExit == 0)
                {
                    pthread_mutex_lock(&pIns->mutexProcess);
                    for (iter = pIns->mapCmdProcess.begin(); iter != pIns->mapCmdProcess.end(); iter++)
                    {
                        (pIns->*(iter->second.fpFunc))(iter->second.pArg);
                    }
                    pthread_mutex_unlock(&pIns->mutexProcess);
                    usleep(40);
                }
            }
            return NULL;
        }
        std::map<std::string, MDB_Cmd_t> mapMdbBaseCmd;
        std::map<std::string, MDB_Process_t> mapCmdProcess;
        pthread_mutex_t mutexProcess;
        unsigned int GetTime(void);
        unsigned int GetTimeDiff(unsigned int intOldTime);

    private:
        std::map< std::string, std::vector<MDB_PushedCmds_t> > mapPushedData;
        std::vector<std::string> strPushedStrings;
        std::vector<std::string> strMdbBaseInStrings;
        std::string strMdbBaseSubCmd;
        static Mdb_Base* pInstance;
        static std::map<std::string, std::string> mapModule;
        std::string strMdbBaseSubName;
        pthread_t tid;
        int bExit;
};

#endif //MDB_BASE_H
