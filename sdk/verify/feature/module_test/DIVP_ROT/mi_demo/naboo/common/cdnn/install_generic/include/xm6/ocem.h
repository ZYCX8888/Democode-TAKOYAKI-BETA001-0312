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

#ifndef _INCLUDE_OCEM_XM6_H_
#define _INCLUDE_OCEM_XM6_H_

#include "cpm_io.h"

/**
* @addtogroup OCEM_DRIVER OCEM Driver
* @{
* @brief CEVA-XM6 OCEM (On-Chip Emulation Module) driver
*
* The OCEM driver is used to configure the XM6 On-Chip Emulation Module hardware.
* @}
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
* @addtogroup OCEM_DRIVER_REGS OCEM Register Map
* @ingroup OCEM_DRIVER
* @{
*/

 /**
 * @brief OCEM registers' addresses
 */
typedef enum
{
    OCEM_MSS_CONFIG_ADDR        = 0x15C,                        /**< MSS_CONFIG Register */
    OCEM_CORE_VERSION_ADDR      = 0x174,                        /**< CORE_VERSION Register */
    OCEM_CORE_ID_ADDR           = 0x178,                        /**< CORE_ID Register */
    OCEM_CORE_CONFIG_ADDR       = 0x17C,                        /**< CORE_CONFIG  Register */
    OCEM_PROF_RESET_ADDR        = 0x300,                        /**< PROF_RESET */
    OCEM_PROF_PAUSE_ADDR        = 0x304,                        /**< PROF_PAUSE */
    OCEM_PROF_CTRL0_ADDR        = 0x308,                        /**< PROF_CTRL0 */
    OCEM_PROF_CTRL1_ADDR        = 0x30C,                        /**< PROF_CTRL1 */
    OCEM_PROF_FRCC_ADDR         = 0x310,                        /**< PROF_FRCC */
    OCEM_PROF_CNT0_ADDR         = 0x320,                        /**< PROF_CNT0 */
    OCEM_PROF_CNT1_ADDR         = OCEM_PROF_CNT0_ADDR + 4,      /**< PROF_CNT1 */
    OCEM_PROF_CNT2_ADDR         = OCEM_PROF_CNT1_ADDR + 4,      /**< PROF_CNT2 */
    OCEM_PROF_CNT3_ADDR         = OCEM_PROF_CNT2_ADDR + 4,      /**< PROF_CNT3 */
    OCEM_PROF_CNT4_ADDR         = OCEM_PROF_CNT3_ADDR + 4,      /**< PROF_CNT4 */
    OCEM_PROF_CNT5_ADDR         = OCEM_PROF_CNT4_ADDR + 4,      /**< PROF_CNT5 */
    OCEM_PROF_CNT6_ADDR         = OCEM_PROF_CNT5_ADDR + 4,      /**< PROF_CNT6 */
    OCEM_PROF_CNT7_ADDR         = OCEM_PROF_CNT6_ADDR + 4,      /**< PROF_CNT7 */
    OCEM_PROF_LOW_ADD0_ADDR     = 0x340,                        /**< PROF_LOW_ADD0 */
    OCEM_PROF_LOW_ADD1_ADDR     = OCEM_PROF_LOW_ADD0_ADDR + 4,  /**< PROF_LOW_ADD1 */
    OCEM_PROF_LOW_ADD2_ADDR     = OCEM_PROF_LOW_ADD1_ADDR + 4,  /**< PROF_LOW_ADD2 */
    OCEM_PROF_LOW_ADD3_ADDR     = OCEM_PROF_LOW_ADD2_ADDR + 4,  /**< PROF_LOW_ADD3 */
    OCEM_PROF_LOW_ADD4_ADDR     = OCEM_PROF_LOW_ADD3_ADDR + 4,  /**< PROF_LOW_ADD4 */
    OCEM_PROF_LOW_ADD5_ADDR     = OCEM_PROF_LOW_ADD4_ADDR + 4,  /**< PROF_LOW_ADD5 */
    OCEM_PROF_LOW_ADD6_ADDR     = OCEM_PROF_LOW_ADD5_ADDR + 4,  /**< PROF_LOW_ADD6 */
    OCEM_PROF_LOW_ADD7_ADDR     = OCEM_PROF_LOW_ADD6_ADDR + 4,  /**< PROF_LOW_ADD7 */
    OCEM_PROF_HI_ADD0_ADDR      = 0x360,                        /**< PROF_HI_ADD0 */
    OCEM_PROF_HI_ADD1_ADDR      = OCEM_PROF_HI_ADD0_ADDR + 4,   /**< PROF_HI_ADD1 */
    OCEM_PROF_HI_ADD2_ADDR      = OCEM_PROF_HI_ADD1_ADDR + 4,   /**< PROF_HI_ADD2 */
    OCEM_PROF_HI_ADD3_ADDR      = OCEM_PROF_HI_ADD2_ADDR + 4,   /**< PROF_HI_ADD3 */
    OCEM_PROF_HI_ADD4_ADDR      = OCEM_PROF_HI_ADD3_ADDR + 4,   /**< PROF_HI_ADD4 */
    OCEM_PROF_HI_ADD5_ADDR      = OCEM_PROF_HI_ADD4_ADDR + 4,   /**< PROF_HI_ADD5 */
    OCEM_PROF_HI_ADD6_ADDR      = OCEM_PROF_HI_ADD5_ADDR + 4,   /**< PROF_HI_ADD6 */
    OCEM_PROF_HI_ADD7_ADDR      = OCEM_PROF_HI_ADD6_ADDR + 4    /**< PROF_HI_ADD7 */
} ocem_regs_addr_e;


