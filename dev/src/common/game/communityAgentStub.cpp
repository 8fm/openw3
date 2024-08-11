#include "build.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "communityUtility.h"
#include "gameWorld.h"
#include "movableRepresentationPathAgent.h"
#include "spawnTreeInitializerAI.h"
#include "storyPhaseLocation.h"
#include "entityPool.h"
#include "../core/diskFile.h"
#include "../core/gameSave.h"
#include "../engine/gameSaveManager.h"
#include "../engine/gameTimeManager.h"
#include "../engine/dynamicLayer.h"
#include "../engine/layerInfo.h"
#include "../engine/jobSpawnEntity.h"
#include "../engine/pathlibWorld.h"

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( ownerWorldHash )

CCommunityAgent::CCommunityAgent()
	: m_agentCategory( 0 )
	, m_personalSpace( 0.5f )	
	, m_currentArea( PathLib::INVALID_AREA_ID )
	, m_position( 0.f, 0.f, 0.f )		
	, m_isEnabled( false )	
{

}


void CCommunityAgent::MoveTo( const Vector3& destination )
{
	m_position = destination;	
}
void CCommunityAgent::SetPosition( const Vector3& destination )
{	
	m_position = destination;
}
const Vector3& CCommunityAgent::GetPosition() const
{	
	return m_position;
}
Bool CCommunityAgent::TestLocation( const Vector3& pos ) const
{
	// TODO: creature<->creature tests
	//CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	//if ( pathlib )
	//{
	//	return pathlib->TestLocation( m_currentArea, pos, m_personalSpace, PathLib::CT_DEFAULT );
	//}
	return true;
}
Bool CCommunityAgent::FindSafeSpot( const Vector3& pos, Float radius, Vector3& outPos ) const
{
	//CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	//if ( pathlib )
	//{
	//	if ( pathlib->TestLocation( m_currentArea, pos, m_personalSpace, PathLib::CT_DEFAULT ) )
	//	{
	//		outPos = pos;
	//		return true;
	//	}
	//	return pathlib->FindSafeSpot( m_currentArea, pos, radius, m_personalSpace, outPos );
	//}
	outPos = pos;
	return true;
}
Bool CCommunityAgent::TestCurrentPosition() const
{
	return TestLocation( GetPosition() );
}

//////////////////////////////////////////////////////////////////////////

SAgentStub::SAgentStub( CCommunitySystem& cs, THandle< CActionPointManager > apMan, const CTimeManager& timeMgr, const IdTag& idTag )
	: m_npc( NULL )
	, m_processingTimer( 0 )
	, m_spawnPos( FLT_MAX, FLT_MAX, FLT_MAX )
	, m_agentYaw( 0 )
	, m_appearance( CName::NONE )
	, m_jobSpawnEntity( NULL )
	, m_mapPinId( -1 )
	, m_cs( cs )
	, m_apMan( apMan )
	, m_timeMgr( timeMgr )
	, m_idTag( idTag )
	, m_state( CAS_Unknown )
	, m_hiddenOnStartTicksLeftHACK( 0 )
	, m_despawnId( 0 )
	, m_forceSpawn( false )
	, m_disabled( false )
	, m_teleportToDestination( true )
{
	m_guid = CGUID::Create();
}

SAgentStub::~SAgentStub()
{
	// free the occupied action point
	if ( m_schedule.m_activeActionPointID != ActionPointBadID )
	{
		if( m_apMan )
		{
			m_apMan->SetFree( m_schedule.m_activeActionPointID, CActionPointManager::REASON_COMMUNITY );
		}

		m_schedule.m_activeActionPointID = ActionPointBadID;
		m_schedule.m_actionCategoryName = CName::NONE;
	}
}

String SAgentStub::GetDescription() const
{
	return String::Printf( TXT( "Appearance : %s, Spawnset: %s, Story phase: %s" ), 
		m_appearance.AsString().AsChar(), 
		m_storyPhaseCurrentOwner.m_community->GetFriendlyName().AsChar(),
		m_storyPhaseCurrentOwner.m_community->GetActivePhaseName().AsString().AsChar() );
}

void SAgentStub::SetStoryPhaseOwner( const SHashedStoryPhaseLocation &storyPhase, CSEntitiesEntry *communityEntitiesEntry )
{
	m_storyPhaseCurrentOwner = storyPhase;
	m_storyPhaseCurrentOwner.m_communityEntitiesEntry = communityEntitiesEntry;
	m_schedule.m_isUsingLastActionPoint = IsUsingLastAP();
	m_schedule.m_storyPhase = &m_storyPhaseCurrentOwner;

	GCommonGame->GetSystem< CCommunitySystem >()->AddActiveStoryPhaseAgent( m_storyPhaseCurrentOwner );

	// copy the proper timetable
	const CSStoryPhaseTimetableEntry* timetableEntry = storyPhase.GetAgentStoryPhaseTimetableEntry();
	if ( timetableEntry != NULL )
	{
		m_schedule.m_timetable = timetableEntry->m_actionCategies;

		// assign the initial layer
		//CLayerInfo *layerInfo = NULL;
		//m_cs.FindRandomActionCategoryAndLayerName( m_schedule.m_timetable, m_schedule.m_actionCategoryName, layerInfo, m_schedule.m_actionPointTags );
		//ASSERT( (!layerInfo || m_schedule.m_actionCategoryName != CName::NONE) && TXT("CCommunitySystem::SetActionPointForAgentStub: actionCategoryName in NONE.") );
		//m_schedule.m_activeLayerInfo = layerInfo;
	}
	
	// Set the ready to work state or if we have action point lock NPC in that action point
	ChangeAgentStubState( CAS_InitAfterCreated );
}

