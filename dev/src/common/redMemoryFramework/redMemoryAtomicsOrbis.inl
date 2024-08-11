/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ATOMICS_WINAPI_H_INCLUDED
#define _RED_MEMORY_ATOMICS_WINAPI_H_INCLUDED
#pragma once

#include <sce_atomic.h>

//////////////////////////////////////////////////////////////////////
// Atomic (memory barrier) operations
// 
namespace Red { namespace MemoryFramework { namespace OrbisAPI {

//////////////////////////////////////////////////////////////////////
// AtomicExchange32
//	Set the target to a new value
RED_INLINE Red::System::Int32 AtomicExchange32( Red::System::Int32 volatile * target, Red::System::Int32 value )
{
	return sceAtomicExchange32( target, value );
}

//////////////////////////////////////////////////////////////////////
// AtomicExchange
//	Set the target to a new value
RED_INLINE Red::System::Int64 AtomicExchange( int64_t volatile * target, int64_t value )
{
	return sceAtomicExchange64( target, value );
}

//////////////////////////////////////////////////////////////////////
// AtomicAdd
//	Add 'toAdd' to 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicAdd( int64_t volatile * target, int64_t toAdd )
{
	return sceAtomicAdd64( target, toAdd );
}

//////////////////////////////////////////////////////////////////////
// AtomicSubtract
//	Remove 'toSubtract' from 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicSubtract( int64_t volatile * target, int64_t toSubtract )
{
	return sceAtomicAdd64( target, -toSubtract );
}

//////////////////////////////////////////////////////////////////////
// AtomicIncrement
//	Increment the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicIncrement( int64_t volatile * target )
{
	return sceAtomicIncrement64( target );
}

//////////////////////////////////////////////////////////////////////
// AtomicDecrement
//	Decrement the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicDecrement( int64_t volatile * target )
{
	return sceAtomicDecrement64( target );
}

} } }

#endif