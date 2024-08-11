/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "vault.h"
#include "systemAllocator.h"
#include "log.h"
#include "poolRoot.h"

namespace red

{
namespace memory
{
	void InitializePermanentHooks( HookHandler & handler );

	Vault::Vault()
	{}

	Vault::~Vault()
	{
		m_threadMonitor.Uninitialize();
	}

	void Vault::Initialize()
	{
		m_pageAllocator.Initialize();
		m_systemAllocator.Initialize( &m_pageAllocator );
		m_flexibleAllocator.Initialize( &m_pageAllocator );

		DefaultAllocatorParameter defaultAllocParam = 
		{
			&m_systemAllocator,
			&m_threadMonitor
		};

		m_defaultAllocator.Initialize( defaultAllocParam );
		m_defaultOOMHandler.SetPoolRegistry( &m_poolRegistry );

		ReporterParameter param = 
		{
			&m_poolRegistry,
			&m_metricsRegistry,
			&m_systemAllocator
		};

		m_reporter.Initialize( param );
		m_defaultOOMHandler.Initialize( &m_reporter );
		m_hookHandler.Initialize();

		InitializePermanentHooks( m_hookHandler );

		m_defaultAllocator.RegisterCurrentThread();
	}

	DefaultAllocator & Vault::GetDefaultAllocator()
	{
		return m_defaultAllocator;
	}

	SystemAllocator & Vault::GetSystemAllocator()
	{
		return m_systemAllocator;
	}

	SystemAllocator & Vault::GetFlexibleAllocator()
	{
		return m_flexibleAllocator;
	}

	PoolHandle Vault::AcquirePoolNodeHandle()
	{
		return m_poolRegistry.AcquireHandle();
	}

	void Vault::RegisterPool( PoolHandle handle, const PoolParameter & param )
	{
		return m_poolRegistry.Register( handle, param );
	}

	u64 Vault::GetPoolBudget( PoolHandle handle ) const
	{
		return m_poolRegistry.GetPoolBudget( handle );
	}

	u64 Vault::GetPoolTotalBytesAllocated( PoolHandle handle ) const
	{
		return m_metricsRegistry.GetPoolTotalBytesAllocated( handle );
	}

	const char * Vault::GetPoolName( PoolHandle handle ) const
	{
		return m_poolRegistry.GetPoolName( handle );
	}

	bool Vault::IsPoolRegistered( PoolHandle handle ) const
	{
		return m_poolRegistry.IsPoolRegistered( handle );
	}

	u32 Vault::GetPoolCount() const
	{
		return m_poolRegistry.GetPoolCount();
	}

	u64 Vault::GetTotalBytesAllocated() const
	{
		return m_poolRegistry.GetTotalBytesAllocated();
	}

	void Vault::AddAllocateMetric( PoolHandle handle, const Block & block )
	{
		m_metricsRegistry.OnAllocate( handle, block );
	}

	void Vault::AddFreeMetric( PoolHandle handle,const Block & block )
	{
		m_metricsRegistry.OnDeallocate( handle, block );
	}
	
	void Vault::AddReallocateMetric( PoolHandle handle,const Block & input, const Block & output )
	{
		m_metricsRegistry.OnReallocate( handle, input, output );
	}

	void Vault::HandleOOM( PoolHandle handle, u32 size, u32 alignment )
	{
		m_defaultOOMHandler.HandleAllocateFailure( handle, size, alignment );
	}

	HookHandle Vault::CreateHook( const HookCreationParameter & param  )
	{
		return m_hookHandler.Create( param );
	}

	void Vault::RemoveHook( HookHandle handle )
	{
		m_hookHandler.Remove( handle );
	}

	void Vault::ProcessPreHooks( HookPreParameter & param )
	{
		m_hookHandler.ProcessPreHooks( param );
	}

	void Vault::ProcessPostHooks( HookPostParameter & param )
	{
		m_hookHandler.ProcessPostHooks( param );
	}

	void Vault::LogMemoryReport()
	{
		m_reporter.WriteReportToLog();
	}

	struct VaultProxy
	{
		VaultProxy()
		{
			instance.Initialize();
		}

		Vault instance;
	};


#ifdef RED_COMPILER_MSC
	RED_DISABLE_WARNING_MSC( 4074 )
#pragma init_seg( compiler )
#endif

	static VaultProxy s_vault RED_STATIC_PRIORITY( 101 ); 

	Vault & AcquireVault()
	{
		return s_vault.instance;
	}

	PoolStorage StaticPoolStorage< PoolDefault >::storage = 
	{
		&AcquireVault().GetDefaultAllocator(),
		0,
		nullptr,
		internal::AcquirePoolHandle()
	};

	PoolStorage StaticPoolStorage< PoolLegacy >::storage = 
	{
		&AcquireVault().GetDefaultAllocator(),
		0,
		nullptr,
		internal::AcquirePoolHandle()
	};
}
}
