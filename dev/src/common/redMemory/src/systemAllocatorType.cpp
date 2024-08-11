/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "systemAllocatorType.h"
#include "vault.h"
#include "../include/defaultAllocator.h"

namespace red
{
namespace memory
{
	SystemAllocator & AcquireSystemAllocator()
	{
		return AcquireVault().GetSystemAllocator();
	}

	SystemAllocator & AcquireFlexibleSystemAllocator()
	{
		return AcquireVault().GetFlexibleAllocator();
	}
}
}

