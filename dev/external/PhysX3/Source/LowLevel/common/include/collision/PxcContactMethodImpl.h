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


#ifndef PXC_CONTACTMETHODIMPL_H
#define PXC_CONTACTMETHODIMPL_H

#include "GuGeometryUnion.h"
#include "CmPhysXCommon.h"

namespace physx
{
namespace Gu
{
	class ContactBuffer;
}

struct PxcNpCache;
class PxcNpThreadContext;
class PxsContext;
class PxcConvexTriangles;
class PxsRigidBody;
struct PxsCCDShape;

namespace Cm
{
	class FastVertex2ShapeScaling;
}

/*!\file
This file contains forward declarations of all implemented contact methods.
*/

#define CONTACT_METHOD_ARGS				\
	const Gu::GeometryUnion& shape0,	\
	const Gu::GeometryUnion& shape1,	\
	const PxTransform& transform0,		\
	const PxTransform& transform1,		\
	const PxReal& contactDistance,		\
	PxcNpCache& npCache,				\
	Gu::ContactBuffer& contactBuffer

#define SWEEP_METHOD_ARGS				\
	const PxsCCDShape& shape0,			\
	const PxsCCDShape& shape1,			\
	const PxsRigidBody* atom0,			\
	const PxsRigidBody* atom1,			\
	const PxTransform& transform0,		\
	const PxTransform& transform1,		\
	const PxTransform& lastTm0,			\
	const PxTransform& lastTm1,			\
	PxReal restDistance,				\
	PxVec3& worldNormal,				\
	PxVec3& worldPoint,					\
	PxcNpCache& cache,					\
	PxcNpThreadContext& context,		\
	const PxF32 toiEstimate				

#define TRIANGLE_SWEEP_METHOD_ARGS			\
	const PxsCCDShape& shape0,				\
	const PxsCCDShape& shape1,				\
	const PxTransform& transform0,			\
	const PxTransform& transform1,			\
	const PxTransform& lastTm0,				\
	const PxTransform& lastTm1,				\
	PxReal restDistance,					\
	PxVec3& worldNormal,					\
	PxVec3& worldPoint,						\
	PxcNpCache& cache,						\
	PxcNpThreadContext& context,			\
	const Cm::FastVertex2ShapeScaling& meshScaling,	\
	Gu::TriangleV& triangle,				\
	const PxF32 toiEstimate

/*! Paramter list without names to avoid unused parameter warnings 
*/
#define CONTACT_METHOD_ARGS_UNUSED			\
	const Gu::GeometryUnion&,				\
	const Gu::GeometryUnion&,				\
	const PxTransform&,						\
	const PxTransform&,						\
	const PxReal&,							\
	PxcNpCache&,							\
	Gu::ContactBuffer&

#define SWEEP_METHOD_ARGS_UNUSED			\
	const PxsCCDShape&,						\
	const PxsCCDShape&,						\
	const PxsRigidBody*,					\
	const PxsRigidBody*,					\
	const PxTransform&,						\
	const PxTransform&,						\
	const PxTransform&,						\
	const PxTransform&,						\
	PxReal,									\
	PxVec3&,								\
	PxVec3&,								\
	PxcNpCache&,							\
	PxcNpThreadContext&,					\
	const PxF32

#define TRIANGLE_SWEEP_METHOD_ARGS_UNUSED	\
	const PxsCCDShape&,						\
	const PxsCCDShape&,						\
	const PxTransform&,						\
	const PxTransform&,						\
	const PxTransform&,						\
	const PxTransform&,						\
	PxReal,									\
	PxVec3&,								\
	PxVec3&,								\
	PxcNpCache&,							\
	PxcNpThreadContext&,					\
	const Cm::FastVertex2ShapeScaling&,		\
	Gu::TriangleV&,							\
	const PxF32


/*!
Method prototype for contact generation routines
*/
typedef bool (*PxcContactMethod) (CONTACT_METHOD_ARGS);

/*!
Method prototype for first impact sweep routines
*/
typedef PxReal (*PxcSweepMethod) (SWEEP_METHOD_ARGS);

// Matrix of types
extern PxcContactMethod g_ContactMethodTable[][7];
extern const bool g_CanUseContactCache[][7];
extern PxcContactMethod g_PCMContactMethodTable[][7];
extern const bool g_CanUsePCMContactCache[][7];

extern const PxcSweepMethod g_SweepMethodTable[][7];

extern bool gUnifiedHeightfieldCollision;

PxReal PxcSweepEstimateAnyShapeMesh(const PxsCCDShape& shape0, const PxsCCDShape& shape1, 
									const PxsRigidBody* atom0, const PxsRigidBody* atom1,
									const PxTransform& transform0, const PxTransform& transform1, PxReal restDistance);

PxReal PxcSweepEstimateAnyShapeHeightfield(const PxsCCDShape& shape0, const PxsCCDShape& shape1, 
									const PxsRigidBody* atom0, const PxsRigidBody* atom1,
									const PxTransform& transform0, const PxTransform& transform1, PxReal restDistance);
}

#endif
