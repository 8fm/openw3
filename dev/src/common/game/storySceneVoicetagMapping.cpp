/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneVoicetagMapping.h"

#include "..\engine\dynamicLayer.h"

#include "storySceneIncludes.h"
#include "storySceneInput.h"
#include "storySceneOutput.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "sceneLog.h"

RED_DEFINE_STATIC_NAME( OnSceneStarted );
RED_DEFINE_STATIC_NAME( OnSceneEnded );
RED_DEFINE_STATIC_NAME( GoToScenePosition );
RED_DEFINE_STATIC_NAME( WorkAtScenePosition );
RED_DEFINE_STATIC_NAME( CAIGoalPrepareForScene );
RED_DEFINE_STATIC_NAME( CAIGoalWorkInScene );


IMPLEMENT_ENGINE_CLASS( CStorySceneActorTemplate )
IMPLEMENT_RTTI_ENUM( EStoryScenePerformActionMode )
IMPLEMENT_ENGINE_CLASS( CStorySceneActorPosition )
IMPLEMENT_ENGINE_CLASS( CStorySceneVoicetagMapping )

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

StorySceneControllerSetup::StorySceneControllerSetup()
	: m_forceMode( SSFM_ForceNothing )
	, m_priority( BTAP_Idle )
	, m_contextActorsProvider( NULL )
	, m_suggestedPosition( NULL )
	, m_mustPlay( false )
	, m_world( NULL )
	, m_player( NULL )
	, m_scenePlayerClass( ClassID< CStoryScenePlayer >() )
	, m_sceneDisplay( NULL )
	, m_sceneDebugger( NULL )
	, m_csWorldInterface( NULL )
	, m_spawnAllActors( false )
	, m_useApprovedVoDurations( false )
	, m_asyncLoading( true )
	, m_hackFlowCtrl( nullptr )
{
}

//////////////////////////////////////////////////////////////////////////

const Float		SCENE_PLAYING_TIMEOUT = 45.0f;
const Float		SCENE_MAX_DISTANCE = 50.0f;

/************************************************************************/
/* CStorySceneActorsMapping                                             */
/************************************************************************/

CStorySceneController::CStorySceneController( const CStorySceneInput * sceneInput, EArbitratorPriorities priority, EStorySceneForcingMode forcingMode, Bool isQuestScene )
  : m_player(NULL)
  , m_started(false)
  , m_paused(false)
  , m_sceneInput(sceneInput)
  , m_listener(NULL)
  , m_allConditionsMet(false)
  , m_reapplyOrders(true)
  , m_priority(priority)
  , m_forcingMode(forcingMode)
  , m_isQuestScene( isQuestScene )
  , m_completionReason( SCR_NONE )
{
	SCENE_ASSERT( m_sceneInput );
	Float currentTime = (Float)GGame->GetEngineTime();
	m_maxStartTime = currentTime + SCENE_PLAYING_TIMEOUT;
}

CStorySceneController::CStorySceneController( const CStorySceneInput* sceneInput )
	: m_player(NULL)
	, m_started(false)
	, m_paused(false)
	, m_sceneInput(sceneInput)
	, m_listener(NULL)
	, m_allConditionsMet(false)
	, m_reapplyOrders(true)
	, m_priority(BTAP_FullCutscene)
	, m_forcingMode(SSFM_SpawnWhenNeeded)
	, m_isQuestScene( true )
	, m_completionReason( SCR_NONE )
{
}

CStorySceneController::~CStorySceneController()
{
	m_completionReason = SCR_PLAYER_DESTROYED;

	if ( m_listener )
		m_listener->OnStorySceneMappingDestroyed( this );

	// mcinek: You know this is destructor, right?...
	m_player = NULL;
	m_started = false;
	m_paused = false;
	m_listener = NULL;
	m_allConditionsMet = false;
}


Bool CStorySceneController::IsPaused() const
{
	return m_paused || ( m_player && m_player->IsPaused() );
}

