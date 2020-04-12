/*****************************************************************************\
* CEVA Confidential property.
* Copyright (C) Ceva Inc. All rights reserved.
*
* This file constitutes proprietary and confidential information of CEVA Inc.
* Any use or copying of this file or any information contained in this file other
* than as expressly approved in writing by Ceva, Inc. is strictly prohibited.
* Any disclosure or distribution of this file or any information contained in
* this file except to the intended recipient is strictly prohibited.
\*****************************************************************************/

/**
@file mss.h
@brief CEVA-XM MSS (Memory Sub-System) driver
*/

#ifndef __MSS_H__
#define __MSS_H__

#include "cevaxm.h"
#include "cpm_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @addtogroup MSS_DRIVER MSS Driver 
* @{
* @brief CEVA-XM MSS (Memory Sub-System) driver
* @details
*    The MSS driver is used to configure the XM6 Memory Sub-System.<BR>
*    With this driver, developers can configure or use the following features:
*    - Change the program cache attributes
*    - Copy program data to the internal program memory using the PDMA (Program DMA)
*    - Query the hardware configuration (e.g. memory sizes)
*    - Read the clock counter value
*    - Change the core's power saving modes
* @}
*/

#if defined XM4
#include "xm4/mss_reg.h"
#elif defined XM6
#include "xm6/mss_reg.h"
#else
//#error Unsupported platform
#endif

/**
* @addtogroup MSS_DRIVER_TYPES MSS Driver Data Types
* @ingroup MSS_DRIVER
* @{
*/

/**
 * @brief Program memory access protection enable <BR>
 */
typedef enum
{
    MSS_PROGRAM_ACCESS_PROTECTION_DISABLE = 0,    /**< Access protection checking disabled */
	MSS_PROGRAM_ACCESS_PROTECTION_ENABLE          /**< Access protection checking enabled */
} mss_program_access_protection_e;

/**
 * @brief Internal program memory enable
 */
typedef enum
{
	MSS_INTERNAL_PROGRAM_MEMORY_DISABLE = 0,     /**< Internal program memory is disabled */
	MSS_INTERNAL_PROGRAM_MEMORY_ENABLE           /**< Internal program memory is enabled */
} mss_internal_program_memory_enable_e;

/**
 * @brief CACHE pre-fetch enable
 */
typedef enum
{
    MSS_CACHE_PREFETCH_DISABLE = 0,   /**< Cache pre-fetch is disabled */
	MSS_CACHE_PREFETCH_ENABLE         /**< Cache pre-fetch is enabled */
} mss_cache_prefetch_enable_e;


/**
 * @brief Memory regions enumeration<BR>In program memory only 16 memory regions are available
 */
typedef enum
{
    MSS_MEMORY_REGION_0 = 0x0,              /**< Memory region number 0. */
	MSS_MEMORY_REGION_1 = 0x10,             /**< Memory region number 1. */
	MSS_MEMORY_REGION_2 = 0x20,             /**< Memory region number 2. */
	MSS_MEMORY_REGION_3 = 0x30,             /**< Memory region number 3. */
	MSS_MEMORY_REGION_4 = 0x40,             /**< Memory region number 4. */
	MSS_MEMORY_REGION_5 = 0x50,             /**< Memory region number 5. */
	MSS_MEMORY_REGION_6 = 0x60,             /**< Memory region number 6. */
	MSS_MEMORY_REGION_7 = 0x70,             /**< Memory region number 7. */
	MSS_MEMORY_REGION_8 = 0x80,             /**< Memory region number 8. */
	MSS_MEMORY_REGION_9 = 0x90,             /**< Memory region number 9. */
	MSS_MEMORY_REGION_10 = 0xA0,            /**< Memory region number 10. */
	MSS_MEMORY_REGION_11 = 0xB0,            /**< Memory region number 11. */
	MSS_MEMORY_REGION_12 = 0xC0,            /**< Memory region number 12. */
	MSS_MEMORY_REGION_13 = 0xD0,            /**< Memory region number 13. */
	MSS_MEMORY_REGION_14 = 0xE0,            /**< Memory region number 14. */
	MSS_MEMORY_REGION_15 = 0xF0,            /**< Memory region number 15. */
	MSS_MEMORY_REGION_16 = 0x100,           /**< Memory region number 16 - Data only. */
	MSS_MEMORY_REGION_17 = 0x110,           /**< Memory region number 17 - Data only. */
	MSS_MEMORY_REGION_18 = 0x120,           /**< Memory region number 18 - Data only. */
	MSS_MEMORY_REGION_19 = 0x130,           /**< Memory region number 19 - Data only. */
	MSS_MEMORY_REGION_20 = 0x140,           /**< Memory region number 20 - Data only. */
	MSS_MEMORY_REGION_21 = 0x150,           /**< Memory region number 21 - Data only. */
	MSS_MEMORY_REGION_22 = 0x160,           /**< Memory region number 22 - Data only. */
	MSS_MEMORY_REGION_23 = 0x170,           /**< Memory region number 23 - Data only. */
	MSS_MEMORY_REGION_24 = 0x180,           /**< Memory region number 24 - Data only. */
	MSS_MEMORY_REGION_25 = 0x190,           /**< Memory region number 25 - Data only. */
	MSS_MEMORY_REGION_26 = 0x1A0,           /**< Memory region number 26 - Data only. */
	MSS_MEMORY_REGION_27 = 0x1B0,           /**< Memory region number 27 - Data only. */
	MSS_MEMORY_REGION_28 = 0x1C0,           /**< Memory region number 28 - Data only. */
	MSS_MEMORY_REGION_29 = 0x1D0,           /**< Memory region number 29 - Data only. */
	MSS_MEMORY_REGION_30 = 0x1E0,           /**< Memory region number 30 - Data only. */
	MSS_MEMORY_REGION_31 = 0x1F0            /**< Memory region number 31 - Data only. */
} mss_mem_region_e;

