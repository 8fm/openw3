/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "reporter.h"
#include "assert.h"
#include "log.h"
#include "poolRegistry.h"
#include "systemAllocator.h"
#include "allocatorMetrics.h"
#include "defaultAllocator.h"

namespace red
{
namespace memory
{
#ifdef RED_MEMORY_ENABLE_LOGGING
	const u32 c_tlsfHistogramWidth = 80;
#endif

	Reporter::Reporter()
	{
		std::memset( &m_parameter, 0, sizeof( m_parameter ) );
	}

	Reporter::~Reporter()
	{}

	void Reporter::Initialize( const ReporterParameter & param )
	{
		m_parameter = param;

		RED_MEMORY_ASSERT( m_parameter.poolRegistry, "Pool Registry can't be null." );
		RED_MEMORY_ASSERT( m_parameter.metricsRegistry, "Metrics Registry can't be null." );
		RED_MEMORY_ASSERT( m_parameter.systemAllocator, "System Allocator can't be null." );
	}

#ifdef RED_MEMORY_ENABLE_LOGGING

	void OutputTLFHistogram( const TLSFAllocatorMetrics & tlsfMetrics, u32 maxGraphWidth )
	{
		u64 maxBlockCount = 0;
		u64 maxTotalAllocSize = 0;
		u32 lastIndexToProcess = 0;
		const auto & blockMetrics = tlsfMetrics.freeBlocksMetrics;
		for( u32 index = 0; index != blockMetrics.max_size(); ++index )
		{
			const auto & blockMetric = blockMetrics[ index ]; 
			maxBlockCount = std::max( maxBlockCount, blockMetric.totalCount );
			maxTotalAllocSize = std::max( maxTotalAllocSize, blockMetric.totalSize );
			lastIndexToProcess = blockMetric.totalSize != 0 ? index : lastIndexToProcess;
		}

		const float totalSizeStep = maxTotalAllocSize / static_cast< float >( maxGraphWidth );
		const float blockCountStep = maxBlockCount / static_cast< float >( maxGraphWidth );
		
		RED_MEMORY_LOG( "Free block Histogram. X -> Total Size, * -> Block Count." );

		for( u32 index = 0; index != lastIndexToProcess + 1; ++index )
		{
			const auto & blockMetric = blockMetrics[ index ]; 

			if( blockMetric.totalCount )
			{
				char sizeGraphBlock[ 128 ] = { 0 };
				char countGraphBlock[ 128 ] = { 0 };
				u32 sizeMarkerCount = static_cast< u32 >( blockMetric.totalSize / totalSizeStep );
				u32 countMarkerCount = static_cast< u32 >( blockMetric.totalCount / blockCountStep );

				std::memset( sizeGraphBlock, 'X', sizeMarkerCount );
				std::memset( countGraphBlock, '*', countMarkerCount );

				Red::System::SNPrintF( sizeGraphBlock + sizeMarkerCount,  128 - sizeMarkerCount, " %lld", blockMetric.totalSize );
				Red::System::SNPrintF( countGraphBlock + countMarkerCount, 128 - countMarkerCount, " %lld", blockMetric.totalCount );

				RED_MEMORY_LOG( "<%15lld |%s", blockMetric.blockSize, sizeGraphBlock );
				RED_MEMORY_LOG( " %15s |%s", " ", countGraphBlock );
			}
		}
	}

	void OutputLocklessSlabAllocatorTable( const LocklessSlabAllocatorMetrics & locklessSlabAllocatorMetrics )
	{
		const char * headerPattern = "| %10s | %10s | %10s | %10s | %9s |";
		const char * rowPattern = "| %10d | %10d | %10d | %10d | %8.2f |";

		RED_MEMORY_LOG( "Thread Cache Informations" );
		RED_MEMORY_LOG( "================================================================" );		
		RED_MEMORY_LOG( headerPattern, "Thread ID", "Used", "Reserved", "Waste", "%% Waste" );
		RED_MEMORY_LOG( "================================================================" );		

		const auto & threadsMetric = locklessSlabAllocatorMetrics.threadCacheInfo;

		for( u32 index = 0; index != c_maxThreadCount; ++index )
		{
			const auto & threadMetric = threadsMetric[ index ];
			if( threadMetric.threadId )
			{
				const auto & localInfo = threadMetric.slabAllocatorInfo;
				const u64 used = localInfo.metrics.consumedMemoryBytes;
				const u64 reserved = localInfo.metrics.consumedSystemMemoryBytes;
				const u64 waste = reserved - used;
				const double percentWaste = reserved ? ( waste / static_cast< double >( reserved ) ) * 100.0 : 0.0;

				RED_MEMORY_LOG( 
					rowPattern, 
					threadMetric.threadId, 
					used,
					reserved,
					waste,
					percentWaste );
			}
		}

		RED_MEMORY_LOG( "================================================================" );		
	}

