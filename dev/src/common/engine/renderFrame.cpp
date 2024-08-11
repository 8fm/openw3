/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFrame.h"
#include "../../common/core/curve3DData.h"
#include "../../common/core/gatheredResource.h"
#include "renderFragment.h"
#include "viewport.h"
#include "fonts.h"

IMPLEMENT_RTTI_ENUM( ERenderingSortGroup ); 
IMPLEMENT_RTTI_ENUM( ETransparencySortGroup ); 
IMPLEMENT_RTTI_ENUM( ERenderingBlendMode );


RED_DISABLE_WARNING_MSC( 4995 )

CGatheredResource resOnScreenTextFont( TXT("engine\\fonts\\parachute.w2fnt"), RGF_Startup );

// These have to be defined somewhere other than renderer project, so cooker can pick them up.
// NOTE: moved from renderGampleyEffects.cpp because file was empty and the linker was stripping this symbols
CGatheredResource resGameplayEffectsFocusModeGroups( TXT("gameplay\\globals\\gameplayfx_focusmode_groups.csv"), RGF_Startup );
CGatheredResource resGameplayEffectsOutlineGroups( TXT("gameplay\\globals\\gameplayfx_outline_groups.csv"), RGF_Startup );


// Get a matrix suitable for screen-space rendering. Will be identity for non-normalized, since at render time, the projection
// is set up for pixel coordinates. If normalized, will transform [-1,1] with (-1,-1) in bottom left to pixel coordinates with
// (0,0) in top left.
static Matrix GetOnScreenMatrix( const CRenderFrameInfo& info, Bool normalized )
{
	if ( normalized )
	{
		const Float halfw = (Float)info.m_width / 2.0f;
		const Float halfh = (Float)info.m_height / 2.0f;
		return Matrix(
			Vector( halfw, 0, 0, 0 ),
			Vector( 0, -halfh, 0, 0 ),
			Vector( 0, 0, 1, 0 ),
			Vector( halfw, halfh, 0, 1 )
			);
	}
	else
	{
		return Matrix::IDENTITY;
	}
}


CRenderFrame::CRenderFrame( const CRenderFrameInfo& info, const CRenderFrameOverlayInfo& overlayInfo )
	: m_info( info )
	, m_overlayInfo( overlayInfo )
	, m_renderTarget( info.m_renderTarget )
{
#ifndef NO_EDITOR
	CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();
#endif

	if ( m_renderTarget )
	{
		m_renderTarget->AddRef();
	}

	Red::System::MemoryZero( m_fragments, sizeof(m_fragments) );
	Red::System::MemoryZero( m_fragmentsLast, sizeof(m_fragments) );
}

CRenderFrame::~CRenderFrame()
{
	// Call destructor of each RenderFragment
	for ( Uint32 i=0; i<RSG_Max; i++ )
	{
		for ( IRenderFragment* cur = m_fragments[i]; cur; cur=cur->GetNextBaseFragment() )
		{
			ASSERT( cur->GetFrame() == this );
			cur->~IRenderFragment();
		}
	}

	if ( m_renderTarget )
	{
		m_renderTarget->Release();
	}
}

// Draw debug plane
void CRenderFrame::AddDebugPlane( const Plane& plane, const Matrix& matrix, const Color& color, Int32 gridSize /*= 10*/, Bool overlay /*=false*/ )
{
	Vector v( 1, 0, 0 );
	v = plane.Project( v );
	if ( v.SquareMag3() <= NumericLimits<Float>::Epsilon() )
	{
		v = Vector( 0, 1, 0 );
		v = plane.Project( v );
	}

	Vector v2( 0, 0, 0 );
	v2 = plane.Project( v2 );
	if ( v.SquareMag3() <= NumericLimits<Float>::Epsilon() )
	{
		v2 = Vector( 0, 0, 1 );
		v2 = plane.Project( v2 );
	}

	Vector n = ( v2 - v ).Normalized3();

	Float qSin = MSin( M_PI * 0.25f );
	Float qCos = MCos( M_PI * 0.25f );
	Vector quat( plane.GetVectorRepresentation().X * qSin, plane.GetVectorRepresentation().Y * qSin, plane.GetVectorRepresentation().Z * qSin, qCos );
	Matrix m;
	m.BuildFromQuaternion( quat );
	Vector pv = m.TransformPoint( n ).Normalized3();

	TDynArray< DebugVertex > vertices;
	for ( Float x = -(Float)gridSize + 1; x < (Float)gridSize; ++x )
	{
		for ( Float y = -(Float)gridSize + 1; y < (Float)gridSize; ++y )
		{
			Vector v1 = v + n * x + pv * y ;
			Vector v2 = v + n * ( 1 + x ) + pv * y;
			Vector v3 = v + n * ( 1 + x ) + pv * ( 1 + y );
			Vector v4 = v + n * x + pv * ( 1 + y );

			vertices.PushBack( DebugVertex( v1, color ) );
			vertices.PushBack( DebugVertex( v2, color ) );

			vertices.PushBack( DebugVertex( v2, color ) );
			vertices.PushBack( DebugVertex( v3, color ) );

			vertices.PushBack( DebugVertex( v3, color ) );
			vertices.PushBack( DebugVertex( v4, color ) );

			vertices.PushBack( DebugVertex( v4, color ) );
			vertices.PushBack( DebugVertex( v1, color ) );
		}
	}
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugLineOnScreen( const Vector2& start, const Vector2& end, const Color& color )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) || GetFrameInfo().IsShowFlagOn( SHOW_Scenes ) ) )
	{
		return;
	}

	DebugVertex vertices[ 2 ];
	vertices[0].Set( Vector(start.X, start.Y, 0.0f), color );
	vertices[1].Set( Vector(end.X, end.Y, 0.0f), color );

	new ( this ) CRenderFragmentOnScreenLineList( this, vertices, 2 );
}

void CRenderFrame::AddDebugGradientLineOnScreen( const Vector2& start, const Vector2& end, const Color& color1, const Color& color2 )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	DebugVertex vertices[ 2 ];
	vertices[0].Set( Vector(start.X, start.Y, 0.0f), color1 );
	vertices[1].Set( Vector(end.X, end.Y, 0.0f), color2 );

	new ( this ) CRenderFragmentOnScreenLineList( this, vertices, 2 );
}


void CRenderFrame::AddDebugLinesOnScreen( const DebugVertex* points, const Uint32 numPoints, Bool normalized /*= false*/ )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( numPoints == 0 )
	{
		return;
	}

	Matrix matrix = GetOnScreenMatrix( m_info, normalized );

	RED_ASSERT( points != nullptr );
	RED_ASSERT( ( numPoints & 1 ) == 0, TXT("Need an even number of points to draw a line list: %u"), numPoints );
	new ( this ) CRenderFragmentDebugLineList( this, matrix, points, numPoints, RSG_2D );
}


