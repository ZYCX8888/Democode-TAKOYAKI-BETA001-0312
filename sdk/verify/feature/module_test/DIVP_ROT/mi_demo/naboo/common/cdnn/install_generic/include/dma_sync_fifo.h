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

/*
 * dma_sync_fifo.h
 *
 *  Created on: Feb 2016
 *      Author: yurys
 */

#ifndef INCLUDE_DMA_SYNC_FIFO_H_
#define INCLUDE_DMA_SYNC_FIFO_H_

#include "dma_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

dma_status_e dma_sync_message_fifo_init(void);

#ifndef CEVAXM
extern int register_dma_isr(void(*cb)(void));
#endif //!defined CEVAXM

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DMA_SYNC_FIFO_H_ */
