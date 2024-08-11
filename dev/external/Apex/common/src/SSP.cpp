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

#include "foundation/PxPreprocessor.h"

#if defined PX_PS3 && !defined NDEBUG

#include "foundation/PxAssert.h"

extern "C"
{

// Sony does not include these in libgcc due to problems with license
// so we have to implement them manually

void* __stack_chk_guard = 0;

void __stack_chk_guard_setup()
{
	unsigned char* guard = (unsigned char*)&__stack_chk_guard;
	guard[sizeof(__stack_chk_guard) - 1] = 0xff;
	guard[sizeof(__stack_chk_guard) - 2] = '\n';
	guard[0] = 0;
}

void __attribute__((noreturn)) __stack_chk_fail() {
	PX_ASSERT(0 && "__stack_chk_fail");
	while(1);
}

}

#endif
