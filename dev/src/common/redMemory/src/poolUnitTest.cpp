/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poolUnitTest.h"
#include "systemAllocatorType.h"
#include "flags.h"
#include "operators.h"

namespace red
{
namespace memory
{
namespace unitTest
{
	static DynamicFixedSizeAllocator s_fixedSizeAllocator;

	void InitializePools( u32 blockSize )
	{
		const DynamicFixedSizeAllocatorParameter param = 
		{
			&AcquireSystemAllocator(),
			blockSize,
			16,
			10,
			10,
			Flags_CPU_Read_Write
		};

		s_fixedSizeAllocator.Initialize( param );

		PoolStorageProxy< UnitTest_Local_Allocator_Pool>::SetAllocator( s_fixedSizeAllocator );
	}

	void UninitializePools()
	{
		s_fixedSizeAllocator.Uninitialize();
	}

	void * AllocateFromExternalAllocator( u32 blockSize )
	{
		return RED_ALLOCATE( UnitTest_External_Allocator_Pool, blockSize );
	}

	void * AllocateAlignedFromExternalAllocator( u32 blockSize )
	{
		return RED_ALLOCATE_ALIGNED( UnitTest_External_Allocator_Pool, blockSize, 16 );
	}
	
	void FreeFromExternalAllocator( void * block )
	{
		RED_FREE( UnitTest_External_Allocator_Pool, block );
	}

	void * ReallocateFromExternalAllocator( u32 blockSize )
	{
		return RED_REALLOCATE( UnitTest_External_Allocator_Pool, nullptr, blockSize );
	}
}
}
}
