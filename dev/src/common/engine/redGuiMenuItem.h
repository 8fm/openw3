/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiMenu.h"

namespace RedGui
{
	class CRedGuiMenuItem : public RedGui::CRedGuiUserData
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiMenuItem( ERedGuiMenuItemType type );
		virtual ~CRedGuiMenuItem();

		ERedGuiMenuItemType GetType() const;

		void SetText( const String& text );
		String GetText() const;

		CRedGuiMenu* GetSubMenu() const;
		void SetSubmenu( CRedGuiMenu* submenu );

		Vector2 GetSize() const;

	private:
		void CalculateSize();

	private:
		ERedGuiMenuItemType	m_type;
		CRedGuiMenu*		m_submenu;
		Vector2				m_size;
		String				m_text;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
