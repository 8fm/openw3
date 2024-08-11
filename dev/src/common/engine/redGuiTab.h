/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiTab : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiTab(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiTab();

		// Events
		Event2_PackageControl EventTabChanged;

		Uint32 AddTab( const String& name );

		CRedGuiScrollPanel* GetTabAt( Uint32 index );
		CRedGuiScrollPanel* GetTabByName( const String& tabName );

		Uint32 GetTabCount() const;

		void SetActiveTab( Uint32 value );
		Int32 GetActiveTabIndex() const;
		CRedGuiScrollPanel* GetActiveTab() const;

		void Draw();

	private:
		void RecalculateTabsPosition();

		void NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value );

		void OnSizeChanged( const Vector2& oldSize, const Vector2& newSize );
		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

	private:
		CRedGuiPanel* m_panelForTabs;
		CRedGuiPanel* m_panelForActiveTab;
		CRedGuiControl* m_activeTab;
		TDynArray< CRedGuiScrollPanel*, MC_RedGuiControls, MemoryPool_RedGui > m_tabs;
		TDynArray< CRedGuiButton*, MC_RedGuiControls, MemoryPool_RedGui > m_tabsButtons;

		TDynArray< CRedGuiPanel*, MC_RedGuiControls, MemoryPool_RedGui > m_rowsWithTabs;
		THashMap< CRedGuiPanel*, TDynArray< CRedGuiButton*, MC_RedGuiControls, MemoryPool_RedGui > > m_mapRowsTabButtons;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
