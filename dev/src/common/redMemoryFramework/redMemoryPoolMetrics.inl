/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryAtomics.h"

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	#define RESET_STAT( statName )	statName = 0;	statName ## Peak = 0;
	#define RESET_ARRAY_STAT( statName, index )	statName[ index ] = 0;	statName ## Peak[ index ] = 0;
	#define MODIFY_STAT( statName, modAmount ) OSAPI::AtomicAdd( &statName, modAmount );	statName ## Peak = Red::Math::NumericalUtils::Max( statName ## Peak, statName );
	#define MODIFY_ARRAY_STAT( statName, index, modAmount )	OSAPI::AtomicAdd( &statName[ index ], modAmount );	statName ## Peak[ index ] = Red::Math::NumericalUtils::Max( statName ## Peak[ index ], statName[ index ] );
#else
	#define MODIFY_STAT( statName, modAmount )	statName += modAmount;
#endif

namespace Red { namespace MemoryFramework {

///////////////////////////////////////////////////////////////////
// CTor
//
RED_INLINE RuntimePoolMetrics::RuntimePoolMetrics() :
	m_totalBytesAllocated(0)
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	, m_allocationsPerFrame(0)
	, m_allocationsPerFramePeak(0)
	, m_deallocationsPerFrame(0)
	, m_deallocationsPerFramePeak(0)
	, m_bytesAllocatedPerFrame(0)
	, m_bytesAllocatedPerFramePeak(0)
	, m_bytesDeallocatedPerFrame(0)
	, m_bytesDeallocatedPerFramePeak(0)
	, m_totalAllocations(0)
	, m_totalAllocationsPeak(0)
	, m_totalBytesAllocatedPeak(0)
#endif
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	for( Red::System::Uint32 memClass = 0; memClass < k_MaximumMemoryClasses; ++memClass )
	{
		RESET_ARRAY_STAT( m_allocationsPerMemoryClass, memClass );
		RESET_ARRAY_STAT( m_allocatedBytesPerMemoryClass, memClass );
	}
#endif
}

///////////////////////////////////////////////////////////////////
// DTor
//
RED_INLINE RuntimePoolMetrics::~RuntimePoolMetrics()
{
}

///////////////////////////////////////////////////////////////////
// ClearPerFrameMetrics
//	Reset per-frame metrics
RED_INLINE void PoolMetrics::ClearPerFrameMetrics()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_metrics.m_allocationsPerFrame = 0;
	m_metrics.m_deallocationsPerFrame = 0;
	m_metrics.m_bytesAllocatedPerFrame = 0;
	m_metrics.m_bytesDeallocatedPerFrame = 0;
#endif
}

RED_INLINE void PoolMetrics::ResetPerFramePeakMetrics()
{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	m_metrics.m_allocationsPerFramePeak = 0;
	m_metrics.m_deallocationsPerFramePeak = 0;
	m_metrics.m_bytesAllocatedPerFramePeak = 0;
	m_metrics.m_bytesDeallocatedPerFramePeak = 0;
#endif
}

///////////////////////////////////////////////////////////////////
// OnAllocate
//	Update metrics
RED_INLINE void PoolMetrics::OnAllocate( void* address, Red::System::MemSize size, MemoryClass memClass )
{
	RED_UNUSED( address );
	Red::System::Int64 sizeAsInt = static_cast< Red::System::Int64 >( size );
	MODIFY_STAT( m_metrics.m_totalBytesAllocated, sizeAsInt );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	MODIFY_STAT( m_metrics.m_allocationsPerFrame, 1 );
	MODIFY_STAT( m_metrics.m_bytesAllocatedPerFrame, sizeAsInt );
	MODIFY_STAT( m_metrics.m_totalAllocations, 1 );
	MODIFY_ARRAY_STAT( m_metrics.m_allocationsPerMemoryClass, memClass, 1 );
	MODIFY_ARRAY_STAT( m_metrics.m_allocatedBytesPerMemoryClass, memClass, sizeAsInt );
#endif
}

///////////////////////////////////////////////////////////////////
// OnFree
//	Fixup metrics
RED_INLINE void PoolMetrics::OnFree( const void* address, Red::System::MemSize freedSize, MemoryClass memClass )
{
	RED_UNUSED( address );
	Red::System::Int64 sizeAsInt = static_cast< Red::System::Int64 >( freedSize );
	MODIFY_STAT( m_metrics.m_totalBytesAllocated, -sizeAsInt );

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	MODIFY_STAT( m_metrics.m_deallocationsPerFrame, 1 );
	MODIFY_STAT( m_metrics.m_bytesDeallocatedPerFrame, sizeAsInt );
	MODIFY_STAT( m_metrics.m_totalAllocations, -1 );
	MODIFY_ARRAY_STAT( m_metrics.m_allocationsPerMemoryClass, memClass, -1 );
	MODIFY_ARRAY_STAT( m_metrics.m_allocatedBytesPerMemoryClass, memClass, -sizeAsInt );
#endif
}

///////////////////////////////////////////////////////////////////
// GetMetrics
//	Get a const reference to the metrics
RED_INLINE const RuntimePoolMetrics& PoolMetrics::GetMetrics() const
{
	return m_metrics;
}

} }