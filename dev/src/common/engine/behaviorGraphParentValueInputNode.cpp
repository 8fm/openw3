/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphParentValueInputNode.h"
#include "behaviorGraphSocket.h"

#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphInstance.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphParentValueInputNode );

CBehaviorGraphParentValueInputNode::CBehaviorGraphParentValueInputNode()	
	: m_parentSocket( TXT("Input") )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphParentValueInputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

#endif

void CBehaviorGraphParentValueInputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Reset
	m_cachedParentValueNode = NULL;

	// Cache the input
	CBehaviorGraphNode *parent = SafeCast< CBehaviorGraphNode >( GetParent() );
	while ( parent && !m_cachedParentValueNode )
	{
		m_cachedParentValueNode = parent->CacheValueBlock( m_parentSocket.AsString().AsChar() );
		parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
	}
}

void CBehaviorGraphParentValueInputNode::SetParentInputSocket( const CName &name )
{
	m_parentSocket = name;
}

void CBehaviorGraphParentValueInputNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphParentValueInputNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphParentValueInputNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphParentValueInputNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphParentValueInputNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedParentValueNode )
	{
		return m_cachedParentValueNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphParentValueInputNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->Activate( instance );
	}
}

void CBehaviorGraphParentValueInputNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphParentValueInputNode::GetCaption() const
{ 
	return String::Printf( TXT("Input [%s]"), m_parentSocket.AsString().AsChar() ); 
}

#endif

Float CBehaviorGraphParentValueInputNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedParentValueNode )
	{
		return m_cachedParentValueNode->GetValue( instance );
	}

	return 0.0f;
}

void CBehaviorGraphParentValueInputNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedParentValueNode )
	{
		m_cachedParentValueNode->ProcessActivationAlpha( instance, alpha );
	}
}
