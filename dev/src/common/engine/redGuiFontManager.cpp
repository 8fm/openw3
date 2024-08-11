/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiFontManager.h"
#include "redGuiEmbeddedResources.h"
#include "../core/gatheredResource.h"
#include "fonts.h"

namespace RedGui
{
	CRedGuiFontManager::CRedGuiFontManager()
	{
		LoadFonts();
	}

	CRedGuiFontManager::~CRedGuiFontManager()
	{
		/* intentionally empty */
	}

	CFont* CRedGuiFontManager::GetDefaultFont() const
	{
		return m_fonts[RGFT_Default];
	}

	Bool CRedGuiFontManager::ChangeDefaultFont( enum ERedGuiFontType value )
	{
		CFont* font = GetFont( value );
		if( font == nullptr )
		{
			return false;
		}

		m_fonts[RGFT_Default] = font;
		return true;
	}

	CFont* CRedGuiFontManager::GetFont( enum ERedGuiFontType value )
	{
		if( value < RGFT_Count )
		{
			return m_fonts[value];
		}
		return nullptr;
	}

	Vector2 CRedGuiFontManager::GetStringSize(const String& text, enum ERedGuiFontType value )
	{
		CFont* font = GetFont( value );
		if(font == nullptr)
		{
			return Vector2(0,0);
		}

		Int32 unusedX, unusedY;
		Uint32 textWidth, textHeight;
		font->GetTextRectangle(text, unusedX, unusedY, textWidth, textHeight);

		return Vector2((Float)textWidth, (Float)textHeight);
	}

	void CRedGuiFontManager::LoadFonts()
	{
		m_fonts[RGFT_Parachute] = Resources::GParachuteFont.LoadAndGet< CFont >();

		// set default font
		m_fonts[RGFT_Default] = m_fonts[RGFT_Parachute];
	}
	
}	// namespace RedGui

#endif	// NO_RED_GUI
