/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../core/version.h"
#include "../core/depot.h"
#include "../core/resource.h"
#include "../core/rttiSystem.h"
#include "../core/diskFile.h"

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

#include "debugWindowFileLoadingStats.h"
#include "baseEngine.h"

namespace DebugWindows
{
	CDebugWindowFileLoadingStats::CDebugWindowFileLoadingStats()
		: RedGui::CRedGuiWindow( 200, 200, 1000, 600 )
		, m_lastRefreshTime( GEngine->GetRawEngineTime() )
	{
		SetCaption( TXT("File loading stats [Exported resources won't appear in file lists]") );

		CreateControls();
	}

	CDebugWindowFileLoadingStats::~CDebugWindowFileLoadingStats()
	{
	}

	void CDebugWindowFileLoadingStats::CreateControls()
	{
#ifndef NO_FILE_LOADING_STATS
		RedGui::CRedGuiGridLayout* menuPanel = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
		menuPanel->SetDock( RedGui::DOCK_Top );
		menuPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		menuPanel->SetDimensions( 5, 1 );
		AddChild( menuPanel );

		RedGui::CRedGuiButton* resetStats = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
		resetStats->SetText( TXT("Reset stats") );
		resetStats->SetMargin( Box2( 15, 5, 10, 0 ) );
		resetStats->EventButtonClicked.Bind( this, &CDebugWindowFileLoadingStats::NotifyOnClickedResetStats );
		menuPanel->AddChild( resetStats );

		RedGui::CRedGuiButton* dumpsStats = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
		dumpsStats->SetText( TXT("Dump stats") );
		dumpsStats->SetMargin( Box2( 15, 5, 10, 0 ) );
		dumpsStats->EventButtonClicked.Bind( this, &CDebugWindowFileLoadingStats::NotifyOnClickedDumpStats );
		menuPanel->AddChild( dumpsStats );

		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		m_tabs->SetMargin( Box2( 5, 5, 5, 5 ) );
		AddChild( m_tabs );

		// Class stats
		{
			const Uint32 tabIndex = m_tabs->AddTab( TXT("Class stats") );
			RedGui::CRedGuiScrollPanel* classStatsTab = m_tabs->GetTabAt( tabIndex );
			RedGui::CRedGuiGridLayout* gridLayout = new RedGui::CRedGuiGridLayout( 0, 0, 0, 0 );
			gridLayout->SetDimensions( 1, 2 );
			gridLayout->SetDock( RedGui::DOCK_Fill );
			classStatsTab->AddChild( gridLayout );

			m_classStatsList = CreateClassStatsList();
			m_classStatsList->EventSelectedItem.Bind( this, &CDebugWindowFileLoadingStats::NotifyEventSelectedClassItemChanged );
			gridLayout->AddChild( m_classStatsList );

			m_filteredFileStatsList = CreateFileStatsList();
			gridLayout->AddChild( m_filteredFileStatsList );
		}

		// File stats
		{
			const Uint32 tabIndex = m_tabs->AddTab( TXT("File stats") );
			RedGui::CRedGuiScrollPanel* fileStatsTab = m_tabs->GetTabAt( tabIndex );
			m_fileStatsList = CreateFileStatsList();
			fileStatsTab->AddChild( m_fileStatsList );
		}
			
		m_tabs->SetActiveTab( 0 );
#else
		RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel(0,0,100,26);
		label->SetText( TXT("NO_FILE_LOADING_STATS defined in source") );
		AddChild( label );
#endif // ! NO_FILE_LOADING_STATS
	}

