/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGraphicContext.h"
#include "fonts.h"
#include "renderFrame.h"
#include "renderVertices.h"
#include "redGuiManager.h"
#include "../core/configVarSystem.h"

namespace RedGui
{
	CRedGuiGraphicContext::CRedGuiGraphicContext() 
		: m_renderFrame( nullptr )
		, m_globalAlphaValue( 127 )
	{
		ResetClip();
	}

	CRedGuiGraphicContext::~CRedGuiGraphicContext()
	{
		/* intentionally empty */
	}

	void CRedGuiGraphicContext::SetRenderTarget( CRenderFrame* renderFrame )
	{
		m_renderFrame = renderFrame;
		ResetClip();
	}

	CRenderFrame* CRedGuiGraphicContext::GetRenderTarget() const
	{
		return m_renderFrame;
	}

	void CRedGuiGraphicContext::ResetClip()
	{
		m_clipBox.Min = Vector2( 0, 0 );
		if(m_renderFrame != nullptr)
		{
			m_clipBox.Max = Vector2( (Float)m_renderFrame->GetFrameOverlayInfo().m_width, (Float)m_renderFrame->GetFrameOverlayInfo().m_height );
		}
		else
		{
			m_clipBox.Max = m_clipBox.Min;
		}
		m_clipStack.Clear();
	}

	void CRedGuiGraphicContext::OpenClip( const Box2& clipBox )
	{
		// x2 and y2 are width and height
		// before clip translate these values to coordinates
		Float coordMaxX = clipBox.Max.X + clipBox.Min.X;
		Float coordMaxY = clipBox.Max.Y + clipBox.Min.Y;

		m_clipStack.PushBack(  m_clipBox );
		m_clipBox.Min.X = Max<Float>( m_clipBox.Min.X, clipBox.Min.X );
		m_clipBox.Min.Y = Max<Float>( m_clipBox.Min.Y, clipBox.Min.Y );
		m_clipBox.Max.X = Min<Float>( m_clipBox.Max.X, coordMaxX );
		m_clipBox.Max.Y = Min<Float>( m_clipBox.Max.Y, coordMaxY );
	}

	void CRedGuiGraphicContext::CloseClip()
	{
		m_clipBox = m_clipStack.PopBack();
	}

	Bool CRedGuiGraphicContext::ClipLine( Int32& x1, Int32& y1, Int32& x2, Int32& y2 )
	{
		// Check if both coordinates are outside the clipping box
		if ( ( x1 < m_clipBox.Min.X && x2 < m_clipBox.Min.X ) ||
			 ( y1 < m_clipBox.Min.Y && y2 < m_clipBox.Min.Y ) ||
			 ( x1 > m_clipBox.Max.X && x2 > m_clipBox.Max.X ) ||
			 ( y1 > m_clipBox.Max.Y && y2 > m_clipBox.Max.Y ) ) return false;

		// Calculate slopes for verticals using floating point versions of the coordinates
		Float fx1 = (Float)x1, fy1 = (Float)y1, fx2 = (Float)x2, fy2 = (Float)y2;
		Float slope = fy2 != fy1 ? (fx2 - fx1)/(fy2 - fy1) : 0.0f;

		// Clip at top
		if ( fy1 < m_clipBox.Min.Y )
		{
			fx1 += slope*(m_clipBox.Min.Y - fy1);
			fy1 = m_clipBox.Min.Y;
		}
		if ( fy2 < m_clipBox.Min.Y )
		{
			fx2 += slope*(m_clipBox.Min.Y - fy2);
			fy2 = m_clipBox.Min.Y;
		}

		// Clip at bottom
		if ( fy2 > m_clipBox.Max.Y )
		{
			fx2 -= slope*(fy2 - m_clipBox.Max.Y);
			fy2 = m_clipBox.Max.Y;
		}
		if ( fy1 > m_clipBox.Max.Y )
		{
			fx1 -= slope*(fy1 - m_clipBox.Max.Y);
			fy1 = m_clipBox.Max.Y;
		}

		// Slope for horizontals
		slope = fx2 != fx1 ? (fy2 - fy1)/(fx2 - fx1) : 0.0f;

		// Clip at left
		if ( fx1 < m_clipBox.Min.X )
		{
			fy1 += slope*(m_clipBox.Min.X - fx1);
			fx1 = m_clipBox.Min.X;
		}
		if ( fx2 < m_clipBox.Min.X )
		{
			fy2 += slope*(m_clipBox.Min.X - fx2);
			fx2 = m_clipBox.Min.X;
		}

		// Clip at right
		if ( fx2 > m_clipBox.Max.X )
		{
			fy2 -= slope*(fx2 - m_clipBox.Max.X);
			fx2 = m_clipBox.Max.X;
		}
		if ( fx1 > m_clipBox.Max.X )
		{
			fy1 -= slope*(fx1 - m_clipBox.Max.X);
			fx1 = m_clipBox.Max.X;
		}

		// Make ints again
		x1 = (Int32)Red::Math::MRound( fx1 );
		y1 = (Int32)Red::Math::MRound( fy1 );
		x2 = (Int32)Red::Math::MRound( fx2 );
		y2 = (Int32)Red::Math::MRound( fy2 );


		return true;
	}

