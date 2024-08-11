/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphParentVectorValueInputNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphInstance.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphParentVectorValueInputNode );

CBehaviorGraphParentVectorValueInputNode::CBehaviorGraphParentVectorValueInputNode()	
	: m_parentSocket( TXT("Input") )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphParentVectorValueInputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

#endif

void CBehaviorGraphParentVectorValueInputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Reset
	m_cachedParentVectorValueNode = NULL;

	// Cache the input
	CBehaviorGraphNode *parent = SafeCast< CBehaviorGraphNode >( GetParent() );
	while ( parent && !m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode = parent->CacheVectorValueBlock( m_parentSocket.AsString().AsChar() );
		parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
	}
}

void CBehaviorGraphParentVectorValueInputNode::SetParentInputSocket( const CName &name )
{
	m_parentSocket = name;
}

void CBehaviorGraphParentVectorValueInputNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphParentVectorValueInputNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphParentVectorValueInputNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphParentVectorValueInputNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphParentVectorValueInputNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		return m_cachedParentVectorValueNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphParentVectorValueInputNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->Activate( instance );
	}
}

void CBehaviorGraphParentVectorValueInputNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphParentVectorValueInputNode::GetCaption() const
{ 
	return String::Printf( TXT("Input [%s]"), m_parentSocket.AsString().AsChar() ); 
}

#endif

Vector CBehaviorGraphParentVectorValueInputNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedParentVectorValueNode )
	{
		return m_cachedParentVectorValueNode->GetVectorValue( instance );
	}

	return Vector::ZEROS;
}

void CBehaviorGraphParentVectorValueInputNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedParentVectorValueNode )
	{
		m_cachedParentVectorValueNode->ProcessActivationAlpha( instance, alpha );
	}
}
