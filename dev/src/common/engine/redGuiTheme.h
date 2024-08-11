/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"

namespace RedGui
{
	class IRedGuiTheme
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiThemes );
	public:
		static const Uint32 CIRCLE_DEFAULT_SEGMENTS = 20;

	public:
		virtual ~IRedGuiTheme() { /* intentionally empty */ }

		virtual void DrawPanel( const CRedGuiControl* control ) = 0;
		virtual void DrawButton( const CRedGuiControl* control ) = 0;
		virtual void DrawLabel( const CRedGuiControl* control ) = 0;
		virtual void DrawImage( const CRedGuiControl* control ) = 0;
		virtual void DrawLine( const CRedGuiControl* control ) = 0;

		virtual void DrawRawLine( const Vector2& startPoint, const Vector2& endPoint, const Color& color, Bool ignoreGlobalAlpha = false  ) = 0;
		virtual void DrawRawRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha = false  ) = 0;
		virtual void DrawRawFilledRectangle( const Vector2& leftTopCorner, const Vector2& size, const Color& color, Bool ignoreGlobalAlpha = false  ) = 0;
		virtual void DrawRawCircle( const Vector2& centre, Float radius, const Color& color, Uint32 segments = CIRCLE_DEFAULT_SEGMENTS, Bool ignoreGlobalAlpha = false  ) = 0;
		virtual void DrawRawText( const Vector2& position, const String& text, const Color& color, Bool ignoreGlobalAlpha = false  ) = 0;
		virtual void DrawRawImage( const Vector2& position, const Vector2& size, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha = false  ) = 0;

		virtual void DrawRawRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false ) = 0;
		virtual void DrawRawFilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false ) = 0;

		virtual void SetCroppedParent( const CRedGuiCroppedRect* rect ) = 0;
		virtual void ResetCroppedParent() = 0;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
