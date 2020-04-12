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
 * ceva_device.h
 *
 *  Created on: Oct 10, 2013
 *      Author: idor
 */

#ifndef CEVA_DEVICE_H_
#define CEVA_DEVICE_H_

#include <ceva_link_api.h>
#include <mt/mutex.h>
/*!
 * The interface to the CEVA device's memory operations
 *
 * This is an abstract class that interfaces the memory operation
 * over the CEVA device.
 * The class's purpose is to link the user's space and the CEVA device
 * driver (whichever is available).
 */
class CevaDevice
{
public:
	/*!
	 * @brief Creates a CevaDevice object
	 */
	CevaDevice();
	/*!
	 * @brief Destroys a CevaDevice object
	 */
	virtual ~CevaDevice();
	/*!
	 * @brief Returns a pointer to the mapped DMA memory in the requested offset
	 *
	 * This function gets an offset and returns a pointer
	 * to the place in the mapped memory with the requested offset.
	 * @param [in] offset Offset to be translated to an address in the
	 * mapped memory
	 * @return A pointer to the mapped memory
	 */
	virtual void *translateOffset(link_dev_mem_offset &) = 0;
	/*!
	 * @brief Creates a CEVA device interrupt
	 *
	 * This function triggers an interrupt to the driver by sending a
	 * designated ioctl with a specific interrupt descriptor.
	 *
	 * @param [in] interrupt Interrupt descriptor
	 * @return Nonzero for success, zero for failure
	 */
	virtual int generateInterrupt(ceva_int_descriptor &) = 0;
	/*!
	 * @brief Executes a read operation on the CEVA device
	 *
	 * This function triggers a data transfer from the CEVA device
	 * to the host by calling a read on the driver file descriptor.
	 * @param [in] offset Offset to read from
	 * @param [out] buffer Buffer to be read into
	 * @param size Number of bytes to read
	 * @return The number of bytes read (negative values indicate errors)
	 */
	virtual int read(link_dev_mem_offset, void *, link_dev_mem_size) = 0;

	/*!
	 * @brief Writes a buffer to the CEVA device
	 *
	 * This function triggers a data transfer from the host to the CEVA
	 * device by calling a write on the driver file descriptor.
	 * The data must be ready in the mapped memory area.
	 * The write operation updates the device memory in the correct offset and
	 * size.
	 * @param [in] offset Offset to write
	 * @param [in] buffer Buffer to be written
	 * @param size Number of bytes to write
	 * @return The number of bytes written (negative values indicate errors)
	 */
	virtual int write(link_dev_mem_offset, void *, link_dev_mem_size) = 0;
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
	virtual int getDebugInfo(ceva_debug *) = 0;

protected:
	Mutex _mutex;
	ceva_debug_level _debugLevel;
};

#endif /* CEVA_DEVICE_H_ */
