/*************************************************************************************\
*                                                                                     *
* Copyright (C) CEVA Inc. All rights reserved                                         *
*                                                                                     *
*                                                                                     *
* THIS PRODUCT OR SOFTWARE IS MADE AVAILABLE EXCLUSIVELY TO LICENSEES THAT HAVE       *
* RECEIVED EXPRESS WRITTEN AUTHORIZATION FROM CEVA TO DOWNLOAD OR RECEIVE THE         *
* PRODUCT OR SOFTWARE AND HAVE AGREED TO THE END USER LICENSE AGREEMENT (EULA).       *
* IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE               *
* CEVA EULA, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS PRODUCT OR SOFTWARE.           *
*                                                                                     *
* The information contained in this document is subject to change without notice and  *
* does not represent a commitment on any part of CEVA®, Inc. CEVA®, Inc. and its      *
* subsidiaries make no warranty of any kind with regard to this material, including,  *
* but not limited to implied warranties of merchantability and fitness for a          *
* particular purpose whether arising out of law, custom, conduct or otherwise.        *
*                                                                                     *
* While the information contained herein is assumed to be accurate, CEVA®, Inc.       *
* assumes no responsibility for any errors or omissions contained herein, and         *
* assumes no liability for special, direct, indirect or consequential damage,         *
* losses, costs, charges, claims, demands, fees or expenses, of any nature or kind,   *
* which are incurred in connection with the furnishing, performance or use of this    *
* material.                                                                           *
*                                                                                     *
* This document contains proprietary information, which is protected by U.S. and      *
* international copyright laws. All rights reserved. No part of this document may be  *
* reproduced, photocopied, or translated into another language without the prior      *
* written consent of CEVA®, Inc.                                                      *
*                                                                                     *
***************************************************************************************
* Author: Adi Panzer                                                                  *
* Date  : 35/05/2016                                                                  *
\**************************************************************************************/

#ifndef __CDMA_LIB_H__
#define __CDMA_LIB_H__

#define FULL_DMA_REQUEST_SIZE	16

#define DMA_NO_PRED_REQUEST		0x7f							/**< A special value to indicate in the request header that no predefined request template is used */

/** @brief CDMA driver status enumeration */
typedef enum
{
	CDMA_ERROR_NO_ERROR = 0,									/**< No Error */
	CDMA_ERROR_POOL_FULL,										/**< No space for adding new request to the pool */
	CDMA_ERROR_OUT_OF_BOUNDARIES								/**< The value is out of boundaries */
} CDMA_ERROR_TYPE_enum;


/** @brief Data Transfer Configuration (DTC) registers enumeration */
typedef enum
{
	CDMA_DTC_INDEX_CTRL = 0x00,									/**< Control register */
	CDMA_DTC_INDEX_SSA  = 0x01,									/**< Source start address */
	CDMA_DTC_INDEX_DSA  = 0x02,									/**< Destination start address */
	CDMA_DTC_INDEX_BEN =  0x03,									/**< Block's elements number */
	CDMA_DTC_INDEX_CBN =  0x04,									/**< Cluster's blocks number */
	CDMA_DTC_INDEX_SEPM = 0x05,									/**< Source element post modification register */
	CDMA_DTC_INDEX_DEPM = 0x06,									/**< Destination element post modification register */
	CDMA_DTC_INDEX_SBPM = 0x07,									/**< Source block post modification register */
	CDMA_DTC_INDEX_DBPM = 0x08,									/**< Destination block post modification register */
	CDMA_DTC_INDEX_SCPM = 0x09,									/**< Source cluster post modification register */
	CDMA_DTC_INDEX_DCPM = 0x0a,									/**< Destination cluster post modification register */
	CDMA_DTC_INDEX_SFPM = 0x0b,									/**< Source frame post modification register */
	CDMA_DTC_INDEX_DFPM = 0x0c,									/**< Destination frame post modification register */
	CDMA_DTC_INDEX_SSPM = 0x0d,									/**< Source section post modification register */
	CDMA_DTC_INDEX_DSPM = 0x0e									/**< Destination section post modification register */
	
} CDMA_DTC_REGS_INDEX_enum;

