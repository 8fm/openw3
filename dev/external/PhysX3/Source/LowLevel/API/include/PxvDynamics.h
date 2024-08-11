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


#ifndef PXV_DYNAMICS_H
#define PXV_DYNAMICS_H

#include "PxSimpleTypes.h"
#include "PsIntrinsics.h"
#include "PxVec3.h"
#include "PxQuat.h"
#include "PxTransform.h"
#include "PxRigidDynamic.h"

namespace physx
{

/*!
\file
Dynamics interface.
*/

/************************************************************************/
/* Atoms                                                                */
/************************************************************************/

class PxsContext;
class PxsRigidBody;
class PxShape;
class PxGeometry;
struct PxsShapeCore;


struct PxsRigidCore
{
	PxsRigidCore()									{}
	PxsRigidCore(const PxEMPTY&) : mFlags(PxEmpty)	{}

	PX_ALIGN_PREFIX(16)
	PxTransform			body2World PX_ALIGN_SUFFIX(16);
	PxRigidBodyFlags	mFlags;					// API body flags
	//PxU16				padFromFlags;
	PxU16				solverIterationCounts;	//vel iters are in low word and pos iters in high word.

	PxU32 isKinematic() const
	{
		return mFlags & PxRigidBodyFlag::eKINEMATIC;
	}

	PxU32 hasCCD() const
	{
		return mFlags & PxRigidBodyFlag::eENABLE_CCD;
	}

	PxU32 hasCCDFriction() const
	{
		return mFlags & PxRigidBodyFlag::eENABLE_CCD_FRICTION;
	}
};

struct PxsBodyCore: public PxsRigidCore
{
	PxsBodyCore() : PxsRigidCore()							{}
	PxsBodyCore(const PxEMPTY&) : PxsRigidCore(PxEmpty)		{}

	PxTransform			body2Actor;
	PxReal				ccdAdvanceCoefficient;	//32

	PxVec3				linearVelocity;
	PxReal				mWakeCounter;			//48 PT: moved here to limit L2s in BodySim ctor

	PxVec3				angularVelocity;
	PxReal				contactReportThreshold;	//64
    
	PxReal				maxAngularVelocitySq;
	PxReal				maxLinearVelocitySq;	//72

	PxReal				linearDamping;
	PxReal				angularDamping;			//80

	PxVec3				inverseInertia;
	PxReal				inverseMass;			//96
};

PX_COMPILE_TIME_ASSERT(sizeof(PxsBodyCore) == 128);



struct PxvRigidBodyPair
{
	PxsRigidBody* atom0;
	PxsRigidBody* atom1;
};

}

#endif
