#include "../../common/core/mathUtils.h"
/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

//------------------------------------------------------------------------------------------------------------------
// Grid crawler - CCW
//------------------------------------------------------------------------------------------------------------------
CCrowdSpace_Grid::SGridCrawlerCCW::SGridCrawlerCCW( Int32 x, Int32 y, Uint32 size /* = NUM_GRID_CELLS_XY */ ) 
	: SGridCrawler( x, y )
	, m_cellLimit( size * size )
	, m_currCell( 0 )
	, m_dir( DOWN )
	, m_dirLimit( 1 )
	, m_currDirLimit( 1 )
{
	#ifdef DEBUG_CROWD
		ASSERT( size > 0 );
	#endif
}

RED_INLINE void CCrowdSpace_Grid::SGridCrawlerCCW::Next()
{
	//#define DBG_CRAWLER_CCW

	do 
	{
		++m_currCell;
		if ( !IsValid() )
		{
			#ifdef DBG_CRAWLER_CCW
				RED_LOG( Crowd, TXT("crawler: done.") );
			#endif
			return; // end crawling
		}

		// move caret
		if ( m_dir == DOWN )		--Y;
		else if ( m_dir == RIGHT )	++X;
		else if ( m_dir == UP )		++Y;
		else if ( m_dir == LEFT )	--X;

		// need to change dir?
		#ifdef DEBUG_CROWD
			ASSERT( m_currDirLimit > 0 );
		#endif
		--m_currDirLimit;
		
		if ( m_currDirLimit == 0 )
		{
			// renew the limit
			m_currDirLimit = m_dirLimit;

			// increase the limit for the next time?
			if ( m_dir == DOWN || m_dir == UP )
			{
				++m_dirLimit;
			}

			// finally, change the dir
			m_dir = NextDir( m_dir );
		}

		#ifdef DBG_CRAWLER_CCW
			RED_LOG( Crowd, TXT("crawler: cell %ld @ (%ld, %ld) is %svalid."), m_currCell, X, Y, IsValidCell() ? TXT("") : TXT("in") ); 
		#endif
	}
	while ( !IsValidCell() ); // skip all cells that are out of bounds

}

//------------------------------------------------------------------------------------------------------------------
// Grid crawler - in order
//------------------------------------------------------------------------------------------------------------------
CCrowdSpace_Grid::SGridCrawlerInOrder::SGridCrawlerInOrder( Int32 x, Int32 y, Int32 sizeX, Int32 sizeY ) 
	: SGridCrawler( x, y )
	, m_startX( x )
	, m_startY( y )
	, m_sizeX( sizeX )
	, m_sizeY( sizeY )
{
	#ifdef DEBUG_CROWD
		ASSERT( sizeX > 0 && sizeY > 0 );
	#endif
}

