/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneAreaComponent.h"
#include "storySceneVoicetagMapping.h"
#include "factsDB.h"
#include "storyScene.h"
#include "storySceneSystem.h"
#include "sceneLog.h"
#include "wayPointComponent.h"

#include "../engine/weatherManager.h"
#include "../engine/tickManager.h"
#include "../engine/tagManager.h"

IMPLEMENT_RTTI_ENUM( ESceneSelectionMode );
IMPLEMENT_RTTI_ENUM( ESceneActorType );
IMPLEMENT_ENGINE_CLASS( CScenesTableEntry );
IMPLEMENT_ENGINE_CLASS( CSceneAreaComponent );

#define	TIME_SCENE_MAX						(0x7fffffffffffffff)
#define TIME_SCENE_MAY_START				EngineTime::ZERO
#define TIME_SCENE_IN_PROGRESS				EngineTime( TIME_SCENE_MAX )
#define NUM_SEARCH_SCENE_TO_START_RETRIES	1
#define INVALID_SCENE						-1

namespace
{

	class CSceneAreaScenesListener : public CStorySceneController::IListener
	{
		THandle< CSceneAreaComponent >	m_owner;
		CScenesTableEntry *		m_entry;

	public:
		CSceneAreaScenesListener( CSceneAreaComponent* owner, CScenesTableEntry & entry )
			: m_owner( owner ), m_entry( &entry )
		{}

		virtual void OnStorySceneMappingDestroyed( CStorySceneController * mapping )
		{
			CSceneAreaComponent* area = m_owner.Get();
			ASSERT( area && "SceneAreaComponent was lost. Serious bug." );
			if ( area )
			{
				area->SceneEnded( *m_entry );
			}
			delete this;
		}
	};
}

Bool CScenesTableEntry::IsActive() const
{
	if ( m_timeOfUnlocking > GEngine->GetRawEngineTime() )
	{
		return false;
	}

	if ( m_sceneFile.GetPath().Empty() )
	{
		return false;
	}

	if ( ! m_requiredFact.Empty() && ! GCommonGame->GetSystem< CFactsDB >()->DoesExist( m_requiredFact ) )
	{
		return false;
	}

	if ( ! m_forbiddenFact.Empty() && GCommonGame->GetSystem< CFactsDB >()->DoesExist( m_forbiddenFact ) )
	{
		return false;
	}

	return true;
}

void CSceneAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( world == GGame->GetActiveWorld() && GGame->IsActive() )
	{
		m_timeOfNextScene		= 0.f;//GEngine->GetRawEngineTime() + GEngine->GetRandomNumberGenerator().Get< Float >( m_intervalBetweenScenes );
		m_numScenesInProgress	= 0;
		m_lastPlayedScene		= -1;
		m_sceneStartIsPending	= false;

		for ( Uint32 i = 0; i < m_scenes.Size(); ++i )
		{
			m_scenes[i].m_timeOfUnlocking = TIME_SCENE_MAY_START;
		}

		m_dialogsets.Clear();

		for ( ComponentIterator< CWayPointComponent > it( GetEntity() ); it; ++it )
		{
			m_dialogsets.PushBack( *it );
		}
	}
}

void CSceneAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Deactivate scenes starting
	if ( m_isInTick )
	{
		world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
		m_isInTick = false;
	}	
}

Bool CSceneAreaComponent::IsRaining()
{
	const static Float RAIN_TREESHOLD = 0.01f;

	if( CWorld* world = GGame->GetActiveWorld() )
	{
		if( CEnvironmentManager* envManager = world->GetEnvironmentManager() )
		{
			CWeatherManager* weatherManager = envManager->GetWeatherManager();

			if( weatherManager )
			{
				Float rainStrength = weatherManager->GetEffectStrength( EWeatherEffectType::WET_RAIN );
				return rainStrength > RAIN_TREESHOLD;
			}
		}
	}
	return false;
}

void CSceneAreaComponent::EnteredArea( CComponent* component )
{
	if( IsRaining() )
		return;

	if ( component && component->GetEntity() == GCommonGame->GetPlayer() )
	{
		ASSERT( !m_isInTrigger );
		m_isInTrigger = true;

		// Activate scenes starting
		if ( !m_isInTick )
		{
			GGame->GetActiveWorld()->GetTickManager()->AddToGroup( this, TICK_Main );
			m_isInTick = true;
		}
	}
}

