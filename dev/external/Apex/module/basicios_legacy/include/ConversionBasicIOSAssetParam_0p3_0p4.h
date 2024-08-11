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

#ifndef CONVERSIONBASICIOSASSETPARAM_0P3_0P4H_H
#define CONVERSIONBASICIOSASSETPARAM_0P3_0P4H_H

#include "ParamConversionTemplate.h"
#include "BasicIOSAssetParam_0p3.h"
#include "BasicIOSAssetParam_0p4.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<BasicIOSAssetParam_0p3, BasicIOSAssetParam_0p4, 3, 4> ConversionBasicIOSAssetParam_0p3_0p4Parent;

class ConversionBasicIOSAssetParam_0p3_0p4: ConversionBasicIOSAssetParam_0p3_0p4Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionBasicIOSAssetParam_0p3_0p4));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionBasicIOSAssetParam_0p3_0p4)(t) : 0;
	}

protected:
	ConversionBasicIOSAssetParam_0p3_0p4(NxParameterized::Traits* t) : ConversionBasicIOSAssetParam_0p3_0p4Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (PxU32)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		// copy 'collisionGroupMaskName' to 'collisionFilterDataName'
		NxParameterized::Handle handle(*mNewData, "collisionFilterDataName");
		handle.setParamString(mLegacyData->collisionGroupMaskName);

		return true;
	}
};

} // namespace legacy
} // namespace apex
} // namespace physx

#endif
