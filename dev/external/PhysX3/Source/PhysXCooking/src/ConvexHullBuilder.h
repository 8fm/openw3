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


#ifndef PX_CONVEXHULLBUILDER_H
#define PX_CONVEXHULLBUILDER_H

#include "GuIceSupport.h"
#include "GuConvexMeshData.h"

namespace physx
{
	namespace Gu
	{
		struct EdgeDescData;
	} // namespace Gu

	struct HullTriangleData
	{
		PxU32	mRef[3];
	};

	//! This minimal interface is used to link one data structure to another in a unified way.
	struct SurfaceInterface
	{
		PX_INLINE SurfaceInterface()
			: mNbVerts	(0),
			mVerts		(NULL),
			mNbFaces	(0),
			mDFaces		(NULL),
			mWFaces		(NULL)
		{}

		PX_INLINE SurfaceInterface(
			PxU32			nb_verts,
			const PxVec3*	verts,
			PxU32			nb_faces,
			const PxU32*	dfaces,
			const PxU16*	wfaces
			)
			: mNbVerts	(nb_verts),
			mVerts		(verts),
			mNbFaces	(nb_faces),
			mDFaces		(dfaces),
			mWFaces		(wfaces)
		{}

		PxU32			mNbVerts;	//!< Number of vertices
		const PxVec3*	mVerts;		//!< List of vertices
		PxU32			mNbFaces;	//!< Number of faces
		const PxU32*	mDFaces;	//!< List of faces (dword indices)
		const PxU16*	mWFaces;	//!< List of faces (word indices)

	};

	class ConvexHullBuilder : public Ps::UserAllocated
	{
		public:
												ConvexHullBuilder(Gu::ConvexHullData* hull);
		virtual									~ConvexHullBuilder();

		virtual		bool						Init(const SurfaceInterface& surface, const PxU32 nb_polygons=0, const PxHullPolygon* polygon_data=NULL, const bool* internal=NULL);

		virtual		bool						Save(PxOutputStream& stream, bool platformMismatch)	const;

					bool						CreateEdgeList();
					bool						CreatePolygonData();
					bool						CheckHullPolygons()	const;
					bool						CreateTrianglesFromPolygons();
					bool						ComputeGeomCenter(PxVec3& center) const;

					void						CalculateVertexMapTable(PxU32 nbPolygons, bool userPolygons = false);

					bool						ComputeHullPolygons(const PxU32& nbVerts,const PxVec3* verts, const PxU32& nbTriangles, const PxU32* triangles);

		PX_INLINE	PxU32						ComputeNbPolygons()		const
												{
													if(!mHull->mNbPolygons)	const_cast<ConvexHullBuilder* const>(this)->CreatePolygonData();	// "mutable method"
													return mHull->mNbPolygons;
												}

		PX_INLINE	PxU32						GetNbFaces()const	{ return mNbHullFaces;	}
		PX_INLINE	const HullTriangleData*		GetFaces()	const	{ return mFaces;		}

					PxVec3*						mHullDataHullVertices;
					Gu::HullPolygonData*		mHullDataPolygons;
					PxU8*						mHullDataVertexData8;
					PxU8*						mHullDataFacesByEdges8;
					PxU8*						mHullDataFacesByVertices8;

					Gu::ConvexHullData*			mHull;
	private:
					Gu::EdgeDescData*			mEdgeToTriangles;
					PxU16*						mEdgeData16;	//!< Edge indices indexed by hull polygons
					PxU32						mNbHullFaces;	//!< Number of faces in the convex hull
					HullTriangleData*			mFaces;			//!< Triangles.
	};
}

#endif	// PX_CONVEXHULLBUILDER_H