Bool SAgentStub::ChangeSchedule( const SStoryPhaseLocation &storyPhase )
{
	if ( storyPhase.DoesReplace( m_storyPhaseCurrentOwner ) == false )
	{
		return false;
	}

	ASSERT( storyPhase.m_communityEntry != NULL );

	// Force last ap reset on every phase change, to allow last ap checks to be consistent for phases using last ap
	CNewNPC* npc = m_npc.Get();
	if( npc && m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry != storyPhase.m_communityStoryPhaseEntry )
	{
		npc->SignalGameplayEvent( CNAME( ResetLastAp ) );
	}

	GCommonGame->GetSystem< CCommunitySystem >()->MarkActiveStoryPhaseAgentsModified( m_storyPhaseCurrentOwner );
	GCommonGame->GetSystem< CCommunitySystem >()->MarkActiveStoryPhaseAgentsModified( storyPhase );

	m_storyPhaseCurrentOwner = storyPhase;
	m_storyPhaseCurrentOwner.m_communityEntry->GetRandomWeightsEntityEntry( &m_storyPhaseCurrentOwner.m_communityEntitiesEntry );	// Hack for getting entities entry for story phase
	m_schedule.m_isUsingLastActionPoint = IsUsingLastAP();
	m_schedule.m_storyPhase = &m_storyPhaseCurrentOwner;

	// copy the proper timetable
	const CSStoryPhaseTimetableEntry* timetableEntry = storyPhase.GetAgentStoryPhaseTimetableEntry();
	if ( timetableEntry != NULL )
	{
		m_schedule.m_timetable = timetableEntry->m_actionCategies;
		m_schedule.m_lastUsedTimetabEntry = NULL;
	}
    else
    {
        m_schedule.m_timetable.Clear();
        m_schedule.m_lastUsedTimetabEntry = NULL;
        if ( m_schedule.m_activeActionPointID != ActionPointBadID )
        {
			if( m_apMan )
			{
	            m_apMan->SetFree( m_schedule.m_activeActionPointID, CActionPointManager::REASON_COMMUNITY );
			}
        }
        m_schedule.m_activeActionPointID = ActionPointBadID;

        // If there's no timetable and we're not starting in AP then we must update the current layer with the one from spawn point
        if ( ! IsSpawned() && ! m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_startInAP )
        {
            ChangeAgentStubState( CAS_InitAfterCreated );
            ChangeAgentStubState( InitAfterCreated( 0.f ) );
        }
    }

	// don't change the agent's state - let the state machine catch up in its due time

	return true;
}

CSEntitiesEntry* SAgentStub::FindMatchForSteal( CSTableEntry& communityEntry )
{
	CSEntitiesEntry *communityEntitiesEntry = m_storyPhaseCurrentOwner.m_communityEntitiesEntry;
	TDynArray< CSEntitiesEntry > &entities = communityEntry.m_entities;
	const CName &agentAppearance = m_appearance;

	if ( communityEntitiesEntry == NULL )
	{
		ASSERT( (m_state == CAS_Despawning || m_state == CAS_ToDespawn ) && TXT("Agent stub with empty story phase owner should be in despawning state") );
		// If we are here, then agent stub should be in despawning state due to community component unregister (unload)
		return NULL;
	}

	for ( TDynArray< CSEntitiesEntry >::iterator entity = entities.Begin();
		entity != entities.End();
		++entity )
	{
		const Bool communityEntryMatch = m_storyPhaseCurrentOwner.m_communityEntry == &communityEntry;
		const Bool stateMatch = m_state == CAS_Despawning || m_state == CAS_ToDespawn;
		if ( !communityEntryMatch || !stateMatch ) continue; // optimization
		const Bool entityTemplateMatch = communityEntitiesEntry->m_entityTemplate == entity->m_entityTemplate;
		const Bool entityTagsMatch = TagList::MatchAll( communityEntitiesEntry->m_entitySpawnTags, entity->m_entitySpawnTags );

		if ( communityEntryMatch &&
			stateMatch &&
			entityTemplateMatch && 
			entityTagsMatch )
		{
			return &*entity;
		}
	}

	return NULL;
}

Bool SAgentStub::IsHiddenSpawn() const 
{ 
	if( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry )
	{
		return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_isHiddenSpawn;//.m_communityEntry->m_isHiddenSpawn; 
	}	
	return true;
}

ECommMapPinType SAgentStub::GetMapPinType() const 
{ 
	return m_storyPhaseCurrentOwner.m_communityEntitiesEntry->m_mappinType; 
}

CName SAgentStub::GetMapPinTag() const 
{ 
	return m_storyPhaseCurrentOwner.m_communityEntitiesEntry->m_mappinTag; 
}

Bool SAgentStub::IsNonQuestMapPinTypeSet() const 
{ 
	return m_storyPhaseCurrentOwner.m_communityEntitiesEntry->m_mappinType != CMPT_None; 
}

Bool SAgentStub::IsUsingLastAP() const
{
	if ( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry )
	{
		return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_useLastAP;
	}
	else
	{
		return false;
	}
}

CEntity* SAgentStub::GetSpawnJobEntity()
{
	if( m_jobSpawnEntity )
	{
		return m_jobSpawnEntity->GetSpawnedEntity( 0 );
	}

	return NULL;
}

Bool SAgentStub::IsStartingInAP() const 
{ 
	return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_startInAP; 
}

Bool SAgentStub::IsAlwaysSpawned() const
{
	if ( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry )
	{
		return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_alwaysSpawned;
	}
	else
	{
		return false;
	}
}

void SAgentStub::GetDebugInfo( ICommunityDebugPage& debugPage ) const
{
#ifdef COMMUNITY_DEBUG_STUBS

	debugPage.AddText( TXT("Spawnset: "), Color::YELLOW );
	debugPage.AddText( m_storyPhaseCurrentOwner.m_community->GetFriendlyName(), Color::WHITE );

	debugPage.AddText( TXT("Appearance: "), Color::YELLOW );
	debugPage.AddText( m_appearance.AsString(), Color::WHITE );

	debugPage.AddText( TXT("Visibility state: "), Color::YELLOW );
	switch ( m_debugInfo.m_visibilityState )
	{
	case SAgentStubDebugInfo::VS_NoPlayer:
		debugPage.AddText( TXT("No player."), Color::WHITE );
		break;
	case SAgentStubDebugInfo::VS_NoValidSpawnPos:
		debugPage.AddText( TXT("No valid spawn position."), Color::RED );
		break;
	case SAgentStubDebugInfo::VS_VisibilityArea:
		debugPage.AddText( TXT("Isn't in visibility area."), Color::BLUE );
		break;
	case SAgentStubDebugInfo::VS_TooFarFromPlayer:
		debugPage.AddText( TXT("Is too far from player."), Color::BLUE );
		break;
	case SAgentStubDebugInfo::VS_WorkingOnUnloadedLayer:
		debugPage.AddText( TXT("Is working on unloaded layer."), Color::BLUE );
		break;
	case SAgentStubDebugInfo::VS_Dead:
		debugPage.AddText( TXT("Is dead."), Color::CYAN );
		break;
	case SAgentStubDebugInfo::VS_Despawning:
		debugPage.AddText( TXT("Is in despawning state."), Color::GREEN );
		break;
	case SAgentStubDebugInfo::VS_IsNpcAlready:
		debugPage.AddText( TXT("Is already spawned as NPC."), Color::GREEN );
		break;
	case SAgentStubDebugInfo::VS_NoPathEngineWorld:
		debugPage.AddText( TXT("There is no pathengine world."), Color::RED);
		break;
	case SAgentStubDebugInfo::VS_NoWorld:
		debugPage.AddText( TXT("There is no world."), Color::RED );
		break;
	case SAgentStubDebugInfo::VS_ToSpawn:
		debugPage.AddText( TXT("Is ready to spawn."), Color::GREEN );
		break;
	case SAgentStubDebugInfo::VS_NotStreamed:
		debugPage.AddText( TXT("Not streamed."), Color::WHITE );
		break;
	default:
		debugPage.AddText( TXT("Unknown visibility state"), Color::RED );
		break;
	}

	debugPage.AddText( TXT("IsAlwaysSpawned: "), Color::YELLOW );
	debugPage.AddText( IsAlwaysSpawned() ? TXT("true") : TXT("false"), Color::WHITE );

	// streaming layer
	debugPage.AddText( String( TXT("Streaming partition name: ") ) + m_debugInfo.m_partitionName, Color::YELLOW );

	if ( m_communityAgent.IsEnabled() )
	{
		Vector worldSpacePos = Vector( m_communityAgent.GetPosition() );
		debugPage.AddText( String( TXT("Stub world pos: ") ) + ToString( worldSpacePos ), Color::WHITE );
		debugPage.FocusOn( worldSpacePos );
	}

	// action points
	debugPage.AddText( TXT("Current AP component ID: ") + ToString( m_schedule.m_activeActionPointID ), Color::WHITE );
	debugPage.AddText( TXT("Current action category name: ") + m_schedule.m_actionCategoryName.AsString(), Color::WHITE );

#endif
}