/**
 * @brief Memory region active state
 */
typedef enum
{
    MSS_MEMORY_REGION_ACTIVE = 0,      /**< The region is active. */
	MSS_MEMORY_REGION_INACTIVE         /**< This region is in active. */
} mss_mem_region_active_e;

/**
 * @brief PMSS Memory region cache ability attribute
 */
typedef enum
{
    MSS_PROGRAM_MEM_REGION_CACHE_DISABLE = 0,   /**< Not cacheable in L1LC */
	MSS_PROGRAM_MEM_REGION_CACHE_ENABLE         /**< Cacheable in L1LC */
} mss_program_mem_region_cacheability_e;

/**
 * @brief PMSS Memory region lock after cache-line fill attribute
 */
typedef enum
{
    MSS_PROGRAM_MEM_REGION_LOCK_AFTER_FILL_DISABLE = 0,   /**< No effect after cache-line fill */
	MSS_PROGRAM_MEM_REGION_LOCK_AFTER_FILL_ENABLE         /**< Lock after cache-line fill */
} mss_program_mem_region_lock_after_line_fill_e;

/**
 * @brief Data memory access protection
 *  Value |   Supervisor |    User0    |    User1
 *  ------|--------------|------------|----------
 *  0     | Read/Write   | Read/Write  | Read/Write
 *  1     | Read/Write   | Read/Write  | Read      
 *  2     | Read/Write   | Read        | Read       
 *  3     | Read/Write   | Read/Write  | No Access 
 *  4     | Read/Write   | No Access   | No Access 
 *  5     | Read         | Read        | Read      
 *  6     | Read         | No Access   | No Access 
 *  7     | No Access    | No Access   | No Access 
 *
 */
typedef unsigned int mss_access_protection_t;

/**
 * @brief AXI Cache Policy flags
 */
typedef enum {
    MSS_AXI_CACHE_POLICY_BUFFERABLE = 1,
    MSS_AXI_CACHE_POLICY_CACHEABLE = 2,
    MSS_AXI_CACHE_POLICY_READ_ALLOC = 4,
    MSS_AXI_CACHE_POLICY_WRITE_ALLOC = 8
} mss_axi_cache_policy_e;

/**
 * @brief L2 Cache Policy for read accesses - bit-mask of \link mss_axi_cache_policy_e \endlink values
 */
typedef unsigned int mss_mem_region_l2_cache_policy_t;

/**
* @brief Type of program cache operation
*/
typedef enum
{
	MSS_PROGRAM_CACHE_OPERATION_PREFETCH = 1,      /**< Prefetch */
	MSS_PROGRAM_CACHE_OPERATION_LOCK     = 2,      /**< Lock */
	MSS_PROGRAM_CACHE_OPERATION_UNLOCK   = 3,      /**< Unlock */
	MSS_PROGRAM_CACHE_OPERATION_INVALIDATE  = 4    /**< Invalidate */
} mss_program_cache_operation_type_e;

