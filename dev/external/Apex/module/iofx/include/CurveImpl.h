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

#ifndef __CURVE_IMPL_H__
#define __CURVE_IMPL_H__

#include "NxApex.h"
#include "foundation/PxAssert.h"
#include "NxParameterized.h"
#include "PsArray.h"
#include "NxCurve.h"
#include <PsShare.h>

namespace physx
{
namespace apex
{
namespace iofx
{

typedef physx::Array<NxVec2R> Vec2RPointArray;

/**
	The CurveImpl is a class for storing control points on a curve and evaluating the results later.
*/
class CurveImpl : public NxCurve
{
public:
	CurveImpl()
	{}

	~CurveImpl()
	{}

	/**
		Retrieve the output Y for the specified input x, based on the properties of the stored curve described
		by mControlPoints.
	*/
	physx::PxF32 evaluate(physx::PxF32 x) const;

	/**
		Add a control point to the list of control points, returning the index of the new point.
	*/
	physx::PxU32 addControlPoint(const NxVec2R& controlPoint);

	/**
		Add a control points to the list of control points.  Assuming the
		hPoints points to a list of vec2s
	*/
	void addControlPoints(::NxParameterized::Interface* param, ::NxParameterized::Handle& hPoints);

	/**
		Locates the control points that contain x, placing the resulting control points in the two
		out parameters. Returns true if the points were found, false otherwise. If the points were not
		found, the output variables are untouched
	*/
	bool calculateControlPoints(physx::PxF32 x, NxVec2R& outXPoints, NxVec2R& outYPoints) const;

	/**
		Locates the first control point with x larger than xValue or the nimber of control points if such point doesn't exist
	*/
	physx::PxU32 calculateFollowingControlPoint(physx::PxF32 xValue) const;

	///get the array of control points
	const NxVec2R* getControlPoints(physx::PxU32& outCount) const;

private:
	// mControlPoints is a sorted list of control points for a curve. Currently, the curve is a lame
	// lirp'd curve. We could add support for other curvetypes in the future, either bezier curves,
	// splines, etc.
	Vec2RPointArray mControlPoints;
};


}
}
} // namespace apex

#endif /* __CURVE_IMPL_H__ */
