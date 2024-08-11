/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushBuilder.h"
#include "brushComponent.h"
#include "brushFace.h"

IMPLEMENT_ENGINE_CLASS( CBrushBuilder );

void CBrushBuilder::BeginBrush( CBrushComponent* brush )
{
	ASSERT( !m_brush );
	ASSERT( brush );

	// Set brush
	m_brush = brush; 

	// Prepare brush, delete old faces
	for ( Uint32 i=0; i<m_brush->m_faces.Size(); i++ )
	{
		m_brush->m_faces[i]->Discard();
	}

	// Clear face list
	m_brush->m_faces.Clear();
}

void CBrushBuilder::EndBrush()
{
	// Brush is ready
	ASSERT( m_brush );
	m_brush = NULL;
}

Uint32 CBrushBuilder::AddVertex( const Vector& pos, Float u, Float v )
{
	ASSERT( m_face );

	// Add vertex
	BrushVertex vertex;
	vertex.m_position = pos;
	vertex.m_mapping = Vector( u, v, 0.0f, 0.0f );
	m_face->m_vertices.PushBack( vertex );

	// Return vertex index
	return m_face->m_vertices.Size() - 1;
}

Uint32 CBrushBuilder::AddVertex( const Vector& pos, Float u, Float v, const Vector& normal )
{
	ASSERT( m_face );

	// Add vertex
	BrushVertex vertex;
	vertex.m_position = pos;
	vertex.m_mapping = Vector( u, v, 0.0f, 0.0f );
	vertex.m_normal = normal;
	m_face->m_vertices.PushBack( vertex );

	// Return vertex index
	return m_face->m_vertices.Size() - 1;
}

Uint32 CBrushBuilder::AddVertex( Float x, Float y, Float z, Float u, Float v )
{
	// Add vertex
	BrushVertex vertex;
	vertex.m_position = Vector( x, y, z, 1.0f );
	vertex.m_mapping = Vector( u, v, 0.0f, 0.0f );
	m_face->m_vertices.PushBack( vertex );

	// Return vertex index
	return m_face->m_vertices.Size() - 1;
}

void CBrushBuilder::BeginFace()
{
	ASSERT( m_brush );
	ASSERT( !m_face );

	// Create face
	m_face = ::CreateObject< CBrushFace >( m_brush );
	m_face->SetMaterial( NULL );
}

void CBrushBuilder::EndFace( Bool buildNormals /*= true*/ )
{
	ASSERT( m_brush );
	ASSERT( m_face );
	ASSERT( m_tempFaceSign == 0 );

	// Finish face
	if ( !m_face->m_polygons.Size() )
	{
		m_face->Discard();
	}
	else
	{
		// Calculate automatic normals
		if ( buildNormals )
		{
			m_face->RebuildNormals();
		}

		m_brush->m_faces.PushBack( m_face );
	}

	// Done
	m_face = NULL;
}

void CBrushBuilder::BeginPolygon( Int32 sideSign )
{
	ASSERT( m_brush );
	ASSERT( m_face );
	ASSERT( m_tempFaceSign == 0 );

	// Begin polygon
	m_tempFaceSign = sideSign;
	m_tempIndices.Clear();
}

void CBrushBuilder::EndPolygon()
{
	ASSERT( m_brush );
	ASSERT( m_face );
	ASSERT( m_tempFaceSign != 0 );

	// Begin polygon
	if ( m_tempIndices.Size() )
	{
		CBrushFace::Polygon polygon;

		// Generate indices
		if ( m_tempFaceSign > 0 )
		{
			for ( Uint32 i=0; i<m_tempIndices.Size(); i++ )
			{
				Uint32 index = m_tempIndices[i];
				ASSERT( index < m_face->m_vertices.Size() );
				polygon.m_indices.PushBack( index );
			}
		}
		else
		{
			for ( Uint32 i=0; i<m_tempIndices.Size(); i++ )
			{
				Uint32 index = m_tempIndices[ (m_tempIndices.Size()-1) - i ];
				ASSERT( index < m_face->m_vertices.Size() );
				polygon.m_indices.PushBack( index );
			}
		}

		// Upload polygon
		m_face->m_polygons.PushBack( polygon );
	}

	// End polygon
	m_tempFaceSign = 0;
}

void CBrushBuilder::AddTriangle( Int32 side, Uint32 a, Uint32 b, Uint32 c )
{
	BeginPolygon( side );
	AddIndex( a );
	AddIndex( b );
	AddIndex( c );
	EndPolygon();
}

void CBrushBuilder::AddQuad( Int32 side, Uint32 a, Uint32 b, Uint32 c, Uint32 d )
{
	BeginPolygon( side );
	AddIndex( a );
	AddIndex( b );
	AddIndex( c );
	AddIndex( d );
	EndPolygon();
}

void CBrushBuilder::AddIndex( Uint32 index )
{
	ASSERT( m_brush );
	ASSERT( m_face );
	ASSERT( m_tempFaceSign != 0 );
	ASSERT( index < m_face->m_vertices.Size() );
	m_tempIndices.PushBack( index );
}