void CRenderFrame::AddDebugLine( const Vector& start, const Vector& end, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	DebugVertex points[ 2 ];
	points[0].Set( start, color );
	points[1].Set( end, color );

	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, points, 2, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugLines( const DebugVertex* points, const Uint32 numPoints, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	AddDebugLines( Matrix::IDENTITY, points, numPoints, overlay, alwaysVisible );
}

void CRenderFrame::AddDebugLines( const Matrix& localToWorld, const DebugVertex* points, const Uint32 numPoints, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( numPoints && points )
	{
		ASSERT( !(numPoints & 1) );
		new ( this ) CRenderFragmentDebugLineList( this, localToWorld, points, numPoints, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
	}
}

void CRenderFrame::AddDebugIndexedLines( const DebugVertex* vertices, const Uint32 numVertices, const Uint16* indices, const Uint32 numIndices, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( indices && numIndices )
	{
		new ( this ) CRenderFragmentDebugIndexedLineList( this, Matrix::IDENTITY, vertices, numVertices, indices, numIndices, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
	}
}

// Draw debug tetrahedron
void CRenderFrame::AddDebugTetrahedron( const Tetrahedron& tetra, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Transform them corners
	Vector transformedCorners[4];
	for ( Uint32 i = 0; i < 4; i++ )
	{
		transformedCorners[i] = matrix.TransformPoint( tetra.m_points[i] );
	}
	// Draw them
	DebugVertex lines[12];
	lines[0].Set( transformedCorners[0], color );
	lines[1].Set( transformedCorners[1], color );
	lines[2].Set( transformedCorners[0], color );
	lines[3].Set( transformedCorners[2], color );
	lines[4].Set( transformedCorners[0], color );
	lines[5].Set( transformedCorners[3], color );
	lines[6].Set( transformedCorners[1], color );
	lines[7].Set( transformedCorners[2], color );
	lines[8].Set( transformedCorners[2], color );
	lines[9].Set( transformedCorners[3], color );
	lines[10].Set( transformedCorners[3], color );
	lines[11].Set( transformedCorners[1], color );
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, lines, 12, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugBrackets( const Vector* corners, const Matrix& matrix, const Color& color, Bool overlay /*= false*/, Bool alwaysVisible /*= false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Transform them corners
	Vector transformedCorners[8];
	for ( Uint32 i = 0; i < 8; i++ )
	{
		transformedCorners[i] = matrix.TransformPoint( corners[i] );
	}

	Float line = 0.1f;

	// Assemble lines
	DebugVertex lines[ 48 ];

	// bracket for corner 0
	lines[0].Set( transformedCorners[0], color );
	lines[1].Set( transformedCorners[0] + Vector( line, 0.0f, 0.0f ), color );
	lines[2].Set( transformedCorners[0], color );
	lines[3].Set( transformedCorners[0] + Vector( 0.0f, line, 0.0f ), color );
	lines[4].Set( transformedCorners[0], color );
	lines[5].Set( transformedCorners[0] + Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 1
	lines[6].Set( transformedCorners[1], color );
	lines[7].Set( transformedCorners[1] - Vector( line, 0.0f, 0.0f ), color );
	lines[8].Set( transformedCorners[1], color );
	lines[9].Set( transformedCorners[1] + Vector( 0.0f, line, 0.0f ), color );
	lines[10].Set( transformedCorners[1], color );
	lines[11].Set( transformedCorners[1] + Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 2
	lines[12].Set( transformedCorners[2], color );
	lines[13].Set( transformedCorners[2] + Vector( line, 0.0f, 0.0f ), color );
	lines[14].Set( transformedCorners[2], color );
	lines[15].Set( transformedCorners[2] - Vector( 0.0f, line, 0.0f ), color );
	lines[16].Set( transformedCorners[2], color );
	lines[17].Set( transformedCorners[2] + Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 3
	lines[18].Set( transformedCorners[3], color );
	lines[19].Set( transformedCorners[3] - Vector( line, 0.0f, 0.0f ), color );
	lines[20].Set( transformedCorners[3], color );
	lines[21].Set( transformedCorners[3] - Vector( 0.0f, line, 0.0f ), color );
	lines[22].Set( transformedCorners[3], color );
	lines[23].Set( transformedCorners[3] + Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 4
	lines[24].Set( transformedCorners[4], color );
	lines[25].Set( transformedCorners[4] + Vector( line, 0.0f, 0.0f ), color );
	lines[26].Set( transformedCorners[4], color );
	lines[27].Set( transformedCorners[4] + Vector( 0.0f, line, 0.0f ), color );
	lines[28].Set( transformedCorners[4], color );
	lines[29].Set( transformedCorners[4] - Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 5
	lines[30].Set( transformedCorners[5], color );
	lines[31].Set( transformedCorners[5] - Vector( line, 0.0f, 0.0f ), color );
	lines[32].Set( transformedCorners[5], color );
	lines[33].Set( transformedCorners[5] + Vector( 0.0f, line, 0.0f ), color );
	lines[34].Set( transformedCorners[5], color );
	lines[35].Set( transformedCorners[5] - Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 6
	lines[36].Set( transformedCorners[6], color );
	lines[37].Set( transformedCorners[6] + Vector( line, 0.0f, 0.0f ), color );
	lines[38].Set( transformedCorners[6], color );
	lines[39].Set( transformedCorners[6] - Vector( 0.0f, line, 0.0f ), color );
	lines[40].Set( transformedCorners[6], color );
	lines[41].Set( transformedCorners[6] - Vector( 0.0f, 0.0f, line ), color );

	// bracket for corner 7
	lines[42].Set( transformedCorners[7], color );
	lines[43].Set( transformedCorners[7] - Vector( line, 0.0f, 0.0f ), color );
	lines[44].Set( transformedCorners[7], color );
	lines[45].Set( transformedCorners[7] - Vector( 0.0f, line, 0.0f ), color );
	lines[46].Set( transformedCorners[7], color );
	lines[47].Set( transformedCorners[7] - Vector( 0.0f, 0.0f, line ), color );


	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, lines, 48, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugBrackets( const Box& box, const Matrix& matrix, const Color& color, Bool overlay /*= false*/, Bool alwaysVisible /*= false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Get box corners
	Vector corners[ 8 ];
	box.CalcCorners( corners );

	// Draw box
	AddDebugBrackets( corners, matrix, color, overlay, alwaysVisible );
}

void CRenderFrame::AddDebugBox( const Box& box, const Matrix& matrix, const Color& color, Bool overlay/*=false*/, Bool alwaysVisible/*=false*/ )
{
	AddDebugBox( box, matrix, color, overlay ? RSG_DebugOverlay : RSG_DebugUnlit, alwaysVisible );
}

void CRenderFrame::AddDebugBox( const Vector* corners, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Transform them corners
	Vector transformedCorners[8];
	for ( Uint32 i = 0; i < 8; i++ )
	{
		transformedCorners[i] = matrix.TransformPoint( corners[i] );
	}

	// Assemble lines
	DebugVertex lines[ 24 ];
	lines[0].Set( transformedCorners[0], color );
	lines[1].Set( transformedCorners[4], color );
	lines[2].Set( transformedCorners[1], color );
	lines[3].Set( transformedCorners[5], color );
	lines[4].Set( transformedCorners[2], color );
	lines[5].Set( transformedCorners[6], color );
	lines[6].Set( transformedCorners[3], color );
	lines[7].Set( transformedCorners[7], color );
	lines[8].Set( transformedCorners[0], color );
	lines[9].Set( transformedCorners[1], color );
	lines[10].Set( transformedCorners[1], color );
	lines[11].Set( transformedCorners[3], color );
	lines[12].Set( transformedCorners[3], color );
	lines[13].Set( transformedCorners[2], color );
	lines[14].Set( transformedCorners[2], color );
	lines[15].Set( transformedCorners[0], color );
	lines[16].Set( transformedCorners[4], color );
	lines[17].Set( transformedCorners[5], color );
	lines[18].Set( transformedCorners[5], color );
	lines[19].Set( transformedCorners[7], color );
	lines[20].Set( transformedCorners[7], color );
	lines[21].Set( transformedCorners[6], color );
	lines[22].Set( transformedCorners[6], color );
	lines[23].Set( transformedCorners[4], color );

	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, lines, 24, sortingGroup );
}

void CRenderFrame::AddDebugBox( const Box& box, const Matrix& matrix, const Color& color, ERenderingSortGroup sortingGroup, Bool alwaysVisible/*=false*/)
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Get box corners
	Vector corners[ 8 ];
	box.CalcCorners( corners );

	// Draw box
	AddDebugBox( corners, matrix, color, sortingGroup, alwaysVisible );
}

void CRenderFrame::AddDebugFatBox( const Vector* corners, const Matrix& matrix, const Color& color, Float width/*=4.0f*/, Bool overlay/*=false*/, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Transform them corners
	Vector transformedCorners[8];
	for ( Uint32 i = 0; i < 8; i++ )
	{
		transformedCorners[i] = matrix.TransformPoint( corners[i] );
	}

	// Assemble lines
	DebugVertex lines[ 24 ];
	lines[0].Set( transformedCorners[0], color );
	lines[1].Set( transformedCorners[4], color );
	lines[2].Set( transformedCorners[1], color );
	lines[3].Set( transformedCorners[5], color );
	lines[4].Set( transformedCorners[2], color );
	lines[5].Set( transformedCorners[6], color );
	lines[6].Set( transformedCorners[3], color );
	lines[7].Set( transformedCorners[7], color );
	lines[8].Set( transformedCorners[0], color );
	lines[9].Set( transformedCorners[1], color );
	lines[10].Set( transformedCorners[1], color );
	lines[11].Set( transformedCorners[3], color );
	lines[12].Set( transformedCorners[3], color );
	lines[13].Set( transformedCorners[2], color );
	lines[14].Set( transformedCorners[2], color );
	lines[15].Set( transformedCorners[0], color );
	lines[16].Set( transformedCorners[4], color );
	lines[17].Set( transformedCorners[5], color );
	lines[18].Set( transformedCorners[5], color );
	lines[19].Set( transformedCorners[7], color );
	lines[20].Set( transformedCorners[7], color );
	lines[21].Set( transformedCorners[6], color );
	lines[22].Set( transformedCorners[6], color );
	lines[23].Set( transformedCorners[4], color );

	// Draw them
	AddDebugFatLines( lines, 24, width, overlay, false, alwaysVisible );
}

void CRenderFrame::AddDebugFatBox( const Box& box, const Matrix& matrix, const Color& color, Float width/*=4.0f*/, Bool overlay/*=false*/, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Get box corners
	Vector corners[ 8 ];
	box.CalcCorners( corners );

	// Draw box
	AddDebugFatBox( corners, matrix, color, width, overlay, alwaysVisible );
}

void CRenderFrame::AddDebugSolidBox( const Box& box, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	AddDebugSolidBox( box, matrix, color, overlay ? RSG_DebugOverlay : RSG_DebugUnlit, alwaysVisible );
}

void CRenderFrame::AddDebugSolidBox( const Box& box, const Matrix& matrix, const Color& color, ERenderingSortGroup sortGroup, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Get box corners
	Vector corners[ 8 ];
	box.CalcCorners( corners );

	// Draw box
	AddDebugSolidBox( corners, matrix, color, sortGroup, alwaysVisible );
}

void CRenderFrame::AddDebugSolidBox( const Vector* corners, const Matrix& matrix, const Color& color, ERenderingSortGroup sortGroup, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Create the vertices
	DebugVertex vertices[ 8 ];
	for ( Uint32 i=0; i<8; i++ )
	{
		vertices[ i ] = DebugVertex( matrix.TransformPoint( corners[i] ), color );
	}

	// Create the indices
	const Uint16 indices[ 36 ] = {
		0, 1, 2, 1, 3, 2,				// top
		5, 4, 6, 5, 6, 7,				// bottom
		0, 2, 4, 4, 2, 6,				// left
		1, 5, 3, 3, 5, 7,				// right
		2, 3, 6, 3, 7, 6,				// front
		0, 4, 1, 1, 4, 5				// back
	};

	// Draw them
	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, vertices, 8, indices, 36, sortGroup );
}

void CRenderFrame::AddDebugSphere( const Vector& center, Float radius, const Matrix& matrix, const Color& color, Bool overlay/*=false*/, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	const Uint32 numRings = 5;
	const Uint32 numPoints = 12;

	TStaticArray< DebugVertex, 3 * numRings * numPoints * 2 > vertices;

	const Vector uaxes[3] = { Vector::EY, Vector::EX, Vector::EX };
	const Vector vaxes[3] = { Vector::EZ, Vector::EZ, Vector::EY };
	const Vector waxes[3] = { Vector::EX, Vector::EY, Vector::EZ };
	const Float angletable[5] = { 30.0f, 60.0f, 90.0f, 120.0f, 150.0f };

	// Generate rings
	for ( Uint32 axis=0; axis<3; axis++ )
	{
		for ( Uint32 ring=0; ring<numRings; ring++ )
		{			
			const Float angle = DEG2RAD( angletable[ring] );

			// Generate points			
			TStaticArray< Vector, numPoints + 1 > ringPoints;
			for ( Uint32 i=0; i<=numPoints; i++ )
			{
				const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );
				const Float u = cos( localAngle ) * sin( angle ) * radius;
				const Float v = sin( localAngle ) * sin( angle ) * radius;
				const Float w = cos( angle ) * radius;
				Vector point = ( uaxes[axis] * u ) + ( vaxes[axis] * v ) + ( waxes[axis] * w );
				ringPoints.PushBack( center + matrix.TransformPoint( point ) );
			}

			// Generate lines
			for ( Uint32 i=0; i<numPoints; i++ )
			{
				vertices.PushBack( DebugVertex( ringPoints[i], color ) );
				vertices.PushBack( DebugVertex( ringPoints[i+1], color ) );
			}
		}
	}

	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}


