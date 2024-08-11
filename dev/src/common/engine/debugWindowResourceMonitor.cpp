/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_RESOURCE_MONITORING

#include "redGuiManager.h"
#include "redGuiGridLayout.h"
#include "redGuiTab.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiSpin.h"
#include "redGuiLabel.h"

#include "../../common/core/depot.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/garbageCollector.h"

#include "../../common/redSystem/stringWriter.h"

#include "debugWindowResourceMonitor.h"

namespace DebugWindows
{

	CDebugWindowResourceMonitor::ResourceInfo::ResourceInfo( const CDiskFile* file )
		: m_file( file )
		, m_monitor( file->GetMonitorData() )
		, m_listItem( nullptr )
	{
	}

	void CDebugWindowResourceMonitor::ResourceInfo::GetValueForColumn( const EFileListColumns column, Char* buf, Uint32 bufSize ) const
	{
		switch ( column )
		{
			case eEFileListColumn_FrameLoaded: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_frameLoaded ); break;
			case eEFileListColumn_TimeLoaded: Red::SNPrintF( buf, bufSize, TXT("%1.3f"), m_monitor->m_timeLoaded ); break;
			case eEFileListColumn_LoadCount: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_loadCount ); break;
			case eEFileListColumn_FrameUnloaded: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_frameUnloaded ); break;
			case eEFileListColumn_TimeUnloaded: Red::SNPrintF( buf, bufSize, TXT("%1.3f"), m_monitor->m_timeUnloaded ); break;
			case eEFileListColumn_UnloadCount: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_unloadCount ); break;
			case eEFileListColumn_FrameExpelled: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_frameExpelled ); break;
			case eEFileListColumn_TimeExpelled: Red::SNPrintF( buf, bufSize, TXT("%1.3f"), m_monitor->m_timeExpelled ); break;
			case eEFileListColumn_ExpellCount: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_expellCount ); break;
			case eEFileListColumn_FrameRevived: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_frameRevived ); break;
			case eEFileListColumn_TimeRevived: Red::SNPrintF( buf, bufSize, TXT("%1.3f"), m_monitor->m_timeRevived ); break;
			case eEFileListColumn_ReviveCount: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_reviveCount ); break;
			case eEFileListColumn_LoadTime: Red::SNPrintF( buf, bufSize, TXT("%1.1f"), m_monitor->m_loadTime * 1000.0f ); break;
			case eEFileListColumn_WorstLoadTime: Red::SNPrintF( buf, bufSize, TXT("%1.1f"), m_monitor->m_worstLoadTime * 1000.0f ); break;
			case eEFileListColumn_HadImports: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_hadImports ? 1 : 0 ); break;
			case eEFileListColumn_PostLoadTime: Red::SNPrintF( buf, bufSize, TXT("%1.1f"), m_monitor->m_postLoadTime * 1000.0f ); break;
			case eEFileListColumn_WorstPostLoadTime: Red::SNPrintF( buf, bufSize, TXT("%1.1f"), m_monitor->m_worstPostLoadTime * 1000.0f ); break;
			case eEFileListColumn_HadPostLoadImports: Red::SNPrintF( buf, bufSize, TXT("%d"), m_monitor->m_hadPostLoadImports ? 1 : 0 ); break;
		}
	}
	
	void CDebugWindowResourceMonitor::ResourceInfo::Refresh( const TColumnSelection& columnSelection )
	{
		if ( !m_listItem )
			return;

		// status
		if ( m_monitor->m_isMissing )
			m_listItem->SetText( TXT("Missing"), 1 );
		else if ( m_monitor->m_isQuarantined )
			m_listItem->SetText( TXT("Not Used"), 1 );
		else if ( m_monitor->m_isLoaded )
			m_listItem->SetText( TXT("Loaded"), 1 );
		else
			m_listItem->SetText( TXT("Unloaded"), 1 );

		// coloring
		if ( m_monitor->m_isMissing )
			m_listItem->SetTextColor( Color::RED );
		else if ( m_monitor->m_isQuarantined )
		m_listItem->SetTextColor( Color::GRAY );
		else
			m_listItem->SetTextColor( Color::WHITE );
		
		// additional columns
		Uint32 columnIndex = 2;
		for ( const ColumnInfo& info : columnSelection )
		{
			if ( info.m_isVisible )
			{
				Char buf[256];
				GetValueForColumn( info.m_id, buf, ARRAY_COUNT(buf) );
				m_listItem->SetText( buf, columnIndex );
				columnIndex += 1;
			}
		}
	}

