/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "areaVertexEdit.h"
#include "undoVertex.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/pathComponent.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CEdVertexEdit );

CEdVertexEdit::CEdVertexEdit()
	: m_world( NULL )
	, m_viewport( NULL )
	, m_closestTrackComponent( NULL )
	, m_closestTrackVertex( -1 )
	, m_hoveredTrackVertex( -1 )
{
}

String CEdVertexEdit::GetCaption() const
{
	return TXT("Vertex edit");
}

static CVertexEditorEntity * CreateVertex( CComponent* vertexOwner, const Vector& worldPosition, Float topVertexHeight /*= 0.f*/ )
{
	ASSERT( vertexOwner );

	// Get world to create vertex in
	CWorld* world = vertexOwner->GetLayer()->GetWorld();
	CLayer* dynamicLayer = world->GetDynamicLayer();

	// Setup spawn info
	EntitySpawnInfo info;
	info.m_entityClass = CVertexEditorEntity::GetStaticClass();
	info.m_spawnPosition = worldPosition;
	
	// Create vertex entity
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
		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("Vertex");
		spawnInfo.m_spawnPosition = vertexOwner->GetLocalToWorld().TransformVector( Vector( 0.f, 0.f, topVertexHeight ) );
		vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );
	}

	return vertexEntity;
}

Bool CEdVertexEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;

	viewport->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Start editing
	for ( Uint32 i = 0; i < selection.Size(); i++ )
	{
		CComponent* component = selection[i];

		TDynArray< Vector > vertices;
		Bool				isClosed;
		Float				height;

		if ( component->OnEditorBeginVertexEdit( vertices, isClosed, height ) && ! vertices.Empty() )
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

	// No area components to edit
	if ( m_editedComponents.Empty() )
	{
		WARN_EDITOR( TXT("No area components selected") );
		return false;
	}

	// Initialized
	return true;
}

void CEdVertexEdit::End()
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

Bool CEdVertexEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
    CSelectionManager::CSelectionTransaction transaction(*m_viewport->GetSelectionManager());

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_viewport->GetSelectionManager()->DeselectAll();
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
		m_viewport->GetSelectionManager()->Select( vertex );
	}
	
	// Handled
	return true;
}

Bool CEdVertexEdit::OnDelete()
{
	// Enumerate vertices
	TDynArray< CEntity* > vertices;
	m_viewport->GetSelectionManager()->GetSelectedEntities( vertices );
	// Deselect'em all
	m_viewport->GetSelectionManager()->DeselectAll();

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

Bool CEdVertexEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		return HandleActionClick( x, y );
	}
	return false;
}

Bool CEdVertexEdit::OnViewportTrack( const CMousePacket& packet )
{
	const Vector testPoint( packet.m_x, packet.m_y, 0.0f );

	// Invalidate fragment renderer data
	if ( m_closestTrackComponent != NULL && m_hoveredTrackVertex >= 0 && m_hoveredTrackVertex < m_closestTrackComponent->m_vertices.SizeInt() )
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

Bool CEdVertexEdit::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Draw closest edge
	if ( m_closestTrackComponent && m_closestTrackVertex != -1 )
	{
		// Draw segment
		Uint32 numVertices = m_closestTrackComponent->m_vertices.Size();

		if ( m_closestTrackVertex < (Int32)numVertices )
		{
			const Vector a = m_closestTrackComponent->m_vertices[ m_closestTrackVertex ]->GetWorldPosition();
			const Vector b = m_closestTrackComponent->m_vertices[ (m_closestTrackVertex+1) % numVertices ]->GetWorldPosition();
			frame->AddDebugLine( a, b, Color::YELLOW, true );
		}
	}

	// Not filtered
	return false;
}

Bool CEdVertexEdit::GetNearestPositionAt( CWorld* world, const Vector& pos, Vector& worldPos, Vector& worldNormal, Vector& directionNormal, Float maxDistance )
{
	const Vector directions[6] = {
		Vector( -1, 0, 0 ),
		Vector( 1, 0, 0 ),
		Vector( 0, -1, 0 ),
		Vector( 0, 1, 0 ),
		Vector( 0, 0, -1 ),
		Vector( 0, 0, 1 )
	};

	Float nearestFound;
	Bool anyFound = false;

	// Send a bunch of random rays
	for ( Int32 i=0; i < 6; ++i )
	{
		Vector end = directions[i] * maxDistance;
		Vector wpos, wnormal;

		// Check for intersection
		if ( world->CheckForSegmentIntersection( pos, end, wpos, wnormal ) )
		{
			Float distance = pos.DistanceTo( wpos );
			if ( distance <= maxDistance && ( !anyFound || distance < nearestFound ) )
			{
				nearestFound = distance;
				anyFound = true;
				worldPos = wpos;
				worldNormal = wnormal;
				directionNormal = directions[i];
			}
		}
	}

	return anyFound;
}

