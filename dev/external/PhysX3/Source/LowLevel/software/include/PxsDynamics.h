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


#ifndef PXS_DYNAMICS_H
#define PXS_DYNAMICS_H

#include "PxvConfig.h"
#include "CmSpatialVector.h"
#include "CmTask.h"
#include "PxcPool.h"
#include "PxcThreadCoherantCache.h"
#include "PxsThreadContext.h"
//#include "PxsIslandManager.h"
#include "PxcConstraintBlockStream.h"
#include "PxcSolverBody.h"

namespace physx
{
#ifndef PX_PS3
#define PX_CONSTRAINT_PARTITIONING		1
#endif


class PxsContext;
class PxsRigidBody;
class PxsSolverCore;
class PxsThreadContext;
class PxsArticulation;
class PxsStreamedThresholdTable;
struct PxcArticulationSolverDesc;
struct PxsBodyCore;
struct PxsDynamicsStats;
struct PxsIslandObjects;
class PxsIslandIndices;
struct PxsIndexedInteraction;
struct PxcSolverConstraintDesc;
class PxsDynamicsContext;
class PxsMaterialManager;


#define SOLVER_PARALLEL_METHOD_ARGS									\
	PxsDynamicsContext&	context,									\
	const PxU32 positionIterations,									\
	const PxU32 velocityIterations,									\
	PxcSolverBody* PX_RESTRICT atomListStart,						\
	PxcSolverBodyData* PX_RESTRICT atomDataList,					\
	const PxU32 solverBodyOffset,									\
	const PxU32 atomListSize,										\
	PxcArticulationSolverDesc* PX_RESTRICT articulationListStart,	\
	const PxU32 articulationListSize,								\
	PxcSolverConstraintDesc* PX_RESTRICT constraintList,			\
	const PxU32 constraintListSize,									\
	PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList,	\
	const PxU32 frictionConstraintListSize,							\
	PxI32* pConstraintIndex, PxI32* pFrictionConstraintIndex,		\
	PxI32* pAtomListIndex, PxI32* pAtomIntegrationListIndex,		\
	PxcThresholdStreamElement* PX_RESTRICT thresholdStream,			\
	const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,	\
	Cm::SpatialVector* motionVelocityArray,							\
	PxsBodyCore*const* bodyArray,									\
	Cm::BitMap& localChangedShapes,									\
	PxsArticulation*const* PX_RESTRICT articulations,				\
	PxU32 _numArtics,												\
	volatile PxI32* pNumObjectsIntegrated,							\
	Ps::Array<PxsConstraintBatchHeader>& contactBlocks,				\
	Ps::Array<PxsConstraintBatchHeader>& frictionBlocks

typedef	void (*PxsSolveParallelMethod)(SOLVER_PARALLEL_METHOD_ARGS);
extern PxsSolveParallelMethod solveParallel[3];

void solveParallelDefaultFriction(SOLVER_PARALLEL_METHOD_ARGS);
void solveParallelCouloumFriction(SOLVER_PARALLEL_METHOD_ARGS);

class PxBaseTask;
class PxTaskManager;

namespace Cm
{
	class Bitmap;
	class SpatialVector;
}

/**
\brief Solver body pool (array) that enforces 128-byte alignment for base address of array.
\note This reduces cache misses on platforms with 128-byte-size cache lines by aligning the start of the array to the beginning of a cache line.
*/
class PxsSolverBodyPool : public Ps::Array<PxcSolverBody, Ps::AlignedAllocator<128, Ps::ReflectionAllocator<PxcSolverBody> > > 
{ 
	PX_NOCOPY(PxsSolverBodyPool)
public:
	PxsSolverBodyPool() {}
};

/**
\brief Solver body data pool (array) that enforces 128-byte alignment for base address of array.
\note This reduces cache misses on platforms with 128-byte-size cache lines by aligning the start of the array to the beginning of a cache line.
*/
class PxsSolverBodyDataPool : public Ps::Array<PxcSolverBodyData, Ps::AlignedAllocator<128, Ps::ReflectionAllocator<PxcSolverBodyData> > >
{
	PX_NOCOPY(PxsSolverBodyDataPool)
public:
	PxsSolverBodyDataPool() {}
};

/**
\brief Encapsules the data used by the constraint solver.
*/
class PxsDynamicsContext
{
	PX_NOCOPY(PxsDynamicsContext)
public:
	
