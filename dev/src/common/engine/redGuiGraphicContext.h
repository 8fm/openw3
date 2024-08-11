/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

class CRenderFrame;

namespace RedGui
{
	class CRedGuiGraphicContext
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiInternals );
	public:
		CRedGuiGraphicContext();
		virtual ~CRedGuiGraphicContext();

		// manage clipping areas
		void ResetClip();
		void OpenClip( const Box2& clipBox );
		void CloseClip();

		//
		void SetGlobalAlpha(Uint32 value);
		//
		Uint32 GetGlobalAlpha() const;

		// set new render frame
		void SetRenderTarget( CRenderFrame* renderFrame );
		// get stored render frame
		CRenderFrame* GetRenderTarget() const;

		// simple shapes
		void Line( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const Color& color1, Bool ignoreGlobalAlpha = false );
		void Rectangle( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color, Bool ignoreGlobalAlpha = false );
		void FilledRectangle( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color, Bool ignoreGlobalAlpha = false );
		void Rectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false );
		void FilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha = false );

		// advanced color shapes
		void LineAdvancedColor( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha = false );
		void RectangleAdvancedColor( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha = false );
		void FilledRectangleAdvancedColor( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha = false );
		void RectanglesAdvancedColor( Uint32 numRectangles, const Rect* rectangles, const Color* colors1, const Color* colors2, Bool ignoreGlobalAlpha = false );
		void FilledRectanglesAdvancedColor( Uint32 numRectangles, const Rect* rectangles, const Color* colors1, const Color* colors2, Bool ignoreGlobalAlpha = false );

		// text
		void TextOut( Int32 x, Int32 y, const String& text, const Color& color, CFont* font, Bool ignoreGlobalAlpha = false );
		Vector2 TextSize( const String& text, CFont* font  );

		// image
		void DrawImage( Int32 x1, Int32 y1, Int32 width, Int32 height, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha = false );

	private:
		// clipping functions
		Bool ClipLine( Int32& x1, Int32& y1, Int32& width, Int32& height );
		Bool ClipRectangle( Int32& x1, Int32& y1, Int32& width, Int32& height );
		String ClipText(Int32& x, Int32& y, const String& text, CFont* font, Uint32& deltaFromLeft );

		// modify alpha value from controls to global value
		Color ModifyAlpha( const Color& color, Bool ignoreGlobalAlpha );

		TDynArray<Box2, MC_RedGuiInternals>		m_clipStack;			//!<
		Box2									m_clipBox;				//!<
		CRenderFrame*							m_renderFrame;			//!<

		Uint32									m_globalAlphaValue;		//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
