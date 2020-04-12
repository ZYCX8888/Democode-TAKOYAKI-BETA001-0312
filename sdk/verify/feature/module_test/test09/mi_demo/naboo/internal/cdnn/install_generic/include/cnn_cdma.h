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

#ifndef __CDMA_H__
#define __CDMA_H__
#ifdef XM
#include <inttypes.h>
#else
#include <cinttypes>
#endif
#define FULL_DMA_REQUEST_SIZE	16

#define DMA_NO_PRED_REQUEST		0x7f							/**< A special value to indicate in the request header that no predefined request template is used */

namespace CevaAcceleratorNS {


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
        CDMA_DTC_INDEX_SSA = 0x01,									/**< Source start address */
        CDMA_DTC_INDEX_DSA = 0x02,									/**< Destination start address */
        CDMA_DTC_INDEX_BEN = 0x03,									/**< Block's elements number */
        CDMA_DTC_INDEX_CBN = 0x04,									/**< Cluster's blocks number */
        CDMA_DTC_INDEX_SFCN = 0x05,									/**< Source frame cluster number register */
        CDMA_DTC_INDEX_DFCN = 0x06,									/**< Destination frame cluster number register */
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
        CDMA_DTC_BITMAP_SSA = (1 << CDMA_DTC_INDEX_SSA),			/**< Source start address */
        CDMA_DTC_BITMAP_DSA = (1 << CDMA_DTC_INDEX_DSA),			/**< Destination start address */
        CDMA_DTC_BITMAP_BEN = (1 << CDMA_DTC_INDEX_BEN),			/**< Block's elements number */
        CDMA_DTC_BITMAP_CBN = (1 << CDMA_DTC_INDEX_CBN),			/**< Cluster's blocks number */
        CDMA_DTC_BITMAP_SFCN = (1 << CDMA_DTC_INDEX_SFCN),			/**< Source frame cluster number register */
        CDMA_DTC_BITMAP_DFCN = (1 << CDMA_DTC_INDEX_DFCN),			/**< Destination frame cluster number register */
        CDMA_DTC_BITMAP_SBPM = (1 << CDMA_DTC_INDEX_SBPM),			/**< Source block post modification register */
        CDMA_DTC_BITMAP_DBPM = (1 << CDMA_DTC_INDEX_DBPM),			/**< Destination block post modification register */
        CDMA_DTC_BITMAP_SCPM = (1 << CDMA_DTC_INDEX_SCPM),			/**< Source cluster post modification register */
        CDMA_DTC_BITMAP_DCPM = (1 << CDMA_DTC_INDEX_DCPM),			/**< Destination cluster post modification register */
        CDMA_DTC_BITMAP_SFPM = (1 << CDMA_DTC_INDEX_SFPM),			/**< Source frame post modification register */
        CDMA_DTC_BITMAP_DFPM = (1 << CDMA_DTC_INDEX_DFPM),			/**< Destination frame post modification register */
        CDMA_DTC_BITMAP_SSPM = (1 << CDMA_DTC_INDEX_SSPM),			/**< Source multi-frame post modification register */
        CDMA_DTC_BITMAP_DSPM = (1 << CDMA_DTC_INDEX_DSPM)			/**< Destination multi-frame post modification register */
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


    /** @brief Request memory direction enumeration */
    typedef enum
    {
        CDMA_EXT_EXT_MEM = 0,										/**< External to external memory transfer request */
        CDMA_EXT_CONTROL = 1,										/**< External to CNN HWA internal FIFO's transfer request */
        CDMA_EXT_INT = 2,											/**< External to internal memory transfer request*/
        CDMA_INT_EXT = 4											/**< Internal to external memory transfer request*/
    }CDMA_MEM_DIR_enum;

    /** @brief Request element size enumeration */
    typedef enum
    {
        CDMA_8BIT_ELEMENT = 0,										/**< 8 bits element size */
        CDMA_16BIT_ELEMENT = 1,										/**< 16 bits element size */
        CDMA_32BIT_ELEMENT = 4,										/**< 32 bits element size */
        CDMA_64BIT_ELEMENT = 5,										/**< 64 bits element size */
        CDMA_128BIT_ELEMENT = 6,									/**< 128 bits element size */
        CDMA_256BIT_ELEMENT = 7									/**< 256 bits element size */
    }CDMA_ELEMENT_SIZE_enum;

    /** @brief Request sign extension enumeration */
    typedef enum
    {
        CDMA_NO_EXTENSION = 0,										/**< no extension */
        CDMA_ZERO_EXTEND = 1,										/**< 8 to 16 bits with zero extension */
        CDMA_SIGN_EXTEND = 2										/**< 8 to 16 bits with sign extension */
    }CDMA_SIGN_EXTEND_enum;