	void CDebugWindowFileLoadingStats::OnWindowOpened( CRedGuiControl* control )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowFileLoadingStats::NotifyOnTick );
	}

	void CDebugWindowFileLoadingStats::OnWindowClosed( CRedGuiControl* control )
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowFileLoadingStats::NotifyOnTick );
	}

	RedGui::CRedGuiList* CDebugWindowFileLoadingStats::CreateFileStatsList()
	{
		RedGui::CRedGuiList* list = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		list->SetMargin( Box2( 5, 5, 5, 5 ) );
		list->SetDock( RedGui::DOCK_Fill );
		list->AppendColumn( TXT("LoadTables [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("CreateExports [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("LoadExports [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("PostLoad [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Blocks [N]"), 100, RedGui::SA_Integer );
		list->AppendColumn( TXT("Data [KB]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Path"), 600 );
		list->SetSorting( true );
		list->SetTextAlign( RedGui::IA_MiddleLeft );

		return list;
	}

	RedGui::CRedGuiList* CDebugWindowFileLoadingStats::CreateClassStatsList()
	{
		RedGui::CRedGuiList* list = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		list->SetMargin( Box2( 5, 5, 5, 5 ) );
		list->SetDock( RedGui::DOCK_Fill );
		list->AppendColumn( TXT("Name"), 250 );
		list->AppendColumn( TXT("Create time [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Serialize [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("PostLoad [ms]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Blocks [N]"), 100, RedGui::SA_Integer );
		list->AppendColumn( TXT("Data [KB]"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Instances [N]"), 100, RedGui::SA_Integer );
		list->SetSorting( true );

		return list;
	}

	void CDebugWindowFileLoadingStats::RefreshStats()
	{
		RefreshClassStatsList();
		RefreshFileStatsList();
		RefreshFilteredFileStatsList();
	}

	void CDebugWindowFileLoadingStats::RefreshClassStatsList()
	{	
		for ( THashMap< const CClass*, CFileLoadingStats::ClassStats* >::const_iterator it = m_classStatsMap.Begin(); it != m_classStatsMap.End(); ++it )
		{
			const CClass* classDesc = it->m_first;
			CName className = classDesc->GetName();
			CFileLoadingStats::ClassStats* classStats = it->m_second;

			RedGui::CRedGuiListItem* listItem = nullptr;
			if ( ! m_classItemsMap.Find( className, listItem ) )
			{
				listItem = new RedGui::CRedGuiListItem;
				m_classStatsList->AddItem( listItem );
				m_classItemsMap.Insert( className, listItem );
			}

			const Float MS = 1000.f;

			const String fmtClassName = classDesc->IsA< CResource >() ? String::Printf(TXT("%s [Resource]"), className.AsString().AsChar() ) : className.AsString();
			Uint32 col = 0;
			listItem->SetUserString( TXT("Class"), className.AsString() );
			listItem->SetText( fmtClassName, col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), classStats->m_creationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), classStats->m_deserializationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), classStats->m_postLoadTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8i"), classStats->m_numBlocks ), col++ );
			listItem->SetText( String::Printf( TXT("%9.1f"), classStats->m_bytesRead / 1024.f ), col++ );
			listItem->SetText( String::Printf( TXT("%9i"), classStats->m_numObjects ), col++ );
		}
	}

	void CDebugWindowFileLoadingStats::RefreshFileStatsList()
	{
		String absoluteDepotPath;
		GDepot->GetAbsolutePath( absoluteDepotPath );
		
		for ( THashMap< String, CFileLoadingStats::FileStats* >::const_iterator it = m_fileStatsMap.Begin(); it != m_fileStatsMap.End(); ++it )
		{
			const String& filePath = it->m_first;
			CFileLoadingStats::FileStats* fileStats = it->m_second;

			RedGui::CRedGuiListItem* listItem = nullptr;
			if ( ! m_fileItemsMap.Find( filePath, listItem ) )
			{
				listItem = new RedGui::CRedGuiListItem;
				m_fileStatsList->AddItem( listItem );
				m_fileItemsMap.Insert( filePath, listItem );
			}

			const Float MS = 1000.f;
			Uint32 col = 0;
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_tableLoadingTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_objectCreationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_deserializationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_postLoadTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8i"), fileStats->m_numBlocks ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_bytesRead / 1024.f ), col++ );

			//TBD: Could just offset the pointer, but in case not loaded with depot path
			String depotPath = filePath.StringAfter( absoluteDepotPath );
			listItem->SetText( depotPath.Empty() ? filePath : depotPath, col++ );
		}
	}

	void CDebugWindowFileLoadingStats::RefreshFilteredFileStatsList()
	{
		String absoluteDepotPath;
		GDepot->GetAbsolutePath( absoluteDepotPath );

		for ( THashMap< String, CFileLoadingStats::FileStats* >::const_iterator it = m_fileStatsMap.Begin(); it != m_fileStatsMap.End(); ++it )
		{
			const String& filePath = it->m_first;
			CFileLoadingStats::FileStats* fileStats = it->m_second;

			const CName className = fileStats->m_className;
			
			if ( ! className || className != m_filteredClassName )
			{
				continue;
			}

			RedGui::CRedGuiListItem* listItem = nullptr;
			if ( ! m_filteredFileItemsMap.Find( filePath, listItem ) )
			{
				listItem = new RedGui::CRedGuiListItem;
				m_filteredFileStatsList->AddItem( listItem );
				m_filteredFileItemsMap.Insert( filePath, listItem );
			}

			const Float MS = 1000.f;
			Uint32 col = 0;
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_tableLoadingTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_objectCreationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_deserializationTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_postLoadTime * MS ), col++ );
			listItem->SetText( String::Printf( TXT("%8i"), fileStats->m_numBlocks ), col++ );
			listItem->SetText( String::Printf( TXT("%8.1f"), fileStats->m_bytesRead / 1024.f ), col++ );

			//TBD: Could just offset the pointer, but in case not loaded with depot path
			String depotPath = filePath.StringAfter( absoluteDepotPath );
			listItem->SetText( depotPath.Empty() ? filePath : depotPath, col++ );
		}
	}

	void CDebugWindowFileLoadingStats::ClearStats()
	{
		ClearClassStatsList();
		ClearFileStatsList();
		ClearFilteredFileStatsList();
	}

	void CDebugWindowFileLoadingStats::ClearClassStatsList()
	{
		m_classItemsMap.Clear();
		m_classStatsList->RemoveAllItems();
		RefreshClassStatsList();
	}

	void CDebugWindowFileLoadingStats::ClearFileStatsList()
	{
		m_fileItemsMap.Clear();
		m_fileStatsList->RemoveAllItems();
		RefreshFileStatsList();
	}

	void CDebugWindowFileLoadingStats::ClearFilteredFileStatsList()
	{
		m_filteredFileItemsMap.Clear();
		m_filteredFileStatsList->RemoveAllItems();
		RefreshFilteredFileStatsList();
	}

	void CDebugWindowFileLoadingStats::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
#ifndef NO_FILE_LOADING_STATS
		RED_UNUSED( deltaTime ); // <-- Strange values

		const EngineTime& now = GEngine->GetRawEngineTime();
		if ( now - m_lastRefreshTime > 1.f )
		{
			m_fileStatsMap = SFileLoadingStats::GetInstance().GetFileStatsCopy();
			m_classStatsMap = SFileLoadingStats::GetInstance().GetClassStatsCopy();

			// FIXME: Refresh whichever tab is visible
			RefreshStats();
			m_lastRefreshTime = now;
		}
#endif // !NO_FILE_LOADING_STATS
	}

	void CDebugWindowFileLoadingStats::NotifyOnClickedResetStats( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );

		{
			// Lock here st_loadingMutex so loading files don't have their stat pointers invalidated
			// Ugly and so is how we safely load resources
			typedef Red::Threads::CMutex CMutex;
			typedef Red::Threads::CScopedLock< CMutex > CScopedLock;
			SFileLoadingStats::GetInstance().Clear();
		}

		m_fileStatsMap.Clear();
		m_classStatsMap.Clear();
		ClearStats();
	}

	void CDebugWindowFileLoadingStats::NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage )
	{
		SFileLoadingStats::GetInstance().Dump();
	}

	void CDebugWindowFileLoadingStats::NotifyEventSelectedClassItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RED_UNUSED( eventPackage );

		if(selectedIndex != -1)
		{
			RedGui::CRedGuiListItem* listItem = m_classStatsList->GetItem( selectedIndex );
			const String& className = listItem->GetUserString( TXT("Class") );
			m_filteredClassName.Set( className );
			ClearFilteredFileStatsList();
			RefreshStats();
		}
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
