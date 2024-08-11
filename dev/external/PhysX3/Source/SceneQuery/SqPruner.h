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

#ifndef SQ_PRUNER_H
#define SQ_PRUNER_H

#include "GuBoxConversion.h"
#include "GuGeomUtilsInternal.h"
#include "GuConvexMesh.h"
#include "CmSPU.h"
#include "PxQueryReport.h" // for PxAgain
#include "GuSPUHelpers.h" // for GU_FETCH_CONVEX_DATA
#include "GuConvexUtilsInternal.h" // for computeOBBAroundConvex
#include "CmMemFetch.h"

namespace physx
{

using namespace Cm;

namespace Sq
{

typedef PxU32 PrunerHandle;
static const PrunerHandle INVALID_PRUNERHANDLE = 0xFFffFFff;

class ShapeData
{
public:

	ShapeData(const PxGeometry& g, const PxTransform& t, PxReal inflation):mWorldTransform(t)
	{

		// a copy is needed not to destroy the input geometry
		// some types are converted to OBBs for queries
		mGeometry.set(g); 
		// BucketPruner uses AABB as cullBox
		PxBounds3 aabb;
		mGeometry.computeBounds(aabb,mWorldTransform,inflation, NULL);
		// grow the AABB by 1% for overlap/sweep/GJK accuracy
		mInflatedAABB = PxBounds3::centerExtents(aabb.getCenter(),aabb.getExtents()*1.01f);
		
		PxGeometryType::Enum theType = g.getType();
		switch(theType)
		{
		case PxGeometryType::eBOX:
			{
				// for BucketPruner:	compute OBB
				// for AABBPruner:		will use PxBoxGeometry copy
				const PxBoxGeometry& shape = mGeometry.get<PxBoxGeometry>();
				// grow the OBB by 1% for overlap/sweep/GJK accuracy
				PxVec3 he = shape.halfExtents*1.01f;
				Gu::Box& box = reinterpret_cast<Gu::Box&>(mBox);
				buildFrom(box,mWorldTransform.p,he,mWorldTransform.q);
			}
			break;
		case PxGeometryType::eCAPSULE:
			{
				// for BucketPruner:	compute OBB
				// for AABBPruner:		will use PxCapsuleGeometry copy
				Gu::Box& capsuleBox = reinterpret_cast<Gu::Box&>(mBox);
				Gu::computeBoxAroundCapsule(mGeometry.get<PxCapsuleGeometry>(),mWorldTransform,capsuleBox);
				// grow the OBB by 1% for overlap/sweep/GJK accuracy
				// there is no test for this one, but can theoretically happen
				capsuleBox.extents*=1.01f;
			}
			break;
		case PxGeometryType::eCONVEXMESH:
			{
				// for BucketPruner:	compute OBB
				// for AABBPruner:		replace the PxConvexMeshGeometry with PxBoxGeometry
				const PxConvexMeshGeometry& convexGeom = mGeometry.get<PxConvexMeshGeometryLL>();
				GU_FETCH_CONVEX_DATA(convexGeom); // needs to be done for SPU before we compute OBB around it
				Gu::Box& box = reinterpret_cast<Gu::Box&>(mBox);
				computeOBBAroundConvex(box, convexGeom, cm, mWorldTransform);
				// grow the OBB by 1% for overlap/sweep/GJK accuracy
				box.extents*=1.01f;
				mGeometry.set(PxBoxGeometry(box.extents));
				mWorldTransform = box.getTransform();
			}
			break;
		case PxGeometryType::eSPHERE:
			{
				// for BucketPruner:	compute Gu::Sphere
				// for AABBPruner:		will use Gu::Sphere as well
				const PxSphereGeometry& shape = mGeometry.get<PxSphereGeometry>();
				Gu::Sphere& sphere = reinterpret_cast<Gu::Sphere&>(mSphere);
				sphere = Gu::Sphere(mWorldTransform.p,shape.radius);
				// no need to grow the Sphere
			}
			break;
		default:
			PX_ASSERT(0);
		}
	}

	ShapeData(const PxBounds3& aabb): mWorldTransform(aabb.getCenter()), mInflatedAABB(aabb)
	{
		mGeometry.set(PxBoxGeometry(aabb.getExtents()));
	}
	
	PX_FORCE_INLINE const PxGeometry&	getGeometry()			const { return mGeometry.get(); }
	PX_FORCE_INLINE const PxTransform&	getWorldTransform()		const { return mWorldTransform; }
	PX_FORCE_INLINE const PxBounds3&	getInflatedWorldAABB()	const { return mInflatedAABB; }

