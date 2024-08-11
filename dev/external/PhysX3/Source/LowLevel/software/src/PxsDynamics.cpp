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


#include "PsTime.h"
#include "PsAtomic.h"
#include "PxvDynamics.h"

#include "PxsContext.h"
#include "PsFastMemory.h"
#include "PxsRigidBody.h"
#include "PxsContactManager.h"
#include "PxsContext.h"
#include "PxsDynamics.h"
#include "PxsBodyCoreIntegrator.h"
#include "PxsSolverCore.h"
#include "PxsSolverCoreGeneral.h"
#include "PxcSolverContact.h"
#include "PxsSolverContact.h"
#include "PxsSolverConstraintDesc.h"
#include "PxsIslandManager.h"
#include "PxsSolverConstraintExt.h"
#include "PxsSolverBody.h"

#include "PxsConstraint.h"
#include "PxsConstraintPartition.h"
#include "PxsArticulation.h"

#include "CmFlushPool.h"
#include "PxsSolverCoreSIMD.h"
#include "PxcArticulationPImpl.h"
#include "PxCpuDispatcher.h"
#include "PxsMaterialManager.h"
#include "PxcSolverContactPF4.h"
#include "PxsContactReduction.h"

#define PXS_BATCH_1D 1

#ifdef PX_PS3
#define SPU_DYNAMICS 1
#define SPU_ATOM_INTEGRATION		SPU_DYNAMICS
#define SPU_CONSTRAINT_SHADER		SPU_DYNAMICS
#define SPU_CONSTRAINT_PARTITIONING SPU_DYNAMICS
#define SPU_SOLVER					SPU_DYNAMICS
#define SPU_SOLVER_WRITEBACK		SPU_DYNAMICS
#define SPU_INTEGRATE_CORE			SPU_DYNAMICS

#define FORCE_SINGLE_SPU 0
#else
#define SPU_DYNAMICS 0
#define PX_PARALLEL_CONSTRAINT_PROCESSOR	PX_CONSTRAINT_PARTITIONING
#endif

#ifdef PX_PS3
#include "PxSpuTask.h"
void noAvailableBlocksError(const physx::PxU32 numAcquired, const physx::PxU32 numRequested, const physx::PxU32 sceneParam);
#if !SPU_CONSTRAINT_PREPARATION
#include "PxsSolverExt.h"
#endif
#endif

#if SPU_DYNAMICS
#include "CellAtomIntegratorTask.h"
#include "CellConstraintShaderTask.h"
#include "CellConstraintPartitionerTask.h"
#include "CellSolverTask.h"
#include "CellIntegrateCoreTask.h"
#include "PS3Support.h"
#include "PxsResourceManager.h"
using namespace physx;
#endif


//Enable tuner profiling.
#ifdef PX_PS3
#include "CellTimerMarker.h"
#endif

//Enable timers embedded in spu code.
#ifdef PX_PS3
#include "CellSPUProfiling.h"
#define SPU_PROFILE_ATOM_INTEGRATE 0
#define SPU_PROFILE_CONSTRAINT_SHADER 0
#define SPU_PROFILE_CONSTRAINT_PREP 0
#define SPU_PROFILE_CONSTRAINT_PRTN 0
#define SPU_PROFILE_SOLVER 0
#define SPU_PROFILE_INTEGRATE_CORE 0
#if (SPU_PROFILE_ATOM_INTEGRATE + SPU_PROFILE_CONSTRAINT_SHADER + SPU_PROFILE_CONSTRAINT_PREP + SPU_PROFILE_CONSTRAINT_PRTN + SPU_PROFILE_SOLVER + SPU_PROFILE_INTEGRATE_CORE)
PxU64 PX_ALIGN(16, gProfileCounters[MAX_NUM_SPU_PROFILE_ZONES])={0};
#endif
#endif

#if !SPU_DYNAMICS
#include "PxsSolverExt.h"
#endif
  
using namespace physx;

// this function makes sure the dynamics solver articulation functions are registered for non-SPU. It's called
// from PxvRegisterArticulations



PxsDynamicsContext* PxsDynamicsContext::create(PxsContext* context)
{
	PxsDynamicsContext* dc = reinterpret_cast<PxsDynamicsContext*>(
		PX_ALLOC(sizeof(PxsDynamicsContext), PX_DEBUG_EXP("PxsDynamicsContext")));

	if(dc)
	{
		new(dc) PxsDynamicsContext(context);
	}
	return dc;
}


void PxsDynamicsContext::destroy()
{
	this->~PxsDynamicsContext();
	PX_FREE(this);
}


// =========================== Basic methods

PxsDynamicsContext::PxsDynamicsContext(PxsContext* context) : 
	mContext					(context),
	mDt							(1.0f), 
	mInvDt						(1.0f),
	mBounceThreshold			(-2.0f),
	mSolverBatchSize			(32)
	//mMergeTask					(this, "PxsDynamicsContext::mergeResults")
{
	mWorldSolverBodyData.solverBody = &mWorldSolverBody;
	mWorldSolverBody.linearVelocity = PxVec3(0);
	mWorldSolverBody.angularVelocity = PxVec3(0);
	//mWorldSolverBodyData.motionLinearVelocity = PxVec3(0);
	//mWorldSolverBodyData.motionAngularVelocity = PxVec3(0);
	mWorldSolverBodyData.invMass = 0;
	mWorldSolverBodyData.invInertia = PxMat33(PxZero);
	mWorldSolverBodyData.originalBody = NULL;
	mWorldSolverBodyData.reportThreshold = PX_MAX_REAL;
	mWorldSolverBody.solverProgress=MAX_PERMITTED_SOLVER_PROGRESS;
	mWorldSolverBody.maxSolverNormalProgress=MAX_PERMITTED_SOLVER_PROGRESS;
	mWorldSolverBody.maxSolverFrictionProgress=MAX_PERMITTED_SOLVER_PROGRESS;
	mSolverCore = PxsSolverCoreGeneral::create();
}

PxsDynamicsContext::~PxsDynamicsContext()
{
	mSolverCore->destroyV();
}

// =========================== Solve methods!

void PxsDynamicsContext::setDescFromIndices(PxcSolverConstraintDesc& desc, const PxsIndexedInteraction& constraint)
{
	if(constraint.indexType0 == PxsIndexedInteraction::eARTICULATION)
	{
		PxsArticulation* a = getArticulation(constraint.articulation0);
		desc.articulationA = a->getFsDataPtr();
		desc.articulationALength = Ps::to16(a->getTotalDataSize());
		PX_ASSERT(0==(desc.articulationALength & 0x0f));
		desc.linkIndexA = Ps::to16(a->getLinkIndex(constraint.articulation0));
	}
	else
	{
		desc.linkIndexA = PxcSolverConstraintDesc::NO_LINK;
		//desc.articulationALength = 0; //this is unioned with bodyADataIndex
		desc.bodyA = constraint.indexType0 == PxsIndexedInteraction::eWORLD ? &mWorldSolverBody
																			: &mSolverBodyPool[(PxU32)constraint.solverBody0];
		desc.bodyADataIndex = constraint.indexType0 == PxsIndexedInteraction::eWORLD ? 0
																			: (PxU16)constraint.solverBody0+1;
	}

	if(constraint.indexType1 == PxsIndexedInteraction::eARTICULATION)
	{
		PxsArticulation* a = getArticulation(constraint.articulation1);
		desc.articulationB = a->getFsDataPtr();
		desc.articulationBLength = Ps::to16(a->getTotalDataSize());
		PX_ASSERT(0==(desc.articulationBLength & 0x0f));
		desc.linkIndexB = Ps::to16(a->getLinkIndex(constraint.articulation1));
	}
	else
	{
		desc.linkIndexB = PxcSolverConstraintDesc::NO_LINK;
		//desc.articulationBLength = 0; //this is unioned with bodyBDataIndex
		desc.bodyB = constraint.indexType1 == PxsIndexedInteraction::eWORLD ? &mWorldSolverBody
																			: &mSolverBodyPool[(PxU32)constraint.solverBody1];
		desc.bodyBDataIndex = constraint.indexType1 == PxsIndexedInteraction::eWORLD ? 0
																			: (PxU16)constraint.solverBody1 + 1;
	}
}

void _SetupConstraintsParallel(const PxF32 invDt, const PxF32 dt, const PxU32 _numConstraints, PxsIndexedConstraint* constraints, PxcConstraintBlockStream& blockStream,
							   PxcSolverConstraintDesc* descArray, PxcSolverBodyData* bodyDataArray, PxI32* pIndex, PxI32* pSetupCounter, PxI32* pAxisCount,
							   PxsConstraintBlockManager& constraintBlockManager)
{
	//PIX_PROFILE_ZONE(SetupConstraintsParallel);
	const PxI32 unrollCount = 8;
	PxI32 index = physx::atomicAdd(pIndex, unrollCount) - unrollCount;
	PxI32 numSetup = 0;
	PxI32 axisCount = 0;
	PxI32 numConstraints = (PxI32)_numConstraints;
	while(index < numConstraints)
	{
		PxI32 remainder = PxMin(numConstraints - index, unrollCount);
		for(PxI32 a = 0; a < remainder; ++a)
		{
			axisCount += PxsSetupSolverConstraint(constraints[index + a].constraint, bodyDataArray, blockStream, dt, invDt, descArray[index + a], constraintBlockManager);
			numSetup++;
		}
		index = physx::atomicAdd(pIndex, unrollCount) - unrollCount;
	}

	physx::atomicAdd(pAxisCount, axisCount);
	physx::atomicAdd(pSetupCounter, numSetup);
}

class PxsAtomIntegrateTask : public PxLightCpuTask
{
	PxsAtomIntegrateTask& operator=(const PxsAtomIntegrateTask&);
public:
	PxsAtomIntegrateTask(	PxsDynamicsContext&			context,
							PxsBodyCore*const*			bodyArray,
							PxsRigidBody*const*			originalBodyArray,
							PxcSolverBody*				solverBodies,
							PxcSolverBodyData*			solverBodyDataPool,
							Cm::SpatialVector*			motionVelocityArray,
							const Cm::SpatialVector*	accelerationArray,
							PxF32						dt,
							PxU32						numBodies,
							PxI32*						pCounter,
							volatile PxI32*				pNumIntegrated,
							volatile PxU32*				maxSolverPositionIterations,
							volatile PxU32*				maxSolverVelocityIterations) :
		mContext(context),
		mBodyArray(bodyArray),
		mOriginalBodyArray(originalBodyArray),
		mSolverBodies(solverBodies),
		mSolverBodyDataPool(solverBodyDataPool),
		mMotionVelocityArray(motionVelocityArray),
		mAccelerationArray(accelerationArray),
		mDt(dt),
		mNumBodies(numBodies),
		m_pCounter(pCounter),
		m_pNumIntegrated(pNumIntegrated),
		mMaxSolverPositionIterations(maxSolverPositionIterations),
		mMaxSolverVelocityIterations(maxSolverVelocityIterations)
	{}

	virtual void run();

	virtual const char* getName() const
	{
		return "PxsIntegrateTask";
	}

public:
	PxsDynamicsContext&			mContext;
	PxsBodyCore*const*			mBodyArray;
	PxsRigidBody*const*			mOriginalBodyArray;
	PxcSolverBody*				mSolverBodies;
	PxcSolverBodyData*			mSolverBodyDataPool;
	Cm::SpatialVector*			mMotionVelocityArray;
	const Cm::SpatialVector*	mAccelerationArray;
	PxF32						mDt;
	PxU32						mNumBodies;
	PxI32*						m_pCounter;
	volatile PxI32*				m_pNumIntegrated;
	volatile PxU32*				mMaxSolverPositionIterations;
	volatile PxU32*				mMaxSolverVelocityIterations;
};


class PxsSetupConstraintTask : public PxLightCpuTask
{
public:
	PxsSetupConstraintTask( const PxF32 invDt, const PxF32 dt, const PxU32 numConstraints,PxsIndexedConstraint* constraints, PxcNpMemBlockPool& blockPool,
		PxcSolverConstraintDesc* descArray, PxcSolverBodyData* solverBodyData, PxI32* pIndex, PxI32* pSetupCounter, PxI32* pAxisCount, PxsConstraintBlockManager& constraintBlockManager ) :
	mInvDt(invDt), mDt(dt), mNumConstraints(numConstraints), mConstraints(constraints), mBlockStream(blockPool), 
		mDescArray(descArray), mSolverBodyData(solverBodyData), m_pIndex(pIndex), m_pSetupCounter(pSetupCounter), m_pAxisCount(pAxisCount),
		mConstraintBlockManager(constraintBlockManager)
	{}

	virtual void run()
	{
		_SetupConstraintsParallel(mInvDt, mDt, mNumConstraints, mConstraints, mBlockStream,
			mDescArray, mSolverBodyData, m_pIndex, m_pSetupCounter, m_pAxisCount, mConstraintBlockManager);
	}

	virtual const char* getName() const
	{
		return "PxsSetupConstraintTask";
	}

public:
	const PxF32 mInvDt;
	const PxF32 mDt;
	const PxU32 mNumConstraints;
	PxsIndexedConstraint*		mConstraints;
	PxcConstraintBlockStream mBlockStream;
	PxcSolverConstraintDesc* mDescArray;
	PxcSolverBodyData* mSolverBodyData;
	PxI32* m_pIndex; 
	PxI32* m_pSetupCounter;
	PxI32* m_pAxisCount;
	PxsConstraintBlockManager& mConstraintBlockManager;
};

class PxsParallelSolverTask : public PxLightCpuTask
{
	PxsParallelSolverTask& operator=(PxsParallelSolverTask&);
public:

		PxsParallelSolverTask(PxsDynamicsContext& context, PxsFrictionModel frictionType, PxU32 maxSolverPositionIterations, PxU32 maxSolverVelocityIterations, PxcSolverBody* pSolverBodyPool, PxcSolverBodyData* pSolverBodyDataPool,
		PxU32 solverBodyOffset, PxU32 bodyCount, PxcSolverConstraintDesc* PX_RESTRICT pContactDescArray, PxcSolverConstraintDesc* PX_RESTRICT pFrictionDescArray, PxcArticulationSolverDesc* PX_RESTRICT	pArticulationDescArray,
		PxU32 contactDescArraySize, PxU32 frictionDescArraySize, PxU32 articulationDescArraySize, PxI32* pConstraintIteration, PxI32* pFrictionIteration, PxI32* pAtomIteration, PxI32* pAtomIntegrationIteration, PxI32* pThresholdPairsOut,
		PxcThresholdStreamElement* PX_RESTRICT pThresholdStream, PxU32 contactManagers, Cm::SpatialVector* PX_RESTRICT pMotionVelocityArray,
		PxsBodyCore*const* PX_RESTRICT pBodyArray, Cm::BitMap& localChangedShapes, PxsArticulation*const* PX_RESTRICT pArticulations, PxU32 numArtics,
		volatile PxI32* pRunningThreads, Ps::Array<PxsConstraintBatchHeader>& _contactBlocks, Ps::Array<PxsConstraintBatchHeader>& _frictionBlocks)
		: contactBlocks(_contactBlocks)
		, frictionBlocks(_frictionBlocks)
		, mContext(context)
		, mMaxSolverPositionIterations(maxSolverPositionIterations)
		, mMaxSolverVelocityIterations(maxSolverVelocityIterations)
		, m_pSolverBodyPool(pSolverBodyPool)
		, m_pSolverBodyDataPool(pSolverBodyDataPool)
		, mSolverBodyOffset(solverBodyOffset)
		, m_BodyCount(bodyCount)
		, m_pContactDescArray(pContactDescArray)
		, m_pFrictionDescArray(pFrictionDescArray)
		, m_pArticulationDescArray(pArticulationDescArray)
		, m_pMotionVelocityArray(pMotionVelocityArray)
		, m_pBodyArray(pBodyArray)
		, m_pArticulations(pArticulations)
		, m_NumArtics(numArtics)
		, m_ContactDescArraySize(contactDescArraySize)
		, m_FrictionDescArraySize(frictionDescArraySize)
		, m_ArticulationDescArraySize(articulationDescArraySize)
		, m_LocalChangedShapes(localChangedShapes)
		, m_pConstraintIteration(pConstraintIteration)
		, m_pFrictionIteration(pFrictionIteration)
		, m_pAtomIteration(pAtomIteration)
		, m_pThresholdPairsOut(pThresholdPairsOut)
		, m_pAtomIntegrationIteration(pAtomIntegrationIteration)
		, m_pRunningThreads(pRunningThreads)
		, m_pThresholdStream(pThresholdStream)
		, m_ContactManagers(contactManagers)
		, m_FrictionType(frictionType)
	{
	}

	virtual void run()
	{
		CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::SimTask::GetPxsParallelSolve());
		{
			solveParallel[m_FrictionType](mContext, mMaxSolverPositionIterations, mMaxSolverVelocityIterations, m_pSolverBodyPool, m_pSolverBodyDataPool, mSolverBodyOffset, m_BodyCount, m_pArticulationDescArray, m_ArticulationDescArraySize,
				m_pContactDescArray,m_ContactDescArraySize, m_pFrictionDescArray, m_FrictionDescArraySize, m_pConstraintIteration,  m_pFrictionIteration, m_pAtomIteration, m_pAtomIntegrationIteration, m_pThresholdStream, m_ContactManagers, m_pThresholdPairsOut,
				m_pMotionVelocityArray, m_pBodyArray, m_LocalChangedShapes, m_pArticulations, m_NumArtics, m_pRunningThreads, contactBlocks, frictionBlocks);
		}
	}

	virtual const char* getName() const
	{
		return "PxsParallelSolverTask";
	}

	Ps::Array<PxsConstraintBatchHeader>&	contactBlocks;
	Ps::Array<PxsConstraintBatchHeader>&	frictionBlocks;
	PxsDynamicsContext&						mContext;
	PxU32									mMaxSolverPositionIterations;
	PxU32									mMaxSolverVelocityIterations;
	PxcSolverBody* PX_RESTRICT				m_pSolverBodyPool;
	PxcSolverBodyData* PX_RESTRICT			m_pSolverBodyDataPool;
	PxU32									mSolverBodyOffset;
	PxU32									m_BodyCount;

	PxcSolverConstraintDesc* PX_RESTRICT	m_pContactDescArray;
	PxcSolverConstraintDesc* PX_RESTRICT	m_pFrictionDescArray;
	PxcArticulationSolverDesc* PX_RESTRICT	m_pArticulationDescArray;
	Cm::SpatialVector* PX_RESTRICT			m_pMotionVelocityArray;
	PxsBodyCore*const* PX_RESTRICT			m_pBodyArray;
	PxsArticulation*const* PX_RESTRICT		m_pArticulations;
	PxU32									m_NumArtics;
	PxU32									m_ContactDescArraySize;
	PxU32									m_FrictionDescArraySize;
	PxU32									m_ArticulationDescArraySize;

	Cm::BitMap&								m_LocalChangedShapes;

	PxI32*									m_pConstraintIteration;
	PxI32*									m_pFrictionIteration;
	PxI32*									m_pAtomIteration;
	PxI32*									m_pThresholdPairsOut;
	PxI32*									m_pAtomIntegrationIteration;
	volatile PxI32*							m_pRunningThreads;
	PxcThresholdStreamElement* PX_RESTRICT	m_pThresholdStream;
	PxU32									m_ContactManagers;
	PxsFrictionModel						m_FrictionType;

};

void atomIntegration(
					 const PxF32 dt,
					 PxsBodyCore*const* bodyArray,					// INOUT: core body attributes
					 PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
					 PxU32 bodyCount,									// IN: body count
					 const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
					 PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
					 PxcSolverBodyData* solverBodyPoolData,				// IN: solver atom pool (space preallocated)
					 Cm::SpatialVector* motionVelocityArray,			// OUT: motion velocities
					 PxU32& maxSolverPositionIterations,
					 PxU32& maxSolverVelocityIterations
					 );


namespace physx
{

#if SPU_ATOM_INTEGRATION

class PxsAtomIntegrationSpuTask : public PxSpuTask
{
public:

	PxsAtomIntegrationSpuTask(
		const PxU32 numSpusForAtomIntegration,
		const PxF32 dt,
		PxsBodyCore*const* bodyCoreArray,				// INOUT: core body attributes
		PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
		PxU32 bodyCount,								// IN: body count
		const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
		PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
		PxcSolverBodyData* solverBodyData,				// IN: inertia/invMass values for solver body
		Cm::SpatialVector* motionVelocityArray,			// OUT: motion velocities
		PxU32& maxSolverPositionIterations,
		PxU32& maxSolverVelocityIterations) : 
		PxSpuTask(gPS3GetElfImage(SPU_ELF_ATOM_INTEGRATOR_TASK), gPS3GetElfSize(SPU_ELF_ATOM_INTEGRATOR_TASK), numSpusForAtomIntegration),
		mMaxSolverPositionIterations(maxSolverPositionIterations),
		mMaxSolverVelocityIterations(maxSolverVelocityIterations)
	{
		//Prepare the input data that will be passed to all the spus.
		mCellAtomIntegratorSPUInput.mTotalTaskCount = numSpusForAtomIntegration;
		mCellAtomIntegratorSPUInput.mActiveTaskCount = numSpusForAtomIntegration;
		mCellAtomIntegratorSPUInput.mBodyCorePtrArray=const_cast<PxsBodyCore**>(bodyCoreArray);
		mCellAtomIntegratorSPUInput.mOriginalBodyPtrArray=(PxcRigidBody**)const_cast<PxsRigidBody**>(originalBodyArray);
		mCellAtomIntegratorSPUInput.mSolverBodyArray=solverBodyPool;
		mCellAtomIntegratorSPUInput.mAccelArray=const_cast<Cm::SpatialVector*>(accelerationArray);
		mCellAtomIntegratorSPUInput.mNumBodyCores=bodyCount;
		mCellAtomIntegratorSPUInput.mSolverBodyDataArray = solverBodyData;
		mCellAtomIntegratorSPUInput.mDt=dt;
#if SPU_PROFILE_ATOM_INTEGRATE
		mCellAtomIntegratorSPUInput.mProfileZones=gProfileCounters;
#endif

		//Prepare the output data for each spu
		//(each spu will count the number of dynamic/static objects and compute the maximum
		//solver iterations of the bodies handled by that spu).
		for(PxU32 uiTask=0; uiTask< getSpuCount(); uiTask++)
		{
			mCellAtomIntegratorSPUOutput[uiTask].maxSolverVelocityIterations=0;
			mCellAtomIntegratorSPUOutput[uiTask].maxSolverPositionIterations=0;
		}

		//Start all the spu tasks.
		for (PxU32 uiTask=0; uiTask < getSpuCount(); uiTask++) 
		{
			setArgs(uiTask, uiTask | (unsigned int)&mCellAtomIntegratorSPUOutput[uiTask], (unsigned int)&mCellAtomIntegratorSPUInput);
		}
	}

	virtual void release()
	{
		//Now add up all the kinematic and dynamic body counts stored in the spu output data array.
		//Work out the maximum solver iterations too.
	/*	mMaxSolverVelocityIterations=0;
		mMaxSolverPositionIterations=0;*/

		for (PxU32 uiTask=0; uiTask < getSpuCount(); uiTask++)
		{
			mMaxSolverVelocityIterations=PxMax<PxU32>(mCellAtomIntegratorSPUOutput[uiTask].maxSolverVelocityIterations,mMaxSolverVelocityIterations);
			mMaxSolverPositionIterations=PxMax<PxU32>(mCellAtomIntegratorSPUOutput[uiTask].maxSolverPositionIterations,mMaxSolverPositionIterations);
		}

		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsAtomIntegrationSpuTask"; }

	PxU32& mMaxSolverPositionIterations;
	PxU32& mMaxSolverVelocityIterations;

	CellAtomIntegratorSPUInput	PX_ALIGN(128, mCellAtomIntegratorSPUInput);				
	CellAtomIntegratorSPUOutput	PX_ALIGN(128, mCellAtomIntegratorSPUOutput[6]);		
};

#endif //SPU_ATOM_INTEGRATION

#if SPU_CONSTRAINT_PARTITIONING

class PxsConstraintPartitionSpuTask : public PxSpuTask
{
public:

