#ifndef _ST_VIF_H_
#define _ST_VIF_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"

static void _mi_ao_deinterleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size);
static void _mi_ao_interleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size);



#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_VIF_H_
