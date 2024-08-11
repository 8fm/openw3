/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphParentInputNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphParentInputNode );

CBehaviorGraphParentInputNode::CBehaviorGraphParentInputNode()	
	: m_parentSocket( TXT("Input") )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphParentInputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

#endif

void CBehaviorGraphParentInputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Reset
	m_cachedInputNode = NULL;

	// Cache the input
	CBehaviorGraphNode *parent = SafeCast< CBehaviorGraphNode >( GetParent() );
	while ( parent && !m_cachedInputNode )
	{
		m_cachedInputNode = parent->CacheBlock( m_parentSocket );
		parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
	}
}

void CBehaviorGraphParentInputNode::SetParentInputSocket( const CName &name )
{
	m_parentSocket = name;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphParentInputNode::GetTitleColor() const
{
	return Color( 255, 64, 64 );
}

String CBehaviorGraphParentInputNode::GetCaption() const
{ 
	return String::Printf( TXT("Input [%s]"), m_parentSocket.AsString().AsChar() ); 
}

#endif