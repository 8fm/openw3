/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_VAULT_H_
#define _RED_MEMORY_VAULT_H_

#include "defaultAllocator.h"
#include "poolRegistry.h"
#include "metricsRegistry.h"
#include "threadMonitor.h"
#include "oomHandlerBreak.h"
#include "reporter.h"
#include "hookHandler.h"
#include "systemAllocatorType.h"
#include "pageAllocatorType.h"

namespace red
{
namespace memory
{
	struct HookPostParameter;

	// This is the static storage of memory system. 
	// To keep memory localize as much as possible, all needed class are stored here, and not randomly everywhere in static memory
	// It also glued everything together. This class is never referenced by any allocator or system.
	// It is never directly access by anything, and only access indirectly by client. 
	// For example when using operator like RED_NEW( MyClass ).
	class Vault 
	{
	public:

		Vault();
		~Vault();

		void Initialize();

		DefaultAllocator & GetDefaultAllocator();
		SystemAllocator & GetSystemAllocator();
		SystemAllocator & GetFlexibleAllocator();

		PoolHandle AcquirePoolNodeHandle();
		void RegisterPool( PoolHandle handle, const PoolParameter & param );
		u64 GetPoolBudget( PoolHandle handle ) const;
		u64 GetPoolTotalBytesAllocated( PoolHandle handle ) const;
		const char * GetPoolName( PoolHandle handle ) const;
		bool IsPoolRegistered( PoolHandle handle ) const;
		u32 GetPoolCount() const;
		
		u64 GetTotalBytesAllocated() const;

		void AddAllocateMetric( PoolHandle handle, const Block & block );
		void AddFreeMetric( PoolHandle handle,const Block & block );
		void AddReallocateMetric( PoolHandle handle,const Block & input, const Block & output );

		HookHandle CreateHook( const HookCreationParameter & param  );
		void RemoveHook( HookHandle handle );
		void ProcessPreHooks( HookPreParameter & param );
		void ProcessPostHooks( HookPostParameter & param );
		void HandleOOM( PoolHandle handle, u32 size, u32 alignment );

		void LogMemoryReport();

	private:

		Vault( const Vault & );
		const Vault & operator=( const Vault & );

		DefaultAllocator m_defaultAllocator;
	
		PlatformSystemAllocator m_systemAllocator;
		PlatformFlexibleAllocator m_flexibleAllocator;
		PlatformPageAllocator m_pageAllocator;

		HookHandler m_hookHandler;
		PoolRegistry m_poolRegistry;
		MetricsRegistry m_metricsRegistry;
		
		ThreadMonitor m_threadMonitor;
		Reporter m_reporter;
		OOMHandlerBreak m_defaultOOMHandler;
	};

	Vault & AcquireVault();
}
}

#endif
