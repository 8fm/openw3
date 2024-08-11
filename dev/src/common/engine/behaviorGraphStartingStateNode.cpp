/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphStartingStateNode.h"
#include "../engine/entity.h"
#include "../engine/graphSocket.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFlowTransitionNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphFlowTransitionNode::GetClientColor() const
{
	return m_isEnabled ? Color( 172, 172, 140 ) : Color( 200, 172, 172 );
}

#endif

void CBehaviorGraphFlowTransitionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_currentTime ] < m_transitionTime )
	{
		BEH_WARN( TXT("Behavior sync is broken - actor '%ls' - instnace '%ls'"), 
			instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar(),
			instance.GetInstanceName().AsString().AsChar() );
	}

	TBaseClass::OnDeactivated( instance );
}

Bool CBehaviorGraphFlowTransitionNode::GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const
{
	if ( m_isEnabled == false )
	{
		return false;
	}

	if ( m_transitionCondition && !m_transitionCondition->Check( instance ) )
	{
		return false;
	}

	const CBehaviorGraphFlowConnectionNode* node = Cast< CBehaviorGraphFlowConnectionNode >( GetDestState() );
	return node ? node->GetSyncInfoForInstance( instance, info ) : false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFlowConnectionNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphFlowConnectionNode::GetCaption() const
{
	return !m_stateID.Empty() ? String::Printf( TXT("Flow state - %s"), m_stateID.AsString().AsChar() ) : TXT("Flow state");
}

Color CBehaviorGraphFlowConnectionNode::GetTitleColor() const
{
	return Color( 172, 172, 140 );
}

Bool CBehaviorGraphFlowConnectionNode::CanBeExpanded() const
{
	return false;
}

Bool CBehaviorGraphFlowConnectionNode::HasAnimation() const
{
	return m_animNode ? m_animNode->GetAnimationName() != CName::NONE : false;
}

void CBehaviorGraphFlowConnectionNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	ASSERT( m_rootNode );

	// Create anim node
	GraphBlockSpawnInfo spawnInfo( CBehaviorGraphAnimationNode::GetStaticClass() );

	m_animNode = SafeCast< CBehaviorGraphAnimationNode >( CreateChildNode( spawnInfo.GetClass() ) );

	// Connect anim node to root node
	if ( m_rootNode )
	{
		CGraphSocket* rootSocket = m_rootNode->CGraphBlock::FindSocket( CNAME( Input ) );
		CGraphSocket* animSocket = m_animNode->CGraphBlock::FindSocket( CNAME( Animation ) );

		if ( rootSocket && animSocket )
		{
			ASSERT( animSocket->CanConnectTo( rootSocket ) );
			animSocket->ConnectTo( rootSocket );
		}
		else
		{
			ASSERT( rootSocket && animSocket );
		}
	}
}

#endif

//#define DEBUG_BEH_FLOW

CBehaviorGraphFlowConnectionNode::CBehaviorGraphFlowConnectionNode()
	: m_animNode( NULL )
{
	
}

void CBehaviorGraphFlowConnectionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( FlowConnection );
	TBaseClass::OnUpdate( context, instance, timeDelta );

#ifdef DEBUG_BEH_FLOW
	ASSERT( m_animNode );
	if ( m_animNode )
	{
		BEH_LOG( TXT("Behavior synchronization - time '%.2f'"), m_animNode->GetAnimTime( instance ) );
	}
#endif
}

void CBehaviorGraphFlowConnectionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );
}

