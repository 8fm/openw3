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
#include "GuVecConvexHull.h"
#include "GuVecConvexHullNoScale.h"
#include "GuVecTriangle.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuPCMShapeConvex.h"
#include "GuTriangleMeshData.h"
#include "GuConvexUtilsInternal.h"
#include "PxTriangleMesh.h"
#include "GuContactBuffer.h"
#include "OPC_OBBCollider.h"
#include "GuPCMContactConvexCommon.h"
#include "GuPCMContactMeshCallback.h"

using namespace physx;
using namespace Gu;

#ifdef	PCM_LOW_LEVEL_DEBUG
#include "CmRenderOutput.h"
extern physx::Cm::RenderOutput* gRenderOutPut;
#endif


namespace physx
{

struct PCMConvexVsMeshContactGenerationCallback : PCMMeshContactGenerationCallback<PCMConvexVsMeshContactGenerationCallback>
{

public:
	PCMConvexVsMeshContactGeneration		mGeneration;

	PCMConvexVsMeshContactGenerationCallback(
		const Ps::aos::FloatVArg				contactDistance,
		const Ps::aos::FloatVArg				replaceBreakingThreshold,
		const PsTransformV&						convexTransform, 
		const PsTransformV&						meshTransform,
		Gu::MultiplePersistentContactManifold&	multiManifold,
		Gu::ContactBuffer&						contactBuffer,
		const Gu::PolygonalData&				polyData,
		SupportLocal*							polyMap,
		Gu::Container&							delayedContacts,
		const Cm::FastVertex2ShapeScaling&		convexScaling,
		bool									idtConvexScale,
		const Cm::FastVertex2ShapeScaling&		meshScaling,
		const PxU8*								extraTriData,
		bool									idtMeshScale
		
	) :
		PCMMeshContactGenerationCallback<PCMConvexVsMeshContactGenerationCallback>(meshScaling, extraTriData, idtMeshScale),
		mGeneration(contactDistance, replaceBreakingThreshold, convexTransform, meshTransform, multiManifold, contactBuffer, polyData, polyMap, delayedContacts, convexScaling, idtConvexScale)
		
	{
	}

