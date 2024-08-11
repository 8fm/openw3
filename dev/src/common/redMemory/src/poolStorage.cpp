/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poolStorage.h"
#include "vault.h"
#include "poolRoot.h"

namespace red
{
namespace memory
{
namespace internal
{
	PoolHandle AcquirePoolHandle()
	{
		return AcquireVault().AcquirePoolNodeHandle();
	}

	void PoolDefaultOOMHandler( PoolHandle handle, u32 size, u32 alignment )
	{
		AcquireVault().HandleOOM( handle, size, alignment );
	}

	void HandleAllocateFailure( PoolStorage & storage, u32 size )
	{
		if( storage.oomHandler )
		{
			storage.oomHandler->HandleAllocateFailure( storage.handle, size, 16 );
		}
		else
		{
			internal::PoolDefaultOOMHandler( storage.handle, size, 16 );
		}
	}
}
}
}
