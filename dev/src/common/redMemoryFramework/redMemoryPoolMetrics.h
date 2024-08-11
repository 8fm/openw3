/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_POOL_METRICS_H
#define _RED_MEMORY_POOL_METRICS_H

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////////////
// A wrapper around pool metrics
class RuntimePoolMetrics
{
public:
	RuntimePoolMetrics();
	~RuntimePoolMetrics();

	// This is the only stat we ALWAYS track
	Red::System::Int64		m_totalBytesAllocated;	

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Per-frame statistics
	Red::System::Int64		m_allocationsPerFrame;
	Red::System::Int64		m_allocationsPerFramePeak;
	Red::System::Int64		m_deallocationsPerFrame;
	Red::System::Int64		m_deallocationsPerFramePeak;
	Red::System::Int64		m_bytesAllocatedPerFrame;
	Red::System::Int64		m_bytesAllocatedPerFramePeak;
	Red::System::Int64		m_bytesDeallocatedPerFrame;
	Red::System::Int64		m_bytesDeallocatedPerFramePeak;

	// Total statistics
	Red::System::Int64		m_allocationsPerMemoryClass[ k_MaximumMemoryClasses ];
	Red::System::Int64		m_allocationsPerMemoryClassPeak[ k_MaximumMemoryClasses ];
	Red::System::Int64		m_allocatedBytesPerMemoryClass[ k_MaximumMemoryClasses ];
	Red::System::Int64		m_allocatedBytesPerMemoryClassPeak[ k_MaximumMemoryClasses ];
	Red::System::Int64		m_totalAllocations;
	Red::System::Int64		m_totalAllocationsPeak;
	Red::System::Int64		m_totalBytesAllocatedPeak;
#endif
};

/////////////////////////////////////////////////////////////////////
// PoolMetrics is used to get a run-time breakdown of allocations in a pool
//	Note that it does NOT keep callstack information - it would take too much memory
//	/ time to copy all the data around!
class PoolMetrics
{
public:
	RED_INLINE void OnAllocate( void* address, Red::System::MemSize size, MemoryClass memClass );
	RED_INLINE void OnFree( const void* address, Red::System::MemSize freedSize, MemoryClass memClass );
	RED_INLINE const RuntimePoolMetrics& GetMetrics() const;
	RED_INLINE void ClearPerFrameMetrics();
	RED_INLINE void ResetPerFramePeakMetrics();

private:
	RuntimePoolMetrics m_metrics;
};

} }

#include "redMemoryPoolMetrics.inl"

#endif