	PxsConstraintPartitionSpuTask(
		PxcSolverBody* solverBodyPool, const PxU32 bodyCount,
		PxcArticulationSolverDesc* articulations, const PxU32 articulationCount,
		PxcSolverConstraintDesc* descArray, PxU32 descCount,
		Ps::Array<PxcSolverConstraintDesc>& orderedConstraints,
		PxU32& numDifferentBodyConstraints, 
		PxU32& numSelfConstraints,
		PxcFsSelfConstraintBlock* selfConstraintBlocks,
		PxU32& numSelfConstraintBlocks,
		PxU32& maxPartitions,
		PxcSolverBody* kinematicBodyStart,
		const PxU32 kinematicBodyCount,
		bool* successfulSpuConstraintPartition,
		PxU32* successfulSpuConstraintPartitionCount,
		const PxU32 targetSuccessfulSpuConstraintPartitionCount,
		PxU32* numConstraintsPerPartition,
		PxsThreadContext& threadContext) :
		PxSpuTask(gPS3GetElfImage(SPU_ELF_CONSTRAINT_PARTITIONER_TASK), gPS3GetElfSize(SPU_ELF_CONSTRAINT_PARTITIONER_TASK), 1),
		mNumDifferentBodyConstraints(numDifferentBodyConstraints),
		mNumSelfConstraints(numSelfConstraints),
		mNumSelfConstraintBlocks(numSelfConstraintBlocks),
		mMaxPartitions(maxPartitions),
		mSuccessfulSpuConstraintPartition(successfulSpuConstraintPartition),
		mSuccessfulSpuConstraintPartitionCount(successfulSpuConstraintPartitionCount),
		mTargetSuccessfulSpuConstraintPartitionCount(targetSuccessfulSpuConstraintPartitionCount),
		mThreadContext(threadContext)
	{
		mCellConstraintPartitionerSPUInput.mAtoms = solverBodyPool;
		mCellConstraintPartitionerSPUInput.mNumAtoms = bodyCount;
		mCellConstraintPartitionerSPUInput.mArticulationPtrs = articulations;
		mCellConstraintPartitionerSPUInput.mNumArticulationPtrs = articulationCount;
		mCellConstraintPartitionerSPUInput.mConstraintDescriptors = descArray;
		mCellConstraintPartitionerSPUInput.mNumConstraintDescriptors = descCount;
		mCellConstraintPartitionerSPUInput.mActiveTaskCount = 1;
		mCellConstraintPartitionerSPUInput.mTotalTaskCount = 1;
		mCellConstraintPartitionerSPUInput.mKinematicBodyStart = kinematicBodyStart;
		mCellConstraintPartitionerSPUInput.mKinematicBodyCount = kinematicBodyCount;
		mCellConstraintPartitionerSPUInput.mNumConstraintsPerPartition = numConstraintsPerPartition;
	#if SPU_PROFILE_CONSTRAINT_PREP
		mCellConstraintPartitionerSPUInput.mProfileZones = gProfileCounters;
	#endif

		mCellConstraintPartitionerSPUOutput[0].mEaOrderedConstraints = orderedConstraints.begin();
		mCellConstraintPartitionerSPUOutput[0].mEaSelfConstraintBlocks = selfConstraintBlocks;

		//Start spu work.
		for(PxU32 i=0;i<1;i++)
		{
			setArgs(i, (unsigned int)&mCellConstraintPartitionerSPUOutput[0], (unsigned int)&mCellConstraintPartitionerSPUInput);
		}
	}

	virtual void release()
	{
		if(CellConstraintPartitionerSPUOutput::eSUCCESS_STATE_SUCCESS==mCellConstraintPartitionerSPUOutput[0].mSuccessState)
		{
			mNumDifferentBodyConstraints=mCellConstraintPartitionerSPUOutput[0].mNumDifferentBodyConstraints;
			mNumSelfConstraints=mCellConstraintPartitionerSPUOutput[0].mNumSelfConstraints;
			mNumSelfConstraintBlocks=mCellConstraintPartitionerSPUOutput[0].mNumSelfConstraintBlocks;
			mMaxPartitions = mCellConstraintPartitionerSPUOutput[0].mMaxPartitions;
			(*mSuccessfulSpuConstraintPartitionCount)++;
		}
		else
		{
			ConstraintPartitionArgs args;
			args.mAtoms = mCellConstraintPartitionerSPUInput.mAtoms;
			args.mArticulationPtrs = mCellConstraintPartitionerSPUInput.mArticulationPtrs;
			args.mContactConstraintDescriptors = mCellConstraintPartitionerSPUInput.mConstraintDescriptors;
			args.mNumArticulationPtrs = mCellConstraintPartitionerSPUInput.mNumArticulationPtrs;
			args.mNumAtoms = mCellConstraintPartitionerSPUInput.mNumAtoms;
			args.mNumContactConstraintDescriptors = mCellConstraintPartitionerSPUInput.mNumConstraintDescriptors;
			args.mOrderedContactConstraintDescriptors = mCellConstraintPartitionerSPUOutput[0].mEaOrderedConstraints;

			args.mNumDifferentBodyConstraints = args.mNumSelfConstraints = args.mNumSelfConstraintBlocks = 0;

			args.mConstraintsPerPartition = &mThreadContext.mConstraintsPerPartition[0];
			args.mFrictionConstraintsPerPartition = &mThreadContext.mFrictionConstraintsPerPartition[0];

			mThreadContext.mMaxPartitions = PartitionContactConstraints(args);
			mThreadContext.mNumDifferentBodyConstraints = args.mNumDifferentBodyConstraints;
			mThreadContext.mNumSelfConstraints = args.mNumSelfConstraints;
			mThreadContext.mNumSelfConstraintBlocks = args.mNumSelfConstraintBlocks;
		}
		*mSuccessfulSpuConstraintPartition = true;//(*mSuccessfulSpuConstraintPartitionCount == mTargetSuccessfulSpuConstraintPartitionCount);
		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsConstraintPartitionSpuTask"; }

	CellConstraintPartitionerSPUInput	PX_ALIGN(128, mCellConstraintPartitionerSPUInput);
	CellConstraintPartitionerSPUOutput	PX_ALIGN(128, mCellConstraintPartitionerSPUOutput[6]);

	PxU32& mNumDifferentBodyConstraints;
	PxU32& mNumSelfConstraints;
	PxU32& mNumSelfConstraintBlocks;
	PxU32& mMaxPartitions;

	bool* mSuccessfulSpuConstraintPartition;
	PxU32* mSuccessfulSpuConstraintPartitionCount;
	PxU32 mTargetSuccessfulSpuConstraintPartitionCount;
	PxsThreadContext& mThreadContext;
};

#endif // SPU_CONSTRAINT_PARTITIONING

#define PX_CONTACT_REDUCTION 1

class PxsSolverConstraintPostProcessTask : public PxLightCpuTask
{
	PxsSolverConstraintPostProcessTask& operator=(const PxsSolverConstraintPostProcessTask&);
public:

	PxsSolverConstraintPostProcessTask(PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset,
		PxU32 startIndex,
		PxU32 stride) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset),
		mStartIndex(startIndex),
		mStride(stride)
	{}

	void mergeContacts(PxsCompoundContactManager& header, PxsThreadContext& threadContext)
	{
		Gu::ContactBuffer& buffer = threadContext.mContactBuffer;
		PxsMaterialInfo materialInfo[Gu::ContactBuffer::MAX_CONTACTS];
		PxU32 size = 0;

		bool hasForces = false;
		for(PxU32 a = 0; a < header.mStride; ++a)
		{
			PxsContactManager* manager = mThreadContext.orderedContactList[a+header.mStartIndex]->contactManager;
			PxcNpWorkUnit& unit = manager->getWorkUnit();
			PxContactStreamIterator iter(unit.compressedContacts, unit.compressedContactSize);

			PxU32 origSize = size;
			PX_UNUSED(origSize);
			if(!iter.forceNoResponse)
			{
				while(iter.hasNextPatch())
				{
					iter.nextPatch();
					while(iter.hasNextContact())
					{
						PX_ASSERT(size < Gu::ContactBuffer::MAX_CONTACTS);
						iter.nextContact();
						PxsMaterialInfo& info = materialInfo[size];
						Gu::ContactPoint& point = buffer.contacts[size++];
						point.dynamicFriction = iter.getDynamicFriction();
						point.staticFriction = iter.getStaticFriction();
						point.restitution = iter.getRestitution();
						point.internalFaceIndex0 = iter.getFaceIndex0();
						point.internalFaceIndex1 = iter.getFaceIndex1();
						point.maxImpulse = iter.getMaxImpulse();
						point.targetVel = iter.getTargetVel();
						point.normal = iter.getContactNormal();
						point.point = iter.getContactPoint();
						point.separation = iter.getSeparation();
						info.mMaterialIndex0 = iter.getMaterialIndex0();
						info.mMaterialIndex1 = iter.getMaterialIndex1();
					}
				}
				hasForces = hasForces || unit.contactForces != NULL;
			}
			PX_ASSERT(unit.contactCount == (size - origSize));
		}

		PxU32 origSize = size;
#if PX_CONTACT_REDUCTION
		PxsContactReduction<3> reduction(buffer.contacts, materialInfo, size);
		reduction.reduceContacts();
		//OK, now we write back the contacts...

		PxU8 histo[Gu::ContactBuffer::MAX_CONTACTS];
		PxMemZero(histo, sizeof(histo));

		//Cm::RenderOutput output = this->mContext.getContext()->getRenderOutput();
		size = 0;
		for(PxU32 a = 0; a < reduction.mNumPatches; ++a)
		{
			PxsReducedContactPatch& patch = reduction.mPatches[a];
			for(PxU32 b = 0; b < patch.numContactPoints; ++b)
			{
				histo[patch.contactPoints[b]] = 1;
				++size;
			}
		}
#endif

#ifdef __SPU__
		uintptr_t eaForceBuffer = 0;
		PxU16* PX_RESTRICT data = gMemBlock.reserve(size * sizeof(PxU16), &eaForceBuffer);
		header.forceBufferList = (PxU8* PX_RESTRICT)eaForceBuffer;
#else
		PxU16* PX_RESTRICT data = (PxU16*)threadContext.mConstraintBlockStream.reserve(size * sizeof(PxU16), mThreadContext.mConstraintBlockManager);
		header.forceBufferList = data;
#endif

#if PX_CONTACT_REDUCTION
		size = 0;
		for(PxU32 a = 0; a < origSize; ++a)
		{
			if(histo[a])
			{
				if(size != a)
					buffer.contacts[size] = buffer.contacts[a];
				data[size] = Ps::to16(a);
				size++;
			}
		}
#else
		for(PxU32 a = 0; a < size; ++a)
			data[a] = a;
#endif


		PxU32 contactForceByteSize = size * sizeof(PxReal);

		if(!size || !hasForces)
		{
			contactForceByteSize = 0;
		}

		header.unit->contactCount = Ps::to16(size);
		physx::writeCompressedContact(buffer.contacts, size, threadContext, *header.unit, contactForceByteSize, false, materialInfo, true, &mThreadContext);
	}

	virtual void run()
	{
		PxU32 endIndex = mStartIndex + mStride;

		PxsThreadContext* threadContext = this->mContext.getContext()->getThreadContext();
		threadContext->mContactBlockStream.reset();
		threadContext->mConstraintBlockStream.reset();

		for(PxU32 a = mStartIndex; a < endIndex; ++a)
		{
			mergeContacts(mThreadContext.compoundConstraints[a], *threadContext);
		}
		mContext.getContext()->putThreadContext(threadContext);
	}

	virtual const char* getName() const { return "PxsSolverConstraintPostProcessTask"; }


	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	PxU32						mSolverBodyOffset;
	PxU32						mStartIndex;
	PxU32						mStride;
};


class PxsSolverStartTask : public PxLightCpuTask
{
	PxsSolverStartTask& operator=(const PxsSolverStartTask&);
public:

	PxsSolverStartTask(PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset)
	{}

	void startTasks()
	{
		CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::SimTask::GetPxsDynamicsSolver());
		{
			mThreadContext.mMaxSolverPositionIterations = 0;
			mThreadContext.mMaxSolverVelocityIterations = 0;
			mThreadContext.mAxisConstraintCount = 0;
			mThreadContext.mContactDescPtr = mThreadContext.contactConstraintDescArray.begin();
			mThreadContext.mFrictionDescPtr = mThreadContext.frictionConstraintDescArray.begin();
			mThreadContext.mNumDifferentBodyConstraints = 0;
			mThreadContext.mNumSelfConstraintBlocks = 0;
			mThreadContext.mNumSelfConstraints = 0;
			mThreadContext.mNumDifferentBodyFrictionConstraints = 0;
			mThreadContext.mNumSelfConstraintFrictionBlocks = 0;
			mThreadContext.mNumSelfFrictionConstraints = 0;


	#if SPU_DYNAMICS
	#if (SPU_PROFILE_ATOM_INTEGRATE + SPU_PROFILE_CONSTRAINT_PREP + SPU_PROFILE_CONSTRAINT_PRTN + SPU_PROFILE_SOLVER + SPU_PROFILE_INTEGRATE_CORE)
			for(PxU32 i=0; i<MAX_NUM_SPU_PROFILE_ZONES; i++) 
			{
				gProfileCounters[i]=0;
			}
	#endif
	#endif

	#ifdef PX_PS3
			const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
	#else
			const PxU32 numSpusToUse = 0;
	#endif
			PX_UNUSED(numSpusToUse);

			Cm::BitMap& localChangedShapes = mThreadContext.getLocalChangedShapes();
			PX_UNUSED(localChangedShapes);

			CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext.getContext(),Sim,solveGroup);

			// Reserve array space for bodies.

			PxU32 numArticulationConstraints=0;
			for(PxU32 i=0;i<mCounts.articulations; i++)
			{
				PxsArticulation &a = *mObjects.articulations[i];
				numArticulationConstraints+=a.getBodyCount();
			}


			const PxU32 frictionConstraintCount = mThreadContext.mFrictionType == physx::PXS_STICKY_FRICTION ? 0 : mCounts.contactManagers;
			mThreadContext.resizeArrays(mCounts.bodies, mCounts.contactManagers, mCounts.constraints + mCounts.contactManagers + numArticulationConstraints, frictionConstraintCount, mCounts.articulations);

			PxsBodyCore** PX_RESTRICT bodyArrayPtr = mThreadContext.bodyCoreArray.begin();
			Cm::SpatialVector* PX_RESTRICT accelerationArrayPtr = mThreadContext.accelerationArray.begin();

			for(PxU32 i=0; i < mCounts.bodies; i++)
			{
				const PxU32 prefetch = PxMin(i+8, (PxU32)mCounts.bodies - 1);
				Ps::prefetchLine(mObjects.bodies[prefetch]);
				Ps::prefetchLine(mObjects.bodies[prefetch],128);
				Ps::prefetchLine(&accelerationArrayPtr[i], 128);
				Ps::prefetchLine(&bodyArrayPtr[i], 128);			
				Ps::prefetchLine(&mObjects.bodies[i], 128);
				bodyArrayPtr[i] = &mObjects.bodies[i]->getCore();
				accelerationArrayPtr[i] = mObjects.bodies[i]->getAccelerationV();
			}
		}
	}

	void atomIntegrate()
	{
#ifdef PX_PS3
		startTimerMarker(eATOM_INTEGRATION);
#endif

		PxcSolverBody *solverBodies = mContext.mSolverBodyPool.begin() + mSolverBodyOffset;
		PxcSolverBodyData *solverBodyData = mContext.mSolverBodyDataPool.begin() + mSolverBodyOffset;

#if SPU_ATOM_INTEGRATION

		//Work out how many spus we'll be using for atom integration.
		//Make sure that at least one body is handled by each spu - there's no point in wasting spus with empty tasks.
#if FORCE_SINGLE_SPU
		PX_ASSERT(g_iPhysXSPUCount>=1);
		const PxU32 numSpusForAtomIntegration = 1;
#else
		const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
		const PxU32 numSpusForAtomIntegration = mCounts.bodies < numSpusToUse ? 1 : numSpusToUse ;
#endif

		if(numSpusForAtomIntegration>0)
		{
			CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext.getContext(),Sim,updateVelocities);

			void* taskMem = mContext.getContext()->getTaskPool().allocate(sizeof(PxsAtomIntegrationSpuTask), 128);
			PxsAtomIntegrationSpuTask* spuTask = PX_PLACEMENT_NEW(taskMem, PxsAtomIntegrationSpuTask)(
				numSpusForAtomIntegration,
				mContext.mDt,
				mThreadContext.bodyCoreArray.begin(),		
				mObjects.bodies,
				mCounts.bodies,
				mThreadContext.accelerationArray.begin(),	
				solverBodies,
				solverBodyData,
				mThreadContext.motionVelocityArray.begin(),
				mThreadContext.mMaxSolverPositionIterations,
				mThreadContext.mMaxSolverVelocityIterations);

			spuTask->setContinuation(mCont);
			spuTask->removeReference();
		}
		else
#endif
		{
			CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext.getContext(),Sim,updateVelocities);

#if PX_PARALLEL_CONSTRAINT_PROCESSOR

			mContext.atomIntegrationParallel(	
				mContext.mDt,
				mThreadContext.bodyCoreArray.begin(),
				mObjects.bodies,
				mCounts.bodies,
				mThreadContext.accelerationArray.begin(),
				solverBodies,
				solverBodyData,
				mThreadContext.motionVelocityArray.begin(),
				mThreadContext.mMaxSolverPositionIterations,
				mThreadContext.mMaxSolverVelocityIterations,
				*mCont
				);
#else
			
			atomIntegration(	
				mContext.mDt,
				mThreadContext.bodyCoreArray.begin(),
				mObjects.bodies,
				mCounts.bodies,
				mThreadContext.accelerationArray.begin(),
				solverBodies,
				solverBodyData,
				mThreadContext.motionVelocityArray.begin(),
				mThreadContext.mMaxSolverPositionIterations,
				mThreadContext.mMaxSolverVelocityIterations
				);
#endif
		}
	}

	void articulationTask()
	{
		PxcArticulationSolverDesc* articulationDescArray = mThreadContext.getArticulations().begin();

		mThreadContext.mConstraintBlockStream.reset(); //Clear in case there's some left-over memory in this context, for which the block has already been freed 
		PxU32 maxVelIters = 0;
		PxU32 maxPosIters = 0;
		PxU32 maxArticulationLength = 0;
		for(PxU32 i=0;i<mCounts.articulations; i++)
		{
			PxsArticulation& a = *mObjects.articulations[i];
			a.getSolverDesc(articulationDescArray[i]);

			PxU32 acCount, descCount;
			
			descCount = PxcArticulationPImpl::computeUnconstrainedVelocities(articulationDescArray[i], mContext.mDt, mThreadContext.mConstraintBlockStream, 
				mThreadContext.mContactDescPtr, acCount, mContext.getContext()->getEventProfiler(), mContext.getContext()->getScratchAllocator(), 
				mThreadContext.mConstraintBlockManager);

			mThreadContext.mContactDescPtr += descCount;
			mThreadContext.mAxisConstraintCount += acCount;
			maxArticulationLength = PxMax(maxArticulationLength, (PxU32)articulationDescArray[i].totalDataSize);

			const PxU16 iterWord = a.getIterationCounts();
			maxVelIters = PxMax<PxU32>(iterWord >> 8,	maxVelIters);
			maxPosIters = PxMax<PxU32>(iterWord & 0xff, maxPosIters);
		}
		Ps::atomicMax((PxI32*)&mThreadContext.mMaxSolverPositionIterations, maxPosIters);
		Ps::atomicMax((PxI32*)&mThreadContext.mMaxSolverVelocityIterations, maxVelIters);
		mThreadContext.mMaxArticulationLength = maxArticulationLength;
	}

	void setupDescTask()
	{
		PxcSolverConstraintDesc* contactDescPtr = mThreadContext.mContactDescPtr;

		//PxU32 constraintCount = mCounts.constraints + mCounts.contactManagers;
		for(PxU32 i = 0; i< mCounts.constraints; i++)
		{
			PxcSolverConstraintDesc& desc = *contactDescPtr;
			mContext.setDescFromIndices(desc, mObjects.constraints[i]);
			desc.constraint = (PxU8*)mObjects.constraints[i].constraint; //a PxConstraint constraint
			desc.constraintLengthOver16 = PXS_SC_TYPE_RB_1D;
			contactDescPtr++;
		}

		mThreadContext.orderedContactList.forceSize_Unsafe(0);
		mThreadContext.orderedContactList.reserve(mCounts.contactManagers);
		mThreadContext.orderedContactList.forceSize_Unsafe(mCounts.contactManagers);
		mThreadContext.tempContactList.forceSize_Unsafe(0);
		mThreadContext.tempContactList.reserve(mCounts.contactManagers);
		mThreadContext.tempContactList.forceSize_Unsafe(mCounts.contactManagers);

		const PxsIndexedContactManager** constraints = mThreadContext.orderedContactList.begin();



#if 0
		for(PxU32 i = 0; i < mCounts.contactManagers; ++i)
		{
			constraints[i] = mObjects.contactManagers + i;
		}

		struct SortConstraintPredicate
		{
			PxU32 mBodies;
			SortConstraintPredicate(const PxU32 numBodies) : mBodies(numBodies)
			{
			}

			bool operator()(const PxsIndexedContactManager*& idx1, const PxsIndexedContactManager*& idx2) const
			{
				/*PxU32 ind00 = idx1->indexType0 == PxsIndexedInteraction::eARTICULATION ? 
					mBodies + idx1->articulation0 : idx1->solverBody0;
				PxU32 ind01 = idx1->indexType1 == PxsIndexedInteraction::eARTICULATION ? 
					mBodies + idx1->articulation1 : idx1->solverBody1;

				PxU32 ind10 = idx2->indexType0 == PxsIndexedInteraction::eARTICULATION ? 
					mBodies + idx2->articulation0 : idx2->solverBody0;
				PxU32 ind11 = idx2->indexType1 == PxsIndexedInteraction::eARTICULATION ? 
					mBodies + idx2->articulation1 : idx2->solverBody1;*/

				PxU32 ind00 = idx1->solverBody0;
				PxU32 ind01 = idx1->solverBody1;

				PxU32 ind10 = idx2->solverBody0;
				PxU32 ind11 = idx2->solverBody1;

				return ind00 < ind10 || (ind00 == ind10 && ind01 < ind11);
			}
		};
#endif

		//OK, we sort the orderedContactList 

		mThreadContext.compoundConstraints.forceSize_Unsafe(0);
		if(mCounts.contactManagers)
		{
#if 0
			SortConstraintPredicate pred(mCounts.bodies);

			Ps::sort<const physx::PxsIndexedContactManager*, SortConstraintPredicate>(constraints, mCounts.contactManagers, pred);
#else
			{
				mThreadContext.sortIndexArray.forceSize_Unsafe(0);

				const PxU32 solverBodyOffset = mSolverBodyOffset;
				const PxU32 solverBodyOffsetAdjusted = mSolverBodyOffset - mContext.mKinematicCount;

				const PxU32 totalBodies = mContext.mKinematicCount + mCounts.bodies+1;

				mThreadContext.sortIndexArray.reserve(totalBodies);
				mThreadContext.sortIndexArray.forceSize_Unsafe(totalBodies);
				PxMemZero(mThreadContext.sortIndexArray.begin(), totalBodies * 4);

				//Iterate over the array based on solverBodyDatapool, creating a list of sorted constraints (in order of body pair)
				//We only do this with contacts. It's important that this is done this way because we don't want to break our rules that all joints
				//appear before all contacts in the constraint list otherwise we will lose all guarantees about sorting joints.
				
				for(PxU32 a = 0; a < mCounts.contactManagers; ++a)
				{
					//Index first body...
					if(mObjects.contactManagers[a].indexType1 != PxsIndexedInteraction::eARTICULATION)
					{
						PxU32 index = (PxU32)mObjects.contactManagers[a].solverBody1 + 1;
						if(index > solverBodyOffset)
							index -= solverBodyOffsetAdjusted;					

						mThreadContext.sortIndexArray[index]++;
					}
				}

				PxU32 accumulatedCount = 0;
				for(PxU32 a = 0; a < mThreadContext.sortIndexArray.size(); ++a)
				{
					PxU32 val = mThreadContext.sortIndexArray[a];
					mThreadContext.sortIndexArray[a] = accumulatedCount;
					accumulatedCount += val;
				}

				//OK, now copy across data to orderedConstraintDescs, pushing articulations to the end...
				for(PxU32 a = 0; a < mCounts.contactManagers; ++a)
				{
					//Index first body...
					if(mObjects.contactManagers[a].indexType1 != PxsIndexedInteraction::eARTICULATION)
					{
						PxU32 index = (PxU32)mObjects.contactManagers[a].solverBody1 + 1;
						if(index > solverBodyOffset)
							index -= solverBodyOffsetAdjusted;	
						mThreadContext.tempContactList[mThreadContext.sortIndexArray[index]++] = &mObjects.contactManagers[a];
					}
					else
					{
						mThreadContext.tempContactList[accumulatedCount++] = &mObjects.contactManagers[a];
					}
				}

				//Now do the same again with bodyB, being careful not to overwrite the joints
				PxMemZero(mThreadContext.sortIndexArray.begin(), totalBodies * 4);


				for(PxU32 a = 0; a < mCounts.contactManagers; ++a)
				{
					PX_ASSERT(mThreadContext.tempContactList[a]->indexType0 != PxsIndexedInteraction::eWORLD);
					//Index first body...
					if(mThreadContext.tempContactList[a]->indexType0 != PxsIndexedInteraction::eARTICULATION)
					{
						PxU32 index = (PxU32)mThreadContext.tempContactList[a]->solverBody0 + 1;
						if(index > solverBodyOffset)
							index -= solverBodyOffsetAdjusted;
						mThreadContext.sortIndexArray[index]++;
					}
				}

				accumulatedCount = 0;
				for(PxU32 a = 0; a < mThreadContext.sortIndexArray.size(); ++a)
				{
					PxU32 val = mThreadContext.sortIndexArray[a];
					mThreadContext.sortIndexArray[a] = accumulatedCount;
					accumulatedCount += val;
				}

				//OK, now copy across data to orderedConstraintDescs, pushing articulations to the end...
				for(PxU32 a = 0; a < mCounts.contactManagers; ++a)
				{
					//Index first body...
					if(mThreadContext.tempContactList[a]->indexType0 != PxsIndexedInteraction::eARTICULATION)
					{
						PxU32 index = (PxU32)mThreadContext.tempContactList[a]->solverBody0 + 1;
						if(index > solverBodyOffset)
							index -= solverBodyOffsetAdjusted;
						constraints[mThreadContext.sortIndexArray[index]++] = mThreadContext.tempContactList[a];
					}
					else
					{
						constraints[accumulatedCount++] = mThreadContext.tempContactList[a];
					}
				}

			}
#endif

			mThreadContext.mStartContactDescPtr = contactDescPtr;

			mThreadContext.compoundConstraints.resize(mCounts.contactManagers);
			mThreadContext.compoundConstraints.forceSize_Unsafe(mCounts.contactManagers);

			PxcSolverConstraintDesc* startDesc = contactDescPtr;
			mContext.setDescFromIndices(*startDesc, *constraints[0]);
			startDesc->constraint = (PxU8*)constraints[0]->contactManager;
			startDesc->constraintLengthOver16 = PXS_SC_TYPE_RB_CONTACT;

			PxU32 contactCount = constraints[0]->contactManager->getContactCount();
			PxU32 startIndex = 0;
			PxU32 numHeaders = 0;
			for(PxU32 a = 1; a < mCounts.contactManagers; ++a)
			{
				PxcSolverConstraintDesc& desc = *(contactDescPtr+1);
				mContext.setDescFromIndices(desc, *constraints[a]);

				PxsContactManager* manager = (PxsContactManager*)constraints[a]->contactManager;

				desc.constraint = (PxU8*)constraints[a]->contactManager;
				desc.constraintLengthOver16 = PXS_SC_TYPE_RB_CONTACT;
				
				if(startDesc->bodyA != desc.bodyA || startDesc->bodyB != desc.bodyB 
					|| startDesc->linkIndexA != PxcSolverConstraintDesc::NO_LINK || startDesc->linkIndexB != PxcSolverConstraintDesc::NO_LINK
					|| contactCount + manager->getContactCount() > Gu::ContactBuffer::MAX_CONTACTS)
				{
					PxU32 stride = a - startIndex;
					if(contactCount > 0)
					{
						if(stride > 1)
						{
							PxsCompoundContactManager& header = mThreadContext.compoundConstraints[numHeaders++];
							header.mStartIndex = startIndex;
							header.mStride = Ps::to16(stride);	
							header.mReducedContactCount = Ps::to16(contactCount);
							PxsContactManager* manager = constraints[startIndex]->contactManager;
							PxcNpWorkUnit& unit = manager->getWorkUnit();
							header.unit = &unit;
							header.originalContactBuffer = unit.compressedContacts;
							header.originalContactBufferSize = unit.compressedContactSize;
							header.originalContactCount = unit.contactCount;
							header.originalForceBuffer = unit.contactForces;
						}
						startDesc = ++contactDescPtr;
					}
					else
					{
						//Copy back next contactDescPtr
						*startDesc = *(contactDescPtr+1);
					}
					contactCount = 0;
					startIndex = a;
				}
				contactCount += manager->getContactCount();
			}
			PxU32 stride = mCounts.contactManagers - startIndex;
			if(contactCount > 0)
			{
				if(stride > 1)
				{
					PxsCompoundContactManager& header = mThreadContext.compoundConstraints[numHeaders++];
					header.mStartIndex = startIndex;
					header.mStride = Ps::to16(stride);
					header.mReducedContactCount = Ps::to16(contactCount);
					PxsContactManager* manager = constraints[startIndex]->contactManager;
					PxcNpWorkUnit& unit = manager->getWorkUnit();
					header.unit = &unit;
					header.originalContactBuffer = unit.compressedContacts;
					header.originalContactBufferSize = unit.compressedContactSize;
					header.originalContactCount = unit.contactCount;
					header.originalForceBuffer = unit.contactForces;
				}
				contactDescPtr++;
			}

			mThreadContext.compoundConstraints.forceSize_Unsafe(numHeaders);

			if(numHeaders)
			{
				const PxU32 unrollSize = 8;
				for(PxU32 a = 0; a < numHeaders; a+= unrollSize)
				{
					PxsSolverConstraintPostProcessTask* postProcessTask = PX_PLACEMENT_NEW( mContext.getContext()->getTaskPool().allocate(sizeof(PxsSolverConstraintPostProcessTask)), 
						PxsSolverConstraintPostProcessTask)(mContext, mThreadContext, mObjects, mCounts, mSolverBodyOffset, a, PxMin(unrollSize, numHeaders - a));
					postProcessTask->setContinuation(mCont);
					postProcessTask->removeReference();
				}

			/*	PxsSolverConstraintPostProcessTask* postProcessTask = PX_PLACEMENT_NEW( mContext.getContext()->getTaskPool().allocate(sizeof(PxsSolverConstraintPostProcessTask)), 
						PxsSolverConstraintPostProcessTask)(mContext, mThreadContext, mObjects, mCounts, mSolverBodyOffset, 0, unrollSize);
				postProcessTask->setContinuation(mCont);
				postProcessTask->removeReference();*/
			}
		}

		mThreadContext.contactConstraintDescArray.forceSize_Unsafe(PxU32(contactDescPtr - mThreadContext.contactConstraintDescArray.begin()));
		mThreadContext.orderedContactConstraints.forceSize_Unsafe(PxU32(contactDescPtr - mThreadContext.contactConstraintDescArray.begin()));
	}

	virtual void run()
	{
		startTasks();
		articulationTask();
		atomIntegrate();
		setupDescTask();
	}

	virtual const char* getName() const
	{
		return "PxsSolverStartTask";
	}

