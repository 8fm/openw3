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

#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuVecConvexHull.h"
#include "GuVecConvexHullNoScale.h"
#include "GuVecTriangle.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuTriangleMeshData.h"
#include "PxTriangleMesh.h"
#include "GuContactBuffer.h"
#include "GuHeightField.h"
#include "GuPCMContactConvexCommon.h"
#include "GuPCMContactMeshCallback.h"

using namespace physx;
using namespace Gu;

#if defined(__SPU__)
#include "CmMemFetch.h"
extern unsigned char HeightFieldBuffer[sizeof(physx::Gu::HeightField)+16];
#endif




namespace physx
{

struct PCMSphereVsHeightfieldContactGenerationCallback :  PCMHeightfieldContactGenerationCallback<PCMSphereVsHeightfieldContactGenerationCallback>
{

public:
	PCMSphereVsMeshContactGeneration		mGeneration;

	PCMSphereVsHeightfieldContactGenerationCallback(
		const Ps::aos::Vec3VArg		sphereCenter,
		const Ps::aos::FloatVArg	sphereRadius,
		const Ps::aos::FloatVArg	contactDistance,
		const Ps::aos::FloatVArg	replaceBreakingThreshold,
	
		const PsTransformV& sphereTransform, 
		const PsTransformV& heightfieldTransform,
		const PxTransform&	heightfieldTransform1,
		Gu::MultiplePersistentContactManifold& multiManifold,
		Gu::ContactBuffer& contactBuffer,
		Gu::HeightFieldUtil& hfUtil 
		
		
	) :
		PCMHeightfieldContactGenerationCallback<PCMSphereVsHeightfieldContactGenerationCallback>(hfUtil, heightfieldTransform1),
		mGeneration(sphereCenter, sphereRadius, contactDistance, replaceBreakingThreshold, sphereTransform, heightfieldTransform, multiManifold, contactBuffer)
	{
	}

	template<PxU32 CacheSize>
	void processTriangleCache(Gu::TriangleCache<CacheSize>& cache)
	{
		mGeneration.processTriangleCache<CacheSize, PCMSphereVsMeshContactGeneration>(cache);
	}

};


bool Gu::pcmContactSphereHeightField(GU_CONTACT_METHOD_ARGS)
{
	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();
	const PxHeightFieldGeometryLL& shapeHeight = shape1.get<const PxHeightFieldGeometryLL>();

	Gu::MultiplePersistentContactManifold& multiManifold = cache.getMultipleManifold();

	//gRenderOutPut = cache.mRenderOutput;

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV sphereRadius = FLoad(shapeSphere.radius);
	const FloatV contactDist = FLoad(contactDistance);
	
	const PsTransformV sphereTransform(p0, q0);//sphere transform
	const PsTransformV heightfieldTransform(p1, q1);//height feild
	const PsTransformV curTransform = heightfieldTransform.transformInv(sphereTransform);
	const PsMatTransformV aToB(curTransform);

	// We must be in local space to use the cache
	const FloatV replaceBreakingThreshold = FMul(sphereRadius, FLoad(0.001f));
	const FloatV projectBreakingThreshold = FMul(sphereRadius, FLoad(0.05f));
	const PxU32 previousTotalContacts = multiManifold.mNumTotalContacts;
	const FloatV refereshDistance = FAdd(sphereRadius, contactDist);
	multiManifold.refreshManifold(aToB, projectBreakingThreshold, refereshDistance);
	const bool bLostContacts = (multiManifold.mNumTotalContacts != previousTotalContacts);

	if(bLostContacts || multiManifold.invalidate(curTransform, sphereRadius, FLoad(0.02f)))
	{

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
		const PxVec3 sphereCenterShape1Space = transform1.transformInv(transform0.p);
		const Vec3V sphereCenter = V3LoadU(sphereCenterShape1Space);
		PxReal inflatedRadius = shapeSphere.radius + contactDistance;
		PxVec3 inflatedRadiusV(inflatedRadius);

		PxBounds3 bounds(sphereCenterShape1Space - inflatedRadiusV, sphereCenterShape1Space + inflatedRadiusV);

		PCMSphereVsHeightfieldContactGenerationCallback blockCallback(
			sphereCenter,
			sphereRadius,
			contactDist,
			replaceBreakingThreshold,
			sphereTransform,
			heightfieldTransform,
			transform1,
			multiManifold,
			contactBuffer,
			hfUtil);

		hfUtil.overlapAABBTriangles(transform1, bounds, 0, &blockCallback);

		blockCallback.mGeneration.processContacts(GU_SPHERE_MANIFOLD_CACHE_SIZE, false);
	}

	return multiManifold.addManifoldContactsToContactBuffer(contactBuffer, sphereTransform, heightfieldTransform, sphereRadius);
}


}