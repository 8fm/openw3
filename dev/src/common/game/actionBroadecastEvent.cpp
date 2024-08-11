/**
* Copyright ©2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "actionBroadecastEvent.h"

#include "entityActionsRouter.h"

IMPLEMENT_ENGINE_CLASS( CActionBroadcastEvent );

void CActionBroadcastEvent::PerformOnEntity( CEntity* parent )
{
	IEntityActionsRouter* router = parent->AsEntityActionsRouter();
	if( router )
	{
		router->RouteEvent( m_eventToBrodecast , parent );
	}
}
