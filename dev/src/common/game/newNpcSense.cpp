/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpcSense.h"

const Float CNewNPCSense::UPDATE_KNOWLEDGE_INTERVAL = 0.5f;
const Float CNewNPCSense::PLAYER_CHECK_INTERVAL = 0.5f;
const Float CNewNPCSense::NPC_CHECK_INTERVAL = 1.0f;

CNewNPCSense::~CNewNPCSense()
{
}

void CNewNPCSense::ForceUpdateKnowledge()
{
	m_updateKnowladgeTimer = -1.0f;
	m_timeToPlayerCheck = -1.0f;
	m_timeToNPCCheck = -1.0f;
}

NewNPCSenseUpdateFlags CNewNPCSense::UpdateKnowledge( Float timeDelta, TNoticedObjects& noticedObjects )
{
	if ( m_npc == nullptr || !m_npc->IsAlive() )
	{
		// If dead clear objects
		if ( noticedObjects.Size() > 0 )
		{
			noticedObjects.ClearFast();
			return FLAG_NOTICED_OBJECTS_VANISHED;
		}
		return FLAG_NO_CHANGES;
	}

	m_updateKnowladgeTimer -= timeDelta;
	if ( m_updateKnowladgeTimer > 0.0f  )
	{
		return FLAG_NO_CHANGES;
	}
	m_updateKnowladgeTimer = UPDATE_KNOWLEDGE_INTERVAL;
	
	NewNPCSenseUpdateFlags changed = FLAG_NO_CHANGES;
	const EngineTime& currentTime = GGame->GetEngineTime();
	Float objectLoseTime = GGame->GetAIObjectLoseTime();
	
	for ( Int32 i = static_cast< Int32 >( noticedObjects.Size() ) - 1; i >= 0; i-- )
	{
		NewNPCNoticedObject& obj = noticedObjects[i];
		obj.SetIsVisible( false );
		CActor* actor = obj.m_actorHandle.Get();
		if ( actor != nullptr )
		{
			if ( actor->IsAlive() )
			{
				if ( currentTime - obj.m_lastNoticedTime < objectLoseTime )
				{
					continue;
				}
			}
		}
		noticedObjects.RemoveAt( i );
		changed = FLAG_NOTICED_OBJECTS_VANISHED;
	}

	return changed;
}

NewNPCSenseUpdateFlags CNewNPCSense::EndUpdate( TNoticedObjects& noticedObjects, NewNPCSenseUpdateFlags updated, Float timeDelta )
{
	m_timeToPlayerCheck -= timeDelta;
	if ( m_timeToPlayerCheck < 0.0f )
	{
		m_timeToPlayerCheck = PLAYER_CHECK_INTERVAL;
	}

	m_timeToNPCCheck -= timeDelta;
	if ( m_timeToNPCCheck < 0.0f )
	{
		m_timeToNPCCheck = NPC_CHECK_INTERVAL;
	}

	if ( updated & FLAG_NOTICED_OBJECTS_APPEARS )
	{
		m_npc->SignalGameplayEvent( CNAME( NoticedObjectReevaluation ) );
	}

	return updated;
}
