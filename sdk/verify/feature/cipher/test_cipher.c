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
/******************************************************************************

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "mi_common.h"
#include "mi_cipher.h"
#include "mi_cipher_datatype.h"
#include "test_cipher.h"

MI_S32 Setconfiginfo(MI_HANDLE chnHandle, MI_CIPHER_ALG_e alg, const MI_U8 u8KeyBuf[16], const MI_U8 u8IVBuf[16])
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_CIPHER_Config_t stconfig;

    memset(&stconfig, 0, sizeof(MI_CIPHER_Config_t));

    stconfig.eAlg = alg;

    if(alg != MI_CIPHER_ALG_AES_ECB)
    {
        memcpy(stconfig.iv, u8IVBuf,16);
    }
    stconfig.eKeySize = E_MI_CIPHER_KEY_SIZE_128;
    memcpy(stconfig.key, u8KeyBuf, 16);
    s32Ret = MI_CIPHER_ConfigHandle(chnHandle, &stconfig);
    if (MI_SUCCESS != s32Ret)
    {
        s32Ret = -1;
    }
    return s32Ret;
}

/* encrypt data using special chn*/
void CBC_AES128(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32TestSrcDataLen = 16;
    MI_U32 u32TestDstDataLen = 0;
    MI_U32 u32Testcached = 0;
    MI_U8 pInputAddrVir[16];
    MI_U8 pOutputAddrVir[16];
    MI_HANDLE hTestchnid = -1;
    MI_U8 aes_key[16] = {0x3B,0x6F,0x18,0x27,0x45,0xBC,0xE1,0xA9,0xBA,0xE7,0x45,0x98,0x19,0xEF,0x7F,0x3D};
    MI_U8 aes_IV[16]  = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    MI_U8 aes_src[16] = {0x6A,0xE1,0xFE,0xE4,0x8E,0x50,0x5F,0x86,0xF9,0x3B,0x6E,0x22,0x63,0x43,0x77,0x6A};
    MI_U8 aes_dst[16] = {0xF3,0x08,0x8B,0xDE,0x60,0xF5,0x0C,0xEF,0x0D,0x43,0x14,0xAD,0x52,0x81,0x41,0xB5};

    s32Ret = MI_CIPHER_Init();
    if(MI_SUCCESS != s32Ret)
    {
        return ;
    }

    s32Ret = MI_CIPHER_CreateHandle(&hTestchnid);
    if(MI_SUCCESS != s32Ret)
    {
        MI_CIPHER_Uninit();
        return ;
    }

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_CBC, aes_key, aes_IV);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(pInputAddrVir, 0x0, u32TestSrcDataLen);
    memcpy(pInputAddrVir, aes_src, u32TestSrcDataLen);
    printBuffer("CBC-AES-128-ORI:", aes_src, sizeof(aes_src));

    memset(pOutputAddrVir, 0x0, u32TestSrcDataLen);

    s32Ret = MI_CIPHER_Encrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher encrypt failed.\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("CBC-AES-128-ENC:", pOutputAddrVir, u32TestDstDataLen);
    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_dst, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

   /* For decrypt */
    memcpy(pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen);
    memset(pOutputAddrVir, 0x00, u32TestSrcDataLen);

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_CBC, aes_key, aes_IV);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }
    s32Ret = MI_CIPHER_Decrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher decrypt failed.\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("CBC-AES-128-DEC:", pOutputAddrVir, u32TestDstDataLen);
    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_src, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printf("\033[0;32m""sample CBC_AES128 %s run successfully!\n""\033[0m",  __FUNCTION__);
    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    return;

__CIPHER_EXIT__:

    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    CIPHER_ERR("\033[0;32m""sample CBC_AES128 %s run fail!\n""\033[0m",  __FUNCTION__);
    return ;
}

