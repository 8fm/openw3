/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerSpawnLimitMonitor.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "spawnTreeBaseEntry.h"


IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerSpawnLimitMonitor )

///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerSpawnLimitMonitor
///////////////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerSpawnLimitMonitor::MonitorCreature( CSpawnTreeInstance& instance, CActor* actor, CBaseCreatureEntry* entry, ESpawnMonitorEvent eventType ) const
{
	// filter out events by creature definition
	if ( !m_creatureDefinition.Empty() )
	{
		CEncounter* encounter = instance.GetEncounter();
		CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();
		CEncounterCreaturePool::SCreature* creatureEntry = creaturePool.GetCreatureEntry( actor );
		ASSERT( creatureEntry );
		CEncounterCreatureDefinition* creatureDefinition = encounter->GetCreatureDefinition( creatureEntry->m_creatureDefId );
		if( creatureDefinition && creatureDefinition->GetDefinitionName() != m_creatureDefinition )
		{
			return;
		}
	}

	switch ( eventType  )
	{
	case ESME_Killed:
		++instance[ i_deadCount ];
		// NOTICE: no break
	case ESME_Lost:
		ASSERT( instance[ i_spawnedNow ] > 0 );
		instance[ i_spawnedNow ] = Max( 0, --instance[ i_spawnedNow ] );
		break;
	case ESME_Spawned:
		++instance[ i_spawnedNow ];
		break;
	}

	Bool& isTriggered = instance[ i_isTriggered ];
	Bool shouldBeTriggered = instance[ i_spawnedNow ]+instance[ i_deadCount ] >= instance[ i_spawnLimit ];
	if ( shouldBeTriggered != isTriggered )
	{
		isTriggered = shouldBeTriggered;

		ISpawnTreeBaseNode* parentNode = Cast< ISpawnTreeBaseNode >( GetParent() );
		if ( parentNode )
		{
			parentNode->UpdateEntriesSetup( instance );
		}
	}
}

void CSpawnTreeInitializerSpawnLimitMonitor::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT("totalSpawnLimitMin") )
	{
		m_totalSpawnLimitMax = Max( m_totalSpawnLimitMin, m_totalSpawnLimitMax );
	}
	else if ( property->GetName() == TXT("totalSpawnLimitMax") )
	{
		m_totalSpawnLimitMin = Min( m_totalSpawnLimitMin, m_totalSpawnLimitMax );
	}

	TBaseClass::OnPropertyPostChange( property );
}

void CSpawnTreeInitializerSpawnLimitMonitor::UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const
{
	if ( instance[ i_isTriggered ] )
	{
		if ( !m_creatureDefinition.Empty() )
		{
			if ( !entry->UsesCreatureDefinition( m_creatureDefinition ) )
			{
				return;
			}
		}
		setup.m_spawnRatio = 0.f;
	}
}
void CSpawnTreeInitializerSpawnLimitMonitor::OnFullRespawn( CSpawnTreeInstance& instance ) const
{
	if ( m_resetOnFullRespawn )
	{
		instance[ i_isTriggered ] = false;
		instance[ i_deadCount ] = 0;
		instance[ i_spawnLimit ] = RandomizeSpawnLimit();
	}
}

String CSpawnTreeInitializerSpawnLimitMonitor::GetEditorFriendlyName() const
{
	return TXT("SpawnLimit");
}
String CSpawnTreeInitializerSpawnLimitMonitor::GetBlockDebugCaption( const CSpawnTreeInstance& instance ) const
{
	String dead;
	if ( instance[ i_deadCount ] > 0 )
	{
		dead = String::Printf( TXT(" [dead %d]"), instance[ i_deadCount ] );
	}
	return String::Printf( TXT("SpawnLimit %d/%d%s%s"),
		instance[ i_deadCount ] + instance[ i_spawnedNow ],
		instance[ i_spawnLimit ],
		dead.AsChar(),
		instance[ i_isTriggered ] ? TXT(" [stop]") : TXT("")
		);
}

void CSpawnTreeInitializerSpawnLimitMonitor::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) 
{
	compiler << i_spawnLimit;
	compiler << i_spawnedNow;
	compiler << i_deadCount;
	compiler << i_isTriggered;

	TBaseClass::OnBuildDataLayout( compiler );
}

void CSpawnTreeInitializerSpawnLimitMonitor::OnInitData( CSpawnTreeInstance& instance )
{
	instance[ i_isTriggered ] = false;
	instance[ i_spawnedNow ] = 0;
	instance[ i_deadCount ] = 0;
	instance[ i_spawnLimit ] = RandomizeSpawnLimit();

	TBaseClass::OnInitData( instance );
}

Bool CSpawnTreeInitializerSpawnLimitMonitor::IsStateSaving( CSpawnTreeInstance& instance ) const
{
	return instance[ i_deadCount ] > 0;
}

void CSpawnTreeInitializerSpawnLimitMonitor::SaveState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	Int16 deadCount = instance[ i_deadCount ];
	
	writer->WriteValue< Int16 >( CNAME( dead ), deadCount );
	writer->WriteValue< Int16 >( CNAME( spawnLimit ), instance[ i_spawnLimit ] );
}

void CSpawnTreeInitializerSpawnLimitMonitor::LoadState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	Int16 deadCount = 0;

	reader->ReadValue< Int16 >( CNAME( dead ), deadCount );

	if ( reader->GetSaveVersion() >= SAVE_VERSION_SPAWN_LIMIT_SAVING )
	{
		reader->ReadValue< Int16 >( CNAME( spawnLimit ), instance[ i_spawnLimit ] );
	}	

	instance[ i_deadCount ] = deadCount;
	instance[ i_isTriggered ] = ( instance[ i_spawnedNow ]+instance[ i_deadCount ] >= instance[ i_spawnLimit ] );
}
