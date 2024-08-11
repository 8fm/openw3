/**
 * Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "encounter.h"

#include "../core/gameSave.h"
#include "../core/tagList.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"

#include "../engine/gameTimeManager.h"
#include "../engine/dynamicLayer.h"
#include "../engine/tickManager.h"
#include "../engine/tagManager.h"
#include "../engine/triggerAreaComponent.h"
#include "../engine/jobSpawnEntity.h"
#include "../engine/weatherManager.h"
#include "../engine/worldIterators.h"
#include "../engine/idTagManager.h"

#include "aiParamInjectHandler.h"
#include "aiSpawnTreeParameters.h"
#include "actorsManager.h"
#include "actionPointComponent.h"
#include "actionPointManager.h"
#include "commonGame.h"
#include "communitySystem.h"
#include "entityPool.h"
#include "gameFastForwardSystem.h"
#include "gameWorld.h"
#include "spawnPointComponent.h"
#include "spawnTreeBaseEntry.h"
#include "spawnTreeCustomSet.h"
#include "spawnTreeIncludeNode.h"
#include "spawnTreeInitializationContext.h"
#include "spawnTreeInitializer.h"
#include "spawnTreeStateSerializer.h"
#include "spawnTreeSpawnStrategy.h"
#include "strayActorManager.h"
#include "wayPointCookingContext.h"
#include "wayPointsCollection.h"

Bool SEncounterSettings::m_enableDelays = true;
const Float DEACTIVATION_DELAY = 8.f;

Bool CEncounter::SActiveEntry::Sorter::Less( SActiveEntry* a, SActiveEntry* b )
{
	RED_ASSERT( a != NULL );
	RED_ASSERT( b != NULL );
	RED_ASSERT( a->m_entry != NULL );
	RED_ASSERT( b->m_entry != NULL );

	if( a->m_entry->m_group == b->m_entry->m_group )
	{
		return a < b;
	}

	return a->m_entry->m_group < b->m_entry->m_group;
}

Float SEncounterSettings::GetSpawnPointInvalidDelay()
{
	return m_enableDelays ? GEngine->GetRandomNumberGenerator().Get< Float >( 2.0f, 3.0f ) : 0.0f;
}

Float SEncounterSettings::GetFailedWaypointTestLocationDelay()
{
	return m_enableDelays ? GEngine->GetRandomNumberGenerator().Get< Float >( 7.0f, 10.0f ) : 0.0f;
}

Float SEncounterSettings::GetSpawnLimitReachedDelay()
{
	return m_enableDelays ? GEngine->GetRandomNumberGenerator().Get< Float >( 2.0f, 3.0f ) : 0.0f;
}

Float SEncounterSettings::GetUpdateSpawnAnewDelay()
{
	return m_enableDelays ? GEngine->GetRandomNumberGenerator().Get< Float >( 0.8f, 1.2f ) : 0.0f;
}

IMPLEMENT_ENGINE_CLASS( SEncounterGroupLimit );
IMPLEMENT_ENGINE_CLASS( CEncounterGlobalSettings );
IMPLEMENT_ENGINE_CLASS( CEncounter );

CEncounterGlobalSettings* CEncounterGlobalSettings::m_instance = NULL;

RED_DEFINE_STATIC_NAME( EEncounterSpawnGroup );
RED_DEFINE_STATIC_NAME( currentEncounterGroup );

CEncounterGlobalSettings& CEncounterGlobalSettings::GetInstance()
{
	if( m_instance )
	{
		return *m_instance;
	}

	m_instance = CreateObject<CEncounterGlobalSettings>();
	m_instance->AddToRootSet();

	String spawnStrategy;
	if( SConfig::GetInstance().GetLegacy().ReadParam( TXT("gameplay"), TXT("Encounter"), TXT("SpawnStrategy"), spawnStrategy ) )
	{
		if( spawnStrategy == TXT("Simple") )
		{
			m_instance->m_defaultSpawnStrategy = CreateObject<CSimpleSpawnStrategy>( m_instance );
		}
		else if( spawnStrategy == TXT("MultiRange") )
		{
			m_instance->m_defaultSpawnStrategy = CreateObject<CMultiRangeSpawnStrategy>( m_instance );
		}
	}
	else
	{
		m_instance->m_defaultSpawnStrategy = CreateObject<CSimpleSpawnStrategy>( m_instance );
	}

	m_instance->m_defaultSpawnStrategy->LoadFromConfig();
	m_instance->LoadSpawnGroupConfig();

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( EnginePerformancePlatformChanged ), m_instance );
#endif

	return *m_instance;
}

RED_DEFINE_STATIC_NAME( defaultSpawnStrategy );
RED_DEFINE_STATIC_NAME( groupLimits );
void CEncounterGlobalSettings::OnPropertyPostChange( IProperty* property )
{
	if( property->GetName() == CNAME( defaultSpawnStrategy ) )
	{
		m_defaultSpawnStrategy->SaveToConfig();
	}
	else if( property->GetName() == CNAME( groupLimits ) )
	{
		SaveSpawnGroupConfig();
	}
}

void CEncounterGlobalSettings::LoadSpawnGroupConfig()
{
	m_groupLimits.ClearFast();

	CEnum* groups = SRTTI::GetInstance().FindEnum( CNAME( EEncounterSpawnGroup ) );
	if( groups )
	{
		// Add the priority group
		{
			SEncounterGroupLimit priorityGroup;
			priorityGroup.m_groupName = groups->GetOptions()[0];
			priorityGroup.m_group = 0;
			priorityGroup.m_limit = 10000;

			m_groupLimits.PushBack( priorityGroup );
		}

		// Add the rest
		for( Uint32 i = 1, size = groups->GetOptions().Size(); i < size; ++i )
		{
			const CName& groupName = groups->GetOptions()[i];

			SEncounterGroupLimit newGroup;
			newGroup.m_groupName = groupName;
			newGroup.m_group = i;
			newGroup.m_limit = 0;

			switch( i )
			{
			case 1:
				newGroup.m_limit = 100;
				break;
			case 2:
				newGroup.m_limit = 60;
				break;
			case 3:
				newGroup.m_limit = 30;
				break;
			case 4:
			default:
				newGroup.m_limit = 10;
				break;
			}
			SConfig::GetInstance().GetLegacy().ReadParam( TXT("profile"), TXT("Community"), groupName.AsString(), newGroup.m_limit );

			m_groupLimits.PushBack( newGroup );
		}
	}
}

void CEncounterGlobalSettings::SaveSpawnGroupConfig()
{
	for( auto it = m_groupLimits.Begin(), end = m_groupLimits.End(); it != end; ++it )
	{
		const SEncounterGroupLimit& group = *it;

		SConfig::GetInstance().GetLegacy().WriteParam( TXT("profile"), TXT("Community"), group.m_groupName.AsChar(), group.m_limit );
	}
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CEncounterGlobalSettings::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Show/hide layer when in editor and PP mode is changed
	if ( name == CNAME( EnginePerformancePlatformChanged ) )
	{
		LoadSpawnGroupConfig();
	}
}
#endif // NO_EDITOR_EVENT_SYSTEM


//////////////////////////////////////////////////////////////
/// CEncounter::FastForwardUpdateContext
//////////////////////////////////////////////////////////////
CEncounter::FastForwardUpdateContext::FastForwardUpdateContext( SFastForwardExecutionContext* context )
	: m_runningSpawnJobsCount( 0 )
	, m_isLoadingTemplates( false )
	, m_globalContext( context )
{

}

//////////////////////////////////////////////////////////////
/// CEncounter::SActiveEntry
//////////////////////////////////////////////////////////////
CEncounter::SActiveEntry::SActiveEntry( CBaseCreatureEntry* entry, CSpawnTreeInstance* instance )
	: m_entry( entry )
	, m_instanceBuffer( instance )
	, m_spawnedActors( entry->GetUniqueListId( *instance ) )
	, m_delayedUpdate( EngineTime::ZERO )
	, m_branchHash( 0 )
	, m_isActive( false )
{}

Bool CEncounter::SActiveEntry::Comperator::Less( const SActiveEntry& e1, const SActiveEntry& e2 )
{
	return e1.m_entry->GetUniqueListId( *e1.m_instanceBuffer ) < e2.m_entry->GetUniqueListId( *e2.m_instanceBuffer );
}
Bool CEncounter::SActiveEntry::Comperator::Less( Uint16 listId, const SActiveEntry& e )
{
	return listId < e.m_entry->GetUniqueListId( *e.m_instanceBuffer );
}
Bool CEncounter::SActiveEntry::Comperator::Less( const SActiveEntry& e, Uint16 listId )
{
	return e.m_entry->GetUniqueListId( *e.m_instanceBuffer ) < listId;
}


//////////////////////////////////////////////////////////////
// CEncounter
//////////////////////////////////////////////////////////////

CEncounter::CEncounter()
	: m_enabled( true )
	, m_active( false )
	, m_fullRespawnScheduled( false )
	, m_wasRaining( false )
	, m_isFullRespawnTimeInGameTime( true )
	, m_ticked( false )
	, m_tickRequested( false )
	, m_tickSuppressed( false )
	, m_isEncounterStateLoaded( false )
	, m_activationReason( 0 )
	, m_runningSpawnJobsCount( 0 )
	, m_fullRespawnDelay( 0 )
	, m_conditionRetestTimeout( 2.0f )
	, m_conditionRetestTimer( 0.0f )
	, m_deactivationTimeout( DEACTIVATION_DELAY )
	, m_ignoreAreaTrigger( false )
	, m_lastUpdatedEntry( nullptr )
{
	SetForceNoLOD( true );
}

CEncounter::~CEncounter()
{
	UnbindBuffer();

	// Pending spawn task that were never processes. Get rid of them. 
	RED_FATAL_ASSERT( m_spawnJobs.Empty(), "Pending Spawning Job still not release! FIX ME THIS WILL LEAK LIKE HELL" );
}

void CEncounter::EnableTick()
{
	if ( !m_ticked )
	{
		m_ticked = true;
		CWorld* world = GetLayer()->GetWorld();
		world->GetTickManager()->AddEntity( this );
	}
}
void CEncounter::DisableTick()
{
	if ( m_ticked )
	{
		m_ticked = false;
		CWorld* world = GetLayer()->GetWorld();
		world->GetTickManager()->RemoveEntity( this );
	}
}

void CEncounter::RequestTicks()
{
	if ( !m_tickRequested )
	{
		m_tickRequested = true;
		if ( !m_tickSuppressed )
		{
			EnableTick();
		}
	}
}
void CEncounter::DontRequestTicks()
{
	if ( m_tickRequested )
	{
		m_tickRequested = false;
		DisableTick();
	}
}
void CEncounter::SuppressTicks()
{
	if ( !m_tickSuppressed )
	{
		m_tickSuppressed = true;
		DisableTick();
	}
}
void CEncounter::UnsuppressTicks()
{
	if ( m_tickSuppressed )
	{
		m_tickSuppressed = false;
		if ( m_tickRequested )
		{
			EnableTick();
		}
	}
}

void CEncounter::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	LazyBindBuffer();

	{
		CGameSaverBlock block( saver, CNAME( encounter ) );

		// Write number of spawned creatures
		Int32 numSpawnedCreatures = 0;
		for ( auto it = m_creaturePool.GetCreatures().Begin(); it != m_creaturePool.GetCreatures().End(); ++it )
		{
			if ( (it->m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_IS_IN_PARTY) == 0 && it->m_actor->IsAlive() )
			{
				SActiveEntry* entry = FindActiveCreatureEntry( it->m_lastOwningEntry );
				if ( entry )
				{
					++numSpawnedCreatures;
				}
			}
		}
		saver->WriteValue( CNAME( numSaved ), numSpawnedCreatures );

		// Write info on spawned creatures
		for ( auto it = m_creaturePool.GetCreatures().Begin(); it != m_creaturePool.GetCreatures().End(); ++it )
		{
			if ( (it->m_stateFlags & CEncounterCreaturePool::SCreature::FLAG_IS_IN_PARTY) == 0 && it->m_actor->IsAlive() )
			{
				SActiveEntry* entry = FindActiveCreatureEntry( it->m_lastOwningEntry );
				if ( entry )
				{
					CEncounterCreatureDefinition* def = GetCreatureDefinition( it->m_creatureDefId );
					CGameSaverBlock block( saver, CNAME( spawnedCreature ) );
					saver->WriteValue< Uint64 >( CNAME( entryId ), entry->m_entry->GetId() );
					saver->WriteValue< Uint64 >( CNAME( entryHash ), entry->m_branchHash );
					saver->WriteValue< Vector3 >( CNAME( position ), it->m_actor->GetWorldPosition() );
					saver->WriteValue< Float >( CNAME( yaw ), it->m_actor->GetWorldYaw() );
					saver->WriteValue< CName >( CNAME( appearance ), it->m_actor->GetAppearance() );
					saver->WriteValue< CName >( CNAME( creatureDefinition ), def->GetDefinitionName() );
					saver->WriteValue< Int32 >( CNAME( currentEncounterGroup ), it->m_actor->GetEncounterGroupUsedToSpawn() );					
				}
			}
		}

		Uint16 creatureDefinitionsToSave = 0;
		for ( auto it = m_compiledCreatureList.Begin(), end = m_compiledCreatureList.End(); it != end; ++it )
		{
			if ( (*it)->ShouldSave() )
			{
				++creatureDefinitionsToSave;
			}
		}

		saver->WriteValue< Uint16 >( CNAME( numCreatureDefinitions ), creatureDefinitionsToSave );

		// save creature definitions state
		if ( creatureDefinitionsToSave > 0 )
		{
			for ( auto it = m_compiledCreatureList.Begin(), end = m_compiledCreatureList.End(); it != end; ++it )
			{
				if ( (*it)->ShouldSave() )
				{
					saver->WriteValue< CName >( CNAME( creatureDefinition ), (*it)->GetDefinitionName() );
					(*it)->Save( saver );
				}
			}
		}

		// serialize spawn tree state
		{
			CGameSaverBlock block( saver, CNAME( treeState ) );
			CSpawnTreeStateSerializer serializer( this );
			serializer.Save( saver );
		}
	}
}

void CEncounter::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	{
		LazyBindBuffer();

		CGameSaverBlock block( loader, CNAME(encounter) );

		Int32 numSpawnedCreatures = 0;
		loader->ReadValue( CNAME( numSaved ), numSpawnedCreatures );

		if ( numSpawnedCreatures > 0 )
		{
			m_isEncounterStateLoaded = true;

			// Respawn creatures
			for ( Int32 i = 0; i < numSpawnedCreatures; ++i )
			{
				CGameSaverBlock block( loader, CNAME( spawnedCreature ) );

				Uint64 entryId = 0;
				Uint64 entryHash = 0;
				Vector3 position( 0, 0, 0 );
				Float yaw = 0.0f;
				CName appearance;
				CName creatureDefinitionName;
				Int32 encounterSpawnGroup = -1;

				loader->ReadValue< Uint64 >( CNAME( entryId ), entryId );
				loader->ReadValue< Uint64 >( CNAME( entryHash ), entryHash );
				loader->ReadValue< Vector3 >( CNAME( position ), position );
				loader->ReadValue< Float >( CNAME( yaw ), yaw );
				loader->ReadValue< CName >( CNAME( appearance ), appearance );
				loader->ReadValue< CName >( CNAME( creatureDefinition ), creatureDefinitionName );
				loader->ReadValue< Int32 >( CNAME( currentEncounterGroup ), encounterSpawnGroup );				

				SActiveEntry* activeEntry = FindCreatureEntry( entryId, entryHash );
				if ( !activeEntry )
				{
					WARN_GAME( TXT("Failed to get creature entry by guid") );
					continue;
				}

				const Bool isEntryActive = IsCreatureEntryActive( activeEntry );

				// Make sure entity template is loaded
				Int16 creatureDefinitionId = GetCreatureDefinitionId( creatureDefinitionName );
				if ( creatureDefinitionId < 0 )
				{
					continue;
				}
				CEncounterCreatureDefinition* creatureDefinition = GetCreatureDefinition( creatureDefinitionId );
				if ( !creatureDefinition )
				{
					continue;
				}

				TSoftHandle< CEntityTemplate > entityTemplateFileHandle = creatureDefinition->GetEntityTemplate();
				THandle< CEntityTemplate > entityTemplateHandle = entityTemplateFileHandle.Get();
				if ( !entityTemplateHandle.IsValid() )
				{
					WARN_GAME( TXT("Failed to load entity template '%ls'."), creatureDefinition->GetEntityTemplatePath().AsChar() );
					continue;
				}
				CEntityTemplate* entityTemplate = entityTemplateHandle.Get();

				if ( !activeEntry->m_entry->CheckEntityTemplate( entityTemplate ) )
				{
					continue;
				}

				// Mark appearance as used
				creatureDefinition->MarkAppearanceAsUsed( entityTemplate, appearance );

				// Set up spawn info
				EntitySpawnInfo data;
				data.m_template = entityTemplateHandle;
				data.m_spawnPosition = position;
				data.m_spawnRotation = EulerAngles(0.0f, 0.0f, yaw);
				data.m_appearances.PushBack( appearance );
				data.m_tags = creatureDefinition->GetTags();
				data.m_entityFlags = EF_DestroyableFromScript;

				if ( data.m_template->GetEntityObject()->GetClass()->GetName() == CNAME( W3MerchantNPC ) ) 
				{
					data.m_idTag = GCommonGame->GetIdTagManager()->CreateFromUint64( entryHash );
				}
				data.m_encounterEntryGroup = ( encounterSpawnGroup == -1 ? activeEntry->m_entry->m_group : encounterSpawnGroup );

				data.AddHandler( new CAiSpawnSystemParamInjectHandler( GetEncounterParameters() ) );

				//const auto& activeEntryInitializers = activeEntry->m_entry->GetRuntimeInitializers( *activeEntry->m_instanceBuffer );
				//for( auto it = activeEntryInitializers.Begin(), end = activeEntryInitializers.End(); it != end; ++it )
				//{
				//	if ( (*it).m_initializer->CallActivateOnRestore() )
				//	{
				//		(*it).m_initializer->OnCreatureSpawn( data );
				//	}
				//}

				// Spawn entity
				if ( CActor* spawnedActor = Cast< CActor >( GCommonGame->GetEntityPool()->SpawnEntity_Sync( data, CEntityPool::SpawnType_Encounter ) ) )
				{
					spawnedActor->RegisterTerminationListener( true, this );

					m_compiledCreatureList[ creatureDefinitionId ]->OnCreatureSpawned();
					CEncounterCreaturePool::SCreature* creatureEntry = m_creaturePool.AddCreature( spawnedActor, creatureDefinitionId, isEntryActive ? activeEntry->m_spawnedActors : m_creaturePool.GetDetachedCreaturesList(), activeEntry->m_entry->m_group );
					creatureEntry->m_lastOwningEntry = activeEntry->m_entry->GetUniqueListId( *activeEntry->m_instanceBuffer );
					if ( isEntryActive )
					{
						activeEntry->m_entry->ActivateInitializers( *activeEntry->m_instanceBuffer, spawnedActor, EST_GameIsRestored, 0 );
						creatureEntry->m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY;
					}					
				}
			}
		}

		// read creature defintions state
		Uint16 creatureDefinitionsToLoad = 0;
		
		loader->ReadValue< Uint16 >( CNAME( numCreatureDefinitions ), creatureDefinitionsToLoad );

		// save creature definitions state
		for ( Uint16 i = 0; i < creatureDefinitionsToLoad; ++i )
		{
			CName definitionName;
			loader->ReadValue< CName >( CNAME( creatureDefinition ), definitionName );
			Int16 defId = GetCreatureDefinitionId( definitionName );
			if ( defId < 0 )
			{
				// we failed to load all this stuff
				break;
			}
			CEncounterCreatureDefinition* creatureDef = GetCreatureDefinition( defId );
			creatureDef->Load( loader );
		}

		// serialize spawn tree state
		{
			CGameSaverBlock block( loader, CNAME( treeState ) );
			CSpawnTreeStateSerializer serializer( this );
			serializer.Load( loader );

			// if tree state was actually changed because of save - give entries a feedback to update its internals
			if ( serializer.GetSerializedNodesCount() > 0 )
			{
				for ( const SActiveEntry& entry : m_entries )
				{
					entry.m_entry->PostLoadGameplayState( *entry.m_instanceBuffer );
				}
			}
		}

		if ( !m_creaturePool.GetDetachedCreaturesList().Empty() )
		{
			RequestTicks();
		}
	}
}

void CEncounter::OnSerialize( IFile& file )
{
	m_spawnPoints.OnSerialize( file );

	TBaseClass::OnSerialize( file );
}

void CEncounter::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	// Serialization issue hack
	// Some m_encounterParameters has still CGuardAreaParameters saved here
	CEncounterParameters* parameters = m_encounterParameters.Get();
	if ( parameters && !parameters->IsA< CEncounterParameters >() )
	{
		m_encounterParameters = nullptr;
	}
}

CEncounter::SActiveEntry* CEncounter::FindCreatureEntry( Uint64 entryId, Uint64 entryHash )
{
	for ( auto it = m_entries.Begin(); it != m_entries.End(); ++it )
	{
		if ( it->m_branchHash == entryHash && it->m_entry->GetId() == entryId )
		{
			return &(*it);
		}
	}
	return nullptr;
}

Bool CEncounter::IsCreatureEntryActive( SActiveEntry* entry )
{
	return entry->m_isActive;
}

CEncounter::SActiveEntry* CEncounter::FindActiveCreatureEntry( CEncounterCreaturePool::SCreatureList::ListId listId )
{
	auto itFind = m_entries.Find( listId );
	if ( itFind == m_entries.End() )
	{
		return NULL;
	}
	return &(*itFind);
}
CEncounter::SActiveEntry* CEncounter::FindActiveCreatureEntry( CActor* actor )
{
	SCreatureData* creatureData = m_creaturePool.GetCreatureEntry( actor );
	if ( !creatureData )
	{
		return NULL;
	}

	return FindActiveCreatureEntry( creatureData->m_listId );
}

void CEncounter::SpawnAsync( IJobEntitySpawn* spawnJob, CBaseCreatureEntry* entry, CSpawnTreeInstance& instance )
{
	SpawnJob spJob;
	spJob.m_spawnJob = spawnJob;
	spJob.m_spawningEntry = entry->GetUniqueListId( instance );
	
	m_spawnJobs.PushBack( Move( spJob ) );
	++m_runningSpawnJobsCount;
}

CEncounter::SCreatureData* CEncounter::RegisterSpawnedCreature( CActor* actor, CEncounter::SActiveEntry& entryData, Int16 creatureDefinitionId )
{
	actor->RegisterTerminationListener( true, this );

	CEncounterCreaturePool::SCreature* newCreature = m_creaturePool.AddCreature( actor, creatureDefinitionId, entryData.m_spawnedActors, entryData.m_entry->m_group );
	newCreature->m_lastOwningEntry = entryData.m_spawnedActors.m_listId;
	newCreature->m_stateFlags |= CEncounterCreaturePool::SCreature::FLAG_WAS_ACTIVATED_BY_ENTRY;

	m_compiledCreatureList[ creatureDefinitionId ]->OnCreatureSpawned();	
	
	return newCreature;
}

void CEncounter::BroadcastEvent( CName eventName )
{
	if( !m_active )
		return;

	for ( auto it = m_entries.Begin(); it != m_entries.End(); ++it )
	{
		CSpawnTreeInstance* instance = it->m_instanceBuffer;
		it->m_entry->OnEvent( *instance, eventName );
	}	
}

void CEncounter::HandleRaining()
{
	CWeatherManager* weatherManager = GetWeatherManager();
	if( !weatherManager )
	{
		return;
	}

	Float rainStrength = weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN );
	if( rainStrength > 0.01f )
	{
		if( !m_wasRaining )
		{
			m_wasRaining = true;
			BroadcastEvent( CNAME( RainStarted ) );
		}
	}
	else
	{
		if( m_wasRaining )
		{
			m_wasRaining = false;
			BroadcastEvent( CNAME( RainEnded ) );
		}
	}
}

CWeatherManager* CEncounter::GetWeatherManager()
{
	if( CWorld* world = GGame->GetActiveWorld() )
	{
		if( CEnvironmentManager* envManager = world->GetEnvironmentManager() )
		{
			return envManager->GetWeatherManager();
		}
	}
	return nullptr;
}

void CEncounter::DetachCreatureInstance( CActor* actorToDetach )
{
	actorToDetach->RegisterTerminationListener( false, this );
	SCreatureData* creatureData = m_creaturePool.GetCreatureEntry( actorToDetach );
	if ( creatureData )
	{
		RemoveScheduledDespawn( *creatureData );
		m_creaturePool.RemoveCreature( *creatureData );
	}
}

void CEncounter::ForceDespawnDetached()
{
	auto& detachedCreatures = m_creaturePool.GetDetachedCreaturesList();

	while ( !detachedCreatures.Empty() )
	{
		SCreatureData& creatureData = *detachedCreatures.Begin();
		DespawnCreature( creatureData );
	}
}

void CEncounter::ForceDespawnAll()
{
	CEncounterCreaturePool::CreatureArray& creatureArray = m_creaturePool.GetCreatures();
	while ( !creatureArray.Empty() )
	{
		SCreatureData& creatureData = creatureArray.Back();
		DespawnCreature( creatureData );
	}
}

void CEncounter::DespawnCreature( SCreatureData& kriczer )
{
	CActor* actor = kriczer.m_actor;
	RemoveCreature( actor, SPAWN_TREE_REMOVAL_POOL );
	GCommonGame->GetEntityPool()->AddEntity( actor );

	//if ( kriczer.m_stateFlags & SCreature::FLAG_IS_IN_PARTY )
	//{
	//	m_creaturePool.GetPartiesManager().RemoveFromParty( m_creaturePool, kriczer );
	//}
	//// If alive, increment total spawn counter
	//if ( kriczer.m_creatureDefId >= 0 )
	//{
	//	CEncounterCreatureDefinition* creatureDef = m_compiledCreatureList[ kriczer.m_creatureDefId ];

	//	// despawn fucker
	//	if( !actor->IsAlive() )
	//	{
	//		creatureDef->OnCreatureDead();
	//	}

	//	creatureDef->OnCreatureDespawned();
	//}

	//DetachCreatureInstance( actor );
	
}

Bool CEncounter::ScheduleCreatureToDespawn( SCreatureData& kriczer )
{
	if ( ( kriczer.m_stateFlags & SCreatureData::FLAG_DESPAWN_SCHEDULED) == 0 )
	{
		kriczer.m_stateFlags |= SCreatureData::FLAG_DESPAWN_SCHEDULED;

		SActiveEntry* lastEntry = FindActiveCreatureEntry( kriczer.m_lastOwningEntry );

		kriczer.m_despawnersId = m_despawnHandler.GetNextDespawnId();
		
		if( lastEntry && lastEntry->m_entry->CreateDespawners( *lastEntry->m_instanceBuffer, kriczer.m_actor, kriczer.m_despawnersId ) )
		{
			return false;
		}
		m_despawnHandler.RegisterDespawner( CSpawnTreeDespawnAction( m_defaultImmediateDespawnConfiguration, kriczer.m_actor, kriczer.m_despawnersId ) );
	}
	return false;
}

void CEncounter::RemoveScheduledDespawn( SCreatureData& kriczer )
{
	if ( kriczer.m_stateFlags & SCreatureData::FLAG_DESPAWN_SCHEDULED )
	{
		kriczer.m_stateFlags &= ~SCreatureData::FLAG_DESPAWN_SCHEDULED;

		if( m_despawnHandler.RemoveDespawner( kriczer.m_despawnersId ) )
		{
			kriczer.m_despawnersId = SPAWN_TREE_INVALID_DESPAWNER_ID;
		}
	}
}

void CEncounter::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( IsInGame() )
	{
		LazyBindBuffer();

		// Find first trigger
		m_triggerArea = FindComponent< CTriggerAreaComponent >();
	}

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Encounter );
}

void CEncounter::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	if( m_spawnTree && m_instanceBuffer.IsBinded() )
	{
		if ( m_spawnTree->IsActive( m_instanceBuffer ) )
		{
			m_spawnTree->Deactivate( m_instanceBuffer );
		}
	}

	if ( m_ticked )
	{
		world->GetTickManager()->RemoveEntity( this );
		m_ticked = false;
	}

	m_tickRequested = false;

	m_active = false;

	for ( Uint32 i = 0, n = m_compiledCreatureList.Size(); i != n; ++i )
	{
		if ( m_compiledCreatureList[ i ] )
		{
			m_compiledCreatureList[ i ]->FullRespawn();
		}
	}

	UnbindBuffer();

	auto& creatureArray = m_creaturePool.GetCreatures();
	for ( auto it = creatureArray.Begin(), end = creatureArray.End(); it != end; ++it )
	{
		CActor* kriczer = it->m_actor;
		if ( kriczer )
		{
			kriczer->RegisterTerminationListener( false, this );
		}
	}

	m_creaturePool.Clear();

	m_compiledCreatureList.ClearFast();
	
	m_fullRespawnScheduled = false;
	
	for( auto iter : m_spawnJobs )
	{
		IJobEntitySpawn * job = iter.m_spawnJob;
		if( job )
		{
			job->Cancel();
			job->Release();
		}
	}

	m_spawnJobs.ClearFast();
	m_runningSpawnJobsCount = 0;
	
	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Encounter );
}

Uint16 CEncounter::RegisterEntry( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer )
{
	// TODO! Insert destroys list enclosed in entries array. But this function is not meant to be used away from initialization...
	auto it = m_entries.Insert( Move( SActiveEntry( entry, instanceBuffer ) ) );
	return it->m_spawnedActors.m_listId;
}

void CEncounter::SaveEntryHash( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer, Uint64 entryParentalHash )
{
	CEncounter::SActiveEntry* listItem = FindActiveCreatureEntry( entry->GetUniqueListId( *instanceBuffer ) );
	ASSERT( listItem );
	listItem->m_branchHash = entryParentalHash;
}

void CEncounter::AddActiveEntry( SActiveEntry* entry )
{	
	entry->m_isActive = true;		
	m_activeEntriesList.InsertUnique( entry );
}

void CEncounter::RegisterForUpdates( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer )
{
	CEncounter::SActiveEntry* listItem = FindActiveCreatureEntry( entry->GetUniqueListId( *instanceBuffer ) );
	ASSERT( listItem );	
	AddActiveEntry( listItem );
	entry->SetIsUpdated( *instanceBuffer, true );
}

void CEncounter::UnregisterForUpdates( CBaseCreatureEntry* entry, CSpawnTreeInstance* instanceBuffer )
{
	CEncounter::SActiveEntry* listItem = FindActiveCreatureEntry( entry->GetUniqueListId( *instanceBuffer ) );
	ASSERT( listItem );	
	auto it = m_activeEntriesList.Find( listItem );
	if( it != m_activeEntriesList.End() )
	{
		m_activeEntriesList.Erase( it );
	}	
	listItem->m_isActive = false;
	entry->SetIsUpdated( *instanceBuffer, false );
}

const TDynArray< Vector >* CEncounter::GetWorldPoints() const
{
	CTriggerAreaComponent* triggerArea = m_triggerArea.Get();
	if ( triggerArea )
	{
		return (TDynArray< Vector >*) &triggerArea->GetWorldPoints();
	}
	else
	{
		return NULL;
	}
}

void CEncounter::LazyBindBuffer()
{
	if ( m_spawnTree )
	{
		if ( !m_instanceBuffer.IsBinded() )
		{
			// initialize creature definitions
			m_compiledCreatureList.ClearFast();

			CompileCreatureDefinitions( m_compiledCreatureList );

			CSpawnTreeInitializationContext context;

			ISpawnTreeInitializer* spawnStrategyInitializer = CEncounterGlobalSettings::GetInstance().GetSpawnStrategy();
			context.PushInitialInitializer( spawnStrategyInitializer, &m_instanceBuffer );

			// compile instance buffer
			{
				InstanceDataLayoutCompiler compiler( m_dataLayout );

				spawnStrategyInitializer->OnBuildDataLayout( compiler );

				m_spawnTree->OnBuildDataLayout( compiler );
				m_dataLayout.ChangeLayout( compiler );
			}
			m_instanceBuffer.Bind( m_dataLayout, this );

			m_spawnTree->OnInitData( m_instanceBuffer, context );
			m_spawnTree->GenerateHashRecursively( m_id, &m_instanceBuffer );
		}
	}
}

void CEncounter::UnbindBuffer()
{
	if ( m_instanceBuffer.IsBinded() )
	{
		ASSERT( m_activeEntriesList.Empty() );
		m_entries.ClearFast();
		m_spawnTree->OnDeinitData( m_instanceBuffer );
		m_instanceBuffer.Unbind();
	}
}

Bool CEncounter::TryDeactivate()
{
	if ( !m_active && m_activeEntriesList.Empty() && m_creaturePool.GetDetachedCreaturesList().Empty() && m_spawnJobs.Empty() )
	{
		DontRequestTicks();

		return true;
	}
	return false;
}

Bool CEncounter::IsPointSeenByPlayer( const Vector& testPoint )
{
	return GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( testPoint );
}

Bool CEncounter::GenerateIdsRecursively()
{
	Bool wasRegenerated = false;

	if ( m_spawnTree )
	{
		wasRegenerated |= m_spawnTree->GenerateIdsRecursively();
	}

	return wasRegenerated;
}

void CEncounter::ScheduleFullRespawn()
{
	if ( !m_fullRespawnScheduled && m_fullRespawnDelay.m_seconds > 0 )
	{
		m_fullRespawnScheduled = true;
		GameTime delay = m_fullRespawnDelay * ( 0.75f + GEngine->GetRandomNumberGenerator().Get< Float >( 0.5f ) );
		if ( !m_isFullRespawnTimeInGameTime )
		{
			// Rescale delay from game time into real time
			delay = delay * ( 60.0f * GGame->GetTimeManager()->GetHoursPerMinute() );
		}
		m_fullRespawnTime = GGame->GetTimeManager()->GetTime() + delay;
	}
}

Bool CEncounter::CheckFullRespawn()
{
	if ( m_fullRespawnScheduled )
	{
		if ( GGame->GetTimeManager()->GetTime() >= m_fullRespawnTime )
		{
			DoFullRespawn();
			return true;
		}
	}
	return false;
}
void CEncounter::DoFullRespawn()
{
	m_fullRespawnScheduled = false;
	if ( m_spawnTree )
	{
		m_spawnTree->OnFullRespawn( m_instanceBuffer );
	}
	CallFunction( this, CNAME( OnFullRespawn ) );
	for ( Uint32 i = 0, n = m_compiledCreatureList.Size(); i != n; ++i )
	{
		if ( m_compiledCreatureList[ i ] )
		{
			m_compiledCreatureList[ i ]->FullRespawn();
		}
	}
}

Bool CEncounter::IsWithinArea( const Vector &testPoint )
{
	CTriggerAreaComponent* triggerArea = m_triggerArea.Get();
	// first test - fast
	if ( triggerArea && triggerArea->GetBoundingBox().Contains( testPoint ) )
	{
		// second test - accurate
		if ( triggerArea->TestPointOverlap( testPoint ) )
		{
			return true;
		}
	}
	return false;
}

// Its very dirty and its NOT SUPPORTING Parties (as I dumped party support for encounter saving)
void CEncounter::PostStateLoadInitialization()
{
	m_spawnTree->UpdateLogic( m_instanceBuffer );

	auto& detachedCreatures = m_creaturePool.GetDetachedCreaturesList();

	for ( auto it = detachedCreatures.Begin(), end = detachedCreatures.End(); it != end;  )
	{
		SCreatureData& creatureData = *it;
		++it;

		SActiveEntry* entry = FindActiveCreatureEntry( creatureData.m_lastOwningEntry );
		if ( !entry )
		{
			continue;
		}

		entry->m_entry->OnCreatureLoaded( *entry->m_instanceBuffer, creatureData, *entry );
	}
}

void CEncounter::ActivateEncounter( EActivationReason reason )
{
	m_activationReason |= reason;
	m_deactivationTimeout = DEACTIVATION_DELAY;
	
	if ( !m_active && m_enabled )
	{
		CGameWorld* gameWorld = Cast< CGameWorld >( GetLayer()->GetWorld() );
		if ( m_spawnTree && gameWorld && m_spawnPoints.Initialize( gameWorld, this )  )
		{
			m_active = true;

			// check for full respawn only when re-entering encounter
			CheckFullRespawn();

			RequestTicks();

			if ( m_isEncounterStateLoaded )
			{
				PostStateLoadInitialization();
				m_isEncounterStateLoaded = false;
			}
		}
	}
}

void CEncounter::DeactivateInstantly()
{
	if ( m_activationReason == 0 && m_active )
	{
		m_active = false;
		m_deactivationTimeout = DEACTIVATION_DELAY;
		m_spawnTree->Deactivate( m_instanceBuffer );

		TryDeactivate();
	}
}

void CEncounter::DeactivateEncounter( EActivationReason reason, Bool performImmediately )
{
	m_activationReason &= ~reason;

	if ( performImmediately )
	{
		DeactivateInstantly();
	}
}

void CEncounter::EnableEncounter( Bool enable )
{
	if ( m_enabled == enable )
	{
		return;
	}

	m_enabled = enable;

	if( m_enabled )
	{
		CEntity* playerEntity = GGame->GetPlayerEntity();
		if( playerEntity && IsWithinArea( playerEntity->GetWorldPositionRef() ) )
		{
			EnterArea();
		}
	}
	else
	{
		if ( m_active )
		{
			DeactivateEncounter( REASON_TRIGGER, true );
		}
	}
}

void CEncounter::ProcessPendingSpawnJobs()
{
	m_runningSpawnJobsCount = 0;
	for ( Int32 i = m_spawnJobs.Size() - 1; i >= 0; --i )
	{
		IJobEntitySpawn* spawnJob = m_spawnJobs[ i ].m_spawnJob;
		if ( !spawnJob->HasEnded() )
		{
			++m_runningSpawnJobsCount;
			continue;
		}
		
		if ( !spawnJob->AreEntitiesReady() )
		{
			continue;
		}

		PC_SCOPE_PIX( SpawnJob_Finished );

		spawnJob->LinkEntities();
		SActiveEntry* entryData = FindActiveCreatureEntry( m_spawnJobs[ i ].m_spawningEntry );
		ASSERT( entryData, TXT("Entry must exist") );
		CBaseCreatureEntry* treeEntry = entryData->m_entry;
		CSpawnTreeInstance& instanceBuffer = *entryData->m_instanceBuffer;

		treeEntry->OnSpawnJobIsDone( instanceBuffer, spawnJob, *entryData );

		spawnJob->Release();
		m_spawnJobs.RemoveAtFast( i );
	}
}

void CEncounter::OnTick( Float timeDelta )
{
	PC_SCOPE_PIX( EncounterTick );	

	// handle raining
	HandleRaining();

	// process pending spawn jobs
	if ( !m_spawnJobs.Empty() )
	{
		ProcessPendingSpawnJobs();
	}

	// update spawn tree logic
	if( m_active )
	{
		PC_SCOPE_PIX( UpdateLogic );

		m_conditionRetestTimer -= timeDelta;
		if ( m_conditionRetestTimer <= 0.0f )
		{
			m_spawnTree->UpdateLogic( m_instanceBuffer );
			m_conditionRetestTimer = m_conditionRetestTimeout;
		}

		if ( m_activationReason == 0 )
		{
			m_deactivationTimeout -= timeDelta;
			if ( m_deactivationTimeout <= 0.f )
			{
				DeactivateInstantly();
			}
		}
	}

	// compute entries update context
	Matrix referencePos;
	CEntity* player = GGame->GetPlayerEntity();
	if ( player )
	{
		player->GetLocalToWorld( referencePos );
	}
	else
	{
		referencePos = Matrix::IDENTITY;
		referencePos.SetTranslation( GetLayer()->GetWorld()->GetCameraPosition() );
	}

	SSpawnTreeUpdateSpawnContext context(
		GGame->GetEngineTime(),
		referencePos,
		(m_runningSpawnJobsCount == 0) ? 1 : 0
		);

	
	// update active etnries
	{
		PC_SCOPE_PIX( UpdateActiveEntries );

		CTemplateLoadBalancer* loadBalancer = GCommonGame->GetTemplateLoadBalancer();
		loadBalancer->LockJobSpawning();

		for ( Uint32 it = 0; it < m_activeEntriesList.Size(); ++it )
		{
			const Bool canSpawn = m_spawnJobs.Empty();

			SActiveEntry* entryData = m_activeEntriesList[ it ];

			if ( entryData->m_delayedUpdate > context.m_currentTime )
			{
				continue;
			}

			CSpawnTreeInstance& instance = *entryData->m_instanceBuffer;

			Bool isValid;
			{
				PC_SCOPE_PIX( UpdateSpawn );
				context.m_runtimeData = entryData;

				isValid = entryData->m_entry->UpdateSpawn( instance, context );
			}

			if ( isValid )
			{
				PC_SCOPE_PIX( TickInitializers );
				entryData->m_entry->TickInitializers( instance, entryData->m_spawnedActors, context );
			}
			else
			{
				entryData->m_isActive = false;
				m_activeEntriesList.RemoveAt( it );
				--it;
			}
		}

		loadBalancer->UnlockJobSpawning();
	}

	// update creatures despawn
	{
		PC_SCOPE_PIX( UpdateDespawn );

		auto& detachedCreatures = m_creaturePool.GetDetachedCreaturesList();

		for ( auto it = detachedCreatures.Begin(), end = detachedCreatures.End(); it != end; ++it )
		{
			SCreatureData& creatureData = *it;
			if ( ScheduleCreatureToDespawn( creatureData ) )
			{
				break;
			}
		}

		m_despawnHandler.Update();
	}

	// check deactivation conditions
	TryDeactivate();
}

void CEncounter::FastForwardTick(  Float timeDelta, FastForwardUpdateContext& fastForwardContext )
{
	PC_SCOPE_PIX( EncounterFastForwardTick );

	if ( !m_spawnJobs.Empty() )
	{
		ProcessPendingSpawnJobs();
	}
	
	HandleRaining();

	if ( timeDelta != 0.f && m_active )
	{
		PC_SCOPE_PIX( UpdateLogic );

		m_spawnTree->UpdateLogic( m_instanceBuffer );
	}

	// compute entries update context
	const Int32 SPAWN_JOB_LIMIT = 16;										// We need to pickup some optimal number of concurrent spawn jobs we want to run. Actually we usually want every entry to have its own spawn job (as it is limited to just one).
	
	SSpawnTreeUpdateSpawnContext context(
		GGame->GetEngineTime(),
		fastForwardContext.m_globalContext->m_referenceTransform,
		(m_runningSpawnJobsCount < SPAWN_JOB_LIMIT && !fastForwardContext.m_globalContext->m_isShutdownRequested) ? SPAWN_JOB_LIMIT - m_runningSpawnJobsCount : 0
		);

	context.m_optimizeTimeouts = false;
	context.m_ignoreMinDistance = true;
	context.m_ignoreVision = true;
	context.m_ignoreSpawnerTimeout = true;

	if ( context.m_spawnJobLimit > 0 )
	{
		PC_SCOPE_PIX( UpdateActiveEntries );

		CTemplateLoadBalancer* loadBalancer = GCommonGame->GetTemplateLoadBalancer();
		loadBalancer->LockJobSpawning();

		for ( Uint32 it = 0; it < m_activeEntriesList.Size(); ++it )
		{
			SActiveEntry* entryData = m_activeEntriesList[ it ];			

			Bool isValid = true;

			if ( !entryData->m_entry->IsPerformingSpawnJob( *entryData->m_instanceBuffer ) )
			{
				PC_SCOPE_PIX( UpdateSpawn );
				context.m_runtimeData = entryData;

				isValid = entryData->m_entry->FastForwardUpdateSpawn( *entryData->m_instanceBuffer, context, fastForwardContext );
			}

			if ( !isValid )
			{
				entryData->m_isActive = false;
				m_activeEntriesList.RemoveAt( it );
				--it;
			}

			if ( context.m_spawnJobLimit <= 0 )
			{
				break;
			}
		}

		loadBalancer->UnlockJobSpawning();
	}

	ForceDespawnDetached();

	if( !m_active )
	{
		TryDeactivate();
	}

	fastForwardContext.m_runningSpawnJobsCount = m_spawnJobs.Size();
}

Bool CEncounter::BeginFastForward( const Vector& referencePosition, FastForwardUpdateContext& context )
{
	SuppressTicks();

	if ( context.m_globalContext->m_parameters.m_despawnExistingGuyz )
	{
		ForceDespawnAll();
	}

	CAreaComponent* trigger = m_triggerArea.Get();
	if ( !trigger || !trigger->TestPointOverlap( referencePosition ) )
	{
		return false;
	}

	ActivateEncounter( REASON_FAST_FORWARD );

	if ( !m_active )
	{
		return false;
	}

	m_spawnTree->UpdateLogic( m_instanceBuffer );

	return true;
}
void CEncounter::EndFastForward()
{
	DeactivateEncounter( REASON_FAST_FORWARD, false );
	UnsuppressTicks();
}

void CEncounter::EnableMember( CName& name, Bool enable )
{
	if( m_spawnTree )
	{
		m_spawnTree->EnableMember( m_instanceBuffer, name, enable );
	}
}

CEncounterParameters* CEncounter::GetEncounterParameters()
{
	CEncounterParameters* encounterParams = m_encounterParameters.Get();
	if ( !encounterParams )
	{
		encounterParams = CEncounterParameters::GetStaticClass()->CreateObject< CEncounterParameters >();
		encounterParams->InitializeAIParameters();
		m_encounterParameters = encounterParams;
	}
	
	encounterParams->SetEncounter( this );

	return encounterParams;
}

Int16 CEncounter::GetCreatureDefinitionId( CName name )
{
	auto itFind = m_compiledCreatureList.Find( name );
	if ( itFind != m_compiledCreatureList.End() )
	{
		return Int16( PtrDiffToUint32( (void*)(itFind - m_compiledCreatureList.Begin()) ) );
	}
	return -1;
}

Bool CEncounter::SetSpawnPhase( CName phaseName )
{
	if ( m_spawnTree->SetSpawnPhase( m_instanceBuffer, phaseName ) )
	{
		m_conditionRetestTimer = 0.f;
		return true;
	}
	return false;
}
void CEncounter::GetSpawnPhases( TDynArray< CName >& outPhaseNames )
{
	m_spawnTree->GetSpawnPhases( outPhaseNames );
}


CEncounterCreatureDefinition* CEncounter::GetCreatureDefinition( CName name )
{
	// Editor only
	if ( !m_instanceBuffer.IsBinded() )
	{
		// fully compute creature definitions
		m_compiledCreatureList.ClearFast();
		CompileCreatureDefinitions( m_compiledCreatureList );
	}

	Int16 id = GetCreatureDefinitionId( name );
	if ( id >= 0 )
	{
		return m_compiledCreatureList[ id ];
	}
	return NULL;
}
ISpawnTreeBaseNode* CEncounter::GetRootNode() const
{
	return m_spawnTree;
}
ISpawnTreeBaseNode* CEncounter::InternalGetRootTreeNode() const
{
	return m_spawnTree;
}
TDynArray< CEncounterCreatureDefinition* >& CEncounter::InternalGetCreatureDefinitions()
{
	return m_creatureDefinition;
}

CEncounterCreatureDefinition* CEncounter::AddCreatureDefinition()
{
	m_creatureDefinition.PushBack( CreateObject< CEncounterCreatureDefinition >( this ) );
	return m_creatureDefinition.Back();
}

void CEncounter::RemoveCreatureDefinition( CEncounterCreatureDefinition* def )
{
	ptrdiff_t idx = m_creatureDefinition.GetIndex( def );
	if ( idx != -1 )
	{
		m_creatureDefinition.Erase( m_creatureDefinition.Begin() + idx );
	}
}

void CEncounter::RemoveCreature( CActor* actor, ESpawnTreeCreatureRemovalReason removalReason )
{
	SCreatureData* creatureEntry = m_creaturePool.GetCreatureEntry( actor );
	// check if creature is 'in the system'
	if ( creatureEntry )
	{
		RemoveScheduledDespawn( *creatureEntry );

		// If actor was already detached from his owning entry, we still need to call deinitializers on him
		SActiveEntry* owningEntry = FindActiveCreatureEntry( creatureEntry->m_lastOwningEntry );
		if ( owningEntry )
		{
			Bool isDetached = creatureEntry->m_lastOwningEntry != creatureEntry->m_listId;
			owningEntry->m_entry->OnCreatureRemoval( *owningEntry->m_instanceBuffer, *creatureEntry, removalReason, isDetached );
		}

		if ( creatureEntry->IsInParty() )
		{
			m_creaturePool.GetPartiesManager().RemoveFromParty( m_creaturePool, *creatureEntry );
		}
		
		if ( creatureEntry->m_creatureDefId >= 0 )
		{
			CEncounterCreatureDefinition* creatureDef = m_compiledCreatureList[ creatureEntry->m_creatureDefId ];
			if ( removalReason == SPAWN_TREE_REMOVAL_KILLED )
			{
				// guy did die
				creatureDef->OnCreatureDead();
				ScheduleFullRespawn();
			}

			// bring him back to pool to spawn
			creatureDef->OnCreatureDespawned();
		}
		
		// remove creature from system
		m_creaturePool.RemoveCreature( *creatureEntry );
	}
	// stop listening to termination
	actor->RegisterTerminationListener( false , this );
}
void CEncounter::GetContextMenuSpecialOptions( SpecialOptionsList& outOptions )
{
	static const String STR_FILL_DEFINITIONS( TXT("Fill up creature definitions") );
	static const String STR_CLEAR_DEFINITIONS( TXT("Clear creature definitions") );
	outOptions.PushBack( STR_FILL_DEFINITIONS );
	outOptions.PushBack( STR_CLEAR_DEFINITIONS );
}
void CEncounter::RunSpecialOption( Int32 option )
{
	switch( option )
	{
	case 0:
		FillUpCreatureDefinition();

		break;
	case 1:
		ClearCreatureDefinitions();
		
		break;
	default:
		break;
	}
}


CEncounter::EDebugState CEncounter::GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const 
{
	return m_active ? CEncounter::EDEBUG_ACTIVE : CEncounter::EDEBUG_DEACTIVATED;
}
Bool CEncounter::HoldsInstanceBuffer() const
{
	return true;
}
CSpawnTreeInstance* CEncounter::GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer )
{
	return m_instanceBuffer.IsBinded() ? &m_instanceBuffer : NULL;
}
ICreatureDefinitionContainer* CEncounter::AsCreatureDefinitionContainer()
{
	return this;
}
void CEncounter::OnDeath( CActor* actor )
{
	RemoveCreature( actor, SPAWN_TREE_REMOVAL_KILLED );
	GCommonGame->GetSystem< CStrayActorManager >()->ConvertToStrayActor( actor );
}
void CEncounter::OnDetach( CActor* actor )
{
	RemoveCreature( actor, SPAWN_TREE_REMOVAL_DESPAWN );
}
void CEncounter::OnPrePooling( CActor* actor )
{
	RemoveCreature( actor, SPAWN_TREE_REMOVAL_POOL );
}
	
CEncounter* CEncounter::AsEncounter()
{
	return this;
}

// Stray actor interface ( See CStrayActorManager )
Bool CEncounter::CanBeConvertedToStrayActor()const
{
	return true;
}
void CEncounter::OnConvertToStrayActor( CActor *const actor )
{
	RemoveCreature( actor, SPAWN_TREE_REMOVAL_KILLED );	
}

CObject* CEncounter::AsCObject()
{
	return this;
}
IEdSpawnTreeNode* CEncounter::GetParentNode() const
{
	return NULL;
}
Bool CEncounter::CanAddChild() const
{
	return m_spawnTree == NULL;
}
void CEncounter::AddChild( IEdSpawnTreeNode* node )
{
	m_spawnTree = static_cast< ISpawnTreeBranch* >( node );
	if ( m_spawnTree )
	{
		m_spawnTree->SetParent( this );
	}
}
void CEncounter::RemoveChild( IEdSpawnTreeNode* node )
{
	if ( m_spawnTree == node )
	{
		m_spawnTree = NULL;
	}
}
Int32 CEncounter::GetNumChildren() const
{
	return m_spawnTree ? 1 : 0;
}
IEdSpawnTreeNode* CEncounter::GetChild( Int32 index ) const
{
	return m_spawnTree;
}
Bool CEncounter::CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const
{
	if (
		classId == CSpawnTreeQuestNode::GetStaticClass() ||
		classId == CSpawnTreeIncludeTreeNode::GetStaticClass()
		)
	{
		return true;
	}
	return false;
}
void CEncounter::GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const
{
	rootClasses.PushBack( ISpawnTreeBranch::GetStaticClass() );
}
void CEncounter::PreStructureModification()
{
	ASSERT( !IsInGame() );
	UnbindBuffer();
}

void CEncounter::funcEnableMember( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, memberName, CName::NONE );
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	EnableMember( memberName, enable );
}

void CEncounter::funcGetPlayerDistFromArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float dist = -1.f;

	CTriggerAreaComponent* triggerArea = m_triggerArea.Get();
	if ( triggerArea )
	{
		CEntity* playerEntity = GGame->GetPlayerEntity();
		if ( playerEntity )
		{
			Vector playerPos = playerEntity->GetWorldPosition();

			if ( triggerArea->TestPointOverlap( playerPos ) ) // player is in area
			{
				dist = 0.f;
			}
			else
			{
				dist = triggerArea->CalcDistToClosestEdge2D( playerPos );
			}
		}
	}

	RETURN_FLOAT( dist );
}

void CEncounter::funcGetEncounterArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_triggerArea.Get() );
}

void CEncounter::funcIsPlayerInEncounterArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const CEntity* player = GGame->GetPlayerEntity();
	ASSERT( player );

	if ( player )
	{
		RETURN_BOOL( IsWithinArea( player->GetWorldPositionRef() ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CEncounter::funcIsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool b = IsEnabled();
	RETURN_BOOL( b );
}

void CEncounter::funcEnterArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if( !m_ignoreAreaTrigger )
	{
		EnterArea();
	}
}

void CEncounter::funcLeaveArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if( !m_ignoreAreaTrigger )
	{
		LeaveArea();
	}
}

void CEncounter::funcEnableEncounter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	EnableEncounter( enable );
}
void CEncounter::funcForceDespawnDetached( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ForceDespawnDetached();
}

void CEncounter::funcSetSpawnPhase( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, phase, CName::NONE );
	FINISH_PARAMETERS;

	Bool outcome = SetSpawnPhase( phase );

	RETURN_BOOL( outcome );
}

Uint64 CEncounter::ComputeHash()
{
	if ( !m_instanceBuffer.IsBinded() )
	{
		return INVALID_HASH;
	}

	Uint64 hash = 0xbaadf00da55550c4;
	for ( const SActiveEntry& entry : m_entries )
	{
		hash ^= entry.m_branchHash;
	}
	return hash;
}

void CEncounter::ForceCleanupAllEncounters()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world )
	{
		WorldAttachedEntitiesIterator it( world );
		while ( it )
		{
			CEncounter* encounter = Cast< CEncounter >( *it );
			if ( encounter )
			{
				if ( encounter->m_active && encounter->m_spawnTree )
				{
					encounter->m_spawnTree->UpdateLogic( encounter->m_instanceBuffer );	
				}
				encounter->ForceDespawnDetached();
			}
			++it;
		}
	}

}

#ifndef NO_EDITOR
CWayPointsCollection::Input* CEncounter::ComputeWaypointCollectionCookingInput( CWorld* world,  CWayPointCookingContext* waypointsData ) const
{
	struct Handler : public CWayPointCookingContext::Handler
	{
		Handler( CWayPointsCollection::Input& input, CTriggerAreaComponent& triggerArea, const TagList& tagList )
			: m_input( input )
			, m_area( triggerArea )
			, m_tagList( tagList ) {}

		virtual void Handle( const CWayPointCookingContext* context, const SWayPointCookingData& waypoint ) override
		{
			if ( !m_area.TestPointOverlap( waypoint.m_position ) )
			{
				return;
			}
			if ( !TagList::MatchAny( waypoint.m_tagList, m_tagList ) )
			{
				return;
			}

			m_input.m_aps.Insert( waypoint.m_id );
		}

		CWayPointsCollection::Input&		m_input;
		const CAreaComponent&				m_area;
		const TagList&						m_tagList;
	};

	CTriggerAreaComponent* triggerArea = FindComponent< CTriggerAreaComponent >();
	if ( !triggerArea || !m_spawnTree )
	{
		return nullptr;
	}

	TagList tags;
	m_spawnTree->CollectSpawnTags( tags );
	CWayPointsCollection::Input* input = new CWayPointsCollection::Input( *waypointsData );

	Handler handler( *input, *triggerArea, tags );

	waypointsData->HandleWaypoints( handler, triggerArea->GetBoundingBox() );

	for ( const SActiveEntry& entry : m_entries )
	{
		entry.m_entry->CookSpawnPoints( *entry.m_instanceBuffer, *input );
	}
	return input;
}

void CEncounter::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	TBaseClass::OnNavigationCook( world, context );

	LazyBindBuffer();

	CWayPointCookingContext* cookedWaypoints = context->Get< CWayPointCookingContext >();

	CWayPointsCollection::Input* input = ComputeWaypointCollectionCookingInput( world, cookedWaypoints );
	if ( input )
	{
		cookedWaypoints->ScheduleCollectionToSave( GetGUID(), input );
	}

	UnbindBuffer();
}
#endif
void CEncounter::OnSpawnPointsCollectionLoaded()
{
	for ( const SActiveEntry& entry : m_entries )
	{
		entry.m_entry->OnSpawnPointsCollectionLoaded( *entry.m_instanceBuffer );
	}
}

namespace
{

	void funcForceCleanupAllEncounters( CObject* context, CScriptStackFrame& stack, void* result )
	{
		FINISH_PARAMETERS;

		CEncounter::ForceCleanupAllEncounters();
	}

};


void RegisterEncounterScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "ForceCleanupAllEncounters", funcForceCleanupAllEncounters );
}