	/**
	\brief Creates a PxsDynamicsContext associated with a PxsContext
	\param[in] context The context to associate the new PxsDynamicsContext with
	\return A pointer to the newly-created PxsDynamicsContext.
	*/
	static PxsDynamicsContext*	create(PxsContext* context);
	
	/**
	\brief Destroys this PxsDynamicsContext
	*/
	void						destroy();

	/**
	\brief Get the PxsContext associated with this dynamics context.
	\return The PxsContext associated with this dynamics context.
	*/
	PX_FORCE_INLINE PxsContext*			getContext()							{ return mContext;			}
	/**
	\brief Returns the bounce threshold
	\return The bounce threshold.
	*/
	PX_FORCE_INLINE PxReal				getBounceThreshold()			const	{ return mBounceThreshold;	}
	/**
	\brief Returns the friction offset threshold
	\return The friction offset threshold.
	*/
	PX_FORCE_INLINE PxReal				getFrictionOffsetThreshold()	const	{ return mFrictionOffsetThreshold;	}
	/**
	\brief Sets the bounce threshold
	\param[in] f The bounce threshold
	*/
	PX_FORCE_INLINE void				setBounceThreshold(PxReal f)			{ mBounceThreshold = f;		}
	/**
	\brief Sets the friction offset threshold
	\param[in] f The friction offset threshold
	*/
	PX_FORCE_INLINE void				setFrictionOffsetThreshold(PxReal offset)		{ mFrictionOffsetThreshold = offset;				}
	/**
	\brief Returns the solver batch size
	\return The solver batch size.
	*/
	PX_FORCE_INLINE int					getSolverBatchSize()			const	{ return mSolverBatchSize;	}
	/**
	\brief Sets the solver batch size
	\param[in] f The solver batch size
	*/
 	PX_FORCE_INLINE void				setSolverBatchSize(PxU32 f)				{ mSolverBatchSize = f;		}
	/**
	\brief Returns the current frame's timestep
	\return The current frame's timestep.
	*/
	PX_FORCE_INLINE PxReal				getDt()							const	{ return mDt;				}
	/**
	\brief Returns 1/(current frame's timestep)
	\return 1/(current frame's timestep).
	*/
	PX_FORCE_INLINE PxReal				getInvDt()						const	{ return mInvDt;			}
	/**
	\brief Returns the maximum solver constraint size
	\return The maximum solver constraint size in this island in bytes.
	*/
	PX_FORCE_INLINE PxU32				getMaxSolverConstraintSize()	const	{ return mMaxSolverConstraintSize; }
	/**
	\brief Returns the static world solver body
	\return The static world solver body.
	*/
	PX_FORCE_INLINE PxcSolverBody&		getWorldSolverBody()					{ return mWorldSolverBody;  }



	/**
	\brief The entry point for the constraint solver. 
	\param[in]	dt	The simulation time-step
	\param[in]	continuation The continuation task for the solver

	This method is called after the island generation has completed. Its main responsibilities are:
	(1) Reserving the solver body pools
	(2) Initializing the static and kinematic solver bodies, which are shared resources between islands.
	(3) Construct the solver task chains for each island

	Each island is solved as an independent solver task chain in parallel.

	*/
	void								update(PxReal dt, PxBaseTask* continuation);

	/**
	\brief Legacy non-threaded solve function.
	\param[in] threadContext Per-island context to store transient per-island data.
	\param[in] objects The list of objects (bodies/articulations/constraints/contacts) in the island.
	\param[in] counts The sizes of the list of objects in the island.
	\param[out] shapeChangedMap A bit field representing the shapes whose transforms have changed.
	\param[in] solverBodyOffset Offset to the first dynamic solver body in this island.
	\param[in] numSpus The total number of SPUs available to solve this island. This variable only has any meaning on PS3.

	This method solves an island single-threaded. This is retained as a reference implementation of the solver but is not used explicitly in the SDK.
	*/
	void								solveGroup(PxsThreadContext& threadContext, 
											   const PxsIslandObjects& objects, 
											   const PxsIslandIndices& counts,
											   Cm::BitMap& shapeChangedMap, 
											   const PxU32 solverBodyOffset,
											   const PxU32 numSpus = 0);

	/**
	\brief This method combines the results of several islands, e.g. constructing scene-level simulation statistics and merging together threshold streams for contact notification.
	*/
	void								mergeResults();

