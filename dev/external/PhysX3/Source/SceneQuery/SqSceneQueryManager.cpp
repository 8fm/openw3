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


#include "SqSceneQueryManager.h"
#include "SqAABBPruner.h"
#include "SqBucketPruner.h"
#include "NpBatchQuery.h"
#include "OPC_AABBTree.h"
#include "PxFiltering.h"
#include "NpRigidDynamic.h"
#include "NpRigidStatic.h"
#include "NpArticulationLink.h"

using namespace physx;
using namespace Sq;


///////////////////////////////////////////////////////////////////////////////

SceneQueryManager::SceneQueryManager(Scb::Scene& scene, const PxSceneDesc& desc) :
	mDirtyList	(PX_DEBUG_EXP("SQmDirtyList")),
	mScene			(scene)
{
	mPrunerType[0]		= desc.staticStructure;
	mPrunerType[1]		= desc.dynamicStructure;

	mTimestamp[0] = 0;
	mTimestamp[1] = 0;

	mPruners[0] = createPruner(desc.staticStructure);
	mPruners[1] = createPruner(desc.dynamicStructure);

	setDynamicTreeRebuildRateHint(desc.dynamicTreeRebuildRateHint);

	preallocate(desc.limits.maxNbStaticShapes, desc.limits.maxNbDynamicShapes);
}

SceneQueryManager::~SceneQueryManager()
{
	for(PxU32 i=0;i<2;i++)
		PX_DELETE_AND_RESET(mPruners[i]);
}


Pruner* SceneQueryManager::createPruner(PxPruningStructure::Enum type)
{
	switch(type)
	{
		case PxPruningStructure::eSTATIC_AABB_TREE:		return PX_NEW(AABBPruner)(false);
		case PxPruningStructure::eNONE:					return PX_NEW(BucketPruner);
		case PxPruningStructure::eDYNAMIC_AABB_TREE:	return PX_NEW(AABBPruner)(true);
		default:										break;
	}
	return NULL;
}

void SceneQueryManager::markForUpdate(Sq::ActorShape* s)
{ 
	PxU32 index = getPrunerIndex(s);
	PrunerHandle handle = getPrunerHandle(s);

	if(!mDirtyMap[index].test(handle))
	{
		mDirtyMap[index].set(handle);
		mDirtyList.pushBack(s);
		mTimestamp[index]++;
	}
}

void SceneQueryManager::preallocate(PxU32 staticShapes, PxU32 dynamicShapes)
{
	if(staticShapes > mDirtyMap[0].size())
		mDirtyMap[0].resize(staticShapes);

	if(dynamicShapes > mDirtyMap[1].size())
		mDirtyMap[1].resize(dynamicShapes);

	if(mPruners[0])
		mPruners[0]->preallocate(staticShapes);
	if(mPruners[1])
		mPruners[1]->preallocate(dynamicShapes);
}

ActorShape* SceneQueryManager::addShape(const NpShape& shape, const PxRigidActor& actor, bool dynamic, PxBounds3* bounds)
{
	PX_ASSERT(mPruners[dynamic?1:0]);

	PrunerPayload pp;
	const Scb::Shape& scbShape = shape.getScbShape();
	const Scb::Actor& scbActor = NpActor::getScbFromPxActor(actor);
	pp.data[0] = (size_t)&scbShape;
	pp.data[1] = (size_t)&scbActor;

	PxBounds3 b;
	if(bounds)							// using ?: generates an extra copy of the return from inflateBounds on 360.
		b = inflateBounds(*bounds); 
	else 
		b = Sq::computeWorldAABB(scbShape, scbActor);

	PxU32 index = dynamic;
	PrunerHandle handle;
	mPruners[index]->addObjects(&handle, &b, &pp);
	mTimestamp[index]++;

	// pruners must either provide indices in order or reuse existing indices, so this 'if' is enough to ensure we have space for the new handle
	if(mDirtyMap[index].size() <= handle)
		mDirtyMap[index].resize(PxMax<PxU32>(mDirtyMap[index].size() * 2, 1024));
	PX_ASSERT(handle<mDirtyMap[index].size());
	mDirtyMap[index].reset(handle);

	return createRef(index, handle);
}

