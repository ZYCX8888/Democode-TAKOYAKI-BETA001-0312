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
#ifndef __MI_CIPHER_H__
#define __MI_CIPHER_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "mi_common.h"
#define KEY_SIZE    16
#define AES_BLOCK_SIZE 16

typedef enum
{
    ALG_AES_CBC ,
    ALG_AES_CTR ,
    ALG_AES_ECB ,
} ALG_e;

typedef struct
{
    U8    key[KEY_SIZE];
    U8    iv[AES_BLOCK_SIZE];
    ALG_e alg;
} CipherConfig_t ;

typedef void* CIPHER_HANDLE;
MI_RET MI_CIPHER_Init();
MI_RET MI_CIPHER_UnInit();
MI_RET MI_CIPHER_CreateHandle(CIPHER_HANDLE *handle);
MI_RET MI_CIPHER_DestroyHandle(CIPHER_HANDLE handle);
MI_RET MI_CIPHER_ConfigHandle(CIPHER_HANDLE handle, CipherConfig_t *config);
MI_RET MI_CIPHER_Encrypt(CIPHER_HANDLE handle, void* srcAddr, void* dstAddr, U32 len);
MI_RET MI_CIPHER_Decrypt(CIPHER_HANDLE handle, void* srcAddr, void* dstAddr, U32 len);
#ifdef __cplusplus
}
#endif

#endif //__MI_CIPHER_H__
