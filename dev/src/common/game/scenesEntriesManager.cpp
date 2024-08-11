/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scenesEntriesManager.h"
#include "communityData.h"
#include "storySceneVoicetagMapping.h"
#include "storySceneSystem.h"
#include "sceneLog.h"
#include "communitySystem.h"
#include "../engine/gameTimeManager.h"

#define TIMEOUT_LOCKED_TILL_END_OF_SCENE	FLT_MIN
#define SCENES_UPDATE_INTERVAL				3.f
#define NUM_SEARCH_SCENE_TO_START_RETRIES	3


///////////////////////////////////////////////////////////////////////////////

class CCommunitySceneMappingListener : public CStorySceneController::IListener
{
	const CSSceneTableEntry	* m_storyPhase;
	const CSSceneTimetableScenesEntry	* m_sceneDef;

public:
	CCommunitySceneMappingListener( const CSSceneTableEntry & storyPhase, const CSSceneTimetableScenesEntry & sceneDef )
		: m_storyPhase( &storyPhase ), m_sceneDef( &sceneDef )
	{}

	virtual void OnStorySceneMappingDestroyed( CStorySceneController * mapping )
	{
		ASSERT( GCommonGame->GetSystem< CCommunitySystem >() );

		GCommonGame->GetSystem< CCommunitySystem >()->GetScenesEntriesManager()->m_timeouts[ m_storyPhase ] = m_storyPhase->m_cooldownTime;
		GCommonGame->GetSystem< CCommunitySystem >()->GetScenesEntriesManager()->m_timeouts[ m_sceneDef ]   = m_sceneDef->m_cooldownTime;

		delete this;
	}
};

///////////////////////////////////////////////////////////////////////////////

void CScenesEntriesManager::OnTick( Float timeDelta )
{
	m_updateTimeout -= timeDelta;
	if ( m_updateTimeout > 0.f )
	{
		return;
	}
	timeDelta += SCENES_UPDATE_INTERVAL - m_updateTimeout;
	m_updateTimeout = SCENES_UPDATE_INTERVAL;
	
	// Process reaction timeouts
	TDynArray< const void* > timeoutsToRemove;
	THashMap< const void*, Float >::iterator
		currTimeout = m_timeouts.Begin(),
		lastTimeout = m_timeouts.End();
	for ( ; currTimeout != lastTimeout; ++currTimeout )
	{
		if ( currTimeout->m_second == TIMEOUT_LOCKED_TILL_END_OF_SCENE )
			continue;
		
		if ( currTimeout->m_second <= timeDelta )
			timeoutsToRemove.PushBack( currTimeout->m_first );
		else
			currTimeout->m_second -= timeDelta;
	}
	for ( Uint32 i = 0; i < timeoutsToRemove.Size(); ++i )
	{
		m_timeouts.Erase( timeoutsToRemove[ i ] );
	}

	GameTime currentGameDayTime = GGame->GetTimeManager()->GetTime() % GameTime::DAY;
	CTimeCounter processingTimer;

	// Scan all active StoryPhases
	const TDynArray< THandle< CCommunity > >& spawnsets = GetAttachedSpawnsets();
	for ( TDynArray< THandle< CCommunity > >::const_iterator spawnsetIt = spawnsets.Begin(); 
		spawnsetIt != spawnsets.End(); ++spawnsetIt )
	{
		const CCommunity* spawnset = (*spawnsetIt).Get();
		if ( !spawnset->IsActive() )
		{
			continue;
		}

		const CName& activePhaseName = spawnset->GetActivePhaseName();
		const CSSceneTableEntry* activePhase = spawnset->GetScene( activePhaseName );
		if ( !activePhase )
		{
			continue;
		}

		Float * timeout = m_timeouts.FindPtr( activePhase );
		if ( timeout )
		{
			continue;
		}

		if ( activePhase->m_timetable.Size() == 1 && 
			activePhase->m_timetable[0].m_time.m_begin == GameTime(0) &&
			activePhase->m_timetable[0].m_time.m_end   == GameTime(0) ) 
		{
			StartSceneFromTimetable( *spawnset, *activePhase, activePhase->m_timetable[0] );
		}
		else
		{
			TDynArray< CSSceneTimetableEntry >::const_iterator 
				currTime = activePhase->m_timetable.Begin(),
				lastTime = activePhase->m_timetable.End();
			for ( ; currTime != lastTime; ++currTime )
				if ( currTime->m_time.DoesContainTime( currentGameDayTime ) )
				{
					StartSceneFromTimetable( *spawnset, *activePhase, *currTime );
					break;
				}
		}

		// If processing took to long, continue during the next update
		Float timeSoFar = processingTimer.GetTimePeriod();
		if ( timeSoFar > 0.009f )
		{
			break;
		}
	}
}

