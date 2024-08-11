/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communitySystem.h"

#include "../engine/gameTime.h"
#include "../engine/gameTimeManager.h"
#include "../engine/idTagManager.h"
#include "../engine/tagManager.h"
#include "../engine/layerInfo.h"
#include "../engine/renderFrame.h"
#include "../engine/jobSpawnEntity.h"

#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/depot.h"
#include "../core/gameSave.h"

#include "actionPointComponent.h"
#include "scenesEntriesManager.h"
#include "actionPointManager.h"
#include "communityData.h"
#include "communityFindApResult.h"
#include "communityErrorReport.h"
#include "communityUtility.h"
#include "communityThreadSlicer.h"
#include "communityAgentStub.h"
#include "communityDebugCounters.h"
#include "communityAgentStubTagManager.h"
#include "oldCommunitySpawnStrategy.h"
#include "questsSystem.h"
#include "spawnTreeInitializer.h"
#include "spawnTreeInitializerAI.h"
#include "spawnTreeDespawnInitializer.h"

RED_DEFINE_STATIC_NAME( CommunityInfo );
RED_DEFINE_STATIC_NAME( CommunityPerformance );

IMPLEMENT_ENGINE_CLASS( CCommunitySystem )

CCommunitySystem::CCommunitySystem()
	: m_currentWorldHash( 0 )
	, m_timeMgr( *GGame->GetTimeManager() )
	, m_active( AS_Inactive )
	, m_scenesEntriesManager( NULL )
	, m_prevSpawnRadius( 0.f )
	, m_prevDespawnRadius( 0.f )
	, m_isCurrentlySpawning( false )
	, m_defaultSpawnStrategy( nullptr )
	, m_communitySpawnInitializer( nullptr )
	, m_agentStubTagManager( nullptr )
#ifdef COMMUNITY_DEBUG_STUBS
	, m_debugData( nullptr )
	, m_dbgOnlyAgentStub( nullptr )
#endif
	, m_npcAttachmentPerformedThisFrame( false )

{}

CCommunitySystem::~CCommunitySystem()
{}

void CCommunitySystem::Initialize()
{
	m_apMan = CreateObject< CActionPointManager >( this );

	// Load global community settings
	LoadCommunityConstants();

	// Create scene manager
	m_scenesEntriesManager = new CScenesEntriesManager( m_registeredCommunities );	

	m_agentsWorld.Reset();

	m_defaultSpawnStrategy = ::CreateObject< COldCommunitySpawnStrategy >();
	m_defaultSpawnStrategy->AddToRootSet();

	m_apMan->AttachListener( *this );

	m_agentStubTagManager = new CCommunityAgentStubTagManager();

	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CCommunitySystem::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );

	SGameSessionManager::GetInstance().RemoveGameSystemDependency( this );
	// destroy the world the agent's live in
	m_agentsWorld.Reset();

	if ( m_apMan )
	{
		m_apMan->DetachListener( *this );
	}


	if ( m_defaultSpawnStrategy )
	{
		m_defaultSpawnStrategy->RemoveFromRootSet();
		m_defaultSpawnStrategy->Discard();
		m_defaultSpawnStrategy = NULL;
	}

	delete m_scenesEntriesManager;

	if ( m_apMan )
	{
		m_apMan->ShutdownManager();
	}

	if ( m_agentStubTagManager )
	{
		delete m_agentStubTagManager;
		m_agentStubTagManager = nullptr;
	}
}

void CCommunitySystem::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( CommunitySystem );

	m_isCurrentlySpawning = false;

	switch ( m_active )
	{
	case AS_StateRestored:
		{
			m_isCurrentlySpawning = true;
			// verify that all agents that were previously spawned are also spawned now
			Bool allRequiredAgentsSpawned = true;
			static TDynArray< SAgentStub* > stubsNotSpawned;
			stubsNotSpawned.ClearFast();

			for ( Uint32 agentStubIdx = 0; agentStubIdx < m_agentsStubs.Size(); ++agentStubIdx )
			{
				SAgentStub *agentStub = m_agentsStubs[ agentStubIdx ];

				if ( agentStub->IsSpawned() == false || agentStub->IsFunctional() )
				{
					continue;
				}

				CNewNPC* npc = agentStub->m_npc.Get();
				if ( npc != NULL && npc->HasFlag( NF_PostAttachSpawnCalled ) )
				{
					continue;
				}

				// Ignore stubs on unloaded layers
				if ( agentStub->IsActionPointLoaded() == false )
				{
					continue;
				}

				allRequiredAgentsSpawned = false;
				stubsNotSpawned.PushBack( agentStub );
			}

			if ( stubsNotSpawned.Empty() )
			{
				allRequiredAgentsSpawned = true;
			}

			if ( allRequiredAgentsSpawned && GCommonGame->GetPlayer() )
			{
				// Hack...
				/*
				//adding null check in case no inventory component present
				CInventoryComponent* playerInvComponent = GCommonGame->GetPlayer()->GetInventoryComponent();
				if ( playerInvComponent && playerInvComponent->AreAllMountedItemsSpawned() )
				{
					// at this point we spawned everything there was to initially spawn,
					// so we can activate the community system
					m_active = AS_Active;
					m_agentsWorld.EnableRestoringState( false );
				}*/

				m_active = AS_Active;
				m_agentsWorld.EnableRestoringState( false );
			}
			break;
		}

	case AS_Active:
		{
			m_wasDetachmentPerformedThisFrame = false;
			m_npcAttachmentPerformedThisFrame = false;
			// spawn agents
			{
				PC_SCOPE_PIX( SpawnTick );
				SpawnAgentsStubsTick();
			}

			// tick spawned agents
			{
				PC_SCOPE_PIX( AgentsTick );
				AgentsTick( timeDelta );
			}

			// despawn agents
			{
				PC_SCOPE_PIX( DespawnTick );
				DespawnTick();
			}

			#ifndef NO_DEBUG_PAGES
			{
				PC_SCOPE_PIX( CommunityDebugTick );
				DebugTick( timeDelta );
			}
			#endif

			{
				PC_SCOPE_PIX( ScenesTick );
				ScenesTick( timeDelta );
			}

			m_despawnerHandler.Update();

			break;
		}
	}

	{
		PC_SCOPE_PIX( AgentsWorldTick );
		m_agentsWorld.Update( timeDelta );
	}

	if ( m_agentStubTagManager != nullptr )
	{
		PC_SCOPE_PIX( UpdateAgentStubTagManager );
		m_agentStubTagManager->Update();
	}
}

#ifndef NO_DEBUG_PAGES

void CCommunitySystem::DebugTick( Float timeDelta )
{
	// update the budget profiler that counts the number of the spawned non-background characters
	static CCommunityMainAgentsCounter mainAgentsCounter;
	Uint32 count = m_agentsStubs.Size();
	Uint32 spawnedAgentsCount = 0;
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( m_agentsStubs[i]->IsSpawned() )
		{
			++spawnedAgentsCount;
		}
	}
	mainAgentsCounter.Set( spawnedAgentsCount );
}
#endif

void CCommunitySystem::ScenesTick( Float timeDelta )
{
	if ( m_scenesEntriesManager )
	{
		m_scenesEntriesManager->OnTick( timeDelta );
	}
}

void CCommunitySystem::AgentsTick( Float timeDelta )
{
	{
		PC_SCOPE( OnAgentsStubTick );
		OnAgentsStubTick( timeDelta );
	}
}

