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

#ifndef GU_VEC_CONVEXHULL_NOSCALE_H
#define GU_VEC_CONVEXHULL_NOSCALE_H

#include "PxPhysXCommonConfig.h"
#include "GuVecConvexHull.h"


namespace physx
{
namespace Gu
{

	class ConvexHullNoScaleV : public ConvexHullV
	{


		public:
		/**
		\brief Constructor
		*/
		PX_SUPPORT_INLINE ConvexHullNoScaleV(): ConvexHullV()
		{
		}

		PX_SUPPORT_INLINE ConvexHullNoScaleV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot)												
		{
			PX_UNUSED(scaleRot);
			PX_UNUSED(_center);

			using namespace Ps::aos;

			hullData = _hullData;	
			const PxVec3* __restrict tempVerts = _hullData->getHullVertices();
			//const PxU8* __restrict polyInds = _hullData->getFacesByVertices8();
			//const HullPolygonData* __restrict polygons = _hullData->mPolygons;
			verts = tempVerts;
			numVerts = _hullData->mNbHullVertices;
			CalculateConvexMargin( _hullData, margin, minMargin, scale);

		//	searchIndex = 0;
			data = _hullData->mBigConvexRawData;

			PxU8* startAddress = (PxU8*)_hullData->mPolygons;
			PxI32 totalPrefetchBytes = PxI32((_hullData->getFacesByVertices8() + _hullData->mNbHullVertices * 3) - startAddress);

			//Prefetch core data
			
			while(totalPrefetchBytes > 0)
			{
				totalPrefetchBytes -= 128;
				Ps::prefetchLine(startAddress);
				startAddress += 128;
			}

			if(data)
			{
				PxI32 totalSize = data->mNbSamples + data->mNbVerts * sizeof(Gu::Valency) + data->mNbAdjVerts;
				startAddress = data->mSamples;
				while(totalSize > 0)
				{
					totalSize -= 128;
					Ps::prefetchLine(startAddress);
					startAddress += 128;
				}
			}
			
		}


		PX_SUPPORT_INLINE ConvexHullNoScaleV(const Gu::ConvexHullData* _hullData, const Ps::aos::Vec3VArg _center, const Ps::aos::FloatVArg _margin, const Ps::aos::FloatVArg _minMargin, const Ps::aos::Vec3VArg scale, const Ps::aos::QuatVArg scaleRot)
		{
			PX_UNUSED(scaleRot);
			PX_UNUSED(_center);
			PX_UNUSED(scale);

			using namespace Ps::aos;

			hullData = _hullData;
			margin = _margin;
			minMargin = _minMargin;
	
			const PxVec3* tempVerts = _hullData->getHullVertices();
			const PxU8* __restrict polyInds = _hullData->getFacesByVertices8();
			const HullPolygonData* __restrict polygons = _hullData->mPolygons;
			verts = tempVerts;
			numVerts = _hullData->mNbHullVertices;

		//	ConstructSkewMatrix(scale, scaleRot, vertex2Shape, shape2Vertex);

			Ps::prefetchLine(tempVerts);
			Ps::prefetchLine(tempVerts,128);
			Ps::prefetchLine(tempVerts,256);

			Ps::prefetchLine(polyInds);
			Ps::prefetchLine(polyInds,128);

			Ps::prefetchLine(polygons);
			Ps::prefetchLine(polygons, 128);
			Ps::prefetchLine(polygons, 256);
		}


		PX_FORCE_INLINE Ps::aos::Vec3V supportPoint(const PxI32 index)const
		{
			using namespace Ps::aos;
			return V3LoadU(verts[index]);
		}


		////This function is used in the full contact manifold generation code
		//PX_PHYSX_COMMON_API Ps::aos::Vec3V supportVertsLocal(const Ps::aos::Vec3VArg dir)const;

		////This function is used in the full contact manifold generation code
		//PX_PHYSX_COMMON_API void populateVerts(const PxU8* inds, PxU32 numInds, const PxVec3* originalVerts, Ps::aos::Vec3V* verts) const;

		PX_SUPPORT_INLINE void populateVerts(const PxU8* inds, PxU32 numInds, const PxVec3* originalVerts, Ps::aos::Vec3V* verts)const
		{
			using namespace Ps::aos;

			for(PxU32 i=0; i<numInds; ++i)
			{
				verts[i] = V3LoadU(originalVerts[inds[i]]);
			}
		}
		

		//This function is used in epa
		//dir is in the shape space
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir)const
		{
			using namespace Ps::aos;
			const PxU32 maxIndex = supportVertexIndex(dir);
			return V3LoadU(verts[maxIndex]);
		}

		//this is used in the sat test for the full contact gen
		PX_SUPPORT_INLINE void supportLocal(const Ps::aos::Vec3VArg dir, Ps::aos::FloatV& min, Ps::aos::FloatV& max)const
		{
			supportVertexMinMax(dir, min, max);
		}


		//This function is used in epa
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob)const
		{
			using namespace Ps::aos;
		
			//transform dir into the shape space
			const Vec3V _dir = aTob.rotateInv(dir);//relTra.rotateInv(dir);
			const Vec3V maxPoint =supportLocal(_dir);
			//translate maxPoint from shape space of a back to the b space
			return aTob.transform(maxPoint);//relTra.transform(maxPoint);
		}

		//dir in the shape space, this function is used in gjk
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;
			//scale dir and put it in the vertex space, for non-uniform scale, we don't want the scale in the dir, therefore, we are using
			//the transpose of the inverse of shape2Vertex(which is vertex2shape). This will allow us igore the scale and keep the rotation
			//get the extreme point index
			const PxU32 maxIndex = supportVertexIndex(dir);
			index = maxIndex;
			const Vec3V p = V3LoadU(verts[index]);
			support = p;
			return p;
		}

		//this function is used in gjk
		PX_SUPPORT_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob, Ps::aos::Vec3V& support, PxI32& index)const
		{
			using namespace Ps::aos;

			//transform dir from b space to the shape space of a space
			const Vec3V _dir = aTob.rotateInv(dir);//relTra.rotateInv(dir);//M33MulV3(skewInvRot, dir);
			const Vec3V p = supportLocal(_dir, support, index);
			//transfrom from a to b space
			const Vec3V ret = aTob.transform(p);
			support = ret;
			return ret;
		}


	};

	#ifdef PX_PS3
	PX_FORCE_INLINE ConvexHullNoScaleV* PX_CONVEX_TO_NOSCALECONVEX(ConvexHullV* x)
	{
		return physx::PxUnionCast<ConvexHullNoScaleV*, ConvexHullV*>(x);
	}
	#else
	#define PX_CONVEX_TO_NOSCALECONVEX(x)			((ConvexHullNoScaleV*)(x))
	#endif



}

}

#endif	// 
