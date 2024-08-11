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


#ifndef PXC_RIGIDBODY_H
#define PXC_RIGIDBODY_H

#include "PxVec3.h"
#include "PxTransform.h"
#include "PxvDynamics.h"
#include "PxsAABBManagerId.h"
#include "PxsIslandManagerAux.h"
#include "CmSpatialVector.h"
#include "PxsBlockArray.h"

namespace physx
{

struct PxcSolverBody;
class PxsDynamicsContext;
class PxsContactManager;
struct PxsCCDPair;
struct PxsCCDBody;

PX_ALIGN_PREFIX(16)
class PxcRigidBody
{
public:

	PxcRigidBody()
	: mAcceleration(Cm::SpatialVector::zero()),
	  mLastTransform(PxTransform(PxIdentity)),
	  mCCD(NULL),
	  mCore(NULL)
	{
	}

	PX_FORCE_INLINE PxcRigidBody(PxsBodyCore* core)  
	: mAcceleration(Cm::SpatialVector::zero()),
	  mLastTransform(core->body2World),
	  mCCD(NULL),
	  mCore(core)
	{
	}

	void						adjustCCDLastTransform();

protected:
	
	~PxcRigidBody()
	{
	}

public:

	Cm::SpatialVector 			mAcceleration;			//32 (32)	// articulations solve needs this public so we don't have deps on Pxs stuff	//32

	PxTransform					mLastTransform;			//60 (60)

	PxsCCDBody*					mCCD;					//64 (72)	// only valid during CCD	

	PxsBodyCore*				mCore;					//68 (80)

	AABBMgrId					mAABBMgrId;				//76 (88)

}
PX_ALIGN_SUFFIX(16);
PX_COMPILE_TIME_ASSERT(0 == (sizeof(PxcRigidBody) & 0x0f));

//#ifndef PX_X64
//PX_COMPILE_TIME_ASSERT(96==sizeof(PxcRigidBody));
//#endif

class PxcRigidBodySPU: public PxcRigidBody {};

}

#endif //PXC_RIGIDBODY_H