void CSceneAreaComponent::ExitedArea( CComponent* component )
{
	if ( component && component->GetEntity() == GCommonGame->GetPlayer() )
	{
		ASSERT( m_isInTrigger );
		m_isInTrigger = false;

		// Deactivate scenes starting
		if ( m_isInTick && CanRemoveFromTick() )
		{
			GGame->GetActiveWorld()->GetTickManager()->RemoveFromGroup( this, TICK_Main );
			m_isInTick = false;
		}
	}
}

Int32 CSceneAreaComponent::GetNextSceneToPlay( Int32 lastScene )
{
	if ( m_scenes.Empty() )
	{
		return -1;
	}

	const Uint32 numScenes = m_scenes.Size();

	if ( numScenes == 1 )
	{
		return m_scenes[ 0 ].IsActive() ? 0 : INVALID_SCENE;
	}

	// Select next scene only if it is active
	if ( m_sceneSelectionMode == SceneSelectionMode_SequentialWithoutSkipping )
	{
		lastScene = ( lastScene + 1 ) % numScenes;
		return m_scenes[ lastScene ].IsActive() ? lastScene : INVALID_SCENE;
	}

	// Select next active scene
	if ( m_sceneSelectionMode == SceneSelectionMode_Sequential )
	{
		Uint32 numAvailableScenes = numScenes;

		while ( numAvailableScenes > 0 )
		{
			lastScene = ( lastScene + 1 ) % numScenes;

			if ( m_scenes[ lastScene ].IsActive() )
			{
				return lastScene;
			}

			--numAvailableScenes;
		}
	}

	// Select random active scene
	else if ( m_sceneSelectionMode == SceneSelectionMode_Random )
	{
		TDynArray< Uint32 > validEntries;

		for ( Uint32 i = 0; i < m_scenes.Size(); ++i )
		{
			if ( m_scenes[ i ].IsActive() )
			{
				validEntries.PushBack( i );
			}
		}

		if ( ! validEntries.Empty() )
		{
			return validEntries[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( validEntries.Size() ) ];
		}
	}

	return INVALID_SCENE;
}

