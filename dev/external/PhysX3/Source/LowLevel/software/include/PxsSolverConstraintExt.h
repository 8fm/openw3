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
   

#ifndef PXS_SOLVERCONSTRAINTEXT_H
#define PXS_SOLVERCONSTRAINTEXT_H

#include "PxsSolverExt.h"

namespace physx
{
#define FINALIZE_EXT_CONTACT_METHOD_ARGS	\
	const PxcSolverConstraintDesc& desc,	\
	PxReal invDt,							\
	PxReal bounceThreshold,					\
	PxsSolverExtBody& b0,					\
	PxsSolverExtBody& b1		

struct PxcNpWorkUnit;
struct PxcCorrelationBuffer;
struct PxcCorrelationBufferCoulomb;

namespace Gu
{
	class ContactBuffer;
}

/*!
Method prototype for finalize the ext contact
*/

typedef	void (*PxcFinalizeExtContactMethod)(FINALIZE_EXT_CONTACT_METHOD_ARGS);

extern PxcFinalizeExtContactMethod finalizeExtContactsMethod[3];

PxReal getImpulseResponse(const PxsSolverExtBody& b0, const Cm::SpatialVector& impulse0, Cm::SpatialVector& deltaV0, PxReal dom0,
						  const PxsSolverExtBody& b1, const Cm::SpatialVector& impulse1, Cm::SpatialVector& deltaV1, PxReal dom1,
						  bool allowSelfCollision = false);

PxReal getImpulseResponse(const PxsSolverExtBody& b0, const Cm::SpatialVector& impulse0, Cm::SpatialVector& deltaV0, PxReal dom0, PxReal angDom0,
						  const PxsSolverExtBody& b1, const Cm::SpatialVector& impulse1, Cm::SpatialVector& deltaV1, PxReal dom1, PxReal angDom1,
						  bool allowSelfCollision = false);

void setupFinalizeExtSolverConstraints(PxcNpWorkUnit& n,
							const Gu::ContactBuffer& buffer,
							const PxcCorrelationBuffer& c,
							const PxTransform& bodyFrame0,
							const PxTransform& bodyFrame1,
							bool perPointFriction,
							PxU8* workspace,
							const PxsSolverExtBody& b0,
							const PxsSolverExtBody& b1,
							const PxcSolverBodyData& data0,
							const PxcSolverBodyData& data1,
							const PxReal invDtF32,
							PxReal bounceThresholdF32,
							PxReal invMassScale0, PxReal invInertiaScale0, 
							PxReal invMassScale1, PxReal invInertiaScale1);


bool setupFinalizeExtSolverConstraintsCoulomb(PxcNpWorkUnit& n,
							const Gu::ContactBuffer& buffer,
							const PxcCorrelationBufferCoulomb& c,
							const PxTransform& bodyFrame0,
							const PxTransform& bodyFrame1,
							bool perPointFriction,
							PxU8* workspace,
							PxReal invDt,
							PxReal bounceThreshold,
							PxsSolverExtBody& b0,
							PxsSolverExtBody& b1,
							PxU32 frictionCountPerPoint,
							PxReal invMassScale0, PxReal invInertiaScale0, 
							PxReal invMassScale1, PxReal invInertiaScale1);



}

#endif
