/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hookOverrunDetector.h"
#include "hookTypes.h"
#include "vault.h"

namespace red
{
namespace memory
{
namespace
{
	static HookHandle s_memoryOverrunDetectorHandle = 0;
	const u64 c_slabOverrunFence = 0x0df0adbaefbeadde;	// Will result in deadbeefbaadf00d in memory window :)
}

	void PreAllocateOverrunDetectorCallback( HookPreParameter & param, void*  )
	{
		u32 & size = *param.size;
		Block & block = *param.block;

		if( size )
		{
			// allocate or reallocate with size > 0
			size += sizeof( c_slabOverrunFence );
		}

		if( block.address )
		{
			// Free function called, or Reallocate with size 0.
			block.size -= sizeof( c_slabOverrunFence );
			const u64 fence = *reinterpret_cast< u64* >( block.address + block.size );
			RED_MEMORY_ASSERT( fence == c_slabOverrunFence, "MEMORY OVERRUN DETECTED." );
			RED_UNUSED( fence );
		}
	}

	void PostAllocateOverrunDetectorCallback( HookPostParameter & param, void*  )
	{
		Block & output = *param.outputBlock;

		if( output.address )
		{
			// Simple allocate function, or reallocate with
			output.size -= sizeof( c_slabOverrunFence );
			*reinterpret_cast< u64* >( output.address + output.size ) = c_slabOverrunFence;	
		}
	}

	void EnableOverrunDetector()
	{
		const HookCreationParameter param = 
		{
			PreAllocateOverrunDetectorCallback,
			PostAllocateOverrunDetectorCallback,
			nullptr
		};

		s_memoryOverrunDetectorHandle = AcquireVault().CreateHook( param );
	}
	
	void DisableOverrunDetector()
	{
		AcquireVault().RemoveHook( s_memoryOverrunDetectorHandle );
	}
}
}
