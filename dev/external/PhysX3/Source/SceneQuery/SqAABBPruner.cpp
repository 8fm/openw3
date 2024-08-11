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


// TODO:
// - does the swapcallback post fix really work? Didn't handles change?
// the new mapping maps the *new pool* to the new tree, why should we fix the pool again?

// - stress test create/update/delete random objects each frame
// - temp hack....
// * check mAdded removal => always ok?
// * fix mAddedSize when removing from mAdded
// * queries vs mAdded
// * skip work when objects didn't move => important if we use this for statics as well
// - optimize full refit for new tree => partial refit
// * optimize "AddUnique"
// - heuristics/etc for progressive rebuild
// * bit arrays & refit optims
// - recycle memory buffers/limit realloc
// - words in mRecorded
// - progressive mapping? ie computed in AABB-tree during build

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "PsIntrinsics.h"
#include "PsUserAllocated.h"
#include "PsBitUtils.h"
#include "SqAABBPruner.h"
#include "OPC_AABBTree.h"
#include "OPC_TreeBuilders.h"
#include "GuSphere.h"
#include "GuBox.h"
#include "GuCapsule.h"
#include "CmMemFetch.h"
#include "GuRawQueryTestsSIMD.h"
#include "OPC_AABBTree.h"
#include "CmMemFetch.h"


using namespace physx;
using namespace Gu;
using namespace Sq;
using namespace Cm;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __SPU__
#undef PX_NEW
#undef PX_DELETE_AND_RESET

#define PX_NEW(a) (a*)((a*)NULL)
#define PX_DELETE_AND_RESET(a)
#endif

AABBPruner::AABBPruner(bool incrementalUpdate) 
#ifndef __SPU__
:	mAABBTree			(NULL)
,	mNewTree			(NULL)
,	mCachedBoxes		(NULL)
,	mNbCachedBoxes		(0)
,	mProgress			(BUILD_NOT_STARTED)
,	mNbCalls			(0)
,	mAddedObjects0		(&mAdded0)
,	mAddedObjects1		(&mAdded1)
,	mBucketPruner		(false)
,	mRebuildRateHint	(100)
,	mAdaptiveRebuildTerm(0)
,	mIncrementalUpdate  (incrementalUpdate)
,	mDirty				(false)
,	mNeedsNewTree		(false)
,	mRecords			(PX_DEBUG_EXP("AABBPrunerMapper::mRecords"))
,	mIsBuilding			(false)
#endif
{
}


