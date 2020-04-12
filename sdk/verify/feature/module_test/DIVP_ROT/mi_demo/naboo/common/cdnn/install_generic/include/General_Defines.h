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

#ifndef _GEN_DEF
#define _GEN_DEF

/*************************************************************************************\
*                                                                                     *
*                          includes                                                   *
*                                                                                     *
\**************************************************************************************/

/*************************************************************************************\
*                                                                                     *
*                          Defines                                                    *
*                                                                                     *
\**************************************************************************************/

//#define POWER2_PATCH

#define DUMP_EXT_BUFFERS 0
#if DUMP_EXT_BUFFERS
#define DUMP_INTERNAL 1
#else
#define DUMP_INTERNAL 0
#endif

#define USE_LOGGER 0
#define USE_DEFAULT_LOG 1
#define DEFAULT_LOG_PATH ".\\"
#define LOGGER_LEVEL 1  //1 - cycles/bw summary only , 2 - Full

//#define ENABLE_COUNTER



#ifdef TRUE
#undef	TRUE 
#endif 

#ifdef FALSE
#undef	FALSE
#endif
#define TRUE	1
#define FALSE	0


#undef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#undef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#undef ABS
#define ABS(a)	 ((a)>(0) ? (a) : (-(a)))

#undef MAX_S16
#define MAX_S16(a,b)  (((int16_t)(a))>((int16_t)(b)) ? (int32_t)(a) : (int32_t)(b))

#undef MIN_S16
#define MIN_S16(a,b)  (((int16_t)(a))<((int16_t)(b)) ? (int32_t)(a) : (int32_t)(b))

#undef CLIPU8
#define CLIPU8(a) (uint8_t)MIN(MAX(0,a),255)

#undef CLIPS8
#define CLIPS8(a) (int8_t)MIN(MAX(-128,a),127)

#undef CLIPU16
#define CLIPU16(a) (uint16_t)MIN(MAX(0,a),65535)

#undef CLIPS16
#define CLIPS16(a) (int16_t)MIN(MAX(-32768,a),32767)

#define STRIDE_BYTES_FROM_WIDTH(myWidth)  ((((myWidth)+3)>>2)<<2)
#define STRIDE_WORDS_FROM_WIDTH(myWidth)  ((((myWidth)+1)>>1)<<1)

#define S8_MIN_VALUE	(-0x7f - 1)
#define S16_MIN_VALUE	(-0x7fff - 1)
#define S32_MIN_VALUE	(-0x7fffffff - 1)

#define S8_MAX_VALUE	(0x7f)
#define S16_MAX_VALUE	(0x7fff)
#define S32_MAX_VALUE	(0x7fffffff)
#define U8_MAX_VALUE	(0xff)
#define U16_MAX_VALUE	(0xffff)
#define U32_MAX_VALUE	(0xffffffff)

#define MAX3(a, b, c) ((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c))
#define MIN3(a, b, c) ((a) < (b)) ? (((a) < (c)) ? (a) : (c)) : (((b) < (c)) ? (b) : (c))
#define ABSDIFF(a,b) ((a)>(b) ? ((a)-(b)) : ((b)-(a)))
#define SATURATE_WORD_TO_BYTE_HIGH(a) ((a) > 255 ? 255 : (a))
#define SATURATE_WORD_TO_BYTE_LOW(a) ((a) < 0 ? 0 : (a))
#define SATURATE_WORD_TO_BYTE(a) SATURATE_WORD_TO_BYTE_HIGH(SATURATE_WORD_TO_BYTE_LOW(a))
#define ADDS16(a,b) ((a)+(b))
#define SUBTRACTS16(a,b) ((a)-(b))
#define ADDU8(a,b) SATURATE_WORD_TO_BYTE((a)+(b))
#define SUBTRACTU8(a,b) SATURATE_WORD_TO_BYTE((a)-(b))
#define SWAP(a, b)  do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while (0)


#define MALLOC_U8(x,y)	do {x = (uint8_t *)malloc(y); if (x==NULL)err=1;} while(0)
#define MALLOC_S8(x,y)	do {x = (int8_t *)malloc(y); if (x==NULL)err=1;} while(0)
#define MALLOC_S16(x,y)	do {x = (int16_t *)malloc(y*2); if (x==NULL)err=1;} while(0)
#define MALLOC_U16(x,y)	do {x = (uint16_t *)malloc(y*2); if (x==NULL)err=1;} while(0)

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;



/*************************************************************************************\
*                                                                                     *
*                          structs                                                    *
*                                                                                     *
\**************************************************************************************/



/*************************************************************************************\
*                                                                                     *
*                          externals                                                  *
*                                                                                     *
\**************************************************************************************/


/*************************************************************************************\
*                                                                                     *
*                          functions                                                  *
*                                                                                     *
\**************************************************************************************/

#endif	// _GEN_DEF