const PrunerPayload& SceneQueryManager::getPayload(const ActorShape* ref) const
{
	PxU32 index = getPrunerIndex(ref);
	PrunerHandle handle = getPrunerHandle(ref);
	return mPruners[index]->getPayload(handle);
}


void SceneQueryManager::removeShape(ActorShape* data)
{
	PxU32 index = getPrunerIndex(data);
	PrunerHandle handle = getPrunerHandle(data);

	PX_ASSERT(mPruners[index]);

	if(mDirtyMap[index].test(handle))
	{
		mDirtyMap[index].reset(handle);
		mDirtyList.findAndReplaceWithLast(data);
	}

	mTimestamp[index]++;
	mPruners[index]->removeObjects(&handle);
}

void SceneQueryManager::setDynamicTreeRebuildRateHint(PxU32 rebuildRateHint)
{
	mRebuildRateHint = rebuildRateHint;

	for(PxU32 i=0;i<2;i++)
	{
		if(mPruners[i] && mPrunerType[i] == PxPruningStructure::eDYNAMIC_AABB_TREE)
			static_cast<AABBPruner*>(mPruners[i])->setRebuildRateHint(rebuildRateHint);
	}
}


PX_FORCE_INLINE	bool SceneQueryManager::updateObject(PxU32 index, PrunerHandle handle)
{
	// this method is only logically const and should not be 
	// called from multiple threads without synchronisation

	const PrunerPayload& pp = mPruners[index]->getPayload(handle);
	PxBounds3 worldAABB = Sq::computeWorldAABB(*(Scb::Shape*)pp.data[0], *(Scb::Actor*)pp.data[1]);

	mTimestamp[index]++;

	// Update appropriate pool
	mPruners[index]->updateObjects(&handle,&worldAABB);
	return true;
}


PX_FORCE_INLINE void SceneQueryManager::processActiveShapes(ActorShape** PX_RESTRICT data, PxU32 nb)
{
	for(PxU32 i=0;i<nb;i++)
	{
		PxU32 index = getPrunerIndex(data[i]);
		PrunerHandle handle = getPrunerHandle(data[i]);

		if(!mDirtyMap[index].test(handle))	// PT: if dirty, will be updated in "flushUpdates"
			updateObject(index, handle);
	}
}

void SceneQueryManager::processSimUpdates()
{
	// update all active objects
	Sc::BodyIterator actorIterator;
	mScene.initActiveBodiesIterator(actorIterator);

	ActorShape* tmpBuffer[4];
	PxU32 nb=0;

	Sc::BodyCore* b = NULL;
	while(NULL != (b = actorIterator.getNext()) )
	{
		PxRigidBody* pxBody = static_cast<PxRigidBody*>(b->getPxActor());

		PX_ASSERT(pxBody->getConcreteType()==PxConcreteType::eRIGID_DYNAMIC || pxBody->getConcreteType()==PxConcreteType::eARTICULATION_LINK);
	
		NpShapeManager& shapeManager = 
			pxBody->getConcreteType()==PxConcreteType::eRIGID_DYNAMIC ? static_cast<NpRigidDynamic*>(pxBody)->getShapeManager()
																	  :	static_cast<NpArticulationLink*>(pxBody)->getShapeManager();

		NpShape*const * shapes = shapeManager.getShapes();
		const PxU32 nbShapes = shapeManager.getNbShapes();

		Sq::ActorShape*const * sqData = shapeManager.getSceneQueryData();
		for(PxU32 i = 0; i<nbShapes; i++)
		{
			if(sqData[i])
			{
				tmpBuffer[nb++] = sqData[i];
				const char* p = (const char*)shapes[i];
				Ps::prefetchLine(p);
				Ps::prefetchLine(p+128);
				Ps::prefetchLine(p+256);
			}

			if(nb==4)
			{
				nb = 0;
				processActiveShapes(tmpBuffer, 4);
			}
		}
	}
	processActiveShapes(tmpBuffer, nb);

	// flush user modified objects
	flushShapes();

	for(PxU32 i=0;i<2;i++)
	{
		if(mPruners[i] && mPrunerType[i] == PxPruningStructure::eDYNAMIC_AABB_TREE)
			static_cast<AABBPruner*>(mPruners[i])->buildStep();

		mPruners[i]->commit();
	}
}