// Draw debug curve
void CRenderFrame::AddDebug3DCurve( const SCurve3DData& curve, const Color& color, const Matrix& matrix, Uint32 numPointsPerKeyFrame, Bool overlay )
{
	TDynArray< DebugVertex > vertices;
	TDynArray< Float > keyFrames;
	curve.GetKeyframes( keyFrames );
	if ( keyFrames.Size() <= 1 ) return;
	for ( Uint32 j = 0; j < keyFrames.Size() - 1; j++ )
	{
		Float start = keyFrames[j];
		Float d = keyFrames[j+1] - start;
		for ( Uint32 i = 0; i < numPointsPerKeyFrame; i++ )
		{
			Float time = start + ( d * i ) / ( numPointsPerKeyFrame - 1 );
			Vector v = curve.GetValue( time );
			if ( !( ( i == 0 && j == 0 ) || ( i == numPointsPerKeyFrame - 1 && j == keyFrames.Size() - 2 ) ) )
			{
				vertices.PushBack( DebugVertex( v, color ) );
			}
			vertices.PushBack( DebugVertex( v, color ) );
		}
	}
	new ( this ) CRenderFragmentDebugLineList( this, matrix, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugCircle( const Vector& center, Float radius, const Matrix& matrix, const Color& color, Uint32 numPoints /*=12*/, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	numPoints = Clamp< Uint32 > ( numPoints, 5, 1000 );

	TDynArray< DebugVertex > vertices;
	vertices.Reserve( numPoints * 2 );

	const Float angle = DEG2RAD( 90.f );

	// Generate points			
	TDynArray< Vector > ringPoints;
	ringPoints.Reserve( numPoints + 1 );

	for ( Uint32 i=0; i<=numPoints; i++ )
	{
		const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );
		const Float u = cos( localAngle ) * sin( angle ) * radius;
		const Float v = sin( localAngle ) * sin( angle ) * radius;
		const Float w = cos( angle ) * radius;
		Vector point = ( Vector::EX * u ) + ( Vector::EY * v ) + ( Vector::EZ * w );
		ringPoints.PushBack( center + matrix.TransformPoint( point ) );
	}

	// Generate lines
	for ( Uint32 i=0; i<numPoints; i++ )
	{
		vertices.PushBack( DebugVertex( ringPoints[i], color ) );
		vertices.PushBack( DebugVertex( ringPoints[i+1], color ) );
	}

	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugTransparent );
}
void CRenderFrame::AddDebugWireframeTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Bool overlay/* = false*/, Uint16 numPoints/* = 12*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	numPoints = Clamp< Uint16 > ( numPoints, 5, 1000 );

	TDynArray< DebugVertex > vertices;
	vertices.Reserve( ( numPoints + 1 ) * 2 );

	const Float angle = DEG2RAD( 90.f );

	Vector norm = ( center2 - center1 );
	norm.W = 0;
	norm.Normalize3();

	Float a = MAcos( Vector::Dot3( norm, Vector( 0, 0, 1 ) ) );
	Plane p( norm, Vector::ZERO_3D_POINT );
	Vector v( 1, 1, 1 );
	v = p.Project( v );
	if ( v.SquareMag3() <= 0.000000001f )
	{
		v = Vector( 1, -2, 1 );
		v = p.Project( v );
	}
	v.Normalize3();

	Float qSin = MSin( M_PI / (Float)numPoints );
	Float qCos = MCos( M_PI / (Float)numPoints );
	Vector quat( norm.X * qSin, norm.Y * qSin, norm.Z * qSin, qCos );
	Matrix m;
	m.BuildFromQuaternion( quat );

	for ( Uint32 i=0; i<=numPoints; i++ )
	{
		Vector r = v * radius1;
		Vector r2 = center1 + v * radius1;
		vertices.PushBack( DebugVertex( /*matrix.TransformPoint(*/ center1 + v * radius1 /*)*/, color1 ) );
		vertices.PushBack( DebugVertex( /*matrix.TransformPoint(*/ center2 +  v * radius2 /*)*/, color1 ) );
		v = m.TransformVector( v );
	}

	vertices.PushBack( DebugVertex( /*matrix.TransformPoint(*/ center1 /*)*/, color1 ) );
	vertices.PushBack( DebugVertex( /*matrix.TransformPoint(*/ center2 /*)*/, color2 ) );

	TDynArray< DebugVertex > verticesF;
	// Generate indices
	for ( Uint16 i=0; i<numPoints; i++ )
	{
		verticesF.PushBack( vertices[ i * 2 + 2 ] );
		verticesF.PushBack( vertices[ i * 2 ] );

		verticesF.PushBack( vertices[ i * 2 ] );
		verticesF.PushBack( vertices[ i * 2 + 1 ] );

		verticesF.PushBack( vertices[ i * 2 + 1 ] );
		verticesF.PushBack( vertices[ i * 2 + 2 ] );

		verticesF.PushBack( vertices[ i * 2 + 3 ] );
		verticesF.PushBack( vertices[ i * 2 + 2 ] );

		verticesF.PushBack( vertices[ i * 2 + 2 ] );
		verticesF.PushBack( vertices[ i * 2 + 1 ] );

		verticesF.PushBack( vertices[ i * 2 + 1 ] );
		verticesF.PushBack( vertices[ i * 2 + 3 ] );
	}
	for ( Uint16 i=0; i<numPoints; i++ )
	{
		verticesF.PushBack( vertices[ i * 2 ] );
		verticesF.PushBack( vertices[ vertices.Size() - 2 ] );

		verticesF.PushBack( vertices[ i * 2 +1 ] );
		verticesF.PushBack( vertices[ vertices.Size() - 1 ] );
	}

	// Draw polygon
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, verticesF.TypedData(), verticesF.Size(), overlay ? RSG_DebugOverlay : RSG_DebugTransparent );
}

void CRenderFrame::AddDebugTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, ERenderingSortGroup sortGroup, Uint16 numPoints /*=12*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	numPoints = Clamp< Uint16 > ( numPoints, 5, 1000 );

	TDynArray< DebugVertex > vertices;
	vertices.Reserve( ( numPoints + 1 ) * 2 );

	TDynArray< Uint16 > indices;
	indices.Reserve( numPoints * 6 );

	const Float angle = DEG2RAD( 90.f );

	Vector norm = ( center2 - center1 );
	norm.W = 0;
	norm.Normalize3();

	Float a = MAcos( Vector::Dot3( norm, Vector( 0, 0, 1 ) ) );
	Plane p( norm, Vector::ZERO_3D_POINT );
	Vector v( 1, 1, 1 );
	v = p.Project( v );
	if ( v.SquareMag3() <= NumericLimits<Float>::Epsilon() )
	{
		v = Vector( 1, -2, 1 );
		v = p.Project( v );
	}
	v.Normalize3();

	Float qSin = MSin( M_PI / (Float)numPoints );
	Float qCos = MCos( M_PI / (Float)numPoints );
	Vector quat( norm.X * qSin, norm.Y * qSin, norm.Z * qSin, qCos );
	Matrix m;
	m.BuildFromQuaternion( quat );

	for ( Uint32 i=0; i<=numPoints; i++ )
	{

		vertices.PushBack( DebugVertex( center1 + matrix.TransformPoint( v * radius1 ), color1 ) );
		vertices.PushBack( DebugVertex( center2 + matrix.TransformPoint( v * radius2 ), color1 ) );
		v = m.TransformVector( v );
	}

	vertices.PushBack( DebugVertex( center1, color1 ) );
	vertices.PushBack( DebugVertex( center2, color2 ) );

	// Generate indices
	for ( Uint16 i=0; i<numPoints; i++ )
	{
		indices.PushBack( i * 2 + 2 );
		indices.PushBack( i * 2 );
		indices.PushBack( i * 2 + 1 );
		indices.PushBack( i * 2 + 3 );
		indices.PushBack( i * 2 + 2 );
		indices.PushBack( i * 2 + 1 );
	}
	for ( Uint16 i=0; i<numPoints; i++ )
	{
		indices.PushBack( i * 2 + 2 );
		indices.PushBack( (Uint16)vertices.Size() - 2 );
		indices.PushBack( i * 2 );
		indices.PushBack( i * 2 + 1);
		indices.PushBack( (Uint16)vertices.Size() - 1 );
		indices.PushBack( i * 2 + 3 );
	}

	// Draw polygon
	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), sortGroup );
}

void CRenderFrame::AddDebugTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Uint16 numPoints /*=12*/, Bool alwaysVisible /*=false*/ )
{
	AddDebugTube( center1, center2, radius1, radius2, matrix, color1, color2, RSG_DebugTransparent, numPoints, alwaysVisible );
}

void CRenderFrame::AddDebugSolidTube( const Vector& center1, const Vector& center2, Float radius1, Float radius2, const Matrix& matrix, const Color& color1, const Color& color2, Bool overlay /*=false*/, Uint16 numPoints /*=12*/ )
{
	AddDebugTube( center1, center2, radius1, radius2, matrix, color1, color2, overlay ? RSG_DebugOverlay : RSG_DebugUnlit, numPoints );
}