void CCommunitySystem::SpawnAgentsStubsTick()
{
#ifdef COMMUNITY_DEBUG_STUBS
	if ( m_dbgOnlyAgentStub )
	{
		return;
	}
#endif

	static TDynArray< SAgentStub* > stubsToDespawn;
	static TDynArray< Bool > forcedDespawnFlags;
	ASSERT( stubsToDespawn.Empty() && forcedDespawnFlags.Empty() );
	{
		PC_SCOPE_PIX( DeactivateStoryPhases );
		// deactivate story phases
		for ( TDynArray< SDeactivatedStoryPhase >::iterator storyPhaseI = m_deactivatedStoryPhases.Begin(); storyPhaseI != m_deactivatedStoryPhases.End(); ++storyPhaseI )
		{
			const CSTableEntry* inactiveTableEntry = storyPhaseI->m_location.m_communityEntry;
			for ( TDynArray< SAgentStub* >::iterator agentStub = m_allAgentsStubs.Begin(); agentStub != m_allAgentsStubs.End(); ++agentStub )
			{
				if ( (*agentStub)->GetActiveSpawnsetLine() == inactiveTableEntry )
				{
					stubsToDespawn.PushBack( *agentStub );
					forcedDespawnFlags.PushBack( storyPhaseI->m_isForceDespawn );
				}
			}
		}
		m_deactivatedStoryPhases.Clear();
	}

	{
		PC_SCOPE_PIX( ChangeSchedules );
		// process the story phases change - go through the phases to activate
		// and see if those changes apply to the stubs that are about to be deactivated
		for ( auto activeStoryPhase = m_activeStoryPhases.Begin(), end = m_activeStoryPhases.End(); activeStoryPhase != end; ++activeStoryPhase )
		{
			const SStoryPhaseLocation& loc = *activeStoryPhase;
			for ( Int32 i = stubsToDespawn.Size() - 1; i >= 0; --i )
			{
				SAgentStub* stub = stubsToDespawn[i];
				if ( stub->ChangeSchedule( loc ) )
				{
					// we changed this guy's schedule, so we don't really want to despawn him
					stubsToDespawn.Erase( stubsToDespawn.Begin() + i );
					forcedDespawnFlags.Erase( forcedDespawnFlags.Begin() + i );

					// run initializers, since that guy is staying put
					CCommunityInitializers*	initializers = stub->GetInitializers( true );
					if( initializers )
					{
						CNewNPC* npc = stub->m_npc.Get();
						if ( npc )
						{
							const Uint32 s = initializers->m_initializers.Size();
							for( Uint32 i = 0; i < s; i++ )
							{
								ISpawnTreeInitializer* init = initializers->m_initializers[i];
								if( init )
								{	
									ISpawnTreeInitializer::EOutput res = init->Activate( npc, nullptr, nullptr, ISpawnTreeInitializer::EAR_Steal );
									if( res != ISpawnTreeInitializer::OUTPUT_SUCCESS )
									{
										AI_LOG( TXT("ERROR: Story phase community initializer '%ls' for npc '%ls' failed!"), init->GetClass()->GetName().AsString().AsChar(), npc->GetName().AsChar() );
									}
								}
							}
						}
					}
				}
			}
		}
	}

	{
		PC_SCOPE_PIX( DespawnRemaining );
		// despawn the remaining guys
		Uint32 count = stubsToDespawn.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			if( !stubsToDespawn[i]->IsForceDespawned() )
			{
				CSEntitiesEntry* entry = stubsToDespawn[i]->GetEntitiesEntry();
				if( entry )
				{
					stubsToDespawn[i]->m_despawnId = m_despawnerHandler.GetNextDespawnId();
					for( Uint32 i = 0; i < entry->m_despawners->m_initializers.Size(); ++i )
					{
						entry->m_despawners->m_initializers[i]->CreateDespawners( m_despawnerHandler, stubsToDespawn[i]->m_npc.Get(), stubsToDespawn[i]->m_despawnId);
					}
				}
			}

			stubsToDespawn[i]->DespawnAgent( forcedDespawnFlags[i] );
		}
		stubsToDespawn.ClearFast();
		forcedDespawnFlags.ClearFast();
	}

	{
		PC_SCOPE_PIX( ActiveStoryPhase );

		GameTime gameTime = m_timeMgr.GetTime();

		// process story phase activations
		for ( auto activeStoryPhase = m_activeStoryPhases.Begin(), end = m_activeStoryPhases.End(); activeStoryPhase != end; ++activeStoryPhase )
		{
			const auto& storyPhaseEntry = activeStoryPhase->m_communityStoryPhaseEntry;
			const Int32 numKilledNPCs = storyPhaseEntry->GetStoryPhaseData().m_killedNPCsNum;
			Int32 spawnedAgentsDestinationNum = storyPhaseEntry->GetDesiredNumberSpawnedAgents( gameTime );
			if ( numKilledNPCs > 0 )
			{
				ProcessRespawnTimers( storyPhaseEntry );
				spawnedAgentsDestinationNum = Max( spawnedAgentsDestinationNum - numKilledNPCs, 0 );
			}

			const Int32 spawnedAgentsNum = activeStoryPhase->GetNumberSpawnedAgents( this );
			Int32 agentsToSpawnNum = spawnedAgentsDestinationNum - spawnedAgentsNum;

			if ( agentsToSpawnNum > 0 )
			{
				SpawnAgentsStub( agentsToSpawnNum, *activeStoryPhase );
			}
			else if ( agentsToSpawnNum < 0 )
			{
				DespawnAgentsStub( -agentsToSpawnNum, *activeStoryPhase );
			}
		}
	}
}

void CCommunitySystem::SpawnAgentsStub( Int32 agentsToSpawnNum, SActiveStoryPhaseLocation &storyPhase, bool useDelayedSpawn /* = true */ )
{
	PC_SCOPE_PIX( SpawnAgentsStub );
	ASSERT( agentsToSpawnNum > 0 );

	if( !storyPhase.IsWorldValid() )
	{
		return;
	}	

	CSTableEntry& communityEntry = *storyPhase.m_communityEntry;

	// Check if data are correct
	if ( communityEntry.m_entities.Size() == 0 )
	{
		SCSErrRep::GetInstance().BadData( TXT("Cannot spawn agent stub. Empty entities list."), storyPhase );
		return;
	}

	// Delayed spawn
	if ( useDelayedSpawn )
	{
		SStoryPhaseData &sphaseData = storyPhase.m_communityStoryPhaseEntry->GetStoryPhaseData();

		if ( agentsToSpawnNum > 0 && (storyPhase.m_communityStoryPhaseEntry->m_spawnDelay.GetSeconds() / 60) > 0 )
		{
			//// DM: We use engine time for spawning delays, because game world time may be paused/scaled and in effect monsters will not respawn as intended
			// initialize timer
			if ( sphaseData.m_spawnDelay == EngineTime::ZERO )
			{
				sphaseData.m_spawnDelay = GGame->GetEngineTime() + (storyPhase.m_communityStoryPhaseEntry->m_spawnDelay.GetSeconds() / 60);
				return;
			}
			// the time has run out - spawn agent stub
			else if ( sphaseData.m_spawnDelay <= GGame->GetEngineTime() )
			{
				sphaseData.m_spawnDelay = EngineTime::ZERO; // reset timer
				agentsToSpawnNum = 1; // spawn only one stub at once
			}
			// the time hasn't run out yet - don't spawn
			else
			{
				return;
			}
		}
	}

	// Create new agents stubs
	for ( Int32 i = 0; i < agentsToSpawnNum; ++i )
	{
		PC_SCOPE( CreateNewAgent );

		CSEntitiesEntry *communityEntitiesEntry = NULL;
		communityEntry.GetRandomWeightsEntityEntry( &communityEntitiesEntry );
		if ( communityEntitiesEntry == NULL )
		{
			SCSErrRep::GetInstance().BadData( TXT("Cannot spawn agent stub. Empty entity template."), storyPhase );
			continue;
		}

		// Determine spawn position
		SAgentStub *agentStub = CreateAgentStubAtSpawnLocation( storyPhase );
		if ( agentStub )
		{
			// Initialize story phase data
			agentStub->SetStoryPhaseOwner( storyPhase, communityEntitiesEntry );

			// Register
			RegisterSpawnedStub( agentStub );
		}
	}
}

void CCommunitySystem::RegisterSpawnedStub( SAgentStub* agentStub )
{
	// Add to list of stubs
	m_allAgentsStubs.PushBack( agentStub );
	if( IfWorldHashMatch( agentStub->m_ownerWorldHash ) )
	{
		m_agentsStubs.PushBack( agentStub );	
		if ( m_agentStubTagManager )
		{
			m_agentStubTagManager->AddStub( agentStub, agentStub->GetEntitiesEntry()->m_entitySpawnTags );
		}
	}
}

