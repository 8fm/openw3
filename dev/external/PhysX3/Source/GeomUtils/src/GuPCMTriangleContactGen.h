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

#ifndef GU_PCM_TRIANGLE_CONTACT_GEN_H
#define GU_PCM_TRIANGLE_CONTACT_GEN_H

#include "GuPCMContactGenUtil.h"
#include "GuConvexSupportTable.h"
#include "GuPersistentContactManifold.h"
#include "GuShapeConvex.h"
#include "GuTriangleCache.h"


#if 1
#ifndef __SPU__
extern physx::Gu::PersistentContactManifold* gManifold;
#endif
#define	PCM_LOW_LEVEL_DEBUG	1
#endif

namespace physx
{
	struct PxTriangleMeshGeometryLL;
	class PxHeightFieldGeometry;

namespace Gu
{

	//bool generateTriangleFullContactManifold(Gu::TriangleV& triangle, const PxU8 triFlags, const Gu::PolygonalData& polyData, Gu::SupportLocalImpl<Gu::TriangleV>* triMap, Gu::SupportLocal* polyMap, Gu::PersistentContact* manifoldContacts, PxU32& numContacts,
	//	const Ps::aos::FloatVArg contactDist, Ps::aos::Vec3V& patchNormal);  


	bool PCMContactConvexMesh(const Gu::PolygonalData& polyData0, 
		Gu::SupportLocal* polyMap, 
		const Ps::aos::FloatVArg minMargin, 
		const PxBounds3& hullAABB, 
		const PxTriangleMeshGeometryLL& shapeMesh,
						const PxTransform& transform0, const PxTransform& transform1,
						PxReal contactDistance, Gu::ContactBuffer& contactBuffer,
						const Cm::FastVertex2ShapeScaling& convexScaling, const Cm::FastVertex2ShapeScaling& meshScaling,
						bool idtConvexScale, bool idtMeshScale, Gu::MultiplePersistentContactManifold& multiManifold);

	bool PCMContactConvexHeightfield(const Gu::PolygonalData& polyData0, 
		Gu::SupportLocal* polyMap, 
		const Ps::aos::FloatVArg minMargin, 
		const PxBounds3& hullAABB, 
		const PxHeightFieldGeometry& shapeHeightfield,
						const PxTransform& transform0, const PxTransform& transform1,
						PxReal contactDistance, Gu::ContactBuffer& contactBuffer,
						const Cm::FastVertex2ShapeScaling& convexScaling, bool idtConvexScale, Gu::MultiplePersistentContactManifold& multiManifold);


}
}

#endif