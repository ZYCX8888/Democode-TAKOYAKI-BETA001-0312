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

#ifndef _SHARED_MEM_COMMON_H
#define _SHARED_MEM_COMMON_H

#ifdef WIN32
#include	<windows.h>
#endif

#include	<stdio.h>
#include	"ceva_typedef.h"
#include	"ceva_link_mailbox.h"
#include 	"ceva_dsp_and_modules_types.h"
//#include	"debug_funcs.h"

#ifdef __cplusplus
extern "C" {
#endif


#define	TO_MODULE		0
#define	FROM_MODULE		1
#define	TO_DSP			0
#define	FROM_DSP		1

/**
* \brief The DSP registration state enumeration
*/
typedef enum
{
	E_REG_STATE_UNKNOWN = 0,	/**< Undefined state */
	E_REG_STATE_IN_PROGRESS,	/**< DSP registration in progress */		
	E_REG_STATE_DONE			/**< DSP registration complete */		
}dsp_reg_state_enum;

/**
* \brief The module state enumeration
*/
typedef enum
{
	E_MODULE_AVAILABLE = 0x0,	/**< Module available */	
	E_MODULE_LINK_IN_PROCESS,	/**< Module link in progress */	
	E_MODULE_NOT_AVAILABLE		/**< Module not available */	
}module_availability_enum;

/**
* \brief The DSP endianness mode enumeration
*/
typedef enum
{
	E_LITTLE_ENDIAN,			/**< Little endian */
	E_BIG_ENDIAN				/**< Big endian */
}dsp_endianity_enum;


/**
* \brief The module data buffer structure
*/
typedef struct  
{
	U32							size;				/**< The data buffer size */
	void						*data_ptr;			/**< The data buffer pointer */
}module_data_buf_struct;


/**
* \brief The module info structure
*/
typedef struct
{
	dsp_module_type_enum		module_type;							/**< The module type */
	module_availability_enum	module_available;						/**< The module availability state*/
	ceva_link_mailbox_struct	module_mailbox[NUM_MAILBOX_FOR_DSP];	/**< The module mailboxes (to/from module) */
	module_data_buf_struct		module_data_buf;						/**< The module data buffer */
}dsp_module_info_struct;

/**
* \brief The DSP info structure
*/
typedef struct
{
	U32							dsp_id;								/**< The DSP ID */
	dsp_reg_state_enum			dsp_reg_state;						/**< The DSP registration state */
	dsp_endianity_enum			dsp_mem_endianity;					/**< The DSP memory endianness */
	U32							dsp_mem_element_size;				/**< The DSP memory element size, in bytes */
	U32							dsp_shared_mem_offset;				/**< The DSP shared memory start address */
	U32							dsp_info_mem_size;					/**< The DSP info memory size requirement */
	U32							dsp_data_mem_size;					/**< The DSP data memory size requirement */
	dsp_type_enum				dsp_type;							/**< The DSP type */
	U32							dsp_num_of_modules;					/**< The number of supported modules */
	ceva_link_mailbox_struct	dsp_mailbox[2];						/**< The DSP mailboxes (to/from DSP) */
	dsp_module_info_struct		*module_info_ptr[MAX_DSP_MODULES];	/**< Pointers to module info structures of the DSP  */	
}dsp_info_struct;


/**
* \brief The shared memory DSPs registry
*/
typedef struct
{
	dsp_info_struct		dsp_info[NUM_OF_DSPS];		/**< System DSP info  */	
	
}shared_mem_info_struct;


//Shared memory region [words]
// Hardware-dependent memory configuration
#define		SM_TOTAL_SIZE				((0x1000000)/sizeof(S16))
// round up the info size to page size: 4K (0x1000 bytes = 0x800 words)
#define		SM_INFO_SIZE				((((sizeof(shared_mem_info_struct) + NUM_OF_DSPS*MAX_DSP_MODULES*sizeof(dsp_module_info_struct))/sizeof(S16))+0x7ff) & ~0x7ff)
#define		SM_DATA_SIZE				(SM_TOTAL_SIZE-SM_INFO_SIZE)

/**
* \brief The maximum data buffer size, in S16 units
*/
#define MAX_COMPONENT_TO_MODULE_DATA_BUFFER_SIZE			(SM_DATA_SIZE/2)
#define MAX_COMPONENT_FROM_MODULE_DATA_BUFFER_SIZE			(SM_DATA_SIZE/2)

//Address translation
#ifdef _CEVA_LINK_DSP_
#define CONVERT_ADDRESS_DSP_TO_HOST(shared_mem,dsp_id,ptr)					(ptr)
#define CONVERT_ADDRESS_HOST_TO_DSP(shared_mem,dsp_id,ptr)					(ptr)
#else // _CEVA_LINK_DSP_
#ifdef _CEVA_LINK_HOST_
#define CONVERT_ADDRESS_DSP_TO_HOST(shared_mem,dsp_id,ptr)					((void*)( ( (U32)ptr - DSP_MEM_OFFSET(shared_mem,dsp_id) ) * DSP_ELEMENT_SIZE(shared_mem,dsp_id) + (U32)shared_mem ))
#define CONVERT_ADDRESS_HOST_TO_DSP(shared_mem,dsp_id,ptr)					((void*)( ( (U32)ptr - (U32)shared_mem ) / DSP_ELEMENT_SIZE(shared_mem,dsp_id) + (U32)DSP_MEM_OFFSET(shared_mem,dsp_id) ))
#else // _CEVA_LINK_HOST_
#error "CEVA LINK platform not defined"
#endif // _CEVA_LINK_HOST_
#endif // _CEVA_LINK_DSP_



//DSP info
#define	DSP_INFO_PTR(shared_mem,dsp_id)										((&((shared_mem)->dsp_info[dsp_id])))
#define DSP_REGISTRY_ID(shared_mem,dsp_id)									(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_id)
#define DSP_REGISTRY_STATE(shared_mem,dsp_id)								(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_reg_state)
#define DSP_REGISTRY_INFO_MEM_SIZE(shared_mem,dsp_id)						(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_info_mem_size)
#define DSP_REGISTRY_DATA_MEM_SIZE(shared_mem,dsp_id)						(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_data_mem_size)
#define	DSP_TYPE(shared_mem,dsp_id)											(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_type)
#define	DSP_ENDIANITY(shared_mem,dsp_id)									(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_mem_endianity)
#define	DSP_ELEMENT_SIZE(shared_mem,dsp_id)									(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_mem_element_size)
#define	DSP_MEM_OFFSET(shared_mem,dsp_id)									(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_shared_mem_offset)
#define	DSP_MODULE_NUM(shared_mem,dsp_id)									(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_num_of_modules)
#define DSP_MAILBOX_PTR(shared_mem,dsp_id,direction)						((&(DSP_INFO_PTR(shared_mem,dsp_id)->dsp_mailbox[direction])))
#define	DSP_MODULES_PTR(shared_mem,dsp_id)									(((&DSP_INFO_PTR(shared_mem,dsp_id)->module_info_ptr[0])))


//Module info
#define	MODULE_INFO_PTR(shared_mem,dsp_id, module_id)						((dsp_module_info_struct*)(CONVERT_ADDRESS_DSP_TO_HOST(shared_mem,dsp_id,DSP_INFO_PTR(shared_mem,dsp_id)->module_info_ptr[module_id])))
#define	MODULE_TYPE(shared_mem,dsp_id,module_id)							(((dsp_module_info_struct*)MODULE_INFO_PTR(shared_mem,dsp_id, module_id))->module_type)
#define	MODULE_AVAILABLE(shared_mem,dsp_id,module_id)						(((dsp_module_info_struct*)MODULE_INFO_PTR(shared_mem,dsp_id, module_id))->module_available)
#define	MODULE_MAILBOX_PTR(shared_mem,dsp_id,module_id,direction)			((&(((dsp_module_info_struct*)MODULE_INFO_PTR(shared_mem,dsp_id, module_id))->module_mailbox[direction])))
#define	MODULE_DATA_PTR(shared_mem,dsp_id,module_id)						((&(((dsp_module_info_struct*)MODULE_INFO_PTR(shared_mem,dsp_id, module_id))->module_data_buf)))

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_SHARED_MEM_COMMON_H