SAgentStub* CCommunitySystem::CreateAgentStubAtSpawnLocation( const SStoryPhaseLocation &storyPhase )
{
	// Determine IdTag to use for spawned stub
	IdTag allocatedIdTag = GGame->GetIdTagManager()->Allocate();

	// OK, we now have valid agent for stub, create rest of the shit
	SAgentStub* stub = new SAgentStub( *this, m_apMan, m_timeMgr, allocatedIdTag );

	stub->m_ownerWorldHash = m_currentWorldHash;
	// register the stub with the agents' world
	m_agentsWorld.AddAgent( stub );
	
	// Return created stub
	return stub;
}

SAgentStub* CCommunitySystem::CreateAgentStubAfterLoad( const SStoryPhaseLocation &storyPhase, const StubSpawnData& spawnData, Bool worldMatch )
{
	// Determine IdTag to use for spawned stub
	IdTag allocatedIdTag;
	if ( spawnData.m_suggestedTag.IsValid() )
	{
		// Use specified IdTag
		allocatedIdTag = spawnData.m_suggestedTag;
	}
	else
	{
		// Allocate new IdTag
		allocatedIdTag = GGame->GetIdTagManager()->Allocate();
	}

	// OK, we now have valid agent for stub, create rest of the shit
	SAgentStub* stub = new SAgentStub( *this, m_apMan, m_timeMgr, allocatedIdTag );

	// register the stub with the agents' world
	if( worldMatch )
	{
		m_agentsWorld.AddAgent( stub );
	}	
    stub->m_agentYaw = spawnData.m_rotation;

	// Return created stub
	return stub;
}


void CCommunitySystem::ProcessRespawnTimers( CSStoryPhaseEntry *communityStoryPhaseEntry )
{
	PC_SCOPE_PIX( ProcessRespawnTimers );

	const CSStoryPhaseSpawnTimetableEntry *storyPhaseSpawnTimetabEntry = CCommunityUtility::GetTimeActiveEntry( communityStoryPhaseEntry->m_spawnTimetable, m_timeMgr.GetTime() );

	if ( !storyPhaseSpawnTimetabEntry || storyPhaseSpawnTimetabEntry->m_respawn == false )
	{
		return;
	}

	SStoryPhaseData& storyPhaseData = communityStoryPhaseEntry->GetStoryPhaseData();

	// Check if respawn delay is implemented
	if ( (storyPhaseSpawnTimetabEntry->m_respawnDelay.GetSeconds() / 60 ) > 0 )
	{
		//// DM: We use engine time for spawning delays, because game world time may be paused/scaled and in effect monsters will not respawn as intended
		if ( storyPhaseData.m_respawnDelay == EngineTime::ZERO )
		{
			storyPhaseData.m_respawnDelay = GGame->GetEngineTime() + (storyPhaseSpawnTimetabEntry->m_respawnDelay.GetSeconds() / 60);
			return;
		}
		else if ( storyPhaseData.m_respawnDelay <= GGame->GetEngineTime() )
		{
			storyPhaseData.m_respawnDelay = EngineTime::ZERO;
			storyPhaseData.m_killedNPCsNum--;
		}
		else
		{
			return;
		}
	}
	else
	{
		storyPhaseData.m_killedNPCsNum--;
	}

	ASSERT( storyPhaseData.m_killedNPCsNum >= 0 && TXT("END: Killed NPCs num cannot be negative") );
}

void CCommunitySystem::DespawnAgentsStub( Int32 agentsToDespawnNum, const SActiveStoryPhaseLocation &storyPhase )
{
	PC_SCOPE_PIX( DespawnAgentsStub );
	ASSERT( agentsToDespawnNum > 0 );

	for ( TDynArray< SAgentStub* >::iterator agentStub = m_agentsStubs.Begin();
		  agentStub != m_agentsStubs.End();
		  ++agentStub )
	{
		// No more agents to despawn
		if ( agentsToDespawnNum <= 0 ) break;

		if ( (*agentStub)->GetActivePhase() == storyPhase.m_communityStoryPhaseEntry )
		{
			(*agentStub)->DespawnAgent();
			--agentsToDespawnNum;
		}
	}

	if ( agentsToDespawnNum != 0 )
	{
		HALT( "Cannot despawn %d agents", agentsToDespawnNum );
	}
}

void CCommunitySystem::RemoveFromAllAgentsList( SAgentStub* agentStub )
{
	 m_allAgentsStubs.RemoveFast( agentStub );
}

void CCommunitySystem::DespawnTick()
{
	for ( TDynArray< SAgentStub* >::iterator it = m_agentsStubs.Begin(); it != m_agentsStubs.End(); )
	{
		SAgentStub* stub = *it;

		if ( stub->m_state == CAS_ToDespawn )
		{
			ASSERT( stub->m_idTag.IsValid() );

			// Cancel spawn job
			if ( stub->m_jobSpawnEntity )
			{
				stub->m_jobSpawnEntity->Cancel();
				stub->m_jobSpawnEntity->Release();
				stub->m_jobSpawnEntity = NULL;
			}

			// remove from the agents' world
			m_agentsWorld.RemoveAgent( stub );

			// Remove from savegame
			GGame->GetUniverseStorage()->RemoveEntity( GGame->GetActiveWorld(), stub->m_idTag );

			if ( m_agentStubTagManager )
			{
				m_agentStubTagManager->RemoveStub( stub, stub->GetEntitiesEntry()->m_entitySpawnTags );
			}

			RemoveFromAllAgentsList( stub );

			// Delete stub
			delete stub;

			// Remove it from list
			m_agentsStubs.EraseFast( it );
			it = m_agentsStubs.Begin();
		}
		else
		{
			++it;
		}
	}
}


void CCommunitySystem::OnGameStart( const CGameInfo& gameInfo )
{	

	SGameSessionManager::GetInstance().RemoveGameSystemDependency( this );	

	CQuestsSystem* questSystem = GCommonGame->GetSystem< CQuestsSystem >();
	ASSERT( questSystem != NULL );
	SGameSessionManager::GetInstance().DefineGameSystemDependency( questSystem, this );

	if( gameInfo.m_isChangingWorldsInGame )
		return;

	// Community AI initialization (script side)
	CObject* context = this;
	CallFunction( context, CNAME( Init ) );
}

void CCommunitySystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	// Cancel stub's streaming jobs
	for ( Int32 i =  m_agentsStubs.Size() - 1; i >= 0; --i )
	{
		SAgentStub* stub = m_agentsStubs[ i ];
		if ( stub && stub->m_jobSpawnEntity )
		{
			stub->m_jobSpawnEntity->Cancel();
			stub->m_jobSpawnEntity->Release();
			stub->m_jobSpawnEntity = NULL;
		}

		CNewNPC *agentStubNPC = stub->m_npc.Get();
		if ( agentStubNPC )
		{
			//stub->DespawnAgentStub();
			agentStubNPC->RegisterTerminationListener( false, this );
			agentStubNPC->Destroy();
		}

		// remove from the agents' world
		m_agentsWorld.RemoveAgent( stub );

		// List needs to be updated here because agentStubNPC->Destroy() ends up calling OnDeath() ( which use m_agentsStubs )
		m_agentsStubs.Erase( m_agentsStubs.Begin() + i );

		if ( m_agentStubTagManager )
		{
			m_agentStubTagManager->RemoveStub( stub, stub->GetEntitiesEntry()->m_entitySpawnTags );
		}

		// dont delete stub, instance is keept in m_allAgentsStubs
		//delete stub;
	}	
}

void CCommunitySystem::OnWorldStart( const CGameInfo& gameInfo )
{
	m_active = AS_Inactive;
	//m_activeStoryPhases.Clear();
	m_currentWorldHash = GGame->GetActiveWorld()->DepotPath().CalcHash();
	LoadCommunityConstants();

	GetVisibilityRadius( m_prevSpawnRadius, m_prevDespawnRadius );

	// Restore or reinitialize the community
	if ( gameInfo.m_gameLoadStream )
	{
		RestoreState( gameInfo.m_gameLoadStream );
	}
	else
	{
		RestoreAgentsAfterWorldChange();
		InitRandomCommunityState();
	}

	m_active = AS_StateRestored;
	m_agentsWorld.EnableRestoringState( true );
}

