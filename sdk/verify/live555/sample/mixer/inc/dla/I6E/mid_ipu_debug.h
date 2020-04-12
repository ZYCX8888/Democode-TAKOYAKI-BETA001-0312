#ifndef _MID_IPU_DEBUG_H_
#define _MID_IPU_DEBUG_H_

#include <iostream>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <map>
#include <dirent.h>

#include "mid_ipu_interface.h"
#include "FaceDatabase.h"
#include "mid_dla.h"

class CMidIPUDebug: public CMidIPUInterface
{
public:
    CMidIPUDebug(IPU_InitInfo_S &stInitInfo);
    ~CMidIPUDebug();

public:
    int InitResource();
    int ReleaseResource();
    void DealDataProcess();
    MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop);
    
private:
    int InitStreamSource(Model_Info_S &stModelInfo);
    int ReleaseStreamSource(Model_Info_S &stModelInfo);


private:

    Model_Info_S                m_stDebugFdaModel;
    Model_Info_S                m_stDebugFrModel;

    MI_S32                      m_s32MaxFd;
    MI_U8                       m_dataReady;

};

#endif // _MID_IPU_FDFR_H_

