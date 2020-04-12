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

#ifndef _CEVA_TYPEDEF_H
#define	_CEVA_TYPEDEF_H


#include <stdio.h>
#include <string.h>

//#warning This is a bypass for redefinition issues.
#ifndef __INV_DEFINES_H__
#if	!defined(SHARED_TYPES_H)
typedef signed char		S8;
typedef unsigned char	U8;
typedef unsigned short 	U16;
typedef short		   	S16;
typedef unsigned long  	U32;
typedef long  			S32;
#endif //!defined(SHARED_TYPES_H)

#ifndef WIN32
#if !defined TRUE
typedef enum BOOL
{
	FALSE = 0,
	TRUE = 1
} BOOL;
#endif // !defined TRUE
#endif //WIN32

#define CLIPU8(a) MIN(MAX(0,a),255)
#define CLIPS8(a) MIN(MAX(-128,a),127)
#define CLIPU16(a) MIN(MAX(0,a),65535)
#define CLIPS16(a) MIN(MAX(-32768,a),32767)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#define ABS(a)	 ((a)>(0) ? (a) : (-(a)))


#endif // !_INV_DEFINES_H__

#define		MAX_MAILBOX_SIZE			8    // The maximum number of messages in one mailbox
#define		MAILBOX_SIZE_MASK			(MAX_MAILBOX_SIZE - 1)
#define		NUM_OF_DSPS					1    // The maximum number of DSPs that will be implemented on the board
#define		MAX_DSP_MODULES				10   // The maximum number of modules in one DSP
#define		MAX_CEVA_COMPONENTS			10   // The maximum number of CEVA components to be registered to the CEVA-Link
//#define		MAX_MODULE_BUFFER_SIZE	1024 // The maximum module buffer size
#define		NUM_MAILBOX_FOR_DSP			2    // Each DSP has two mailboxes: incoming and outgoing
#define		NOT_DEFINED					-1			

#define MEDIAN(a,b,c) ((a)+(b)+(c)-MIN(MIN(a,b),c)-MAX(MAX(a,b),c))
#define MAX_S16(a,b)  (((S16)(a))>((S16)(b)) ? (S32)(a) : (S32)(b))
#define MIN_S16(a,b)  (((S16)(a))<((S16)(b)) ? (S32)(a) : (S32)(b))
#define CLIP(x,min_val,max_val)	MIN(MAX((x),(min_val)),(max_val))

#endif //_CEVA_TYPEDEF_H

