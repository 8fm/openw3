/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerMonitor.h"

#include "spawnTreeBaseEntry.h"

IMPLEMENT_ENGINE_CLASS( ISpawnTreeSpawnMonitorBaseInitializer )
IMPLEMENT_ENGINE_CLASS( ISpawnTreeSpawnMonitorInitializer )

///////////////////////////////////////////////////////////////////////////////
// ISpawnTreeSpawnMonitorBaseInitializer
///////////////////////////////////////////////////////////////////////////////

Bool ISpawnTreeSpawnMonitorBaseInitializer::IsSpawnable() const
{
	return GetClass() != GetStaticClass();
}

ISpawnTreeSpawnMonitorBaseInitializer::EOutput ISpawnTreeSpawnMonitorBaseInitializer::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	MonitorCreature( *instance, actor, entry, ESME_Spawned );
	return OUTPUT_SUCCESS;
}

void ISpawnTreeSpawnMonitorBaseInitializer::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	MonitorCreature( *instance, actor, entry, ESME_Lost );
}

void ISpawnTreeSpawnMonitorBaseInitializer::OnCreatureRemoval( CSpawnTreeInstance& instance, CActor* actor, ESpawnTreeCreatureRemovalReason removalReason, CBaseCreatureEntry* entry ) const
{
	MonitorCreature( instance, actor, entry, (removalReason == SPAWN_TREE_REMOVAL_KILLED) ? ESME_Killed : ESME_Lost );
}

Bool ISpawnTreeSpawnMonitorBaseInitializer::IsSpawnableOnPartyMembers() const
{
	return false;
}

String ISpawnTreeSpawnMonitorBaseInitializer::GetEditorFriendlyName() const 
{
	return TXT("SpawnMonitor");
}

///////////////////////////////////////////////////////////////////////////////
// ISpawnTreeSpawnMonitorInitializer
///////////////////////////////////////////////////////////////////////////////

void ISpawnTreeSpawnMonitorInitializer::MonitorCreature( CSpawnTreeInstance& instance, CActor* actor, CBaseCreatureEntry* entry, ESpawnMonitorEvent eventType ) const
{
	m_contextEntry = entry;
	m_contextInstance = &instance;

	THandle< CActor > actorHandle( actor );
	THandle< CBaseCreatureEntry > entryHandle( m_contextEntry );
	THandle< CEncounter > encounterHandle( instance.GetEncounter() );

	//Int32 numCreaturesSpawned = entry->GetNumCreaturesSpawned( instance );
	//Int32 numCreaturesToSpawn = entry->GetNumCreaturesToSpawn( instance );
	//Int32 numCreaturesDead = entry->GetNumCreaturesDead( instance );

	CName functionName;
	switch( eventType )
	{
	case ESME_Killed:
		functionName = CNAME( MonitorCreatureKilled );
		break;
	case ESME_Lost:
		functionName = CNAME( MonitorCreatureLost );
		break;
	case ESME_Spawned:
		functionName = CNAME( MonitorCreatureSpawned );
		break;
	default:
		ASSUME( false );
	}

	CallFunction( const_cast< ISpawnTreeSpawnMonitorInitializer* >( this ), functionName, actorHandle, entryHandle, encounterHandle );

	m_contextEntry = nullptr;
	m_contextInstance = nullptr;
}

void ISpawnTreeSpawnMonitorInitializer::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	m_contextInstance = &instance;

	THandle< CEncounter > encounterHandle( instance.GetEncounter() );

	CallFunction( const_cast< ISpawnTreeSpawnMonitorInitializer* >( this ), CNAME( OnFullRespawn ), encounterHandle  );

	m_contextInstance = nullptr;
}




Bool ISpawnTreeSpawnMonitorInitializer::IsSpawnable() const
{
	return GetClass() != GetStaticClass();
}
Bool ISpawnTreeSpawnMonitorInitializer::IsConflicting( const ISpawnTreeInitializer* initializer ) const
{
	return false;
}


String ISpawnTreeSpawnMonitorInitializer::GetEditorFriendlyName() const
{
	String outName;
	if ( CallFunctionRet< String >( const_cast< ISpawnTreeSpawnMonitorInitializer* >( this ), CNAME( GetFriendlyName ), outName ) )
	{
		return outName;
	}

	return GetClass()->GetName().AsString();
}
///////////////////////////////////////////////////////////////////////////////
// Script interface
void ISpawnTreeSpawnMonitorInitializer::funcGetNumCreaturesSpawned( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Int32 num = 0;
	if ( m_contextEntry && m_contextInstance )
	{
		num = m_contextEntry->GetNumCreaturesSpawned( *m_contextInstance );
	}
	RETURN_INT( num );
}
void ISpawnTreeSpawnMonitorInitializer::funcGetNumCreaturesToSpawn( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Int32 num = 0;
	if ( m_contextEntry && m_contextInstance )
	{
		num = m_contextEntry->GetNumCreaturesToSpawn( *m_contextInstance );
	}
	RETURN_INT( num );
}
void ISpawnTreeSpawnMonitorInitializer::funcGetNumCreaturesDead( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Int32 num = 0;
	if ( m_contextEntry && m_contextInstance )
	{
		num = m_contextEntry->GetNumCreaturesDead( *m_contextInstance );
	}
	RETURN_INT( num );
}
///////////////////////////////////////////////////////////////////////////////

