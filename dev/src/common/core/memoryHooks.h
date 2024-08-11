#pragma once

#include "../redMemoryFramework/redMemoryHook.h"
#include "../redSystem/timer.h"

// This hook will fill memory with 0x3c on allocate
class UninitialiseDataCatcher : public Red::MemoryFramework::MemoryHook
{
public:
	UninitialiseDataCatcher()
	{
		OnPostAllocate = FnOnAllocate;
		OnPostReallocate = FnOnReallocate;
	}

private:
	static const Uint32 c_fillOnAllocValue = 0x3c3c3c3c;

	static void FnOnAllocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		Red::System::MemorySet( allocatedBuffer, c_fillOnAllocValue, allocatedSize );
	}

	static void FnOnReallocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		if( freedSize < newSize )
		{
			Red::System::MemUint target = reinterpret_cast< Red::System::MemUint >( ptr ) + freedSize;
			Red::System::MemorySet( reinterpret_cast< void* >( target ), c_fillOnAllocValue, newSize - freedSize );
		}
	}
};

// This hook can add a uint32 at the end of each allocation, and check for overruns on delete
class MemoryStompHook : public Red::MemoryFramework::MemoryHook
{
public:
	MemoryStompHook()
	{
		OnPreAllocate = FnPreAllocate;
		OnPostAllocate = FnPostAllocate;
		OnPreReallocate = FnPreReallocate;
		OnPostReallocate = FnPostReallocate;
		OnFree = FnFree;
	}
private:
	static const Uint32 c_overrunValue = 0xDEFECA7E;
	static const Uint32 c_overrunIterations = 2;

	static void TestForStomps( const void* ptr, Red::System::MemSize size )
	{
		// Avoid false positives - probably not our pointer, the memory system checks the allocator then the static pool
		if ( size < 1 )
		{
			return;
		}

		MemInt endAddress = reinterpret_cast< MemInt >( ptr ) + size - ( sizeof( c_overrunValue ) * c_overrunIterations );
		Uint32* overrunBuffer = reinterpret_cast< Uint32* >( endAddress );
		RED_UNUSED( overrunBuffer );
		for( Uint32 i = 0; i < c_overrunIterations; ++i )
		{
			RED_FATAL_ASSERT( *overrunBuffer == c_overrunValue, "STOP! HAMMER TIME! A buffer overrun has occured" );
			++overrunBuffer;
		}		
	}

	static void AddSentinel( void* ptr, Red::System::MemSize totalSize )
	{
		MemInt endAddress = reinterpret_cast< MemInt >( ptr ) + totalSize - ( sizeof( c_overrunValue ) * c_overrunIterations );
		Uint32* overrunBuffer = reinterpret_cast< Uint32* >( endAddress );
		for( Uint32 i = 0; i < c_overrunIterations; ++i )
		{
			*overrunBuffer = c_overrunValue;
			++overrunBuffer;
		}
	}

	static Red::System::MemSize FnPreAllocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, size_t allocSize, size_t allocAlignment )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		RED_UNUSED( allocAlignment );

		// Increase size enough to fit sentinels + alignment
		size_t alignedUp = (allocSize + (allocAlignment - 1)) & ~( allocAlignment - 1 );
		return alignedUp + ( sizeof( c_overrunValue ) * c_overrunIterations );
	}

	static void FnPostAllocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize allocatedSize, void* allocatedBuffer )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );

		// Always assume we allocate space at the start and end
		if( allocatedBuffer )
		{
			AddSentinel( allocatedBuffer, allocatedSize );
		}
	}

	static Red::System::MemSize FnPreReallocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize size, Red::System::MemSize allocAlignment, void* ptr )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		RED_UNUSED( allocAlignment );
		RED_UNUSED( ptr );

		// Increase size enough to fit sentinels if we are allocating
		if( size > 0 )
		{
			size_t alignedUp = (size + (allocAlignment - 1)) & ~( allocAlignment - 1 );
			return alignedUp + ( sizeof( c_overrunValue ) * c_overrunIterations );
		}
		else
		{
			// We could check for a overrun on the original buffer here, but we don't have the old size.
			return 0;
		}
	}

	static void FnPostReallocate( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize freedSize, Red::System::MemSize newSize, void* ptr )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );

		// If it got freed, we can't really do anything at this point
		if( ptr && ( newSize > 0 ) )
		{
			if( freedSize > 0 && newSize >= freedSize && ptr )
			{
				// If the new buffer is big enough to hold the old one, we inherited its sentinels and can test them
				TestForStomps( ptr, freedSize );
			}

			// Now, write new sentinels
			AddSentinel( ptr, newSize );
		}		
	}

	static void FnFree( Red::MemoryFramework::PoolLabel label, Red::MemoryFramework::MemoryClass memoryClass, Red::System::MemSize sizeToFree, const void* ptr )
	{
		RED_UNUSED( label );
		RED_UNUSED( memoryClass );
		TestForStomps( ptr, sizeToFree );
	}
};
