/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGradientTheme.h"
#include "redGuiControl.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiImage.h"
#include "redGuiGraphicContext.h"

namespace RedGui
{
	CRedGuiGradientTheme::CRedGuiGradientTheme(CRedGuiGraphicContext* gc)
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

	void CRedGuiGradientTheme::DrawPanel(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();

		if(control->GetState() == STATE_Disabled)
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabled, DarkerColor(m_disabled), ignoreGlobalAlpha );
			DrawBorder(control, m_disabledBorder);
		}
		else
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), DarkerColor(control->GetBackgroundColor()), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
		}

		if( control->GetNeedKeyFocus() == true && control == GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() )
		{
			m_graphicContex->Rectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X+1, (Uint32)area.Max.Y+1, m_keyFocus, ignoreGlobalAlpha );
		}
	}

	void CRedGuiGradientTheme::DrawButton(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();

		if(control->GetState() == STATE_Highlighted)
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), DarkerColor(control->GetBackgroundColor()), ignoreGlobalAlpha );
			DrawBorder(control, control->GetForegroundColor());
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_highlight, DarkerColor(m_highlight), ignoreGlobalAlpha );
		}
		else if(control->GetState() == STATE_Normal)
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), DarkerColor(control->GetBackgroundColor()), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
		}
		else if(control->GetState() == STATE_Pushed)
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, control->GetBackgroundColor(), DarkerColor(control->GetBackgroundColor()), ignoreGlobalAlpha );
			DrawBorder(control, m_border);
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_pushed, DarkerColor(m_pushed), ignoreGlobalAlpha );
		}
		else if(control->GetState() == STATE_Disabled)
		{
			m_graphicContex->FilledRectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabled, DarkerColor(m_disabled), ignoreGlobalAlpha );
			DrawBorder(control, m_disabledBorder);
		}

		if( control->GetNeedKeyFocus() == true && control == GRedGui::GetInstance().GetInputManager()->GetKeyFocusControl() )
		{
			m_graphicContex->Rectangle((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X+1, (Uint32)area.Max.Y+1, m_keyFocus, ignoreGlobalAlpha );
		}
	}

	void CRedGuiGradientTheme::DrawLabel(const CRedGuiControl* control)
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

	void CRedGuiGradientTheme::DrawImage(const CRedGuiControl* control)
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
		const CRedGuiImage* image = static_cast<const CRedGuiImage*>(control);
		CRedGuiImage* unsafeImage = const_cast< CRedGuiImage*>( image );
		Color fgColor = control->GetBackgroundColor();

		m_graphicContex->DrawImage((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, unsafeImage->GetImage(), fgColor, ignoreGlobalAlpha );
	}

	void CRedGuiGradientTheme::SetCroppedParent(const CRedGuiCroppedRect* rect)
	{
		m_graphicContex->OpenClip(rect->GetAbsoluteCoord());
	}

	void CRedGuiGradientTheme::ResetCroppedParent()
	{
		m_graphicContex->CloseClip();
	}

	void CRedGuiGradientTheme::DrawBorder( const CRedGuiControl* control, const Color& borderColor, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if(control->GetBorderVisible() == true)
		{
			Box2 area = control->GetAbsoluteCoord();
			Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
			m_graphicContex->RectangleAdvancedColor((Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, borderColor, DarkerColor(borderColor), ignoreGlobalAlpha );
		}
	}

	Color CRedGuiGradientTheme::DarkerColor( const Color& color )
	{
		Color darkerColor = color;
		darkerColor.R = Clamp<Uint8>(darkerColor.R - (Uint8)(darkerColor.R*0.5), 0, 255);
		darkerColor.G = Clamp<Uint8>(darkerColor.G - (Uint8)(darkerColor.G*0.5), 0, 255);
		darkerColor.B = Clamp<Uint8>(darkerColor.B - (Uint8)(darkerColor.B*0.5), 0, 255);

		return darkerColor;
	}

	CRedGuiGradientTheme::~CRedGuiGradientTheme()
	{
		/*intentionally empty*/
	}

	void CRedGuiGradientTheme::DrawLine( const CRedGuiControl* control )
	{
		Box2 area = control->GetAbsoluteCoord();
		Bool ignoreGlobalAlpha = control->GetIgnoreGlobalAlpha();
		m_graphicContex->LineAdvancedColor( (Uint32)area.Min.X, (Uint32)area.Min.Y, (Uint32)area.Max.X, (Uint32)area.Max.Y, m_disabledBorder, DarkerColor( m_disabledBorder ), ignoreGlobalAlpha );
	}

	void CRedGuiGradientTheme::DrawRawLine( const Vector2& startPoint, const Vector2& endPoint, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->LineAdvancedColor( (Uint32)startPoint.X, (Uint32)startPoint.Y, (Uint32)endPoint.X, (Uint32)endPoint.Y, color, DarkerColor( color ), ignoreGlobalAlpha  );
	}

	void CRedGuiGradientTheme::DrawRawRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->RectangleAdvancedColor( (Uint32)leftTopCorner.X, (Uint32)leftTopCorner.Y, (Uint32)size.X, (Uint32)size.Y, color, DarkerColor( color ), ignoreGlobalAlpha  );
	}

	void CRedGuiGradientTheme::DrawRawFilledRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->FilledRectangleAdvancedColor( (Uint32)leftTopCorner.X, (Uint32)leftTopCorner.Y, (Uint32)size.X, (Uint32)size.Y, color, DarkerColor( color ), ignoreGlobalAlpha  );
	}

	void CRedGuiGradientTheme::DrawRawCircle( const Vector2& centre, Float radius, const Color& color, Uint32 segments, Bool ignoreGlobalAlpha /*= false*/ )
	{
		// Colour Calculations
		Vector lightColor = color.ToVector();
		Color targetColor = DarkerColor( color );
		Vector colorRange = ( targetColor.ToVector() - lightColor );

		Float diameter = radius * 2.0f;

		// Segment calculations
		Float increment = ( 360.0f / segments );

		// Pre-calculated values for a == 0
		Float x1 = centre.X + 0.0f;
		Float y1 = centre.Y + radius;
		
		for( Float a = increment; a <= 360.0f + 0.00001f; a += increment )
		{
			const Float angle = DEG2RAD( a );

			const Float x2 = centre.X + ( Red::Math::MSin( angle ) * radius );
			const Float y2 = centre.Y + ( Red::Math::MCos( angle ) * radius );

			Color c1( lightColor + ( colorRange * ( ( y1 - centre.Y ) / diameter ) ) );
			Color c2( lightColor + ( colorRange * ( ( y2 - centre.Y ) / diameter ) ) );

			m_graphicContex->LineAdvancedColor( static_cast< Int32 >( x1 ), static_cast< Int32 >( y1 ), static_cast< Int32 >( x2 ), static_cast< Int32 >( y2 ), c1, c2, ignoreGlobalAlpha  );

			x1 = x2;
			y1 = y2;
		}
	}

	void CRedGuiGradientTheme::DrawRawText( const Vector2& position, const String& text, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if( text.Empty() == false )
		{
			m_graphicContex->TextOut( (Uint32)position.X, (Uint32)position.Y, text, color, GRedGui::GetInstance().GetFontManager()->GetDefaultFont(), ignoreGlobalAlpha  );
		}
	}

	void CRedGuiGradientTheme::DrawRawImage( const Vector2& position, const Vector2& size, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		m_graphicContex->DrawImage( (Uint32)position.X, (Uint32)position.Y, (Uint32)size.X, (Uint32)size.Y, image, color, ignoreGlobalAlpha  );
	}


	void CRedGuiGradientTheme::DrawRawRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( numRectangles == 0 )
		{
			return;
		}
		RED_ASSERT( rectangles != nullptr && colors != nullptr );

		TDynArray< Color > darkerColors( numRectangles );
		for ( Uint32 i = 0; i < numRectangles; ++i )
		{
			darkerColors[i] = DarkerColor( colors[i] );
		}
		m_graphicContex->RectanglesAdvancedColor( numRectangles, rectangles, colors, darkerColors.TypedData(), ignoreGlobalAlpha );
	}

	void CRedGuiGradientTheme::DrawRawFilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( numRectangles == 0 )
		{
			return;
		}
		RED_ASSERT( rectangles != nullptr && colors != nullptr );

		TDynArray< Color > darkerColors( numRectangles );
		for ( Uint32 i = 0; i < numRectangles; ++i )
		{
			darkerColors[i] = DarkerColor( colors[i] );
		}
		m_graphicContex->FilledRectanglesAdvancedColor( numRectangles, rectangles, colors, darkerColors.TypedData(), ignoreGlobalAlpha );
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
