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
@file mss_reg.h
@brief CEVA-XM4 MSS (Memory Sub-System) driver register map
*/

#ifndef __MSS_REG_H__
#define __MSS_REG_H__

#include "cpm_io.h"

/**
* @addtogroup MSS_DRIVER_REGS MSS Register Map
* @ingroup MSS_DRIVER
* @{
*/


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory sub-system registers' addresses
 */
typedef enum
{
	MSS_PMSS_PCR_ADDR               = 0x404,      /**< PMSS PDMA Program Control Register */
    MSS_PMSS_PDEA_ADDR              = 0x408,      /**< PMSS PDMA External Address Register */
    MSS_PMSS_PDIA_ADDR              = 0x40C,      /**< PMSS PDMA Internal Address Register */
    MSS_PMSS_PDTC_ADDR              = 0x410,      /**< PMSS PDMA Transfer Control */
    MSS_PMSS_PADD_START_BASE_ADDR   = 0x414,      /**< PMSS Memory Region Start Base Address Register*/
	MSS_PMSS_PADD_ATT0_BASE_ADDR    = 0x418,      /**< PMSS Memory Region Attribute Base Register*/
	MSS_PMSS_CCOSAR_ADDR            = 0x514,      /**< PMSS Instruction Cache Operations Start Address Register*/
	MSS_PMSS_CCOCR_ADDR             = 0x518,      /**< PMSS Instruction Cache Operations Configuration Register*/
	MSS_PMSS_ECADD_ADDR             = 0x51C,      /**< PMSS ECC Error Address Register*/
	MSS_PMSS_MAPAR_ADDR             = 0x520,      /**< PMSS Program Memory Access Protection Address Register*/
	MSS_PMSS_MAPSR_ADDR             = 0x524,      /**< PMSS Program Memory Access Protection Status Register*/
	MSS_PMSS_AXI_OU_C_ADDR          = 0x528,      /**< PMSS PDMA Outstanding And Burst Size Configuration Register*/
	MSS_PMSS_MECCCOR_ADDR           = 0x530,      /**< PMSS AXI Master ECC COR Data Address Register*/
	MSS_PMSS_PMSSACS_ADDR           = 0x540,      /**< PMSS Enable Access To PMSS Shadow Registers Register*/
	
	MSS_DMSS_DMBA_ADDR              = 0x614,      /**< DMSS Data Memory Base Address Register*/
	MSS_DMSS_DMBE_ADDR              = 0x618,      /**< DMSS Data Memory Status Register*/
	MSS_DMSS_HDCFG_ADDR             = 0x630,      /**< DMSS Hardware Memory Configuration Register*/
	MSS_DMSS_DACC_ADDR              = 0x63c,      /**< DMSS Data Protection and DMA Outstanding Configuration Register*/
	MSS_DMSS_SDCFG_ADDR             = 0x640,      /**< DMSS SDCFG Register*/
	MSS_DMSS_2DCFG1_ADDR            = 0x644,      /**< DMSS 2D Memory Configuration First Register*/
	MSS_DMSS_2DCFG2_ADDR            = 0x648,      /**< DMSS 2D Memory Configuration Second Register*/
	
	MSS_DDMA_DBG_GEN_ADDR           = 0xD14,      /**< DDMA General Violation First Register*/
	MSS_DDMA_DBG_GEN2_ADDR          = 0xD24,      /**< DDMA General Violation Second Register*/
	MSS_DDMA_DBG_GEN_MASK_ADDR      = 0xD28,      /**< DDMA General Violation First Mask Register*/
	MSS_DDMA_DBG_GEN2_MASK_ADDR     = 0xD2C,      /**< DDMA General Violation Second Mask Register*/
	
	MSS_PSU_PSVM_ADDR               = 0xE50,      /**< PSU Save Mode Register*/
	MSS_PSU_PGR_ADDR                = 0xE54,      /**< PSU General Register*/
	
	MSS_AP_MAPAR_ADDR               = 0xC80,      /**< AP Memory Access Protection Address Register*/
	MSS_AP_MAPSR_ADDR               = 0xC84,      /**< AP Memory Access Protection Status Register*/
	MSS_AP_DTAP_ADDR                = 0xC88,      /**< AP TCM Access Protection Register*/
	
	MSS_IC_ADD_START_BASE_ADDR      = 0x720,      /**< Memory Region Start Address Base Register*/
	MSS_IC_ADD_ATT0_BASE_ADDR       = 0x724,      /**< Memory Region Attribute 0 Base Register*/
	MSS_IC_ADD_ATT1_BASE_ADDR       = 0x728,      /**< Memory Region Attribute 1 Base Register*/
	
	MSS_WD_SYSTIML_ADDR             = 0x680,      /**< WD MSS System Clock lower part Register*/
	MSS_WD_SYSTIMH_ADDR             = 0x684       /**< WD MSS System Clock upper part Register*/
}   mss_regs_addr_e;


