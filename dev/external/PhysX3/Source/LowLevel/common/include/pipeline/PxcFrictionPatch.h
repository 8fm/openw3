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



#ifndef PXC_FRICTIONPATCH_H
#define PXC_FRICTIONPATCH_H

#include "PxvConfig.h"
#include "PxSimpleTypes.h"
#include "PxVec3.h"

namespace physx
{

struct PxcFrictionPatch
{

	PxU8  PX_ALIGN(16, broken);				//Want broken to be 16-byte aligned for Cm::memFetch.
	PxU8				pad[3];				//Padding with 15 extra bytes of data for Cm::memFetch 
	PxU32				finalPad[3];
	PxU16				anchorCount;
	PxU16				materialIndex1;
	PxU16				materialIndex0;
	PxU16				materialFlags;		//Stores whether we've disabled strong friction on this patch
	PxVec3				body0Normal;
	PxVec3				body1Normal;
	PxVec3				body0Anchors[2];
	PxVec3				body1Anchors[2];

	PX_FORCE_INLINE	void	operator = (const PxcFrictionPatch& other)
	{
		broken = other.broken;
		anchorCount = other.anchorCount;
		body0Normal = other.body0Normal;
		body1Normal = other.body1Normal;
		body0Anchors[0] = other.body0Anchors[0];   
		body0Anchors[1] = other.body0Anchors[1];
		body1Anchors[0] = other.body1Anchors[0];
		body1Anchors[1] = other.body1Anchors[1];
		materialIndex0 = other.materialIndex0;
		materialIndex1 = other.materialIndex1;
		materialFlags = other.materialFlags;
	}
};  

PX_COMPILE_TIME_ASSERT(sizeof(PxcFrictionPatch)==96);

struct PxcFrictionPatchCoulomb
{
	PxVec3	normal;		
	PxU16	materialIndex0;
	PxU16	materialIndex1;
	PxU16	numConstraints;
	PxU16	materialFlags;
};

PX_COMPILE_TIME_ASSERT(sizeof(PxcFrictionPatchCoulomb)==20);

}

#endif
