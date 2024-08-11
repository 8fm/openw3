/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventInterlocutor.h"

#include "idInterlocutor.h"
#include "idTopic.h"
#include "idInstance.h"
#include "eventRouterComponent.h"

IMPLEMENT_ENGINE_CLASS( CIdEventInterlocutor )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventInterlocutor::Activate( CIDTopicInstance* topicInstance )
{
	CIDInterlocutorComponent*	interlocutor	= topicInstance->GetDialogInstance()->GetInterlocutor( m_data.m_InterlocutorName );
	RaiseGeneralEvent( interlocutor->GetEntity(), m_data.m_EventName );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventInterlocutor::RaiseGeneralEvent( CEntity* entity, CName eventName )
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