	/**
	\brief This method integrates the pose of a given atom.
	\param[in] atom The rigid body to integrate.
	\param[in] shapeChangedMap The bit map indicating which bodies' transforms have changed.
	\param[in] lv The body's linear velocity.
	\param[in] av The body's angular velocity.
	*/
	void								integrateAtomPose(	PxsRigidBody* atom, Cm::BitMap& shapeChangedMap,
												const PxVec3& lv, const PxVec3& av) const;



protected:

	/**
	\brief Constructor for PxsDynamicsContext
	\param[in] context The PxsContext that is associated with this PxsDynamicsContext.
	*/
										PxsDynamicsContext(PxsContext* context);
	/**
	\brief Destructor for PxsDynamicsContext
	*/
										~PxsDynamicsContext();


	// Solver helper-methods
	/**
	\brief Computes the unconstrained velocity for a given PxsRigidBody
	\param[in] atom The PxsRigidBody
	*/
	void								computeUnconstrainedVelocity(PxsRigidBody* atom)	const;

	/**
	\brief fills in a PxsSolverConstraintDesc from an indexed interaction
	\params[inout] desc The PxsSolverConstraintDesc
	\params[in] constraint The PxsIndexedInteraction
	*/
	void								setDescFromIndices(PxcSolverConstraintDesc& desc, 
													  const PxsIndexedInteraction& constraint);

	/**
	\brief Creates a set of constraints for a given friction model. This method may spawn additional tasks to create constraints in parallel
	\param[in] frictionType The friction model being used
	\param[in] descArray The array of PxcSolverConstraintDesc
	\param[in] solverBodyData The array of PxcSolverBodyData
	\param[in] descCount The number of constraint descriptors
	\param[in] continuation The continuation task
	\param[in] threadContext The per-island thread context
	*/
	void								createFinalizeContacts(PxsFrictionModel frictionType, PxcSolverConstraintDesc* descArray, PxcSolverBodyData* solverBodyData, 
											PxU32 descCount, PxBaseTask* continuation, PxsThreadContext& threadContext);

	/**
	\brief Compute the unconstrained velocity for set of bodies in parallel. This function may spawn additional tasks.
	\param[in] dt The timestep
	\param[in] bodyArray The array of body cores
	\param[in] originalBodyArray The array of PxsRigidBody
	\param[in] bodyCount The number of bodies
	\param[in] accelerationArray The array of body accelerations
	\param[out] solverBodyPool The pool of solver bodies. These are synced with the corresponding body in bodyArray.
	\param[out] solverBodyDataPool The pool of solver body data. These are synced with the corresponding body in bodyArray
	\param[out] motionVelocityArray The motion velocities for the bodies
	\param[out] maxSolverPositionIterations The maximum number of position iterations requested by any body in the island
	\param[out] maxSolverVelocityIterations The maximum number of velocity iterations requested by any body in the island
	\param[out] integrateTask The continuation task for any tasks spawned by this function.
	*/
	void								atomIntegrationParallel(
											   const PxF32 dt,
											   PxsBodyCore*const* bodyArray,					// INOUT: core body attributes
											   PxsRigidBody*const* originalBodyArray,			// IN: original body atom names (LEGACY - DON'T deref the ptrs!!)
											   PxU32 bodyCount,									// IN: body count
											   const Cm::SpatialVector* accelerationArray,		// IN: body accelerations
											   PxcSolverBody* solverBodyPool,					// IN: solver atom pool (space preallocated)
											   PxcSolverBodyData* solverBodyDataPool,
											   Cm::SpatialVector* motionVelocityArray,			// OUT: motion velocities
											   PxU32& maxSolverPositionIterations,
											   PxU32& maxSolverVelocityIterations,
											   PxBaseTask& integrateTask
											   );

