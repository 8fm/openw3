/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI

#include "redGuiClipmapVisualizer.h"
#include "clipMap.h"

namespace RedGui
{

	CRedGuiClipmapVisualizer::CRedGuiClipmapVisualizer( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
	{
		Reset();
	}

	CRedGuiClipmapVisualizer::~CRedGuiClipmapVisualizer()
	{
	}

	void CRedGuiClipmapVisualizer::SetTileColor( Uint32 x, Uint32 y, const Color& border, const Color& fill )
	{
		RED_ASSERT( x < m_tilesPerEdge && y < m_tilesPerEdge );
		if ( x < m_tilesPerEdge && y < m_tilesPerEdge )
		{
			TileData& tile = m_tiles[x + y * m_tilesPerEdge];
			tile.m_borderColor	= border;
			tile.m_fillColor	= fill;
		}
	}

	void CRedGuiClipmapVisualizer::SetNormalizedViewPosition( const Vector2& pos )
	{
		m_viewPosition = pos;
	}


	void CRedGuiClipmapVisualizer::SetClipmapParameters( const SClipmapParameters& params )
	{
		m_clipmapSize	= params.clipmapSize;
		m_clipSize		= params.clipSize;
		m_tileRes		= params.tileRes;
		m_tilesPerEdge	= params.clipmapSize / params.tileRes;

		m_tiles.Resize( m_tilesPerEdge * m_tilesPerEdge );

		Uint32 numLevels = CClipMap::ComputeNumberOfClipmapStackLevels( params.clipmapSize, params.clipSize );
		m_clipmapWindows.Resize( numLevels );
	}


	void CRedGuiClipmapVisualizer::SetClipWindow( Uint32 level, const Rect& rect )
	{
		RED_ASSERT( level < m_clipmapWindows.Size() );
		if ( level < m_clipmapWindows.Size() )
		{
			m_clipmapWindows[level] = rect;
		}
	}


	void CRedGuiClipmapVisualizer::Reset()
	{
		m_clipmapSize	= 0;
		m_clipSize		= 0;
		m_tileRes		= 0;
		m_tilesPerEdge	= 0;

		m_viewPosition	= Vector2( 0.0f, 0.0f );

		m_tiles.Clear();
		m_clipmapWindows.Clear();
	}


	void CRedGuiClipmapVisualizer::Draw()
	{
		GetTheme()->DrawPanel( this );

		// If we don't have proper data, we don't draw anything. Probably there's no terrain or something.
		if ( m_clipmapSize == 0 || m_clipSize == 0 || m_tileRes == 0 || m_tilesPerEdge == 0 || m_tiles.Empty() )
		{
			return;
		}

		GetTheme()->SetCroppedParent( this );

		Color gridColour( 255, 255, 255, 128 );

		Vector2 basePos( (Float)GetAbsoluteLeft(), (Float)GetAbsoluteTop() );

		Float xPerGrid = (Float)GetWidth() / (Float)m_tilesPerEdge;
		Float yPerGrid = (Float)GetHeight() / (Float)m_tilesPerEdge;
		for ( Uint32 i = 0; i <= m_tilesPerEdge; ++i )
		{
			GetTheme()->DrawRawLine( basePos + Vector2( 0, yPerGrid * i ), basePos + Vector2( (Float)GetWidth(), yPerGrid * i ), gridColour );
			GetTheme()->DrawRawLine( basePos + Vector2( xPerGrid * i, 0 ), basePos + Vector2( xPerGrid * i, (Float)GetHeight() ), gridColour );
		}


		m_tileRects.ClearFast();
		m_tileFillColors.ClearFast();
		m_tileBorderColors.ClearFast();

		for ( Uint32 y = 0; y < m_tilesPerEdge; ++y )
		{
			for ( Uint32 x = 0; x < m_tilesPerEdge; ++x )
			{
				AddTileRect( x, y, m_tileRects, m_tileFillColors, m_tileBorderColors );
			}
		}
		GetTheme()->DrawRawFilledRectangles( m_tileRects.Size(), m_tileRects.TypedData(), m_tileFillColors.TypedData() );
		GetTheme()->DrawRawRectangles( m_tileRects.Size(), m_tileRects.TypedData(), m_tileBorderColors.TypedData() );


		Int32 viewX = static_cast< Int32 >( m_viewPosition.X * m_clipmapSize );
		Int32 viewY = static_cast< Int32 >( m_viewPosition.Y * m_clipmapSize );
		Vector2 v = TerrainToControl( viewX, viewY );
		GetTheme()->DrawRawCircle( basePos + v, 5, Color( 255, 255, 255 ), 10 );


		Vector2 ctrlSize( (Float)GetWidth(), (Float)GetHeight() );

		for ( Uint32 i = 0; i < m_clipmapWindows.Size(); ++i )
		{
			const Rect& rect = m_clipmapWindows[i];

			Vector2 boxSize(
				ctrlSize.X * rect.Width() / (Float)m_clipmapSize,
				ctrlSize.Y * rect.Height() / (Float)m_clipmapSize );

			// (left,bottom) because the terrain grid is flipped.
			Vector2 windowTL = basePos + TerrainToControl( rect.m_left, rect.m_bottom );

			GetTheme()->DrawRawRectangle( windowTL, boxSize, Color( 255, 255, 255 ) );
		}

		GetTheme()->ResetCroppedParent();
	}


	void CRedGuiClipmapVisualizer::AddTileRect( Uint32 x, Uint32 y, TDynArray< Rect >& rects, TDynArray< Color >& fillColors, TDynArray< Color >& borderColors )
	{
		RED_ASSERT( x < m_tilesPerEdge && y < m_tilesPerEdge );

		const TileData& tile = m_tiles[x + y * m_tilesPerEdge];

		// If both colors are totally transparent, don't need this one at all.
		if ( tile.m_borderColor.A == 0 && tile.m_fillColor.A == 0 )
		{
			return;
		}

		Vector2 basePos( (Float)GetAbsoluteLeft(), (Float)GetAbsoluteTop() );

		Float xPerGrid = (Float)GetWidth() / (Float)m_tilesPerEdge;
		Float yPerGrid = (Float)GetHeight() / (Float)m_tilesPerEdge;

		Uint32 u = x;
		Uint32 v = m_tilesPerEdge - y - 1;

		Int32 left   = (Int32)( xPerGrid * u + 2 );
		Int32 top    = (Int32)( yPerGrid * v + 2 );
		Int32 right  = (Int32)( xPerGrid * ( u + 1 ) - 2 );
		Int32 bottom = (Int32)( yPerGrid * ( v + 1 ) - 2 );

		Rect rect;
		rect.m_left		= ( Int32 )basePos.X + left;
		rect.m_right	= ( Int32 )basePos.X + right;
		rect.m_top		= ( Int32 )basePos.Y + top;
		rect.m_bottom	= ( Int32 )basePos.Y + bottom;

		rects.PushBack( rect );
		fillColors.PushBack( tile.m_fillColor );
		borderColors.PushBack( tile.m_borderColor );
	}


	Vector2 CRedGuiClipmapVisualizer::TerrainToControl( Int32 u, Int32 v ) const
	{
		Float x = u / (Float)m_clipmapSize;
		Float y = 1.0f - v / (Float)m_clipmapSize;
		return Vector2( GetWidth() * x, GetHeight() * y );
	}


}	// namespace RedGui

#endif	// NO_RED_GUI