void CSceneAreaComponent::OnTick( Float timeDelta )
{
	if ( !IsEnabled() )
	{
		return;
	}

	PC_SCOPE_PIX( SceneAreaComponentTick );

	// Update chat players
	UpdateChatPlayers( timeDelta );

	//////////////////////////////////////////////////////////////////////////
	// Return if scene starting is locked
	if ( m_timeOfNextScene > GEngine->GetRawEngineTime() )
	{
		return;
	}

	// TODO: Consult use cases with DMac
	// Cannot start scene area if player is in non gameplay scene
	if ( GCommonGame->GetPlayer()->IsInNonGameplayScene() == true )
	{
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	// Start scene
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Choose one of available scenes
	Uint32 retriesLeft = m_sceneSelectionMode == SceneSelectionMode_SequentialWithoutSkipping
		? 0 : NUM_SEARCH_SCENE_TO_START_RETRIES;
	while ( retriesLeft > 0 )
	{
		// Get next scene to play
		Int32 nextScene = m_sceneStartIsPending ? m_lastPlayedScene : GetNextSceneToPlay( m_lastPlayedScene );
		if ( nextScene < 0 )
		{
			return; // No valid scene found
		}
		ASSERT( (Uint32) nextScene < m_scenes.Size() );
		
		// Try to play the scene
		CScenesTableEntry & entry = m_scenes[ nextScene ];

		BaseSoftHandle::EAsyncLoadingResult loadingResult = entry.m_sceneFile.GetAsync( true );

		m_sceneStartIsPending = loadingResult == BaseSoftHandle::ALR_InProgress;
		if ( m_sceneStartIsPending )
		{
			m_lastPlayedScene = nextScene;
			break;
		}
		else if ( loadingResult == BaseSoftHandle::ALR_Loaded )
		{
			CStoryScene * sceneFile = entry.m_sceneFile.Get();
			if ( sceneFile )
			{
				const CStorySceneInput * input = sceneFile->FindInput( entry.m_sceneInput );
				if ( input )
				{
					Bool ret = false;

					if ( m_actorsType == SceneActorType_NewNpcs )
					{
						ret = StartSceneForNewNpcsActors( input, entry );
					}
					else if ( m_actorsType == SceneActorType_BackgroundNpcs )
					{
						ret = StartSceneForBackgroundNpcsActors( input, entry );
					}
					else
					{
						ASSERT( 0 );
					}

					if ( ret )
					{
						++m_numScenesInProgress;

						m_timeOfNextScene	    = m_numScenesInProgress >= m_maxConcurrentScenes
							? TIME_SCENE_IN_PROGRESS
							: GEngine->GetRawEngineTime() + m_intervalBetweenScenes;
						m_lastPlayedScene       = nextScene;
						entry.m_timeOfUnlocking = TIME_SCENE_IN_PROGRESS;

						// Success!!!
						return; 
					}
					else
					{
						// E3 hack for improving performance
						m_timeOfNextScene = GEngine->GetRawEngineTime() + 1.0f; 
					}
				}
				else
				{
					SCENE_WARN( TXT("Cannot find input '%ls' in scene '%ls' for scenes trigger '%ls'"),
						entry.m_sceneInput.AsChar(),
						entry.m_sceneFile.GetPath().AsChar(),
						GetFriendlyName().AsChar() );
				}		
			}
			else
			{
				SCENE_WARN( TXT("Cannot find scene resource '%ls' for scenes trigger '%ls'"),
					entry.m_sceneFile.GetPath().AsChar(),
					GetFriendlyName().AsChar() );
			}
		}
		//else if ( loadingResult == TSoftHandle::ALR_Failed )
		//{}

		// Failure!!! Try to select other mapping
		--retriesLeft;
	}
}

void CSceneAreaComponent::UpdateChatPlayers( Float dt )
{
	const Int32 size = m_chatPlayers.SizeInt();
	for ( Int32 i=size-1; i>=0; --i )
	{
		CSceneAreaComponentChatPlayer* player = m_chatPlayers[ i ];

		if ( !player->Update( dt ) )
		{
			SceneEnded( player->GetEntry() );

			delete player;
			m_chatPlayers.RemoveAt( i );
		}
	}

	if ( CanRemoveFromTick() )
	{
		RemoveFromTick();
	}
}

void CSceneAreaComponent::SceneEnded( CScenesTableEntry & entry )
{
	ASSERT( m_numScenesInProgress > 0 );
	ASSERT( m_numScenesInProgress <= m_maxConcurrentScenes );

	--m_numScenesInProgress;

	entry.m_timeOfUnlocking = GEngine->GetRawEngineTime() + m_intervalBetweenScenes;
	
	if ( m_timeOfNextScene == TIME_SCENE_IN_PROGRESS )
	{
		m_timeOfNextScene = GEngine->GetRawEngineTime() + m_intervalBetweenScenes;
	}

	// Release scene file
	entry.m_sceneFile.Release();
}

Bool CSceneAreaComponent::ValidateNPCsPosition( const CStorySceneInput * input )
{
	CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();

	if( !tagManager )
	{
		return false;
	}

	TDynArray< SExtractedSceneLineData > lines;
	if ( !CStorySceneSystem::ExtractChatDataFromSceneInput( input, lines ) )
	{
		return false;
	}

	TDynArray< TagList > checkedNPC;
	const Vector& scenePos = GetEntity()->GetWorldPositionRef();	
	TDynArray< CEntity* > tempEntities;

	for ( Uint32 i=0; i<lines.Size(); ++i )
	{				
		if( checkedNPC.GetIndex( lines[ i ].m_actorTags ) >= 0 )
			continue;

		tempEntities.ClearFast();
		tagManager->CollectTaggedEntities( lines[ i ].m_actorTags, tempEntities, BCTO_MatchAll );
	
		if ( tempEntities.Size() == 0 )
		{
			SCENE_WARN( TXT( "Cannot play input as chat - couldn't find actor with '%ls' in world" ), lines[ i ].m_actorTags.ToString().AsChar() );
			return false;
		}

		Bool valid = false;
		for ( Uint32 j=0; j<tempEntities.Size() && !valid; ++j )
		{
			CNewNPC* npc = Cast< CNewNPC >( tempEntities[ j ] );
			valid = npc && npc->IsWorkingInAP();			
		}


		if( !valid )
		{
			return false;
		}		
		
		checkedNPC.PushBack( lines[ i ].m_actorTags );
	}

	return true;
}

Bool CSceneAreaComponent::StartSceneForNewNpcsActors( const CStorySceneInput * input, CScenesTableEntry & entry )
{

	if( !ValidateNPCsPosition( input) )
	{
		return false;
	}

	CWayPointComponent * suggestedScenePosition = m_dialogsets.Empty()
		? NULL
		: m_dialogsets[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_dialogsets.Size() ) ];

	CStorySceneController * scene =
		GCommonGame->GetSystem< CStorySceneSystem >()->PlayInputExt( input, SSFM_ForcePosition, m_scenesPriority, NULL, suggestedScenePosition );

	// Add listener that will reset timeouts
	if ( scene )
	{
		scene->SetListener( new CSceneAreaScenesListener( this, entry ) );
	}

	return scene != NULL;
}