/**
* @brief Scope of program cache operation
*/
typedef enum
{
	MSS_PROGRAM_CACHE_OPERATION_SIZE_BY_ADDRESS = 0,   /**< Apply operation to the range specified by a start address and a number of lines */
	MSS_PROGRAM_CACHE_OPERATION_SIZE_FULL       = 1    /**< Apply operation to the entire cache */
} mss_program_cache_operation_size_e;

/**
* @brief Maximum size of AXI burst for DDMA 
*/
typedef enum
{
	MSS_DDMA_MAX_BURST_SIZE_DDTC     = 0x0,    /**< The maximum burst size is configured in the DDMA task (see the Data DMA Transfer Control register (MSS_DDTC))  */
	MSS_DDMA_MAX_BURST_SIZE_1_FIXED  = 0x1,    /**< 1 transfer with fixed address(FIXED).  */
	MSS_DDMA_MAX_BURST_SIZE_4_FIXED  = 0x2,    /**< 4 transfers(FIXED).  */
	MSS_DDMA_MAX_BURST_SIZE_8_FIXED  = 0x3,    /**< 8 transfers(FIXED).  */
	MSS_DDMA_MAX_BURST_SIZE_16_FIXED = 0x4,    /**< 16 transfers(FIXED).  */
	MSS_DDMA_MAX_BURST_SIZE_1_INCR   = 0x8,    /**< 1 transfer with incrementing addresses(INCR).  */
	MSS_DDMA_MAX_BURST_SIZE_4_INCR   = 0x9,    /**< 4 transfers(INCR).  */
	MSS_DDMA_MAX_BURST_SIZE_8_INCR   = 0xA,    /**< 8 transfers(INCR).  */
	MSS_DDMA_MAX_BURST_SIZE_16_INCR  = 0xB,    /**< 16 transfers(INCR).  */
	MSS_DDMA_MAX_BURST_SIZE_32_INCR  = 0xC,    /**< 32 transfers(INCR).  */
	MSS_DDMA_MAX_BURST_SIZE_64_INCR  = 0xD,    /**< 64 transfers(INCR) (AXI4 only).  */
	MSS_DDMA_MAX_BURST_SIZE_128_INCR = 0xE,    /**< 128 transfers(INCR) (AXI4 only).  */
	MSS_DDMA_MAX_BURST_SIZE_256_INCR = 0xF     /**< 256 transfers(INCR) (128 - bits wide AXI4 only).  */
} mss_data_mem_region_max_burst_size_e;

/**
* @brief Global limit for outstanding AXI read bursts in DDMA 
*/
typedef enum
{
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_4  = 0x0,    /**< DDMA Can issue up to 4 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_5  = 0x1,    /**< DDMA Can issue up to 5 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_6  = 0x2,    /**< DDMA Can issue up to 6 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_7  = 0x3,    /**< DDMA Can issue up to 7 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_8  = 0x4,    /**< DDMA Can issue up to 8 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_9  = 0x5,    /**< DDMA Can issue up to 9 outstanding AXI read bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_10 = 0x6,    /**< DDMA Can issue up to 10 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_11 = 0x7,    /**< DDMA Can issue up to 11 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_12 = 0x8,    /**< DDMA Can issue up to 12 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_13 = 0x9,    /**< DDMA Can issue up to 13 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_14 = 0xA,    /**< DDMA Can issue up to 14 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_15 = 0xB,    /**< DDMA Can issue up to 15 outstanding AXI read bursts  */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_READ_16 = 0xC     /**< DDMA Can issue up to 16 outstanding AXI read bursts  */
} mss_ddma_global_max_outstanding_read_e;

/**
* @brief Global limit for outstanding AXI write bursts in DDMA (when using EDP)
*/
typedef enum
{
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_WRITE_4  = 0x0,    /**< DDMA Can issue up to 4 outstanding AXI write bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_WRITE_8  = 0x1,    /**< DDMA Can issue up to 8 outstanding AXI write bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_WRITE_16 = 0x2,    /**< DDMA Can issue up to 16 outstanding AXI write bursts   */
	MSS_DDMA_GLOBAL_MAX_OUTSTANDING_WRITE_32 = 0x3     /**< DDMA Can issue up to 32 outstanding AXI write bursts   */
} mss_ddma_global_max_outstanding_write_e;

