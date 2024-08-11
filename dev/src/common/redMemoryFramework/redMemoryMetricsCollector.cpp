/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsCollector.h"
#include "redMemoryAllocator.h"
#include "redMemoryLog.h"

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// CTor
//
AllocationMetricsCollector::AllocationMetricsCollector()
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	: m_memoryPoolNames(nullptr)
	, m_memoryClassNames(nullptr)
	, m_groupNames(nullptr)
	, m_poolToTrackForLeaks( (PoolLabel)-1 )
	, m_currentFrameId( 0 )
	, m_areaCallbackPool( (PoolLabel)-1 )
	, m_frameStarted(false)
#endif
{
}

///////////////////////////////////////////////////////////////////
// DTor
//
AllocationMetricsCollector::~AllocationMetricsCollector()
{
}

///////////////////////////////////////////////////////////////////
// RegisterAllocationStringCallback
//
void AllocationMetricsCollector::RegisterAllocationStringCallback( MetricsAllocStringCallback callback )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	RED_MEMORY_ASSERT( m_allocStringCallbacksCount < c_maxAllocStringCallbacks, "Too many callbacks registered. Increase c_maxAllocStringCallbacks" );
	m_allocStringCallbacks[ m_allocStringCallbacksCount++ ] = callback;
#endif
}

///////////////////////////////////////////////////////////////////
// DumpRuntimeMemoryDifference
//
void AllocationMetricsCollector::DumpRuntimeMemoryDifference( const RuntimePoolMetrics& start, const RuntimePoolMetrics& end )
{
#if defined( RED_LOGGING_ENABLED ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )
	Red::System::Int64 totalActiveBytesDiff = end.m_totalBytesAllocated - start.m_totalBytesAllocated;
	Red::System::Int64 totalActiveAllocDiff = end.m_totalAllocations - start.m_totalAllocations;
	Red::System::Int64 totalAllocsDiff = end.m_allocationsPerFrame - start.m_allocationsPerFrame;
	Red::System::Int64 totalFreesDiff = end.m_deallocationsPerFrame - start.m_deallocationsPerFrame;

	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" ) );
	RED_MEMORY_LOG( TXT( "Memory Diff Report" ) );
	RED_MEMORY_LOG( TXT( "Total allocations performed: %ld. Total deallocations performed: %ld" ), totalAllocsDiff, totalFreesDiff );
	RED_MEMORY_LOG( TXT( "Total new active allocs: %ld (%ld bytes)" ), totalActiveAllocDiff, totalActiveBytesDiff );
	RED_MEMORY_LOG( TXT( "...................................................................................." ) );
	RED_MEMORY_LOG( TXT( "Breakdown per memory-class" ) );
	RED_MEMORY_LOG( TXT( "Memory Class                             | Allocations         | Kb Allocated" ) );
	for( Uint32 memClass = 0; memClass < k_MaximumMemoryClasses; ++memClass )
	{
		Red::System::Int64 activeAllocsDifference = end.m_allocationsPerMemoryClass[ memClass ] - start.m_allocationsPerMemoryClass[ memClass ];
		Red::System::Int64 activeBytesDifference = end.m_allocatedBytesPerMemoryClass[ memClass ] - start.m_allocatedBytesPerMemoryClass[ memClass ];
		if( activeAllocsDifference != 0 || activeBytesDifference != 0 )
		{
			Red::System::AnsiChar memClassName[ 65 ] = { '\0' };
			GetMemoryClassName( memClass, memClassName, 64 );
			RED_MEMORY_LOG( TXT( "%40" ) RED_PRIWas TXT(" | %19ld | %.2f" ), memClassName, activeAllocsDifference, (Red::System::Float)activeBytesDifference / 1024.0f );
		}
	}
	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" ) );
	RED_UNUSED( totalActiveAllocDiff );
	RED_UNUSED( totalActiveBytesDiff );
	RED_UNUSED( totalAllocsDiff );
	RED_UNUSED( totalFreesDiff );
#else
	RED_UNUSED( start );
	RED_UNUSED( end );
#endif
}

#ifdef ENABLE_EXTENDED_MEMORY_METRICS

