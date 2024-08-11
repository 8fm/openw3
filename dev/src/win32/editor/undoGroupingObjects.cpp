/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "undoGroupingObjects.h"
#include "../../common/engine/entityGroup.h"

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CUndoGroupObjects )

CUndoGroupObjects::CUndoGroupObjects( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups, Bool grouping )
	: IUndoStep( undoManager )
	, m_grouping( grouping )
{
	for( auto it = groups.Begin(); it != groups.End(); ++it )
	{
		SUndoEntityGroupInfo info;
		info.m_entities = ( *it )->GetEntities();
		info.m_parentLayer = ( *it )->GetLayer();
		info.m_name = ( *it )->GetName();
		m_entitiesInGroups.PushBack( std::move( info ) );
	}
}

/*static*/ 
void CUndoGroupObjects::CreateGroupStep( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups  )
{
	CUndoGroupObjects* step = new CUndoGroupObjects( undoManager, groups, false );
	step->PushStep();
}

/*static*/ 
void CUndoGroupObjects::CreateUngroupStep( CEdUndoManager& undoManager, const TDynArray< CEntityGroup* >& groups )
{
	CUndoGroupObjects* step = new CUndoGroupObjects( undoManager, groups, true );
	step->PushStep();
}

void CUndoGroupObjects::DoStep( Bool grouping )
{
	if( grouping == true )
	{
		for( auto it = m_entitiesInGroups.Begin(); it != m_entitiesInGroups.End(); ++it )
		{
			SUndoEntityGroupInfo groupInfo = ( *it );

			CEntityGroup* group = groupInfo.m_entities[0]->GetContainingGroup();
			if( group != nullptr )
			{
				group->Destroy();
				group = nullptr;
			}
		}
	}
	else
	{
		for( auto it = m_entitiesInGroups.Begin(); it != m_entitiesInGroups.End(); ++it )
		{
 			SUndoEntityGroupInfo groupInfo = ( *it );
 
 			// Create new group
 			EntitySpawnInfo info;
 			info.m_entityClass = CEntityGroup::GetStaticClass();
 			info.m_name = groupInfo.m_name;
 			info.m_spawnPosition = groupInfo.m_entities[ 0 ]->GetPosition();
 			info.m_spawnRotation = groupInfo.m_entities[ 0 ]->GetRotation();
 			info.AddHandler( new TemplateInfo::CSpawnEventHandler() );
 
 			// Add entities to the group
 			CEntityGroup* newGroup = Cast< CEntityGroup >( groupInfo.m_parentLayer->CreateEntitySync( info ) );
 			newGroup->AddEntities( groupInfo.m_entities );
		}
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

/*virtual*/ 
void CUndoGroupObjects::DoUndo()
{
	DoStep( !m_grouping );
}

/*virtual*/ 
void CUndoGroupObjects::DoRedo()
{
	DoStep( m_grouping );
}

/*virtual*/ 
String CUndoGroupObjects::GetName()
{
	return String( m_grouping ? TXT("ungrouping") : TXT("grouping") ) + TXT(" objects");
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CUndoLockGroupObjects )

CUndoLockGroupObjects::CUndoLockGroupObjects( CEdUndoManager& undoManager, CEntityGroup* group, Bool locking )
	: IUndoStep( undoManager )
	, m_locking( locking )
	, m_group( group )
{
	/* intentionally empty */
}

/*static*/ 
void CUndoLockGroupObjects::CreateStep( CEdUndoManager& undoManager, CEntityGroup* group, Bool locking )
{
	CUndoLockGroupObjects* step = new CUndoLockGroupObjects( undoManager, group, locking );
	step->PushStep();
}

void CUndoLockGroupObjects::DoStep( Bool locking )
{
	if( locking == true )
	{
		m_group->Unlock();
	}
	else
	{
		m_group->Lock();
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

/*virtual*/ 
void CUndoLockGroupObjects::DoUndo()
{
	DoStep( !m_locking );
}

/*virtual*/ 
void CUndoLockGroupObjects::DoRedo()
{
	DoStep( m_locking );
}

/*virtual*/ 
String CUndoLockGroupObjects::GetName()
{
	return String( ( m_locking == true ) ? TXT("unlocking") : TXT("locking") ) + TXT(" groups");
}

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CUndoRemoveObjectFromGroup )

CUndoRemoveObjectFromGroup::CUndoRemoveObjectFromGroup( CEdUndoManager& undoManager, const TDynArray< CEntity* >& entities, Bool removing )
	: IUndoStep( undoManager )
	, m_removing( removing )
{
	for( auto it = entities.Begin(); it != entities.End(); ++it )
	{
		CEntity* entity = ( *it );
		if( entity->GetPartOfAGroup() == true )
		{
			CEntityGroup* group = entity->GetContainingGroup();
			TDynArray< CEntity* >& entities = m_info.GetRef( group );
			entities.PushBack( entity );
		}
	}
}

/*static*/ 
void CUndoRemoveObjectFromGroup::CreateStep( CEdUndoManager& undoManager, const TDynArray< CEntity* >& allEntities, Bool removing )
{
	CUndoRemoveObjectFromGroup* step = new CUndoRemoveObjectFromGroup( undoManager, allEntities, removing );
	step->PushStep();
}

void CUndoRemoveObjectFromGroup::DoStep( Bool removing )
{
	if( removing == true )
	{
		for( auto infoItr = m_info.Begin(); infoItr != m_info.End(); ++infoItr )
		{
			CEntityGroup* entityGroup = infoItr.Key();
			entityGroup->AddEntities( infoItr.Value() );
		}

		SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
	}
	else
	{
		for( auto infoItr = m_info.Begin(); infoItr != m_info.End(); ++infoItr )
		{
			const TDynArray< CEntity* >& entities = infoItr.Value();

			for( auto entityItr = entities.Begin(); entityItr != entities.End(); ++entityItr )
			{
				CEntity* entity = ( *entityItr );
				CEntityGroup* entityGroup = nullptr;
				entityGroup = entity->GetContainingGroup();
				entityGroup->DeleteEntity( entity );
			}
		}

		SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
	}

	SEvents::GetInstance().QueueEvent( CNAME( UpdateSceneTree ), nullptr );
}

/*virtual*/ 
void CUndoRemoveObjectFromGroup::DoUndo()
{
	DoStep( true );
}

/*virtual*/ 
void CUndoRemoveObjectFromGroup::DoRedo()
{
	DoStep( false );
}

/*virtual*/ 
String CUndoRemoveObjectFromGroup::GetName()
{
	return String( ( m_removing == true ) ? TXT("Removing") : TXT("Adding") ) + TXT(" objects from group");
}