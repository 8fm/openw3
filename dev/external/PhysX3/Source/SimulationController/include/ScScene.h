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


#ifndef PX_PHYSICS_SCP_SCENE
#define PX_PHYSICS_SCP_SCENE

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#ifdef PX_PS3
#include "ps3/PxPS3Config.h"
#endif
#include "PxScene.h"
#include "PxSceneDesc.h"
#include "PxSimulationEventCallback.h"
#include "PsHashMap.h"
#include "PsPool.h"
#include "CmRenderOutput.h"
#include "CmReaderWriterLock.h"
#include "CmTask.h"
#include "CmFlushPool.h"
#include "CmPreallocatingPool.h"
#include "CmEventProfiler.h"
#include "CmUtils.h"
#include "CmBitMap.h"
#include "ScIterators.h"
#include "PxvContext.h"
#include "PxsMaterialManager.h"

#define EXTRA_PROFILING 0
#define PX_MAX_DOMINANCE_GROUP 32


#if EXTRA_PROFILING
#include <cstdio>
#endif

namespace physx
{

class PxSceneGpu;
class PxBaseTask;
struct PxTriggerPair;

class AABBMgrId;

enum SceneInternalFlag
{
	SCENE_SIP_STATES_DIRTY_DOMINANCE		= (1<<1),
	SCENE_SIP_STATES_DIRTY_VISUALIZATION	= (1<<2),
	SCENE_DEFAULT							= 0
};


#if PX_USE_CLOTH_API
namespace cloth
{
	class Solver;
	class Factory;
}
#endif

namespace Sc
{
           
	class ShapeCore;
	class RigidCore;
	class StaticCore;
	class ConstraintCore;
	class MaterialCore;
	class ArticulationCore;
	class ArticulationJointCore;
	class ClothCore;
	class ParticleSystemCore;

	class NPhaseCore;
	class LowLevelThreadingThunk;
	class Client;
	class ConstraintInteraction;

	class BodySim;
	class ShapeSim;
	class RigidSim;
	class StaticSim;
	class BodySim;
	class ConstraintSim;
	class ConstraintProjectionManager;
	struct TriggerPairExtraData;
	class ObjectIDTracker;

#if PX_USE_PARTICLE_SYSTEM_API
	class ParticleSystemSim;
	class ParticlePacketShape;
#endif

#if PX_USE_CLOTH_API
	class ClothShape;
#endif

	class SimStats;
	class InteractionScene;

	struct SimStateData;

	struct BatchInsertionState
	{
		BodySim*  bodySim;
		StaticSim*staticSim; 
		ShapeSim* shapeSim;
		ptrdiff_t staticActorOffset;
		ptrdiff_t staticShapeTableOffset;
		ptrdiff_t dynamicActorOffset;
		ptrdiff_t dynamicShapeTableOffset;
		ptrdiff_t shapeOffset;
	};

	struct BatchRemoveState
	{
		Ps::InlineArray<Sc::ShapeSim*, 64>				bufferedShapes;
		Ps::InlineArray<const Sc::ShapeCore*, 64>		removedShapes;
	};


	class Scene : public Ps::UserAllocated
	{
		struct SimpleBodyPair
		{
			BodySim* body1;
			BodySim* body2;
			PxU32 body1ID;
			PxU32 body2ID;
		};



		//---------------------------------------------------------------------------------
		// External interface
		//---------------------------------------------------------------------------------
	public:    
   
					void						release();

	PX_FORCE_INLINE	void						setGravity(const PxVec3& g)			{ mGravity = g;	mBodyGravityDirty = true;	}
	PX_FORCE_INLINE	PxVec3						getGravity()				const	{ return mGravity;							}
	PX_FORCE_INLINE void						setElapsedTime(const PxReal t)		{ mDt = t; mOneOverDt = t > 0.f ? 1.f/t : 0.f;}

					void						setBounceThresholdVelocity(const PxReal t);
					PxReal						getBounceThresholdVelocity() const;