void SceneQueryManager::flushShapes()
{
	// must already have acquired writer lock here

	PxU32 numDirtyList = mDirtyList.size();
	for(PxU32 i = 0; i < numDirtyList; i++)
	{
		ActorShape* data = mDirtyList[i];

		PxU32 index = getPrunerIndex(data);
		PrunerHandle handle = getPrunerHandle(data);

		mDirtyMap[index].reset(handle);
		updateObject(index, handle);
	}
	mDirtyList.clear();
}

void SceneQueryManager::flushUpdates()
{
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(mScene,SceneQuery,flushUpdates);

	// no need to take lock if manual sq update is enabled
	// as flushUpdates will only be called from NpScene::flushQueryUpdates()
	mSceneQueryLock.lock();

	flushShapes();

	for(PxU32 i=0;i<2;i++)
		if(mPruners[i])
			mPruners[i]->commit();

	mSceneQueryLock.unlock();
}

void SceneQueryManager::forceDynamicTreeRebuild(bool rebuildStaticStructure, bool rebuildDynamicStructure)
{
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(mScene,Sim,updatePruningTrees);

	bool rebuild[2] = { rebuildStaticStructure, rebuildDynamicStructure };

	Ps::Mutex::ScopedLock lock(mSceneQueryLock);
	for(PxU32 i=0; i<2; i++)
	{
		if(rebuild[i] && mPruners[i] && mPrunerType[i] == PxPruningStructure::eDYNAMIC_AABB_TREE)
		{
			static_cast<AABBPruner*>(mPruners[i])->purge();
			static_cast<AABBPruner*>(mPruners[i])->commit();
		}
	}
}

void SceneQueryManager::shiftOrigin(const PxVec3& shift)
{
	mPruners[0]->shiftOrigin(shift);
	mPruners[1]->shiftOrigin(shift);
}

PxScene* SceneQueryManager::getPxScene() const
{
	return getScene().getPxScene();
}

#if PX_IS_PS3 && !PX_IS_SPU // the task only exists on PPU
#include "PxSpuTask.h"
#include "PS3Support.h"

namespace physx { namespace Sq {

struct ROSSpuTask : public PxSpuTask
{
	ROSSpuTask(CellSpursElfId_t taskId, PxU32 numSpus, shdfnd::Sync& sync)
		: PxSpuTask(gPS3GetElfImage(taskId), gPS3GetElfSize(taskId), numSpus), mSync(sync)
	{}

	virtual void		release()		{ mSync.set(); }

	shdfnd::Sync& mSync;
	NpQuerySpuContext mSpuContext;
};

struct RaycastSpuTask : public ROSSpuTask
{
	RaycastSpuTask(shdfnd::Sync& sync, PxU32 numSpus) : ROSSpuTask(SPU_ELF_RAYCAST_TASK, numSpus, sync) {}
	virtual const char* getName() const { return "RaycastSpuTask"; }
};

struct OverlapSpuTask : public ROSSpuTask
{
	OverlapSpuTask(shdfnd::Sync& sync, PxU32 numSpus) : ROSSpuTask(SPU_ELF_OVERLAP_TASK, numSpus, sync) {}
	virtual const char* getName() const { return "OverlapSpuTask"; }
};

struct SweepSpuTask : public ROSSpuTask
{
	SweepSpuTask(shdfnd::Sync& sync, PxU32 numSpus) : ROSSpuTask(SPU_ELF_SWEEP_TASK, numSpus, sync) {}
	virtual const char* getName() const { return "SweepSpuTask"; }
};

}} // physx::Sq
#endif

