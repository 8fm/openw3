/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "bezierTool.h"
#include "undoVertex.h"
#include "../../common/engine/IBezierOwner.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"


IMPLEMENT_ENGINE_CLASS( CEdBezierEdit );

CEdBezierEdit::CEdBezierEdit()
	: m_world( NULL )
	, m_viewport( NULL )
	, m_closestTrackComponent( NULL )
	, m_closestTrackVertex( -1 )
	, m_hoveredTrackVertex( -1 )
{
}

String CEdBezierEdit::GetCaption() const
{
	return TXT("Bezier edit");
}

void CEdBezierEdit::UpdateCurve(SEditedComponent * ec)
{
	if(!ec){return;}
	IBezierOwner2* owner = dynamic_cast< IBezierOwner2* >( ec->m_component );
	if(!owner){return;}
	const Uint32 size = ec->m_vertices.Size();
	QuadraticBezierNamespace::float2* data = new QuadraticBezierNamespace::float2[size];
	//const Matrix & localToWorld = GetLocalToWorld();
	for(Uint32 i=0;i<size;i++)
	{
		Vector globalPos = ec->m_vertices[i]->GetWorldPosition();
		data[i]=QuadraticBezierNamespace::float2(globalPos.X, globalPos.Y);
	}
	owner->GetBezier()->Update((QuadraticBezierNamespace::float2*)data, size);
	delete [] data;
}

static CVertexEditorEntity * CreateVertex( CComponent* vertexOwner, const Vector& worldPosition, Float topVertexHeight )
{
	ASSERT( vertexOwner );
	CWorld* world = vertexOwner->GetLayer()->GetWorld();
	CLayer* dynamicLayer = world->GetDynamicLayer();
	EntitySpawnInfo info;
	info.m_entityClass = CVertexEditorEntity::GetStaticClass();
	info.m_spawnPosition = worldPosition;
	CVertexEditorEntity * vertexEntity = Cast< CVertexEditorEntity >( dynamicLayer->CreateEntitySync( info ) );
	if ( ! vertexEntity )
	{
		return NULL;
	}
	vertexEntity->m_owner = vertexOwner;

	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = TXT("Vertex");
	vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );
	if ( topVertexHeight != 0.f )
	{
		Vector topVertexPos = vertexOwner->GetLocalToWorld().TransformVector( Vector( 0.f, 0.f, topVertexHeight ) );

		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("Vertex");
		spawnInfo.m_spawnPosition = topVertexPos;
		vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );
	}
	return vertexEntity;
}

Bool CEdBezierEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_world = world;
	m_viewport = viewport;
	world->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );
	for ( Uint32 i = 0; i < selection.Size(); i++ )
	{
		CComponent* component = selection[i];
		TDynArray< Vector > vertices;
		Bool				isClosed;
		Float				height;
		IBezierOwner2* owner = dynamic_cast< IBezierOwner2* >( selection[i] );
		if ( owner && owner->GetBezier() )
		{
			QuadraticBezierNamespace::Curve2* curve = owner->GetBezier();
			QuadraticBezierNamespace::float2 StartPoint = curve->m_Segments[0]+(curve->m_Segments[0]-curve->m_Segments[1]);
			int LastIndex = curve->NumPoints(curve->m_NumSegments)-2;
			QuadraticBezierNamespace::float2 LastPoint = curve->m_Segments[LastIndex]+(curve->m_Segments[LastIndex]-curve->m_Segments[LastIndex-1]);
			Vector Pos0( StartPoint.x, StartPoint.y, 0.f);
			vertices.PushBack(  Pos0 );
			for ( Int32 i=1; i < curve->NumPoints(curve->m_NumSegments); i+=2 )
			{
				Vector Pos( curve->m_Segments[i].x, curve->m_Segments[i].y, 0.f);
				vertices.PushBack(  Pos );
			}
			Vector Pos1( LastPoint.x, LastPoint.y, 0.f);
			vertices[vertices.Size()-1].Set3(Pos1);
			isClosed = false;
			height   = 0.f;
			if ( ! vertices.Empty() )
			{
				SEditedComponent * ec = new SEditedComponent();
				ec->m_component = component;
				ec->m_isClosed  = isClosed;
				ec->m_height	= height;
				for ( Uint32 i = 0; i < vertices.Size(); ++i )
				{
					CVertexEditorEntity * vertex = CreateVertex( component, vertices[ i ], height );
					ASSERT( vertex );
					ec->m_vertices.PushBack( vertex );
					vertex->m_index = i;
				}
				m_editedComponents.Insert( component, ec );
			}
		}
	}
	if ( m_editedComponents.Empty() )
	{
		WARN_EDITOR( TXT("No area components selected") );
		return false;
	}
	return true;
}

void CEdBezierEdit::End()
{
	// Kill vertices
	TEditedMap::iterator curr = m_editedComponents.Begin();
	TEditedMap::iterator last = m_editedComponents.End();
	for ( ; curr != last; ++curr )
	{
		SEditedComponent * ec = curr->m_second;
		ec->m_component->OnEditorEndVertexEdit();

		for ( Uint32 i = 0; i < ec->m_vertices.Size(); ++i )
		{
			if ( ! ec->m_vertices[i]->IsDestroyed() )
			{
				ec->m_vertices[i]->Destroy();
			}
		}
		delete ec;
	}
	m_editedComponents.Clear();
}

Bool CEdBezierEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager::CSelectionTransaction transaction(*m_world->GetSelectionManager());

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_world->GetSelectionManager()->DeselectAll();
	}

	// Select only edited vertices
	for ( Uint32 i = 0; i < objects.Size(); i++ )
	{
		CVertexEditorEntity * vertex = Cast< CVertexEditorEntity >( objects[i]->GetHitObject()->GetParent() );
		if ( vertex == NULL || vertex->IsSelected() )
		{
			continue;
		}

		ASSERT( m_editedComponents.KeyExist( static_cast< CComponent* >( vertex->m_owner ) ) );
		m_world->GetSelectionManager()->Select( vertex );
	}

	// Handled
	return true;
}

Bool CEdBezierEdit::OnDelete()
{
	// Enumerate vertices
	TDynArray< CEntity* > vertices;
	m_world->GetSelectionManager()->GetSelectedEntities( vertices );
	// Deselect'em all
	m_world->GetSelectionManager()->DeselectAll();

	// Invalidate fragment renderer data
	Reset();

	// Delete vertices
	for ( Uint32 i=0; i < vertices.Size(); i++ )
	{
		CVertexEditorEntity* vertex = Cast< CVertexEditorEntity >( vertices[i] );
		if ( vertex )
		{
			SEditedComponent** ec = m_editedComponents.FindPtr( static_cast< CComponent* >( vertex->m_owner ) );
			if ( ec )
			{
				ASSERT( *ec );
				if ( *ec )
				{	
					DeleteVertex( *ec, vertex->m_index );
				}
			}
		}
	}
	if ( m_viewport->GetUndoManager() )
	{
		CUndoVertexCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
	}

	// Handled
	return true;
}

Bool CEdBezierEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		return HandleActionClick( x, y );
	}
	return false;
}

