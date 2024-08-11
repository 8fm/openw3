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


#ifndef PXS_BROADPHASE_MBP_COMMON_H
#define PXS_BROADPHASE_MBP_COMMON_H

#define MBP_USE_WORDS
#define MBP_USE_SENTINELS		// Probably doesn't work with 16bits boxes
#define MBP_USE_NO_CMP_OVERLAP
//#define MBP_USE_NO_CMP_OVERLAP_3D	// Seems slower
#if defined (WIN32) || defined (WIN64)
	#define MBP_SIMD_OVERLAP
#endif

#ifdef MBP_USE_WORDS
	typedef	physx::PxU16	MBP_Index;
#else
	typedef	physx::PxU32	MBP_Index;
#endif

	PX_FORCE_INLINE physx::PxU32 EncodeHandle(physx::PxU32 object_index, physx::PxU32 flip_flop, bool is_static)
	{
	/*	object_index += object_index;
		object_index |= flip_flop;
		return object_index;*/
		return (object_index<<2)|(flip_flop<<1)|physx::PxU32(is_static);
	}

	PX_FORCE_INLINE physx::PxU32 DecodeHandle_Index(physx::PxU32 handle)
	{
	//	return handle>>1;
		return handle>>2;
	}

	PX_FORCE_INLINE physx::PxU32 DecodeHandle_IsStatic(physx::PxU32 handle)
	{
		return handle&1;
	}

	struct MBPEntry_Data
	{
		#ifdef PX_DEBUG
			bool		mUpdated;
		#endif

		// ### mIndex could be PxU16 but beware, we store mFirstFree there
		physx::PxU32		mIndex;			// Out-to-in, maps user handle to internal array. mIndex indexes either the static or dynamic array.
		physx::PxU32		mMBPHandle;		// MBP-level handle (the one returned to users)

		PX_FORCE_INLINE		physx::PxU32	isStatic()	const
		{
			return DecodeHandle_IsStatic(mMBPHandle);
		}
	};

	struct IAABB_Data
	{
		physx::PxU32 mMinX;
//	physx::PxU32 mMaxX;
		physx::PxU32 mMinY;
		physx::PxU32 mMinZ;
		physx::PxU32 mMaxX;
		physx::PxU32 mMaxY;
		physx::PxU32 mMaxZ;
	};

	struct SIMD_AABB_Data
	{
		physx::PxU32 mMinX;
		physx::PxU32 mMaxX;
		physx::PxU32 mMinY;
		physx::PxU32 mMinZ;
		physx::PxU32 mMaxY;
		physx::PxU32 mMaxZ;
	};


	// SPU:
	// Overhead for overlap results for 6 SPUS is currently MBP_MAX_NB_OVERLAPS*sizeof(MBP_Overlap)*6
	// With MBP_MAX_NB_OVERLAPS = 8192, that's 8192*4*6 = 196608 bytes.
	// This one can go to the scratchpad.

//	#define MBP_MAX_NB_OVERLAPS	4096
	#define MBP_MAX_NB_OVERLAPS	8192
	struct MBP_Overlap
	{
		physx::PxU16	mIndex0;
		physx::PxU16	mIndex1;
	};

	// SPU:
	// Overhead from batch DMAs is currently MBP_BOX_CACHE_SIZE*sizeof(MBP_AABB)*3
	// With MBP_BOX_CACHE_SIZE = 16*32, that's 16*32*24*3 = 36864 bytes.
	// This one can not go to the scratchpad.

//	#define MBP_BOX_CACHE_SIZE	(16)
//	#define MBP_BOX_CACHE_SIZE	(16*2)
//	#define MBP_BOX_CACHE_SIZE	(16*4)
//	#define MBP_BOX_CACHE_SIZE	(16*8)
//	#define MBP_BOX_CACHE_SIZE	(16*16)
	#define MBP_BOX_CACHE_SIZE	(16*32)
//	#define MBP_BOX_CACHE_SIZE	(16*64)

#endif //PXS_BROADPHASE_MBP_COMMON_H