Bool CStorySceneController::Start()
{
	SCENE_ASSERT( m_sceneInput );
	if ( ! m_sceneInput )
	{
		return false;
	}

	if ( m_paused == true )
	{
		return false;
	}

	Float currentTime = (Float)GGame->GetEngineTime();
	if ( currentTime > m_maxStartTime && m_isQuestScene == false )
	{	
		SCENE_LOG( TXT("Scene starting has timeouted (max %f) (%s)"), SCENE_PLAYING_TIMEOUT, m_sceneInput->GetFriendlyName().AsChar() );
		Stop( SCR_TIMEDOUT );
		return false;
	}

	m_allConditionsMet = true;
	m_started = true;
	for( TDynArray< SceneActorInfo >::iterator actorIter = m_mappedActors.Begin(); actorIter != m_mappedActors.End(); ++actorIter )
	{
		CActor* actor = Cast< CActor >( actorIter->m_actor.Get() );

		if ( actor == NULL )
		{
			continue;
		}

		if ( actor->IsLockedByScene() == true )
		{
			SCENE_LOG( TXT("%s - %s - Actor (%s) is locked by other scene. Scene playing delayed"), m_sceneInput->GetFriendlyName().AsChar(), actorIter->m_voicetag.AsString().AsChar(), actor->GetName().AsChar() );
			m_allConditionsMet = false;
			return false;
		}
	}

	for( TDynArray< SceneActorInfo >::iterator actorIter = m_mappedActors.Begin(); actorIter != m_mappedActors.End(); ++actorIter )
	{
		CActor* actor = Cast< CActor >( actorIter->m_actor.Get() );

		if ( actor == NULL )
		{
			continue;
		}

		// Stop other scenes this actor plays
		const TDynArray< CStorySceneController * > & scenes = actor->GetStoryScenes();
		for ( Uint32 i = 0; i < scenes.Size(); ++i )
		{
			if ( scenes[ i ] != this )
			{
				if ( scenes[ i ]->GetPlayer() != NULL )
				{
					String messageText = String::Printf( 
						TXT( "WARNING: Scene [%s] stopped because of stolen actor %s in scene [%s]" ), 
						scenes[ i ]->GetPlayer()->GetName().AsChar(), 
						actor->GetVoiceTag().AsString().AsChar(),
						m_sceneInput->GetFriendlyName().AsChar() );

					SCENE_WARN( messageText.AsChar() );
					SET_ERROR_STATE( actor, messageText );

				}

				scenes[i]->Stop( SCR_ACTOR_STOLEN );
			}
		}

		actor->AddStoryScene( this );
		LockActor( actor, actorIter->m_invulnerable );
	}

	return m_allConditionsMet;
}

Bool CStorySceneController::Play( const ScenePlayerPlayingContext& context )
{
	SCENE_ASSERT( m_player.Get() == nullptr );
	SCENE_ASSERT( m_sceneInput != nullptr );

	if ( !m_allConditionsMet )
	{
		return false;
	}

	// Try to start the scene
	if ( GCommonGame->GetSystem< CStorySceneSystem >() )
	{
		m_player = GCommonGame->GetSystem< CStorySceneSystem >()->StartScene( *this, context );
	}

	return m_player.Get() != nullptr;
}

void CStorySceneController::Pause( Bool pause )
{
	m_paused = pause;

	if ( m_player )
	{
		m_player->Pause( pause, CStoryScenePlayer::EPC_Controller );
	}
	else if ( ! pause )
	{
		m_reapplyOrders = true;
	}
}

