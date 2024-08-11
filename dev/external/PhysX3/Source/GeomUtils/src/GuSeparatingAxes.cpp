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

#include "GuSeparatingAxes.h"

using namespace physx;

// Disabled VMX128 version pending further testing.
//#ifndef PX_VMX
#if 1

union FloatInt
{
	float	f;
	PxU32	i;
};

bool Gu::SeparatingAxes::addAxis(const PxVec3& axis)
{
#ifdef _XBOX

/*	// 5 ticks
	size_t numAxes = mNbAxes;
	const PxVec3* PX_RESTRICT axes = mAxes;
	const PxVec3* PX_RESTRICT axes_end = axes + numAxes;
	const Cm::PxSimd::Vector4 axis_4 = Cm::PxSimd::load(axis);
	const Cm::PxSimd::Vector4 threshold = Cm::PxSimd::load(0.9999f);
	while(axes<axes_end)
	{
		const Cm::PxSimd::Vector4 axe_4 = Cm::PxSimd::load(*axes);
		const Cm::PxSimd::Vector4 dp_4 = Cm::PxSimd::dot(axe_4, axis_4);
		if(Cm::PxSimd::greaterXBool(Cm::PxSimd::abs(dp_4), threshold))
			return false;

		axes++;
	}*/

	// PT: this version has no FCMps but LHS instead - 4 ticks
	const float val = 0.9999f;
	const PxU32 Limit = (PxU32&)val;

	size_t numAxes = mNbAxes;
	const PxVec3* PX_RESTRICT axes = mAxes;
	const PxVec3* PX_RESTRICT axes_end = axes + numAxes;

	FloatInt batch[16];
	while(axes<axes_end)
	{
		const PxU32 nbToGo = numAxes>=16 ? 16 : numAxes;
		numAxes -= nbToGo;

		for(PxU32 i=0;i<nbToGo;i++)
			batch[i].f = axis.dot(axes[i]);
		axes += nbToGo;

		for(PxU32 i=0;i<nbToGo;i++)
		{
			if( (batch[i].i & ~PX_SIGN_BITMASK) > Limit)
				return false;
		}
	}
#else
	// PT: this version has FCMPs but no LHS - 5 ticks
	PxU32 numAxes = getNumAxes();
	const PxVec3* PX_RESTRICT axes = getAxes();
	const PxVec3* PX_RESTRICT axes_end = axes + numAxes;
	while(axes<axes_end)
	{
		if(PxAbs(axis.dot(*axes))>0.9999f)
			return false;
		axes++;
	}
#endif

#ifdef SEP_AXIS_FIXED_MEMORY
	if(mNbAxes<SEP_AXIS_FIXED_MEMORY)
	{
		mAxes[mNbAxes++] = axis;
		return true;
	}

	// PT: is that a good idea here?
//	Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "PxcSeparatingAxes::addAxis: axis buffer is full!");
	return false;
#else
	mAxes.pushBack(axis);
	return true;
#endif
}

#else
/*
TODO:
	Unroll loop
	store the axis array aligned.
*/

#include "CmSimd.h"

// PT: the initial test should be removed in that version as well, if we ever use it
bool Gu::SeparatingAxes::addAxis(const PxVec3& axis)
{

	/*
	PxVec3 realAxis;
	if(axis.x < 0.0f) 
		realAxis = -axis;
	else
		realAxis = axis;
	*/

	PxSimd::Vector4 realAxis4 = PxSimd::load(axis);
	PxSimd::Vector4 mask = PxSimd::less(realAxis4, PxSimd::zero());

	mask = PxSimd::and4(mask, PxSimd::xMask());

	realAxis4 = PxSimd::select(realAxis4, PxSimd::subtract(PxSimd::zero(), realAxis4), mask);

	PxU32 numAxes = mAxes.getSize();
	const PxVec3* axes = mAxes.begin();

	PxSimd::Vector4 bestDot = PxSimd::floatMin();
	while(numAxes--)
	{
		PxSimd::Vector4 axis = PxSimd::load(*axes);
		PxSimd::Vector4 absDot = PxSimd::abs(PxSimd::dot(realAxis4, axis));

		bestDot = PxSimd::maximum(absDot, bestDot);
		axes++;

		/*if(PxAbs(realAxis.dot(*axes))>0.9999f) return false;
		axes++;*/
	}

	PxSimd::Vector4 threshold = PxSimd::load(0.9999f);
	
	if(PxSimd::greaterXBool(bestDot, threshold))
		return false;

	// 	mAxes.insert(realAxis);
	PxSimd::store(mAxes.insert(), realAxis4);

	return true;

}
#endif