/** @brief Data Transfer Configuration (DTC) registers bitmap values */
typedef enum
{
	CDMA_DTC_BITMAP_CTRL = (1 << CDMA_DTC_INDEX_CTRL),			/**< Control register */
	CDMA_DTC_BITMAP_SSA  = (1 << CDMA_DTC_INDEX_SSA),			/**< Source start address */
	CDMA_DTC_BITMAP_DSA  = (1 << CDMA_DTC_INDEX_DSA),			/**< Destination start address */
	CDMA_DTC_BITMAP_BEN =  (1 << CDMA_DTC_INDEX_BEN),			/**< Block's elements number */
	CDMA_DTC_BITMAP_CBN =  (1 << CDMA_DTC_INDEX_CBN),			/**< Cluster's blocks number */
	CDMA_DTC_BITMAP_DEPM = (1 << CDMA_DTC_INDEX_DEPM),			/**< Destination element post modification register */
	CDMA_DTC_BITMAP_SBPM = (1 << CDMA_DTC_INDEX_SBPM),			/**< Source block post modification register */
	CDMA_DTC_BITMAP_DBPM = (1 << CDMA_DTC_INDEX_DBPM),			/**< Destination block post modification register */
	CDMA_DTC_BITMAP_SCPM = (1 << CDMA_DTC_INDEX_SCPM),			/**< Source cluster post modification register */
	CDMA_DTC_BITMAP_DCPM = (1 << CDMA_DTC_INDEX_DCPM),			/**< Destination cluster post modification register */
	CDMA_DTC_BITMAP_SFPM = (1 << CDMA_DTC_INDEX_SFPM),			/**< Source frame post modification register */
	CDMA_DTC_BITMAP_DFPM = (1 << CDMA_DTC_INDEX_DFPM),			/**< Destination frame post modification register */
	CDMA_DTC_BITMAP_SSPM = (1 << CDMA_DTC_INDEX_SSPM),			/**< Source multi-frame post modification register */
	CDMA_DTC_BITMAP_DSPM = (1 << CDMA_DTC_INDEX_DSPM),			/**< Destination multi-frame post modification register */
	CDMA_DTC_BITMAP_SEPM = (1 << CDMA_DTC_INDEX_SEPM)			/**< Source element post modification register */
} CDMA_DTC_REGS_BITMAP_enum;

/** @brief Request dimensions enumeration */
typedef enum
{
	CDMA_1D = 16,												/**< One dimension request */
	CDMA_2D = 8,												/**< Two dimensions request */
	CDMA_3D = 4,												/**< Three dimensions request */
	CDMA_4D = 2,												/**< Four dimensions request */
	CDMA_5D = 1													/**< Five dimensions request */
}CDMA_DIMENSION_enum;

/** @brief Request data manipulation enumeration */
typedef enum
{
	CDMA_DMP_NONE = 0,											/**< Data manipulation disabled */
	CDMA_DMP_4xDW = 1											/**< Data manipulation enabled */
}CDMA_DMP_enum;

/** @brief Request memory direction enumeration */
typedef enum
{
	CDMA_EXT_EXT_MEM = 0,										/**< External to external memory transfer request */
	CDMA_EXT_CONTROL = 1,
	CDMA_EXT_INT = 2,											/**< External to internal memory transfer request*/
	CDMA_INT_EXT = 4											/**< Internal to external memory transfer request*/
//	CDMA_INT_INT = 3,											/**< Internal to internal memory transfer request*/
}CDMA_MEM_VS_enum;

/** @brief Request element size enumeration */
typedef enum
{
	CDMA_8BIT_ELEMENT = 0,										/**< 8 bit element size */
	CDMA_16BIT_ELEMENT = 1,										/**< 16 bit element size */
	CDMA_32BIT_ELEMENT = 4,										/**< 32 bit element size */
	CDMA_64BIT_ELEMENT = 5,										/**< 64 bit element size */
	CDMA_128BIT_ELEMENT = 6,									/**< 128 bit element size */
	CDMA_256BIT_ELEMENT = 7									/**< 256 bit element size */
}CDMA_element_size_enum;