void CStorySceneController::Stop( SceneCompletionReason stopReason )
{
	if ( ! m_started )
	{
		return;
	}
	
	m_started = false;
	m_completionReason = stopReason;

	// Unlock mapping actors
	for ( Uint32 iActor = 0; iActor < m_mappedActors.Size(); ++iActor )
	{
		SceneActorInfo & mappedActor = m_mappedActors[iActor];
		if ( CActor *actor = Cast< CActor >( m_mappedActors[iActor].m_actor.Get() ) )
		{
			if ( mappedActor.m_destroyOnFinish )
			{
				SCENE_LOG( TXT( "%s - %s - Despawning actor spawned by scene" ), m_sceneInput->GetFriendlyName().AsChar(), mappedActor.m_voicetag.AsString().AsChar() );
				actor->RemoveStoryScene( this );
				UnlockActor( actor );
				actor->SetDisableAllDissolves( true );
				actor->Destroy();
			}
			else
			{
				actor->RemoveStoryScene( this );
				UnlockActor( actor );
			}			
		}		
	}
	
	// Unlock additional actors
	while ( m_lockedActors.Empty() == false )
	{
		THandle< CActor > actorHandle = m_lockedActors.Back();
		CActor *actor = actorHandle.Get();
		if ( actor )
		{
			actor->RemoveStoryScene( this );
			UnlockActor( actor );
		}
		else
		{
			m_lockedActors.RemoveFast( actorHandle );
		}
	}


	Bool hasBeenPlaying = m_player.Get() != nullptr;
	Bool wasGameplay = false;
	if ( m_player )
	{
		m_activatedOutput = m_player->GetActivatedOutputName();

		// in some cases scenes should return output when killed
		if ( SCR_SCENE_ENDED != stopReason && m_activatedOutput == CName::NONE && m_sceneInput )
		{
			TDynArray<const CStorySceneLinkElement*> elements;
			 m_sceneInput->GetAllNextLinkedElements( elements );
			for ( Uint32 i = 0; i < elements.Size(); ++i )
			{
				const CStorySceneOutput* output = Cast<const CStorySceneOutput>( elements[i] );
				if ( output != NULL )
				{
					m_activatedOutput = CName( output->GetName() );
					break;
				}
			}
		}

		wasGameplay = m_player->IsGameplay();
		m_player->Stop();
		m_player->SetDestroyFlag();
	}

	if ( GCommonGame->GetSystem< CStorySceneSystem >() )
	{
		GCommonGame->GetSystem< CStorySceneSystem >()->OnSceneMappingStopped( this, hasBeenPlaying );
	}
	if ( m_listener != NULL )
	{
		m_listener->OnStorySceneMappingStopped( this );
	}

	// GC
	//if ( hasBeenPlaying == true && wasGameplay == false && GGame->GetGameplayConfig().m_gcAfterNotGameplayScenes )
	//{
	//	SGarbageCollector::GetInstance().RequestGC();
	//}
}

void CStorySceneController::Free()
{
	if ( m_started )
	{
		Stop( SCR_PLAYER_DESTROYED );
		return;
	}

#ifdef DEBUG_SCENES_2
	if ( GGame && GGame->GetActiveWorld() )
	{
		auto& ents = GGame->GetActiveWorld()->GetDynamicLayer()->GetEntities();
		for ( CEntity* ent : ents )
		{
			if( ent && ent->GetClass() == ClassID< CStoryScenePlayer >() )
			{
				CStoryScenePlayer* player = static_cast< CStoryScenePlayer* >( ent );

				CStorySceneController* controller = player->GetSceneController();
				if ( controller )
				{
					SCENE_ASSERT( controller != this );
				}

				SCENE_ASSERT( GetPlayer() != player );
			}
		}
	}
#endif

	delete this;
}

Bool CStorySceneController::IsPositionReached( SceneActorInfo & mappedActor, Bool force )
{
	SCENE_ASSERT( mappedActor.m_actor.Get() );

	CActor *actor = Cast< CActor >( mappedActor.m_actor.Get() );
	if ( actor == NULL )
	{
		return true;
	}

	if ( ! actor->IsSpawned() )
	{
		return false;
	}
	if ( m_forcingMode == SSFM_SpawnAndForcePosition || m_forcingMode == SSFM_SpawnWhenNeeded )
	{
		// If actors may be spawned, they may be teleported to their positions too
		return true;
	}
	
	return true;
}

void CStorySceneController::LockActor( CActor* actor, Bool setInvulnerable /*= true */ )
{
	SCENE_ASSERT( actor )

	m_lockedActors.PushBack( actor );
}

