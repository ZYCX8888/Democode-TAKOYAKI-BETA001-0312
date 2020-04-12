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
#ifndef MDB_H//MDB_H
#define MDB_H
#include <pthread.h>

#include <vector>
#include <string>

class Mdb
{
   public:
    explicit Mdb()
    {
        bExitTransThread = 0;
        return;
    }
    virtual ~Mdb()
    {
        return;
    }
    static void Run(int intMod);
    static void Wait(void);
   private:
    void ProcessStrings(char *pInStr, char *pOutStr);
    void ParseStrings(char *pStr);
    void ShowWelcome(void);
    void TransGetDataSocket(void);
    void TransGetDataShm(void);
    std::vector<std::string> strInStrings;
    std::string strSubCmd;
    std::string strOutString;
    unsigned char bExitTransThread;
    pthread_attr_t tAttr;
    pthread_t tTid;

    static void *TransThread(void *pPara);
    static Mdb *pInstance;
};
#endif //MDB_H
