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

#include "GuGeometryUnion.h"
#include "GuPCMShapeConvex.h"
#include "GuPCMContactGen.h"
#include "CmRenderOutput.h"

using namespace physx;
using namespace Gu;

namespace physx
{
namespace Gu
{
	const PxU8 gPCMBoxPolygonData[24] = 
	{
		0, 3, 2, 1,
		1, 2, 6, 5,
		5, 6, 7, 4,
		4, 7, 3, 0,
		3, 7, 6, 2,
		4, 0, 1, 5,
	};

   const PxU8 gPCMTrianglePolygonData[3] = 
   {
		0, 1, 2
	};

   Gu::PCMPolygonalBox::PCMPolygonalBox(const PxVec3& halfSide) : mHalfSide(halfSide)
	{
		//Precompute the convex data
		//     7+------+6			0 = ---
		//     /|     /|			1 = +--
		//    / |    / |			2 = ++-
		//   / 4+---/--+5			3 = -+-
		// 3+------+2 /    y   z	4 = --+
		//  | /    | /     |  /		5 = +-+
		//  |/     |/      |/		6 = +++
		// 0+------+1      *---x	7 = -++

		PxVec3 minimum = -mHalfSide;
		PxVec3 maximum = mHalfSide;
		// Generate 8 corners of the bbox
		mVertices[0] = PxVec3(minimum.x, minimum.y, minimum.z);
		mVertices[1] = PxVec3(maximum.x, minimum.y, minimum.z);
		mVertices[2] = PxVec3(maximum.x, maximum.y, minimum.z);
		mVertices[3] = PxVec3(minimum.x, maximum.y, minimum.z);
		mVertices[4] = PxVec3(minimum.x, minimum.y, maximum.z);
		mVertices[5] = PxVec3(maximum.x, minimum.y, maximum.z);
		mVertices[6] = PxVec3(maximum.x, maximum.y, maximum.z);
		mVertices[7] = PxVec3(minimum.x, maximum.y, maximum.z);

		//Setup the polygons
		for(PxU8 i=0; i < 6; i++)
		{
			mPolygons[i].mNbVerts = 4;
			mPolygons[i].mVRef8 = i*4;
		}

		// ### planes needs *very* careful checks
		// X axis
		mPolygons[1].mPlane.n = PxVec3(1.0f, 0.0f, 0.0f);
		mPolygons[1].mPlane.d = -mHalfSide.x;
		mPolygons[3].mPlane.n = PxVec3(-1.0f, 0.0f, 0.0f);
		mPolygons[3].mPlane.d = -mHalfSide.x;
		
		mPolygons[1].mMinIndex = 0;
		mPolygons[3].mMinIndex = 1;

	//	mPolygons[1].mMinObsolete = -mHalfSide.x;
	//	mPolygons[3].mMinObsolete = -mHalfSide.x;

		PX_ASSERT(mPolygons[1].getMin(mVertices) == -mHalfSide.x); 
		PX_ASSERT(mPolygons[3].getMin(mVertices) == -mHalfSide.x);


		// Y axis
		mPolygons[4].mPlane.n = PxVec3(0.f, 1.0f, 0.0f);
		mPolygons[4].mPlane.d = -mHalfSide.y;
		mPolygons[5].mPlane.n = PxVec3(0.0f, -1.0f, 0.0f);
		mPolygons[5].mPlane.d = -mHalfSide.y;

		mPolygons[4].mMinIndex = 0;
		mPolygons[5].mMinIndex = 2;
	//	mPolygons[4].mMinObsolete = -mHalfSide.y;
	//	mPolygons[5].mMinObsolete = -mHalfSide.y;

		PX_ASSERT(mPolygons[4].getMin(mVertices) == -mHalfSide.y); 
		PX_ASSERT(mPolygons[5].getMin(mVertices) == -mHalfSide.y);

		// Z axis
		mPolygons[2].mPlane.n = PxVec3(0.f, 0.0f, 1.0f);
		mPolygons[2].mPlane.d = -mHalfSide.z;
		mPolygons[0].mPlane.n = PxVec3(0.0f, 0.0f, -1.0f);
		mPolygons[0].mPlane.d = -mHalfSide.z;

		mPolygons[2].mMinIndex = 0;
		mPolygons[0].mMinIndex = 4;
	//	mPolygons[2].mMinObsolete = -mHalfSide.z;
	//	mPolygons[0].mMinObsolete = -mHalfSide.z;
		PX_ASSERT(mPolygons[2].getMin(mVertices) == -mHalfSide.z); 
		PX_ASSERT(mPolygons[0].getMin(mVertices) == -mHalfSide.z);
	}

   void Gu::PCMPolygonalBox::getPolygonalData(Gu::PolygonalData* PX_RESTRICT dst) const
	{
		dst->mCenter				= PxVec3(0.0f, 0.0f, 0.0f);
		dst->mNbVerts				= 8;
		dst->mNbPolygons			= 6;
		dst->mPolygons				= mPolygons;
		dst->mNbEdges				= 0;
		dst->mVerts					= mVertices;
		dst->mPolygonVertexRefs		= gPCMBoxPolygonData;
		dst->mFacesByEdges			= NULL;
		dst->mInternal.mRadius		= 0.0f;
		dst->mInternal.mExtents[0]	= 0.0f;
		dst->mInternal.mExtents[1]	= 0.0f;
		dst->mInternal.mExtents[2]	= 0.0f;
		dst->mPrefetchHull			= NULL;
	}