void CRenderFrame::AddDebugOrientedBox( const OrientedBox& box, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	TStaticArray< DebugVertex, 24 > vertices;
	Vector e1 = box.GetEdge1();
	Vector e2 = box.GetEdge2();
	Vector e3 = box.GetEdge3();

	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 + e2 ), color ) );

	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e1 + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 + e1 + e2 ), color ) );

	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e3 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e1 + e3 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 + e3 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 + e1 ), color ) );
	vertices.PushBack( DebugVertex( matrix.TransformPoint( box.m_position + e2 + e1 + e3 ), color ) );

	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugCapsule( const FixedCapsule& capsule, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	const Uint32 numRings = 5;
	const Uint32 numPoints = 12;

	TStaticArray< DebugVertex, 3 * numRings * ( 6 + numPoints ) * 2 > vertices;

	const Vector uaxes[3] = { Vector::EY, Vector::EX, Vector::EX };
	const Vector vaxes[3] = { Vector::EZ, Vector::EZ, Vector::EY };
	const Vector waxes[3] = { Vector::EX, Vector::EY, Vector::EZ };
	const Float angletable[5] = { DEG2RAD(30.0f), DEG2RAD(60.0f), DEG2RAD(90.0f), DEG2RAD(120.0f), DEG2RAD(150.0f) };
	const Vector& pos = capsule.GetPosition();
	const Float radius = capsule.GetRadius();
	Float height = capsule.GetHeight();
	const Vector vZradius( 0, 0, radius );

	// check params
	ASSERT( height > 2.0f*radius );
	if ( height < 2.0f*radius )
	{
		height = 2.0f*radius;
	}

	// Generate rings
	for ( Uint32 axis=0; axis<3; axis++ )
	{
		for ( Uint32 ring=0; ring<numRings; ring++ )
		{			
			const Float angle = angletable[ring];

			// Generate points			
			TStaticArray< Vector, numPoints + 3 > ringPoints;
			Vector prevPoint;
			for ( Uint32 i=0; i <= numPoints; i++ )
			{
				const Float localAngle = 2.0f * M_PI * ( ( i == numPoints ? 0 : i ) / (Float)numPoints );
				const Float u = cosf( localAngle ) * sinf( angle ) * radius;
				const Float v = sinf( localAngle ) * sinf( angle ) * radius;
				const Float w = cosf( angle ) * radius;
				Vector point = ( uaxes[axis] * u ) + ( vaxes[axis] * v ) + ( waxes[axis] * w );
				if ( point.Z > 0 )
				{
					if ( i > 0 && prevPoint.Z == 0)
					{
						ringPoints.PushBack( pos + vZradius + matrix.TransformPoint( Vector( prevPoint.X, prevPoint.Y, prevPoint.Z + height - 2.0f*radius ) ) );
						vertices.PushBack( DebugVertex( ringPoints[ringPoints.Size()-2], color ) );
						vertices.PushBack( DebugVertex( ringPoints[ringPoints.Size()-1], color ) );
					}
					point.Z += height - 2.0f * radius;
				}
				ringPoints.PushBack( pos + vZradius + matrix.TransformPoint( point ) );
				if ( i > 0 )
				{
					vertices.PushBack( DebugVertex( ringPoints[ringPoints.Size()-2], color ) );
					vertices.PushBack( DebugVertex( ringPoints[ringPoints.Size()-1], color ) );
				}
				prevPoint = point;
			}
		}
	}

	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugEllipsoid( const Vector& center, const Vector& ellipsoid, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	AddDebugEllipsoid( center, ellipsoid.X, ellipsoid.Y, ellipsoid.Z, matrix, color, overlay, alwaysVisible );
}

void CRenderFrame::AddDebugEllipsoid( const Vector& center, Float a, Float b, Float c, const Matrix& matrix, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	const Float angletable[] = { 15.f, 30.0f, 45.f, 60.0f, 75.f, 90.0f, 105.f, 120.0f, 135.f, 150.0f, 175.f };

	const Uint32 numRings = ARRAY_COUNT( angletable );
	const Uint32 numPoints = 12;

	TStaticArray< DebugVertex, 3 * numRings * numPoints * 2 > vertices;

	const Int32 uaxes[3] = { 1, 0, 0 };
	const Int32 vaxes[3] = { 2, 2, 1 };
	const Int32 waxes[3] = { 0, 1, 2 };

	const Vector vecs[3] = { Vector::EX, Vector::EY, Vector::EZ };
	const Float params[3] = { a, b, c };

	// Generate rings
	for ( Uint32 axis=0; axis<3; axis++ )
	{
		for ( Uint32 ring=0; ring<numRings; ring++ )
		{			
			const Float angle = DEG2RAD( angletable[ring] );

			// Generate points			
			TStaticArray< Vector, numPoints + 1 > ringPoints;
			for ( Uint32 i=0; i<=numPoints; i++ )
			{
				// x = a sinA cosB
				// y = b sinA sinB
				// z = c cosA

				const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

				const Float u = sin( angle ) * cos( localAngle ) * params[uaxes[axis]];
				const Float v = sin( angle ) * sin( localAngle ) * params[vaxes[axis]];
				const Float w = cos( angle ) * params[waxes[axis]];

				Vector point = ( vecs[uaxes[axis]] * u ) + ( vecs[vaxes[axis]] * v ) + ( vecs[waxes[axis]] * w );
				ringPoints.PushBack( center + matrix.TransformPoint( point ) );
			}

			// Generate lines
			for ( Uint32 i=0; i<numPoints; i++ )
			{
				vertices.PushBack( DebugVertex( ringPoints[i], color ) );
				vertices.PushBack( DebugVertex( ringPoints[i+1], color ) );
			}
		}
	}

	// Draw them
	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugAngledRange( const Vector& position, Float yaw, Float range, Float rangeAngle, Float heading, Color col, Bool drawSides, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Vector pT = position + Vector( 0.0f, 0.0f, 2.0f );
	Vector pB = position + Vector( 0.0f, 0.0f, 0.2f );

	// clamp
	rangeAngle = Min< Float >( rangeAngle, 360.0f );
	heading = Min< Float >( heading, 360.0f );

	const Uint32 MAX_STEPS = 36;
	Uint32 steps = Max<Uint32>( Uint32( rangeAngle / 10 ), 2 );
	steps = Min<Uint32>( MAX_STEPS, steps );
	Float step = rangeAngle / (Float) steps;

	TStaticArray< DebugVertex, (MAX_STEPS + 1) * 2 + 2 > vertices;
	TStaticArray< Uint16, (MAX_STEPS + 1) * 6 + 8 > indices;

	// add center points
	vertices.PushBack( DebugVertex( pT, col ) );
	vertices.PushBack( DebugVertex( pB, col ) );

	yaw += heading;

	Float angle = rangeAngle * -0.5f;
	for ( Uint32 s = 0; s<=steps; s++, angle += step )
	{
		EulerAngles ang( 0.0f, 0.f, angle + yaw );
		Vector dir( 0.0f, range, 0.0f );		
		Vector pT2 = pT + ang.ToMatrix().TransformVector( dir );
		Vector pB2 = pB + ang.ToMatrix().TransformVector( dir );

		// add border top point 
		vertices.PushBack( DebugVertex( pT2, col ) );

		// add border bottom point 
		vertices.PushBack( DebugVertex( pB2, col ) );

		Uint16 index = (Uint16) vertices.Size()-1;

		// border top to ground
		if ( index > 2 )
		{
			indices.PushBack( index-1 );
			indices.PushBack( index );
		}

		// between borders
		if ( index > 3 )
		{
			// top
			indices.PushBack( index-3 );
			indices.PushBack( index-1 );

			// bottom
			indices.PushBack( index-2 );
			indices.PushBack( index );
		}
	}

	if ( drawSides && rangeAngle < 360.0f )
	{
		// sides top
		indices.PushBack( 0 );
		indices.PushBack( 2 );
		indices.PushBack( 0 );
		indices.PushBack( (Uint16) vertices.Size()-2 );

		// sides bottom
		indices.PushBack( 1 );
		indices.PushBack( 3 );
		indices.PushBack( 1 );
		indices.PushBack( (Uint16) vertices.Size()-1 );
	}

	AddDebugIndexedLines( &vertices[0], vertices.Size(), &indices[0], indices.Size(), false );
}

void CRenderFrame::AddDebugAngledRange( const Matrix& localToWorld, Float height, Float range, Float rangeAngle, Color col, Bool drawSides, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Vector pT = Vector( 0.0f, 0.0f, height );
	Vector pB = Vector( 0.0f, 0.0f, 0.0f );

	// clamp
	rangeAngle = Min< Float >( rangeAngle, 360.0f );

	const Uint32 MAX_STEPS = 36;
	Uint32  steps = Max< Uint32 >( Uint32( rangeAngle / 10 ), 2 );
	steps = Min<Uint32>( MAX_STEPS, steps );
	Float stepAngle = rangeAngle / (Float) steps;

	TStaticArray< DebugVertex, (MAX_STEPS + 1) * 2 + 2 > vertices;
	TStaticArray< Uint16, (MAX_STEPS + 1) * 6 + 8 > indices;

	// add center points
	vertices.PushBack( DebugVertex( pT, col ) );
	vertices.PushBack( DebugVertex( pB, col ) );

	Vector radius( 0.0f, range, 0.0f );		
	Float  angle = rangeAngle * -0.5f;

	for ( Uint32 s = 0; s <= steps; ++s, angle += stepAngle )
	{
		EulerAngles ang( 0.0f, 0.0f, angle );
		Vector pT2 = pT + ang.ToMatrix().TransformVector( radius );
		Vector pB2 = pB + ang.ToMatrix().TransformVector( radius );

		// add border top point 
		vertices.PushBack( DebugVertex( pT2, col ) );

		// add border bottom point 
		vertices.PushBack( DebugVertex( pB2, col ) );

		Uint16 index = (Uint16) vertices.Size()-1;

		// border top to ground
		if ( index > 2 )
		{
			indices.PushBack( index-1 );
			indices.PushBack( index );
		}

		// between borders
		if ( index > 3 )
		{
			// top
			indices.PushBack( index-3 );
			indices.PushBack( index-1 );

			// bottom
			indices.PushBack( index-2 );
			indices.PushBack( index );
		}
	}

	if ( drawSides && rangeAngle < 360 )
	{
		// sides top
		indices.PushBack( 0 );
		indices.PushBack( 2 );
		indices.PushBack( 0 );
		indices.PushBack( (Uint16) vertices.Size()-2 );

		// sides bottom
		indices.PushBack( 1 );
		indices.PushBack( 3 );
		indices.PushBack( 1 );
		indices.PushBack( (Uint16) vertices.Size()-1 );
	}

	new ( this ) CRenderFragmentDebugIndexedLineList( this, localToWorld, &vertices[0], vertices.Size(), &indices[0], indices.Size(), RSG_DebugOverlay );
}

void CRenderFrame::AddDebugFrustum( const Matrix& frustumMatrix, const Color& color, Bool backFaces /*=false*/, Bool overlay /*=false*/, Float farPlaneScale/*=1.0f*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	const Vector frustumMin( -1.0f, -1.0f, 0.0f, 1.0f );
	const Vector frustumMax( +1.0f, +1.0f, farPlaneScale, 1.0f );

	Vector unprojected[8];
	for ( Uint32 i = 0; i < 8; ++i )
	{
		// Calculate vertex in screen space		
		Vector screenVertex = Vector::Permute( frustumMin, frustumMax, i&1, i&2, i&4, 0 );

		// Unproject to world space
		unprojected[i] = frustumMatrix.TransformVectorWithW( screenVertex );
		unprojected[i] /= unprojected[i].W;
	}

	Vector cameraPosition = GetFrameInfo().m_camera.GetPosition();

	// Draw each face of frustum
	Int32 planes[ 6 ][ 4 ] = { { 0,1,3,2 }, { 6,7,5,4 }, { 5,7,3,1 }, { 6,4,0,2 }, { 5,1,0,4 }, { 3,7,6,2 } };
	for ( Int32 i = 0; i < 6; ++i )
	{
		// Create frustum face plane
		Plane frustumPlane( unprojected[ planes[i][0] ], unprojected[ planes[i][1] ], unprojected[ planes[i][2] ] );

		// Check if face is front facing
		if ( backFaces == true || frustumPlane.GetSide( cameraPosition ) == Plane::PS_Front )
		{
			AddDebugLine( unprojected[ planes[i][0] ], unprojected[ planes[i][1] ], color, overlay );
			AddDebugLine( unprojected[ planes[i][1] ], unprojected[ planes[i][2] ], color, overlay );
			AddDebugLine( unprojected[ planes[i][2] ], unprojected[ planes[i][3] ], color, overlay );
			AddDebugLine( unprojected[ planes[i][3] ], unprojected[ planes[i][0] ], color, overlay );
		}
	}
}

void CRenderFrame::AddDebugCone( const Matrix& matrix, Float radius, Float innerAngle, Float outerAngle, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	TDynArray< DebugVertex > lines;

	// Get light vectors
	Vector o = matrix.GetTranslation();
	Vector x = matrix.GetAxisX();
	Vector y = matrix.GetAxisY();
	Vector z = matrix.GetAxisZ();

	// Calculate cone points
	const Uint32 numSegments = 18;
	TDynArray< Vector > coneInnerPoints;
	TDynArray< Vector > coneOuterPoints;
	for ( Uint32 i=0; i<numSegments; i++ )
	{
		const Float angle = (i / (Float)numSegments) * M_PI * 2.0f;
		Vector coneVector = ( x * cos( angle ) ) + ( z * sin( angle ) );
		coneInnerPoints.PushBack( o + ( y * radius * cos( DEG2RAD( innerAngle * 0.5f ) ) ) + coneVector * sin( DEG2RAD( innerAngle * 0.5f ) ) * radius );
		coneOuterPoints.PushBack( o + ( y * radius * cos( DEG2RAD( outerAngle * 0.5f  ) ) ) + coneVector * sin( DEG2RAD( outerAngle * 0.5f ) ) * radius );
	}

	// Cone lines
	for ( Uint32 i=0; i<numSegments; i++ )
	{
		lines.PushBack( DebugVertex( o, Color::WHITE ) );
		lines.PushBack( DebugVertex( coneOuterPoints[i], Color::WHITE ) );
	}

	// Rings
	for ( Uint32 i=0; i<numSegments; i++ )
	{
		lines.PushBack( DebugVertex( coneInnerPoints[i], Color::MAGENTA ) );
		lines.PushBack( DebugVertex( coneInnerPoints[(i+1)%numSegments], Color::MAGENTA ) );
		lines.PushBack( DebugVertex( coneOuterPoints[i], Color::WHITE ) );
		lines.PushBack( DebugVertex( coneOuterPoints[(i+1)%numSegments], Color::WHITE ) );
	}

	// Axis
	lines.PushBack( DebugVertex( o, Color::WHITE ) );
	lines.PushBack( DebugVertex( o + y * radius, Color::WHITE ) );

	// Sphere
	const Int32 numSphereSegments = 10;
	TDynArray< Vector > sphereSegments;
	for ( Int32 i=-numSphereSegments; i<=numSphereSegments; i++ )
	{
		const Float angle = DEG2RAD( 0.5f * outerAngle * i / (Float)numSphereSegments );
		sphereSegments.PushBack( o + y * cos( angle ) * radius + z * sin( angle ) * radius );
		sphereSegments.PushBack( o + y * cos( angle ) * radius + x * sin( angle ) * radius );
	}

	// Sphere segments
	for ( Int32 i=0; i<numSphereSegments*2; i++ )
	{
		lines.PushBack( DebugVertex( sphereSegments[2*i + 0], Color::WHITE ) );
		lines.PushBack( DebugVertex( sphereSegments[2*i + 2], Color::WHITE ) );
		lines.PushBack( DebugVertex( sphereSegments[2*i + 1], Color::WHITE ) );
		lines.PushBack( DebugVertex( sphereSegments[2*i + 3], Color::WHITE ) );
	}

	AddDebugLines( lines.TypedData(), lines.Size(), false );
}

void CRenderFrame::AddDebugTriangles( const Matrix &matrix, const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, const Color& color, Bool overlay/*=false*/, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Draw them
	new ( this ) CRenderFragmentDebugPolyList( this, matrix, points, numPoints, indices, numIndices, overlay ? RSG_DebugOverlay : RSG_DebugTransparent );
}

void CRenderFrame::AddDebugTrianglesOnScreen( const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, const Color& color, Bool normalized /*= false*/ )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Matrix matrix = GetOnScreenMatrix( m_info, normalized );

	new ( this ) CRenderFragmentDebugPolyList( this, matrix, points, numPoints, indices, numIndices, RSG_2D );
}


void CRenderFrame::AddSprite( const Vector& center, Float size, const Color& color, const CHitProxyID& hitId, CBitmapTexture* texture, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
 	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Disable sprites with distance (filters option)
	Float visualDebugMaxRenderDistance = m_info.GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
	if( visualDebugMaxRenderDistance > 0.0001f && ( center - m_info.m_camera.GetPosition()  ).SquareMag3() > visualDebugMaxRenderDistance )
	{
		return;
	}

	// Override drawing color for hit proxy rendering
	Color drawColor = color;
	if ( m_info.m_renderingMode == RM_HitProxies )
	{
		drawColor  = hitId.GetColor();
	}

	// Generate fragment
	new ( this ) CRenderFragmentDebugSprite( this, center, size, texture, drawColor, overlay ? RSG_DebugOverlay : RSG_Sprites );
}

void CRenderFrame::AddEnvProbe( Float gameTime, IRenderResource *renderResource, const Vector& center, Float radius, const CHitProxyID& hitId, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Override drawing color for hit proxy rendering
	CHitProxyID hitProxy;
	if ( m_info.m_renderingMode == RM_HitProxies )
	{
		hitProxy = hitId;
	}

	// Generate fragment
	new ( this ) CRenderFragmentDebugEnvProbe( this, gameTime, renderResource, center, radius, hitProxy, RSG_DebugUnlit );
}

void CRenderFrame::AddDebugFatLine( const Vector& start, const Vector& end, const Color& color, Float width/* = 4.0f*/, Bool overlay/*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Calculate line plane
	const Vector cameraPosition = m_info.m_camera.GetPosition();
	Plane linePlane( start, end, cameraPosition );
	const Vector up = linePlane.NormalDistance * width;

	// Calculate side vector
	const Vector vertexDir = ( start - cameraPosition ).Normalized3();
	const Vector side = Vector::Cross( up, vertexDir ).Normalized3() * width;

	// Setup vertices
	DebugVertex vertices[4];
	vertices[0].Set( end + side - up, color );
	vertices[1].Set( end + side + up, color );
	vertices[2].Set( start - side + up, color );
	vertices[3].Set( start - side - up, color );

	// Setup indices
	Uint16 indices[6];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;

	// Draw polygon
	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, vertices, 4, indices, 6, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugFatLines( const DebugVertex* points, const Uint32 numPoints, Float width /*=4.0f*/, Bool overlay /*=false*/, Bool isRibbon /*=true*/, Bool alwaysVisible /*=false*/ )
{
	struct Output
	{
		Output( Float width, const Vector& cameraPosition )
			: m_baseVert( 0 ), m_baseInd( 0 ), m_width( width ), m_cameraPosition( cameraPosition ) {}

		~Output()
		{
			ASSERT( m_baseInd == m_indices.Size() && m_baseVert == m_vertices.Size() );

		}
		Bool Initialize( Uint32 numSegments )
		{
			if ( numSegments > 0U && numSegments < NumericLimits< Uint16 >::Max() / 4U )
			{
				m_vertices.Resize( numSegments * 4 );
				m_indices.Resize( numSegments * 6 );
				return true;
			}
			return false;
			
		}
		void PushSegment( const Vector& start, const Uint32 startColor, const Vector& end, const Uint32 endColor )
		{
			// Calculate line plane
			Plane linePlane( start, end, m_cameraPosition );
			const Vector up = linePlane.NormalDistance * m_width;

			// Calculate side vector
			const Vector vertexDir = ( start - m_cameraPosition ).Normalized3();
			const Vector side = Vector::Cross( up, vertexDir ).Normalized3() * m_width;

			// Setup vertices
			DebugVertex* _vertices = &m_vertices[ m_baseVert ];
			_vertices[0].Set( end + side - up, endColor );
			_vertices[1].Set( end + side + up, endColor );
			_vertices[2].Set( start - side + up, startColor );
			_vertices[3].Set( start - side - up, startColor );

			// Setup indices
			Uint16* _indices = &m_indices[ m_baseInd ];
			_indices[0] = m_baseVert + 0;
			_indices[1] = m_baseVert + 1;
			_indices[2] = m_baseVert + 2;
			_indices[3] = m_baseVert + 0;
			_indices[4] = m_baseVert + 2;
			_indices[5] = m_baseVert + 3;

			m_baseVert += 4;
			m_baseInd += 6;
		}

		TDynArray<DebugVertex>	m_vertices;
		TDynArray<Uint16>		m_indices;
		Uint16					m_baseVert;
		Uint32					m_baseInd;
		Float					m_width;
		Vector					m_cameraPosition;
	};

	if ( !alwaysVisible && ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) || numPoints < 2 ) )
	{
		return;
	}

	const Uint32			MAX_SEGMENTS = NumericLimits< Uint16 >::Max() / 4U;

	const Uint32 numSegments = isRibbon ? numPoints - 1 : numPoints / 2;
	if ( numSegments >= MAX_SEGMENTS )
	{
		// split the input in half (possibly recursive)
		Uint32 segmentsL = numSegments / 2 + numSegments % 2;
		Uint32 segmentsR = numSegments / 2;

		Uint32 numPointsL, numPointsR, indexR;

		if ( isRibbon )
		{
			numPointsL = segmentsL+1;
			numPointsR = segmentsR+1;
			indexR = segmentsL;
		}
		else
		{
			numPointsL = segmentsL*2;
			numPointsR = segmentsR*2;
			indexR = numPointsL;
		}

		AddDebugFatLines( points, numPointsL, width, overlay, isRibbon );
		AddDebugFatLines( points+indexR, numPointsR, width, overlay, isRibbon );
		
		return;
	}

	Output					output( width, m_info.m_camera.GetPosition() );
	const DebugVertex*		input( points );	

	output.Initialize( numSegments );

	if ( isRibbon )
	{
		for ( Uint32 i = 0; i < numSegments; i++ )
		{
			const Vector start( &input[0].x );
			const Uint32 startColor = input[0].color;

			const Vector end( &input[1].x );
			const Uint32 endColor = input[1].color;

			++input;

			output.PushSegment( start, startColor, end, endColor );
		}
	}
	else
	{

		for ( Uint32 i = 0; i < numSegments; i++ )
		{
			const Vector start( &input->x );
			const Uint32 startColor = input->color;
			++input;

			const Vector end( &input->x );
			const Uint32 endColor = input->color;
			++input;

			output.PushSegment( start, startColor, end, endColor );
		}
	}

	// Draw polygons
	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, output.m_vertices.TypedData(), output.m_vertices.Size(), output.m_indices.TypedData(), output.m_indices.Size(), overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugLineWithArrow( const Vector& start, const Vector& end, Float arrowPostionOnLine, Float arrowSizeX, Float arrowSizeY, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	DebugVertex points[ 8 ];
	points[0].Set( start, color );
	points[1].Set( end, color );
	Vector dirVec = end - start;
	Vector arrowTop = start + dirVec*arrowPostionOnLine;	
	Vector arrowRight( arrowSizeX, -arrowSizeY, 0.0f );	// in local space
	Vector arrowLeft( -arrowSizeX, -arrowSizeY, 0.0f );	// in local space

	// Calculate matrix
	Matrix mat;
	mat.V[1] = dirVec;
	mat.V[1].Normalize3();
	mat.V[2] = ( dirVec.SquareMag2() > 0.001f ) ? Vector::EZ : Vector::EY;
	mat.V[0] = Vector::Cross( mat.V[1], mat.V[2] );
	mat.V[0].Normalize3();
	mat.V[2] = Vector::Cross( mat.V[0], mat.V[1] );
	mat.V[2].Normalize3();
	mat.V[3] = arrowTop;

	mat.V[0].W = 0.0f;
	mat.V[1].W = 0.0f;
	mat.V[2].W = 0.0f;
	mat.V[3].W = 1.0f;

	arrowRight = mat.TransformVectorAsPoint( arrowRight );	// to world space
	arrowLeft = mat.TransformVectorAsPoint( arrowLeft );	// to world space

	points[2].Set( arrowTop, color );
	points[3].Set( arrowRight, color );

	points[4].Set( arrowRight, color );
	points[5].Set( arrowLeft, color );

	points[6].Set( arrowLeft, color );
	points[7].Set( arrowTop, color );

	new ( this ) CRenderFragmentDebugLineList( this, Matrix::IDENTITY, points, 8, overlay ? RSG_DebugOverlay : RSG_DebugUnlit );
}