	Vector2 CRedGuiGraphicContext::TextSize( const String& text, CFont* font  )
	{
		// Calculate text dimensions
		Uint32 textWidth = 0, textHeight = 0;
		int unusedX, unusedY;
		font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

		return Vector2((Float)textWidth, (Float)textHeight);
	}

	String CRedGuiGraphicContext::ClipText( Int32& x, Int32& y, const String& text, CFont* font, Uint32& deltaFromLeft )
	{
		Vector2 originalTextSize = TextSize(text, font);

		// clip by y coordinate - bottom side
		if(y + (Int32)originalTextSize.Y > m_clipBox.Max.Y)
		{
			return String::EMPTY;
		}
		// clip by y coordinate - top side
		if(y < m_clipBox.Min.Y)
		{
			return String::EMPTY;
		}

		if( x > m_clipBox.Max.X )
		{
			return String::EMPTY;
		}

		String clippedString = text;
		Int32 textLength = text.GetLength();
		Int32 leftDelta = (Int32)m_clipBox.Min.X - x;
		Int32 rightDelta = ( x + (Int32)originalTextSize.X ) - (Int32)m_clipBox.Max.X;
		Int32 leftIndex = 0;
		Int32 rightIndex = 0;


		// clip by x coordinate - left side
		if( leftDelta > 0 )
		{
			if( leftDelta > (Int32)originalTextSize.X )
			{
				leftIndex = textLength;
			}
			else
			{
				Uint32 textCount = text.GetLength();
				Uint32 halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
				String lhs( text.LeftString( halfTextCount ) );
				String rhs( text.RightString( textCount - halfTextCount ) );
				Int32 lhsSize = (Int32)TextSize( lhs, font ).X;
				Int32 rhsSize = (Int32)TextSize( rhs, font ).X;

				while( textCount > 1 )
				{
					if( lhsSize > leftDelta )
				{
						textCount = lhs.GetLength();
						halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
						rhs.Set( lhs.RightString( textCount - halfTextCount ).AsChar() );
						lhs.Set( lhs.LeftString( halfTextCount ).AsChar() );
					}
					else
					{
						textCount = rhs.GetLength();
						halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
						String temp( rhs.LeftString( halfTextCount ) );
						lhs.Append( temp.AsChar(), temp.GetLength() );
						rhs.Set( rhs.RightString( textCount - halfTextCount ).AsChar() );
					}

					lhsSize = (Int32)TextSize( lhs, font ).X;
					rhsSize = (Int32)TextSize( rhs, font ).X;
				}
				leftIndex = lhs.GetLength();
				deltaFromLeft = lhsSize;
			}
		}

		// clip by x coordinate - right side
		if( rightDelta > 0 )
		{
			if( rightDelta > (Int32)originalTextSize.X )
			{
				rightIndex = textLength;
			}
			else
			{
				Uint32 textCount = text.GetLength();
				Uint32 halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
				String lhs( text.LeftString( halfTextCount ) );
				String rhs( text.RightString( textCount - halfTextCount ) );
				Int32 lhsSize = (Int32)TextSize( lhs, font ).X;
				Int32 rhsSize = (Int32)TextSize( rhs, font ).X;

				while( textCount > 1 )
				{
					if( rhsSize > rightDelta )
				{
						textCount = rhs.GetLength();
						halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
						lhs.Set( rhs.LeftString( halfTextCount ).AsChar() );
						rhs.Set( rhs.RightString( textCount - halfTextCount ).AsChar() );
					}
					else
					{
						textCount = lhs.GetLength();
						halfTextCount = ( Uint32 )Red::Math::MCeil( textCount / 2.0f );
						String temp( rhs );
						rhs.Set( lhs.RightString( halfTextCount ).AsChar() );
						rhs.Append( temp.AsChar(), temp.GetLength() );
						lhs.Set( lhs.LeftString( textCount - halfTextCount ).AsChar() );
					}

					lhsSize = (Int32)TextSize( lhs, font ).X;
					rhsSize = (Int32)TextSize( rhs, font ).X;
				}
				rightIndex = rhs.GetLength();
			}
		}

		Uint32 letterCount = text.GetLength() - ( rightIndex + leftIndex );

		if( letterCount > 0 )
		{
			return text.MidString( leftIndex, letterCount );
		}
		
		return String::EMPTY;
	}