void SceneQueryManager::freeSPUTasks(NpBatchQuery* bq)
{ 
	PX_UNUSED(bq);
	#if PX_IS_PS3 && !PX_IS_SPU // the task only exists on PPU
	if (bq->mRaycastTask)
	{
		bq->mRaycastTask->~RaycastSpuTask();
		AlignedAllocator<128>().deallocate(bq->mRaycastTask);
		bq->mRaycastTask = NULL;
	}
	if (bq->mOverlapTask)
	{
		bq->mOverlapTask->~OverlapSpuTask();
		AlignedAllocator<128>().deallocate(bq->mOverlapTask);
		bq->mOverlapTask = NULL;
	}
	if (bq->mSweepTask)
	{
		bq->mSweepTask->~SweepSpuTask();
		AlignedAllocator<128>().deallocate(bq->mSweepTask);
		bq->mSweepTask = NULL;
	}
	#endif
}

bool SceneQueryManager::canRunOnSPU(const NpBatchQuery& bq) const
{
	PxPruningStructure::Enum staticType = getStaticStructure();
	PxPruningStructure::Enum dynamicType = getDynamicStructure();
	PxPruningStructure::Enum s0 = PxPruningStructure::eSTATIC_AABB_TREE;
	PxPruningStructure::Enum d0 = PxPruningStructure::eDYNAMIC_AABB_TREE;
	bool canRunOnSPU = ((staticType == s0 || staticType == d0) && (dynamicType == s0 || dynamicType == d0));
	if(!canRunOnSPU)
		PX_WARN_ONCE(true,		"BatchedQuery: PxPruningStructure not supported on SPU, reverting to PPU.");

	if(bq.getDesc().runOnSpu && !getPxScene()->getTaskManager()->getSpuDispatcher())
	{
		PX_WARN_ONCE(true,		"BatchedQuery: No SpuDispatcher available, reverting to PPU.");
		canRunOnSPU = false;
	}

	return canRunOnSPU;
}

#if PX_IS_PPU
void SceneQueryManager::fallbackToPPUByType(const NpBatchQuery& bq, bool runOnPPU[3]) const
{	
	const PxBatchQueryDesc& qdesc = bq.getDesc();

	PxU32 counts[3] = { bq.mNbRaycasts, bq.mNbOverlaps, bq.mNbSweeps }; 

	bool isMemOnStack[3] = { 
		isMemoryOnStack(qdesc.queryMemory.userRaycastResultBuffer)	||	isMemoryOnStack(qdesc.queryMemory.userRaycastTouchBuffer),
		isMemoryOnStack(qdesc.queryMemory.userOverlapResultBuffer)	||	isMemoryOnStack(qdesc.queryMemory.userOverlapTouchBuffer),
		isMemoryOnStack(qdesc.queryMemory.userSweepResultBuffer)	||	isMemoryOnStack(qdesc.queryMemory.userSweepTouchBuffer)
	};

	bool isNotAligned16[3] = {
		(PxU32)qdesc.queryMemory.userRaycastResultBuffer	& 0xF	||	(PxU32)qdesc.queryMemory.userRaycastTouchBuffer	& 0xF,
		(PxU32)qdesc.queryMemory.userOverlapResultBuffer	& 0xF	||  (PxU32)qdesc.queryMemory.userOverlapTouchBuffer	& 0xF,
		(PxU32)qdesc.queryMemory.userSweepResultBuffer		& 0xF	||	(PxU32)qdesc.queryMemory.userSweepTouchBuffer	& 0xF
	};

	runOnPPU[0] = runOnPPU[1] = runOnPPU[2] = true;

	for(PxU32 i = 0; i < 3; i++)
	{
		if(bq.getDesc().runOnSpu && counts[i] > 0)
		{
			runOnPPU[i] = false;

			if ((qdesc.preFilterShader  && !qdesc.spuPreFilterShader) ||
				(qdesc.postFilterShader && !qdesc.spuPostFilterShader))
			{
				PX_WARN_ONCE(true,		"BatchedQuery: PPU filter shader but no SPU filter shader set, reverting to PPU.");
				runOnPPU[i] = true;
			}

			if (isMemOnStack[i])
			{
				PX_WARN_ONCE(true,		"BatchedQuery: User allocated buffer must not be on the stack, reverting to PPU.");
				runOnPPU[i] = true;
			}
			
			if (isNotAligned16[i])
			{
				PX_WARN_ONCE(true,		"BatchedQuery: User allocated buffer must be 16 bytes aligned, reverting to PPU.");
				runOnPPU[i] = true;
			}
		}
	}
}
#endif