void CRenderFrame::AddDebugScreenFormatedText( Int32 X, Int32 Y, const Color& colorText, const Char* format, ... )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();
	if ( font )
	{
		// Format text
		va_list arglist;
		va_start( arglist, format );
		Char formattedBuf[ 512 ];
		Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist ); 

		// Print formated text
		font->Print( this, X, Y, 0.5f, formattedBuf, colorText );
	}
}

void CRenderFrame::AddDebugScreenFormatedText( Int32 X, Int32 Y, const Char* format, ... )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();
	if ( font )
	{
		// Format text
		va_list arglist;
		va_start( arglist, format );
		Char formattedBuf[ 512 ];
		Red::System::VSNPrintF( formattedBuf, ARRAY_COUNT(formattedBuf), format, arglist ); 

		// Print formated text
		font->Print( this, X, Y, 0.5f, formattedBuf, Color::WHITE );
	}
}

void CRenderFrame::AddDebugScreenText( Int32 X, Int32 Y, const Char* text, const Color& colorText /*= Color( 255, 255, 255 )*/, CFont *font /* = NULL */ 
	/* = NULL */, Bool withBackground /* = false */,const Color &colorBackground /* = Color( 0, 0, 0 ) */, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( font == NULL )
	{
		font = resOnScreenTextFont.LoadAndGet< CFont >();
	}

	if ( font )
	{
		// Draw background
		if ( withBackground )
		{
			// Calculate text dimensions
			Uint32 textWidth, textHeight;
			int unusedX, unusedY;
			font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

			Matrix translationMatrix;
			translationMatrix.SetIdentity();
			translationMatrix.SetTranslation( (Float)X, (Float)Y, 0.4f );
			new ( this ) CRenderFragmentDebugRectangle( this, translationMatrix, (Float)textWidth, (Float)textHeight, (Float)font->GetLineDist()*0.75f,
				colorBackground );
		}
		//then text
		font->Print( this, X, Y, 0.0f, text, colorText );
	}
}