//----

CDebugWindowResourceMonitor::CDebugWindowResourceMonitor()
	: RedGui::CRedGuiWindow( 100, 100, 900, 600 )
	, m_realtimeUpdate( true )
	, m_showUnloaded( false )
	, m_tabs( nullptr )
	, m_resourcesList( nullptr )
	, m_maxEvents( 500 ) // todo: customize
	, m_lastEventMarker( 0 )
{
	SetCaption( TXT( "Resource Monitor" ) );

	CreateColumnSelection();
	LoadColumnSettings();

	RedGui::CRedGuiGridLayout* menuPanel = new RedGui::CRedGuiGridLayout( 0, 0, 100, 26 );
	menuPanel->SetDock( RedGui::DOCK_Top );
	menuPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
	menuPanel->SetDimensions( 5, 1 );
	AddChild( menuPanel );

	RedGui::CRedGuiButton* dumpsStats = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
	dumpsStats->SetText( TXT("Dump stats") );
	dumpsStats->SetMargin( Box2( 15, 5, 10, 0 ) );
	dumpsStats->EventButtonClicked.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedDumpStats );
	menuPanel->AddChild( dumpsStats );

	RedGui::CRedGuiButton* refreshButton = new RedGui::CRedGuiButton( 60, 0, 50, 20 );
	refreshButton->SetText( TXT("Refresh") );
	refreshButton->SetMargin( Box2( 15, 5, 10, 0 ) );
	refreshButton->EventButtonClicked.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedRefresh );
	menuPanel->AddChild( refreshButton );

	RedGui::CRedGuiButton* gcButton = new RedGui::CRedGuiButton( 120, 0, 50, 20 );
	gcButton->SetText( TXT("Force GC") );
	gcButton->SetMargin( Box2( 15, 5, 10, 0 ) );
	gcButton->EventButtonClicked.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedForceGC );
	menuPanel->AddChild( gcButton );

	RedGui::CRedGuiCheckBox* realtimeCheckbox = new RedGui::CRedGuiCheckBox( 190, 10, 80, 20 );
	realtimeCheckbox->SetText( TXT("Realtime events") );
	realtimeCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	realtimeCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedShow );
	realtimeCheckbox->SetChecked( m_realtimeUpdate );
	menuPanel->AddChild( realtimeCheckbox );

	RedGui::CRedGuiCheckBox* showUnloadedCheckbox = new RedGui::CRedGuiCheckBox( 290, 10, 80, 20 );
	showUnloadedCheckbox->SetText( TXT("Show unloaded") );
	showUnloadedCheckbox->SetMargin( Box2( 15, 5, 10, 0 ) );
	showUnloadedCheckbox->EventCheckedChanged.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedShowUnloaded );
	showUnloadedCheckbox->SetChecked( m_showUnloaded );
	menuPanel->AddChild( showUnloadedCheckbox );

	m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
	m_tabs->SetMargin(Box2(5, 5, 5, 5));
	m_tabs->SetDock( RedGui::DOCK_Fill );
	AddChild( m_tabs );

	m_tabs->AddTab( TXT( "Resources" ) );
	m_tabs->AddTab( TXT( "Events" ) );
	m_tabs->AddTab( TXT( "Filters" ) );
	m_tabs->SetActiveTab( 0 );

	// create the resource list
	{
		RedGui::CRedGuiScrollPanel* restTab = m_tabs->GetTabAt( 0 );

		m_resourcesList	= new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_resourcesList->SetMargin( Box2( 5,5,5,5 ) );
		m_resourcesList->SetDock( RedGui::DOCK_Fill );
		m_resourcesList->SetSorting( true );
		restTab->AddChild( m_resourcesList );

		if( !ResourceMonitorStats::IsEnabled() )
		{
			RedGui::CRedGuiListItem* cmdListItem = new RedGui::CRedGuiListItem( TXT( "Resource monitoring disabled. Add EnableResourceMonitor to your user.ini" ), nullptr, Color::RED );
			m_resourcesList->AddItem( cmdListItem );
		}
	}

	// create the event list
	{
		RedGui::CRedGuiScrollPanel* restTab = m_tabs->GetTabAt( 1 );
		restTab->SetVisibleVScroll( true );

		m_eventList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		m_eventList->SetMargin( Box2( 5,5,5,5 ) );
		m_eventList->SetDock( RedGui::DOCK_Fill );
		m_eventList->SetSorting( false );
		restTab->AddChild( m_eventList );

		m_eventList->AppendColumn( TXT("Path"), 500 );
		m_eventList->AppendColumn( TXT("Event"), 80 );
		m_eventList->AppendColumn( TXT("Time"), 70 );
		m_eventList->AppendColumn( TXT("Frame"), 50 );			
		m_eventList->AppendColumn( TXT("Sync"), 50 );
		m_eventList->AppendColumn( TXT("LoadTime"), 70 );
	}

	// create the columns tab
	{
		RedGui::CRedGuiScrollPanel* restTab = m_tabs->GetTabAt( 2 );
		restTab->SetVisibleVScroll( true );

		Uint32 yPos = 10;
		Uint32 ySize = 15;
		Uint32 yMargin = 5;
		for ( const ColumnInfo& info : m_columns )
		{
			RedGui::CRedGuiCheckBox* checkbox = new RedGui::CRedGuiCheckBox( 10, yPos, 200, ySize );
			checkbox->SetMargin( Box2( 5,5,5,5 ) );
			checkbox->SetText( info.m_name );
			checkbox->SetChecked( info.m_isVisible );
			checkbox->SetUserData( new EFileListColumns( info.m_id ) );
			checkbox->EventCheckedChanged.Bind( this, &CDebugWindowResourceMonitor::NotifyOnClickedColumnFilterUpdate );

			restTab->AddChild( checkbox );

			yPos += ySize + yMargin;
		}
	}
}

