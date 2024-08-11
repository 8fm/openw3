/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

/*
#include "storySceneSectionOverrideCondition.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( IStorySceneSectionOverrideCondition );
IMPLEMENT_ENGINE_CLASS( CStorySceneSectionDistanceOverrideCondition );
IMPLEMENT_ENGINE_CLASS( CStorySceneSectionEavesdroppingOverrideCondition );

//////////////////////////////////////////////////////////////////////////

Bool CStorySceneSectionDistanceOverrideCondition::ShouldOverride( const CStoryScenePlayer* scenePlayer ) const
{
	TDynArray< THandle< CEntity > > actors;
	scenePlayer->GetCurrentSectionActors( actors );

	CEntity* player = GCommonGame->GetPlayer();

	Bool allWithinDistance = true;

	for ( TDynArray< THandle< CEntity > >::const_iterator actorIter = actors.Begin();
		actorIter != actors.End(); ++actorIter )
	{
		if ( actorIter->Get() == NULL )
		{
			continue;
		}

		if ( actorIter->Get()->GetWorldPositionRef().DistanceSquaredTo( player->GetWorldPositionRef() ) > m_distance * m_distance )
		{
			allWithinDistance = false;
			break;
		}
	}

	return allWithinDistance == false;
}

//////////////////////////////////////////////////////////////////////////

Bool CStorySceneSectionEavesdroppingOverrideCondition::ShouldOverride( const CStoryScenePlayer* scenePlayer ) const
{
	Vector listenerPosition;
	Vector listenerVelocity;
	Vector listenerDirection;
	GSoundSystem->GetListenerVectors( listenerPosition, listenerVelocity, listenerDirection );
	
	ASSERT( GCommonGame->GetPlayer() );

	const Vector& playerPosition = GCommonGame->GetPlayer()->GetWorldPositionRef();

	TDynArray< THandle< CEntity > > actors;
	scenePlayer->GetCurrentSectionActors( actors );

	for ( TDynArray< THandle< CEntity > >::const_iterator actorIter = actors.Begin();
		actorIter != actors.End(); ++actorIter )
	{
		CEntity* actorEntity = actorIter->Get();
		if ( actorEntity == NULL )
		{
			continue;
		}

		const Vector& actorPosition = actorEntity->GetWorldPositionRef();

		if ( actorPosition.DistanceSquaredTo( playerPosition ) < m_distanceFromPlayer  * m_distanceFromPlayer )
		{
			// Player is too close
			return true;
		}
		if ( actorPosition.DistanceSquaredTo( listenerPosition ) > m_distanceFromListener * m_distanceFromListener )
		{
			// Listener is too far
			return true;
		}
	}

	return false;
}
*/
