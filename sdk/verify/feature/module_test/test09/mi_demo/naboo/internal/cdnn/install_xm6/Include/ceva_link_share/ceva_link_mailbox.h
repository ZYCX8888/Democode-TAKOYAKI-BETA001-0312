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


/*! \file ceva_link_mailbox.h
\brief Contains the CEVA-Link mailbox structure type definitions
*/

#ifndef _CEVA_LINK_MAILBOX_H_
#define	_CEVA_LINK_MAILBOX_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "ceva_typedef.h"
#include "ceva_dsp_command_struct.h"

/**
* \brief The error handler enumeration
*/
typedef enum
{
	E_MAILBOX_OK,		/**< The mailbox operates properly */
	E_MAILBOX_ERROR,	/**< Undefined error */
	E_MAILBOX_FULL,		/**< The mailbox is full */
	E_MAILBOX_EMPTY		/**< The mailbox is empty */
}ceva_link_mailbox_error_enum;


/**
* \brief The CEVA-Link mailbox structure
*/
typedef struct
{
	U32				head;						/**< The mailbox head (read location) */
	U32				tail;						/**< The mailbox tail (write location) */
	cmd_struct		cmd_msg[MAX_MAILBOX_SIZE];	/**< The mailbox data */
}ceva_link_mailbox_struct;


/*!
* \brief Initializes the mailbox structure
* @param  ceva_link_mailbox				Pointer to the mailbox
* @return ceva_link_mailbox_error_enum	The error handler enumeration
*/
ceva_link_mailbox_error_enum	ceva_link_mailbox_init(ceva_link_mailbox_struct *ceva_link_mailbox);

/*!
* \brief Puts a command at the end (tail) of the mailbox
* @param  ceva_link_mailbox				Pointer to the mailbox 
* @param  cmd_msg						Pointer to the command structure to be copied to the mailbox
* @return ceva_link_mailbox_error_enum	The error handler enumeration
*/
ceva_link_mailbox_error_enum	ceva_link_mailbox_put_data(ceva_link_mailbox_struct	*ceva_link_mailbox, cmd_struct *cmd_msg);

/*!
* \brief Puts a command with specific size at the end (tail) of the mailbox
* @param  ceva_link_mailbox				Pointer to the mailbox
* @param  cmd_msg						Pointer to the command structure to be copied to the mailbox
* @param  size							Size of the message (must be equal to or smaller than size of ceva_link_mailbox_struct)
* @return ceva_link_mailbox_error_enum	The error handler enumeration
*/
ceva_link_mailbox_error_enum	ceva_link_mailbox_put_data_with_size(ceva_link_mailbox_struct *ceva_link_mailbox, cmd_struct *cmd_msg, U32 size);

/*!
* \brief Gets a command from the beginning (head) of the mailbox
* @param  ceva_link_mailbox				Pointer to the mailbox
* @param  cmd_msg						Pointer to the command structure to fill in from the mailbox
* @return ceva_link_mailbox_error_enum	The error handler enumeration
*/
ceva_link_mailbox_error_enum	ceva_link_mailbox_get_data(ceva_link_mailbox_struct	*ceva_link_mailbox, cmd_struct *cmd_msg);

/*!
* \brief Gets a command with a specific size from the beginning (head) of the mailbox
* @param  ceva_link_mailbox				Pointer to the mailbox
* @param  cmd_msg						Pointer to the command structure to fill in from the mailbox
* @param  size							Size of the message (must be equal to or smaller than size of ceva_link_mailbox_struct)
* @return ceva_link_mailbox_error_enum	The error handler enumeration
*/
ceva_link_mailbox_error_enum	ceva_link_mailbox_get_data_with_size(ceva_link_mailbox_struct *ceva_link_mailbox, cmd_struct *cmd_msg, U32 size);


#ifdef __cplusplus
}
#endif

#endif



