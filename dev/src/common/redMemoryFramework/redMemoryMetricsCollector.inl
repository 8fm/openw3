/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryLog.h"
#include "../redSystem/crt.h"

namespace Red { namespace MemoryFramework {

// How many bottom stack addresses should we ignore
// Dependant on debug / release (i.e. if these are inlined)
#if defined(RED_CONFIGURATION_NOPTS) || defined(RED_CONFIGURATION_DEBUG)
	const Red::System::Int32 k_StackDepthToIgnore = 4;	
#else
	const Red::System::Int32 k_StackDepthToIgnore = 3;	
#endif

// Max length of strings that can be added to single allocations
const Red::System::Int32 k_MaximumAllocStringLength = 256;

////////////////////////////////////////////////////////////////////
// Default log device
//
class AllocationMetricsCollector::LogToMemoryLog
{
public:
	void Log( Red::System::Char* text )
	{
		RED_MEMORY_LOG( text );
	}
};

// Dump helper macro
#define MEMORY_DUMP_DO_LOG( device, fmt, ... )	{       \
		Red::System::Char outputLine[ 512 ];	\
		Red::System::SNPrintF( outputLine, 512, fmt, ##__VA_ARGS__ );	\
		device.Log( outputLine );	\
	}

////////////////////////////////////////////////////////////////////
// DumpClassMemoryReport
//	Output metrics for all memory classes with allocated memory for all pools
template< class LogDevice >
void AllocationMetricsCollector::DumpPoolMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* title )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==== Pool Memory stats for %32" ) RED_PRIWas TXT( "==" ), title );
	DumpPoolClassMemoryReport< LogDevice >( logDevice, "Static Pool", m_staticMetrics.GetMetrics() );
	DumpPoolClassMemoryReport< LogDevice >( logDevice, "Overflow Pool", m_overflowMetrics.GetMetrics() );
	for( Red::System::Uint32 i=0; i < k_MaximumPools; ++i )
	{
		const PoolMetrics& poolMetrics = m_runtimeMetrics[i];
		if( poolMetrics.GetMetrics().m_totalAllocations > 0 || poolMetrics.GetMetrics().m_totalBytesAllocated > 0 )
		{
			Red::System::AnsiChar poolNameBuffer[32] = {'\0'};
			GetPoolName( i, poolNameBuffer, ARRAY_COUNT( poolNameBuffer ) );
			DumpPoolClassMemoryReport< LogDevice >( logDevice, poolNameBuffer, poolMetrics.GetMetrics() );
		}
	}
#endif
}

////////////////////////////////////////////////////////////////////
// DumpPoolClassMemoryReport
//	Output metrics for a particular pool
template< class LogDevice >
void AllocationMetricsCollector::DumpPoolClassMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* poolName, const RuntimePoolMetrics& metrics )
{
	RED_UNUSED( poolName );
	RED_UNUSED( metrics );

#ifdef RED_LOGGING_ENABLED
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==== Class Memory stats for %32" ) RED_PRIWas TXT( "==" ), poolName );

	MEMORY_DUMP_DO_LOG( logDevice, TXT( "Number of allocations: %" ) RED_PRIWd64, metrics.m_totalAllocations );
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "Bytes allocated so far: %1.2fKB" ), metrics.m_totalBytesAllocated / 1024.0f );

	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==========================================================" ) );
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "| Memory class                   |    KB     |  KB peak  |  Blocks   |" ) );
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==========================================================" ) );

	for ( Red::System::Uint32 i=0; i<k_MaximumMemoryClasses; i++ )
	{
		Red::System::Int64 size = metrics.m_allocatedBytesPerMemoryClass[ i ];
		Red::System::Int64 allocations = metrics.m_allocationsPerMemoryClass[ i ];
		Red::System::Int64 peakSize = metrics.m_allocatedBytesPerMemoryClassPeak[ i ];
		if ( size && allocations )
		{
			Red::System::AnsiChar memClassName[64] = {'\0'};
			GetMemoryClassName( i, memClassName, ARRAY_COUNT( memClassName ) );
			MEMORY_DUMP_DO_LOG( logDevice, TXT( "| %30" ) RED_PRIWas TXT(" | %9.2f | %9.2f | %" ) RED_PRIWd64, memClassName, size / 1024.0f, peakSize / 1024.0f, allocations );
		}
	}

	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==========================================================" ) );
