/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControlSkin.h"
#include "redGuiManager.h"

namespace RedGui
{
	CRedGuiControlSkin::CRedGuiControlSkin()
		: m_backgroundColor( Color( 45, 45, 45, 255 ) )
		, m_foregroundColor( Color( 244, 244, 244, 255 ) )
		, m_font( nullptr )
		, m_ignoreGlobalAlpha( false )
	{
		m_font = GRedGui::GetInstance().GetFontManager()->GetFont( RedGui::RGFT_Default );
	}

	CRedGuiControlSkin::~CRedGuiControlSkin()
	{
		/*intentionally empty*/
	}

	void CRedGuiControlSkin::SetBackgroundColor( const Color& color )
	{
		m_backgroundColor = color;
	}

	Color CRedGuiControlSkin::GetBackgroundColor() const
	{
		return m_backgroundColor;
	}

	void CRedGuiControlSkin::SetForegroundColor( const Color& color )
	{
		m_foregroundColor = color;
	}

	Color CRedGuiControlSkin::GetForegroundColor() const
	{
		return m_foregroundColor;
	}

	void CRedGuiControlSkin::SetFont( enum ERedGuiFontType fontType )
	{
		m_font = GRedGui::GetInstance().GetFontManager()->GetFont( fontType );
	}

	CFont* CRedGuiControlSkin::GetFont() const
	{
		return m_font;
	}

	void CRedGuiControlSkin::SetIgnoreGlobalAlpha( Bool value )
	{
		m_ignoreGlobalAlpha = value;
	}

	Bool CRedGuiControlSkin::GetIgnoreGlobalAlpha() const
	{
		return m_ignoreGlobalAlpha;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
