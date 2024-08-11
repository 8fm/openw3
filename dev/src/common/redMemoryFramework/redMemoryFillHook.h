/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_HOOK_FILLER_H
#define _RED_MEMORY_HOOK_FILLER_H
#pragma once

#include "redMemoryHook.h"

namespace Red { namespace MemoryFramework {

	// Filter prototype. Add your own to customise when filler operates
	struct MemoryFillerDefaultFilter
	{
		RED_INLINE static Bool ShouldFillMemory( PoolLabel label, MemoryClass memoryClass ) { RED_UNUSED( label ); RED_UNUSED( memoryClass ); return true; }
	};

	// A hook that can fill with 0 on allocation, and fill with 0xcc for debugging freed memory
	// Pass a 'filter' to disable fill-on-free for specific pools / classes
	// Note that DebugOnFree does not use the filtering
	template< class FilterClass = MemoryFillerDefaultFilter >
	class MemoryFiller : public MemoryHook
	{
	public:
		enum Mode 
		{
			ZeroOnAllocate = ( 1 << 1 ),		// Buffer is filled with 0 after each allocation
			ZeroOnReallocate = ( 1 << 2 ),		// 'New' bit of realloc'd buffer is filled with 0
			DebugOnFree = ( 1 << 3 )			// Buffer is filled with some debug value after each free
		};

		MemoryFiller( Uint32 modeFlags )
		{
			if( modeFlags & ZeroOnAllocate )
			{
				OnPostAllocate = FnZeroOnAllocate;
			}

			if( modeFlags & ZeroOnReallocate )
			{
				OnPostReallocate = FnZeroOnReallocate;
			}

			if( modeFlags & DebugOnFree )
			{
				OnFree = FnDebugFillOnFree;
			}
		}

	private:
		static const Red::System::Uint32 c_FillOnFreeDebug = 0xcd;
		static void FnZeroOnAllocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer );
		static void FnZeroOnReallocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr );
		static void FnDebugFillOnFree( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr );
	};

	template< class FilterClass >
	void MemoryFiller< FilterClass >::FnZeroOnAllocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer )
	{
		if( FilterClass::ShouldFillMemory( label, memoryClass ) )
		{
			Red::System::MemorySet( allocatedBuffer, 0, allocatedSize );
		}
	}

	template< class FilterClass >
	void MemoryFiller< FilterClass >::FnZeroOnReallocate( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr )
	{
		// If the buffer grew, fill in the 'grow' space with 0
		if( freedSize < newSize && FilterClass::ShouldFillMemory( label, memoryClass ) )
		{ 
			Red::System::MemUint target = reinterpret_cast< Red::System::MemUint >( ptr ) + freedSize;
			Red::System::MemorySet( reinterpret_cast< void* >( target ), 0, newSize - freedSize );
		}
	}

	template< class FilterClass >
	void MemoryFiller< FilterClass >::FnDebugFillOnFree( PoolLabel label, MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		Red::System::MemorySet( const_cast< void* >(ptr), c_FillOnFreeDebug, sizeToFree );
	}

} }

#endif