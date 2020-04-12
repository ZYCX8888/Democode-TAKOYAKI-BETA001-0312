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

#ifndef _DSP_COMMAND_STRUCT_H
#define	_DSP_COMMAND_STRUCT_H

/*! \file ceva_dsp_command_struct.h
\brief Contains the CEVA-Link command structure type definition
*/

#include	"ceva_typedef.h"
#include	"ceva_dsp_and_modules_types.h"

#define		MAX_PAYLOAD_SIZE		128
#define		EVENT_COMP_RECEIVE		15 //(CEVA_LINK__NUM_OF_EVENTS-1)

/**
* \brief The host-to-DSP command type enumeration (HOST -> DSP)
*/
typedef enum
{	
	E_MAKE_LINK_CMD = 0,	/**< Create link */	
	E_UNLINK_CMD,			/**< Terminate link */
	E_SOFT_RESET,			/**< Perform soft reset */
	E_GET_DSP_STATUS,		/**< Get status */
	E_NUM_OF_BASE_HOST_COMMANDS
}host_dsp_cmd_type_enum;


/**
* \brief The DSP-to-host command type enumeration (sent from the DSP module to host)
*/
typedef enum
{
	E_EVENT_LINK_DONE = 0,	/**< Make link done */
	E_EVENT_LINK_ERROR,		/**< Make link encountered error */
	E_EVENT_UNLINK_DONE,	/**< Unlink done */
	E_EVENT_UNLINK_ERROR,	/**< Unlink encountered error */
	E_NUM_OF_BASE_DSP_EVENTS
}dsp_host_cmd_type_enum;

/**
* \brief The DSP status request type enumeration
*/
typedef enum
{
	MIPS_USAGE,				/**< MIPS usage */
	MODULE_TYPES_IN_USE,	/**< Module types used */
	MIPS_DISABLED_MODULES,	/**< Modules disabled due to insufficient MIPS */
	MAX_RESPONSE_TIME		/**< Module response time */
}param_id_enum;

/**
* \brief The DSP status request payload structure
*/
typedef struct
{
	param_id_enum			param_id;	/**< The parameter ID */
}dsp_get_status_payload_struct;

/**
* \brief The DSP link command payload structure
*/
typedef struct
{
	dsp_module_type_enum	module_type;				/**< The module type */
	U32						module_id;					/**< The module ID */
	U32						client_id;					/**< The client ID */
}dsp_link_payload_struct, *p_dsp_link_payload_struct;

/**
* \brief The DSP status:"MIPS usage" payload structure (from DSP)
*/
typedef struct
{
	U32						dsp_used_mips;				/**< The MIPS currently used by the DSP */
	U32						dsp_max_mips;				/**< The DSP's maximum MIPS capability */
}dsp_status_mips_payload_struct;

/**
* \brief The main payload structure
*/
//typedef struct
//{
//	param_id_enum			param_id;	/**< Parameter ID */
//	union
//	{
//		dsp_link_payload_struct			link_cmd;										/**< The link command data */
//		dsp_status_mips_payload_struct	status_mips;									/**< The DSP status:"MIPS usage" data */
//		U32								status_working_modules[NUM_OF_MODULE_TYPES];	/**< The working module type's data: 1 = Module of this type is in use; 0 = otherwise */
//		U32								status_mips_disabled_modules[MAX_DSP_MODULES];	/**< The modules disabled due to MIPS: 1 = Module is available MIPS-wise; 0 = Module is not available; -1 = Module does not exist for this DSP*/
//	}data;								/**< The payload data */
//}dsp_payload_struct;

/**
* \brief The basic command structure
*/
typedef struct
{
	U32						msg_id;							/**< The message ID (the command type itself) */
	U32						cmd_cnt;						/**< The counter (mainly for debugging) */
	U32						payload[MAX_PAYLOAD_SIZE];		/**< The message payload */
}cmd_struct;


#define	CMD_MSG_ID(cmd_msg)									((cmd_msg)->msg_id)
#define CMD_CNT(cmd_msg)									((cmd_msg)->cmd_cnt)
#define	CMD_PAYLOAD_PTR(cmd_msg)							((cmd_msg)->payload)

//defines for link command payload
#define DSP_CMD_MODULE_TYPE(payload)						((payload)->module_type)
#define DSP_CMD_MODULE_ID(payload)							((payload)->module_id)
#define DSP_CMD_CEVA_CLIENT_ID(payload)						((payload)->client_id)

//defines for status command payload
#define DSP_CMD_PARAM_ID(payload)							((payload)->param_id)	//use with CMD_PAYLOAD_PTR
//***The following should be used with CMD_STATUS_PAYLOAD_PTR:
#define DSP_CMD_USED_MIPS(payload)							((payload)->data.status_mips.dsp_used_mips)
#define DSP_CMD_MAX_MIPS(payload)							((payload)->data.status_mips.dsp_max_mips)
#define DSP_CMD_WORKING_MODULES(payload,module_type)		((payload)->data.status_working_modules[module_type])
#define DSP_CMD_MIPS_DISABLED_MODULES(payload,module_id)	((payload)->data.status_mips_disabled_modules[module_id])

#endif