Bool CEdBezierEdit::OnViewportTrack( const CMousePacket& packet )
{
	const Vector testPoint( packet.m_x, packet.m_y, 0.0f );

	// Invalidate fragment renderer data
	UpdateCurve(m_closestTrackComponent);
	if ( m_closestTrackComponent != NULL && m_hoveredTrackVertex >= 0 )
	{
		CVertexEditorEntity * vertex = m_closestTrackComponent->m_vertices[ m_hoveredTrackVertex ];
		vertex->m_hovered = false;
	}
	Reset();
	Float closestDistance = FLT_MAX;
	// Prepare projection
	CRenderFrameInfo info( packet.m_viewport );

	TEditedMap::iterator curr = m_editedComponents.Begin();
	TEditedMap::iterator last = m_editedComponents.End();
	for ( ; curr != last; ++curr )
	{
		SEditedComponent * ec = curr->m_second;

		const Uint32 numVerts = ec->m_vertices.Size();
		ASSERT( numVerts > 0 );
		const Uint32 numEdges = ec->m_isClosed ? numVerts : numVerts - 1;

		Vector pointW = ec->m_vertices[ 0 ]->GetWorldPosition();
		Vector pointA;
		Vector pointB;

		info.ProjectPoints( & pointW, & pointB, 1 );

		// Hit test edges
		for ( Uint32 j=0; j < numEdges; j++ )
		{
			pointA = pointB;

			pointW = ec->m_vertices[ (j+1) % numVerts ]->GetWorldPosition();
			info.ProjectPoints( & pointW, & pointB, 1 );

			Float edgeDistance = testPoint.DistanceToEdge( pointA, pointB );
			if ( edgeDistance < closestDistance )
			{
				closestDistance = edgeDistance;
				m_closestTrackComponent = ec;
				m_closestTrackVertex    = j;
			}		
		}
	}

	// Intersection
	if ( m_closestTrackComponent && m_closestTrackVertex != -1 )
	{
		const Uint32 numVerts = m_closestTrackComponent->m_vertices.Size();
		const Vector a = m_closestTrackComponent->m_vertices[ (m_closestTrackVertex  ) % numVerts ]->GetWorldPosition();
		const Vector b = m_closestTrackComponent->m_vertices[ (m_closestTrackVertex+1) % numVerts ]->GetWorldPosition();

		// Calculate average normal
		const Vector center = ( a + b ) * 0.5f;

		Vector averageNormal = m_closestTrackComponent->m_component->GetLocalToWorld().GetAxisZ().Normalized3();

		// Make sure the plane is always faced to the camera (in other case IntersectRay returns false)
		if ( Vector::Dot3( averageNormal, packet.m_rayDirection ) > 0.f )
		{
			averageNormal = -averageNormal;
		}

		Plane projectionPlane( averageNormal, center );

		// Calculate intersection with plane
		Vector intersectionPoint;
		Float intersectionDistance;
		if ( projectionPlane.IntersectRay( packet.m_rayOrigin, packet.m_rayDirection, intersectionPoint, intersectionDistance ) )
		{
			m_closestTrackPoint = intersectionPoint;
		}

		// Do hit proxy check on vertices
		CHitProxyMap map;
		CHitProxyObject* object = m_viewport->GetHitProxyAtPoint( map, packet.m_x, packet.m_y );
		CVertexEditorEntity * hoveredVertex = object ? Cast< CVertexEditorEntity >( object->GetHitObject()->GetParent() ) : NULL;

		if ( hoveredVertex )
		{
			m_hoveredTrackVertex = hoveredVertex->m_index;
			hoveredVertex->m_hovered = true;
		}
	}

	// Not filtered
	return false;
}

Bool CEdBezierEdit::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Draw closest edge
	if ( m_closestTrackComponent && m_closestTrackVertex != -1 )
	{
		// Draw segment
		Uint32 numVertices = m_closestTrackComponent->m_vertices.Size();

		if ( m_closestTrackVertex+1 < (Int32)numVertices || m_closestTrackComponent->m_isClosed )
		{
			const Vector a = m_closestTrackComponent->m_vertices[ m_closestTrackVertex ]->GetWorldPosition();
			const Vector b = m_closestTrackComponent->m_vertices[ (m_closestTrackVertex+1) % numVertices ]->GetWorldPosition();
			frame->AddDebugLine( a, b, Color::YELLOW, true );
		}
	}
	// Not filtered
	return false;
}

