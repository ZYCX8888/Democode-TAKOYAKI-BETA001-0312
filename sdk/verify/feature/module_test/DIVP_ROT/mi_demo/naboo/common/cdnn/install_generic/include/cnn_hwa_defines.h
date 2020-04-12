/*************************************************************************************\
*                                                                                     *
* Copyright (C) CEVA Inc. All rights reserved                                         *
*                                                                                     *
*                                                                                     *
* THIS PRODUCT OR SOFTWARE IS MADE AVAILABLE EXCLUSIVELY TO LICENSEES THAT HAVE       *
* RECEIVED EXPRESS WRITTEN AUTHORIZATION FROM CEVA TO DOWNLOAD OR RECEIVE THE         *
* PRODUCT OR SOFTWARE AND HAVE AGREED TO THE END USER LICENSE AGREEMENT (EULA).       *
* IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE               *
* CEVA EULA, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS PRODUCT OR SOFTWARE.           *
*                                                                                     *
* The information contained in this document is subject to change without notice and  *
* does not represent a commitment on any part of CEVA®, Inc. CEVA®, Inc. and its      *
* subsidiaries make no warranty of any kind with regard to this material, including,  *
* but not limited to implied warranties of merchantability and fitness for a          *
* particular purpose whether arising out of law, custom, conduct or otherwise.        *
*                                                                                     *
* While the information contained herein is assumed to be accurate, CEVA®, Inc.       *
* assumes no responsibility for any errors or omissions contained herein, and         *
* assumes no liability for special, direct, indirect or consequential damage,         *
* losses, costs, charges, claims, demands, fees or expenses, of any nature or kind,   *
* which are incurred in connection with the furnishing, performance or use of this    *
* material.                                                                           *
*                                                                                     *
* This document contains proprietary information, which is protected by U.S. and      *
* international copyright laws. All rights reserved. No part of this document may be  *
* reproduced, photocopied, or translated into another language without the prior      *
* written consent of CEVA®, Inc.                                                      *
*                                                                                     *
***************************************************************************************
* Author: Adi Panzer                                                                  *
* Date  : 35/05/2017                                                                  *
\**************************************************************************************/

#ifndef HWA_DEFINES_H_
#define HWA_DEFINES_H_


#define CNN_APB_BASE                0x18000000
#define CNN_APB_DMA_R_OFFSET        0x1000
#define CNN_APB_DMA_W_OFFSET        0x2000

#define CNN_CTRL_ADDRESS            0x0
#define CNN_STAT_ADDRESS            0x4
#define CNN_TASK_COUNTER_ADDRESS    0x24
#define CNN_FREE_COUNTER_ADDRESS    0x3c
#define CNN_WAITS_COUNTER_ADDRESS    0x28
#define CNN_VERSION_ADDRESS         0x4c

#define CDMA_CSR_CTRL_ADDRESS       0x100
#define CDMA_CSR_CFG_ADDRESS        0x104
#define CDMA_BA_HP_LP_ADDRESS       0x180
#define CDMA_BA_PRD_ADDRESS         0x184
#define CDMA_FIFO_DEPTH_ADDRESS     0x188
#define CDMA_FIFO_USED_ADDRESS      0x18c

#define CNN_SLAVE                   0x10000000
#define CNN_MEM_DMA_R_OFFSET        0x00264000
#define CNN_MEM_DMA_W_OFFSET        0x00260000


#define CDMA_LP_ADDRESS             0x0000
#define CDMA_HP_ADDRESS             0x7C00

#endif /* HWA_DEFINES_H_ */
