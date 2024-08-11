/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushRenderData.h"
#include "renderCommands.h"
#include "brushFace.h"
#include "renderer.h"
#include "renderProxy.h"
#include "material.h"

// How many texture repeats per 1m
#define TEXTURE_MAPPING_SCALE  0.5f

BrushRenderFace::BrushRenderFace()
	: m_material( NULL )
	, m_firstVertex( 0 )
	, m_firstIndex( 0 )
	, m_numVertices( 0 )
	, m_numIndices( 0 )
{
}

BrushRenderFace::BrushRenderFace( CBrushFace* sourceFace, Uint16 firstVertex, Uint16 firstIndex )
	: m_material( sourceFace->GetMaterial() )
	, m_firstVertex( firstVertex )
	, m_firstIndex( firstIndex )
	, m_numVertices( 0 )
	, m_numIndices( 0 )
{
}

static void CalculateBestMappingAxes( const Vector& normal, Vector& u, Vector& v )
{
	Float absX = Abs< Float >( normal.X );
	Float absY = Abs< Float >( normal.Y );
	Float absZ = Abs< Float >( normal.Z );
	if ( absX > absY && absX > absZ )
	{
		u = Vector::EY;
		v = Vector::EZ;
	}
	else if ( absY > absX && absY > absZ )
	{
		u = Vector::EX;
		v = Vector::EZ;
	}
	else
	{
		u = Vector::EX;
		v = Vector::EY;
	}
}

void BrushRenderFace::UpdateMaterial( IMaterial* material )
{
	// Set new material
	m_material = material;
}

void BrushRenderFace::UpdateMapping( BrushRenderData& renderData, const Matrix& brushLocalToWorld, const BrushFaceMapping& mapping )
{
	// Get first vertex to modify
	BrushRenderVertex* vertex = &renderData.m_vertices[ m_firstVertex ];

	// Update mapping
	if ( mapping.m_mapping == BFM_Face )
	{
		// Simple face mapping;
		for ( Uint32 i=0; i<m_numVertices; i++, ++vertex )
		{
			vertex->m_uv[0] = vertex->m_baseUV[0] * mapping.m_scaleU + mapping.m_offsetU;
			vertex->m_uv[1] = vertex->m_baseUV[1] * mapping.m_scaleV + mapping.m_offsetV;
		}
	}
	else
	{
		// Get mapping relative center
		Vector center;
		if ( mapping.m_mapping == BFM_World )
		{
			// Use world origin as relative mapping origin
			center = Vector::ZEROS;
		}
		else
		{
			// Map relative to brush center
			center = brushLocalToWorld.GetTranslation();
		}

		// Get best mapping axes
		Vector uAxis, vAxis;
		if ( mapping.m_mapping == BFM_World )
		{
			Vector worldNormal = brushLocalToWorld.TransformVector( vertex->m_normal );
			CalculateBestMappingAxes( vertex->m_normal, uAxis, vAxis );
		}
		else
		{
			CalculateBestMappingAxes( vertex->m_normal, uAxis, vAxis );
			uAxis = brushLocalToWorld.TransformVector( uAxis ).Normalized3();
			vAxis = brushLocalToWorld.TransformVector( vAxis ).Normalized3();
		}

		// Calculate UV vectors
		Int32 uMin = INT_MAX, vMin = INT_MAX;
		BrushRenderVertex* writeVertex = vertex;
		for ( Uint32 i=0; i<m_numVertices; i++, ++writeVertex )
		{
			Vector worldPosition = brushLocalToWorld.TransformPoint( writeVertex->m_position );
			writeVertex->m_uv[0] = Vector::Dot3( worldPosition - center, uAxis ) * mapping.m_scaleU * TEXTURE_MAPPING_SCALE + mapping.m_offsetU;
			writeVertex->m_uv[1] = Vector::Dot3( worldPosition - center, vAxis ) * mapping.m_scaleV * TEXTURE_MAPPING_SCALE + mapping.m_offsetV;

			// Get min UV
			uMin = Min< Int32 >( (Int32)writeVertex->m_uv[0], uMin );
			vMin = Min< Int32 >( (Int32)writeVertex->m_uv[1], vMin );
		}

		//  Normalize
		writeVertex = vertex;
		for ( Uint32 i=0; i<m_numVertices; i++, ++writeVertex )
		{
			writeVertex->m_uv[0] -= ( Float ) uMin;
			writeVertex->m_uv[1] -= ( Float ) vMin;
		}
	}
}

void BrushRenderFace::Serialize( IFile& file )
{
	file << m_material;
	file << m_firstVertex;
	file << m_firstIndex;
	file << m_numVertices;
	file << m_numIndices;
}

