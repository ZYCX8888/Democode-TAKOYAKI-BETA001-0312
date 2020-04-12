/********************************************************************************************************************************************************
Copyright (C) CEVA(R) Inc. All rights reserved

This information embodies materials and concepts, which are proprietary and confidential to CEVA Inc., and is made available solely pursuant to the 
terms of a written license agreement, or NDA, or another written agreement, as applicable (“CEVA Agreement”), with CEVA Inc. or any of its subsidiaries 
(“CEVA”). 

This information can be used only with the written permission from CEVA, in accordance with the terms and conditions stipulated in the CEVA Agreement, 
under which the information has been supplied and solely as expressly permitted for the purpose specified in the CEVA Agreement.
This information is made available exclusively to licensees or parties that have received express written authorization from CEVA to download or receive 
the information and have agreed to the terms and conditions of the CEVA Agreement.
IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE CEVA AGREEMENT, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS INFORMATION.

The information contained in this document is subject to change without notice and does not represent a commitment on any part of CEVA. Unless 
specifically agreed otherwise in the CEVA Agreement, CEVA make no warranty of any kind with regard to this material, including, but not limited to 
implied warranties of merchantability and fitness for a particular purpose whether arising out of law, custom, conduct or otherwise.

While the information contained herein is assumed to be accurate, CEVA assumes no responsibility for any errors or omissions contained herein, 
and assumes no liability for special, direct, indirect or consequential damage, losses, costs, charges, claims, demands, fees or expenses, of any nature
or kind, which are incurred in connection with the furnishing, performance or use of this material.

This document contains proprietary information, which is protected by U.S. and international copyright laws. All rights reserved. 
No part of this document may be reproduced, photocopied, or translated into another language without the prior written consent of CEVA. 
********************************************************************************************************************************************************/

/**
* \file 
*  Defines the enumeration types for the DSP cores and modules
*/



#ifndef _DSP_AND_MODULE_TYPES_H
#define	_DSP_AND_MODULE_TYPES_H

/*!
* \addtogroup CevaLinkClient
*  @{
*/

#define NUM_OF_MODULE_TYPES	(E_MT_BYPASS + 1)

/**
* \brief The DSP core type enumeration
*/
typedef enum 
{
	E_DT_UNKNOWN = -1,	/**< Undefined */
	E_DT_TKL3,			/**< CEVA-TeakLite-III */
	E_DT_CEVAX,			/**< CEVA-X	*/
	E_DT_CEVAXC,		/**< CEVA-XC */
	E_DT_MM3101,		/**< CEVA-MM3101 */	
    E_DT_XM4,		    /**< CEVA-XM4 */
	E_DT_XM6		    /**< CEVA-XM6 */
} dsp_type_enum;

/**
* \brief The module type enumeration
*/
typedef enum
{
	E_MT_UNKNOWN = -1,	/**< Undefined */
	E_MT_MP3 = 0,		/**< MP3 */
	E_MT_WMA,			/**< WMA */
	E_MT_AAC,			/**< AAC */
	E_MT_DOLBY,			/**< Dolby Digital */
	E_MT_BYPASS,		/**< Bypass */
	E_MT_SMARTFRAME,	/**< Smart Frame */
	E_MT_NVISO,			/**< Nviso module */
    E_MT_DVS,           /**< Digital Video Stabilizer */
    E_MT_GTRANS,        /**< Geometrical Transformation */	
	E_MT_PCM,			/**< PCM */
	E_MT_EFFECT_MANAGER,
	E_MT_DEBUG,
	E_MT_NUM
} dsp_module_type_enum;

/** @}*/

#endif