/**
 * @brief Program Memory configuration register (MSS_PCR)
 */
typedef union {
    struct {
        unsigned access_prot_enable           :1;             /**< Access Protection Enable:<BR> 1: Enabled<BR> 0: Disabled */
        unsigned reserved1                    :1;             /**< Reserved */
        unsigned cache_prefetch_enable        :1;             /**< Cache Pre-fetch Enable:<BR> 1: Enabled<BR> 0: Disabled */
        unsigned tcm_enable                   :1;             /**< TCM Enable:<BR> 1: Enabled<BR> 0: Disabled */
        unsigned reserved2                    :28;            /**< Reserved */
    } field;                                                /**< Fields view */
    unsigned int   overlay;         /**< Overlay view */
} mss_program_mem_config_reg_t;

/**
 * @brief PDMA Transfer Control register (MSS_PDTC)
 */
typedef union {
    struct {
        /**
        PDMA transfer count: The number of program bytes to be transferred in a frame.<BR>
        The written value is rounded by the instruction fetch-line width (Bits [4:0] are always zero).<BR>
        This field is decremented by one fetch-line width after each IPM write access.<BR>
        Writing a value to this register starts the DMA transfer.<BR>
        Note: PDIA + PDTC must not exceed the TCM size. */
        unsigned pdtc           :17;
        
        unsigned reserved1      :12;             /**< Reserved */
        
        /**
        PDMA status. This read-only bit indicates the PDMA status.<BR>
        When set, the Program DMA is busy, which means that a frame is currently being transferred.<BR>
        When cleared, the DMA is not busy.<BR>
        This bit is automatically set by hardware when a frame transfer starts, and is cleared when the frame transfer has been completed. */
        unsigned pdst           :1;
        
        unsigned reserved2      :1;             /**< Reserved */

        unsigned pdie           :1;             /**< Program DMA interrupt enable.<BR> When set, the program DMA interrupt at the end of transfer is enabled. */

    } field;                       /**< Fields view */
    unsigned int   overlay;        /**< Overlay view */
} mss_pdma_transfer_control_reg_t;

/**
 * @brief Program Address Region configuration register (P_ADDx_START)
 */
typedef union {
	struct {
	    unsigned p_region_start :20;            /**< Start address of the program region, aligned to 4KB.
	                                                For region 0, this field is read-only and is hardwired to the value of 0x0_0000.*/

        unsigned reserved1      :8;             /**< Reserved */
        
        unsigned inactive       :1;             /**< Activate the region- see \link mss_mem_region_active_e \endlink. For region 0, this field is read-only, and is hardwired to the value of 0x0 (active).*/

        unsigned reserved2      :3;             /**< Reserved */

    } field;                       /**< Fields view */
	unsigned int   overlay;        /**< Overlay view */
} mss_program_mem_region_start_reg_t;

/**
 * @brief Program Address Region attributes register (P_ADDx_ATT0)
 */
typedef union {
	struct {
	    unsigned cacheable            :1;       /**< Cacheability - see \link mss_program_mem_region_cacheability_e \endlink */
        unsigned lock_after_line_fill :1;       /**< Lock after cache-line fill - see \link mss_program_mem_region_lock_after_line_fill_e \endlink */
        unsigned reserved1            :2;       /**< Reserved */
        unsigned access_prot          :3;       /**< Access Protection. See \link mss_program_access_protection_e \endlink */
        unsigned reserved2            :1;       /**< Reserved */
        unsigned l2_cache_policy      :4;	    /**< L2 Cache Policy for read accesses - see \link mss_mem_region_l2_cache_policy_t \endlink */
        unsigned reserved3            :12;      /**< Reserved */
        unsigned read_qos             :4;       /**< Read transaction Quality of Service (QoS), used only if AXI4 is supported */
        unsigned reserved4            :1;       /**< Reserved */
    }   field;                         /**< Fields view */
	unsigned int      overlay;         /**< Overlay view */
} mss_program_mem_region_attrib_t;

/**
 * @brief Program Cache Software Operation control and status register (P_CCOCR)
 */
typedef union {
	struct {
	unsigned reserved1      :1;             /**< Reserved */
	    /**
	    Operation status:
	    - 0: No operation or operation completed.
	    - 1: Operation is performed on program cache.

	    Cleared by the hardware after the operation completes.*/
	    unsigned operation_status :1;
        
        unsigned operation_type   :4;             /**< Type of cache operation - see \link mss_program_cache_operation_type_e \endlink */

        unsigned reserved2        :1;             /**< Reserved */
        
        unsigned operation_size   :1;             /**< Scope of cache operation - \link mss_program_cache_operation_size_e \endlink */
        
        unsigned reserved3        :8;             /**< Reserved */
        
        unsigned num_cache_lines  :16;            /**< Number of cache-lines affected by the operation */

    }  field;           /**< Fields view */
	unsigned int        overlay;         /**< Overlay view */
} mss_program_cache_software_operation_reg_t;