void SceneQueryManager::blockingSPURaycastOverlapSweep(NpBatchQuery* bq, bool runOnPPU[3])
{
	PX_ASSERT(canRunOnSPU(*bq));
	PX_UNUSED(bq);
	PX_UNUSED(runOnPPU);

	#if PX_IS_PPU // PS3 PPU only code

	if (bq->mRaycastTask == NULL)
		bq->mRaycastTask = (RaycastSpuTask*)AlignedAllocator<128>().allocate(sizeof(RaycastSpuTask), __FILE__, __LINE__);
	if (bq->mOverlapTask == NULL)
		bq->mOverlapTask = (OverlapSpuTask*)AlignedAllocator<128>().allocate(sizeof(OverlapSpuTask), __FILE__, __LINE__);
	if (bq->mSweepTask == NULL)
		bq->mSweepTask = (SweepSpuTask*)AlignedAllocator<128>().allocate(sizeof(SweepSpuTask), __FILE__, __LINE__);

	ROSSpuTask* tasks[] = {bq->mRaycastTask, bq->mOverlapTask, bq->mSweepTask};

	// initialize pre-allocated tasks with updated query parameters
	PX_PLACEMENT_NEW(bq->mRaycastTask, RaycastSpuTask)(bq->mSync, 1);
	PX_PLACEMENT_NEW(bq->mOverlapTask, OverlapSpuTask)(bq->mSync, 1);
	PX_PLACEMENT_NEW(bq->mSweepTask, SweepSpuTask)(bq->mSync, 1);

	fallbackToPPUByType(*bq,runOnPPU);

	for (PxU32 i = 0; i < 3; i ++)
	{
		if(runOnPPU[i])
			continue;

		ROSSpuTask* task = tasks[i];

		const AABBPruner* sp = static_cast<const AABBPruner*>(getStaticPruner());
		const AABBPruner* dp = static_cast<const AABBPruner*>(getDynamicPruner());
		task->mSpuContext.staticPruner = sp;
		task->mSpuContext.dynamicPruner = dp;
		task->mSpuContext.staticTree = (Gu::AABBTree*)sp->getAABBTree();
		task->mSpuContext.dynamicTree = (Gu::AABBTree*)dp->getAABBTree();
		task->mSpuContext.staticNodes = sp->getAABBTree() ? (Gu::AABBTreeNode*)sp->getAABBTree()->GetNodes() : NULL;
		task->mSpuContext.dynamicNodes = dp->getAABBTree() ? (Gu::AABBTreeNode*)dp->getAABBTree()->GetNodes() : NULL;
		task->mSpuContext.scenePassForeignShapes = getScene().getClientBehaviorFlags(bq->mDesc.ownerClient) & PxClientBehaviorFlag::eREPORT_FOREIGN_OBJECTS_TO_SCENE_QUERY;
		task->mSpuContext.actorOffsets = &NpActor::sOffsets;

		task->setArgs(0, PxU32(&task->mSpuContext), PxU32(bq));
		task->setContinuation(*getPxScene()->getTaskManager(), NULL);
		task->removeReference();

		// each task runs in wide mode on multiple SPUs, we don't try to run raycasts and overlaps and sweeps at the same time
		bq->mSync.wait();
		bq->mSync.reset();
	}

	#endif // PX_IS_PPU
}

