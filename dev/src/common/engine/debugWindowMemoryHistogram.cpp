/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugWindowMemoryHistogram.h"

#if !defined( NO_RED_GUI ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

#include "../core/memoryAdapter.h"

CDebugWindowMemoryHistogram::CDebugWindowMemoryHistogram( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
	: CRedGuiGraphBase( left, top, width, height )
	, m_poolLabel( 0 )
	, m_memoryClass( 0 )
	, m_memoryManager( nullptr )
{
	SetXAxisMaximum( 32.0f );		// for 32 buckets
	SetYAxisMaximum( 1.0f );		// 0 - 1 for easy scaling
	SetXAxisPadding( 20 );
	SetYAxisPadding( 100 );
	SetKeyWidth( 200 );
}

CDebugWindowMemoryHistogram::~CDebugWindowMemoryHistogram()
{

}

void CDebugWindowMemoryHistogram::SetParameters( Memory::Adapter* manager, Red::MemoryFramework::PoolLabel pool, Red::MemoryFramework::MemoryClass memClass )
{
	m_memoryManager = manager;
	m_poolLabel = pool;
	m_memoryClass = memClass;
}

RED_INLINE String CDebugWindowMemoryHistogram::GetBucketKeyString( Uint32 bucket )
{
	const Red::MemoryFramework::MetricsHistogramData& histogramData = m_memoryManager->GetMetricsCollector().GetHistogramData();
	Uint64 minSize = 0, maxSize = 0;
	histogramData.GetBucketAllocationRange( bucket, minSize, maxSize );
	
	// Special key for last bucket
	if( bucket == histogramData.GetBucketCount() - 1 )
	{
		return String::Printf( TXT( "%.0f Mb +" ), minSize / ( 1024.0f * 1024.0f ) );
	}

	if( minSize < 1024 )
	{
		return String::Printf( TXT( "%d - %d bytes" ), minSize, maxSize );
	}
	else if( minSize < ( 1024 * 1024 ) )
	{
		return String::Printf( TXT( "%.0f - %.0f Kb" ), minSize / 1024.0f, maxSize / 1024.0f );
	}
	else
	{
		return String::Printf( TXT( "%.0f - %.0f Mb" ), minSize / ( 1024.0f * 1024.0f ), maxSize / ( 1024.0f * 1024.0f ) );
	}
}

RED_INLINE Color CDebugWindowMemoryHistogram::CalculateBucketColour( Uint32 bucket )
{
	Uint32 red = 64 + ( bucket % 5 ) * 47;
	Uint32 green = 64 + ( bucket % 8 ) * 27;
	Uint32 blue = 64 + ( bucket % 2 ) * 191;

	return Color( (Uint8)red, (Uint8)green, (Uint8)blue );
}

void CDebugWindowMemoryHistogram::UpdateKey()
{
	const Red::MemoryFramework::MetricsHistogramData& histogramData = m_memoryManager->GetMetricsCollector().GetHistogramData();
	Uint32 bucketCount = histogramData.GetBucketCount();
	m_graphKeys.ClearFast();
	m_graphKeys.Reserve( bucketCount );

	for( Uint32 b = 0; b < bucketCount; ++b )
	{
		AddKey( GetBucketKeyString( b ), CalculateBucketColour( b ) );
	}
}

void CDebugWindowMemoryHistogram::DrawGraph( const Vector2& origin, const Vector2& dimensions )
{
	if( !m_memoryManager )
	{
		m_graphKeys.ClearFast();
		return;
	}

	UpdateKey();

	const Red::MemoryFramework::MetricsHistogramData& histogramData = m_memoryManager->GetMetricsCollector().GetHistogramData();
	Uint32 bucketCount = histogramData.GetBucketCount();
	Uint32 maxAllocationsInBucket = 0;
	Uint32 totalPoolBuckets[ Red::MemoryFramework::MetricsHistogramData::k_bucketCount ] = {0};

	// First pass, calculate max 
	if( m_memoryClass == -1 )
	{
		histogramData.PopulatePoolHistogram( m_poolLabel, totalPoolBuckets );
		for( Uint32 b = 0; b < bucketCount; ++b )
		{
			maxAllocationsInBucket = Red::Math::NumericalUtils::Max( totalPoolBuckets[ b ], maxAllocationsInBucket );
		}
	}
	else
	{	
		for( Uint32 b = 0; b < bucketCount; ++b )
		{
			maxAllocationsInBucket = Red::Math::NumericalUtils::Max( histogramData.GetAllocationsInBucket( m_poolLabel, m_memoryClass, b ), maxAllocationsInBucket );
		}
	}

	m_yAxisLabels.ClearFast();
	AddYAxisLabel( String::Printf( TXT( "Max: %d" ), maxAllocationsInBucket ), 0.95f );

	Float singleBarWidth = dimensions.X / bucketCount;

	// Draw it
	for( Uint32 b = 0; b < bucketCount; ++b )
	{
		Uint32 allocations = m_memoryClass != -1 ? histogramData.GetAllocationsInBucket( m_poolLabel, m_memoryClass, b ) : totalPoolBuckets[ b ];
		Float allocFraction = (Float)allocations / (Float)maxAllocationsInBucket;
		Vector2 screenCoords = CalculateGraphScreenOffset( Vector2( (Float)b, allocFraction ), dimensions );
		GetTheme()->DrawRawFilledRectangle( origin + Vector2( screenCoords.X, dimensions.Y - screenCoords.Y + m_xAxisPadding ), Vector2( singleBarWidth, screenCoords.Y ), CalculateBucketColour( b ) );
	}
}

#endif