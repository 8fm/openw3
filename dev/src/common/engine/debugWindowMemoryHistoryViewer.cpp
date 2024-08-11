/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RED_GUI
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

#include "debugWindowMemoryHistoryViewer.h"
#include "memoryClassDebugColourPalette.h"
#include "../core/memoryAdapter.h"

CDebugWindowMemoryHistoryViewer::CDebugWindowMemoryHistoryViewer( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
	: CRedGuiGraphBase( left, top, width, height )
	, m_displayMode( Display_Classes )
{
	SetXAxisMaximum( (Float)k_historyBufferSize );
	SetYAxisMaximum( 1.0f );
	SetXAxisPadding( 20 );
	SetYAxisPadding( 20 );
	SetKeyWidth( 200 );
}

CDebugWindowMemoryHistoryViewer::~CDebugWindowMemoryHistoryViewer()
{

}

void CDebugWindowMemoryHistoryViewer::ResetAll()
{
	Red::System::MemorySet( m_history, 0, sizeof( m_history ) );
}

RED_INLINE void CDebugWindowMemoryHistoryViewer::PushAllocations( MemoryClassRingBuffer& target, Uint64 allocatedBytes )
{
	target.m_hasAllocations |= allocatedBytes > 0;
	target.m_allocatedBytes[ m_headIndex ] = allocatedBytes;
}

RED_INLINE Uint64 CDebugWindowMemoryHistoryViewer::GetPreviousValue( const MemoryClassRingBuffer& target, Uint32 framesBack )
{
	// For speed, we are not testing count, be careful!
	Int32 wrappedIndex = (Int32)m_headIndex - (Int32)framesBack;
	if( wrappedIndex < 0 )
	{
		return target.m_allocatedBytes[ k_historyBufferSize + wrappedIndex ];
	}
	else
	{
		return target.m_allocatedBytes[ m_headIndex - framesBack ];
	}
}

void CDebugWindowMemoryHistoryViewer::RefreshData()
{
	if( m_memoryManager != nullptr )
	{
		Red::MemoryFramework::RuntimePoolMetrics poolMetrics;
		if( m_poolLabel == (Red::MemoryFramework::PoolLabel)-1 )
		{
			m_memoryManager->GetMetricsCollector().PopulateAllMetrics( poolMetrics );
		}
		else
		{
			poolMetrics = m_memoryManager->GetMetricsCollector().GetMetricsForPool( m_poolLabel );
		}

		if( m_displayMode == Display_Classes )
		{
			for( Uint32 memClass = 0; memClass < Red::MemoryFramework::k_MaximumMemoryClasses; ++memClass )
			{
				PushAllocations( m_history[ memClass ], poolMetrics.m_allocatedBytesPerMemoryClass[ memClass ] );
			}
		}
		else
		{
			for( Uint32 groupIndex = 0; groupIndex < m_memoryManager->GetMetricsCollector().GetMemoryClassGroupCount(); ++groupIndex )
			{
				Uint32 memClassCount = m_memoryManager->GetMetricsCollector().GetMemoryClassCountInGroup( groupIndex );
				Int64 bytesTotal = 0;
				for( Uint32 classIndex = 0; classIndex < memClassCount; ++classIndex )
				{
					Red::MemoryFramework::MemoryClass memClass = m_memoryManager->GetMetricsCollector().GetMemoryClassInGroup( groupIndex, classIndex );
					bytesTotal += poolMetrics.m_allocatedBytesPerMemoryClass[ memClass ];
				}
				PushAllocations( m_history[ groupIndex ], bytesTotal );
			}
		}

		m_headIndex++;
		if( m_headIndex >= k_historyBufferSize )
		{
			m_headIndex = 0;		// Faster than doing % k_historyBufferSize
		}
		m_count += m_count < k_historyBufferSize ? 1 : 0;
	}
}

void CDebugWindowMemoryHistoryViewer::DrawGraph( const Vector2& origin, const Vector2& dimensions )
{
	RefreshData();
	UpdateKey();

	Float xOffset = 0.0f;
	Float xScale = MCeil( dimensions.X / k_historyBufferSize );
	for( Uint32 xIndex = m_count - 1; xIndex > 0; --xIndex )
	{
		Float memClassOffsetY = 0.0f;
		for( Uint32 memClass = Red::MemoryFramework::k_MaximumMemoryClasses; memClass > 0; --memClass )		// Run backwards so order is consistent with key
		{
			if( m_history[ memClass-1 ].m_hasAllocations )
			{
				Uint64 thisValue = GetPreviousValue( m_history[ memClass-1 ], xIndex );
				Vector2 thisPoint = CalculateGraphScreenOffset( Vector2( (Float)xIndex, (Float)thisValue ), dimensions );
				thisPoint.Y = MFloor( thisPoint.Y );
				if( thisPoint.Y >= 1.0f )
				{
					GetTheme()->DrawRawFilledRectangle( origin + Vector2( xOffset, dimensions.Y - thisPoint.Y + m_xAxisPadding - memClassOffsetY ), Vector2( xScale, thisPoint.Y ), GenerateMemoryClassColour( memClass-1 ) );
					m_history[ memClass-1 ].m_hasDrawn = true;
				}
				else
				{
					m_history[ memClass-1 ].m_hasDrawn = false;
				}
				memClassOffsetY += thisPoint.Y;
			}
		}
		xOffset += xScale;
	}
}

void CDebugWindowMemoryHistoryViewer::UpdateKey()
{
	m_graphKeys.ClearFast();
	if( m_memoryManager != nullptr )
	{
		if( m_displayMode == Display_Classes )
		{
			for( Uint32 memClass = 0; memClass < Red::MemoryFramework::k_MaximumMemoryClasses; ++memClass )
			{
				if( m_history[ memClass ].m_hasAllocations && m_history[ memClass ].m_hasDrawn )
				{
					AddKey( ANSI_TO_UNICODE( m_memoryManager->GetMemoryClassName( memClass ) ), GenerateMemoryClassColour( memClass ) );
				}
			}
		}
		else 
		{
			for( Uint32 groupIndex = 0; groupIndex < m_memoryManager->GetMetricsCollector().GetMemoryClassGroupCount(); ++groupIndex )
			{
				if( m_history[ groupIndex ].m_hasAllocations && m_history[ groupIndex ].m_hasDrawn )
				{
					AddKey( ANSI_TO_UNICODE( m_memoryManager->GetMetricsCollector().GetMemoryClassGroupName( groupIndex ) ), GenerateMemoryClassColour( groupIndex ) );
				}
			}
		}
	}
}

void CDebugWindowMemoryHistoryViewer::SetParameters( Memory::Adapter* manager, Red::MemoryFramework::PoolLabel poolLabel )
{
	m_memoryManager = manager;
	m_poolLabel = poolLabel;
	ResetAll();

	// Update y-axis as we have a new pool
	if( manager != nullptr )
	{
		if( m_poolLabel != -1 )
		{
			Red::MemoryFramework::AllocatorInfo allocatorInfo;
			if( manager->PoolExist( m_poolLabel ) )
			{
				manager->RequestAllocatorInfo( m_poolLabel, allocatorInfo );
				SetYAxisMaximum( (Float)allocatorInfo.GetBudget() );	
			}
		}
		else
		{
			Red::System::MemSize budget = 0;
			for( Uint16 memPoolIndex = 0; memPoolIndex < manager->GetRegisteredPoolCount(); ++memPoolIndex )
			{
				Red::MemoryFramework::AllocatorInfo poolInfo;
				Red::MemoryFramework::PoolLabel theLabel = manager->GetPoolLabelForIndex( memPoolIndex );
				if( manager->PoolExist( theLabel ) )
				{
					budget += manager->GetPoolBudget( theLabel );	
				}
			}
			SetYAxisMaximum( (Float)budget );	
		}
	}
}

#endif
#endif