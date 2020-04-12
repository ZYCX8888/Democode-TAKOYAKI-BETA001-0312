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
#ifndef __DMA_INT_H__
#define __DMA_INT_H__

#include <stdint.h>
#include "csl.h"

/** @brief DMA Driver internal definitions */

/** @brief DMA Descriptor structure*/
#ifdef  CEVA_DMA_EXTENDED_DESC
    typedef struct {
	    unsigned int floc : 1;
	    unsigned int pri_abs : 4;
	    unsigned int pri_frame_no : 2;
	    unsigned int pri_frame_order : 8;
	    unsigned int tframe_len : 4;
	    unsigned int qman_src_type : 2;
	    unsigned int qman_dst_type : 2;
	    unsigned int wd_wait : 1;
	    unsigned int dup : 2;
	    unsigned int ddie : 1;
	    unsigned int tdt : 1;
	    unsigned int reserved1 : 4;
	    union {
		    struct {
			    unsigned int message : 32;
			    unsigned int dst_ptr : 32;
			    unsigned int dma_size : 22;
			    unsigned int reserved1 : 3;
			    unsigned int bsz : 4;
			    unsigned int reserved2 : 3;
			    //unsigned int reserved3[4];
		    } message;

		    struct {
			    unsigned int src_ptr : 32;
			    unsigned int dst_ptr : 32;
			    unsigned int dma_size : 22;
			    unsigned int reserved1 : 3;
			    unsigned int bsz : 4;
			    unsigned int trtyp : 3;
			    //unsigned int reserved3[4];
		    } single_dimension;

		    struct {
			    unsigned int src_ptr : 32;
			    unsigned int dst_ptr : 32;
			    unsigned int num_planes : 16;
			    unsigned int tile_width : 16;
			    union {
				    struct {
					    unsigned int src_x : 16;
					    unsigned int src_y : 16;
					    unsigned int src_z : 16;
					    unsigned int dst_x : 16;
					    unsigned int dst_y : 16;
					    unsigned int dst_z : 16;
				    } pixel;
				    struct {
					    unsigned int src_plane_stride : 32;
					    unsigned int src_line_stride : 16;
					    unsigned int dst_line_stride : 16;
					    unsigned int dst_plane_stride : 32;
				    } absolute;
			    } stride;
			    unsigned int tile_height : 16;
			    unsigned int src_typ : 1;
			    unsigned int dst_typ : 1;
			    unsigned int bsz : 4;
			    unsigned int trtyp : 3;
			    unsigned int chn_unpack : 4;
			    unsigned int reserved : 3;

		    } multi_dimension;
	    } u0;
    } dma_desc_int_t;
