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


#ifndef PXD_CONFIG_H
#define PXD_CONFIG_H

/*! \file internal top level include file for lowlevel. */

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"

/************************************************************************/
/* Compiler workarounds                                                 */
/************************************************************************/
#ifdef PX_VC
#pragma warning(disable: 4355 )	// "this" used in base member initializer list
#pragma warning(disable: 4146 ) // unary minus operator applied to unsigned type.
#endif

namespace physx
{

// CA: Cell stuff
#ifdef PX_PS3
	#define USE_NEW_ISLAND_GEN
	#define CELL_SPU_ISLAND_GEN_MAX_ATOMS (20000) // Should be always smaller or equal CELL_SPU_ISLAND_GEN_MAX_CMS
	#define CELL_SPU_ISLAND_GEN_MAX_CMS   (40000)
	#define CELL_SPU_MAX_SLABS_PER_POOL   (256) // Check with elements per slab in active bodies and active contact managers
	#define CELL_SPU_UNCONST_ISLAND_FLAG_MASK (0x8000)
	#define CELL_SPU_UNCONST_ISLAND_MAX_SIZE (512)	// Crashes if > 512, 512 seems OK
	#define CELL_SPU_MERGED_ISLANDS_MAX_SIZE (32)
	#define CELL_SPU_MERGE_CANDIDATE_MAX_SIZE (16)
	#define CELL_SPU_MERGE_ISLANDS

	#define	SPU_NEW_CONVEX_VS_MESH
#endif //PX_PS3

}

#endif
