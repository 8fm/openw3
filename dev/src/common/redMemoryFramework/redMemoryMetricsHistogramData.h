/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_METRICS_HISTOGRAM_DATA_H
#define _RED_MEMORY_METRICS_HISTOGRAM_DATA_H

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { 

// This class handles allocation size histograms for all pools and memory classes
class MetricsHistogramData
{
	friend class AllocationMetricsCollector;
	static const Red::System::Uint32 k_smallestAllocationToTrack = 1 << 3;
public:
	static const Red::System::Uint32 k_bucketCount = 32;

	MetricsHistogramData();
	~MetricsHistogramData();

	void Reset();

	Uint32 GetBucketCount() const;
	void GetBucketAllocationRange( Red::System::Uint32 bucketIndex, Red::System::Uint64& minSize, Red::System::Uint64& maxSize ) const;
	Uint32 GetAllocationsInBucket( PoolLabel pool, MemoryClass memClass, Red::System::Uint32 bucketIndex ) const;
	void PopulatePoolHistogram( PoolLabel pool, Red::System::Uint32 (&buckets)[ k_bucketCount ] ) const;

private:
	void OnAllocation( PoolLabel pool, MemoryClass memClass, Red::System::MemSize allocSize );
	void OnFree( PoolLabel pool, MemoryClass memClass, Red::System::MemSize freedSize );

	Red::System::Uint64 CalculateBucketMaxSize( Red::System::Uint32 bucketIndex ) const;
	Red::System::Uint32 CalculateBucket( Red::System::MemSize size ) const;
	Red::System::Uint32 m_activeAllocations[ k_MaximumPools ][ k_MaximumMemoryClasses ][ k_bucketCount ];
};

RED_INLINE Red::System::Uint64 MetricsHistogramData::CalculateBucketMaxSize( Red::System::Uint32 bucketIndex ) const
{
	const Red::System::Uint64 k_smallestAllocationLog2 = Red::System::BitUtils::Log2( k_smallestAllocationToTrack );
	return ((Red::System::Uint64)1u << ( bucketIndex + k_smallestAllocationLog2 ) );
}

RED_INLINE Uint32 MetricsHistogramData::CalculateBucket( Red::System::MemSize size ) const
{
	const Red::System::Uint32 k_smallestAllocationLog2 = Red::System::BitUtils::Log2( k_smallestAllocationToTrack );
	if( size <= k_smallestAllocationToTrack )
	{
		return 0;
	}

	Red::System::Uint32 bucketIndex = static_cast< Red::System::Uint32 >( Red::System::BitUtils::Log2( size ) - k_smallestAllocationLog2 );
	return bucketIndex < k_bucketCount ? bucketIndex : k_bucketCount - 1;
}

} }

#endif