#endif	//ENABLE_EXTENDED_MEMORY_METRICS
#endif
}

////////////////////////////////////////////////////////////////////
// DumpClassMemoryReport
//	Output metrics for all memory classes with allocated memory for all pools
template< class LogDevice >
void AllocationMetricsCollector::DumpClassMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* title )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	MEMORY_DUMP_DO_LOG( logDevice, TXT( "==== Reporting Class Memory for %32" ) RED_PRIWas TXT( "==" ), title );
	RuntimePoolMetrics allPoolMetrics;
	PopulateAllMetrics( allPoolMetrics );
	DumpPoolClassMemoryReport< LogDevice >( logDevice, "All Pools", allPoolMetrics );
#endif
}

#ifdef ENABLE_EXTENDED_MEMORY_METRICS

/////////////////////////////////////////////////////////////////////////////////
// GetHistogramData
//	
RED_INLINE const MetricsHistogramData& AllocationMetricsCollector::GetHistogramData() const
{
	return m_allocHistogram;
}

#endif	//ENABLE_EXTENDED_MEMORY_METRICS

/////////////////////////////////////////////////////////////////////////////////
// GetAllocatorAreaCallback
//	Cache the label and return this
RED_INLINE AllocatorAreaCallback& AllocationMetricsCollector::GetAllocatorAreaCallback( PoolLabel thePool )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_areaCallbackPool = thePool;
#endif
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////
// GetMetricsForPool
//
RED_INLINE const RuntimePoolMetrics& AllocationMetricsCollector::GetMetricsForPool( PoolLabel label ) const
{
	RED_MEMORY_ASSERT( label < k_MaximumPools,  "Trying to get the metrics for an invalid pool label" );
	return m_runtimeMetrics[ label ].GetMetrics();
}

/////////////////////////////////////////////////////////////////////////////////
// GetMetricsForStaticPool
//
RED_INLINE const RuntimePoolMetrics& AllocationMetricsCollector::GetMetricsForStaticPool( ) const
{
	return m_staticMetrics.GetMetrics();
}

/////////////////////////////////////////////////////////////////////////////////
// GetMetricsForOverflowPool
//
RED_INLINE const RuntimePoolMetrics& AllocationMetricsCollector::GetMetricsForOverflowPool( ) const
{
	return m_overflowMetrics.GetMetrics();
}

