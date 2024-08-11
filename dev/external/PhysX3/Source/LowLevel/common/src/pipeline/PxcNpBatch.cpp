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


#include "PxcNpBatch.h"
#include "PxsThreadContext.h"
#include "PxcNpWorkUnit.h"
#include "PxcNpContactPrep.h"
#include "PxsContactManager.h"
#include "GuGeometryUnion.h"
#include "PxcContactMethodImpl.h"
#include "PxcMaterialMethodImpl.h"
#include "PxcNpContactPrep.h"
#include "PxvDynamics.h"			// for PxsBodyCore
#include "PxvGeometry.h"			// for PxsShapeCore
#include "CmFlushPool.h"
#include "PxTriangleMesh.h"
#include "PxsMaterialManager.h"
#include "PxsTransformCache.h"

#include "./Ice/IceUtils.h"

using namespace physx;
using namespace Gu;

#if defined(PX_PS3)
#define SPU_NARROWPHASE 1
#define FORCE_SINGLE_SPU 0
#endif

#if SPU_NARROWPHASE

#include "CellTimerMarker.h"
#include "CellNarrowPhaseTask.h"
#include "PS3Support.h"
#include "PxsResourceManager.h"
#include "ps3/PxPS3Config.h"

#endif

#define	LOW_LEVEL_DEBUG	0

#if CACHE_LOCAL_CONTACTS_XP
namespace physx
{
	void PxcCacheLocalContacts(const PxGeometryType::Enum type0, const PxGeometryType::Enum type1, PxcNpThreadContext& context, PxcNpCache& pairContactCache, const PxTransform& tm0, const PxTransform& tm1,
							   const PxReal contactDistance, const PxcContactMethod conMethod, const Gu::GeometryUnion& shape0, const Gu::GeometryUnion& shape1);
}
#endif
	
static void startContacts(PxcNpWorkUnit& n, PxcNpThreadContext& context)
{
	context.mContactBuffer.reset();
	
	PxcNpWorkUnitClearContactState(n);
}

static void flipContacts(PxcNpThreadContext& threadContext, PxsMaterialInfo* PX_RESTRICT materialInfo)
{
	ContactBuffer& buffer = threadContext.mContactBuffer;
	for(PxU32 i=0; i<buffer.count; ++i)
	{
		Gu::ContactPoint& contactPoint = buffer.contacts[i];
		contactPoint.normal = -contactPoint.normal;
		Gu::TSwap(contactPoint.internalFaceIndex0, contactPoint.internalFaceIndex1);
		Gu::TSwap(materialInfo[i].mMaterialIndex0, materialInfo[i].mMaterialIndex1);
	}
}

static PX_INLINE PxTransform getShapeAbsPose(const PxsShapeCore* shapeCore, const PxsRigidCore* rigidCore, PxU32 isDynamic)
{
	if(isDynamic)
	{
		const PxsBodyCore* PX_RESTRICT bodyCore = static_cast<const PxsBodyCore* PX_RESTRICT>(rigidCore);
		return bodyCore->body2World * bodyCore->body2Actor.getInverse() *shapeCore->transform;
	}
	else 
	{
		return rigidCore->body2World * shapeCore->transform;
	}
}


void runNpBatchPPU(	PxcNpThreadContext* context,
					PxcNpBatchEntry* entries, PxU32 numEntriesStart, PxU32 numEntriesEnd,
					PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
					PxU32& _touchLost, PxU32& _touchFound)
{
	PX_UNUSED(changeBitmapWordCount);

	PxU32 touchLost = 0;
	PxU32 touchFound = 0;

	for(PxU32 i=numEntriesStart;i<numEntriesEnd;i++)
	{
		PxcNpWorkUnit* unit = entries[i].workUnit;

		PxsShapeCore* shape0 = const_cast<PxsShapeCore*>(unit->shapeCore0);
		PxsShapeCore* shape1 = const_cast<PxsShapeCore*>(unit->shapeCore1);

		PxGeometryType::Enum type0 = shape0->geometry.getType();
		PxGeometryType::Enum type1 = shape1->geometry.getType();

		PxsRigidCore* body0 = const_cast<PxsRigidCore*>(unit->rigidCore0);
		PxsRigidCore* body1 = const_cast<PxsRigidCore*>(unit->rigidCore1);

		PxU32 body0Dynamic = (unit->flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY0);
		PxU32 body1Dynamic = (unit->flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1);

		bool flip = (type1<type0);
		if(flip)
		{
			Gu::TSwap(body0, body1);
			Gu::TSwap(type0, type1);
			Gu::TSwap(shape0, shape1);
			Gu::TSwap(body0Dynamic, body1Dynamic);
		}

#if PX_ENABLE_SIM_STATS
		context->discreteContactPairs[type0][type1]++;
#endif
		PX_ALIGN(16, PxTransform tm0) = getShapeAbsPose(shape0, body0, body0Dynamic);
		PX_ALIGN(16, PxTransform tm1) = getShapeAbsPose(shape1, body1, body1Dynamic);

		bool oldTouch = unit->touch;

		startContacts(*unit, *context);

		PxcContactMethod conMethod = g_ContactMethodTable[type0][type1];
		
#if CACHE_LOCAL_CONTACTS_XP
		const bool useContactCache = context->mContactCache && g_CanUseContactCache[type0][type1];
		if(useContactCache)
		{
			PxcCacheLocalContacts(type0, type1, *context, unit->pairCache, tm0, tm1, shape0->contactOffset + shape1->contactOffset, conMethod, shape0->geometry, shape1->geometry);
		}
		else
		{
			conMethod(shape0->geometry, shape1->geometry,tm0, tm1, shape0->contactOffset + shape1->contactOffset, unit->pairCache, context->mContactBuffer);
		}
#else
		conMethod(shape0->geometry, shape1->geometry,tm0, tm1, shape0->contactOffset + shape1->contactOffset, unit->pairCache, context->mContactBuffer);
#endif

		PxsMaterialInfo materialInfo[ContactBuffer::MAX_CONTACTS];

		PxcGetMaterialMethod materialMethod = g_GetMaterialMethodTable[type0][type1];

		PX_ASSERT(materialMethod);

		materialMethod(shape0, shape1, *context,  materialInfo);

		if(flip)
			flipContacts(*context, materialInfo);

		finishContacts(*unit, *context, materialInfo);

		const bool newTouch = unit->touch;
		if(oldTouch ^ newTouch)
		{
			const PxU32 index = entries[i].cmIndex;
			changeBitmapBase[index>>5] |= 1<<(index&31);
			if(oldTouch)
				touchLost++;
			else
				touchFound++;
		}
	}

	_touchLost += touchLost;
	_touchFound += touchFound;
}