	void CRedGuiGraphicContext::Line( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( !ClipLine( x1, y1, x2, y2 ) )
		{
			return;
		}

		Vector2 point1( static_cast< Float >( x1 ), static_cast< Float >( y1 ) );
		Vector2 point2( static_cast< Float >( x2 ), static_cast< Float >( y2 ) );

		m_renderFrame->AddDebugLineOnScreen( point1, point2, ModifyAlpha( color, ignoreGlobalAlpha ) );
	}

	void CRedGuiGraphicContext::Rectangle( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		/*
		 * Alpha in the color is modified in the Line function
		 */
		
		Int32 bottom = y + height;
		Int32 right = x + width;

		// Top
		Line( x, y, right, y, color, ignoreGlobalAlpha );

		// Bottom
		Line( x, bottom, right, bottom, color, ignoreGlobalAlpha );

		// Left
		Line( x, y, x, bottom, color, ignoreGlobalAlpha );

		// Right
		Line( right, y, right, bottom, color, ignoreGlobalAlpha );
	}

	void CRedGuiGraphicContext::FilledRectangle( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if( ClipRectangle(x, y, width, height) == false )
		{
			return;
		}

		m_renderFrame->AddDebugRect(x, y, width, height, ModifyAlpha( color1, ignoreGlobalAlpha ) );
	}

	void CRedGuiGraphicContext::LineAdvancedColor( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( !ClipLine( x1, y1, x2, y2 ) )
		{
			return;
		}

		Vector2 point1( static_cast< Float >( x1 ), static_cast< Float >( y1 ) );
		Vector2 point2( static_cast< Float >( x2 ), static_cast< Float >( y2 ) );

		m_renderFrame->AddDebugGradientLineOnScreen( point1, point2, ModifyAlpha( color1, ignoreGlobalAlpha ), ModifyAlpha( color2, ignoreGlobalAlpha ) );
	}

	void CRedGuiGraphicContext::RectangleAdvancedColor( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha /*= false*/ )
	{
		/*
		 * Alpha in the color is modified in the Line function
		 */

		Int32 bottom = y + height;
		Int32 right = x + width;

		// Top
		Line( x, y, right, y, color1, ignoreGlobalAlpha );

		// Bottom
		Line( x, bottom, right, bottom, color2, ignoreGlobalAlpha );

		// Left
		LineAdvancedColor( x, y, x, bottom, color1, color2, ignoreGlobalAlpha );

		// Right
		LineAdvancedColor( right, y, right, bottom, color1, color2, ignoreGlobalAlpha );
	}

	void CRedGuiGraphicContext::FilledRectangleAdvancedColor( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if( ClipRectangle(x, y, width, height) == false )
		{
			return;
		}

		m_renderFrame->AddDebugGradientRect(x, y, width, height, ModifyAlpha( color1, ignoreGlobalAlpha ), ModifyAlpha( color2, ignoreGlobalAlpha ) );
	}

