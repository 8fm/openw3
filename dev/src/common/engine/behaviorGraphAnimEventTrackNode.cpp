/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimEventTrackNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animationEvent.h"
#include "extAnimEvent.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimEventTrackNode );

CBehaviorGraphAnimEventTrackNode::CBehaviorGraphAnimEventTrackNode()
{

}

void CBehaviorGraphAnimEventTrackNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
}

void CBehaviorGraphAnimEventTrackNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = 0.f;
}

void CBehaviorGraphAnimEventTrackNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
}

Float CBehaviorGraphAnimEventTrackNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_value ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimEventTrackNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Anim Event Track - %s"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Anim Event Track") );
	}
}

#endif

void CBehaviorGraphAnimEventTrackNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	for ( Uint32 i = 0; i < output.m_numEventsFired; ++i )
	{
		ASSERT( output.m_eventsFired[i].m_extEvent != NULL );
		
		if ( output.m_eventsFired[i].m_extEvent->GetEventName() == m_eventName )
		{
			instance[ i_value ] = output.m_eventsFired[i].m_alpha;
			return;
		}
	}

	instance[ i_value ] = 0.0f;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimEventTrackNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );		
}

#endif

void CBehaviorGraphAnimEventTrackNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimEventTrack );
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAnimEventTrackNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = 0.0f;
}

void CBehaviorGraphAnimEventTrackNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimEventTrackNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphAnimEventTrackNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphAnimEventTrackNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphAnimEventTrackNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimEventTrackNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphAnimEventTrackNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}