Bool CEdVertexEdit::HandleActionClick( Int32 x, Int32 y )
{
	// Add/remove vertex
	if ( m_closestTrackComponent && m_closestTrackVertex != -1 )
	{
		if ( RIM_IS_KEY_DOWN( IK_X ) && m_closestTrackComponent->m_component->IsA<CPathComponent>() )
		{
			CPathComponent* path = static_cast< CPathComponent* >( m_closestTrackComponent->m_component );
			for ( auto it=m_closestTrackComponent->m_vertices.Begin(); it != m_closestTrackComponent->m_vertices.End(); ++it )
			{
				CVertexEditorEntity* vertex = *it;
				Vector oldPosition = vertex->GetWorldPosition();
				Vector newPosition, newNormal, newDirection;

				if ( GetNearestPositionAt( m_world, oldPosition, newPosition, newNormal, newDirection, 0.5f ) )
				{
					// Nudge the position a bit away from the surface
					newPosition -= newDirection*0.05f;

					// Update the position
					vertex->SetPosition( newPosition );
				}
			}
			return true;
		}
		else if ( RIM_IS_KEY_DOWN( IK_Alt ) )
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
				Vector newPoint;
				Int32 insertPos;
				if ( m_closestTrackComponent->m_component->OnEditorVertexInsert( m_closestTrackVertex, m_closestTrackPoint, newPoint, insertPos ) )
				{
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

			return true;
		}
		else if ( RIM_IS_KEY_DOWN( IK_Ctrl ) && !RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			Vector newPoint, desiredPoint = m_closestTrackPoint, clickPos, clickNormal;
			if ( m_world->ConvertScreenToWorldCoordinates( wxTheFrame->GetWorldEditPanel()->GetViewport(), x, y, clickPos, &clickNormal ) )
			{
				desiredPoint = clickPos + clickNormal*0.05f;
			}

			Int32 insertPos;
			if ( m_closestTrackComponent->m_component->OnEditorVertexInsert( 2147483647, desiredPoint, newPoint, insertPos ) )
			{
				Int32  edge = m_closestTrackVertex;

				CVertexEditorEntity * vertex = CreateVertex( m_closestTrackComponent->m_component, newPoint, m_closestTrackComponent->m_height );
				ASSERT( vertex );
				m_closestTrackComponent->m_vertices.Insert( insertPos, vertex );

				// Reindex vertices
				for ( Uint32 i = 0; i < m_closestTrackComponent->m_vertices.Size(); ++i )
				{
					m_closestTrackComponent->m_vertices[ i ]->m_index = i;
				}

				if ( m_viewport->GetUndoManager() )
				{
					CUndoVertexCreateDestroy::CreateStep( m_viewport->GetUndoManager(), m_viewport, vertex, true );
					CUndoVertexCreateDestroy::FinishStep( m_viewport->GetUndoManager() );
				}
			}

			return true;
		}
	}

	// Do not filter
	return false;
}

void CEdVertexEdit::DeleteVertex( SEditedComponent * ec, Int32 index )
{
	ASSERT( index >= 0 );
	if ( index < 0 )
	{
		return;
	}
	ASSERT( ec );
	ASSERT( index < (Int32)ec->m_vertices.Size() );

	if ( ec->m_component->OnEditorVertexDestroy( index ) )
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

void CEdVertexEdit::OnVertexAdded( CVertexEditorEntity * vertex )
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

void CEdVertexEdit::OnVertexRemoved( CVertexEditorEntity * vertex )
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

void CEdVertexEdit::Reset()
{
	// Reset tracking info
	m_closestTrackComponent = NULL;
	m_closestTrackVertex = -1;
	m_closestTrackPoint.SetZeros();
	m_hoveredTrackVertex = -1;
}