CDebugWindowResourceMonitor::~CDebugWindowResourceMonitor()
{
}

void CDebugWindowResourceMonitor::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
{
	if ( m_realtimeUpdate )
	{
		RefreshEventList();
	}
}

void CDebugWindowResourceMonitor::NotifyOnClickedRefresh( RedGui::CRedGuiEventPackage& eventPackage )
{
	RefreshResourceList( true );
}

void CDebugWindowResourceMonitor::NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage )
{
}

void CDebugWindowResourceMonitor::NotifyOnClickedForceGC( RedGui::CRedGuiEventPackage& eventPackage )
{
	SGarbageCollector::GetInstance().CollectNow();
	RefreshResourceList( true );
}

void CDebugWindowResourceMonitor::NotifyOnClickedShow( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
{
	m_realtimeUpdate = value;
}

void CDebugWindowResourceMonitor::NotifyOnClickedShowUnloaded( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
{
	if ( m_showUnloaded != value )
	{
		m_showUnloaded = value;
		RefreshResourceList( true );
	}
}

void CDebugWindowResourceMonitor::NotifyOnClickedColumnFilterUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
{
	const EFileListColumns* columnId = eventPackage.GetEventSender()->GetUserData<EFileListColumns>(); // HACK!

	// toggle the visibility of the column
	if ( columnId )
	{
		Bool changed = false;
		for ( ColumnInfo& info : m_columns )
		{
			if ( info.m_id == *columnId )
			{
				if ( info.m_isVisible != value )
				{
					info.m_isVisible = value;
					changed = true;
					break;
				}
			}
		}

		// save config
		StoreColumnSettings();

		// refresh the list
		RefreshResourceListColumns();
		RefreshResourceList();
	}
}

void CDebugWindowResourceMonitor::OnWindowOpened( CRedGuiControl* control )
{
	GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowResourceMonitor::NotifyOnTick );

	CreateResourceEntries();

	RefreshResourceListColumns();
	RefreshResourceList( true );

	RefreshEventList();
}

void CDebugWindowResourceMonitor::OnWindowClosed( CRedGuiControl* control )
{
	GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowResourceMonitor::NotifyOnTick );
}

void CDebugWindowResourceMonitor::CreateResourceEntries()
{
	if( ResourceMonitorStats::IsEnabled() )
	{
		// cleanup current data
		m_resourcesList->RemoveAllItems();
		m_entries.ClearPtr();

		// get file list
		TDynArray< CDiskFile* > allFiles;
		allFiles.Reserve( 500000 );
		GDepot->CollectFiles( allFiles, String::EMPTY, true, true );

		// create the resource entry for each depot file
		m_entries.Reserve( allFiles.Size() );
		for ( CDiskFile* file : allFiles )
		{
			ResourceInfo* entry = new ResourceInfo( file );
			m_entries.PushBack( entry );
		}
	}
}

