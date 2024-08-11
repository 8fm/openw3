/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicLayer.h"
#include "layerStorage.h"
#include "entity.h"
#include "../core/memoryFileAnalizer.h"

IMPLEMENT_ENGINE_CLASS( CDynamicLayer );

CDynamicLayer::CDynamicLayer()
	: m_storage( nullptr )
{
}

void CDynamicLayer::SetStorage( CLayerStorage* storage )
{
	m_storage = storage;
}

CLayerStorage* CDynamicLayer::GetLayerStorage()
{
	return m_storage;
}

void CDynamicLayer::OnEntityAdded( const CEntity* entity )
{
	/*if ( entity->IsExactlyA< CEntity >() )
	{
		if ( entity->GetEntityTemplate() )
		{
			LOG_ENGINE( TXT("Entity added to dynamic layer: %s  - %s"), entity->GetFriendlyName().AsChar(), entity->GetEntityTemplate()->GetDepotPath().AsChar() );
		}
		else
		{
			LOG_ENGINE( TXT("Entity added to dynamic layer: %s"), entity->GetFriendlyName().AsChar() );
		}
	}*/
}

void CDynamicLayer::OnEntityRemoved( const CEntity* entity )
{

}

Uint32 CDynamicLayer::CalcObjectDynamicDataSize() const
{
	return m_storage->GetDataSize();
}

Uint32 CDynamicLayer::CalcObjectsSize() const
{
	return CObjectMemoryAnalizer::CalcObjectSize( const_cast< CDynamicLayer* >( this ) );
}

void CDynamicLayer::AttachEntity( CEntity* entity )
{
	ASSERT( entity->IsInitialized() );
	ASSERT( !entity->IsAttached() );

	RestoreEntityState( entity );

	// Add to this layer
	m_entities.PushBack( entity );

	// Reparent
	entity->SetParent( this );

	// Attach entity
	entity->AttachToWorld( m_world );
	ASSERT( entity->IsAttached() );

	// Inform
	OnEntityAdded( entity );
}

void CDynamicLayer::DetachEntity( CEntity* entity )
{
	ASSERT( entity->GetParent() == this );
	ASSERT( entity->IsAttached() );

	StoreEntityState( entity );

	// Remove from this layer
	m_entities.Remove( entity );

	// Detach entity
	entity->DetachFromWorld( m_world );
	ASSERT( !entity->IsAttached() );

	// Inform
	OnEntityRemoved( entity );

	// Reparent
	entity->SetParent( NULL );
}