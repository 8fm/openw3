/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiMenu : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiMenu(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiMenu();

		// Events
		Event2_PackageMenuItem EventMenuItemSelected;

		CRedGuiMenuItem* AppendItem( const String& item, RedGuiAny userData = nullptr );
		CRedGuiMenuItem* AppendSubMenuItem( const String& item, CRedGuiMenu* submenu, RedGuiAny userData = nullptr );
		CRedGuiMenuItem* AppendSeparator();

		virtual void SetVisible( Bool value );

		void SetActiveSubmenu( CRedGuiMenu* submenu );

		void AttachToMenuBar( CRedGuiMenuBar* parentMenuBar );
	
		void Draw();

		Bool ProcessInput( enum EInputKey key );

	protected:
		void NotifyParentPositionChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldPosition, const Vector2& newPosition );
		void NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );

		void OpenSubmenu();
		void CloseSubmenu();

		void OnMouseMove( const Vector2& mousePosition );
		void OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button );

		Int32 CheckPoint( const Vector2& position );

		TDynArray< CRedGuiMenuItem*, MC_RedGuiControls, MemoryPool_RedGui >	m_entries;			//!<
		CRedGuiMenuBar*					m_parentMenuBar;	//!<
		Int32							m_activeMenuIndex;
										
		Bool							m_timerOn;			//!<
		Float							m_timer;			//!<

		Int32							m_keyFocusedItem;	//!<

		CRedGuiMenu*					m_openedSubmenu;
		CRedGuiImage*					m_arrowIcon;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
