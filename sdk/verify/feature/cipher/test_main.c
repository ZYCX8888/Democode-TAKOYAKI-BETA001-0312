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

#include "mi_sys.h"
#include "test_rsa_enc.h"
#include "test_hash.h"
#include "test_rsa_sign.h"
#include "test_cipher.h"

int main(int argc, char* argv[])
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 nTesthashType = 0;
    MI_U32 nTestCipherType = 0;

    printf("which cipher type ?\n [0] AES, [1] RSA enc, [2]RSA sign [3]hash\n");
    scanf("%d", &nTestCipherType);
    test_cipher_Flush();
    if(3 == nTestCipherType)
    {
        printf("which hash type ?\n [0] SHA1; [1] SHA256 \n");
        scanf("%d", &nTesthashType);
        test_cipher_Flush();
    }

    MI_SYS_Init();

    switch(nTestCipherType)
    {
    case 0:
    {
        CBC_AES128();
        CTR_AES128();
        ECB_AES128();
    }
    break;
    case 1:
    {
        PKCS_PUB_ENC(MI_CIPHER_RSA_ENC_SCHEME_NO_PADDING, NULL);
        PKCS_PUB_ENC(MI_CIPHER_RSA_ENC_SCHEME_RSAES_OAEP_SHA1, NULL);
        PKCS_PUB_ENC(MI_CIPHER_RSA_ENC_SCHEME_RSAES_OAEP_SHA256, NULL);
        PKCS_PUB_ENC(MI_CIPHER_RSA_ENC_SCHEME_RSAES_PKCS1_V1_5, NULL);
    }
    break;
    case 2:
    {
        RSA_SIGN_VERIFY(MI_CIPHER_RSA_SIGN_SCHEME_RSASSA_PKCS1_V15_SHA256);
        RSA_SIGN_VERIFY(MI_CIPHER_RSA_SIGN_SCHEME_RSASSA_PKCS1_PSS_SHA256);
    }
    break;
    case 3:
    {
        if (0 == nTesthashType)
        {
            s32Ret = SHA1();
        }
        else if (1 == nTesthashType)
        {
            s32Ret = SHA256();
        }
        else
        {
            CIPHER_ERR("nTesthashType %d do not exist!\n",nTesthashType);
        }
    }
    break;
    default:
        CIPHER_ERR("nTestCipherType %d do not exist!\n",nTestCipherType);
    break;
    }

    MI_SYS_Exit();
    return MI_SUCCESS;
}

