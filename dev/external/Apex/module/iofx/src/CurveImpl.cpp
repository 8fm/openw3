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

#include "NxApex.h"
#include "foundation/PxAssert.h"
#include "NxParameterized.h"
#include "NxCurve.h"
#include "CurveImpl.h"

namespace physx
{
namespace apex
{
namespace iofx
{

/**
	Linear interpolation. "in" and "out" stands here for X and Y coordinates of the control points
*/
inline physx::PxF32 lerp(physx::PxF32 inCurrent, physx::PxF32 inMin, physx::PxF32 inMax, physx::PxF32 outMin, physx::PxF32 outMax)
{
	if (inMin == inMax)
	{
		return outMin;
	}

	return ((inCurrent - inMin) / (inMax - inMin)) * (outMax - outMin) + outMin;
}

/**
	The CurveImpl is a class for storing control points on a curve and evaluating the results later.
*/

/**
	Retrieve the output Y for the specified input x, based on the properties of the stored curve described
	by mControlPoints.
*/
physx::PxF32 CurveImpl::evaluate(physx::PxF32 x) const
{
	NxVec2R xPoints, yPoints;
	if (calculateControlPoints(x, xPoints, yPoints))
	{
		return lerp(x, xPoints[0], xPoints[1], yPoints[0], yPoints[1]);
	}
	else if (mControlPoints.size() == 1)
	{
		return mControlPoints[0].y;
	}
	else
	{
		// This is too noisy for editors...
		//PX_ASSERT(!"Unable to find control points that contained the specified curve parameter");
		return 0;
	}
}

/**
	Add a control point to the list of control points, returning the index of the new point.
*/
physx::PxU32 CurveImpl::addControlPoint(const NxVec2R& controlPoint)
{
	physx::PxU32 index = calculateFollowingControlPoint(controlPoint.x);

	if (index == mControlPoints.size())
	{
		// add element to the end
		NxVec2R& v2 = mControlPoints.insert();
		v2 = controlPoint;
	}
	else
	{
		// memmove all elements from index - end to index+1 - new_end
		physx::PxU32 oldSize = mControlPoints.size();
		mControlPoints.insert();
		memmove(&mControlPoints[index + 1], &mControlPoints[index], sizeof(NxVec2R) * (oldSize - index));
		mControlPoints[index] = controlPoint;
	}
	return index;
}

/**
	Add a control points to the list of control points.  Assuming the
	hPoints points to a list of vec2s
*/
void CurveImpl::addControlPoints(::NxParameterized::Interface* param, ::NxParameterized::Handle& hPoints)
{
	::NxParameterized::Handle ih(*param), hMember(*param);
	int arraySize = 0;
	PX_ASSERT(hPoints.getConstInterface() == param);
	hPoints.getArraySize(arraySize);
	for (int i = 0; i < arraySize; i++)
	{
		hPoints.getChildHandle(i, ih);
		NxVec2R tmpVec2;
		ih.getChildHandle(0, hMember);
		hMember.getParamF32(tmpVec2.x);
		ih.getChildHandle(1, hMember);
		hMember.getParamF32(tmpVec2.y);

		addControlPoint(tmpVec2);
	}
}

/**
	Locates the control points that contain x, placing the resulting control points in the two
	out parameters. Returns true if the points were found, false otherwise. If the points were not
	found, the output variables are untouched
*/
bool CurveImpl::calculateControlPoints(physx::PxF32 x, NxVec2R& outXPoints, NxVec2R& outYPoints) const
{
	physx::PxU32 controlPointSize = mControlPoints.size();
	if (controlPointSize < 2)
	{
		return false;
	}

	physx::PxU32 followControlPoint = calculateFollowingControlPoint(x);
	if (followControlPoint == 0)
	{
		outXPoints[0] = outXPoints[1] = mControlPoints[0].x;
		outYPoints[0] = outYPoints[1] = mControlPoints[0].y;
		return true;
	}
	else if (followControlPoint == controlPointSize)
	{
		outXPoints[0] = outXPoints[1] = mControlPoints[followControlPoint - 1].x;
		outYPoints[0] = outYPoints[1] = mControlPoints[followControlPoint - 1].y;
		return true;
	}

	outXPoints[0] = mControlPoints[followControlPoint - 1].x;
	outXPoints[1] = mControlPoints[followControlPoint].x;

	outYPoints[0] = mControlPoints[followControlPoint - 1].y;
	outYPoints[1] = mControlPoints[followControlPoint].y;

	return true;
}

/**
	Locates the first control point with x larger than xValue or the nimber of control points if such point doesn't exist
*/
physx::PxU32 CurveImpl::calculateFollowingControlPoint(physx::PxF32 xValue) const
{
	// TODO: This could be made O(log(N)), but I think there should
	// be so few entries that it's not worth the code complexity.
	physx::PxU32 cpSize = mControlPoints.size();

	for (physx::PxU32 u = 0; u < cpSize; ++u)
	{
		if (xValue <= mControlPoints[u].x)
		{
			return u;
		}
	}

	return cpSize;
}

///get the array of control points
const NxVec2R* CurveImpl::getControlPoints(physx::PxU32& outCount) const
{
	outCount = mControlPoints.size();
	if (outCount)
	{
		return &mControlPoints.front();
	}
	else
	{
		// LRR: there's more to this, chase this down later
		return NULL;
	}
}

}
}
} // namespace physx::apex