private:
	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	const PxU32					mSolverBodyOffset;
};

class PxsSolverConstraintPartitionTask : public PxLightCpuTask
{
	PxsSolverConstraintPartitionTask& operator=(const PxsSolverConstraintPartitionTask&);
public:

	PxsSolverConstraintPartitionTask(PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset)
	{}

	virtual void run()
	{
#ifdef PX_PS3
		stopTimerMarker(eATOM_INTEGRATION);
		startTimerMarker(eCONSTRAINT_PARTITION);
#endif

		//PIX_PROFILE_ZONE(PxsSolverConstraintPartitoining);
		PxcSolverConstraintDesc* descBegin = mThreadContext.contactConstraintDescArray.begin();
		PxcSolverConstraintDesc* descPtr = mThreadContext.mContactDescPtr;
		PX_UNUSED(descPtr);
		//PxU32 descCount = mCounts.constraints + mCounts.contactManagers + (descPtr - descBegin);
		PxU32 descCount = mThreadContext.contactConstraintDescArray.size();
		PX_UNUSED(descCount);

		PxcSolverConstraintDesc* frictionDescBegin = mThreadContext.frictionConstraintDescArray.begin();
		PxcSolverConstraintDesc* frictionDescPtr = mThreadContext.mFrictionDescPtr;
		PxU32 frictionDescCount = PxU32(frictionDescPtr-frictionDescBegin);
		PX_UNUSED(frictionDescCount);

		PxcSolverBody* solverBodies = mContext.mSolverBodyPool.begin() + mSolverBodyOffset;
		PxcSolverBody* kinematicBodies = mContext.mSolverBodyPool.begin();
		PxsIslandManager& im = mContext.getContext()->getIslandManager();
		PxU32 kinematicCount = im.getActiveKinematicCount();

		PX_UNUSED(solverBodies);
		PX_UNUSED(kinematicBodies);
		PX_UNUSED(kinematicCount);

		mThreadContext.mNumDifferentBodyConstraints = descCount;

#if SPU_DYNAMICS
#if SPU_CONSTRAINT_PARTITIONING

#ifdef PX_PS3
		const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
#else
		const PxU32 numSpusToUse = 0;
#endif
		mThreadContext.orderedContactConstraints.forceSize_Unsafe(0);
		mThreadContext.orderedContactConstraints.reserve(descCount + 3);
		mThreadContext.orderedContactConstraints.forceSize_Unsafe(descCount + 3);

		mThreadContext.mSelfConstraintBlocks=mSelfConstraintBlocks;
		mThreadContext.mSelfConstraintFrictionBlocks = mSelfConstraintFrictionBlocks;

		mThreadContext.mNumDifferentBodyFrictionConstraints = 0;
		mThreadContext.mNumSelfFrictionConstraints = 0;
		mThreadContext.mNumSelfConstraintFrictionBlocks = 0;

		if(numSpusToUse>0 && descCount>0 && 
			(mCounts.bodies + mThreadContext.getArticulations().size() < CellConstraintPartitionerSPUInput::eMAX_NUM_ATOMS) && 
			(mThreadContext.getArticulations().size() < MAX_NUM_SPU_ARTICULATIONS) &&
			descCount < MAX_NUM_SPU_CONSTRAINTS)
		{
			mThreadContext.mSuccessfulSpuConstraintPartition = false;
			mThreadContext.mSuccessfulSpuConstraintPartitionCount = 0;

			void* taskMem = mContext.getContext()->getTaskPool().allocate(sizeof(PxsConstraintPartitionSpuTask), 128);
			PxsConstraintPartitionSpuTask* spuTask = PX_PLACEMENT_NEW(taskMem, PxsConstraintPartitionSpuTask)(
				solverBodies, mCounts.bodies, 
				mThreadContext.getArticulations().begin(), mThreadContext.getArticulations().size(), 
				descBegin, descCount, mThreadContext.orderedContactConstraints,
				mThreadContext.mNumDifferentBodyConstraints, mThreadContext.mNumSelfConstraints,
				mThreadContext.mSelfConstraintBlocks, mThreadContext.mNumSelfConstraintBlocks,
				mThreadContext.mMaxPartitions, kinematicBodies, kinematicCount,
				&mThreadContext.mSuccessfulSpuConstraintPartition,
				&mThreadContext.mSuccessfulSpuConstraintPartitionCount,
				1, mThreadContext.mConstraintsPerPartition, mThreadContext);

			spuTask->setContinuation(mCont);
			spuTask->removeReference();
		}
		else
		{
			

			mThreadContext.mNumDifferentBodyConstraints = 0;
			mThreadContext.mNumSelfConstraints = 0;
			mThreadContext.mNumSelfConstraintBlocks = 0;
			mThreadContext.mNumDifferentBodyFrictionConstraints = 0;
			mThreadContext.mNumSelfConstraintFrictionBlocks = 0;
			mThreadContext.mNumSelfFrictionConstraints = 0;

			if(descCount > 0)
			{
				ConstraintPartitionArgs args;
				args.mAtoms = solverBodies;
				args.mArticulationPtrs = mThreadContext.getArticulations().begin();
				args.mContactConstraintDescriptors = descBegin;
				args.mFrictionConstraintDescriptors = frictionDescBegin;
				args.mNumArticulationPtrs = mThreadContext.getArticulations().size();
				args.mNumAtoms = mCounts.bodies;
				args.mNumContactConstraintDescriptors = descCount;
				args.mNumFrictionConstraintDescriptors = frictionDescCount;
				args.mOrderedContactConstraintDescriptors = mThreadContext.orderedContactConstraints.begin();
				//args.mOrderedFrictionConstraintDescriptors = mThreadContext.orderedFrictionConstraints.begin();
				args.mSelfConstraintBlocks = mThreadContext.mSelfConstraintBlocks;
				args.mNumDifferentBodyConstraints = args.mNumSelfConstraints = args.mNumSelfConstraintBlocks = 0;
				args.contactConstraintBatchHeader = &mThreadContext.contactConstraintBatchHeaders;
				args.frictionConstraintBatchHeader = &mThreadContext.frictionConstraintBatchHeaders;
				args.mConstraintsPerPartition = &mThreadContext.mConstraintsPerPartition[0];
				args.mFrictionConstraintsPerPartition = &mThreadContext.mFrictionConstraintsPerPartition[0];

				mThreadContext.mMaxPartitions = PartitionContactConstraints(args);
				mThreadContext.mNumDifferentBodyConstraints = args.mNumDifferentBodyConstraints;
				mThreadContext.mNumSelfConstraints = args.mNumSelfConstraints;
				mThreadContext.mNumSelfConstraintBlocks = args.mNumSelfConstraintBlocks;
			}
			else
			{
				PxMemZero(mThreadContext.mConstraintsPerPartition, sizeof(mThreadContext.mConstraintsPerPartition));
			}
		}

#endif //SPU_CONSTRAINT_PARTITIONING
#endif
#if PX_CONSTRAINT_PARTITIONING || (SPU_SOLVER && (!SPU_CONSTRAINT_PARTITIONING))

#if SPU_SOLVER
		Cm::FlushPool& taskPool = mContext.getContext()->getTaskPool();
		taskPool.lock();
		mThreadContext.mSelfConstraintBlocks = (PxcFsSelfConstraintBlock*)taskPool.allocateNotThreadSafe(sizeof(PxcFsSelfConstraintBlock) * MAX_NUM_SPU_ARTICULATIONS);
		mThreadContext.mSelfConstraintFrictionBlocks = (PxcFsSelfConstraintBlock*)taskPool.allocateNotThreadSafe(sizeof(PxcFsSelfConstraintBlock) * MAX_NUM_SPU_ARTICULATIONS);
		taskPool.unlock();
#endif

		

		mThreadContext.mNumDifferentBodyConstraints = 0;
		mThreadContext.mNumSelfConstraints = 0;
		mThreadContext.mNumSelfConstraintBlocks = 0;
		mThreadContext.mNumDifferentBodyFrictionConstraints = 0;
		mThreadContext.mNumSelfConstraintFrictionBlocks = 0;
		mThreadContext.mNumSelfFrictionConstraints = 0;

		

		if(descCount > 0)
		{
			ConstraintPartitionArgs args;
			args.mAtoms = solverBodies;
			args.mArticulationPtrs = mThreadContext.getArticulations().begin();
			args.mContactConstraintDescriptors = descBegin;
			args.mFrictionConstraintDescriptors = frictionDescBegin;
			args.mNumArticulationPtrs = mThreadContext.getArticulations().size();
			args.mNumAtoms = mCounts.bodies;
			args.mNumContactConstraintDescriptors = descCount;
			args.mNumFrictionConstraintDescriptors = frictionDescCount;
			args.mOrderedContactConstraintDescriptors = mThreadContext.orderedContactConstraints.begin();
			//args.mOrderedFrictionConstraintDescriptors = mThreadContext.orderedFrictionConstraints.begin();
			args.mSelfConstraintBlocks = mThreadContext.mSelfConstraintBlocks;
			args.mNumDifferentBodyConstraints = args.mNumSelfConstraints = args.mNumSelfConstraintBlocks = 0;
			args.contactConstraintBatchHeader = &mThreadContext.contactConstraintBatchHeaders;
			args.frictionConstraintBatchHeader = &mThreadContext.frictionConstraintBatchHeaders;
			args.mConstraintsPerPartition = &mThreadContext.mConstraintsPerPartition[0];
			args.mFrictionConstraintsPerPartition = &mThreadContext.mFrictionConstraintsPerPartition[0];

			mThreadContext.mMaxPartitions = PartitionContactConstraints(args);
			mThreadContext.mNumDifferentBodyConstraints = args.mNumDifferentBodyConstraints;
			mThreadContext.mNumSelfConstraints = args.mNumSelfConstraints;
			mThreadContext.mNumSelfConstraintBlocks = args.mNumSelfConstraintBlocks;

#if SPU_SOLVER
			mThreadContext.mSuccessfulSpuConstraintPartition = true;
#endif
		}
		else
		{
			PxMemZero(mThreadContext.mConstraintsPerPartition, sizeof(mThreadContext.mConstraintsPerPartition));
		}

		PX_ASSERT((mThreadContext.mNumDifferentBodyConstraints + mThreadContext.mNumSelfConstraints) == descCount);

#endif

	}

	virtual const char* getName() const { return "PxsSolverConstraintPartitionTask"; }

#ifdef PX_PS3
	PxcFsSelfConstraintBlock	PX_ALIGN(16,  mSelfConstraintBlocks[MAX_NUM_SPU_ARTICULATIONS]);
	PxcFsSelfConstraintBlock	PX_ALIGN(16,  mSelfConstraintFrictionBlocks[MAX_NUM_SPU_ARTICULATIONS]);
#endif

	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	PxU32						mSolverBodyOffset;
};



#if SPU_CONSTRAINT_SHADER

class PxsSetupConstraintSpuTask : public PxSpuTask
{
public:

	PxsSetupConstraintSpuTask(
		PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		PxcSolverConstraintDesc* constraintDescs,
		PxU32 numConstraintDescs,
		PxU32 numSelfConstraintDescs,
		PxU32 numSpus,
		PxsConstraintBatchHeader* headers,
		PxU32 numHeaders) :
		PxSpuTask(context.getContext()->getFrictionModel() == PXS_STICKY_FRICTION ? gPS3GetElfImage(SPU_ELF_CONSTRAINT_SHADER_TASK) : gPS3GetElfImage(SPU_ELF_CONSTRAINT_SHADER_COULOMB_TASK), 
		context.getContext()->getFrictionModel() == PXS_STICKY_FRICTION ? gPS3GetElfSize(SPU_ELF_CONSTRAINT_SHADER_TASK) : gPS3GetElfSize(SPU_ELF_CONSTRAINT_SHADER_COULOMB_TASK), numSpus),
		mContext(context), 
		mThreadContext(threadContext),
		mDescs(constraintDescs),
		mNumDescs(numConstraintDescs),
		mNumAcquiredBlocks(0),
		mBlockStart(0)		
	{
		//Grab memory that we can write to from spu.		
		const PxU32 numBlocks = PxMin((PxU32)PxsResourceManager::MAX_NUM_BLOCKS, context.getContext()->getSceneParamInt(PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS));
		const PxU32 numFrictionBlocks = context.getContext()->getSceneParamInt(PxPS3ConfigParam::eMEM_FRICTION_BLOCKS)/2;

		mResourceManager = PX_PLACEMENT_NEW(context.getContext()->getTaskPool().allocate(sizeof(PxsResourceManager),128), PxsResourceManager)();
		mFrictionResourceManager = PX_PLACEMENT_NEW(context.getContext()->getTaskPool().allocate(sizeof(PxsResourceManager),128), PxsResourceManager)();

		const PxcNpMemBlock* const* constraintShaderBlocks = mThreadContext.mConstraintBlockStream.getMemBlockPool().acquireMultipleConstraintBlocks(numBlocks, mBlockStart, mNumAcquiredBlocks);
#ifdef PX_CHECKED
		noAvailableBlocksError(mNumAcquiredBlocks,numBlocks,PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS);
#endif
		mResourceManager->setMemBlocks(mNumAcquiredBlocks,constraintShaderBlocks);

		const PxcNpMemBlock* const* frictionBlocks = mThreadContext.mConstraintBlockStream.getMemBlockPool().acquireMultipleFrictionBlocks(numFrictionBlocks, mFrictionBlockStart, mNumAcquiredFrictionBlocks);
#ifdef PX_CHECKED
		noAvailableBlocksError(mNumAcquiredFrictionBlocks,numFrictionBlocks,PxPS3ConfigParam::eMEM_FRICTION_BLOCKS);
#endif
		mFrictionResourceManager->setMemBlocks(mNumAcquiredFrictionBlocks,frictionBlocks);


		mAxisConstraintCount=0;

		mCellConstraintShaderSPUInput.mNumConstraints=numConstraintDescs + numSelfConstraintDescs;
		mCellConstraintShaderSPUInput.mConstraintDescs=constraintDescs;
		mCellConstraintShaderSPUInput.mResourceManager=mResourceManager;
		mCellConstraintShaderSPUInput.mFrictionResourceManager = mFrictionResourceManager;
		mCellConstraintShaderSPUInput.mDT=mContext.getDt();
		mCellConstraintShaderSPUInput.mRecipDT=mContext.getInvDt();		
		mCellConstraintShaderSPUInput.mAxisConstraintCount=&mAxisConstraintCount;
		mCellConstraintShaderSPUInput.mActiveTaskCount=numSpus;
		mCellConstraintShaderSPUInput.mTotalTaskCount=numSpus;
		mCellConstraintShaderSPUInput.mSolverBodyData=mContext.mSolverBodyDataPool.begin();
		mCellConstraintShaderSPUInput.mBounceThreshold = mContext.getBounceThreshold();
		mCellConstraintShaderSPUInput.mFrictionOffsetThreshold = mContext.getFrictionOffsetThreshold();
		mCellConstraintShaderSPUInput.mCorrelationDistance = mContext.getContext()->getCorrelationDistance();
		mCellConstraintShaderSPUInput.mFrictionModel = mContext.getContext()->getFrictionModel();
		mCellConstraintShaderSPUInput.mConstraintHeaders = headers;
		mCellConstraintShaderSPUInput.mNumConstraintHeaders = numHeaders;
#if SPU_PROFILE_CONSTRAINT_PREP
		mCellConstraintShaderSPUInput.mProfileZones=gProfileCounters;
#endif

		//Start all the spu tasks.
		for (PxU32 uiTask=0; uiTask < getSpuCount(); uiTask++) 
		{
			setArgs(uiTask, uiTask | (unsigned int)&mCellConstraintShaderSPUOutput[uiTask], (unsigned int)&mCellConstraintShaderSPUInput);
		}
	}