#ifndef __SPU__
AABBPruner::~AABBPruner()
{
	release();
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Add, Remove, Update methods
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool AABBPruner::addObjects(PrunerHandle* results, const PxBounds3* bounds, const PrunerPayload* payload, PxU32 count)
{
	mDirty = true;

	if(!mIncrementalUpdate)
		PX_DELETE_AND_RESET(mAABBTree);

	PxU32 valid = 0;

	for(PxU32 i=0;i<count;i++)
	{
		PrunerHandle h = mPool.addObject(bounds[i], payload[i]);
		results[i] = h;
		if(h == INVALID_PRUNERHANDLE)
			break;

		valid++;
	}

	if(mAABBTree) // no need to check for static pruner, since it just deleted the tree above
	{
		mNeedsNewTree = true;

		PxU32 i;
		for(i=0;i<valid;i++)
		{
			PrunerHandle h = results[i];
			mAddedObjects0->insert(h);
			mBucketPruner.addObject(payload[i], bounds[i]);
		}
	}

	return valid==count;
}

void AABBPruner::updateObjects(const PrunerHandle* handles, const PxBounds3* newBounds, PxU32 count)
{
	mDirty = true;

	if(!mIncrementalUpdate)
		PX_DELETE_AND_RESET(mAABBTree);

	for(PxU32 i=0; i<count; i++)
		mPool.updateObject(handles[i],newBounds[i]);

	if(mAABBTree) // no need to check for static pruner, since it just deleted the tree above
	{
		mNeedsNewTree = true;
		for(PxU32 i=0; i<count; i++)
		{
			PxU32 poolIndex = mPool.getIndex(handles[i]);
			PxU32 treeNodeIndex = mTreeMap[poolIndex];
			if(treeNodeIndex!=INVALID_PRUNERHANDLE)
				mAABBTree->MarkForRefit(treeNodeIndex);
			else
			{
				mBucketPruner.updateObject(newBounds[i], mPool.getPayload(handles[i]));
			}
		}
	}
}

void AABBPruner::removeObjects(const PrunerHandle* handles, PxU32 count)
{
	mDirty = true;
	if(!mIncrementalUpdate)
		PX_DELETE_AND_RESET(mAABBTree);

	for(PxU32 i=0; i<count; i++)
	{
		PrunerHandle h = handles[i];
		const PrunerPayload removedPayload = mPool.getPayload(h);
		PxU32 index = mPool.getIndex(h);
		PxU32 prevLastIndex = mPool.removeObject(h);

		// no need to check for static pruner, since it just deleted the tree above
		if(mAABBTree)
		{
			mNeedsNewTree = true;
			
			PxU32 treeIndex = mTreeMap[index];
			if(treeIndex != INVALID_PRUNERHANDLE)
				mAABBTree->MarkForRefit(treeIndex);

			if(mAdded0.erase(h) || mAdded1.erase(h))
				mBucketPruner.removeObject(removedPayload);

			mTreeMap.invalidate(index,prevLastIndex,*mAABBTree);
			if(mIsBuilding)
				mRecords.pushBack(Record(index,prevLastIndex));
		}
	}

	if(mPool.getNbActiveObjects()==0)
		release();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Query Implementation
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __SPU__
	#define NODE_PARAMS	mAABBTreeNodes, mNodesEA, mIndicesEA
#else
	#define NODE_PARAMS	mAABBTree->GetNodes(), mAABBTree->GetNodes(), mAABBTree->GetIndices()
#endif

// this function performs translation from PxU32 object stored in AABBTree to PrunerPayload* that is expected to be returned
// by PrunerInterface
static PX_FORCE_INLINE PrunerPayload* translatePxU32ToPrunerPayload(PxU32 storedInAABBtree, PrunerPayload* objectsBase)
{
	if (PX_IS_SPU && storedInAABBtree == PxU32(-1)) // AP: can happen if the object is deleted, why only on SPU though?
		return NULL;

	MemFetchSmallBuffer buf;
	// this seems unnecessary (and very costly at this point).. why double indirection here?
	// we fetch another PPU address here
	//pxPrintf("SQRawQuerySweepVisitor, objectIndex = %d\n", objectIndex);
	PrunerPayload* p = memFetchAsync<PrunerPayload>(MemFetchPtr(objectsBase+storedInAABBtree), 4, buf);
	memFetchWait(4);

	return p;
};

// AP: moved from GuRawQueryTreeTraversal.h so that traversal is now a part of StaticPruner module
// and can perform translation from PxU32 to PrunerPayload inline.
// This way we can eliminate a level of callstack without sacrificing dependencies and move one step closer
// to merging StaticPruner with AABBTree
#define RAW_TRAVERSAL_STACK_SIZE 256

template<typename Test>
class AABBTreeOverlap
{
public:
	bool operator()(PrunerPayload* objects, const Gu::AABBTree& tree, const Test& test, PrunerCallback &visitor)
	{
		using namespace Cm;

		const Gu::AABBTreeNode* stack[RAW_TRAVERSAL_STACK_SIZE];
		const Gu::AABBTreeNode* nodeBase = tree.GetNodes();
		stack[0] = nodeBase;
		PxU32 stackIndex = 1;

		MemFetchBufferU<Gu::AABBTreeNode> nodeBuf;
		MemFetchBufferU<Gu::AABBTreeNode, 2> childBuf;
		MemFetchSmallBuffer buf;
		PxU32* prunableIndex;

		while(stackIndex>0)
		{
			const Gu::AABBTreeNode* node = memFetchAsync<Gu::AABBTreeNode>(MemFetchPtr(stack[--stackIndex]), 3, nodeBuf);	
			memFetchWait(3);
			Vec3V center, extents;
			node->GetAABBCenterExtentsV(&center, &extents);
			while(test(center, extents))
			{
				if(node->IsLeaf())
				{
					prunableIndex = memFetchAsync<PxU32>(MemFetchPtr(node->GetPrimitives(tree.GetIndices())), 4, buf);
					memFetchWait(4);
					PxReal unusedDistance;
					if (!visitor.invoke(unusedDistance, translatePxU32ToPrunerPayload(*prunableIndex, objects), 1))
						return false;
					break;
				}

				const Gu::AABBTreeNode *childrenEA = node->GetPos(nodeBase);
				const Gu::AABBTreeNode *children = memFetchAsync<Gu::AABBTreeNode,2>(MemFetchPtr(childrenEA), 3, childBuf);
				memFetchWait(3);

				node = children;
				stack[stackIndex++] = childrenEA+1;
				PX_ASSERT(stackIndex < RAW_TRAVERSAL_STACK_SIZE);
				node->GetAABBCenterExtentsV(&center, &extents);
			}
		}
		return true;
	}
};

template <bool tInflate> // use inflate=true for sweeps, inflate=false for raycasts
class AABBTreeRaycast
{
public:
	bool operator()(
		PrunerPayload* objects, const Gu::AABBTree& tree,
		const PxVec3& origin, const PxVec3& unitDir, PxReal &maxDist, const PxVec3& inflation,
		PrunerCallback& pcb)
	{
		using namespace Cm;

		Gu::RayAABBTest test(origin, unitDir, maxDist, inflation);

		const Gu::AABBTreeNode* stack[RAW_TRAVERSAL_STACK_SIZE]; // stack always contains PPU addresses
		const Gu::AABBTreeNode* nodeBase = tree.GetNodes();
		stack[0] = nodeBase;
		PxU32 stackIndex = 1;

		MemFetchBufferU<Gu::AABBTreeNode> nodeBuf;
		MemFetchBufferU<Gu::AABBTreeNode, 2> childBuf;
		MemFetchSmallBuffer buf;
		PxU32* prunableIndex;

		PxReal oldMaxDist;
		while(stackIndex--)
		{
			const Gu::AABBTreeNode* node = memFetchAsync<Gu::AABBTreeNode>(MemFetchPtr(stack[stackIndex]), 3, nodeBuf);	
			memFetchWait(3);
			Vec3V center, extents;
			node->GetAABBCenterExtentsV(&center, &extents);
			if(test.check<tInflate>(center, extents))	// TODO: try timestamp ray shortening to skip this
			{
				PxReal md = maxDist; // has to be before the goto below to avoid compile error
				while(!node->IsLeaf())
				{
					const Gu::AABBTreeNode *childrenEA = node->GetPos(nodeBase);
					const Gu::AABBTreeNode *children = memFetchAsync<Gu::AABBTreeNode,2>(MemFetchPtr(childrenEA), 3, childBuf);
					memFetchWait(3);
					Vec3V c0, c1, e0, e1;
					children[0].GetAABBCenterExtentsV(&c0, &e0);
					const PxU32 b0 = test.check<tInflate>(c0, e0);

					children[1].GetAABBCenterExtentsV(&c1, &e1);
					const PxU32 b1 = test.check<tInflate>(c1, e1);

					if(b0 && b1)	// if both intersect, push the one with the further center on the stack for later
					{
						// & 1 because FAllGrtr behavior differs across platforms
						PxU32 bit = FAllGrtr(V3Dot(V3Sub(c1, c0), test.mDir), FZero()) & 1;
						stack[stackIndex++] = childrenEA + bit;
						node = children + (1 - bit);
						PX_ASSERT(stackIndex < RAW_TRAVERSAL_STACK_SIZE);
					}
					else if (b0)
						node = children;
					else if (b1)
						node = children+1;
					else
						goto skip_leaf_code;
				}

				oldMaxDist = maxDist; // we copy since maxDist can be updated in the callback and md<maxDist test below can fail
				prunableIndex = memFetchAsync<PxU32>(MemFetchPtr(node->GetPrimitives(tree.GetIndices())), 4, buf);
				memFetchWait(4);
				if (!pcb.invoke(md, translatePxU32ToPrunerPayload(*prunableIndex, objects), 1))
					return false;

				if(md < oldMaxDist)
				{
					maxDist = md;
					test.setDistance(md);
				}
skip_leaf_code:;
			}
		}
		return true;
	}
};



PxAgain AABBPruner::overlap(const ShapeData& queryVolume, PrunerCallback& pcb) const
{
	PX_ASSERT(!mDirty);

	PxAgain again = true;
	
	if(mAABBTree)
	{
		switch(queryVolume.getGeometry().getType())
		{
		case PxGeometryType::eBOX:
			{
				if (PxAbs(queryVolume.getWorldTransform().q.w) < 0.999999f)
				{	
					Gu::OBBAABBTest test(queryVolume.getWorldTransform(), queryVolume.getBoxGeom());
					again = AABBTreeOverlap<Gu::OBBAABBTest>()(mPool.getObjects(), *mAABBTree, test, pcb);
				}
				else
				{
					Gu::AABBAABBTest test(queryVolume.getInflatedWorldAABB());
					again = AABBTreeOverlap<Gu::AABBAABBTest>()(mPool.getObjects(), *mAABBTree, test, pcb);
				}
			}
			break;
		case PxGeometryType::eCAPSULE:
			{
				Gu::CapsuleAABBTest test(queryVolume.getWorldTransform(), queryVolume.getCapsuleGeom());
				again = AABBTreeOverlap<Gu::CapsuleAABBTest>()(mPool.getObjects(), *mAABBTree, test, pcb);
			}
			break;
		case PxGeometryType::eSPHERE:
			{
				const Gu::Sphere& sphere = queryVolume.getGuSphere();
				Gu::SphereAABBTest test(sphere.center, sphere.radius);
				again = AABBTreeOverlap<Gu::SphereAABBTest>()(mPool.getObjects(), *mAABBTree, test, pcb);
			}
			break;
		case PxGeometryType::eCONVEXMESH:
			{
				Gu::OBBAABBTest test(queryVolume.getWorldTransform(), queryVolume.getBoxGeom());
				again = AABBTreeOverlap<Gu::OBBAABBTest>()(mPool.getObjects(), *mAABBTree, test, pcb);			
			}
			break;
		default:
			PX_ASSERT(0 && "unsupported overlap query volume geometry type");
		}
	}

	if(again && mIncrementalUpdate && (mAdded0.size() || mAdded1.size())/* && mBucketPruner.mCoreNbObjects*/)
		again = mBucketPruner.overlap(queryVolume, pcb);

	return again;
}

PxAgain AABBPruner::sweep(const ShapeData& queryVolume, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback& pcb) const
{
	PX_ASSERT(!mDirty);

	PxAgain again = true;

	if(mAABBTree)
	{
		const PxBounds3& aabb = queryVolume.getInflatedWorldAABB();
		PxVec3 extents = aabb.getExtents();
		again = AABBTreeRaycast<true>()(mPool.getObjects(), *mAABBTree, aabb.getCenter(), unitDir, inOutDistance, extents, pcb);
	}

	if(again && mIncrementalUpdate && (mAdded0.size() || mAdded1.size()))
		again = mBucketPruner.sweep(queryVolume, unitDir, inOutDistance, pcb);

	return again;
}

PxAgain AABBPruner::raycast(const PxVec3& origin, const PxVec3& unitDir, PxReal& inOutDistance, PrunerCallback& pcb) const
{
	PX_ASSERT(!mDirty);

	PxAgain again = true;

	if(mAABBTree)
		again = AABBTreeRaycast<false>()(mPool.getObjects(), *mAABBTree, origin, unitDir, inOutDistance, PxVec3(0.0f), pcb);
		
	if(again && mIncrementalUpdate && (mAdded0.size() || mAdded1.size()))
		again = mBucketPruner.raycast(origin, unitDir, inOutDistance, pcb);

	return again;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Other methods of Pruner Interface
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AABBPruner::purge() 
{
	release();
	mDirty=true;
} 

void AABBPruner::setRebuildRateHint(PxU32 nbStepsForRebuild) 
{ 
	if( nbStepsForRebuild <= 3 )
	{
		nbStepsForRebuild = 0;
	}
	else
	{
		mRebuildRateHint = (nbStepsForRebuild-3); 
	}
	mAdaptiveRebuildTerm = 0; 
}

void AABBPruner::commit()
{
	if(!mDirty)
		return;

	mDirty = false;

	if(!mAABBTree)
		rebuildAABBTree();

	if(!mIncrementalUpdate)
		return;

	// Note: it is not safe to call AABBPruner::build() here
	// because the first thread will perform one step of the incremental update,
	// continue raycasting, while the second thread performs the next step in
	// the incremental update

	// Calling Refit() below is safe. It will call 
	// StaticPruner::build() when necessary. Both will early
	// exit if the tree is already up to date, if it is not already, then we 
	// must be the first thread performing raycasts on a dirty tree and other 
	// scene query threads will be locked out by the write lock in 
	// SceneQueryManager::flushUpdates()


	if(mProgress!=BUILD_FINISHED)	
	{
		refit();
	}
	else
	{
		// Finalize
		PX_FREE_AND_RESET(mCachedBoxes);
		mProgress = BUILD_NOT_STARTED;

		// Adjust adaptive term to get closer to specified rebuild rate.
		if (mNbCalls > mRebuildRateHint)
			mAdaptiveRebuildTerm++;
		else if (mNbCalls < mRebuildRateHint)
			mAdaptiveRebuildTerm--;

		// Switch trees
		PX_DELETE(mAABBTree);
		#ifdef PX_DEBUG
		mNewTree->Validate();
		#endif
		mAABBTree = mNewTree;
		mNewTree = NULL;

		// Switch mapping
		mTreeMap.initMap(PxMax(mPool.getNbActiveObjects(),mNbCachedBoxes),*mAABBTree);
		// The new mapping has been computed using only indices stored in the new tree. Those indices map the pruning pool
		// we had when starting to build the tree. We need to re-apply recorded moves to fix the tree.
		for(Record* r = mRecords.begin(); r < mRecords.end(); r++)
		{
			mTreeMap.invalidate(r->indexA,r->indexB,*mAABBTree);
		}
		mRecords.clear();
		mIsBuilding = false;


		//### Full refit, to be replaced
		// We need to refit the new tree because objects may have moved while we were building it.
		PxU32 NbObjects = mPool.getNbActiveObjects();

		AABBTreeBuilder TB;
		TB.mNbPrimitives	= NbObjects;
		TB.mAABBArray		= mPool.getCurrentWorldBoxes();
//		TB.mRules			= SPLIT_COMPLETE|SPLIT_SPLATTER_POINTS;
		TB.mSettings.mRules	= SPLIT_SPLATTER_POINTS;
		TB.mSettings.mLimit	= 1;
		((AABBTree*)getAABBTree())->Refit2(&TB, (PxU32*)getAABBTree()->GetIndices());

		for(PrunerHandleSet::Iterator it1 = mAddedObjects1->getIterator(); !it1.done(); ++it1)
		{
			PrunerHandle p = *it1;
			mBucketPruner.removeObject(mPool.getPayload(p));
		}

		{
			mNeedsNewTree = mAddedObjects0->size()>0;
			mAddedObjects1->clear();
		}
	}

	updateBucketPruner();
}


void AABBPruner::shiftOrigin(const PxVec3& shift)
{
	mPool.shiftOrigin(shift);

	if(mAABBTree)
		mAABBTree->ShiftOrigin(shift);

	if(mIncrementalUpdate)
		mBucketPruner.shiftOrigin(shift);

	//
	// currently there seems no need to shift the build-in-progress tree because when we switch to that tree, it gets
	// refit anyway. And that tree is built on the basis of cached bounds which do not change either.
	// see mCachedBoxes, mNewTree
	//
}

#ifndef __SPU__
#include "CmRenderOutput.h"
void AABBPruner::visualize(Cm::RenderOutput& out, PxU32 color) const
{
	const AABBTree* tree = mAABBTree; // getAABBTree() asserts when pruner is dirty. NpScene::visualization() does not enforce flushUpdate. see DE7834

	if(tree)
	{
		struct Local
		{
			static void _Draw(const AABBTreeNode* root, const AABBTreeNode* node, Cm::RenderOutput& out)
			{
				out << Cm::DebugBox(PxBounds3::centerExtents(node->GetAABBCenter(), node->GetAABBExtents()), true);
				if (node->IsLeaf())
					return;
				_Draw(root, node->GetPos(root), out);
				_Draw(root, node->GetNeg(root), out);
			}
		};
		out << PxTransform(PxIdentity);
		out << color;
		Local::_Draw(tree->GetNodes(), tree->GetNodes(), out);
	}

	// Render added objects not yet in the tree
	out << PxTransform(PxIdentity);
	out << PxU32(PxDebugColor::eARGB_WHITE);

	for(PrunerHandleSet::Iterator it0 = const_cast<AABBPruner*>(this)->mAdded0.getIterator(); !it0.done(); ++it0)
	{
		PrunerHandle p = *it0;
		out << Cm::DebugBox(mPool.getWorldAABB(p), true);
	}
	
	for(PrunerHandleSet::Iterator it1 = const_cast<AABBPruner*>(this)->mAdded1.getIterator(); !it1.done(); ++it1)
	{
		PrunerHandle p = *it1;
		out << Cm::DebugBox(mPool.getWorldAABB(p), true);		
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Internal methods
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool AABBPruner::buildStep()
{
	PX_ASSERT(mIncrementalUpdate);
	if(mNeedsNewTree)
	{
		if(mProgress==BUILD_NOT_STARTED)
		{
			const PxU32 nbObjects = mPool.getNbActiveObjects();
			if(!nbObjects)
				return true;

			PX_DELETE(mNewTree);
			mNewTree = PX_NEW(AABBTree);

			mNbCachedBoxes = nbObjects;
			mCachedBoxes = (PxBounds3*)PX_ALLOC(sizeof(PxBounds3)*nbObjects, PX_DEBUG_EXP("PxBound3"));

			PxBounds3* aabbs = mPool.getCurrentWorldBoxes();
			for(PxU32 i=0;i<nbObjects;i++)
			{
				mCachedBoxes[i] = aabbs[i];
			}

			// Objects currently in mAdded will be part of the new tree. However more objects can
			// get added while we compute the new tree, and those ones will not be part of it.
			Ps::swap(mAddedObjects0, mAddedObjects1);
			mBuilder.Reset();
			mBuilder.mNbPrimitives		= mNbCachedBoxes;
			mBuilder.mAABBArray			= mCachedBoxes;
			mBuilder.mSettings.mRules	= SPLIT_SPLATTER_POINTS;
			mBuilder.mSettings.mLimit	= 1;

			// start recording moves during rebuild to reapply (fix the new tree) eventually
			mIsBuilding = true;
			PX_ASSERT(mRecords.size()==0);

			mProgress = BUILD_INIT;
		}
		else if(mProgress==BUILD_INIT)
		{
			mNewTree->ProgressiveBuild(&mBuilder, 0, 0);
			mProgress = BUILD_IN_PROGRESS;
			mNbCalls = 0;

			// Use a heuristic to estimate the number of work units needed for rebuilding the tree.
			// The general idea is to use the number of work units of the previous tree to build the new tree.
			// This works fine as long as the number of leaves remains more or less the same for the old and the
			// new tree. If that is not the case, this estimate can be way off and the work units per step will
			// be either much too small or too large. Hence, in that case we will try to estimate the number of work
			// units based on the number of leaves of the new tree as follows:
 			//
			// - Assume new tree with n leaves is perfectly-balanced
			// - Compute the depth of perfectly-balanced tree with n leaves
			// - Estimate number of working units for the new tree

			PxU32 depth = Ps::ilog2(mBuilder.mNbPrimitives);	// Note: This is the depth without counting the leaf layer
			PxU32 estimatedNbWorkUnits = depth * mBuilder.mNbPrimitives;	// Estimated number of work units for new tree
			PxU32 estimatedNbWorkUnitsOld = mAABBTree->GetTotalPrims();
			if ((estimatedNbWorkUnits <= (estimatedNbWorkUnitsOld << 1)) && (estimatedNbWorkUnits >= (estimatedNbWorkUnitsOld >> 1)))
			{	// The two estimates do not differ by more than a factor 2

				mTotalWorkUnits = estimatedNbWorkUnitsOld;
			}
 			else
			{
 				mAdaptiveRebuildTerm = 0;
				mTotalWorkUnits = estimatedNbWorkUnits;
 			}
 
 			PxI32 totalWorkUnits = mTotalWorkUnits + (mAdaptiveRebuildTerm * mBuilder.mNbPrimitives);
 			mTotalWorkUnits = PxMax(totalWorkUnits, 0);
		}
		else if(mProgress==BUILD_IN_PROGRESS)
		{
			mNbCalls++;
			const PxU32 Limit = 1 + (mTotalWorkUnits / mRebuildRateHint);
			if(!mNewTree->ProgressiveBuild(&mBuilder, 1, Limit))
			{
				// Done
				mProgress = BUILD_FINISHED;
				#ifdef PX_DEBUG
				mNewTree->Validate();
				#endif
			}
		}
		mDirty = true;
		return mProgress==BUILD_FINISHED;
	}

	return true;
}



// ---------------------- internal methods


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Builds an AABB-tree for objects in the pruning pool.
 *	\return		true if success
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBPruner::rebuildAABBTree()
{
#ifndef __SPU__
	// Release possibly already existing tree
	PX_DELETE_AND_RESET(mAABBTree);

	// Don't bother building an AABB-tree if there isn't a single static object
	const PxU32 nbObjects = mPool.getNbActiveObjects();
	if(!nbObjects)	return true;

	bool Status;
	{
		// Create a new tree
		mAABBTree = PX_NEW(AABBTree);

		AABBTreeBuilder TB;
		TB.mNbPrimitives	= nbObjects;
		TB.mAABBArray		= mPool.getCurrentWorldBoxes();
		TB.mSettings.mRules	= SPLIT_SPLATTER_POINTS;
		TB.mSettings.mLimit	= 1;
//		TB.mSettings.mLimit	= 4;
//		TB.mSettings.mLimit	= 8;
		Status = mAABBTree->Build(&TB);
	}

	if(mIncrementalUpdate)
		mTreeMap.initMap(PxMax(nbObjects,mNbCachedBoxes),*mAABBTree);
	

	return Status;
#else
	return true;
#endif
}

void AABBPruner::updateBucketPruner()
{
	PX_ASSERT(mIncrementalUpdate);
	mBucketPruner.build();
}

#ifndef __SPU__
void AABBPruner::release()
{
	mBucketPruner.release(true);

	mAdded0.clear();
	mAdded1.clear();
	mAddedObjects0 = &mAdded0;
	mAddedObjects1 = &mAdded1;

	mTreeMap.release();

	PX_FREE_AND_RESET(mCachedBoxes);
	mBuilder.Reset();
	PX_DELETE_AND_RESET(mNewTree);
	PX_DELETE_AND_RESET(mAABBTree);

	mNbCachedBoxes = 0;
	mProgress = BUILD_NOT_STARTED;
	mRecords.clear();
	mIsBuilding = false;
}

// Refit current tree
void AABBPruner::refit()
{
	PX_ASSERT(mIncrementalUpdate);
	AABBTree* Tree = getAABBTree();
	if(!Tree)
		return;

#ifdef PX_DEBUG
	Tree->Validate();
#endif

	//### missing a way to skip work if not needed

	PxU32 nbObjects = mPool.getNbActiveObjects();
	if(!nbObjects)
		return;

//	printf("Refit modified\n");
	Tree->RefitMarked(nbObjects, mPool.getCurrentWorldBoxes(), Tree->GetIndices());
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


