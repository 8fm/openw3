/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/brushBuilder.h"

/// Cube brush builder
class CBrushBuilderCube : public CBrushBuilder
{
	DECLARE_ENGINE_CLASS( CBrushBuilderCube, CBrushBuilder, 0 );

public:
	Float		m_width;		//!< Cube width
	Float		m_height;		//!< Cube height
	Float		m_depth;		//!< Cube depth

public:
	CBrushBuilderCube()
		: m_width( 1.0f )
		, m_height( 1.0f )
		, m_depth( 1.0f )
	{};

public:
	//! Build brush geometry !
	virtual void Build( CBrushComponent* brush );
};

BEGIN_CLASS_RTTI( CBrushBuilderCube );
	PARENT_CLASS( CBrushBuilder );
	PROPERTY_EDIT_RANGE( m_width, TXT("Width of the cube"), 0.01f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_height, TXT("Height of the cube"), 0.01f, 1000.0f );
	PROPERTY_EDIT_RANGE( m_depth, TXT("Depth of the cube"), 0.01f, 1000.0f );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CBrushBuilderCube )

void CBrushBuilderCube::Build( CBrushComponent* brush )
{
	BeginBrush( brush );

	// Get half size
	Float w2 = m_width / 2.0f;		// X = width
	Float h2 = m_depth / 2.0f;		// Y = depth
	Float d2 = m_height / 2.0f;		// Z = height

	// Top face h = const +
	BeginFace();
	AddVertex( -w2, +h2, -d2, 0.0f, 0.0f );
	AddVertex( +w2, +h2, -d2, 1.0f, 0.0f );
	AddVertex( +w2, +h2, +d2, 1.0f, 1.0f );
	AddVertex( -w2, +h2, +d2, 0.0f, 1.0f );
	AddQuad( -1, 0,1,2,3 );
	EndFace();

	// Bottom face h = const -
	BeginFace();
	AddVertex( -w2, -h2, -d2, 0.0f, 0.0f );
	AddVertex( +w2, -h2, -d2, 1.0f, 0.0f );
	AddVertex( +w2, -h2, +d2, 1.0f, 1.0f );
	AddVertex( -w2, -h2, +d2, 0.0f, 1.0f );
	AddQuad( 1, 0,1,2,3 );
	EndFace();

	// Left face w = const -
	BeginFace();
	AddVertex( -w2, -h2, -d2, 0.0f, 0.0f );
	AddVertex( -w2, -h2, +d2, 1.0f, 0.0f );
	AddVertex( -w2, +h2, +d2, 1.0f, 1.0f );
	AddVertex( -w2, +h2, -d2, 0.0f, 1.0f );
	AddQuad( +1, 0,1,2,3 );
	EndFace();

	// Right face w = const +
	BeginFace();
	AddVertex( +w2, -h2, -d2, 0.0f, 0.0f );
	AddVertex( +w2, -h2, +d2, 1.0f, 0.0f );
	AddVertex( +w2, +h2, +d2, 1.0f, 1.0f );
	AddVertex( +w2, +h2, -d2, 0.0f, 1.0f );
	AddQuad( -1, 0,1,2,3 );
	EndFace();

	// Front face d = const -
	BeginFace();
	AddVertex( -w2, -h2, -d2, 0.0f, 0.0f );
	AddVertex( +w2, -h2, -d2, 1.0f, 0.0f );
	AddVertex( +w2, +h2, -d2, 1.0f, 1.0f );
	AddVertex( -w2, +h2, -d2, 0.0f, 1.0f );
	AddQuad( -1, 0,1,2,3 );
	EndFace();

	// Back face d = const +
	BeginFace();
	AddVertex( -w2, -h2, +d2, 0.0f, 0.0f );
	AddVertex( +w2, -h2, +d2, 1.0f, 0.0f );
	AddVertex( +w2, +h2, +d2, 1.0f, 1.0f );
	AddVertex( -w2, +h2, +d2, 0.0f, 1.0f );
	AddQuad( +1, 0,1,2,3 );
	EndFace();

	// Finalize brush
	EndBrush();
}