	/**
	\brief Solves an island in parallel using the default friction model.

	\param[in] positionIterations Number of position iterations.
	\param[in] velocityIterations Number of velocity iterations.
	\param[in] atomListStart The list of rigid bodies in the island.
	\param[in] atomListSize The number of rigid bodies in the island.
	\param[in] articulationListSTart The list of articulations in the island.
	\param[in] articulationListSize The number of articulations in the island.
	\param[in] contactConstraintList The ordered list of constraints to solve in the island.
	\param[in] contactConstraintListSize The number of constraints in the island.
	\param[in] pConstraintIndex A shared constraint progress counter used to lock access to constraints across multiple solver threads.
	\param[in] pAtomListIndex A shared atom progress counter used to lock access to rigid bodies/articulations across multiple solver threads. Used to write back motion velocities.
	\param[in] pAtomIntegrationListIndex A shared atom progress counter used to lock access to rigid bodies/articulations across multiple solver threads. Used to control access for final integration phase.
	\param[out] thresholdStream Per-constraint threshold stream objects used by contact notification filtering. Records the total force applied by a given constraint.
	\param[in] thresholdStreamLength The length of the threshold stream.
	\param[out] outThresholdPairs The total number of threshold pairs in the thresholdStream.
	\param[out] motionVelocityArray Array of motion velocities. These are used to update the objects' transforms.
	\param[out] bodyArray Array of body cores into which the motion velocities are integrated.
	\param[out] localChangedShapes A bitmap to indicate which bodies' transforms have changed.
	\param[out] articulations Array of articulations into which the motion velocities are integrated.
	\param[in] numArtics The number of articulations.
	\param[out] pNumObjectsIntegrated Shared variable used to count the number of objects integrated.
	\param[in] contactBlocks Headers used for constraint batching.
	\param[in] frictionBlocks Unused param used in alternative friction models.
	*/
	void								solveParallel(const PxU32 positionIterations, const PxU32 velocityIterations, 
											 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, 
											 const PxU32 solverBodyOffset, const PxU32 atomListSize, PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, 
											 const PxU32 articulationListSize, PxcSolverConstraintDesc* PX_RESTRICT contactConstraintList, const PxU32 contactConstraintListSize, 
											 PxI32* pConstraintIndex, PxI32* pAtomListIndex, PxI32* pAtomIntegrationListIndex,
											 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
											 Cm::SpatialVector* motionVelocityArray, PxsBodyCore*const* bodyArray, Cm::BitMap& localChangedShapes,
											 PxsArticulation*const* PX_RESTRICT articulations, PxU32 numArtics, volatile PxI32* pNumObjectsIntegrated,
											 Ps::Array<PxsConstraintBatchHeader>& contactBlocks,  Ps::Array<PxsConstraintBatchHeader>& frictionBlocks);

/**
	\brief Solves an island in parallel using the 1D/2D friction models.

	\param[in] positionIterations Number of position iterations.
	\param[in] velocityIterations Number of velocity iterations.
	\param[in] atomListStart The list of rigid bodies in the island.
	\param[in] atomListSize The number of rigid bodies in the island.
	\param[in] articulationListSTart The list of articulations in the island.
	\param[in] articulationListSize The number of articulations in the island.
	\param[in] contactConstraintList The ordered list of constraints to solve in the island.
	\param[in] contactConstraintListSize The number of constraints in the island.
	\param[in] frictionConstraintList The ordered list of friction constraints to solve in the island.
	\param[in] frictionConstraintListSize The number of friction constraints to solve in the island.
	\param[in] pConstraintIndex A shared constraint progress counter used to lock access to constraints across multiple solver threads.
	\param[in] pFrictionConstraintIndex A shared constraint progress counter used to lock access to friction constraints across multiple solver threads.
	\param[in] pAtomListIndex A shared atom progress counter used to lock access to rigid bodies/articulations across multiple solver threads. Used to write back motion velocities.
	\param[in] pAtomIntegrationListIndex A shared atom progress counter used to lock access to rigid bodies/articulations across multiple solver threads. Used to control access for final integration phase.
	\param[out] thresholdStream Per-constraint threshold stream objects used by contact notification filtering. Records the total force applied by a given constraint.
	\param[in] thresholdStreamLength The length of the threshold stream.
	\param[out] outThresholdPairs The total number of threshold pairs in the thresholdStream.
	\param[out] motionVelocityArray Array of motion velocities. These are used to update the objects' transforms.
	\param[out] bodyArray Array of body cores into which the motion velocities are integrated.
	\param[out] localChangedShapes A bitmap to indicate which bodies' transforms have changed.
	\param[out] articulations Array of articulations into which the motion velocities are integrated.
	\param[in] numArtics The number of articulations.
	\param[out] pNumObjectsIntegrated Shared variable used to count the number of objects integrated.
	\param[in] contactBlocks Headers used for constraint batching.
	\param[in] frictionBlocks Headers used for friction constraint batching.
	*/

	
	void								solveParallelCoulomb(const PxU32 positionIterations, const PxU32 velocityIterations, 
											 PxcSolverBody* PX_RESTRICT atomListStart, PxcSolverBodyData* PX_RESTRICT atomDataList, const PxU32 solverBodyOffset, 
											 const PxU32 atomListSize, PxcArticulationSolverDesc* PX_RESTRICT articulationListStart, const PxU32 articulationListSize,
											 PxcSolverConstraintDesc* PX_RESTRICT constraintList, const PxU32 constraintListSize,
											  PxcSolverConstraintDesc* PX_RESTRICT frictionConstraintList, const PxU32 frictionConstraintListSize,
											 PxI32* pConstraintIndex, PxI32* pFrictionConstraintIndex, PxI32* pAtomListIndex, PxI32* pAtomIntegrationListIndex,
											 PxcThresholdStreamElement* PX_RESTRICT thresholdStream, const PxU32 thresholdStreamLength, PxI32* outThresholdPairs,
											 Cm::SpatialVector* motionVelocityArray, PxsBodyCore*const* bodyArray, Cm::BitMap& localChangedShapes,
											 PxsArticulation*const* PX_RESTRICT articulations, PxU32 _numArtics, volatile PxI32* pNumObjectsIntegrated,
											 Ps::Array<PxsConstraintBatchHeader>& contactBlocks, Ps::Array<PxsConstraintBatchHeader>& frictionBlocks);


