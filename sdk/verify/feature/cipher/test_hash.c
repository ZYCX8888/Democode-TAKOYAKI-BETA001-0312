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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <assert.h>

#include "mi_common.h"
#include "mi_cipher.h"
#include "test_hash.h"

static unsigned char sha1_buf[3][128] = {
    {"abc"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"},
    {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopqabcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"}
};
static const int sha1_buflen[4] ={3, 56, 112, 1000000};
static const unsigned char sha1_sum[5][20] =
{
    {0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E, 0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D},
    {0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE, 0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1},
    {0xaf, 0xc5, 0x3a, 0x4e, 0xa2, 0x08, 0x56, 0xf9, 0x8e, 0x08, 0xdc, 0x6f, 0x3a, 0x5c, 0x98, 0x33, 0x13, 0x77, 0x68, 0xed},
    {0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4, 0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6f},
    {0x7d, 0xf9, 0x62, 0x1f, 0x17, 0xad, 0x18, 0xc5, 0x8a, 0x5a, 0xf7, 0x99, 0x1d, 0x12, 0x62, 0x20, 0x0f, 0xaf, 0xa9, 0x0f},
};

static const unsigned char sha2_sum[4][32] ={
    /** SHA-256 sample vectors*/
    {0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23, 0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C, 0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD},
    {0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8, 0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39, 0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67, 0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1},
    {0x59, 0xf1, 0x09, 0xd9, 0x53, 0x3b, 0x2b, 0x70, 0xe7, 0xc3, 0xb8, 0x14, 0xa2, 0xbd, 0x21, 0x8f, 0x78, 0xea, 0x5d, 0x37, 0x14, 0x45, 0x5b, 0xc6, 0x79, 0x87, 0xcf, 0x0d, 0x66, 0x43, 0x99 ,0xcf},
    {0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, 0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67, 0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97, 0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0},
};

static MI_U8 au8Buf[LONG_DATA_SIZE];

MI_S32 SHA1()
{
    MI_S32 ret = MI_SUCCESS;
    MI_U8 u8Hash[20];
    MI_U32 i = 0,j = 0;
    MI_HANDLE hHandle[MAX_HASH_HANDLE];
    MI_CIPHER_HASH_ALGO_e eHashAlgoType;
    MI_U32 u32HashOutLen = 0;

    ret = MI_CIPHER_Init();
    if ( MI_SUCCESS != ret )
    {
        return -1;
    }

    memset(u8Hash, 0, 20);

    for(i = 0; i < MAX_HASH_HANDLE; i++)
    {
        eHashAlgoType = MI_CIPHER_HASH_ALG_SHA1;

        ret = MI_CIPHER_HashInit(eHashAlgoType, &hHandle[i]);
        if ( MI_SUCCESS != ret )
        {
           CIPHER_ERR("hHandle :%d MI_CIPHER_HashUpdate failed \n", hHandle[i]);
           MI_CIPHER_HashUnInit(hHandle[i]);
           goto __CIPHER_HASH_EXIT__;
        }
     }
     for(i = 0; i < MAX_HASH_HANDLE; i++)
     {
        if(i == 3)
        {
            memset(au8Buf, 'a', LONG_DATA_SIZE);
            for(j=0; j<1000000/LONG_DATA_SIZE; j++)
            {
                ret = MI_CIPHER_HashUpdate(hHandle[i], au8Buf, LONG_DATA_SIZE);
                if ( MI_SUCCESS != ret )
                {
                    CIPHER_ERR("hHandle :%d MI_CIPHER_HashUpdate failed \n", hHandle[i]);
                    MI_CIPHER_HashUnInit(hHandle[i]);
                    goto __CIPHER_HASH_EXIT__;
                }
            }
        }
        else
        {
            ret = MI_CIPHER_HashUpdate(hHandle[i], sha1_buf[i], strlen(sha1_buf[i]));
            if ( MI_SUCCESS != ret )
            {
                CIPHER_ERR("hHandle :%d MI_CIPHER_HashUpdate failed \n", hHandle[i]);
                MI_CIPHER_HashUnInit(hHandle[i]);
                goto __CIPHER_HASH_EXIT__;
            }
        }
     }

     for(i = 0; i < MAX_HASH_HANDLE; i++)
     {
        ret = MI_CIPHER_HashFinal(hHandle[i], u8Hash, &u32HashOutLen);
        if ( MI_SUCCESS != ret )
        {
            CIPHER_ERR("hHandle :%d MI_CIPHER_HashFinal failed \n", hHandle[0]);
            MI_CIPHER_HashUnInit(hHandle[i]);
            goto __CIPHER_HASH_EXIT__;
        }
        if(memcmp(u8Hash, sha1_sum[i], 20) != 0)
        {
            CIPHER_ERR("\033[0;31m" "SHA1 run failed, sample %d!\n" "\033[0m", i);
            printBuffer("Sha1 result:", u8Hash, 20);
            printBuffer("golden data:", sha1_sum[i], 20);
            MI_CIPHER_HashUnInit(hHandle[i]);
            goto __CIPHER_HASH_EXIT__;
            return -1;
        }
        MI_CIPHER_HashUnInit(hHandle[i]);
        CIPHER_DBG("SHA1 run success, sample %d!\n", i);
     }

    CIPHER_DBG("sample SHA1  run successfully!\n");
    MI_CIPHER_Uninit();
    return MI_SUCCESS;

__CIPHER_HASH_EXIT__:

    MI_CIPHER_Uninit();
    CIPHER_ERR("sample  SHA1 run fail!\n");
    return -1;
}

MI_S32 SHA256()
{
    MI_S32 ret = MI_SUCCESS;
    MI_U8 u8Hash[32];
    MI_U32 i = 0, j = 0;
    MI_HANDLE hHandle[MAX_HASH_HANDLE];
    MI_CIPHER_HASH_ALGO_e eHashAlgoType;
    MI_U32 u32HashOutLen = 0;

    ret = MI_CIPHER_Init();
    if ( MI_SUCCESS != ret )
    {
        return -1;
    }

    for(i = 0; i < MAX_HASH_HANDLE; i++)
    {
        eHashAlgoType = MI_CIPHER_HASH_ALG_SHA256;
        ret = MI_CIPHER_HashInit(eHashAlgoType, &hHandle[i]);
        if ( MI_SUCCESS != ret )
        {
            CIPHER_ERR("hHandle :%d MI_CIPHER_HashInit failed \n", hHandle[i]);
            MI_CIPHER_HashUnInit(hHandle[i]);
            goto __CIPHER_HASH_EXIT__;
        }
    }

    for(i = 0; i < MAX_HASH_HANDLE; i++)
    {
        if(i == 3)
        {
            memset(au8Buf, 'a', LONG_DATA_SIZE);
            for(j=0; j<1000000/LONG_DATA_SIZE; j++)
            {
                ret = MI_CIPHER_HashUpdate(hHandle[i], au8Buf, LONG_DATA_SIZE);
                if ( MI_SUCCESS != ret )
                {
                    CIPHER_ERR("hHandle :%d MI_CIPHER_HashUpdate  failed \n", hHandle[i]);
                    MI_CIPHER_HashUnInit(hHandle[i]);
                    goto __CIPHER_HASH_EXIT__;
                }
            }
        }
        else
        {
            ret = MI_CIPHER_HashUpdate(hHandle[i], sha1_buf[i], strlen(sha1_buf[i]));
            if ( MI_SUCCESS != ret )
            {
                CIPHER_ERR("hHandle :%d MI_CIPHER_HashUpdate  failed \n", hHandle[i]);
                MI_CIPHER_HashUnInit(hHandle[i]);
                goto __CIPHER_HASH_EXIT__;
            }
        }
    }
    for(i = 0; i < MAX_HASH_HANDLE; i++)
    {
        ret = MI_CIPHER_HashFinal(hHandle[i], u8Hash, &u32HashOutLen);
        if ( MI_SUCCESS != ret )
        {
            CIPHER_ERR("hHandle :%d MI_CIPHER_HashFinal  failed \n", hHandle[i]);
            MI_CIPHER_HashUnInit(hHandle[i]);
            goto __CIPHER_HASH_EXIT__;
        }
        if(memcmp(u8Hash, sha2_sum[i], 32) != 0)
        {
            CIPHER_ERR("\033[0;31m" "SHA256 run failed, sample %d!\n" "\033[0m", i);
            printBuffer("Sha256 result:", u8Hash, 32);
            printBuffer("golden data:", sha2_sum[i], 32);
            MI_CIPHER_HashUnInit(hHandle[i]);
            goto __CIPHER_HASH_EXIT__;
        }
        MI_CIPHER_HashUnInit(hHandle[i]);
        CIPHER_DBG("SHA256 run success, sample %d!\n", i);
    }

    CIPHER_DBG("sample  SHA256 run successfully!\n");
    MI_CIPHER_Uninit();
    return MI_SUCCESS;

__CIPHER_HASH_EXIT__:

    MI_CIPHER_Uninit();
    CIPHER_ERR("sample  SHA256 run fail!\n");
    return -1;
}