   Gu::PCMPolygonalTriangle::PCMPolygonalTriangle(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2)
   {
	   mVertices[0]= p0;
	   mVertices[1]= p1;
	   mVertices[2]= p2;

	   const PxVec3 v0 = p0 - p2;
	   const PxVec3 v1 = p1 - p0;
	   mPolygons[0].mPlane.n = (v0.cross(v1)).getNormalized();
	   mPolygons[0].mPlane.d = 0.f;

	   mPolygons[0].mNbVerts = 3;
	   mPolygons[0].mVRef8 = 0;
	   mPolygons[0].mMinIndex = 0;

   }

   void Gu::PCMPolygonalTriangle::getPolygonalData(Gu::PolygonalData* PX_RESTRICT dst) const
	{
		dst->mCenter				= (mVertices[0] + mVertices[1] + mVertices[2])/3;
		dst->mNbVerts				= 3;
		dst->mNbPolygons			= 1;
		dst->mPolygons				= mPolygons;
		dst->mNbEdges				= 0;
		dst->mVerts					= mVertices;
		dst->mPolygonVertexRefs		= gPCMTrianglePolygonData;
		dst->mFacesByEdges			= NULL;
		dst->mInternal.mRadius		= 0.0f;
		dst->mInternal.mExtents[0]	= 0.0f;
		dst->mInternal.mExtents[1]	= 0.0f;
		dst->mInternal.mExtents[2]	= 0.0f;
		dst->mPrefetchHull			= NULL;
	}

    static void gPCMHullPrefetchCB(PxU32 numVerts, const PxVec3* PX_RESTRICT verts)
	{
		PX_UNUSED(numVerts);
		PX_UNUSED(verts);

	#ifdef _XBOX
		const PxU32 vertexSize = numVerts * sizeof(PxVec3);
		const PxU32 prefetchSize = vertexSize<1024 ? vertexSize : 1024;

		const PxU8* PX_RESTRICT adr = reinterpret_cast<const PxU8*>(verts);
	//	const PxU8* PX_RESTRICT end = adr + numVerts * sizeof(PxVec3);
		const PxU8* PX_RESTRICT end = adr + prefetchSize;
		while(adr<end)
		{
			Ps::prefetchLine(adr);
			adr += 128;
		}
	#endif
	}


   	static void getPCMPolygonalData_Convex(Gu::PolygonalData* PX_RESTRICT dst, const Gu::ConvexHullData* PX_RESTRICT src)
	{
		//dst->mCenter			= scaling * src->mCenterOfMass;
		dst->mNbVerts			= src->mNbHullVertices;
		dst->mNbPolygons		= src->mNbPolygons;
		dst->mNbEdges			= src->mNbEdges;
		dst->mPolygons			= src->mPolygons;
		dst->mVerts				= src->getHullVertices();
		dst->mPolygonVertexRefs	= src->getVertexData8();
		dst->mFacesByEdges		= src->getFacesByEdges8();

		dst->mBigData			= src->mBigConvexRawData;

		dst->mPrefetchHull		= gPCMHullPrefetchCB;
	}

	static void getPCMPolygonalData_Convex(Gu::PolygonalData* PX_RESTRICT dst, const Gu::ConvexHullData* PX_RESTRICT src, const Cm::FastVertex2ShapeScaling& scaling)
	{
		dst->mCenter			= scaling * src->mCenterOfMass;
		dst->mNbVerts			= src->mNbHullVertices;
		dst->mNbPolygons		= src->mNbPolygons;
		dst->mNbEdges			= src->mNbEdges;
		dst->mPolygons			= src->mPolygons;
		dst->mVerts				= src->getHullVertices();
		dst->mPolygonVertexRefs	= src->getVertexData8();
		dst->mFacesByEdges		= src->getFacesByEdges8();

		dst->mBigData			= src->mBigConvexRawData;

		dst->mPrefetchHull		= gPCMHullPrefetchCB;
	}


	void getPCMConvexData(const Gu::ConvexHullData* hullData, const bool idtScale, PolygonalData& polyData, const PxConvexMeshGeometryLL& shapeConvex)
	{
	
		PX_ASSERT(!hullData->mAABB.isEmpty());

		Cm::FastVertex2ShapeScaling scaling;
		if(!idtScale)
			scaling.init(shapeConvex.scale);

		//this is used to calculate the convex hull's center of mass
		getPCMPolygonalData_Convex(&polyData, hullData, scaling);

	}

	void getPCMConvexData(const Gu::ConvexHullData* hullData,  const bool idtScale, PolygonalData& polyData)
	{
		PX_ASSERT(!hullData->mAABB.isEmpty());
		PX_UNUSED(idtScale);

		getPCMPolygonalData_Convex(&polyData, hullData);
	}
   
	bool getPCMConvexData(const Gu::GeometryUnion& shape, Cm::FastVertex2ShapeScaling& scaling, PxBounds3& bounds, PolygonalData& polyData)
	{
		const PxConvexMeshGeometryLL& shapeConvex = shape.get<const PxConvexMeshGeometryLL>();

		const bool idtScale = shapeConvex.scale.isIdentity();
		if(!idtScale)
			scaling.init(shapeConvex.scale);

		PX_ASSERT(!shapeConvex.hullData->mAABB.isEmpty());
		bounds = PxBounds3::transformFast(scaling.getVertex2ShapeSkew(), shapeConvex.hullData->mAABB);

		getPCMPolygonalData_Convex(&polyData, shapeConvex.hullData, scaling);

		return idtScale;
	}

}
}
	
