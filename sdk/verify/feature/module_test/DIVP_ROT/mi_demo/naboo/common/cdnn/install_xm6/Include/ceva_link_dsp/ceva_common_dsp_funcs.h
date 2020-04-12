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

#ifndef _CEVA_COMMON_DSP_FUNCS_H_
#define _CEVA_COMMON_DSP_FUNCS_H_

/*! \file
    This file defines the CEVA-Link DSP Framework types and functions.
*/



/**
* \addtogroup CevaLinkDsp CEVA-Link DSP
*  @{
*  \brief Provides the API for the DSP applications to communicate with the host applications
*  \details The  CEVA-Link DSP library defines the data structures and functions for registration of DSP modules and communication with host applications.
*  This library includes the following functionality:
*  - DSP core registration
*  - DSP module registration
*  - Event handling
*  - Mailbox messaging to and from the host
*/

#ifdef __cplusplus
extern "C" {
#endif

#include	"ceva_shared_mem_common.h"
#include	"ceva_typedef.h"
#include	"ceva_link_mailbox.h"
#include 	"ceva_dsp_and_modules_types.h"


/**
* \brief The command handler function pointer
*/
typedef S32 (*command_handler_callback_func)(dsp_info_struct *dsp_info, S32 module_id, void* args);


/**
* \brief The command handler registration structure
*/
typedef struct {
	command_handler_callback_func*		handlers;			/**< Pointer to array of handler functions */
	ceva_link_mailbox_struct**			mailboxes;			/**< Pointer to mailboxes */
	dsp_info_struct*					dsp_info;			/**< Pointer to shared DSP info */
	U32									num_of_host_cmds;	/**< Number of supported host commands */
} messages_registry_struct;


/*!
* \brief Initializes the message handler registry
* @param  registry					Message handler registry structure to initialize
* @param  p_handlers_array			Pointer to the handler functions
* @param  default_handler			Pointer to the default handler function
* @param  p_mailboxs_array		    Pointer to the mailboxes
* @param  num_of_host_cmds		    Number of supported host commands
* @param  num_of_mailboxes		    Number of mailboxes in p_mailboxs_array
* @return S32						The return code
*/
S32 dsp_init_message_handlers(messages_registry_struct*			registry,
							  command_handler_callback_func*	p_handlers_array,
							  command_handler_callback_func		default_handler,
							  ceva_link_mailbox_struct*			p_mailboxs_array[],
							  U32								num_of_host_cmds,
							  U32								num_of_mailboxes);

/*!
* \brief Initializes the mailbox 
* @param  registry					Message handler registry structure to initialize
* @param  p_mailboxs_array		    Pointer to the mailboxes
* @return void                      
*/
void dsp_init_mailbox(messages_registry_struct* registry,
    ceva_link_mailbox_struct* 		p_mailboxs_array[]);

/*!
* \brief Assigns a handler to a message type of a given module
* @param  registry				Message handler registry structure
* @param  module_id				Module ID
* @param  message_id			Message type 
* @param  handler				Message handler for the given message type
* @return S32					The return code
*/
S32 dsp_register_message_handler(messages_registry_struct*		registry, 
								 S32							module_id, 
								 S32							message_id, 
								 command_handler_callback_func	handler);


/*!
* \brief Scans a given mailbox for new messages and calls the assigned message handler
* @param  registry				Message handler registry structure
* @param  mailbox_id			Mailbox ID
* @return S32					The return code
*/
S32 dsp_cmd_parser(messages_registry_struct* registry, U32 mailbox_id);

/*!
* \brief Performs link operations (must be called from the user-defined make link handler)
* @param  dsp_info			Pointer to the DSP info structure
* @param  module_id			Module ID of the handling module
* @param  args				Additional arguments
* @return S32				The return code
*/
S32 dsp_make_link(dsp_info_struct *dsp_info, S32 module_id, void* args);

/*!
* \brief Performs unlink operations (must be called from the user-defined unlink handler)
* @param  dsp_info			Pointer to the DSP info structure
* @param  module_id			Module ID of the handling module
* @param  args				Additional arguments
* @return S32				The return code
*/
S32 dsp_unlink(dsp_info_struct *dsp_info, S32 module_id, void* args);

/*!
* \brief Inserts a new module into the DSP info structure
* @param shared_info_mem 			Pointer to the shared memory info region
* @param shared_data_mem 			Pointer to the shared memory data region
* @param registry 					Message handler registry structure
* @param dsp_id 					DSP ID
* @param module_type				Module type
* @param input_data_size			Module's required input data buffer size, in words
* @param output_data_size			Module's required output data buffer size, in words
* @return S32						The return code
*/
S32 dsp_insert_module( shared_mem_info_struct*	shared_info_mem,
				    S16*						shared_data_mem,
					messages_registry_struct*	registry,
					U32							dsp_id,
					dsp_module_type_enum		module_type,
					U32							input_data_size,
					U32							output_data_size);


/*!
* \brief Registers a new module to the DSP info structure
* @param shared_info_mem 			Pointer to the shared memory info region
* @param registry 					Message handler registry structure
* @param dsp_id 					DSP ID
* @param dsp_mem_endianity			DSP core memory endianness type
* @param dsp_mem_element_size		DSP core memory element size, in bytes
* @param dsp_type					DSP type
* @param dsp_num_of_modules			Number of supported modules
* @return S32						The return code
*/
S32 dsp_register_dsp(shared_mem_info_struct*	shared_info_mem, 
				 messages_registry_struct*		registry,
				 U32							dsp_id,
				 dsp_endianity_enum				dsp_mem_endianity,
				 U32							dsp_mem_element_size,
				 dsp_type_enum					dsp_type,
				 U32							dsp_num_of_modules);



void dsp_generate_link_event_by_client(U32 event, U32 client_id);
void dsp_generate_link_event_by_module(U32 event, U32 module_id);


#ifdef __cplusplus
}
#endif

/** @}*/

#endif