    /** @brief Request burst size values (for external memory) */
    typedef enum
    {
        CDMA_SINGLE_TRANSFER = 0,									/**< 1 element in a burst */
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
    }CDMA_MAX_BURST_enum;

    typedef struct
    {
        unsigned _5D : 1;											/**<  Use 5-dimensional transfer */
        unsigned _4D : 1;											/**<  Use 4-dimensional transfer */
        unsigned _3D : 1;											/**<  Use 3-dimensional transfer */
        unsigned _2D : 1;											/**<  Use 2-dimensional transfer */
        unsigned SD : 1;											/**<  Use 1-dimensional transfer */
    } DTCx_DIMENSION_struct;

    /** @brief DTC CTRL register definition */
    typedef struct
    {
        unsigned CHEN : 1;											/**<  Channel enable */
        unsigned PAUSE : 1;											/**<  Channel pause */
        unsigned R1 : 1;											/**<  Reserved */
        unsigned R2 : 1;											/**<  Reserved */
        unsigned EOTIE : 1;											/**<  EOT (End Of Transfer) interrupt enable */
        unsigned SOTIE : 1;											/**<  SOT (Start Of Transfer) interrupt enable */
        unsigned _5D : 1;											/**<  Use 5-dimensional transfer */
        unsigned _4D : 1;											/**<  Use 4-dimensional transfer */
        unsigned _3D : 1;											/**<  Use 3-dimensional transfer */
        unsigned _2D : 1;											/**<  Use 2-dimensional transfer */
        unsigned SD : 1;											/**<  Use 1-dimensional transfer */
        unsigned PEOB : 1;											/**<  Pause at end of block (1d) */
        unsigned PEOC : 1;											/**<  Pause at end of cluster (2d) */
        unsigned MEM_SEL : 3;										/**<  Transfer type */
        unsigned DSIZE : 3;											/**<  Destination element size */
        unsigned CTOEN : 1;											/**<  channel trigger out enable */
        unsigned BURST : 4;											/**<  Max burst size */
        unsigned ZSEXT : 2;											/**<  8 to 16 bit extension */
        unsigned SSIZE : 3;											/**<  Source element size */
        unsigned PEOF : 1;											/**<  Pause at end of frame (3d) */
        unsigned PEOMF : 1;											/**<  Pause at end of multi frame (4d) */
        unsigned PEOS : 1;											/**<  Pause at end of section (5d) */
    }DTCx_CTRL_struct;

    /** @brief  DTC Control register union */
    typedef union
    {
        uint32_t overlay;											/**< Overlay view */
        DTCx_CTRL_struct field;										/**< Fields view */
    }DTCx_CTRL_union;

    /** @brief DTC SCBEN register definition */
    typedef struct
    {
        unsigned SBEN : 16;									/**< Number of elements in a block for source */
        unsigned DBEN : 16;									/**< Number of elements in a block for source */
    }DTCx_BEN_struct;

    /** @brief  DTC SCBEN register union */
    typedef union
    {
        uint32_t overlay;									/**< Overlay view */
        DTCx_BEN_struct field;								/**< Fields view */
    }DTCx_BEN_union;

    /** @brief DTC DCBEN register definition */
    typedef struct
    {
        unsigned SCBN : 16;									/**< Number of blocks in a cluster for source */
        unsigned DCBN : 16;									/**< Number of blocks in a cluster for source */
    }DTCx_CBN_struct;

    /** @brief  DTC SCBEN register union */
    typedef union
    {
        uint32_t overlay;									/**< Overlay view */
        DTCx_CBN_struct field;								/**< Fields view */
    }DTCx_CBN_union;


    /** @brief DTC source/destination frame cluster number register definition */
    typedef struct
    {
        unsigned FCN : 16;									/**< Number of clusters in a frame */
        unsigned MFN : 8;									/**< Number of frames in a multi-frame */
        unsigned SMN : 8;									/**< Number of multi-frames in a section */
    }DTCx_FCN_struct;

    /** @brief DTC destination frame cluster number register union */
    typedef union
    {
        uint32_t overlay;			    					/**< Overlay view */
        DTCx_FCN_struct field;								/**< Fields view */
    }DTCx_DFCN_union;

    /** @brief DTC source frame cluster number register union */
    typedef union
    {
        uint32_t overlay;									/**< Overlay view */
        DTCx_FCN_struct field;								/**< Fields view */
    }DTCx_SFCN_union;