/**
* @brief Hardware Memory Configuration register (MSS_HDCFG)
*/
typedef union {
	struct  {
	    /**
	    Number of blocks in Internal Data Memory (IDM):
	    - 0: 4 blocks
	    - 1: Reserved */
	    unsigned num_data_tcm_blocks : 1;
	    
        unsigned reserved1 : 1;             /**< Reserved */
	    
        /**
	    Size of Internal Data Memory (IDM):
	    - 000: Reserved
	    - 001: 128 KB
	    - 010: Reserved
	    - 011: 256 KB
	    - 100: 512 KB
	    - 101: 1,024 KB
	    - 110-111: Reserved */
	    unsigned data_tcm_size : 3;
        /**
	    Width of EDAP AXI slave port:
	    - 0: 128 bits
	    - 1: Reserved */
	    unsigned edap_axi_width : 1;
	    
        unsigned reserved2 : 1;             /**< Reserved */
	    
        /**
        Width of EDP AXI master port:
	    - 00: 128 bits
	    - 01: 256 bits
	    - 10: Reserved
	    - 11: Reserved */
	    unsigned edp_axi_width : 2;
	    
        /**
	    Program TCM memory size:
	    - 000: 0KB (no TCM)
	    - 001: 32 KB
	    - 010: 64 KB
	    - 011: 128 KB
	    - 100: 256 KB
	    - 101-111: Reserved */
	    unsigned prog_tcm_size : 3;
	    
        /**
	    Program cache size:
	    - 000: 0 KB (no cache)
	    - 001: 32 KB
	    - 010: 64 KB
	    - 011: 128 KB 
	    - 100-111: Reserved */
	    unsigned prog_cache_size : 3;
        
        /**
        Width of EPP AXI master port:
	    - 0: 128 bits
	    - 1: Reserved */
	    unsigned epp_axi_width : 1;

        unsigned other : 16;               /**< Other fields */

    } field;                         /**< Fields view */
    unsigned int  overlay;           /**< Overlay view */
} mss_hardware_memory_config_reg_t;

/**
* @brief  DDMA maximum outstanding AXI bursts configuration register (MSS_DACC)
*/
typedef union {
	struct {
	    /**
	    Data protection RAW/SO indication:<BR>
	    This bit should not be modified	during AXI write or read transfers.BR>
	    Note: this option must be set when using the ACE interface.*/
	    unsigned dpraw : 1;
	    
        unsigned reserved1 : 3;                         /**< Reserved */
        
	    unsigned ddma_max_outstanding_read : 4; 	    /**< Maximum outstanding DDMA read AXI bursts - see \link mss_ddma_global_max_outstanding_read_e \endlink */
	    unsigned ddma_max_outstanding_write : 3;       	/**< Maximum outstanding DDMA write AXI bursts when using EDP - see \link mss_ddma_global_max_outstanding_write_e \endlink */

	    unsigned reserved2 : 21;                        /**< Reserved */

    }   field;                      /**< Fields view */

	unsigned int   overlay;         /**< Overlay view */
} mss_ddma_max_outstanding_config_reg_t;

/**
* @brief Power Saving Mode control register (PSVM)
*/
typedef union {
	struct {
	    unsigned power_save_mode : 2; /**< Power save mode - \link mss_power_save_mode_e \endlink */
	    
        /** Debug Block Configuration (DBC):
	    - 0: Operational Production - the Debug Block power and clocks are shut down regardless of the Power Saving mode configurations 
        - 1: Debug Block On - clocks and power are provided to the debug block

        @note this field is read/write for external masters and read only for the core.
        The core sets it with psu {debug_on} and clear with psu {debug_off}. */
	    unsigned debug_config : 1;
	    unsigned reserved1 : 1;             /**< Reserved */
        /**
	    Retention Indication bus:
	    When clear, indicates that the physical memory is in retention:
	    - 1: PMSS TCM retention indication (*)
	    - 2: PMSS cache and tag retention indication (*)
	    - 3: DMSS block 0 retention indication
	    - 4: DMSS block 1 retention indication
	    - 5: DMSS block 2 retention indication
	    - 6: DMSS block 3 retention indication

	    (*) This field is only present if the PSU Power Gating is set */
	    unsigned mem_retention : 6;
        /**
	    Shutdown Indications bus.
	    When clear, indicates that the physical memory is in shutdown (no voltage is supplied):
	    - 1: Core and MSS shutdown indication
	    - 2: PMSS TCM shutdown indication (*)
	    - 3: PMSS cache shutdown indication (*)
	    - 4: DMSS block 0 shutdown indication
	    - 5: DMSS block 1 shutdown indication
	    - 6: DMSS block 2 shutdown indication
	    - 7: DMSS block 3 shutdown indication

	    (*) This field is only present if the PSU Power Gating is set.*/
	    unsigned shutdown : 7;

	    unsigned reserved : 15;            /**< Reserved */

    } field;                       /**< Fields view */
	unsigned int   overlay;        /**< Overlay view */
} mss_power_save_mode_reg_t;

