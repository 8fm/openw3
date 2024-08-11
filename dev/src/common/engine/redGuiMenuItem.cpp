/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiMenuItem.h"
#include "redGuiManager.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultHeight = 20;
		const Float GSeparatorHeight = 10.0f;
		const Float GSeparatorWidth= 50.0f;
		const Uint32 GDefaultFreeSpace = 5;
	}

	CRedGuiMenuItem::CRedGuiMenuItem( ERedGuiMenuItemType type )
		: m_submenu( nullptr )
		, m_type( type )
	{
		CalculateSize();
	}

	CRedGuiMenuItem::~CRedGuiMenuItem()
	{
		/* intentionally empty */
	}

	ERedGuiMenuItemType CRedGuiMenuItem::GetType() const
	{
		return m_type;
	}

	void CRedGuiMenuItem::SetSubmenu( CRedGuiMenu* submenu )
	{
		m_submenu = submenu;
	}

	void CRedGuiMenuItem::SetText( const String& text )
	{
		m_text = text;
		CalculateSize();
	}

	void CRedGuiMenuItem::CalculateSize()
	{
		if( m_type == MENUITEM_Separator )
		{
			m_size = Vector2( GSeparatorWidth + ( 2 * GDefaultFreeSpace ), GSeparatorHeight );
		}
		else if( m_type == MENUITEM_SubMenu )
		{
			m_size = Vector2( GRedGui::GetInstance().GetFontManager()->GetStringSize( m_text, RGFT_Default ).X + (Float)( 2 * GDefaultFreeSpace ), (Float)GDefaultHeight );
			m_size += Vector2( 20.0f, 0.0f );	// add place for arrow
		}
		else
		{
			m_size = Vector2( GRedGui::GetInstance().GetFontManager()->GetStringSize( m_text, RGFT_Default ).X + (Float)( 2 * GDefaultFreeSpace ), (Float)GDefaultHeight );
		}
	}

	String CRedGuiMenuItem::GetText() const
	{
		return m_text;
	}

	Vector2 CRedGuiMenuItem::GetSize() const
	{
		return m_size;
	}

	CRedGuiMenu* CRedGuiMenuItem::GetSubMenu() const
	{
		return m_submenu;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