	virtual void release()
	{
		PxU32 axisConstraintCount = mAxisConstraintCount;

		//Release the unused memory blocks.
		mThreadContext.mConstraintBlockStream.getMemBlockPool().releaseMultipleConstraintBlocks(mBlockStart + mResourceManager->getNextFreeBlockCount(), 
			mBlockStart + mNumAcquiredBlocks);

		//Release the friction blocks
		mThreadContext.mConstraintBlockStream.getMemBlockPool().releaseMultipleFrictionBlocks(mFrictionBlockStart + mFrictionResourceManager->getNextFreeBlockCount(), 
			mFrictionBlockStart + mNumAcquiredFrictionBlocks);

		//Now complete any shaders that didn't execute on spu because we ran out of memory (or any other reason)
		bool errorWarning = false;
		const PxsFrictionModel frictionType = mContext.getContext()->getFrictionModel();

		PxU16 maxSolverConstraintSize = 0;

		PxcSolverConstraintDesc* contactDescPtr = mCellConstraintShaderSPUInput.mConstraintDescs;

		PxReal dt = mContext.getDt();
		PxReal invDt = mContext.getInvDt();
		PxReal bounceThreshold = mContext.getBounceThreshold();

		PxcSolverBodyData* solverBodyData = mCellConstraintShaderSPUInput.mSolverBodyData;

		for (PxU32 uiTask=0; uiTask < getSpuCount(); uiTask++)
		{
			const PxU32 start=mCellConstraintShaderSPUOutput[uiTask].mPPUStartHeader;
			const PxU32 stride=mCellConstraintShaderSPUOutput[uiTask].mPPUStride;
			const PxU32 end=mCellConstraintShaderSPUOutput[uiTask].mPPUEnd;
			PxU32 headerInd = mCellConstraintShaderSPUOutput[uiTask].mPPUHeaderIndex;
			PxU32 ind = mCellConstraintShaderSPUOutput[uiTask].mPPUDescIndex;
			axisConstraintCount += mCellConstraintShaderSPUOutput[uiTask].mAxisConstraintCount;
			maxSolverConstraintSize = PxMax(maxSolverConstraintSize, (PxU16)mCellConstraintShaderSPUOutput[uiTask].mMaxConstraintLength);

#if PX_CHECKED
			if(mCellConstraintShaderSPUOutput[uiTask].mOverflowedFrictionPatches)
			{
				Ps::getFoundation().error(physx::PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
					"Dropping contacts in solver because we exceeded limit of 32 friction patches.");
			}
#endif

			//TODO - call createFinalize here. This should work on descs only!
			for(PxU32 i = start; i < end; i += stride)
			{
				errorWarning = true;
				//SPU unrolls 8 at a time now
				PxU32 numToProcess = PxMin(end - i, 4u);
				for(PxU32 j = headerInd; j < numToProcess; ++j)
				{
					PxsConstraintBatchHeader& header = mCellConstraintShaderSPUInput.mConstraintHeaders[i+j];

					if(contactDescPtr[header.mStartIndex+ind].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
					{
						PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
						if(ind == 0 && header.mStride == 4)
						{
							PxcSolverConstraintDesc& desc0 = contactDescPtr[header.mStartIndex];
							PxcSolverConstraintDesc& desc1 = contactDescPtr[header.mStartIndex+1];
							PxcSolverConstraintDesc& desc2 = contactDescPtr[header.mStartIndex+2];
							PxcSolverConstraintDesc& desc3 = contactDescPtr[header.mStartIndex+3];

							PxsContactManager *cm0 = (PxsContactManager*)desc0.constraint;
							PxsContactManager *cm1 = (PxsContactManager*)desc1.constraint;
							PxsContactManager *cm2 = (PxsContactManager*)desc2.constraint;
							PxsContactManager *cm3 = (PxsContactManager*)desc3.constraint;


							PxcNpWorkUnit* n[4] = {&cm0->getWorkUnit(), &cm1->getWorkUnit(), &cm2->getWorkUnit(), &cm3->getWorkUnit()};
							PxcSolverConstraintDesc* descs[4] = {&desc0, &desc1, &desc2, &desc3};
							const PxcSolverBody* sBody0[4] = {desc0.bodyA, desc1.bodyA, desc2.bodyA, desc3.bodyA};
							const PxcSolverBody* sBody1[4] = {desc0.bodyB, desc1.bodyB, desc2.bodyB, desc3.bodyB};

							const PxcSolverBodyData* sbd0[4] = {&solverBodyData[desc0.bodyADataIndex], &solverBodyData[desc1.bodyADataIndex], &solverBodyData[desc2.bodyADataIndex],
								&solverBodyData[desc3.bodyADataIndex]};
							const PxcSolverBodyData* sbd1[4] = {&solverBodyData[desc0.bodyBDataIndex], &solverBodyData[desc1.bodyBDataIndex], &solverBodyData[desc2.bodyBDataIndex],
								&solverBodyData[desc3.bodyBDataIndex]};

							const PxsRigidCore* core0[4] = {n[0]->rigidCore0, n[1]->rigidCore0, n[2]->rigidCore0, n[3]->rigidCore0};
							const PxsRigidCore* core1[4] = {n[0]->rigidCore1, n[1]->rigidCore1, n[2]->rigidCore1, n[3]->rigidCore1};

							state = createFinalizeMethods4[frictionType](n, mThreadContext,
								 descs,
								 sBody0,
								 sBody1,
								 sbd0,
								 sbd1,
								 core0,
								 core1,
								 invDt,
								 bounceThreshold,
								 mCellConstraintShaderSPUInput.mFrictionOffsetThreshold,
								 mCellConstraintShaderSPUInput.mCorrelationDistance,
								 mThreadContext.mConstraintBlockManager,
								 frictionType);

							maxSolverConstraintSize = PxMax(maxSolverConstraintSize, desc0.constraintLengthOver16);

						}
						if(PxcSolverConstraintPrepState::eUNBATCHABLE == state)
						{
							for(PxU32 i = ind; i < header.mStride; ++i)
							{
								PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
								PxsContactManager *cm = (PxsContactManager*)desc.constraint;
								PxcNpWorkUnit& n = cm->getWorkUnit();
							
								PxcSolverBodyData& data0 = desc.linkIndexA != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
								PxcSolverBodyData& data1 = desc.linkIndexB != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];
								
								createFinalizeMethods[frictionType](n, mThreadContext, false, false,
									desc, desc.bodyA, desc.bodyB, &data0, &data1, n.rigidCore0, n.rigidCore1, invDt, bounceThreshold, 
									mCellConstraintShaderSPUInput.mFrictionOffsetThreshold,mCellConstraintShaderSPUInput.mCorrelationDistance,
									mThreadContext.mConstraintBlockManager, frictionType);
						
								axisConstraintCount += (PxI32)cm->getConstraintDesc(desc);

								maxSolverConstraintSize = PxMax(maxSolverConstraintSize, desc.constraintLengthOver16);
							}
						}
					}
					else if(contactDescPtr[header.mStartIndex+ind].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
					{
						PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
						if(ind == 0 && header.mStride == 4)
						{
							PxTransform id = PxTransform(PxIdentity);

							PxcSolverConstraint4Desc descs[4];
							for(PxU32 i = 0; i < 4; ++i)
							{
								PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
								PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
							
								const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
								const void* constantBlock = constraint->constantBlock;
								const PxU32 constantBlockByteSize = constraint->constantBlockSize;
								const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
								const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
								const PxcSolverBody* sbody0 = desc.bodyA;
								const PxcSolverBody* sbody1 = desc.bodyB;
								PxcSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
								PxcSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];


								descs[i].constantBlock = constantBlock;
								descs[i].constantBlockByteSize = constantBlockByteSize;
								descs[i].constraint = constraint;
								descs[i].desc = &desc;
								descs[i].pose0 = &pose0;
								descs[i].pose1 = &pose1;
								descs[i].sBodyData0 = sbodyData0;
								descs[i].sBodyData1 = sbodyData1;
								descs[i].solverBody0 = sbody0;
								descs[i].solverBody1 = sbody1;
								descs[i].solverPrep = solverPrep;
							}

							state = setupSolverConstraint4
								(descs, dt, invDt,
								 mThreadContext.mConstraintBlockStream, mThreadContext.mConstraintBlockManager);

							maxSolverConstraintSize = PxMax(maxSolverConstraintSize, descs[0].desc->constraintLengthOver16);
						}
						if(state == PxcSolverConstraintPrepState::eUNBATCHABLE)
						{
							for(PxU32 i = ind; i < header.mStride; ++i)
							{
								PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
								PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
								axisConstraintCount +=  PxsSetupSolverConstraint(constraint, solverBodyData, mThreadContext.mConstraintBlockStream, dt, 
									invDt, desc, mThreadContext.mConstraintBlockManager);
								maxSolverConstraintSize = PxMax(maxSolverConstraintSize, desc.constraintLengthOver16);
							}
						}
					}
					ind = 0;
					headerInd = 0;
				}
			}
		}

		mThreadContext.mAxisConstraintCount += axisConstraintCount;

		if(errorWarning)
		{
			PX_WARN_ONCE(true, "Reverting spu constraint prep/finalize to ppu - need more 16K blocks to complete on spu");
			PX_WARN_ONCE(true, "Increase number reserved for spu with PxPS3Config::setSceneParamInt(myScene,PxPS3ConfigParam::eMEM_CONSTRAINT_BLOCKS)");
			PX_WARN_ONCE(true, ", PxPS3Config::setSceneParamInt(myScele, PxPS3ConfigParam::eMEM_CONTACT_BLOCKS) or  or PxPS3Config::setSceneParamInt(myScele, PxPS3ConfigParam::eMEM_FRICTION_BLOCKS)");
			PX_WARN_ONCE(true, "Be aware that PxSceneDesc::maxNbContactDataBlocks may need to be increased accordingly to ensure");
			PX_WARN_ONCE(true, " that there are sufficient 16K blocks to match the total number reserved for spu.");
			PX_WARN_ONCE(true, "See PxPS3Config::getSpuMemBlockCounters() for more information");
		}

		mContext.mMaxSolverConstraintSize = ((PxU32)maxSolverConstraintSize) * 16;

		mThreadContext.mConstraintBlockStream.getMemBlockPool().updateSpuConstraintBlockCount();
		mThreadContext.mConstraintBlockStream.getMemBlockPool().updateSpuFrictionBlockCount();
		
		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsSetupConstraintSpuTask"; }

	CellConstraintShaderSPUInput	PX_ALIGN(128, mCellConstraintShaderSPUInput);				
	CellConstraintShaderSPUOutput	PX_ALIGN(128, mCellConstraintShaderSPUOutput[6]);	
	PxU32							PX_ALIGN(128, mAxisConstraintCount);

	PxsResourceManager*				mResourceManager;
	PxsResourceManager*				mFrictionResourceManager;


	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	PxcSolverConstraintDesc*	mDescs;
	PxU32						mNumDescs;

	PxU32 mNumAcquiredBlocks;
	PxU32 mBlockStart;
	PxU32 mNumAcquiredFrictionBlocks;
	PxU32 mFrictionBlockStart;
};

#endif // SPU_CONSTRAINT_SHADER


#if SPU_SOLVER

PX_FORCE_INLINE bool correctOffsets()
{
	PxcFsData PX_ALIGN(16, d);
	PxcSolverBody PX_ALIGN(16, s);
	return (((PxU8*)&d.solverProgress - (PxU8*)&d) == ((PxU8*)&s.solverProgress -(PxU8*)&s) &&
		((PxU8*)&d.maxSolverNormalProgress - (PxU8*)&d) == ((PxU8*)&s.maxSolverNormalProgress -(PxU8*)&s));
}

class PxsSolverSpuTask : public PxSpuTask
{
public:

	PxsSolverSpuTask(
		const PxU32 numSpusForSolver,
		const PxF32 dt,
		const PxU32 maxSolverPositionIterations, const PxU32 maxSolverVelocityIterations,
		PxcSolverBody* solverBodyPool, PxcSolverBodyData* solverBodyData, Cm::SpatialVector* motionVelocity, const PxU32 bodyCount,
		PxcArticulationSolverDesc* solverArtics, const PxU32 solverArticsCount,
		PxcSolverConstraintDesc* orderedConstraints, const PxU32 orderedConstraintsCount, const PxU32 numDifferentBodyConstraint, const PxU32 numSelfConstraints,
		PxcFsSelfConstraintBlock* selfConstraintBlocks, const PxU32 numSelfConstraintBlocks,
		PxcThresholdStreamElement* thresholdStream, PxU32& thresholdPairsOut, PxU32 contactManagerCount,
		PxsDynamicsContext& context, PxsConstraintBatchHeader* batchHeaders, PxU32 numBatchHeaders) :
		PxSpuTask(gPS3GetElfImage(SPU_ELF_SOLVER_TASK), gPS3GetElfSize(SPU_ELF_SOLVER_TASK), numSpusForSolver),
		mConstraintProgressCounter(0),
		mAtomSaveProgressCounter(0),
		mArticulationProgressCounter(0),
		mOrderedConstraints(orderedConstraints),
		mDescCount(orderedConstraintsCount),
		mContactManagerCount(contactManagerCount),
		mThresholdStream(thresholdStream),
		mContext(context),
		mSolverBodyData(solverBodyData),
		mThresholdPairsOut(thresholdPairsOut)
	{
		PX_ASSERT(orderedConstraintsCount == numDifferentBodyConstraint + numSelfConstraints);

		//Make sure that our assumptions about solverProgress and maxSolverProgress hold true.
		PX_ASSERT(correctOffsets());

		//Prepare the input data that will be passed to all the spus.
		mCellSolverSPUInput.mDt=dt;
		mCellSolverSPUInput.mNumPosIterations=maxSolverPositionIterations;
		mCellSolverSPUInput.mNumVelIterations=maxSolverVelocityIterations;
		mCellSolverSPUInput.mSolverAtoms=solverBodyPool;
		mCellSolverSPUInput.mNumSolverAtoms=bodyCount;
		mCellSolverSPUInput.mSolverArtics=solverArtics;
		mCellSolverSPUInput.mNumSolverArtics=solverArticsCount;
		mCellSolverSPUInput.mSolverConstraints=orderedConstraints;
		mCellSolverSPUInput.mNumSolverConstraints=orderedConstraintsCount;
		mCellSolverSPUInput.mNumDifferentBodyConstraints=numDifferentBodyConstraint;
		mCellSolverSPUInput.mNumSelfConstraints=numSelfConstraints;
		mCellSolverSPUInput.mSelfConstraintBlocks=selfConstraintBlocks;
		mCellSolverSPUInput.mNumSelfConstraintBlocks=numSelfConstraintBlocks;
		mCellSolverSPUInput.mTotalTaskCount = numSpusForSolver;
		mCellSolverSPUInput.mActiveTaskCount = numSpusForSolver;
		mCellSolverSPUInput.mMotionVelocityArray = motionVelocity;
	#if SPU_PROFILE_SOLVER
		mCellSolverSPUInput.mProfileZones = gProfileCounters;
	#endif
		mThresholdStreamSize=0;
		mCellSolverSPUInput.mThresholdStreamSize=&mThresholdStreamSize;
		mCellSolverSPUInput.mThresholdStreamCapacity=orderedConstraintsCount;	
		mCellSolverSPUInput.mThresholdStream=thresholdStream;	
		mCellSolverSPUInput.mSolverBodyData = solverBodyData;
		mCellSolverSPUInput.mAtomicProgressCounter = &mConstraintProgressCounter;
		mCellSolverSPUInput.mAtomicSaveStateCounter = &mAtomSaveProgressCounter;
		mCellSolverSPUInput.mArticulationProgressCounter = &mArticulationProgressCounter;
		mCellSolverSPUInput.mBatchHeaders = batchHeaders;
		mCellSolverSPUInput.mNumBatchHeaders = numBatchHeaders;

		//Start all the spu tasks.
		for (PxU32 uiTask=0; uiTask < numSpusForSolver; uiTask++) 
		{
			setArgs(uiTask, uiTask | (unsigned int)&mCellSolverSPUOutput[uiTask], (unsigned int)&mCellSolverSPUInput);
		}
	}

	virtual void release()
	{

#if SPU_SOLVER_WRITEBACK
		mThresholdPairsOut=mThresholdStreamSize;
#else
		mContext.mSolverCore->writeBackV(mOrderedConstraints, mDescCount, mCellSolverSPUInput.mBatchHeaders, mCellSolverSPUInput.mNumBatchHeaders, 
			mThresholdStream, mContactManagerCount, mThresholdPairsOut,
			mSolverBodyData, gVTableWriteBackBlock);
#endif 

		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsSolverSpuTask"; }

	CellSolverSPUInput	PX_ALIGN(128, mCellSolverSPUInput);
	CellSolverSPUOutput	PX_ALIGN(128, mCellSolverSPUOutput[6]);
	PxU32 				PX_ALIGN(128, mThresholdStreamSize);

	PxU32				PX_ALIGN(128, mConstraintProgressCounter);
	PxU32				PX_ALIGN(128, mAtomSaveProgressCounter);
	PxU32				PX_ALIGN(128, mArticulationProgressCounter);

	PxcSolverConstraintDesc* mOrderedConstraints;
	PxU32 mDescCount;
	PxU32 mContactManagerCount;
	PxcThresholdStreamElement* mThresholdStream;
	PxsDynamicsContext&			mContext;
	PxcSolverBodyData* mSolverBodyData;

	PxU32& mThresholdPairsOut;
};


class PxsSolverCoulombSpuTask : public PxSpuTask
{
public:

	PxsSolverCoulombSpuTask(
		const PxU32 numSpusForSolver,
		const PxF32 dt,
		const PxU32 maxSolverPositionIterations, const PxU32 maxSolverVelocityIterations,
		PxcSolverBody* solverBodyPool, PxcSolverBodyData* solverBodyData, Cm::SpatialVector* motionVelocity, const PxU32 bodyCount,
		PxcArticulationSolverDesc* solverArtics, const PxU32 solverArticsCount,
		PxcSolverConstraintDesc* orderedConstraints, const PxU32 orderedConstraintsCount,
		PxcSolverConstraintDesc* orderedFrictionConstraints, const PxU32 orderedFrictionConstraintsCount,
		const PxU32 numDifferentBodyConstraint, const PxU32 numSelfConstraints,
		const PxU32 numDifferentBodyFrictionConstraint, const PxU32 numSelfFrictionConstraints,
		PxcFsSelfConstraintBlock* selfConstraintBlocks, const PxU32 numSelfConstraintBlocks,
		PxcFsSelfConstraintBlock* selfConstraintFrictionBlocks, const PxU32 numSelfConstraintFrictionBlocks,
		PxcThresholdStreamElement* thresholdStream, PxU32& thresholdPairsOut, PxU32 contactManagerCount,
		PxsDynamicsContext& context, PxsConstraintBatchHeader* batchHeaders, PxU32 numBatchHeaders, PxsConstraintBatchHeader* frictionBatchHeaders,
		PxU32 numFrictionBatchHeaders) :
		PxSpuTask(gPS3GetElfImage(SPU_ELF_SOLVER_COULOMB_TASK), gPS3GetElfSize(SPU_ELF_SOLVER_COULOMB_TASK), numSpusForSolver),
		mConstraintProgressCounter(0),
		mAtomSaveProgressCounter(0),
		mArticulationProgressCounter(0),
		mOrderedConstraints(orderedConstraints),
		mDescCount(orderedConstraintsCount),
		mOrderedFrictionConstraints(orderedFrictionConstraints),
		mFrictionDescCount(orderedFrictionConstraintsCount),
		mContactManagerCount(contactManagerCount),
		mThresholdStream(thresholdStream),
		mContext(context),
		mSolverBodyData(solverBodyData),
		mThresholdPairsOut(thresholdPairsOut)
	{
	
		PX_ASSERT(orderedConstraintsCount == numDifferentBodyConstraint + numSelfConstraints);
		PX_ASSERT(orderedFrictionConstraintsCount == numDifferentBodyFrictionConstraint + numSelfFrictionConstraints);

		//Make sure that our assumptions about solverProgress and maxSolverProgress hold true.
		PX_ASSERT(correctOffsets());

		//Prepare the input data that will be passed to all the spus.
		mCellSolverSPUInput.mDt=dt;
		mCellSolverSPUInput.mNumPosIterations=maxSolverPositionIterations;
		mCellSolverSPUInput.mNumVelIterations=maxSolverVelocityIterations;
		mCellSolverSPUInput.mSolverAtoms=solverBodyPool;
		mCellSolverSPUInput.mNumSolverAtoms=bodyCount;
		mCellSolverSPUInput.mSolverArtics=solverArtics;
		mCellSolverSPUInput.mNumSolverArtics=solverArticsCount;
		mCellSolverSPUInput.mSolverConstraints=orderedConstraints;
		mCellSolverSPUInput.mNumSolverConstraints=orderedConstraintsCount;
		mCellSolverSPUInput.mSolverFrictionConstraints = orderedFrictionConstraints;
		mCellSolverSPUInput.mNumSolverFrictionConstraints = orderedConstraintsCount;
		mCellSolverSPUInput.mNumDifferentBodyConstraints=numDifferentBodyConstraint;
		mCellSolverSPUInput.mNumDifferentBodyFrictionConstraints = numDifferentBodyFrictionConstraint;
		mCellSolverSPUInput.mNumSelfConstraints=numSelfConstraints;
		mCellSolverSPUInput.mNumSelfFrictionConstraints = numSelfConstraints;
		mCellSolverSPUInput.mSelfConstraintBlocks=selfConstraintBlocks;
		mCellSolverSPUInput.mNumSelfConstraintBlocks=numSelfConstraintBlocks;
		mCellSolverSPUInput.mSelfConstraintFrictionBlocks = selfConstraintFrictionBlocks;
		mCellSolverSPUInput.mNumSelfConstraintFrictionBlocks = numSelfConstraintFrictionBlocks;
		mCellSolverSPUInput.mTotalTaskCount = numSpusForSolver;
		mCellSolverSPUInput.mActiveTaskCount = numSpusForSolver;
		mCellSolverSPUInput.mMotionVelocityArray = motionVelocity;
	#if SPU_PROFILE_SOLVER
		mCellSolverSPUInput.mProfileZones = gProfileCounters;
	#endif
		mThresholdStreamSize=0;
		mCellSolverSPUInput.mThresholdStreamSize=&mThresholdStreamSize;
		mCellSolverSPUInput.mThresholdStreamCapacity=orderedConstraintsCount;	
		mCellSolverSPUInput.mThresholdStream=thresholdStream;	
		mCellSolverSPUInput.mSolverBodyData = solverBodyData;
		mCellSolverSPUInput.mAtomicProgressCounter = &mConstraintProgressCounter;
		mCellSolverSPUInput.mAtomicSaveStateCounter = &mAtomSaveProgressCounter;
		mCellSolverSPUInput.mArticulationProgressCounter = &mArticulationProgressCounter;
		mCellSolverSPUInput.mBatchHeaders = batchHeaders;
		mCellSolverSPUInput.mFrictionBatchHeaders = frictionBatchHeaders;
		mCellSolverSPUInput.mNumBatchHeaders = numBatchHeaders;
		mCellSolverSPUInput.mNumFrictionBatchHeaders = numFrictionBatchHeaders;

		//Start all the spu tasks.
		for (PxU32 uiTask=0; uiTask < numSpusForSolver; uiTask++) 
		{
			setArgs(uiTask, uiTask | (unsigned int)&mCellSolverSPUOutput[uiTask], (unsigned int)&mCellSolverSPUInput);
		}
	}

	virtual void release()
	{

#if SPU_SOLVER_WRITEBACK
		mThresholdPairsOut=mThresholdStreamSize;
#else
		mContext.mSolverCore->writeBackV(mOrderedConstraints, mDescCount, mThresholdStream, mContactManagerCount, mThresholdPairsOut,
			mSolverBodyData, gVTableWriteBack);
#endif 

		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsSolverColoumbSpuTask"; }

	CellSolverSPUInput	PX_ALIGN(128, mCellSolverSPUInput);
	CellSolverSPUOutput	PX_ALIGN(128, mCellSolverSPUOutput[6]);
	PxU32 				PX_ALIGN(128, mThresholdStreamSize);

	PxU32				PX_ALIGN(128, mConstraintProgressCounter);
	PxU32				PX_ALIGN(128, mAtomSaveProgressCounter);
	PxU32				PX_ALIGN(128, mArticulationProgressCounter);

	PxcSolverConstraintDesc* mOrderedConstraints;
	PxU32 mDescCount;
	PxcSolverConstraintDesc* mOrderedFrictionConstraints;
	PxU32 mFrictionDescCount;
	PxU32 mContactManagerCount;
	PxcThresholdStreamElement* mThresholdStream;
	PxsDynamicsContext&			mContext;
	PxcSolverBodyData* mSolverBodyData;

	PxU32& mThresholdPairsOut;
};

#endif //SPU_SOLVER


class PxsSolverSetupSolveTask : public PxLightCpuTask
{
	PxsSolverSetupSolveTask& operator=(const PxsSolverSetupSolveTask&);
public:

	PxsSolverSetupSolveTask(
		PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset)
	{}


