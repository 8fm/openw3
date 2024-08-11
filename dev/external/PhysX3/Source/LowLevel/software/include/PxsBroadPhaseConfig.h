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


#ifndef PXS_BROADPHASE_CONFIG_H
#define PXS_BROADPHASE_CONFIG_H

#include "CmPhysXCommon.h"
#include "PsVecMath.h"

namespace physx
{

using namespace Ps::aos;

///////////////////////////
//ICE SAP
///////////////////////////

//Maximum number of aabbs (static + dynamic) allowed for spu sap.
//Revert to ppu for greater numbers of aabbs.
#define MAX_NUM_BP_SPU_SAP_AABB 4096 //512 for spu debug
#define MAX_NUM_BP_SPU_SAP_END_POINTS (2*MAX_NUM_BP_SPU_SAP_AABB+2)
//Maximum number of overlaps allowed on spu.
#define MAX_NUM_BP_SPU_SAP_OVERLAPS 8192 //1024 for spu debug

//////////////////////////
//COMPOUND MANAGER
//////////////////////////

#define MAX_COMPOUND_BOUND_SIZE 128	//Max number of bounds in a compound bound.
#define MAX_COMPOUND_BITMAP_SIZE 2048 //(128 * 128 / 8)
#define MAX_COMPOUND_WORD_COUNT 512 //(128*128 / (32))

}

#endif //PXS_BROADPHASE_CONFIG_H