	void WriteDefaultAllocatorReport()
	{
		DefaultAllocatorMetrics metrics;
		Memzero( &metrics, sizeof( metrics ) );

		AcquireDefaultAllocator().BuildMetrics( metrics );

		const AllocatorMetrics & defaultAllocatorMetric = metrics.metrics;

		RED_MEMORY_LOG( "Default Allocator Info" );
		RED_MEMORY_LOG( "\tSystem Memory Consumed: %lld", defaultAllocatorMetric.consumedSystemMemoryBytes );
		RED_MEMORY_LOG( "\tBookKeeping overhead: %lld", defaultAllocatorMetric.bookKeepingBytes );

		RED_MEMORY_LOG( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" );

		const LocklessSlabAllocatorMetrics & locklessSlabAllocatorMetrics = metrics.locklessSlabAllocatorMetrics;

		RED_MEMORY_LOG( "Default Allocator Lockless SLAB Information:" );

		RED_MEMORY_LOG( "\tSystem Memory Consumed: %lld", locklessSlabAllocatorMetrics.metrics.consumedSystemMemoryBytes );
		RED_MEMORY_LOG( "\tMemory Consumed: %lld", locklessSlabAllocatorMetrics.metrics.consumedMemoryBytes );
		RED_MEMORY_LOG( "\tMemory Waste: %lld", locklessSlabAllocatorMetrics.waste );
		RED_MEMORY_LOG( "\tMemory Waste Percent: %.2f", locklessSlabAllocatorMetrics.wastePercent );

		RED_MEMORY_LOG( "\tBook Keeping overhead: %lld", locklessSlabAllocatorMetrics.metrics.bookKeepingBytes );
		RED_MEMORY_LOG( "\tSmallest Free Block: %lld", locklessSlabAllocatorMetrics.metrics.smallestBlockSize );
		RED_MEMORY_LOG( "\tLargest Free Block: %lld", locklessSlabAllocatorMetrics.metrics.largestBlockSize );
		RED_MEMORY_LOG( "\tUsed Chunks: %d", locklessSlabAllocatorMetrics.usedChunkCount );
		RED_MEMORY_LOG( "\tFree Chunks: %d", locklessSlabAllocatorMetrics.freeChunkCount );

		RED_MEMORY_LOG( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" );

		OutputLocklessSlabAllocatorTable( locklessSlabAllocatorMetrics );

		RED_MEMORY_LOG( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" );

		const TLSFAllocatorMetrics & tlsfMetrics = metrics.tlsfAllocatorMetrics;

		RED_MEMORY_LOG( "Default Allocator TLSF Information:" );

		RED_MEMORY_LOG( "\tSystem Memory Consumed: %lld", tlsfMetrics.metrics.consumedSystemMemoryBytes );
		RED_MEMORY_LOG( "\tBook Keeping overhead: %lld", tlsfMetrics.metrics.bookKeepingBytes );
		RED_MEMORY_LOG( "\tSmallest Free Block: %lld", tlsfMetrics.metrics.smallestBlockSize );
		RED_MEMORY_LOG( "\tLargest Free Block: %lld", tlsfMetrics.metrics.largestBlockSize );

		RED_MEMORY_LOG( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" );

		OutputTLFHistogram( tlsfMetrics, c_tlsfHistogramWidth );

		RED_MEMORY_LOG( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" );

		RED_MEMORY_LOG( "***********************************************************************" );

		RED_LOG_FLUSH();
	}

#endif

	void Reporter::WriteReportToLog() const
	{
#ifdef RED_MEMORY_ENABLE_LOGGING

		RED_MEMORY_ASSERT( m_parameter.poolRegistry, "Pool Registry can't be null. Initialize before using Reporter." );
		RED_MEMORY_ASSERT( m_parameter.metricsRegistry, "Metrics Registry can't be null. Initialize before using Reporter." );
		RED_MEMORY_ASSERT( m_parameter.systemAllocator, "System Allocator can't be null. Initialize before using Reporter." );

		// 1) System Allocators resume. 
		// 2) Pool resume. KB Allocated, KB budget, Alloc count. Graph check.
		// 3) Pool details and Worst offender.
		// 4) Allocators resume. Pools bound to it, KB Allocated, KB budget, Alloc count.
		// 5) Allocators details. 

		RED_MEMORY_LOG( "***********************************************************************" );

		RED_MEMORY_LOG( "MEMORY REPORT" );

		RED_MEMORY_LOG( "***********************************************************************" );

		m_parameter.systemAllocator->WriteReportToLog();

		RED_MEMORY_LOG( "***********************************************************************" );

		m_parameter.poolRegistry->WritePoolReportToLog( *m_parameter.metricsRegistry );

		RED_MEMORY_LOG( "***********************************************************************" );

		WriteDefaultAllocatorReport();

#endif	
	}
}
}
