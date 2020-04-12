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

/*
 * CevaLinkComponentBase.h
 */

#ifndef _CEVALINKCOMPONENTBASE_H_
#define _CEVALINKCOMPONENTBASE_H_

#include "ceva_log.h"
#include "CevaConditionVariable.h"
#include "ceva_link_api.h"
#include "ICevaLinkClient.h"
#include "CevaLinkClientFactory.h"
#include <atomic>

namespace CEVA_AMF {
	class CevaLinkComponentBase
	{
	public:

		enum e_status {
			E_SUCCEED = 0x0000,	//!< Function returned successfully
			E_FAILED = 0xffff	//!< General failure
		};

		CevaLinkComponentBase();
		virtual e_status Init(dsp_module_type_enum e_moduleType);
		virtual e_status Send(const cmd_struct* p_cmd, unsigned int size = sizeof(cmd_struct));
		virtual e_status Receive(cmd_struct** retval, unsigned int size = sizeof(cmd_struct));
		virtual e_status Deinit();
		virtual ~CevaLinkComponentBase();
		void* AllocateDataBuffer(size_t size, U32 alignValue = 1);
		void* ConvertAddressHost2DSP(void* pBuff);
		void* ConvertAddressDSP2Host(void* pBuff);
		void FreeDataBuffer(void* pBuff);

	protected:

		static void EventHandlerReceive(ceva_link *link, ceva_event event_id, void *context);
		e_status Init(dsp_module_type_enum e_moduleType, ceva_event_cb receive_cb);
		void SetEvent(ceva_event event_id);
		void SignalEvent();
		void Wait();

		CEVA_AMF::ICevaLinkClient* p_client;
		ceva_event received_event;
		CEVA_AMF::CevaConditionVariable cond_variable;
		cmd_struct received_cmd;
		std::atomic<int> receive_counter;
		bool is_initialized;
	};
}
#endif /* _CEVALINKCOMPONENTBASE_H_ */
