/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugWindowMemoryClassPanel.h"
#include "memoryClassDebugColourPalette.h"
#include "redGuiTab.h"
#include "redGuiList.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

enum ESelectedTabIndices
{
	SelectedTab_AllGroups,
	SelectedTab_AllClasses,
	SelectedTab_ByGroup
};

namespace DebugWindows
{
	CDebugWindowMemoryClassPanel::CDebugWindowMemoryClassPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiPanel(left, top, width, height)
		, m_tabs( nullptr )
		, m_list( nullptr )
		, m_lastMetricsCollector( nullptr )
	{
		m_topPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 40 );
		m_topPanel->SetDock( RedGui::DOCK_Top );
		AddChild( m_topPanel );

		m_list = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_list->SetDock( RedGui::DOCK_Fill );
		m_list->SetSorting( true );
		m_list->AppendColumn( TXT("Memory class"), 200 );
		m_list->AppendColumn( TXT("Size [MB]"), 200, RedGui::SA_Real );
		m_list->AppendColumn( TXT("Allocs"), 200, RedGui::SA_Integer );
		m_list->AppendColumn( TXT("Peak [MB]"), 200, RedGui::SA_Real );
		AddChild( m_list );
	}

	CDebugWindowMemoryClassPanel::~CDebugWindowMemoryClassPanel()
	{
	}

	void CDebugWindowMemoryClassPanel::Refresh( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo )
	{
		RefreshTabs( metricsCollector );
		RefreshActiveTab( poolMetrics, metricsCollector );

		m_lastMetricsCollector = &metricsCollector;
	}

	void CDebugWindowMemoryClassPanel::Reset()
	{
		m_list->RemoveAllItems();
		if( m_tabs != nullptr )
		{
			m_tabs->Dispose();
			m_tabs = nullptr;
		}
	}

	void CDebugWindowMemoryClassPanel::RefreshActiveTab( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector )
	{
		switch( m_tabs->GetActiveTabIndex() )
		{
		case SelectedTab_AllGroups:
			RefreshAllGroupsToList( poolMetrics, metricsCollector );
			break;
		case SelectedTab_AllClasses:
			RefreshAllClassesToList( poolMetrics, metricsCollector );
			break;
		default:
			RefreshOneGroupToList( m_tabs->GetActiveTabIndex() - SelectedTab_ByGroup, poolMetrics, metricsCollector );
			break;
		}
	}

	void CDebugWindowMemoryClassPanel::NotifySelectedTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* control )
	{
		RED_UNUSED( eventPackage );
		RED_UNUSED( control );
		m_list->RemoveAllItems();
	}

	void CDebugWindowMemoryClassPanel::RefreshAllClassesToList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector )
	{
		for( Uint32 memClassIndex = 0; memClassIndex < Red::MemoryFramework::k_MaximumMemoryClasses; ++memClassIndex )
		{
			Color memClassDiffColour( 255, 255, 255 );

			AnsiChar memClassName[64] = {'\0'};
			Int64 bytesAllocated = poolMetrics.m_allocatedBytesPerMemoryClass[ memClassIndex ];
			Int64 activeAllocations = poolMetrics.m_allocationsPerMemoryClass[ memClassIndex ];
			Int64 bytesAllocatedPeak = poolMetrics.m_allocatedBytesPerMemoryClassPeak[ memClassIndex ];

			if( bytesAllocated > 0 || activeAllocations > 0 || bytesAllocatedPeak > 0 )
			{
				metricsCollector.GetMemoryClassName( memClassIndex, memClassName, 64 );
				String name = ANSI_TO_UNICODE( memClassName );
				Int32 index = m_list->Find( name );
				if( index == -1 )
				{
					Color color = GenerateMemoryClassColour( memClassIndex );
					index = m_list->AddItem( name, color );
				}
				m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocated / ( 1024.0f * 1024.0f ) ), index, 1 );
				m_list->SetItemText( String::Printf( TXT("%lld"), activeAllocations ), index, 2 );
				m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocatedPeak / ( 1024.0f * 1024.0f ) ), index, 3 );
			}
		}
	}

	void CDebugWindowMemoryClassPanel::RefreshOneGroupToList( Uint32 groupIndex, const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector )
	{
		Uint32 groupCount = metricsCollector.GetMemoryClassGroupCount();
		Uint32 classesInGroup = metricsCollector.GetMemoryClassCountInGroup( groupIndex );
		RED_ASSERT( groupIndex < groupCount, TXT( "Bad group index" ) );
		
		AnsiChar memClassName[64] = {'\0'};
		for( Uint32 c = 0; c < classesInGroup; ++c )
		{
			Red::MemoryFramework::MemoryClass memClass = metricsCollector.GetMemoryClassInGroup( groupIndex, c );
			Int64 bytesAllocated = poolMetrics.m_allocatedBytesPerMemoryClass[ memClass ];
			Int64 activeAllocations = poolMetrics.m_allocationsPerMemoryClass[ memClass ];
			Int64 bytesAllocatedPeak = poolMetrics.m_allocatedBytesPerMemoryClassPeak[ memClass ];

			metricsCollector.GetMemoryClassName( memClass, memClassName, 64 );
			String name = ANSI_TO_UNICODE( memClassName );
			Int32 index = m_list->Find( name );
			if( index == -1 )
			{
				Color color = GenerateMemoryClassColour( memClass );
				index = m_list->AddItem( name, color );
			}
			m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocated / ( 1024.0f * 1024.0f ) ), index, 1 );
			m_list->SetItemText( String::Printf( TXT("%lld"), activeAllocations ), index, 2 );
			m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocatedPeak / ( 1024.0f * 1024.0f ) ), index, 3 );
		}
	}

	void CDebugWindowMemoryClassPanel::RefreshAllGroupsToList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector )
	{
		Uint32 groupCount = metricsCollector.GetMemoryClassGroupCount();
		for( Uint32 g=0; g<groupCount; ++g )
		{
			String groupName( ANSI_TO_UNICODE( metricsCollector.GetMemoryClassGroupName( g ) ) );
			Int64 bytesAllocated = 0;
			Int64 activeAllocations = 0;
			Int64 bytesAllocatedPeak = 0;
			Uint32 classCount = metricsCollector.GetMemoryClassCountInGroup( g );
			for( Uint32 c = 0; c < classCount; ++c )
			{
				Red::MemoryFramework::MemoryClass memClass = metricsCollector.GetMemoryClassInGroup( g, c );
				bytesAllocated += poolMetrics.m_allocatedBytesPerMemoryClass[ memClass ];
				activeAllocations += poolMetrics.m_allocationsPerMemoryClass[ memClass ];
				bytesAllocatedPeak += poolMetrics.m_allocatedBytesPerMemoryClassPeak[ memClass ];
			}

			Int32 index = m_list->Find( groupName );
			if( index == -1 )
			{
				index = m_list->AddItem( groupName, Color::WHITE );
			}

			m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocated / ( 1024.0f * 1024.0f ) ), index, 1 );
			m_list->SetItemText( String::Printf( TXT("%lld"), activeAllocations ), index, 2 );
			m_list->SetItemText( String::Printf( TXT("%1.5f"), bytesAllocatedPeak / ( 1024.0f * 1024.0f ) ), index, 3 );
		}
	}

	void CDebugWindowMemoryClassPanel::RefreshTabs( const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector )
	{
		if( m_lastMetricsCollector != &metricsCollector || !m_tabs )
		{
			if( m_tabs )
			{
				m_tabs->Dispose();
			}

			m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
			m_tabs->SetDock( RedGui::DOCK_Top );
			m_tabs->AddTab( TXT( "All Groups" ) );
			m_tabs->AddTab( TXT( "All Classes" ) );
			m_tabs->SetActiveTab( SelectedTab_AllGroups );
			m_tabs->EventTabChanged.Bind( this, &CDebugWindowMemoryClassPanel::NotifySelectedTab );
			m_topPanel->AddChild( m_tabs );
			m_topPanel->SetOutOfDate();

			Uint32 groupCount = metricsCollector.GetMemoryClassGroupCount();
			for( Uint32 g=0; g<groupCount; ++g )
			{
				const AnsiChar* groupName = metricsCollector.GetMemoryClassGroupName( g );
				m_tabs->AddTab( ANSI_TO_UNICODE( groupName ) );
			}
		}
	}
}

#endif
#endif
#endif