Bool CBehaviorGraphFlowConnectionNode::GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const
{
	if ( m_animNode )
	{
		info.m_id = m_stateID;
		info.m_animation = m_animNode->GetAnimationName();

#ifdef DEBUG_BEH_FLOW
		BEH_LOG( TXT("Behavior get synchronization - '%ls':"), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		BEH_LOG( TXT(" >instance: %s"), instance.GetInstanceName().AsChar() );
		BEH_LOG( TXT(" >id:   %s"), info.m_id.AsChar() );

		if ( info.m_animation != CName::NONE )
		{
			BEH_LOG( TXT(" >anim: %s"), info.m_animation.AsChar() );
		}
		else
		{
			BEH_LOG( TXT(" >anim: <EMPTY> !!!") );
		}

		BEH_LOG( TXT(" >time: %f"), info.m_time );
#endif

		return true;
	}

	return false;
}

Bool CBehaviorGraphFlowConnectionNode::CanSynchronizeIntanceTo( CBehaviorGraphInstance& instance, const CBehaviorSyncInfo& info ) const
{
	return m_animNode && info.m_id == m_stateID;
}

Bool CBehaviorGraphFlowConnectionNode::SynchronizeInstanceTo( CBehaviorGraphInstance& instance, const CBehaviorSyncInfo& info ) const
{
	if ( CanSynchronizeIntanceTo( instance, info ) )
	{
		m_animNode->SetTempRuntimeAnimationName( instance, info.m_animation );
		m_animNode->SetAnimTime( instance, info.m_time );

#ifdef DEBUG_BEH_FLOW
		BEH_LOG( TXT("Behavior set synchronization - '%ls':"), instance.GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		BEH_LOG( TXT(" >instance: %s"), instance.GetInstanceName().AsChar() );
		BEH_LOG( TXT(" >from: %s"), info.m_instanceName.AsChar() );
		BEH_LOG( TXT(" >id:   %s"), info.m_id.AsChar() );

		if ( info.m_animation != CName::NONE )
		{
			BEH_LOG( TXT(" >anim: %s"), info.m_animation.AsChar() );
		}
		else
		{
			BEH_LOG( TXT(" >anim: <EMPTY> !!!") );
		}

		BEH_LOG( TXT(" >time: %f"), info.m_time );

		Uint32 transNum = m_cachedStateTransitions.Size();
		BEH_LOG( TXT(" >trans num: %d"), transNum );

		for ( Uint32 i=0; i<m_cachedStateTransitions.Size(); ++i )
		{
			CBehaviorGraphStateTransitionNode* trans = m_cachedStateTransitions[ i ];
			ASSERT( trans );

			String syncName;

			CBehaviorGraphFlowTransitionNode* flowTrans = Cast< CBehaviorGraphFlowTransitionNode >( trans );
			if ( flowTrans )
			{
				const IBehaviorSyncMethod* syncMethod = flowTrans->GetSyncMethod();
				if ( syncMethod )
				{
					if ( syncMethod->IsA< CBehaviorSyncMethodEventStart >() )
					{
						const CBehaviorSyncMethodEventStart* syncEvent = static_cast< const CBehaviorSyncMethodEventStart* >( syncMethod );
						syncName = String::Printf( TXT("Event start - %s"), syncEvent->GetEventName().AsChar() );
					}
					else if ( syncMethod->IsA< CBehaviorSyncMethodEventProp >() )
					{
						const CBehaviorSyncMethodEventProp* syncEvent = static_cast< const CBehaviorSyncMethodEventProp* >( syncMethod );
						syncName = String::Printf( TXT("Event prop - %s"), syncEvent->GetEventName().AsChar() );
					}
					else if ( syncMethod->IsA< CBehaviorSyncMethodDuration >() )
					{
						syncName = TXT("Duration");
					}
					else if ( syncMethod->IsA< CBehaviorSyncMethodTime >() )
					{
						syncName = TXT("Time");
					}
					else
					{
						syncName = syncMethod->GetClass()->GetName().AsString();
					}
				}
				else
				{
					syncName = TXT("<no sync>");
				}
			}
			else
			{
				syncName = TXT("<not flow node>");
			}

			BEH_LOG( TXT(" >trans [%d]: %s"), i, syncName.AsChar() );
		}
#endif

		return true;
	}

	return false;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif

