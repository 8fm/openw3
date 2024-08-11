/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "brushComponent.h"
#include "brushCSG.h"
#include "brushFace.h"
#include "renderFragment.h"
#include "world.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CBrushComponent );
IMPLEMENT_RTTI_ENUM( EBrushCSGType );

static Bool GBrushFaceEditMode = false;

CBrushComponent::CBrushComponent()
	: m_brushIndex( -1 )
{
}

void CBrushComponent::SetCSGType( EBrushCSGType csgType )
{
	if ( CanModify() )
	{
		// Set CSG type and schedule recompilation
		m_csgType = csgType;
		MarkModified();
	}
}

void CBrushComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Register in drawing groups
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BuildingBrush );
}

void CBrushComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Register in drawing groups
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BuildingBrush );
}

static Color GetBrushColor( EBrushCSGType csgType, Bool isSelected )
{
	if ( csgType == CSG_Edit )
	{
		return isSelected ? Color( 255, 0, 0 ) : Color( 127, 0,0 );
	}
	else if ( csgType == CSG_Addtive )
	{
		return isSelected ? Color( 0, 0, 255 ) : Color( 0, 0, 127 );
	}
	else if ( csgType == CSG_Subtractive )
	{
		return isSelected ? Color( 255, 255, 0 ) : Color( 127, 127, 0 );
	}
	else if ( csgType == CSG_Detail )
	{
		return isSelected ? Color( 0, 255, 255 ) : Color( 0, 127, 127 );
	}

	return Color::WHITE;
}

void CBrushComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw wireframe brush only when the building brushes are visible
	if ( flag == SHOW_BuildingBrush )
	{
		// Non building brush brushes are drawn only when selected
		const bool isBuildBrush = ( m_csgType == CSG_Edit );
		const Bool isSelected = IsSelected();
		if ( isBuildBrush || isSelected )
		{
			// Get the draw color
			const Color drawColorBase  = GetBrushColor( m_csgType, isSelected );

			// Build colors for each bruch element
			ASSERT( 255 == drawColorBase.A );
			const Color drawColorPoint = drawColorBase;
			const Color drawColorLine  = drawColorBase;
			const Color drawColorFace  = Color ( drawColorBase.R, drawColorBase.G, drawColorBase.B, 96 );

			// Draw wireframe brush
			TDynArray< DebugVertex > lineSegments;
			for ( Uint32 i=0; i<m_faces.Size(); i++ )
			{
				CBrushFace* face = m_faces[i];

				// Transform vertices to world space
				TDynArray< Vector > worldVertices( face->m_vertices.Size() );
				for ( Uint32 j=0; j<face->m_vertices.Size(); j++ )
				{
					const Vector localPos = face->m_vertices[j].Position();
					worldVertices[ j ] = GetLocalToWorld().TransformPoint( localPos );
				}

				// Draw vertices
#ifndef NO_COMPONENT_GRAPH
				if ( IsSelected() )
				{
					for ( Uint32 j=0; j<face->m_vertices.Size(); j++ )
					{
						frame->AddSprite( worldVertices[j], 0.1f, drawColorPoint, GetHitProxyID(), NULL );
					}
				}
#endif

				// Emit polygon line segments
				for ( Uint32 j=0; j<face->m_polygons.Size(); j++ )
				{
					const CBrushFace::Polygon& poly = face->m_polygons[j];
					for ( Uint32 k=0; k<poly.m_indices.Size(); k++ )
					{
						const Uint32 vertexA = poly.m_indices[ (k+0) % poly.m_indices.Size() ];
						const Uint32 vertexB = poly.m_indices[ (k+1) % poly.m_indices.Size() ];
						lineSegments.PushBack( DebugVertex( worldVertices[ vertexA ], drawColorLine ) );
						lineSegments.PushBack( DebugVertex( worldVertices[ vertexB ], drawColorLine ) );
					}
				}
			}

			// Emit lines
			const Bool isOverlay = isSelected || isBuildBrush;
			frame->AddDebugLines( lineSegments.TypedData(), lineSegments.Size(), isOverlay );

			// If it's a selected builder brush emit also the transparent faces
			if ( isSelected && isBuildBrush )
			{
				// Emit indices
				TDynArray< Uint16 > polyIndices;
				TDynArray< DebugVertex > polyVertices;
				for ( Uint32 i=0; i<m_faces.Size(); i++ )
				{
					CBrushFace* face = m_faces[i];

					// Emit vertices
					Uint16 baseIndex = ( Uint16 ) polyVertices.Size();
					for ( Uint32 j=0; j<face->m_vertices.Size(); j++ )
					{
						const Vector localPos = face->m_vertices[j].Position();
						new ( polyVertices ) DebugVertex( localPos, drawColorFace.ToUint32() );
					}

					// Emit polygons
					for ( Uint32 j=0; j<face->m_polygons.Size(); j++ )
					{
						const CBrushFace::Polygon& poly = face->m_polygons[j];
						for ( Uint32 k=2; k<poly.m_indices.Size(); k++ )
						{
							polyIndices.PushBack( baseIndex + ( Uint16 ) poly.m_indices[k] );
							polyIndices.PushBack( baseIndex + ( Uint16 ) poly.m_indices[k-1] );
							polyIndices.PushBack( baseIndex + ( Uint16 ) poly.m_indices[0] );
						}
					}
				}

				// Emit faces
				new ( frame ) CRenderFragmentDebugPolyList( 
					frame, 
					GetLocalToWorld(), 
					polyVertices.TypedData(), 
					polyVertices.Size(), 
					polyIndices.TypedData(), 
					polyIndices.Size(),
					RSG_DebugTransparent,
					true );
			}
		}
	}
}

