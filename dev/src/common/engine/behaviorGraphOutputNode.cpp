/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphNode.h"
#include "behaviorGraph.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphInstance.h"
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOutputNode );

CBehaviorGraphOutputNode::CBehaviorGraphOutputNode()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphOutputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	

	// Custom float tracks
	for( Uint32 i=0; i<SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS; ++i )
	{
		const CName socketName = GetGraph()->GetCustomTrackName(i);
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( socketName, false ) );		
	}

	SetCustomFloatSocketCaptions();

	// Float tracks
	for( Uint32 i=0; i<SBehaviorGraphOutput::FFT_Last; ++i )
	{
		const CName socketName = GetGraph()->GetFloatTrackName(i);
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( socketName, false ) );
	}

	SetFloatSocketCaptions();
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphOutputNode::SetCustomFloatSocketCaptions()
{
	const CBehaviorGraph *graph = GetGraph();
	if ( !graph )
	{
		return;
	}

	for( Uint32 i=0; i<SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS; ++i )
	{
		const CName socketName = GetGraph()->GetCustomTrackName(i);
		CBehaviorGraphVariableInputSocket* socket = Cast< CBehaviorGraphVariableInputSocket >( CGraphBlock::FindSocket( socketName  ) );
		if ( socket )
		{															 
			socket->SetCaption( graph->GetCustomTrackNameStr( i ) );
		}
	}
}

void CBehaviorGraphOutputNode::SetFloatSocketCaptions()
{
	const CBehaviorGraph *graph = GetGraph();
	if ( !graph )
	{
		return;
	}

	for( Uint32 i=0; i<SBehaviorGraphOutput::FFT_Last; ++i )
	{
		const CName socketName = GetGraph()->GetFloatTrackName(i);
		CBehaviorGraphVariableInputSocket* socket = Cast< CBehaviorGraphVariableInputSocket >( CGraphBlock::FindSocket( socketName ) );
		if ( socket )
		{															 
			socket->SetCaption( graph->GetFloatTrackNameStr( i ) );
		}
	}
}

#endif

void CBehaviorGraphOutputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedInputNode = CacheBlock( TXT("Input") );

	// Clear
	m_cachedCustomInputNodes.Clear();
	m_cachedFloatInputNodes.Clear();

	// Cache float inputs
	COMPILE_ASSERT( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5);
	const Uint32 numFloats = SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS;
	for ( Uint32 i=0; i<numFloats; i++ )
	{
		// Custom socket
		const String customSocketName = GetGraph()->GetCustomTrackName(i).AsString();
		m_cachedCustomInputNodes.PushBack( CacheValueBlock( customSocketName ) );
	}

	for ( Uint32 i=0; i<SBehaviorGraphOutput::FFT_Last; i++ )
	{
		// Float track sockets
		const String floatSocketName = GetGraph()->GetFloatTrackName(i).AsString();
		m_cachedFloatInputNodes.PushBack( CacheValueBlock( floatSocketName ) );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphOutputNode::GetTitleColor() const
{
	return Color( 64, 255, 64 );
}

#endif

void CBehaviorGraphOutputNode::OnUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Output );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	for ( Uint32 i=0; i<m_cachedCustomInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedCustomInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Update( context, instance, timeDelta );
		}
	}

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphOutputNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( Output );

	ANIM_NODE_PRE_SAMPLE

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	for ( Uint32 i=0; i<m_cachedCustomInputNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* valueNode = m_cachedCustomInputNodes[ i ];
		if ( valueNode )
		{
			output.m_customFloatTracks[ i ] = valueNode->GetValue( instance );
		}
	}

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode && output.m_numFloatTracks > i )
		{
			if ( valueNode->GetValue( instance ) > 0.01f && output.m_floatTracks[i] < 0.01f )
			{
				output.m_floatTracks[i] = valueNode->GetValue( instance );
			}
		}
	}

	ANIM_NODE_POST_SAMPLE
}

Bool CBehaviorGraphOutputNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->PreloadAnimations( instance );
	}

	return true;
}

void CBehaviorGraphOutputNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
 	if ( m_cachedInputNode )
 	{
 		m_cachedInputNode->GetSyncInfo( instance, info );
 	}
}

void CBehaviorGraphOutputNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphOutputNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphOutputNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	for ( Uint32 i=0; i<m_cachedCustomInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedCustomInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Activate( instance );
		}
	}

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Activate( instance );
		}
	}
}

void CBehaviorGraphOutputNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	for ( Uint32 i=0; i<m_cachedCustomInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedCustomInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Deactivate( instance );
		}
	}

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Deactivate( instance );
		}
	}
}

void CBehaviorGraphOutputNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	for ( Uint32 i=0; i<m_cachedCustomInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedCustomInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->ProcessActivationAlpha( instance, alpha );
		}
	}

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOverrideFloatTracksNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphOverrideFloatTracksNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	for( Uint32 i=0; i<SBehaviorGraphOutput::FFT_Last; ++i )
	{
		const CName socketName = GetGraph()->GetFloatTrackName(i);
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( socketName, false ) );
	}
}

#endif

CBehaviorGraphOverrideFloatTracksNode::CBehaviorGraphOverrideFloatTracksNode()
	: m_overrideZeros( false )
{

}

void CBehaviorGraphOverrideFloatTracksNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedFloatInputNodes.Clear();

	for ( Uint32 i=0; i<SBehaviorGraphOutput::FFT_Last; i++ )
	{
		// Float track sockets
		const String floatSocketName = GetGraph()->GetFloatTrackName(i).AsString();
		m_cachedFloatInputNodes.PushBack( CacheValueBlock( floatSocketName ) );
	}
}

void CBehaviorGraphOverrideFloatTracksNode::OnUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( OverrideFloatTracks );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphOverrideFloatTracksNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphValueNode* valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode && output.m_numFloatTracks > i )
		{
			if ( valueNode->GetValue( instance ) > 0.f || m_overrideZeros == true )
			{
				output.m_floatTracks[i] = valueNode->GetValue( instance );
			}
		}
	}
}

void CBehaviorGraphOverrideFloatTracksNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Activate( instance );
		}
	}
}

void CBehaviorGraphOverrideFloatTracksNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->Deactivate( instance );
		}
	}
}

void CBehaviorGraphOverrideFloatTracksNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	for( Uint32 i=0; i<m_cachedFloatInputNodes.Size(); ++i )
	{
		CBehaviorGraphNode *valueNode = m_cachedFloatInputNodes[ i ];
		if ( valueNode )
		{
			valueNode->ProcessActivationAlpha( instance, alpha );
		}
	}
}
