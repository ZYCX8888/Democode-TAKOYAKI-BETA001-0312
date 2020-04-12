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
 * ceva_pci_device.h
 *
 *  Created on: Oct 10, 2013
 *      Author: idor
 */

#ifndef CEVA_PCI_DEVICE_H_
#define CEVA_PCI_DEVICE_H_

#include <mt/mutex.h>
#include <ceva/ceva_device.h>
#include <ceva/ceva_link.h>

/*!
 * This class is an instantiation of the CevaDevice interface.
 * It implements the abstract base class's methods to maintain an
 * actual CEVA device over a PCI connection.
 * This class uses the PCI character device driver to perform
 * memory access to the CEVA device.
 */
class CevaLink;

class CevaDevice_PCI: public CevaDevice
{
public:
	/*!
	 * @brief Creates a CevaDevice_PCI object
	 *
	 * This constructor initiates the CevaDevice_PCI object with the following:
	 * - A reference to the CevaLink that triggered its creation
	 * - Debug information
	 * - A file descriptor to the device
	 * - A file descriptor to the event procfs fd and mapping the driver memory (using mmap()) to have memory access to the device
	 * In addition, this constructor opens an internal thread that runs the
	 * internalEventThread() function. The thread is responsible for getting the events from the
	 * driver and calling handleEvent() for each one of them.
	 * @param [in] reference CevaLink object that initiated the constructor
	 */
	CevaDevice_PCI(CevaLink &);
	/*!
	 * @brief Destroys a CevaDevice_PCI object
	 *
	 * This destructor unmaps the driver memory, closes the driver file
	 * descriptor, closes the event procfs file descriptor, and then waits for the termination
	 * of the internal thread.
	 *
	 */
	virtual ~CevaDevice_PCI();

	/*!
	 * @see CevaDevice::translateOffset()
	 */
	virtual void *translateOffset(link_dev_mem_offset &);
	/*!
	 * @see CevaDevice::generateInterrupt()
	 */
	virtual int generateInterrupt(ceva_int_descriptor &);
	/*!
	 * @see CevaDevice::read()
	 */
	virtual int read(link_dev_mem_offset, void *, link_dev_mem_size);
	/*!
	 * @see CevaDevice::write()
	 */
	virtual int write(link_dev_mem_offset, void *, link_dev_mem_size);
	/*!
	 * @see CevaDevice::getDebugInfo()
	 */
	virtual int getDebugInfo(ceva_debug *);

private:
	/*!
	 * @brief The internal thread execution function
	 *
	 * This thread function is created during the construction.
	 * It contains an endless loop in which the thread is blocked upon read operation,
	 * and is not returned until at least one event is sent from the driver via the procfs
	 * file that each new process that calls cevaLink::init() has.
	 * After the read operation is returned, the thread calls the CevaLink::handleEvent() function
	 * for each new event that was read.
	 *
	 */
	static void *internalEventThread(void *context);

private:
	/*!
	 * A reference to the CevaLink object that triggered
	 * the creation of this CevaDevice_PCI object */
	CevaLink &_link;
	ceva_debug _debugInfo; /*!< All of the debug info from the driver */
	int _fd; /*!< Driver file descriptor */
	int _eventFd; /*!< procfs file descriptor */
	pthread_t _thread; /*!< Internal thread handle */
	void *_memory; /*!< DMA driver memory pointer */
	unsigned int _memLen; /*!< DMA driver memory length, in bytes */
};

#endif /* CEVA_PCI_DEVICE_H_ */