	void CRedGuiGraphicContext::Rectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		// For convenience, can pass this to RectanglesAdvanced, just use the same colors for both. Will be slightly less efficient than
		// handling it separately, but is much simpler, less code duplication, etc. And should still be fast enough for redgui.
		RectanglesAdvancedColor( numRectangles, rectangles, colors, colors, ignoreGlobalAlpha );
	}

	void CRedGuiGraphicContext::FilledRectangles( Uint32 numRectangles, const Rect* rectangles, const Color* colors, Bool ignoreGlobalAlpha /*= false*/ )
	{
		// For convenience, can pass this to FilledRectanglesAdvanced, just use the same colors for both. Will be slightly less efficient than
		// handling it separately, but is much simpler, less code duplication, etc. And should still be fast enough for redgui.
		FilledRectanglesAdvancedColor( numRectangles, rectangles, colors, colors, ignoreGlobalAlpha );
	}

	void CRedGuiGraphicContext::RectanglesAdvancedColor( Uint32 numRectangles, const Rect* rectangles, const Color* colors1, const Color* colors2, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( numRectangles == 0 )
		{
			return;
		}

		RED_ASSERT( rectangles != nullptr && colors1 != nullptr && colors2 != nullptr );

		// Build a list of all lines to draw. Each line needs to be clipped.
		TDynArray< DebugVertex > points;
		points.Reserve( numRectangles * 8 );
		for ( size_t i = 0; i < numRectangles; ++i )
		{
			Color c1 = ModifyAlpha( colors1[i], ignoreGlobalAlpha );
			Color c2 = ModifyAlpha( colors2[i], ignoreGlobalAlpha );

			// Top
			{
				Int32 x1 = rectangles[i].m_left;
				Int32 y1 = rectangles[i].m_top;
				Int32 x2 = rectangles[i].m_right;
				Int32 y2 = rectangles[i].m_top;

				if ( ClipLine( x1, y1, x2, y2 ) )
				{
					points.PushBack( DebugVertex( Vector( (Float)x1, (Float)y1, 0.5f ), c1 ) );
					points.PushBack( DebugVertex( Vector( (Float)x2, (Float)y2, 0.5f ), c1 ) );
				}
			}
			// Bottom
			{
				Int32 x1 = rectangles[i].m_left;
				Int32 y1 = rectangles[i].m_bottom;
				Int32 x2 = rectangles[i].m_right;
				Int32 y2 = rectangles[i].m_bottom;

				if ( ClipLine( x1, y1, x2, y2 ) )
				{
					points.PushBack( DebugVertex( Vector( (Float)x1, (Float)y1, 0.5f ), c2 ) );
					points.PushBack( DebugVertex( Vector( (Float)x2, (Float)y2, 0.5f ), c2 ) );
				}
			}
			// Left
			{
				Int32 x1 = rectangles[i].m_left;
				Int32 y1 = rectangles[i].m_top;
				Int32 x2 = rectangles[i].m_left;
				Int32 y2 = rectangles[i].m_bottom;

				if ( ClipLine( x1, y1, x2, y2 ) )
				{
					points.PushBack( DebugVertex( Vector( (Float)x1, (Float)y1, 0.5f ), c1 ) );
					points.PushBack( DebugVertex( Vector( (Float)x2, (Float)y2, 0.5f ), c2 ) );
				}
			}
			// Right
			{
				Int32 x1 = rectangles[i].m_right;
				Int32 y1 = rectangles[i].m_top;
				Int32 x2 = rectangles[i].m_right;
				Int32 y2 = rectangles[i].m_bottom;

				if ( ClipLine( x1, y1, x2, y2 ) )
				{
					points.PushBack( DebugVertex( Vector( (Float)x1, (Float)y1, 0.5f ), c1 ) );
					points.PushBack( DebugVertex( Vector( (Float)x2, (Float)y2, 0.5f ), c2 ) );
				}
			}
		}

		// If we did generate some lines, draw them!
		if ( !points.Empty() )
		{
			m_renderFrame->AddDebugLinesOnScreen( points.TypedData(), points.Size() );
		}
	}

	void CRedGuiGraphicContext::FilledRectanglesAdvancedColor( Uint32 numRectangles, const Rect* rectangles, const Color* colors1, const Color* colors2, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if ( numRectangles == 0 )
		{
			return;
		}

		RED_ASSERT( rectangles != nullptr && colors1 != nullptr && colors2 != nullptr );

		// Build lists of clipped rectangles and adjusted colors. We can't just use the input colors, because we might need to modify alpha.
		TDynArray< Rect > clippedRects;
		TDynArray< Color > cols1;
		TDynArray< Color > cols2;
		clippedRects.Reserve( numRectangles );
		cols1.Reserve( numRectangles );
		cols2.Reserve( numRectangles );
		for ( size_t i = 0; i < numRectangles; ++i )
		{
			Int32 x = rectangles[i].m_left;
			Int32 y = rectangles[i].m_top;
			Int32 width = rectangles[i].Width();
			Int32 height = rectangles[i].Height();
			if( !ClipRectangle(x, y, width, height) )
			{
				continue;
			}

			clippedRects.PushBack( Rect( x, x + width, y, y + height ) );
			cols1.PushBack( ModifyAlpha( colors1[i], ignoreGlobalAlpha ) );
			cols2.PushBack( ModifyAlpha( colors2[i], ignoreGlobalAlpha ) );
		}

		if ( !clippedRects.Empty() )
		{
			m_renderFrame->AddDebugGradientRects( clippedRects.Size(), clippedRects.TypedData(), cols1.TypedData(), cols2.TypedData() );
		}
	}


	void CRedGuiGraphicContext::TextOut( Int32 x, Int32 y, const String& text, const Color& color, CFont* font, Bool ignoreGlobalAlpha /*= false*/ )
	{
		Uint32 deltaFromLeft = 0;
		String newText = ClipText(x, y, text, font, deltaFromLeft );
		Vector2 textSize = TextSize( newText, font );
		m_renderFrame->AddDebugScreenText( deltaFromLeft+x, y+(Uint32)textSize.Y, newText, ModifyAlpha( color, ignoreGlobalAlpha ), font );
	}

	void CRedGuiGraphicContext::DrawImage( Int32 x, Int32 y, Int32 width, Int32 height, const CBitmapTexture* image, const Color& color, Bool ignoreGlobalAlpha /*= false*/ )
	{
		if( ClipRectangle(x, y, width, height) == false )
		{
			return;
		}

		// remove word 'const' because AddDebugTexturedRect function has a parameter without const modifier
		CBitmapTexture* freeImage = const_cast<CBitmapTexture*>(image);
		m_renderFrame->AddDebugTexturedRect(x, y, width, height, freeImage, ModifyAlpha( color, ignoreGlobalAlpha ) );
	}

	void CRedGuiGraphicContext::SetGlobalAlpha( Uint32 value )
	{
		m_globalAlphaValue = Clamp<Uint32>(value, 0, 255);
		SConfig::GetInstance().Save();
	}

	Uint32 CRedGuiGraphicContext::GetGlobalAlpha() const
	{
		return m_globalAlphaValue;
	}

	Color CRedGuiGraphicContext::ModifyAlpha( const Color& color, Bool ignoreGlobalAlpha )
	{
		Uint8 alpha = ( ignoreGlobalAlpha == true ) ? (Uint8)color.A : (Uint8)( (Float)color.A * (Float)( m_globalAlphaValue/255.0f ) );
		return Color(color.R, color.G, color.B, alpha );
	}

	Bool CRedGuiGraphicContext::ClipRectangle( Int32& x1, Int32& y1, Int32& width, Int32& height )
	{
		Int32 x2 = x1 + width;
		Int32 y2 = y1 + height;

		x1 = Max( x1, (Int32)m_clipBox.Min.X );
		y1 = Max( y1, (Int32)m_clipBox.Min.Y );
		x2 = Min( x2, (Int32)m_clipBox.Max.X );
		y2 = Min( y2, (Int32)m_clipBox.Max.Y );

		width = x2 - x1;
		height = y2 - y1;

		if(width < 0 || height < 0)
		{
			return false;
		}

		return true;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