/////////////////////////////////////////////////////////////////////////////////
// ResetMetrics
//	Use this to reset peak per-frame metrics
RED_INLINE void AllocationMetricsCollector::ResetMetrics( )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Reset metrics
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		m_runtimeMetrics[ poolIndex ].ResetPerFramePeakMetrics();
	}
	m_staticMetrics.ResetPerFramePeakMetrics();
	m_overflowMetrics.ResetPerFramePeakMetrics();
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// PopulateAllMetrics
//	Enumerate metrics from all registered pools (Not thread safe - numbers may not be exact)
RED_INLINE void AllocationMetricsCollector::PopulateAllMetrics( RuntimePoolMetrics& metrics )
{
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		metrics.m_totalBytesAllocated += m_runtimeMetrics[ poolIndex ].GetMetrics().m_totalBytesAllocated;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		metrics.m_allocationsPerFrame += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocationsPerFrame;
		metrics.m_allocationsPerFramePeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocationsPerFramePeak;
		metrics.m_deallocationsPerFrame += m_runtimeMetrics[ poolIndex ].GetMetrics().m_deallocationsPerFrame;
		metrics.m_deallocationsPerFramePeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_deallocationsPerFramePeak;
		metrics.m_bytesAllocatedPerFrame += m_runtimeMetrics[ poolIndex ].GetMetrics().m_bytesAllocatedPerFrame;
		metrics.m_bytesAllocatedPerFramePeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_bytesAllocatedPerFramePeak;
		metrics.m_bytesDeallocatedPerFrame += m_runtimeMetrics[ poolIndex ].GetMetrics().m_bytesDeallocatedPerFrame;
		metrics.m_bytesDeallocatedPerFramePeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_bytesDeallocatedPerFramePeak;
		metrics.m_totalAllocations += m_runtimeMetrics[ poolIndex ].GetMetrics().m_totalAllocations;
		metrics.m_totalAllocationsPeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_totalAllocationsPeak;
		metrics.m_totalBytesAllocatedPeak += m_runtimeMetrics[ poolIndex ].GetMetrics().m_totalBytesAllocatedPeak;
		for( Red::System::Uint32 memClassIndex = 0; memClassIndex < k_MaximumMemoryClasses; ++memClassIndex )
		{
			metrics.m_allocatedBytesPerMemoryClass[ memClassIndex ] += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocatedBytesPerMemoryClass[ memClassIndex ];
			metrics.m_allocatedBytesPerMemoryClassPeak[ memClassIndex ] += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocatedBytesPerMemoryClassPeak[ memClassIndex ];
			metrics.m_allocationsPerMemoryClass[ memClassIndex ] += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocationsPerMemoryClass[ memClassIndex ];
			metrics.m_allocationsPerMemoryClassPeak[ memClassIndex ] += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocationsPerMemoryClassPeak[ memClassIndex ];
		}
#endif
	}

	// Also include overflow and statics pools!
	metrics.m_totalBytesAllocated += m_staticMetrics.GetMetrics().m_totalBytesAllocated;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	metrics.m_allocationsPerFrame += m_staticMetrics.GetMetrics().m_allocationsPerFrame;
	metrics.m_allocationsPerFramePeak += m_staticMetrics.GetMetrics().m_allocationsPerFramePeak;
	metrics.m_deallocationsPerFrame += m_staticMetrics.GetMetrics().m_deallocationsPerFrame;
	metrics.m_deallocationsPerFramePeak += m_staticMetrics.GetMetrics().m_deallocationsPerFramePeak;
	metrics.m_bytesAllocatedPerFrame += m_staticMetrics.GetMetrics().m_bytesAllocatedPerFrame;
	metrics.m_bytesAllocatedPerFramePeak += m_staticMetrics.GetMetrics().m_bytesAllocatedPerFramePeak;
	metrics.m_bytesDeallocatedPerFrame += m_staticMetrics.GetMetrics().m_bytesDeallocatedPerFrame;
	metrics.m_bytesDeallocatedPerFramePeak += m_staticMetrics.GetMetrics().m_bytesDeallocatedPerFramePeak;
	metrics.m_totalAllocations += m_staticMetrics.GetMetrics().m_totalAllocations;
	metrics.m_totalAllocationsPeak += m_staticMetrics.GetMetrics().m_totalAllocationsPeak;
	metrics.m_totalBytesAllocatedPeak += m_staticMetrics.GetMetrics().m_totalBytesAllocatedPeak;	
	for( Red::System::Uint32 memClassIndex = 0; memClassIndex < k_MaximumMemoryClasses; ++memClassIndex )
	{
		metrics.m_allocatedBytesPerMemoryClass[ memClassIndex ] += m_staticMetrics.GetMetrics().m_allocatedBytesPerMemoryClass[ memClassIndex ];
		metrics.m_allocatedBytesPerMemoryClassPeak[ memClassIndex ] += m_staticMetrics.GetMetrics().m_allocatedBytesPerMemoryClassPeak[ memClassIndex ];
		metrics.m_allocationsPerMemoryClass[ memClassIndex ] += m_staticMetrics.GetMetrics().m_allocationsPerMemoryClass[ memClassIndex ];
		metrics.m_allocationsPerMemoryClassPeak[ memClassIndex ] += m_staticMetrics.GetMetrics().m_allocationsPerMemoryClassPeak[ memClassIndex ];
	}