Bool CEdBezierEdit::HandleActionClick( Int32 x, Int32 y )
{
	// Add/remove vertex
	if ( m_closestTrackComponent && m_closestTrackVertex != -1 )
	{
		if ( RIM_IS_KEY_DOWN( IK_Alt ) )
		{
			// If we hit existing vertex delete it, otherwise add new
			if ( m_hoveredTrackVertex > -1 )
			{
				DeleteVertex( m_closestTrackComponent, m_hoveredTrackVertex );

				if ( m_viewport->GetUndoManager() )
				{
					CUndoVertexCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
				}

				// Invalidate fragment renderer data
				Reset();
			}
			else
			{
				Vector newPoint=m_closestTrackPoint;

				if ( true )
				{
					Int32  edge = m_closestTrackVertex;
					Uint32 insertPos;
					if ( m_closestTrackComponent->m_isClosed )
					{
						Uint32 numEdges = m_closestTrackComponent->m_vertices.Size();
						while ( edge < 0 )
						{
							edge += numEdges;
						}
						insertPos = ( edge + 1 ) % numEdges;
					}
					else
					{
						Int32 numEdges = (Int32) m_closestTrackComponent->m_vertices.Size() - 1;
						if ( edge < -1 )
						{
							edge = -1;
						}
						if ( edge > numEdges )
						{
							edge = numEdges;
						}
						insertPos = edge + 1;
					}

					CVertexEditorEntity * vertex = CreateVertex( m_closestTrackComponent->m_component, newPoint, m_closestTrackComponent->m_height );
					ASSERT( vertex );
					m_closestTrackComponent->m_vertices.Insert( insertPos, vertex );

					// Reindex vertices
					for ( Uint32 i = insertPos; i < m_closestTrackComponent->m_vertices.Size(); ++i )
					{
						m_closestTrackComponent->m_vertices[ i ]->m_index = i;
					}

					if ( m_viewport->GetUndoManager() )
					{
						CUndoVertexCreateDestroy::CreateStep( m_viewport->GetUndoManager(), m_viewport, vertex, true );
						CUndoVertexCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
					}
				}
			}
			UpdateCurve(m_closestTrackComponent);
			return true;
		}
	}

	// Do not filter
	return false;
}

void CEdBezierEdit::DeleteVertex( SEditedComponent * ec, Int32 index )
{
	ASSERT( index >= 0 );
	if ( index < 0 )
	{
		return;
	}
	ASSERT( ec );
	ASSERT( index < (Int32)ec->m_vertices.Size() );

	if(ec->m_vertices.Size()>3)
	{
		if ( m_viewport->GetUndoManager() )
		{
			CUndoVertexCreateDestroy::CreateStep( m_viewport->GetUndoManager(), m_viewport, ec->m_vertices[ index ], false );
		}
		else
		{
			ec->m_vertices[ index ]->Destroy();
		}

		ec->m_vertices.Erase( ec->m_vertices.Begin() + index );

		// Reindex vertices
		for ( Uint32 i = index; i < ec->m_vertices.Size(); ++i )
		{
			ec->m_vertices[ i ]->m_index = i;
		}
	}

}

void CEdBezierEdit::OnVertexAdded( CVertexEditorEntity * vertex )
{
	SEditedComponent ** ec = m_editedComponents.FindPtr( static_cast< CComponent* >( vertex->m_owner ) );
	if ( ec )
	{
		ASSERT( *ec );
		if ( *ec )
		{
			(*ec)->m_vertices.Insert( vertex->m_index, vertex );

			// Reindex vertices
			for ( Uint32 i = vertex->m_index+1; i < (*ec)->m_vertices.Size(); ++i )
			{
				(*ec)->m_vertices[ i ]->m_index = i;
			}
		}
	}
}

void CEdBezierEdit::OnVertexRemoved( CVertexEditorEntity * vertex )
{
	SEditedComponent ** ec = m_editedComponents.FindPtr( static_cast< CComponent* >( vertex->m_owner ) );
	if ( ec )
	{
		ASSERT( *ec );
		if ( *ec )
		{
			(*ec)->m_vertices.Erase( (*ec)->m_vertices.Begin() + vertex->m_index );

			// Reindex vertices
			for ( Uint32 i = vertex->m_index; i < (*ec)->m_vertices.Size(); ++i )
			{
				(*ec)->m_vertices[ i ]->m_index = i;
			}
		}
	}
}

void CEdBezierEdit::Reset()
{
	// Reset tracking info
	m_closestTrackComponent = NULL;
	m_closestTrackVertex = -1;
	m_closestTrackPoint.SetZeros();
	m_hoveredTrackVertex = -1;
}