void SAgentStub::SaveState( IGameSaver* writer )
{
	CGameSaverBlock block( writer, CNAME(a) );

	// Save the owner
	m_storyPhaseCurrentOwner.SaveState( writer );

	// Save the Id Tag
	writer->WriteValue( CNAME(idTag), m_idTag );
	writer->WriteValue( CNAME( ownerWorldHash ), m_ownerWorldHash );

	// Save the placement
	CNewNPC *npc = m_npc.Get();

	Vector worldSpacePos;
	Float agentYaw;
	Bool hasAgent = true;
	CMovingAgentComponent* mac = npc ? npc->GetMovingAgentComponent() : NULL;
	if ( mac )
	{
		worldSpacePos = mac->GetAgentPosition();
		agentYaw = mac->GetWorldYaw();
	}
	else if ( m_communityAgent.IsEnabled() )
	{
		worldSpacePos = Vector( m_communityAgent.GetPosition() );
		agentYaw = m_agentYaw;
	}
	else
	{
		hasAgent = false;
	}
	writer->WriteValue( CNAME(hasAgent), hasAgent );
	if ( hasAgent )
	{
		writer->WriteValue( CNAME(position), worldSpacePos );
		writer->WriteValue( CNAME(rotation), agentYaw );
	}

	writer->WriteValue( CNAME( teleportToDestination ), m_teleportToDestination );

	// save AP IDs
	THandle< CActionPointManager > apMgr = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	apMgr->SaveApId( writer, CNAME( currentAPComponentID ), m_schedule.m_activeActionPointID );
	apMgr->SaveApId( writer, CNAME( lastAPComponentID ), m_schedule.m_lastActionPointID );

	// Save less important stub params
	writer->WriteValue( CNAME(actionCategoryName), m_schedule.m_actionCategoryName );
	writer->WriteValue( CNAME(processingTimer), m_processingTimer );
	writer->WriteValue( CNAME(appearance), m_appearance );
	Int32 state = m_state;
	writer->WriteValue( CNAME(state), state );		
}

SAgentStub* SAgentStub::RestoreState( IGameLoader* reader, CCommunitySystem* community, const SActiveStoryPhaseLocation::List& phasesLookup, const GameTime& currGameTime  )
{
	CGameSaverBlock block( reader, CNAME(a) );

	// Find the owner story phase
	SStoryPhaseLocation owner;
	if ( !SActiveStoryPhaseLocation::RestoreState( owner, reader, phasesLookup ) )
	{
		// Stub not restored
		WARN_GAME( TXT("Unable to restore stub from save game - no story phase owner found.") );
		return NULL;
	}

	// Load stub spawn data
	StubSpawnData spawnData;
	reader->ReadValue( CNAME(idTag), spawnData.m_suggestedTag );
	Uint32 ownerWorldHash = 0;
	if ( reader->GetSaveVersion() >= SAVE_VERSION_STUB_KEEPS_WORLD_HASH )
	{
		reader->ReadValue( CNAME( ownerWorldHash ), ownerWorldHash );
	}
	if( ownerWorldHash == 0 )
	{
		ownerWorldHash = community->GetCurrentWorldHash();
	}

	Bool worldMatch = community->IfWorldHashMatch( ownerWorldHash );

	Bool hasAgent = false;
	reader->ReadValue( CNAME(hasAgent), hasAgent );
	if ( hasAgent )
	{
		reader->ReadValue( CNAME(position), spawnData.m_position );
		reader->ReadValue( CNAME(rotation), spawnData.m_rotation );
	}

	Bool teleportToDestination = true;
	reader->ReadValue( CNAME( teleportToDestination ), teleportToDestination );

	// Create agent stub	
	SAgentStub* stub = community->CreateAgentStubAfterLoad( owner, spawnData, worldMatch );
	if ( !stub )
	{
		WARN_GAME( TXT("Unable to restore stub from save game - stub not spawned.") );
		return NULL;
	}

	// we have a position - place the agent
	if ( hasAgent )
	{
		stub->m_communityAgent.SetEnabled( true );
		stub->PlaceCommunityAgent( spawnData.m_position );
		if ( !stub->m_communityAgent.IsEnabled() )
		{
			// ups - failed to recreate the agent's state
			delete stub;
			return NULL;
		}
	}
	else
	{
		stub->m_communityAgent.SetEnabled( false );
	}

	stub->m_teleportToDestination = teleportToDestination;

	// Set stub owner
	stub->SetStoryPhaseOwner( owner, owner.m_communityEntitiesEntry );

	// set the last used timetable to the one currently in use to avoid
	// AP acquisition on startup
	stub->m_schedule.m_lastUsedTimetabEntry = CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( stub->m_schedule.m_timetable, currGameTime );

	// load AP IDs
	THandle< CActionPointManager > apMgr = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	stub->m_schedule.m_activeActionPointID = apMgr->LoadApId( reader, CNAME( currentAPComponentID ) );
	stub->m_schedule.m_lastActionPointID = apMgr->LoadApId( reader, CNAME( lastAPComponentID ) );
    if ( reader->GetSaveVersion() < SAVE_NO_AP_SEQUENCE )
    {
        apMgr->LoadApId( reader, CNAME( currentAPComponentInSeqID ) );
    }
    
	// immediately lock the AP
	if ( owner.IsWorldValid() && stub->m_schedule.m_activeActionPointID != ActionPointBadID && apMgr->IsFree( stub->m_schedule.m_activeActionPointID, stub->m_npc.Get() ) )
	{
		apMgr->SetReserved( stub->m_schedule.m_activeActionPointID, CActionPointManager::REASON_COMMUNITY, stub->m_npc.Get() );
	}

	// Load less important stub params
	reader->ReadValue( CNAME(actionCategoryName), stub->m_schedule.m_actionCategoryName );
	reader->ReadValue( CNAME(processingTimer), stub->m_processingTimer );
	reader->ReadValue( CNAME(appearance), stub->m_appearance );

	// Check if NPC was working in AP and stub should start in AP too
	Int32 state;
	reader->ReadValue( CNAME(state), state );
	if( state > 0 )
	{
		stub->ChangeAgentStubState( (ECommunityAgentState)state );

		// Was spawned during save, so force spawning now.
		if ( state == CAS_Spawned )
		{
			stub->SetForceSpawn();
			stub->SetLOD( ALOD_CloseDistance );
		}
	}
	stub->m_ownerWorldHash = ownerWorldHash;
	// Return created stub
	return stub;
}