	virtual void run()
	{

		//PIX_PROFILE_ZONE(blockSolver);
#ifdef PX_PS3
		stopTimerMarker(eSETUPSOLVERCONSTRAINT);
		startTimerMarker(eSOLVER);
#endif

		//PIX_PROFILE_ZONE(PxsSetupSolveTask);


		PxcSolverConstraintDesc* contactDescBegin = mThreadContext.orderedContactConstraints.begin();
		PxcSolverConstraintDesc* contactDescPtr = mThreadContext.orderedContactConstraints.begin();

		PxcSolverBody* solverBodies = mContext.mSolverBodyPool.begin() + mSolverBodyOffset;
		PxcSolverBodyData* solverBodyData = mContext.mSolverBodyDataPool.begin();

		//PxcSolverConstraintDesc* _frictionDescBegin = mThreadContext.frictionConstraintDescArray.begin();
		//PxcSolverConstraintDesc* _frictionDescPtr = mThreadContext.mFrictionDescPtr;
		
		PxU32 frictionDescCount = mThreadContext.mNumDifferentBodyFrictionConstraints;

		PxU32 j = 0, i = 0;
		
		//On PS3, self-constraints will be bumped to the end of the constraint list
		//and processed separately. On PC/360, they will be mixed in the array and
		//classed as "different body" constraints regardless of the fact that they're self-constraints.
		PxU32 numBatches = mThreadContext.numDifferentBodyBatchHeaders;
		// TODO: maybe replace with non-null joints from end of the array

		//while(i<totalConstraintCount)
		PxU32 bb = 0;
		for(PxU32 b = 0; b < numBatches; ++b)
		{
			PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[b];
			PxU16 stride = header.mStride, newStride = header.mStride;
			PxU32 startIndex = j;
			for(PxU16 c = 0; c < stride; ++c)
			{
				if(getConstraintLength(contactDescBegin[i]) == 0)
				{
					newStride--;
					i++;
				}
				else
				{
					if(i!=j)
						contactDescBegin[j] = contactDescBegin[i];
					i++;
					j++;
					contactDescPtr++;
				}
			}

			if(newStride != 0)
			{
				mThreadContext.contactConstraintBatchHeaders[bb].mStartIndex = startIndex;
				mThreadContext.contactConstraintBatchHeaders[bb].mStride = newStride;
				mThreadContext.contactConstraintBatchHeaders[bb].mConstraintType = *contactDescBegin[startIndex].constraint;
				bb++;
			}
		}

		numBatches = bb;

		PxU32 contactDescCount = PxU32(contactDescPtr - contactDescBegin);

		//mThreadContext.mOrderedContactDescCount = contactDescCount;
		//mThreadContext.mContactDescPtr = contactDescPtr;


		//PxU32 numSkipped = mThreadContext.mNumDifferentBodyConstraints - contactDescCount;
		mThreadContext.mNumDifferentBodyConstraints = contactDescCount;

		PxU32 numSelfConstraintBlocks = mThreadContext.mNumSelfConstraintBlocks;

		//Remap self constraint array. Self-constraint blocks exists on PS3 as an optimization for SPU solver.
		for(PxU32 a = 0; a < numSelfConstraintBlocks; ++a)
		{
			PX_ASSERT(mThreadContext.mSelfConstraintBlocks[a].startId == i);
			PxU32 origNumSelfConstraints = mThreadContext.mSelfConstraintBlocks[a].numSelfConstraints;
			PxU32 startId = j;

			for(PxU32 b = 0; b < origNumSelfConstraints; ++b)
			{
				PxcSolverConstraintDesc& desc = contactDescBegin[i];

				if(getConstraintLength(desc))
				{
					PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[bb++];
					header.mStride = 1;
					header.mStartIndex = j;
					header.mConstraintType = *desc.constraint;
					if(i != j)
						contactDescBegin[j] = contactDescBegin[i];
					j++;
				}
				i++;
			}
			mThreadContext.mSelfConstraintBlocks[a].startId = startId;
			mThreadContext.mSelfConstraintBlocks[a].numSelfConstraints = j - startId;
		}

		mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(bb);	

		
		//Now we have a list of self-constraint blocks which map directly to the constraint list 

		PxU32 maxProgress = PostProcessConstraintPartitioning(solverBodies, mCounts.bodies, mThreadContext.getArticulations().begin(), 
					mThreadContext.getArticulations().size(), contactDescBegin, contactDescCount, mThreadContext.mSelfConstraintBlocks,
					mThreadContext.mNumSelfConstraintBlocks);


		mThreadContext.mNumSelfConstraints = j - contactDescCount; //self constraint count
		contactDescCount = j;
		mThreadContext.mOrderedContactDescCount = j;

		//Now do the friction constraints if we're not using the sticky model
		if(mThreadContext.mFrictionType != PXS_STICKY_FRICTION)
		{

			//mThreadContext.orderedFrictionConstraints.forceSize_Unsafe(0);
			//mThreadContext.orderedFrictionConstraints.reserve(contactDescCount + 3);
			//mThreadContext.orderedFrictionConstraints.forceSize_Unsafe(contactDescCount + 3);

			PxcSolverConstraintDesc* frictionDescBegin = mThreadContext.frictionConstraintDescArray.begin();
			PxcSolverConstraintDesc* frictionDescPtr = frictionDescBegin;

			Ps::Array<PxsConstraintBatchHeader>& frictionHeaderArray = mThreadContext.frictionConstraintBatchHeaders;
			frictionHeaderArray.forceSize_Unsafe(0);
			frictionHeaderArray.reserve(mThreadContext.contactConstraintBatchHeaders.size());
			PxsConstraintBatchHeader* headers = frictionHeaderArray.begin();

			PxU32 fricI = 0;
			for(PxU32 a = 0; a < numBatches; ++a)
			{
				PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[a];
				PxU16 stride = header.mStride;
				if(header.mConstraintType == PXS_SC_TYPE_RB_CONTACT || header.mConstraintType == PXS_SC_TYPE_EXT_CONTACT || 
					header.mConstraintType == PXS_SC_TYPE_STATIC_CONTACT)
				{
					PxU8 type = 0;
					//Extract friction from this constraint
					for(PxU16 b = 0; b < stride; ++b)
					{
						//create the headers...
						PxcSolverConstraintDesc& desc = contactDescBegin[header.mStartIndex + b];
						PX_ASSERT(desc.constraint);
						PxcSolverContactCoulombHeader* header = (PxcSolverContactCoulombHeader*)desc.constraint;
						PxU32 frictionOffset = header->frictionOffset;
						PxU8* PX_RESTRICT constraint =  (PxU8*)header + frictionOffset;
						const PxU32 origLength = getConstraintLength(desc);
						const PxU32 length = (origLength - frictionOffset);

						setConstraintLength(*frictionDescPtr, length);
						frictionDescPtr->constraint	= constraint;
						frictionDescPtr->bodyA = desc.bodyA;
						frictionDescPtr->bodyB = desc.bodyB;
						frictionDescPtr->bodyADataIndex = desc.bodyADataIndex;
						frictionDescPtr->bodyBDataIndex = desc.bodyBDataIndex;
						frictionDescPtr->linkIndexA = desc.linkIndexA;
						frictionDescPtr->linkIndexB = desc.linkIndexB;
						frictionDescPtr->writeBack = NULL;
						frictionDescPtr->writeBackLengthOver4 = 0;
						type = *constraint;
						frictionDescPtr++;
					}
					headers->mStartIndex = fricI;
					headers->mStride = stride;
					headers->mConstraintType = type;
					headers++;
					fricI += stride;
				}
				else if(header.mConstraintType == PXS_SC_TYPE_BLOCK_RB_CONTACT || header.mConstraintType == PXS_SC_TYPE_BLOCK_STATIC_RB_CONTACT)
				{
					//KS - TODO - Extract block of 4 contacts from this constraint. This isn't implemented yet for coulomb friction model
					PX_ASSERT(contactDescBegin[header.mStartIndex].constraint);
					PxcSolverContactCoulombHeader4* head = (PxcSolverContactCoulombHeader4*)contactDescBegin[header.mStartIndex].constraint;
					PxU32 frictionOffset = head->frictionOffset;
					PxU8* PX_RESTRICT constraint =  (PxU8*)head + frictionOffset;
					const PxU32 origLength = getConstraintLength(contactDescBegin[header.mStartIndex]);
					const PxU32 length = (origLength - frictionOffset);
					PxU8 type = *constraint;
					PX_ASSERT(type == PXS_SC_TYPE_BLOCK_FRICTION || type == PXS_SC_TYPE_BLOCK_STATIC_FRICTION);
					for(PxU32 b = 0; b < 4; ++b)
					{
						PxcSolverConstraintDesc& desc = contactDescBegin[header.mStartIndex+b];
						setConstraintLength(*frictionDescPtr, length);
						frictionDescPtr->constraint	= constraint;
						frictionDescPtr->bodyA = desc.bodyA;
						frictionDescPtr->bodyB = desc.bodyB;
						frictionDescPtr->bodyADataIndex = desc.bodyADataIndex;
						frictionDescPtr->bodyBDataIndex = desc.bodyBDataIndex;
						frictionDescPtr->linkIndexA = desc.linkIndexA;
						frictionDescPtr->linkIndexB = desc.linkIndexB;
						frictionDescPtr->writeBack = NULL;
						frictionDescPtr->writeBackLengthOver4 = 0;
						frictionDescPtr++;
					}
					headers->mStartIndex = fricI;
					headers->mStride = stride;
					headers->mConstraintType = type;
					headers++;
					fricI += stride;
				}
			}
		

			frictionDescCount = PxU32(frictionDescPtr - frictionDescBegin);
			//mThreadContext.mOrderedFrictionDescCount = frictionDescCount;

			//numSkipped = mThreadContext.mNumDifferentBodyFrictionConstraints - frictionDescCount;
			mThreadContext.mNumDifferentBodyFrictionConstraints = frictionDescCount;

			frictionHeaderArray.forceSize_Unsafe(PxU32(headers - frictionHeaderArray.begin()));

			//KS - TODO - self-constraint blocks

			PostProcessFrictionConstraintPartitioning(solverBodies, mCounts.bodies, mThreadContext.getArticulations().begin(), 
					mThreadContext.getArticulations().size(), frictionDescBegin, frictionDescCount,
					mThreadContext.mSelfConstraintFrictionBlocks, mThreadContext.mNumSelfConstraintFrictionBlocks);

			mThreadContext.mNumSelfFrictionConstraints = fricI - frictionDescCount; //self constraint count
			mThreadContext.mNumDifferentBodyFrictionConstraints = frictionDescCount;
			frictionDescCount = fricI;
			mThreadContext.mOrderedFrictionDescCount = frictionDescCount;


		}

#if SPU_SOLVER
		//Work out how many spus we'll be using for the solver.
		//Make sure that at least one constraint is handled by each spu - there's no point in wasting spus with empty tasks.
		//PxU32 contactDescCount = contactDescPtr - contactDescBegin;
#if FORCE_SINGLE_SPU
		PX_ASSERT(g_iPhysXSPUCount>=1);
		const PxU32 numSpusForSolver = 1;
#else
		const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
		//const PxU32 numSpusForSolver = contactDescCount < numSpusToUse ? 1 : numSpusToUse ;
		const PxU32 unrollSize = 16;
		//PX_UNUSED(unrollSize);
		const PxU32 numSpusForSolver = PxMin((contactDescCount/((maxProgress+1)*unrollSize)) + 1, numSpusToUse);
#endif

		if(mThreadContext.mSuccessfulSpuConstraintPartition && numSpusForSolver>0 && (mContext.getMaxSolverConstraintSize() + 16u)<=MAX_SPU_CONSTRAINT_LENGTH
			&& mThreadContext.mMaxArticulationLength <= MAX_SPU_ARTICULATED_LENGTH)
		{
			void* taskMem = mContext.getContext()->getTaskPool().allocate(sizeof(PxsSolverSpuTask), 128);
			if(mThreadContext.mFrictionType == PXS_STICKY_FRICTION)
			{
	
				PxsSolverSpuTask* spuTask = PX_PLACEMENT_NEW(taskMem, PxsSolverSpuTask)(
					numSpusForSolver,
					mContext.mDt,
					mThreadContext.mMaxSolverPositionIterations,
					mThreadContext.mMaxSolverVelocityIterations,
					solverBodies,
					solverBodyData,
					mThreadContext.motionVelocityArray.begin(),
					mCounts.bodies,
					mThreadContext.getArticulations().begin(), 
					mThreadContext.getArticulations().size(),
					mThreadContext.orderedContactConstraints.begin(),
					contactDescCount,
					mThreadContext.mNumDifferentBodyConstraints,
					mThreadContext.mNumSelfConstraints,
					mThreadContext.mSelfConstraintBlocks,
					mThreadContext.mNumSelfConstraintBlocks,
					mThreadContext.getThresholdStream().begin(),			
					mThreadContext.mThresholdPairCount,
					mCounts.contactManagers,
					mContext,
					mThreadContext.contactConstraintBatchHeaders.begin(),
					numBatches
					);

				spuTask->setContinuation(mCont);
				spuTask->removeReference();
			}
			else
			{
				PxsSolverCoulombSpuTask* spuTask = PX_PLACEMENT_NEW(taskMem, PxsSolverCoulombSpuTask)(
					numSpusForSolver,
					mContext.mDt,
					mThreadContext.mMaxSolverPositionIterations,
					mThreadContext.mMaxSolverVelocityIterations,
					solverBodies,
					solverBodyData,
					mThreadContext.motionVelocityArray.begin(),
					mCounts.bodies,
					mThreadContext.getArticulations().begin(), 
					mThreadContext.getArticulations().size(),
					mThreadContext.orderedContactConstraints.begin(),
					contactDescCount,
					mThreadContext.frictionConstraintDescArray.begin(),
					frictionDescCount,
					mThreadContext.mNumDifferentBodyConstraints,
					mThreadContext.mNumSelfConstraints,
					mThreadContext.mNumDifferentBodyFrictionConstraints,
					mThreadContext.mNumSelfFrictionConstraints,
					mThreadContext.mSelfConstraintBlocks,
					mThreadContext.mNumSelfConstraintBlocks,
					mThreadContext.mSelfConstraintFrictionBlocks,
					mThreadContext.mNumSelfConstraintFrictionBlocks,
					mThreadContext.getThresholdStream().begin(),			
					mThreadContext.mThresholdPairCount,
					mCounts.contactManagers,
					mContext,
					mThreadContext.contactConstraintBatchHeaders.begin(),
					numBatches,
					mThreadContext.frictionConstraintBatchHeaders.begin(),
					mThreadContext.frictionConstraintBatchHeaders.size()
					);


				spuTask->setContinuation(mCont);
				spuTask->removeReference();
			}
		}
		else
#endif //SPU_SOLVER
		{

			{
				CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext.getContext(),Sim,solver);

#if PX_CONSTRAINT_PARTITIONING || SPU_CONSTRAINT_PARTITIONING
				PxcSolverConstraintDesc* pContactDescs = mThreadContext.orderedContactConstraints.begin();
				PxcSolverConstraintDesc* pFrictionDescs = mThreadContext.frictionConstraintDescArray.begin();
#else
				PxcSolverConstraintDesc* pContactDescs = contactDescBegin;
				PxcSolverConstraintDesc* pFrictionDescs = mThreadContext.frictionConstraintDescArray.begin();
#endif

#if PX_PARALLEL_CONSTRAINT_PROCESSOR

				PxI32* iterValues = mThreadContext.mSolverProgressCounters;

				for(PxI32 a = 0; a < 6; ++a)
				{
					iterValues[a] = 0;
				}

				PxI32* pConstraintIteration = &iterValues[0];
				PxI32* pAtomIteration = &iterValues[1];
				PxI32* pThresholdPairsOut = &iterValues[2];
				PxI32* pAtomIntegrationIteration = &iterValues[3];
				volatile PxI32* pNumObjectsIntegrated = &iterValues[4];
				PxI32* pFrictionConstraintIteration = &iterValues[5];

				const PxU32 unrollSize = 16;
				const PxU32 MaxTasks = getTaskManager()->getCpuDispatcher()->getWorkerCount();
				const PxU32 numTasks = PxMin((contactDescCount/((maxProgress+1)*unrollSize)) + 1, MaxTasks);


				if(numTasks > 1)
				{
					for(PxU32 a = 1; a < numTasks; ++a)
					{
						void* tsk = mContext.getContext()->getTaskPool().allocate(sizeof(PxsParallelSolverTask));
						PxsParallelSolverTask* pTask = PX_PLACEMENT_NEW(tsk, PxsParallelSolverTask)(
							mContext,
							mThreadContext.mFrictionType,
							mThreadContext.mMaxSolverPositionIterations, 
							mThreadContext.mMaxSolverVelocityIterations,
							solverBodies,
							solverBodyData,
							mSolverBodyOffset,
							mCounts.bodies, 
							pContactDescs,
							pFrictionDescs,
							mThreadContext.getArticulations().begin(), 
							contactDescCount,
							frictionDescCount,
							mThreadContext.getArticulations().size(), 
							pConstraintIteration,
							pFrictionConstraintIteration,
							pAtomIteration, 
							pAtomIntegrationIteration, 
							pThresholdPairsOut, 
							mThreadContext.getThresholdStream().begin(), 
							mCounts.contactManagers, 
							mThreadContext.motionVelocityArray.begin(), 
							mThreadContext.bodyCoreArray.begin(), 
							mThreadContext.getLocalChangedShapes(), 
							mObjects.articulations, 
							mCounts.articulations, 
							pNumObjectsIntegrated,
							mThreadContext.contactConstraintBatchHeaders,
							mThreadContext.frictionConstraintBatchHeaders);

						//Force to complete before merge task!
						pTask->setContinuation(mCont);
						//pTask->setContinuation(this->mContext->getTaskManager(), NULL);
						pTask->removeReference();
					}

					//Avoid kicking off one parallel task when we can do the work inline in this function
					{
						CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::SimTask::GetPxsParallelSolve());

						solveParallel[mThreadContext.mFrictionType](
							mContext,
							mThreadContext.mMaxSolverPositionIterations, 
							mThreadContext.mMaxSolverVelocityIterations, 
							solverBodies, 
							solverBodyData,
							mSolverBodyOffset,
							mCounts.bodies, 
							mThreadContext.getArticulations().begin(), 
							mThreadContext.getArticulations().size(), 
							pContactDescs, 
							contactDescCount,
							pFrictionDescs,
							frictionDescCount,
							pConstraintIteration,
							pFrictionConstraintIteration,
							pAtomIteration, 
							pAtomIntegrationIteration, 
							mThreadContext.getThresholdStream().begin(), 
							mCounts.contactManagers, 
							pThresholdPairsOut,
							mThreadContext.motionVelocityArray.begin(), 
							mThreadContext.bodyCoreArray.begin(), 
							mThreadContext.getLocalChangedShapes(),
							mObjects.articulations, 
							mCounts.articulations, 
							pNumObjectsIntegrated,
							mThreadContext.contactConstraintBatchHeaders,
							mThreadContext.frictionConstraintBatchHeaders );
					}
					const PxI32 numAtomsPlusArtics = (PxI32)( mCounts.bodies + mCounts.articulations );
					while(*pNumObjectsIntegrated < numAtomsPlusArtics)
					{
					}

					mThreadContext.mThresholdPairCount = *pThresholdPairsOut;
				}
				else
#endif
				{
					PxU32 thresholdPairsOut = 0;
					//Only one task - a small island so do a sequential solve (avoid the atomic overheads)
#if (SPU_CONSTRAINT_PARTITIONING || PX_CONSTRAINT_PARTITIONING)
		
					solveVBlock[mThreadContext.mFrictionType](mContext.mSolverCore, mContext.mDt, mThreadContext.mMaxSolverPositionIterations, mThreadContext.mMaxSolverVelocityIterations, solverBodies, 
						solverBodyData, mSolverBodyOffset, mCounts.bodies, mThreadContext.getArticulations().begin(), mThreadContext.getArticulations().size(),
						pContactDescs, contactDescCount,  pFrictionDescs, frictionDescCount, mThreadContext.contactConstraintBatchHeaders, mThreadContext.frictionConstraintBatchHeaders, mThreadContext.motionVelocityArray.begin(),
						mThreadContext.getThresholdStream().begin(), mCounts.contactManagers, thresholdPairsOut);
#else
					solveV[mThreadContext.mFrictionType](mContext.mSolverCore, mContext.mDt, mThreadContext.mMaxSolverPositionIterations, mThreadContext.mMaxSolverVelocityIterations, solverBodies, 
						solverBodyData, mSolverBodyOffset, mCounts.bodies, mThreadContext.getArticulations().begin(), mThreadContext.getArticulations().size(),
						pContactDescs, contactDescCount, pFrictionDescs, frictionDescCount, mThreadContext.motionVelocityArray.begin());
					WriteBackMethod* method =  (mThreadContext.mFrictionType == PXS_STICKY_FRICTION)? gVTableWriteBack : gVTableWriteBackCoulomb;
					mContext.mSolverCore->writeBackV(pContactDescs, contactDescCount, mThreadContext.getThresholdStream().begin(), 
						mCounts.contactManagers, thresholdPairsOut, solverBodyData, method);
#endif
					//Write back

					

					mThreadContext.mThresholdPairCount = thresholdPairsOut;

#ifndef PX_PS3

					const PxU32 bodyCountMin1 = mCounts.bodies - 1;
					for(PxU32 i=0; i < mCounts.bodies; i++)
					{
						const PxU32 prefetchAddress = PxMin(i+4, bodyCountMin1);
						Ps::prefetchLine(mThreadContext.bodyCoreArray[prefetchAddress]);
						Ps::prefetchLine(&mThreadContext.motionVelocityArray[i], 128);
						Ps::prefetchLine(&mThreadContext.bodyCoreArray[prefetchAddress], 128);
						
						integrateCore(*mThreadContext.bodyCoreArray[i], mThreadContext.motionVelocityArray[i], mContext.mDt);
					}

					for(PxU32 i=0;i<mCounts.articulations;i++)
					{
						PxcArticulationSolverDesc &d = mThreadContext.getArticulations()[i];
						CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::Articulations::Getintegrate());

						PxcArticulationPImpl::updateBodies(d, mContext.getDt());
						for(PxU32 i=0;i<d.linkCount;i++)
						{
							const PxsRigidBody* body = static_cast<PxsRigidBody*>(d.links[i].body);
							const AABBMgrId aabbMgrId = body->getAABBMgrId();
							if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
								mThreadContext.getLocalChangedShapes().growAndSet(aabbMgrId.mSingleOrCompoundId);
						}

					}
#endif
				}
			}
		}
	}

	virtual const char* getName() const { return "PxsSolverSetupSolve"; }

	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	PxU32						mSolverBodyOffset;
};

#if SPU_INTEGRATE_CORE

class PxsIntegrateSpuTask : public PxSpuTask
{
public:

	PxsIntegrateSpuTask(
		PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset,
		PxU32 numSpusForIntegrateCore) :
		PxSpuTask(gPS3GetElfImage(SPU_ELF_INTEGRATE_CORE_TASK), gPS3GetElfSize(SPU_ELF_INTEGRATE_CORE_TASK), numSpusForIntegrateCore),
		mContext(context),
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts)
	{
		PxcSolverBody* solverBodies = mContext.mSolverBodyPool.begin() + solverBodyOffset;

		mCellIntegrateCoreSPUInput.mDt=mContext.mDt;
		mCellIntegrateCoreSPUInput.mBodyCorePtrArray=const_cast<PxsBodyCore**>(mThreadContext.bodyCoreArray.begin());	
		mCellIntegrateCoreSPUInput.mSolverBodyArray=solverBodies;	
		mCellIntegrateCoreSPUInput.mMotionVelocityArray=mThreadContext.motionVelocityArray.begin();
		mCellIntegrateCoreSPUInput.mNumBodyCores=mCounts.bodies;		
		mCellIntegrateCoreSPUInput.mActiveTaskCount=numSpusForIntegrateCore;	
		mCellIntegrateCoreSPUInput.mTotalTaskCount=numSpusForIntegrateCore;	
#if SPU_PROFILE_INTEGRATE_CORE
		mCellIntegrateCoreSPUInput.mProfileZones=gProfileCounters;		
#endif

		//Start all the spu tasks.
		for (PxU32 uiTask=0; uiTask < numSpusForIntegrateCore; uiTask++) 
		{
			setArgs(uiTask, uiTask | (unsigned int)&mCellIntegrateCoreSPUOutput[uiTask], (unsigned int)&mCellIntegrateCoreSPUInput);
		}
	}

	virtual void release()
	{
		//Articulations not on spu yet.
		for(PxU32 i=0;i<mCounts.articulations;i++)
		{
			PxcArticulationPImpl::updateBodies(mThreadContext.getArticulations()[i], mContext.mDt);
							
			for(PxU32 j=0;j<mThreadContext.getArticulations()[i].linkCount;j++)
			{
				const PxsRigidBody* body = static_cast<PxsRigidBody*>(mThreadContext.getArticulations()[i].links[j].body);
				const AABBMgrId aabbMgrId = body->getAABBMgrId();
				if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
					mThreadContext.getLocalChangedShapes().growAndSet(aabbMgrId.mSingleOrCompoundId);
			}

		}

		PxSpuTask::release();
	}

	virtual const char* getName() const { return "PxsIntegrateSpuTask"; }

	CellIntegrateCoreSPUInput	PX_ALIGN(128, mCellIntegrateCoreSPUInput);
	CellIntegrateCoreSPUOutput	PX_ALIGN(128, mCellIntegrateCoreSPUOutput[6]);

	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
};

#endif //SPU_INTEGRATE_CORE

#ifdef PX_PS3
class PxsSolverIntegrateTask : public PxLightCpuTask
{
	PxsSolverIntegrateTask& operator=(const PxsSolverIntegrateTask&);
public:

	PxsSolverIntegrateTask(PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset)
	{}

	virtual void run()
	{
		PxcSolverBody *solverBodies = mContext.mSolverBodyPool.begin() + mSolverBodyOffset;
		PX_UNUSED(solverBodies);

		stopTimerMarker(eSOLVER);
		startTimerMarker(eINTEGRATE_CORE);

#if SPU_INTEGRATE_CORE

		//Work out how many spus we'll be using for the core integration.
		//Make sure that at least one atom is handled by each spu - there's no point in wasting spus with empty tasks.
#if FORCE_SINGLE_SPU
		PX_ASSERT(g_iPhysXSPUCount>=1);
		const PxU32 numSpusForIntegrateCore = 1;
#else
		const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
		const PxU32 numSpusForIntegrateCore = mCounts.bodies < numSpusToUse ? 1 : numSpusToUse ;
#endif

		if(numSpusForIntegrateCore>0)
		{
			void* taskMem = mContext.getContext()->getTaskPool().allocate(sizeof(PxsIntegrateSpuTask), 128);
			PxsIntegrateSpuTask* spuTask = PX_PLACEMENT_NEW(taskMem, PxsIntegrateSpuTask)(
				mContext,
				mThreadContext,
				mObjects,
				mCounts,
				mSolverBodyOffset,
				numSpusForIntegrateCore);

			spuTask->setContinuation(mCont);
			spuTask->removeReference();
		}
		else
#endif
		{
			CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext.getContext(),Sim,updatePositions);

			PX_ASSERT(mThreadContext.mThresholdPairCount<=mCounts.contactManagers);

			const PxU32 bodyCountMin1 = mCounts.bodies - 1;
			for(PxU32 i=0; i < mCounts.bodies; i++)
			{
				const PxU32 prefetchAddress = PxMin(i+4, bodyCountMin1);
				Ps::prefetchLine(mThreadContext.bodyCoreArray[prefetchAddress]);
				Ps::prefetchLine(&mThreadContext.motionVelocityArray[i], 128);
				Ps::prefetchLine(&mThreadContext.bodyCoreArray[prefetchAddress], 128);
				
				integrateCore(*mThreadContext.bodyCoreArray[i], mThreadContext.motionVelocityArray[i], mContext.mDt);
			}

			for(PxU32 i=0;i<mCounts.articulations;i++)
			{
				PxcArticulationSolverDesc &d = mThreadContext.getArticulations()[i];
				CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::Articulations::Getintegrate());

				PxcArticulationPImpl::updateBodies(d, mContext.getDt());
				for(PxU32 i=0;i<d.linkCount;i++)
				{
					const PxsRigidBody* body = static_cast<PxsRigidBody*>(d.links[i].body);
					const AABBMgrId aabbMgrId = body->getAABBMgrId();
					if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
						mThreadContext.getLocalChangedShapes().growAndSet(aabbMgrId.mSingleOrCompoundId);
				}
			}

			PX_ASSERT(mThreadContext.mThresholdPairCount<=mCounts.contactManagers);
		}	
	}

	virtual const char* getName() const
	{
		return "PxsSolverIntegrateTask";
	}

private:
	PxsDynamicsContext&			mContext;
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	const PxU32					mSolverBodyOffset;

};
#endif


class PxsSolverEndTask : public PxLightCpuTask
{
	PxsSolverEndTask& operator=(const PxsSolverEndTask&);
public:

	PxsSolverEndTask(PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		const PxsIslandObjects& objects,				  
		const PxsIslandIndices& counts,
		const PxU32 solverBodyOffset) :
		mContext(context), 
		mThreadContext(threadContext),
		mObjects(objects),
		mCounts(counts),
		mSolverBodyOffset(solverBodyOffset)
	{}

