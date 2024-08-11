/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphTopLevelNode.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphInstance.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphTopLevelNode );

CBehaviorGraphTopLevelNode::CBehaviorGraphTopLevelNode()
	: m_rootNode( NULL )
{	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphTopLevelNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	m_rootNode = SafeCast< CBehaviorGraphNode >( CreateChildNode( GraphBlockSpawnInfo( CBehaviorGraphOutputNode::GetStaticClass() ) ) );
}

#endif

void CBehaviorGraphTopLevelNode::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	file << m_rootNode;
}

void CBehaviorGraphTopLevelNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	ASSERT( m_rootNode );
	m_rootNode->Update( context, instance, timeDelta );
}

void CBehaviorGraphTopLevelNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;
	ASSERT( m_rootNode );
	m_rootNode->Sample( context, instance, output );
}

Bool CBehaviorGraphTopLevelNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	ASSERT( m_rootNode );
	return m_rootNode->ProcessEvent( instance, event );
}

void CBehaviorGraphTopLevelNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	ASSERT( m_rootNode );
	m_rootNode->GetSyncInfo( instance, info );
}

void CBehaviorGraphTopLevelNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	ASSERT( m_rootNode );
	m_rootNode->SynchronizeTo( instance, info );
}

void CBehaviorGraphTopLevelNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	ASSERT( m_rootNode );
	m_rootNode->Activate( instance );
}

void CBehaviorGraphTopLevelNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	ASSERT( m_rootNode );
	m_rootNode->Deactivate( instance );
}

void CBehaviorGraphTopLevelNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );
	ASSERT( m_rootNode );
	m_rootNode->ProcessActivationAlpha( instance, alpha );
}

void CBehaviorGraphTopLevelNode::CacheConnections()
{
	TBaseClass::CacheConnections();
	ASSERT( m_rootNode );
	m_rootNode->CacheConnections();
}

void CBehaviorGraphTopLevelNode::RemoveConnections()
{
	TBaseClass::RemoveConnections();
	ASSERT( m_rootNode );
	m_rootNode->RemoveConnections();
}