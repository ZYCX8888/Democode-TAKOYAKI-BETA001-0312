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
#ifndef __TEST_HASH_H__
#define __TEST_HASH_H__

#include "mi_common_datatype.h"
#include "test_cipher_common.h"

#define MAX_HASH_HANDLE 3
#define LONG_DATA_SIZE 10000
MI_S32 SHA1();
MI_S32 SHA256();

#endif /* __TEST_HASH_H__ */