	PX_FORCE_INLINE const Gu::Box&		getOBB()	const
	{
		PX_ASSERT(	mGeometry.getType() == PxGeometryType::eBOX ||
					mGeometry.getType() == PxGeometryType::eCAPSULE);
		const Gu::Box& box = reinterpret_cast<const Gu::Box&>(mBox);
		return box;
	}
	PX_FORCE_INLINE const Gu::Sphere&	getGuSphere() const
	{
		PX_ASSERT(mGeometry.getType() == PxGeometryType::eSPHERE);
		const Gu::Sphere& sphere = reinterpret_cast<const Gu::Sphere&>(mSphere);
		return sphere;
	}

	PX_FORCE_INLINE const PxBoxGeometry& getBoxGeom() const
	{
		return mGeometry.get<PxBoxGeometry>();
	}

	PX_FORCE_INLINE const PxCapsuleGeometry& getCapsuleGeom() const
	{
		return mGeometry.get<PxCapsuleGeometry>();
	}

private:
		Gu::GeometryUnion	mGeometry;
		PxTransform			mWorldTransform;
		union
		{
			PxU8 mBox[sizeof(Gu::Box)];
			PxU8 mSphere[sizeof(Gu::Sphere)];
		};
		PxBounds3		mInflatedAABB;
};

struct PrunerPayload
{
	size_t data[2];

	PX_FORCE_INLINE	bool operator == (const PrunerPayload& other) const
	{
		return (data[0] == other.data[0]) && (data[1] == other.data[1]);
	}
};

struct PrunerCallback
{
	virtual PxAgain invoke(PxReal& distance, const PrunerPayload* payload, PxU32 nb=1) = 0;
};

class Pruner : public Ps::UserAllocated
{
public:


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	\brief		Adds objects to the pruner.
	 *	\param		results		[out]	an array for resulting handles
	 *  \param		bounds		[in]	an array of bounds
	 *  \param		userData	[in]	an array of object data
	 *  \param		count		[in]	the number of objects in the arrays
	 *
	 *	\return		true if success, false if internal allocation failed. The first failing add results in a INVALID_PRUNERHANDLE.
	 *
	 *  Handles are usable as indices. Each handle is either be a recycled handle returned by the client via removeObjects(),
	 *  or a fresh handle that is either zero, or one greater than the last fresh handle returned.
	 *
	 *	Objects and bounds in the arrays have the same number of elements and ordering. 
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual bool						addObjects(PrunerHandle* results, const PxBounds3* bounds, const PrunerPayload* userData, PxU32 count = 1) = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Removes objects from the pruner.
	 *	\param		handles		[in]	the objects to remove
	 *  \param		count		[in]	the number of objects to remove
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						removeObjects(const PrunerHandle* handles, PxU32 count = 1) = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Updates objects with new bounds.
	 *	\param		handles		[in]	the objects to update
	 *  \param		newBounds	[in]	updated bounds 
	 *  \param		count		[in]	the number of objects to update
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						updateObjects(const PrunerHandle* handles, const PxBounds3* newBounds, PxU32 count = 1) = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Makes the queries consistent with previous changes.
	 *	This function must be called before starting queries on an updated Pruner and assert otherwise.
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						commit() = 0;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Query functions
	 *  
	 *	Note: return value may disappear if PrunerCallback contains the necessary information
	 *			currently it is still used for the dynamic pruner internally (to decide if added objects must be queried)
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual	PxAgain raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const = 0;
	virtual	PxAgain	overlap(const ShapeData& queryVolume, PrunerCallback&) const = 0;
	virtual	PxAgain	sweep(const ShapeData& queryVolume, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback&) const = 0;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Retrieve the object data associated with the handle
	 *	
	 *	\param	handle		The handle returned by addObjects()
	 *
	 *	\return				A reference to the object data
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual const PrunerPayload&		getPayload(const PrunerHandle&) const = 0;




	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	Preallocate space 
	 *	
	 *	\param	entries		the number of entries to preallocate space for
	 *
	 *	\return				A reference to the object data
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						preallocate(PxU32 entries) = 0;


	// shift the origin of the pruner objects
	virtual void						shiftOrigin(const PxVec3& shift) = 0;

	virtual								~Pruner() {};

	// additional 'internal' interface		
	virtual	void						visualize(Cm::RenderOutput&, PxU32) const {}
};
}

}

#endif // SQ_PRUNER_H
