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
 * CevaLinkClientFactory.h
 *
 *  Created on: Jan 08, 2014
 *      Author: yurys
 */

#ifndef _CEVALINKCLIENTFACTORY_H_
#define _CEVALINKCLIENTFACTORY_H_

/*!
* \addtogroup CevaLinkClient
*  @{
*/


#include "ICevaLinkClient.h"

namespace CEVA_AMF {

//! Factory class for the concrete CEVA Link client instances
class CevaLinkClientFactory {
public:
	//! Enumeration type for the CEVA Link classes
    enum ClientType { CLIENT_TYPE_BASE };

    /// \brief Creates an instance of the given client type. The instance must be freed by the user.
    /// \param[in] type The enumeration value of the client type
    /// \return A pointer to the new instance
    static ICevaLinkClient* create( ClientType type );
};

}

/** @}*/
#endif /* _CEVALINKCLIENTFACTORY_H_ */
