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


#ifndef PX_FOUNDATION_PS_WINDOWS_INTRINSICS_H
#define PX_FOUNDATION_PS_WINDOWS_INTRINSICS_H

#include "Ps.h"
#include "foundation/PxAssert.h"

// this file is for internal intrinsics - that is, intrinsics that are used in
// cross platform code but do not appear in the API

#if !(defined PX_WINDOWS || defined PX_WINMODERN)
	#error "This file should only be included by Windows or WIN8ARM builds!!"
#endif

#include <math.h>
#include <intrin.h>
#include <float.h>
#include <mmintrin.h>

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

namespace physx
{
namespace shdfnd
{

		/*
	 * Implements a memory barrier
	 */
	PX_FORCE_INLINE void memoryBarrier()
	{
		_ReadWriteBarrier();
		/* long Barrier;
		__asm {
			xchg Barrier, eax
		}*/
	}

	/*!
	Returns the index of the highest set bit. Not valid for zero arg.
	*/
	PX_FORCE_INLINE PxU32 highestSetBitUnsafe(PxU32 v)
	{
		unsigned long retval;
		_BitScanReverse(&retval, v);
		return retval;
	}

	/*!
	Returns the index of the highest set bit. Undefined for zero arg.
	*/
	PX_FORCE_INLINE PxU32 lowestSetBitUnsafe(PxU32 v)
	{
		unsigned long retval;
		_BitScanForward(&retval, v);
		return retval;
	}


	/*!
	Returns the number of leading zeros in v. Returns 32 for v=0.
	*/
	PX_FORCE_INLINE PxU32 countLeadingZeros(PxU32 v)
	{
		if(v)
		{
			unsigned long bsr = (unsigned long)-1;
			_BitScanReverse(&bsr, v);
			return 31 - bsr;
		}
		else
			return 32;
	}

	/*!
	Prefetch aligned cache size around \c ptr+offset.
	*/
#ifndef PX_ARM
	PX_FORCE_INLINE void prefetchLine(const void* ptr, PxU32 offset = 0)
	{
		//cache line on X86/X64 is 64-bytes so a 128-byte prefetch would require 2 prefetches.
		//However, we can only dispatch a limited number of prefetch instructions so we opt to prefetch just 1 cache line
		/*_mm_prefetch(((const char*)ptr + offset), _MM_HINT_T0);*/
		//We get slightly better performance prefetching to non-temporal addresses instead of all cache levels
		_mm_prefetch(((const char*)ptr + offset), _MM_HINT_NTA);
	}
#else
	PX_FORCE_INLINE void prefetchLine(const void* ptr, PxU32 offset = 0)
	{
		// arm does have 32b cache line size
		__prefetch (((const char*)ptr + offset));
	}
#endif

	/*!
	Prefetch \c count bytes starting at \c ptr.
	*/
#ifndef PX_ARM
	PX_FORCE_INLINE void prefetch(const void* ptr, PxU32 count = 1)
	{
		const char* cp = (char*)ptr;
		PxU64 p = size_t(ptr);
		PxU64 startLine = p>>6, endLine = (p+count-1)>>6;
		PxU64 lines = endLine - startLine + 1;
		do
		{
			prefetchLine(cp);
			cp+=64;
		} while(--lines);
	}
#else
	PX_FORCE_INLINE void prefetch(const void* ptr, PxU32 count = 1)
	{
		const char* cp = (char*)ptr;
		PxU32 p = size_t(ptr);
		PxU32 startLine = p>>5, endLine = (p+count-1)>>5;
		PxU32 lines = endLine - startLine + 1;
		do
		{
			prefetchLine(cp);
			cp+=32;
		} while(--lines);
	}
#endif

	//! \brief platform-specific reciprocal
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recipFast(float a)				{	return 1.0f/a;			}

	//! \brief platform-specific fast reciprocal square root
	PX_CUDA_CALLABLE PX_FORCE_INLINE float recipSqrtFast(float a)			{   return 1.0f/::sqrtf(a); }

	//! \brief platform-specific floor
	PX_CUDA_CALLABLE PX_FORCE_INLINE float floatFloor(float x)
	{
		return ::floorf(x);
	}

	#define PX_PRINTF printf
	#define PX_EXPECT_TRUE(x) x
	#define PX_EXPECT_FALSE(x) x

} // namespace shdfnd
} // namespace physx

#define PX_EXPECT_TRUE(x) x
#define PX_EXPECT_FALSE(x) x

#endif
