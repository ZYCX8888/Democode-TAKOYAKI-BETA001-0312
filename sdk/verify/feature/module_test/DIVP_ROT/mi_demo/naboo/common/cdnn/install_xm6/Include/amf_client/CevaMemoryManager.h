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

/*
 * CevaMemoryManager.h
 *
 *  Created on: Dec 20, 2013
 *      Author: nirg
 */

#ifndef _CEVAMEMORYMANAGER_H_
#define _CEVAMEMORYMANAGER_H_

#include "ceva_typedef.h"
#include "ceva_log.h"

#define MAX_SUPPORTED_BUFFERS			100

namespace CEVA_AMF {


class CevaMemoryManager
{

typedef struct 
{
	void	*start_addr;
	U32	size;
}mem_buf_struct;

private:
	mem_buf_struct	m_sRegistry[MAX_SUPPORTED_BUFFERS+2];//2 support markers
	U32				m_nUsedBufNum;

public:
	void	Init(void *start_addr, U32 size);
	void*	AllocateBuffer(U32 size, U32 alignValue = 1);
	void	FreeBuffer(void *addr);
};

}
#endif
