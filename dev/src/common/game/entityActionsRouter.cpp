/**
* Copyright ©2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "entityActionsRouter.h"
#include "../engine/performableAction.h"

IMPLEMENT_ENGINE_CLASS( SEntityActionsRouterEntry );

void IEntityActionsRouter::RouteEvent( CName eventName, CEntity* parent )
{
	for( Uint32 i=0; i<m_events.Size(); ++i )
	{
		if( m_events[ i ].m_eventName == eventName )
		{
			TDynArray< IPerformableAction* >& actions = m_events[ i ].m_actionsToPerform;
			for( Uint32 j=0; j<actions.Size(); ++j )
			{
				if( actions[ j ] )
				{
					actions[ j ]->Perform( parent );
				}
			}
		}
	}
}
