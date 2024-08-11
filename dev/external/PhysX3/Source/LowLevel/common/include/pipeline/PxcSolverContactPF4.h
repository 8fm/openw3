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

#ifndef PXC_SOLVER_CONTACT_PF_4_H
#define PXC_SOLVER_CONTACT_PF_4_H


#include "PxvConfig.h"
#include "PxSimpleTypes.h"
#include "PxVec3.h"

#include "PsVecMath.h"

namespace physx
{

using namespace Ps::aos;

struct PxcSolverContactCoulombHeader4
{
	PxU8	type;					//Note: mType should be first as the solver expects a type in the first byte.
	PxU8	numNormalConstr;
	PxU16	frictionOffset;
	PxU8	numNormalConstr0, numNormalConstr1, numNormalConstr2, numNormalConstr3;
	PxU8	flags[4];
	PxU32	pad;
	Vec4V	restitution;
	Vec4V	normalX;
	Vec4V	normalY;
	Vec4V	normalZ;
	Vec4V	invMassADom;
	Vec4V	invMassBDom;
}; 
PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverContactCoulombHeader4) == 112);

struct PxcSolverContact4Base
{
	Vec4V raXnX;
	Vec4V raXnY;
	Vec4V raXnZ;
	Vec4V delAngVel0X;
	Vec4V delAngVel0Y;
	Vec4V delAngVel0Z;
	Vec4V appliedForce;
	Vec4V velMultiplier;
	Vec4V targetVelocity;
	Vec4V scaledBias;
	Vec4V maxImpulse;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverContact4Base) == 176);

struct PxcSolverContact4Dynamic : public PxcSolverContact4Base
{
	Vec4V rbXnX;
	Vec4V rbXnY;
	Vec4V rbXnZ;
	Vec4V delAngVel1X;
	Vec4V delAngVel1Y;
	Vec4V delAngVel1Z;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverContact4Dynamic) == 272);

struct PxcSolverFrictionHeader4
{
	PxU8	type;					//Note: mType should be first as the solver expects a type in the first byte.
	PxU8	numNormalConstr;
	PxU8	numFrictionConstr;
	PxU8	numNormalConstr0;
	PxU8	numNormalConstr1;
	PxU8	numNormalConstr2;
	PxU8	numNormalConstr3;
	PxU8	numFrictionConstr0;
	PxU8	numFrictionConstr1;
	PxU8	numFrictionConstr2;
	PxU8	numFrictionConstr3;
	PxU8	pad0;
	PxU32	frictionPerContact;

	Vec4V	staticFriction;
	Vec4V   invMassADom;
	Vec4V   invMassBDom;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverFrictionHeader4) == 64);

struct PxcSolverFriction4Base
{
	Vec4V normalX;
	Vec4V normalY;
	Vec4V normalZ;
	Vec4V raXnX;
	Vec4V raXnY;
	Vec4V raXnZ;
	Vec4V delAngVel0X;
	Vec4V delAngVel0Y;
	Vec4V delAngVel0Z;
	Vec4V appliedForce;
	Vec4V velMultiplier;
	Vec4V targetVelocity;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverFriction4Base) == 192);

struct PxcSolverFriction4Dynamic : public PxcSolverFriction4Base
{
	Vec4V rbXnX;
	Vec4V rbXnY;
	Vec4V rbXnZ;
	Vec4V delAngVel1X;
	Vec4V delAngVel1Y;
	Vec4V delAngVel1Z;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcSolverFriction4Dynamic) == 288);

}



#endif

