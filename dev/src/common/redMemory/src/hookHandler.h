/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_HANDLER_H_
#define _RED_MEMORY_HOOK_HANDLER_H_

#include "hookTypes.h"
#include "hookPool.h"

namespace red
{
namespace memory
{
	class Hook;
	struct Block;

	class RED_MEMORY_API HookHandler
	{
	public:

		HookHandler();
		~HookHandler();

		void Initialize();

		HookHandle Create( const HookCreationParameter & param );
		void Remove( HookHandle handle );

		void ProcessPreHooks( HookPreParameter & param );
		void ProcessPostHooks( HookPostParameter & param );

	private:

		void Register( Hook * hook );
		void Unregister( Hook * hook ); 

		typedef CRWSpinLock LockPrimitive;
		typedef CScopedLock< LockPrimitive > ScopedWriteLock;
		typedef CScopedSharedLock< LockPrimitive > ScopedReadLock;

		Hook * m_rootHook;
		LockPrimitive m_lock;
		HookPool m_pool;
	};
}
}

#include "hookHandler.hpp"

#endif