Bool CStorySceneController::IsActorLocked( CActor* actor ) const
{
	for ( const THandle< CActor >& it : m_lockedActors )
	{
		if ( it.Get() == actor )
		{
			return true;
		}
	}
	return false;
}

void CStorySceneController::UnlockActor( CActor* actor )
{
	SCENE_ASSERT( actor )

	m_lockedActors.Remove( actor );
}

void CStorySceneController::RegisterNewSceneActor( CActor* actor, CName actorName, Bool dontDestroyAfterScene )
{
	if ( actor == NULL )
	{
		SCENE_WARN( TXT( "Tried to register new spawned actor, who is NULL" ) );
		return;
	}

	Bool actorFoundInMappins = false;
	Bool actorInvulnerable = true;
	for ( Uint32 i = 0; i < m_mappedActors.Size(); ++i )
	{
		SceneActorInfo& mappedActor = m_mappedActors[ i ];
		if ( mappedActor.m_voicetag == actorName && mappedActor.m_actor.Get() == NULL )
		{
			mappedActor.m_actor = actor;
			mappedActor.m_destroyOnFinish = !dontDestroyAfterScene;
			actorFoundInMappins = true;
			actorInvulnerable = mappedActor.m_invulnerable;
			SCENE_LOG( TXT( "%s - %s - actor spawned by scene. It should be despawned once the scene ends" ), m_sceneInput->GetFriendlyName().AsChar(), actorName.AsString().AsChar() );
			break;
		}
	}

	if ( actorFoundInMappins == false )
	{
		SCENE_WARN( TXT( "New spawned actor %s not found in scene controller mappings" ), actor->GetVoiceTag().AsString().AsChar() );
	}

	LockActor( actor, actorInvulnerable );
}

Bool CStorySceneController::UnregisterSceneActor( CActor* actor, Bool preserveMappingActors )
{
	if ( actor == NULL || ( preserveMappingActors == true && IsMappingActor( actor ) == true ) )
	{
		return false;
	}

	UnlockActor( actor );

	return true;
}

Bool CStorySceneController::IsPropDestroyedAfterScene( CName id )
{
	for ( Uint32 i = 0; i < m_mappedProps.Size(); ++i )
	{
		if ( m_mappedProps[ i ].m_id == id )
		{
			return m_mappedProps[ i ].m_destroyAfterScene;
		}
	}
	return true;
}

Bool CStorySceneController::IsEffectDestroyedAfterScene( CName id )
{
	for ( Uint32 i = 0; i < m_mappedEffects.Size(); ++i )
	{
		if ( m_mappedEffects[ i ].m_id == id )
		{
			return m_mappedEffects[ i ].m_destroyAfterScene;
		}
	}
	return true;
}

Bool CStorySceneController::IsLightDestroyedAfterScene( CName id )
{
	for ( Uint32 i = 0; i < m_mappedLights.Size(); ++i )
	{
		if ( m_mappedLights[ i ].m_id == id )
		{
			return m_mappedLights[ i ].m_destroyAfterScene;
		}
	}
	return true;
}

Bool CStorySceneController::IsMappingActor( CActor* actor )
{
	if ( actor == NULL )
	{
		return false;
	}

	return IsMappingActor( actor->GetVoiceTag() );
}

Bool CStorySceneController::IsMappingActor( const CName& actorVoicetag )
{
	for ( Uint32 i = 0; i < m_mappedActors.Size(); ++i )
	{
		if ( m_mappedActors[ i ].m_voicetag == actorVoicetag )
		{
			return true;
		}
	}
	return false;
}

Bool CStorySceneController::CanStopByExternalSystem() const
{
	if ( m_sceneInput )
	{
		return m_sceneInput->IsGameplay() && m_sceneInput->CanStopByExternalSystem();
	}
	return true;
	//return false;
}

void CStorySceneController::StoreActorItems( CActor* actor )
{
	if ( m_actorItems.KeyExist( actor ) )
	{
		return;
	}

	SActorRequiredItems items( CName::NONE, CName::NONE );
	actor->StoreRequiredItems( items );
	m_actorItems.Insert( actor, Move( items ) );
}

