/********************************************************************************************************************************************************
Copyright (C) CEVA(R) Inc. All rights reserved

This information embodies materials and concepts, which are proprietary and confidential to CEVA Inc., and is made available solely pursuant to the
terms of a written license agreement, or NDA, or another written agreement, as applicable (�CEVA Agreement�), with CEVA Inc. or any of its subsidiaries
(�CEVA�).

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
 * CevaConditionVariable.h
 *
 *  Created on: Jan 26, 2014
 *      Author: nirg
 */

#ifndef _CEVACONDITIONVARIABLE_H_
#define _CEVACONDITIONVARIABLE_H_

/*! \file CevaConditionVariable.h
\brief This file contains the CEVA wrapper class for a conditional variable.
*/

#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif
#include <errno.h>

namespace CEVA_AMF {


/** This structure contains the conditional variable value, mutex, and green-light flag.
*/
class CevaConditionVariable
{
public:
	CevaConditionVariable() { isInitialized = false; flag=false;};
	
	/** Initializes the conditional variable at a given value*/
	int Init();

	/** Destroys the conditional variable*/
	void Deinit();

	/** Waits on the condition*/
	void WaitForEvent();

	/** Waits on the condition, with a timeout*/
	int WaitForEvent(unsigned int msWait);

	/** Signals the condition, if waiting	*/
	void SignalEvent();

	~CevaConditionVariable();

private:
#ifdef WIN32
	CONDITION_VARIABLE condition;
	CRITICAL_SECTION mutex;
#else
	pthread_cond_t condition;
	pthread_mutex_t mutex;
#endif
	bool	flag;
	bool 	isInitialized;
};

}
#endif /* _CEVACONDITIONVARIABLE_H_ */