// ------------------------------------------------------------------------
// IAgent implementation
// ------------------------------------------------------------------------

const Vector& SAgentStub::GetWorldPositionRef() const
{
	CNewNPC* npc = m_npc.Get();
	if ( npc )
	{
		return npc->GetWorldPositionRef();
	}
	else if ( m_communityAgent.IsEnabled() )
	{
		m_tmpWorldPos = Vector( m_communityAgent.GetPosition() );
		return m_tmpWorldPos;
	}
	else
	{
		static Vector noPos( Vector::ZEROS );
		return noPos;
	}
}

Bool SAgentStub::OnLODChanged( EAgentLOD newLod )
{
	if ( m_disabled )
	{
		return false;
	}
	EAgentSpawnResult result = ASR_Loading;
	if ( newLod > ALOD_Invisible )
	{
		// spawn the agent
		result = SpawnAgentNPC();
		if ( result == ASR_Spawned )
		{
			// Stub is spawned
			CNewNPC* npc = m_npc.Get();
			ASSERT( npc != NULL );
			
			ChangeAgentStubState( CAS_Spawned );
			npc->SetWorkSchedule( m_schedule );

			// Set despawn point tags
			npc->SetDespawnTags( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_despawnPointTags );
		}
	}
	else
	{
		DespawnAgentNPC();
	}

	return result != ASR_Failed;
}

const ISpawnStrategy& SAgentStub::GetSpawnStrategy() const
{
	const ISpawnStrategy* strategy = m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->GetSpawnStrategy();
	if ( !strategy )
	{
		strategy = m_cs.GetDefaultSpawnStrategy();
	}

	ASSERT( strategy );
	return *strategy;
}

// ----------------------------------------------------------------------------
// LOD management
// ----------------------------------------------------------------------------

void SAgentStub::HACKSetStartInAPAndHide()
{
	CNewNPC* npc = m_npc.Get();
	if ( npc )
	{
		npc->SetIsStartingInAP( true ) ;
		npc->SuspendRendering( true );
		m_hiddenOnStartTicksLeftHACK = 5;
	}
}

void SAgentStub::CalculateSpawnPosition()
{
	Vector agentWorldPos = Vector( m_communityAgent.GetPosition() );
	// Try to place the NPC where the agent is now, this saves a lot of cycles		
	if ( !m_communityAgent.TestLocation( agentWorldPos.AsVector3() ) )
	{
		Vector3 newPosition;
		if ( !m_communityAgent.FindSafeSpot( agentWorldPos.AsVector3(), CCommunityConstants::SPAWN_POINT_RADIUS, agentWorldPos.AsVector3() ) )
		{
			WARN_GAME( TXT("Cannot spawn NPC from agent stub - no unobstracted position found." ) );
			SET_STUB_DEBUG_INFO( this , m_visibilityState, SAgentStubDebugInfo::VS_NoValidSpawnPos );
			return;
		}
	}

	m_spawnPos = agentWorldPos;
}

