/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "metricsRegistry.h"
#include "block.h"

namespace red
{
namespace memory
{
	MetricsRegistry::MetricsRegistry()
	{
		std::memset( &m_metricsDictionary, 0, sizeof( m_metricsDictionary ) );
	}

	MetricsRegistry::~MetricsRegistry()
	{}

	void MetricsRegistry::OnAllocate( PoolHandle handle, const Block & block )
	{
		UpdatePoolAllocateMetrics( m_metricsDictionary[ handle ], block.size );
	}
	
	void MetricsRegistry::OnDeallocate( PoolHandle handle, const Block & block )
	{
		UpdatePoolDeallocateMetrics( m_metricsDictionary[ handle ], block.size );
	}
	
	void MetricsRegistry::OnReallocate( PoolHandle handle, const Block & inputBlock, const Block & outputBlock )
	{
		UpdatePoolReallocateMetrics( m_metricsDictionary[ handle ], inputBlock.size, outputBlock.size );
	}

	u64 MetricsRegistry::GetPoolTotalBytesAllocated( PoolHandle handle ) const
	{
		return m_metricsDictionary[ handle ].bytesAllocated;
	}

	u32 MetricsRegistry::GetPoolAllocationCount( PoolHandle handle ) const
	{
#ifdef RED_MEMORY_ENABLE_EXTENDED_METRICS
		return m_metricsDictionary[ handle ].allocationCount;
#else
		RED_UNUSED( handle );
		return 0;
#endif
	}

	u64 MetricsRegistry::GetTotalBytesAllocated() const
	{
		u64 bytes = 0;

		for( auto iter = m_metricsDictionary.begin(), end = m_metricsDictionary.end(); iter != end; ++iter )
		{
			bytes += iter->bytesAllocated;
		}

		return bytes; 
	}
}
}
