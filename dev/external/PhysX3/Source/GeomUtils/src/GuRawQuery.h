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

#ifndef GU_RAWQUERY_H
#define GU_RAWQUERY_H
/** \addtogroup physics 
@{ */

#include "PxRawQuery.h"
#include "PsArray.h"
#include "PsPool.h"
#include "GuInternalTriangleMesh.h"

namespace physx
{

class PxGeometry;
class PxTransform;

namespace Gu
{
class AABBTree;
}

namespace Gu
{
struct RawQueryData
{
public:
	PxRawQueryItem&	item;
	PxU32		index;
	bool		inTree;		// TODO: store this in the bit word of the index

	RawQueryData(PxRawQueryItem& _item): item(_item), inTree(false) {}
};

class RawQuery : public PxRawQuery, public Ps::UserAllocated
{
public:
										RawQuery();
										~RawQuery();
	
					PxRawQueryItemID	insert(PxRawQueryItem& item, const PxBounds3 &bounds);
					void				update(PxRawQueryItemID itemID, const PxBounds3& bounds);
					void				remove(PxRawQueryItemID itemID);
					PxBounds3			getBounds(PxRawQueryItemID itemID) const;

					bool				overlap(const PxGeometry& geometry, const PxTransform& pose, PxRawQueryOverlapVisitor& visitor) const;
					bool				raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal maxDist, PxRawQueryRaycastVisitor& visitor) const;
					bool				sweep(const PxGeometry&geometry, const PxTransform& pose, const PxVec3& unitDir, const PxReal maxDist, PxRawQuerySweepVisitor& visitor) const;

					void				rebuild();
					void				release();
														
private:
					Ps::Array<PxBounds3>			mTreeBounds;		// contiguous array of bounds in the tree
					Ps::Array<RawQueryData*>		mTreeToData;		// map from bounds indices to the data objects that are in the tree
					Ps::Array<PxBounds3>			mAddedBounds;		// array of bounds added to the query struct but not yet in the tree
					Ps::Array<RawQueryData*>		mAddedToData;
					Ps::Pool<RawQueryData>			mData;

					void updateTree(PxU32 index, const PxBounds3& newBounds);

					Gu::AABBTree*					mAABBTree;		//!< AABB tree for static objects
};
}
}

/** @} */
#endif


