/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiThemeManager.h"
#include "redGuiDefaultTheme.h"
#include "redGuiGradientTheme.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiThemeManager::CRedGuiThemeManager( CRedGuiGraphicContext* gc )
		: m_defaultTheme( nullptr )
	{
		// add default theme
		m_defaultTheme = new CRedGuiDefaultTheme( gc );
		ASSERT(m_defaultTheme != nullptr);
		m_themes.Insert( RED_NAME( RedGuiDefaultTheme ) , m_defaultTheme );

		// add gradient theme
		CRedGuiGradientTheme* gradientTheme = new CRedGuiGradientTheme( gc );
		ASSERT(gradientTheme != nullptr);
		m_themes.Insert( RED_NAME( RedGuiGradientTheme ), gradientTheme );
	}

	CRedGuiThemeManager::~CRedGuiThemeManager()
	{
		for( ThemesContainer::const_iterator i=m_themes.Begin(); i != m_themes.End(); ++i)
		{
			IRedGuiTheme* theme = i->m_second;
			delete theme;
			theme = nullptr;
		}
		m_themes.Clear();
	}

	IRedGuiTheme* CRedGuiThemeManager::GetDefaultTheme()
	{
		return m_defaultTheme;
	}

	Bool CRedGuiThemeManager::ChangeDefaultTheme( const CName& themeName )
	{
		IRedGuiTheme* theme = GetThemeByName( themeName );
		if(theme == nullptr)
		{
			return false;
		}

		m_defaultTheme = theme;
		return true;
	}

	IRedGuiTheme* CRedGuiThemeManager::GetThemeByName( const CName& themeName )
	{
		if( m_themes.FindPtr( themeName ) == nullptr )
		{
			return nullptr;
		}

		return *m_themes.FindPtr( themeName );
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