	PX_FORCE_INLINE	void						setPublicFlags(PxSceneFlags flags)	{ mPublicFlags = flags;						}
	PX_FORCE_INLINE	PxSceneFlags				getPublicFlags()			const	{ return mPublicFlags;						}

					void						setFrictionModel(PxsFrictionModel model);
					PxsFrictionModel 			getFrictionModel() const;
					void						setPCM(bool enabled);
					void						setContactCache(bool enabled);

					void						addStatic(StaticCore&, void*const *shapes, PxU32 nbShapes, size_t shapePtrOffset);
					void						removeStatic(StaticCore&, Ps::InlineArray<const Sc::ShapeCore*,64>& removedShapes, bool wakeOnLostTouch);

					void						addBody(BodyCore&, void*const *shapes, PxU32 nbShapes, size_t shapePtrOffset);
					void						removeBody(BodyCore&, Ps::InlineArray<const Sc::ShapeCore*,64>& removedShapes, bool wakeOnLostTouch);

					// Batch insertion API.
					// the bounds generated here are the uninflated bounds for the shapes, *if* they are trigger or sim shapes. 
					// It's up to the caller to ensure the bounds array is big enough.
					// Some care is required in handling these since sim and SQ tweak the bounds in slightly different ways.

					void						startBatchInsertion(BatchInsertionState&);
					void						addStatic(PxActor* actor, BatchInsertionState&, PxBounds3* outBounds);
					void						addBody(PxActor* actor, BatchInsertionState&, PxBounds3* outBounds);
					void						finishBatchInsertion(BatchInsertionState&);

					// Batch remove helpers
					void						setBatchRemove(BatchRemoveState* bs) { mBatchRemoveState = bs; }
					BatchRemoveState*			getBatchRemove() const { return mBatchRemoveState; }
					void						prefetchForRemove(const BodyCore& ) const;
					void						prefetchForRemove(const StaticCore& ) const;

					void						addConstraint(ConstraintCore&, RigidCore*, RigidCore*);
					void						removeConstraint(ConstraintCore&);

					void						addArticulation(ArticulationCore&, BodyCore& root);
					void						removeArticulation(ArticulationCore&);

					void						addArticulationJoint(ArticulationJointCore&, BodyCore& parent, BodyCore& child);
					void						removeArticulationJoint(ArticulationJointCore&);

#if PX_USE_PARTICLE_SYSTEM_API
					void						addParticleSystem(ParticleSystemCore&);
					void						removeParticleSystem(ParticleSystemCore&, bool isRelease);

					PxU32						getNbParticleSystems() const;
					ParticleSystemCore**		getParticleSystems();
#endif
					bool						hasParticleSystems() const;

#if PX_USE_CLOTH_API
					bool						addCloth(ClothCore&);
					void						removeCloth(ClothCore&);
#endif
					bool						hasCloths() const;

					PxU32						getNbArticulations() const;
					Sc::ArticulationCore**		getArticulations();

					PxU32						getNbConstraints() const;
					Sc::ConstraintCore**		getConstraints();

					void						initActiveBodiesIterator(BodyIterator&);					
					void						initContactsIterator(ContactIterator&);

		// Simulation events
					void						setSimulationEventCallback(PxSimulationEventCallback* callback, PxClientID client);
					PxSimulationEventCallback*	getSimulationEventCallback(PxClientID client) const;

		// Contact modification
					void						setContactModifyCallback(PxContactModifyCallback* callback);
					PxContactModifyCallback*	getContactModifyCallback() const;
					void						setCCDContactModifyCallback(PxCCDContactModifyCallback* callback);
					PxCCDContactModifyCallback*	getCCDContactModifyCallback() const;

					void						setCCDMaxPasses(PxU32 ccdMaxPasses);
					PxU32						getCCDMaxPasses() const;