#else //!CEVA_DMA_EXTENDED_DESC
    typedef struct {
        unsigned int message : 32;
    #ifndef CEVAXM
	    void* int_ptr;
    #else
	    unsigned int int_ptr : 20;
	    unsigned int reserved1 : 12;
    #endif
	    unsigned int reserved2 : 6;
	    unsigned int pri_abs : 4;
	    unsigned int extw : 1;
	    unsigned int ddie : 1;
	    unsigned int bsz : 4;
	    unsigned int reserved3 : 4;
	    unsigned int reserved4 : 1;
	    unsigned int floc : 1;
	    unsigned int qman_int_type : 2;
	    unsigned int pri_frame_order : 8;
	    unsigned int pri_frame_no : 2;
	    unsigned int qman_ext_type : 2;
	    unsigned int tframe_len : 4;
	    unsigned int reserved5 : 3;
	    unsigned int tdt : 1;
	    unsigned int reserved6 : 20;
    } dma_internal_message_desc_t;

    typedef struct {
    #ifndef CEVAXM
	    void* ext_ptr;
    #else
	    unsigned int ext_ptr : 32;
    #endif

        unsigned int message : 32;

	    unsigned int reserved1 : 5;
	    unsigned int wd_wait : 1;
	    unsigned int pri_abs : 4;
	    unsigned int extw : 1;
	    unsigned int ddie : 1;
	    unsigned int bsz : 4;
	    unsigned int reserved2 : 4;
	    unsigned int reserved3 : 1;
	    unsigned int floc : 1;
	    unsigned int qman_int_type : 2;
	    unsigned int pri_frame_order : 8;
	    unsigned int pri_frame_no : 2;
	    unsigned int qman_ext_type : 2;
	    unsigned int tframe_len : 4;
	    unsigned int reserved4 : 3;
	    unsigned int tdt : 1;
	    unsigned int reserved5 : 20;
    } dma_external_message_desc_t;

    typedef struct {
    #ifndef CEVAXM
	    void* ext_ptr;
	    void* int_ptr;
    #else
	    unsigned int ext_ptr : 32;
	    unsigned int int_ptr : 20;        // The following fields are not supported in MSVS simulation
	    unsigned int reserved1 : 1;     
	    unsigned int wd_wait : 1;        
	    unsigned int pri_abs : 4;			
	    unsigned int extw : 1;
	    unsigned int ddie : 1;
	    unsigned int bsz : 4;		
    #endif
	    unsigned int dma_size : 20;
	    unsigned int iit : 1;
	    unsigned int floc : 1;
	    unsigned int qman_int_type : 2;
	    unsigned int pri_frame_order : 8;
	    unsigned int pri_frame_no : 2;
	    unsigned int qman_ext_type : 2;
    #ifndef CEVAXM
	    unsigned int reserved1 : 1;
	    unsigned int wd_wait : 1;
	    unsigned int extw : 1;
	    unsigned int ddie : 1;
    #else
	    unsigned int tframe_len : 4;		//Removed field from MSVS simulation
    #endif
	    unsigned int trtyp : 3;
	    unsigned int tdt : 1;
	    unsigned int reserved3 : 20;
    } dma_1d_desc_t;

    typedef struct {
    #ifndef CEVAXM
	    void* ext_ptr;
	    void* int_ptr;
    #else
	    unsigned int ext_ptr : 32;
	    unsigned int int_ptr : 20;
	    unsigned int tframe_len : 4;		//Removed field from MSVS simulation
	    unsigned int pri_frame_order : 8;	//Removed field from MSVS simulation
    #endif
	    unsigned int tile_width : 10;
	    unsigned int ddie : 1;
	    unsigned int wd_wait : 1;
	    unsigned int iit : 1;
	    unsigned int reserved1 : 3;
	    unsigned int tile_height : 10;
	    unsigned int pri_abs : 4;
	    unsigned int pri_frame_no : 2;
	    unsigned int int_stride : 10;
	    unsigned int floc : 1;
	    unsigned int tdt : 1;
	    unsigned int extw : 1;
	    unsigned int trtyp : 3;
	    unsigned int ext_stride : 16;
    } dma_2d_desc_t;

    typedef struct {
	    uint32_t	dw0;
	    uint32_t	dw1;
	    uint32_t	dw2;
	    uint32_t	dw3;
    } dma_dw_t;

    typedef struct {
	    unsigned short	w0;
	    unsigned short	w1;
	    unsigned short	w2;
	    unsigned short	w3;
	    unsigned short	w4;
	    unsigned short	w5;
	    unsigned short	w6;
	    unsigned short	w7;
    } dma_w_t;

    /** @brief DMA task descriptor structure */
    typedef union
    {
	    dma_dw_t	raw_dw;
	    dma_w_t		raw_w;
	    dma_internal_message_desc_t internal_message;
	    dma_external_message_desc_t external_message;
	    dma_1d_desc_t dma_1d;
	    dma_2d_desc_t dma_2d;
    } dma_desc_int_t;
#endif // !CEVA_DMA_EXTENDED_DESC

#define DESC_PTR_CAST(p)    ((dma_desc_int_t*)(p)) 

/** @brief Queue Depth register structure */
typedef struct {
	unsigned int depth : 13;
	unsigned int r1 : 3;
	unsigned int pri_abs : 4;
	unsigned int pri_abs_sel : 1;
	unsigned int r2 : 9;
	unsigned int cont_frame : 1;
	unsigned int enable : 1;
} dma_queue_depth_register_t;

/** @brief Queue Depth register union */
typedef union
{
	dma_queue_depth_register_t fields;
	uint32_t overlay;
} dma_queue_depth_t;

