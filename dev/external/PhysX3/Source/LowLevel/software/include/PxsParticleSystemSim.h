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


#ifndef PXS_PARTICLE_SYSTEM_SIM_H
#define PXS_PARTICLE_SYSTEM_SIM_H

#include "PxvParticleSystemSim.h"
#include "PxsFluidDynamics.h"
#include "PxsFluidCollision.h"
#include "PxcGridCellVector.h"
#include "PsAlignedMalloc.h"
#include "PxvParticleSystemSim.h"
#include "PxsParticleData.h"
#include "CmTask.h"
#include "PxsContext.h"

namespace physx
{

struct PxsFluidConstraintPair;
class PxsFluidSpatialHash;
class PxBaseTask;

class PxsParticleSystemSim : public PxvParticleSystemSim
{
public:

	//---------------------------
	// Implements PxvParticleSystemSim
	virtual		PxvParticleSystemState& getParticleStateV();
	virtual		void					getSimParticleDataV(PxvParticleSystemSimDataDesc& simParticleData, bool devicePtr) const;

	virtual		void					getShapesUpdateV(PxvParticleShapeUpdateResults& updateResults) const;

	virtual		void					setExternalAccelerationV(const PxVec3& v);
	virtual		const PxVec3&			getExternalAccelerationV()					const;

	virtual		void					setSimulationTimeStepV(PxReal value);
	virtual		PxReal					getSimulationTimeStepV()					const;

	virtual		void					setSimulatedV(bool);
	virtual		Ps::IntBool				isSimulatedV()								const;

	virtual		void					addInteractionV(const PxvParticleShape& /*particleShape*/, PxvShapeHandle /*shape*/, PxvBodyHandle /*body*/, bool /*isDynamic*/, bool /*secondaryBroadphase*/) {}
	virtual		void					removeInteractionV(const PxvParticleShape& particleShape, PxvShapeHandle shape, PxvBodyHandle body, bool isDynamic, bool isDyingRb, bool secondaryBroadphase);
	virtual		void					onRbShapeChangeV(const PxvParticleShape& particleShape, PxvShapeHandle shape, PxvBodyHandle body);
	
	virtual		void					flushBufferedInteractionUpdatesV() {}

	virtual		void					passCollisionInputV(PxvParticleCollisionUpdateInput input);
#if PX_SUPPORT_GPU_PHYSX
	virtual		Ps::IntBool				isGpuV() const { return false; }	
	virtual		void					getReadWriteCudaBuffersGpuV(struct PxCudaReadWriteParticleBuffers& /*buffers*/) { PX_ASSERT(0); }
	virtual		void					setValidParticleRangeGpuV(PxU32 /*validParticleRange*/) { PX_ASSERT(0); }
	virtual		void					setDeviceExclusiveModeFlagsGpuV(PxU32 /*flags*/) { PX_ASSERT(0); }
	virtual		Ps::IntBool				isInDeviceExclusiveModeGpuV() const { return false; }
#endif
	
	//~Implements PxvParticleSystemSim
	//---------------------------

						PxsParticleSystemSim(PxsContext* context, PxU32 index);
	virtual				~PxsParticleSystemSim();
						void				init(PxsParticleData& particleData, const PxvParticleSystemParameter& parameter);
						void				clear();
						PxsParticleData* 	obtainParticleState();

	PX_FORCE_INLINE		PxsContext&			getContext()					const { return mContext; }

	PX_FORCE_INLINE		void				getPacketBounds(const PxcGridCellVector& coord, PxBounds3& bounds);

	PX_FORCE_INLINE		PxReal				computeViscosityMultiplier(PxReal viscosityStd, PxReal particleMassStd, PxReal radius6Std);

	PX_FORCE_INLINE		PxU32				getIndex() const	{ return mIndex; }
	
						void				packetShapesUpdate(PxBaseTask* continuation);
						void				packetShapesFinalization(PxBaseTask* continuation);
						void				dynamicsUpdate(PxBaseTask* continuation);
						void				collisionUpdate(PxBaseTask* continuation);
						void				collisionFinalization(PxBaseTask* continuation);
						void				spatialHashUpdateSections(PxBaseTask* continuation);