void CScenesEntriesManager::StartSceneFromTimetable( const CCommunity & community, const CSSceneTableEntry & storyPhase, const CSSceneTimetableEntry & commTimeTabEntry )
{
	////// Collect scenes that are not asleep
	Float											awakenInputsTotalWeight = 0.f;
	TDynArray< const CSSceneTimetableScenesEntry* >	awakenInputs;
	{
		TDynArray<CSSceneTimetableScenesEntry>::const_iterator
			currInput = commTimeTabEntry.m_scenes.Begin(),
			lastInput = commTimeTabEntry.m_scenes.End();
		for ( ; currInput != lastInput; ++currInput )
		{
			const CSSceneTimetableScenesEntry & sceneInputDefinition = *currInput;
			if ( !sceneInputDefinition.m_storyScene )
			{
				continue;
			}

			Float * timeout = m_timeouts.FindPtr( & sceneInputDefinition );
			if ( ! timeout )
			{
				awakenInputs.PushBack( & sceneInputDefinition );
				awakenInputsTotalWeight += sceneInputDefinition.m_weight;
			}
		}
	}

	////// Choose one of available scenes
	Uint32 retriesLeft = NUM_SEARCH_SCENE_TO_START_RETRIES;
	while ( retriesLeft > 0 && !awakenInputs.Empty() && awakenInputsTotalWeight != 0.f )
	{
		Float randWeight = GEngine->GetRandomNumberGenerator().Get< Float >( awakenInputsTotalWeight );

		TDynArray< const CSSceneTimetableScenesEntry* >::iterator
			currInput = awakenInputs.Begin(),
			lastInput = awakenInputs.End();
		for ( ; currInput != lastInput; ++currInput )
		{
			// Look for randomly selected input
			const CSSceneTimetableScenesEntry * sceneInputDefinition = *currInput;
			if ( randWeight >= sceneInputDefinition->m_weight )
			{
				randWeight -= sceneInputDefinition->m_weight;
				continue;
			}
			
			// Try to play the found input
			const CStorySceneInput * input = sceneInputDefinition->m_storyScene->FindInput( sceneInputDefinition->m_sceneInputSection );
			if ( input )
			{
				CStorySceneController * scene =
					GCommonGame->GetSystem< CStorySceneSystem >()->PlayInputExt( input, sceneInputDefinition->m_forceMode, sceneInputDefinition->m_priority );

				if ( scene )
				{
					m_timeouts[ sceneInputDefinition ] = TIMEOUT_LOCKED_TILL_END_OF_SCENE;
					if ( storyPhase.m_cooldownTime )
						m_timeouts[ & storyPhase ] = TIMEOUT_LOCKED_TILL_END_OF_SCENE;
					// Add listener that will reset timeouts
					scene->SetListener( new CCommunitySceneMappingListener( storyPhase, *sceneInputDefinition ) );

					return; // Success!!!
				}
			}
			else
			{
				SCENE_WARN( TXT("Cannot find input '%ls' in scene '%ls' for community '%ls'"),
					sceneInputDefinition->m_sceneInputSection.AsChar(),
					sceneInputDefinition->m_storyScene->GetFriendlyName().AsChar(),
					community.GetFriendlyName().AsChar() );
			}

			awakenInputsTotalWeight -= sceneInputDefinition->m_weight;
			awakenInputs.EraseFast( currInput );

			break; // Failure!!! Try to select other mapping
		}

		--retriesLeft;
	}
}

///////////////////////////////////////////////////////////////////////////////
