/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTheme.h"

namespace RedGui
{
	class CRedGuiGradientTheme : public IRedGuiTheme
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiThemes );
	public:
		CRedGuiGradientTheme( CRedGuiGraphicContext* gc );
		virtual ~CRedGuiGradientTheme();

		virtual void DrawPanel( const CRedGuiControl* control );
		virtual void DrawButton( const CRedGuiControl* control );
		virtual void DrawLabel( const CRedGuiControl* control );
		virtual void DrawImage( const CRedGuiControl* control );
		virtual void DrawLine( const CRedGuiControl* control );

		virtual void DrawRawLine( const Vector2& startPoint, const Vector2& endPoint, const Color& color, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawFilledRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawCircle( const Vector2& centre, Float radius, const Color& color, Uint32 segments = CIRCLE_DEFAULT_SEGMENTS, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawText( const Vector2& position, const String& text, const Color& color, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawImage( const Vector2& position, const Vector2& size, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha = false );

		virtual void DrawRawRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false );
		virtual void DrawRawFilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false );

		virtual void SetCroppedParent( const CRedGuiCroppedRect* rect );
		virtual void ResetCroppedParent();

		void DrawBorder( const CRedGuiControl* control, const Color& borderColor, Bool ignoreGlobalAlpha = false );

	private:
		Color DarkerColor( const Color& color );

		CRedGuiGraphicContext*	m_graphicContex;	//!<
		Color					m_highlight;		//!<
		Color					m_pushed;			//!<
		Color					m_disabled;			//!<
		Color					m_border;			//!<
		Color					m_disabledBorder;	//!<
		Color					m_keyFocus;			//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