void CStorySceneController::ForgetActorItems( CActor* actor )
{
	m_actorItems.Erase( actor );
}

void CStorySceneController::RestoreActorItems()
{
	for ( auto it = m_actorItems.Begin(), end = m_actorItems.End(); it != end; ++it )
	{
		it->m_first->IssueRequiredItems( it->m_second, true );
	}
	m_actorItems.Clear();
}

Bool CStorySceneController::IsGameplayScene() const
{
	if ( m_sceneInput )
	{
		return m_sceneInput->IsGameplay();
	}
	return true;
}

Bool CStorySceneController::IsActorOptional( CName id ) const
{
	for ( Uint32 i=0; i<m_mappedActors.Size(); ++i )
	{
		if( m_mappedActors[i].m_voicetag == id )
		{
			return m_mappedActors[i].m_actorOptional;
		}
	}
	return false;
}

CActor* CStorySceneController::FindPlayerActor()
{
	for ( auto it = m_mappedActors.Begin(), end = m_mappedActors.End(); it != end; ++it )
	{
		CActor* actor = Cast< CActor >( it->m_actor.Get() );
		if ( actor && actor->GetTags().HasTag( CNAME( PLAYER ) ) )
		{
			return actor;
		}
	}
	return nullptr;
}

void CStorySceneController::MarkLightForDestruction( CName id, Bool mark )
{
	for ( Uint32 i = 0; i < m_mappedLights.Size(); ++i )
	{
		if ( m_mappedLights[ i ].m_id == id )
		{
			m_mappedLights[ i ].m_destroyAfterScene = mark;
			return;
		}
	}
	ASSERT( false );
}

void CStorySceneController::MarkEffectForDestruction( CName id, Bool mark )
{
	for ( Uint32 i = 0; i < m_mappedEffects.Size(); ++i )
	{
		if ( m_mappedEffects[ i ].m_id == id )
		{
			m_mappedEffects[ i ].m_destroyAfterScene = mark;
			return;
		}
	}
	ASSERT( false );
}

void CStorySceneController::MarkPropForDestruction( CName id, Bool mark )
{
	for ( Uint32 i = 0; i < m_mappedProps.Size(); ++i )
	{
		if ( m_mappedProps[ i ].m_id == id )
		{
			m_mappedProps[ i ].m_destroyAfterScene = mark;
			return;
		}
	}
	ASSERT( false );
}

void CStorySceneController::RegisterSpawnedLight( const CName& id, CEntity* prop )
{
	for ( Uint32 i = 0; i < m_mappedLights.Size(); ++i )
	{
		if ( m_mappedLights[ i ].m_id == id )
		{
			m_mappedLights[ i ].m_destroyAfterScene = true;
			m_mappedLights[ i ].m_prop = prop;
			return;
		}
	}
}

void CStorySceneController::RegisterSpawnedProp( const CName& id, CEntity* prop )
{
	for ( Uint32 i = 0; i < m_mappedProps.Size(); ++i )
	{
		if ( m_mappedProps[ i ].m_id == id )
		{
			m_mappedProps[ i ].m_destroyAfterScene = true;
			m_mappedProps[ i ].m_prop = prop;
			return;
		}
	}
}

void CStorySceneController::RegisterSpawnedEffect( const CName& id, CEntity* prop )
{
	for ( Uint32 i = 0; i < m_mappedEffects.Size(); ++i )
	{
		if ( m_mappedEffects[ i ].m_id == id )
		{
			m_mappedEffects[ i ].m_destroyAfterScene = true;
			m_mappedEffects[ i ].m_prop = prop;
			return;
		}
	}
}

IFile& operator<<( IFile& file, CStorySceneController* controller )
{
	/*if ( controller->m_player )
	{
		file << controller->m_player;
	}*/
	if ( controller->m_sceneInput )
	{
		// DIALOG_TOMSIN_TODO - WTF???
		//file << (const_cast< CStorySceneInput* >( controller->m_sceneInput ));

		CStoryScene* scene = controller->m_sceneInput->GetScene();
		if ( scene )
		{
			file << scene;
		}
	}
	return file;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
