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

#ifndef SQ_PRUNERTREEMAP_H
#define SQ_PRUNERTREEMAP_H

#include "SqPruner.h"

namespace physx
{
namespace Gu { class AABBTree; }
namespace Sq
{
	static const PxU32 INVALID_NODE_ID = 0xFFffFFff;
	static const PxU32 INVALID_POOL_ID = 0xFFffFFff;

	// Maps pruning pool indices to AABB-tree indices (i.e. locates the object's box in the aabb-tree nodes pool)
	// 
	// The map spans pool indices from 0..N-1, where N is the number of pool entries when the map was created from a tree.
	//
	// It maps: 
	//		to node indices in the range 0..M-1, where M is the number of nodes in the tree the map was created from,
	//   or to INVALID_NODE_ID if the pool entry was removed or pool index is outside input domain.
	//
	// The map is the inverse of the tree mapping: (node[map[poolID]].primitive == poolID) is true at all times.

	class AABBTreeUpdateMap 
	{
	public:

		AABBTreeUpdateMap() {}

		void release()
		{
			mMapping.reset();
		}

		void initMap(PxU32 numPoolObjects, const Gu::AABBTree& tree);

		void invalidate(PxU32 poolIndex, PxU32 replacementPoolIndex, Gu::AABBTree& tree);

		PxU32 operator[](PxU32 i) const
		{ 
			return i < mMapping.size() ? mMapping[i] : INVALID_NODE_ID;
		}

	private:
		bool checkMap(PxU32 numPoolObjects, const Gu::AABBTree& tree) const;

		Ps::Array<PxU32>	mMapping;
	};

}
}

#endif