Bool CSceneAreaComponent::StartSceneForBackgroundNpcsActors( const CStorySceneInput * input, CScenesTableEntry & entry )
{
	if ( !GGame->GetActiveWorld() || !GGame->GetActiveWorld()->GetTagManager() )
	{
		return false;
	}

	TDynArray< SExtractedSceneLineData > lines;

	// Extract data
	if ( !CStorySceneSystem::ExtractChatDataFromSceneInput( input, lines ) )
	{
		return false;
	}

	// Try to find actors
	TDynArray< CBgNpc* > npcs;
	npcs.Resize( lines.Size() );
	for ( Uint32 i=0; i<npcs.Size(); ++i )
	{
		npcs[ i ] = NULL;
	}

	TDynArray< TPair< TagList, CBgNpc* > > cachedEntities;
	TDynArray< CEntity* > tempEntities;
	for ( Uint32 i=0; i<lines.Size(); ++i )
	{
		tempEntities.ClearFast();

		Bool found = false;

		for ( Uint32 j=0; j<cachedEntities.Size(); ++j )
		{
			if ( cachedEntities[ j ].m_first == lines[ i ].m_actorTags )
			{
				npcs[ i ] = cachedEntities[ j ].m_second;
				found = true;
				break;
			}
		}

		if ( found )
		{
			continue;
		}

		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( lines[ i ].m_actorTags, tempEntities, BCTO_MatchAll );
		for ( Uint32 j=0; j<tempEntities.Size(); ++j )
		{
			CEntity* e = tempEntities[ j ];
			if ( e->IsA< CBgNpc >() )
			{
				CBgNpc* npc = static_cast< CBgNpc* >( e );

				npcs[ i ] = npc;
				cachedEntities.PushBack( TPair< TagList, CBgNpc* >( lines[ i ].m_actorTags, npc ) );

				found = true;
				break;
			}
		}

		if ( !found )
		{
			SCENE_WARN( TXT( "Cannot play input as chat - couldn't find actor with '%ls' in world" ), 
				lines[ i ].m_actorTags.ToString().AsChar() );
			return false;
		}
	}

	for ( Uint32 i=0; i<npcs.Size(); ++i )
	{
		ASSERT( npcs[ i ] );
	}

	// Create player and start chat
	CSceneAreaComponentChatPlayer* player = new CSceneAreaComponentChatPlayer( entry );

	if ( player->Create( npcs, lines ) )
	{
		m_chatPlayers.PushBack( player );

		return true;
	}
	else
	{
		delete player;

		return false;
	}
}

Bool CSceneAreaComponent::CanRemoveFromTick() const
{
	return m_chatPlayers.Size() == 0 && !m_isInTrigger;
}

void CSceneAreaComponent::RemoveFromTick()
{
	CWorld* world = GetLayer()->GetWorld();
	if ( world && world->GetTickManager() )
	{
		world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
		m_isInTick = false;
	}
}