/** @brief Request sign extension enumeration */
typedef enum
{
	CDMA_NO_EXTENSION = 0,										/**< no extension */
	CDMA_ZERO_EXTEND = 1,										/**< 8 to 16 bits with zero extension */
	CDMA_SIGN_EXTEND = 2										/**< 8 to 16 bits with sign extension */
}CDMA_sign_extend_enum;

/** @brief Request burst size values (for external memory) */
typedef enum
{
	CDMA_SINGLE_TRANSFER	= 0,								/**< 1 element in a burst */
	CDMA_2_BEAT_BURST,											/**< 2 elements in a burst */
	CDMA_3_BEAT_BURST,											/**< 3 elements in a burst */
	CDMA_4_BEAT_BURST,											/**< 4 elements in a burst */
	CDMA_5_BEAT_BURST,											/**< 5 elements in a burst */
	CDMA_6_BEAT_BURST,											/**< 6 elements in a burst */
	CDMA_7_BEAT_BURST,											/**< 7 elements in a burst */
	CDMA_8_BEAT_BURST,											/**< 8 elements in a burst */
	CDMA_9_BEAT_BURST,											/**< 9 elements in a burst */
	CDMA_10_BEAT_BURST,											/**< 10 elements in a burst */
	CDMA_11_BEAT_BURST,											/**< 11 elements in a burst */
	CDMA_12_BEAT_BURST,											/**< 12 elements in a burst */
	CDMA_13_BEAT_BURST,											/**< 13 elements in a burst */
	CDMA_14_BEAT_BURST,											/**< 14 elements in a burst */
	CDMA_15_BEAT_BURST,											/**< 15 elements in a burst */
	CDMA_16_BEAT_BURST											/**< 16 elements in a burst */
}CDMA_burst_size_enum;

typedef enum
{
	CDMA_PRIORITY_HI_LOW = 0,
	CDMA_PRIORITY_ROUND_ROBIN = 1
}CDMA_PRIORITY_enum;


/** @brief DTC CTRL register definition */
typedef struct
{
	unsigned CHEN		: 1;								/**<  Channel enable */
	unsigned PAUSE		: 1;								/**<  Channel pause */
	unsigned CLIP		: 1;								/**<  Clipping enable */
	unsigned CLIP_SEL	: 1;								/**<  Clipping type (2D,3D) */
	unsigned EOTIE		: 1;								/**<  EOT (End Of Transfer) interrupt enable */
	unsigned SOTIE		: 1;								/**<  SOT (Start Of Transfer) interrupt enable */
	unsigned _5D		: 1;								/**<  Use 5-dimensional transfer */
	unsigned _4D		: 1;								/**<  Use 4-dimensional transfer */
	unsigned _3D		: 1;								/**<  Use 3-dimensional transfer */
	unsigned _2D		: 1;								/**<  Use 2-dimensional transfer */
	unsigned SD			: 1;								/**<  Use 1-dimensional transfer */
	unsigned PEOB		: 1;								/**<  Pause at end of block (1d) */
	unsigned PEOC		: 1;								/**<  Pause at end of cluster (2d) */
	unsigned MEM_SEL	: 3;								/**<  Transfer type */
	unsigned DSIZE		: 3;								/**<  Destination element size */
	unsigned CTOEN		: 1;								/**<  channel trigger out enable */
	unsigned BURST		: 4;								/**<  Destination burst size */
	unsigned ZSEXT		: 2;								/**<  8 to 16 bit extension */
	unsigned SSIZE		: 3;								/**<  Source element size */
	unsigned PEOF		: 1;								/**<  Pause at end of frame (3d) */
	unsigned PEOMF		: 1;								/**<  Pause at end of multi frame (4d) */
	unsigned PEOS		: 1;								/**<  Pause at end of section (5d) */
}DTCx_CTRL_struct;

/** @brief  DTC Control register union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	DTCx_CTRL_struct field;										/**< Fields view */
}DTCx_CTRL_union;

/** @brief DTC SCBEN register definition */
typedef struct
{
	unsigned SBEN : 16;								/**< Number of elements in a block for source */
	unsigned DBEN : 16;								/**< Number of elements in a block for source */
}DTCx_BEN_struct;

/** @brief  DTC SCBEN register union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	DTCx_BEN_struct field;										/**< Fields view */
}DTCx_BEN_union;

