/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class CRedGuiControlSkin
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiControlSkin();
		virtual ~CRedGuiControlSkin();

		void SetBackgroundColor( const Color& color );
		Color GetBackgroundColor() const;

		void SetForegroundColor( const Color& color );
		Color GetForegroundColor() const;

		void SetFont( enum ERedGuiFontType fontType );
		CFont* GetFont() const;

		void SetIgnoreGlobalAlpha( Bool value );
		Bool GetIgnoreGlobalAlpha() const;

	protected:
		Color	m_backgroundColor;
		Color	m_foregroundColor;
		CFont*	m_font;
		Bool	m_ignoreGlobalAlpha;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
