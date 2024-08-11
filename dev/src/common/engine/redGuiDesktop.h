/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"
#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiDesktop : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiDesktop();
		~CRedGuiDesktop();

		CRedGuiWindow* AddWindow( CRedGuiWindow* window );
		void RemoveWindow( CRedGuiWindow* window );

		void SetMenuBar(CRedGuiMenuBar* menuBar);
		CRedGuiMenuBar* GetMenuBar() const;

		void ResizeLayerItemView( const Vector2& oldView, const Vector2& newView );

		void Draw();

		void AddToMenuBar( RedGui::CRedGuiControl* control );

		void SetActiveModalWindow( CRedGuiControl* modalWindow );

	private:
		virtual void AddChild(CRedGuiControl* child);
		virtual void RemoveChild(CRedGuiControl* child);

		virtual Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		void NotifyOnRootKeyChangeFocus( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnWindowOpened( RedGui::CRedGuiEventPackage& eventPackage );

	private:
		CRedGuiPanel*	m_menuBarPanel;			//!<
		CRedGuiMenuBar* m_menuBar;				//!<
		CRedGuiControl* m_client;				//!<

		Int32			m_activeKeyFocusedControl;
		TDynArray< CRedGuiControl*, MC_RedGuiControls, MemoryPool_RedGui >	m_keyFocusedControls;
		CRedGuiControl*					m_focusedDesktopControl;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
