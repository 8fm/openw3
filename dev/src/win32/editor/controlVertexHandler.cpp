
#include "build.h"
#include "controlVertexHandler.h"
#include "../../common/game/actionAreaVertex.h"
#include "../../common/engine/dynamicLayer.h"


CEdControlVertexHandler::ControlVertex::ControlVertex( 
		IEditorNodeMovementHook* controlledObject, Uint32 id,
		const Vector& position, const EulerAngles& rotation, Float scale
	)
	: m_controlledObject( controlledObject )
	, m_id( id )
	, m_position( position )
	, m_rotation( rotation )
	, m_scale( scale )
	{}

CEdControlVertexHandler::CEdControlVertexHandler( Bool rotatable, Bool scalable )
	: m_rotatable( rotatable )
	, m_scalable( scalable )
{
}

void CEdControlVertexHandler::DestroyVertexEntities()
{
	CWorld* world = GGame->GetActiveWorld();
	CSelectionManager::CSelectionTransaction transaction( *world->GetSelectionManager() );
	world->GetSelectionManager()->DeselectAll();

	// Destroy vertex entities
	for ( auto it = m_verticies.Begin(); it != m_verticies.End(); ++it )
	{
		if ( !(*it)->IsDestroyed() )
		{
			(*it)->Destroy();
		}
	}

	m_verticies.Clear();
}

void CEdControlVertexHandler::RebuildVertexEntities()
{
	CWorld* world = GGame->GetActiveWorld();
	CSelectionManager::CSelectionTransaction transaction( *world->GetSelectionManager() );
	DestroyVertexEntities();

	// Ask about vertices list
	TDynArray< ControlVertex > vertices;
	GetControlVertices( vertices );

	// Build new entities
	for ( auto vertIt = vertices.Begin(); vertIt != vertices.End(); ++vertIt )
	{
		EntitySpawnInfo info;
		info.m_entityClass = CVertexEditorEntity::GetStaticClass();
		info.m_spawnPosition = vertIt->m_position;

		CVertexEditorEntity* vertexEntity = Cast< CVertexEditorEntity >( world->GetDynamicLayer()->CreateEntitySync( info ) );
		ASSERT( vertexEntity, TXT("Failed to create the vertex entity for the stripe's control point") );
		if ( !vertexEntity )
		{
			continue;
		}

		vertexEntity->m_index     = vertIt->m_id;
		vertexEntity->m_rotatable = m_rotatable;
		vertexEntity->m_scalable  = m_scalable;
		
		if ( m_rotatable )
		{
			vertexEntity->SetRotation( vertIt->m_rotation );
		}

		if ( m_scalable )
		{
			vertexEntity->SetScale( Vector( vertIt->m_scale, vertIt->m_scale, vertIt->m_scale ) );
		}

		// set owner after setting rotation and scale to avoid sending redundant events
		vertexEntity->m_owner = vertIt->m_controlledObject;

		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("Vertex");
		vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );

		m_verticies.PushBack( vertexEntity );
		world->GetSelectionManager()->Select( vertexEntity );
	}
}