void CCommunitySystem::OnGameEnd( const CGameInfo& gameInfo )
{
	// Delete the agents management suite
	m_agentsWorld.Reset();
	m_active = AS_Inactive;

	if( gameInfo.m_isChangingWorldsInGame )
	{
		for ( auto activeStoryPhase = m_activeStoryPhases.Begin(), end = m_activeStoryPhases.End(); activeStoryPhase != end; ++activeStoryPhase )
		{
			activeStoryPhase->MarkDirty();
		}		
	}
	else
	{	
		// Deactivate all active communities
		TDynArray< THandle< CCommunity > > registeredCommunities = m_registeredCommunities;
		for ( Uint32 i=0; i<registeredCommunities.Size();	++i )
		{
			// Deactivate community
			CCommunity* community = registeredCommunities[i].Get();
			community->Deactivate();
		}

		// Cleanup community list
		m_registeredCommunities.Clear();

		// Reset active story phase list		
		m_activeStoryPhases.Clear();

		m_allAgentsStubs.ClearPtr();		
	}
}

void CCommunitySystem::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
	if ( file.IsGarbageCollector() )
	{
		for ( Uint32 i=0; i<m_registeredCommunities.Size(); ++i )
		{
			file << m_registeredCommunities[i];
		}

        for ( Uint32 i=0; i<m_allAgentsStubs.Size(); ++i )
        {
            SAgentStub* stub = m_allAgentsStubs[i];
            CCommunity* comm = stub->GetCommunity().Get();
			if ( comm )
			{
				file << comm;
			}
        }
	}
}

void CCommunitySystem::OnAgentsStubTick( Float timeDelta )
{
	GameTime currGameTime = m_timeMgr.GetTime();
	for ( Uint32 agentStubIdx = 0; agentStubIdx < m_agentsStubs.Size(); ++agentStubIdx )
	{
		SAgentStub *agentStub = m_agentsStubs[ agentStubIdx ];		
#ifdef COMMUNITY_DEBUG_STUBS
		if ( m_dbgOnlyAgentStub && m_dbgOnlyAgentStub != agentStub )
		{
			continue;
		}
#endif

		agentStub->Tick( timeDelta, currGameTime );
	}
}

void CCommunitySystem::OnGenerateDebugFragments( CRenderFrame* frame )
{
	m_agentsWorld.OnGenerateDebugFragments( frame );

	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_Spawnset ) ) ) return;

	THashMap< const CWayPointComponent*, String > dbgAPWayPointsInfo;
	THashMap< const CActionPointComponent*, String > dbgAPsInfo;

	Int32 agentIndex = 0;
	for ( TDynArray< SAgentStub* >::const_iterator agentStub = m_agentsStubs.Begin();
		  agentStub != m_agentsStubs.End();
		  ++agentStub, ++agentIndex )
	{
		if ( (*agentStub)->IsOnlyStub() && (*agentStub)->m_communityAgent.IsEnabled() )
		{
			// Skip invalid agents
			Vector worldPos = Vector( (*agentStub)->m_communityAgent.GetPosition() );
			if ( (*agentStub)->m_communityAgent.TestLocation( worldPos.AsVector3() ) )
			{
				// Show agent stub system informations
				frame->AddDebugSphere( worldPos, 0.5f, Matrix::IDENTITY, Color::BLUE );
				String agentStatus = CCommunityUtility::GetFriendlyAgentStateName( (*agentStub)->m_state );
				if ( (*agentStub)->m_state == CAS_WorkInProgress )
				{
					agentStatus += String::Printf( TXT("\n%.1f"), (*agentStub)->m_processingTimer );
				}

				// Action point info
				if ( (*agentStub)->m_state == CAS_MovingToActionPoint )
				{
					String agentNumStr = String::Printf( TXT("Agent num: %d"), agentIndex );
					agentStatus = agentNumStr + TXT("\n") + agentStatus;
				}

				frame->AddDebugText( worldPos, agentStatus.AsChar(), true, Color::WHITE, Color::BLACK );
			}
		}
	}

	// Print debug texts for registered way points
	for ( THashMap< const CWayPointComponent*, String >::const_iterator ci = dbgAPWayPointsInfo.Begin();
		  ci != dbgAPWayPointsInfo.End();
		  ++ci )
	{
		frame->AddDebugText( (*ci).m_first->GetWorldPosition() , (*ci).m_second.AsChar(), true, Color::WHITE, Color::BLACK );
	}

	// Print debug texts for registered action points
	for ( THashMap< const CActionPointComponent*, String >::const_iterator ci = dbgAPsInfo.Begin();
		  ci != dbgAPsInfo.End();
		  ++ci )
	{
		frame->AddDebugText( (*ci).m_first->GetWorldPosition() , (*ci).m_second.AsChar(), true, Color::WHITE, Color::BLACK );
	}

#ifdef COMMUNITY_DEBUG_STUBS
	// Show debug data
	if ( m_debugData )
	{
		if ( m_debugData->m_currentAgentStub )
		{
			// Mark agent position
			if ( !m_debugData->m_currentAgentStub->IsOnlyStub() )
			{
				frame->AddDebugSphere( m_debugData->m_currentAgentStub->m_npc.Get()->GetWorldPosition(), 0.5f, Matrix::IDENTITY, Color::BLACK );
			}
		}

		if ( m_debugData->m_currentAPID != ActionPointBadID )
		{
			Vector apWorldPos;
			m_apMan->GetActionExecutionPosition( m_debugData->m_currentAPID, &apWorldPos, NULL );
			frame->AddDebugSphere( apWorldPos, 0.5f, Matrix::IDENTITY, Color::LIGHT_CYAN );
		}

		if ( m_debugData->m_despawnWayPoint )
		{
			frame->AddDebugSphere( m_debugData->m_despawnWayPoint->GetWorldPosition(), 0.6f, Matrix::IDENTITY, Color::LIGHT_MAGENTA );
		}
	}
#endif
}

void CCommunitySystem::ActivateStoryPhase( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase )
{ 
	if ( community->GetSpawnsetType() == CST_Global )
	{		
		m_activeStoryPhases.Insert( SActiveStoryPhaseLocation( community, communityEntry, communityStoryPhase ) );		
		m_agentsWorld.ForceFullLodUpdate();
	}
}

void CCommunitySystem::DeactivateStoryPhase( CCommunity *community, CSTableEntry *communityEntry, CSStoryPhaseEntry *communityStoryPhase )
{
	if ( community->GetSpawnsetType() == CST_Global )
	{
		Bool isForceDespawn = true;

		// Remove from list of active story phases
		SHashedStoryPhaseLocation storyPhase( community, communityEntry, communityStoryPhase );
		auto itFind = m_activeStoryPhases.Find( storyPhase );
		if( itFind != m_activeStoryPhases.End() )
		{
			CSEntitiesEntry *communityEntitiesEntry = NULL;
			communityEntry->GetRandomWeightsEntityEntry( &communityEntitiesEntry );
			if ( communityEntitiesEntry )
			{
				isForceDespawn = communityEntitiesEntry->IsForceDespawned();
			}

			m_activeStoryPhases.Erase( itFind );
			// despawn inactive agents
			m_deactivatedStoryPhases.PushBack( SDeactivatedStoryPhase( storyPhase, isForceDespawn ) );
		}
		m_agentsWorld.ForceFullLodUpdate();
	}
}

void CCommunitySystem::InitRandomCommunityState()
{
	SpawnAgentsStubsTick();
	Int32 simStepsNum = GEngine->GetRandomNumberGenerator().Get< Int32 >( 5 , 50 );
	for ( int step = 0; step < simStepsNum; ++step )
	{
		OnAgentsStubTick( 2.0f );
	}
	DespawnTick();
}

void CCommunitySystem::SetVisibilityArea( CAreaComponent *area )
{
	m_visibilityArea = area;
}

Bool CCommunitySystem::IsVisibilityAreaEnabled() const
{
	return m_visibilityArea.Get() == NULL ? false : true;
}

void CCommunitySystem::ResetVisibilityArea()
{
	m_visibilityArea = NULL;
}

CAreaComponent* CCommunitySystem::GetVisibilityArea() const 
{ 
	return m_visibilityArea.Get(); 
}

