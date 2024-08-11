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


#ifndef PX_PHYSICS_NP_FACTORY
#define PX_PHYSICS_NP_FACTORY

#include "PsPool.h"
#include "PsMutex.h"
#include "PsHashSet.h"

#include "GuMeshFactory.h"
#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#include "PxShape.h"

#if PX_USE_CLOTH_API
#include "cloth/PxClothTypes.h"
#include "cloth/PxClothFabric.h"
#endif

namespace physx
{

class PxActor;

class PxRigidActor;
class PxRigidStatic;

class PxRigidDynamic;

class NpConnectorArray;

struct PxConstraintShaderTable;
class PxConstraintConnector;
class PxConstraint;

class PxArticulation;
class NpArticulation;
class PxArticulationLink;
class NpArticulationLink;

class PxParticleBase;
class PxParticleSystem;

class PxParticleFluid;

class PxClothFabric;
class NpClothFabric;
class PxCloth;
class PxMaterial;

class PxGeometry;

class NpShape;

class NpScene;

class PxAggregate;
class NpConnectorArray;
class PxInputStream;

namespace Scb
{
	class RigidObject;
}

class NpFactoryListener : public GuMeshFactoryListener
{
protected:
	virtual ~NpFactoryListener(){}
public:
#if PX_USE_CLOTH_API
	virtual void onNpFactoryBufferRelease(PxClothFabric& data) = 0;
#endif
};

class NpFactory : public GuMeshFactory
{
public:
												NpFactory();
private:
												~NpFactory();

public:
	static		void							createInstance();
	static		void							destroyInstance();
	static		void							registerArticulations();
	static		void							registerCloth();
	static		void							registerParticles();

				void							release();

				void							addCollection(PxU32 nbObjects, PxBase*const* objects);

	PX_INLINE static NpFactory&					getInstance() { return *mInstance; }

				PxRigidDynamic*					createRigidDynamic(const PxTransform& pose);
				void							addRigidDynamic(PxRigidDynamic*, bool lock=true);

				PxRigidStatic*					createRigidStatic(const PxTransform& pose);
				void							addRigidStatic(PxRigidStatic*, bool lock=true);

				NpShape*						createShape(const PxGeometry& geometry,
															PxShapeFlags shapeFlags,
															PxMaterial*const* materials,
															PxU16 materialCount,
															bool isExclusive);

				void							addShape(PxShape*, bool lock=true);

				PxU32							getNbShapes() const;
				PxU32							getShapes(PxShape** userBuffer, PxU32 bufferSize, PxU32 startIndex)	const;

				void							addConstraint(PxConstraint*, bool lock=true);
				PxConstraint*					createConstraint(PxRigidActor* actor0, PxRigidActor* actor1, PxConstraintConnector& connector, const PxConstraintShaderTable& shaders, PxU32 dataSize);

				void							addArticulation(PxArticulation*, bool lock=true);
				PxArticulation*					createArticulation();
				PxArticulationLink*				createArticulationLink(NpArticulation&, NpArticulationLink* parent, const PxTransform& pose);

				void							addAggregate(PxAggregate*, bool lock=true);
				PxAggregate*					createAggregate(PxU32 maxActors, bool selfCollisions);


#if PX_USE_PARTICLE_SYSTEM_API
				void							addParticleSystem(PxParticleSystem*, bool lock=true);
				PxParticleSystem*				createParticleSystem(PxU32 maxParticles, bool perParticleRestOffset);
				void							addParticleFluid(PxParticleFluid*, bool lock=true);
				PxParticleFluid*				createParticleFluid(PxU32 maxParticles, bool perParticleRestOffset);
#endif

#if PX_USE_CLOTH_API
				void							addCloth(PxCloth* cloth, bool lock=true);
				void							addClothFabric(NpClothFabric* cf, bool lock=true);
				PxClothFabric*					createClothFabric(PxInputStream&);
				PxClothFabric*					createClothFabric(const PxClothFabricDesc& desc);
				bool							removeClothFabric(PxClothFabric&);
				PxU32							getNbClothFabrics()	const;
				PxU32							getClothFabrics(PxClothFabric** userBuffer, PxU32 bufferSize) const;

				PxCloth*						createCloth(const PxTransform& globalPose, PxClothFabric& fabric, const PxClothParticle* particles, PxClothFlags flags);
#endif

				PxMaterial*						createMaterial(PxReal staticFriction, PxReal dynamicFriction, PxReal restitution);

				// It's easiest to track these uninvasively, so it's OK to use the Px pointers

				void							onActorRelease(PxActor*);
				void							onConstraintRelease(PxConstraint*);
				void							onAggregateRelease(PxAggregate*);
				void							onArticulationRelease(PxArticulation*);
				void							onShapeRelease(PxShape*);

				NpConnectorArray*				acquireConnectorArray();
				void							releaseConnectorArray(NpConnectorArray*);
				
#if PX_SUPPORT_VISUAL_DEBUGGER
				void							setNpFactoryListener( NpFactoryListener& );
#endif

private:

						void					releaseExclusiveShapeUserReferences();
#if PX_SUPPORT_GPU_PHYSX
				virtual void					notifyReleaseTriangleMesh(const PxTriangleMesh& tm);
				virtual	void					notifyReleaseHeightField(const PxHeightField& hf);
				virtual	void					notifyReleaseConvexMesh(const PxConvexMesh& cm);
#endif

				Ps::Pool<NpConnectorArray>		mConnectorArrayPool;
				Ps::Mutex						mConnectorArrayPoolLock;

				Ps::HashSet<PxAggregate*>		mAggregateTracking;
				Ps::HashSet<PxArticulation*>	mArticulationTracking;
				Ps::HashSet<PxConstraint*>		mConstraintTracking;
				Ps::HashSet<PxActor*>			mActorTracking;
				Ps::HashSet<PxMaterial*>		mMaterialTracking;
				Ps::CoalescedHashSet<PxShape*>	mShapeTracking;

#if PX_USE_CLOTH_API
				Ps::Array<NpClothFabric*>		mClothFabricArray;
#endif

	static		NpFactory*						mInstance;

#if PX_SUPPORT_VISUAL_DEBUGGER
				NpFactoryListener*				mNpFactoryListener;
#endif

};


}

#endif
