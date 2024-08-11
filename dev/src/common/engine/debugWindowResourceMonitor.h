/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_DEBUG_WINDOW_RESOURCE_MONITOR_H_
#define _RED_DEBUG_WINDOW_RESOURCE_MONITOR_H_

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef ENABLE_RESOURCE_MONITORING

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowResourceMonitor : public RedGui::CRedGuiWindow
	{
	public:
		CDebugWindowResourceMonitor();
		virtual ~CDebugWindowResourceMonitor();

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );
		void NotifyOnClickedRefresh( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedForceGC( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnClickedShow( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnClickedShowUnloaded( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnClickedColumnFilterUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool value );


		virtual void OnWindowOpened( CRedGuiControl* control ) override final;
		virtual void OnWindowClosed( CRedGuiControl* control ) override final;

		void CreateResourceEntries();
		void RefreshResourceListColumns();
		void RefreshResourceList( const Bool recreateItems = false );

		void RefreshEventList();

		void CreateColumnSelection();
		void LoadColumnSettings();
		void StoreColumnSettings();

		enum EFileListColumns
		{
			eEFileListColumn_FrameLoaded,
			eEFileListColumn_TimeLoaded,
			eEFileListColumn_LoadCount,
			eEFileListColumn_FrameUnloaded,
			eEFileListColumn_TimeUnloaded,
			eEFileListColumn_UnloadCount,
			eEFileListColumn_FrameExpelled,
			eEFileListColumn_TimeExpelled,
			eEFileListColumn_ExpellCount,
			eEFileListColumn_FrameRevived,
			eEFileListColumn_TimeRevived,
			eEFileListColumn_ReviveCount,
			eEFileListColumn_LoadTime,
			eEFileListColumn_WorstLoadTime,
			eEFileListColumn_HadImports,
			eEFileListColumn_PostLoadTime,
			eEFileListColumn_WorstPostLoadTime,
			eEFileListColumn_HadPostLoadImports,

			eEFileListColumn_MAX,
		};

		struct ColumnInfo
		{
			EFileListColumns	m_id;
			String				m_name;
			String				m_title;
			Bool				m_isVisible;

			ColumnInfo()
				: m_isVisible( false )
			{}

			ColumnInfo( const EFileListColumns id, const String& name, const String& title, const Bool isVisible = false )
				: m_id( id )
				, m_title( title )
				, m_name( name )
				, m_isVisible( isVisible )
			{}
		};

		typedef TDynArray< ColumnInfo >			TColumnSelection;
		TColumnSelection			m_columns;

		struct ResourceInfo
		{
			const CDiskFile*					m_file;
			const ResourceMonitorStats*			m_monitor;

			RedGui::CRedGuiListItem*			m_listItem; // only if visible

			ResourceInfo( const CDiskFile* file );

			// get value for column
			void GetValueForColumn( const EFileListColumns column, Char* buf, Uint32 bufSize ) const;

			// refresh the entry
			void Refresh( const TColumnSelection& columnSelection );
		};

		typedef TDynArray< ResourceInfo* >		TResourceInfos;
		TResourceInfos				m_entries;

		typedef TDynArray< RedGui::CRedGuiListItem* >		TEventItems;
		typedef TDynArray< ResourceMonitorStats* >			TMonitorData;
		TEventItems					m_eventItems;
		TMonitorData				m_eventData;
		Uint32						m_maxEvents;
		Uint32						m_lastEventMarker;

		RedGui::CRedGuiTab*			m_tabs;
		RedGui::CRedGuiList*		m_resourcesList;
		RedGui::CRedGuiList*		m_eventList;

		Bool		m_realtimeUpdate;
		Bool		m_showUnloaded;
	};
}

#endif // ENABLE_RESOURCE_MONITORING
#endif // NO_DEBUG_WINDOWS
#endif // NO_RED_GUI

#endif // _RED_DEBUG_WINDOW_RESOURCE_SYSTEM_H_