const ISpawnStrategy* CCommunitySystem::GetDefaultSpawnStrategy()
{
	return m_defaultSpawnStrategy;
}

void CCommunitySystem::MarkActiveStoryPhaseAgentsModified( const SHashedStoryPhaseLocation& h )
{
	auto itFind = m_activeStoryPhases.Find( h );
	if ( itFind != m_activeStoryPhases.End() )
	{
		itFind->MarkDirty();
	}
}
void CCommunitySystem::AddActiveStoryPhaseAgent( const SHashedStoryPhaseLocation& h )
{
	auto itFind = m_activeStoryPhases.Find( h );
	if ( itFind != m_activeStoryPhases.End() )
	{
		itFind->AddSpawned();
	}
}
void CCommunitySystem::DelActiveStoryPhaseAgent( const SHashedStoryPhaseLocation& h )
{
	auto itFind = m_activeStoryPhases.Find( h );
	if ( itFind != m_activeStoryPhases.End() )
	{
		itFind->DelSpawned();
	}
}

Bool CCommunitySystem::SetVisibilitySpawnRadius( Float radius )
{
	if ( !CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED ) return false;

	if ( radius > 1.0f && radius < CCommunityConstants::VISIBILITY_DESPAWN_RADIUS )
	{
		CCommunityConstants::VISIBILITY_SPAWN_RADIUS = radius;

		// set the values on the classes that use them
		if ( m_defaultSpawnStrategy )
		{
			m_defaultSpawnStrategy->SetSpawnRadius( CCommunityConstants::VISIBILITY_SPAWN_RADIUS );
		}

		return true;
	}
	else
	{
		return false;
	}
}

Bool CCommunitySystem::SetVisibilityDespawnRadius( Float radius )
{
	if ( !CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED ) return false;

	if ( radius > 1.0f && radius > CCommunityConstants::VISIBILITY_SPAWN_RADIUS )
	{
		CCommunityConstants::VISIBILITY_DESPAWN_RADIUS = radius;

		// set the values on the classes that use them
		if ( m_defaultSpawnStrategy )
		{
			m_defaultSpawnStrategy->SetDespawnRadius( CCommunityConstants::VISIBILITY_DESPAWN_RADIUS );
		}

		return true;
	}
	else
	{
		return false;
	}
}

Bool CCommunitySystem::SetVisibilityRadius( Float spawnRadius, Float despawnRadius )
{
	if ( !CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED ) return false;

	if ( spawnRadius > 1.0f && despawnRadius > 1.0f && despawnRadius > spawnRadius )
	{
		CCommunityConstants::VISIBILITY_SPAWN_RADIUS = spawnRadius;
		CCommunityConstants::VISIBILITY_DESPAWN_RADIUS = despawnRadius;

		// set the values on the classes that use them
		if ( m_defaultSpawnStrategy )
		{
			m_defaultSpawnStrategy->SetSpawnRadius( CCommunityConstants::VISIBILITY_SPAWN_RADIUS );
			m_defaultSpawnStrategy->SetDespawnRadius( CCommunityConstants::VISIBILITY_DESPAWN_RADIUS );
		}
		return true;
	}
	else
	{
		return false;
	}
}

Bool CCommunitySystem::SetVisibilityAreaDespawnRadius( Float areaDespawnRadius )
{
	if ( areaDespawnRadius >= 0 )
	{
		CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS = areaDespawnRadius;
		return true;
	}
	else
	{
		return false;
	}
}

Bool CCommunitySystem::SetVisibilityAreaSpawnRadius( Float areaSpawnRadius )
{
	if ( areaSpawnRadius >= 0 )
	{
		CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS = areaSpawnRadius;
		return true;
	}
	else
	{
		return false;
	}
}

void CCommunitySystem::GetVisibilityRadius( Float &spawnRadius /* out */, Float &despawnRadius /* out */ ) const
{
	spawnRadius = CCommunityConstants::VISIBILITY_SPAWN_RADIUS;
	despawnRadius = CCommunityConstants::VISIBILITY_DESPAWN_RADIUS;
}

void CCommunitySystem::GetPrevVisibilityRadius( Float &prevSpawnRadius /* out */, Float &prevDespawnRadius /* out */ ) const
{
	prevSpawnRadius = m_prevSpawnRadius;
	prevDespawnRadius = m_prevDespawnRadius;
}

void CCommunitySystem::UpdatePrevVisibilityRadius()
{
	m_prevSpawnRadius = CCommunityConstants::VISIBILITY_SPAWN_RADIUS;;
	m_prevDespawnRadius = CCommunityConstants::VISIBILITY_DESPAWN_RADIUS;
}

void CCommunitySystem::GetVisibilityAreaRadius( Float &areaSpawnRadius /* out */, Float &areaDespawnRadius /* out */ ) const
{
	areaSpawnRadius = CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS;
	areaDespawnRadius = CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS;
}

void CCommunitySystem::RestoreDefaultVisibilityRadiuses()
{
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_SPAWN_RADIUS"), CCommunityConstants::VISIBILITY_SPAWN_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_DESPAWN_RADIUS"), CCommunityConstants::VISIBILITY_DESPAWN_RADIUS );
}

void CCommunitySystem::EnableVisibilityRadiusChange( Bool enable /* = true */ )
{
	CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED = enable;
}

const SAgentStub * CCommunitySystem::FindStubForNPC( const CNewNPC * npc ) const
{
	if( npc )
	{
		// Linear search
		for ( Uint32 i=0; i<m_agentsStubs.Size(); ++i )
		{
			SAgentStub* stub = m_agentsStubs[i];
			if ( stub->m_npc.Get() == npc || stub->GetSpawnJobEntity() == npc )
			{
				return stub;
			}
		}
	}

	// Not found
	return NULL;
}

Bool CCommunitySystem::IsNPCInCommunity( const CNewNPC* npc ) const
{
	return FindStubForNPC( npc ) != NULL;
}

#define MAX_WAIT_ASYNC_SPAWN_TIME 10.f

// AP TODO: ta kwerende to lepiej do swiata agentow
Bool CCommunitySystem::GetSpawnedNPC( const TSoftHandle<CEntityTemplate>& entityTemplate, CName appearance, const Vector &worldPosition, Bool spawnOnDemand, CNewNPC *&outputNPC )
{
	SAgentStub *agentStubFound = NULL;
	Float shortestDistSq = NumericLimits< Float >::Max();

	for ( SAgentStub* agentStub : m_agentsStubs )
	{
		if ( agentStub->m_state == CAS_Despawning || agentStub->m_state == CAS_ToDespawn )
		{
			continue;
		}

		CSEntitiesEntry* communityEntitiesEntry = agentStub->GetEntitiesEntry();
		Bool entityTemplateMatch = communityEntitiesEntry && ( communityEntitiesEntry->m_entityTemplate == entityTemplate );
		Bool appearanceMatch = !appearance || !agentStub->m_appearance || appearance == agentStub->m_appearance;
		
		if ( entityTemplateMatch && appearanceMatch && agentStub->m_communityAgent.IsEnabled() )
		{
			const Float currDistSq = ( worldPosition.AsVector3() - agentStub->m_communityAgent.GetPosition() ).SquareMag();
			if ( currDistSq < shortestDistSq )
			{
				shortestDistSq = currDistSq;
				agentStubFound = agentStub;
			}
		}
	}

	if ( agentStubFound )
	{
		if ( agentStubFound->IsOnlyStub() && spawnOnDemand )
		{
			//This contraption is made to make sure npc will be spawned here (sync) even if he is currently being loaded on async thread 
			CTimeCounter timer;
			SAgentStub::EAgentSpawnResult jobResult = SAgentStub::ASR_Loading;
			while( timer.GetTimePeriod() < MAX_WAIT_ASYNC_SPAWN_TIME && jobResult == SAgentStub::ASR_Loading )
			{				
				jobResult = agentStubFound->SpawnAgentNPC( false );									
			}			
		}		
		outputNPC = agentStubFound->m_npc.Get();		
	}
	return outputNPC != nullptr;
}

