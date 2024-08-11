/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsHistogramData.h"

namespace Red { namespace MemoryFramework { 

//#pragma optimize( "", off )
MetricsHistogramData::MetricsHistogramData()
{
	Reset();
}

MetricsHistogramData::~MetricsHistogramData()
{

}

void MetricsHistogramData::Reset()
{
	Red::System::MemorySet( m_activeAllocations, 0, sizeof( Red::System::Uint32 ) * k_MaximumPools * k_MaximumMemoryClasses * k_bucketCount );
}

void MetricsHistogramData::OnAllocation( PoolLabel pool, MemoryClass memClass, Red::System::MemSize allocSize )
{
	RED_MEMORY_ASSERT( pool < k_MaximumPools, "Bad pool index" );
	RED_MEMORY_ASSERT( memClass < k_MaximumMemoryClasses, "Bad memory class index" );

	Red::System::Uint32 bucketIndex = CalculateBucket( allocSize );
	++m_activeAllocations[ pool ][ memClass ][ bucketIndex ];
}

void MetricsHistogramData::OnFree( PoolLabel pool, MemoryClass memClass, Red::System::MemSize freedSize )
{
	RED_MEMORY_ASSERT( pool < k_MaximumPools, "Bad pool index" );
	RED_MEMORY_ASSERT( memClass < k_MaximumMemoryClasses, "Bad memory class index" );

	Red::System::Uint32 bucketIndex = CalculateBucket( freedSize );
	Uint32 activeAllocs = m_activeAllocations[ pool ][ memClass ][ bucketIndex ];
	if( activeAllocs > 0 )
	{
		m_activeAllocations[ pool ][ memClass ][ bucketIndex ] = activeAllocs-1;
	}
}

Uint32 MetricsHistogramData::GetBucketCount() const
{
	return k_bucketCount;
}

void MetricsHistogramData::GetBucketAllocationRange( Uint32 bucketIndex, Uint64& minSize, Uint64& maxSize ) const
{
	RED_MEMORY_ASSERT( bucketIndex < k_bucketCount, "Bad bucket index" );
	if( bucketIndex == 0 )
	{
		minSize = 0;
		maxSize = CalculateBucketMaxSize( 0 );
	}
	else
	{
		minSize = CalculateBucketMaxSize( bucketIndex - 1 );
		maxSize = CalculateBucketMaxSize( bucketIndex );
	}
}

Uint32 MetricsHistogramData::GetAllocationsInBucket( PoolLabel pool, MemoryClass memClass, Uint32 bucketIndex ) const
{
	RED_MEMORY_ASSERT( bucketIndex < k_bucketCount, "Bad bucket index" );
	RED_MEMORY_ASSERT( pool < k_MaximumPools, "Bad pool index" );
	RED_MEMORY_ASSERT( memClass < k_MaximumMemoryClasses, "Bad memory class index" );

	return m_activeAllocations[ pool ][ memClass ][ bucketIndex ];
}

void MetricsHistogramData::PopulatePoolHistogram( PoolLabel pool, Red::System::Uint32 (&buckets)[ k_bucketCount ] ) const
{
	Red::System::MemorySet( buckets, 0, sizeof( buckets ) );
	for( Red::System::Uint16 memClass = 0; memClass < k_MaximumMemoryClasses; ++memClass )
	{
		for( Red::System::Uint32 bucket = 0; bucket < k_bucketCount; ++bucket )
		{
			buckets[ bucket ] += m_activeAllocations[ pool ][ memClass ][ bucket ];
		}
	}
}

//#pragma optimize( "", on )

} }