void runNpPCMBatchPPU(	PxcNpThreadContext* context,
						PxcNpBatchEntry* entries, PxU32 numEntriesStart, PxU32 numEntriesEnd,
						PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
						PxU32& _touchLost, PxU32& _touchFound)
{
	PX_UNUSED(changeBitmapWordCount);

	PxU32 touchLost = 0;
	PxU32 touchFound = 0;

	for(PxU32 i=numEntriesStart;i<numEntriesEnd;i++)
	{
		PxcNpWorkUnit* unit = entries[i].workUnit;

		PxsShapeCore* shape0 = const_cast<PxsShapeCore*>(unit->shapeCore0);
		PxsShapeCore* shape1 = const_cast<PxsShapeCore*>(unit->shapeCore1);

		PxGeometryType::Enum type0 = shape0->geometry.getType();
		PxGeometryType::Enum type1 = shape1->geometry.getType();


		PxsRigidCore* body0 = const_cast<PxsRigidCore*>(unit->rigidCore0);
		PxsRigidCore* body1 = const_cast<PxsRigidCore*>(unit->rigidCore1);

		PxU32 body0Dynamic = (unit->flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY0);
		PxU32 body1Dynamic = (unit->flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1);

		bool flip = (type1<type0);
		if(flip)
		{
			Gu::TSwap(body0, body1);
			Gu::TSwap(type0, type1);
			Gu::TSwap(shape0, shape1);
			Gu::TSwap(body0Dynamic, body1Dynamic);
		}

#if PX_ENABLE_SIM_STATS
		context->discreteContactPairs[type0][type1]++;
#endif
		PX_ALIGN(16, PxTransform tm0) = getShapeAbsPose(shape0, body0, body0Dynamic);
		PX_ALIGN(16, PxTransform tm1) = getShapeAbsPose(shape1, body1, body1Dynamic);

		bool oldTouch = unit->touch;

		startContacts(*unit, *context);

		const PxcContactMethod conMethod = g_PCMContactMethodTable[type0][type1];
		
		
#if CACHE_LOCAL_CONTACTS_XP
		const bool useContactCache = context->mContactCache && g_CanUsePCMContactCache[type0][type1];
		if(useContactCache)
		{
			PxcCacheLocalContacts(type0, type1, *context, unit->pairCache, tm0, tm1, shape0->contactOffset + shape1->contactOffset, conMethod, shape0->geometry, shape1->geometry);
		}
		else
		{
			conMethod(shape0->geometry, shape1->geometry,tm0, tm1, shape0->contactOffset + shape1->contactOffset, unit->pairCache, context->mContactBuffer);
		}
#else
		conMethod(shape0->geometry, shape1->geometry,tm0, tm1, shape0->contactOffset + shape1->contactOffset, unit->pairCache, context->mContactBuffer);
#endif

		PX_ASSERT(conMethod);

		PxsMaterialInfo materialInfo[ContactBuffer::MAX_CONTACTS];

		PxcGetMaterialMethod materialMethod = g_GetMaterialMethodTable[type0][type1];

		PX_ASSERT(materialMethod);

		materialMethod(shape0, shape1, *context,  materialInfo);

		if(flip)
			flipContacts(*context, materialInfo);

		finishContacts(*unit, *context, materialInfo);

		const bool newTouch = unit->touch;
		if(oldTouch ^ newTouch)
		{
			const PxU32 index = entries[i].cmIndex;
			changeBitmapBase[index>>5] |= 1<<(index&31);
			if(oldTouch)
				touchLost++;
			else
				touchFound++;
		}
	}

	_touchLost += touchLost;
	_touchFound += touchFound;
}

#if(SPU_NARROWPHASE)

#include <PxSpuTask.h>
#include <PsSync.h>

class NarrowPhaseEndTask : public physx::PxLightCpuTask
{
public:

	NarrowPhaseEndTask(PxcNpMemBlockPool& memPool, PxU32 acquiredContactStart, 
		PxU32 numAcquiredContactBlocks, PxU32 acquiredNpCacheStart, 
		PxU32 numAcquiredNpCacheBlocks, 
		PxsResourceManager* resourceManager,
		PxsResourceManager* resourceCacheManager) :
		mMemPool(&memPool),
		mAcquiredContactStart(acquiredContactStart),
		mNumAcquiredContactBlocks(numAcquiredContactBlocks),
		mAcquiredNpCacheStart(acquiredNpCacheStart),
		mNumAcquiredNpCacheBlocks(numAcquiredNpCacheBlocks),
		mResourceManager(resourceManager),
		mResourceCacheManager(resourceCacheManager)
	{
	}