/**
* @brief Limit for outstanding AXI read bursts in DDMA for a memory region
*/
typedef enum
{
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_USE_GLOBAL = 0x0,    /**< The limit is taken from DOL field at MSS_DACC register   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_2          = 0x1,    /**< DDMA Can issue up to 2 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_3          = 0x2,    /**< DDMA Can issue up to 3 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_4          = 0x3,    /**< DDMA Can issue up to 4 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_5          = 0x4,    /**< DDMA Can issue up to 5 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_6          = 0x5,    /**< DDMA Can issue up to 6 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_7          = 0x6,    /**< DDMA Can issue up to 7 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_8          = 0x7,    /**< DDMA Can issue up to 8 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_9          = 0x8,    /**< DDMA Can issue up to 9 outstanding AXI read bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_10         = 0x9,    /**< DDMA Can issue up to 10 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_11         = 0xA,    /**< DDMA Can issue up to 11 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_12         = 0xB,    /**< DDMA Can issue up to 12 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_13         = 0xC,    /**< DDMA Can issue up to 13 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_14         = 0xD,    /**< DDMA Can issue up to 14 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_15         = 0xE,    /**< DDMA Can issue up to 15 outstanding AXI read bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_READ_16         = 0xF     /**< DDMA Can issue up to 16 outstanding AXI read bursts  */
} mss_ddma_region_max_outstanding_read_e;

/**
* @brief Limit for outstanding AXI write bursts in DDMA for a memory region
*/
typedef enum
{
	MSS_DDMA_REGION_MAX_OUTSTANDING_WRITE_USE_GLOBAL = 0x0,    /**< The limit is taken from UOL field at MSS_DACC register   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_WRITE_4          = 0x2,    /**< DDMA Can issue up to 4 outstanding AXI write bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_WRITE_8          = 0x3,    /**< DDMA Can issue up to 8 outstanding AXI write bursts   */
	MSS_DDMA_REGION_MAX_OUTSTANDING_WRITE_16         = 0x4,    /**< DDMA Can issue up to 16 outstanding AXI write bursts  */
	MSS_DDMA_REGION_MAX_OUTSTANDING_WRITE_32         = 0x5     /**< DDMA Can issue up to 32 outstanding AXI write bursts  */
} mss_ddma_region_max_outstanding_write_e;


/**
* @brief PSU power save modes
*/
typedef enum
{
	MSS_POWER_SAVE_MODE_FREE_RUN = 0,               /**< All clocks are enabled */
	MSS_POWER_SAVE_MODE_DYNAMIC     ,               /**< Dynamic Power Save - automatic clock gating for active units */
	MSS_POWER_SAVE_MODE_LIGHT_SLEEP ,               /**< The core and WB clocks are shut down */
	MSS_POWER_SAVE_MODE_STANDBY                     /**< All clock are shut down and can also be shut down externally */
} mss_power_save_mode_e;

/**
* @brief Data memory access protection enable
*/
typedef enum
{
	MSS_DATA_ACCESS_PROTECTION_DISABLED = 0,    /**< Access protection checking disabled */
	MSS_DATA_ACCESS_PROTECTION_ENABLED          /**< Access protection checking enabled */
} mss_data_access_protection_e;

/**
* @brief Enable access to core core from EDAP (External Data Access Port)
*/
typedef enum
{
	MSS_EDAP_ACCESS_ENABLE = 0,     /**< EDAP port accesses are enabled*/
	MSS_EDAP_ACCESS_DISABLE         /**< EDAP port accesses are disabled*/
} mss_edap_access_enable_e;

/**
* @brief Enable access to the core from AXI slave port
*/
typedef enum
{
	MSS_AXI_SLAVE_PORT_ACCESS_ENABLE = 0,     /**< AXIs slave port accesses to the core are enabled*/
	MSS_AXI_SLAVE_PORT_ACCESS_DISABLE         /**< AXIs slave port accesses to the core are disabled*/
} mss_axi_slave_access_e;

/**
* @brief DDMA external port read policy
*/
typedef enum
{
	MSS_DDMA_READ_POLICY_MIN_CYCLES = 0,     /**< DDMA pursues minimal cycle count resulting in possible over reading of data from the external address*/
	MSS_DDMA_READ_POLICY_EXACT_READ         /**< DDMA pursues exact read resulting is possible additional read cycles on the external bus used*/
} mss_ddma_read_policy_e;

