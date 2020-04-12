/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

/*    filename Video.c
*   Description:
*    Copyright(C):    2016 Mstar Semiconductor
*    Version:        V 1.0
*    Author:        fisher.yang
*    Created:        2016-08-10
*/

#include "mid_common.h"
#include "PacketCore.h"
#include "PacketModule.h"

CPacket::CPacket()
{
    m_pBuffer = NULL;
    m_Length = 0x0;
    m_RefCount = 0x0;
    memset(&packetlist,0x00,sizeof(list_head));
}

CPacket::~CPacket()
{

}


MI_U32    CPacket::PutBuffer(MI_U32 dwLength)
{
    if (0x0 == dwLength)
        return 0x0;

    //m_pBuffer = new unsigned char[dwLength];
    m_pBuffer = (MI_U8 *)malloc(sizeof(unsigned char) * dwLength);
    if (NULL == m_pBuffer)
        return -1;

    m_Length = dwLength;

    return 0x0;
}

MI_U8 *CPacket::GetBuffer()
{
    return m_pBuffer;
}

MI_U32    CPacket::GetLength()                    //取数据长度
{
    return m_Length;
}

MI_U32    CPacket::Release()
{
    m_Mutex.OnLock();

    #if 0
    if (m_RefCount <= 0)
    {
        assert(m_RefCount > 0);

    }

    if (m_RefCount == 0)
    {
        m_Mutex.OnUnLock();
        return 0;
    }
    #else
    /*MI_S32 _tmp;

    do{
        _tmp = m_RefCount;
    }while(_tmp != m_RefCount);*/

    if(GetRef() <= 0)
    {
       m_Mutex.OnUnLock();
       return 0;
    }
    #endif

    --m_RefCount;

    //因为 m_RefCount 是volatile 类型的，因此这个做法是必须的。
    int tmp = m_RefCount;
    m_Mutex.OnUnLock();
    if (0x0 == tmp)
    {
        g_PacketModule->FreePacket(this);
        /*PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());
        pobjPacketModule->FreePacket(this);*/
    }

    return 0;
}

MI_U32    CPacket::AddRef()
{
    m_RefCount++;
    return m_RefCount;
}

MI_S32 CPacket::GetRef()
{
    int tmp;

    do {
        tmp = m_RefCount;
    } while (tmp != m_RefCount);

    return tmp;
}