	virtual void run()
	{
		//Release any unused memory blocks.
		mMemPool->releaseMultipleContactBlocks(mAcquiredContactStart + mResourceManager->getNextFreeBlockCount(), mAcquiredContactStart + mNumAcquiredContactBlocks);

#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
		mMemPool->releaseMultipleCacheBlocks(mAcquiredNpCacheStart + mResourceCacheManager->getNextFreeBlockCount(), mAcquiredNpCacheStart + mNumAcquiredNpCacheBlocks);
#endif
	}

	virtual const char* getName() const { return "NarrowPhaseEndTask"; }

	PxcNpMemBlockPool* mMemPool;
	PxU32 mAcquiredContactStart;
	PxU32 mNumAcquiredContactBlocks;
	PxU32 mAcquiredNpCacheStart;
	PxU32 mNumAcquiredNpCacheBlocks;
	PxsResourceManager* mResourceManager;
	PxsResourceManager* mResourceCacheManager;
};

class NarrowPhaseSpuTask : public physx::PxSpuTask
{
public:

	NarrowPhaseSpuTask(const void* elfImage=NULL, PxU32 elfSize=0, const char* name=NULL) 
		: PxSpuTask(elfImage, elfSize), mName(name) {}

	virtual void release()
	{
		*mOutTouchLost += mTouchLost;
		*mOutTouchFound += mTouchFound;

#ifdef PX_CHECKED
		bool successfulSpuCompletion=true;
		bool convexError = false;
#endif

		for(PxU32 uiTask=0; uiTask<getSpuCount(); uiTask++)
		{
			//It's possible that the spu failed to complete all the work assigned
			//to it due to limited memory resources. Process whatever remains on ppu.
			const PxU32 ppuStart=mSpuOutput[uiTask].mPPUStartIndex;
			const PxU32 ppuEnd=mSpuOutput[uiTask].mSPUEndIndex;
			if(ppuStart<ppuEnd)
			{
#ifdef PX_CHECKED
				successfulSpuCompletion=false;
#endif
				if(mSpuInput.mContext->mPCM)
				{
					runNpPCMBatchPPU(mSpuInput.mContext,mSpuInput.mEntriesStartEA,ppuStart,ppuEnd,mSpuInput.mChangeBitMapBase,mSpuInput.mChangeBitMapWordCount,*mOutTouchLost,*mOutTouchFound); 
				}
				else
				{
					runNpBatchPPU(mSpuInput.mContext,mSpuInput.mEntriesStartEA,ppuStart,ppuEnd,mSpuInput.mChangeBitMapBase,mSpuInput.mChangeBitMapWordCount,*mOutTouchLost,*mOutTouchFound); 
				}
			}

			if(mSpuInput.mChangeBitMapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
			{
				Cm::BitMap bitmapTar;
				bitmapTar.setWords(mSpuInput.mChangeBitMapBase,mSpuInput.mChangeBitMapWordCount);
				Cm::BitMap bitmapSrc;
				bitmapSrc.setWords(mSpuOutput[uiTask].mChangeBitmapBase, mSpuInput.mChangeBitMapWordCount);
				bitmapTar.combineInPlace<Cm::BitMap::OR>(bitmapSrc);
			}
			else
			{
				//Do some post-processing on the items processed on spu.
				const PxU32 spuStart=mSpuOutput[uiTask].mSPUStartIndex;
				for(PxU32 i=spuStart;i<ppuStart;i++)
				{
					PxcNpWorkUnit *unit = mSpuInput.mEntriesStartEA[i].workUnit;
					if(unit->touchLost || unit->touchFound)
					{
						PxU32 index = mSpuInput.mEntriesStartEA[i].cmIndex;
						mSpuInput.mChangeBitMapBase[index>>5] |= 1<<(index&31);
					}
				}
			}

#ifdef PX_CHECKED
			if(mSpuOutput[uiTask].mSphereVsMeshContactLimitExceeded)
			{
				Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Dropping contacts in sphere vs mesh: exceeded limit of 64 ");
			}
			if(mSpuOutput[uiTask].mErrorCode)
			{
				convexError = true;
				Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Narrow phase completed on ppu because a convex was too large for spu processing");
			}
#endif

		}

#ifdef PX_CHECKED
		if(!successfulSpuCompletion && !convexError)
		{
			PX_WARN_ONCE(true, "Narrow phase completed on ppu because spu ran out of 16K data blocks.");

			PX_WARN_ONCE(true, "Increase number reserved for spu with PxPS3Config::setSceneParamInt(myScene,PxPS3ConfigParam::eMEM_CONTACT_STREAM_BLOCKS)");

			PX_WARN_ONCE(true, "Be aware that PxSceneDesc::maxNbContactDataBlocks may need to be increased accordingly to ensure");
			PX_WARN_ONCE(true, " that there are sufficient 16K blocks to match the total number reserved for spu.");

			PX_WARN_ONCE(true, "See PxPS3Config::getSpuMemBlockCounters() for more information");
		}
#endif
		
		PxSpuTask::release();
	}

	virtual const char* getName() const { return mName; }

	CellNarrowPhaseSPUInput	PX_ALIGN(128, mSpuInput);				
	CellNarrowPhaseSPUOutput PX_ALIGN(128, mSpuOutput[6]);

	PxU32 PX_ALIGN(128, mTouchLost);
	PxU32 PX_ALIGN(128, mTouchFound);

	PxU32* mOutTouchLost;
	PxU32* mOutTouchFound;

	const char* mName;
};

namespace
{
	NarrowPhaseSpuTask* createNPTask(Cm::FlushPool& pool, CellSpursElfId_t elfId, const char* name)
	{
		return PX_PLACEMENT_NEW(pool.allocate((sizeof(NarrowPhaseSpuTask)), 128), NarrowPhaseSpuTask)(gPS3GetElfImage(elfId), gPS3GetElfSize(elfId), name);
	}
}

void runNpBatchSPU(PxcNpThreadContext* context,
				   PxcNpBatchEntry* entries, PxU32 entryCount,
			  	   PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
				   PxU32& touchLost, PxU32& touchFound,
				   NarrowPhaseSpuTask& task, PxLightCpuTask* continuation, const PxU32 numSpus,
				   PxsResourceManager* resourceManager,  
				   PxsResourceManager* resourceCachesManager)
{
	if(0==entryCount)
	{
		return;
	}

	PX_ASSERT(changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS);

#if FORCE_SINGLE_SPU
	const PxU32 numSpusForNarrowPhase=1;
#else
	const PxU32 numSpusForNarrowPhase=numSpus;
#endif

	task.mSpuInput.mContext						= context;
	task.mSpuInput.mEntriesStartEA				= entries;
	task.mSpuInput.mNumEntries					= entryCount;
	task.mSpuInput.mChangeBitMapBase			= changeBitmapBase;
	task.mSpuInput.mChangeBitMapWordCount		= changeBitmapWordCount;
	task.mSpuInput.mManagerEA					= resourceManager;
	//task.mSpuInput.mManagerFrictionPatchesEA	= resourceFrictionManager;
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	task.mSpuInput.mManagerCachesEA				= resourceCachesManager;
#endif
	task.mTouchLost = 0;
	task.mSpuInput.mTouchLost					= &task.mTouchLost;
	task.mTouchFound = 0;
	task.mSpuInput.mTouchFound					= &task.mTouchFound;
	task.mSpuInput.mNumBitMapWords				= changeBitmapWordCount;
	task.mSpuInput.mNumSpus						= numSpusForNarrowPhase;

	task.mOutTouchLost = &touchLost;
	task.mOutTouchFound = &touchFound;

	//context->mContactBuffer.reset();

	task.setSpuCount(numSpusForNarrowPhase);

	for(PxU32 uiTask=0; uiTask<numSpusForNarrowPhase; uiTask++)
	{
		task.setArgs(uiTask, uiTask | (unsigned int)&task.mSpuOutput[uiTask],(unsigned int)&task.mSpuInput);
	}

	task.setContinuation(continuation);
	task.removeReference();
}

#endif

void physx::PxcSkipNarrowPhase(PxcNpWorkUnit &n)
{
	// blow away the entire contact state and cache pointers

	PxcNpWorkUnitClearContactState(n);
	PxcNpWorkUnitClearCachedState(n);
}

void physx::PxcDiscreteNarrowPhase(PxcNpThreadContext& context, PxcNpWorkUnit& n)
{
	PxGeometryType::Enum g0 = static_cast<PxGeometryType::Enum>(n.geomType0);
	PxGeometryType::Enum g1 = static_cast<PxGeometryType::Enum>(n.geomType1);

	PxsShapeCore* shape0 = const_cast<PxsShapeCore*>(n.shapeCore0);
	PxsShapeCore* shape1 = const_cast<PxsShapeCore*>(n.shapeCore1);

	PxsRigidCore* body0 = const_cast<PxsRigidCore*>(n.rigidCore0);
	PxsRigidCore* body1 = const_cast<PxsRigidCore*>(n.rigidCore1);

	PxU32 body0Dynamic = (n.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY0);
	PxU32 body1Dynamic = (n.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1);

	PxU32 tmCache0 = n.mTransformCache0;
	PxU32 tmCache1 = n.mTransformCache1;


	bool flip = (g1<g0);
	if(flip)
	{
		Gu::TSwap(body0, body1);
		Gu::TSwap(g0, g1);
		Gu::TSwap(shape0, shape1);
		Gu::TSwap(body0Dynamic, body1Dynamic);
		Gu::TSwap(tmCache0, tmCache1);
	}

	//PX_ASSERT(g0<=g1);
#if PX_ENABLE_SIM_STATS
	context.discreteContactPairs[g0][g1]++;
#endif
	PxcContactMethod conMethod = g_ContactMethodTable[g0][g1];
	
	PX_ASSERT(conMethod);

	startContacts(n, context);

	// PT: many cache misses here...
	// PT: TODO: refactor this change with runNpBatchPPU

	
	Ps::prefetchLine(shape1, 0);	// PT: at least get rid of L2s for shape1
	Ps::prefetchLine(body1, 0);	// PT: at least get rid of L2s for body1

	// PT: this form is cleaner but suffers from 10000+ LHS in "pot_pourri_box"...
//	PxTransform tm0 = body0 ? body0->body2World.transform(shape0->transform) : shape0->transform;
//	PxTransform tm1 = body1 ? body1->body2World.transform(shape1->transform) : shape1->transform;
//	PX_ASSERT(tm0.isValid() && tm1.isValid());

	// PT: ...while this version has none.
	// KS: transforms are aligned in shape. If we align them here also, we can use aligned loads in leaf functions
	PX_ALIGN(16, PxTransform tm0memory);
	PX_ALIGN(16, PxTransform tm1memory);

	tm0memory = context.mTransformCache->getTransformCache(tmCache0);
	tm1memory = context.mTransformCache->getTransformCache(tmCache1);

	const PxTransform* tm0 = &tm0memory;
	const PxTransform* tm1 = &tm1memory;

	PX_ASSERT(tm0->isSane() && tm1->isSane());

	const PxReal contactDistance = shape0->contactOffset + shape1->contactOffset;	//temp until we refactor this into a parameter.

#if CACHE_LOCAL_CONTACTS_XP
	const bool useContactCache = context.mContactCache && g_CanUseContactCache[g0][g1];
	if(useContactCache)
	{
		PxcCacheLocalContacts(g0, g1, context, n.pairCache, *tm0, *tm1, contactDistance, conMethod, shape0->geometry, shape1->geometry);
	}
	else
	{
		conMethod(shape0->geometry, shape1->geometry, *tm0, *tm1, contactDistance, n.pairCache, context.mContactBuffer);
	}
#else
	conMethod(shape0->geometry, shape1->geometry, *tm0, *tm1, contactDistance, n.pairCache, context.mContactBuffer);
#endif

	PxsMaterialInfo materialInfo[ContactBuffer::MAX_CONTACTS];

	PxcGetMaterialMethod materialMethod = g_GetMaterialMethodTable[g0][g1];

	PX_ASSERT(materialMethod);

	materialMethod(shape0, shape1, context, materialInfo);

	if(flip)
		flipContacts(context, materialInfo);

	finishContacts(n, context, materialInfo);
}


void physx::PxcDiscreteNarrowPhasePCM(PxcNpThreadContext& context, PxcNpWorkUnit& n)
{

//#ifdef	LOW_LEVEL_DEBUG
//	n.pairCache.mRenderOutput = &context.mRenderOutput;
//#endif
	
	Ps::prefetch((void*)n.pairCache.manifold);
	Ps::prefetch((void*)n.pairCache.manifold,128);
	Ps::prefetch((void*)n.pairCache.manifold,256);
	PxGeometryType::Enum g0 = static_cast<PxGeometryType::Enum>(n.geomType0);
	PxGeometryType::Enum g1 = static_cast<PxGeometryType::Enum>(n.geomType1);

	PxsShapeCore* shape0 = const_cast<PxsShapeCore*>(n.shapeCore0);
	PxsShapeCore* shape1 = const_cast<PxsShapeCore*>(n.shapeCore1);

	PxsRigidCore* body0 =const_cast<PxsRigidCore*>(n.rigidCore0);
	PxsRigidCore* body1 =const_cast<PxsRigidCore*>(n.rigidCore1);

	PxU32 body0Dynamic = (n.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY0);
	PxU32 body1Dynamic = (n.flags & PxcNpWorkUnitFlag::eDYNAMIC_BODY1);


	// KS: transforms are aligned in shape. If we align them here also, we can use aligned loads in leaf functions
	PX_ALIGN(16, PxTransform tm0memory);
	PX_ALIGN(16, PxTransform tm1memory);

	/*tm0memory = getShapeAbsPose(shape0, body0, body0Dynamic);
	tm1memory = getShapeAbsPose(shape1, body1, body1Dynamic);*/

	tm0memory = context.mTransformCache->getTransformCache(n.mTransformCache0);
	tm1memory = context.mTransformCache->getTransformCache(n.mTransformCache1);

	const PxReal contactDistance = shape0->contactOffset + shape1->contactOffset;	//temp until we refactor this into a parameter.

	bool flip = (g1<g0);

	PxTransform* tm0 = &tm0memory;
	PxTransform* tm1 = &tm1memory;

	if(flip)
	{
		tm0 = &tm1memory;
		tm1 = &tm0memory;
		Gu::TSwap(body0, body1);
		Gu::TSwap(shape0, shape1);
		Gu::TSwap(g0, g1);
		Gu::TSwap(body0Dynamic, body1Dynamic);
	}

	PX_ASSERT(tm0->isSane() && tm1->isSane());

	//PX_ASSERT(g0<=g1);
#if PX_ENABLE_SIM_STATS
	context.discreteContactPairs[g0][g1]++;
#endif
	PxcContactMethod conMethod =  g_PCMContactMethodTable[g0][g1];

	PX_ASSERT(conMethod);

	startContacts(n, context);

	
#if CACHE_LOCAL_CONTACTS_XP
	bool useContactCache = context.mContactCache && g_CanUsePCMContactCache[g0][g1];
	if(useContactCache)
	{
		PxcCacheLocalContacts(g0, g1, context, n.pairCache, *tm0, *tm1, contactDistance, conMethod, shape0->geometry, shape1->geometry);
	}
	else
	{
		conMethod(shape0->geometry, shape1->geometry, *tm0, *tm1, contactDistance, n.pairCache, context.mContactBuffer);
	}
#else
	conMethod(shape0->geometry, shape1->geometry, *tm0, *tm1, contactDistance, n.pairCache, context.mContactBuffer);
#endif

	PxsMaterialInfo materialInfo[ContactBuffer::MAX_CONTACTS];

	PxcGetMaterialMethod materialMethod = g_GetMaterialMethodTable[g0][g1];

	PX_ASSERT(materialMethod);

	materialMethod(shape0, shape1, context,  materialInfo);

	if(flip)
		flipContacts(context, materialInfo);

	finishContacts(n, context, materialInfo);
}

#ifdef PX_CHECKED
#if SPU_NARROWPHASE

void noAvailableBlocksError(const PxU32 numAcquired, const PxU32 numRequested, const PxU32 sceneParam)
{
	if(numAcquired < numRequested)
	{
		//Need to print with several error messages because only the first 128 characters or so are printed to the tty.

		char error1a[256];
		sprintf(error1a, "Trying to reserve %d 16K memory blocks for spu narrow phase but only managed to reserve %d.", numRequested, numAcquired);

		switch(sceneParam)
		{
		case PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS:
			PX_WARN_ONCE(true, error1a);
			PX_WARN_ONCE(true, "Either increase PxSceneDesc::maxNbContactDataBlocks or reduce with PxPS3Config::setSceneParamInt(myScene,PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS)");
			PX_WARN_ONCE(true, "See PxPS3Config::getSpuMemBlockCounters() for more information");
			break;
		case PxPS3ConfigParam::eMEM_FRICTION_BLOCKS:
			PX_WARN_ONCE(true, error1a);
			PX_WARN_ONCE(true, "Either increase PxSceneDesc::maxNbContactDataBlocks or reduce with PxPS3Config::setSceneParamInt(myScene,PxPS3ConfigParam::eMEM_FRICTION_BLOCKS)");
			PX_WARN_ONCE(true, "See PxPS3Config::getSpuMemBlockCounters() for more information");
			break;
		case PxPS3ConfigParam::eMEM_CONTACT_STREAM_BLOCKS:
			PX_WARN_ONCE(true, error1a);
			PX_WARN_ONCE(true, "Either increase PxSceneDesc::maxNbContactDataBlocks or reduce with PxPS3Config::setSceneParamInt(myScene,PxPS3ConfigParam::eMEM_CONTACT_STREAM_BLOCKS)");
			PX_WARN_ONCE(true, "See PxPS3Config::getSpuMemBlockCounters() for more information");
			break;
		default:
			PX_ASSERT(false);
			break;
		}
	}
}

#endif
#endif

void physx::PxcRunNpBatch
(const PxU32 numSpusPrim, const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh, const PxU32 numSpusCnvxMesh,
				   PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
				   const PxU32 numContactBlocks, const PxU32 numNpCacheBlocksBlocks,
				   PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
				   PxcNpBatchEntry* entriesPrimOrCnvxVsCnvx, PxU32 numEntriesPrimOrCnvxVsCnvx,
				   PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
				   PxcNpBatchEntry* entriesPrimVsTrimesh, PxU32 numEntriesPrimVsTrimesh,
				   PxcNpBatchEntry* entriesCvxBoxVsTrimesh, PxU32 numEntriesCvxBoxVsTrimesh,
				   PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
				   PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
				   PxU32& touchLost, PxU32& touchFound,
				   physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool)
{	
	PX_UNUSED(taskPool);
	PX_UNUSED(continuation);
	PX_UNUSED(numNpCacheBlocksBlocks);
	PX_UNUSED(numContactBlocks);
	PX_UNUSED(memBlockPool);
	PX_UNUSED(numSpusCnvxMesh);
	PX_UNUSED(numSpusMesh);
	PX_UNUSED(numSpusHF);
	PX_UNUSED(numSpusCnvx);
	PX_UNUSED(numSpusPrim);

	touchLost = 0;
	touchFound = 0;

#if(SPU_NARROWPHASE) // also needed for non-PS3 platforms to compile

	PxsResourceManager* resourceManager = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager),128), PxsResourceManager)();
	//PxsResourceManager* resourceManagerFrictionPatches = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager),128), PxsResourceManager)();
	PxsResourceManager* resourceManagerCaches = NULL;

	PxU32 numAcquiredContactBlocks=0;
	PxU32 acquiredContactBlockStart=0;
	const PxcNpMemBlock* const* contactBlocks=memBlockPool.acquireMultipleContactBlocks(numContactBlocks, acquiredContactBlockStart, numAcquiredContactBlocks);
