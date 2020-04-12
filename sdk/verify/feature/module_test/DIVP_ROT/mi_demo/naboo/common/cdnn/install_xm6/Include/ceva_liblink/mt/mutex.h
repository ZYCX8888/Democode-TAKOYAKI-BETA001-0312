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
 * mutex.h
 *
 *  Created on: Oct 9, 2013
 *      Author: idor
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include <pthread.h>

/*!
 * This class implements a mutex based on pthread_mutex_t.
 */
class Mutex
{
public:
	/*!
	 * FAST - Normal mutex
	 * RECURSIVE - Recursive mutex
	 *
	 */
	typedef enum {
		FAST, RECURSIVE
	} Type;

public:
	/*!
	 * @brief The default constructor
	 *
	 * The default mutex type is FAST.
	 */
	Mutex();
	/*!
	 * @brief Constructor
	 * @param [in] mutex The mutex type (default is FAST)
	 */
	Mutex(Mutex::Type);
	/*!
	 * @brief Destructor
	 */
	virtual ~Mutex();

	/*!
	 * @brief Locks a mutex object
	 * @return Zero for success, nonzero for failure
	 */
	int lock();
	/*!
	 * @brief Unlocks a mutex object
	 * @return Zero for success, nonzero for failure
	 */
	int unlock();

	/*!
	 * @brief Gets a pthread_mutex_t member object
	 * @return The pthread_mutex_t object
	 */
	pthread_mutex_t &getMutex();

private:
	pthread_mutex_t _mutex;
};

#endif /* MUTEX_H_ */