#endif

	metrics.m_totalBytesAllocated += m_overflowMetrics.GetMetrics().m_totalBytesAllocated;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	metrics.m_allocationsPerFrame += m_overflowMetrics.GetMetrics().m_allocationsPerFrame;
	metrics.m_allocationsPerFramePeak += m_overflowMetrics.GetMetrics().m_allocationsPerFramePeak;
	metrics.m_deallocationsPerFrame += m_overflowMetrics.GetMetrics().m_deallocationsPerFrame;
	metrics.m_deallocationsPerFramePeak += m_overflowMetrics.GetMetrics().m_deallocationsPerFramePeak;
	metrics.m_bytesAllocatedPerFrame += m_overflowMetrics.GetMetrics().m_bytesAllocatedPerFrame;
	metrics.m_bytesAllocatedPerFramePeak += m_overflowMetrics.GetMetrics().m_bytesAllocatedPerFramePeak;
	metrics.m_bytesDeallocatedPerFrame += m_overflowMetrics.GetMetrics().m_bytesDeallocatedPerFrame;
	metrics.m_bytesDeallocatedPerFramePeak += m_overflowMetrics.GetMetrics().m_bytesDeallocatedPerFramePeak;
	metrics.m_totalAllocations += m_overflowMetrics.GetMetrics().m_totalAllocations;
	metrics.m_totalAllocationsPeak += m_overflowMetrics.GetMetrics().m_totalAllocationsPeak;
	metrics.m_totalBytesAllocatedPeak += m_overflowMetrics.GetMetrics().m_totalBytesAllocatedPeak;
	for( Red::System::Uint32 memClassIndex = 0; memClassIndex < k_MaximumMemoryClasses; ++memClassIndex )
	{
		metrics.m_allocatedBytesPerMemoryClass[ memClassIndex ] += m_overflowMetrics.GetMetrics().m_allocatedBytesPerMemoryClass[ memClassIndex ];
		metrics.m_allocatedBytesPerMemoryClassPeak[ memClassIndex ] += m_overflowMetrics.GetMetrics().m_allocatedBytesPerMemoryClassPeak[ memClassIndex ];
		metrics.m_allocationsPerMemoryClass[ memClassIndex ] += m_overflowMetrics.GetMetrics().m_allocationsPerMemoryClass[ memClassIndex ];
		metrics.m_allocationsPerMemoryClassPeak[ memClassIndex ] += m_overflowMetrics.GetMetrics().m_allocationsPerMemoryClassPeak[ memClassIndex ];
	}	
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetTotalBytesAllocated
//	Return total bytes allocated by all registered pools (Not thread safe - numbers may not be exact)
RED_INLINE Red::System::Int64  AllocationMetricsCollector::GetTotalBytesAllocated( ) const
{
	Red::System::Int64 totalBytesAllocated = 0;
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		totalBytesAllocated += m_runtimeMetrics[ poolIndex ].GetMetrics().m_totalBytesAllocated;
	}

	// Also include overflow and statics pools!
	totalBytesAllocated += m_staticMetrics.GetMetrics().m_totalBytesAllocated;
	totalBytesAllocated += m_overflowMetrics.GetMetrics().m_totalBytesAllocated;

	return totalBytesAllocated;
}

/////////////////////////////////////////////////////////////////////////////////
// GetTotalBytesAllocatedForClass
//	Get the total for a memory class - totals from all pools
RED_INLINE Red::System::Int64 AllocationMetricsCollector::GetTotalBytesAllocatedForClass( MemoryClass memoryClass )
{
	Red::System::Int64 total = 0;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		total += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocatedBytesPerMemoryClass[ memoryClass ];
	}
#endif
	return total;
}