	template<PxU32 CacheSize>
	void processTriangleCache(Gu::TriangleCache<CacheSize>& cache)
	{
		mGeneration.processTriangleCache<CacheSize, PCMConvexVsMeshContactGeneration>(cache);
	}
	
};


bool Gu::PCMContactConvexMesh(const Gu::PolygonalData& polyData, Gu::SupportLocal* polyMap, const Ps::aos::FloatVArg minMargin, const PxBounds3& hullAABB, const PxTriangleMeshGeometryLL& shapeMesh,
						const PxTransform& transform0, const PxTransform& transform1,
						PxReal contactDistance, Gu::ContactBuffer& contactBuffer,
						const Cm::FastVertex2ShapeScaling& convexScaling, const Cm::FastVertex2ShapeScaling& meshScaling,
						bool idtConvexScale, bool idtMeshScale, Gu::MultiplePersistentContactManifold& multiManifold)

{

	using namespace Ps::aos;

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const QuatV q1 = QuatVLoadA(&transform1.q.x);
	const Vec3V p1 = V3LoadA(&transform1.p.x);

	const FloatV contactDist = FLoad(contactDistance);
	//Transfer A into the local space of B
	const PsTransformV convexTransform(p0, q0);//box
	const PsTransformV meshTransform(p1, q1);//triangleMesh  
	const PsTransformV curTransform = meshTransform.transformInv(convexTransform);
	const PsMatTransformV aToB(curTransform);

	
	const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));

	const PxU32 previousTotalContacts = multiManifold.mNumTotalContacts;
	//multiManifold.refreshManifold(aToB, projectBreakingThreshold, contactDist);

	bool invalidate = multiManifold.invalidate(curTransform, minMargin)!=0;

	if(!invalidate)
	{
		const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
		multiManifold.refreshManifold(aToB, projectBreakingThreshold, contactDist);
	}

	const bool bLostContacts = (multiManifold.mNumTotalContacts != previousTotalContacts);
	
	if(bLostContacts || invalidate)
	{
		multiManifold.mNumManifolds = 0;
		multiManifold.setRelativeTransform(curTransform); 

	
	////////////////////

		const Gu::InternalTriangleMeshData* PX_RESTRICT meshData = shapeMesh.meshData;

		////////////////////

		const Cm::Matrix34 world0(transform0);
		const Cm::Matrix34 world1(transform1);
		Gu::Box hullOBB;
		Gu::computeHullOBB(hullOBB, hullAABB, contactDistance, transform0, world0, world1, meshScaling, idtMeshScale);

		// Setup the collider
		Gu::RTreeMidphaseData hmd;	
	#if defined(__SPU__)
		// fetch meshData to temp buffer
		PX_ALIGN_PREFIX(16) char meshDataBuf[sizeof(Gu::InternalTriangleMeshData)] PX_ALIGN_SUFFIX(16);
		Cm::memFetchAlignedAsync(Cm::MemFetchPtr(meshDataBuf), Cm::MemFetchPtr(shapeMesh.meshData), sizeof(Gu::InternalTriangleMeshData), 5);
		Cm::memFetchWait(5);
		meshData = reinterpret_cast<const Gu::InternalTriangleMeshData*>(meshDataBuf);
	#endif
		meshData->mOpcodeModel.getRTreeMidphaseData(hmd);

		LocalContainer(delayedContacts, PCM_LOCAL_CONTACTS_SIZE);

		const PxU8* PX_RESTRICT extraData = meshData->mExtraTrigData;
		PCMConvexVsMeshContactGenerationCallback blockCallback(
			contactDist,
			replaceBreakingThreshold,
			convexTransform, 
			meshTransform,
			multiManifold,
			contactBuffer,
			polyData,
			polyMap,
			delayedContacts,
			convexScaling, 
			idtConvexScale,
			meshScaling,
			extraData,
			idtMeshScale
		);

		Gu::HybridOBBCollider collider;
		collider.SetPrimitiveTests(true);	

		collider.Collide<1,1,0>(hullOBB, hmd, &blockCallback, NULL, NULL);


		PX_ASSERT(multiManifold.mNumManifolds <= GU_MAX_MANIFOLD_SIZE);
		//This is very important
		blockCallback.mGeneration.generateLastContacts();
		blockCallback.mGeneration.processContacts(GU_SINGLE_MANIFOLD_CACHE_SIZE, false);
	}

	//multiManifold.drawManifold(*gRenderOutPut, convexTransform, meshTransform);
	return multiManifold.addManifoldContactsToContactBuffer(contactBuffer, meshTransform);

}

