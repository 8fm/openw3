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
#include "Ps.h"
#include "PsAtomic.h"

#define PAUSE() asm ("nop")

namespace physx
{
namespace shdfnd
{

void *atomicCompareExchangePointer(volatile void **dest,void *exch,void *comp)
{
	return __sync_val_compare_and_swap((void**)dest, comp, exch);
}

PxI32 atomicCompareExchange(volatile PxI32 *dest,PxI32 exch,PxI32 comp)
{
	return __sync_val_compare_and_swap(dest, comp, exch);
}

PxI32 atomicIncrement(volatile PxI32 *val)
{
	return __sync_add_and_fetch(val, 1);
}

PxI32 atomicDecrement(volatile PxI32 *val)
{
	return __sync_sub_and_fetch(val, 1);
}

PxI32 atomicAdd(volatile PxI32 *val, PxI32 delta)
{
	return __sync_add_and_fetch(val, delta);
}

PxI32 atomicMax(volatile PxI32 *val,PxI32 val2)
{
	PxI32 oldVal, newVal;

	do
	{
		PAUSE();
		oldVal = *val;

		if (val2 > oldVal)
			newVal = val2;
		else
	    		newVal = oldVal;

	}
	while (atomicCompareExchange(val, newVal, oldVal) != oldVal);
	
	return *val;
}

PxI32 atomicExchange(volatile PxI32 *val,PxI32 val2)
{
	PxI32 newVal, oldVal;

	do
	{
		PAUSE();
		oldVal = *val;
		newVal = val2;
	}
	while (atomicCompareExchange(val, newVal, oldVal) != oldVal);
	
	return oldVal;
}

} // namespace shdfnd
} // namespace physx

