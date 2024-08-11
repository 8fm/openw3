/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../include/poolUtils.h"
#include "vault.h"

namespace red
{
namespace memory
{
	void RegisterPool( PoolHandle handle, const PoolParameter & param )
	{
		return AcquireVault().RegisterPool( handle, param );
	}

	u64 GetPoolBudget( PoolHandle handle )
	{
		return AcquireVault().GetPoolBudget( handle );
	}

	const char * GetPoolName( PoolHandle handle )
	{
		return AcquireVault().GetPoolName( handle );
	}

	bool IsPoolRegistered( PoolHandle handle )
	{
		return AcquireVault().IsPoolRegistered( handle );
	}

	u32 GetPoolCount()
	{
		return AcquireVault().GetPoolCount();
	}

	void InitializeRootPools()
	{
		const SystemAllocator & systemAllocator = AcquireSystemAllocator();
		u64 systemBudget = systemAllocator.GetTotalPhysicalMemoryAvailable();
		DefaultAllocator & defaultAllocator = AcquireDefaultAllocator();
		NullAllocator & nullAllocator = AcquireNullAllocator();

#ifdef RED_PLATFORM_ORBIS
		const SystemAllocator & flexibleAllocator = AcquireFlexibleSystemAllocator();
		const u64 flexibleBudget = flexibleAllocator.GetTotalPhysicalMemoryAvailable();
		systemBudget += flexibleBudget;
		RED_INITIALIZE_MEMORY_POOL( PoolFlexible, PoolRoot, nullAllocator, flexibleBudget );
#endif

		const PoolParameter rootParam = 
		{
			"PoolRoot",
			&StaticPoolStorage< PoolRoot >::storage,
			systemBudget,
			c_poolNodeInvalid
		};

		InitializePool< PoolRoot >( rootParam, nullAllocator );  
		RED_INITIALIZE_MEMORY_POOL( PoolCPU, PoolRoot, nullAllocator, 0 );  
			RED_INITIALIZE_MEMORY_POOL( PoolDefault, PoolCPU, defaultAllocator, 0 );  
			RED_INITIALIZE_MEMORY_POOL( PoolLegacy, PoolCPU, defaultAllocator, 0 ); 
		RED_INITIALIZE_MEMORY_POOL( PoolGPU, PoolRoot, nullAllocator, 0 );
	}
}
}
