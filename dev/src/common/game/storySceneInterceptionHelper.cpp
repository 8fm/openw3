/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneSystem.h"

CStorySceneInterceptionHelper::CStorySceneInterceptionHelper( CStoryScenePlayer* player )
	: m_player( player )
	, m_currentInterceptingSection( nullptr )
	, m_currentInterceptedSection( nullptr )
	, m_interceptTimeout( 0.0f )
	, m_isComingBackFromInterception( false )
{}

Bool CStorySceneInterceptionHelper::IsEmptyInterceptionInProgress()
{
	// Interceptions only work in game

	if ( !m_player->IsSceneInGame() )
	{
		return false;
	}

	return m_currentInterceptedSection && !m_currentInterceptingSection;
}

Bool CStorySceneInterceptionHelper::TickEmptyInterception()
{
	ASSERT( IsEmptyInterceptionInProgress() );

	// Interceptions only work in game

	if ( !m_player->IsSceneInGame() )
	{
		return false;
	}

	// Some actors still not in range?

	CStorySceneSectionPlayingPlan* plan = m_player->GetCurrentPlayingPlan();
	CStorySceneSection* currentSection = plan->m_section;

	if ( !AreAllSectionActorsWithinDistanceFromPlayer( currentSection, currentSection->GetInterceptRadius() ) )
	{
		return true; // Keep ticking
	}

	// All in range - stop interception

	m_currentInterceptedSection = nullptr;
	return false; // Done
}

Bool CStorySceneInterceptionHelper::OnGoToNextElement( Bool& changedSection )
{
	changedSection = false;

	// Interceptions only work in game

	if ( !m_player->IsSceneInGame() )
	{
		return false;
	}

	// Get into intercepted section

	CStorySceneSectionPlayingPlan* plan = m_player->GetCurrentPlayingPlan();
	CStorySceneSection* currentSection = plan->m_section;

	if ( !IsInterceptionInProgress() )
	{
		if ( currentSection->HasInterception() )
		{
			if ( !AreAllSectionActorsWithinDistanceFromPlayer( currentSection, currentSection->GetInterceptRadius() ) )
			{
				if ( m_interceptTimeout <= 0.0f &&					// Go to 'interception' section if timeout (after previous 'interception') expires
					currentSection->HasInterceptionSections() )		// And if there is section to go to
				{													// Otherwise just pause current section
					BeginInterception();
					changedSection = true;
					return true;
				}
				else
				{
					m_currentInterceptedSection = currentSection;
					return true;
				}
			}
		}
	}

	// Get out of the intercepted section

	else if ( IsInterceptionInProgress() )
	{
		if ( AreAllSectionActorsWithinDistanceFromPlayer( m_currentInterceptedSection, m_currentInterceptedSection->GetInterceptRadius() ) )
		{
			EndInterception();
			changedSection = true;
			return true;
		}
	}

	// Reset "coming back from interception" state

	m_isComingBackFromInterception = false;

	return false;
}

Bool CStorySceneInterceptionHelper::OnChangeSection()
{
	if ( IsInterceptionInProgress() )
	{
		EndInterception();
		return true;
	}
	return false;
}

void CStorySceneInterceptionHelper::OnPlayerTick( Float timeDelta )
{
	if ( m_interceptTimeout > 0.0f )
	{
		m_interceptTimeout -= timeDelta;
	}
}

Bool CStorySceneInterceptionHelper::OnCheckElement( IStorySceneElementInstanceData* currElem )
{
	return currElem && m_isComingBackFromInterception;
}

Bool CStorySceneInterceptionHelper::OnPlayElement()
{
	// Return from intercepting section back to intercepted one

	if ( IsInterceptionInProgress() )
	{
		EndInterception();
		return true;
	}
	return false;
}

void CStorySceneInterceptionHelper::OnResetPlayer()
{
	m_interceptTimeout = 0.0f;
	m_currentInterceptingSection = nullptr;
	m_currentInterceptedSection = nullptr;
	m_isComingBackFromInterception = false;
}

Bool CStorySceneInterceptionHelper::AreAllSectionActorsWithinDistanceFromPlayer( const CStorySceneSection* section, Float radius )
{
	SCENE_ASSERT( radius > 0.0f );

	CPlayer* player = m_player ? m_player->m_player.Get() : nullptr;
	SCENE_ASSERT( player );
	if ( !player )
	{
		return true;
	}

	const Float radiusSqr = radius * radius;
	const Vector& playerPosition = player->GetWorldPositionRef();

	for ( auto it = m_player->m_sceneActorEntities.Begin(), end = m_player->m_sceneActorEntities.End(); it != end; ++it )
	{
		if ( CEntity* entity = it->m_second.Get() )
		{
			const Vector& actorPosition = entity->GetWorldPositionRef();
			const Float distanceFromPlayerSquare = ( actorPosition - playerPosition ).SquareMag3();
			if ( distanceFromPlayerSquare > radiusSqr )
			{
				return false;
			}
		}
	}

	return true;
}

void CStorySceneInterceptionHelper::BeginInterception()
{
	CStorySceneSectionPlayingPlan* playingPlan = m_player->GetCurrentPlayingPlan();

	CStorySceneSection* interceptedSection = playingPlan->m_section;
	CStorySceneSection* interceptingSection = interceptedSection->DrawInterceptSection();
	m_player->ChangeSection( interceptingSection, false, false );
	m_currentInterceptingSection = interceptingSection;
	m_currentInterceptedSection = interceptedSection;
}

void CStorySceneInterceptionHelper::EndInterception()
{
	m_interceptTimeout = m_currentInterceptedSection->GetInterceptTimeout();

	CStorySceneSectionPlayingPlan* playingPlan = m_player->GetCurrentPlayingPlan();
	ASSERT( playingPlan->m_section == m_currentInterceptingSection );
	m_currentInterceptingSection = nullptr;

	CStorySceneSection* interceptedSection = m_currentInterceptedSection;
	m_currentInterceptedSection = nullptr;

	if ( CStorySceneSectionPlayingPlan* plan = m_player->m_sectionLoader.GetPlayingPlan( interceptedSection ) )
	{
		plan->SetPreventRemoval( false );
		m_isComingBackFromInterception = true;
	}

	m_player->ChangeSection( interceptedSection, true, false );
}