/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_EXTENDED_MEMORY_METRICS

#include "redGuiPanel.h"

namespace RedGui
{
	class CRedGuiTab;
}

namespace DebugWindows
{
	// This control displays all memory classes for a memory manager, with options to filter by group
	class CDebugWindowMemoryClassPanel : public RedGui::CRedGuiPanel
	{
	public:
		CDebugWindowMemoryClassPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CDebugWindowMemoryClassPanel();

		void Refresh( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector, Red::MemoryFramework::AllocatorInfo& allocInfo );
		void Reset();

	private:
		void RefreshActiveTab( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector );
		void RefreshTabs( const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector );
		void RefreshAllGroupsToList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector );
		void RefreshAllClassesToList( const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector );
		void RefreshOneGroupToList( Uint32 groupIndex, const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics, const Red::MemoryFramework::AllocationMetricsCollector& metricsCollector );
		void NotifySelectedTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedItem );

		RedGui::CRedGuiTab* m_tabs;
		RedGui::CRedGuiList* m_list;
		CRedGuiPanel* m_topPanel;

		const Red::MemoryFramework::AllocationMetricsCollector* m_lastMetricsCollector;
	};
}

#endif
#endif
#endif