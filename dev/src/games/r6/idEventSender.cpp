#include "build.h"
#include "idEventSender.h"
/*
#include "idEventSenderDataStructs.h"

#include "idInterlocutor.h"
#include "idTopic.h"
#include "idInstance.h"
#include "eventRouterComponent.h"

#include "../../common/game/commonGame.h"
#include "../../common/game/factsDB.h"

#include "../../common/game/questStoryPhaseProperty.h"
#include "../../common/game/encounter.h"
*/

IMPLEMENT_ENGINE_CLASS( CEventSender )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::ActivateAll( CIDTopicInstance* topicInstance )
{
	for( TDynArray< CIdEvent* >::iterator it	= m_Events.Begin(); it != m_Events.End(); ++it )
	{
		(*it)->Activate( topicInstance );
	}

	/*
	AddFacts();
	RemoveFacts();
	RaiseGeneralEvents( );
	RaiseInterlocutorEvents( topicInstance );
	RaiseAIEvents( topicInstance );
	RaiseAnimationEvents( topicInstance );
	RaiseQuestSpawnsets();
	RaiseEncounterSets();
	*/
}
/*
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::AddFacts( )
{
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
	{
		for( TDynArray< String >::iterator it = m_FactsToAdd.Begin(); it != m_FactsToAdd.End(); ++it )
		{
			String	l_Name	= *it;
			GCommonGame->GetSystem< CFactsDB >()->AddFact( l_Name, 1, -1 );
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RemoveFacts( )
{
	if ( GCommonGame && GCommonGame->GetSystem< CFactsDB >() )
	{
		for( TDynArray< String >::iterator it = m_FactsToAdd.Begin(); it != m_FactsToAdd.End(); ++it )
		{
			String	l_Name	= *it;
			GCommonGame->GetSystem< CFactsDB >()->RemoveFact( l_Name );
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseGeneralEvents()
{
	for( TDynArray< SGeneralEventData >::iterator it	= m_GeneralEvents.Begin(); it != m_GeneralEvents.End(); ++it )
	{
		SGeneralEventData*	l_Data	= it;
		CEntity*			entity	= l_Data->m_EntityHandle.Get();
		RaiseGeneralEvent( entity, l_Data->m_EventName );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseGeneralEvent( CEntity* entity, CName eventName )
{
	if( entity == NULL )
	{
		return;
	}

	// Call on entity
	entity->CallEvent( eventName );

	// Call on components
	CEventRouterComponent* router = entity->FindComponent< CEventRouterComponent >();
	if( router )
	{
		router->RouteEvent( eventName );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseInterlocutorEvents( CIDTopicInstance* topicInstance )
{
	for( TDynArray< SInterlocutorEventData >::iterator it	= m_GeneralInterlocutorEvents.Begin(); it != m_GeneralInterlocutorEvents.End(); ++it )
	{
		SInterlocutorEventData*		l_Data			= it;
		CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( l_Data->m_InterlocutorName );
		RaiseGeneralEvent( interlocutor->GetEntity(), l_Data->m_EventName );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseAIEvents( CIDTopicInstance* topicInstance )
{
	for( TDynArray< SAIEventData >::iterator it	= m_AIEvents.Begin(); it != m_AIEvents.End(); ++it )
	{
		SAIEventData*	l_Data	= it;
		CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( l_Data->m_InterlocutorName );
		interlocutor->RaiseAIEvent( l_Data );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseAnimationEvents( CIDTopicInstance* topicInstance )
{
	for( TDynArray< SAnimationEventData >::iterator it	= m_AnimationEvents.Begin(); it != m_AnimationEvents.End(); ++it )
	{
		SAnimationEventData*	l_Data	= it;
		CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( l_Data->m_InterlocutorName );
		interlocutor->RaiseBehaviorEvent( l_Data );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseQuestSpawnsets()
{
	// activate a community story phase
	for ( TDynArray< IQuestSpawnsetAction* >::const_iterator it = m_QuestSpawnsets.Begin();
		it != m_QuestSpawnsets.End(); ++it )
	{
		const IQuestSpawnsetAction* action = *it;
		if ( action )
		{
			action->Perform();
		}
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CEventSender::RaiseEncounterSets()
{
	CWorld* world = GGame->GetActiveWorld() ;
	if ( world )
	{
		for ( TDynArray< SEncounterPhaseData >::const_iterator it = m_EncounterSpawnsets.Begin();
			it != m_EncounterSpawnsets.End(); ++it )
		{
			SEncounterPhaseData encounterData	= *it;
			CEntity* entity = world->GetTagManager()->GetTaggedEntity( encounterData.m_encounterTag );
			if ( entity && entity->IsA< CEncounter >() )
			{
				CEncounter* encounter = static_cast< CEncounter* >( entity );
				encounter->SetSpawnPhase( encounterData.m_encounterSpawnPhase );
			}
		}
	}
}
*/