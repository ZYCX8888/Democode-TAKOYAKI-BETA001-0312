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
#ifndef __TEST_CIPHER_H__
#define __TEST_CIPHER_H__

#include "mi_common_datatype.h"
#include "test_cipher_common.h"

MI_S32 Setconfiginfo(MI_HANDLE chnHandle, MI_CIPHER_ALG_e alg, const MI_U8 u8KeyBuf[16], const MI_U8 u8IVBuf[16]);
void CBC_AES128(void);
void CTR_AES128(void);
void ECB_AES128(void);

#endif /*__TEST_CIPHER_H__ end */