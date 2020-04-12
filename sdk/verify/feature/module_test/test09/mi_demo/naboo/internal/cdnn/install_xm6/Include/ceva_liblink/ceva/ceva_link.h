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
 * ceva_link.h
 *
 *  Created on: Oct 10, 2013
 *      Author: idor
 */

#ifndef CEVA_LINK_H_
#define CEVA_LINK_H_

#include <map>
#include <mt/mutex.h>
#include <mt/condition.h>
#include <ceva_link_api.h>
#include <ceva/ceva_device.h>
#include <ceva/ceva_device_pci.h>

/*!
 * CevaLink is the internal handler class that holds the module's resources
 * and states.
 * Because this class is the main user interface, it also holds a reference to the actual
 * CevaDevice instance.
 * This class is a singleton, which means that only one instance is allowed per
 * process,
 * and it is accessed using the instance static method.
 */
class CevaLink
{

private:
	/*
	 * Because this class implements the singleton pattern, its constructor and destructor are private.
	 */
	CevaLink();
	virtual ~CevaLink();

public:
	/*!
	 * @brief Returns the CevaLink object
	 *
	 * Because this class implements the singleton pattern,
	 * this function always returns a reference to the same object.
	 *
	 * @return The CevaLink object reference
	 */
	static CevaLink &instance();

public:
	/*!
	 * @brief Initializes a CevaLink object
	 *
	 * This function creates a CevaDevice object, and then allocates all of the
	 * designated buffers and communication channels.
	 * Because the module keeps track of how many times a single process has initialized it,
	 * the init() function can be called multiple times within the same process,
	 * and only closes after all of the initializations are de-initialized.
	 * @return A reference to the CevaLink object
	 */
	CevaLink &init();
	/*!
	 * @brief De-initializes a CevaLink object
	 *
	 * This function frees all allocated buffers, terminates its relative threads,
	 * and then closes opened devices.
	 * The CevaLink object holds a _refCount counter for each process
	 * (which is incremented upon init, and de-incremented upon deinit). When the
	 * _refCount reaches zero, the actual
	 * destruction occurs.
	 */
	void deinit();
	/*!
	 * @brief Waits on a device event
	 *
	 * This function can be called only on registered events.
	 *
	 * @param [in] event Event ID to wait on
	 * @param timeout Timeout, in milliseconds (zero for infinite)
	 * @return Zero for success, nonzero for failure
	 */
	int waitEvent(ceva_event, unsigned int timeout);
	/*!
	 *
	 * @brief Registers a callback to a specific event and
	 * context
	 *
	 * The event ID and context combination is unique.
	 * Only one callback can be registered for a specific event ID and
	 * context combination.
	 * @param [in] event Event ID of the registered event
	 * @param [in] callback Callback to be triggered when the event happens
	 * @param [in] context Context of the callback function
	 * @return Zero for success, nonzero for failure
	 */
	int registerCallback(ceva_event, ceva_event_cb &, void *);
	/*!
	 * @brief Unregisters a callback to a specific event and
	 * context
	 *
	 * The event ID and context combination is unique.
	 * Only one callback can be registered for a specific event ID and
	 * context combination.
	 * @param [in] event Event ID of the unregistered event
	 * @param [in] context Context of the registered callback
	 * @return Zero for success, nonzero for failure
	 */
	int unregisterCallback(ceva_event, void *);
	/*!
	 * @brief Handles any event coming from the device
	 *
	 * The handling is done by triggering all of the callbacks that are
	 * registered to the event. Different callbacks can be registered to
	 * the same event only if they have different contexts.
	 * This function also notifies all of the threads that are waiting on this event.
	 * @param [in] event Event ID to be handled
	 */
	void handleEvent(ceva_event);

	/*!
	 * @brief Gets an offset and returns a pointer
	 * to the place in the mapped memory that matches the offset
	 * @param [in] offset Offset to be translated to an address in the mapped memory
	 * @return A pointer to the mapped memory
	 */
	void *translateOffset(link_dev_mem_offset &);
	/*!
	 * @brief Creates a device interrupt
	 * @param [in] interrupt Interrupt descriptor
	 * @return Nonzero for success, zero for failure
	 */
	int generateInterrupt(ceva_int_descriptor &);
	/*!
	 * @brief Executes a read operation on a device
	 *
	 * This function triggers a data transfer from the device to the host.
	 * @param [in] offset Offset to be read from
	 * @param [out] buffer Buffer to be read into
	 * @param size Number of bytes to read
	 * @return The number of bytes read (negative values indicate errors)
	 */
	int read(link_dev_mem_offset, void *, link_dev_mem_size);
	/*!
	 * @brief Writes a buffer to a device
	 *
	 * This function triggers a data transfer from the host to the
	 * device.
	 * The data must be ready in the mapped memory area.
	 * The write operation updates the device memory in the correct offset and
	 * size.
	 * @param [in] offset Offset to write
	 * @param [in] buffer Buffer to be written to
	 * @param size Number of bytes to write
	 * @return The number of bytes written (negative values indicate errors)
	 */
	int write(link_dev_mem_offset, void *, link_dev_mem_size);
	/*!
	 * @brief Gets the debug level from the module
	 * @return The current debug level
	 */
	ceva_debug_level getDebugLevel();
	/*!
	 * @brief Sets the module's debug level
	 * @param [in] debug New debug level
	 * @return Zero for success, nonzero for failure
	 */
	int setDebugLevel(ceva_debug_level);
	/*!
	 * @brief Returns the debug information
	 * @param [out] debug_data Pointer to the debug_data structure
	 * @return Zero for success, nonzero for failure
	 */
	int getDebugInfo(ceva_debug *);

protected:
	/*!
	 *
	 * @return A reference to the device object
	 */
	CevaDevice &getDevice() const;
	/*!
	 *
	 * @return The current value of _refCount
	 */
	int getRefCount() const;

private:
	typedef std::pair<void *, ceva_event_cb> EventCallbackDescriptor;
	typedef std::multimap<ceva_event, EventCallbackDescriptor>::iterator
	eventCallbackIter;

	int _refCount; /*!< Counter for initialize requests */
	Mutex _mutex_cond;
	Mutex _mutex_callback; /*!< Internal lock object */
	/*!< Array for conditions that are waiting to be broadcast */
	Condition *_waiting[CEVA_LINK__NUM_OF_EVENTS][CEVA_LINK__NUM_OF_LINKS];
	std::multimap<ceva_event, EventCallbackDescriptor>
	_callbacks; /*!< Events callback container */
	CevaDevice *_device; /*!< Device interface */

	static CevaLink _instance;
};

#endif /* CEVA_LINK_H_ */
