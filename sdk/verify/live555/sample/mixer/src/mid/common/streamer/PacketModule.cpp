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

/*    filename PacketManager.cpp
*   Description:
*    Copyright(C):    2016 Mstar Semiconductor
*    Version:        V 1.0
*    Author:        fisher.yang
*    Created:        2016-07-15
*/
#include <assert.h>

#include "mid_sys.h"
#include "mid_common.h"
#include "PacketModule.h"


PacketModule::PacketModule()
{
  total_length = 0x00;
  m_pPacketList = NULL;
  memset(&m_pPacketEmptyList,0x00,sizeof(list_head));
  PacketFreeNum = 0x00;
}

PacketModule::~PacketModule()
{

}

PacketModule* PacketModule::getInstance()
{
    static PacketModule objPacketModule;
    return &objPacketModule;
}

MI_S32 PacketModule::init()
{
    return 0;
}

MI_S32 PacketModule::unInit()
{
    return 0;
}

MI_S32 PacketModule::start()
{
    if (m_pPacketList)
        return 0;

    m_pPacketList = new CPacket[PACKETLENGTH];
    assert(m_pPacketList);

    MIXER_DBG("m_pPacketList addr (%p)\n", m_pPacketList);
    INIT_LIST_HEAD(&m_pPacketEmptyList);

    PacketFreeNum = PACKETLENGTH;

    MI_S32 i = 0x0;

    for (; i < PACKETLENGTH; i++)
    {
        m_pPacketList[i].m_pBuffer = NULL;
        m_pPacketList[i].m_Length = 0x0;
        m_pPacketList[i].m_RefCount = 0x0;

        list_add_tail(&m_pPacketList[i].packetlist, &m_pPacketEmptyList);
    }

    return 0;
}

MI_S32 PacketModule::stop()
{
    MI_S32 i = 0x0;

    m_Mutex.OnLock();

    if (m_pPacketList)
    {
        for (; i < PACKETLENGTH; i++)
        {
            //delete m_pPacketList[i].m_pBuffer;
            //m_pPacketList[i].m_pBuffer = NULL;
            if(m_pPacketList[i].m_pBuffer)
            {
            free(m_pPacketList[i].m_pBuffer);
            m_pPacketList[i].m_pBuffer = NULL;
         }
            INIT_LIST_HEAD(&m_pPacketList[i].packetlist);
        }
    }

    delete [] m_pPacketList;
    m_pPacketList = NULL;
    m_Mutex.OnUnLock();
    return 0;
}

CPacket* PacketModule::MallocPacket(MI_U32 dwBytes /* = 0*/)
{
     //MIXER_DBG("~~~~~~~ GetPacketFreeNum()    free:%d\n", GetPacketFreeNum());
    if ((NULL == m_pPacketList) || (GetPacketFreeNum() <= 0x0))
    {
        return NULL;
    }

    m_Mutex.OnLock();
    if (list_empty(&m_pPacketEmptyList))
    {
        m_Mutex.OnUnLock();
        return NULL;
    }

    CPacket* tmp = NULL;

    tmp = list_entry(m_pPacketEmptyList.next, CPacket, packetlist);

    //MIXER_DBG("tmp addr (%p)\n", tmp);
    if (tmp->PutBuffer(dwBytes) < 0x0)
    {
        m_Mutex.OnUnLock();
        return NULL;
    }
    total_length += dwBytes;

    //list_del(m_pPacketEmptyList.next);
    list_del(&tmp->packetlist);
    PacketFreeNum--;

    m_Mutex.OnUnLock();

    return tmp;
}

void PacketModule::FreePacket(CPacket *pPacket)
{
    if((NULL == pPacket) || (NULL == m_pPacketList))
        return;

    m_Mutex.OnLock();

    if (pPacket->m_pBuffer)
    {
        free(pPacket->m_pBuffer);
        pPacket->m_pBuffer = NULL;
    }
    total_length -= pPacket->m_Length;

    pPacket->m_Length = 0x0;
    //pPacket->m_pBuffer = NULL;

    list_add_tail(&pPacket->packetlist, &m_pPacketEmptyList);
    PacketFreeNum++;

    m_Mutex.OnUnLock();
}

MI_U32 PacketModule::GetBufferInfo()
{
    MIXER_DBG("Packet Num = %d\n", PACKETLENGTH - GetPacketFreeNum());
    MIXER_DBG("Packet total length = %lld\n", total_length);
    return 0;
}

MI_S32 PacketModule::GetPacketFreeNum()
{
    MI_S32 tmp;

    do {
        tmp = PacketFreeNum;
    } while (tmp != PacketFreeNum);

    return tmp;
}