void CRenderFrame::AddDebugScreenText( Int32 X, Int32 Y, const String& text, Uint32 offsetYinLines, Bool withBackground /*= false*/, const Color &colorText /*= Color( 255, 255, 255 )*/, const Color &colorBackground /*= Color( 0, 0, 0 ) */, CFont *font /*= NULL */, Bool alwaysVisible/*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( font == NULL )
	{
		font = resOnScreenTextFont.LoadAndGet< CFont >();
	}

	if ( font )
	{
		// Calculate text dimensions
		Uint32 textWidth, textHeight;
		int unusedX, unusedY;
		font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

		// Draw background
		if ( withBackground )
		{
			Matrix translationMatrix;
			translationMatrix.SetIdentity();
			translationMatrix.SetTranslation( (Float)X, (Float)(Y + textHeight * 2 * offsetYinLines), 0.4f );
			new ( this ) CRenderFragmentDebugRectangle( this, translationMatrix, (Float)textWidth, (Float)textHeight, (Float)font->GetLineDist()*0.75f,
				colorBackground );
		}
		//then text
		font->Print( this, X, Y + textHeight * 2 * offsetYinLines, 0.0f, text.AsChar(), colorText );
	}
}

void CRenderFrame::AddDebugText( const Vector& position, const String& text, Bool withBackground /*= false*/, 
								 const Color &colorText /*=Color( 255, 255, 255 )*/, const Color &colorBackground /*=Color( 0, 0, 0 )*/,
								 CFont *font /*= NULL*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( font == NULL )
	{
		font = resOnScreenTextFont.LoadAndGet< CFont >();
	}

	if ( font )
	{
		// Get matrix projection from camera and transform the 3D position into 2D screen coordinates

		Vector pos3D = position;
		pos3D.W = 1;
		Vector pos2D = GetFrameInfo().m_camera.GetWorldToScreen().TransformVectorWithW( pos3D );
		pos2D.Y = -pos2D.Y;

		// we don't want to divide by 0
		if ( pos2D.W < -FLT_EPSILON || pos2D.W > FLT_EPSILON )
		{
			pos2D /= pos2D.W; // translate to uniform coordinates
		}
		else
		{
			return;
		}
		// pos2D.X and pos2D.Y are now from [-1; 1] range

		if ( pos2D.X <= 1.0f && pos2D.X >= -1.0f && pos2D.Y <= 1.0f && pos2D.Y >= -1.0f && pos2D.Z < 1.0f )
		{
			// transform [-1; 1] to [0, WIDTH] and [0, HEIGHT]
			pos2D.X += 1;
			pos2D.Y += 1;
			pos2D.X *= GetFrameOverlayInfo().m_width / 2;
			pos2D.Y *= GetFrameOverlayInfo().m_height / 2;

			// Draw background
			if ( withBackground )
			{
				// Calculate text dimensions
				Uint32 textWidth, textHeight;
				int unusedX, unusedY;
				font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

				Matrix translationMatrix;
				translationMatrix.SetIdentity();
				translationMatrix.SetTranslation( (Float)pos2D.X, (Float)pos2D.Y, 0.4f );
				new ( this ) CRenderFragmentDebugRectangle( this, translationMatrix, (Float)textWidth, (Float)textHeight, (Float)font->GetLineDist()*0.75f,
					colorBackground );
			}

			// Draw text
			font->Print( this, (int)pos2D.X, (int)pos2D.Y, 0.0f, text.AsChar(), colorText );
		}
	}
}

void CRenderFrame::AddDebugText( const Vector& position, const String& text, Int32 offsetXinPx, Int32 offsetYinLines,
								Bool withBackground /*=false*/, const Color &colorText /*=Color( 255, 255, 255 )*/, const Color &colorBackground /*=Color( 0, 0, 0 )*/,
								CFont *font /*=nullptr*/, Bool alwaysVisible /*=false*/)
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	if ( font == NULL )
	{
		font = resOnScreenTextFont.LoadAndGet< CFont >();
	}

	if ( font )
	{
		// Get matrix projection from camera and transform the 3D position into 2D screen coordinates

		Vector pos3D = position;
		pos3D.W = 1;
		Vector pos2D = GetFrameInfo().m_camera.GetWorldToScreen().TransformVectorWithW( pos3D );
		pos2D.Y = -pos2D.Y;

		// we don't want to divide by 0
		if ( pos2D.W < -FLT_EPSILON || pos2D.W > FLT_EPSILON )
		{
			pos2D /= pos2D.W; // translate to uniform coordinates
		}
		else
		{
			return;
		}
		// pos2D.X and pos2D.Y are now from [-1; 1] range

		if ( pos2D.X <= 1.0f && pos2D.X >= -1.0f && pos2D.Y <= 1.0f && pos2D.Y >= -1.0f && pos2D.Z < 1.0f )
		{
			// transform [-1; 1] to [0, WIDTH] and [0, HEIGHT]
			pos2D.X += 1;
			pos2D.Y += 1;
			pos2D.X *= GetFrameOverlayInfo().m_width / 2;
			pos2D.Y *= GetFrameOverlayInfo().m_height / 2;

			// Calculate text dimensions
			Uint32 textWidth, textHeight;
			Int32 unusedX, unusedY;
			font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

			// Line offset
			Int32 lineOffset = textHeight*offsetYinLines;

			// Draw background
			if ( withBackground )
			{
				Matrix translationMatrix;
				translationMatrix.SetIdentity();
				translationMatrix.SetTranslation( Float(pos2D.X + offsetXinPx), Float(pos2D.Y + lineOffset), 0.4f );
				Float shiftY = (Float)font->GetLineDist()*0.75f;
				new ( this ) CRenderFragmentDebugRectangle( this, translationMatrix, (Float)textWidth, (Float)textHeight, shiftY, colorBackground );
			}

			// Draw text
			font->Print( this, (Int32)pos2D.X + offsetXinPx, (Int32)pos2D.Y + lineOffset, 0.0f, text.AsChar(), colorText );
		}
	}
}


