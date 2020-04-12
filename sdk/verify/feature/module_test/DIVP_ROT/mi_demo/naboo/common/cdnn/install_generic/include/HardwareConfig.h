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
* Date  : 35/05/2016                                                                  *
\**************************************************************************************/

#ifndef _HW_CONFIG
#define _HW_CONFIG

/*************************************************************************************\
*                                                                                     *
*                          includes                                                   *
*                                                                                     *
\**************************************************************************************/

/*************************************************************************************\
*                                                                                     *
*                          Defines                                                    *
*                                                                                     *
\**************************************************************************************/

#define EXT_DATA_BUFF_LEN				32000000
#define EXT_Q_LEN						16384
#define READ_DMA_ADDRESS				0x00264000
#define WRITE_DMA_ADDRESS				0x00260000
#define CONFIG_QUEUE_ADDRESS			0x00240000
#define DATA_BLOCK1_ADDRESS				0x00000000
#define WEIGHTS_BLOCK1_ADDRESS			0x00200000
#define BIAS_BLOCK_ADDRESS				0x00250000


/*************************************************************************************\
*                                                                                     *
*                          structs                                                    *
*                                                                                     *
\**************************************************************************************/



/*************************************************************************************\
*                                                                                     *
*                          externals                                                  *
*                                                                                     *
\**************************************************************************************/


/*************************************************************************************\
*                                                                                     *
*                          functions                                                  *
*                                                                                     *
\**************************************************************************************/

#endif	// _HW_CONFIG