void CTR_AES128(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32TestSrcDataLen = 32;
    MI_U32 u32TestDstDataLen = 0;
    MI_U32 u32Testcached = 0;
    MI_U8 pInputAddrVir[32];
    MI_U8 pOutputAddrVir[32];
    MI_HANDLE hTestchnid = -1;
    MI_U8 aes_key[16] = {"\x1E\x2E\x89\x73\x57\x6A\xE7\x98\x53\xE6\x3E\x8F\x12\x5F\x61\x73"};
    MI_U8 aes_IV[16]  = {"\x01\x62\xB3\xD4\xC5\x56\x37\x58\xD9\x4A\xDB\xCB\x0D\xE0\x0F\x31"};
    MI_U8 aes_src[32] = {"\x00\x11\x22\x23\x24\x35\x36\x47\x48\x59\x5A\x6B\x6C\x7D\x7E\x8F\x1F\x1F\x3B\x4B\x5C\x6C\x7C\x8E\x9E\x1F\x1D\x1A\x3C\x5D\x6E\x8F"};
    MI_U8 aes_dst[32] = {"\xEA\xb9\x4A\x54\x9A\xC5\xEA\x8A\xD6\x3C\xF3\x37\x01\x97\xA4\xED\x8C\xCE\x90\xC7\x53\xB1\xD2\xCF\x34\x20\xD5\x7B\x93\x90\x20\x0B"};

    s32Ret = MI_CIPHER_Init();
    if(MI_SUCCESS != s32Ret)
    {
        return ;
    }

    s32Ret = MI_CIPHER_CreateHandle(&hTestchnid);
    if(MI_SUCCESS != s32Ret)
    {
        MI_CIPHER_Uninit();
        return ;
    }

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_CTR, aes_key, aes_IV);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(pInputAddrVir, 0x00, u32TestSrcDataLen);
    memcpy(pInputAddrVir, aes_src, u32TestSrcDataLen);
    printBuffer("CTR-AES-128-ORI:", aes_src, u32TestSrcDataLen);

    memset(pOutputAddrVir, 0x00, u32TestSrcDataLen);

    s32Ret = MI_CIPHER_Encrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher encrypt failed s32Ret=0x%x.\n", s32Ret);
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("CTR-AES-128-ENC:", pOutputAddrVir, u32TestDstDataLen);

    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_dst, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

   /* For decrypt */
    memcpy(pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen);
    memset(pOutputAddrVir, 0x0, u32TestSrcDataLen);

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_CTR, aes_key, aes_IV);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    s32Ret = MI_CIPHER_Decrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher decrypt failed.\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("CTR-AES-128-DEC", pOutputAddrVir, u32TestDstDataLen);

    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_src, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printf("\033[0;32m""sample ECB_AES128  %s run successfully!\n""\033[0m",  __FUNCTION__);
    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    return;

__CIPHER_EXIT__:

    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    CIPHER_ERR("\033[0;32m""sample ECB_AES128  %s run fail!\n""\033[0m",  __FUNCTION__);
    return ;
}


/* encrypt data using special chn*/
void ECB_AES128(void)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32TestSrcDataLen = 16;
    MI_U32 u32TestDstDataLen = 0;
    MI_U32 u32Testcached = 0;
    MI_U8 pInputAddrVir[16];
    MI_U8 pOutputAddrVir[16];
    MI_HANDLE hTestchnid = -1;
    MI_U8 aes_key[16] = {0x1B,0x2F,0x38,0x47,0x55,0x6C,0x71,0x89,0x9A,0xE1,0x42,0x93,0x14,0xE5,0x76,0x7D};
    MI_U8 aes_src[16] = {0x61,0xE2,0xF3,0xE4,0x85,0x56,0x57,0x88,0xF9,0x3A,0x6B,0x2C,0x6D,0x4E,0x7F,0x6A};
    MI_U8 aes_dst[16] = {0x53,0x6D,0x21,0xB8,0x7C,0x68,0xEC,0x31,0xB2,0xA0,0x64,0x72,0x65,0x6E,0xA2,0xDA};

    s32Ret = MI_CIPHER_Init();
    if(MI_SUCCESS != s32Ret)
    {
        return ;
    }

    s32Ret = MI_CIPHER_CreateHandle(&hTestchnid);
    if(MI_SUCCESS != s32Ret)
    {
        MI_CIPHER_Uninit();
        return ;
    }

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_ECB, aes_key, NULL);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }

    memset(pInputAddrVir, 0x0, u32TestSrcDataLen);
    memcpy(pInputAddrVir, aes_src, u32TestSrcDataLen);
    printBuffer("ECB-AES-128-ORI:", aes_src, sizeof(aes_src));

    memset(pOutputAddrVir, 0x0, u32TestSrcDataLen);

    s32Ret = MI_CIPHER_Encrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher encrypt failed.\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("ECB-AES-128-ENC:", pOutputAddrVir, u32TestDstDataLen);

    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_dst, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

   /* For decrypt */
    memcpy(pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen);
    memset(pOutputAddrVir, 0x00, u32TestSrcDataLen);

    s32Ret = Setconfiginfo(hTestchnid, MI_CIPHER_ALG_AES_ECB, aes_key, NULL);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Set config info failed.\n");
        goto __CIPHER_EXIT__;
    }
    s32Ret = MI_CIPHER_Decrypt(hTestchnid, pInputAddrVir, pOutputAddrVir, u32TestSrcDataLen, &u32TestDstDataLen);
    if(MI_SUCCESS != s32Ret)
    {
        CIPHER_ERR("Cipher decrypt failed.\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }

    printBuffer("ECB-AES-128-DEC:", pOutputAddrVir, u32TestDstDataLen);
    /* compare */
    if ( 0 != memcmp(pOutputAddrVir, aes_src, u32TestSrcDataLen) )
    {
        CIPHER_ERR("Memcmp failed!\n");
        s32Ret = -1;
        goto __CIPHER_EXIT__;
    }
    printf("\033[0;32m""sample ECB_AES128 %s run successfully!\n""\033[0m",  __FUNCTION__);
    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    return;

__CIPHER_EXIT__:

    MI_CIPHER_DestroyHandle(hTestchnid);
    MI_CIPHER_Uninit();
    CIPHER_ERR("\033[0;32m""sample  ECB_AES128  run fail!\n""\033[0m");
    return ;
}


