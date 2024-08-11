/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushFace.h"
#include "brushCompiledData.h"
#include "hitProxyMap.h"
#include "hitProxyObject.h"
#include "hitProxyId.h"
#include "layer.h"
#include "material.h"

IMPLEMENT_ENGINE_CLASS( CBrushFace );
IMPLEMENT_RTTI_ENUM( EBrushFaceMapping );

static BrushRenderData* GetRenderData( const CBrushFace* face )
{
	// Find the layer
	CLayer* layer = face->FindParent< CLayer >();
	if ( layer )
	{
		CBrushCompiledData* compiledData = layer->GetBrushCompiledData();
		if ( compiledData )
		{
			return &compiledData->GetRenderData();
		}
	}

	// Not found
	return NULL;
}

CBrushFace::CBrushFace()
	: m_material( NULL )
	, m_renderFaceID( -1 )
{
}

void CBrushFace::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Data
	file << m_vertices;
	file << m_polygons;
}

void CBrushFace::RebuildNormals()
{
	TDynArray< Vector > accumulatedNormals;

	// Initialize
	accumulatedNormals.Resize( m_vertices.Size() );
	for ( Uint32 i=0; i<accumulatedNormals.Size(); i++ )
	{
		accumulatedNormals[i] = Vector::ZEROS;
	}

	// Rebuild polygon normals and accumulate vertex normals
	for ( Uint32 i=0; i<m_polygons.Size(); i++ )
	{
		Polygon& poly = m_polygons[i];

		// Update normal
		const BrushVertex& v0 = m_vertices[ poly.m_indices[0] ];
		const BrushVertex& v1 = m_vertices[ poly.m_indices[1] ];
		const BrushVertex& v2 = m_vertices[ poly.m_indices[2] ];
		poly.m_plane = Plane( v0.Position(), v1.Position(), v2.Position() );

		// Accumulate vertex normals
		for ( Uint32 j=0; j<poly.m_indices.Size(); j++ )
		{
			Vector normalToAdd = poly.m_plane.NormalDistance;
			normalToAdd.W = 1.0f;
			accumulatedNormals[ poly.m_indices[j] ] += normalToAdd;
		}
	}

	// Normalize and update
	for ( Uint32 i=0; i<accumulatedNormals.Size(); i++ )
	{
		// Normalize
		ASSERT( accumulatedNormals[i].W );	// Unused vertex, that's illegal
		accumulatedNormals[i].Normalize3();

		// Update vertices
		m_vertices[i].m_normal = accumulatedNormals[i];
	}
}

void CBrushFace::GeneratePolygons( TDynArray< CPolygon* >& polygons ) const
{
	// Extract vertices
	TDynArray< Vector > vertices;
	for ( Uint32 i=0; i<m_vertices.Size(); i++ )
	{
		vertices.PushBack( m_vertices[i].Position() );
	}

	// Generate polygons using given local vertices
	for ( Uint32 i=0; i<m_polygons.Size(); i++ )
	{
		// Get source polygon
		const Polygon& sourcePolygon = m_polygons[i];
		ASSERT( sourcePolygon.m_indices.Size() );


		// Create polygon
		CPolygon* polygon = new CPolygon( vertices.TypedData(), sourcePolygon.m_indices.TypedData(), sourcePolygon.m_indices.Size() );
		polygons.PushBack( polygon );
	}
}

void CBrushFace::GeneratePolygons( TDynArray< CBrushPolygon* >& polygons ) const
{
	// Generate polygons using given local vertices
	for ( Uint32 i=0; i<m_polygons.Size(); i++ )
	{
		// Get source polygon
		const Polygon& sourcePolygon = m_polygons[i];
		ASSERT( sourcePolygon.m_indices.Size() );

		// Create polygon
		CBrushPolygon* polygon = new CBrushPolygon( m_vertices.TypedData(), sourcePolygon.m_indices.TypedData(), sourcePolygon.m_indices.Size() );
		polygons.PushBack( polygon );
	}
}

void CBrushFace::SetMapping( const BrushFaceMapping& mapping )
{
	// Update mapping
	m_mapping = mapping;

	// Update material in the render proxy face
	if ( m_renderFaceID != -1 )
	{
		BrushRenderData* data = GetRenderData( this );
		if ( data )
		{
			data->UpdateMapping( m_renderFaceID, GetBrush()->GetLocalToWorld(), m_mapping );
		}
	}

	// Mark level as modified
	MarkModified();
}

void CBrushFace::SetMaterial( IMaterial* material )
{
	// Update only if changed
	if ( material != m_material.Get() )
	{
		// Change material
		m_material = material;

		// Update material in the render proxy face
		if ( m_renderFaceID != -1 )
		{
			BrushRenderData* data = GetRenderData( this );
			if ( data )
			{
				data->SetMaterial( m_renderFaceID, m_material.Get() );
			}
		}
	}

	// Mark layer as modified
	MarkModified();
}

void CBrushFace::SetSelection( Bool isSelected )
{
	// Update only if changed
	if ( m_isSelected != isSelected )
	{
		// Change flag
		m_isSelected = isSelected;

		// Update selection flag in the render proxy face
		if ( m_renderFaceID != -1 )
		{
			BrushRenderData* data = GetRenderData( this );
			if ( data )
			{
				data->SetSelection( m_renderFaceID, m_isSelected );
			}
		}
	}
}

void CBrushFace::GenerateHitProxies( CHitProxyMap& map )
{
	// Register simple hit proxy object for this component
	CHitProxyID id = map.RegisterHitProxy( new CHitProxyObject( this ) );
	if ( m_renderFaceID != -1 )
	{
		// Update hit proxy ID in the render proxy face
		if ( m_renderFaceID != -1 )
		{
			BrushRenderData* data = GetRenderData( this );
			if ( data )
			{
				data->SetHitProxy( m_renderFaceID, id );
			}
		}
	}
}

CBrushFace* CBrushFace::CreateCopy( CObject* owner, Int32 faceFlipFlag, IMaterial* material ) const
{
	ASSERT( owner );

	// Create new face
	CBrushFace* newFace = ::CreateObject< CBrushFace >( owner );
	newFace->m_material = material ? material : m_material.Get();
	newFace->m_mapping = m_mapping;
	newFace->m_vertices = m_vertices;

	// Copy and swap polygons
	newFace->m_polygons.Resize( m_polygons.Size() );
	for ( Uint32 i=0; i<m_polygons.Size(); i++ )
	{
		const Polygon& sourcePoly = m_polygons[i];
		Polygon& newPoly = newFace->m_polygons[i];

		// Copy plane
		const Bool isFlipped = ( faceFlipFlag < 0 );
		if ( isFlipped )
		{
			// Use swapped geometry
			newPoly.m_plane = -sourcePoly.m_plane;
			newPoly.m_indices.Resize( sourcePoly.m_indices.Size() );
			for ( Uint32 j=0; j<newPoly.m_indices.Size(); j++ )
			{
				const Uint32 swappedIndex = ( newPoly.m_indices.Size()-1 ) - j;
				newPoly.m_indices[ j ] = sourcePoly.m_indices[ swappedIndex ];
			}
		}
		else
		{
			// Use normal crap
			newPoly.m_plane = sourcePoly.m_plane;
			newPoly.m_indices = sourcePoly.m_indices;
		}
	}

	// Return created face
	return newFace;
}