/** @brief Maximum queue depth */
#define QUEUE_MAX_DEPTH		16383

// QMAN registers
#ifdef CEVA_DMA_EXTENDED_DESC

#define INTERNAL_QMAN_BASE_ADDR			0x1200
#define QX_EN_DEPTH						(INTERNAL_QMAN_BASE_ADDR + 0x00)
#define QX_FIRST_ADDR					(INTERNAL_QMAN_BASE_ADDR + 0x04)
#define QX_BASE_PTR						(INTERNAL_QMAN_BASE_ADDR + 0x08)
#define QX_CHNK_SIZE					(INTERNAL_QMAN_BASE_ADDR + 0x0C)
#define QX_DSC_EN_INC0					(INTERNAL_QMAN_BASE_ADDR + 0x10)
#define QX_STATUS						(INTERNAL_QMAN_BASE_ADDR + 0x14)
#define QX_DSC_EN_INC1					(INTERNAL_QMAN_BASE_ADDR + 0x40)

#define QMAN_X_OFFSET_SHIFT				0x07
#define QMAN_DESC_INC_VALUE				0x00010001
#define QMAN_DESC_CHK_SIZE				0x00100010
#define QMAN_DESC_INIT_INC_VALUE		0x80000000

#define QMAN_BASE_ADDR					0x1184
#define QMAN_ACTIVE_ADDR				(QMAN_BASE_ADDR + 0x00)
#define QMAN_VI_MASK_ADDR				(QMAN_BASE_ADDR + 0x10)
#define QMAN_RQ_STATUS_ADDR				(QMAN_BASE_ADDR + 0x14)
#define QMAN_RST_ADDR					(QMAN_BASE_ADDR + 0x18)

#else // !CEVA_DMA_EXTENDED_DESC

#define INTERNAL_QMAN_BASE_ADDR			0x1100
#define QX_EN_DEPTH						(INTERNAL_QMAN_BASE_ADDR + 0x00)
#define QX_FIRST_ADDR					(INTERNAL_QMAN_BASE_ADDR + 0x04)
#define QX_BASE_PTR						(INTERNAL_QMAN_BASE_ADDR + 0x08)
#define QX_CHNK_SIZE					(INTERNAL_QMAN_BASE_ADDR + 0x0C)
#define QX_DSC_EN_INC0					(INTERNAL_QMAN_BASE_ADDR + 0x10)
#define QX_STATUS						(INTERNAL_QMAN_BASE_ADDR + 0x14)
#define QX_DSC_EN_INC1					(INTERNAL_QMAN_BASE_ADDR + 0x2C)

#define QMAN_X_OFFSET_SHIFT				0x06
#define QMAN_DESC_INC_VALUE				0x00010001
#define QMAN_DESC_CHK_SIZE				0x00100010
#define QMAN_DESC_INIT_INC_VALUE		0x80000000

#define QMAN_BASE_ADDR					0x10dc
#define QMAN_ACTIVE_ADDR				(QMAN_BASE_ADDR + 0x00)
#define QMAN_VI_MASK_ADDR				(QMAN_BASE_ADDR + 0x10)
#define QMAN_RQ_STATUS_ADDR				(QMAN_BASE_ADDR + 0x14)
#define QMAN_RST_ADDR					(QMAN_BASE_ADDR + 0x18)




#define DMA_TYPE_MESSAGE 3
#endif // !CEVA_DMA_EXTENDED_DESC

// Debug log definitions
//#define DEBUG_LOG_ENABLE
#if (!defined CEVAXM) && (defined DEBUG_LOG_ENABLE)
#include <memory.h>
#include <stdio.h>
#include <windows.h>
#include <stdio.h>
char tracebuf[256];
#define LOG_PREFIX "DMA Driver - "
#define DEBUG_LOG(...) \
					{\
					sprintf(tracebuf, __VA_ARGS__); \
					OutputDebugString(tracebuf); \
																				}
#else // !CEVAXM
#include <string.h>
#define DEBUG_LOG(...) 
#endif //!CEVAXM

#endif // __DMA_INT_H__