/////////////////////////////////////////////////////////////////////////////////
// GetTotalBytesAllocatedForClass
//	Get the total for a memory class - totals from all pools
RED_INLINE Red::System::Int64 AllocationMetricsCollector::GetTotalBytesAllocatedForClassPeak( MemoryClass memoryClass )
{
	Red::System::Int64 total = 0;
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		total += m_runtimeMetrics[ poolIndex ].GetMetrics().m_allocatedBytesPerMemoryClassPeak[ memoryClass ];
	}
#endif
	return total;
}

/////////////////////////////////////////////////////////////////////////////////
// OnAreaAdded
//	Called when an allocator footprint increases
RED_INLINE void AllocationMetricsCollector::OnAreaAdded( Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		m_serialiser.OnAreaAdded( m_areaCallbackPool, lowAddress, highAddress );
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// OnAreaRemoved
//	Called when an allocator footprint decreases
RED_INLINE void AllocationMetricsCollector::OnAreaRemoved( Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		m_serialiser.OnAreaRemoved( m_areaCallbackPool, lowAddress, highAddress );
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// AppendAllocationStringsFromCallbacks
//	Runs through registered callbacks, building debug strings and sending them to the serialiser
//	Callbacks should append to the end of the string
RED_INLINE Int32 AllocationMetricsCollector::AppendAllocationStringsFromCallbacks( void* address, AnsiChar* allocStringBuffer, Int32 maxCharacters )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	Int32 charactersWritten = 0;

	for( Uint32 i=0; i < m_allocStringCallbacksCount; ++i )
	{
		if( m_allocStringCallbacks[ i ] != nullptr )
		{
			charactersWritten += m_allocStringCallbacks[ i ]( allocStringBuffer, maxCharacters );
		}
	}

	return charactersWritten;
#else
	return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// OnAllocation
//	Should be called for all named allocators registered in the system
//	Note that even failed allocations can go through here!
RED_INLINE void AllocationMetricsCollector::OnAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment )
{
	RED_UNUSED( alignment );

	if( address == nullptr || size == 0 )
	{
		return ;
	}

	m_runtimeMetrics[label].OnAllocate( address, size, memoryClass );
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		AnsiChar allocStringBuffer[ k_MaximumAllocStringLength ] = {'\0'};
		Int32 charactersWritten = AppendAllocationStringsFromCallbacks( address, allocStringBuffer, k_MaximumAllocStringLength );

		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_serialiser.OnAllocation( label, memoryClass, address, size, callstack, allocStringBuffer, charactersWritten );
	}
	if( m_poolToTrackForLeaks == label )
	{
		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_leakTracker.OnAllocation( address, size, memoryClass, callstack );
	}
	m_allocHistogram.OnAllocation( label, memoryClass, size );
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// OnReallocation
//	Should be called for all named allocators registered in the system
//	Note that even failed reallocations can go through here!
RED_INLINE void AllocationMetricsCollector::OnReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment )
{
	OnFree( label, memoryClass, oldAddress, Free_OK, freeSize );
	OnAllocation( label, memoryClass, newAddress, allocatedSize, alignment );
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnFree
//	Should be called for all named allocators registered in the system
//	Note that even failed frees can go through here!
RED_INLINE void AllocationMetricsCollector::OnFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize )
{				 
	if( freeResult != Free_OK || freedSize == 0 )
	{
		return ;
	}

	m_runtimeMetrics[label].OnFree( address, freedSize, memoryClass );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_serialiser.OnFree( label, memoryClass, address, freedSize, callstack );
	}
	if( m_poolToTrackForLeaks == label )
	{
		m_leakTracker.OnFree( address, freedSize );
	}

	m_allocHistogram.OnFree( label, memoryClass, freedSize );
