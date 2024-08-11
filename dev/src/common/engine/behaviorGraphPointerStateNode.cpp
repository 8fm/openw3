
#include "build.h"
#include "behaviorGraphPointerStateNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphNode.h"
#include "behaviorGraph.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointerTransitionNode );

void CBehaviorGraphPointerTransitionNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	CBehaviorGraphStateNode* state = NULL;

	CBehaviorGraphPointerStateNode* pointerState = Cast< CBehaviorGraphPointerStateNode >( m_cachedEndStateNode );
	if ( pointerState )
	{
		const String& pointedStateName = pointerState->GetPointedStateName();

		CBehaviorGraphNode* node = GetGraph()->FindNodeByName( pointedStateName, GetParentNode(), false );
		if ( node )
		{
			state = Cast< CBehaviorGraphStateNode >( node );
			if ( state )
			{
				m_cachedEndStateNode = state;
			}
		}
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	m_pointerState = pointerState;
	if ( m_pointerState )
	{
		m_pointerState->SetPointedState( state );
	}
#endif

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointerStateNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphPointerStateNode::GetCaption() const
{
	String pointedStateName;

	if ( m_pointedState )
	{
		pointedStateName = m_pointedStateName;
	}
	else
	{
		if ( !m_pointedStateName.Empty() )
		{
			pointedStateName = m_pointedStateName + TXT(" [nt]");
		}
		else
		{
			pointedStateName = TXT("None");
		}
	}

	return String::Printf( TXT("{%s}"), pointedStateName.AsChar() );
}

void CBehaviorGraphPointerStateNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphStateInSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphStateOutSocketSpawnInfo( CNAME( Out ), false ) );
}

Color CBehaviorGraphPointerStateNode::GetTitleColor() const
{
	return Color( 171, 224, 27 );
}

Bool CBehaviorGraphPointerStateNode::CanBeExpanded() const
{
	return false;
}

#endif


void CBehaviorGraphPointerStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	
}

void CBehaviorGraphPointerStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	
}

Bool CBehaviorGraphPointerStateNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	return true;
}

void CBehaviorGraphPointerStateNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	
}

void CBehaviorGraphPointerStateNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	
}

Bool CBehaviorGraphPointerStateNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

Bool CBehaviorGraphPointerStateNode::GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const
{
	return false;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
