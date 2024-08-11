/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTheme.h"

namespace RedGui
{
	class CRedGuiThemeManager
	{
	public:
		CRedGuiThemeManager( CRedGuiGraphicContext* gc );
		virtual ~CRedGuiThemeManager();

		// return actually default theme
		IRedGuiTheme* GetDefaultTheme();

		// change default font - true if can change
		Bool ChangeDefaultTheme(const CName& themeName);

		// get font by name
		IRedGuiTheme* GetThemeByName(const CName& themeName);

	private:
		IRedGuiTheme*	m_defaultTheme;	//!< default theme for all controls
		ThemesContainer	m_themes;		//!< container with all possible themes
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