SAgentStub::EAgentSpawnResult SAgentStub::SpawnAgentNPC( Bool asyncLoad )
{
#ifdef PROFILE_COMMUNITY
	PC_SCOPE_PIX( SpawnAgent );
#endif
	// We do have a spawn job pending
	if ( m_jobSpawnEntity )
	{
		m_cs.MarkAsCurrentlySpawning();

		if ( m_jobSpawnEntity->HasEnded() && m_jobSpawnEntity->AreEntitiesReady()  && !m_cs.IsNPCAttachmentPerformedThisFrame() )
		{
			if ( m_jobSpawnEntity->HasFinishedWithoutErrors() )
			{
				// Get the spawned NPC from the job
				m_jobSpawnEntity->LinkEntities();
				m_cs.SetNPCAttachmentPerformedThisFrame( true );
				CEntity* entity = m_jobSpawnEntity->GetSpawnedEntity( 0 );
				if ( !entity )
				{
					// Release the job
					m_jobSpawnEntity->Release();
					m_jobSpawnEntity = NULL;

					// Report error
					ERR_GAME( TXT("Cannot spawn NPC from agent stub: No entity created") );
					return ASR_Failed;
				}

#ifndef NO_EDITOR
				GCommonGame->GetEntityPool()->RegisterCommunityEntity( entity );
#endif

				// Spawn entity, should be CNewNPC
				CNewNPC *const npc = Cast< CNewNPC >( entity );
				if ( !npc )
				{
					// Release the job
					m_jobSpawnEntity->Release();
					m_jobSpawnEntity = NULL;

					// Destroy the entity
					entity->Destroy();

					// Report error
					ERR_GAME( TXT("Cannot spawn NPC from agent stub: Created entity is not an NPC") );
					return ASR_Failed;
				}

				npc->RegisterTerminationListener( true, & m_cs );
				m_npc = npc;
				
				// HACK: Don't show npc for the first few frames
				if ( IsStartingInAP() && !m_forceSpawn )
				{
					HACKSetStartInAPAndHide();
				}
			}

			// Release the job
			m_jobSpawnEntity->Release();
			m_jobSpawnEntity = NULL;
		}

		// We are still processing the spawn job...
		Bool ret = m_npc.Get() != NULL;
        if ( ret )
        {
            m_forceSpawn = false;
			m_teleportToDestination = false;

			return ASR_Spawned;
        }

        return ASR_Loading;
	}
	
	CalculateSpawnPosition();
	Vector worldSpawnPosition = m_spawnPos;

	// Spawn NPCs only behind camera if spawnset entry requires that
	if ( IsHiddenSpawn() && !ForceSpawn() )
	{
		ASSERT( GGame->GetActiveWorld() );
		Float distSqFromPlayer = GCommonGame->GetPlayer()->GetWorldPositionRef().DistanceSquaredTo( worldSpawnPosition );
		const Bool isSpawnPointInCamera = GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( worldSpawnPosition, 1.1f ) && !GGame->IsBlackscreen();
		const Bool isNearPlayer = distSqFromPlayer < 1.0f;
		const Bool isFarFromPlayer = distSqFromPlayer > (CCommunityConstants::VISIBILITY_SPAWN_RADIUS * CCommunityConstants::VISIBILITY_SPAWN_RADIUS);
		if ( !isFarFromPlayer && (isSpawnPointInCamera || isNearPlayer) )
		{
			return ASR_Loading;
		}
	}

    CSEntitiesEntry* communityEntitiesEntry = m_storyPhaseCurrentOwner.m_communityEntitiesEntry;
	if( communityEntitiesEntry == nullptr )
	{
		m_disabled = true;
		return ASR_Failed;
	}

	if ( communityEntitiesEntry->m_entityTemplate.IsEmpty() )
	{
		HALT( "COMMUNITY '%ls' HAS ENTRIES WITHOUT TEMPLATE SPECIFIED!", m_storyPhaseCurrentOwner.m_community->GetDepotPath().AsChar() );
		m_disabled = true;
		return ASR_Failed;
	}

	// Preload entity template
    if ( asyncLoad )
    {
	    const BaseSoftHandle::EAsyncLoadingResult loadingResult = communityEntitiesEntry->m_entityTemplate.GetAsync( true );
	    if ( loadingResult != BaseSoftHandle::ALR_Loaded )
	    {
			// Terminate on failure
			if( loadingResult == BaseSoftHandle::ALR_Failed)
			{
				HALT( "COMMUNITY ENTRY INVALID TEMPLATE '%ls'! LOADING FAILED!", communityEntitiesEntry->m_entityTemplate.GetPath().AsChar() );
				m_disabled = true;
				return ASR_Failed;
			}

			m_cs.MarkAsCurrentlySpawning();
		    // Template not yet loaded or invalid
		    return ASR_Loading;
	    }
    }

	// Get the template to use
	CEntityTemplate* entityTemplate = communityEntitiesEntry->m_entityTemplate.Get();
	if ( entityTemplate )
	{
		// Do not let entities other than CNewNpc get spawned by the community!
		if( !entityTemplate->GetTemplateInstance()->IsA< CNewNPC>() )
		{
			HALT( "COMMUNITY ENTRY HAS TEMPLATE '%ls' THAT IS NOT A 'CNewNpc'!", entityTemplate->GetFile()->GetDepotPath().AsChar() );
			m_disabled = true;
			return ASR_Failed;
		}
	}
	else
	{
		// Template was not loaded
		return ASR_Loading;
	}

	// When not spawning in AP we need a valid placement
	if ( !IsStartingInAP() )
	{	
#ifdef PROFILE_COMMUNITY
		PC_SCOPE_PIX( GetSafePlacement );
#endif

		if ( !m_communityAgent.FindSafeSpot( worldSpawnPosition.AsVector3(), 3.f, worldSpawnPosition.AsVector3() ) )
		{	
			ERR_GAME( TXT("Cannot spawn NPC from agent stub: cannot find safe position!") );
			return ASR_Failed;
		}
	}

	// Fill spawn info structure
	EntitySpawnInfo spawnInfo;
	spawnInfo.m_spawnPosition = worldSpawnPosition;
	spawnInfo.m_tags = communityEntitiesEntry->m_entitySpawnTags;
	spawnInfo.m_spawnRotation = EulerAngles( 0.0f, 0.0f, m_agentYaw );
	spawnInfo.m_template = entityTemplate;
	spawnInfo.m_entityFlags = EF_DestroyableFromScript;
	spawnInfo.m_idTag = m_idTag;
	spawnInfo.m_canThrowOutUnusedComponents = !IsAlwaysSpawned();
	spawnInfo.m_entityNotSavable = false;
	spawnInfo.m_importantEntity = true;
	
	ISpawnTreeInitializerAI* initializer = GCommonGame->GetCommunitySystem()->GetCommunitySpawnInitializer();
	if( initializer )
	{
		initializer->OnCreatureSpawn( spawnInfo, nullptr, nullptr );
	}

	// Set appearance to use
	if ( m_appearance == CName::NONE )
	{
		communityEntitiesEntry->GetRandomAppearance( &m_appearance );
	}
	if ( m_appearance != CName::NONE )
	{
		spawnInfo.m_appearances.PushBack( m_appearance );
	}

    // Create entity
    if ( asyncLoad )
    {
        // Create spawn job
		m_jobSpawnEntity = GCommonGame->GetEntityPool()->SpawnEntity( std::move( spawnInfo ), CEntityPool::SpawnType_Community );

        communityEntitiesEntry->m_entityTemplate.Release();

		m_cs.MarkAsCurrentlySpawning();

        // Not yet spawned
        return ASR_Loading;
    }
    else
    {

        CEntity* entity = GCommonGame->GetEntityPool()->SpawnEntity_Sync( spawnInfo, CEntityPool::SpawnType_Community );

        // No entity was created
        if ( !entity )
        {
            //SCSErrRep::GetInstance().LogSpawn( TXT("Cannot spawn NPC from agent stub: No entity was spawned"), m_storyPhaseCurrentOwner );
			m_disabled = true;
            return ASR_Failed;
        }

        // Spawn entity, should be CNewNPC
		ASSERT( entity->IsA< CNewNPC >(), TXT("Pre spawning tests should make double check that spawn job is indeed spawning NPCs") );
        CNewNPC* newNPC = Cast< CNewNPC >( entity );
        if ( newNPC == NULL )
        {
            entity->Destroy();
			m_disabled = true;
            //SCSErrRep::GetInstance().LogSpawn( TXT("Cannot spawn NPC from agent stub: Created entity is not an NPC"), m_storyPhaseCurrentOwner );
            return ASR_Failed;
		}
		m_npc = newNPC;

		SetLOD( ALOD_CloseDistance );

        // Set despawn point tags
        ChangeAgentStubState( CAS_Spawned );
        newNPC->SetWorkSchedule( m_schedule );

		// HACK: Don't show npc for the first few frames
		if ( IsStartingInAP() && !m_forceSpawn )
		{
			HACKSetStartInAPAndHide();
		}	

        // Set despawn point tags
        newNPC->SetDespawnTags( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_despawnPointTags );

        communityEntitiesEntry->m_entityTemplate.Release();

        // Spawned
        m_forceSpawn = false;
		m_teleportToDestination = false;

		m_cs.MarkAsCurrentlySpawning();

        return ASR_Spawned;
    }
}

