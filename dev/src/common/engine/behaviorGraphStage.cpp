/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphStage.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStageNode );

CBehaviorGraphStageNode::CBehaviorGraphStageNode() 	
	: m_rootNode( NULL )
	, m_activeByDefault( true )
{	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStageNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_rootNode = SafeCast< CBehaviorGraphNode >( CreateChildNode( GraphBlockSpawnInfo( CBehaviorGraphOutputNode::GetStaticClass() ) ) );

	CreateAnimationInput( CNAME( Input ) );
}

#endif

void CBehaviorGraphStageNode::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	file << m_rootNode;
}

void CBehaviorGraphStageNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_stageActive;
}

void CBehaviorGraphStageNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_stageActive ] = true;
}

void CBehaviorGraphStageNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_stageActive );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStageNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		

	TBaseClass::OnRebuildSockets();
}

#endif

Bool CBehaviorGraphStageNode::IsStageActive( CBehaviorGraphInstance& instance ) const 
{ 
	return instance[ i_stageActive ]; 
}

void CBehaviorGraphStageNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( instance[ i_stageActive ] )  
	{
		// if stage is active, update it
		m_rootNode->Update( context, instance, timeDelta );		
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphStageNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( instance[ i_stageActive ] )  
	{
		// if stage is active, update it
		m_rootNode->Sample( context, instance, output );		
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}
}

void CBehaviorGraphStageNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( instance[ i_stageActive ] )
	{
		return m_rootNode->GetSyncInfo( instance, info );
	}
	else if ( m_cachedInputNode )
	{
		return m_cachedInputNode->GetSyncInfo( instance, info );
	}	
}

void CBehaviorGraphStageNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( instance[ i_stageActive ] )
	{
		return m_rootNode->SynchronizeTo( instance, info );
	}
	else if ( m_cachedInputNode )
	{
		return m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphStageNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_stageActive ] = m_activeByDefault;
}

Bool CBehaviorGraphStageNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( instance[ i_stageActive ] )
	{
		return m_rootNode->ProcessEvent( instance, event );
	}
	else if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphStageNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_stageActive ] )
	{
		ASSERT( m_rootNode );
		m_rootNode->Activate( instance );
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}	
}

void CBehaviorGraphStageNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_stageActive ] )
	{
		ASSERT( m_rootNode );
		ASSERT( m_rootNode->IsActive( instance ) );

		m_rootNode->Deactivate( instance );
	}
	else if ( m_cachedInputNode )
	{
		ASSERT( m_cachedInputNode->IsActive( instance ) );

		m_cachedInputNode->Deactivate( instance );
	}	
}

void CBehaviorGraphStageNode::ActivateStage( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_stageActive ] )
		return;

	instance[ i_stageActive ] = true; 

	// order does matter!
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	m_rootNode->Activate( instance );
}

void CBehaviorGraphStageNode::DeactivateStage( CBehaviorGraphInstance& instance ) const
{ 
	if ( !instance[ i_stageActive ] )
		return;

	instance[ i_stageActive ] = false; 

	// order does matter!
	m_rootNode->Deactivate( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );	
	}
} 

void CBehaviorGraphStageNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphStageNode::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Stage [ %s ]"), m_name.AsChar() );

	return String( TXT("Stage") );
}

#endif

void CBehaviorGraphStageNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( instance[ i_stageActive ] )
	{
		m_rootNode->ProcessActivationAlpha( instance, alpha );
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphStageNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_stageActive ] )
	{
		return m_rootNode->PreloadAnimations( instance );
	}
	else if ( m_cachedInputNode )
	{
		return m_cachedInputNode->PreloadAnimations( instance );
	}

	return true;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