		// Broad-phase callback
					void						setBroadPhaseCallback(PxBroadPhaseCallback* callback, PxClientID client);
					PxBroadPhaseCallback*		getBroadPhaseCallback(PxClientID client)	const;

		// Broad-phase management
					void						addBroadPhaseVolume(const PxBounds3& bounds, ShapeSim& shapeSim);
					void						addBroadPhaseVolume(const PxBounds3& bounds, const PxU32 actorGroup, const AABBMgrId& aabbMgrId, ShapeSim& shapeSim);
#if PX_USE_PARTICLE_SYSTEM_API
					void						addBroadPhaseVolume(ParticlePacketShape& particlePacketShape);
					void						removeBroadPhaseVolume(ParticlePacketShape& particlePacketShape);
#endif
#if PX_USE_CLOTH_API
					void						addBroadPhaseVolume(ClothShape& clothShape);
					void						removeBroadPhaseVolume(ClothShape& clothShape);
#endif

					void						removeBroadPhaseVolume(const bool wakeOnLostTouch, ShapeSim& shapeSim);

					void						startBroadPhase();
					void						finishBroadPhase(const bool secondaryBroadphase);

		// Groups
					void						setDominanceGroupPair(PxDominanceGroup group1, PxDominanceGroup group2, const PxDominanceGroupPair& dominance);
					PxDominanceGroupPair		getDominanceGroupPair(PxDominanceGroup group1, PxDominanceGroup group2) const;

					void						setSolverBatchSize(PxU32 solverBatchSize);
					PxU32						getSolverBatchSize() const;

					void						setVisualizationParameter(PxVisualizationParameter::Enum param, PxReal value);
					PxReal						getVisualizationParameter(PxVisualizationParameter::Enum param) const;

					void						setVisualizationCullingBox(const PxBounds3& box);
					const PxBounds3&			getVisualizationCullingBox() const;

		// Run
					void						simulate(PxReal timeStep, PxBaseTask* continuation);
					void						solve(PxReal timeStep, PxBaseTask* continuation);
					void						collide(PxReal timeStep, PxBaseTask* continuation);
					void						endSimulation();
					void						flush(bool sendPendingReports);
					void						fireCallBacksPreSync();
					void						fireCallBacksPostSync();
					bool						fireOutOfBoundsCallbacks();
					void						prepareOutOfBoundsCallbacks();
					void						postReportsCleanup();
					PxU32						getErrorState();

					void						setMeshContactMargin(PxReal contactMargin);
					PxReal						getMeshContactMargin() const;

					PxU32						getDefaultContactReportStreamBufferSize() const;

					PxReal						getFrictionOffsetThreshold() const;

					void						setLimits(const PxSceneLimits& limits);
					const PxSceneLimits&		getLimits() const;

					void						visualizeStartStep();
					void						visualizeEndStep();
					Cm::RenderBuffer&			getRenderBuffer();

					void						setNbContactDataBlocks(PxU32 blockCount);
					PxU32						getNbContactDataBlocksUsed() const;
					PxU32						getMaxNbContactDataBlocksUsed() const;
					PxU32						getMaxNbConstraintDataBlocksUsed() const;

					void						setScratchBlock(void* addr, PxU32 size);

// PX_ENABLE_SIM_STATS
					void						getStats(PxSimulationStatistics& stats) const;
		PX_INLINE	SimStats&					getStatsInternal() { return *mStats; }
		// PX_ENABLE_SIM_STATS

					void						buildActiveTransforms();
					PxActiveTransform*			getActiveTransforms(PxU32& nbTransformsOut, PxClientID client);

					PxClientID					createClient();
					void						setClientBehaviorFlags(PxClientID client, PxClientBehaviorFlags clientBehaviorFlags); 
					PxClientBehaviorFlags		getClientBehaviorFlags(PxClientID client) const;

#if PX_USE_CLOTH_API
					void						setClothInterCollisionDistance(PxF32 distance);
					PxF32						getClothInterCollisionDistance() const;
					void						setClothInterCollisionStiffness(PxF32 stiffness); 
					PxF32						getClothInterCollisionStiffness() const;
					void						setClothInterCollisionNbIterations(PxU32 nbIterations);
					PxU32						getClothInterCollisionNbIterations() const;
					void						createClothSolver();
#endif

