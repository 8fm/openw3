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
#include "PxsParticleSystemBatcher.h"
#include "PxsContext.h"
#include "PxvParticleSystemSim.h"
#include "PxsParticleSystemSim.h"

#if PX_USE_PARTICLE_SYSTEM_API

#if PX_SUPPORT_GPU_PHYSX
#include "PxPhysXGpu.h"
#endif

using namespace physx;

namespace
{
	template<class T>
	static void sortBatchedInputs(PxvParticleSystemSim** particleSystems, T* inputs, PxU32 batchSize, PxU32& cpuOffset, PxU32& cpuCount, PxU32& gpuOffset, PxU32& gpuCount)
	{
		cpuOffset = 0;
		gpuOffset = 0;

		//in place sort of both arrays
		PxU32 i = 0;
		PxU32 j = 0;
		
		while ((i < batchSize) && (j < batchSize))
		{
#if PX_SUPPORT_GPU_PHYSX
			if (particleSystems[i]->isGpuV())
			{
				j = i+1;
				while (j < batchSize && particleSystems[j]->isGpuV())
				{
					j++;
				}

				if (j < batchSize)
				{
					Ps::swap(particleSystems[i], particleSystems[j]);
					if (inputs)
					{
						Ps::swap(inputs[i], inputs[j]);
					}
					i++;
				}
			}
			else
#endif
			{
				i++;
			}
		}
		
		gpuOffset = i;
		cpuCount = gpuOffset;
		gpuCount = batchSize - cpuCount;			
	}

