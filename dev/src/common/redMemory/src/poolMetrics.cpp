/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "poolMetrics.h"

namespace red
{
namespace memory
{
	void UpdatePoolAllocateMetrics( PoolMetrics & metrics, u64 size )
	{
		atomic::ExchangeAdd64( &metrics.bytesAllocated, size );
#ifdef RED_MEMORY_ENABLE_EXTENDED_METRICS
		atomic::ExchangeAdd64( &metrics.bytesAllocatedPerFrame, size );
		atomic::Increment32( &metrics.allocationCount );
		atomic::Increment32( &metrics.allocationPerFrame );

		metrics.bytesAllocatedPeak = std::max( metrics.bytesAllocatedPeak, metrics.bytesAllocated );
		metrics.bytesAllocatedPerFramePeak = std::max( metrics.bytesAllocatedPerFramePeak, metrics.bytesAllocatedPerFrame );
		metrics.allocationCountPeak = std::max( metrics.allocationCountPeak, metrics.allocationCount );
		metrics.allocationPerFramePeak = std::max( metrics.allocationPerFramePeak, metrics.allocationPerFrame );
#endif
	}

	void UpdatePoolDeallocateMetrics( PoolMetrics  & metrics, u64 size )
	{
		atomic::ExchangeAdd64( &metrics.bytesAllocated, -static_cast< i64 >( size ) );
#ifdef RED_MEMORY_ENABLE_EXTENDED_METRICS	
		atomic::ExchangeAdd64( &metrics.bytesDeallocatedPerFrame, size );
		atomic::Decrement32( &metrics.allocationCount );
		atomic::Increment32( &metrics.deallocationPerFrame );

		metrics.bytesDeallocatedPerFramePeak = std::max( metrics.bytesDeallocatedPerFramePeak, metrics.bytesDeallocatedPerFrame );
		metrics.deallocationPerFramePeak = std::max( metrics.deallocationPerFramePeak, metrics.deallocationPerFrame );
#endif
	}

	void UpdatePoolReallocateMetrics( PoolMetrics  & metrics, u64 inputSize, u64 outputSize )
	{		
#ifdef RED_MEMORY_ENABLE_EXTENDED_METRICS	
		if( inputSize )
		{
			UpdatePoolDeallocateMetrics( metrics, inputSize );
		}

		if( outputSize )
		{
			UpdatePoolAllocateMetrics( metrics, outputSize );
		}
#else
		const i64 size = outputSize - inputSize;
		atomic::ExchangeAdd64( &metrics.bytesAllocated, size );
#endif
	}

}
}