					PxSceneGpu*					getSceneGpu();

#ifdef PX_PS3
					void						setSceneParamInt(PxPS3ConfigParam::Enum param, PxU32 value);
					PxU32						getSceneParamInt(PxPS3ConfigParam::Enum param);
					void						getSpuMemBlockCounters(PxU32& numContactStreamBlocks, PxU32& numFrictionBlocks, PxU32& numConstraintBlocks);
#endif
 
					PxTaskManager*		getTaskManagerPtr() const { return mTaskManager; }

					void						shiftOrigin(const PxVec3& shift);

					Ps::Pool<SimStateData>*		getSimStateDataPool() { return mSimStateDataPool; }

		//---------------------------------------------------------------------------------
		// Miscellaneous
		//---------------------------------------------------------------------------------							
		//internal public methods:
	public:
					void						addShape(RigidSim&, ShapeCore&);
					void						removeShape(ShapeSim&, bool wakeOnLostTouch);

												Scene(const PxSceneDesc& desc, Cm::EventProfiler& eventBuffer);
												~Scene() {}	//use release() plz.

					void						preAllocate(PxU32 nbStatics, PxU32 nbBodies, PxU32 nbStaticShapes, PxU32 nbDynamicShapes);

					void						addBrokenConstraint(Sc::ConstraintCore*);
					void						addActiveBreakableConstraint(Sc::ConstraintSim*);
					void						removeActiveBreakableConstraint(Sc::ConstraintSim*);
		//the Actor should register its top level shapes with these.
					void						removeBody(BodySim&);

					void						raiseSceneFlag(SceneInternalFlag flag) { mInternalFlags |= flag; }

					//lists of actors woken up or put to sleep last simulate
					void                        onBodyWakeUp(BodySim* body);
					void                        onBodySleep(BodySim* body);

					bool						isValid() const;

					void						addToLostTouchList(BodySim* body1, BodySim* body2);

					void						initDominanceMatrix();

					PxDominanceGroupPair		getDominanceGroupPairFast(PxDominanceGroup group1, PxDominanceGroup group2) const;
					
					void						setCreateContactReports(bool s);

// PX_AGGREGATE
					PxU32						createCompound(void* userData, bool selfCollisions);
					void						deleteCompound(PxU32 id);
					bool						getCompoundSelfCollisions(PxU32 id);
//~PX_AGGREGATE

					Ps::Pool<ConstraintInteraction>*
													getConstraintInteractionPool()			const	{ return mConstraintInteractionPool;	}
	public:
						PxBroadPhaseType::Enum		getBroadPhaseType()																				const;
						bool						getBroadPhaseCaps(PxBroadPhaseCaps& caps)														const;
						PxU32						getNbBroadPhaseRegions()																		const;
						PxU32						getBroadPhaseRegions(PxBroadPhaseRegionInfo* userBuffer, PxU32 bufferSize, PxU32 startIndex)	const;
						PxU32						addBroadPhaseRegion(const PxBroadPhaseRegion& region, bool populateRegion);
						bool						removeBroadPhaseRegion(PxU32 handle);
						Ps::Array<void*>&			getOutOfBoundsObjects();
						Ps::Array<void*>&			getOutOfBoundsAggregates();

		PX_FORCE_INLINE	const PxsMaterialManager*	getMaterialManager()					const	{ return &mMaterialManager;				}
		PX_FORCE_INLINE	PxsMaterialManager*			getMaterialManager()							{ return &mMaterialManager;				}

