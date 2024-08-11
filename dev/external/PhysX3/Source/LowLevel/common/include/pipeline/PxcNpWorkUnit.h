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


         
#ifndef PXC_NPWORKUNIT_H
#define PXC_NPWORKUNIT_H

#include "PxcFrictionPatch.h"
#include "PxcNpThreadContext.h"
#include "PxcContactMethodImpl.h"
#include "PxcMaterialMethodImpl.h"
#include "PxcNpCache.h"
#include "PxcNpCCDState.h"

namespace physx
{

class PxvContact;

struct PxsRigidCore;
struct PxsShapeCore;

class PxsMaterialManager;

struct PxcNpWorkUnitFlag
{
	enum Enum
	{
		eOUTPUT_CONTACTS			= 1,
		eOUTPUT_CONSTRAINTS			= 2,
		eDISABLE_STRONG_FRICTION	= 4,
		eARTICULATION_BODY0			= 8,
		eARTICULATION_BODY1			= 16,
		eDYNAMIC_BODY0				= 32,
		eDYNAMIC_BODY1				= 64,
		eMODIFIABLE_CONTACT			= 128,
		eFORCE_THRESHOLD			= 256,
	};
};

struct PxcNpWorkUnit
{
	// OUTPUT - if outputSolverConstraints is true, info about the constraints, else NULL/zero

	// PT: changed alignment back to 16 here, else we waste almost 100 bytes in each contact manager. Talk to me if you want to change this again.
//	PX_ALIGN(128,PxU8*		solverConstraintPointer);	
	PX_ALIGN(16,PxU8*		solverConstraintPointer);	// OUTPUT
	PxU32					solverConstraintSize;		// OUTPUT
	//PxU32					axisConstraintCount;		// OUTPUT - can be 16 bits
	PxU16					axisConstraintCount;		// OUTPUT
	bool					disableResponse;			// OUTPUT
	bool					disableCCDResponse;			// OUTPUT

	PxU16					contactCount;				// OUTPUT - can be 16 bits				//14	//18
	PxU16					flags;						// INPUT								//16	//20
	PxReal*					contactForces;				// OUTPUT								//20	//28

	PxU8*					compressedContacts;			// OUTPUT								//24	//36
	PxU32					compressedContactSize;		// OUTPUT								//28	//40


	PxU32					frictionPatchCount;			// INOUT - can be 16 bits (or even 8)	//32	//44
	PxU8*					frictionDataPtr;			// INOUT								//36	//52
	PxcNpCache				pairCache;					// INOUT								//52	//72

	PxU32					index;						// INPUT								//56	//76

	//CCD

	const PxsRigidCore*		rigidCore0;					// INPUT								//60	//84
	const PxsRigidCore*		rigidCore1;					// INPUT								//64	//92
		
	const PxsShapeCore*		shapeCore0;					// INPUT								//68	//100
	const PxsShapeCore*		shapeCore1;					// INPUT								//72	//108

	const PxsMaterialManager*	materialManager;		// INPUT								//76	//116

	//PxReal					dynamicFriction;			// INPUT
	//PxReal					staticFriction;				// INPUT
	//PxReal					restitution;				// INPUT
	PxReal					dominance0;					// INPUT
	PxReal					dominance1;					// INPUT
	PxReal					restDistance;				// INPUT
	//PxReal					correlationDistance;		// INPUT - moved to context it's not per pair information

	PxU8					geomType0;					// INPUT								//93	//133
	PxU8					geomType1;					// INPUT								//94	//134

	bool					touch;						// OUTPUT								//95	//135
	bool					hasSolverConstraints;		// OUTPUT								//96	//136

	PxU32					mTransformCache0;
	PxU32					mTransformCache1;

#ifdef PX_PS3
	// this is here for the moment, until BitMap is on SPU as well
	PxU32					touchLost;
	PxU32					touchFound;
	PxU32					mPad[2];
#endif
};

#ifndef PX_X64
PX_COMPILE_TIME_ASSERT(0 == (sizeof(PxcNpWorkUnit) & 0x0f));
#endif

PX_FORCE_INLINE void PxcNpWorkUnitClearContactState(PxcNpWorkUnit& n)
{
	n.touch	= false;
	//n.contactPoints = NULL;
	n.contactCount = 0;
	n.contactForces = NULL;
	n.solverConstraintPointer = NULL;
	n.axisConstraintCount = 0;
	n.solverConstraintSize = 0;
	n.compressedContacts = NULL;
	n.compressedContactSize = 0;
	n.hasSolverConstraints = false;
}


PX_FORCE_INLINE void PxcNpWorkUnitClearCachedState(PxcNpWorkUnit& n)
{
	n.frictionDataPtr = 0;
	n.frictionPatchCount = 0;
	//n.contactPoints = NULL;
	n.contactCount = 0;
	n.compressedContacts = NULL;
	n.compressedContactSize = 0;
	n.pairCache.invalidate();
	n.hasSolverConstraints = false;
}

PX_FORCE_INLINE bool PxcNpWorkUnitIsBatchable(const PxcNpWorkUnit&)
{ 
#ifdef PX_PS3
	//return (!(n.flags & PxcNpWorkUnitFlag::eDO_CCD_FINALIZATION));
	return true;
#else
	return false;
#endif
}



#ifndef PX_X64
//PX_COMPILE_TIME_ASSERT(sizeof(PxcNpWorkUnit)==128);
#endif

}

#endif