///////////////////////////////////////////////////////////////////
// BeginTrackingLeaks
//	Begin tracking allocations for a specific pool
void AllocationMetricsCollector::BeginTrackingLeaks( PoolLabel poolToTrack )
{
	m_leakTracker.ResetLeaksList();
	m_leakTracker.EnableAllocationTracking();
	m_poolToTrackForLeaks = poolToTrack;
}

///////////////////////////////////////////////////////////////////
// EndTrackingLeaks
//	Stop tracking new allocations
void AllocationMetricsCollector::EndTrackingLeaks( )
{
	m_poolToTrackForLeaks = (PoolLabel)-1;
}

///////////////////////////////////////////////////////////////////
// GetMemoryLeakTracker
//	Memory leak tracker access
MemoryLeakTracker& AllocationMetricsCollector::GetMemoryLeakTracker()
{
	return m_leakTracker;
}

///////////////////////////////////////////////////////////////////
// GetLeakTrackerPoolLabel
//	Get the label of the tracked pool
PoolLabel AllocationMetricsCollector::GetLeakTrackerPoolLabel()
{
	return m_poolToTrackForLeaks;
}

#endif	// ENABLE_EXTENDED_MEMORY_METRICS

///////////////////////////////////////////////////////////////////
// OnFrameStart
//
void AllocationMetricsCollector::OnFrameStart()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	if( m_serialiser.IsWritingDump() )
	{
		m_frameStarted = true;
	}
#endif
}

///////////////////////////////////////////////////////////////////
// OnFrameEnd
//	Reset per-frame metrics at the end of each frame
void AllocationMetricsCollector::OnFrameEnd()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Generate an end-of-frame tag
	if( m_serialiser.IsWritingDump() && m_frameStarted )
	{
		m_frameStarted = false;
	}

	// Reset metrics
	for( Red::System::Uint32 poolIndex = 0; poolIndex < k_MaximumPools; ++poolIndex )
	{
		m_runtimeMetrics[ poolIndex ].ClearPerFrameMetrics();
	}
	m_staticMetrics.ClearPerFrameMetrics();
	m_overflowMetrics.ClearPerFrameMetrics();

	++m_currentFrameId;
#endif
}

///////////////////////////////////////////////////////////////////
// BeginMetricsDump
//	Write metrics to a file
void AllocationMetricsCollector::BeginMetricsDump( const Red::System::Char* filename )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_serialiser.BeginDump( filename );
	m_serialiser.BeginHeader();
#endif
}

///////////////////////////////////////////////////////////////////
// EndMetricsDumpHeader
//	Signal that we have finished writing the dump header
void AllocationMetricsCollector::EndMetricsDumpHeader()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	
	m_serialiser.EndHeader();

#endif
}

///////////////////////////////////////////////////////////////////
// WriteAllocatorHeader
//	Dump the header for a particular allocator
void AllocationMetricsCollector::WriteAllocatorHeader( PoolLabel label, IAllocator* theAllocator )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_serialiser.BeginAllocatorHeader( label );
	theAllocator->WalkAllocator( &m_serialiser );
	m_serialiser.EndAllocatorHeader();
#endif
}

///////////////////////////////////////////////////////////////////
// EndMetricsDump
//	Stop writing metrics to a file
void AllocationMetricsCollector::EndMetricsDump( )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

	if( m_frameStarted )
	{
		m_frameStarted = false;
	}
	m_serialiser.EndDump();
#endif
}

///////////////////////////////////////////////////////////////////
// IsDumpingMetrics
//	Check if the metrics file is being written
Red::System::Bool AllocationMetricsCollector::IsDumpingMetrics()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	return m_serialiser.IsWritingDump();
#else
	return false;
#endif
}

///////////////////////////////////////////////////////////////////
// SetMetricsNameLookupTables
//	Pass a pool and class name lookup table so the metrics can report strings rather than labels
void AllocationMetricsCollector::SetMetricsNameLookupTables( PoolNamesList* poolNames, MemoryClassNamesList* classNames, MemoryClassGroups< k_MaximumMemoryClassGroups >* groupNames )
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_memoryPoolNames = poolNames;
	m_memoryClassNames = classNames;
	m_groupNames = groupNames;
	m_serialiser.SetMetricsNameLookupTables( poolNames, classNames );
#endif
}

} } 
