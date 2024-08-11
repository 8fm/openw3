/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poolRegistry.h"
#include "poolRoot.h"
#include "log.h"
#include "metricsRegistry.h"

namespace red
{
namespace memory
{
	const char * c_poolUnkownName = "<Unknown Pool>";

	PoolRegistry::PoolRegistry()
		: m_currentNode( 0 ) 
	{
		std::memset( &m_nodes, 0, sizeof( m_nodes ) );
	}

	PoolRegistry::~PoolRegistry()
	{}

	PoolHandle PoolRegistry::AcquireHandle()
	{
		atomic::TAtomic32 node = atomic::Increment32( &m_currentNode );
		RED_FATAL_ASSERT( node <= c_poolMaxCount, "No more pool can be registered." );
		return node - 1;
	}

	void PoolRegistry::Register( PoolHandle handle, const PoolParameter & param )
	{
		SetupPoolInfo( handle, param );
	}

	u64 PoolRegistry::GetPoolBudget( PoolHandle handle ) const
	{
		return m_nodes[ handle ].budget;
	}

	const char * PoolRegistry::GetPoolName( PoolHandle handle ) const
	{
		return m_nodes[ handle ].name;
	}

	bool PoolRegistry::IsPoolRegistered( PoolHandle handle ) const
	{
		return static_cast< atomic::TAtomic32 >( handle ) < m_currentNode;
	}
	
	u32 PoolRegistry::GetPoolCount() const
	{
		return m_currentNode;
	}

	u64 PoolRegistry::GetTotalBytesAllocated() const
	{
		u64 bytes = 0;
		for( auto iter = m_nodes.begin(), end = m_nodes.end(); iter != end; ++iter )
		{
			PoolStorage * storage = iter->storage;
			if( storage )
			{
				bytes += storage->bytesAllocated;
			}
		}

		return bytes;
	}

	void PoolRegistry::SetupPoolInfo( PoolHandle node, const PoolParameter & param )
	{
		const char * name = param.name ? param.name : c_poolUnkownName;
		PoolInfo & info = m_nodes[ node ];

		if( !info.budget )
		{
			info.storage = param.storage;
			info.budget = param.budget;
			Memcpy( info.name, name, std::min( c_poolNameMaxSize - 1, static_cast< u32 >( Strlen( name ) ) ) );
			if( param.parentHandle != c_poolNodeInvalid )
			{
				PoolInfo & parent = m_nodes[ param.parentHandle ];
				
				PoolInfo * lastChild = parent.child; 
				
				if( lastChild )
				{
					while( lastChild->sibling ){ lastChild = lastChild->sibling; }
					lastChild->sibling = &info;
				}
				else
				{
					parent.child = &info;
				}
			}
		}
		else
		{ /* Already registered. Should we assert ? */ }
	}

	void PoolRegistry::WritePoolReportToLog( const MetricsRegistry & registry ) const
	{
#ifdef RED_MEMORY_ENABLE_LOGGING

		PoolMetrics metrics;
		Memzero( &metrics, sizeof( metrics ) );
		ComputePoolMetrics( registry, metrics );

		const char * headerPattern = "| %30s | %10s | %10s | %10s | %12s | %12s |";

		RED_MEMORY_LOG( "Pool Informations" );
		RED_MEMORY_LOG( "=======================================================================================================" );
		RED_MEMORY_LOG( headerPattern, "Pool", "Incl. KB", "Excl. KB", "Budget", "Incl. Blocks", "Excl. Blocks" );
		RED_MEMORY_LOG( "=======================================================================================================" );
		LogPoolMetrics( PoolStorageProxy< PoolRoot >::GetHandle(), metrics );
		RED_MEMORY_LOG( "=======================================================================================================" );
#endif	
		RED_UNUSED( registry );
	}

	void PoolRegistry::ComputePoolMetrics( const MetricsRegistry & registry, PoolMetrics & metrics ) const
	{
		const PoolHandle root = PoolStorageProxy< PoolRoot >::GetHandle();
		ComputePoolMetric( root, registry, metrics );
	}

	PoolRegistry::PoolMetric PoolRegistry::ComputePoolMetric( PoolHandle handle, const MetricsRegistry & registry, PoolMetrics & metrics ) const
	{
		const PoolInfo & info = m_nodes[ handle ]; 
		PoolMetric & value = metrics[ handle ];

		const PoolInfo * currentChild = info.child;

		while( currentChild )
		{
			PoolMetric result = ComputePoolMetric( static_cast< PoolHandle >( std::distance( &m_nodes.front(), currentChild ) ), registry, metrics );
			value.inclusiveBytesAllocated += result.inclusiveBytesAllocated;
			value.inclusiveAllocationCount += result.inclusiveAllocationCount;
			currentChild = currentChild->sibling;
		}

		value.exclusiveBytesAllocated = registry.GetPoolTotalBytesAllocated( handle );
		value.exclusiveAllocationCount = registry.GetPoolAllocationCount( handle );

		value.inclusiveBytesAllocated += value.exclusiveBytesAllocated;
		value.inclusiveAllocationCount += value.exclusiveAllocationCount;

		return value;
	}

	void PoolRegistry::LogPoolMetrics( PoolHandle node, const PoolMetrics & metrics ) const
	{
#ifdef RED_MEMORY_ENABLE_LOGGING

		const PoolInfo & info = m_nodes[ node ]; 

		const auto & poolMetric = metrics[ node ];

		const float inclusiveKBytes = poolMetric.inclusiveBytesAllocated / 1024.0f;
		const float exclusiveKBytes = poolMetric.exclusiveBytesAllocated / 1024.0f;

		const u32 inclusiveAllocCount = poolMetric.inclusiveAllocationCount;
		const u32 exclusiveAllocCount = poolMetric.exclusiveAllocationCount;
		const char * poolName =info.name;
		const float budget = info.budget / 1024.0f;

		RED_MEMORY_LOG( "| %30s | %10.2f | %10.2f | %10.2f | %12d | %12d |", poolName, inclusiveKBytes, exclusiveKBytes, budget, inclusiveAllocCount, exclusiveAllocCount );
		
		const PoolInfo * child = info.child;

		if( child )
		{
			LogPoolMetrics( static_cast< PoolHandle >( std::distance( &m_nodes[0], child ) ), metrics );
		}

		const PoolInfo * sibling = info.sibling;

		if( sibling )
		{
			LogPoolMetrics( static_cast< PoolHandle >( std::distance( &m_nodes[0], sibling ) ), metrics );
		}
#endif
		RED_UNUSED( node );
		RED_UNUSED( metrics );
	}
}
}
