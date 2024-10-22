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

#ifndef CONVEX_DECOMPOSITION_H

#define CONVEX_DECOMPOSITION_H

#include "PlatformConfigHACD.h"

namespace CONVEX_DECOMPOSITION
{

class ConvexResult
{
public:
  ConvexResult(void)
  {
    mHullVcount = 0;
    mHullVertices = 0;
    mHullTcount = 0;
    mHullIndices = 0;
    mHullVolume = 0;
  }

// the convex hull.result
  hacd::HaU32		   	mHullVcount;			// Number of vertices in this convex hull.
  hacd::HaF32 			*mHullVertices;			// The array of vertices (x,y,z)(x,y,z)...
  hacd::HaU32       	mHullTcount;			// The number of triangles int he convex hull
  hacd::HaU32			*mHullIndices;			// The triangle indices (0,1,2)(3,4,5)...
  hacd::HaF32           mHullVolume;		    // the volume of the convex hull.

};

// just to avoid passing a zillion parameters to the method the
// options are packed into this descriptor.
class DecompDesc
{
public:
	DecompDesc(void)
	{
		mVcount = 0;
		mVertices = 0;
		mTcount   = 0;
		mIndices  = 0;
		mDepth    = 10;
		mCpercent = 4;
		mMeshVolumePercent = 0.01f;
		mMaxVertices = 32;
		mCallback = NULL;
  }

// describes the input triangle.
	hacd::HaU32			mVcount;   // the number of vertices in the source mesh.
	const hacd::HaF32	*mVertices; // start of the vertex position array.  Assumes a stride of 3 doubles.
	hacd::HaU32			mTcount;   // the number of triangles in the source mesh.
	const hacd::HaU32	*mIndices;  // the indexed triangle list array (zero index based)
// options
	hacd::HaU32			mDepth;    // depth to split, a maximum of 10, generally not over 7.
	hacd::HaF32			mCpercent; // the concavity threshold percentage.  0=20 is reasonable.
	hacd::HaF32			mMeshVolumePercent; // if less than this percentage of the overall mesh volume, ignore
	hacd::ICallback		*mCallback;

// hull output limits.
	hacd::HaU32		mMaxVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
};

class ConvexDecomposition
{
public:
	virtual hacd::HaU32 performConvexDecomposition(const DecompDesc &desc) = 0; // returns the number of hulls produced.
	virtual void release(void) = 0;
	virtual ConvexResult * getConvexResult(hacd::HaU32 index,bool takeMemoryOwnership) = 0;
protected:
	virtual ~ConvexDecomposition(void) { };
};

ConvexDecomposition * createConvexDecomposition(void);

};

#endif
