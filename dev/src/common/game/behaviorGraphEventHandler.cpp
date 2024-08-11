#include "build.h"
#include "behaviorGraphEventHandler.h"

#define W3BEHAVIOR_EVENT_HANDLER_HANDLE_EVENT_CLASS( uniqueName )		m_## uniqueName ##_events.Handle( instance );



void CW3BehaviorGraphEventHandler::HandleDelayedEvents( CBehaviorGraphInstance* instance )
{
	W3BEHAVIOR_EVENT_HANDLER_HANDLE_EVENT_CLASS( CBehaviorGraphScriptStateNode );
	W3BEHAVIOR_EVENT_HANDLER_HANDLE_EVENT_CLASS( CBehaviorGraphScriptStateReportingNode );
	W3BEHAVIOR_EVENT_HANDLER_HANDLE_EVENT_CLASS( CBehaviorGraphScriptComponentStateNode );
}

void CW3BehaviorGraphEventHandler::LazyCreate( CBehaviorGraphInstance& instance )
{
	if ( !instance.GetEventHandler() )
	{
		instance.SetEventHandler( new CW3BehaviorGraphEventHandler() );
	}
}
