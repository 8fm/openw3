/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI

#include "redGuiControl.h"


struct SClipmapParameters;


namespace RedGui
{

	class CRedGuiClipmapVisualizer : public RedGui::CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

	private:

		struct TileData
		{
			Color	m_borderColor;
			Color	m_fillColor;
		};


		Uint32					m_clipmapSize;					//!< Total texel size of the full-res clipmap.
		Uint32					m_clipSize;						//!< Size of clip window.
		Uint32					m_tileRes;						//!< Resolution of a single tile.
		Uint32					m_tilesPerEdge;					//!< Number of tiles on each side of the clipmap.

		Vector2					m_viewPosition;					//!< Normalized position of the camera.

		TDynArray< TileData >	m_tiles;						//!< How to draw each of the tiles.

		TDynArray< Rect >		m_clipmapWindows;				//!< Texel-space (level 0) rectangles of each clip window.


		// These are only used for Draw(), but we'll keep them as members so we aren't reallocating basically the same thing every frame.
		TDynArray< Rect >		m_tileRects;
		TDynArray< Color >		m_tileFillColors;
		TDynArray< Color >		m_tileBorderColors;


	public:
		CRedGuiClipmapVisualizer( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
		virtual ~CRedGuiClipmapVisualizer();

		// Setup clipmap configuration. Must be called before the below functions, as it sets up some arrays with appropriate sizes. Fine
		// to call per-frame if desired, since generally the terrain doesn't change so the arrays are already set up with the right size.
		void SetClipmapParameters( const SClipmapParameters& params );

		// Set border and fill color used to draw the given tile.
		void SetTileColor( Uint32 x, Uint32 y, const Color& border, const Color& fill );

		// Set camera position, in normalized terrain coordinates.
		void SetNormalizedViewPosition( const Vector2& pos );

		// Set level 0 texel-space rectangle for the given clip window
		void SetClipWindow( Uint32 level, const Rect& rect );

		// Clear everything. Nothing will be drawn until the visualizer is reconfigured (SetClipmapParameters etc)
		void Reset();

		virtual void Draw();

	private:
		void AddTileRect( Uint32 x, Uint32 y, TDynArray< Rect >& rects, TDynArray< Color >& fillColors, TDynArray< Color >& borderColors );

		// Convert a position in terrain's level0 texel space to a position within this control (not offset by control's position).
		Vector2 TerrainToControl( Int32 u, Int32 v ) const;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
