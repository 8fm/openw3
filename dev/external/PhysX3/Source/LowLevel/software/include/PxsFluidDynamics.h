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


#ifndef PXS_FLUID_DYNAMICS_H
#define PXS_FLUID_DYNAMICS_H

#include "PxsFluidConfig.h"
#include "PxsFluidParticle.h"
#include "PxsFluidDynamicsParameters.h"
#include "PxsFluidDynamicsTempBuffers.h"
#include "CmBitMap.h"
#include "CmTask.h"

#ifdef PX_PS3
#include "CellFluidDynamicTask.h"
#endif

namespace physx
{

struct PxsParticleCell;
struct PxsFluidPacketSections;
struct PxsFluidPacketHaloRegions;

class PxsFluidDynamics
{
public:

	PxsFluidDynamics(class PxsParticleSystemSim& particleSystem);
	~PxsFluidDynamics();

	void init(bool isSph);
	void clear();
	
	void updateSph(PxBaseTask& continuation);

	PX_FORCE_INLINE PxsFluidDynamicsParameters& getParameter() { return mParams; }

private:

	// Table to get the neighboring halo region indices for a packet section
	struct SectionToHaloTable
	{
		PxU32	numHaloRegions;
		PxU32	haloRegionIndices[19];	// No packet section has more than 19 neighboring halo regions
	};

	struct OrderedIndexTable
	{	
		OrderedIndexTable();
		PxU32	indices[PXS_FLUID_SUBPACKET_PARTICLE_LIMIT_FORCE_DENSITY];
	};
	
	struct TaskData
	{
		PxU16 beginPacketIndex;
		PxU16 endPacketIndex;
	};

	void adjustTempBuffers(PxU32 count);

	void schedulePackets(PxsSphUpdateType updateType, PxBaseTask& continuation);
	void processPacketRange(PxU32 taskDataIndex);

	void updatePacket(PxsSphUpdateType updateType, PxVec3* forceBuf, PxsFluidParticle* particles, const PxsParticleCell& packet, const PxsFluidPacketSections& packetSections,
					  const PxsFluidPacketHaloRegions& haloRegions, struct PxsFluidDynamicsTempBuffers& tempBuffers);

	void updatePacketLocalHash(PxsSphUpdateType updateType, PxVec3* forceBuf, PxsFluidParticle* particles, const PxsParticleCell& packet,
							   const PxsFluidPacketSections& packetSections, const PxsFluidPacketHaloRegions& haloRegions, PxsFluidDynamicsTempBuffers& tempBuffers);

	void updateSubpacketPairHalo(PxVec3* __restrict forceBufA, PxsFluidParticle* __restrict particlesSpA, PxU32 numParticlesSpA, PxsParticleCell* __restrict particleCellsSpA, 
		PxU32* __restrict particleIndicesSpA, bool& isLocalHashSpAValid, PxU32 numCellHashBucketsSpA, 
		PxsFluidParticle* __restrict particlesSpB, PxU32 numParticlesSpB, PxsParticleCell* __restrict particleCellsSpB, 
		PxU32* __restrict particleIndicesSpB, const PxVec3& packetCorner, PxsSphUpdateType updateType, PxU16* __restrict hashKeyArray, PxsFluidDynamicsTempBuffers& tempBuffers);

	PX_FORCE_INLINE void updateParticlesBruteForceHalo(PxsSphUpdateType updateType, PxVec3* forceBuf, PxsFluidParticle* particles,
		const PxsFluidPacketSections& packetSections, const PxsFluidPacketHaloRegions& haloRegions, PxsFluidDynamicsTempBuffers& tempBuffers);

	void mergeDensity(PxBaseTask* continuation);
	void mergeForce(PxBaseTask* continuation);

private:

	PxsFluidDynamics& operator=(const PxsFluidDynamics&);
	static SectionToHaloTable sSectionToHaloTable[26];	// Halo region table for each packet section
	static OrderedIndexTable sOrderedIndexTable;

	class PxsParticleSystemSim& mParticleSystem;
	PxsFluidParticle* mTempReorderedParticles;
	PX_ALIGN(16, PxsFluidDynamicsParameters	mParams);
	PxVec3* mTempParticleForceBuf;

#ifdef PX_PS3
	CellFluidDynamicTask mDynamicSPU;	
#endif

	typedef Cm::DelegateTask<PxsFluidDynamics, &PxsFluidDynamics::mergeDensity> MergeDensityTask;
	typedef Cm::DelegateTask<PxsFluidDynamics, &PxsFluidDynamics::mergeForce> MergeForceTask;
	
	MergeDensityTask mMergeDensityTask;
	MergeForceTask mMergeForceTask;
	PxU32 mNumTasks;
	PxsSphUpdateType mCurrentUpdateType;
	PxU32 mNumTempBuffers;
	PxsFluidDynamicsTempBuffers mTempBuffers[PXS_FLUID_MAX_PARALLEL_TASKS_SPH];
	TaskData mTaskData[PXS_FLUID_MAX_PARALLEL_TASKS_SPH];
	friend class PxsFluidDynamicsSphTask;
};

}

#endif // PXS_FLUID_DYNAMICS_H