#ifdef PX_CHECKED
	noAvailableBlocksError(numAcquiredContactBlocks,numContactBlocks,PxPS3ConfigParam::eMEM_CONTACT_STREAM_BLOCKS);
#endif
	resourceManager->setMemBlocks(numAcquiredContactBlocks,contactBlocks);

#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	resourceManagerCaches = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager), 128), PxsResourceManager)();
	PxU32 numAcquiredCacheBlocks=0;
	PxU32 acquiredCacheBlockStart=0;
	const PxcNpMemBlock* const* cacheBlocks=memBlockPool.acquireMultipleCacheBlocks(numNpCacheBlocksBlocks, acquiredCacheBlockStart, numAcquiredCacheBlocks);
#ifdef PX_CHECKED
	noAvailableBlocksError(numAcquiredCacheBlocks,numNpCacheBlocksBlocks,PxPS3ConfigParam::eMEM_FRICTION_BLOCKS);
#endif
	resourceManagerCaches->setMemBlocks(numAcquiredCacheBlocks,cacheBlocks);
#endif //USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU

	//If we can't run on spu then give the 16K blocks back because we'll need them on ppu.
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	if(NULL==contactBlocks || NULL==cacheBlocks)
#else
	if(NULL==contactBlocks || NULL==frictionBlocks)
