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
 * ceva_inter_process_db.h
 *
 *  Created on: Oct 13, 2013
 *      Author: idor
 */
#ifndef WIN32
#include <semaphore.h>
#endif
#include <ceva/ceva_linkdrv_shared_process_protected_db_def.h>

#ifndef CEVA_INTER_PROCESS_DB_H_
#define CEVA_INTER_PROCESS_DB_H_


/*!
 * This class is a process-safe interface to a global variable database
 * towards the LINK layer. Because Android does not support sys/v ipc,
 * the actual shared memory is managed using a miscellaneous driver
 * (allocation, synchronization).
 * It is a singleton class (accessible from anywhere in the code).
 * The DB structure is defined in the \b ceva_process_db_def.h file, which is shared by
 * the LINK library and the shared memory driver. During compilation, the \b
 * ceva_process_db_def.h file is copied to the shared memory driver directory.
 * For each data field, there are get and set methods to access the data.
 * The data access is synchronized and protected (process/threadwise)
 * by the driver, which locks an internal semaphore for each read/write
 * operation. During the class instantiation, it opens the driver for
 * read/write/lock/unlock operations.
 * For each field within the class, the get and set methods must be implemented,
 * both for implementing locking of the semaphore, and accessing the shared memory
 * to get/set each field value.
 *
 */

class CevaInterProcessDB
{

private:
	/*!
	 * @brief Creates a CevaInterProcessDB object
	 *
	 * This constructor is empty. The initialization work is done in the instance function
	 * and in the init of the shared memory driver, which should be loaded
	 * before calling any function of this class.
	 * Because this class is a singleton, this constructor is private and is called only once per process.
	 */
	CevaInterProcessDB();
	/*!
	 * @brief Destroys a CevaInterProcessDB object
	 *
	 * This destructor only destroys the object.
	 * Because this class is a singleton, this destructor is private.
	 */
	~CevaInterProcessDB();
	/*!
	 * @brief Helps getting fields in the database
	 *
	 * This function reads from the shared memory driver.
	 * The driver read function locks a mutex if it did not get a lock
	 * ioctl from the same process previously. It then reads DummySize bytes of data from a
	 * preallocated memory region with DummyOffest offset, and copies the data to the
	 * value parameter. When this is completed, the driver unlocks the mutex only if it was
	 * locked in the same operation.
	 *
	 * @param [out] value Value of the dummy field read from the shared
	 * memory
	 * @param [in] DummyOffset Offset of the dummy field from the start of the database structure, in
	 * bytes
	 * @param [DummySize] Size of dummy field, in bytes
	 *
	 * @return The number of bytes read for success (negative values indicate errors)
	 */
	int getHelper(void *value, unsigned long offset,
	              unsigned int size);
	/*!
	 * @brief Helps setting fields in the database
	 * This function writes from the shared memory driver.
	 * The driver write function locks a mutex if it did not get a lock
	 * ioctl from the same process previously. It then writes DummySize bytes of data to a
	 * preallocated memory region with DummyOffest offset.
	 * When this is completed, the driver unlocks the mutex only if it was
	 * locked in the same operation.
	 *
	 * @param [in] DummyOffest Offset of the dummy field from the start of the database structure,
	 * in bytes
	 * @param [in] DummySize Size of the dummy field, in bytes
	 * @param [in] data Reference to the data to be inserted into the dummy
	 * field in the shared memory
	 *
	 * @return The number of bytes written for success (negative values indicate errors)
	 */
	int setHelper(unsigned long offest,
	              unsigned int size, void *data);


public:
	/*!
	 * @brief Instantiates the shared memory object
	 *
	 * This function opens the shared memory miscellaneous driver if it is not
	 * already opened and returns an object reference.
	 *
	 * @return The object reference for success, NULL for failure
	 */
	static CevaInterProcessDB *instance();
	/*!
	 * @brief Destroys the object resources (if it exists)
	 *
	 * This function closes the file descriptor of the shared memory driver.
	 */
	static void destroy();
	/*!
	 * @brief Reads the dummy field value from shared memory
	 *
	 * This function uses getDummyHelper() to read a specific field from
	 * the shared memory.
	 *
	 * @param [out] variable Reference to the stored read value
	 * @return The number of bytes read for success (negative values indicate errors)
	 */
	//int getDummy(int *);
	/*!
	 * @brief Sets the dummy shared object
	 *
	 * This function uses setDummyHelper() to write a specific field to the
	 * shared memory.
	 * @param [in] New value to be written
	 * @return The number of bytes written for success (negative values indicate errors)
	 */
	//int setDummy(int);
	/*!
	 * @brief Locks the shared memory
	 *
	 * This function triggers an ioctl to the driver.
	 * When the driver gets the ioctl, it locks a mutex in the kernel space.
	 * @return Zero for success, nonzero for failure
	 */
	int lock(void);
	/*!
	 * @brief Unlocks the shared memory
	 *
	 * This function triggers an ioctl to the driver.
	 * When the driver gets the ioctl, it unlocks a mutex in the kernel space.
	 * @return Zero for success, nonzero for failure
	 */
	int unlock(void);	

	int findFreeClient(void);
	int freeClient(int);
	int getDoInit(void);
	int setDoInit(int);

private:
	int _ref_cnt; /*!< Reference counter of the shared memory driver */
	static int _fd; /*!< File descriptor of the shared memory driver */
	static CevaInterProcessDB _instance;
};


#endif /* CEVA_INTER_PROCESS_DB_H_ */
