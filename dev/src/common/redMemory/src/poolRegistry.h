/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_REGISTRY_H_
#define _RED_MEMORY_POOL_REGISTRY_H_

#include "../include/poolTypes.h"
#include "poolConstant.h"

namespace red
{
namespace memory
{
	const u32 c_poolNameMaxSize = 32;

	class MetricsRegistry;
	struct PoolStorage;

	class RED_MEMORY_API PoolRegistry
	{
	public:

		PoolRegistry();
		~PoolRegistry();

		PoolHandle AcquireHandle();
		void Register( PoolHandle handle, const PoolParameter & param );

		u64 GetPoolBudget( PoolHandle handle ) const;
		const char * GetPoolName( PoolHandle handle ) const; 
		
		bool IsPoolRegistered( PoolHandle handle ) const;
		u32 GetPoolCount() const;

		u64 GetTotalBytesAllocated() const;

		void WritePoolReportToLog( const MetricsRegistry & metrics ) const;
	
	private:

		struct PoolInfo
		{
			u64 budget;
			PoolStorage * storage;
			PoolInfo* child;
			PoolInfo* sibling;
			char name[ c_poolNameMaxSize ];
		};

		struct PoolMetric
		{
			u64 inclusiveBytesAllocated;
			u64 exclusiveBytesAllocated;

			u32 inclusiveAllocationCount;
			u32 exclusiveAllocationCount;
		};

		typedef std::array< PoolInfo, c_poolMaxCount > PoolNodes; 
		typedef std::array< PoolMetric, c_poolMaxCount > PoolMetrics;

		void SetupPoolInfo( PoolHandle node, const PoolParameter & param );
		void ComputePoolMetrics( const MetricsRegistry & registry, PoolMetrics & metrics ) const;
		PoolMetric ComputePoolMetric( PoolHandle handle, const MetricsRegistry & registry, PoolMetrics & metrics ) const;
		void LogPoolMetrics( PoolHandle node, const PoolMetrics & metrics ) const;

		PoolNodes m_nodes;
		atomic::TAtomic32 m_currentNode;
	};
}
}

#include "poolRegistry.hpp"

#endif