	virtual void run()
	{		
#ifdef PX_PS3
		stopTimerMarker(eINTEGRATE_CORE);
#endif
		//PIX_PROFILE_ZONE(PxsSolverEndTask);
#if PX_ENABLE_SIM_STATS
		mThreadContext.getSimStats().numAxisSolverConstraints = mThreadContext.mAxisConstraintCount;
#endif

		PxcSolverBody *solverBodies = mContext.mSolverBodyPool.begin() + mSolverBodyOffset;
		PxcSolverBodyData *solverBodyData = mContext.mSolverBodyDataPool.begin() + mSolverBodyOffset;

		Cm::BitMap& localChangedShapes = mThreadContext.getLocalChangedShapes();

		for(PxU32 i=0; i < mCounts.bodies; i++)
		{
			PxsRigidBody &b = *mObjects.bodies[i];
			//b.setLinearMotionVelocity(threadContext.motionVelocityArray[i].linear);
			//b.setAngularMotionVelocity(threadContext.motionVelocityArray[i].angular);
			const PxU32 maxPrefetch2 = PxMin(i+4, (PxU32)mCounts.bodies - 1);
			const PxU32 maxPrefetch1 = PxMin(i+8, (PxU32)mCounts.bodies - 1);
			Ps::prefetchLine(mObjects.bodies[maxPrefetch2]->mCore);
			Ps::prefetchLine(mObjects.bodies[maxPrefetch2]->mCore,128);
			Ps::prefetchLine(mObjects.bodies[maxPrefetch1]);
			Ps::prefetchLine(mObjects.bodies[maxPrefetch1],128);
			Ps::prefetchLine(&solverBodies[i+8]);
			Ps::prefetchLine(&solverBodyData[i+8]);
			Ps::prefetchLine(&mObjects.bodies[i+32]);
			b.setVelocity(solverBodies[i].linearVelocity, solverBodies[i].angularVelocity);
			solverBodyData[i+1].originalBody = NULL; 
			if(PX_INVALID_BP_HANDLE!=b.getAABBMgrId().mSingleOrCompoundId)
				localChangedShapes.growAndSet(b.getAABBMgrId().mSingleOrCompoundId);
		}

		mThreadContext.saveThresholdStream();

		//Patch up the contact managers (TODO - fix up force writeback)
		PxU32 numCompoundConstraints = mThreadContext.compoundConstraints.size();
		for(PxU32 i = 0; i < numCompoundConstraints; ++i)
		{
			PxsCompoundContactManager& manager = mThreadContext.compoundConstraints[i];

			PxReal* contactForces = manager.unit->contactForces;
			PxU32 contactCount = manager.unit->contactCount;
			manager.unit->compressedContacts = manager.originalContactBuffer;
			manager.unit->compressedContactSize = manager.originalContactBufferSize;
			manager.unit->contactCount = Ps::to16(manager.originalContactCount);
			manager.unit->contactForces = manager.originalForceBuffer;
			
			for(PxU32 a = 1; a < manager.mStride; ++a)
			{
				PxsContactManager* pManager = mThreadContext.orderedContactList[manager.mStartIndex + a]->contactManager;
				pManager->getWorkUnit().frictionDataPtr = manager.unit->frictionDataPtr;
				pManager->getWorkUnit().frictionPatchCount = manager.unit->frictionPatchCount;
			}

			//This is a stride-based contact force writer. The assumption is that we may have skipped certain unimportant contacts reported by the 
			//discrete narrow phase
			if(contactForces)
			{
				PxU32 currentContactIndex = 0;
				PxU32 currentManagerIndex = manager.mStartIndex;
				PxU32 currentManagerContactIndex = 0;

				for(PxU32 a = 0; a < contactCount; ++a)
				{
					PxU32 index = manager.forceBufferList[a];
					while(currentContactIndex < index)
					{
						//Step forwards...first in this manager...
						PxsContactManager* pManager = mThreadContext.orderedContactList[currentManagerIndex]->contactManager;
						PxU32 numToStep = PxMin(index - currentContactIndex, (PxU32)pManager->getWorkUnit().contactCount - currentManagerContactIndex);
						currentContactIndex += numToStep;
						currentManagerContactIndex += numToStep;
						if(currentManagerContactIndex == pManager->getWorkUnit().contactCount)
						{
							currentManagerIndex++;
							currentManagerContactIndex = 0;
						}
					}
					//if(currentManagerIndex < mThreadContext.orderedContactList.size())
					{
						PxsContactManager* pManager = mThreadContext.orderedContactList[currentManagerIndex]->contactManager;
						if(pManager->getWorkUnit().contactForces)
							pManager->getWorkUnit().contactForces[currentManagerContactIndex] = contactForces[a];
					}
				}
			}
		}

		mThreadContext.compoundConstraints.forceSize_Unsafe(0);

		mThreadContext.mConstraintBlockManager.reset();

		mContext.getContext()->putThreadContext(&mThreadContext);
	}


	virtual const char* getName() const
	{
		return "PxsSolverEndTask";
	}

	PxsDynamicsContext&			mContext;	
	PxsThreadContext&			mThreadContext;
	const PxsIslandObjects		mObjects;
	const PxsIslandIndices		mCounts;
	const PxU32					mSolverBodyOffset;
};

class PxsSolverCreateFinalizeConstraintsTask : public PxLightCpuTask
{
	PxsSolverCreateFinalizeConstraintsTask& operator=(const PxsSolverCreateFinalizeConstraintsTask&);
public:

	PxsSolverCreateFinalizeConstraintsTask(
		PxsDynamicsContext& context,
		PxsThreadContext& threadContext,
		PxU32 solverDataOffset,
		const PxsIslandIndices& counts) : 
		mContext(context),
		mThreadContext(threadContext),
		mSolverDataOffset(solverDataOffset),
		mCounts(counts)
	{
	}
	
	virtual void run();

	virtual const char* getName() const { return "PxsSolverCreateFinalizeConstraintsTask"; }

	PxsDynamicsContext&	mContext;
	PxsThreadContext&	mThreadContext;
	PxU32 mSolverDataOffset;
	const PxsIslandIndices		mCounts;
};


}

namespace
{

// helper function to join two tasks together and ensure ref counts are correct
void chainTasks(PxLightCpuTask* first, PxLightCpuTask* next)
{
	first->setContinuation(next);
	next->removeReference();
}

PxBaseTask* createSolverTaskChain(PxsDynamicsContext& dynamicContext,
										const PxsIslandObjects& objects,				  
										const PxsIslandIndices& counts,
										const PxU32 solverBodyOffset, PxBaseTask* continuation)
{

	PxsContext*  context = dynamicContext.getContext();

	Cm::FlushPool& taskPool =  context->getTaskPool();

	taskPool.lock();

	// allocate a new thread context for the task chain
	PxsThreadContext* threadContext = context->getThreadContext();

	// create lead task
	PxsSolverStartTask* startTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverStartTask)), PxsSolverStartTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);	
	PxsSolverEndTask* endTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverEndTask)), PxsSolverEndTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);	


	threadContext->mFrictionType = context->getFrictionModel();

	PxsSolverCreateFinalizeConstraintsTask* createFinalizeConstraintsTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverCreateFinalizeConstraintsTask)), PxsSolverCreateFinalizeConstraintsTask)(dynamicContext, *threadContext, solverBodyOffset, counts);
	PxsSolverSetupSolveTask* setupSolveTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverSetupSolveTask)), PxsSolverSetupSolveTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);

	//PxsSolverConstraintPostProcessTask* postProcessTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverConstraintPostProcessTask)), PxsSolverConstraintPostProcessTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);
	PxsSolverConstraintPartitionTask* partitionConstraintsTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverConstraintPartitionTask)), PxsSolverConstraintPartitionTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);
	
#ifdef PX_PS3
	PxsSolverIntegrateTask* integrateTask = PX_PLACEMENT_NEW(taskPool.allocateNotThreadSafe(sizeof(PxsSolverIntegrateTask)), PxsSolverIntegrateTask)(dynamicContext, *threadContext, objects, counts, solverBodyOffset);
#endif

	taskPool.unlock();

	endTask->setContinuation(continuation);

	// set up task chain in reverse order
#ifdef PX_PS3
	chainTasks(integrateTask, endTask);
	chainTasks(setupSolveTask, integrateTask);
#else
	chainTasks(setupSolveTask, endTask);
#endif
	chainTasks(createFinalizeConstraintsTask, setupSolveTask);
	chainTasks(partitionConstraintsTask, createFinalizeConstraintsTask);
	//chainTasks(setupConstraintsTask, partitionConstraintsTask);
	//chainTasks(articulationTask, setupConstraintsTask);
	chainTasks(startTask, partitionConstraintsTask);
	/*chainTasks(postProcessTask, partitionConstraintsTask);
	chainTasks(startTask, postProcessTask);*/

	return startTask;
}


} // anonymous namepsace

void PxsDynamicsContext::update(PxReal dt, PxBaseTask* continuation)
{
	//PIX_PROFILE_ZONE(Dyanmics_Update);
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext, Sim, queueSolverTasks);
	mDt = dt;
	mInvDt = dt == 0.0f ? 0.0f : 1.0f/dt;

	const PxsIslandManager& im = mContext->getIslandManager();
	const PxU32 islandCount = im.getIslandCount();

	//If there is no work to do then we can do nothing at all.
	if(0 == islandCount)
	{
		return;
	}

	const PxU32 kinematicCount = im.getActiveKinematicCount();
	this->mKinematicCount = kinematicCount;
	PxsRigidBody *const* kinematics = im.getActiveKinematics();

	const PxsIslandObjects& objects = im.getIslandObjects();
	const PxsIslandIndices* indices = im.getIslandIndices();
	const PxsIslandIndices* start = indices, * sentinel = indices+islandCount;

	//ML: the start will be NULL pointer on the first frame if we run the solveStep before the collisionStep, because the islandGen is kicked off by the collisionStep
	//As such, we can bypass all this logic.
	if(start)
	{

		if(kinematicCount + sentinel->bodies > mSolverBodyPool.capacity())
		{
			mSolverBodyPool.reserve((kinematicCount + sentinel->bodies + 31) & ~31); // pad out to 32 * 128 = 4k to prevent alloc churn
			mSolverBodyDataPool.reserve((kinematicCount + sentinel->bodies + 31 + 1) & ~31); // pad out to 32 * 128 = 4k to prevent alloc churn
		}

		{
			PxcSolverBody emptySolverBody;
			PxMemZero(&emptySolverBody, sizeof(PxcSolverBody));
			mSolverBodyPool.resize(kinematicCount + sentinel->bodies, emptySolverBody);
			PxcSolverBodyData emptySolverBodyData;
			PxMemZero(&emptySolverBodyData, sizeof(PxcSolverBodyData));
			mSolverBodyDataPool.resize(kinematicCount + sentinel->bodies + 1, emptySolverBodyData);
		}

		// integrate and copy all the kinematics - overkill, since not all kinematics
		// need solver bodies

		mSolverBodyDataPool[0] = mWorldSolverBodyData;

		{
			CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext,Sim,updateKinematics);
			for(PxU32 i=0;i<kinematicCount;i++)
			{
				const PxsBodyCore& core = kinematics[i]->getCore();
				copyToSolverBody(mSolverBodyPool[i], mSolverBodyDataPool[i+1], core, *kinematics[i]);
				// Only really necessary for PS3 at the moment (for the cross island parallel constraint solver
				// but we might switch to the same on other platforms)
				mSolverBodyPool[i].solverProgress=MAX_PERMITTED_SOLVER_PROGRESS;
				mSolverBodyPool[i].maxSolverNormalProgress=MAX_PERMITTED_SOLVER_PROGRESS;
				mSolverBodyPool[i].maxSolverFrictionProgress=MAX_PERMITTED_SOLVER_PROGRESS;
			}
		}
	}

#if PX_CONSTRAINT_PARTITIONING
	//If we're doing the parallel solver on a non-SPU platform
	PxU32 solverBatchMax = mSolverBatchSize;//8192;
	PxU32 articulationBatchMax = 2;
	PxU32 minimumConstraintCount = 1;
#else
	PxU32 solverBatchMax = mSolverBatchSize;
	PxU32 minimumConstraintCount = 0;
	PxU32 articulationBatchMax = 16;
#endif
#ifdef PX_PS3
	//If we're going to run on spu then we want the spu-parallel solver to solve
	//all the constraints in a batch go because the cost of starting/stopping spus
	//for each batch is quite expensive. Also, the spu-parallel solver will have
	//fewer dependencies if all constraints are bundled in a single batch.
	if(getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS)>0)
	{
		solverBatchMax=8192;
		//KS - making this very large to force all articulations into a single island on SPU.
		//We need to do more work to support multiple islands
		articulationBatchMax = 1024;
	}
#endif
	//mMergeTask.setContinuation(continuation);

	while(start<sentinel)
	{
		PxsIslandObjects objectStarts;
		objectStarts.articulations		= objects.articulations		+ start->articulations;
		objectStarts.bodies				= objects.bodies			+ start->bodies;
		objectStarts.constraints		= objects.constraints		+ start->constraints;
		objectStarts.contactManagers	= objects.contactManagers	+ start->contactManagers;

		PxU32 constraintCount = 0;

		const PxsIslandIndices* end = start+1;
		while(end<sentinel && (((PxU32)(end->bodies - start->bodies) < solverBatchMax || (constraintCount < minimumConstraintCount)) && 
			(PxU32)(end->articulations - start->articulations) < articulationBatchMax))
		{
			end++;
			constraintCount = (end->articulations	- start->articulations) + (end->constraints - start->constraints) + (end->contactManagers - start->contactManagers);
		}

		PxsIslandIndices counts;
		
		counts.articulations	= end->articulations	- start->articulations;
		counts.bodies			= end->bodies			- start->bodies;

		counts.constraints		= end->constraints		- start->constraints;
		counts.contactManagers	= end->contactManagers	- start->contactManagers;

		if(counts.articulations + counts.bodies > 0)
		{
			PxBaseTask* task = createSolverTaskChain(*this, objectStarts, counts, kinematicCount + start->bodies, continuation);		
			task->removeReference();
		}

		start = end;
	}

	//mMergeTask.removeReference();
}

void PxsDynamicsContext::mergeResults()
{
	CM_PROFILE_ZONE_WITH_SUBSYSTEM(*mContext,Sim,solverMerge);

	PxsThresholdStream& mainStream = getContext()->getThresholdStream();
	mainStream.clear();

	PxcThreadCoherantCacheIterator<PxsThreadContext> threadContextIt(getContext()->getContactCache());
	PxsThreadContext* threadContext = threadContextIt.getNext();

#if PX_ENABLE_SIM_STATS
	PxsIslandManager& m = mContext->getIslandManager();
	PxsThreadContext::ThreadSimStats stats;
	stats.clear();
	if(m.getIslandCount() > 0)
	{
		PX_ASSERT(m.getIslandIndices());
		const PxsIslandIndices& indices = m.getIslandIndices()[m.getIslandCount()];
		stats.numActiveKinematicBodies = m.getActiveKinematicCount();
		stats.numActiveDynamicBodies = indices.bodies;	
		stats.numActiveConstraints = indices.constraints;
	}
	else
	{
		stats.numActiveKinematicBodies = m.getActiveKinematicCount();
		stats.numActiveDynamicBodies = 0;	
		stats.numActiveConstraints = 0;
	}

	getContext()->addThreadStats(stats);
#endif

	while(threadContext != NULL)
	{
		getContext()->mergeChangedShapeMap(threadContext->getLocalChangedShapes());
		mainStream.append(threadContext->getAccumulatedThresholdStream());
		threadContext->mConstraintBlockStream.reset();

#if PX_ENABLE_SIM_STATS
		PxsThreadContext::ThreadSimStats& threadStats = threadContext->getSimStats();
		getContext()->addThreadStats(threadStats);
		threadStats.clear();
#endif

		threadContext = threadContextIt.getNext();
	}

	mContext->getNpMemBlockPool().releaseConstraintMemory();
}


void _atomIntegrationParallel(
   const PxF32 dt,
   PxsBodyCore*const* bodyArray,					// INOUT: core body attributes
   PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
   PxU32 _bodyCount,									// IN: body count
   const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
   PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
   PxcSolverBodyData* solverBodyDataPool,			// IN: solver atom pool (space preallocated)
   Cm::SpatialVector* /*motionVelocityArray*/,			// OUT: motion velocities
   PxI32* pCounter,
   volatile PxI32* pIntegratedCount,
   volatile PxU32* maxSolverPositionIterations,
   volatile PxU32* maxSolverVelocityIterations)
{
	//PIX_PROFILE_ZONE(atomIntegrationParallel);
	PxU32 localMaxPosIter = 0;
	PxU32 localMaxVelIter = 0;

	const PxI32 bodyCount = (PxI32)_bodyCount;
	const PxI32 bodyCountMin1 = bodyCount - 1;

	// Integrate acceleration into velocity for every body and setup solver bodies

	PxI32 index = physx::atomicAdd(pCounter, 32) - 32;

	//for(PxU32 i=0; i < bodyCount; i++)

	PxI32 numIntegrated = 0;

	while(index < bodyCount)
	{
		const PxI32 numToIntegrate = PxMin(bodyCount - index, 32);
		const PxI32 numToIntegrateMin1 = numToIntegrate - 1;

		for(PxI32 a = 0; a < numToIntegrateMin1; ++a)
		{
			PxI32 i = index + a;
			const PxI32 prefetchIndex = PxMin(i + 4, bodyCountMin1);
			Ps::prefetchLine(bodyArray[prefetchIndex]);
			Ps::prefetchLine(bodyArray[prefetchIndex],128);
			Ps::prefetchLine(&solverBodyPool[i+4]);
			Ps::prefetchLine(&solverBodyDataPool[i+4]);
			Ps::prefetchLine(&solverBodyDataPool[i+4],128);
			Ps::prefetchLine(&accelerationArray[i+16]);

			PxsBodyCore& core = *bodyArray[i];
			
			PxU16 iterWord = core.solverIterationCounts;
			localMaxPosIter = PxMax<PxU32>(iterWord & 0xff, localMaxPosIter);
			localMaxVelIter = PxMax<PxU32>(iterWord >> 8, localMaxVelIter);

			bodyCoreComputeUnconstrainedVelocity(core, accelerationArray[i].linear, accelerationArray[i].angular, dt);

			copyToSolverBody(solverBodyPool[i], solverBodyDataPool[i+1], core, *originalBodyArray[i]);
			solverBodyPool[i].solverProgress = 0;
			solverBodyPool[i].maxSolverNormalProgress = 0;
			solverBodyPool[i].maxSolverFrictionProgress = 0;
		}
		const PxI32 i = numToIntegrateMin1 + index;
		PxsBodyCore& core = *bodyArray[i];
			
		PxU16 iterWord = core.solverIterationCounts;
		localMaxPosIter = PxMax<PxU32>(iterWord & 0xff, localMaxPosIter);
		localMaxVelIter = PxMax<PxU32>(iterWord >> 8, localMaxVelIter);

		bodyCoreComputeUnconstrainedVelocity(core, accelerationArray[i].linear, accelerationArray[i].angular, dt);

		copyToSolverBody(solverBodyPool[i], solverBodyDataPool[i+1], core, *originalBodyArray[i]);
		solverBodyPool[i].solverProgress = 0;
		solverBodyPool[i].maxSolverNormalProgress = 0;
		solverBodyPool[i].maxSolverFrictionProgress = 0;
		numIntegrated += numToIntegrate;

		index = physx::atomicAdd(pCounter, 32) - 32;
	}

	physx::atomicAdd(pIntegratedCount, numIntegrated);
	physx::atomicMax(reinterpret_cast<volatile PxI32*>(maxSolverPositionIterations), localMaxPosIter);
	physx::atomicMax(reinterpret_cast<volatile PxI32*>(maxSolverVelocityIterations), localMaxVelIter);
}

void atomIntegration(
   const PxF32 dt,
   PxsBodyCore*const* bodyArray,					// INOUT: core body attributes
   PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
   PxU32 bodyCount,									// IN: body count
   const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
   PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
   PxcSolverBodyData* solverBodyDataPool,			// IN: solver atom pool (space preallocated)
   Cm::SpatialVector* /*motionVelocityArray*/,			// OUT: motion velocities
   PxU32& maxSolverPositionIterations,
   PxU32& maxSolverVelocityIterations
   )
{
	PxU32 localMaxPosIter = 0;
	PxU32 localMaxVelIter = 0;

	// Integrate acceleration into velocity for every body and setup solver bodies
	for(PxU32 i=0; i < bodyCount; i++)
	{
		if(i+1<bodyCount)
		{
			Ps::prefetchLine(&solverBodyPool[i+1]);
			Ps::prefetchLine(&solverBodyDataPool[i+2]);
			Ps::prefetchLine(bodyArray[i+1]);
			Ps::prefetchLine(originalBodyArray[i+1]);
			Ps::prefetchLine(const_cast<Cm::SpatialVector*>(&accelerationArray[i+1]));
		}

		PxsBodyCore& core = *bodyArray[i];
		
		PxU16 iterWord = core.solverIterationCounts;
		localMaxPosIter = PxMax<PxU32>(iterWord & 0xff, localMaxPosIter);
		localMaxVelIter = PxMax<PxU32>(iterWord >> 8, localMaxVelIter);

		bodyCoreComputeUnconstrainedVelocity(core, accelerationArray[i].linear, accelerationArray[i].angular, dt);

		copyToSolverBody(solverBodyPool[i], solverBodyDataPool[i+1], core, *originalBodyArray[i]);
		solverBodyPool[i].solverProgress = 0;
		solverBodyPool[i].maxSolverNormalProgress = 0;
		solverBodyPool[i].maxSolverFrictionProgress = 0;
	}

	maxSolverPositionIterations=PxMax(localMaxPosIter, maxSolverPositionIterations);
	maxSolverVelocityIterations=PxMax(localMaxVelIter, maxSolverVelocityIterations);
}

void PxsAtomIntegrateTask::run()
{
	CM_PROFILE_ZONE(mContext.getContext()->getEventProfiler(),Cm::ProfileEventId::SimTask::GetPxsAtomIntegration());
	{
		_atomIntegrationParallel(mDt, mBodyArray, mOriginalBodyArray, mNumBodies, mAccelerationArray,
							mSolverBodies, mSolverBodyDataPool, mMotionVelocityArray, m_pCounter, m_pNumIntegrated,
							mMaxSolverPositionIterations, mMaxSolverVelocityIterations);
	}
}

void PxsDynamicsContext::atomIntegrationParallel(
   const PxF32 dt,
   PxsBodyCore*const* bodyArray,					// INOUT: core body attributes
   PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
   PxU32 _bodyCount,									// IN: body count
   const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
   PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
   PxcSolverBodyData* solverBodyDataPool,			// IN: solver atom pool (space preallocated)
   Cm::SpatialVector* motionVelocityArray,			// OUT: motion velocities
   PxU32& maxSolverPositionIterations,
   PxU32& maxSolverVelocityIterations,
   PxBaseTask& task
   )
{
	const PxI32 bodyCount = (PxI32)_bodyCount;

	//TODO - make this based on some variables so we can try different configurations
	const PxU32 IntegrationPerThread = 64;

	const PxU32 maxTasks = task.getTaskManager()->getCpuDispatcher()->getWorkerCount();
	const PxU32 numTasks = PxMin((bodyCount/IntegrationPerThread) + 1, maxTasks);

	if(numTasks > 1)
	{
		PxsAtomIntegrateTask* tasks = (PxsAtomIntegrateTask*)mContext->getTaskPool().allocate(sizeof(PxsAtomIntegrateTask) * numTasks);
		PxI32* atomicTesters = (PxI32*)mContext->getTaskPool().allocate(sizeof(PxI32) * 2);

		PxI32* pCounter = atomicTesters;
		volatile PxI32* pNumIntegrated = &atomicTesters[1];

		*pCounter = 0;

		*pNumIntegrated = 0;

		for(PxU32 a = 0; a < numTasks; ++a)
		{
			const PxU32 remainingBodies = PxMin(bodyCount - a, IntegrationPerThread);
			PX_UNUSED(remainingBodies);
			PxsAtomIntegrateTask* pTask = PX_PLACEMENT_NEW((&tasks[a]), PxsAtomIntegrateTask)(*this, bodyArray,
							originalBodyArray, solverBodyPool, solverBodyDataPool, motionVelocityArray, accelerationArray, dt,bodyCount, pCounter, pNumIntegrated,
							&maxSolverPositionIterations, &maxSolverVelocityIterations);

			pTask->setContinuation(&task);
			pTask->removeReference();
		}
	}
	else
	{
		atomIntegration( mDt, bodyArray, originalBodyArray, bodyCount, accelerationArray, solverBodyPool, solverBodyDataPool, motionVelocityArray, 
			maxSolverPositionIterations, maxSolverVelocityIterations );
	}
}

inline void WaitBodyRequiredState(volatile PxU32* state, PxU32 requiredState)
{
	while(requiredState != *state );
}

void physx::solveParallelDefaultFriction(SOLVER_PARALLEL_METHOD_ARGS)
{
	PX_UNUSED(pFrictionConstraintIndex);
	PX_UNUSED(frictionConstraintListSize);
	PX_UNUSED(frictionConstraintList);

	context.solveParallel(positionIterations, velocityIterations, 
		atomListStart, atomDataList, solverBodyOffset, atomListSize, articulationListStart, articulationListSize, constraintList, constraintListSize,
		pConstraintIndex, pAtomListIndex,pAtomIntegrationListIndex, thresholdStream, thresholdStreamLength, outThresholdPairs, motionVelocityArray, 
		bodyArray, localChangedShapes, articulations, _numArtics, pNumObjectsIntegrated, contactBlocks, frictionBlocks);
}


void physx::solveParallelCouloumFriction(SOLVER_PARALLEL_METHOD_ARGS)
{
	context.solveParallelCoulomb(positionIterations, velocityIterations, 
		atomListStart, atomDataList, solverBodyOffset, atomListSize, articulationListStart, articulationListSize, constraintList, constraintListSize,
		frictionConstraintList, frictionConstraintListSize, pConstraintIndex, pFrictionConstraintIndex, pAtomListIndex,pAtomIntegrationListIndex, thresholdStream, thresholdStreamLength, 
		outThresholdPairs, motionVelocityArray, bodyArray, localChangedShapes, articulations, _numArtics, pNumObjectsIntegrated, contactBlocks, frictionBlocks);
}

PxsSolveParallelMethod physx::solveParallel[3] =
{
	solveParallelDefaultFriction,
	solveParallelCouloumFriction,
	solveParallelCouloumFriction
};

