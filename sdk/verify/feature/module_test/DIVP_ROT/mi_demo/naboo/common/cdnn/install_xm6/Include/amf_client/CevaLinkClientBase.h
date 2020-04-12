/********************************************************************************************************************************************************
Copyright (C) CEVA(R) Inc. All rights reserved

This information embodies materials and concepts, which are proprietary and confidential to CEVA Inc., and is made available solely pursuant to the 
terms of a written license agreement, or NDA, or another written agreement, as applicable (“CEVA Agreement”), with CEVA Inc. or any of its subsidiaries 
(“CEVA”). 

This information can be used only with the written permission from CEVA, in accordance with the terms and conditions stipulated in the CEVA Agreement, 
under which the information has been supplied and solely as expressly permitted for the purpose specified in the CEVA Agreement.
This information is made available exclusively to licensees or parties that have received express written authorization from CEVA to download or receive 
the information and have agreed to the terms and conditions of the CEVA Agreement.
IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE CEVA AGREEMENT, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS INFORMATION.

The information contained in this document is subject to change without notice and does not represent a commitment on any part of CEVA. Unless 
specifically agreed otherwise in the CEVA Agreement, CEVA make no warranty of any kind with regard to this material, including, but not limited to 
implied warranties of merchantability and fitness for a particular purpose whether arising out of law, custom, conduct or otherwise.

While the information contained herein is assumed to be accurate, CEVA assumes no responsibility for any errors or omissions contained herein, 
and assumes no liability for special, direct, indirect or consequential damage, losses, costs, charges, claims, demands, fees or expenses, of any nature
or kind, which are incurred in connection with the furnishing, performance or use of this material.

This document contains proprietary information, which is protected by U.S. and international copyright laws. All rights reserved. 
No part of this document may be reproduced, photocopied, or translated into another language without the prior written consent of CEVA. 
********************************************************************************************************************************************************/


#ifndef _CEVALINKCLIENTBASE_H_
#define	_CEVALINKCLIENTBASE_H_


#include "ICevaLinkClient.h"
#include "CevaMemoryManager.h"
#include "ceva_inter_process_db.h"
#include "CevaConditionVariable.h"
#include "ceva_shared_mem_common.h"

#define HTOC_AT(address)	(address - m_nATTOffset)
#define CTOH_AT(address)	(address + m_nATTOffset)






namespace CEVA_AMF 
{

class CevaLinkClientBase: public ICevaLinkClient
{
private:
	S32							m_nClientId;				/**< Client unique ID */
	U32							m_nLinkedDspId;				/**< Linked DSP ID */
	U32							m_nLinkedModuleId;			/**< Linked module ID */
	U32							m_nMessageCnt;				/**< Client message counter */
	S32							m_nATOffset;				/**< Client address translation offset */
	bool 						m_isInterrupt;				/**< Client interrupts mode*/

	ceva_link*					m_pCevaDataLinkHandle;		/**< Data link handle */
	CevaInterProcessDB*			m_pCevaLinkDB;				/**< Inter-process database */
	shared_mem_info_struct*		m_pDmaRegion;				/**< DMA region address */

	ceva_link_mailbox_struct*	m_pDspMailbox[2];			/**< DSP mailbox pointers 0-to DSP 1-from DSP */
	ceva_link_mailbox_struct*	m_pModuleMailbox[2];		/**< Module mailbox pointers 0-to module 1-from module */

	int							Lock();
	int							Unlock();
	ClientStatus				LinkComponent(dsp_module_type_enum);
	void						UnLinkComponent();	
	virtual ClientStatus		SendDSPMsg(const cmd_struct* msg, U32 nDspId);
	virtual ClientStatus 		SendDSPMsg(const cmd_struct*);


protected:
	CevaMemoryManager			m_ModuleMemManager;         /**< Manages DMA region buffer allocations */
	CevaConditionVariable 		m_CondVariable;				/**< Wait for event functionality */
	ceva_event_type				m_eReceivedDSPEvent;		/**< Last DSP event received */

	static void					EventHandler(ceva_link *link, ceva_event event_id, void *context);
public:
	CevaLinkClientBase();

	virtual ClientStatus		Init(dsp_module_type_enum moduleType);

	virtual ClientStatus		Deinit();

	virtual ClientStatus		RegisterEventCallback(ceva_event_type event_type,
											  ceva_event_cb cb, void* context);

	virtual ClientStatus		UnregisterEventCallback(ceva_event_type event_type, void* context);

	virtual ClientStatus		WaitForEvent(ceva_event_type event_type, U32 timeout);

	virtual void*				AllocateDataBuffer(U32 size, U32 alignValue = 1);

	virtual void				FreeDataBuffer(U8* addr);

	virtual ClientStatus		SendModuleMsg(const cmd_struct* msg, U32 size = sizeof(cmd_struct));

	virtual ClientStatus		ReceiveModuleMsg(const cmd_struct* msg, U32 size = sizeof(cmd_struct));

	virtual void*				ConvertAddressHost2DSP(void* ptr);

	virtual void*				ConvertAddressDSP2Host(void* ptr);
	
	virtual void				EnableModuleInterrupts(bool isInterrupt);

	virtual 					~CevaLinkClientBase();
};

}
#endif /* _CEVA_LINK_CLIENT_BASE_H_ */
