/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "metricsUtils.h"
#include "vault.h"

namespace red
{
namespace memory
{
	void AddAllocateMetric( PoolHandle handle, const Block & block )
	{
		AcquireVault().AddAllocateMetric( handle, block );
	}

	void AddFreeMetric( PoolHandle handle,const Block & block )
	{
		AcquireVault().AddFreeMetric( handle, block );
	}
	
	void AddReallocateMetric( PoolHandle handle,const Block & input, const Block & output )
	{
		AcquireVault().AddReallocateMetric( handle, input, output );
	}

	u64 GetTotalBytesAllocated()
	{
		return AcquireVault().GetTotalBytesAllocated();
	}
}
}