/**
* @brief Data Region Master ID
*/
typedef enum
{
	MSS_DATA_MEM_REGION_MASTER_EDP   = 0x1,     /**< EDP*/
	MSS_DATA_MEM_REGION_MASTER_AXIM0 = 0x2,     /**< AXIm0*/
	MSS_DATA_MEM_REGION_MASTER_AXIM1 = 0x4,     /**< AXIm1*/
	MSS_DATA_MEM_REGION_MASTER_BLANK = 0x80     /**< Blank Region*/
} mss_data_mem_region_master_id_e;

/**
* @brief Memory Ordering Model
*/
typedef enum
{
	MSS_DATA_MEM_ORDERING_MODEL_TOTAL_STORE = 0,     /**< Total Store Ordering*/
	MSS_DATA_MEM_ORDERING_MODEL_STRONG_ORDER         /**< Strong Ordering*/
} mss_data_memory_ordering_model_e;

/**
* @brief Program memory region settings 
*
* Used as input to \link mss_set_program_memory_region \endlink function
*/
typedef struct {
 
	unsigned int start_address;                                 /**< The memory region start address. The end address is defined by next memory region start address */

	mss_mem_region_e memory_region;  	                        /**< The memory region for which the desired address and attributes are set */
	
	mss_mem_region_active_e active;                             /**< Activate the memory region */

	mss_program_mem_region_cacheability_e cache_enable; 	    /**< Enable cache for memory region */

	mss_program_mem_region_lock_after_line_fill_e cache_lock; 	/**< Lock after cache-line fill */

	mss_access_protection_t access_protection; 	                /**< Memory access protection */

	mss_mem_region_l2_cache_policy_t l2_cache_policy; 	        /**< L2 Cache Policy */

    unsigned int qos; 	                                        /**< Quality of Service, used as a priority indicator for the associated read transaction.
	                                                                 QoS range is 0 - 15, a higher value indicates a higher priority */

} mss_program_memory_region_t;

/**
* @brief Data memory region settings
* 
* Used as input to \link mss_set_data_memory_region \endlink function
*/
typedef struct {

	unsigned int start_address;              	                /**< The memory region start address. The end address is defined by the next memory region start address.*/

	mss_mem_region_e memory_region;                             /**< The memory region for which the desired address and attributes are set */

	mss_data_mem_region_master_id_e master_id;                  /**< The memory region master ID */

	mss_mem_region_active_e active;                             /**< Activate the memory region */

	mss_data_memory_ordering_model_e memory_ordering; 	        /**< Memory ordering model - total store or strong ordering */

	mss_access_protection_t access_protection;                  /**< Memory access protection */

    unsigned int read_qos;                	                    /**< Read Quality of Service, used as a priority indicator for the associated read transaction.
	                                                                QoS range is 0-15, a higher value indicates a higher priority */

	unsigned int write_qos;                                     /**< Write Quality of Service, used as a priority indicator for the associated write transaction.
	                                                                QoS range is 0-15, a higher value indicates a higher priority */

	mss_ddma_read_policy_e ddma_read_policy;                    /**< DDMA external port read policy */

	mss_mem_region_l2_cache_policy_t l2_cache_write_policy; 	/**< L2 Cache Policy for write accesses */

	mss_mem_region_l2_cache_policy_t l2_cache_read_policy;      /**< L2 Cache Policy for read accesses */
	
	mss_data_mem_region_max_burst_size_e ddma_max_burst_size;   /**< Maximum DDMA burst size for this region */

	mss_ddma_region_max_outstanding_read_e ddma_max_outstanding_read; 	/**< Maximum outstanding DDMA read AXI bursts */

	mss_ddma_region_max_outstanding_write_e ddma_max_outstanding_write; /**< Maximum outstanding DDMA write AXI bursts */
} mss_data_memory_region_t;

/**
* @}
*/


/**
* @addtogroup MSS_DRIVER_API MSS Driver API
* @ingroup MSS_DRIVER
* @{
*/


/**
* @brief Write to MSS register
* @param[in]     reg -  MSS register enumeration
* @param[in]     value - 32 bits value
* @return void
*/
#define mss_write_reg(reg, value)  cpm_out(reg, value)