void BrushRenderData::ResetGeometry()
{
	m_vertices.Clear();
	m_indices.Clear();
	m_faces.Clear();
	m_renderingProxies.Clear();
}

Int32 BrushRenderData::BeginFace( CBrushFace* sourceFace )
{
	// Get face Id
	const Int32 faceID = ( Int32 ) m_faces.Size();

	// Initialize face
	const Uint16 firstVertex = ( Uint16 ) m_vertices.Size();
	const Uint16 firstIndex = ( Uint16 ) m_indices.Size();
	new ( m_faces ) BrushRenderFace( sourceFace, firstVertex, firstIndex );

	// Return allocated face ID
	return faceID;
}

Uint16 BrushRenderData::AddVertex( Int32 faceID, const Vector& position, const Vector& normal, Float faceU, Float faceV )
{
	// Create vertex
	BrushRenderVertex vertex;
	vertex.m_position[0] = position.X;
	vertex.m_position[1] = position.Y;
	vertex.m_position[2] = position.Z;
	vertex.m_normal[0] = normal.X;
	vertex.m_normal[1] = normal.Y;
	vertex.m_normal[2] = normal.Z;
	vertex.m_uv[0] = faceU;
	vertex.m_uv[1] = faceV;
	vertex.m_baseUV[0] = faceU;
	vertex.m_baseUV[1] = faceV;
	vertex.m_tangent[0] = 1.0f;
	vertex.m_tangent[1] = 0.0f;
	vertex.m_tangent[2] = 0.0f;
	vertex.m_binormal[0] = 0.0f;
	vertex.m_binormal[1] = 1.0f;
	vertex.m_binormal[2] = 0.0f;

	// Find existing vertex
	BrushRenderFace& face = m_faces[ faceID ];
	for ( Uint16 i=0; i<face.m_numVertices; i++ )
	{
		const BrushRenderVertex& existingVertex = m_vertices[ face.m_firstVertex + i ];
		if ( 0 == Red::System::MemoryCompare( &existingVertex, &vertex, sizeof(vertex) ) )
		{
			return i;
		}
	}

	// Add new vertex
	m_vertices.PushBack( vertex );
	return face.m_numVertices++;
}

void BrushRenderData::AddIndex( Int32 faceID, Uint16 vertexIndex )
{
	BrushRenderFace& face = m_faces[ faceID ];
	ASSERT( vertexIndex < face.m_numVertices );
	m_indices.PushBack( vertexIndex );
	face.m_numIndices++;
}

void BrushRenderData::CreateRenderingProxies( IRenderScene* scene )
{
}

void BrushRenderData::DestroyRenderingProxies( IRenderScene* scene )
{	
}

void BrushRenderData::UpdateMapping( Int32 faceID, const Matrix& brushLocalToWorld, const BrushFaceMapping& mapping )
{
	ASSERT( faceID >= 0 && faceID < ( Int32 ) m_faces.Size() );
	m_faces[ faceID ].UpdateMapping( *this, brushLocalToWorld, mapping );
}

void BrushRenderData::SetSelection( Int32 faceID, Bool isSelected )
{
	ASSERT( faceID >= 0 && faceID < ( Int32 ) m_faces.Size() );
	
	// Update the proxy
	if ( m_renderingProxies.Size() )
	{
		IRenderProxy* faceProxy = m_renderingProxies[ faceID ];
		if ( faceProxy )
		{
			// Send render command to face render proxy
			( new CRenderCommand_SetSelectionFlag( faceProxy, isSelected ) )->Commit();
		}
	}
}

void BrushRenderData::SetHitProxy( Int32 faceID, const CHitProxyID& id )
{
	ASSERT( faceID >= 0 && faceID < ( Int32 ) m_faces.Size() );

	// Update the proxy
	if ( m_renderingProxies.Size() )
	{
		IRenderProxy* faceProxy = m_renderingProxies[ faceID ];
		if ( faceProxy )
		{
			// Send render command to face render proxy
			( new CRenderCommand_UpdateHitProxyID( faceProxy, id ) )->Commit();
		}
	}
}

void BrushRenderData::SetMaterial( Int32 faceID, IMaterial* material )
{
	ASSERT( faceID >= 0 && faceID < ( Int32 ) m_faces.Size() );
	m_faces[ faceID ].UpdateMaterial( material );
}

void BrushRenderData::Serialize( IFile& file )
{
	// Geometry data
	if ( file.IsCooker() && file.IsWriter() )
	{
		file << m_vertices;
		file << m_indices;
	}
	else
	{
		m_vertices.SerializeBulk( file );
		m_indices.SerializeBulk( file );
	}

	// Save faces
	file << m_faces;
}