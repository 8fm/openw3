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


#ifndef PX_SCENE_GPU_H
#define PX_SCENE_GPU_H

#include "Pxg.h"
#include "Ps.h"

namespace physx
{

namespace cloth
{
	class Factory;
	class Cloth;
}

class PxBaseTask;
class PxvParticleSystemSim;
struct PxvParticleSystemStateDataDesc;
struct PxvParticleSystemParameter;

typedef size_t PxvShapeHandle;
typedef size_t PxvBodyHandle;
/**
\brief Interface to manage a set of cuda accelerated feature instances that share the same physx::PxCudaContextManager and PxRigidBodyAccessGpu instance.
*/
class PxSceneGpu
{
public:

	/**
	\brief release instance.
	*/
	virtual		void					release() = 0;
	
	/**
	Adds a particle system to the cuda PhysX lowlevel. Currently the particle system is just associated with a CudaContextManager. 
	Later it will be will be is some kind of scene level context for batched stepping.

	\param state The particle state to initialize the particle system. For initialization with 0 particles, PxvParticleSystemStateDataDesc::validParticleRange == 0.
	\param parameter To configure the particle system pipeline
	*/
	virtual		PxvParticleSystemSim*	addParticleSystem(const PxvParticleSystemStateDataDesc& state, const PxvParticleSystemParameter& parameter) = 0;

	/**
	Removed a particle system from the cuda PhysX lowlevel.
	*/
	virtual		void					removeParticleSystem(PxvParticleSystemSim* particleSystem) = 0;

	/**
	Notify shape change
	*/
	virtual		void					onShapeChange(PxvShapeHandle shapeHandle, PxvBodyHandle bodyHandle, bool isDynamic) = 0;

	/**
	Batched scheduling of shape generation. PxvParticleShapesUpdateInput::shapes ownership transfered to callee.
	*/														
	virtual		PxBaseTask&		scheduleParticleShapeUpdate(PxvParticleSystemSim** particleSystems, struct PxvParticleShapesUpdateInput* inputs, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;
	
	/**
	Batched scheduling of collision input update.
	*/
	virtual		PxBaseTask&		scheduleParticleCollisionInputUpdate(PxvParticleSystemSim** particleSystems, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;
	
	/**
	Batched scheduling of particles update.
	*/														
	virtual		PxBaseTask&		scheduleParticlePipeline(PxvParticleSystemSim** particleSystems, physx::PxU32 batchSize, physx::PxBaseTask& continuation) = 0;

	/**
	Create a GPU cloth factory
	*/
	virtual		cloth::Factory*			createClothFactory() = 0;

	/**
	Create a cloth by cloning another one

	\param targetFactory Specifies which factory to use to create the new cloth (use GPU factory for GPU cloth, CPU factory for CPU cloth)
	\param clothTemplate The cloth to clone
	*/
	virtual		cloth::Cloth*			createCloth(cloth::Factory& targetFactory, cloth::Cloth& clothTemplate) = 0;
};

}

#endif // PX_SCENE_GPU_H