void CDebugWindowResourceMonitor::RefreshResourceListColumns()
{
	// delete all current columns
	while ( m_resourcesList->GetColumnCount() > 0 )
		m_resourcesList->DeleteColumn( 0 );

	// add static columns
	m_resourcesList->AppendColumn( TXT("Path"), 500 );
	m_resourcesList->AppendColumn( TXT("State"), 60 );

	// add dynamic columns
	for ( const ColumnInfo& info : m_columns )
	{
		if ( info.m_isVisible )
		{
			m_resourcesList->AppendColumn( info.m_title, 70, RedGui::SA_Real );
		}
	}
}

void CDebugWindowResourceMonitor::RefreshResourceList( const Bool recreateItems )
{
	if ( recreateItems && ResourceMonitorStats::IsEnabled() )
	{
		// unlink current entries
		for ( ResourceInfo* info : m_entries )
			info->m_listItem = nullptr;

		// delete current entries
		m_resourcesList->RemoveAllItems();

		// create list items based on the entries
		for ( ResourceInfo* info : m_entries )
		{
			if ( !m_showUnloaded )
				if ( !info->m_monitor->m_isLoaded && !info->m_monitor->m_isMissing && !info->m_monitor->m_isQuarantined )
					continue;

			// creat list item
			info->m_listItem = new RedGui::CRedGuiListItem();
			info->m_listItem->SetText( info->m_file->GetDepotPath() ); // set the name string

			// add to list
			m_resourcesList->AddItem( info->m_listItem );
		}
	}

	// refresh item text
	for ( ResourceInfo* info : m_entries )
		info->Refresh( m_columns );
}

void CDebugWindowResourceMonitor::RefreshEventList()
{
	// get events
	Uint32 marker = 0;
	m_eventData.ClearFast();
	m_eventData.Reserve( m_maxEvents );
	ResourceMonitorStats::CollectLastEvents( m_eventData, m_maxEvents, marker );
	if ( marker == m_lastEventMarker )
		return;

	// update
	m_lastEventMarker = marker;

	// create the list items
	if ( m_eventData.Size() > m_eventItems.Size() )
	{
		m_eventItems.Reserve( m_eventData.Size() );

		const Uint32 toCreate = m_eventData.Size() - m_eventItems.Size();
		for ( Uint32 i=0; i<toCreate; ++i )
		{
			RedGui::CRedGuiListItem* listItem = new RedGui::CRedGuiListItem();
			m_eventItems.PushBack( listItem );
			m_eventList->AddItem( listItem );
		}
	}

	// update the event captions
	Double timeBase = 0.0f;
	Uint32 frameBase = 0;
	const Char* timeSign = TXT("");
	for ( Uint32 i=0; i<m_eventData.Size(); ++i )
	{
		const ResourceMonitorStats* data = m_eventData[i];
		RedGui::CRedGuiListItem* listItem = m_eventItems[i];

		// file name
		listItem->SetText( data->GetFile()->GetDepotPath(), 0 );
		listItem->SetText( data->m_isSyncLoaded ? TXT("Sync") : TXT(""), 4 );

		// event type related data
		Double currentTime  = 0.0f;
		Uint32 currentFrame = 0;
		switch ( data->GetLastEvent() )
		{
			case ResourceMonitorStats::eEventType_Loaded:
			{
				if ( data->m_isSyncLoaded )
					listItem->SetTextColor( Color::WHITE );
				else
					listItem->SetTextColor( Color::LIGHT_GREEN );

				listItem->SetText( TXT("Loaded"), 1 );
				listItem->SetText( String::Printf( TXT("%1.3f"), data->m_loadTime * 100.0f ), 6 );
				currentTime = data->m_timeLoaded;
				currentFrame = data->m_frameLoaded;
				break;
			}

			case ResourceMonitorStats::eEventType_Unloaded:
			{
				listItem->SetTextColor( Color::LIGHT_RED );
				listItem->SetText( TXT("Unloaded"), 1 );
				listItem->SetText( TXT(""), 6 );
				currentTime = data->m_timeUnloaded;
				currentFrame = data->m_frameUnloaded;
				break;
			}

			case ResourceMonitorStats::eEventType_Expelled:
			{
				listItem->SetTextColor( Color::LIGHT_YELLOW );
				listItem->SetText( TXT("Expelled"), 1 );
				listItem->SetText( TXT(""), 6 );
				currentTime = data->m_timeExpelled;
				currentFrame = data->m_frameExpelled;
				break;
			}

			case ResourceMonitorStats::eEventType_Revived:
			{
				listItem->SetTextColor( Color::LIGHT_MAGENTA );
				listItem->SetText( TXT("Revived"), 1 );
				listItem->SetText( TXT(""), 6 );
				currentTime = data->m_timeRevived;
				currentFrame = data->m_frameRevived;
				break;
			}
		}

		// timing info
		listItem->SetText( String::Printf( TXT("%1.3f"), currentTime - timeBase ), 2 );
		listItem->SetText( String::Printf( TXT("%d"), currentFrame - frameBase ), 3 );
		frameBase = currentFrame;
		timeBase = currentTime;
	}
}

