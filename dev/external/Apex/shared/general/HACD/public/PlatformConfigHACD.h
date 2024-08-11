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

#ifndef PLATFORM_CONFIG_H

#define PLATFORM_CONFIG_H

// Modify this header file to make the HACD data types be compatible with your
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <new>

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxMath.h"
#include "PsAllocator.h"
#include "PsUserAllocated.h"
#include "PsString.h"

// This header file provides a brief compatibility layer between the PhysX and APEX SDK foundation header files.
// Modify this header file to your own data types and memory allocation routines and do a global find/replace if necessary

namespace hacd
{

	typedef physx::PxI64	HaI64;
	typedef physx::PxI32	HaI32;
	typedef physx::PxI16	HaI16;
	typedef physx::PxI8	HaI8;

	typedef physx::PxU64	HaU64;
	typedef physx::PxU32	HaU32;
	typedef physx::PxU16	HaU16;
	typedef physx::PxU8	HaU8;

	typedef physx::PxF32	HaF32;
	typedef physx::PxF64	HaF64;



	class PxEmpty;

#define HACD_SIGN_BITMASK		0x80000000

	// avoid unreferenced parameter warning (why not just disable it?)
	// PT: or why not just omit the parameter's name from the declaration????
#define HACD_FORCE_PARAMETER_REFERENCE(_P) (void)(_P);
#define HACD_UNUSED(_P) HACD_FORCE_PARAMETER_REFERENCE(_P)

#define HACD_ALLOC_ALIGNED(x,y) PX_ALLOC(x,#x)
#define HACD_ALLOC(x) PX_ALLOC(x,#x) 
#define HACD_FREE PX_FREE

#define HACD_ASSERT PX_ASSERT
#define HACD_ALWAYS_ASSERT PX_ALWAYS_ASSERT

#define HACD_INLINE PX_INLINE
#define HACD_NOINLINE PX_NOINLINE
#define HACD_FORCE_INLINE PX_FORCE_INLINE
#define HACD_PLACEMENT_NEW PX_PLACEMENT_NEW

#define HACD_SPRINTF_S physx::string::sprintf_s

#define HACD_ISFINITE(x) physx::PxIsFinite(x)

#define HACD_NEW PX_NEW

#ifdef PX_WINDOWS
#define HACD_WINDOWS 
#endif

#ifdef PX_VC
#define HACD_VC
#endif

#ifdef PX_VC9
#define HACD_VC9
#endif

#ifdef PX_VC8
#define HACD_VC8
#endif

#ifdef PX_VC7
#define HACD_VC7
#endif

#ifdef PX_VC6
#define HACD_VC6
#endif

#ifdef PX_GNUC
#define HACD_GNUC
#endif

#ifdef PX_CW
#define HACD_CW 
#endif

#ifdef PX_X86
#define HACD_X86
#endif

#ifdef PX_X64
#define HACD_X64
#endif

#ifdef PX_PPC
#define HACD_PPC
#endif

#ifdef PX_PPC64
#define HACD_PPC64
#endif

#ifdef PX_VMX
#define HACD_VMX
#endif

#ifdef  PX_ARM
#define HACD_ARM
#endif

#ifdef PX_WINDOWS
#define HACD_WINDOWS
#endif

#ifdef PX_X360
#define HACD_X360
#endif

#ifdef PX_PS3
#define HACD_PS3
#endif

#ifdef PX_LINUX
#define HACD_LINUX
#endif

#ifdef PX_ANDROID
#define HACD_ANDROID
#endif

#ifdef PX_APPLE
#define HACD_APPLE
#endif

#ifdef PX_CYGWIN
#define HACD_CYGWIN
#endif

#ifdef PX_WII
#define HACD_WII
#endif

#ifdef PX_GC
#define HACD_GC
#endif


#define HACD_C_EXPORT PX_C_EXPORT
#define HACD_CALL_CONV PX_CALL_CONV
#define HACD_PUSH_PACK_DEFAULT PX_PUSH_PACK_DEFAULT
#define HACD_POP_PACK PX_POP_PACK
#define HACD_INLINE PX_INLINE
#define HACD_FORCE_INLINE PX_FORCE_INLINE
#define HACD_NOINLINE PX_NOINLINE
#define HACD_RESTRICT PX_RESTRICT
#define HACD_NOALIAS PX_NOALIAS
#define HACD_ALIGN PX_ALIGN
#define HACD_ALIGN_PREFIX PX_ALIGN_PREFIX
#define HACD_ALIGN_SUFFIX PX_ALIGN_SUFFIX
#define HACD_DEPRECATED PX_DEPRECATED
#define HACD_JOIN_HELPER PX_JOIN_HELPER
#define HACD_JOIN PX_JOIN
#define HACD_COMPILE_TIME_ASSERT PX_COMPILE_TIME_ASSERT
#define HACD_OFFSET_OF PX_OFFSET_OF
#define HACD_CHECKED PX_CHECKED
#define HACD_CUDA_CALLABLE PX_CUDA_CALLABLE


class ICallback
{
public:
	virtual void ReportProgress(const char *, HaF32 progress) = 0;
	virtual bool Cancelled() = 0;
};

#ifdef HACD_X64
typedef HaU64 HaSizeT;
#else
typedef HaU32 HaSizeT;
#endif


}; // end HACD namespace

#define UANS physx::shdfnd	// the user allocator namespace

#define HACD_SQRT(x) hacd::HaF32 (physx::PxSqrt(x))	
#define HACD_RECIP_SQRT(x) hacd::HaF32 (physx::PxRecipSqrt(x))	

#include "PxVector.h"



#endif
