/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hookUtils.h"
#include "vault.h"
#include "hookHandler.h"
#include "hookMarkBlock.h"
#include "hookSettings.h"
#include "hookOverrunDetector.h"

namespace red
{
namespace memory
{
namespace internal
{
	void ProcessPreHooks( HookPreParameter & param )
	{
		AcquireVault().ProcessPreHooks( param );
	}

	void ProcessPostHooks( HookPostParameter & param )
	{
		AcquireVault().ProcessPostHooks( param );
	}
}

	void InitializePermanentHooks( HookHandler & handler )
	{
		if( c_enableAllocateMemoryMarking || c_enableFreeMemoryMarking )
		{
			HookCreationParameter param = 
			{
				c_enableFreeMemoryMarking ? MarkFreeBlock : nullptr,
				c_enableAllocateMemoryMarking ? MarkAllocatedBlock : nullptr,
				nullptr
			};

			handler.Create( param );
		}

		if( c_enableOverrunDetection )
		{
			const HookCreationParameter param = 
			{
				PreAllocateOverrunDetectorCallback,
				PostAllocateOverrunDetectorCallback,
				nullptr
			};

			handler.Create( param );
		}
	}
}
}
