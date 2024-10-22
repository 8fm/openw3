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



#ifndef PXC_CORRELATIONBUFFER_H
#define PXC_CORRELATIONBUFFER_H

#include "PxvConfig.h"
#include "PxSimpleTypes.h"
#include "PxVec3.h"
#include "PxcFrictionPatch.h"
#include "GuContactBuffer.h"

namespace physx
{

struct PxcNpWorkUnit;
struct PxsMaterialInfo;
class PxcNpThreadContext;

struct PxcCorrelationBuffer
{
	static const PxU32 MAX_FRICTION_PATCHES = 32;
	static const PxU16 LIST_END = 0xffff;

	struct ContactPatchData
	{
		PxU16 start, count;
		PxU16 next, flags;
		PxU16 materialIndex0, materialIndex1;
	};

	// we can have as many contact patches as contacts, unfortunately
	ContactPatchData	contactPatches[Gu::ContactBuffer::MAX_CONTACTS];

	// for each friction patch, we store the patch, the address from which the patch came (so we
	// can write it back) and the head of the patch list

	PxcFrictionPatch	PX_ALIGN(16, frictionPatches[MAX_FRICTION_PATCHES]);

	PxU32				frictionPatchContactCounts[MAX_FRICTION_PATCHES];
	PxU32				correlationListHeads[MAX_FRICTION_PATCHES+1];

	// contact IDs are only used to identify auxiliary contact data when velocity
	// targets have been set. 

	PxU16				contactID[MAX_FRICTION_PATCHES][2];

	PxU32 contactPatchCount, frictionPatchCount;

};

struct PxcCorrelationBufferCoulomb
{
	static const PxU32 MAX_FRICTION_PATCHES = 32;
	static const PxU32 LIST_END = 0xffffffff;

	PxcCorrelationBufferCoulomb() : contactPatchCount(0), frictionPatchCount(0)
	{
	}

	struct ContactPatchData
	{
		PxU16 start, count;
		PxU16 next, flags;
		PxU16 materialIndex0, materialIndex1;
	};

	// we can have as many contact patches as contacts, unfortunately
	ContactPatchData	contactPatches[Gu::ContactBuffer::MAX_CONTACTS];

	// for each friction patch, we store the patch, the address from which the patch came (so we
	// can write it back) and the head of the patch list

	PxcFrictionPatchCoulomb	PX_ALIGN(16, frictionPatches[MAX_FRICTION_PATCHES]);		
  
	PxU32				frictionPatchContactCounts[MAX_FRICTION_PATCHES];
	PxU32				correlationListHeads[MAX_FRICTION_PATCHES];

	// contact IDs are only used to identify auxiliary contact data when velocity
	// targets have been set. 
  
	PxU16				contactID[MAX_FRICTION_PATCHES][2];

	PxU32 contactPatchCount, frictionPatchCount;

};

bool createContactPatches(PxcNpThreadContext& threadContext, PxU32 startIndex, PxReal normalTolerance);
bool createContactPatchesCoulomb(PxcCorrelationBufferCoulomb& correlationBuffer, Gu::ContactBuffer& contactBuffer, PxReal normalTolerance, PxU32 startContactIndex);


bool correlatePatches(PxcCorrelationBuffer& frictionBuffer,
					  const Gu::ContactBuffer& contactBuffer,
					  const PxTransform& bodyFrame0,
					  const PxTransform& bodyFrame1,
					  PxReal normalTolerance,
					  PxU32 startContactPatchIndex,
					  PxU32 startFrictionPatchIndex);

//bool correlatePatchesCoulomb1(PxcCorrelationBufferCoulomb& fb, 
//					  const Gu::ContactBuffer& cb,
//					  const PxTransform& bodyFrame0,
//					  const PxTransform& bodyFrame1,
//					  PxReal normalTolerance);

bool correlatePatchesCoulomb(PxcCorrelationBufferCoulomb& fb, 
					  const Gu::ContactBuffer& cb,
					  const PxTransform& bodyFrame0,
					  const PxTransform& bodyFrame1,
					  PxReal normalTolerance,
					  PxU32 numFrictionPerPoint,
					  PxU32 startContactPatchIndex,
					  PxU32 startFrictionPatchIndex);

void growPatches(PxcCorrelationBuffer& frictionBuffer,
				 const Gu::ContactBuffer& buffer,
				 const PxTransform& bodyFrame0,
				 const PxTransform& bodyFrame1,
				 PxReal normalTolerance,
				 PxU32 frictionPatchStartIndex,
				 PxReal frictionOffsetThreshold);

}

#endif