		// Collision filtering
		PX_FORCE_INLINE	const void*					getFilterShaderDataFast()				const	{ return mFilterShaderData;				}
		PX_FORCE_INLINE	PxU32						getFilterShaderDataSizeFast()			const	{ return mFilterShaderDataSize;			}
		PX_FORCE_INLINE	PxSimulationFilterShader	getFilterShaderFast()					const	{ return mFilterShader;					}
		PX_FORCE_INLINE	PxSimulationFilterCallback*	getFilterCallbackFast()					const	{ return mFilterCallback;				}

		PX_FORCE_INLINE	PxReal						getGlobalTime()							const	{ return mGlobalTime;					}
		PX_FORCE_INLINE	PxU32						getTimeStamp()							const	{ return mTimeStamp;					}
		PX_FORCE_INLINE	PxU32						getReportShapePairTimeStamp()			const	{ return mReportShapePairTimeStamp;		}

		PX_FORCE_INLINE	PxReal						getOneOverDt()							const	{ return mOneOverDt;					}
		PX_FORCE_INLINE	PxReal						getDt()									const	{ return mDt;							}
						PxReal						getContactCorrelationDistance()			const;

		PX_FORCE_INLINE	const PxVec3&				getGravityFast()						const	{ return mGravity;						}
		PX_FORCE_INLINE	bool						readFlag(SceneInternalFlag flag)		const	{ return (mInternalFlags & flag) != 0;	}
		PX_FORCE_INLINE	bool						readPublicFlag(PxSceneFlag::Enum flag)	const	{ return (mPublicFlags & flag);			}

		PX_FORCE_INLINE	NPhaseCore*					getNPhaseCore()							const	{ return mNPhaseCore;					}
		PX_FORCE_INLINE	Cm::EventProfiler&			getEventProfiler()								{ return mEventProfiler;				}

						void						checkConstraintBreakage();

		PX_FORCE_INLINE	Ps::Array<TriggerPairExtraData>&		
													getTriggerBufferExtraData()						{ return *mTriggerBufferExtraData;		}
		PX_FORCE_INLINE	Ps::Array<PxTriggerPair>&	getTriggerBufferAPI()							{ return mTriggerBufferAPI;				}
		PX_FORCE_INLINE void						onDeletedShapeInTriggerPair()					{ mCheckForDeletedShapesInTriggerPairs = true; }

		PX_FORCE_INLINE	ObjectIDTracker&			getRigidIDTracker()								{ return *mRigidIDTracker;				}
		PX_FORCE_INLINE	ObjectIDTracker&			getShapeIDTracker()								{ return *mShapeIDTracker;				}

		PX_FORCE_INLINE	void						markReleasedBodyIDForLostTouch(PxU32 id)		{ mLostTouchPairsDeletedBodyIDs.growAndSet(id); }
						void						resizeReleasedBodyIDMaps(PxU32 maxActors, PxU32 numActors);

		PX_FORCE_INLINE	const InteractionScene&		getInteractionScene()					const	{ return *mInteractionScene;			}
		PX_FORCE_INLINE	InteractionScene&			getInteractionScene()							{ return *mInteractionScene;			}

		PX_FORCE_INLINE	StaticSim&					getStaticAnchor()								{ return *mStaticAnchor;				}

		PX_FORCE_INLINE	ConstraintProjectionManager& getProjectionManager()							{ return *mProjectionManager;			}

		PX_FORCE_INLINE	PxReal						getVisualizationScale()					const	{ return mVisualizationScale;			}  // This is actually redundant but makes checks whether debug visualization is enabled faster

		PX_FORCE_INLINE Ps::Array<BodyCore*>&		getSleepBodiesArray()							{ return mSleepBodies;					}

		PX_FORCE_INLINE PxTaskManager&		getTaskManager()						const	{ PX_ASSERT(mTaskManager); return *mTaskManager; }

									void*			allocateConstraintBlock(PxU32 size);
									void			deallocateConstraintBlock(void* addr, PxU32 size);