void CBrushComponent::OnGenerateEditorHitProxies( CHitProxyMap& map )
{
	// Generate main hit proxy
	TBaseClass::OnGenerateEditorHitProxies( map );

	// Generate hit proxies for faces
	if ( GBrushFaceEditMode )
	{
		for ( Uint32 i=0; i<m_faces.Size(); i++ )
		{
			CBrushFace* face = m_faces[i];
			face->GenerateHitProxies( map );
		}
	}
}

void CBrushComponent::OnUpdateBounds()
{
	// Reset
	m_boundingBox.Clear();

	// Update bounds with all faces points in the local space
	for ( Uint32 i=0; i<m_faces.Size(); i++ )
	{
		CBrushFace* face = m_faces[i];
		for ( Uint32 j=0; j<face->m_vertices.Size(); j++ )
		{
			Vector worldPosition = GetLocalToWorld().TransformPoint( face->m_vertices[j].Position() );
			m_boundingBox.AddPoint( worldPosition );
		}
	}
}

/*void CBrushComponent::CompileBSP()
{
	// Collect polygons
	TDynArray< CPolygon* > polygons;
	for ( Uint32 i=0; i<m_faces.Size(); i++ )
	{
		// Generate face polygons
		CBrushFace* face = m_faces[i];
		face->GeneratePolygons( polygons );
	}

	// Compile BSP
	m_localBSP.Build( polygons );
	//m_localBSP.Print();
	MarkModified();

	// Request recompilation
	ScheduleUpdateTransform();
}
*/

void CBrushComponent::RemoveGeometry()
{
	// Discard existing faces
	for ( Uint32 i=0; i<m_faces.Size(); i++ )
	{
		CBrushFace* face = m_faces[i];
		face->Discard();
	}

	// Cleanup data
	m_faces.Clear();
}

Bool CBrushComponent::CopyData( const CBrushComponent* sourceData, Int32 flipSideFlag, IMaterial* materialToUse, EBrushCSGType csgType )
{
	// Do not copy from self or from invalid brushes
	if ( sourceData == this || !sourceData )
	{
		return false;
	}

	// Remove existing geometry
	RemoveGeometry();

	// Copy faces
	const TDynArray< CBrushFace* >& sourceFaces = sourceData->GetFaces();
	for ( Uint32 i=0; i<sourceFaces.Size(); i++ )
	{
		// Get source face
		const CBrushFace* sourceFace = sourceFaces[i];

		// Create new face
		CBrushFace* newFace = sourceFace->CreateCopy( this, flipSideFlag, materialToUse );
		if ( newFace )
		{
			m_faces.PushBack( newFace );
		}
	}

	// Set new CSG type
	m_csgType = csgType;
	return true;
}