EFindAPResult CCommunitySystem::GetRandomActionCategoryAndLayer( const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry, CLayerInfo* &layerInfo /* out */, CName &actionCategoryName /* in out */, TagList &actionPointTags /* out */, Vector* mapPinPos )
{
	if ( timetabEntry->m_actions.Empty() )
	{
		// TODO: add WARN - empty action categories
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - empty action categories.") );
		return FAPR_TimetabEmptyCategory;
	}

	Float weightsSum = 0.0f;
	Float omitedWeightsSum = 0.0f;
	Bool  useOmited = false;
	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		  action != timetabEntry->m_actions.End();
		  ++action )
	{
		if( !action->m_layerName.GetCachedLayer() )
		{
			continue;
		}

		for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
			  actionCategory != action->m_actionCategories.End();
			  ++actionCategory )
		{			
			if ( actionCategory->m_weight <= 0 )
			{
				SCSErrRep::GetInstance().LogActionPoint( TXT("Action category weight has to be greater than zero.") );
			}

			// do not choose the same action category as the previous one (passed by parameter)
			if ( actionCategoryName != CName::NONE && actionCategoryName == actionCategory->m_name )
			{
				omitedWeightsSum += actionCategory->m_weight;
				continue;
			}

			weightsSum += actionCategory->m_weight;
		}
	}

	// there isn't any category different from the previous one (passed by parameter)
	if ( weightsSum <= 0 )
	{
		weightsSum += omitedWeightsSum;
		useOmited = true;
	}

	if ( weightsSum <= 0 )
	{
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - empty action categories or weights are not greater than zero.") );
		return FAPR_TimetabEmptyCategory;
	}

	Float randomFloat = GEngine->GetRandomNumberGenerator().Get< Float >() * weightsSum;

	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		action != timetabEntry->m_actions.End();
		++action )
	{
		if( !action->m_layerName.GetCachedLayer() )
		{
			continue;
		}
		for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
			actionCategory != action->m_actionCategories.End();
			++actionCategory )
		{
			// do not choose the same action category as the previous one (passed by parameter)
			if ( !useOmited && actionCategoryName != CName::NONE && actionCategoryName == actionCategory->m_name )
			{
				continue;
			}
			randomFloat -= actionCategory->m_weight;
			CLayerInfo* layer =  action->m_layerName.GetCachedLayer();
			if ( randomFloat <= 0 && layer )
			{
				// get layer
				layerInfo = layer;
				//if ( !layerInfo )
				//{
				//	return FAPR_LayerNotFound;
				//}

				// write result end exit
				actionCategoryName = actionCategory->m_name;
				actionPointTags = actionCategory->m_apTags;

				return FAPR_Success;
			}
		}
	}

	ASSERT( !TXT("Problem in random method, this should never happen.") );
	return FAPR_UnknownError;
}

Bool CCommunitySystem::FindDespawnPoint( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > *timetable, const TagList &despawnTags, Vector &despawnPoint /* out */ )
{
	// First try to find despawn point at agent stub house
	if ( timetable )
	{
		// TODO: integrate despawning on doors, etc
	}

	// No house spawn point found, so try to find waypoint with despawn tag
	TDynArray< CNode* > despawnNodes;
	CWorld *world = GGame->GetActiveWorld();
	if ( world )
	{
		world->GetTagManager()->CollectTaggedNodes( despawnTags, despawnNodes, BCTO_MatchAll );
	}

	// Get random despawn point
	if ( !despawnNodes.Empty() )
	{
		Int32 spawnPointIndex = GEngine->GetRandomNumberGenerator().Get< Int32 >( despawnNodes.Size() );
		despawnPoint = despawnNodes[ spawnPointIndex ]->GetWorldPosition();
		return true;
	}

	return false;
}

EFindAPResult CCommunitySystem::FindCurrentActionPointFilter( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry >& timetable, SActionPointFilter& actionPointFilter )
{
	// Get active entry based on current time.
	const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry = CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( timetable, m_timeMgr.GetTime() );
	if ( timetabEntry == NULL )
	{
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - timetable entry is empty.") );
		return FAPR_TimetabEmpty;
	}

	EFindAPResult result;
	result = GetActionPointFilter( timetabEntry, actionPointFilter );
	if ( result != FAPR_Success )
	{
		return result;
	}

	ASSERT( actionPointFilter.m_category != CName::NONE && TXT("CCommunitySystem::FindCurrentActionPointFilter(): Category name is NONE on method Success") );
	return FAPR_Success;
}

EFindAPResult CCommunitySystem::GetActionPointFilter( const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry, SActionPointFilter& actionPointFilter )
{
	if ( timetabEntry->m_actions.Empty() )
	{
		// TODO: add WARN - empty action categories
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - empty action categories.") );
		return FAPR_TimetabEmptyCategory;
	}

	Float weightsSum = 0.0f;
	Float omitedWeightsSum = 0.0f;
	Bool  useOmited = false;
	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		action != timetabEntry->m_actions.End();
		++action )
	{
		if( !action->m_layerName.GetCachedLayer() )
		{
			continue;
		}

		for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
			actionCategory != action->m_actionCategories.End();
			++actionCategory )
		{
			if ( actionCategory->m_weight <= 0 )
			{
				SCSErrRep::GetInstance().LogActionPoint( TXT("Action category weight has to be greater than zero.") );
			}

			// do not choose the same action category as the previous one (passed by parameter)
			if ( actionPointFilter.m_category != CName::NONE && actionPointFilter.m_category == actionCategory->m_name )
			{
				omitedWeightsSum += actionCategory->m_weight;
				continue;
			}

			weightsSum += actionCategory->m_weight;
		}
	}

	// there isn't any category different from the previous one (passed by parameter)
	if ( weightsSum <= 0 )
	{
		weightsSum += omitedWeightsSum;
		useOmited = true;
	}

	if ( weightsSum <= 0 )
	{
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - empty action categories or weights are not greater than zero.") );
		return FAPR_TimetabEmptyCategory;
	}

	Float randomFloat = GEngine->GetRandomNumberGenerator().Get< Float >( weightsSum );

	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		action != timetabEntry->m_actions.End();
		++action )
	{

		if( !action->m_layerName.GetCachedLayer() )
		{
			continue;
		}			

		for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
			actionCategory != action->m_actionCategories.End();
			++actionCategory )
		{
			// do not choose the same action category as the previous one (passed by parameter)
			if ( !useOmited && actionPointFilter.m_category != CName::NONE && actionPointFilter.m_category == actionCategory->m_name )
			{
				continue;
			}
			randomFloat -= actionCategory->m_weight;
			if ( randomFloat <= 0 )
			{
				// write result end exit
				ASSERT(actionCategory->m_name != CName::NONE, TXT("Action category name is empty. Make sure all community timetable entries have categories assigned."));
				actionPointFilter.m_category = actionCategory->m_name;
				actionPointFilter.m_actionPointTags = actionCategory->m_apTags;
				CLayerInfo* layerInfo = action->m_layerName.GetCachedLayer();
				if ( layerInfo )
				{
					actionPointFilter.m_layerGuid = layerInfo->GetGUID();
				}

				return FAPR_Success;
			}
		}
	}

	ASSERT( !TXT("Problem in random method, this should never happen.") );
	return FAPR_UnknownError;
}

ISpawnTreeInitializerAI* CCommunitySystem::GetCommunitySpawnInitializer() const
{
	return m_communitySpawnInitializer.Get();
}

EFindAPResult CCommunitySystem::FindRandomActionCategoryAndLayerName( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > &timetable, CName &actionCategoryName /* out */, CLayerInfo *&layerInfo /* out */, TagList &actionPointTags /* out */, Vector* mapPinPos )
{
	// Get active entry based on current time.
	const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry = CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( timetable, m_timeMgr.GetTime() );
	if ( timetabEntry == NULL )
	{
		SCSErrRep::GetInstance().LogActionPoint( TXT("No AP found - timetable entry is empty.") );
		return FAPR_TimetabEmpty;
	}

	EFindAPResult result;
	result = GetRandomActionCategoryAndLayer( timetabEntry, layerInfo, actionCategoryName, actionPointTags, mapPinPos );
	if ( result != FAPR_Success )
	{
		return result;
	}

	ASSERT( actionCategoryName != CName::NONE && TXT("CCommunitySystem::FindRandomActionCategoryAndLayerName(): Category name is NONE on method Success") );
	return FAPR_Success;
}
#ifdef COMMUNITY_DEBUG_STUBS
void CCommunitySystem::SetDebugData( const SDebugData &debugData )
{
	if ( m_debugData )
	{
		delete m_debugData;
		m_debugData = NULL;
	}

	m_debugData = new SDebugData( debugData );
}
#endif

