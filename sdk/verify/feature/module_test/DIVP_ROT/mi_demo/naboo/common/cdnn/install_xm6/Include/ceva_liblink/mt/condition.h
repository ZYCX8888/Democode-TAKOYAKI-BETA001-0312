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
 * condition.h
 *
 *  Created on: Oct 9, 2013
 *      Author: idor
 */

#ifndef CONDITION_H_
#define CONDITION_H_

#include <pthread.h>
#include <mt/mutex.h>
#include <time.h>

class Mutex;

/*!
 * This class implements a condition variable mechanism.
 * The class holds a pthread mutex and a pthread condition variable object.
 * The mutex and the condition object are initialized in the  constructor.
 */
class Condition
{
public:
	/*!
	 * @brief Constructor
	 */
	Condition(Mutex &mux);
	/*!
	 * @brief Destructor
	 */
	virtual ~Condition();

	/*! @brief Waits on the conditional variable
	 *
	 * @param timeout The timeout to wait (zero for indefinite)
	 * @return Zero for success, positive values for timeout (negative values indicate errors)
	 */
	int wait(unsigned int timeout);
	/*!
	 * @brief Unblocks one of the threads
	 * that are blocked on the specified condition
	 * @return Zero for success, nonzero for failure
	 */
	int signal();
	/*!
	 * @brief Unblock all of the threads that are currently blocked on the specified condition variable cond.
	 * @return Zero for success, nonzero for failure
	 */
	int broadcast();

private:
	Mutex &_mutex;
	pthread_cond_t _cond;
};

#endif /* CONDITION_H_ */
