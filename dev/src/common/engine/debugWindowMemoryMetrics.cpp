/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

#include "../redIO/redIO.h"
#include "../core/version.h"
#include "../gpuApiUtils/gpuApiInterface.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../redMemoryFramework/redMemoryAllocatorInfo.h"
#include "../core/depot.h"
#include "../core/garbageCollector.h"

#include "redGuiTab.h"
#include "redGuiPanel.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiLabel.h"
#include "redGuiButton.h"
#include "redGuiScrollPanel.h"
#include "redGuiGridLayout.h"
#include "redGuiTextBox.h"
#include "redGuiComboBox.h"
#include "redGuiManager.h"
#include "redGuiOccupancyChart.h"
#include "redGuiAreaChart.h"
#include "redGuiSaveFileDialog.h"
#include "memoryMetricsPoolAreaControl.h"
#include "memoryMetricsMemoryUsageControl.h"
#include "debugWindowMemoryMetrics.h"
#include "debugWindowMemoryHistogram.h"
#include "debugWindowMemoryHistoryViewer.h"
#include "debugWindowMemoryBudgets.h"
#include "debugWindowMemoryClassPanel.h"
#include "memoryClassDebugColourPalette.h"
#include "..\core\2darray.h"
#include "../core/memoryAdapter.h"

namespace
{
	const Uint16 GAllPoolsLabelID = 0;
	const Uint16 GStaticPoolLabelID = 1;
	const Uint16 GOverflowPoolLabelID = 2;
	const Uint16 GPoolLabelOffset = 3;
}