Bool SAgentStub::DespawnAgentNPC()
{
	ASSERT( !IsOnlyStub() && TXT("Community system: empty NPC in agent stub on despawn.") );

	// Cancel spawning
	if ( m_jobSpawnEntity )
	{
		m_jobSpawnEntity->Cancel();
		m_jobSpawnEntity->Release();
		m_jobSpawnEntity = NULL;
	}
	
	CSEntitiesEntry* communityEntitiesEntry = m_storyPhaseCurrentOwner.m_communityEntitiesEntry;
	if ( communityEntitiesEntry )
	{
		communityEntitiesEntry->m_entityTemplate.Release();
	}

	// Process the NPC
	CNewNPC *npc = m_npc.Get();
	if ( npc )
	{
		// Log when destroying NPC that is locked by quest
		if ( npc->IsLockedByQuests() )
		{
			WARN_GAME( TXT("NPC %s is locked by quests and is despawned!"), npc->GetFriendlyName().AsChar() );
		}

		Vector destinationPosition = Vector::ZEROS;
		Float destinationRotation;
		
		if( m_apMan->GetActionExecutionPosition( m_schedule.m_activeActionPointID, &destinationPosition, &destinationRotation ) )
		{
			m_communityAgent.SetPosition( destinationPosition );
			m_agentYaw = destinationRotation;
		}
		else
		{
			// Update agent stub world position
			CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
			CPathAgent* pathAgent = mac ? mac->GetPathAgent() : NULL;
			if ( pathAgent )
			{
				m_communityAgent.SetPosition( pathAgent->GetPosition() );
				m_agentYaw = npc->GetWorldYaw();
			}
		}
		// Start despawning
		if ( IsForceDespawned() )
		{
			npc->EnterForcedDespawn();
			m_cs.SetWasDetachmentPerformedThisFrame( true );
			m_teleportToDestination = true;
		}
		return false;
	}

	return true;
}

void SAgentStub::PlaceCommunityAgent( const Vector& pos ) const
{
	Vector3 safePos = pos;
	m_communityAgent.FindSafeSpot( safePos, 4.f, safePos );
	m_communityAgent.SetPosition( safePos );
	m_communityAgent.SetEnabled( true );
}

void SAgentStub::DespawnAgent( Bool isForceDespawn /* = false */ )
{
	if( CNewNPC* npc = m_npc.Get() )
	{
		ASSERT( !npc->IsInNonGameplayScene() );
	}

	ChangeAgentStubState( CAS_Despawning );
}

void SAgentStub::DespawnAgentStub()
{
	ChangeAgentStubState( CAS_ToDespawn );
}

void SAgentStub::Tick( Float timeDelta, const GameTime& currGameTime )
{
	
	CNewNPC* spawnedNPC = m_npc.Get();
	if ( spawnedNPC && spawnedNPC->IsAttached() )
	{
		if ( m_hiddenOnStartTicksLeftHACK > 0 )
		{
			//LOG_GAME( TXT("================ %i"), m_hiddenOnStartTicksLeftHACK );
			if ( --m_hiddenOnStartTicksLeftHACK == 0 )
			{
				//LOG_GAME( TXT("================ go!"));
				spawnedNPC->SuspendRendering( false );
			}
		}
	}
	
	// Check if time in timetable hasn't changed, so NPC should break work
	const CSStoryPhaseTimetableACategoriesTimetableEntry* currTimetableEntry = CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( m_schedule.m_timetable, currGameTime );
	if ( m_schedule.m_lastUsedTimetabEntry != currTimetableEntry )
	{
		m_schedule.m_lastUsedTimetabEntry = currTimetableEntry;
		OnTimetableChanged();
	}

    Bool frameProcessed = false;

	// Agent stub is spawned
	ECommunityAgentState nextState = m_state;
	do 
	{
		ChangeAgentStubState( nextState );

        if ( frameProcessed )
        {
            break;
        }

		switch( m_state )
		{
		case CAS_InitAfterCreated:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( InitAfterCreated );
#endif
				nextState = InitAfterCreated( timeDelta );
                frameProcessed = true;
				break;
			}

        case CAS_WaitingToSpawnInSpawnPoint:
            {
#ifdef PROFILE_COMMUNITY
                PC_SCOPE_PIX( WaitingToSpawnInSpawnPoint );
#endif
                nextState = WaitingToSpawnInSpawnPoint( timeDelta );
                break;
            }

		case CAS_AcquireNextAP:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( SetActionPointForAgentStub );
#endif
				nextState = SetActionPointForAgentStub();
				break;
			}

		case CAS_ReadyToWork:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( ProcessReadyToWork );
#endif
				nextState = ProcessReadyToWork();
				break;
			}

		case CAS_NoAPFound:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( ProcessAgentStubWithNoActionPointFound );
#endif
				nextState = ProcessAgentStubWithNoActionPointFound( timeDelta );
                frameProcessed = true;
				break;
			}

		case CAS_MovingToActionPoint:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( ProcessMovingToActionPointAgentStub );
#endif
				nextState = ProcessMovingToActionPointAgentStub( timeDelta );
                frameProcessed = true;
				break;
			}

		case CAS_WorkInProgress:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( ProcessWorkInProgressAgentStub );
#endif
				nextState = ProcessWorkInProgressAgentStub( timeDelta );
                frameProcessed = true;
				break;
			}

		case CAS_Spawned:
			{
#ifdef PROFILE_COMMUNITY
				PC_SCOPE_PIX( ProcessSpawnedAgentStub );
#endif

				if ( IsOnlyStub() )
				{
					if ( m_schedule.m_activeActionPointID != ActionPointBadID && IsActionPointLoaded() == false )
					{
						// if the layer we were working on was unloaded, just keep on working there
						m_processingTimer = 10.0f;
						nextState = CAS_WorkInProgress;
					}
					else
					{
                        if ( IsStartingInAP() )
                        {
						    nextState = CAS_AcquireNextAP;
                        }
                        else
                        {
                            nextState = CAS_WaitingToSpawnInSpawnPoint;
                        }
					}
				}
				else
				{
					ProcessSpawnedAgentStub();
				}
				
				// the state can't be changed from inside of it
				break;
			}

		case CAS_Despawning:
			{
				if ( IsOnlyStub() )
				{
					// the agent's LOD changed - despawn the stub
					nextState = CAS_ToDespawn;
				}
				break;
			}

		case CAS_ToDespawn:
			{
				// nothing to do here - just wait until the stub gets destroyed
				break;
			}
		}
		if( nextState != m_state )
		{
			m_cs.MarkAsCurrentlySpawning();
		}
	}
	while( nextState != m_state );
}