					void							releaseConstraints();
					void							stepSetupCollide();//This is very important to guarantee thread safty in the collide

		//internal private methods:
	private:
		PX_INLINE	void						clearBrokenConstraintBuffer();

		/////////////////////////////////////////////////////////////

					void						prepareSimulate();

					void						collideStep(PxBaseTask* continuation);
					void						solveStep(PxBaseTask* continuation);

		// subroutines of collideStep/solveStep:
					void						kinematicsSetup();
					void						stepSetupSolve();
					void						stepSetupSimulate();
					void						afterGenerateIslands();
					void						beforeSolver();
					void						afterSolver();
					void						afterIntegration();
					void						processCallbacks(bool pendingReportsOnly = false);
					void						endStep();

	private:
					PxBaseTask&			scheduleCloth(PxBaseTask& continuation, bool afterBroadPhase);
					void						scheduleClothGpu(PxBaseTask& continuation);
					PxBaseTask&			scheduleParticleShapeGeneration(PxBaseTask& broadPhaseDependent,
																				PxBaseTask& dynamicsCpuDependent);
					PxBaseTask&			scheduleParticleDynamicsCpu(PxBaseTask& continuation);
					PxBaseTask&			scheduleParticleCollisionPrep(PxBaseTask& collisionCpuDependent,
																				PxBaseTask& gpuDependent);
					PxBaseTask&			scheduleParticleCollisionCpu(PxBaseTask& continuation);
					PxBaseTask&			scheduleParticleGpu(PxBaseTask& continuation);

					void						prepareParticleSystems();
					void						finishParticleSystems();

					void						fireQueuedContactCallbacks(bool asPartOfFlush);
					void						fireTriggerCallbacks();
					void						fireBrokenConstraintCallbacks();

					void						updateFromVisualizationParameters();

					void						clearSleepWakeBodies(void);
		PX_INLINE	void						cleanUpSleepBodies();
		PX_INLINE	void						cleanUpWokenBodies();
		PX_INLINE	void						cleanUpSleepOrWokenBodies(Ps::Array<BodyCore*>& bodyList, PxU32 removeFlag, bool& validMarker);


		//internal variables:
	private:
					PxSceneLimits				mLimits;

					PxVec3						mGravity;			//!< Gravity vector
					PxU32						mBodyGravityDirty; // Set to true before body->updateForces() when the mGravity has changed					

		//time:
		//constants set with setTiming():
					PxReal						mDt;						//delta time for current step.
					PxReal						mOneOverDt;					//inverse of dt.
		//stepping / counters:
					PxReal						mGlobalTime;				//Accumulated time. For debugging.
					PxU32						mTimeStamp;					//Counts number of steps.
					PxU32						mReportShapePairTimeStamp;	//Timestamp for refreshing the shape pair report structure. Updated before delayed shape/actor deletion and before CCD passes.
		//containers:
		// Those ones contain shape ptrs from Actor, i.e. compound level, not subparts

					Ps::Array<Sc::ConstraintCore*> 
												mConstraintArray;
												
					Sc::ConstraintProjectionManager* mProjectionManager;

					Ps::Array<PxTriggerPair>	mTriggerBufferAPI;
					Ps::Array<TriggerPairExtraData>*		
												mTriggerBufferExtraData;
					bool						mCheckForDeletedShapesInTriggerPairs;
					// Marks if trigger pairs need to be checked for deleted shapes. Necessary because:
					// - Only the first deleted shape can be detected when the trigger interaction is removed but at a later point the second shape might get deleted too
					// - Delayed shape deletion due to double buffering can cause previous trigger pair events to contain deleted shapes

					Ps::Array<ArticulationCore*>mArticulations;

#if PX_USE_PARTICLE_SYSTEM_API
					Ps::Array<ParticleSystemCore*>	mParticleSystems;
					Ps::Array<ParticleSystemSim*>	mEnabledParticleSystems;	// List of enabled particle systems updated every simulation step.
#endif

#if PX_USE_CLOTH_API
					cloth::Solver*				mClothSolver;
					Ps::Array<ClothCore*>		mCloths;
#endif

