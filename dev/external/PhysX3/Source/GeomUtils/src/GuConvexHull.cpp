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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PsIntrinsics.h"
#include "PsUserAllocated.h"
#include "GuConvexHull.h"
#include "GuHillClimbing.h"
#include "GuBigConvexData.h"
#include "PxMat33.h"
#include "GuCubeIndex.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace physx
{
namespace Gu
{

void initConvexHullData(Gu::ConvexHullData& data)
{
	data.mAABB.setEmpty();
	data.mCenterOfMass = PxVec3(0);
	data.mNbEdges = 0;
	data.mNbHullVertices = 0;
	data.mNbPolygons = 0;
	data.mPolygons = NULL;
	data.mBigConvexRawData = NULL;
	data.mInternal.mRadius = 0.0f;
	data.mInternal.mExtents[0] = data.mInternal.mExtents[1] = data.mInternal.mExtents[2] = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Checks a point is inside the hull.
 *	\param		p	[in] point in local space
 *	\return		true if the hull contains the point
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool convexHullContains(const ConvexHullData& data, const PxVec3& p)
{
	PxU32 Nb = data.mNbPolygons;
	const Gu::HullPolygonData* Polygons = data.mPolygons;
	while(Nb--)
	{
		const PxPlane& pl = Polygons->mPlane;
		if(pl.distance(p) > 0.0f)	return false;
		Polygons++;
	}
	return true;
}

//returns the maximal vertex in shape space
// PT: this function should be removed. We already have 2 different project hull functions in PxcShapeConvex & GuGJKObjectSupport, this one looks like a weird mix of both!
PxVec3 projectHull_(const ConvexHullData& hull,
				   float& minimum, 
				   float& maximum, 
				   const PxVec3& localDir, // expected to be normalized
				   const PxMat33& vert2ShapeSkew)
{
	PX_ASSERT(localDir.isNormalized());

	//use property that x|My == Mx|y for symmetric M to avoid having to transform vertices.
	const PxVec3 vertexSpaceDir = vert2ShapeSkew * localDir;

	const PxVec3* Verts = hull.getHullVertices();
	const PxVec3* bestVert = NULL;

	if(!hull.mBigConvexRawData)	// Brute-force, local space. Experiments show break-even point is around 32 verts.
	{
		PxU32 NbVerts = hull.mNbHullVertices;
		float min_ = PX_MAX_F32;
		float max_ = -PX_MAX_F32;
		while(NbVerts--)
		{
			const float dp = (*Verts).dot(vertexSpaceDir);
//			if(dp < min_)	min_ = dp;
			min_ = physx::intrinsics::selectMin(min_, dp);
			if(dp > max_)	{ max_ = dp; bestVert = Verts; }

			Verts++;
		}
		minimum = min_;
		maximum = max_;

		PX_ASSERT(bestVert != NULL);

		return vert2ShapeSkew * *bestVert;
	}
	else //*/if(1)	// This version is better for objects with a lot of vertices
	{
		const PxU32 Offset = ComputeCubemapNearestOffset(vertexSpaceDir, hull.mBigConvexRawData->mSubdiv);
		PxU32 MinID = hull.mBigConvexRawData->mSamples[Offset];
		PxU32 MaxID = hull.mBigConvexRawData->getSamples2()[Offset];

		localSearch(MinID, -vertexSpaceDir, Verts, hull.mBigConvexRawData);
		localSearch(MaxID, vertexSpaceDir, Verts, hull.mBigConvexRawData);

		minimum = (Verts[MinID].dot(vertexSpaceDir));
		maximum = (Verts[MaxID].dot(vertexSpaceDir));

		PX_ASSERT(maximum >= minimum);

		return vert2ShapeSkew * Verts[MaxID];
	}
}

}

}

//~PX_SERIALIZATION