namespace DebugWindows
{
	CDebugWindowMemoryMetrics::CDebugWindowMemoryMetrics()
		: RedGui::CRedGuiWindow( 200, 200, 1000, 600 )
		, m_selectedManager( nullptr )
		, m_activePoolChart( nullptr )
		, m_isDumping( false )
		, m_graphType( View_Groups )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnTick );
		SetCaption( TXT("Memory metrics") );

		CreateControls();
	}

	CDebugWindowMemoryMetrics::~CDebugWindowMemoryMetrics()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowMemoryMetrics::NotifyOnTick );
	}

	void CDebugWindowMemoryMetrics::CreateControls()
	{
		m_dumpSaveDialog = new RedGui::CRedGuiSaveFileDialog();
		m_dumpSaveDialog->SetDefaultFileName( TXT("MemoryStats") );
		m_dumpSaveDialog->AddFilter( TXT("Text file"), TXT("txt") );
		m_dumpSaveDialog->EventFileOK.Bind( this, &CDebugWindowMemoryMetrics::NotifyDumpStatsFileOK );

		m_trackingSaveDialog = new RedGui::CRedGuiSaveFileDialog();
		m_trackingSaveDialog->SetDefaultFileName( TXT("MemoryDump") );
		m_trackingSaveDialog->AddFilter( TXT("Red Memory Metrics"), TXT("rmm") );
		m_trackingSaveDialog->EventFileOK.Bind( this, &CDebugWindowMemoryMetrics::NotifyDumpTrackingFileOK );

		RedGui::CRedGuiGridLayout* menuPanel = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
		menuPanel->SetDock( RedGui::DOCK_Top );
		menuPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		menuPanel->SetDimensions( 6, 1 );
		AddChild( menuPanel );
		{
			RedGui::CRedGuiPanel* leftPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
			leftPanel->SetBorderVisible( false );
			leftPanel->SetBackgroundColor( Color::CLEAR );
			menuPanel->AddChild( leftPanel );
			{
				RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
				label->SetMargin( Box2( 3, 6, 3, 3 ) );
				label->SetText( TXT("Memory manager: ") );
				label->SetDock( RedGui::DOCK_Left );
				leftPanel->AddChild( label );

				m_managerComboBox = new RedGui::CRedGuiComboBox( 0, 0, 100, 20 );
				m_managerComboBox->SetMargin( Box2( 5, 3, 13, 3 ) );
				m_managerComboBox->SetDock( RedGui::DOCK_Fill );
				m_managerComboBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryMetrics::NotifySelectedManager );
				leftPanel->AddChild( m_managerComboBox );
			}

			RedGui::CRedGuiPanel* rightPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
			rightPanel->SetBorderVisible( false );
			rightPanel->SetBackgroundColor( Color::CLEAR );
			menuPanel->AddChild( rightPanel );
			{
				RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
				label->SetMargin( Box2( 13, 6, 3, 3 ) );
				label->SetText( TXT("Pool: ") );
				label->SetDock( RedGui::DOCK_Left );
				rightPanel->AddChild( label );

				m_poolComboBox = new RedGui::CRedGuiComboBox( 0, 0, 100, 20, 250 );
				m_poolComboBox->SetMargin( Box2( 5, 3, 3, 3 ) );
				m_poolComboBox->SetDock( RedGui::DOCK_Fill );
				m_poolComboBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryMetrics::NotifySelectedPool );
				rightPanel->AddChild( m_poolComboBox );
			}

			RedGui::CRedGuiButton* resetMetrics = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
			resetMetrics->SetText( TXT("Reset metrics") );
			resetMetrics->SetMargin( Box2( 15, 5, 10, 0 ) );
			resetMetrics->EventButtonClicked.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnClickedRestMetrics );
			menuPanel->AddChild( resetMetrics );

			RedGui::CRedGuiButton* dumpStats = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
			dumpStats->SetText( TXT("Dump Stats - CSV") );
			dumpStats->SetMargin( Box2( 15, 5, 10, 0 ) );
			dumpStats->EventButtonClicked.Bind( this, &CDebugWindowMemoryMetrics::NotifyDumpStatsFileOK );
			menuPanel->AddChild( dumpStats );

			RedGui::CRedGuiButton* dumpMetrics = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
			dumpMetrics->SetText( TXT("Continuous Dump") );
			dumpMetrics->SetMargin( Box2( 15, 5, 10, 0 ) );
			dumpMetrics->EventButtonClicked.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnClickedTrackMetrics );
			menuPanel->AddChild( dumpMetrics );
			m_dumpButton = dumpMetrics;

			RedGui::CRedGuiButton* dumpDebug = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
			dumpDebug->SetText( TXT("Debug Output") );
			dumpDebug->SetMargin( Box2( 15, 5, 10, 0 ) );
			dumpDebug->EventButtonClicked.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnClickedDebugOutput );
			menuPanel->AddChild( dumpDebug );
		}

		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		AddChild( m_tabs );
		{
			CreateMemoryClassesPanel();
			CreateMemoryUsageControls();
			CreateHeapAreaViewer();
			CreateLeakTrackerPanel();
			CreateGraphViewer();
			CreateBudgetsTab();
			CreateGCTab();
		}

		m_tabs->SetActiveTab( 0 );
	}

	void CDebugWindowMemoryMetrics::AddMemoryManager( Red::TUniquePtr< Memory::Adapter > manager, const String& name )
	{
		m_memoryManagers.PushBack( std::move( manager ) );
		m_managerComboBox->AddItem( name );
	}

	void CDebugWindowMemoryMetrics::NotifySelectedManager( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
	{
		RED_UNUSED( eventPackage );

		m_selectedManager = m_memoryManagers[selectedItem].Get();

		m_poolComboBox->ClearAllItems();
		m_poolComboBox->AddItem( TXT("All pools") );
		m_poolComboBox->AddItem( TXT("Statics") );
		m_poolComboBox->AddItem( TXT("Overflow") );

		// Display list of registered pools
		for( Uint16 i=0; i<m_selectedManager->GetMaximumPoolCount(); ++i )
		{
			AnsiChar poolName[ 64 ] = {'\0'};
			Red::MemoryFramework::PoolLabel selectedLabel = m_selectedManager->GetPoolLabelForIndex( i );
			if( m_selectedManager->GetPoolName( selectedLabel, poolName, 64 ) && m_selectedManager->PoolExist( selectedLabel ))
			{
				m_poolComboBox->AddItem( ANSI_TO_UNICODE( poolName ) );
			}
		}

		if( m_poolComboBox->GetItemCount() > 0 )
		{
			m_poolComboBox->SetSelectedIndex( 0 );
			RedGui::CRedGuiEventPackage eventPackage( m_poolComboBox );
			NotifySelectedPool( eventPackage, 0 );
		}
		else
		{
			m_historyViewer->SetParameters( m_selectedManager, (Red::MemoryFramework::PoolLabel)-1 );
			m_histogram->SetParameters( m_selectedManager, 0, (Red::MemoryFramework::MemoryClass)-1 );
			UpdateGraphViewerClassSelector();
		}
	}

	void CDebugWindowMemoryMetrics::NotifySelectedPool( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
	{
		RED_UNUSED( eventPackage );

		if( selectedItem < GPoolLabelOffset )
		{
			m_tabs->GetTabAt( 3 )->SetEnabled( false );
			m_selectedPoolLabel = (Red::MemoryFramework::PoolLabel)selectedItem;
			m_histogram->SetParameters( nullptr, 0, 0 );
			m_historyViewer->SetParameters( m_selectedManager, (Red::MemoryFramework::PoolLabel)-1 );
		}
		else
		{
			StringAnsi selectedPoolName = UNICODE_TO_ANSI( m_poolComboBox->GetSelectedItemName().AsChar() );

			// Don't trust the index (they can be out of order), test the names instead.
			for( Uint16 i=0; i<m_selectedManager->GetMaximumPoolCount(); ++i )
			{
				AnsiChar poolName[ 64 ] = {'\0'};
				Red::MemoryFramework::PoolLabel selectedLabel = m_selectedManager->GetPoolLabelForIndex( i );
				m_selectedManager->GetPoolName( selectedLabel, poolName, 64 );
				if( Red::System::StringCompare( poolName, selectedPoolName.AsChar() ) == 0 )
				{
					m_selectedPoolLabel = selectedLabel + GPoolLabelOffset;
					m_tabs->GetTabAt( 3 )->SetEnabled( true );
					m_histogram->SetParameters( m_selectedManager, m_selectedPoolLabel - GPoolLabelOffset, (Red::MemoryFramework::MemoryClass)-1 );
					m_historyViewer->SetParameters( m_selectedManager, m_selectedPoolLabel - GPoolLabelOffset );
					break;
				}
			}
		}

		UpdateGraphViewerClassSelector();

		ClearGUI();
	}

	void CDebugWindowMemoryMetrics::OnWindowOpened( CRedGuiControl* control )
	{
		m_memoryManagers.Clear();
		m_managerComboBox->ClearAllItems();

		m_memoryManagers.Reserve( 2 );

		// Register memory managers here!
		AddMemoryManager( Memory::CreateCPUAdapter(), TXT( "Core Memory" ) );
		AddMemoryManager( Memory::CreateGPUAdapter(), TXT( "GPU Memory" ) );

		if( m_managerComboBox->GetItemCount() > 0 )
		{
			m_managerComboBox->SetSelectedIndex( 0 );
			RedGui::CRedGuiEventPackage eventPackage( m_managerComboBox );
			NotifySelectedManager( eventPackage, 0 );
			m_isDumping = m_selectedManager->IsDumpingMetrics();
			if( m_isDumping )
			{
				m_dumpButton->SetText( TXT( "End Dump" ) );
			}
		}
	}

	void CDebugWindowMemoryMetrics::CreateGraphViewer()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Graphs") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			RedGui::CRedGuiPanel* topPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
			topPanel->SetDock( RedGui::DOCK_Top );
			topPanel->SetBorderVisible( true );
			{
				RedGui::CRedGuiComboBox* graphTypeSelector = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				graphTypeSelector->SetDock( RedGui::DOCK_Left );
				graphTypeSelector->AddItem( TXT( "Groups" ) );
				graphTypeSelector->AddItem( TXT( "Classes" ) );
				graphTypeSelector->AddItem( TXT( "Size Histogram" ) );
				graphTypeSelector->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryMetrics::NotifySelectedGraphType );
				graphTypeSelector->SetSelectedIndex( 0 );
				topPanel->AddChild( graphTypeSelector );

				m_graphClassSelector = new RedGui::CRedGuiComboBox( 0, 0, 200, 20, 300 );
				m_graphClassSelector->SetDock( RedGui::DOCK_Left );
				m_graphClassSelector->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryMetrics::NotifySelectedGraphClass );
				m_graphClassSelector->SetVisible( false );
				topPanel->AddChild( m_graphClassSelector );
			}
			tab->AddChild( topPanel );

			m_histogram = new CDebugWindowMemoryHistogram( 0, 0, 100, 100 );
			m_histogram->SetDock( RedGui::DOCK_Fill );
			m_histogram->SetBorderVisible( true );
			m_histogram->SetParameters( m_selectedManager, 0, (Red::MemoryFramework::MemoryClass)-1 );
			m_histogram->SetVisible( false );
			tab->AddChild( m_histogram );

			m_historyViewer = new CDebugWindowMemoryHistoryViewer( 0, 0, 100, 100 );
			m_historyViewer->SetDock( RedGui::DOCK_Fill );
			m_historyViewer->SetBorderVisible( true );
			m_historyViewer->SetParameters( m_selectedManager, (Red::MemoryFramework::PoolLabel)-1 );
			m_historyViewer->SetVisible( true );
			m_historyViewer->SetDisplayMode( CDebugWindowMemoryHistoryViewer::Display_Groups );
			tab->AddChild( m_historyViewer );
		}
		UpdateGraphViewerClassSelector();
	}

	void CDebugWindowMemoryMetrics::NotifySelectedGraphType( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
	{
		switch( selectedItem )
		{
		case View_Histogram:
			m_histogram->SetVisible( true );
			m_historyViewer->SetVisible( false );
			m_graphClassSelector->SetVisible( true );
			break;
		case View_Groups:
			m_histogram->SetVisible( false );
			m_historyViewer->SetVisible( true );
			m_graphClassSelector->SetVisible( false );
			m_historyViewer->SetDisplayMode( CDebugWindowMemoryHistoryViewer::Display_Groups );
			break;
		case View_Classes:
			m_histogram->SetVisible( false );
			m_historyViewer->SetVisible( true );
			m_graphClassSelector->SetVisible( false );
			m_historyViewer->SetDisplayMode( CDebugWindowMemoryHistoryViewer::Display_Classes );
			break;
		}
	}

	void CDebugWindowMemoryMetrics::NotifySelectedGraphClass( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
	{
		if( m_selectedPoolLabel >= GPoolLabelOffset )
		{
			// Selected item needs to be offset by 1
			Red::MemoryFramework::MemoryClass memClass = static_cast< Red::MemoryFramework::MemoryClass >( selectedItem ) - 1;
			m_histogram->SetParameters( m_selectedManager, m_selectedPoolLabel - GPoolLabelOffset, memClass );
		}
		else
		{
			m_histogram->SetParameters( nullptr, 0, 0 );
		}
	}

	void CDebugWindowMemoryMetrics::UpdateGraphViewerClassSelector()
	{
		m_graphClassSelector->ClearAllItems();
		if( m_selectedManager != nullptr )
		{
			// First entry = all classes
			m_graphClassSelector->AddItem( TXT( "All MemClasses" ) );

			for( Uint16 classIndex = 0; classIndex < Red::MemoryFramework::k_MaximumMemoryClasses; ++classIndex )
			{
				const AnsiChar* className = m_selectedManager->GetMemoryClassName( classIndex );
				if( className != nullptr )
				{
					m_graphClassSelector->AddItem( String( ANSI_TO_UNICODE( className ) ) );
				}
			}
		}
		m_graphClassSelector->SetSelectedIndex( 0 );
	}

	void CDebugWindowMemoryMetrics::CreateLeakTrackerPanel()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Leak tracker") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			layout->SetDock( RedGui::DOCK_Fill );
			layout->SetDimensions( 2, 1 );
			tab->AddChild( layout );

			RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 100, 100 );
			panel->SetDock( RedGui::DOCK_Fill );
			panel->SetBorderVisible( false );
			panel->SetMargin( Box2( 5, 5, 5, 5 ) );
			layout->AddChild( panel );
			{
				RedGui::CRedGuiButton* button = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
				button->SetDock( RedGui::DOCK_Bottom );
				button->SetText( TXT("Start tracking leaks") );
				button->SetMargin( Box2( 0, 5, 0, 0 ) );
				button->SetToggleMode( true );
				button->EventCheckedChanged.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnClickedTrackLeaks );
				panel->AddChild( button );

				m_leaksList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
				m_leaksList->SetDock( RedGui::DOCK_Fill );
				m_leaksList->SetSorting( true );
				m_leaksList->AppendColumn( TXT("Leak ID"), 150, RedGui::SA_Integer );
				m_leaksList->AppendColumn( TXT("Size [B]"), 150, RedGui::SA_Real );
				m_leaksList->AppendColumn( TXT("Memory class"), 200 );
				m_leaksList->EventSelectedItem.Bind( this, &CDebugWindowMemoryMetrics::NotifyOnSelectedLeak );
				panel->AddChild( m_leaksList );
			}

			m_callstackText = new RedGui::CRedGuiTextBox( 0, 0, 100, 100 );
			m_callstackText->SetDock( RedGui::DOCK_Fill );
			m_callstackText->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_callstackText->SetReadOnly( true );
			m_callstackText->SetMultiLine( true );
			layout->AddChild( m_callstackText );
		}
	}

	void CDebugWindowMemoryMetrics::NotifyOnClickedTrackLeaks( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		RedGui::CRedGuiButton* button = static_cast< RedGui::CRedGuiButton* >( sender );
		if( button != nullptr )
		{
			if( value == true )
			{
				button->SetText( TXT("Stop tracking leaks") );
				button->SetTextColor( Color::LIGHT_GREEN );

				StartTrackingLeaks();
			}
			else
			{
				button->SetText( TXT("Start tracking leaks") );
				button->SetTextColor( Color::WHITE );

				StopTrackingLeaks();
			}
		}
	}

	void CDebugWindowMemoryMetrics::StartTrackingLeaks()
	{
		m_leaksList->RemoveAllItems();
		m_callstackText->SetText( TXT("") );

		// ctremblay do not work, too late to fix, and we never ever used it anyway.
		//SGarbageCollector::GetInstance().CollectNow();
		//m_selectedManager->GetMetricsCollector().BeginTrackingLeaks( m_selectedPoolLabel - GPoolLabelOffset );
	}

	void CDebugWindowMemoryMetrics::StopTrackingLeaks()
	{
		// Before we end tracking, kick out a garbage collection pass to ensure anything that will be GC'd in the near future will not turn up as a leak!
		// Since garbage collection can allocate memory (logging, etc), disable allocation tracking before doing it
		
		// ctremblay do not work, too late to fix, and we never ever used it anyway.
		//m_selectedManager->GetMetricsCollector().GetMemoryLeakTracker().DisableAllocationTracking();
		//SGarbageCollector::GetInstance().CollectNow();
		//m_selectedManager->GetMetricsCollector().EndTrackingLeaks();

		//PopulateTrackingLeaksResults();
	}

	void CDebugWindowMemoryMetrics::NotifyOnSelectedLeak( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		Red::MemoryFramework::TrackedAllocation* leak = static_cast< Red::MemoryFramework::TrackedAllocation* >( m_leaksList->GetItemUserData( value ) );
		if( leak != nullptr )
		{
			// Build the callstack
			Red::MemoryFramework::MetricsCallstack& callstack = leak->m_callStack;
			String fullCallstack = String::EMPTY;

			Char csText[512] = {'\0'};
			for( Int32 stackIndex = 0; stackIndex < callstack.GetCallstackDepth(); ++stackIndex )
			{
				callstack.GetAsString( stackIndex, csText, 512 );
				fullCallstack += csText;
				fullCallstack += TXT("\n");
			}

			m_callstackText->SetText( fullCallstack );
		}
	}

	void CDebugWindowMemoryMetrics::CreateMemoryClassesPanel()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Memory classes") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 100, 100 );
			panel->SetDock( RedGui::DOCK_Top );
			panel->SetMargin( Box2( 5, 5, 5, 5 ) );
			panel->SetBackgroundColor( Color( 20, 20, 20, 255 ) );
			tab->AddChild( panel );
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 4, 4 );
				panel->AddChild( layout );
				{
					// internal function to create labels
					static class 
					{
					public:
						void operator()( RedGui::CRedGuiControl* parent, RedGui::CRedGuiLabel** label ) const
						{
							( *label ) = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
							parent->AddChild( ( *label ) );
						}
					} CreateLabel;


					CreateLabel( layout, &m_allocatorTypeLabel );
					CreateLabel( layout, &m_allocatorBudgetLabel );
					CreateLabel( layout, &m_allocatorOverheadLabel );

					// add fake label for correct fill layout
					RedGui::CRedGuiLabel* unusedLabel = nullptr;
					CreateLabel( layout, &unusedLabel );
					
					CreateLabel( layout, &m_totalAllocatedLabel );
					CreateLabel( layout, &m_activeAllocationsLabel );
					CreateLabel( layout, &m_totalAllocatedPeekLabel );
					CreateLabel( layout, &m_activeAllocationsPeekLabel );
					CreateLabel( layout, &m_allocatedPerFrameLabel );
					CreateLabel( layout, &m_allocationsPerFrameLabel );
					CreateLabel( layout, &m_allocatedPerFramePeekLabel );
					CreateLabel( layout, &m_allocationsPerFramePeekLabel );
					CreateLabel( layout, &m_deallocatedPerFrameLabel );
					CreateLabel( layout, &m_deallocationsPerFrameLabel );
					CreateLabel( layout, &m_deallocatedPerFramePeekLabel );
					CreateLabel( layout, &m_deallocationsPerFramePeekLabel );
				}
			}

			m_memoryClassChart = new RedGui::CRedGuiOccupancyChart( 0, 0, 100, 25 );
			m_memoryClassChart->SetDock( RedGui::DOCK_Top );
			m_memoryClassChart->SetLegendVisible( false );
			m_memoryClassChart->SetCheckCorrectPercent( false );
			m_memoryClassChart->SetMargin( Box2( 5, 1, 5, 1 ) );
			m_memoryClassChart->SetBorderVisible( false );
			tab->AddChild( m_memoryClassChart );

			m_memoryClassesAreaChart = new RedGui::CRedGuiAreaChart( 0, 0, 100, 200 );
			m_memoryClassesAreaChart->SetDock( RedGui::DOCK_Bottom );
			m_memoryClassesAreaChart->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_memoryClassesAreaChart->SetBackgroundColor( Color::CLEAR );
			tab->AddChild( m_memoryClassesAreaChart );

			m_memoryClassListControl = new CDebugWindowMemoryClassPanel( 0, 0, 100, 100 );
			m_memoryClassListControl->SetDock( RedGui::DOCK_Fill );
			m_memoryClassListControl->SetMargin( Box2( 5, 5, 5, 5 ) );
			tab->AddChild( m_memoryClassListControl );
		}
	}

	void CDebugWindowMemoryMetrics::UpdateMemoryClassesList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo )
	{
		for( Uint32 memClassIndex = 0; memClassIndex < Red::MemoryFramework::k_MaximumMemoryClasses; ++memClassIndex )
		{
			Color memClassDiffColour( 255, 255, 255 );
			Int32 diffIndex = 1;

			AnsiChar memClassName[64] = {'\0'};
			Int64 bytesAllocated = poolMetrics.m_allocatedBytesPerMemoryClass[ memClassIndex ];
			Int64 activeAllocations = poolMetrics.m_allocationsPerMemoryClass[ memClassIndex ];
			Int64 bytesAllocatedPeak = poolMetrics.m_allocatedBytesPerMemoryClassPeak[ memClassIndex ];
			if( bytesAllocated > 0 || activeAllocations > 0 || bytesAllocatedPeak > 0 )
			{
				metricsCollector.GetMemoryClassName( memClassIndex, memClassName, 64 );
				String name = ANSI_TO_UNICODE( memClassName );

				Float onePercent = 100.0f / (Float)(allocInfo.GetBudget());
				Int64 memClassMemUsed = poolMetrics.m_allocatedBytesPerMemoryClass[ memClassIndex ];
				Float memClassSizePc = ( (Float)memClassMemUsed * onePercent ) / 100.0f;
				Color color = GenerateMemoryClassColour( memClassIndex );
				if( !m_memoryClassChart->UpdateData( name, memClassSizePc ) )
				{
					m_memoryClassChart->AddData( name, color, 0.0f );
				}
			}
		}

		m_memoryClassListControl->Refresh( poolMetrics, metricsCollector, allocInfo );
	}

	void CDebugWindowMemoryMetrics::UpdateMemClassPreviousValues( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics )
	{
		for( Uint32 i=0; i< Red::MemoryFramework::k_MaximumMemoryClasses; ++i )
		{
			m_previousMemoryClassSizes[ i ] = poolMetrics.m_allocatedBytesPerMemoryClass[ i ];
		}
	}

	void CDebugWindowMemoryMetrics::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		RED_UNUSED( eventPackage );

		if( GetVisible() == false )
		{
			return;
		}

		if( m_tabs->GetActiveTabIndex() == 0 )
		{
			Red::MemoryFramework::AllocationMetricsCollector& metricsCollector = m_selectedManager->GetMetricsCollector();
			Red::MemoryFramework::RuntimePoolMetrics poolMetrics;
			Red::MemoryFramework::AllocatorInfo allocatorInfo;
			Bool updateGUI = false;

			if( m_poolComboBox->GetSelectedIndex() == GAllPoolsLabelID )
			{
				allocatorInfo.SetAllocatorTypeName( TXT( "All" ) );
				allocatorInfo.SetPerAllocationOverhead( 0 );		// Its different for every pool ,the best we can do is ignore it for 'all pools'
				metricsCollector.PopulateAllMetrics( poolMetrics );

				// Calculate total budget
				Red::System::MemSize budget = 0;
				for( Uint16 memPoolIndex = 0; memPoolIndex < m_selectedManager->GetRegisteredPoolCount(); ++memPoolIndex )
				{
					Red::MemoryFramework::AllocatorInfo poolInfo;
					Red::MemoryFramework::PoolLabel theLabel = m_selectedManager->GetPoolLabelForIndex( memPoolIndex ); 
					if( m_selectedManager->PoolExist( theLabel ) )
					{
						budget += m_selectedManager->GetPoolBudget( theLabel );	
					}
				}
				allocatorInfo.SetAllocatorBudget( budget );
				updateGUI = true;
			}
			else if( m_poolComboBox->GetSelectedIndex() == GStaticPoolLabelID )
			{
				poolMetrics = metricsCollector.GetMetricsForStaticPool();
				Red::MemoryFramework::IAllocator*  theAllocator = m_selectedManager->GetStaticPool();
				if( theAllocator != nullptr )
				{
					theAllocator->RequestAllocatorInfo( allocatorInfo );
					updateGUI = true;
				}
			}
			else if( m_poolComboBox->GetSelectedIndex() == GOverflowPoolLabelID )
			{
				poolMetrics = metricsCollector.GetMetricsForOverflowPool();
				Red::MemoryFramework::IAllocator*  theAllocator = m_selectedManager->GetOverflowPool();
				if( theAllocator != nullptr )
				{
					theAllocator->RequestAllocatorInfo( allocatorInfo );
					updateGUI = true;
				}
			}
			else
			{
				poolMetrics = metricsCollector.GetMetricsForPool( m_selectedPoolLabel - GPoolLabelOffset );
				if( m_selectedManager->PoolExist( m_selectedPoolLabel - GPoolLabelOffset ) )
				{
					m_selectedManager->RequestAllocatorInfo( m_selectedPoolLabel - GPoolLabelOffset, allocatorInfo );
					updateGUI = true;
				}
			}

			if( updateGUI == true )
			{
				UpdateMemoryClassesList( poolMetrics, metricsCollector, allocatorInfo );
				UpdateMemoryClassesLabels( poolMetrics, metricsCollector, allocatorInfo );
				UpdateMemClassPreviousValues( poolMetrics );
				m_memoryClassesAreaChart->Update( poolMetrics.m_totalBytesAllocated, allocatorInfo.GetBudget() );
			}
			else
			{
				ResetMemoryClassesLabels();
				m_memoryClassesAreaChart->Reset();
			}
		}
		else if( m_tabs->GetActiveTabIndex() == 1 )
		{
			UpdateMemoryUsageControls();
		}
		else if( m_tabs->GetActiveTabIndex() == 2 )
		{
			UpdateHeapAreaViewer();
		}
	}

	String CDebugWindowMemoryMetrics::GetFormattedSize( Red::System::Int64 memSize )
	{
		const Red::System::Int64 oneKb = 1024;
		const Red::System::Int64 oneMb = 1024 * 1024;
		const Red::System::Int64 oneGb = 1024 * 1024 * 1024;

		if( memSize < oneKb )
		{
			return String::Printf( TXT( "%lld Bytes" ), memSize );
		}
		else if( memSize < oneMb )
		{
			Float sizeAsFloat = (Float)memSize / (Float)oneKb;
			return String::Printf( TXT( "%.1f Kb" ), sizeAsFloat );
		}
		else if( memSize < oneGb )
		{
			Float sizeAsFloat = (Float)memSize / (Float)oneMb;
			return String::Printf( TXT( "%.2f Mb" ), sizeAsFloat );
		}
		else
		{
			Float sizeAsFloat = (Float)memSize / (Float)oneGb;
			return String::Printf( TXT( "%.2f Gb" ), sizeAsFloat );
		}

		return String::EMPTY;
	}

	void CDebugWindowMemoryMetrics::UpdateMemoryClassesLabels( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo )
	{
		m_allocatorTypeLabel->SetText( String::Printf( TXT( "Allocator type: %ls" ), allocInfo.GetTypeName() ) );
		m_allocatorBudgetLabel->SetText( String::Printf( TXT( "Allocator budget: %ls" ), GetFormattedSize( ( Red::System::Int64 )allocInfo.GetBudget() ).AsChar() ) );
		m_allocatorOverheadLabel->SetText( String::Printf( TXT( "Allocator overhead: %ls" ), GetFormattedSize( allocInfo.GetPerAllocationOverhead() * poolMetrics.m_totalAllocations ).AsChar() ) );
		m_totalAllocatedLabel->SetText( String::Printf( TXT( "Total allocated: %ls" ), GetFormattedSize( poolMetrics.m_totalBytesAllocated ).AsChar() ) );
		m_activeAllocationsLabel->SetText( String::Printf( TXT( "Active allocations: %lld" ), poolMetrics.m_totalAllocations ) );
		m_totalAllocatedPeekLabel->SetText( String::Printf( TXT( "Total allocated peek: %ls" ), GetFormattedSize( poolMetrics.m_totalBytesAllocatedPeak ).AsChar() ) );
		m_activeAllocationsPeekLabel->SetText( String::Printf( TXT( "Active allocation peek: %lld" ), poolMetrics.m_totalAllocationsPeak ) );
		m_allocatedPerFrameLabel->SetText( String::Printf( TXT( "Allocated per frame: %ls" ), GetFormattedSize( poolMetrics.m_bytesAllocatedPerFrame ).AsChar() ) );
		m_allocationsPerFrameLabel->SetText( String::Printf( TXT( "Allocation per frame: %lld" ), poolMetrics.m_allocationsPerFrame ) );
		m_allocatedPerFramePeekLabel->SetText( String::Printf( TXT( "Allocated per frame peek: %ls" ), GetFormattedSize( poolMetrics.m_allocationsPerFramePeak ).AsChar() ) );
		m_allocationsPerFramePeekLabel->SetText( String::Printf( TXT( "Allocations per frame peek: %lld" ), poolMetrics.m_allocationsPerFramePeak ) );
		m_deallocatedPerFrameLabel->SetText( String::Printf( TXT( "Deallocated per frame: %ls" ), GetFormattedSize( poolMetrics.m_bytesDeallocatedPerFrame ).AsChar() ) );
		m_deallocationsPerFrameLabel->SetText( String::Printf( TXT( "Deallocations per frame: %lld" ), poolMetrics.m_deallocationsPerFrame ) );
		m_deallocatedPerFramePeekLabel->SetText( String::Printf( TXT( "Deallocated per frame peek: %ls" ), GetFormattedSize( poolMetrics.m_bytesDeallocatedPerFramePeak ).AsChar() ) );
		m_deallocationsPerFramePeekLabel->SetText( String::Printf( TXT( "Deallocation per frame peek: %lld" ), poolMetrics.m_deallocationsPerFramePeak ) );
	}

	void CDebugWindowMemoryMetrics::ClearGUI()
	{
		m_memoryClassListControl->Reset();
		m_leaksList->RemoveAllItems();
		m_callstackText->SetText( TXT("") );
		m_memoryClassChart->ClearData();
		m_memoryClassesAreaChart->Reset();

#ifndef NO_EDITOR
		// clear heap viewers
		const Uint32 viewerCount = m_heapViewers.Size();
		for( Uint32 i=0; i<viewerCount; ++i )
		{
			m_panelForHeapViewers->RemoveChild( m_heapViewers[i] );
			m_heapViewers[i]->Dispose();
		}
		m_heapViewers.Clear();
#endif

		// clear memory usge
		m_memoryUsage->ResetMemoryPool();
	}

	void CDebugWindowMemoryMetrics::ResetMemoryClassesLabels()
	{
		m_allocatorTypeLabel ->SetText( TXT( "Allocator type: ") );
		m_allocatorBudgetLabel ->SetText( TXT( "Allocator budget: ") );
		m_allocatorOverheadLabel->SetText( TXT( "Allocator overhead: " ) );
		m_totalAllocatedLabel ->SetText( TXT( "Total allocated: ") );
		m_activeAllocationsLabel ->SetText( TXT( "Active allocations: ") );
		m_totalAllocatedPeekLabel ->SetText( TXT( "Total allocated peek: ") );
		m_activeAllocationsPeekLabel ->SetText( TXT( "Active allocation peek: ") );
		m_allocatedPerFrameLabel ->SetText( TXT( "Allocated per frame: ") );
		m_allocationsPerFrameLabel ->SetText( TXT( "Allocation per frame: ") );
		m_allocatedPerFramePeekLabel ->SetText( TXT( "Allocated per frame peek: ") );
		m_allocationsPerFramePeekLabel ->SetText( TXT( "Allocations per frame peek: ") );
		m_deallocatedPerFrameLabel ->SetText( TXT( "Deallocated per frame: ") );
		m_deallocationsPerFrameLabel ->SetText( TXT( "Deallocations per frame: ") );
		m_deallocatedPerFramePeekLabel ->SetText( TXT( "Deallocated per frame peek: ") );
		m_deallocationsPerFramePeekLabel ->SetText( TXT( "Deallocation per frame peek: ") );
	}

	void CDebugWindowMemoryMetrics::NotifyOnClickedRestMetrics( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		m_selectedManager->ResetMetrics();
	}

	void CDebugWindowMemoryMetrics::CreateHeapAreaViewer()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Heap area viewer") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			m_panelForHeapViewers = new RedGui::CRedGuiScrollPanel( 0, 0, 100, 100 );
			m_panelForHeapViewers->SetDock( RedGui::DOCK_Fill );
			m_panelForHeapViewers->SetMargin( Box2( 5, 5, 5, 5 ) );
			tab->AddChild( m_panelForHeapViewers );
		}
	}

	void CDebugWindowMemoryMetrics::OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size )
	{
		/*if( m_tabs->GetActiveTabIndex() == 1 )
		{
			m_memoryUsage->SetMemory( GetSelectedAllocator(), address, size );
		}
		else if( m_tabs->GetActiveTabIndex() == 2 )
		{
#ifndef NO_EDITOR
			const Uint32 areaCount = m_heapViewers.Size();
			for( Uint32 i=0; i<areaCount; ++i )
			{
				if( address == m_heapViewers[i]->GetMemoryAddress() )
				{
					m_heapViewers[i]->SetMemory( GetSelectedAllocator(), address, size );
					return;
				}
			}

			// else
			CMemoryMetricsPoolAreaControl* newPoolChart = new CMemoryMetricsPoolAreaControl( 0, 0, 100, 40 );
			newPoolChart->SetDock( RedGui::DOCK_Top );
			newPoolChart->SetMargin( Box2( 5, 5, 5, 5 ) );
			newPoolChart->SetMemory( GetSelectedAllocator(), address, size );
			m_panelForHeapViewers->AddChild( newPoolChart );
			m_heapViewers.PushBack( newPoolChart );
#endif
		}*/
	}

	void CDebugWindowMemoryMetrics::UpdateHeapAreaViewer()
	{
		/*Red::MemoryFramework::IAllocator* allocator = GetSelectedAllocator();
		if( allocator != nullptr )
		{
			allocator->WalkAllocator( this );
		}*/
	}

	void CDebugWindowMemoryMetrics::CreateMemoryUsageControls()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Usage view") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			m_memoryUsage = new CMemoryMetricsUsageControl( 0, 0, 100, 100 );
			m_memoryUsage->SetDock( RedGui::DOCK_Fill );
			m_memoryUsage->SetMargin( Box2( 5, 5, 5, 5 ) );
			tab->AddChild( m_memoryUsage );
		}
	}

	void CDebugWindowMemoryMetrics::UpdateMemoryUsageControls()
	{
		/*Red::MemoryFramework::IAllocator* allocator = GetSelectedAllocator();
		if( allocator != nullptr )
		{
			allocator->WalkAllocator( this );
			m_memoryUsage->Update();
		}*/
	}

	void CDebugWindowMemoryMetrics::NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		m_dumpSaveDialog->SetVisible( true );
	}

	void CDebugWindowMemoryMetrics::NotifyOnClickedDebugOutput( RedGui::CRedGuiEventPackage& eventPackage )
	{
		/*if( GetSelectedAllocator() )
		{
			GetSelectedAllocator()->DumpDebugOutput();
		}*/
	}

	void CDebugWindowMemoryMetrics::NotifyOnClickedTrackMetrics( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		if( m_isDumping )
		{
			if( m_selectedManager != nullptr )
			{
				SGarbageCollector::GetInstance().CollectNow();
				m_selectedManager->EndMetricsDump();
			}
			m_isDumping = false;
			m_dumpButton->SetText( TXT( "Continuous Dump" ) );
		}
		else
		{
			m_trackingSaveDialog->SetVisible( true );
		}
	}

	void CDebugWindowMemoryMetrics::NotifyDumpTrackingFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		String path = String::EMPTY;
		GDepot->GetAbsolutePath( path );
		path += m_trackingSaveDialog->GetFileName();

		m_selectedManager->BeginMetricsDump( path.AsChar() );
		m_isDumping = true;

		m_dumpButton->SetText( TXT( "End Dump" ) );
	}

	class MemoryStatsDumper
	{
	public:
		MemoryStatsDumper( const String& fileName )
		{
			m_dumpFile.Open( fileName.AsChar(), Red::IO::eOpenFlag_WriteNew );
		}
		~MemoryStatsDumper()
		{
			m_dumpFile.Flush();
			m_dumpFile.Close();
		}
		void Log( const Char* txt )
		{
			Uint32 writtenSize = 0;
			const Char* c_newLine = TXT( "\n" );
			m_dumpFile.Write( txt, static_cast< Uint32 >( Red::System::StringLength( txt ) * sizeof( Char ) ), writtenSize );
			m_dumpFile.Write( c_newLine, static_cast< Uint32 >( Red::System::StringLength( c_newLine ) * sizeof( Char ) ), writtenSize );
		}
	private:
		Red::IO::CNativeFileHandle m_dumpFile;
	};

	void CDebugWindowMemoryMetrics::AppendFileStats( const String& filename )
	{
		String directoryToWriteTo;
#ifdef RED_PLATFORM_DURANGO
		directoryToWriteTo = TXT("d:\\");
#else
		CDirectory* direct = GDepot->FindPath( TXT("engine\\") );
		directoryToWriteTo = direct->GetAbsolutePath();
#endif

		String outputtxt;
		Int32 numg = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupCount();
		if( !GFileManager->FileExist( directoryToWriteTo + filename + TXT( ".csv" ) ) )
		{
			AnsiChar buf[256];
			outputtxt += TXT( "Group," );
			
			for( Int32 i=0;i<numg;++i )
			{				
				const AnsiChar* gnam = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupName( i );
				Int32 numc = m_selectedManager->GetMetricsCollector().GetMemoryClassCountInGroup( i );
				for( Int32 j=0;j<numc;++j )
				{
					outputtxt += ANSI_TO_UNICODE( gnam );
					outputtxt += TXT( "," );
				}
			}
			outputtxt += TXT( "\nMemory Class," );
			for( Int32 i=0;i<numg;++i )
			{				
				const AnsiChar* gnam = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupName( i );
				Int32 numc = m_selectedManager->GetMetricsCollector().GetMemoryClassCountInGroup( i );
				for( Int32 j=0;j<numc;++j )
				{
					Red::MemoryFramework::MemoryClass mem = m_selectedManager->GetMetricsCollector().GetMemoryClassInGroup( i, j );
					m_selectedManager->GetMetricsCollector().GetMemoryClassName( mem, buf, 256 );
					outputtxt += ANSI_TO_UNICODE( buf );
					outputtxt += TXT( "," );
				}
			}
			outputtxt += TXT("\n");
		}
		outputtxt += TXT("Memory (MB),");
		for( Int32 i=0;i<numg;++i )
		{				
			const AnsiChar* gnam = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupName( i );
			Int32 numc = m_selectedManager->GetMetricsCollector().GetMemoryClassCountInGroup( i );
			Float sum_mb = 0.0f;
			for( Int32 j=0;j<numc;++j )
			{
				Red::MemoryFramework::MemoryClass mem = m_selectedManager->GetMetricsCollector().GetMemoryClassInGroup( i, j );
				Int64 totalbytesallocatedforclass = m_selectedManager->GetMetricsCollector().GetTotalBytesAllocatedForClass( mem );
				outputtxt += String::Printf( TXT("%f,"), ( totalbytesallocatedforclass / ( 1024.0f * 1024.0f ) ) );
			}
		}
		outputtxt += TXT("\n");
		GFileManager->SaveStringToFile( directoryToWriteTo + filename + TXT( ".csv" ), outputtxt, true );
	}

	void CDebugWindowMemoryMetrics::NotifyDumpStatsFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );
		if( m_selectedManager )
		{
			// why not use 2d arrays and save functionality? well want to run this in release game...
			String filename = String(TXT("memory_metrics"));
			String filenameCollapsed = String(TXT("memory_metrics_collapsed"));	
			AppendFileStats( TXT( "memory_metrics_timelapse" ) );

			String ext = String(TXT(".csv"));
						
			String bigAndNastyStringCSV = String::EMPTY;
			String bigAndNastyStringCSV_collapsed = String::EMPTY;

			Int32 i;
			Int32 j;

			bigAndNastyStringCSV = TXT("Group;");
			bigAndNastyStringCSV += TXT("Class in Group;");
			bigAndNastyStringCSV += TXT("Memory (MB)\n");			

			bigAndNastyStringCSV_collapsed = TXT("Group;");			
			bigAndNastyStringCSV_collapsed += TXT("Memory (MB)\n");	

			Int32 numg = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupCount();
			Int64 totalbytesallocated = m_selectedManager->GetMetricsCollector().GetTotalBytesAllocated();
			AnsiChar buf[256];			

			for( i=0;i<numg;++i )
			{				
				const AnsiChar* gnam = m_selectedManager->GetMetricsCollector().GetMemoryClassGroupName( i );
				Int32 numc = m_selectedManager->GetMetricsCollector().GetMemoryClassCountInGroup( i );
				Float sum_mb = 0.0f;

				for( j=0;j<numc;++j )
				{
					Red::MemoryFramework::MemoryClass mem = m_selectedManager->GetMetricsCollector().GetMemoryClassInGroup( i, j );
					m_selectedManager->GetMetricsCollector().GetMemoryClassName( mem, buf, 256 );
					Int64 totalbytesallocatedforclass = m_selectedManager->GetMetricsCollector().GetTotalBytesAllocatedForClass( mem );
					Float mb = Float(totalbytesallocatedforclass)/Float(1024*1024);
					sum_mb += mb;

					bigAndNastyStringCSV += ANSI_TO_UNICODE(gnam);
					bigAndNastyStringCSV += TXT(";");

					bigAndNastyStringCSV += ANSI_TO_UNICODE(buf);
					bigAndNastyStringCSV += TXT(";");					

					bigAndNastyStringCSV += String::Printf( TXT("%f\n"), mb );				
				}
				
				bigAndNastyStringCSV_collapsed += ANSI_TO_UNICODE(gnam);
				bigAndNastyStringCSV_collapsed += TXT(";");					

				bigAndNastyStringCSV_collapsed += String::Printf( TXT("%f\n"), sum_mb );
			}

			String directoryToWriteTo;
#ifdef RED_PLATFORM_DURANGO
			directoryToWriteTo = TXT("d:\\");
#else
			CDirectory* direct = GDepot->FindPath( TXT("engine\\") );
			directoryToWriteTo = direct->GetAbsolutePath();
#endif
			GFileManager->SaveStringToFile( directoryToWriteTo + filename + ext, bigAndNastyStringCSV );
			GFileManager->SaveStringToFile( directoryToWriteTo + filenameCollapsed + ext, bigAndNastyStringCSV_collapsed );
		}
	}

	void CDebugWindowMemoryMetrics::CreateGCTab()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("GC") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			RedGui::CRedGuiButton* bigAssGcButton = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
			bigAssGcButton->SetText( TXT("Collect Now!") );
			bigAssGcButton->SetMargin( Box2( 50, 50, 50, 50 ) );
			bigAssGcButton->SetDock( RedGui::DOCK_Fill );
			bigAssGcButton->EventButtonClicked.Bind( this, &CDebugWindowMemoryMetrics::NotifyRunGcNow );
			tab->AddChild( bigAssGcButton );
		}
	}

	void CDebugWindowMemoryMetrics::NotifyRunGcNow( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SGarbageCollector::GetInstance().CollectNow();
	}

	void CDebugWindowMemoryMetrics::CreateBudgetsTab()
	{
		Uint32 tabIndex = m_tabs->AddTab( TXT("Budgets") );
		RedGui::CRedGuiControl* tab = m_tabs->GetTabAt( tabIndex );
		if( tab != nullptr )
		{
			CDebugWindowMemoryBudgets* memoryBudgets = new CDebugWindowMemoryBudgets( 0, 0, 100, 100 );
			memoryBudgets->SetDock( RedGui::DOCK_Fill );
			tab->AddChild( memoryBudgets );
		}
	}

}	// namespace DebugWindows

#endif	//ENABLE_EXTENDED_MEMORY_METRICS
#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