					Ps::Array<Sc::ConstraintCore*>	mBrokenConstraints;
					Ps::Array<Sc::ConstraintSim*>	mActiveBreakableConstraints;

					// pools for joint buffers
					// Fixed joint is 92 bytes, D6 is 364 bytes right now. So these three pools cover all the internal cases
					typedef Cm::Block<PxU8, 128> MemBlock128;
					typedef Cm::Block<PxU8, 256> MemBlock256;
					typedef Cm::Block<PxU8, 384> MemBlock384;

					Ps::Pool2<MemBlock128, 8192>	mMemBlock128Pool;
					Ps::Pool2<MemBlock256, 8192>	mMemBlock256Pool;
					Ps::Pool2<MemBlock384, 8192>	mMemBlock384Pool;


					// broad phase data:
					//BroadPhase*					mCollisionSpace;
					NPhaseCore*					mNPhaseCore;

					// Collision filtering
					void*						mFilterShaderData;
					PxU32						mFilterShaderDataSize;
					PxSimulationFilterShader	mFilterShader;
					PxSimulationFilterCallback*	mFilterCallback;

					// Material manager
					PX_ALIGN(16, PxsMaterialManager			mMaterialManager);

					Ps::Array<BodyCore*>		mSleepBodies;
					Ps::Array<BodyCore*>		mWokeBodies;
					bool						mWokeBodyListValid;
					bool						mSleepBodyListValid;
					Ps::Array<Client*>			mClients;	//an array of transform arrays, one for each client.
					Cm::EventProfiler			mEventProfiler;
					SimStats*					mStats;
					PxU32						mInternalFlags;	//!< Combination of ::SceneFlag
					PxSceneFlags				mPublicFlags;	//copy of PxSceneDesc::flags, of type PxSceneFlag

					ObjectIDTracker*			mShapeIDTracker;
					ObjectIDTracker*			mRigidIDTracker;

					InteractionScene*			mInteractionScene;
					StaticSim*					mStaticAnchor;

					Cm::PreallocatingPool<ShapeSim>*	mShapeSimPool;
					Cm::PreallocatingPool<StaticSim>*	mStaticSimPool;
					Cm::PreallocatingPool<BodySim>*		mBodySimPool;
					Ps::Pool<ConstraintSim>*			mConstraintSimPool;
					Ps::Pool<ConstraintInteraction>*
												mConstraintInteractionPool;

					Ps::Pool<SimStateData>*		mSimStateDataPool;

					BatchRemoveState*			mBatchRemoveState;

					Ps::Array<SimpleBodyPair>	mLostTouchPairs;
					Cm::BitMap					mLostTouchPairsDeletedBodyIDs;	// Need to know which bodies have been deleted when processing the lost touch pair list.
																				// Can't use the existing rigid object ID tracker class since this map needs to be cleared at
																				// another point in time.
					Ps::Array<PxU32>			mOutOfBoundsIDs;
					PxU32						mErrorState;

					PxU32						mDominanceBitMatrix[PX_MAX_DOMINANCE_GROUP];

					PxReal						mVisualizationScale;  // Redundant but makes checks whether debug visualization is enabled faster

					bool						mVisualizationParameterChanged;

					// statics:
					PxU32						mNbRigidStatics;
					PxU32						mNbRigidDynamics;
					PxU32						mNbGeometries[PxGeometryType::eGEOMETRY_COUNT];

#if EXTRA_PROFILING || EXTRA_PROFILING_READABLE
	private:
					FILE*	        			mExtraProfileFile;
					PxU32 	        			mLineNum;
#endif

					PxSimulationOrder::Enum		mSimulateOrder;


