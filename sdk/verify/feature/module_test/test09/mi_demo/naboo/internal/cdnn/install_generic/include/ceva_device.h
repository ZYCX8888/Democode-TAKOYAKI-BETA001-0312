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
\**************************************************************************************/
#ifndef _I_CEVA_DEVICE_
#define _I_CEVA_DEVICE_

#include <string>
#include <iostream>
#ifdef XM
#include <inttypes.h>
#else
#include <cinttypes>
#endif

namespace CevaPlatform {

	class DeviceImpl;

	class Version {
	public:
		Version();
		Version(uint8_t version, uint8_t major, uint8_t minor);
		operator const char*() const;
		operator uint32_t()
		{
			union {
				char _char[4];
				uint32_t _uint;
			} ver;
			ver._char[0] = major;
			ver._char[1] = minor;
			ver._char[2] = build;
			ver._char[3] = 0;
			return ver._uint;
		}

	private:
		void buildString();
		//uint8_t version;
		uint8_t major;
		uint8_t minor;
		uint8_t build;
		std::string str;
	};

	class AddressRange {
	public:
		AddressRange() {};
		AddressRange(uint32_t baseAddress, uint32_t endAddress)
			: baseAddress(baseAddress), endAddress(endAddress) {};
		operator const std::string() const;
	private:
		uint32_t baseAddress;
		uint32_t endAddress;
	};

	class IDevice {
	private:
	public:
		//static const std::string type;
		enum Status {
			STATUS_OK,
			STATUS_FAILED
		};
        //IDevice();
        IDevice(const uint32_t ioBaseAddr, const uint32_t devBaseAddr, const uint32_t hostBaseAddr = 0);
        virtual ~IDevice();
		virtual const Version& getDriverVersion() const;
		virtual const Version& getHwVersion() const;
		//virtual operator const std::string&() const;
		virtual operator const char*() const;
	protected:
		DeviceImpl* impl;
        const uint32_t ioBaseAddr;
        const uint32_t devBaseAddr;
        const uint32_t hostBaseAddr;
	};

    // Base device implementation (abstract)
    class DeviceImpl {
    protected:
        void setDecription(const std::string& str);
        std::string desc;
        const uint32_t ioBaseAddr;
        const uint32_t devBaseAddr;
        const uint32_t hostBaseAddr;
    public:
        DeviceImpl(const uint32_t ioBaseAddr, const uint32_t devBaseAddr, const uint32_t hostBaseAddr) : desc("CEVA-Device"),
            ioBaseAddr(ioBaseAddr), devBaseAddr(devBaseAddr), hostBaseAddr(hostBaseAddr) {};
        virtual ~DeviceImpl() {}
        virtual const Version& getDriverVersion() const = 0;
        virtual const Version& getHwVersion() const = 0;
        virtual operator const char*() const { return desc.c_str(); }
    };

} // namespace
#endif // !_I_CEVA_DEVICE_