Rect CRenderFrame::GetDebugTextBounds( const String& text, CFont* font /*= NULL*/ )
{
	if ( font == NULL )
	{
		font = resOnScreenTextFont.LoadAndGet< CFont >();
	}

	if ( font )
	{
		Int32 x, y;
		Uint32 w, h;
		font->GetTextRectangle( text, x, y, w, h );
		return Rect( x, x + w, y, y + h );
	}

	return Rect( 0, 0, 0, 0 );
}


void CRenderFrame::AddDebugAxis( const Vector& pos, const Matrix& rot, const Float scale, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	static Vector axis[3] = { Vector::EX, Vector::EY, Vector::EZ };
	static Color colors[3] = { Color::RED, Color::GREEN, Color::BLUE };

	for (Uint32 i=0; i<3; i++)
	{
		Vector localAxis = rot.TransformVector(axis[i]);
		Vector head = localAxis * scale;
		AddDebugLine( pos, head+pos, colors[i], overlay );
		//AddDebugSphere( head+pos, scale/10.0f, Matrix::IDENTITY, colors[i], overlay);
	}
}

void CRenderFrame::AddDebugAxis( const Vector& pos, const Matrix& rot, const Float scale, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	static Vector axis[3] = { Vector::EX, Vector::EY, Vector::EZ };

	for (Uint32 i=0; i<3; i++)
	{
		Vector localAxis = rot.TransformVector(axis[i]);
		Vector head = localAxis * scale;
		AddDebugLine( pos, head+pos, color, overlay );
		//AddDebugSphere( head+pos, scale/10.0f, Matrix::IDENTITY, colors[i], overlay);
	}
}

void CRenderFrame::AddDebugAxisOnScreen( const Uint32 x, const Uint32 y, const Matrix& rot, const Float scale, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Float x_p = (Float)(x / GetFrameOverlayInfo().m_width);
	Float y_p = (Float)(x / GetFrameOverlayInfo().m_height);

	AddDebugAxisOnScreen( x_p, y_p, rot, scale );
}

void CRenderFrame::AddDebugAxisOnScreen( const Float x, const Float y, const Matrix& rot, const Float scale, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// _c	postfix: clipspace
	// _w	postfix: worldspace
	// _in	postfix: input worldspace

	Vector right_in, up_in, front_in;
	rot.ToEulerAngles().ToAngleVectors( &front_in, &right_in, &up_in );

	Vector center_c = Vector ( x * 2.f - 1.f , y * 2.f - 1.f , 0.5f, 1.f );
	Vector right_c  = center_c + GetFrameInfo().m_camera.GetWorldToView().TransformVector( right_in ) * scale;
	Vector up_c		= center_c + GetFrameInfo().m_camera.GetWorldToView().TransformVector( up_in ) * scale;
	Vector front_c  = center_c + GetFrameInfo().m_camera.GetWorldToView().TransformVector( front_in ) * scale;

	Vector center_w = GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( center_c );
	Vector right_w  = GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( right_c );
	Vector up_w     = GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( up_c );
	Vector front_w  = GetFrameInfo().m_camera.GetScreenToWorld().TransformVectorWithW( front_c );

	center_w	/= center_w.W;
	right_w		/= right_w.W;
	up_w		/= up_w.W;
	front_w		/= front_w.W;

	AddDebugLine( center_w, right_w, Color::RED,   true );
	AddDebugLine( center_w, up_w,	 Color::BLUE,  true );
	AddDebugLine( center_w, front_w, Color::GREEN, true );
}

void CRenderFrame::AddDebugArrow( const Matrix& localToWorld, const Vector& direction, const Float scale, const Color& color, Bool overlay /*=false*/, Bool alwaysVisible /*=false*/)
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Vector dir = localToWorld.TransformVector( direction );
	EulerAngles rot( 0.f, 0.f, 45.f );

	Vector start = localToWorld.GetTranslation();
	Vector end = start + dir * scale;

	Vector start1 = start + ( dir * (scale * 0.9f) ) + rot.TransformVector( dir * (scale * 0.1f) );
	Vector start2 = start + ( dir * (scale * 0.9f) ) - rot.TransformVector( dir * (scale * 0.1f) );

	AddDebugLine( start, end, color, overlay );
	AddDebugLine( start1, end, color, overlay );
	AddDebugLine( start2, end, color, overlay );
}


void CRenderFrame::AddDebug3DArrow( const Vector& position, const Vector& direction, const Float scale, Float radius1, Float radius2, Float headLength, const Color& color )
{
	AddDebug3DArrow( position, direction, scale, radius1, radius2, headLength, color, color );
}

void CRenderFrame::AddDebug3DArrow( const Vector& position, const Vector& direction, const Float scale, Float radius1, Float radius2, Float headLength, const Color& color1, const Color& color2 )
{
	Float arrowLength = direction.Mag3() * scale;

	Float tailLength = Max( arrowLength - headLength, 0.0f );
	headLength = Min( headLength, arrowLength );

	Vector dirNorm = direction.Normalized3();

	Vector headBasePosition = position + dirNorm * tailLength;
	Vector headEndPosition = position + dirNorm * arrowLength;

	if ( tailLength > 0.001f )
	{
		AddDebugSolidTube( position, headBasePosition, radius1, radius1, Matrix::IDENTITY, color1, color1 );
	}
	if ( headLength > 0.001f )
	{
		AddDebugSolidTube( headBasePosition, headEndPosition, radius2, 0.0f, Matrix::IDENTITY, color2, color2 );
	}
}