void SAgentStub::OnTimetableChanged()
{
	CNewNPC* npc = m_npc.Get();

	m_schedule.AquireNextAP( npc, true );

	// Update agent position if there's no NPC (if there's valid NPC it will get updated in tick)
	if ( !npc )
	{
		CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
		CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();

		if ( CActionPointComponent* ap = actionPointManager->GetAP( m_schedule.m_activeActionPointID ) )
		{
			m_communityAgent.SetPosition( ap->GetWorldPositionRef() );
		}
	}

	if ( m_communityAgent.IsEnabled() )
	{
		CNewNPC* npc = m_npc.Get();
		if ( npc )
		{
			ASSERT( m_state == CAS_Spawned );
			npc->SignalGameplayEvent( CNAME( OnTimetableChanged ) );
		}
		else
		{
			// the agent is a stub
			m_state = CAS_ReadyToWork;
		}
	}
}

Bool SAgentStub::FindDespawnPoint( Vector &despawnPoint /* out */ )
{
	// First try to find despawn point at agent stub house
	const CSStoryPhaseTimetableEntry *storyPhaseTimetableEntry = m_storyPhaseCurrentOwner.GetAgentStoryPhaseTimetableEntry();
	const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > *actionCategories = storyPhaseTimetableEntry ? &storyPhaseTimetableEntry->m_actionCategies : NULL;

	// Get the matching despawn tags
	const TagList &despawnTags = m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_despawnPointTags;
	return m_cs.FindDespawnPoint( actionCategories, despawnTags, despawnPoint );
}

void SAgentStub::ChangeAgentStubState( ECommunityAgentState newState )
{
	switch ( newState )
	{
	default:
	case CAS_InitAfterCreated:
		m_processingTimer = 0.f;
	case CAS_WaitingToSpawnInSpawnPoint:
	case CAS_AcquireNextAP:
	case CAS_Spawned:
	case CAS_MovingToActionPoint:
	case CAS_WorkInProgress:
	case CAS_NoAPFound:
	case CAS_ReadyToWork:
		break;
	case CAS_Despawning:
	case CAS_ToDespawn:
		if ( m_state != CAS_Despawning && m_state != CAS_ToDespawn )
		{
			CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
			if ( communitySystem )				// currently null is possible on world destruction
			{
				communitySystem->DelActiveStoryPhaseAgent( m_storyPhaseCurrentOwner );
			}
		}
		break;
	}
	m_state = newState;
}

void SAgentStub::ProcessSpawnedAgentStub()
{
	ASSERT( !IsOnlyStub() );

	// Copy world position from spawned NPC
	CNewNPC *npc = m_npc.Get();
	if ( !npc )
	{
		return;
	}

	CMovingAgentComponent* mac = npc->GetMovingAgentComponent();

	if ( mac )
	{
		CPathAgent* pathAgent = mac->GetPathAgent();
		if ( pathAgent )
		{
			m_communityAgent.SetPosition( pathAgent->GetPosition() );			
		}
	}

#if 0 // GFx 3
	// update the map pin position
	if ( m_mapPinId != -1 && GWitcherGame->GetHudInstance() )
	{
		GWitcherGame->GetHudInstance()->UpdateDynamicMapPin( m_mapPinId,npc->GetWorldPositionRef() );
	}
#endif // #if 0
}

void SAgentStub::AcquireNextAP()
{
	m_schedule.AquireNextAP( m_npc.Get() );
}

ECommunityAgentState SAgentStub::InitAfterCreated( Float deltaTime )
{
	m_processingTimer -= deltaTime;
	if ( m_processingTimer > 0.0f )
	{
		return CAS_InitAfterCreated;
	}

	Vector apPos( 0, 0, 0 );
	if ( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_startInAP )
	{
		m_schedule.AquireNextAP( m_npc.Get() );

		// Create agent here, at the action point
		if ( m_apMan && ( m_apMan->GetGoToPosition( m_schedule.m_activeActionPointID, &apPos, &m_agentYaw ) == false ) )
		{
			m_processingTimer = 1.0f + GEngine->GetRandomNumberGenerator().Get< Float >();
			return CAS_InitAfterCreated;
		}
	}
	else
	{
		CLayerInfo* layerInfo = NULL;
		if ( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->AllocateSpawnPoint( apPos, m_agentYaw, layerInfo ) == false )
		{
			//WARN_GAME( TXT("Cannot spawn agent stub. No spawn point available.") );
			m_processingTimer = 1.0f + GEngine->GetRandomNumberGenerator().Get< Float >();
			return CAS_InitAfterCreated;
		}

		// use the layer of this spawn point if we don't have a timetable set - otherwise
		// keep using the layers from the timetable
		/*if ( m_schedule.m_timetable.Empty() )
		{
			m_schedule.m_activeLayerInfo = layerInfo;
		}*/
	}

	// we have a position - place the agent
	PlaceCommunityAgent( apPos );
	
    if ( m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_startInAP )
    {
        return CAS_ReadyToWork;
    }
    else
    {
        return CAS_WaitingToSpawnInSpawnPoint;
    }
}

ECommunityAgentState SAgentStub::WaitingToSpawnInSpawnPoint( Float deltaTime )
{
    ASSERT( m_state == CAS_WaitingToSpawnInSpawnPoint );

    m_processingTimer -= deltaTime;
    if ( m_processingTimer > 0.0f )
    {
        return CAS_WaitingToSpawnInSpawnPoint;
    }

    if ( !m_communityAgent.TestLocation( m_spawnPos ) )
    {
        InitAfterCreated( 0.f );
        m_processingTimer = 1.f + GEngine->GetRandomNumberGenerator().Get< Float >();    
    }

    return CAS_WaitingToSpawnInSpawnPoint;
}

ECommunityAgentState SAgentStub::SetActionPointForAgentStub()
{
	ASSERT( m_state == CAS_AcquireNextAP );

	m_schedule.AquireNextAP( m_npc.Get() );
	return CAS_ReadyToWork;
}

ECommunityAgentState SAgentStub::ProcessReadyToWork()
{
	ASSERT( m_state == CAS_ReadyToWork );

	// if we have an action point, we can change the state
	if ( m_schedule.m_activeActionPointID != ActionPointBadID )
	{
		// reserve the action point
		return CAS_MovingToActionPoint;
	}
	else
	{
		m_processingTimer = 1.0f;
		return CAS_NoAPFound;
	}
}

