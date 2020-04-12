/*    filename PacketManager.h
*   Description:
*    Copyright(C):    2016 Mstar Semiconductor
*    Version:        V 1.0
*    Author:        fisher.yang
*    Created:        2016-07-15
*/

#ifndef _PACKETMODULE_H_
#define _PACKETMODULE_H_

#include "mid_common.h"
#include "PacketCore.h"
#include "List.h"
#include "MyMutex.h"


#define PACKETLENGTH (256*4)

class PacketModule
{
    PacketModule();
    ~PacketModule();

public:
    static PacketModule* getInstance();
    MI_S32 init();
    MI_S32 unInit();
    MI_S32 start();
    MI_S32 stop();

    CPacket* MallocPacket(MI_U32 dwBytes = 0);
    void FreePacket(CPacket *pPacket);
    MI_U32     GetBufferInfo();
private:
    MI_S32 GetPacketFreeNum();
    MyMutex        m_Mutex;

    MI_U64         total_length;

    CPacket* m_pPacketList;

    struct list_head m_pPacketEmptyList;

    volatile MI_S32  PacketFreeNum;
};

#define g_PacketModule   (PacketModule::getInstance())

#endif
