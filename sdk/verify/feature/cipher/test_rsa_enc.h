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
#ifndef __TEST_RSA_ENC_H__
#define __TEST_RSA_ENC_H__

#include "mi_common_datatype.h"
#include "test_cipher_common.h"
#include "mi_cipher_datatype.h"


MI_S32 PKCS_PUB_ENC(MI_CIPHER_RSA_ENC_SCHEME_E eRsaAlgoType, MI_U8 *pu8Expect);
MI_S32 PKCS_PRI_ENC(MI_CIPHER_RSA_ENC_SCHEME_E eRsaAlgoType);

#endif /* __TEST_RSA_ENC_H__ */