void PxsDynamicsContext::solveParallel(const PxU32 positionIterations, const PxU32 velocityIterations, 
										 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
										 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
										 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
										 PxI32* pConstraintIndex, PxI32* pAtomListIndex, PxI32* pAtomIntegrationListIndex,
										 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
										 Cm::SpatialVector* motionVelocityArray, PxsBodyCore*const* bodyArray, Cm::BitMap& localChangedShapes,
										 PxsArticulation*const* PX_RESTRICT /*articulations*/, PxU32 _numArtics, volatile PxI32* pNumObjectsIntegrated,
										 Ps::Array<PxsConstraintBatchHeader>& contactBlocks, Ps::Array<PxsConstraintBatchHeader>& frictionBlocks)
{

	PxI32 normalIterations = 0;

	mSolverCore->solveVParallelAndWriteBack(mDt, positionIterations, velocityIterations, atomListStart, atomDataList, solverBodyOffset, atomListSize, articulationListStart, articulationListSize,
			constraintList, constraintListSize, pConstraintIndex, pAtomListIndex, thresholdStream, thresholdStreamLength,
			outThresholdPairs, contactBlocks, frictionBlocks, motionVelocityArray, normalIterations);
	const PxI32 unrollCount = 32;

	PxI32 index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollCount;

	const PxI32 numAtoms = (PxI32)atomListSize;
	const PxI32 numArtics = (PxI32)_numArtics;
	//const PxI32 atomsPlusArtics = numAtoms + numArtics;

	PxI32 numIntegrated = 0;

	//+1 for the conclude pass, +1 for save pass
	//const PxU32 iterationCount = positionIterations + velocityIterations + 1;

	PxI32 atomRemainder = unrollCount;

	while(index < numArtics)
	{
		const PxI32 remainder = PxMin(numArtics - index, unrollCount);
		atomRemainder -= remainder;

		for(PxI32 a = 0; a < remainder; ++a, index++)
		{
			const PxI32 i = index;
			PxcFsData* fs = articulationListStart[i].fsData;
			//const PxU32 requiredProgress = iterationCount * fs->maxSolverNormalProgress;
			const PxU16 requiredProgress = Ps::to16(normalIterations * fs->maxSolverNormalProgress);// + frictionIterations * fs->maxSolverFrictionProgress;
			WaitBodyRequiredState(&fs->solverProgress, requiredProgress);

			{
				CM_PROFILE_ZONE(mContext->getEventProfiler(),Cm::ProfileEventId::Articulations::Getintegrate());

				PxcArticulationPImpl::updateBodies(articulationListStart[i], mDt);
								
				for(PxU32 j=0;j<articulationListStart[i].linkCount;j++)
				{
					const PxsRigidBody* body = static_cast<PxsRigidBody*>(articulationListStart[i].links[j].body);
					const AABBMgrId aabbMgrId = body->getAABBMgrId();
					if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
						localChangedShapes.growAndSet(aabbMgrId.mSingleOrCompoundId);
				}
			}

			++numIntegrated;
		}
		if(atomRemainder == 0)
		{
			index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollCount;
			atomRemainder = unrollCount;
		}
	}	

	index -= numArtics;

	const PxI32 unrollPlusArtics = unrollCount + numArtics;

	while(index < numAtoms)
	{
		//WaitForBodyRequiredState(atomListStart[index], 0xFFFFFFFF-1);
		const PxI32 remainder = PxMin(numAtoms - index, atomRemainder);
		atomRemainder -= remainder;
		for(PxI32 a = 0; a < remainder; ++a, index++)
		{
			const PxI32 prefetch = PxMin(index+4, numAtoms - 1);
			Ps::prefetchLine(bodyArray[prefetch]);
			Ps::prefetchLine(bodyArray[prefetch],128);
			Ps::prefetchLine(&atomListStart[index],128);
			Ps::prefetchLine(&motionVelocityArray[index],128);
			Ps::prefetchLine(&bodyArray[index+32]);
			
			PxcSolverBody& b = atomListStart[index];
			//PxcSolverBodyData& bd = atomDataList[index+1 + solverBodyOffset];
			const PxU32 requiredProgress = normalIterations * b.maxSolverNormalProgress;// + frictionIterations * b.maxSolverFrictionProgress;
			//const PxU32 requiredProgress = iterationCount * b.maxSolverNormalProgress;
			WaitBodyRequiredState(&b.solverProgress, requiredProgress);

			integrateCore(*bodyArray[index], motionVelocityArray[index], mDt);

			++numIntegrated;
		}

		{
			index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollPlusArtics;
			atomRemainder = unrollCount;
		}
	}

	Ps::memoryBarrier();
	physx::atomicAdd(pNumObjectsIntegrated, numIntegrated);
}

void PxsDynamicsContext::solveParallelCoulomb(const PxU32 positionIterations, const PxU32 velocityIterations, 
										 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, const PxU32 atomListSize,
										 PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
										 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
										  PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintListSize,
										 PxI32* pConstraintIndex, PxI32* pFrictionConstraintIndex, PxI32* pAtomListIndex, PxI32* pAtomIntegrationListIndex,
										 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
										 Cm::SpatialVector* motionVelocityArray, PxsBodyCore*const* bodyArray, Cm::BitMap& localChangedShapes,
										 PxsArticulation*const* PX_RESTRICT /*articulations*/, PxU32 _numArtics, volatile PxI32* pNumObjectsIntegrated,
										 Ps::Array<PxsConstraintBatchHeader>& contactBlocks, Ps::Array<PxsConstraintBatchHeader>& frictionBlocks)
{

	PxI32 normalIterations = 0;
	PxI32 frictionIterations = 0;

	mSolverCore->solveVCoulombParallelAndWriteBack(mDt, positionIterations, velocityIterations, atomListStart, atomDataList, solverBodyOffset, atomListSize, articulationListStart, articulationListSize,
			constraintList, constraintListSize, frictionConstraintList, frictionConstraintListSize, pConstraintIndex, pFrictionConstraintIndex, pAtomListIndex, thresholdStream, thresholdStreamLength,
			outThresholdPairs, contactBlocks, frictionBlocks, motionVelocityArray, normalIterations, frictionIterations);
	const PxI32 unrollCount = 32;

	PxI32 index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollCount;

	const PxI32 numAtoms = (PxI32)atomListSize;
	const PxI32 numArtics = (PxI32)_numArtics;
	//const PxI32 atomsPlusArtics = numAtoms + numArtics;

	PxI32 numIntegrated = 0;

	//+1 for the conclude pass, +1 for save pass
	//const PxU32 iterationCount = positionIterations + velocityIterations + 1;

	PxI32 atomRemainder = unrollCount;

	while(index < numArtics)
	{
		const PxI32 remainder = PxMin(numArtics - index, unrollCount);
		atomRemainder -= remainder;

		for(PxI32 a = 0; a < remainder; ++a, index++)
		{
			const PxI32 i = index;
			PxcFsData* fs = articulationListStart[i].fsData;
			//const PxU32 requiredProgress = iterationCount * fs->maxSolverNormalProgress;
			const PxU16 requiredProgress = Ps::to16(normalIterations * fs->maxSolverNormalProgress + frictionIterations * fs->maxSolverFrictionProgress);
			WaitBodyRequiredState(&fs->solverProgress, requiredProgress);

			{
				CM_PROFILE_ZONE(mContext->getEventProfiler(),Cm::ProfileEventId::Articulations::Getintegrate());

				PxcArticulationPImpl::updateBodies(articulationListStart[i], mDt);
								
				for(PxU32 j=0;j<articulationListStart[i].linkCount;j++)
				{
					const PxsRigidBody* body = static_cast<PxsRigidBody*>(articulationListStart[i].links[j].body);
					const AABBMgrId aabbMgrId = body->getAABBMgrId();
					if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
						localChangedShapes.growAndSet(aabbMgrId.mSingleOrCompoundId);
				}
			}

			++numIntegrated;
		}
		if(atomRemainder == 0)
		{
			index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollCount;
			atomRemainder = unrollCount;
		}
	}	

	index -= numArtics;

	const PxI32 unrollPlusArtics = unrollCount + numArtics;

	while(index < numAtoms)
	{
		//WaitForBodyRequiredState(atomListStart[index], 0xFFFFFFFF-1);
		const PxI32 remainder = PxMin(numAtoms - index, atomRemainder);
		atomRemainder -= remainder;
		for(PxI32 a = 0; a < remainder; ++a, index++)
		{
			const PxI32 prefetch = PxMin(index+4, numAtoms - 1);
			Ps::prefetchLine(bodyArray[prefetch]);
			Ps::prefetchLine(bodyArray[prefetch],128);
			Ps::prefetchLine(&atomListStart[index],128);
			Ps::prefetchLine(&motionVelocityArray[index],128);
			Ps::prefetchLine(&bodyArray[index+32]);
			
			PxcSolverBody& b = atomListStart[index];
			//PxcSolverBodyData& bd = atomDataList[index+1 + solverBodyOffset];
			const PxU32 requiredProgress = normalIterations * b.maxSolverNormalProgress + frictionIterations * b.maxSolverFrictionProgress;
			//const PxU32 requiredProgress = iterationCount * b.maxSolverNormalProgress;
			WaitBodyRequiredState(&b.solverProgress, requiredProgress);

			integrateCore(*bodyArray[index], motionVelocityArray[index], mDt);

			++numIntegrated;
		}

		{
			index = physx::shdfnd::atomicAdd(pAtomIntegrationListIndex, unrollCount) - unrollPlusArtics;
			atomRemainder = unrollCount;
		}
	}

	Ps::memoryBarrier();
	physx::atomicAdd(pNumObjectsIntegrated, numIntegrated);
}


void PxsDynamicsContext::solveGroup(PxsThreadContext& threadContext, 
									const PxsIslandObjects& objects, 
									const PxsIslandIndices& counts,
									Cm::BitMap& /*shapeChangedMap*/, 
									const PxU32 solverBodyOffset,
									const PxU32 numSpusToUse)
{
	PX_UNUSED(numSpusToUse);

	PxU32 maxSolverPositionIterations = 0;
	PxU32 maxSolverVelocityIterations = 0;

	// Setup body atoms if there are any.

	PxcSolverBody *solverBodies = mSolverBodyPool.begin() + solverBodyOffset;
	PxcSolverBodyData *solverBodyData = mSolverBodyDataPool.begin() + solverBodyOffset;
	Cm::SpatialVector* motionVelocityArray = this->getContext()->getThreadContext()->motionVelocityArray.begin();

	solverBodyData[0] = mWorldSolverBodyData;

	// Integrate acceleration into velocity for every body and setup solver bodies
	for(PxU32 i=0; i < counts.bodies; i++)
	{
		PxsRigidBody& currentBody = *objects.bodies[i];

		PxU32 iterWord = currentBody.getIterationCounts();	//low 16 bits is pos iters, high 16 bits is vel iters.
		maxSolverVelocityIterations = PxMax<PxU32>(iterWord >> 16,	maxSolverVelocityIterations);
		maxSolverPositionIterations = PxMax<PxU32>(iterWord & 0xffff, maxSolverPositionIterations);

		computeUnconstrainedVelocity(&currentBody);
		copyToSolverBody(solverBodies[i], solverBodyData[i+1], currentBody.getCore(), currentBody);

	}

	PxU32 axisConstraints = 0;

	PxcConstraintBlockStream& stream = threadContext.mConstraintBlockStream;
	Ps::Array<PxcSolverConstraintDesc>& contactDescArray = threadContext.contactConstraintDescArray;
	contactDescArray.clear();

	Ps::Array<PxcSolverConstraintDesc>& frictionDescArray = threadContext.frictionConstraintDescArray;
	frictionDescArray.clear();


	threadContext.getArticulations().resize(counts.articulations);
	for(PxU32 i=0;i<counts.articulations; i++)
	{
		PxsArticulation &a = *objects.articulations[i];
		a.getSolverDesc(threadContext.getArticulations()[i]);

		// it's not easy to know up front how many constraints an articulation will generate, so
		// we reserve space then resize down if we got too many.
		PxU32 s = contactDescArray.size();
		contactDescArray.resize(s + a.getBodyCount());

		PxU32 acCount;
		PxU32 descsUsed = PxcArticulationPImpl::computeUnconstrainedVelocities(threadContext.getArticulations()[i], mDt, 
			stream, contactDescArray.begin()+s, acCount, mContext->getEventProfiler(), mContext->getScratchAllocator(), threadContext.mConstraintBlockManager);
		axisConstraints += acCount;
		contactDescArray.resize(s + descsUsed);

		const PxU32 iterWord = a.getIterationCounts();
		maxSolverVelocityIterations = PxMax<PxU32>(iterWord >> 16,	maxSolverVelocityIterations);
		maxSolverPositionIterations = PxMax<PxU32>(iterWord & 0xffff, maxSolverPositionIterations);
	}

	//Loop through constraints and construct them
	{
		const PxsFrictionModel frictionModel = mContext->getFrictionModel();

		//PxsMaterialInfo materialInfo[physx::ContactBuffer::MAX_CONTACTS];

		threadContext.mConstraintBlockManager.reset();
		threadContext.mConstraintBlockStream.reset();

			


		for(PxU32 i=0; i<counts.contactManagers; i++)
		{
			PxsIndexedContactManager& icm = objects.contactManagers[i];
			PxcSolverConstraintDesc& desc = contactDescArray.insert();
			setDescFromIndices(desc, icm);
			PxsContactManager& cm = *icm.contactManager;
			PxcNpWorkUnit& n = cm.getWorkUnit();

			PxcSolverBodyData& data0 = desc.linkIndexA != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
			PxcSolverBodyData& data1 = desc.linkIndexB != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];
			

			createFinalizeMethods[frictionModel](n, threadContext, false, false,
				desc, desc.bodyA, desc.bodyB, &data0, &data1, n.rigidCore0, n.rigidCore1, mInvDt, mBounceThreshold, mFrictionOffsetThreshold,
				threadContext.mCorrelationDistance, threadContext.mConstraintBlockManager, frictionModel);

			PxU32 acCount = icm.contactManager->getConstraintDesc(desc);
			if(!acCount)
				contactDescArray.popBack();

			axisConstraints += acCount;
		}
	}

	


	const PxF32 invdt = getInvDt();
	for(PxU32 i=0; i<counts.constraints; i++)				
	{
		PxsIndexedConstraint& ic = objects.constraints[i];
		PxcSolverConstraintDesc& desc = contactDescArray.insert();
		setDescFromIndices(desc, ic);

		const PxU32 acCount = PxsSetupSolverConstraint(ic.constraint, solverBodyData, stream, mDt, invdt, desc, threadContext.mConstraintBlockManager);
		if(!acCount)
			contactDescArray.popBack();

		axisConstraints += acCount;
	}

	threadContext.getSimStats().numAxisSolverConstraints += axisConstraints;

	//todo: finalize on the PPU until this function refactored into tasks
	//finalizeContacts(threadContext.mFrictionType, 0, contactDescArray.begin(), solverBodyData, contactDescArray.size(), NULL);

	PxsThresholdStream& thresholdStream = threadContext.getThresholdStream(); 
	thresholdStream.clear();
	thresholdStream.reserve(counts.contactManagers);
	PxU32 thresholdPairsOut;

	// temporarily allow zero iteration counts so single deformables can go down this path
	maxSolverVelocityIterations = PxMax<PxU32>(maxSolverVelocityIterations, 1);
	maxSolverPositionIterations = PxMax<PxU32>(maxSolverPositionIterations, 1);
	
	// Run interleaved constraint solver

	mSolverCore->solveV(
		mDt, maxSolverPositionIterations, maxSolverVelocityIterations,
		solverBodies, mSolverBodyDataPool.begin(), solverBodyOffset, counts.bodies,
		threadContext.getArticulations().begin(), threadContext.getArticulations().size(),
		contactDescArray.begin(), contactDescArray.size(), 
		frictionDescArray.begin(), frictionDescArray.size(),
		motionVelocityArray);

	mSolverCore->writeBackV(contactDescArray.begin(), contactDescArray.size(),
							thresholdStream.begin(), counts.contactManagers, thresholdPairsOut, mSolverBodyDataPool.begin(),
							gVTableWriteBack);

	PX_ASSERT(thresholdPairsOut <= counts.contactManagers);
	threadContext.mThresholdPairCount = thresholdPairsOut;
	threadContext.saveThresholdStream();

	for(PxU32 i=0; i < counts.bodies; i++)		// Integrate velocities into pose 
	{
		PxcSolverBody& solverBody = solverBodies[i];
		PxcSolverBodyData& sbd = solverBodyData[i+1];
		PxsRigidBody& originalBody = *sbd.originalBody;
		Cm::SpatialVector& motionVec = motionVelocityArray[i];

		originalBody.setVelocity(solverBody.linearVelocity, solverBody.angularVelocity);
		sbd.originalBody = NULL;
		integrateAtomPose(	&originalBody, threadContext.getLocalChangedShapes(),
							motionVec.linear, motionVec.angular);
	}

	for(PxU32 i=0;i<counts.articulations;i++)
	{
		PxcArticulationSolverDesc &d = threadContext.getArticulations()[i];
		PxcArticulationPImpl::updateBodies(d, mDt);

		CM_PROFILE_ZONE(mContext->getEventProfiler(),Cm::ProfileEventId::Articulations::Getintegrate());
							
		for(PxU32 i=0;i<d.linkCount;i++)
		{
			const PxsRigidBody* body = static_cast<PxsRigidBody*>(d.links[i].body);
			const AABBMgrId aabbMgrId = body->getAABBMgrId();
			if(PX_INVALID_BP_HANDLE!=aabbMgrId.mSingleOrCompoundId)
				threadContext.getLocalChangedShapes().growAndSet(aabbMgrId.mSingleOrCompoundId);
		}
	}

	threadContext.mConstraintBlockManager.reset();
}

void PxsDynamicsContext::computeUnconstrainedVelocity(PxsRigidBody* atom) const
{
	const Cm::SpatialVector& accel=atom->getAccelerationV();
	const PxVec3& linAccel = accel.linear;
	const PxVec3& angAccel = accel.angular;
	bodyCoreComputeUnconstrainedVelocity(atom->getCore(), linAccel, angAccel, mDt);
}

void PxsDynamicsContext::integrateAtomPose(
	PxsRigidBody* atom, Cm::BitMap &shapeChangedMap, const PxVec3& lv, const PxVec3& av
) const
{
	//------ Integrate motion velocity into new pose
	Cm::SpatialVector lav(lv, av);
	integrateCore(atom->getCore(), lav, mDt);
	if (mDt!=0 && !(lv.isZero() && av.isZero()))
	{
		if(PX_INVALID_BP_HANDLE!=atom->getAABBMgrId().mSingleOrCompoundId)
			shapeChangedMap.growAndSet(atom->getAABBMgrId().mSingleOrCompoundId);
	}
}


