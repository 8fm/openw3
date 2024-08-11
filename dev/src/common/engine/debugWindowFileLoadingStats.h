/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../core/fileLoadingStats.h"

#include "redGuiWindow.h"
#include "../core/engineTime.h"

namespace RedGui
{
	class CRedGuiList;
}

namespace DebugWindows
{
	class CMemoryMetricsPoolAreaControl;
	class CMemoryMetricsUsageRenderControl;

	class CDebugWindowFileLoadingStats : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowFileLoadingStats();
		~CDebugWindowFileLoadingStats();

	private:
		virtual void	OnWindowOpened( CRedGuiControl* control ) override;
		virtual void	OnWindowClosed(CRedGuiControl* control) override;

	private:
		void			CreateControls();

	private:
		void			NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void			NotifyOnClickedResetStats( RedGui::CRedGuiEventPackage& eventPackage );
		void			NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage );
		void			NotifyEventSelectedClassItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );

	private:
		RedGui::CRedGuiList*	CreateFileStatsList();
		RedGui::CRedGuiList*	CreateClassStatsList();

	private:
		void			RefreshStats();
		void			ClearStats();

	private:
		void			RefreshClassStatsList();
		void			RefreshFileStatsList();
		void			RefreshFilteredFileStatsList();

	private:
		void			ClearClassStatsList();
		void			ClearFileStatsList();
		void			ClearFilteredFileStatsList();

	private:
		CName											m_filteredClassName;
		THashMap< CName, RedGui::CRedGuiListItem* >		m_classItemsMap;
		THashMap< String, RedGui::CRedGuiListItem* >	m_fileItemsMap;
		THashMap< String, RedGui::CRedGuiListItem* >	m_filteredFileItemsMap;

	private:
		THashMap< String, CFileLoadingStats::FileStats* >			m_fileStatsMap;
		THashMap< const CClass*, CFileLoadingStats::ClassStats* >	m_classStatsMap;

	private:
		RedGui::CRedGuiTab*				m_tabs;
		RedGui::CRedGuiList*			m_fileStatsList;
		RedGui::CRedGuiList*			m_classStatsList;
		RedGui::CRedGuiList*			m_filteredFileStatsList;

//		RedGui::CRedGuiSaveFileDialog*	m_dumpSaveDialog;

	private:
		EngineTime						m_lastRefreshTime;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
