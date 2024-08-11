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

#ifndef __CONVERSIONCLOTHINGACTORPARAM_0P0_0P1H__
#define __CONVERSIONCLOTHINGACTORPARAM_0P0_0P1H__

#include "ParamConversionTemplate.h"
#include "ClothingActorParam_0p0.h"
#include "ClothingActorParam_0p1.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingActorParam_0p0, ClothingActorParam_0p1, 0, 1> ConversionClothingActorParam_0p0_0p1Parent;

class ConversionClothingActorParam_0p0_0p1: ConversionClothingActorParam_0p0_0p1Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingActorParam_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingActorParam_0p0_0p1)(t) : 0;
	}

protected:
	ConversionClothingActorParam_0p0_0p1(NxParameterized::Traits* t) : ConversionClothingActorParam_0p0_0p1Parent(t) {}

	bool convert()
	{
		//TODO:
		//	write custom conversion code here using mNewData and mLegacyData members
		//	note that members with same names were already copied in parent converter
		// and mNewData was already initialized with default values

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif
