/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"

namespace RedGui
{
	class CRedGuiMenuBar : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
		struct SMenu
		{
			SMenu( CRedGuiMenu* menu )
				: m_menu( menu )
				, m_menuText( String::EMPTY )
				, m_menuWidth( 0 )
			{
				/* intentionally empty */
			}

			String			m_menuText;
			Uint32			m_menuWidth;
			CRedGuiMenu*	m_menu;
		};

	public:
		CRedGuiMenuBar();
		virtual ~CRedGuiMenuBar();

		CRedGuiMenu* AppendMenu(CRedGuiMenu* menu, const String& name);
		CRedGuiMenu* AddNewMenu(const String& name);
		CRedGuiMenu* GetMenu(const String& name);

		void Draw();

	protected:
		void NotifyEventVisibleChanged( CRedGuiEventPackage& eventPackage, Bool value );

		void OnKeyButtonPressed( enum EInputKey key, Char text );
		void OnKeyChangeRootFocus( Bool focus );
		void OnMouseMove( const Vector2& mousePosition );
		void OnMouseLostFocus( CRedGuiControl* newControl );
		void OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button );

		void OpenMenu();
		void CloseMenu();
		void Toggle();
		void UpdateOpened();

		Int32 CheckPoint( const Vector2& position );

	private:
		TDynArray< SMenu, MC_RedGuiControls, MemoryPool_RedGui >	m_menus;
		CRedGuiMenu*		m_openedMenu;
		Int32				m_activeMenuIndex;

		Bool				m_menuIsOpened;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
