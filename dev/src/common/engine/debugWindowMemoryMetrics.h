/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

#include "redGuiWindow.h"
#include "../redMemoryFramework/redMemoryAllocatorInfo.h"

class CDebugWindowMemoryHistogram;
class CDebugWindowMemoryHistoryViewer;

namespace Memory { class Adapter; }

namespace DebugWindows
{
	class CMemoryMetricsPoolAreaControl;
	class CMemoryMetricsUsageControl;
	class CDebugWindowMemoryClassPanel;
	
	class CDebugWindowMemoryMetrics : public RedGui::CRedGuiWindow, public Red::MemoryFramework::AllocatorWalker
	{
	public:
		CDebugWindowMemoryMetrics();
		~CDebugWindowMemoryMetrics();

	private:
		void CreateControls();
		void AddMemoryManager( Red::TUniquePtr<Memory::Adapter> manager, const String& name );
	
		// general callbacks
		void OnWindowOpened( CRedGuiControl* control );
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifySelectedManager( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );
		void NotifySelectedPool( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );
		void NotifyOnClickedRestMetrics( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedTrackMetrics( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedDebugOutput( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyDumpStatsFileOK( RedGui::CRedGuiEventPackage& eventPackage );		
		void NotifyDumpTrackingFileOK( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyRunGcNow( RedGui::CRedGuiEventPackage& eventPackage );

		// Allocator walker interface
		void OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size );

		// memory classes
		void CreateMemoryClassesPanel();
		String GetFormattedSize( Red::System::Int64 memSize );
		void UpdateMemoryClassesList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo );
		void UpdateMemClassPreviousValues( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics );
		void UpdateMemoryClassesLabels( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo );

		// memory usage
		void CreateMemoryUsageControls();
		void UpdateMemoryUsageControls();

		// heap area viewer
		void CreateHeapAreaViewer();
		void UpdateHeapAreaViewer();

		// budgets
		void CreateBudgetsTab();

		// GC
		void CreateGCTab();

		// graphs
		void CreateGraphViewer();
		void UpdateGraphViewerClassSelector();
		void NotifySelectedGraphClass( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );
		void NotifySelectedGraphType( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );

		// leak tracker
		void CreateLeakTrackerPanel();
		void NotifyOnClickedTrackLeaks( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnSelectedLeak( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void StartTrackingLeaks();
		void StopTrackingLeaks();
		void ClearGUI();
		void ResetMemoryClassesLabels();

	private:
		void AppendFileStats( const String& filename );

		enum GraphViewerType
		{
			View_Groups,
			View_Classes,
			View_Histogram
		};

		// general- gui
		RedGui::CRedGuiTab*				m_tabs;
		RedGui::CRedGuiComboBox*		m_managerComboBox;
		RedGui::CRedGuiComboBox*		m_poolComboBox;
		RedGui::CRedGuiSaveFileDialog*	m_dumpSaveDialog;
		RedGui::CRedGuiSaveFileDialog*	m_trackingSaveDialog;

		// general - logic
		TDynArray< Red::TUniquePtr<Memory::Adapter> >		m_memoryManagers;
		Memory::Adapter*					m_selectedManager;
		Red::MemoryFramework::PoolLabel							m_selectedPoolLabel;

		// memory classes - gui
		RedGui::CRedGuiLabel*			m_allocatorTypeLabel;
		RedGui::CRedGuiLabel*			m_allocatorBudgetLabel;
		RedGui::CRedGuiLabel*			m_allocatorOverheadLabel;
		RedGui::CRedGuiLabel*			m_totalAllocatedLabel;
		RedGui::CRedGuiLabel*			m_activeAllocationsLabel;
		RedGui::CRedGuiLabel*			m_totalAllocatedPeekLabel;
		RedGui::CRedGuiLabel*			m_activeAllocationsPeekLabel;
		RedGui::CRedGuiLabel*			m_allocatedPerFrameLabel;
		RedGui::CRedGuiLabel*			m_allocationsPerFrameLabel;
		RedGui::CRedGuiLabel*			m_allocatedPerFramePeekLabel;
		RedGui::CRedGuiLabel*			m_allocationsPerFramePeekLabel;
		RedGui::CRedGuiLabel*			m_deallocatedPerFrameLabel;
		RedGui::CRedGuiLabel*			m_deallocationsPerFrameLabel;
		RedGui::CRedGuiLabel*			m_deallocatedPerFramePeekLabel;
		RedGui::CRedGuiLabel*			m_deallocationsPerFramePeekLabel;
		RedGui::CRedGuiOccupancyChart*	m_memoryClassChart;
		RedGui::CRedGuiAreaChart*		m_memoryClassesAreaChart;

		// memory classes - logic
		Red::System::Int64					m_previousMemoryClassSizes[ Red::MemoryFramework::k_MaximumMemoryClasses ];
		CDebugWindowMemoryClassPanel*		m_memoryClassListControl;

		// graphs
		CDebugWindowMemoryHistogram*		m_histogram;
		CDebugWindowMemoryHistoryViewer*	m_historyViewer;
		RedGui::CRedGuiComboBox*			m_graphClassSelector;
		GraphViewerType						m_graphType;

		// memory usage
		CMemoryMetricsUsageControl*			m_memoryUsage;

		// heap area viewer
		RedGui::CRedGuiScrollPanel*			m_panelForHeapViewers;

		// heap area viewer - logic
		TDynArray< CMemoryMetricsPoolAreaControl* >	m_heapViewers;
		CMemoryMetricsPoolAreaControl*				m_activePoolChart;

		// leak tracker - gui
		RedGui::CRedGuiList*		m_leaksList;
		RedGui::CRedGuiTextBox*		m_callstackText;

		// Dumping state
		RedGui::CRedGuiButton*		m_dumpButton;
		Bool						m_isDumping;
	};

}	// namespace DebugWindows

#endif	// ENABLE_EXTENDED_MEMORY_METRICS
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
