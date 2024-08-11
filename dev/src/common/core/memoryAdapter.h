/**
* Copyright © 2016 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CORE_MEMORY_ADAPTER_H_
#define _RED_CORE_MEMORY_ADAPTER_H_

#include "uniquePtr.h"

namespace Memory
{
	class Adapter
	{
	public:

		Adapter();
		virtual ~Adapter();

		virtual Uint16 GetMaximumPoolCount() const = 0;
		virtual Uint32 GetPoolLabelForIndex( Uint32 index ) const = 0; 
		virtual bool GetPoolName( Uint32 label, AnsiChar* buffer, Uint32 maxCharacters ) = 0;
		virtual bool PoolExist( Uint32 label ) const = 0;
		virtual bool IsDumpingMetrics() = 0;
		virtual const AnsiChar * GetMemoryClassName( Uint32 index ) const = 0;
		virtual Uint32 GetRegisteredPoolCount() const = 0;
		virtual Uint64 GetPoolBudget( Uint32 label ) const = 0;
	
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForPool( Uint32 label ) const = 0;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForStaticPool( ) const = 0;
		virtual const Red::MemoryFramework::RuntimePoolMetrics& GetMetricsForOverflowPool( ) const = 0;
		virtual void PopulateAllMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics ) = 0;
		virtual Red::MemoryFramework::IAllocator* GetStaticPool() const = 0;
		virtual Red::MemoryFramework::IAllocator* GetOverflowPool() const = 0;
		virtual void RequestAllocatorInfo( Uint32 label, Red::MemoryFramework::AllocatorInfo& info ) const = 0;
		virtual Red::MemoryFramework::AllocationMetricsCollector& GetMetricsCollector() = 0;
		virtual void ResetMetrics() = 0;
	
		virtual void BeginMetricsDump( const Char * filePath ) = 0;
		virtual void EndMetricsDump() = 0;
	};

	Red::TUniquePtr< Adapter > CreateCPUAdapter();
	Red::TUniquePtr< Adapter > CreateGPUAdapter();
}

#endif