ECommunityAgentState SAgentStub::ProcessMovingToActionPointAgentStub( Float timeDelta )
{
	ASSERT( m_state == CAS_MovingToActionPoint );

	// teleport to the destination

	if ( !m_teleportToDestination )
	{
		return CAS_MovingToActionPoint;
	}

	Vector goToWorldPos;
	if ( m_apMan && ( m_apMan->GetGoToPosition( m_schedule.m_activeActionPointID, &goToWorldPos, NULL ) == false ) )
	{
		// the action point is apparently unloaded - keep trying until it gets loaded
		return CAS_MovingToActionPoint;
	}

	Vector camToAPDir = goToWorldPos - GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

	Bool isAPUnobstructed = m_communityAgent.TestLocation( goToWorldPos.AsVector3() );

	if ( isAPUnobstructed )
	{
		// no collision - we can safely teleport the agent there
		m_communityAgent.MoveTo( goToWorldPos.AsVector3() );

		// update the mappin position
		if ( m_mapPinId != -1 )
		{
			if ( m_communityAgent.TestCurrentPosition() )
			{
#if 0 // GFx 3
				Vector worldPos = CPathEngine::PositionToVector( CPathEngineWorld::GetInstance().GetWorldMesh(), pos );
				GWitcherGame->GetHudInstance()->UpdateDynamicMapPin( m_mapPinId, worldPos );
#endif // #if 0
			}
		}

		// change the state
		m_processingTimer = 10.0f;
		return CAS_WorkInProgress;
	}
	else
	{
		return CAS_MovingToActionPoint;
	}
}

ECommunityAgentState SAgentStub::ProcessAgentStubWithNoActionPointFound( Float timeDelta )
{
	ASSERT( m_state == CAS_NoAPFound );

	m_processingTimer -= timeDelta;
	if ( m_processingTimer <= 0 )
	{
		m_processingTimer = 0;
		return CAS_AcquireNextAP;
	}
	else
	{
		return CAS_NoAPFound;
	}
}

ECommunityAgentState SAgentStub::ProcessWorkInProgressAgentStub( Float timeDelta )
{
	ASSERT( m_state == CAS_WorkInProgress );
	
	if ( IsActionPointLoaded() )
	{
		m_processingTimer -= timeDelta;
	}

	if ( m_processingTimer <= 0 )
	{
		m_processingTimer = 0;
		return CAS_AcquireNextAP;
	}
	else
	{
		return CAS_WorkInProgress;
	}
}

Bool SAgentStub::CheckAgentStubApTimetableMatch()
{
	// agent stub is not working
	if ( m_schedule.m_activeActionPointID == ActionPointBadID )
	{
		return false;
	}

	Bool doesTimetableMachCurrentAP = false;

	// get current timetable for stub
	const CSStoryPhaseTimetableEntry *storyPhaseTimetableEntry = m_storyPhaseCurrentOwner.GetAgentStoryPhaseTimetableEntry();
	if ( storyPhaseTimetableEntry )
	{
		const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > &timetable = storyPhaseTimetableEntry->m_actionCategies;
		doesTimetableMachCurrentAP = DoesAPMatchTimetable( timetable, m_schedule.m_activeActionPointID );
	}

	return doesTimetableMachCurrentAP;
}

Bool SAgentStub::DoesAPMatchTimetable( const TDynArray< CSStoryPhaseTimetableACategoriesTimetableEntry > &timetable, const TActionPointID apID, CName *matchCategory /* = NULL */ /* out */ )
{
	if ( apID == ActionPointBadID )
	{
		return false;
	}

	const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry = CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >( timetable, m_timeMgr.GetTime() );
	if ( timetabEntry == NULL )
	{
		return false;
	}

	if( !m_apMan )
	{
		return false;
	}

	// Get layer full path name that AP is placed on
	String tmpPath;
	CLayer *apLayer = GGame->GetActiveWorld()->FindLayer( m_apMan->GetLayerGUID( apID ) );
	if ( apLayer == NULL || apLayer->GetLayerInfo() == NULL ) return false;
	CLayerInfo* apLayerInfo = apLayer->GetLayerInfo();
	//apLayer->GetLayerInfo()->GetHierarchyPath( tmpPath, true );
	//CName apLayerPath( tmpPath );

	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		action != timetabEntry->m_actions.End();
		++action )
	{
		CLayerInfo* actionLayerInfo = action->m_layerName.GetCachedLayer();
		
		//if ( action->m_layerName.m_layerName == apLayerPath )
		if( actionLayerInfo == apLayerInfo )
		{
			for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator categoryI = action->m_actionCategories.Begin();
				categoryI != action->m_actionCategories.End();
				++categoryI )
			{
				if ( categoryI->m_weight > 0.0f && m_apMan->CanDoAction( apID, categoryI->m_name ) && m_apMan->DoesMatchTags( apID, categoryI->m_apTags ) )
				{
					if ( matchCategory )
					{
						*matchCategory = categoryI->m_name;
					}
					return true;
				}
			}
		}
	}

	return false;
}

CCommunityInitializers* SAgentStub::GetInitializers( Bool storyPhaseInitializers ) const
{
	if( storyPhaseInitializers )
	{
		return m_storyPhaseCurrentOwner.m_communityStoryPhaseEntry->m_initializers;
	}

	return m_storyPhaseCurrentOwner.m_communityEntitiesEntry->m_initializers;
}

CName SAgentStub::GetActivePhaseName() const
{
	return m_storyPhaseCurrentOwner.m_community.Get() != NULL ? m_storyPhaseCurrentOwner.m_community->GetActivePhaseName() : CName::NONE;
}

String SAgentStub::GetSpawnsetName() const
{
	return m_storyPhaseCurrentOwner.m_community.Get() != NULL ? m_storyPhaseCurrentOwner.m_community->GetFriendlyName() : String::EMPTY;
}

Bool SAgentStub::IsActionPointLoaded() const
{
	if( !m_apMan )
	{
		return false;
	}

	if ( m_schedule.m_activeActionPointID != ActionPointBadID && m_apMan.IsValid() )
	{
		if( m_npc.Get() && m_npc.Get()->IsWorkingInAP() )
		{
			return m_apMan->DoesExist( m_schedule.m_activeActionPointID );
		}		
	}

	return true;
	
	/*CLayerInfo* info = m_schedule.m_activeLayerInfo.Get(); 
	if ( info != NULL )
	{
	return info->IsLoaded();
	}
	else
	{
	return true;
	}*/
}


//////////////////////////////////////////////////////////////////////////
