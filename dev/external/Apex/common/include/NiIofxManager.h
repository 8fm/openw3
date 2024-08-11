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

#ifndef __NI_IOFX_MANAGER_H__
#define __NI_IOFX_MANAGER_H__

#include "PsArray.h"
#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"
#include "PxMat33Legacy.h"

namespace physx
{

typedef unsigned long PxTaskID;

namespace apex
{
class NxIofxAsset;
class NxApexRenderVolume;

namespace iofx
{
#ifdef APEX_TEST
struct IofxManagerTestData;
#endif
}

template <class T>
class ApexMirroredArray;


class NiIofxManagerDesc
{
public:
	const char* iosAssetName;
	bool		iosOutputsOnDevice;
	bool		iosSupportsDensity;
	bool		iosSupportsCollision;
	PxU32		maxObjectCount;
	PxU32		maxInputCount;
	PxU32		maxInStateCount;
};

/// The IOFX will update the volumeID each simulation step, the IOS must
/// persist this output. IOS provides initial volumeID, based on emitter's
/// preferred volume.
struct NiIofxActorID
{
	PxU32		value;

	PX_CUDA_CALLABLE PX_INLINE NiIofxActorID() {}
	PX_CUDA_CALLABLE PX_INLINE explicit NiIofxActorID(PxU32 arg)
	{
		value = arg;
	}

	PX_CUDA_CALLABLE PX_INLINE void set(PxU16 volumeID, PxU16 actorClassID)
	{
		value = (PxU32(volumeID) << 16) | PxU32(actorClassID);
	}

	PX_CUDA_CALLABLE PX_INLINE PxU16 getVolumeID() const
	{
		return PxU16(value >> 16);
	}
	PX_CUDA_CALLABLE PX_INLINE void setVolumeID(PxU16 volumeID)
	{
		value &= 0x0000FFFFu;
		value |= (PxU32(volumeID) << 16);
	}

	PX_CUDA_CALLABLE PX_INLINE PxU16 getActorClassID() const
	{
		return PxU16(value & 0xFFFFu);
	}
	PX_CUDA_CALLABLE PX_INLINE void setActorClassID(PxU16 actorClassID)
	{
		value &= 0xFFFF0000u;
		value |= PxU32(actorClassID);
	}

	static const physx::PxU16 NO_VOLUME = 0xFFFFu;
	static const physx::PxU16 INV_ACTOR = 0xFFFFu;

	static const physx::PxU16 ASSETS_PER_MATERIAL_BITS = 6;
	static const physx::PxU16 ASSETS_PER_MATERIAL = (1 << ASSETS_PER_MATERIAL_BITS);
};


/* IOFX Manager returned pointers for simulation data */
class NiIosBufferDesc
{
public:
	/* All arrays are indexed by input ID */
	ApexMirroredArray<PxVec4>*			pmaPositionMass;
	ApexMirroredArray<PxVec4>*			pmaVelocityLife;
	ApexMirroredArray<PxVec4>*			pmaCollisionNormalFlags;
	ApexMirroredArray<PxF32>*			pmaDensity;
	ApexMirroredArray<NiIofxActorID>*	pmaActorIdentifiers;
	ApexMirroredArray<PxU32>*			pmaInStateToInput;
	ApexMirroredArray<PxU32>*			pmaOutStateToInput;

	ApexMirroredArray<PxU32>*			pmaUserData;

	//< Value in inStateToInput field indicates a dead particle, input to IOFX
	static const physx::PxU32 NOT_A_PARTICLE = 0xFFFFFFFFu;

	//< Flag in inStateToInput field indicates a new particle, input to IOFX
	static const physx::PxU32 NEW_PARTICLE_FLAG = 0x80000000u;
};

// This is a representative of uint4 on host
struct IofxSlice
{
	PxU32 x, y, z, w;
};

typedef void (*EventCallback)(void*);

class NiIofxManagerCallback
{
public:
	virtual void operator()(void*) = 0;
};

class NiIofxManager
{
public:
	//! An IOS Actor will call this once, at creation
	virtual void createSimulationBuffers(NiIosBufferDesc& outDesc) = 0;

	//! An IOS actor will call this once, when it creates its fluid simulation
	virtual void setSimulationParameters(PxF32 radius, const PxVec3& up, PxF32 gravity, PxF32 restDensity) = 0;

	//! An IOS Actor will call this method after each simulation step
	virtual void updateEffectsData(PxF32 deltaTime, PxU32 numObjects, PxU32 maxInputID, PxU32 maxStateID) = 0;

	//! An IOS Actor will call this method at the start of each step IOFX will run
	virtual PxTaskID getUpdateEffectsTaskID(PxTaskID) = 0;

	virtual PxU16 getActorClassID(physx::apex::NxIofxAsset* asset, PxU16 meshID) = 0;

	virtual void releaseAssetID(physx::apex::NxIofxAsset* asset) = 0;

	virtual PxU16 getVolumeID(physx::apex::NxApexRenderVolume* vol) = 0;

	//! Triggers the IOFX Manager to copy host buffers to the device
	//! This is intended for use in an IOS post-update task, if they
	//! need the output buffers on the device.
	virtual void outputHostToDevice() = 0;
	
	//! NiIofxManagerCallback will be called before Iofx computations
	virtual void setOnStartCallback(NiIofxManagerCallback*) = 0;
	//! NiIofxManagerCallback will be called after Iofx computations
	virtual void setOnFinishCallback(NiIofxManagerCallback*) = 0;

	//! Called when IOS is being deleted
	virtual void release() = 0;

	//get bounding box
	virtual PxBounds3 getBounds() const = 0;

#ifdef APEX_TEST
	virtual iofx::IofxManagerTestData* createTestData() = 0;
	virtual void copyTestData() const = 0;
	virtual void clearTestData() = 0;
#endif
protected:
	virtual ~NiIofxManager() {}
};


}
} // end namespace physx::apex

#endif // __NI_IOFX_MANAGER_H__
