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


#ifndef PXV_PARTICLE_SYSTEM_SIM_H
#define PXV_PARTICLE_SYSTEM_SIM_H

#include "PxvConfig.h"
#include "PxvParticleSystemCore.h"
#include "PxvParticleShape.h"

namespace physx
{

class PxvParticleSystemSim;
struct PxvParticleSystemSimDataDesc;
class PxBaseTask;

/*!
\file
ParticleSystemSim interface.
*/

/************************************************************************/
/* ParticleSystemsSim                                                      */
/************************************************************************/

/**
Descriptor for the batched shape update pipeline stage
*/
struct PxvParticleShapesUpdateInput
{
	PxvParticleShape**	shapes;
	PxU32				shapeCount;
};

/**
Descriptor for the batched collision update pipeline stage
*/
struct PxvParticleCollisionUpdateInput
{
	PxU8*				contactManagerStream;
};

/*!
Descriptor for updated particle packet shapes
*/
struct PxvParticleShapeUpdateResults
{	
	PxvParticleShape*const*			createdShapes;			//! Handles of newly created particle packet shapes
	PxU32							createdShapeCount;		//! Number of newly created particle packet shapes
	PxvParticleShape*const*			destroyedShapes;		//! Handles of particle packet shapes to delete
	PxU32							destroyedShapeCount;	//! Number of particle packet shapes to delete
};

class PxvParticleSystemSim
{
public:
	virtual		PxvParticleSystemState& getParticleStateV()																											= 0;
	virtual		void					getSimParticleDataV(PxvParticleSystemSimDataDesc& simParticleData, bool devicePtr)									const	= 0;
	virtual		void					getShapesUpdateV(PxvParticleShapeUpdateResults& updateResults)														const	= 0;

	virtual		void					setExternalAccelerationV(const PxVec3& v)																					= 0;
	virtual		const PxVec3&			getExternalAccelerationV()																							const	= 0;

	virtual		void					setSimulationTimeStepV(PxReal value)																						= 0;
	virtual		PxReal					getSimulationTimeStepV()																							const	= 0;

	virtual		void					setSimulatedV(bool)																											= 0;
	virtual		Ps::IntBool				isSimulatedV()																										const	= 0;
	
	// gpuBuffer specifies that the interaction was created asynchronously to gpu execution (for rb ccd)
	virtual		void					addInteractionV(const PxvParticleShape& particleShape, PxvShapeHandle shape, PxvBodyHandle body, 
															bool isDynamic, bool gpuBuffer)																			= 0;

	// gpuBuffer specifies that the interaction was created asynchronously to gpu execution (for rb ccd)
	virtual		void					removeInteractionV(const PxvParticleShape& particleShape, PxvShapeHandle shape, PxvBodyHandle body, 
															bool isDynamic, bool isDyingRb, bool gpuBuffer)															= 0;
	
	virtual		void					onRbShapeChangeV(const PxvParticleShape& particleShape, PxvShapeHandle shape, PxvBodyHandle body)							= 0;

	// applies the buffered interaction updates.
	virtual		void					flushBufferedInteractionUpdatesV()																							= 0;

	// passes the contact manager stream needed for collision - the callee is responsible for releasing it 
	virtual		void					passCollisionInputV(PxvParticleCollisionUpdateInput input)																	= 0;

#if PX_SUPPORT_GPU_PHYSX
	virtual		Ps::IntBool				isGpuV()																											const	= 0;
	virtual		void					getReadWriteCudaBuffersGpuV(struct PxCudaReadWriteParticleBuffers& buffers)													= 0;
	virtual		void					setValidParticleRangeGpuV(PxU32 validParticleRange)																			= 0;
	virtual		void					setDeviceExclusiveModeFlagsGpuV(PxU32 flags)																				= 0;
	virtual		Ps::IntBool				isInDeviceExclusiveModeGpuV()																						const	= 0;
#endif

protected:
	virtual		~PxvParticleSystemSim() {}
};

}

#endif // PXV_PARTICLE_SYSTEM_SIM_H

