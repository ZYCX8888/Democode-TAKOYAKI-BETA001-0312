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
#ifndef _DSP_PLATFORM_MANAGER_
#define _DSP_PLATFORM_MANAGER_

#include <string>
#include <map>
#ifdef XM
#include <inttypes.h>
#else
#include <cinttypes>
#endif

#include <iostream>
#include "ceva_device.h"

namespace CevaPlatform {

	class DspPlatformManager
	{
	private:
		std::map<std::string, IDevice*> devices;
		DspPlatformManager();
		void add(const std::string& name, IDevice* device);
		IDevice* get(const std::string& name);
		void del(const std::string& name);
		IDevice* pDev;
	public:
		~DspPlatformManager();
		static DspPlatformManager& instance();
        template <class T>
        T* addDevice(const std::string& name, const uint32_t ioBaseAddr, const uint32_t devBaseAddr, const uint32_t hostBaseAddr);
        template <class T>
        T* addDevice(const std::string& name, const uint32_t ioBaseAddr, const uint32_t devBaseAddr);
		template <class T>
		T* addDevice(const std::string& name, void* pSim);
		void deleteDevice(const std::string& name);
//         template <class T>
// 		T* addDevice(const std::string& name)
// 		{
// 			T* t = new T(0, 0);
// 			//devices.insert(std::make_pair(name, t));
// 			//add(name, t);
// 			pDev = t;
// 			return t;
// 		}
		template <class T>
		T* getDevice(std::string name)
		{
			return static_cast<T*>(pDev);
			return static_cast<T*>(get(name));
		}
		size_t getNumOfDevies() { return 1;devices.size(); }
	};


    template <class T>
    T* DspPlatformManager::addDevice(const std::string& name, const uint32_t ioBaseAddr, const uint32_t devBaseAddr, const uint32_t hostBaseAddr)
    {
        T* t = new T(ioBaseAddr, devBaseAddr, hostBaseAddr);
        //devices.insert(std::make_pair(name, t));
        //add(name, t);
        pDev = t;
        return t;
    }

    template <class T>
    T* DspPlatformManager::addDevice(const std::string& name, const uint32_t ioBaseAddr, const uint32_t devBaseAddr)
    {
        T* t = new T(ioBaseAddr, devBaseAddr, devBaseAddr);
        //devices.insert(std::make_pair(name, t));
        //add(name, t);
        pDev = t;
        return t;
    }

	template <class T>
	T* DspPlatformManager::addDevice(const std::string& name, void* pSim)
	{
		T* t = new T(pSim);
		//devices.insert(std::make_pair(name, t));
		//add(name, t);
		pDev = t;
		return t;
	}
}
#endif //_DSP_PLATFORM_MANAGER_
