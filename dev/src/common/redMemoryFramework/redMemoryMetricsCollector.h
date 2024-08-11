/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ALLOC_METRICS_COLLECTOR_H
#define _RED_MEMORY_ALLOC_METRICS_COLLECTOR_H

#include "redMemoryFrameworkTypes.h"
#include "redMemoryMetricsSerialiser.h"
#include "redMemoryPoolMetrics.h"
#include "redMemoryLeakTracker.h"
#include "redMemoryMetricsHistogramData.h"
#include "redMemoryMetricsClassGroup.h"
#include "../redSystem/nameHash.h"

namespace Red { namespace MemoryFramework {

class IAllocator;

// This callback is used to 'attach' a string to an allocation. They will then be displayed in the tool
typedef Uint32 ( *MetricsAllocStringCallback )( AnsiChar* metricsString, Uint32 maximumCharacters );

///////////////////////////////////////////////////////////////
// AllocationMetricsCollector
//	This class handles all metrics collected from the memory framework
class AllocationMetricsCollector : public AllocatorAreaCallback
{
friend class MemoryManager;
public:
	AllocationMetricsCollector();
	~AllocationMetricsCollector();

	// Get the area callback for a particular pool
	AllocatorAreaCallback& GetAllocatorAreaCallback( PoolLabel thePool );

	// Pass a pool and class name lookup table so the metrics can report strings rather than labels
	void SetMetricsNameLookupTables( PoolNamesList* poolNames, MemoryClassNamesList* classNames, MemoryClassGroups< k_MaximumMemoryClassGroups >* groupNames );

	// Register allocation string callback. Clients can associate a string with an allocation which will be displayed in the memory tool
	void RegisterAllocationStringCallback( MetricsAllocStringCallback callback );

	// These should be called for all named allocators registered in the system
	RED_INLINE	void OnAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment );
	RED_INLINE	void OnReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment );
	RED_INLINE	void OnFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize );

	// Catch allocations from static / global objects (Keep hold of the label too!)
	RED_INLINE	void OnStaticAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment );
	RED_INLINE	void OnStaticReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment );
	RED_INLINE	void OnStaticFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize );

	// Catch allocations from the overflow buffer
	RED_INLINE	void OnOverflowAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, Red::System::MemSize alignment );
	RED_INLINE	void OnOverflowReallocation( PoolLabel label, MemoryClass memoryClass, void* oldAddress, void* newAddress, Red::System::MemSize freeSize, Red::System::MemSize allocatedSize, Red::System::MemSize alignment );
	RED_INLINE	void OnOverflowFree( PoolLabel label, MemoryClass memoryClass, const void* address, EAllocatorFreeResults freeResult, Red::System::MemSize freedSize );

	// Runtime metrics
	RED_INLINE const RuntimePoolMetrics& GetMetricsForPool( PoolLabel label ) const;
	RED_INLINE const RuntimePoolMetrics& GetMetricsForStaticPool( ) const;
	RED_INLINE const RuntimePoolMetrics& GetMetricsForOverflowPool( ) const;
	RED_INLINE void PopulateAllMetrics( RuntimePoolMetrics& metrics );		// Get metrics for everything
	RED_INLINE void ResetMetrics( );
	RED_INLINE Red::System::Int64 GetTotalBytesAllocated( ) const;
	RED_INLINE Red::System::Int64 GetTotalBytesAllocatedForClass( MemoryClass memoryClass );
	RED_INLINE Red::System::Int64 GetTotalBytesAllocatedForClassPeak( MemoryClass memoryClass );

	// Pool / memory class names
	RED_INLINE Red::System::Bool GetPoolName( PoolLabel label, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters ) const;
	RED_INLINE Red::System::Bool GetMemoryClassName( MemoryClass memClass, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters ) const;
	RED_INLINE Red::System::Uint32 GetMemoryClassGroupCount() const;
	RED_INLINE Red::System::Uint32 GetMemoryClassCountInGroup( Uint32 groupIndex ) const;
	RED_INLINE const AnsiChar* GetMemoryClassGroupName( Uint32 groupIndex ) const;
	RED_INLINE MemoryClass GetMemoryClassInGroup( Uint32 groupIndex, Uint32 memoryClassIndex ) const;

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	void BeginTrackingLeaks( PoolLabel poolToTrack );				// Begin tracking allocations for a specific pool
	void EndTrackingLeaks( );										// Stop tracking new allocations
	PoolLabel GetLeakTrackerPoolLabel();							// Get the label of the tracked pool
	MemoryLeakTracker& GetMemoryLeakTracker();						// Memory leak tracker access
