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

#ifndef GU_SEPARATINGAXES_H
#define GU_SEPARATINGAXES_H

#include "PsArray.h"
#include "PxVec3.h"
#include "PxPhysXCommonConfig.h"

namespace physx
{
namespace Gu
{

// PT: define this value to use a fixed-size buffer. If undefined, uses a dynamic array.
// PT: this is a number of axes. Multiply by sizeof(PxVec3) for size in bytes.
#define SEP_AXIS_FIXED_MEMORY	256

// This class holds a list of potential separating axes.
// - the orientation is irrelevant so V and -V should be the same vector
// - the scale is irrelevant so V and n*V should be the same vector
// - a given separating axis should appear only once in the class
class PX_PHYSX_COMMON_API SeparatingAxes
{
public:
	PX_INLINE SeparatingAxes()
#ifdef SEP_AXIS_FIXED_MEMORY
		: mNbAxes(0)
#else
		: mAxes(PX_DEBUG_EXP("separatingAxes"))
#endif
	{}

	bool addAxis(const PxVec3& axis);

	PX_FORCE_INLINE const PxVec3* getAxes() const
	{
#ifdef SEP_AXIS_FIXED_MEMORY
		return mAxes;
#else
		return mAxes.begin();
#endif
	}

	PX_FORCE_INLINE PxU32 getNumAxes() const
	{
#ifdef SEP_AXIS_FIXED_MEMORY
		return mNbAxes;
#else
		return (PxU32)mAxes.size();
#endif
	}

	PX_FORCE_INLINE void reset()
	{
#ifdef SEP_AXIS_FIXED_MEMORY
		mNbAxes = 0;
#else
		mAxes.clear();
#endif
	}

private:
#ifdef SEP_AXIS_FIXED_MEMORY
	PxU32	mNbAxes;
	PxVec3	mAxes[SEP_AXIS_FIXED_MEMORY];
#else
	Ps::Array<PxVec3> mAxes;
#endif
};


//a bunch of inlines used by box convex and convex convex:

enum PxcSepAxisType
{
	SA_NORMAL0,		// Normal of object 0
	SA_NORMAL1,		// Normal of object 1
	SA_EE			// Cross product of edges
};

}
}

#endif