/**
* @brief MSS Memory Access configuration register (MSS_SDCFG) 
*/
typedef union {
    struct {
        unsigned reserved1 : 22;            /**< Reserved */
        
        unsigned data_access_prot : 1;      /**< Data Access Protection Enable - see \link mss_data_access_protection_e \endlink */

        unsigned reserved2 : 2;             /**< Reserved */

        unsigned edap_access_en : 1; 	    /**< Enable EDAP port accesses to the core - see \link mss_edap_access_enable_e \endlink */

        unsigned axi_slave_access_en : 1;   /**< Enable AXI slave port accesses to the core - see \link mss_axi_slave_access_e \endlink */

        unsigned reserved3 : 4;             /**< Reserved */

    }   field;                    /**< Fields view */
	unsigned int   overlay;       /**< Overlay view */
} mss_memory_access_config_reg_t;

/**
* @brief Data memory region configuration register (ADDx_START)
*/
typedef union {
	struct {
	    unsigned region_start : 20; 	    /**< Start address of the data memory region, aligned to 4KB.
	                                             For region 0, this field is read-only and is hardwired to the value of 0x0_0000.*/
	    
	    unsigned master_id : 8;            /**< Master ID of the region - see \link mss_data_mem_region_master_id_e \endlink */

        unsigned inactive : 1;             /**< Activate the region - see \link mss_mem_region_active_e \endlink.
	                                            For region 0, this field is read-only, and is hardwired to the value of 0x0 (active).*/
        	    
        unsigned reserved : 3;             /**< Reserved */
    }   field;                    /**< Fields view */
	unsigned int  overlay;        /**< Overlay view */
} mss_data_mem_region_reg_t;

/**
* @brief Data memory region's attributes register 0 (ADDx_ATT0)
*/
typedef union {
    struct {
        unsigned reserved1       : 16;     /**< Reserved */

        unsigned memory_ordering : 1;      /**< Memory Ordering Model - see \link mss_data_memory_ordering_model_e \endlink */
        
        unsigned access_prot     : 3;      /**< Access Protection - see \link mss_access_protection_t \endlink */
        
        unsigned reserved2       : 4;      /**< Reserved */
        
        unsigned read_qos        : 4;      /**< Read transaction Quality of Service, used only if AXI4 is supported */
        
        unsigned write_qos       : 4;      /**< Write transaction Quality of Service, used only if AXI4 is supported */

    }   field;                     /**< Fields view */
	unsigned int  overlay;         /**< Overlay view */
} mss_data_mem_region_attrib0_reg_t;

/**
* @brief Data memory region's attributes register 1 (ADDx_ATT1)
*/
typedef union {
    struct {
        unsigned ddma_read_policy        : 1;            /**< DDMA external port read policy - see \link mss_ddma_read_policy_e  \endlink */

        unsigned l2_cache_write_policy   : 4;            /**< L2 Cache Policy for write accesses - see \link mss_axi_cache_policy_e  \endlink */

        unsigned l2_cache_read_policy    : 4;            /**< L2 Cache Policy for read accesses  - see \link mss_axi_cache_policy_e  \endlink */

        unsigned ddma_max_burst_size     : 4;            /**< Maximum DDMA burst size - see \link mss_data_mem_region_max_burst_size_e  \endlink */

        unsigned ddma_max_outstanding_read  : 4;         /**< Maximum outstanding DDMA read AXI bursts- see \link mss_ddma_region_max_outstanding_read_e \endlink */

        unsigned reserved1 : 1;                          /**< Reserved */

        unsigned ddma_max_outstanding_write : 3;         /**< Maximum outstanding DDMA write AXI bursts - see \link mss_ddma_region_max_outstanding_write_e \endlink */

        unsigned reserved2 : 11;                         /**< Reserved */

    } field;                        /**< Fields view */
	unsigned int   overlay;         /**< Overlay view */
} mss_data_mem_region_attrib1_reg_t;

#ifdef __cplusplus
}
#endif
/** @}*/
#endif //__MSS_REG_H__