						PxBaseTask&	schedulePacketShapesUpdate(const PxvParticleShapesUpdateInput& input, PxBaseTask& continuation);
						PxBaseTask&	scheduleDynamicsUpdate(PxBaseTask& continuation);
						PxBaseTask&	scheduleCollisionUpdate(PxBaseTask& continuation);
private:
						void				remapShapesToPackets(PxvParticleShape*const* shapes, PxU32 numShapes);
						void				clearParticleConstraints();
						void				initializeParameter();
						void				updateDynamicsParameter();
						void				updateCollisionParameter();
						void				removeTwoWayRbReferences(const PxsParticleShape& particleShape, const PxsBodyCore* rigidBody);
						void				setCollisionCacheInvalid(const PxsParticleShape& particleShape, const Gu::GeometryUnion& geometry);

private:
	PxsContext&							mContext;
	PxsParticleData*					mParticleState;
	const PxvParticleSystemParameter*	mParameter;

	Ps::IntBool					mSimulated;

	PxsFluidTwoWayData*			mFluidTwoWayData;
		
	PxvParticleShape**			mCreatedDeletedParticleShapes;	// Handles of created and deleted particle packet shapes.
	PxU32						mNumCreatedParticleShapes;
	PxU32						mNumDeletedParticleShapes;
	PxU32*						mPacketParticlesIndices;		// Dense array of sorted particle indices.
	PxU32						mNumPacketParticlesIndices;

	PxsFluidConstraintBuffers	mConstraintBuffers;			// Particle constraints.
		
	PxsFluidParticleOpcodeCache*	mOpcodeCacheBuffer;			// Opcode cache.
	PxVec3*						mTransientBuffer;				//force in SPH , collision normal

	// Spatial ordering, packet generation
	PxsFluidSpatialHash*		mSpatialHash;

	// Dynamics update
	PxsFluidDynamics			mDynamics;

	// Collision update
	PxsFluidCollision			mCollision;

	PxReal						mSimulationTimeStep;
	bool						mIsSimulated;

	PxVec3						mExternalAcceleration;		// This includes the gravity of the scene

	PxU32						mIndex;

	// pipeline tasks
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::packetShapesUpdate>		PacketShapesUpdateTask;
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::packetShapesFinalization>	PacketShapesFinalizationTask;
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::dynamicsUpdate>			DynamicsUpdateTask;
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::collisionUpdate>			CollisionUpdateTask;
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::collisionFinalization>	CollisionFinalizationTask;
	typedef Cm::DelegateTask<PxsParticleSystemSim, &PxsParticleSystemSim::spatialHashUpdateSections> SpatialHashUpdateSectionsTask;
	
	PacketShapesUpdateTask			mPacketShapesUpdateTask;
	PacketShapesFinalizationTask	mPacketShapesFinalizationTask;
	DynamicsUpdateTask				mDynamicsUpdateTask;
	CollisionUpdateTask				mCollisionUpdateTask;
	CollisionFinalizationTask		mCollisionFinalizationTask;
	SpatialHashUpdateSectionsTask	mSpatialHashUpdateSectionsTask;

	PxvParticleShapesUpdateInput	mPacketShapesUpdateTaskInput;
	PxvParticleCollisionUpdateInput	mCollisionUpdateTaskInput;

	Ps::AlignedAllocator<16, Ps::ReflectionAllocator<char> >	mAlign16;
	
	friend class PxsFluidCollision;
	friend class PxsFluidDynamics;
};

//----------------------------------------------------------------------------//

/*!
Compute AABB of a packet given its coordinates.
Enlarge the bounding box such that a particle on the current boundary could
travel the maximum distance and would still be inside the enlarged volume.
*/
PX_FORCE_INLINE void PxsParticleSystemSim::getPacketBounds(const PxcGridCellVector& coord, PxBounds3& bounds)
{
	PxVec3 gridOrigin(static_cast<PxReal>(coord.x), static_cast<PxReal>(coord.y), static_cast<PxReal>(coord.z));
	gridOrigin *= mCollision.getParameter().packetSize;

	PxVec3 collisionRangeVec(mCollision.getParameter().collisionRange);
	bounds.minimum = gridOrigin - collisionRangeVec;
	bounds.maximum = gridOrigin + PxVec3(mCollision.getParameter().packetSize) + collisionRangeVec;
}

PX_FORCE_INLINE PxReal PxsParticleSystemSim::computeViscosityMultiplier(PxReal viscosityStd, PxReal particleMassStd, PxReal radius6Std)
{
	PxReal wViscosityLaplacianScalarStd = 45.0f / (PxPi * radius6Std);
	return (wViscosityLaplacianScalarStd * viscosityStd * particleMassStd);
}

}

#endif // PXS_PARTICLE_SYSTEM_SIM_H