/**
 * @brief Enumeration for selecting the controlled profiling counters in OCEM block
 */
typedef enum {
    PROF_COUNTER_MASK_FRCC = 0x1,
    PROF_COUNTER_MASK_CNT0 = 0x2,
    PROF_COUNTER_MASK_CNT1 = 0x4,
    PROF_COUNTER_MASK_CNT2 = 0x8,
    PROF_COUNTER_MASK_CNT3 = 0x10,
    PROF_COUNTER_MASK_CNT4 = 0x20,
    PROF_COUNTER_MASK_CNT5 = 0x40,
    PROF_COUNTER_MASK_CNT6 = 0x80,
    PROF_COUNTER_MASK_CNT7 = 0x100,
    PROF_COUNTER_MASK_ALL  = 0x1ff
} prof_counter_mask_e;

/**
 * @brief Enumeration for the configurable profiling counters in OCEM block
 */
typedef enum {
    PROF_COUNTER_CNT0,
    PROF_COUNTER_CNT1,
    PROF_COUNTER_CNT2,
    PROF_COUNTER_CNT3,
    PROF_COUNTER_CNT4,
    PROF_COUNTER_CNT5,
    PROF_COUNTER_CNT6,
    PROF_COUNTER_CNT7
} prof_counter_id_e;

    /**
     * @brief Enumeration of the events for the configurable profiling counters
     */
    typedef enum {
        PROF_COUNTER_EVENT_WAIT_CNT = 1,             /**<  Wait counter.*/
        PROF_COUNTER_EVENT_DBLK_CONF_CNT = 3,        /**<  DMSS block conflict counter. */
        PROF_COUNTER_EVENT_OSFC = 4,                 /**<  Output stage full counter. */
        PROF_COUNTER_EVENT_TDRC = 5,                 /**<  TCM DMSS read counter.  */
        PROF_COUNTER_EVENT_TDWC = 6,                 /**<  TCM DMSS write counter. */
        PROF_COUNTER_EVENT_ERWC = 7,                 /**<  EDP read wait counter.  */
        PROF_COUNTER_EVENT_PMSS_HIT_CNT = 17,        /**<  PMSS cache hit counter.  */
        PROF_COUNTER_EVENT_PMSS_MIS_CNT = 18,        /**<  PMSS cache miss counter. */
        PROF_COUNTER_EVENT_PWCWC = 19,               /**<  Parallel write contention wait counter. */
        PROF_COUNTER_EVENT_NOP_PACKET = 28,          /**<  NOP packet. */
        //PROF_COUNTER_EVENT_BRANCH_BTB = 29,          /**<  Branch and BTB event group. */
        PROF_COUNTER_EVENT_DATA_EXT_READ_CNT = 30,  /**<  Data external read counter. */
        PROF_COUNTER_EVENT_DATA_EXT_WRITE_CNT = 31,  /**<  Data external write counter. */
        PROF_COUNTER_EVENT_DATA_INT_READ_CNT = PROF_COUNTER_EVENT_TDRC, /**<  Data internal read counter (alias for TDRC). */
        PROF_COUNTER_EVENT_DATA_INT_WRITE_CNT = PROF_COUNTER_EVENT_TDWC  /**<  Data internal write counter (alias for TDWC). */
    } prof_counter_event_e;

