/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "../core/clipboardBase.h"
#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "fonts.h"
#include "inputKeys.h"

namespace RedGui
{
	CRedGuiLabel::CRedGuiLabel(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
		: CRedGuiControl(x, y, width, height) 
		, m_autoSize( true )
	{ 
		SetFont( RGFT_Default );
		SetMargin(Box2(3,3,3,3));
		SetNeedKeyFocus( false );
	}

	CRedGuiLabel::~CRedGuiLabel()
	{
		/*intentionally empty*/
	}

	const String& CRedGuiLabel::GetText() const 
	{ 
		return m_text; 
	}

	void CRedGuiLabel::SetText(const String& text, const Color& textColor/* = Color::WHITE*/) 
	{ 
		m_text = text; 

		AdjustToText();
		SetForegroundColor( textColor );
	}

	void CRedGuiLabel::Draw() 
	{ 
		if( m_text.Empty() == false )
		{
			GetTheme()->DrawLabel( this ); 
		}
	}

	void CRedGuiLabel::ClearText()
	{
		SetText(TXT(""));
	}

	Bool CRedGuiLabel::GetAutoSize() const
	{
		return m_autoSize;
	}

	void CRedGuiLabel::SetAutoSize( Bool value )
	{
		m_autoSize = value;
		AdjustToText();
	}

	void CRedGuiLabel::AdjustToText()
	{
		if( m_autoSize == true )
		{
			Int32 unusedX, unusedy;
			Uint32 width, height;
			if( CFont* font = GetFont() )
			{
				font->GetTextRectangle( m_text, unusedX, unusedy, width, height );
				SetSize( width, height );
			}
			SetOriginalRect( GetCoord() );
			SetOutOfDate();
		}
	}

	void CRedGuiLabel::OnKeyButtonPressed( enum EInputKey key, Char text )
	{
		// general functionality of the whole text
		if( GRedGui::GetInstance().GetInputManager()->IsControlPressed() == true )
		{
			if( key == IK_C )
			{
				CopyText();
			}
		}
	}

	void CRedGuiLabel::OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button )
	{
		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( this );
	}

	void CRedGuiLabel::CopyText()
	{
		// get text from clipboard
		if( GClipboard->Open( CF_Text ) == false)
		{
			return;
		}

		GClipboard->Copy( m_text );
		GClipboard->Close();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
