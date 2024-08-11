/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ATOMICS_DURANGO_H_INCLUDED
#define _RED_MEMORY_ATOMICS_DURANGO_H_INCLUDED
#pragma once

//////////////////////////////////////////////////////////////////////
// Atomic (memory barrier) operations
// 
namespace Red { namespace MemoryFramework { namespace DurangoAPI {

//////////////////////////////////////////////////////////////////////
// AtomicExchange32
//	Set the target to a new value
RED_INLINE Red::System::Uint32 AtomicExchange32( Red::System::Uint32 volatile * target, Red::System::Uint32 value )
{
	return _InterlockedExchange( target, value );
}

//////////////////////////////////////////////////////////////////////
// AtomicExchange
//	Set the target to a new value
RED_INLINE Red::System::Int64 AtomicExchange( Red::System::Int64 volatile * target, Red::System::Int64 value )
{
	return InterlockedExchange64( target, value );
}

//////////////////////////////////////////////////////////////////////
// AtomicAdd
//	Add 'toAdd' to 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicAdd( Red::System::Int64 volatile * target, Red::System::Int64 toAdd )
{
	return InterlockedExchangeAdd64( target, toAdd );
}

//////////////////////////////////////////////////////////////////////
// AtomicSubtract
//	Remove 'toSubtract' from 'target' and return the new value
RED_INLINE Red::System::Int64 AtomicSubtract( Red::System::Int64 volatile * target, Red::System::Int64 toSubtract )
{
	return InterlockedExchangeAdd64( target, -toSubtract );
}

//////////////////////////////////////////////////////////////////////
// AtomicIncrement
//	Increment the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicIncrement( Red::System::Int64 volatile * target )
{
	return InterlockedIncrement64( target );
}

//////////////////////////////////////////////////////////////////////
// AtomicDecrement
//	Decrement the target value by one and return the new value
RED_INLINE Red::System::Int64	AtomicDecrement( Red::System::Int64 volatile * target )
{
	return InterlockedDecrement64( target );
}

} } }

#endif