/**
* @}
*/

/**
* @addtogroup OCEM_DRIVER_API OCEM Profiling Counters API
* @ingroup OCEM_DRIVER
* @{
*/

   /**
     * @brief Special value for disabling address range condition on profiling events
     */
#define PROF_COUNTER_ADDRESS_RANGE_DISABLE   (0)

    /**
     * @brief Configures a profiling counter in OCEM block. <BR> This functionality in not supported in simulation.
     * @param[in] counter_id Counter ID
     * @param[in] counter_event Counter event
     * @param[in] low_address  Lower program address (low_address <= high_address)
     * @param[in] high_address Upper program address (pass 0 to disable address range check)
     * @returns void
     */

void prof_counter_config(   prof_counter_id_e    counter_id,
							prof_counter_event_e counter_event,
                            unsigned int         low_address,
                            unsigned int         high_address );


/**
 * @brief Resets the selected profiling counters
 * @param[in] mask The bit-mask for selecting the counters to reset (see \link prof_counter_mask_e \endlink)
 */
#define PROF_RESET(mask)     do { cpm_out(OCEM_PROF_RESET_ADDR,(mask)); } while (0)

/**
 * @brief Pauses the selected profiling counters
 * @param[in] mask The bit-mask for selecting the counters to pause (see \link prof_counter_mask_e \endlink)
 */
#define PROF_PAUSE(mask)     do { unsigned int m = cpm_in(OCEM_PROF_PAUSE_ADDR); cpm_out(OCEM_PROF_PAUSE_ADDR,m|(mask)); } while (0)

/**
 * @brief Resumes the counting of the selected profiling counters
 * @param[in] mask The bit-mask for selecting the counters to resume (see \link prof_counter_mask_e \endlink)
 */
#define PROF_RESUME(mask)    do { unsigned int m = cpm_in(OCEM_PROF_PAUSE_ADDR); cpm_out(OCEM_PROF_PAUSE_ADDR,m & ~(mask)); } while (0)

/**
 * @brief Restarts the counting of the selected profiling counters
 * @param[in] mask The bit-mask for selecting the counters to restart (see \link prof_counter_mask_e \endlink)
 */
#define PROF_START(mask)     do { PROF_RESET(mask); PROF_RESUME(mask); } while (0)

/**
 * @brief Reads the value of a profiling counter in OCEM block
 * @param[in] id Counter ID (see \link prof_counter_id_e \endlink)
 * @return the value of the counter
 */
#define PROF_READ_CNT(id)    cpm_in( OCEM_PROF_CNT0_ADDR + ((id)*4) )

/**
 * @brief Reads the value of the FRCC counter in OCEM block
 * @return the value of the counter
 */
#define PROF_READ_FRCC()     cpm_in( OCEM_PROF_FRCC_ADDR )

/** @}*/

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE_OCEM_XM6_H_ */