void CRenderFrame::AddDebugBone(const Matrix& tm, const Matrix& ptm2, const Color& color, Bool overlay /*=false*/, Float mul /*=1.0f*/, Bool alwaysVisible /*=false*/ )
{
	if ( !alwaysVisible && !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}
	
	Vector p1=ptm2.GetTranslation();
	Vector p2= tm.GetTranslation();
	Float len = (p2-p1).Mag3();

	Matrix ptm=ptm2;
	Vector row1=(p2-p1).Normalized3();
	Vector row2 = Vector::Cross(row1,Vector(0,0,1)).Normalized3();
	Vector row3 = Vector::Cross(row2,row1).Normalized3();

	ptm.SetRow(0, row1);
	ptm.SetRow(1, row2);
	ptm.SetRow(2, row3);
	ptm.SetRow(3, p1);

	Float defWidth=len*0.05f;
	if(defWidth<0.005f){defWidth=0.005f;}
	if(defWidth>0.4f){defWidth=0.4f;}

	Float defWidth2=len*0.03f;
	if(defWidth2<0.003f){defWidth2=0.003f;}
	if(defWidth2>0.2f){defWidth2=0.2f;}

	defWidth*=mul;
	defWidth2*=mul;


	DebugVertex points[6];
	Uint16 indices[8*3];
	DebugVertex points2[6];

	points[0].z=0;			points[0].y=0;				points[0].x=0;
	points[1].z=defWidth;	points[1].y=defWidth;		points[1].x=defWidth;
	points[2].z=-defWidth;	points[2].y=defWidth;		points[2].x=defWidth;
	points[3].z=-defWidth;	points[3].y=-defWidth;		points[3].x=defWidth;
	points[4].z=defWidth;	points[4].y=-defWidth;		points[4].x=defWidth;
	points[5].z=0;			points[5].y=0;				points[5].x=defWidth*2;

	points2[0].z=0;			points2[0].y=0;				points2[0].x=0;
	points2[1].z=defWidth2;	points2[1].y=defWidth2;		points2[1].x=defWidth2;
	points2[2].z=-defWidth2;	points2[2].y=defWidth2;		points2[2].x=defWidth2;
	points2[3].z=-defWidth2;	points2[3].y=-defWidth2;		points2[3].x=defWidth2;
	points2[4].z=defWidth2;	points2[4].y=-defWidth2;		points2[4].x=defWidth2;
	points2[5].z=0;			points2[5].y=0;				points2[5].x=len;

	Color col2(color);
	col2.A=64;
	for(int j=0;j<6;j++)
	{
		points[j].color  = col2.ToUint32();
		points2[j].color = col2.ToUint32();
	}

	indices[0]=0;
	indices[1]=1;
	indices[2]=2;

	indices[3]=0;
	indices[4]=2;
	indices[5]=3;

	indices[6]=0;
	indices[7]=3;
	indices[8]=4;

	indices[9]=0;
	indices[10]=4;
	indices[11]=1;

	indices[12]=5;
	indices[13]=2;
	indices[14]=1;

	indices[15]=5;
	indices[16]=1;
	indices[17]=4;

	indices[18]=5;
	indices[19]=4;
	indices[20]=3;

	indices[21]=5;
	indices[22]=3;
	indices[23]=2;

	Vector pts[6];
	Vector pts2[6];
	for(int i=0;i<6;i++)
	{
		pts[i].X=points[i].x;
		pts[i].Y=points[i].y;
		pts[i].Z=points[i].z;
		pts[i]=tm.TransformVectorAsPoint(pts[i]);
		points[i].x=pts[i].X;
		points[i].y=pts[i].Y;
		points[i].z=pts[i].Z;

		pts2[i].X=points2[i].x;
		pts2[i].Y=points2[i].y;
		pts2[i].Z=points2[i].z;
		pts2[i]=ptm.TransformVectorAsPoint(pts2[i]);
		points2[i].x=pts2[i].X;
		points2[i].y=pts2[i].Y;
		points2[i].z=pts2[i].Z;
	}

	AddDebugLine(pts[0], pts[1], color, overlay);
	AddDebugLine(pts[0], pts[2], color, overlay);
	AddDebugLine(pts[0], pts[3], color, overlay);
	AddDebugLine(pts[0], pts[4], color, overlay);

	AddDebugLine(pts[1], pts[2], color, overlay);
	AddDebugLine(pts[2], pts[3], color, overlay);
	AddDebugLine(pts[3], pts[4], color, overlay);
	AddDebugLine(pts[4], pts[1], color, overlay);

	AddDebugLine(pts[1], pts[5], color, overlay);
	AddDebugLine(pts[2], pts[5], color, overlay);
	AddDebugLine(pts[3], pts[5], color, overlay);
	AddDebugLine(pts[4], pts[5], color, overlay);

	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, points, ARRAY_COUNT(points), indices, ARRAY_COUNT(indices), overlay ? RSG_DebugOverlay : RSG_DebugTransparent );

	AddDebugLine(pts2[0], pts2[1], color, overlay);
	AddDebugLine(pts2[0], pts2[2], color, overlay);
	AddDebugLine(pts2[0], pts2[3], color, overlay);
	AddDebugLine(pts2[0], pts2[4], color, overlay);

	AddDebugLine(pts2[1], pts2[2], color, overlay);
	AddDebugLine(pts2[2], pts2[3], color, overlay);
	AddDebugLine(pts2[3], pts2[4], color, overlay);
	AddDebugLine(pts2[4], pts2[1], color, overlay);

	AddDebugLine(pts2[1], pts2[5], color, overlay);
	AddDebugLine(pts2[2], pts2[5], color, overlay);
	AddDebugLine(pts2[3], pts2[5], color, overlay);
	AddDebugLine(pts2[4], pts2[5], color, overlay);

	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, points2, ARRAY_COUNT(points2), indices, ARRAY_COUNT(indices), overlay ? RSG_DebugOverlay : RSG_DebugTransparent );
}

void CRenderFrame::AddDebugFrame( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Draw rect
	DebugVertex vertices[8];
	vertices[0].Set( Vector( (Float)x, (Float)y, 0.0f ), color );
	vertices[1].Set( Vector( (Float)(x+width), (Float)y, 0.0f ), color );
	vertices[2].Set( Vector( (Float)(x+width), (Float)y, 0.0f ), color );
	vertices[3].Set( Vector( (Float)(x+width), (Float)(y+height), 0.0f ), color );
	vertices[4].Set( Vector( (Float)(x+width), (Float)(y+height), 0.0f ), color );
	vertices[5].Set( Vector( (Float)x, (Float)(y+height), 0.0f ), color );
	vertices[6].Set( Vector( (Float)x, (Float)(y+height), 0.0f ), color );
	vertices[7].Set( Vector( (Float)x, (Float)y, 0.0f ), color );
	new ( this ) CRenderFragmentOnScreenLineList( this, vertices, 8 );
}

void CRenderFrame::AddDebugRect( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Draw rect
	new ( this ) CRenderFragmentDebugRectangle2D( this, x, y, width, height, color );
}

void CRenderFrame::AddDebugGradientRect( Int32 x, Int32 y, Int32 width, Int32 height, const Color& color1, const Color& color2, Bool vertical /*= false*/ )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Rect rect( x, x + width, y, y + height );

	AddDebugGradientRects( 1, &rect, &color1, &color2, vertical );
}

void CRenderFrame::AddDebugGradientRects( Uint32 numRects, const Rect* rects, const Color* colors1, const Color* colors2, Bool vertical /*= false*/ )
{
	if ( numRects == 0 )
	{
		return;
	}

	RED_ASSERT( rects != nullptr && colors1 != nullptr && colors2 != nullptr );

	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	Uint32 numVertices = numRects * 4;
	Uint32 numIndices = numRects * 6;

	RED_ASSERT( numVertices < (1<<16), TXT("Trying to draw too many rectangles in one go: %u. Either draw fewer, or make AddDebugGradientRects work in batches"), numRects );


	DebugVertex* vertices = new ( this ) DebugVertex[ numVertices ];
	Uint16* indices = new ( this ) Uint16[ numIndices ];

	DebugVertex* vert = vertices;
	Uint16* ind = indices;
	for ( Uint32 i = 0; i < numRects; ++i )
	{
		const Rect& rect = rects[i];
		const Color& col1 = colors1[i];
		const Color& col2 = colors2[i];

		if ( vertical )
		{
			vert[0].Set( Vector( (Float)rect.m_left,  (Float)rect.m_top, 0.5f ), col1 );
			vert[1].Set( Vector( (Float)rect.m_right, (Float)rect.m_top, 0.5f ), col2 );
			vert[2].Set( Vector( (Float)rect.m_left,  (Float)rect.m_bottom, 0.5f ), col1 );
			vert[3].Set( Vector( (Float)rect.m_right, (Float)rect.m_bottom, 0.5f ), col2 );
		}
		else
		{
			vert[0].Set( Vector( (Float)rect.m_left,  (Float)rect.m_top, 0.5f ), col1 );
			vert[1].Set( Vector( (Float)rect.m_right, (Float)rect.m_top, 0.5f ), col1 );
			vert[2].Set( Vector( (Float)rect.m_left,  (Float)rect.m_bottom, 0.5f ), col2 );
			vert[3].Set( Vector( (Float)rect.m_right, (Float)rect.m_bottom, 0.5f ), col2 );
		}

		ind[0] = (Int16)( i*4 + 0 );
		ind[1] = (Int16)( i*4 + 1 );
		ind[2] = (Int16)( i*4 + 2 );

		ind[3] = (Int16)( i*4 + 2 );
		ind[4] = (Int16)( i*4 + 1 );
		ind[5] = (Int16)( i*4 + 3 );

		vert += 4;
		ind += 6;
	}

	new ( this ) CRenderFragmentDebugPolyList( this, Matrix::IDENTITY, vertices, numVertices, indices, numIndices, RSG_2D, false );
}

void CRenderFrame::AddDebugFilledRect( const Matrix& mtx, const Color& color )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Setup vertices
	DebugVertex vertices[4];
	vertices[0].Set( Vector( -0.5f,  0.5f, 0.0f ), color );
	vertices[1].Set( Vector(  0.5f,  0.5f, 0.0f ), color );
	vertices[2].Set( Vector(  0.5f, -0.5f, 0.0f ), color );
	vertices[3].Set( Vector( -0.5f, -0.5f, 0.0f ), color );

	// Setup indices
	Uint16 indices[6];
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 0;
	indices[4] = 2;
	indices[5] = 3;

	// Draw polygon
	new ( this ) CRenderFragmentDebugPolyList( this, mtx, vertices, 4, indices, 6, RSG_DebugUnlit );
}

void CRenderFrame::AddDebugTexturedRect( Int32 x, Int32 y, Int32 width, Int32 height, CBitmapTexture* texture, const Color& color )
{
	if ( !( GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) ) )
	{
		return;
	}

	// Draw rect
	new ( this ) CRenderFragmentTexturedDebugRectangle( this, (Float)x, (Float)y, (Float)width, (Float)height, texture, color );
}

void CRenderFrame::AddDebugDynamicTexture( Int32 x, Int32 y, Int32 width, Int32 height, GpuApi::TextureRef textureRef, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector )
{
	new ( this ) CRenderFragmentDynamicTexture( this, (Float)x, (Float)y, (Float)width, (Float)height, textureRef, mipIndex, sliceIndex, colorMin, colorMax, channelSelector );
}

void CRenderFrame::SetMaterialDebugMode( EMaterialDebugMode mdm )
{
	m_info.SetMaterialDebugMode( mdm );
}

RED_INLINE static Bool IsNonDebugFragment( IRenderFragment* frag )
{
	switch ( frag->GetType() )
	{
		// NonDebug
		case RFT_MeshChunkSkinned:
		case RFT_MeshChunkStatic:
		case RFT_MeshChunkLeafCard:
		case RFT_MeshChunkFoliage:
		case RFT_TerrainChunk:
		case RFT_BrushFace:
		case RFT_Particle:
			return true;

		// Debug
		case RFT_DebugLineList:
		case RFT_DebugIndexedLineList:
		case RFT_DebugRectangle:
		case RFT_DebugSprite:
		case RFT_DebugEnvProbe:
		case RFT_DebugPolyList:
		case RFT_DebugMesh:
		case RFT_DebugApex:
		case RFT_Text:
			return false;

		default:
			ASSERT( !"invalid type" );
	}

	return false;
}
