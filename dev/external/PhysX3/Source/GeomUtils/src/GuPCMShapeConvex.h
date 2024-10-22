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

#ifndef GU_PCM_SHAPE_CONVEX_H
#define GU_PCM_SHAPE_CONVEX_H

#include "GuConvexSupportTable.h"
#include "GuPersistentContactManifold.h"
#include "GuShapeConvex.h"


#if 1
#ifndef __SPU__
extern physx::Gu::PersistentContactManifold* gManifold;
#endif
#define	PCM_LOW_LEVEL_DEBUG	1
#endif

namespace physx
{

namespace Gu
{

	/*enum FeatureStatus
	{
		POLYDATA0,
		POLYDATA1,
		EDGE0,
		EDGE1
	};*/

	extern const PxU8 gPCMBoxPolygonData[24];

	struct PCMPolygonalData
	{
		// Data
		PxVec3								mCenter;
		PxU32								mNbVerts;
		PxU32								mNbPolygons;
		PxU32								mNbEdges;
		const Gu::HullPolygonData*			mPolygons;
		const PxVec3*						mVerts;
		const PxU8*							mPolygonVertexRefs;
		const PxU8*							mFacesByEdges;
		//Gu::InternalObjectsData				mInternal;

		PX_FORCE_INLINE const PxU8*	getPolygonVertexRefs(const Gu::HullPolygonData& poly)	const
		{
			return mPolygonVertexRefs + poly.mVRef8;
		}
	};

	
	class PCMPolygonalBox
	{
	public:
									PCMPolygonalBox(const PxVec3& halfSide);

									void					getPolygonalData(Gu::PolygonalData* PX_RESTRICT dst)	const;

			const PxVec3&			mHalfSide;
			PxVec3					mVertices[8];
			Gu::HullPolygonData		mPolygons[6];
	private:
			PCMPolygonalBox& operator=(const PCMPolygonalBox&);
	};

	class PCMPolygonalTriangle
	{
	public:
									PCMPolygonalTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2);

									void					getPolygonalData(Gu::PolygonalData* PX_RESTRICT dst)	const;

			PxVec3					mVertices[3];
			Gu::HullPolygonData		mPolygons[1];
	};


	void getPCMConvexData(const Gu::ConvexHullData* hullData, const bool idtScale, Gu::PolygonalData& polyData);
	bool getPCMConvexData(const Gu::GeometryUnion& shape, Cm::FastVertex2ShapeScaling& scaling, PxBounds3& bounds, Gu::PolygonalData& polyData);

}
}

#endif