#endif
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnStaticAllocation
//	Catch allocations into the static pool
RED_INLINE void AllocationMetricsCollector::OnStaticAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment )
{				
	RED_UNUSED( alignment );

	if( address == nullptr || size == 0 )
	{
		return ;
	}

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_staticMetrics.OnAllocate( address, size, memoryClass );
	if( m_serialiser.IsWritingDump() )
	{
		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_serialiser.OnStaticAllocation( label, memoryClass, address, size, callstack );
	}
#endif
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnStaticReallocation
//	Catch reallocations into the static pool
RED_INLINE void AllocationMetricsCollector::OnStaticReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment )
{				 
	OnStaticFree( label, memoryClass, oldAddress, Free_OK, freeSize );
	OnStaticAllocation( label, memoryClass, newAddress, allocatedSize, alignment );
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnStaticFree
//	Catch frees from the static pool
RED_INLINE void AllocationMetricsCollector::OnStaticFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize )
{				 
	if( freeResult != Free_OK || freedSize == 0 )
	{
		return ;
	}

	m_staticMetrics.OnFree( address, freedSize, memoryClass );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_serialiser.OnStaticFree( label, memoryClass, address, freedSize, callstack );
	}
#endif
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnOverflowAllocation
//	Catch allocations into the overflow pool
RED_INLINE void AllocationMetricsCollector::OnOverflowAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment )
{				 
	RED_UNUSED( alignment );

	if( address == nullptr || size == 0 )
	{
		return ;
	}

	m_overflowMetrics.OnAllocate( address, size, memoryClass );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		MetricsCallstack callstack(k_StackDepthToIgnore);
		m_serialiser.OnOverflowAllocation( label, memoryClass, address, size, callstack );
	}
#endif
}				 
				 
/////////////////////////////////////////////////////////////////////////////////
// OnOverflowFree
//	Catch frees from the overflow pool
RED_INLINE void AllocationMetricsCollector::OnOverflowFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize )
{
	if( freeResult != Free_OK || freedSize == 0 )
	{
		return ;
	}

	m_overflowMetrics.OnFree( address, freedSize, memoryClass );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		MetricsCallstack callstack( k_StackDepthToIgnore );
		m_serialiser.OnOverflowFree( label, memoryClass, address, freedSize, callstack );
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// OnOverflowReallocation
//	Catch reallocations into the overflow pool
RED_INLINE void AllocationMetricsCollector::OnOverflowReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment )
{				 
	OnOverflowFree( label, memoryClass, oldAddress, Free_OK, freeSize );
	OnOverflowAllocation( label, memoryClass, newAddress, allocatedSize, alignment );
}	

/////////////////////////////////////////////////////////////////////////////////
// GetPoolName
//	Get memory pool name
RED_INLINE Red::System::Bool AllocationMetricsCollector::GetPoolName( PoolLabel label, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_memoryPoolNames->GetNameByLabel( label, buffer, maxCharacters );
#else
	return false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetMemoryClassName
//	Get class name 
RED_INLINE Red::System::Bool AllocationMetricsCollector::GetMemoryClassName( MemoryClass memClass, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_memoryClassNames->GetNameByLabel( memClass, buffer, maxCharacters );
#else
	return false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetMemoryClassGroupCount
//	How many groups total
RED_INLINE Red::System::Uint32 AllocationMetricsCollector::GetMemoryClassGroupCount() const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_groupNames->GetGroupCount();
#else
	return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetMemoryClassCountInGroup
//	Get number of classes in a group
RED_INLINE Red::System::Uint32 AllocationMetricsCollector::GetMemoryClassCountInGroup( Uint32 groupIndex ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_groupNames->GetClassesInGroup( groupIndex );
#else
	return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetMemoryClassInGroup
//	Get a single memory class label from group and class index
RED_INLINE MemoryClass AllocationMetricsCollector::GetMemoryClassInGroup( Uint32 groupIndex, Uint32 memoryClassIndex ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_groupNames->GetMemoryClassInGroup( groupIndex, memoryClassIndex );
#else
	return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
// GetMemoryClassGroupName
//	Get name of a group
RED_INLINE const AnsiChar* AllocationMetricsCollector::GetMemoryClassGroupName( Uint32 groupIndex ) const
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_groupNames->GetGroupLabel( groupIndex );
#else
	return nullptr;
#endif
}


} }