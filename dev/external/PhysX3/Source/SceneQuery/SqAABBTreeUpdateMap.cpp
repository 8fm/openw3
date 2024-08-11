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

#include "SqAABBTreeUpdateMap.h"
#include "OPC_AABBTree.h"

using namespace physx;
using namespace Gu;
using namespace Sq;

#ifdef __SPU__
#error 
#endif

static const PxU32 SHRINK_THRESHOLD = 1024;

void AABBTreeUpdateMap::initMap(PxU32 nbObjects, const Gu::AABBTree& tree)
{
	if(!nbObjects)	
	{
		release();
		return;
	}

	PxU32 mapSize = nbObjects;
	PxU32 currentCapacity = mMapping.capacity();
	PxU32 targetCapacity = mapSize + (mapSize>>2);

	if( ( targetCapacity < (currentCapacity>>1) ) && ( (currentCapacity-targetCapacity) > SHRINK_THRESHOLD ) )
	{
		// trigger reallocation of a smaller array, there is enough memory to save
		currentCapacity = 0;
	}

	if(mapSize > currentCapacity)
	{
		// the mapping values are invalid and reset below in any case
		// so there is no need to copy the values at all
		mMapping.reset();
		mMapping.reserve(targetCapacity);	// since size is 0, reserve will also just allocate
	}

	mMapping.forceSize_Unsafe(mapSize);

	for(PxU32 i=0;i<mapSize;i++)
		mMapping[i] = INVALID_NODE_ID;

	PxU32 nbNodes = tree.GetNbNodes();
	const AABBTreeNode*	nodes = tree.GetNodes();
	for(PxU32 i=0;i<nbNodes;i++)
	{
		if(nodes[i].IsLeaf() && nodes[i].GetPrimitives(tree.GetIndices()))
		{
			PX_ASSERT(nodes[i].GetNbRuntimePrimitives()==1);
			PxU32 index = nodes[i].GetPrimitives(tree.GetIndices())[0];
			PX_ASSERT(index<nbObjects);		// temp hack
			mMapping[index] = i;
		}
	}
}

bool AABBTreeUpdateMap::checkMap(PxU32 nbObjects, const Gu::AABBTree& tree) const
{
	if(!mMapping.size()) return false;

	const AABBTreeNode* nodes = tree.GetNodes();
	for(PxU32 i=0;i<nbObjects;i++)
	{
		PxU32 index = mMapping[i];
		if(index == INVALID_NODE_ID) continue;
		if(i != nodes[index].GetPrimitives(tree.GetIndices())[0])
			return false;
	}
	return true;
}

void AABBTreeUpdateMap::invalidate(PxU32 prunerIndex0, PxU32 prunerIndex1, Gu::AABBTree& tree)
{
	PxU32 nodeIndex0 = prunerIndex0<mMapping.size() ? mMapping[prunerIndex0] : INVALID_NODE_ID;
	PxU32 nodeIndex1 = prunerIndex1<mMapping.size() ? mMapping[prunerIndex1] : INVALID_NODE_ID;

	//printf("map invalidate pi0:%x ni0:%x\t",prunerIndex0,nodeIndex0);
	//printf("  replace with pi1:%x ni1:%x\n",prunerIndex1,nodeIndex1);

	// invalidate node 0 
	// point node 1 to pool 0
	// map pool 0 to node 1
	// invalidate map pool 1

	AABBTreeNode* nodes = tree.GetNodes();

	if(nodeIndex0!=INVALID_NODE_ID)
	{
		PX_ASSERT(nodeIndex0<tree.GetNbNodes());
		PX_ASSERT(nodes[nodeIndex0].IsLeaf());
		PX_ASSERT(nodes[nodeIndex0].GetNbRuntimePrimitives()==1);
		PxU32* primitives = nodes[nodeIndex0].GetPrimitives(tree.GetIndices());
		PX_ASSERT(primitives);
		PX_ASSERT(prunerIndex0 == primitives[0]);
		nodes[nodeIndex0].setNbRunTimePrimitives(0);

		primitives[0] = INVALID_POOL_ID;			// Mark primitive index as invalid in the node
		mMapping[prunerIndex0] = INVALID_NODE_ID;	// invalidate the node index for pool 0

		if((nodeIndex1!=INVALID_NODE_ID) && (nodes[nodeIndex1].GetPrimitives(tree.GetIndices())[0] == INVALID_POOL_ID) && (nodeIndex0!=nodeIndex1))
			PX_ASSERT(0); 
		// node 1 can only be invalid if it was invalidated just above because of nodeIndex1==nodeIndex0
		// and if there exists a pool object pointed at by node 1, we assert that it is the same pool ID below

		if ((nodeIndex1!=INVALID_NODE_ID) && (nodes[nodeIndex1].GetPrimitives(tree.GetIndices())[0] != INVALID_POOL_ID))
		{
			PxU32* prims = nodes[nodeIndex1].GetPrimitives(tree.GetIndices());
			PX_ASSERT(nodes[nodeIndex1].IsLeaf());
			PX_ASSERT(prims[0] == prunerIndex1);
			prims[0] = prunerIndex0;					// point node 1 to the pool object moved to ID 0
			mMapping[prunerIndex0] = nodeIndex1;		// pool 0 is pointed at by node 1 now
			mMapping[prunerIndex1] = INVALID_NODE_ID;	// pool 1 is no longer stored in the tree
		}
	}
	else
	{
		if(nodeIndex1!=INVALID_NODE_ID)
		{
			PxU32* prims = nodes[nodeIndex1].GetPrimitives(tree.GetIndices());
			PX_ASSERT(nodes[nodeIndex1].IsLeaf());
			PX_ASSERT(prims[0] == prunerIndex1);
			prims[0] = prunerIndex0;					// point node 1 to the pool object moved to ID 0
			mMapping[prunerIndex0] = nodeIndex1;		// pool 0 is pointed at by node 1 now
			mMapping[prunerIndex1] = INVALID_NODE_ID;	// pool 1 is no longer stored in the tree
		}
	}
	//PX_ASSERT(checkMap(mNbMappingEntries,tree));
}