Int32 CCommunitySystem::GetNPCsNum()
{
	Int32 result = 0;

	for ( TDynArray< SAgentStub* >::const_iterator ci = m_agentsStubs.Begin();
		  ci != m_agentsStubs.End();
		  ++ci )
	{
		if ( !(*ci)->IsOnlyStub() ) ++result;
	}

	return result;
}

void CCommunitySystem::GetStubsInfo( Int32 &stubsCount /* out */, Int32 &npcCount /* out */ )
{
	stubsCount = m_agentsStubs.Size();
	npcCount = 0;

	for ( TDynArray< SAgentStub* >::const_iterator ci = m_agentsStubs.Begin();
		ci != m_agentsStubs.End();
		++ci )
	{
		if ( !(*ci)->IsOnlyStub() ) ++npcCount;
	}
}

Int32 CCommunitySystem::GetStubIndex( SAgentStub *agentStub )
{
	for ( Uint32 i = 0; i < m_agentsStubs.Size(); ++i )
	{
		if ( m_agentsStubs[i] == agentStub )
		{
			return static_cast< Int32 >( i );
		}
	}
	return -1;
}

void CCommunitySystem::GetMappinTrackedStubsInfo( Int32 &questStubsFound /* out */, Int32 &merchantStubsFound /* out */, String &info /* out */ )
{
#if 0 // GFx 3
	CHudInstance *hud = GWitcherGame->GetHudInstance();
	questStubsFound = 0;
	merchantStubsFound = 0;
	TDynArray< Int32 > questStubsIdxs;
	TDynArray< Int32 > merchantStubsIdxs;
	EMapPinType mapPinType;
	for ( Uint32 i = 0; i < m_agentsStubs.Size(); ++i )
	{
		if ( m_agentsStubs[i]->HasMappin() )
		{
			if ( hud->GetDynamicMapPinType( m_agentsStubs[i]->m_mapPinId, mapPinType ) )
			{
				if ( mapPinType != MapPinType_Quest )
				{
					++merchantStubsFound;
					merchantStubsIdxs.PushBack( i );
				}
				else
				{
					++questStubsFound;
					questStubsIdxs.PushBack( i );
				}
			}
		}
	}

	info = TXT("Quest Stub idx: ");
	for ( Uint32 i = 0; i < questStubsIdxs.Size(); ++i )
	{
		info += ToString( questStubsIdxs[i] ) + TXT(" ");
	}

	info += TXT("<br>Merchant Stub idx: ");
	for ( Uint32 i = 0; i < merchantStubsIdxs.Size(); ++i )
	{
		info += ToString( merchantStubsIdxs[i] ) + TXT(" ");
	}
#endif // #if 0
}

void CCommunitySystem::DebugDeactivateAllCommunitiesButThis( const String &communityDepotPath )
{
	for ( auto it = m_activeStoryPhases.Begin(); it != m_activeStoryPhases.End(); )
	{
		if ( it->m_community->GetDepotPath() != communityDepotPath )
		{
			m_deactivatedStoryPhases.PushBack( SDeactivatedStoryPhase( *it, false ) );
			m_activeStoryPhases.Erase( it );
			continue;
		}
		++it;
	}
}

void CCommunitySystem::LoadCommunityConstants()
{
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("SPAWN_POINT_RADIUS"), CCommunityConstants::SPAWN_POINT_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_SPAWN_RADIUS"), CCommunityConstants::VISIBILITY_SPAWN_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_DESPAWN_RADIUS"), CCommunityConstants::VISIBILITY_DESPAWN_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_AREA_SPAWN_RADIUS"), CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_AREA_DESPAWN_RADIUS"), CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS );
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("community"), TXT("Constants"), TXT("VISIBILITY_RADIUS_CHANGE_ENABLED"), CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED );

	// set the values on the classes that use them
	if ( m_defaultSpawnStrategy )
	{
		m_defaultSpawnStrategy->SetSpawnRadius( CCommunityConstants::VISIBILITY_SPAWN_RADIUS );
		m_defaultSpawnStrategy->SetDespawnRadius( CCommunityConstants::VISIBILITY_DESPAWN_RADIUS );
	}
}

void CCommunitySystem::OnCommunityActivated( CCommunity* community )
{
	if ( !m_registeredCommunities.Exist( community ) )
	{
		// Add to community list
		m_registeredCommunities.PushBack( community );

		// Cache the internal data
		community->CacheInternalData();

		// Reset community
		community->ResetRTData();
	}
}

void CCommunitySystem::OnCommunityDeactivated( CCommunity* community )
{
	// Remove from list
	m_registeredCommunities.Remove( community );
}

Bool CCommunitySystem::IsCommunityValidForSpawning( const CCommunity* community ) const
{
	// No world, no spawning yet
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return false;
	}

	// Not found in streamable communities, allow to spawn
	return true;
}

Int32 CCommunitySystem::DebugGetNumStatusPages()
{
	return 2 + m_agentsStubs.Size();
}

void CCommunitySystem::DebugUpdateStatusPage( Int32 num, ICommunityDebugPage& debugPage )
{
	if ( num == 0 )
	{
		Int32 stubsCount;
		Int32 npcCount;
		GetStubsInfo( stubsCount, npcCount );

		debugPage.AddText( String::Printf( TXT("Agent stubs (spawned NPCs) : %d [%d]"), stubsCount, npcCount ), Color::WHITE );
		debugPage.AddText( TXT("Is visibility radius change enabled: ") + ToString( CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED ), Color::WHITE );
		debugPage.AddText( TXT("Press 'V' for visibility radius change switch."), Color::WHITE );
	}
	else if ( num == 1 )
	{
		String msg;

		debugPage.AddText( TXT("Active story phases"), Color::YELLOW );

		Uint32 commIdx = 0;
		for ( TDynArray< THandle< CCommunity > >::const_iterator commIt = m_registeredCommunities.Begin();
			commIt != m_registeredCommunities.End();
			++commIt, ++commIdx )
		{
			(*commIt)->GetDebugStatus( commIdx, debugPage );
		}
	}
	else if ( num > 1 && !m_agentsStubs.Empty() )
	{
		Uint32 stubIdx = ::Min< Uint32 >( num - 2, m_agentsStubs.Size() - 1 );
		debugPage.AddText( String::Printf( TXT( "Stub %d" ), stubIdx ), Color::YELLOW );

		if ( m_agentsStubs[stubIdx] )
		{
			m_agentsStubs[stubIdx]->GetDebugInfo( debugPage );
		}
	}
}