#endif
	{
		memBlockPool.releaseMultipleContactBlocks(acquiredContactBlockStart, acquiredContactBlockStart + numAcquiredContactBlocks);
		resourceManager->setMemBlocks(0,NULL);
		numAcquiredContactBlocks = 0;

#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
		memBlockPool.releaseMultipleCacheBlocks(acquiredCacheBlockStart, acquiredCacheBlockStart + numAcquiredCacheBlocks);
		resourceManagerCaches->setMemBlocks(0,NULL);
		numAcquiredCacheBlocks = 0;
#endif
	}

	NarrowPhaseEndTask* narrowPhaseEndTask = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(NarrowPhaseEndTask)), NarrowPhaseEndTask)(
			memBlockPool, acquiredContactBlockStart, 
			numAcquiredContactBlocks, acquiredCacheBlockStart, 
			numAcquiredCacheBlocks, resourceManager, resourceManagerCaches);

	narrowPhaseEndTask->setContinuation(continuation);

	if(numSpusPrim > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseTask = createNPTask(taskPool, SPU_ELF_NP_TASK, "NpSpu");
		runNpBatchSPU(context,entriesPrimVsPrim,numEntriesPrimVsPrim,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound, *narrowPhaseTask, narrowPhaseEndTask, numSpusPrim,
			resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpBatchPPU(context,entriesPrimVsPrim,0,numEntriesPrimVsPrim,changeBitmapBase,changeBitmapWordCount,touchLost,touchFound);
	}

	if(numSpusCnvx > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseCnvxTask = createNPTask(taskPool, SPU_ELF_NPCNVX_TASK, "NPCnvxSpu");
		runNpBatchSPU(context,entriesPrimOrCnvxVsCnvx,numEntriesPrimOrCnvxVsCnvx,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound, *narrowPhaseCnvxTask, narrowPhaseEndTask, numSpusCnvx,
			resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpBatchPPU(context,entriesPrimOrCnvxVsCnvx,0,numEntriesPrimOrCnvxVsCnvx,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}
	if(numSpusHF > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseHFTask = gUnifiedHeightfieldCollision ? createNPTask(taskPool, SPU_ELF_UNIFIED_HF, "NpHFUFSpu") : createNPTask(taskPool, SPU_ELF_NPHF_TASK, "NpHFSpu");
		runNpBatchSPU(context,entriesPrimOrCnvxVsHF,numEntriesPrimOrCnvxVsHF,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound, *narrowPhaseHFTask, narrowPhaseEndTask, numSpusHF,
			resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpBatchPPU(context,entriesPrimOrCnvxVsHF,0,numEntriesPrimOrCnvxVsHF,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}
	if(1 && numSpusMesh > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseMeshTask = createNPTask(taskPool, SPU_ELF_NPMESH_TASK, "NpMeshSpu");
		if (numEntriesPrimVsTrimesh)
			runNpBatchSPU(context,entriesPrimVsTrimesh,numEntriesPrimVsTrimesh,changeBitmapBase,changeBitmapWordCount,
				touchLost,touchFound, *narrowPhaseMeshTask, narrowPhaseEndTask, numSpusMesh,
				resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpBatchPPU(context,entriesPrimVsTrimesh,0,numEntriesPrimVsTrimesh,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}
	if(1 && numSpusCnvxMesh > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseCnvxMeshTask = createNPTask(taskPool, SPU_ELF_NPCNVXMESH_TASK, "NpCnvxMeshSpu");
		if (numEntriesCvxBoxVsTrimesh)
			runNpBatchSPU(context,entriesCvxBoxVsTrimesh,numEntriesCvxBoxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
				touchLost,touchFound, *narrowPhaseCnvxMeshTask, narrowPhaseEndTask, numSpusCnvxMesh,
				resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpBatchPPU(context,entriesCvxBoxVsTrimesh,0,numEntriesCvxBoxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}
	runNpBatchPPU(context,entriesOther,0,numEntriesOther,changeBitmapBase,changeBitmapWordCount,touchLost,touchFound);

	// remove reference, the task will not be executed 
	// until all narrow phase tasks have completed
	narrowPhaseEndTask->removeReference();

#else

	runNpBatchPPU(context, entriesPrimVsPrim,		0, numEntriesPrimVsPrim,		changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpBatchPPU(context, entriesPrimOrCnvxVsCnvx,	0, numEntriesPrimOrCnvxVsCnvx,	changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpBatchPPU(context, entriesPrimOrCnvxVsHF,	0, numEntriesPrimOrCnvxVsHF,	changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpBatchPPU(context, entriesPrimVsTrimesh,	0, numEntriesPrimVsTrimesh,		changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpBatchPPU(context, entriesCvxBoxVsTrimesh,	0, numEntriesCvxBoxVsTrimesh,	changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpBatchPPU(context, entriesOther,			0, numEntriesOther,				changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);

#endif
}

//void physx::PxcRunNpPCMBatch(	const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh, const PxU32 numSpusCnvxMesh,
//								PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
//								const PxU32 numContactBlocks, const PxU32 numNpCacheBlocks,
//								PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
//								PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
//								PxcNpBatchEntry* entriesPrimVsTrimesh, PxU32 numEntriesPrimVsTrimesh,
//								PxcNpBatchEntry* entriesCvxBoxVsTrimesh, PxU32 numEntriesCvxBoxVsTrimesh,
//								PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
//								PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
//								PxU32& touchLost, PxU32& touchFound,
//								physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool)
void physx::PxcRunNpPCMBatch(	const PxU32 numSpusCnvx, const PxU32 numSpusHF, const PxU32 numSpusMesh,
								PxsThreadContext* context, PxcNpMemBlockPool& memBlockPool,
								const PxU32 numContactBlocks, const PxU32 numNpCacheBlocks,
								PxcNpBatchEntry* entriesPrimVsPrim, PxU32 numEntriesPrimVsPrim,
								PxcNpBatchEntry* entriesPrimOrCnvxVsHF, PxU32 numEntriesPrimOrCnvxVsHF,
								PxcNpBatchEntry* entriesPrimOrCnvxVsTrimesh, PxU32 numEntriesPrimOrCnvxVsTrimesh,
								PxcNpBatchEntry* entriesOther, PxU32 numEntriesOther,
								PxU32* changeBitmapBase, PxU32 changeBitmapWordCount,
								PxU32& touchLost, PxU32& touchFound,
								physx::PxLightCpuTask* continuation, Cm::FlushPool& taskPool)
{	
	PX_UNUSED(taskPool);
	PX_UNUSED(continuation);
	PX_UNUSED(numNpCacheBlocks);
	PX_UNUSED(numContactBlocks);
	PX_UNUSED(memBlockPool);
	PX_UNUSED(numSpusCnvx);
	PX_UNUSED(numSpusMesh);
	PX_UNUSED(numSpusHF);

	touchLost = 0;
	touchFound = 0;

#if(SPU_NARROWPHASE) // also needed for non-PS3 platforms to compile
	
	PxsResourceManager* resourceManager = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager), 128), PxsResourceManager)();
	//PxsResourceManager* resourceManagerFrictionPatches = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager), 128), PxsResourceManager)();

	PxU32 numAcquiredContactBlocks=0;
	PxU32 acquiredContactBlockStart=0;
	const PxcNpMemBlock* const* contactBlocks=memBlockPool.acquireMultipleContactBlocks(numContactBlocks, acquiredContactBlockStart, numAcquiredContactBlocks);
	resourceManager->setMemBlocks(numAcquiredContactBlocks,contactBlocks);

	//PxU32 numAcquiredFrictionBlocks=0;
	//PxU32 acquiredFrictionBlockStart=0;
	//const PxcNpMemBlock* const* frictionBlocks=memBlockPool.acquireMultipleFrictionBlocks(numFrictionBlocks, acquiredFrictionBlockStart, numAcquiredFrictionBlocks);
	//resourceManagerFrictionPatches->setMemBlocks(numAcquiredFrictionBlocks, frictionBlocks);

	PxsResourceManager* resourceManagerCaches = NULL;
#if USE_SEPARATE_CACHE_RESOURCE_MANAGER_ON_SPU
	resourceManagerCaches = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(PxsResourceManager), 128), PxsResourceManager)();
	PxU32 numAcquiredCacheBlocks=0;
	PxU32 acquiredCacheBlockStart=0;
	const PxcNpMemBlock* const* cacheBlocks=memBlockPool.acquireMultipleCacheBlocks(numNpCacheBlocks, acquiredCacheBlockStart, numAcquiredCacheBlocks);
	resourceManagerCaches->setMemBlocks(numAcquiredCacheBlocks,cacheBlocks);
#endif

	NarrowPhaseEndTask* narrowPhaseEndTask = PX_PLACEMENT_NEW(taskPool.allocate(sizeof(NarrowPhaseEndTask)), NarrowPhaseEndTask)(
		memBlockPool, acquiredContactBlockStart, 
		numAcquiredContactBlocks, acquiredCacheBlockStart, 
		numAcquiredCacheBlocks, resourceManager, resourceManagerCaches);

	narrowPhaseEndTask->setContinuation(continuation);
	
	if(numSpusCnvx > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseTask = createNPTask(taskPool, SPU_ELF_NP_PCM_TASK, "NpPCMSpu");
		runNpBatchSPU(context,entriesPrimVsPrim,numEntriesPrimVsPrim,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound, *narrowPhaseTask, narrowPhaseEndTask, numSpusCnvx,
			resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpPCMBatchPPU(context,entriesPrimVsPrim,0,numEntriesPrimVsPrim,changeBitmapBase,changeBitmapWordCount,touchLost,touchFound);
	}

	if(numSpusHF > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseHFTask = createNPTask(taskPool, SPU_ELF_NPHEIGHTFIELD_PCM_TASK, "NpHFPCMSpu");
		runNpBatchSPU(context,entriesPrimOrCnvxVsHF,numEntriesPrimOrCnvxVsHF,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound, *narrowPhaseHFTask, narrowPhaseEndTask, numSpusHF,
			resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpPCMBatchPPU(context,entriesPrimOrCnvxVsHF,0,numEntriesPrimOrCnvxVsHF,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}

	if(numSpusMesh > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseMeshTask = createNPTask(taskPool, SPU_ELF_NPMESH_PCM_TASK, "NpMeshPCMSpu");
		if (numEntriesPrimOrCnvxVsTrimesh)
			runNpBatchSPU(context,entriesPrimOrCnvxVsTrimesh, numEntriesPrimOrCnvxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
				touchLost,touchFound, *narrowPhaseMeshTask, narrowPhaseEndTask, numSpusMesh,
				resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpPCMBatchPPU(context,entriesPrimOrCnvxVsTrimesh,0,numEntriesPrimOrCnvxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}
	/*if(numSpusCnvxMesh > 0 && changeBitmapWordCount <= CellNarrowPhaseSPUOutput::eMAX_NUM_CHANGE_BITMAP_WORDS)
	{
		NarrowPhaseSpuTask* narrowPhaseCnvxMeshTask = createNPTask(taskPool, SPU_ELF_NPMESH_PCM_TASK, "NpCnvxMeshSpu");
		if (numEntriesCvxBoxVsTrimesh)
			runNpBatchSPU(context,entriesCvxBoxVsTrimesh,numEntriesCvxBoxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
				touchLost,touchFound, *narrowPhaseCnvxMeshTask, narrowPhaseEndTask, numSpusCnvxMesh,
				resourceManager, resourceManagerCaches);
	}
	else
	{
		runNpPCMBatchPPU(context,entriesCvxBoxVsTrimesh,0,numEntriesCvxBoxVsTrimesh,changeBitmapBase,changeBitmapWordCount,
			touchLost,touchFound);
	}*/
	runNpPCMBatchPPU(context,entriesOther,0,numEntriesOther,changeBitmapBase,changeBitmapWordCount,touchLost,touchFound);

	// remove reference, the task will not be executed 
	// until all narrow phase tasks have completed
	narrowPhaseEndTask->removeReference();

#else

	runNpPCMBatchPPU(context, entriesPrimVsPrim,			0, numEntriesPrimVsPrim,			changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpPCMBatchPPU(context, entriesPrimOrCnvxVsHF,		0, numEntriesPrimOrCnvxVsHF,		changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpPCMBatchPPU(context, entriesPrimOrCnvxVsTrimesh,	0, numEntriesPrimOrCnvxVsTrimesh,	changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	//runNpPCMBatchPPU(context, entriesCvxBoxVsTrimesh,	0, numEntriesCvxBoxVsTrimesh,	changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);
	runNpPCMBatchPPU(context, entriesOther,					0, numEntriesOther,					changeBitmapBase, changeBitmapWordCount, touchLost, touchFound);

#endif
}