					// task decomposition
					void						preBroadPhase(PxBaseTask* continuation);
					void						broadPhase(PxBaseTask* continuation);
					void						postBroadPhase(PxBaseTask* continuation);
					void						rigidBodyNarrowPhase(PxBaseTask* continuation);
					void						islandGen(PxBaseTask* continuation);
					void						postIslandGen(PxBaseTask* continuation);
					void						solver(PxBaseTask* continuation);
					void						postSolver(PxBaseTask* continuation);
					void						postCCDPass(PxBaseTask* continuation);
					void						secondBroadPhase(PxBaseTask* continuation);
					void						updateCCDMultiPass(PxBaseTask* continuation);
					void						updateCCDSinglePass(PxBaseTask* continuation);
					void						finalizationPhase(PxBaseTask* continuation);
					void						saveLastCCDTransforms();

					void						postNarrowPhase(PxBaseTask* continuation);
					void						particlePostCollPrep(PxBaseTask* continuation);
					void						particlePostShapeGen(PxBaseTask* continuation);


					void						clothPreprocessing(PxBaseTask* continuation);

					void						addShapes(void *const* shapes, PxU32 nbShapes, size_t ptrOffset, RigidSim& sim, PxsRigidBody* body);
					void						removeShapes(RigidSim& , Ps::InlineArray<Sc::ShapeSim*, 64>& , Ps::InlineArray<const Sc::ShapeCore*, 64>&, bool wakeOnLostTouch);

	private:

					void						addShapes(void *const* shapes, PxU32 nbShapes, size_t ptrOffset, RigidSim& sim, 
														 PxsRigidBody* body, ShapeSim*& prefetchedShapeSim, PxBounds3* outBounds);

					Cm::FanoutTask                                                      mCollisionTask;

					Cm::DelegateTask<Sc::Scene, &Sc::Scene::clothPreprocessing>			mClothPreprocessing;
					PxBaseTask*													mGpuClothSolverTask;

					Cm::DelegateTask<Sc::Scene, &Sc::Scene::postNarrowPhase>			mPostNarrowPhase;
					Cm::FanoutTask														mParticlePostCollPrep;
					Cm::DelegateFanoutTask<Sc::Scene, &Sc::Scene::particlePostShapeGen>	mParticlePostShapeGen;

					Cm::DelegateFanoutTask<Sc::Scene, &Sc::Scene::finalizationPhase>	mFinalizationPhase;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::updateCCDMultiPass>			mUpdateCCDMultiPass;

					//multi-pass ccd stuff
					Ps::Array<Cm::DelegateTask<Sc::Scene, &Sc::Scene::updateCCDSinglePass> >	mUpdateCCDSinglePass;
					Ps::Array<Cm::DelegateTask<Sc::Scene, &Sc::Scene::secondBroadPhase> >		mSecondBroadPhase;
					Ps::Array<Cm::DelegateTask<Sc::Scene, &Sc::Scene::postCCDPass> >			mPostCCDPass;
					PxU32																		mCurrentCCDTask;

					Cm::DelegateTask<Sc::Scene, &Sc::Scene::postSolver>					mPostSolver;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::solver>						mSolver;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::postIslandGen>				mPostIslandGen;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::islandGen>					mIslandGen;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::rigidBodyNarrowPhase>		mRigidBodyNarrowPhase;
					Cm::DelegateFanoutTask<Sc::Scene, &Sc::Scene::postBroadPhase>		mPostBroadPhase;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::broadPhase>					mBroadPhase;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::solveStep>					mSolveStep;
					Cm::DelegateTask<Sc::Scene, &Sc::Scene::collideStep>				mCollideStep;

					Cm::FlushPool														mTaskPool;
					PxTaskManager*												mTaskManager;

#if PX_USE_CLOTH_API
					cloth::Factory*														mGpuLowLevelClothFactory;  // necessary because we have a CUDA context manager per scene
					cloth::Solver*														mGpuClothSolver;
#endif
	};

} // namespace Sc

}

#endif
