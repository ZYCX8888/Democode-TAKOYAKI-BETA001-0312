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

/*! \file
*    This file defines the CEVA-Link Client interface
*/

#ifndef _ICEVA_LINK_CLIENT_H_
#define	_ICEVA_LINK_CLIENT_H_

#include "ceva_typedef.h"
#include "ceva_dsp_and_modules_types.h"
#include "ceva_dsp_command_struct.h"
#include "ceva_link_api.h"

/*!
* \addtogroup CevaLinkClient
*  @{
*    \brief High-level host API for the CEVA-Link communication layer
*
*/

#define CEVA_LINK_TIMEOT_MS	(3000)
#define CEVA_LINK_TIMEOUT_MS	CEVA_LINK_TIMEOT_MS

namespace CEVA_AMF {
    
    /*!
	*  \brief This is the client interface for communication with the CEVA-Link.
    *
	*  This interface is used by host applications to establish link communication and send/receive messages to/from the CEVA device.
    *
	*  It includes the following functionality:
	*  - Device initialization and de-initialization
	*  - Sending and receiving core messages
	*  - Sending and receiving module messages
	*  - Allocation and deallocation of memory shared by the host and CEVA device
	*  - Address translation between the host and the CEVA device
	*/
class ICevaLinkClient {

public:
	//! Enumeration of possible status values returned by this interface's methods
	enum ClientStatus
	{
		E_STATUS_OK,
		E_LINK_LIB_FAILED,
		E_DMA_REGION_INVALID,
		E_IPC_DB_INVALID,
		E_NO_FREE_CLIENT,
		E_BUFFER_ADDR_INVALID,
		E_REGISTER_EVENT_FAILED,
		E_UNREGISTER_EVENT_FAILED,
		E_SEND_MESSAGE_FAILED,
		E_RECEIVED_MESSAGE_FAILED,
		E_WAIT_EVENT_TIMEOUT,
		E_NOT_LINKED,
		E_INSUFFICIENT_RESOURCES,
		E_UNKNOWN_STATE
		//etc....
	};

//! Direction of the message transmission
enum MailboxDirection
{
	E_TO_MODULE = 0,
	E_FROM_MODULE = 1
};

public:
	/// \brief Initializes the communication mechanism with a specific DSP module
	/// \param[in] moduleType DSP module type enumeration value
	/// \return The status of the initialization
	virtual ClientStatus				Init(dsp_module_type_enum moduleType) = 0;

	/// \brief De-initializes the communication mechanism with the linked DSP module
	/// \return The status of the de-initialization
	virtual ClientStatus				Deinit() = 0;

	/// \brief Registers a callback function for the given event
	/// \param[in] event_type Event descriptor
	/// \param[in] cb Pointer to a callback function
	/// \param[in] context Pointer to an opaque data structure to be passed to the callback function
	/// \return The status of registering the callback
	virtual ClientStatus				RegisterEventCallback(ceva_event_type event_type,
		ceva_event_cb cb, void* context) = 0;

	/// \brief Unregisters the callback function for the given event
	/// \param[in] event_type Event descriptor
	/// \param[in] context Pointer to an opaque data structure to be passed to the callback function
	/// \return The status of unregistering the callback
	virtual ClientStatus				UnregisterEventCallback(ceva_event_type event_type, void* context) = 0;

	/// \brief Waits for the event
	/// \param[in] event_type Event descriptor
	/// \param[in] timeout Timeout interval, in milliseconds (zero for infinite)
	/// \return The status of the wait operation
	virtual ClientStatus				WaitForEvent(ceva_event_type event_type, U32 timeout) = 0;

	/// \brief Allocates a buffer in the shared memory space
	/// \param[in] size Buffer size, in bytes	
    /// \param[in] alignValue Byte alignment 
	/// \return A pointer to the allocated buffer (null if allocation fails)
	virtual void*						AllocateDataBuffer(U32 size, U32 alignValue = 1) = 0;

	/// \brief Deallocates a buffer in the shared memory space
	/// \param[in] addr Pointer to the buffer to deallocate	
	virtual void						FreeDataBuffer(U8* addr) = 0;

	/// \brief Translates the host address (virtual) into a DSP address (physical)
	/// \param[in] ptr Virtual address as seen by the host application
	/// \return The physical address as seen by the DSP
	virtual void*						ConvertAddressHost2DSP(void* ptr) = 0;

	/// \brief Translates the DSP address (physical) into a host address (virtual)
	/// \param[in] ptr Physical address as seen by the DSP
	/// \return The virtual address as seen by the host application
	virtual void*						ConvertAddressDSP2Host(void* ptr) = 0;


	/// \brief Sends a message to the DSP module (non-blocking)
	/// \param[in] msg Pointer the message data structure
    /// \param[in] size Size, in bytes, of the command structure (default is sizeof(cmd_struct))
	/// \return The status of sending the message
	virtual ClientStatus				SendModuleMsg(const cmd_struct* msg, U32 size = sizeof(cmd_struct)) = 0;

	/// \brief Reads the next message from the DSP module (non-blocking)
	/// \param[out] msg Pointer to the message data structure
    /// \param[in] size Size, in bytes, of the command structure (default is sizeof(cmd_struct))
	/// \return The status of reading the message
	/// \note This function does not wait for the message to become available.
	virtual ClientStatus				ReceiveModuleMsg(const cmd_struct* msg, U32 size = sizeof(cmd_struct)) = 0;

	/// \brief Sets whether the send message method triggers an interrupt
	/// \param[in] isInterrupt Boolean value to enable/disable the interrupt operation mode
	virtual void						EnableModuleInterrupts(bool isInterrupt) = 0;

	virtual ~ICevaLinkClient()		{}
};

}
/** @}*/

#endif //_ICEVA_LINK_CLIENT_H_