PxU32 createFinalizeContacts_Parallel(PxsFrictionModel frictionType, PxcSolverConstraintDesc* /*descArray*/, PxcSolverBodyData* solverBodyData, 
									  PxU32 /*descCount*/, PxsThreadContext& mThreadContext, PxI32* pIndex, PxReal dt, PxReal invDt, PxReal bounceThreshold,
									  PxReal frictionOffsetThreshold, PxsContext* context, PxI32* pAxisConstraintCount)
{
	const PxI32 UnrollSize = 8;

	//Do this outside
	//mThreadContext.mConstraintBlockManager.reset();
	PxcSolverConstraintDesc* contactDescPtr = mThreadContext.orderedContactConstraints.begin();
//	PxcSolverConstraintDesc* contactDescStartPtr = mThreadContext.orderedContactConstraints.begin();
	PxsConstraintBatchHeader* headers = mThreadContext.contactConstraintBatchHeaders.begin();
	PxI32 headerCount = (PxI32)mThreadContext.contactConstraintBatchHeaders.size();
	
	PxI32 axisConstraintCount = 0;
	//PxsMaterialInfo materialInfo[physx::ContactBuffer::MAX_CONTACTS];
	PxsThreadContext* threadContext = context->getThreadContext();
	threadContext->mConstraintBlockStream.reset(); //ensure there's no left-over memory that belonged to another island

	PxI32 index = physx::atomicAdd(pIndex, UnrollSize) - UnrollSize;

	while(index < headerCount)
	{
		PxU32 endIndex = PxMin(index + UnrollSize, headerCount);

		for(PxU32 a = index; a < endIndex; ++a)
		{
			//PxU32 ind = a;
			{
				const PxU32 prefetch1 = PxMin(a + 1u, headerCount - 1u);
				const PxsConstraintBatchHeader& nextHeader = headers[prefetch1];

				if(contactDescPtr[nextHeader.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
				{
					//we have a contact...prefetch contact data inside
					for(PxU32 i = 0; i < nextHeader.mStride; ++i)
					{
						PxcSolverConstraintDesc& desc = contactDescPtr[nextHeader.mStartIndex+i];
						PxsContactManager* contactManager = (PxsContactManager*)desc.constraint;
						PxcNpWorkUnit& unit = contactManager->getWorkUnit();
						Ps::prefetchLine(unit.compressedContacts);
						Ps::prefetchLine(unit.compressedContacts, 128);
					}
				}
				else if(contactDescPtr[nextHeader.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
				{
					for(PxU32 i = 0; i < nextHeader.mStride; ++i)
					{
						PxcSolverConstraintDesc& desc = contactDescPtr[nextHeader.mStartIndex+i];
						PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
						Ps::prefetchLine(constraint->constantBlock);
						Ps::prefetchLine(constraint->constantBlock,128);
						Ps::prefetchLine(constraint->constantBlock,256);
					}
				}
			}
			{
				const PxU32 prefetch = PxMin(a+2u, headerCount - 1u);
				const PxsConstraintBatchHeader& nextHeader = headers[prefetch];
				for(PxU32 i = 0; i < nextHeader.mStride; ++i)
				{
					Ps::prefetchLine(contactDescPtr[nextHeader.mStartIndex + i].constraint);
					Ps::prefetchLine(contactDescPtr[nextHeader.mStartIndex + i].constraint, 128);
				}
				Ps::prefetchLine(&contactDescPtr[nextHeader.mStartIndex + nextHeader.mStride]);
				Ps::prefetchLine(&contactDescPtr[nextHeader.mStartIndex + nextHeader.mStride],128);
			}

			PxsConstraintBatchHeader& header = headers[a];

			if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
			{
				PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
				if(header.mStride == 4)
				{
					PxcSolverConstraintDesc& desc0 = contactDescPtr[header.mStartIndex];
					PxcSolverConstraintDesc& desc1 = contactDescPtr[header.mStartIndex+1];
					PxcSolverConstraintDesc& desc2 = contactDescPtr[header.mStartIndex+2];
					PxcSolverConstraintDesc& desc3 = contactDescPtr[header.mStartIndex+3];

					PxsContactManager *cm0 = (PxsContactManager*)desc0.constraint;
					PxsContactManager *cm1 = (PxsContactManager*)desc1.constraint;
					PxsContactManager *cm2 = (PxsContactManager*)desc2.constraint;
					PxsContactManager *cm3 = (PxsContactManager*)desc3.constraint;


					PxcNpWorkUnit* n[4] = {&cm0->getWorkUnit(), &cm1->getWorkUnit(), &cm2->getWorkUnit(), &cm3->getWorkUnit()};
					PxcSolverConstraintDesc* descs[4] = {&desc0, &desc1, &desc2, &desc3};
					const PxcSolverBody* sBody0[4] = {desc0.bodyA, desc1.bodyA, desc2.bodyA, desc3.bodyA};
					const PxcSolverBody* sBody1[4] = {desc0.bodyB, desc1.bodyB, desc2.bodyB, desc3.bodyB};

					const PxcSolverBodyData* sbd0[4] = {&solverBodyData[desc0.bodyADataIndex], &solverBodyData[desc1.bodyADataIndex], &solverBodyData[desc2.bodyADataIndex],
						&solverBodyData[desc3.bodyADataIndex]};
					const PxcSolverBodyData* sbd1[4] = {&solverBodyData[desc0.bodyBDataIndex], &solverBodyData[desc1.bodyBDataIndex], &solverBodyData[desc2.bodyBDataIndex],
						&solverBodyData[desc3.bodyBDataIndex]};

					const PxsRigidCore* core0[4] = {n[0]->rigidCore0, n[1]->rigidCore0, n[2]->rigidCore0, n[3]->rigidCore0};
					const PxsRigidCore* core1[4] = {n[0]->rigidCore1, n[1]->rigidCore1, n[2]->rigidCore1, n[3]->rigidCore1};

					state = createFinalizeMethods4[frictionType](n, *threadContext,
						 descs,
						 sBody0,
						 sBody1,
						 sbd0,
						 sbd1,
						 core0,
						 core1,
						 invDt,
						 bounceThreshold,
						 frictionOffsetThreshold,
						 mThreadContext.mCorrelationDistance,
						 mThreadContext.mConstraintBlockManager,
						 frictionType);

				}
				if(PxcSolverConstraintPrepState::eUNBATCHABLE == state)
				{
					for(PxU32 i = 0; i < header.mStride; ++i)
					{
						PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
						PxsContactManager *cm = (PxsContactManager*)desc.constraint;
						PxcNpWorkUnit& n = cm->getWorkUnit();
					
						PxcSolverBodyData& data0 = desc.linkIndexA != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
						PxcSolverBodyData& data1 = desc.linkIndexB != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];
						
						createFinalizeMethods[frictionType](n, *threadContext, false, false,
							desc, desc.bodyA, desc.bodyB, &data0, &data1, n.rigidCore0, n.rigidCore1, invDt, bounceThreshold, 
							frictionOffsetThreshold, mThreadContext.mCorrelationDistance, mThreadContext.mConstraintBlockManager, frictionType);
				
						axisConstraintCount += (PxI32)cm->getConstraintDesc(desc);
					}
				}
			}
			else if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
			{
#if PXS_BATCH_1D
				PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
				if(header.mStride == 4)
				{
					PxTransform id = PxTransform(PxIdentity);

					PxcSolverConstraint4Desc descs[4];
					for(PxU32 i = 0; i < 4; ++i)
					{
						PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
						PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
					
						const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
						const void* constantBlock = constraint->constantBlock;
						const PxU32 constantBlockByteSize = constraint->constantBlockSize;
						const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
						const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
						const PxcSolverBody* sbody0 = desc.bodyA;
						const PxcSolverBody* sbody1 = desc.bodyB;
						PxcSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
						PxcSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];


						descs[i].constantBlock = constantBlock;
						descs[i].constantBlockByteSize = constantBlockByteSize;
						descs[i].constraint = constraint;
						descs[i].desc = &desc;
						descs[i].pose0 = &pose0;
						descs[i].pose1 = &pose1;
						descs[i].sBodyData0 = sbodyData0;
						descs[i].sBodyData1 = sbodyData1;
						descs[i].solverBody0 = sbody0;
						descs[i].solverBody1 = sbody1;
						descs[i].solverPrep = solverPrep;
					}

					state = setupSolverConstraint4
						(descs, dt, invDt,
						 threadContext->mConstraintBlockStream, mThreadContext.mConstraintBlockManager);
				}
				if(state == PxcSolverConstraintPrepState::eUNBATCHABLE)
#endif
				{
					for(PxU32 i = 0; i < header.mStride; ++i)
					{
						PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
						PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
						axisConstraintCount +=  PxsSetupSolverConstraint(constraint, solverBodyData, threadContext->mConstraintBlockStream, dt, 
							invDt, desc, mThreadContext.mConstraintBlockManager);
					}
				}
			}
		}
		//Grab the next block of work
		index = physx::atomicAdd(pIndex, UnrollSize) - UnrollSize;
	}

	context->putThreadContext(threadContext);
	physx::atomicAdd(pAxisConstraintCount, axisConstraintCount);
	//mThreadContext.mAxisConstraintCount = axisConstraintCount;
	return axisConstraintCount; //Can't write to mThreadContext as it's shared!!!!
}

class PxsCreateFinalizeTask : public PxLightCpuTask
{
	PxsCreateFinalizeTask& operator=(const PxsCreateFinalizeTask&);
public:
	PxsCreateFinalizeTask( PxsFrictionModel frictionType, const PxF32 dt, const PxF32 invDt, const PxF32 bounceThreshold, const PxF32 frictionOffsetThreshold,
		const PxU32 numConstraints, PxcSolverConstraintDesc* descArray, PxI32* pIndex, PxI32* pAxisConstraintCount, PxcSolverBodyData* solverBodyData,
		PxsThreadContext& threadContext, PxsContext* context) :
		mFrictionType(frictionType), mInvDt(invDt), mDt(dt), mBounceThreshold(bounceThreshold), mFrictionOffsetThreshold(frictionOffsetThreshold),
			mNumConstraints(numConstraints), mDescArray(descArray), mSolverBodyData(solverBodyData), m_pIndex(pIndex), 
			m_pAxisConstraintCount(pAxisConstraintCount), mThreadContext(threadContext), mContext(context)
	{}

	virtual void run()
	{
		createFinalizeContacts_Parallel(mFrictionType, mDescArray, mSolverBodyData, mNumConstraints, mThreadContext, m_pIndex, mDt, mInvDt, mBounceThreshold,
									  mFrictionOffsetThreshold, mContext, m_pAxisConstraintCount);
	}

	virtual const char* getName() const
	{
		return "PxsFinalizeTask";
	}

public:
	PxsFrictionModel mFrictionType;
	const PxF32 mInvDt;
	const PxF32 mDt;
	const PxF32 mBounceThreshold;
	const PxF32 mFrictionOffsetThreshold;
	const PxU32 mNumConstraints;
	PxcSolverConstraintDesc* mDescArray;
	PxcSolverBodyData* mSolverBodyData;
	PxI32* m_pIndex; 
	PxI32* m_pAxisConstraintCount;
	PxsThreadContext& mThreadContext;
	PxsContext* mContext;
};

void PxsDynamicsContext::createFinalizeContacts(PxsFrictionModel frictionType, PxcSolverConstraintDesc* /*descArray*/, PxcSolverBodyData* solverBodyData, 
												PxU32 /*descCount*/, PxBaseTask* /*continuation*/, PxsThreadContext& mThreadContext)
{
	//static PxU32 frameIndex = 0;
	//frameIndex++;
	PxcSolverConstraintDesc* contactDescPtr = mThreadContext.orderedContactConstraints.begin();
	PxsConstraintBatchHeader* headers = mThreadContext.contactConstraintBatchHeaders.begin();
	PxU32 totalHeaderCount = mThreadContext.contactConstraintBatchHeaders.size();
	//PxcSolverConstraintDesc* contactDescStartPtr = mThreadContext.orderedContactConstraints.begin();
	
	PxU32 axisConstraintCount = mThreadContext.mAxisConstraintCount;
	PxReal dt = getDt();
	PxReal invDt = getInvDt();
	//PxsMaterialInfo materialInfo[physx::ContactBuffer::MAX_CONTACTS];
	PxsThreadContext* threadContext = getContext()->getThreadContext();
	threadContext->mConstraintBlockStream.reset(); //ensure there's no left-over memory that belonged to another island
	//for(PxU32 a = 1; a < totalConstraintCount; ++a)
	for(PxU32 a = 1; a < totalHeaderCount; ++a)
	{
		PxU32 ind = a-1;

		{
			PxsConstraintBatchHeader& nextHeader = headers[a];

			if(contactDescPtr[nextHeader.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
			{
				for(PxU32 i = 0; i < nextHeader.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[nextHeader.mStartIndex + i];
					PxsContactManager *cm = (PxsContactManager*)desc.constraint;
					PxcNpWorkUnit& unit = cm->getWorkUnit();
					Ps::prefetchLine(unit.compressedContacts);
					Ps::prefetchLine(unit.compressedContacts, 128);
				}
			}
			else if(contactDescPtr[nextHeader.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
			{
				for(PxU32 i = 0; i < nextHeader.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[nextHeader.mStartIndex + i];
					PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
					Ps::prefetchLine(constraint->constantBlock);
					Ps::prefetchLine(constraint->constantBlock,128);
					Ps::prefetchLine(constraint->constantBlock,256);
				}
			}
		}
		{
			const PxU32 prefetch = PxMin(a+2u, totalHeaderCount - 1);

			PxsConstraintBatchHeader& nextHeader = headers[prefetch];

			for(PxU32 i = 0; i < nextHeader.mStride; ++i)
			{
				Ps::prefetchLine(contactDescPtr[nextHeader.mStartIndex + i].constraint);
				Ps::prefetchLine(contactDescPtr[nextHeader.mStartIndex + i].constraint, 128);
			}
			Ps::prefetchLine(&contactDescPtr[nextHeader.mStartIndex + nextHeader.mStride]);
			Ps::prefetchLine(&contactDescPtr[nextHeader.mStartIndex + nextHeader.mStride],128);
		}

		PxsConstraintBatchHeader& header = headers[ind];

		if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
		{
#if PXS_BATCH_1D
			PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
			if(header.mStride == 4)
			{
				PxTransform id = PxTransform(PxIdentity);

				PxcSolverConstraint4Desc descs[4];
				for(PxU32 i = 0; i < 4; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
				
					const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
					const void* constantBlock = constraint->constantBlock;
					const PxU32 constantBlockByteSize = constraint->constantBlockSize;
					const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
					const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
					const PxcSolverBody* sbody0 = desc.bodyA;
					const PxcSolverBody* sbody1 = desc.bodyB;
					PxcSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
					PxcSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];


					descs[i].constantBlock = constantBlock;
					descs[i].constantBlockByteSize = constantBlockByteSize;
					descs[i].constraint = constraint;
					descs[i].desc = &desc;
					descs[i].pose0 = &pose0;
					descs[i].pose1 = &pose1;
					descs[i].sBodyData0 = sbodyData0;
					descs[i].sBodyData1 = sbodyData1;
					descs[i].solverBody0 = sbody0;
					descs[i].solverBody1 = sbody1;
					descs[i].solverPrep = solverPrep;
				}

				state = setupSolverConstraint4
					(descs, dt, invDt,
					 threadContext->mConstraintBlockStream, mThreadContext.mConstraintBlockManager);
			}
			if(state == PxcSolverConstraintPrepState::eUNBATCHABLE)
#endif
			{
				for(PxU32 i = 0; i < header.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
					axisConstraintCount +=  PxsSetupSolverConstraint(constraint, solverBodyData, threadContext->mConstraintBlockStream, dt, 
						invDt, desc, mThreadContext.mConstraintBlockManager);
				}
			}
		}
		else if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
		{
			PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
			if(header.mStride == 4)
			{
				PxcSolverConstraintDesc& desc0 = contactDescPtr[header.mStartIndex];
				PxcSolverConstraintDesc& desc1 = contactDescPtr[header.mStartIndex+1];
				PxcSolverConstraintDesc& desc2 = contactDescPtr[header.mStartIndex+2];
				PxcSolverConstraintDesc& desc3 = contactDescPtr[header.mStartIndex+3];

				PxsContactManager *cm0 = (PxsContactManager*)desc0.constraint;
				PxsContactManager *cm1 = (PxsContactManager*)desc1.constraint;
				PxsContactManager *cm2 = (PxsContactManager*)desc2.constraint;
				PxsContactManager *cm3 = (PxsContactManager*)desc3.constraint;


				PxcNpWorkUnit* n[4] = {&cm0->getWorkUnit(), &cm1->getWorkUnit(), &cm2->getWorkUnit(), &cm3->getWorkUnit()};
				PxcSolverConstraintDesc* descs[4] = {&desc0, &desc1, &desc2, &desc3};
				const PxcSolverBody* sBody0[4] = {desc0.bodyA, desc1.bodyA, desc2.bodyA, desc3.bodyA};
				const PxcSolverBody* sBody1[4] = {desc0.bodyB, desc1.bodyB, desc2.bodyB, desc3.bodyB};

				const PxcSolverBodyData* sbd0[4] = {&solverBodyData[desc0.bodyADataIndex], &solverBodyData[desc1.bodyADataIndex], &solverBodyData[desc2.bodyADataIndex],
					&solverBodyData[desc3.bodyADataIndex]};
				const PxcSolverBodyData* sbd1[4] = {&solverBodyData[desc0.bodyBDataIndex], &solverBodyData[desc1.bodyBDataIndex], &solverBodyData[desc2.bodyBDataIndex],
					&solverBodyData[desc3.bodyBDataIndex]};

				const PxsRigidCore* core0[4] = {n[0]->rigidCore0, n[1]->rigidCore0, n[2]->rigidCore0, n[3]->rigidCore0};
				const PxsRigidCore* core1[4] = {n[0]->rigidCore1, n[1]->rigidCore1, n[2]->rigidCore1, n[3]->rigidCore1};


				state = createFinalizeMethods4[frictionType](n, *threadContext,
					 descs,
					 sBody0,
					 sBody1,
					 sbd0,
					 sbd1,
					 core0,
					 core1,
					 invDt,
					 mBounceThreshold,
					 mFrictionOffsetThreshold,
					 mThreadContext.mCorrelationDistance,
					 mThreadContext.mConstraintBlockManager,
					 frictionType);

			}
			if(PxcSolverConstraintPrepState::eUNBATCHABLE == state)
			{
				for(PxU32 i = 0; i < header.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsContactManager& cm = *(PxsContactManager*)desc.constraint;
					PxcNpWorkUnit& n = cm.getWorkUnit();
					PxcSolverBodyData& data0 = desc.linkIndexA != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
					PxcSolverBodyData& data1 = desc.linkIndexB != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];

					createFinalizeMethods[frictionType](n, *threadContext, false, false,
						desc, desc.bodyA, desc.bodyB, &data0, &data1, n.rigidCore0, n.rigidCore1, mInvDt, mBounceThreshold, 
						mFrictionOffsetThreshold, mThreadContext.mCorrelationDistance, mThreadContext.mConstraintBlockManager, frictionType);
					axisConstraintCount += cm.getConstraintDesc(desc);
				}
			}
		}
	}

	if(totalHeaderCount)
	{
		PxU32 ind = totalHeaderCount-1;
		
		PxsConstraintBatchHeader& header = headers[ind];

		if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_1D)
		{
#if PXS_BATCH_1D
			PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
			if(header.mStride == 4)
			{
				PxTransform id = PxTransform(PxIdentity);

				PxcSolverConstraint4Desc descs[4];
				for(PxU32 i = 0; i < 4; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
				
					const PxConstraintSolverPrep solverPrep = constraint->solverPrep;
					const void* constantBlock = constraint->constantBlock;
					const PxU32 constantBlockByteSize = constraint->constantBlockSize;
					const PxTransform& pose0 = (constraint->body0 ? constraint->body0->getPose() : id);
					const PxTransform& pose1 = (constraint->body1 ? constraint->body1->getPose() : id);
					const PxcSolverBody* sbody0 = desc.bodyA;
					const PxcSolverBody* sbody1 = desc.bodyB;
					PxcSolverBodyData* sbodyData0 = &solverBodyData[desc.linkIndexA != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyADataIndex];
					PxcSolverBodyData* sbodyData1 = &solverBodyData[desc.linkIndexB != PxcSolverConstraintDesc::NO_LINK ? 0 : desc.bodyBDataIndex];


					descs[i].constantBlock = constantBlock;
					descs[i].constantBlockByteSize = constantBlockByteSize;
					descs[i].constraint = constraint;
					descs[i].desc = &desc;
					descs[i].pose0 = &pose0;
					descs[i].pose1 = &pose1;
					descs[i].sBodyData0 = sbodyData0;
					descs[i].sBodyData1 = sbodyData1;
					descs[i].solverBody0 = sbody0;
					descs[i].solverBody1 = sbody1;
					descs[i].solverPrep = solverPrep;
				}

				state = setupSolverConstraint4
					(descs, dt, invDt,
					 threadContext->mConstraintBlockStream, mThreadContext.mConstraintBlockManager);
			}
			if(state == PxcSolverConstraintPrepState::eUNBATCHABLE)
#endif
			{
				for(PxU32 i = 0; i < header.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsConstraint* constraint = (PxsConstraint*)desc.constraint;
					axisConstraintCount +=  PxsSetupSolverConstraint(constraint, solverBodyData, threadContext->mConstraintBlockStream, dt, 
					invDt, desc, mThreadContext.mConstraintBlockManager);
				}
			}
		}
		else if(contactDescPtr[header.mStartIndex].constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT)
		{
			PxcSolverConstraintPrepState::Enum state = PxcSolverConstraintPrepState::eUNBATCHABLE;
			if(header.mStride == 4)
			{
				PxcSolverConstraintDesc& desc0 = contactDescPtr[header.mStartIndex];
				PxcSolverConstraintDesc& desc1 = contactDescPtr[header.mStartIndex+1];
				PxcSolverConstraintDesc& desc2 = contactDescPtr[header.mStartIndex+2];
				PxcSolverConstraintDesc& desc3 = contactDescPtr[header.mStartIndex+3];

				PxsContactManager *cm0 = (PxsContactManager*)desc0.constraint;
				PxsContactManager *cm1 = (PxsContactManager*)desc1.constraint;
				PxsContactManager *cm2 = (PxsContactManager*)desc2.constraint;
				PxsContactManager *cm3 = (PxsContactManager*)desc3.constraint;


				PxcNpWorkUnit* n[4] = {&cm0->getWorkUnit(), &cm1->getWorkUnit(), &cm2->getWorkUnit(), &cm3->getWorkUnit()};
				PxcSolverConstraintDesc* descs[4] = {&desc0, &desc1, &desc2, &desc3};
				const PxcSolverBody* sBody0[4] = {desc0.bodyA, desc1.bodyA, desc2.bodyA, desc3.bodyA};
				const PxcSolverBody* sBody1[4] = {desc0.bodyB, desc1.bodyB, desc2.bodyB, desc3.bodyB};

				const PxcSolverBodyData* sbd0[4] = {&solverBodyData[desc0.bodyADataIndex], &solverBodyData[desc1.bodyADataIndex], &solverBodyData[desc2.bodyADataIndex],
					&solverBodyData[desc3.bodyADataIndex]};
				const PxcSolverBodyData* sbd1[4] = {&solverBodyData[desc0.bodyBDataIndex], &solverBodyData[desc1.bodyBDataIndex], &solverBodyData[desc2.bodyBDataIndex],
					&solverBodyData[desc3.bodyBDataIndex]};

				const PxsRigidCore* core0[4] = {n[0]->rigidCore0, n[1]->rigidCore0, n[2]->rigidCore0, n[3]->rigidCore0};
				const PxsRigidCore* core1[4] = {n[0]->rigidCore1, n[1]->rigidCore1, n[2]->rigidCore1, n[3]->rigidCore1};


				state = createFinalizeMethods4[frictionType](n, *threadContext,
					 descs,
					 sBody0,
					 sBody1,
					 sbd0,
					 sbd1,
					 core0,
					 core1,
					 invDt,
					 mBounceThreshold,
					 mFrictionOffsetThreshold,
					 mThreadContext.mCorrelationDistance,
					 mThreadContext.mConstraintBlockManager,
					 frictionType);

			}
			if(PxcSolverConstraintPrepState::eUNBATCHABLE == state)
			{
				for(PxU32 i = 0; i < header.mStride; ++i)
				{
					PxcSolverConstraintDesc& desc = contactDescPtr[header.mStartIndex+i];
					PxsContactManager& cm = *(PxsContactManager*)desc.constraint;
					PxcNpWorkUnit& n = cm.getWorkUnit();
					PxcSolverBodyData& data0 = desc.linkIndexA != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyADataIndex];
					PxcSolverBodyData& data1 = desc.linkIndexB != 0xffff ? solverBodyData[0] : solverBodyData[desc.bodyBDataIndex];

					createFinalizeMethods[frictionType](n, *threadContext, false, false,
						desc, desc.bodyA, desc.bodyB, &data0, &data1, n.rigidCore0, n.rigidCore1, mInvDt, mBounceThreshold, 
						mFrictionOffsetThreshold, mThreadContext.mCorrelationDistance, mThreadContext.mConstraintBlockManager, frictionType);
					axisConstraintCount += cm.getConstraintDesc(desc);
				}
			}
		}
	}

	getContext()->putThreadContext(threadContext);
	mThreadContext.mAxisConstraintCount = axisConstraintCount;
}

void physx::PxsSolverCreateFinalizeConstraintsTask::run()
{
#ifdef PX_PS3
	stopTimerMarker(eCONSTRAINT_PARTITION);
	startTimerMarker(eSETUPSOLVERCONSTRAINT);
#endif 

#ifdef PX_PS3
	const PxU32 numSpusToUse = mContext.getContext()->getSceneParamInt(PxPS3ConfigParam::eSPU_DYNAMICS);
#endif

	//PIX_PROFILE_ZONE(PxsSolverFinalizeConstriantsTask);
	
	PxU32 descCount = mThreadContext.mNumDifferentBodyConstraints;//(mThreadContext.mContactDescPtr - mThreadContext.contactConstraintDescArray.begin()) + mCounts.constraints + mCounts.contactManagers;
	PxU32 selfConstraintDescCount = mThreadContext.contactConstraintDescArray.size() - mThreadContext.mNumDifferentBodyConstraints;

#if (PX_CONSTRAINT_PARTITIONING || SPU_CONSTRAINT_PARTITIONING)
	mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(0);
	mThreadContext.contactConstraintBatchHeaders.reserve(descCount + selfConstraintDescCount);
	mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(descCount + selfConstraintDescCount);

	PxU32 accumulatedConstraintsPerPartition[33];
	PxU32 accumulation = 0;
	for(PxU32 k = 0; k < 33; ++k)
	{
		accumulation += mThreadContext.mConstraintsPerPartition[k];
		accumulatedConstraintsPerPartition[k] = accumulation;
	}

	PxU32 numHeaders = 0;
	PxU32 currentPartition = 0;
	PxU32 maxJ = accumulatedConstraintsPerPartition[0];

#ifdef PX_PS3
	const PxU32 maxBatchPartition = 31;
#else
	const PxU32 maxBatchPartition = 32;
#endif

	for(PxU32 a = 0; a < descCount;)
	{
		PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[numHeaders++];
		header.mStartIndex = a;

		PxU16 j = 1;
		PxU32 loopMax = PxMin(maxJ - a, 4u);
		//PxU32 loopMax = PxMin(maxJ - a, 1u);
		PxcSolverConstraintDesc& desc = mThreadContext.orderedContactConstraints[a];
		if(!isArticulationConstraint(desc) && (desc.constraintLengthOver16 == PXS_SC_TYPE_RB_CONTACT || 
			desc.constraintLengthOver16 == PXS_SC_TYPE_RB_1D) && currentPartition < maxBatchPartition)
		{
			for(; j < loopMax && desc.constraintLengthOver16 == mThreadContext.orderedContactConstraints[a+j].constraintLengthOver16 && 
				!isArticulationConstraint(mThreadContext.orderedContactConstraints[a+j]); ++j);
		}
		header.mStride = j;
		if(maxJ == (a + j) && currentPartition < 32)
		{
			//Go to next partition!
			currentPartition++;
			maxJ = accumulatedConstraintsPerPartition[currentPartition];
		}
		a+= j;
	}

	PxU32 numDifferentBodyBatchHeaders = numHeaders;

	for(PxU32 a = 0; a < selfConstraintDescCount; ++a)
	{
		PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[numHeaders++];
		header.mStartIndex = a + descCount;
		header.mStride = 1;
	}

	PxU32 numSelfConstraintBatchHeaders = numHeaders - numDifferentBodyBatchHeaders;

	mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(numHeaders);
	mThreadContext.numDifferentBodyBatchHeaders = numDifferentBodyBatchHeaders;
	mThreadContext.numSelfConstraintBatchHeaders = numSelfConstraintBatchHeaders;
#else
	//We create the batch header array but just allowing a single constraint per-batch
	mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(0);
	mThreadContext.contactConstraintBatchHeaders.reserve(descCount);
	mThreadContext.contactConstraintBatchHeaders.forceSize_Unsafe(descCount);
	PxU32 numHeaders = 0;
	for(PxU32 a = 0; a < descCount;++a)
	{
		PxsConstraintBatchHeader& header = mThreadContext.contactConstraintBatchHeaders[numHeaders++];
		header.mStartIndex = a;
		header.mStride = 1;
	}

	mThreadContext.numDifferentBodyBatchHeaders = numHeaders;
	mThreadContext.numSelfConstraintBatchHeaders = 0;

#endif

	PX_UNUSED(descCount);

#if !(PX_CONSTRAINT_PARTITIONING ||SPU_CONSTRAINT_PARTITIONING)  //If we didn't partition already
	//We won't have created the ordered constraint desc array, so do that now

#if defined(PX_PS3) && defined(SPU_CONSTRAINT_PARTITIONING)
	if(numSpusToUse == 0 || !mThreadContext.mSuccessfulSpuConstraintPartition)
#endif
	{
		mThreadContext.orderedContactConstraints.forceSize_Unsafe(0);
		mThreadContext.orderedContactConstraints.reserve(descCount + 3);
		mThreadContext.orderedContactConstraints.forceSize_Unsafe(descCount + 3);

		PxMemCopy(mThreadContext.orderedContactConstraints.begin(), mThreadContext.contactConstraintDescArray.begin(), descCount*sizeof(PxcSolverConstraintDesc));
	}
#endif


#if defined PX_PS3 && SPU_CONSTRAINT_SHADER
	
	if(numSpusToUse)
	{
		//We want to kick off the constraint shaders here...
		PxsSetupConstraintSpuTask* spuTask = (PxsSetupConstraintSpuTask*)mContext.getContext()->getTaskPool().allocate(sizeof(PxsSetupConstraintSpuTask), 128);

		PX_PLACEMENT_NEW(spuTask, PxsSetupConstraintSpuTask(mContext, mThreadContext, 
		mThreadContext.orderedContactConstraints.begin(),
		descCount,
		selfConstraintDescCount,
		numSpusToUse,
		mThreadContext.contactConstraintBatchHeaders.begin(),
		numHeaders));

		spuTask->setContinuation(mCont);
		spuTask->removeReference();
	}
	else
#endif
	{

		PxcSolverConstraintDesc* descBegin = mThreadContext.orderedContactConstraints.begin();

		const PxU32 taskBlockSize = 16;

		const PxU32 taskCount = (numHeaders/taskBlockSize)+1;

		const PxU32 maxTasks = getTaskManager()->getCpuDispatcher()->getWorkerCount();

		const PxU32 numTasks = PxMin(maxTasks, taskCount);

		//mThreadContext.mConstraintBlockManager.reset(); //Don't do here - we do in the articulations task instead!

		
		if(numTasks > 1)
		{
			PxI32* pIndex = (PxI32*)mContext.getContext()->getTaskPool().allocate(sizeof(PxI32));
			*pIndex = 0;
			PxsCreateFinalizeTask* tasks = (PxsCreateFinalizeTask*)mContext.getContext()->getTaskPool().allocate(sizeof(PxsCreateFinalizeTask) * (numTasks-1));
			for(PxU32 a = 1; a < numTasks; ++a)
			{

				PX_PLACEMENT_NEW(&tasks[a-1], PxsCreateFinalizeTask(mThreadContext.mFrictionType, mContext.getDt(), mContext.getInvDt(), mContext.getBounceThreshold(), mContext.getFrictionOffsetThreshold(),
					descCount, descBegin, pIndex, (PxI32*)&mThreadContext.mAxisConstraintCount, mContext.mSolverBodyDataPool.begin(), mThreadContext, mContext.getContext()));

				tasks[a-1].setContinuation(mCont);
				tasks[a-1].removeReference();
			}

			createFinalizeContacts_Parallel(mThreadContext.mFrictionType, descBegin, mContext.mSolverBodyDataPool.begin(), descCount, mThreadContext, pIndex, 
				mContext.getDt(), mContext.getInvDt(), mContext.getBounceThreshold(), mContext.getFrictionOffsetThreshold(), mContext.getContext(), (PxI32*)&mThreadContext.mAxisConstraintCount);

		}
		else
		{
			mContext.createFinalizeContacts(mThreadContext.mFrictionType, descBegin, 
				mContext.mSolverBodyDataPool.begin(), descCount, mCont, mThreadContext);
		}
	}
}



