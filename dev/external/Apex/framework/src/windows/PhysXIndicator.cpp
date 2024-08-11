// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PhysXIndicator.h"
#include "AgMMFile.h"

#include <windows.h>
#include <stdio.h>

using namespace physx;

// Scope-based to indicate to NV driver that CPU PhysX is happening
PhysXCpuIndicator::PhysXCpuIndicator() :
	mPhysXDataPtr(NULL)
{
	bool alreadyExists = false;
	
	mPhysXDataPtr = (NvPhysXToDrv_Data_V1*)PhysXCpuIndicator::createIndicatorBlock(mSharedMemConfig, alreadyExists);
	
	if (!mPhysXDataPtr)
	{
		return;
	}
	
	if (!alreadyExists)
	{
		mPhysXDataPtr->bGpuPhysicsPresent = 0;
		mPhysXDataPtr->bCpuPhysicsPresent = 1;
	}
	else
	{
		mPhysXDataPtr->bCpuPhysicsPresent++;
	}

	// init header last to prevent race conditions
	// this must be done because the driver may have already created the shared memory block,
	// thus alreadyExists may be true, even if PhysX hasn't been initialized
	NvPhysXToDrv_Header_Init(mPhysXDataPtr->header);
}

PhysXCpuIndicator::~PhysXCpuIndicator()
{
	if (!mPhysXDataPtr)
	{
		return;
	}
	
	mPhysXDataPtr->bCpuPhysicsPresent--;
	
	mPhysXDataPtr = NULL;
	mSharedMemConfig.destroy();
}

void* PhysXCpuIndicator::createIndicatorBlock(AgMMFile &mmfile, bool &alreadyExists)
{
	char configName[128];
	OSVERSIONINFOEX windowsVersionInfo;	

    // Get the windows version (we can only create Global\\ namespace objects in XP)
	/**
		Operating system		Version number
		----------------		--------------
		Windows 7				6.1
		Windows Server 2008 R2	6.1
		Windows Server 2008		6.0
		Windows Vista			6.0
		Windows Server 2003 R2	5.2
		Windows Server 2003		5.2
		Windows XP				5.1
		Windows 2000			5.0
	**/
    windowsVersionInfo.dwOSVersionInfoSize = sizeof (windowsVersionInfo);
    GetVersionEx((LPOSVERSIONINFO)&windowsVersionInfo);

	if (windowsVersionInfo.dwMajorVersion < 6)
		NvPhysXToDrv_Build_SectionNameXP(GetCurrentProcessId(), configName);
	else
		NvPhysXToDrv_Build_SectionName(GetCurrentProcessId(), configName);

	mmfile.create(configName, sizeof(NvPhysXToDrv_Data_V1), alreadyExists);

	return mmfile.getAddr();
}

//-----------------------------------------------------------------------------------------------------------

PhysXGpuIndicator::PhysXGpuIndicator() :
	mPhysXDataPtr(NULL),
	mAlreadyExists(false),
	mGpuTrigger(false)
{
	mPhysXDataPtr = (NvPhysXToDrv_Data_V1*)PhysXCpuIndicator::createIndicatorBlock(mSharedMemConfig, mAlreadyExists);
	
	// init header last to prevent race conditions
	// this must be done because the driver may have already created the shared memory block,
	// thus alreadyExists may be true, even if PhysX hasn't been initialized
	NvPhysXToDrv_Header_Init(mPhysXDataPtr->header);
}

PhysXGpuIndicator::~PhysXGpuIndicator()
{
	gpuOff();
	
	mPhysXDataPtr = NULL;
	mSharedMemConfig.destroy();
}

// Explicit set functions to indicate to NV driver that GPU PhysX is happening
void PhysXGpuIndicator::gpuOn()
{
	if (!mPhysXDataPtr || mGpuTrigger)
	{
		return;
	}

	if (!mAlreadyExists)
	{
		mPhysXDataPtr->bGpuPhysicsPresent = 1;
		mPhysXDataPtr->bCpuPhysicsPresent = 0;
	}
	else
	{
		mPhysXDataPtr->bGpuPhysicsPresent++;
	}

	mGpuTrigger = true;
}

void PhysXGpuIndicator::gpuOff()
{
	if (!mPhysXDataPtr || !mGpuTrigger)
	{
		return;
	}

	mPhysXDataPtr->bGpuPhysicsPresent--;

	mGpuTrigger = false;
}
