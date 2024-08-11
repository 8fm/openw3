/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphGetCustomTrackNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGetCustomTrackNode );

CBehaviorGraphGetCustomTrackNode::CBehaviorGraphGetCustomTrackNode()
	: m_trackIndex( 0 )
	, m_defaultValue( 0.0f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphGetCustomTrackNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Value ) ) );
}

String CBehaviorGraphGetCustomTrackNode::GetCaption() const
{	
	String trackName = GetGraph() ? GetGraph()->GetCustomTrackNameStr( m_trackIndex ) : String::EMPTY;

	if ( trackName.Empty() )
	{
		return String::Printf( TXT("Custom track %d"), m_trackIndex );
	}
	else
	{
		return String::Printf( TXT("Custom track [ %s ]"), trackName.AsChar() );
	}
}

void CBehaviorGraphGetCustomTrackNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == CNAME( trackIndex ) )
	{
		if ( m_trackIndex >= SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS )
		{
			m_trackIndex = SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS - 1;
		}
	}
}

#endif

void CBehaviorGraphGetCustomTrackNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->OnUpdate( context, instance, timeDelta );
	}
}

void CBehaviorGraphGetCustomTrackNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Sample( context, instance, output );
	}

	instance[ i_value ] = GetValueFromTrack( output );	
}

Float CBehaviorGraphGetCustomTrackNode::GetValueFromTrack( const SBehaviorGraphOutput& pose ) const
{
	ASSERT( m_trackIndex < SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS );
	return pose.m_customFloatTracks[ m_trackIndex ];
}

void CBehaviorGraphGetCustomTrackNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_value ] = m_defaultValue;
}

void CBehaviorGraphGetCustomTrackNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedAnimInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphGetCustomTrackNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Activate( instance );
	}
}

void CBehaviorGraphGetCustomTrackNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphGetCustomTrackNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphGetCustomTrackNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphGetCustomTrackNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedAnimInputNode )
	{
		m_cachedAnimInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphGetCustomTrackNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedAnimInputNode )
	{
		return m_cachedAnimInputNode->ProcessEvent( instance, event );
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGetFloatTrackNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphGetFloatTrackNode::GetCaption() const
{	
	String trackName = GetGraph() ? GetGraph()->GetFloatTrackNameStr( m_trackIndex ) : String::EMPTY;

	if ( trackName.Empty() )
	{
		return String::Printf( TXT("Float track %d"), m_trackIndex );
	}
	else
	{
		return String::Printf( TXT("Float track [ %s ]"), trackName.AsChar() );
	}
}

void CBehaviorGraphGetFloatTrackNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == CNAME( trackIndex ) )
	{
		if ( m_trackIndex >= SBehaviorGraphOutput::FFT_Last )
		{
			m_trackIndex = SBehaviorGraphOutput::FFT_Last - 1;
		}
	}
}

#endif

Float CBehaviorGraphGetFloatTrackNode::GetValueFromTrack( const SBehaviorGraphOutput& pose ) const
{
	ASSERT( m_trackIndex < SBehaviorGraphOutput::FFT_Last );
	return m_trackIndex < (Int32)pose.m_numFloatTracks ? pose.m_floatTracks[ m_trackIndex ] : 0.f;
}
