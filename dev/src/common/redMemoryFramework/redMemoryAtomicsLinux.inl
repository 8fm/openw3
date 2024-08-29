/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ATOMICS_LINUX_H_INCLUDED
#define _RED_MEMORY_ATOMICS_LINUX_H_INCLUDED
#pragma once

#include "../redSystem/os.h"

#define RED_THREADS_ATOMIC_MEMORDER __ATOMIC_RELAXED

//////////////////////////////////////////////////////////////////////
// Atomic (memory barrier) operations
//
namespace Red { namespace MemoryFramework { namespace LinuxAPI {

//////////////////////////////////////////////////////////////////////
// AtomicExchange32
//	Set the target to a new value
RED_INLINE Red::System::Int32 AtomicExchange32( Red::System::Int32 volatile * target, Red::System::Uint32 value )
{
	return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER );
}

//////////////////////////////////////////////////////////////////////
// AtomicExchange
//	Set the target to a new value
RED_INLINE Red::System::Int64 AtomicExchange( Red::System::Int64 volatile * target, Red::System::Int64 value )
{
	return __atomic_exchange_n( target, value, RED_THREADS_ATOMIC_MEMORDER );
}

//////////////////////////////////////////////////////////////////////
// AtomicAdd
//	Add 'toAdd' to 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicAdd( Red::System::Int64 volatile * target, Red::System::Int64 toAdd )
{
	return __atomic_fetch_add( target, toAdd, RED_THREADS_ATOMIC_MEMORDER );
}

//////////////////////////////////////////////////////////////////////
// AtomicSubtract
//	Remove 'toSubtract' from 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicSubtract( Red::System::Int64 volatile * target, Red::System::Int64 toSubtract )
{
	return __atomic_fetch_add( target, -toSubtract, RED_THREADS_ATOMIC_MEMORDER );
}

//////////////////////////////////////////////////////////////////////
// AtomicIncrement
//	Increment the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicIncrement( Red::System::Int64 volatile * target )
{
	return __atomic_add_fetch( target, 1, RED_THREADS_ATOMIC_MEMORDER );
}

//////////////////////////////////////////////////////////////////////
// AtomicDecrement
//	Decrement the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicDecrement( Red::System::Int64 volatile * target )
{
	return __atomic_add_fetch( target, -1, RED_THREADS_ATOMIC_MEMORDER );
}

} } }

#endif