void CDebugWindowResourceMonitor::CreateColumnSelection()
{
	m_columns.Clear();

	// columns here are added in display order

	new ( m_columns ) ColumnInfo( eEFileListColumn_FrameLoaded, TXT("Load event frame"), TXT("fLoad") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_TimeLoaded, TXT("Load event time"), TXT("tLoad") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_LoadCount, TXT("Number of loads"), TXT("#Load"), true );

	new ( m_columns ) ColumnInfo( eEFileListColumn_FrameUnloaded, TXT("Unload event (frame)"), TXT("fUnload") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_TimeUnloaded, TXT("Unload event (time)"), TXT("tUnload") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_UnloadCount, TXT("Number of unloads"), TXT("#Unload"), true );

	new ( m_columns ) ColumnInfo( eEFileListColumn_FrameUnloaded, TXT("Unload event (frame)"), TXT("fUnload") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_TimeUnloaded, TXT("Unload event (time)"), TXT("tUnload") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_UnloadCount, TXT("Number of unloads"), TXT("#Unload") );

	new ( m_columns ) ColumnInfo( eEFileListColumn_FrameExpelled, TXT("Expell event (frame)"), TXT("fExpell") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_TimeExpelled, TXT("Expell event (time)"), TXT("tExpell") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_ExpellCount, TXT("Number of expells"), TXT("#Expell") );

	new ( m_columns ) ColumnInfo( eEFileListColumn_FrameRevived, TXT("Revive event (frame)"), TXT("fRevive") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_TimeRevived, TXT("Revive event (time)"), TXT("tRevive") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_ReviveCount, TXT("Number of Revives"), TXT("#Revive") );

	new ( m_columns ) ColumnInfo( eEFileListColumn_LoadTime, TXT("Last loading time (ms)"), TXT("Load") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_WorstLoadTime, TXT("Worst loading time (ms)"), TXT("MaxLoad"), true );
	new ( m_columns ) ColumnInfo( eEFileListColumn_PostLoadTime, TXT("Last post load time (ms)"), TXT("PostLoad") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_WorstPostLoadTime, TXT("Worst post loadign time (ms)"), TXT("MaxPostLoad") );

	new ( m_columns ) ColumnInfo( eEFileListColumn_HadImports, TXT("Recursive loading"), TXT("RecLoad") );
	new ( m_columns ) ColumnInfo( eEFileListColumn_HadPostLoadImports, TXT("Recursive post loading"), TXT("RecPostLoad") );
}



void CDebugWindowResourceMonitor::LoadColumnSettings()
{
	for ( Uint32 i=0; i<m_columns.Size(); ++i )
	{
		ColumnInfo& column = m_columns[i];

		const String key = String::Printf( TXT("Column%d"), column.m_id );
		SConfig::GetInstance().GetLegacy().ReadParam( TXT("debug"), TXT("ResourceMonitorColumns"), key, column.m_isVisible );
	}
}

void CDebugWindowResourceMonitor::StoreColumnSettings()
{
	for ( Uint32 i=0; i<m_columns.Size(); ++i )
	{
		ColumnInfo& column = m_columns[i];

		const String key = String::Printf( TXT("Column%d"), column.m_id );
		SConfig::GetInstance().GetLegacy().WriteParam<Bool>( TXT("debug"), TXT("ResourceMonitorColumns"), key.AsChar(), column.m_isVisible );
	}
}


} // DebugWindows

#endif // ENABLE_RESOURCE_MONITORING
#endif // NO_DEBUG_WINDOWS
#endif // NO_RED_GUI
