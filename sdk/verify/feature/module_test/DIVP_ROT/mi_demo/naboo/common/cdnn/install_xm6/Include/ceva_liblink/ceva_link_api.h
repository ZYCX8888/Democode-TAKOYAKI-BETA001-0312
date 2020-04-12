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
 * link.h
 *
 *  Created on: Oct 3, 2013
 *      Author: Ido Reis <ido.reis@tandemg.com>
 */

/*! \file
*   This file describes the CEVA-Link API
*/


//TODO: copyright and legal stuff
#ifndef _CEVA_LINK_API_H_
#define _CEVA_LINK_API_H_

/*!
* \addtogroup CevaLink CEVA-Link
*  @{
* \brief Low-level interface for controlling the CEVA device
* \details This library is used by the CevaLinkClient to establish connection to the core as well as to send and receive messages.
*
*/

#ifdef __cplusplus
extern "C" {
#endif



#define BUFFER_SIZE	(0x1000)

typedef	struct 
{
	char	input_buf[BUFFER_SIZE];
	char	output_buf[BUFFER_SIZE];
}shared_dma_region;

/*!
 * @brief The link module handle (encapsulated) */
struct ceva_link;
typedef struct ceva_link ceva_link;

typedef unsigned long ceva_int_descriptor;

/*!
 * @brief The event descriptor type
 */
typedef unsigned long ceva_event_type;
typedef unsigned long ceva_link_id;

typedef unsigned long ceva_event;

#define extract_ceva_event_type(ceva_event) \
	((ceva_event_type) ((ceva_event) & 0xffff))

#define extract_ceva_link_id(ceva_event) \
	((ceva_link_id) ((ceva_event >> 16) & 0xff))


/*!
 * @brief The maximum number of supported links
 */
#define CEVA_LINK_MAX_SUPPORTED_LINKS	(16)

/*!
 * @brief The maximum number of supported events
 */
#define CEVA_LINK__NUM_OF_EVENTS	(16)

#define CEVA_LINK__NUM_OF_LINKS	(16)



/*!
 * @brief The callback function for ceva_event
 *
 * When the registered event occurs, the callback will be executed from the
 * ceva_link context.
 * @param [in] link Handle to a ceva_link object
 * @param [in] event ceva_event that occurred
 * @param [in] context Additional registered argument
 */
typedef void (*ceva_event_cb)(ceva_link *link, ceva_event event_id,
                              void *context);

/*!
 * @brief The debug info structure
 */
struct ceva_debug_info_s{
	unsigned long int bypass_interrupts_success;
	unsigned long int bypass_interrupts_failed;
	unsigned long int events_recieved;
	unsigned long int fifo_full;
	unsigned long int generated_interrupts;
	unsigned long int generate_interrupt_failures;
	unsigned long int unhandled_irq;
	unsigned long int handled_irq;
};


/*!
 * @brief The debug info structure
 */
typedef union ceva_debug {
	struct ceva_debug_info_s data;
	unsigned long int raw[250];
} ceva_debug;

/*!
 * @brief The enumeration to get/set debug level
 */
typedef enum {
	CEVA_DEBUG_DISABLED = 0, /*!< Debug disabled */
	CEVA_DEBUG_LEVEL_1, /*!< Low debug level (error) */
	CEVA_DEBUG_LEVEL_2, /*!< Medium debug level (warnings, info) */
	CEVA_DEBUG_LEVEL_3 /*!< Highest debug level (debug, trace) */
} ceva_debug_level;

/*!
 * @brief The device memory offset
 *
 * This defines the device memory offset type.
 */
typedef unsigned int link_dev_mem_offset;

/*!
 * @brief The device memory operation size type
 *
 * This defines the type for memory operation sizes (used for read/write operations).
 */
typedef unsigned int link_dev_mem_size;

/*!
 * @brief The interrupt descriptor bit-fields
 *
 */
typedef struct
{
	unsigned int dsp_id : 8;     //!< DSP ID
	unsigned int mailbox_id : 8; //!< ID of the mailbox associated with this interrupt
	unsigned int type : 8;       //!< Event type
	unsigned int reserved : 8;   //!< Reserved
} ceva_int_descriptor_fields;

/*!
 * @brief The interrupt descriptor type (unsigned 32-bit)
 *
 */
typedef union
{
	unsigned long				descriptor; //!< Overlay of an unsigned 32-bit integer
	ceva_int_descriptor_fields	fields;     //!< Descriptor bit-field structure
} ceva_union_descriptor;

/*!
 * @brief Associates an event with a link to create a unique descriptor
 *
 * ceva_event is an association of a specific event with a specific link ID.
 * This utility macro combines the two into a unique descriptor to
 * use it when handling events from the device.
 * @param [in] event CEVA device mailbox event
 * @param [in] link Link ID
 * @return The combined event/link descriptor
 * @see ceva_link_wait_event(), ceva_link_register_event_cb(),
 * ceva_link_unregister_event_cb()
 */
#define ceva_link_generate_ceva_event(event, link) \
	((ceva_event) ((((unsigned long int)(event)) & 0x0000ffff) | (((unsigned long int)(link)) << 16)))
	

/*!
 * @brief Initializes the library and shared memory module
 *
 * This function opens the associated character device and allocates all of the
 * designated buffers and communication channels using the SHMM interface.
 * The returned ceva_link handler is used in all of the other functionalities.
 * Because the module keeps track of how many times a single process has initialized it,
 * the init function can be called multiple times within the same process,
 * and only closes down after it was deinitialized the same number of times
 * (a reference counter is used).
 * @return Nonzero for success, NULL for error
 * @note This is a blocking function, and is thread- and process-safe.
 * @see ceva_link_deinit()
 */
ceva_link *ceva_link_init();

/*!
 * @brief Deinitializes the library and shared memory module
 *
 * This function is responsible for module deinitialization:
 * - Frees all allocated buffers
 * - Terminates its relative threads
 * - Closes opened devices
 * The library holds a counter for each process, which increments upon an init call
 * and decreases upon a deinit call. When the counter reaches zero, the actual
 * deinitialization occurs.
 * 
 * @param [in] link Handle to a ceva_link object
 * @return Zero for success, nonzero for failure
 * @note This is a blocking function, and is thread- and process-safe.
 * @see ceva_link_init()
 */
int ceva_link_deinit(ceva_link *link);

/*!
 * @brief Translates the offset address to a pointer
 *
 * This function returns a pointer to a given offset within the CEVA device.
 * @param [in] link Handle to a ceva_link object
 * @param [in] offset Offset to translate
 * @return NULL for error, a valid pointer otherwise
 * @note This is a blocking function, and is thread- and process-safe.
 */
void *ceva_link_translate_offset(ceva_link *link,
                                 link_dev_mem_offset offset);

/*!
 * @brief Generates an interrupt from the CEVA device to the host
 * @param [in] link Handle to a ceva_link object
 * @param [in] id Interrupt descriptor
 * @return Zero for success, nonzero for error
 * @note This is a blocking function, and is thread- and process-safe.
 */
int ceva_link_generate_interrupt(ceva_link *link,
                                 ceva_int_descriptor id);

/*!
 * @brief Waits for a specific event from the CEVA device
 *
 * This function blocks the caller process either until a timeout or a ceva_event occurs on the CEVA device.
 * @param [in] link Handle to a ceva_link object
 * @param [in] event_id Event ID to wait on
 * @param [in] timeout Timeout, in milliseconds (zero for infinite)
 * @return Zero for success, nonzero for failure
 * @see ceva_link_register_event_cb()
 */
int ceva_link_wait_event(ceva_link *link, ceva_event event_id,
                         unsigned int timeout);

/*!
 * @brief Registers a callback function to a specific ceva_event
 *
 * This function registers a callback function that will be called upon a ceva_event.
 * @param [in] link Handle to a ceva_link object
 * @param [in] event_id Event ID to wait on
 * @param [in] cb Callback function
 * @param [in] context Context argument that will be passed to the callback
 * function
 * @return Zero for success, nonzero for failure
 * @note The callback function will be executed from the ceva_link's own
 * internal thread.
 * @note  If the callback is NULL, this function is identical to the unregister_event function.
 * @note This function is thread-safe.
 * @see ceva_link_unregister_event_cb(), ceva_link_wait_event()
 */
int ceva_link_register_event_cb(ceva_link *link, ceva_event event_id,
                                ceva_event_cb cb, void *context);

/*!
 * @brief Unregisters the callback from a specific ceva_event
 *
 * @param [in] link Handle to a ceva_link object
 * @param [in] event_id Event ID to unregister from
 * @param [in] context Context that was registered with the callback
 * @return Zero for success, nonzero for failure
 * @note This function is thread-safe.
 * @see ceva_link_register_event_cb()
 */
int ceva_link_unregister_event_cb(ceva_link *link, ceva_event event_id,
                                  void *context);

/*!
 * @brief Executes a read operation from the CEVA device
 *
 * This function reads a buffer from the CEVA device's PCIe memory region.
 * It blocks until the read operation is finished (synchronized).
 * @param [in] link Handle to a ceva_link object
 * @param [in] offset Offset to read from
 * @param [out] buf Buffer to receive data from
 * @param [in] size Number of bytes to read
 * @return The number of bytes read (negative values indicate errors)
 * @note The buffer must be preallocated, and its size must be equal to or less than the
 * number of bytes to be read.
 * @see ceva_link_write()
 */
int ceva_link_read(ceva_link *link, link_dev_mem_offset offset,
                   void *buf, link_dev_mem_size size);

/*!
 * @brief Executes a write operation to the CEVA device
 *
 * This function writes a buffer to the CEVA device's PCIe memory region.
 * It blocks until the write operation is finished (synchronized).
 * @param [in] link Handle to a ceva_link object
 * @param [in] offset Offset to write to
 * @param [in] buf Data to be written to the PCIe memory region
 * @param [in] size Number of bytes to write
 * @return The number of bytes written (negative values indicate errors)
 * @see ceva_link_read()
 */
int ceva_link_write(ceva_link *link, link_dev_mem_offset offset,
                    void *buf, link_dev_mem_size size);

/*!
 * @brief Gets the debug level
 *
 * This function gets the debug level from the module.
 * @param [in] link Handle to a ceva_link object
 * @return The current debug level
 * @note This is a blocking function, and is thread- and process-safe.
 * @see ceva_link_set_debug_level(), ceva_link_dump_debug()
 */
ceva_debug_level ceva_link_get_debug_level(ceva_link *link);

/*!
 * @brief Sets the module's debug level
 *
 * If the debug level is not CEVA_DEBUG_DISABLED, the user can pass an optional
 * FILE handler to which debug logs and information will be written to during the
 * module's lifetime (until it is disabled).
 * @param [in] link Handle to a ceva_link object
 * @param [in] debug New debug level
 * @return Zero for success, nonzero for failure
 * @see ceva_link_get_debug_level(), ceva_link_dump_debug()
 */
int ceva_link_set_debug_level(ceva_link *link, ceva_debug_level debug);

/*!
 * @brief Retrieves the debug information
 * @param [in] link Handle to a ceva_link object
 * @param [out] debug_data Pointer to a debug_data structure
 * @return Zero for success, nonzero for failure
 */
int ceva_link_dump_debug(ceva_link *link, ceva_debug *debug_data);

#ifdef __cplusplus
}
#endif

/** @}*/
#endif /* LINK_H_ */
