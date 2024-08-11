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

#ifndef MODULE_H
#define MODULE_H

#include "ApexInterface.h"
#include "ApexString.h"
#include "NiApexSDK.h"
#include "PsArray.h"

struct NxApexParameter;

namespace physx
{
namespace apex
{

class Module : public physx::UserAllocated
{
public:
	Module();
	void release();

	physx::PxU32 getNbParameters() const;
	NxApexParameter** getParameters();
	void setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value);
	physx::PxF32 getCurrentValue(NxRange<physx::PxU32> range, physx::PxU32 staticIndex, NxInterpolator* = NULL) const;
	physx::PxF32 getCurrentValue(NxRange<physx::PxF32> range, physx::PxU32 staticIndex, NxInterpolator* = NULL) const;

	physx::PxF32 getLODUnitCost() const;
	void setLODUnitCost(physx::PxF32 cost);

	physx::PxF32 getLODBenefitValue() const;
	void setLODBenefitValue(physx::PxF32 value);

	bool getLODEnabled() const;
	void setLODEnabled(bool enabled);

	const char* getName() const;

	/* Framework internal NiModule class methods */
	void destroy();

	NiApexSDK* mSdk;


protected:
	ApexSimpleString name;
	NxModule* mApiProxy;

	physx::Array<NxApexParameter*> mParameters;
	physx::Array<char*> allNames;

	bool mLodEnabled;
	physx::PxF32 mLodUnitCost;
	physx::PxF32 mLodBenefitValue;

	void registerLODParameter(const char* name, NxRange<physx::PxU32> range);
};

}
} // end namespace physx::apex

#endif // MODULE_H