/** @brief DTC DCBEN register definition */
typedef struct
{
	unsigned SCBN : 16;								/**< Number of blocks in a cluster for source */
	unsigned DCBN : 16;								/**< Number of blocks in a cluster for source */
}DTCx_CBN_struct;

/** @brief  DTC SCBEN register union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	DTCx_CBN_struct field;										/**< Fields view */
}DTCx_CBN_union;

/**  @brief DTC clip register definition */
typedef struct
{
	unsigned LCLIP 			:8 ;								/**< Left clip value (DW units) */
	unsigned TCLIP			:8 ;								/**< Top clip for 3D clipping (number of rows) */
	unsigned RCLIP			:8 ;								/**< Right clip (DW units) */
	unsigned BCLIP			:8 ;								/**< Bottom clip for 3D clipping (number of rows) */
}DTCx_CLIP_struct;

/**  @brief DTC clip register union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	DTCx_CLIP_struct field;										/**< Fields view */
}DTCx_CLIP_union;

/** @brief DTC source element post modification register definition */
typedef struct
{
	unsigned SFCN			:16;								/**< Number of clusters in a source frame */
	unsigned SMFN			:8;									/**< Number of frames in a source multi-frame */
	unsigned SSMN			:8;									/**< Number of multi-frames in a source section */
//	signed	 SEPM			:16;								/**< Source pointer post-modification per element */
}DTCx_SEPM_struct;

/**  @brief DTC source element post modification register union */
typedef union
{
	unsigned int overlay;												/**< Overlay view */
	DTCx_SEPM_struct field;										/**< Fields view */
}DTCx_SEPM_union;

/** @brief DTC destination element post modification register definition */
typedef struct
{
	unsigned DFCN			:16;								/**< Number of clusters in a destination frame */
	unsigned DMFN			:8;									/**< Number of frames in a destination multi-frame */
	unsigned DSMN			:8;									/**< Number of multi-frames in a destination section */
//	signed	 DEPM			:16;								/**< Destination pointer post-modification per element*/
}DTCx_DEPM_struct;

/** @brief DTC destination element post modification register union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	DTCx_DEPM_struct field;										/**< Fields view */
}DTCx_DEPM_union;

/** @brief Request registers definition */
typedef struct
{
	DTCx_CTRL_union		CTRL;							        /**< Control register */		
	unsigned int		SSA;							        /**< Source start address */		
	unsigned int		DSA;							        /**< Destination start address */		
	DTCx_BEN_union		BEN;							        /**< Block's elements number */
	DTCx_CBN_union		CBN;							        /**< Cluster's blocks number */
	DTCx_SEPM_union		SEPM;							        /**< Source element post modification register */
	DTCx_DEPM_union		DEPM;							        /**< Destination element post modification register */
	int		 			SBPM;							        /**< Source block post modification register */
	int		 			DBPM;							        /**< Destination block post modification register */
	int		 			SCPM;							        /**< Source cluster post modification register */
	int		 			DCPM;							        /**< Destination cluster post modification register */
	int		 			SFPM;							        /**< Source frame post modification register */
	int		 			DFPM;							        /**< Destination frame post modification register */
	int					SSPM;							        /**< Source multi-frame post modification register */
	int					DSPM;							        /**< Destination multi-frame post modification register */
	
	
}DTC_REGISTER_MM3K;

/** @brief Request header fields definition */
typedef struct
{
	unsigned pred_index		:7;									/**< Predefined request index */
	unsigned ZZMD			:6;									/**< ZZMD index (reserved since MM3101 RTL v1.04) */
	unsigned regs_map		:16;								/**< Registers map field */
	unsigned reserved		:3;									/**< Reserved */
}CDMA_HEADER_struct;

/** @brief Request header union */
typedef union
{
	unsigned int overlay;										/**< Overlay view */
	CDMA_HEADER_struct field;									/**< Fields view */
}CDMA_HEADER_union;

typedef struct
{
	CDMA_HEADER_union header;			//Header struct
	DTC_REGISTER_MM3K request;			//Request struct
}CDMA_HEADER_REQUEST;

///																		   ///
//////////////////////////////////////////////////////////////////////////////


#endif //__CDMA_LIB_H__