Bool CCommunitySystem::OnSaveGame( IGameSaver* saver )
{
	TIMER_BLOCK( time )

	CGameSaverBlock block( saver, CNAME(community) );

	// Write spawnsets
	{
		CGameSaverBlock block( saver, CNAME(communities) );

		// Count spawnsets
		const Uint32 numCommunities = m_registeredCommunities.Size();
		saver->WriteValue( CNAME(numCommunities), numCommunities );

		// Store information about active spawn sets
		for ( Uint32 i=0; i<numCommunities; i++ )
		{
			CGameSaverBlock block( saver, CNAME(c) );

			// Save spawnset file path
			CCommunity* community = m_registeredCommunities[i].Get();
			saver->WriteValue( CNAME(p), community->GetDepotPath() );

			// Save spawnset data
			community->SaveState( saver );
		}
	}

	// Stubs
	{
		CGameSaverBlock block( saver, CNAME(stubs) );

		// Count stubs to store
		Int32 numStubsToSave = 0;
		for ( Uint32 i=0; i<m_allAgentsStubs.Size(); ++i )
		{
			SAgentStub* stub = m_allAgentsStubs[ i ];
			if ( stub && stub->m_idTag.IsValid() )
			{
				numStubsToSave += 1;
			}
		}

		// Store stubs
		saver->WriteValue( CNAME(numStubs), numStubsToSave );
		for ( Uint32 i=0; i<m_allAgentsStubs.Size(); ++i )
		{
			SAgentStub* stub = m_allAgentsStubs[ i ];
			if ( stub && stub->m_idTag.IsValid() )
			{
				stub->SaveState( saver );
			}
		}
	}

	// Global community stuff
	{
		CGameSaverBlock block( saver, CNAME(global_community) );

		Float spawnRadius, despawnRadius, areaSpawnRadius, areaDespawnRadius;
		GetVisibilityRadius( spawnRadius, despawnRadius );
		GetVisibilityAreaRadius( areaSpawnRadius, areaDespawnRadius );
		ASSERT( spawnRadius > 0 && despawnRadius > 0 && areaDespawnRadius > 0 );
		saver->WriteValue< Float >( CNAME(spawnRadius), spawnRadius );
		saver->WriteValue< Float >( CNAME(despawnRadius), despawnRadius );
		saver->WriteValue< Float >( CNAME(areaSpawnRadius), areaSpawnRadius );
		saver->WriteValue< Float >( CNAME(areaDespawnRadius), areaDespawnRadius );
		saver->WriteValue< Float >( CNAME(prevSpawnRadius), m_prevSpawnRadius );
		saver->WriteValue< Float >( CNAME(prevDespawnRadius), m_prevDespawnRadius );
	}

	END_TIMER_BLOCK( time )

	// Saved
	return true;
}

void CCommunitySystem::RestoreState( IGameLoader* loader )
{
	m_deactivatedStoryPhases.ClearFast();

	CGameSaverBlock block( loader, CNAME(community) );	

	// Load spawnset
	{
		CGameSaverBlock block( loader, CNAME(communities) );

		// Load count
		const Uint32 count = loader->ReadValue< Uint32 >( CNAME(numCommunities) );
		for ( Uint32 i=0; i<count; i++ )
		{
			CGameSaverBlock block( loader, CNAME(c) );

			// Save spawnset file path
			String path = loader->ReadValue<String>( CNAME(p) );
			CCommunity* spawnset = LoadResource< CCommunity >( path );
			if ( spawnset )
			{
				// Restore state
				spawnset->RestoreState( loader );
			}
			else
			{
				WARN_GAME( TXT( "Spawnset resource could not be loaded from file %s" ), path.AsChar() );
			}
		}
	}

	// Stubs
	{
		CGameSaverBlock block( loader, CNAME(stubs) );

		// Load stubs
		GameTime currGameTime = m_timeMgr.GetTime();
		const Uint32 count = loader->ReadValue< Int32 >( CNAME(numStubs) );
		for ( Uint32 i=0; i<count; ++i )
		{
			SAgentStub* agentStub = SAgentStub::RestoreState( loader, this, m_activeStoryPhases, currGameTime );
			if ( agentStub )
			{
				// Register
				RegisterSpawnedStub( agentStub );
			}
		}
	}

	// Global community stuff
	{
		CGameSaverBlock block( loader, CNAME(global_community) );

		Float spawnRadius, despawnRadius, areaSpawnRadius, areaDespawnRadius;
		spawnRadius = loader->ReadValue< Float >( CNAME(spawnRadius) );
		despawnRadius = loader->ReadValue< Float >( CNAME(despawnRadius) );
		areaSpawnRadius = loader->ReadValue< Float >( CNAME(areaSpawnRadius) );
		areaDespawnRadius = loader->ReadValue< Float >( CNAME(areaDespawnRadius) );
		m_prevSpawnRadius = loader->ReadValue< Float >( CNAME(prevSpawnRadius) );
		m_prevDespawnRadius = loader->ReadValue< Float >( CNAME(prevDespawnRadius) );
		ASSERT( spawnRadius > 0 && despawnRadius > 0 && areaDespawnRadius > 0 );
		SetVisibilityRadius( spawnRadius, despawnRadius );
		SetVisibilityAreaSpawnRadius( areaSpawnRadius );
		SetVisibilityAreaDespawnRadius( areaDespawnRadius );
	}
}

void CCommunitySystem::RestoreAgentsAfterWorldChange()
{
	for( Uint32 i=0; i<m_allAgentsStubs.Size(); ++i )
	{
		SAgentStub* stub = m_allAgentsStubs[ i ];
		if( stub && IfWorldHashMatch( stub->m_ownerWorldHash ) )
		{
			CCommunity * community = stub->GetStoryPhaseCurrentOwner()->m_community.Get();
			if( !community || !community->IsActive() )
			{
				//delete orphant - we used to have a bug, that agents from world different that active were not removed when community was deactivated
				m_allAgentsStubs.RemoveAt( i );
				delete stub;
				--i;
				continue;
			}

			m_agentsWorld.AddAgent( stub );

			Bool added = m_agentsStubs.PushBackUnique( stub );	
			if ( added && m_agentStubTagManager )
			{
				m_agentStubTagManager->AddStub( stub, stub->GetEntitiesEntry()->m_entitySpawnTags );
			}
		}
	}
}

void CCommunitySystem::UpdateAPOccupation( const TActionPointID& apID, Bool& outIsOccupied ) const
{
	for ( TDynArray< SAgentStub* >::const_iterator it = m_agentsStubs.Begin(); it != m_agentsStubs.End(); ++it )
	{
		const NewNPCSchedule& schedule = (*it)->GetSchedule(); 
		if ( schedule.m_activeActionPointID == apID )
		{
			outIsOccupied = true;
			break;
		}
	}
}
void CCommunitySystem::OnAPManagerDeletion()
{
	m_apMan = NULL;
}

// ---------------------------------------------------------------------------
// IActorTerminationListener interface
// ---------------------------------------------------------------------------
void CCommunitySystem::TerminateActor( CActor* actor, Bool death )
{
	for ( TDynArray< SAgentStub* >::iterator stubI = m_agentsStubs.Begin();
		stubI != m_agentsStubs.End();
		++stubI )
	{
		CNewNPC* npc = (*stubI)->m_npc.Get();
		if ( npc == actor )
		{	
			m_despawnerHandler.RemoveDespawner( (*stubI)->m_despawnId );
			
			actor->RegisterTerminationListener( false, this );

			if( death )
			{
				(*stubI)->GetActivePhase()->GetStoryPhaseData().m_killedNPCsNum += 1;
			}

			if ( npc )
			{
				// clear agent stub schedule in npc
				npc->ClearWorkSchedule();
			}

			(*stubI)->m_npc = NULL; // disconnect stub from NPC
			if( death )
			{
				(*stubI)->DespawnAgentStub();	
			}
			break; // NPC is unique for stub, so we can stop our searching
		}
	}
}

/// On death on npc
void CCommunitySystem::OnDeath( CActor* actor )
{
	TerminateActor( actor, true );
}
/// On npc detached
void CCommunitySystem::OnDetach( CActor* actor )
{
	TerminateActor( actor, false );
}

// Stray actor interface ( See CStrayActorManager )
Bool CCommunitySystem::CanBeConvertedToStrayActor()const
{
	return false;
}

void funcSetCommunitySpawnRadius( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, spawnRadius, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( GCommonGame->GetSystem< CCommunitySystem >()->SetVisibilitySpawnRadius( spawnRadius ) );
}

void funcSetCommunityDespawnRadius( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, despawnRadius, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( GCommonGame->GetSystem< CCommunitySystem >()->SetVisibilityDespawnRadius( despawnRadius ) );
}

void funcSetCommunityRadius( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, spawnRadius, 0 );
	GET_PARAMETER( Float, despawnRadius, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( GCommonGame->GetSystem< CCommunitySystem >()->SetVisibilityRadius( spawnRadius, despawnRadius ) );
}

void RegisterCommunityScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "SetCommunitySpawnRadius", funcSetCommunitySpawnRadius );
	NATIVE_GLOBAL_FUNCTION( "SetCommunityDespawnRadius", funcSetCommunityDespawnRadius );
	NATIVE_GLOBAL_FUNCTION( "SetCommunityRadius", funcSetCommunityRadius );
}
