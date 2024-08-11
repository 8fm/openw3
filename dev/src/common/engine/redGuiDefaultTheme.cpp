/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiControl.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiImage.h"
#include "redGuiTextBox.h"
#include "redGuiGraphicContext.h"
#include "redGuiDefaultTheme.h"

namespace RedGui
{
	CRedGuiDefaultTheme::CRedGuiDefaultTheme(CRedGuiGraphicContext* gc)
		: m_graphicContex(gc)
		, m_highlight(Color(255,170, 0, 100))
		, m_pushed(Color(255, 150, 0, 100))
		, m_disabled(Color(244, 244, 244, 255))
		, m_border(Color(112, 112, 112, 255))
		, m_disabledBorder(Color(173, 178, 181, 255))
		, m_keyFocus( Color( 255, 255, 125, 255 ) )
	{
		/* intentionally empty */
	}

	CRedGuiDefaultTheme::~CRedGuiDefaultTheme()
	{
		/*intentionally empty*/
	}

	void CRedGuiDefaultTheme::DrawPanel(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();

		if(control->GetState() == STATE_Disabled)
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabled, ignoreGlobalAlpha );
			DrawBorder(control, m_disabledBorder);
		}
		else
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
		}

		if( control->GetNeedKeyFocus() == true && control == GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() )
		{
			m_graphicContex->Rectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X+1, (Uint32)area.Max.Y+1, m_keyFocus, ignoreGlobalAlpha );
		}
	}

	void CRedGuiDefaultTheme::DrawButton(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();

		if(control->GetState() == STATE_Highlighted)
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), ignoreGlobalAlpha );
			DrawBorder(control, control->GetForegroundColor());
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_highlight, ignoreGlobalAlpha );
		}
		else if(control->GetState() == STATE_Normal)
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
		}
		else if(control->GetState() == STATE_Pushed)
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_pushed, ignoreGlobalAlpha );
		}
		else if(control->GetState() == STATE_Disabled)
		{
			m_graphicContex->FilledRectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabled, ignoreGlobalAlpha );
			DrawBorder(control, m_disabledBorder);
		}

		if( control->GetNeedKeyFocus() == true && control == GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() )
		{
			m_graphicContex->Rectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X+1, (Uint32)area.Max.Y+1, m_keyFocus, ignoreGlobalAlpha );
		}
	}

	void CRedGuiDefaultTheme::DrawLabel(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
		const CRedGuiLabel* label = static_cast<const CRedGuiLabel*>(control);

		if( control->GetWidth() > 0 )
		{
			if(control->GetState() == STATE_Disabled)
			{
				m_graphicContex->TextOut((Uint32)area.Min.X, (Uint32)area.Min.Y, label->GetText(), m_disabledBorder, GRedGui::GetInstance().GetFontManager()->GetDefaultFont(), ignoreGlobalAlpha );
			}
			else
			{
				m_graphicContex->TextOut((Uint32)area.Min.X, (Uint32)area.Min.Y, label->GetText(), label->GetForegroundColor(), GRedGui::GetInstance().GetFontManager()->GetDefaultFont(), ignoreGlobalAlpha );
			}
		}
	}

	void CRedGuiDefaultTheme::DrawImage(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
		const CRedGuiImage* image = static_cast<const CRedGuiImage*>(control);
		CRedGuiImage* unsafeImage = const_cast< CRedGuiImage*>( image );
		Color fgColor = control->GetBackgroundColor();

		m_graphicContex->DrawImage((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, unsafeImage->GetImage(), fgColor, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::SetCroppedParent( const CRedGuiCroppedRect* rect )
	{
		m_graphicContex->OpenClip(rect->GetAbsoluteCoord());
	}

	void CRedGuiDefaultTheme::ResetCroppedParent()
	{
		m_graphicContex->CloseClip();
	}

	void CRedGuiDefaultTheme::DrawBorder( const CRedGuiControl* control, const Color& borderColor, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if(control->GetBorderVisible() == true)
		{
			Box2 area = control->GetAbsoluteCoord();
			m_graphicContex->Rectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, borderColor, ignoreGlobalAlpha );
		}
	}

	void CRedGuiDefaultTheme::DrawLine( const CRedGuiControl* control )
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
		m_graphicContex->Line((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabledBorder, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawLine( const Vector2& startPoint, const Vector2& endPoint, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->Line( (Uint32)startPoint.X, (Uint32)startPoint.Y, (Uint32)endPoint.X, (Uint32)endPoint.Y, color, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->Rectangle( (Uint32)leftTopCorner.X, (Uint32)leftTopCorner.Y, (Uint32)size.X, (Uint32)size.Y, color, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawFilledRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->FilledRectangle( (Uint32)leftTopCorner.X, (Uint32)leftTopCorner.Y, (Uint32)size.X, (Uint32)size.Y, color, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawCircle( const Vector2& centre, Float radius, const Color& color, Uint32 segments, Bool ignoreGlobalAlpha /*= false*/ )
	{
		// Pre-calculated values for a == 0
		Float x1 = centre.X + 0.0f;
		Float y1 = centre.Y + radius;

		// Segment calculations
		Float increment = ( 360.0f / segments );

		for( Float a = increment; a <= 360.0f + 0.00001f; a += increment )
		{
			const Float angle = DEG2RAD( a );

			const Float x2 = centre.X + ( Red::Math::MSin( angle ) * radius );
			const Float y2 = centre.Y + ( Red::Math::MCos( angle ) * radius );

			m_graphicContex->Line( static_cast< Int32 >( x1 ), static_cast< Int32 >( y1 ), static_cast< Int32 >( x2 ), static_cast< Int32 >( y2 ), color, ignoreGlobalAlpha );

			x1 = x2;
			y1 = y2;
		}
	}

	void CRedGuiDefaultTheme::DrawRawText( const Vector2& position, const String& text, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if( text.Empty() == false )
		{
			m_graphicContex->TextOut( (Uint32)position.X, (Uint32)position.Y, text, color, GRedGui::GetInstance().GetFontManager()->GetDefaultFont(), ignoreGlobalAlpha );
		}
	}

	void CRedGuiDefaultTheme::DrawRawImage( const Vector2& position, const Vector2& size, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->DrawImage( (Uint32)position.X, (Uint32)position.Y, (Uint32)size.X, (Uint32)size.Y, image, color, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->Rectangles( numRectangles, rectangles, colors, ignoreGlobalAlpha );
	}

	void CRedGuiDefaultTheme::DrawRawFilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->FilledRectangles( numRectangles, rectangles, colors, ignoreGlobalAlpha );
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