	PxBaseTask& scheduleShapeGenerationImpl(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxvParticleShapesUpdateInput* inputs, PxU32 batchSize, PxBaseTask& continuation)
	{
		PxU32 cpuOffset = 0;
		PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
		PxU32 gpuOffset, gpuCount;
		sortBatchedInputs(particleSystems, inputs, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
		if (batcher.context.getSceneGpu(false) && gpuCount > 0)
		{
			PxBaseTask& task = batcher.context.getSceneGpu(false)->scheduleParticleShapeUpdate(particleSystems + gpuOffset, inputs + gpuOffset, gpuCount, continuation);
			batcher.shapeGenTask.addDependent(task);
			task.removeReference();
		}
#endif

		for (PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
		{
			PxBaseTask& task = static_cast<PxsParticleSystemSim*>(particleSystems[i])->schedulePacketShapesUpdate(inputs[i], continuation);
			batcher.shapeGenTask.addDependent(task);
			task.removeReference();
		}

		if (batcher.shapeGenTask.getReference() > 0)
			return batcher.shapeGenTask;

		continuation.addReference();
		return continuation;
	}

	PxBaseTask& scheduleDynamicsCpuImpl(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
	{
		PxU32 cpuOffset = 0;
		PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
		PxU32 gpuOffset, gpuCount;
		sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
#endif
		for (PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
		{
			PxBaseTask& task = static_cast<PxsParticleSystemSim*>(particleSystems[i])->scheduleDynamicsUpdate(continuation);
			batcher.dynamicsCpuTask.addDependent(task);
			task.removeReference();
		}

		if (batcher.dynamicsCpuTask.getReference() > 0)
			return batcher.dynamicsCpuTask;

		continuation.addReference();
		return continuation;
	}

	PxBaseTask& scheduleCollisionPrepImpl(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks, PxU32 batchSize, PxBaseTask& continuation)
	{
		PxU32 cpuOffset = 0;
		PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
		PxU32 gpuOffset, gpuCount;
		sortBatchedInputs(particleSystems, inputPrepTasks, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
		if (batcher.context.getSceneGpu(false) && gpuCount > 0)
		{
			PxBaseTask& gpuCollisionInputTask = batcher.context.getSceneGpu(false)->scheduleParticleCollisionInputUpdate(particleSystems + gpuOffset, gpuCount, continuation);
			for (PxU32 i = gpuOffset; i < (gpuOffset + gpuCount); ++i)
			{
				inputPrepTasks[i]->setContinuation(&gpuCollisionInputTask);
				batcher.collPrepTask.addDependent(*inputPrepTasks[i]);
				inputPrepTasks[i]->removeReference();
			}
			gpuCollisionInputTask.removeReference();
		}
#else
		PX_UNUSED(particleSystems);
		PX_UNUSED(batchSize);
		PX_UNUSED(batcher);
#endif
		for (PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
		{
			inputPrepTasks[i]->setContinuation(&continuation);
			batcher.collPrepTask.addDependent(*inputPrepTasks[i]);
			inputPrepTasks[i]->removeReference();
		}

		if (batcher.collPrepTask.getReference() > 0)
			return batcher.collPrepTask;

		continuation.addReference();
		return continuation;
	}

	PxBaseTask& scheduleCollisionCpuImpl(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
	{
		PxU32 cpuOffset = 0;
		PxU32 cpuCount = batchSize;
#if PX_SUPPORT_GPU_PHYSX
		PxU32 gpuOffset, gpuCount;
		sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
#endif
		for (PxU32 i = cpuOffset; i < (cpuOffset + cpuCount); ++i)
		{
			PxBaseTask& task = static_cast<PxsParticleSystemSim*>(particleSystems[i])->scheduleCollisionUpdate(continuation);
			batcher.collisionCpuTask.addDependent(task);
			task.removeReference();
		}

		if (batcher.collisionCpuTask.getReference() > 0)
			return batcher.collisionCpuTask;

		continuation.addReference();
		return continuation;
	}

	PxBaseTask& schedulePipelineGpuImpl(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
	{
#if PX_SUPPORT_GPU_PHYSX
		PxU32 cpuOffset, cpuCount, gpuOffset, gpuCount;
		sortBatchedInputs(particleSystems, (PxU8*)NULL, batchSize, cpuOffset, cpuCount, gpuOffset, gpuCount);
		if (batcher.context.getSceneGpu(false) && gpuCount > 0)
		{
			return batcher.context.getSceneGpu(false)->scheduleParticlePipeline(particleSystems + gpuOffset, gpuCount, continuation);
		}
#else
		PX_UNUSED(batchSize);
		PX_UNUSED(batcher);
		PX_UNUSED(particleSystems);
#endif
		continuation.addReference();
		return continuation;
	}

	PxBaseTask& (*sScheduleShapeGenerationFn)	(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxvParticleShapesUpdateInput* inputs, PxU32 batchSize, PxBaseTask& continuation)	= 0;
	PxBaseTask& (*sScheduleDynamicsCpuFn)		(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)										= 0;
	PxBaseTask& (*sScheduleCollisionPrepFn)	(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks, PxU32 batchSize, PxBaseTask& continuation)											= 0;
	PxBaseTask& (*sScheduleCollisionCpuFn)	(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)										= 0;
	PxBaseTask& (*sSchedulePipelineGpuFn)		(PxsParticleSystemBatcher& batcher, PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)										= 0;
}

PxsParticleSystemBatcher::PxsParticleSystemBatcher(class PxsContext& _context) 
: shapeGenTask("PxsParticleSystemBatcher::shapeGen")
, dynamicsCpuTask("PxsParticleSystemBatcher::dynamicsCpu")
, collPrepTask("PxsParticleSystemBatcher::collPrep")
, collisionCpuTask("PxsParticleSystemBatcher::collisionCpu")
, context(_context)
{}

PxBaseTask& PxsParticleSystemBatcher::scheduleShapeGeneration(PxvParticleSystemSim** particleSystems, PxvParticleShapesUpdateInput* inputs, PxU32 batchSize, PxBaseTask& continuation)
		{
	return sScheduleShapeGenerationFn(*this, particleSystems, inputs, batchSize, continuation);
	}

PxBaseTask& PxsParticleSystemBatcher::scheduleDynamicsCpu(PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	return sScheduleDynamicsCpuFn(*this, particleSystems, batchSize, continuation);
}

PxBaseTask& PxsParticleSystemBatcher::scheduleCollisionPrep(PxvParticleSystemSim** particleSystems, PxLightCpuTask** inputPrepTasks, PxU32 batchSize, PxBaseTask& continuation)
{
	return sScheduleCollisionPrepFn(*this, particleSystems, inputPrepTasks, batchSize, continuation);
}

PxBaseTask& PxsParticleSystemBatcher::scheduleCollisionCpu(PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	return sScheduleCollisionCpuFn(*this, particleSystems, batchSize, continuation);
}

PxBaseTask& PxsParticleSystemBatcher::schedulePipelineGpu(PxvParticleSystemSim** particleSystems, PxU32 batchSize, PxBaseTask& continuation)
{
	return sSchedulePipelineGpuFn(*this, particleSystems, batchSize, continuation);
}

void PxsParticleSystemBatcher::registerParticles()
{
	sScheduleShapeGenerationFn = ::scheduleShapeGenerationImpl;
	sScheduleDynamicsCpuFn = ::scheduleDynamicsCpuImpl;
	sScheduleCollisionPrepFn = ::scheduleCollisionPrepImpl;
	sScheduleCollisionCpuFn = ::scheduleCollisionCpuImpl;
	sSchedulePipelineGpuFn = ::schedulePipelineGpuImpl;
}


#endif // PX_USE_PARTICLE_SYSTEM_API