bool Gu::pcmContactConvexMesh(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	const PxConvexMeshGeometryLL& shapeConvex = shape0.get<const PxConvexMeshGeometryLL>();
	const PxTriangleMeshGeometryLL& shapeMesh = shape1.get<const PxTriangleMeshGeometryLL>();

	const Gu::ConvexHullData* hullData = shapeConvex.hullData;
	Gu::MultiplePersistentContactManifold& multiManifold = cache.getMultipleManifold();

	//gRenderOutPut = cache.mRenderOutput;

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const PsTransformV convexTransform(p0, q0);

	const bool idtScaleMesh = shapeMesh.scale.isIdentity();

	Cm::FastVertex2ShapeScaling meshScaling;
	if(!idtScaleMesh)
		meshScaling.init(shapeMesh.scale);

	Cm::FastVertex2ShapeScaling convexScaling;
	PxBounds3 hullAABB;
	PolygonalData polyData;
	const bool idtScaleConvex = getPCMConvexData(shape0, convexScaling, hullAABB, polyData);

	const Vec3V vScale = V3LoadU(shapeConvex.scale.scale);
	const FloatV convexMargin = Gu::CalculatePCMConvexMargin(hullData, vScale);
	const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);
	Gu::ConvexHullV convexHull(hullData, V3Zero(), vScale, vQuat);

	if(idtScaleConvex)
	{
		//SupportLocalShrinkImpl<Gu::ConvexHullNoScaleV, Gu::ShrinkedConvexHullNoScaleV> convexMap((ConvexHullNoScaleV&)convexHull, convexTransform, convexHull.vertex2Shape, convexHull.shape2Vertex);
		SupportLocalImpl<Gu::ConvexHullNoScaleV> convexMap((ConvexHullNoScaleV&)convexHull, convexTransform, convexHull.vertex2Shape, convexHull.shape2Vertex, true);
		return Gu::PCMContactConvexMesh(polyData, &convexMap, convexMargin, hullAABB, shapeMesh,transform0,transform1, contactDistance, contactBuffer, convexScaling,  meshScaling, idtScaleConvex, idtScaleMesh, multiManifold);
	}
	else
	{
		//SupportLocalShrinkImpl<Gu::ConvexHullV, Gu::ShrinkedConvexHullV> convexMap(convexHull, convexTransform, convexHull.vertex2Shape, convexHull.shape2Vertex);
		SupportLocalImpl<Gu::ConvexHullV> convexMap((ConvexHullV&)convexHull, convexTransform, convexHull.vertex2Shape, convexHull.shape2Vertex, false);
		return Gu::PCMContactConvexMesh(polyData, &convexMap, convexMargin, hullAABB, shapeMesh,transform0,transform1, contactDistance, contactBuffer, convexScaling,  meshScaling, idtScaleConvex, idtScaleMesh, multiManifold);
	}
	return false;
}

bool Gu::pcmContactBoxMesh(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;

	MultiplePersistentContactManifold& multiManifold = cache.getMultipleManifold();

	const PxBoxGeometry& shapeBox = shape0.get<const PxBoxGeometry>();
	const PxTriangleMeshGeometryLL& shapeMesh = shape1.get<const PxTriangleMeshGeometryLL>();

	//gRenderOutPut = cache.mRenderOutput;


	const PxBounds3 hullAABB(-shapeBox.halfExtents, shapeBox.halfExtents);  

	const bool idtMeshScale = shapeMesh.scale.isIdentity();

	Cm::FastVertex2ShapeScaling meshScaling;
	if(!idtMeshScale)
		meshScaling.init(shapeMesh.scale);

	Cm::FastVertex2ShapeScaling idtScaling;

	//const Cm::Matrix34 world0(transform0);
	//const Cm::Matrix34 world1(transform1);

	const QuatV q0 = QuatVLoadA(&transform0.q.x);
	const Vec3V p0 = V3LoadA(&transform0.p.x);

	const Vec3V boxExtents = V3LoadU(shapeBox.halfExtents);
	const FloatV minMargin = Gu::CalculatePCMBoxMargin(boxExtents);

	Gu::BoxV boxV(V3Zero(), boxExtents);

	const PsTransformV boxTransform(p0, q0);//box

	Gu::PolygonalData polyData;
	Gu::PCMPolygonalBox polyBox(shapeBox.halfExtents);
	polyBox.getPolygonalData(&polyData);

	Mat33V identity =  M33Identity();
	SupportLocalImpl<Gu::BoxV> boxMap(boxV, boxTransform, identity, identity, true);
	//SupportLocalShrinkImpl<Gu::BoxV, Gu::ShrinkedBoxV> boxMap(boxV, boxTransform, identity, identity);

	return Gu::PCMContactConvexMesh(polyData, &boxMap, minMargin, hullAABB, shapeMesh,transform0,transform1, contactDistance, contactBuffer, idtScaling,  meshScaling, true, idtMeshScale, multiManifold);
}

}