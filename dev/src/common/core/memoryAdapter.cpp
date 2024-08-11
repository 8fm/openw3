/**
* Copyright © 2016 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "memoryAdapter.h"
#include "../gpuApiUtils/gpuApiMemory.h"

namespace Memory
{
	Adapter::Adapter()
	{}

	Adapter::~Adapter()
	{}

	class LegacyAdapter : public Adapter
	{
	public:
		LegacyAdapter( Red::MemoryFramework::MemoryManager * manager );

		virtual Uint16 GetMaximumPoolCount() const override final;
		virtual Uint32 GetPoolLabelForIndex( Uint32 index ) const override final; 
		virtual bool GetPoolName( Uint32 label, AnsiChar* buffer, Uint32 maxCharacters ) override final;
		virtual bool PoolExist( Uint32 label ) const override final;
		virtual bool IsDumpingMetrics() override final;
		virtual const AnsiChar * GetMemoryClassName( Uint32 index ) const override final;
		virtual Uint32 GetRegisteredPoolCount() const override final;
		virtual Uint64 GetPoolBudget( Uint32 label ) const override final;

		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForPool( Uint32 label ) const override final;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForStaticPool( ) const override final;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForOverflowPool( ) const override final;
		virtual void PopulateAllMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics ) override final;
		virtual Red::MemoryFramework::IAllocator* GetStaticPool() const override final;
		virtual Red::MemoryFramework::IAllocator* GetOverflowPool() const override final;
		virtual void RequestAllocatorInfo( Uint32 label, Red::MemoryFramework::AllocatorInfo& info ) const override final;
		virtual Red::MemoryFramework::AllocationMetricsCollector& GetMetricsCollector() override final;
		virtual void ResetMetrics() override final;

		virtual void BeginMetricsDump( const Char * filePath ) override final;
		virtual void EndMetricsDump() override final;

	private:

		Red::MemoryFramework::MemoryManager * m_manager;
	};

	LegacyAdapter::LegacyAdapter( Red::MemoryFramework::MemoryManager * manager )
		: m_manager( manager )
	{}

	Uint16 LegacyAdapter::GetMaximumPoolCount() const
	{
		return Red::MemoryFramework::k_MaximumPools;
	}

	Uint32 LegacyAdapter::GetPoolLabelForIndex( Uint32 index ) const
	{
		return m_manager->GetPoolLabelForIndex( index );
	}
	
	bool LegacyAdapter::GetPoolName( Uint32 label, AnsiChar* buffer, Uint32 maxCharacters )
	{
		return GetMetricsCollector().GetPoolName( label, buffer, maxCharacters );
	}
	
	bool LegacyAdapter::PoolExist( Uint32 label ) const
	{
		return m_manager->GetPool( label ) != nullptr;
	}
	
	bool LegacyAdapter::IsDumpingMetrics()
	{
		return m_manager->IsDumpingMetrics();
	}
	
	const AnsiChar * LegacyAdapter::GetMemoryClassName( Uint32 index ) const
	{
		return m_manager->GetMemoryClassName( index );
	}
	
	Uint32 LegacyAdapter::GetRegisteredPoolCount() const
	{
		return m_manager->GetRegisteredPoolCount();
	}
	
	Uint64 LegacyAdapter::GetPoolBudget( Uint32 label ) const
	{
		auto pool = m_manager->GetPool( label );
		if( pool )
		{
			Red::MemoryFramework::AllocatorInfo info;
			pool->RequestAllocatorInfo( info );
			return info.GetBudget();
		}
		
		return 0;
	}

	const Red::MemoryFramework::RuntimePoolMetrics& LegacyAdapter::GetMetricsForPool( Uint32 label ) const
	{
		return m_manager->GetMetricsCollector().GetMetricsForPool( label );
	}
	
	const Red::MemoryFramework::RuntimePoolMetrics& LegacyAdapter::GetMetricsForStaticPool( ) const
	{
		return m_manager->GetMetricsCollector().GetMetricsForStaticPool();
	}
	
	const Red::MemoryFramework::RuntimePoolMetrics& LegacyAdapter::GetMetricsForOverflowPool( ) const
	{
		return m_manager->GetMetricsCollector().GetMetricsForOverflowPool();
	}
	
	void LegacyAdapter::PopulateAllMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics )
	{
		m_manager->GetMetricsCollector().PopulateAllMetrics( metrics );
	}
	
	Red::MemoryFramework::IAllocator* LegacyAdapter::GetStaticPool() const
	{
		return m_manager->GetStaticPool();
	}
	
	Red::MemoryFramework::IAllocator* LegacyAdapter::GetOverflowPool() const
	{
		return m_manager->GetOverflowPool();
	}
	
	void LegacyAdapter::RequestAllocatorInfo( Uint32 label, Red::MemoryFramework::AllocatorInfo& info ) const
	{
		auto pool = m_manager->GetPool( label );
		if( pool )
		{
			pool->RequestAllocatorInfo( info );
		}
	}
	
	Red::MemoryFramework::AllocationMetricsCollector& LegacyAdapter::GetMetricsCollector()
	{
		return m_manager->GetMetricsCollector();
	}
	
	void LegacyAdapter::ResetMetrics()
	{
		m_manager->ResetMetrics();
	}

	void LegacyAdapter::BeginMetricsDump( const Char * filePath )
	{
		m_manager->BeginMetricsDump( filePath );
	}
	
	void LegacyAdapter::EndMetricsDump()
	{
		m_manager->EndMetricsDump();;
	}

	//////////////////////////////////////////////////////////////////////////
#ifdef RED_USE_NEW_MEMORY_SYSTEM

	class redMemoryAdapter : public Adapter
	{
	public:
		redMemoryAdapter();

		virtual Uint16 GetMaximumPoolCount() const override final;
		virtual Uint32 GetPoolLabelForIndex( Uint32 index ) const override final; 
		virtual bool GetPoolName( Uint32 label, AnsiChar* buffer, Uint32 maxCharacters ) override final;
		virtual bool PoolExist( Uint32 label ) const override final;
		virtual bool IsDumpingMetrics() override final;
		virtual const AnsiChar * GetMemoryClassName( Uint32 index ) const override final;
		virtual Uint32 GetRegisteredPoolCount() const override final;
		virtual Uint64 GetPoolBudget( Uint32 label ) const override final;

		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForPool( Uint32 label ) const override final;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForStaticPool( ) const override final;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForOverflowPool( ) const override final;
		virtual void PopulateAllMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics ) override final;
		virtual Red::MemoryFramework::IAllocator* GetStaticPool() const override final;
		virtual Red::MemoryFramework::IAllocator* GetOverflowPool() const override final;
		virtual void RequestAllocatorInfo( Uint32 label, Red::MemoryFramework::AllocatorInfo& info ) const override final;
		virtual Red::MemoryFramework::AllocationMetricsCollector& GetMetricsCollector() override final;
		virtual void ResetMetrics() override final;

		virtual void BeginMetricsDump( const Char * filePath ) override final;
		virtual void EndMetricsDump() override final;

	private:

	};

	redMemoryAdapter::redMemoryAdapter( )
	{
		
	}

	Uint16 redMemoryAdapter::GetMaximumPoolCount() const
	{
		return 128;
	}

	Uint32 redMemoryAdapter::GetPoolLabelForIndex( Uint32 index ) const
	{
		return index;
	}

	bool redMemoryAdapter::GetPoolName( Uint32 label, AnsiChar* buffer, Uint32 maxCharacters )
	{
		if( PoolExist( label ) )
		{
			const char * poolName = red::memory::GetPoolName( label );
			Red::System::StringCopy( buffer, poolName, maxCharacters );
			  
			return true;
		}

		return false;
	}

	bool redMemoryAdapter::PoolExist( Uint32 label ) const
	{
		return red::memory::IsPoolRegistered( label );
	}

	bool redMemoryAdapter::IsDumpingMetrics()
	{
		return false;
	}

	const AnsiChar * redMemoryAdapter::GetMemoryClassName( Uint32 index ) const
	{
		return Memory::GetMemoryClassName( index );
	}

	Uint32 redMemoryAdapter::GetRegisteredPoolCount() const
	{
		return Memory::GetPoolCount();
	}

	Uint64 redMemoryAdapter::GetPoolBudget( Uint32 label ) const
	{
		return Memory::GetPoolBudget( label );
	}

	const Red::MemoryFramework::RuntimePoolMetrics& redMemoryAdapter::GetMetricsForPool( Uint32 label ) const
	{
		return Memory::GetMetricsCollector().GetMetricsForPool( label );
	}

	const Red::MemoryFramework::RuntimePoolMetrics& redMemoryAdapter::GetMetricsForStaticPool( ) const
	{
		return Memory::GetMetricsCollector().GetMetricsForStaticPool();
	}

	const Red::MemoryFramework::RuntimePoolMetrics& redMemoryAdapter::GetMetricsForOverflowPool( ) const
	{
		return Memory::GetMetricsCollector().GetMetricsForOverflowPool();
	}

	void redMemoryAdapter::PopulateAllMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics )
	{
		GetMetricsCollector().PopulateAllMetrics( metrics );
	}

	Red::MemoryFramework::IAllocator* redMemoryAdapter::GetStaticPool() const
	{
		return nullptr;
	}

	Red::MemoryFramework::IAllocator* redMemoryAdapter::GetOverflowPool() const
	{
		return nullptr;
	}

	void redMemoryAdapter::RequestAllocatorInfo( Uint32 label, Red::MemoryFramework::AllocatorInfo& info ) const
	{
		info.SetAllocatorBudget( GetPoolBudget( label ) );
	}

	Red::MemoryFramework::AllocationMetricsCollector& redMemoryAdapter::GetMetricsCollector()
	{
		return Memory::GetMetricsCollector();
	}

	void redMemoryAdapter::ResetMetrics()
	{
		GetMetricsCollector().ResetMetrics();
	}

	void redMemoryAdapter::BeginMetricsDump( const Char * filePath )
	{
		
	}

	void redMemoryAdapter::EndMetricsDump()
	{
		
	}

#endif 

	//////////////////////////////////////////////////////////////////////////

	Red::TUniquePtr< Adapter > CreateCPUAdapter()
	{
#ifndef RED_USE_NEW_MEMORY_SYSTEM
		return Red::TUniquePtr< Adapter >( new LegacyAdapter( &SRedMemory::GetInstance() ) );
#else
		return Red::TUniquePtr< Adapter >( new redMemoryAdapter() );
#endif
	}

	Red::TUniquePtr< Adapter > CreateGPUAdapter()
	{
		return Red::TUniquePtr< Adapter >( new LegacyAdapter( GpuApi::GpuApiMemory::GetInstance() ) );
	}
}
