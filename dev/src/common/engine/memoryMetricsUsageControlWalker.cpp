/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "memoryMetricsUsageControlWalker.h"

namespace DebugWindows {

MemoryMetricsUsageControlWalker::MemoryMetricsUsageControlWalker()
:	m_totalAreaSize( 0 )
,	m_zoom( 1 )
{

}

MemoryMetricsUsageControlWalker::~MemoryMetricsUsageControlWalker()
{

}

void MemoryMetricsUsageControlWalker::AddArea( Red::MemoryFramework::IAllocator* poolAllocator, MemUint address, MemSize size )
{
	if( m_areas.PushBackUnique( Area( poolAllocator, address, size ) ) )
	{
		m_totalAreaSize += size;
	}
}

void MemoryMetricsUsageControlWalker::SetResolution( Uint32 x, Uint32 y )
{
	m_width = x;
	m_height = y;

	const Uint32 numBlocks = m_width * m_height;

	m_blocks.Resize( numBlocks );
}

void MemoryMetricsUsageControlWalker::Clear()
{
	m_blocks.ClearFast();
	m_areas.ClearFast();

	m_totalAreaSize = 0;
}

RED_INLINE Uint32 MemoryMetricsUsageControlWalker::CalculateBytesPerBlock() const
{
	Float totalAreaSize = m_totalAreaSize / static_cast< Float >( m_zoom );

	return SafeDivide( totalAreaSize, m_width * m_height );
}

void MemoryMetricsUsageControlWalker::Walk()
{
	RED_FATAL_ASSERT( m_totalAreaSize > 0, "Call AddArea first!" );
	RED_FATAL_ASSERT( !m_blocks.Empty(), "Call SetResolution first!" );

	m_bytesPerBlock = CalculateBytesPerBlock();

	Red::System::MemoryZero( m_blocks.Data(), m_blocks.DataSize() );
	Red::System::MemoryZero( m_metrics.Data(), m_metrics.DataSize() );

	m_largestFreeArea = 0;

	m_walkData.m_totalSizeUsage = 0;
	m_walkData.m_metricsCount = 0;
	m_walkData.m_nextFreeMetricsBlock = 0;

	m_walkData.m_blockIndexShift = 0;

	const Uint32 count = m_areas.Size();
	for( Uint32 i = 0; i < count; ++i )
	{
		const Area& area = m_areas[ i ];

		m_walkData.m_currentBlockUsage = 0;
		m_walkData.m_previousBlockTouched = 0;
		m_walkData.m_areaIndex = i;
		m_walkData.m_zoomOnThisArea = false;

		if( m_zoom > 1 && area.m_address <= m_zoomAddress && area.m_address + area.m_size > m_zoomAddress )
		{
			m_walkData.m_blockIndexShift = BlockIndexFromAddress( m_zoomAddress );
			m_walkData.m_zoomOnThisArea = true;
		}

		area.m_allocator->WalkPoolArea( area.m_address, area.m_size, this );
	}

	if( m_metrics.Size() < m_walkData.m_metricsCount )
	{
		m_metrics.Resize( m_walkData.m_metricsCount );
	}
}

void MemoryMetricsUsageControlWalker::OnAreaCommon( MemUint address, MemSize size, std::function< void( BlockMetrics& ) > callback )
{
	const Area& area = m_areas[ m_walkData.m_areaIndex ];

	if( m_walkData.m_zoomOnThisArea )
	{
		if( address + size < m_zoomAddress )
		{
			return;
		}
		else if( address < m_zoomAddress )
		{
			size -= ( m_zoomAddress - address );
			address = m_zoomAddress;
		}
	}

	Uint32 blockIndex = BlockIndexFromAddress( address );

	if( m_walkData.m_previousBlockTouched != blockIndex )
	{
		m_walkData.m_previousBlockTouched = blockIndex;
		m_walkData.m_currentBlockUsage = 0;
	}

	++m_walkData.m_metricsCount;
	m_walkData.m_totalSizeUsage += size;

	// This exists so that we always have something valid to write into
	static BlockMetrics dummyMetrics;
	Red::System::MemoryZero( &dummyMetrics, sizeof( BlockMetrics ) );

	Uint32 metricsBlockIndex = m_walkData.m_nextFreeMetricsBlock++;
	BlockMetrics& metrics = ( metricsBlockIndex < m_metrics.Size() )? m_metrics[ metricsBlockIndex ] : dummyMetrics;
	metrics.size = static_cast< Uint32 >( size );

	if( callback )
	{
		callback( metrics );
	}

	while( size > 0 )
	{
		if( blockIndex >= m_blocks.Size() )
		{
			return;
		}

		Block& block = m_blocks[ blockIndex ];

		if( !block.address )
		{
			block.address = address;
		}

		if( block.metricsStart == 0 )
		{
			block.metricsStart = metricsBlockIndex;
		}

		++block.metricsCount;

		MemSize spaceRemainingInBlock = m_bytesPerBlock - m_walkData.m_currentBlockUsage;

		MemSize sizeToAddToBlock = Red::Math::NumericalUtils::Min( spaceRemainingInBlock, size );

		m_walkData.m_currentBlockUsage += sizeToAddToBlock;

		size -= sizeToAddToBlock;

		if( size > 0 )
		{
			RED_FATAL_ASSERT( m_walkData.m_currentBlockUsage == m_bytesPerBlock, "Miscalulation" );

			m_walkData.m_currentBlockUsage = 0;
			m_walkData.m_previousBlockTouched = blockIndex;
			++blockIndex;
		}
	}
}

void MemoryMetricsUsageControlWalker::OnUsedArea( MemUint address, MemSize size, Uint16 memoryClass )
{
	OnAreaCommon
	(
		address,
		size,
		[ memoryClass ]( BlockMetrics& block )
		{
			block.memoryClass = memoryClass;
			block.used = true;
		}
	);
}

void MemoryMetricsUsageControlWalker::OnFreeArea( MemUint address, MemSize size )
{
	OnAreaCommon
	(
		address,
		size,
		[]( BlockMetrics& block )
		{
			block.free = true;
		}
	);

	if( m_largestFreeArea < size )
	{
		m_largestFreeArea = size;
	}
}

void MemoryMetricsUsageControlWalker::OnLockedArea( MemUint address, MemSize size )
{
	OnAreaCommon
	(
		address,
		size,
		[]( BlockMetrics& block )
		{
			block.locked = true;
		}
	);
}

Uint32 MemoryMetricsUsageControlWalker::BlockIndexFromAddress( MemUint address ) const
{
	const Area& parentArea = m_areas[ m_walkData.m_areaIndex ];

	// Block index relative to area
	MemUint addressOffset = address - parentArea.m_address;
	Uint32 startBlockOffset = SafeDivide( addressOffset, m_bytesPerBlock );

	// Count blocks in all preceding areas
	Uint32 areaFirstBlockIndex = 0;
	for( Uint32 i = 0; i < m_walkData.m_areaIndex; ++i )
	{
		const Area& area = m_areas[ i ];

		areaFirstBlockIndex += static_cast< Uint32 >( area.m_size / m_bytesPerBlock );
	}

	return areaFirstBlockIndex + startBlockOffset - m_walkData.m_blockIndexShift;
}

void MemoryMetricsUsageControlWalker::ZoomIn( MemUint address )
{
	m_zoomAddress = address;
	m_zoom *= 2;
}

void MemoryMetricsUsageControlWalker::ZoomOut()
{
	m_zoom /= 2;

	if( m_zoom < 1 )
	{
		m_zoom = 1;
	}
}

} // namespace DebugWindows {
