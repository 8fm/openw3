/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiFontManager
	{
	public:
		CRedGuiFontManager();
		virtual ~CRedGuiFontManager();

		// return actually default font
		CFont* GetDefaultFont() const;

		// change default font - true if can change
		Bool ChangeDefaultFont( enum ERedGuiFontType value );

		// get font by name
		CFont* GetFont( enum ERedGuiFontType value );

		// Get string size
		Vector2 GetStringSize(const String& text, enum ERedGuiFontType value );

	private:
		void LoadFonts();

	private:
		CFont*	m_fonts[ RGFT_Count ];
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