#endif

	// Metrics dumps. Pass a class with a Log( const Char* text )
	class LogToMemoryLog;
	template< class LogDevice > void DumpClassMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* title );
	template< class LogDevice > void DumpPoolMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* title );

	// Dump a 'diff' of two runtime metrics objects. Useful for finding out how much got allocated / freed during a particular time
	void DumpRuntimeMemoryDifference( const RuntimePoolMetrics& start, const RuntimePoolMetrics& end );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Return the histogram data
	const MetricsHistogramData& GetHistogramData() const;
#endif

	// Metrics events
	void OnFrameStart();
	void OnFrameEnd();

private:
	// Area events
	void OnAreaAdded( Red::System::MemUint lowAddress, Red::System::MemUint highAddress );
	void OnAreaRemoved( Red::System::MemUint lowAddress, Red::System::MemUint highAddress );


	// Metrics file dumps
	void BeginMetricsDump( const Red::System::Char* filename );				// Write metrics to a file
	void WriteAllocatorHeader( PoolLabel label, IAllocator* theAllocator );	// Dump the header for a particular allocator
	void EndMetricsDumpHeader();											// Signal that we can start writing to the dump
	void EndMetricsDump( );													// Stop writing metrics to a file
	Red::System::Bool IsDumpingMetrics();									// Check if the metrics file is being written
	Int32 AppendAllocationStringsFromCallbacks( void* address, AnsiChar* allocStringBuffer, Int32 maxCharacters );

	template< class LogDevice > 
	void DumpPoolClassMemoryReport( LogDevice& logDevice, const Red::System::AnsiChar* poolName, const RuntimePoolMetrics& metrics );	// Write memory class stats for a particular pool

	PoolMetrics	m_runtimeMetrics[k_MaximumPools];					// Track the allocations for individual pools (runtime debugging)
	PoolMetrics	m_overflowMetrics;									// Track the allocations in the overflow pool (runtime debugging)
	PoolMetrics	m_staticMetrics;									// Track the allocations in the static pool (runtime debugging)
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	PoolNamesList* m_memoryPoolNames;								// Lookup table for memory pool names
	MemoryClassNamesList* m_memoryClassNames;						// Lookup table for memory class names
	MemoryClassGroups< k_MaximumMemoryClassGroups >* m_groupNames;	// Lookup table for memory class groups
	MetricsSerialiser m_serialiser;									// Used to dump the metrics to a file
	MemoryLeakTracker m_leakTracker;								// Memory leak tracking
	PoolLabel m_poolToTrackForLeaks;								// Pool to track
	Red::System::Int64 m_currentFrameId;							// Frame ID is used to pass tags to the serialiser
	PoolLabel m_areaCallbackPool;									// Cache the label used in area callbacks
	MetricsHistogramData m_allocHistogram;							// Histogram of allocations
	Bool m_frameStarted;
	static const Uint32 c_maxAllocStringCallbacks = 4;				// Num. callbacks that can add strings to allocations
	MetricsAllocStringCallback m_allocStringCallbacks[ c_maxAllocStringCallbacks ];
	Uint32 m_allocStringCallbacksCount;
#endif
};

} }

#include "redMemoryMetricsCollector.inl"

#endif