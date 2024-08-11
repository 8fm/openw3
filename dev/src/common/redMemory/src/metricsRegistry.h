/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_METRICS_REGISTRY_H_
#define _RED_MEMORY_METRICS_REGISTRY_H_

#include "poolMetrics.h"
#include "poolConstant.h"

namespace red
{
namespace memory
{
	struct Block;

	const u32 c_maxMetricsCount = c_poolMaxCount;

	class MetricsRegistry
	{
	public:

		MetricsRegistry();
		~MetricsRegistry();

		void OnAllocate( PoolHandle handle, const Block & block ); 
		void OnDeallocate( PoolHandle handle, const Block & block );
		void OnReallocate( PoolHandle handle, const Block & inputBlock, const Block & outputBlock );

		u64 GetPoolTotalBytesAllocated( PoolHandle handle ) const; 
		u32 GetPoolAllocationCount( PoolHandle handle ) const; 
		u64 GetTotalBytesAllocated() const;

	private:

		typedef std::array< PoolMetrics, c_maxMetricsCount > MetricsDictionary;
		
		MetricsDictionary m_metricsDictionary;
	};
}
}

#include "metricsRegistry.hpp"

#endif
