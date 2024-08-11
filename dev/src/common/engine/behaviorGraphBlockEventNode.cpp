/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphBlockEventNode.h"
#include "animationEvent.h"
#include "behaviorGraphOutput.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlockEventNode );

///////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphBlockEventNode::GetCaption() const 
{ 
	return String::Printf( TXT( "Block anim event - %s" ), m_eventToBlock.AsString().AsChar() );
}

#endif

void CBehaviorGraphBlockEventNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	CAnimationEventFired* firedEvent = output.m_eventsFired;
	for ( Int32 idx = 0; idx < (Int32)output.m_numEventsFired; ++ idx, ++ firedEvent )
	{
		if ( firedEvent->GetEventName() == m_eventToBlock )
		{
			// this may look strange, but let me explain, what is going on here
			// first we decrement number of fired events and this new value will also point at last event we had
			-- output.m_numEventsFired;
			// we move that last event to replace our current event (if it is the same, it is still faster than branching)
			output.m_eventsFired[ idx ] = output.m_eventsFired[ output.m_numEventsFired ];
			// we move back a little bit (this is Int, not Uint, so we're safe with 0 -> -1) to check event we placed again
			-- idx;
			// and don't forget our pointer :)
			-- firedEvent;
		}
	}
}