	//Data

	/**
	\brief Body to represent the world static body.
	*/
	PxcSolverBody				mWorldSolverBody;
	/**
	\brief Body data to represent the world static body.
	*/
	PxcSolverBodyData			mWorldSolverBodyData;

	/**
	\brief Pointer to shared per-island data.
	*/
	PxsContext*					mContext;

	/**
	\brief Global pool for solver bodies. Kinematic bodies are at the start, and then dynamic bodies
	*/
	PxsSolverBodyPool			mSolverBodyPool;
	/**
	\brief Global pool for solver body data. Kinematic bodies are at the start, and then dynamic bodies
	*/
	PxsSolverBodyDataPool		mSolverBodyDataPool;
	
	/**
	\brief Time-step.
	*/
	PxReal						mDt;
	/**
	\brief 1/time-step.
	*/
	PxReal						mInvDt;
	/**
	\brief max solver constraint size
	*/
	PxU32						mMaxSolverConstraintSize;

	/**
	\brief Interface to the solver core.
	\note We currently only support PxsSolverCoreSIMD. Other cores may be added in future releases.
	*/
	PxsSolverCore*				mSolverCore;

	/**
	\brief Threshold controlling the relative velocity at which the solver transitions between restitution and bias for solving normal contact constraint.
	*/
	PxReal						mBounceThreshold;
	/**
	\brief Threshold controlling whether friction anchors are constructed or not. If the separation is above mFrictionOffsetThreshold, the contact will not be considered to become a friction anchor
	*/
	PxReal						mFrictionOffsetThreshold;
	/**
	\brief The minimum size of an island to generate a solver task chain.
	*/
	PxU32						mSolverBatchSize;
	/**
	\brief The total number of kinematic bodies in the scene
	*/
	PxU32						mKinematicCount;
	
	public:
	//typedef Cm::DelegateTask<PxsDynamicsContext, &PxsDynamicsContext::mergeResults> MergeTask;
	//MergeTask					mMergeTask;
	protected:

	friend class PxsSolverStartTask;
	friend class PxsSolverAtomTask;
	friend class PxsSolverAticulationsTask;
	friend class PxsSolverSetupConstraintsTask;
	friend class PxsSetupConstraintSpuTask;
	friend class PxsSolverCreateFinalizeConstraintsTask;	
	friend class PxsSolverConstraintPartitionTask;
	friend class PxsSolverSetupSolveTask;
	friend class PxsSolverIntegrateTask;
	friend class PxsIntegrateSpuTask;
	friend class PxsSolverEndTask;
	friend class PxsSolverConstraintPostProcessTask;

	friend void solveParallelDefaultFriction(SOLVER_PARALLEL_METHOD_ARGS);
	friend void solveParallelCouloumFriction(SOLVER_PARALLEL_METHOD_ARGS);

#ifdef PX_PS3
	friend class PxsSolverSpuTask;
	friend class PxsSolverCoulombSpuTask;
#endif
};

}

#endif