RED_INLINE CCrowdSpace_Grid::SGridCrawlerInOrder::SGridCrawlerInOrder( const Box2& relativeBox )
{
	#ifdef DEBUG_CROWD
		ASSERT( relativeBox.Min.X >= 0.f && relativeBox.Min.Y >= 0, 
			TXT("CCrowdSpace_Grid::SGridCrawlerInOrder::SGridCrawlerInOrder(): relativeBox must be in grid space.") );

		ASSERT( relativeBox.Max.X > relativeBox.Min.X && relativeBox.Max.Y > relativeBox.Min.Y, 
			TXT("CCrowdSpace_Grid::SGridCrawlerInOrder::SGridCrawlerInOrder(): relativeBox must be non-zero.") );
	#endif

	const Vector2 vecSize = relativeBox.Max - relativeBox.Min;

	m_startX = X = Clamp< Int32 > ( Int32( relativeBox.Min.X / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 
	m_startY = Y = Clamp< Int32 > ( Int32( relativeBox.Min.Y / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 
	m_sizeX = Clamp< Int32 > ( Int32( vecSize.X / GRID_CELL_SIZE ), 1, NUM_GRID_CELLS_XY ); 
	m_sizeY = Clamp< Int32 > ( Int32( vecSize.Y / GRID_CELL_SIZE ), 1, NUM_GRID_CELLS_XY ); 
}

RED_INLINE void CCrowdSpace_Grid::SGridCrawlerInOrder::Next()
{
	//#define DBG_CRAWLER_IO

	do 
	{
		// move caret		
		++X;
		if ( X >= m_startX + m_sizeX )
		{
			++Y;
			X = m_startX;
		}

		// validate
		if ( !IsValid() )
		{
			#ifdef DBG_CRAWLER_IO
				RED_LOG( Crowd, TXT("crawler_io: done.") );
			#endif
			return; // end crawling
		}

		#ifdef DBG_CRAWLER_IO
			RED_LOG( Crowd, TXT("crawler_io: cell (%ld, %ld) is %svalid."), X, Y, IsValidCell() ? TXT("") : TXT("in") ); 
		#endif
	}
	while ( !IsValidCell() ); // skip all cells that are out of bounds

}

//------------------------------------------------------------------------------------------------------------------
// Grid crawler - Ray
//------------------------------------------------------------------------------------------------------------------
RED_INLINE CCrowdSpace_Grid::SGridCrawlerRay::SGridCrawlerRay( const SCrowdRay2& ray, const Vector& aabbMin )
	: m_valid( true )
	, m_dirsToCheckLeft( 3 )
{
	#ifdef DEBUG_CROWD
		ASSERT( ray.m_radius <= GRID_CELL_SIZE, TXT("CCrowdSpace_Grid::SGridCrawlerRay doesn't support big radius values for sweep.") );
	#endif

	m_ray.m_start.X = Clamp< Float > ( ray.m_start.X - aabbMin.X, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE );
	m_ray.m_start.Y = Clamp< Float > ( ray.m_start.Y - aabbMin.Y, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE );
	m_ray.m_end.X = Clamp< Float > ( ray.m_end.X - aabbMin.X, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE );
	m_ray.m_end.Y = Clamp< Float > ( ray.m_end.Y - aabbMin.Y, 0.f, NUM_GRID_CELLS_XY * GRID_CELL_SIZE );
	m_ray.m_radius = Clamp< Float > ( ray.m_radius, 0.f, GRID_CELL_SIZE );

	X = m_currX = Clamp< Int32 > ( Int32( m_ray.m_start.X / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 
	Y = m_currY = Clamp< Int32 > ( Int32( m_ray.m_start.Y / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 
	m_endX = Clamp< Int32 > ( Int32( m_ray.m_end.X / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 
	m_endY = Clamp< Int32 > ( Int32( m_ray.m_end.Y / GRID_CELL_SIZE ), 0, NUM_GRID_CELLS_XY - 1 ); 

	// ok, this is only needed when the radius is set
	if ( m_ray.m_radius > 0.f )
	{
		const Int32 xdiff = ( m_endX - m_currX ); 
		const Int32 xdir = ( xdiff == 0 ) ? 0 : ( xdiff < 0 ? -1 : 1 ); 
		const Int32 ydiff = ( m_endY - m_currY ); 
		const Int32 ydir = ( ydiff == 0 ) ? 0 : ( ydiff < 0 ? -1 : 1 ); 

		if ( xdir && IsRayOnCell( m_currX + xdir, m_currY ) )
		{
			m_dir = XDiffToEDir( xdir );
		}
		else if ( ydir && IsRayOnCell( m_currX, m_currY + ydir ))
		{
			m_dir = YDiffToEDir( ydir );
		}
		else if ( xdir && ydir && IsRayOnCell( m_currX + xdir, m_currY + ydir ) )
		{
			// we need to check all directions
			m_dir = DOWN;
			m_dirsToCheckLeft = 4;
		}
		else
		{
			ASSERT( false, TXT("CCrowdSpace_Grid::SGridCrawlerRay::SGridCrawlerRay() implementation is broken. Please FIX.") );
		}
	}
}

RED_INLINE void CCrowdSpace_Grid::SGridCrawlerRay::Next()
{
	//#define DBG_CRAWLER_RAY

	if ( m_ray.m_radius == 0.f )
	{
		do 
		{
			// was it the last cell to check?
			if ( m_endX == m_currX && m_endY == m_currY )
			{
				m_valid = false;

				#ifdef DBG_CRAWLER_RAY
					RED_LOG( CNAME( Crowd ), TXT("crawler_ray: done.") );
				#endif
				return; // end crawling
			}

			// move main caret		
			const Int32 xdiff = ( m_endX - m_currX ); 
			const Int32 xdir = ( xdiff == 0 ) ? 0 : ( xdiff < 0 ? -1 : 1 ); 
			const Int32 ydiff = ( m_endY - m_currY ); 
			const Int32 ydir = ( ydiff == 0 ) ? 0 : ( ydiff < 0 ? -1 : 1 ); 

			if ( xdir && IsRayOnCell( m_currX + xdir, m_currY ) )
			{
				m_currX += xdir;
			}
			else if ( ydir && IsRayOnCell( m_currX, m_currY + ydir ))
			{
				m_currY += ydir;
			}
			else if ( xdir && ydir && IsRayOnCell( m_currX + xdir, m_currY + ydir ) )
			{
				m_currX += xdir;
				m_currY += ydir;
			}
			else
			{
				ASSERT( false, TXT("CCrowdSpace_Grid::SGridCrawlerRay::Next() implementation is broken. Please FIX.") );
			}

			// move "small" caret
			X = m_currX;
			Y = m_currY;

			#ifdef DBG_CRAWLER_RAY
				RED_LOG( Crowd, TXT("crawler_ray: cell (%ld, %ld) is %svalid."), X, Y, IsValidCell() ? TXT("") : TXT("in") ); 
			#endif
		}
		while ( !IsValidCell() ); // skip all cells that are out of bounds
	}
	else 
	// implementation for "sweep" tests below
	// it's too long, too complicated and probably not-that-much-opitimal as i wish...
	// but for now it should be enough
	// TODO: revisit, refactor, optimize...
	{
		// "small" caret turn
		if ( m_dirsToCheckLeft > 0 )
		{
			do 
			{
				Int32 x, y;

				--m_dirsToCheckLeft;

				// change dir
				m_dir = NextDir( m_dir );

				x = m_currX + EDirToXDiff( m_dir );
				y = m_currY + EDirToYDiff( m_dir );

				// see if this cell is good for checking
				if ( !IsRayOnCell( x, y ) && IsRayCloseToCell( x, y ) && x >= 0 && x < NUM_GRID_CELLS_XY && y >= 0 && y < NUM_GRID_CELLS_XY )
				{
					// move small carret
					X = x;
					Y = y;

					// return, we have new cell
					return;
				}
			} 
			// until we're out of moves 
			while ( m_dirsToCheckLeft > 0 );

		}

		// "big" caret turn
		do
		{
			// was it the last cell to check?
			if ( IsLastCell() )
			{
				m_valid = false;

				#ifdef DBG_CRAWLER_RAY
					RED_LOG( Crowd, TXT("crawler_ray: done.") );
				#endif
				return; // end crawling
			}

			// move main caret		
			const Int32 xdiff = ( m_endX - m_currX ); 
			const Int32 xdir = ( xdiff == 0 ) ? 0 : ( xdiff < 0 ? -1 : 1 ); 
			const Int32 ydiff = ( m_endY - m_currY ); 
			const Int32 ydir = ( ydiff == 0 ) ? 0 : ( ydiff < 0 ? -1 : 1 ); 

			if ( xdir && IsRayOnCell( m_currX + xdir, m_currY ) )
			{
				m_currX += xdir;
				m_dir = XDiffToEDir( xdir );
				m_dirsToCheckLeft = 3;
			}
			else if ( ydir && IsRayOnCell( m_currX, m_currY + ydir ))
			{
				m_currY += ydir;
				m_dir = YDiffToEDir( ydiff );
				m_dirsToCheckLeft = 3;
			}
			else if ( xdir && ydir && IsRayOnCell( m_currX + xdir, m_currY + ydir ) )
			{
				m_currX += xdir;
				m_currY += ydir;
				m_dir = DOWN;
				m_dirsToCheckLeft = 4;
			}
			else
			{
				ASSERT( false, TXT("CCrowdSpace_Grid::SGridCrawlerRay::Next() implementation is broken. Please FIX.") );
			}

			// move "small" caret also, to "big" caret position
			X = m_currX;
			Y = m_currY;

			#ifdef DBG_CRAWLER_RAY
				RED_LOG( Crowd, TXT("crawler_ray: cell (%ld, %ld) is %svalid."), X, Y, IsValidCell() ? TXT("") : TXT("in") ); 
			#endif
		}
		while ( !IsValidCell() ); // skip all cells that are out of bounds
	}
}

RED_INLINE Bool CCrowdSpace_Grid::SGridCrawlerRay::IsRayOnCell( Int32 cellX, Int32 cellY ) const
{
	const Vector2 cellMin( Float( cellX ) * GRID_CELL_SIZE + GRID_EPSILON, Float( cellY ) * GRID_CELL_SIZE + GRID_EPSILON );	
	const Vector2 cellMax( cellMin.X + GRID_CELL_SIZE - GRID_EPSILON - GRID_EPSILON, cellMin.Y + GRID_CELL_SIZE - GRID_EPSILON - GRID_EPSILON );
	return MathUtils::GeometryUtils::TestIntersectionLineRectangle2D( m_ray.m_start, m_ray.m_end, cellMin, cellMax );
}

RED_INLINE Bool CCrowdSpace_Grid::SGridCrawlerRay::IsRayCloseToCell( Int32 cellX, Int32 cellY ) const
{
	Vector2 verts[ 4 ], outPoint;
	verts[ 0 ].X = Float( cellX ) * GRID_CELL_SIZE;
	verts[ 0 ].Y = Float( cellY ) * GRID_CELL_SIZE;
	verts[ 1 ].X = verts[ 0 ].X + GRID_CELL_SIZE;
	verts[ 1 ].Y = verts[ 0 ].Y;
	verts[ 2 ].X = verts[ 0 ].X;
	verts[ 2 ].Y = verts[ 0 ].Y + GRID_CELL_SIZE;
	verts[ 3 ].X = verts[ 0 ].X + GRID_CELL_SIZE;
	verts[ 3 ].Y = verts[ 0 ].Y + GRID_CELL_SIZE;

	const Float sqRadius = m_ray.m_radius * m_ray.m_radius;
	for ( Uint32 i = 0; i < 4; ++i )
	{
		if ( MathUtils::GeometryUtils::DistanceSqrPointToLineSeg2D( verts[ i ], m_ray.m_start, m_ray.m_end,	outPoint ) <= sqRadius )
		{
			return true;
		}
	}

	return false;
}

