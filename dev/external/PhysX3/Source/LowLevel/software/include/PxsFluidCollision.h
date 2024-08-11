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


#ifndef PXS_FLUID_COLLISION_H
#define PXS_FLUID_COLLISION_H


#include "PsBitUtils.h"
#include "PxsFluidConfig.h"
#include "PxTransform.h"
#include "PxsFluidCollisionData.h"
#include "PxsFluidCollisionMethods.h"
#include "PxsFluidParticle.h"
#include "PxsFluidTwoWayData.h"
#include "PxsFluidCollisionParameters.h"
#include "PsAlloca.h"
#include "PsAlignedMalloc.h"
#include "CmTask.h"
#include "PxsParticleContactManagerStream.h"

#ifdef PX_PS3
#include "CellFluidCollisionTask.h"
#endif

namespace physx
{

class PxsFluid;
class PxsParticleShape;
class PxsRigidBody;
class PxsBodyTransformVault;
struct PxsW2STransformTemp;
class PxBaseTask;

class PxsFluidCollision
{
public:

							PxsFluidCollision(class PxsParticleSystemSim& particleSystem);
							~PxsFluidCollision();

		void				updateCollision(const PxU8* contactManagerStream,
							PxBaseTask& continuation);
											
		PX_FORCE_INLINE PxsFluidCollisionParameters& getParameter() { return mParams; }

private:
		typedef Ps::Array<PxsW2STransformTemp, shdfnd::AlignedAllocator<16, Ps::ReflectionAllocator<PxsW2STransformTemp> > > TempContactManagerArray;
		struct TaskData 
		{
			TempContactManagerArray tempContactManagers;
			PxsParticleContactManagerStreamIterator packetBegin;
			PxsParticleContactManagerStreamIterator packetEnd;
			PxBounds3 bounds;
		};
		
private:
		void				processShapeListWithFilter(PxU32 taskDataIndex,const PxU32 skipNum = 0);
		void				mergeResults(PxBaseTask* continuation);


		void				updateFluidShapeCollision(PxsFluidParticle* particles,
													  PxsFluidTwoWayData* fluidTwoWayData,
													  PxVec3* transientBuf,
													  PxsFluidConstraintBuffers& constraintBufs,	
													  PxsFluidParticleOpcodeCache* opcodeCache,													
													  PxBounds3& worldBounds,													 
													  const PxU32* fluidShapeParticleIndices,
													  const PxF32* restOffsets,
													  const PxsW2STransformTemp* w2sTransforms,
													  const PxsParticleStreamShape& streamShape);

		void				updateSubPacket(PxsFluidParticle* particlesSp,											
											 PxsFluidTwoWayData* fluidTwoWayData, 
											 PxVec3* transientBuf,
											 PxsFluidConstraintBuffers& constraintBufs,
											 PxsFluidParticleOpcodeCache* perParticleCacheLocal,
											 PxsFluidParticleOpcodeCache* perParticleCacheGlobal,
											 PxsFluidLocalCellHash& localCellHash,
											 PxBounds3& worldBounds,
											 const PxVec3& packetCorner,
											 const PxU32* particleIndicesSp,
											 const PxU32 numParticlesSp,
											 const PxsParticleStreamContactManager* contactManagers,
											 const PxsW2STransformTemp* w2sTransforms,
											 const PxU32 numContactManagers, 
											 const PxF32* restOffsetsSp);														

		void				updateFluidBodyContactPair(	const PxsFluidParticle* particles,
														PxU32 numParticles,
														PxsParticleCollData* particleCollData,
														PxsFluidConstraintBuffers& constraintBufs,													
														PxsFluidParticleOpcodeCache* perParticleCacheLocal,
														PxsFluidLocalCellHash& localCellHash,
														const PxVec3& packetCorner,
														const PxsParticleStreamContactManager& contactManager,
														const PxsW2STransformTemp& w2sTransform);
			
		void PX_FORCE_INLINE addTempW2STransform(TaskData& taskData, const PxsParticleStreamContactManager& cm);

private:
		PxsFluidCollision& operator=(const PxsFluidCollision&);
		PxsFluidCollisionParameters	mParams;
		PxsParticleSystemSim& mParticleSystem;
		TaskData mTaskData[PXS_FLUID_NUM_PACKETS_PARALLEL_COLLISION];
				
		typedef Cm::DelegateTask<PxsFluidCollision, &PxsFluidCollision::mergeResults> MergeTask;
		MergeTask mMergeTask;
		friend class PxsFluidCollisionTask;
#ifdef PX_PS3
		friend class CellFluidCollisionTask;
#endif
};

}

#endif // PXS_FLUID_COLLISION_H