/**
* @brief Read MSS register
* @param[in]     reg -  MSS register enumeration
* @return 32 bits value
*/
#define mss_read_reg(reg) cpm_in(reg)

/**
* @brief Starts a software operation on the program cache
* @param[in]     start_address   - Start address for the operation (ignored if operation_size == #MSS_PROGRAM_CACHE_OPERATION_SIZE_FULL)
* @param[in]     operation_type  - Type of operation
* @param[in]     operation_size  - Scope of operation - entire cache or the specified start address and the number of lines
* @param[in]     cache_lines_num - Number of cache lines for the operation (ignored if operation_type == #MSS_PROGRAM_CACHE_OPERATION_SIZE_FULL)
* @note          operation_type cannot be #MSS_PROGRAM_CACHE_OPERATION_PREFETCH when operation_size is #MSS_PROGRAM_CACHE_OPERATION_SIZE_FULL
* @return void
*/
void mss_set_software_operation_program_cache_config(
	unsigned int start_address,
	mss_program_cache_operation_type_e operation_type,
	mss_program_cache_operation_size_e operation_size,
	unsigned int cache_lines_num);

/**
* @brief Enables program cache on the entire memory
*
* This function enables cache for program memory region 0.
* @return void
*/
void mss_enable_global_program_cache(void);

/**
* @brief Gets the address of the program access violation
* @return the address value (if no violation occurred the return value is 0)
*/
unsigned int mss_get_program_access_violation_address(void);

/**
* @brief Gets the address of the data access violation
* @return the address value (if no violation occurred the return value is 0)
*/
unsigned int mss_get_data_access_violation_address(void);

/**
* @brief Gets the internal program memory size
* @return the size of the internal program memory in bytes
*/
unsigned int mss_get_internal_program_memory_size(void);

/**
* @brief Gets the internal data memory size
* @return the size of the internal data memory in bytes
*/
unsigned int mss_get_internal_data_memory_size(void);

/**
* @brief Gets the program cache size
* @return the size of the program cache in bytes
*/
unsigned int mss_get_program_cache_size(void);

/**
* @brief Configures the maximum number of outstanding AXI bursts used by DDMA
*
* @param[in]     outstanding_read  - Maximum number of outstanding AXI read bursts
* @param[in]     outstanding_write - Maximum number of outstanding AXI write bursts (for EDP)
* @return void
*/
void mss_set_ddma_global_max_outstanding(mss_ddma_global_max_outstanding_read_e outstanding_read, mss_ddma_global_max_outstanding_write_e outstanding_write);

/**
* @brief Sets the core's power saving mode
* @param[in]     mode - power saving mode
* @return void
*/
void mss_set_power_save_mode(mss_power_save_mode_e mode);

/**
* @brief Sets the global policy for program and data access protection
* @param[in]     program_access_protection - Program access protection
* @param[in]     data_access_protection - Data access protection
* @return void
*/
void mss_set_access_protection(mss_program_access_protection_e program_access_protection, mss_data_access_protection_e data_access_protection);

/**
* @brief Maximum supported PDMA transfer size<BR>
*/
#define MSS_MAX_PDMA_TRANSFER_SIZE   (0x8000)

/**
* @brief Load program to the internal program memory<BR>
* This function uses the program DMA.
* @param[in]     int_address -  PDMA internal destination address, must be 32 byte aligned.
* @param[in]     ext_address -  PDMA external source address, must be 32 byte aligned.
* @param[in]     size - The number of bytes to transfer. Size must be a multiple of 32, maximum is 32768.
* @return void
*/
void mss_load_internal_program(unsigned int int_address, unsigned int ext_address, unsigned int size);

/**
* @brief Gets the current value of the MSS system clock counter (64 bits)
* In Windows simulation this function uses the system clock.
* @return the clock counter value (64 bit)
*/
unsigned long long mss_get_clock(void);

/**
* @brief Configures a program memory region
* @param[in] region_settings - Pointer to the program memory region settings structure
* @return void
*/
void mss_set_program_memory_region(mss_program_memory_region_t* region_settings);

/**
* @brief Configures a data memory region
* @param[in] region_settings - Pointer to the data memory region settings structure
* @return void
*/
void mss_set_data_memory_region(mss_data_memory_region_t* region_settings);

#ifdef __cplusplus
}
#endif
/** @}*/
#endif //__MSS_H__
