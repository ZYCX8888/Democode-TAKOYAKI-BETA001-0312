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
#ifndef ___CEVAXM_H___
#define ___CEVAXM_H___


#include "ceva_types.h"

// Aliasing
#ifdef XM
#define CEVAXM
#endif

#ifdef CEVAXM6
#define XM6
#endif // CEVAXM6

// Define relevant vecc

#ifdef XM6
#define VECC_XM6
#endif // XM6

// CEVA XM attributes
#ifndef PRAGMA_DSECT_NO_LOAD
#ifdef CEVAXM
#define PRAGMA_DSECT_NO_LOAD(name)		__attribute__ ((section (".DSECT " name)))
#define PRAGMA_DSECT_LOAD(name)			__attribute__ ((section (".DSECT " name)))
#define PRAGMA_CSECT(name)				__attribute__ ((section (".CSECT " name)))
#define ALIGN(var,imm)					var __attribute__ ((aligned (imm)))
#elif defined(__GNUC__) // gcc toolchain	
#define PRAGMA_DSECT_NO_LOAD(name)
#define PRAGMA_DSECT_LOAD(name)
#define PRAGMA_CSECT(name)
#define ALIGN(var,imm)					var __attribute__ ((aligned (imm)))
#elif defined(_WIN32)
#define PRAGMA_DSECT_NO_LOAD(name)
#define PRAGMA_DSECT_LOAD(name)
#define PRAGMA_CSECT(name)
#define ALIGN(var,imm)					__declspec(align(imm)) var
#endif 
#endif 

#ifdef CEVAXM
#define MEM_BLOCK(num)					__attribute__ ((mem_block (num)))
#define ALWAYS_INLINE					__attribute__((always_inline))
#define NEVER_INLINE					__attribute__((noinline))
#define RESTRICT						__restrict__
#define DSP_CEVA_UNROLL(x)  DO_PRAGMA(dsp_ceva_unroll = x)
#define DSP_CEVA_TRIP_COUNT(x)  DO_PRAGMA(dsp_ceva_trip_count = x)
#define DSP_CEVA_TRIP_COUNT_FACTOR(x)  DO_PRAGMA(dsp_ceva_trip_count_factor = x)
#define DSP_CEVA_TRIP_COUNT_MIN(x)  DO_PRAGMA(dsp_ceva_trip_count_min = x)
#define DO_PRAGMA(x)		_Pragma ( #x )
#else 
#define MEM_BLOCK(num)					
#define ALWAYS_INLINE			
#define NEVER_INLINE
#define RESTRICT	
#define DSP_CEVA_TRIP_COUNT(x)
#define DSP_CEVA_TRIP_COUNT_FACTOR(x)
#define DSP_CEVA_TRIP_COUNT_MIN(x)
#define DSP_CEVA_UNROLL(num)					
#endif 

// Platform string
#ifdef CEVAXM
	#ifdef VECC_XM6
		#define PLATFORM_STR "CEVA-XM6"
		#define PLATFORM_SHORTSTR "XM6"
	#endif // VECC_XM6
#elif defined(WIN32)	
	#define PLATFORM_STR "WIN32"
	#define PLATFORM_SHORTSTR "WIN32"
#elif defined(linux)	
	#define PLATFORM_STR "linux"
	#define PLATFORM_SHORTSTR "linux"
#endif // CEVAXM


#define INIT_PSH_VAL	0
#define NUM_FILTER		6
#define SRC_OFFSET		8
#define COEFF_OFFSET	16
#define STEP			21
#define PATTERN_OFFSET	24
#define SW_CONFIG(init_psh,num_filter,src_offset,coeff_offset,step,pattern)		(((init_psh) & 0x3f) << INIT_PSH_VAL | ((num_filter) & 0x7) << NUM_FILTER     | ((src_offset) & 0x3f) << SRC_OFFSET | ((coeff_offset) & 0x1f) << COEFF_OFFSET | ((step) & 0x7) << STEP | ((pattern) & 0xff) << PATTERN_OFFSET )				

// sw config for xm6

typedef enum
{
	SWGEN_2D = 0,
	SWGEN_1D = 1
} SWGEN_MODE0_ENUM;

typedef enum
{
	SWGEN_NO_REUSE = 0,
	SWGEN_REUSE = 1
} SWGEN_MODE1_ENUM;

