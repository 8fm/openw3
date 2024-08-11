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

#include "GuVecTriangle.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuTriangleMeshData.h"
#include "PxTriangleMesh.h"
#include "GuContactBuffer.h"
#include "GuHeightField.h"
#include "GuPCMContactConvexCommon.h"
#include "GuSegment.h"
#include "GuGeomUtilsInternal.h"
#include "GuPCMContactMeshCallback.h"

using namespace physx;
using namespace Gu;

#if defined(__SPU__)
#include "CmMemFetch.h"
extern unsigned char HeightFieldBuffer[sizeof(physx::Gu::HeightField)+16];
#endif




namespace physx
{

struct PCMCapsuleVsHeightfieldContactGenerationCallback :  PCMHeightfieldContactGenerationCallback<PCMCapsuleVsHeightfieldContactGenerationCallback>
{

public:
	PCMCapsuleVsMeshContactGeneration		mGeneration;

	PCMCapsuleVsHeightfieldContactGenerationCallback(
		const Gu::CapsuleV&			capsule,
		const Ps::aos::FloatVArg	contactDistance,
		const Ps::aos::FloatVArg	replaceBreakingThreshold,
	
		const PsTransformV& capsuleTransform, 
		const PsTransformV& heightfieldTransform,
		const PxTransform&	heightfieldTransform1,
		Gu::MultiplePersistentContactManifold& multiManifold,
		Gu::ContactBuffer& contactBuffer,
		Gu::HeightFieldUtil& hfUtil 
		
		
	) :
		PCMHeightfieldContactGenerationCallback<PCMCapsuleVsHeightfieldContactGenerationCallback>(hfUtil, heightfieldTransform1),
		mGeneration(capsule, contactDistance, replaceBreakingThreshold, capsuleTransform, heightfieldTransform, multiManifold, contactBuffer)
	{
	}

	template<PxU32 CacheSize>
	void processTriangleCache(Gu::TriangleCache<CacheSize>& cache)
	{
		mGeneration.processTriangleCache<CacheSize, PCMCapsuleVsMeshContactGeneration>(cache);
	}
	
};

bool Gu::pcmContactCapsuleHeightField(GU_CONTACT_METHOD_ARGS)
{
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxHeightFieldGeometryLL& shapeHeight = shape1.get<const PxHeightFieldGeometryLL>();

	Gu::MultiplePersistentContactManifold& multiManifold = cache.getMultipleManifold();

	//gRenderOutPut = cache.mRenderOutput;

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV capsuleRadius = FLoad(shapeCapsule.radius);
	const FloatV contactDist = FLoad(contactDistance);
	
	const PsTransformV capsuleTransform(p0, q0);//sphere transform
	const PsTransformV heightfieldTransform(p1, q1);//height feild
	const PsTransformV curTransform = heightfieldTransform.transformInv(capsuleTransform);
	const PsMatTransformV aToB(curTransform);

	// We must be in local space to use the cache
	const FloatV replaceBreakingThreshold = FMul(capsuleRadius, FLoad(0.001f));
	const FloatV projectBreakingThreshold = FMul(capsuleRadius, FLoad(0.05f));
	const PxU32 previousTotalContacts = multiManifold.mNumTotalContacts;
	const FloatV refereshDistance = FAdd(capsuleRadius, contactDist);
	//multiManifold.refreshManifold(aToB, projectBreakingThreshold, contactDist);
	multiManifold.refreshManifold(aToB, projectBreakingThreshold, refereshDistance);
	const bool bLostContacts = (multiManifold.mNumTotalContacts != previousTotalContacts);

	if(bLostContacts || multiManifold.invalidate(curTransform, capsuleRadius, FLoad(0.02f)))
	{

		multiManifold.mNumManifolds = 0;
		multiManifold.setRelativeTransform(curTransform); 
#ifdef __SPU__
		const Gu::HeightField& hf = *Cm::memFetchAsync<const Gu::HeightField>(HeightFieldBuffer, Cm::MemFetchPtr(static_cast<Gu::HeightField*>(shapeHeight.heightField)), sizeof(Gu::HeightField), 1);
		Cm::memFetchWait(1);
#if HF_TILED_MEMORY_LAYOUT
		g_sampleCache.init((uintptr_t)(hf.getData().samples), hf.getData().tilesU);
#endif
#else
		const Gu::HeightField& hf = *static_cast<Gu::HeightField*>(shapeHeight.heightField);
#endif

		Gu::HeightFieldUtil hfUtil(shapeHeight, hf);

		const PxVec3 tmp = transform0.q.getBasisVector0() * shapeCapsule.halfHeight;

		//// Capsule data
		//Gu::Segment worldCapsule;
		//worldCapsule.p0 = transform0.p + tmp;
		//worldCapsule.p1 = transform0.p - tmp;

		//
		//const Gu::Segment meshCapsule(	// Capsule in mesh space
		//	transform1.transformInv(worldCapsule.p0),
		//	transform1.transformInv(worldCapsule.p1));

		
		const PxReal inflatedRadius = shapeCapsule.radius + contactDistance;

		const PxVec3 capsuleCenterInMesh = transform1.transformInv(transform0.p);
		const PxVec3 capsuleDirInMesh = transform1.rotateInv(tmp);
		const Gu::CapsuleV capsule(V3LoadU(capsuleCenterInMesh), V3LoadU(capsuleDirInMesh), capsuleRadius);


		PCMCapsuleVsHeightfieldContactGenerationCallback callback(
			capsule,
			contactDist,
			replaceBreakingThreshold,
			capsuleTransform,
			heightfieldTransform,
			transform1,
			multiManifold,
			contactBuffer,
			hfUtil
		);

		PxBounds3 bounds;
		bounds.maximum = PxVec3(shapeCapsule.halfHeight + inflatedRadius, inflatedRadius, inflatedRadius);
		bounds.minimum = -bounds.maximum;

		bounds = PxBounds3::transformFast(transform1.transformInv(transform0), bounds);

		hfUtil.overlapAABBTriangles(transform1, bounds, 0, &callback);

		callback.mGeneration.processContacts(GU_CAPSULE_MANIFOLD_CACHE_SIZE, false);
	}

	//multiManifold.drawManifold(*gRenderOutPut, capsuleTransform, heightfieldTransform);
	return multiManifold.addManifoldContactsToContactBuffer(contactBuffer, capsuleTransform, heightfieldTransform, capsuleRadius);
}


}