/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphVariableBaseNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVariableBaseNode );

CBehaviorGraphVariableBaseNode::CBehaviorGraphVariableBaseNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphVariableBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );		
}

#endif
