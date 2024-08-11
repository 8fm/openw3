/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneActorMap.h"
#include "sceneLog.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneActorMap );

CStorySceneActorMap::CStorySceneActorMap()
{
}

void CStorySceneActorMap::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	//++ Why do we need this code?
	for ( THashMap< CName, TDynArray< THandle< CActor > > >::const_iterator mapIter = m_speakers.Begin(); mapIter != m_speakers.End(); ++mapIter )
	{
		const TDynArray< THandle< CActor > >& actorsSet = mapIter->m_second;
		for ( TDynArray< THandle< CActor > >::const_iterator setIter = actorsSet.Begin(); setIter != actorsSet.End(); ++setIter )
		{
			const THandle< CActor >& actorHandle = *setIter;
			CActor *pActor = actorHandle.Get();
			if ( pActor )
			{
				pActor->OnSerialize( file );
			}
		}
	}
	//--
}

void CStorySceneActorMap::Clear()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

	m_speakers.Clear();
}

void CStorySceneActorMap::RegisterSpeaker( CActor* actor, CName voiceTag )
{
	ASSERT( actor );
	//ASSERT( voiceTag );
	if ( voiceTag == CName::NONE )
	{
		SCENE_WARN( TXT( "Registering actor ('%ls') without voicetag" ), actor->GetDisplayName().AsChar() );
	}

	THandle< CActor > actorHandle( actor );

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

		// Find node list
		TDynArray< THandle< CActor > >* nodes = m_speakers.FindPtr( voiceTag );
		if ( nodes )
		{
			// Add node to existing hash set
			nodes->PushBack( actorHandle );
		}
		else
		{
			// Add new hash set
			TDynArray< THandle< CActor > > nodes;
			nodes.PushBack( actorHandle );

			// Map by name
			m_speakers[ voiceTag ] = nodes;
		}
	}

	// Log
	//SCENE_LOG( TXT("Voicetag '%ls' registered with '%ls'"), voiceTag.AsChar(), actor->GetFriendlyName().AsChar() );
}

void CStorySceneActorMap::UnregisterSpeaker( CActor* actor, CName voiceTag )
{
	ASSERT( actor );
	//ASSERT( voiceTag );
	if ( voiceTag == CName::NONE )
	{
		SCENE_WARN( TXT( "Unregistering actor ('%ls') without voicetag" ), actor->GetDisplayName().AsChar() );
	}

	THandle< CActor > actorHandle( actor );

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

		// Find the list of speakers for given voice tag
		TDynArray< THandle< CActor > >* nodes = m_speakers.FindPtr( voiceTag );
		if ( nodes )
		{
			// Remove actor from the set
			if ( nodes->Remove( actorHandle ) )
			{
				//SCENE_LOG( TXT("Voicetag '%ls' unregistered by '%ls'"), voiceTag.AsChar(), actor->GetFriendlyName().AsChar() );
			}
		}
	}
}

void CStorySceneActorMap::UnregisterSpeaker( CActor* actor )
{
	ASSERT( actor );

	THandle< CActor > actorHandle( actor );

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

		// Remove from all sets
		for ( THashMap< CName, TDynArray< THandle< CActor > > >::iterator i = m_speakers.Begin(); i != m_speakers.End(); ++i )
		{
			i->m_second.Remove( actorHandle );
		}
	}
}

Bool CStorySceneActorMap::FindSpeakers( CName voiceTag, TDynArray< THandle< CEntity > >& actors ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

	// Find the group
	Bool actorFound = false;
	const TDynArray< THandle< CActor > >* nodes = m_speakers.FindPtr( voiceTag );
	if ( nodes )
	{
		for ( TDynArray< THandle< CActor > >::const_iterator i=nodes->Begin(); i!=nodes->End(); ++i )
		{
			actors.PushBack( (*i).Get() );
		}
		actorFound = true;
	}

	// Return true if there was at least one actor found
	return actorFound;
}

CActor* CStorySceneActorMap::FindClosestSpeaker( CName voiceTag, const Vector& position, Float searchRadius ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scheduleLock( m_lock );

	CActor* bestActor = NULL;

	// Search for the closest actor
	const TDynArray< THandle< CActor > >* nodes = m_speakers.FindPtr( voiceTag );
	if ( nodes )
	{
		Float bestRadius = searchRadius;

		// Search for the closest actor
		for ( TDynArray< THandle< CActor > >::const_iterator i=nodes->Begin(); i!=nodes->End(); ++i )
		{
			THandle< CActor > actorHandle = *i;
			CActor *pActor = actorHandle.Get();
			if ( pActor == NULL )
			{
				continue;
			}

			const Float distance = position.DistanceTo( pActor->GetWorldPosition() );
			if ( distance <= bestRadius )
			{
				bestRadius = distance;
				bestActor = pActor;
			}
		}
	}

	// Return best actor
	return bestActor;
}