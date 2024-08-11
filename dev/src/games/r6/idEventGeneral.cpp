/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventGeneral.h"
#include "idEventSenderDataStructs.h"
#include "eventRouterComponent.h"


IMPLEMENT_ENGINE_CLASS( CIdEventGeneral )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventGeneral::Activate( CIDTopicInstance* topicInstance )
{
	CEntity*			entity	= m_data.m_EntityHandle.Get();
	RaiseGeneralEvent( entity, m_data.m_EventName );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventGeneral::RaiseGeneralEvent( CEntity* entity, CName eventName )
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