typedef enum
{
	SWGEN_NO_CLIP_NEG = 0,
	SWGEN_CLIP_NEG = 1
} SWGEN_MODE2_ENUM;


typedef enum
{
	FILTER_1 = 0,
	FILTER_2 = 1,
	FILTER_4 = 2,
	FILTER_8 = 3
}SWGEN_FILTER_ENUM;

typedef enum
{
	SLICE_1 = 0,
	SLICE_2 = 1,
	SLICE_4 = 2,
	SLICE_8 = 3
}SWGEN_SLICE_ENUM;

typedef enum
{
	SWGEN_LINE = 0xf0,
	SWGEN_SQUARE = 0xcc
}SWGEN_DEFAULT_PATTERN_ENUM;


//ctrl 0
#define SWGEN_INIT_PSH_VAL  0
#define SWGEN_SRC_OFFSET    8
#define SWGEN_COEFF_OFFSET	16
#define SWGEN_MODE			24
#define SWGEN_SETMODE(dimension,reuse,clip_neg)										(dimension | reuse << 1 | clip_neg << 2) // See enum definitions above
#define SWGEN_CTRL0(init_psh,src_offset,coeff_offset,sw_mode)		(((init_psh) & 0xff) << SWGEN_INIT_PSH_VAL | ((src_offset) & 0xff) << SWGEN_SRC_OFFSET | ((coeff_offset) & 0xff) << SWGEN_COEFF_OFFSET  | ((sw_mode) & 0xff) << SWGEN_MODE ) 

#define SWGEN_BASE_MODE_1D SWGEN_SETMODE(SWGEN_1D,SWGEN_NO_REUSE,SWGEN_NO_CLIP_NEG)
#define SWGEN_BASE_MODE_2D SWGEN_SETMODE(SWGEN_2D,SWGEN_NO_REUSE,SWGEN_NO_CLIP_NEG)

//ctrl 1
#define SWGEN_NUM_FILTER    0
#define SWGEN_STEP          8
#define SWGEN_SLICE			16
#define SWGEN_PATTERN		24
#define SWGEN_CTRL1(num_filter,step,slice,pattern)		(((num_filter) & 0xff) << SWGEN_NUM_FILTER  | ((step) & 0xff) << SWGEN_STEP  |  ((slice) & 0xff) << SWGEN_SLICE |  ((pattern) & 0xff) << SWGEN_PATTERN) 


#define NUM_ABSOLUTE_DIFF	6
#define SATURATION_VAL		24
#define SWSUBCMP_CONFIG(psh_val,num_absolute_diff,src_offset,sat_val)	(((psh_val) & 0x3f) << INIT_PSH_VAL | ((num_absolute_diff) & 0x3) << NUM_ABSOLUTE_DIFF  | ((src_offset) & 0x3f) << SRC_OFFSET | ((sat_val) & 0xff) << SATURATION_VAL )				
#define SWSUB_CONFIG(c,e)			(((c) & 0x3f) << SRC_OFFSET   | ((e) & 0xff) << SATURATION_VAL )

#define STRIDE_IN_BYTES(a)		     MAX(68,((((a) + 59) >> 6) << 6) + 4)
#define STRIDE_IN_WORDS(a)		    (MAX(68,(((((a) << 1) + 59) >> 6) << 6) + 4) >> 1)
#define STRIDE_IN_DWORDS(a)		    (MAX(68,(((((a) << 2) + 59) >> 6) << 6) + 4) >> 2)

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
#define CAS(a,b) if((a)<(b)) SWAP(a,b) //Compare and Swap

#define ROUND_UP_2(a)   ((((a) + 1  )>>1)<<1)
#define ROUND_UP_4(a)   ((((a) + 3  )>>2)<<2)
#define ROUND_UP_8(a)   ((((a) + 7  )>>3)<<3)
#define ROUND_UP_16(a)  ((((a) + 15 )>>4)<<4)
#define ROUND_UP_32(a)  ((((a) + 31 )>>5)<<5)
#define ROUND_UP_64(a)  ((((a) + 63 )>>6)<<6)
#define ROUND_UP_128(a) ((((a) + 127)>>7)<<7)


#define XM_NUM_OF_BANKS 16
#define XM_BANK_WIDTH 4


#endif  //#ifndef ___CEVAXM_H___