    /** @brief Request registers definition */
    typedef struct
    {
        DTCx_CTRL_union		CTRL;							/**< Control register */
        uint32_t			SSA;							/**< Source start address */
        uint32_t			DSA;							/**< Destination start address */
        DTCx_BEN_union		BEN;							/**< Block's elements number */
        DTCx_CBN_union		CBN;							/**< Cluster's blocks number */
        DTCx_SFCN_union		SFCN;							/**< Source frame cluster number register */
        DTCx_DFCN_union		DFCN;							/**< Destination frame cluster number register */
        int32_t		 		SBPM;							/**< Source block post modification register */
        int32_t		 		DBPM;							/**< Destination block post modification register */
        int32_t		 		SCPM;							/**< Source cluster post modification register */
        int32_t		 		DCPM;							/**< Destination cluster post modification register */
        int32_t		 		SFPM;							/**< Source frame post modification register */
        int32_t		 		DFPM;							/**< Destination frame post modification register */
        int32_t				SSPM;							/**< Source multi-frame post modification register */
        int32_t				DSPM;							/**< Destination multi-frame post modification register */
    }DTC_REGISTER_CDMA;

    /** @brief Request header fields definition */
    typedef struct
    {
        unsigned pred_index : 7;							/**< Predefined request index */
        unsigned reserved1 : 6;								/**< Reserved */
        unsigned regs_map : 16;								/**< Registers map field */
        unsigned reserved2 : 3;								/**< Reserved */
    }CDMA_HEADER_struct;

    /** @brief Request header union */
    typedef union
    {
        uint32_t overlay;									/**< Overlay view */
        CDMA_HEADER_struct field;							/**< Fields view */
    }CDMA_HEADER_union;

    typedef struct
    {
        CDMA_HEADER_union header;							//Header struct
        DTC_REGISTER_CDMA request;							//Request struct
    }CDMA_HEADER_REQUEST;

    class DmaHwTask
    {
    public:
        void setEndOfBlockPause(bool);
        void setEndOfClusterPause(bool);
        void setEndOfFramePause(bool);
        void setEndOfMultiFramePause(bool);
        void set1D();
        void set2D();
        void set3D();
        void set4D();
        void set5D();
        void setEndOfSectionPause(bool);
        void setMaxBurst(CDMA_MAX_BURST_enum);
        void setSourceElementSize(CDMA_ELEMENT_SIZE_enum);
        void setDestinationElementSize(CDMA_ELEMENT_SIZE_enum);
        void setMemoryDirection(CDMA_MEM_DIR_enum);
        void setSignExtension(CDMA_SIGN_EXTEND_enum);

        void setSourceAddress(uint32_t);
        uint32_t getSourceAddress() const;

        void setDestinationAddress(uint32_t);
        uint32_t getDestinationAddress() const;

        void setSourceBlockStride(uint32_t);
        uint32_t getSourceBlockStride() const;

        void setDestinationBlockStride(uint32_t);
        uint32_t getDestinationBlockStride() const;

        void setSourceClusterStride(uint32_t);
        uint32_t getSourceClusterStride() const;

        void setDestinationClusterStride(uint32_t);
        uint32_t getDestinationClusterStride() const;

        void setSourceFrameStride(uint32_t);
        uint32_t getSourceFrameStride() const;

        void setDestinationFrameStride(uint32_t);
        uint32_t getDestinationFrameStride() const;

        void setSourceMultiFrameStride(uint32_t);
        uint32_t getSourceMultiFrameStride() const;

        void setDestinationMultiFrameStride(uint32_t);
        uint32_t getDestinationMultiFrameStride() const;

        void setSourceBlockElementNumber(uint32_t);
        uint16_t getSourceBlockElementNumber() const;

        void setDestinationBlockElementNumber(uint32_t);
        uint16_t getDestinationBlockElementNumber() const;

        void setSourceClusterBlockNumber(uint32_t);
        uint16_t getSourceClusterBlockNumber() const;

        void setDestinationClusterBlockNumber(uint32_t);
        uint16_t getDestinationClusterBlockNumber() const;

        void setSourceFrameClusterNumber(uint32_t);
        uint16_t getSourceFrameClusterNumber() const;

        void setDestinationFrameClusterNumber(uint32_t);
        uint16_t getDestinationFrameClusterNumber() const;

        void setSourceMultiFrameFrameNumber(uint32_t);
        uint8_t getSourceMultiFrameFrameNumber() const;

        void setDestinationMultiFrameFrameNumber(uint32_t);
        uint8_t getDestinationMultiFrameFrameNumber() const;

        void setSourceSectionMultiFrameNumber(uint32_t);
        uint8_t getSourceSectionMultiFrameNumber() const;

        void setDestinationSectionMultiFrameNumber(uint32_t);
        uint8_t getDestinationSectionMultiFrameNumber() const;

        void setRegisterValue(CDMA_DTC_REGS_INDEX_enum register, uint32_t value);
        uint32_t getRegisterValue(CDMA_DTC_REGS_INDEX_enum register) const;

        void setHeader(uint32_t predefined = DMA_NO_PRED_REQUEST, uint32_t regsBitMap = 0xFFFF);
        void setHeader(uint32_t value);
        uint32_t getHeader() const;

        void reset();
        void setDummy();
    private:
        CDMA_HEADER_REQUEST request;
    };
}
#endif //__CDMA_H__
