/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animSyncInfo.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

class CBehaviorGraphStateNode;

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionBlendNode );

CBehaviorGraphStateTransitionBlendNode::CBehaviorGraphStateTransitionBlendNode()
	: m_transitionTime( 0.2f )
	, m_synchronize( true )
	, m_syncMethod( NULL )
	, m_motionBlendType( BTBM_Blending )
{
}

void CBehaviorGraphStateTransitionBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currentTime;
}

void CBehaviorGraphStateTransitionBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currentTime ] = 0.f;
}

void CBehaviorGraphStateTransitionBlendNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_currentTime );
}

String CBehaviorGraphStateTransitionBlendNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Blend transition [%s]"), m_name.AsChar() );
	}
	else
	{
		return String::EMPTY;
	}
}

void CBehaviorGraphStateTransitionBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( TransitionBlend );

	instance[ i_currentTime ] += timeDelta;

	// synchronize children playback
	Synchronize( instance, timeDelta );

	{
		if ( m_cachedStartStateNode && !m_cachedStartStateNode->IsActive( instance ) )
		{
			BEH_DUMP_ERROR( instance, TXT("CBehaviorGraphStateTransitionBlendNode::OnUpdate - !m_cachedStartStateNode->IsActive( instance ) ") );

			if ( m_cachedEndStateNode )
			{
				BEH_ERROR( TXT("CBehaviorGraphStateTransitionBlendNode: From %s To %s"), m_cachedStartStateNode->GetName().AsChar(), m_cachedEndStateNode->GetName().AsChar()  );
			}

			ASSERT( !m_cachedStartStateNode->IsActive( instance ) );
			m_cachedStartStateNode->Activate( instance );
		}

		if ( m_cachedEndStateNode && !m_cachedEndStateNode->IsActive( instance ) )
		{
			ASSERT( m_cachedEndStateNode->IsActive( instance ) );
			m_cachedEndStateNode->Activate( instance );
		}
	}

	// update start state
	if ( m_cachedStartStateNode )
	{
		m_cachedStartStateNode->Update( context, instance, timeDelta );
	}

	// update end state
	if ( m_cachedEndStateNode )
	{
		m_cachedEndStateNode->Update( context, instance, timeDelta );
	}

	// transition has finished
	if ( instance[ i_currentTime ] >= m_transitionTime )
	{
		CBehaviorGraphStateMachineNode *stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );

		DisableSynchronization( instance );

		Deactivate( instance );

		ASSERT( m_cachedEndStateNode );

		stateMachine->SwitchToState( m_cachedEndStateNode, instance );

		instance[ i_currentTime ] = 0.0f;
	}
}

void CBehaviorGraphStateTransitionBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( TransitionBlend );

	Float alpha = GetAlpha( instance );

	CCacheBehaviorGraphOutput cachePose1( context );
	CCacheBehaviorGraphOutput cachePose2( context );

	SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
	SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

	if ( temp1 && temp2 )
	{
		// Sample start state
		if ( m_cachedStartStateNode ) 
		{
			m_cachedStartStateNode->Sample( context, instance, *temp1 );
		}

		// sample end state
		if ( m_cachedEndStateNode )
		{
			m_cachedEndStateNode->Sample( context, instance, *temp2 );
		}

		// Interpolate poses
		InterpolatePoses( context, instance, output, *temp1, *temp2, alpha );

		// Merge events - take events only for the destination pose, as the one
		// we're blending out from is invalid for us in terms of the events it's emmiting
		output.MergeEvents( *temp2, 1.0f );
		// But blend used anims from both (to know what composed final pose)
		output.MergeUsedAnims( *temp1, *temp2, 1.0f - alpha, alpha );

		// Motion extraction blending
		if ( m_motionBlendType == BTBM_Source )
		{
			output.m_deltaReferenceFrameLocal = temp1->m_deltaReferenceFrameLocal;
		}
		else if ( m_motionBlendType == BTBM_Destination )
		{
			output.m_deltaReferenceFrameLocal = temp2->m_deltaReferenceFrameLocal;
		}
	}
}

void CBehaviorGraphStateTransitionBlendNode::InterpolatePoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const
{
#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() <= BL_Lod2 )
	{
		output.SetInterpolate( poseA, poseB, alpha );
	}
	else
	{
		output.SetInterpolateME( poseA, poseB, alpha );
	}
#else
	output.SetInterpolate( poseA, poseB, alpha );
#endif
}

void CBehaviorGraphStateTransitionBlendNode::DisableSynchronization( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedStartStateNode )
	{
		m_cachedStartStateNode->GetSyncData( instance ).Reset();
	}

	if ( m_cachedEndStateNode )
	{
		m_cachedEndStateNode->GetSyncData( instance ).Reset();
	}
}

void CBehaviorGraphStateTransitionBlendNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
//	const Float currentTime = instance[ i_currentTime ];

	if ( m_synchronize && m_syncMethod && m_cachedStartStateNode && m_cachedEndStateNode )
	{
		// Interpolate results
		const Float alpha = GetAlpha( instance );
		m_syncMethod->Synchronize( instance, m_cachedStartStateNode, m_cachedEndStateNode, alpha, timeDelta );
	}
	else
	{
		if ( m_cachedStartStateNode )
		{
			m_cachedStartStateNode->GetSyncData( instance ).Reset();
		}

		if ( m_cachedEndStateNode )
		{
			m_cachedEndStateNode->GetSyncData( instance ).Reset();
		}
	}
}

void CBehaviorGraphStateTransitionBlendNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
//	const Float currentTime = instance[ i_currentTime ];

	CSyncInfo firstSyncInfo;
	CSyncInfo secondSyncInfo;

	if ( m_cachedStartStateNode ) 
	{
		m_cachedStartStateNode->GetSyncInfo( instance, firstSyncInfo );
	}

	if ( m_cachedEndStateNode )
	{
		m_cachedEndStateNode->GetSyncInfo( instance, secondSyncInfo );
	}

	// Interpolate results
	const Float alpha = GetAlpha( instance );
	info.SetInterpolate( firstSyncInfo, secondSyncInfo, alpha );
}

void CBehaviorGraphStateTransitionBlendNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_synchronize && m_syncMethod )
	{
		if ( m_cachedStartStateNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedStartStateNode, info );
		}

		if ( m_cachedEndStateNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedEndStateNode, info );
		}
	}
	else
	{
		if ( m_cachedStartStateNode ) 
		{
			m_cachedStartStateNode->SynchronizeTo( instance, info );
		}

		if ( m_cachedEndStateNode )
		{
			m_cachedEndStateNode->SynchronizeTo( instance, info );
		}
	}
}

void CBehaviorGraphStateTransitionBlendNode::OnReset( CBehaviorGraphInstance& instance ) const
{ 
	TBaseClass::OnReset( instance );

	instance[ i_currentTime ] = 0.0f;
}

void CBehaviorGraphStateTransitionBlendNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_currentTime ] = 0.0f;

	if ( m_cachedEndStateNode )
	{
		// Activate target state ( we are blending to it )
		m_cachedEndStateNode->Activate( instance );

		// Synchronize target
		if ( m_cachedStartStateNode && m_synchronize && m_syncMethod )
		{
			CSyncInfo syncInfo;
			m_cachedStartStateNode->GetSyncInfo( instance, syncInfo );
			m_syncMethod->SynchronizeTo( instance, m_cachedEndStateNode, syncInfo );
		}
	}
}

void CBehaviorGraphStateTransitionBlendNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedStartStateNode )
	{
		m_cachedStartStateNode->Deactivate( instance );
	}
}

Float CBehaviorGraphStateTransitionBlendNode::GetAlpha( CBehaviorGraphInstance& instance ) const
{
	return ( m_transitionTime > 0.0f ) ? BehaviorUtils::BezierInterpolation( Clamp( instance[ i_currentTime ] / m_transitionTime, 0.0f, 1.0f ) ) : 1.0f;
}

void CBehaviorGraphStateTransitionBlendNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	Float controlAlpha = GetAlpha( instance );

	if ( m_cachedStartStateNode ) 
	{
		m_cachedStartStateNode->ProcessActivationAlpha( instance, ( 1.0f - controlAlpha ) * alpha );
	}

	if ( m_cachedEndStateNode )
	{
		m_cachedEndStateNode->ProcessActivationAlpha( instance, controlAlpha * alpha );
	}
}

void CBehaviorGraphStateTransitionBlendNode::CopyFrom(const CBehaviorGraphStateTransitionNode* node)
{
	TBaseClass::CopyFrom(node);

	/*if (node->IsA<CBehaviorGraphStateTransitionBlendNode>())
	{
		const CBehaviorGraphStateTransitionBlendNode* templateNode = static_cast<const CBehaviorGraphStateTransitionBlendNode*>(node);

		// Members
		m_transitionTime = templateNode->m_transitionTime;
		m_currentTime	 = templateNode->m_currentTime;
		m_synchronize	 = templateNode->m_synchronize;
		
		// Sync method
		if (templateNode->m_syncMethod)
		{
			if (templateNode->m_syncMethod->GetStaticClass() != m_syncMethod->GetStaticClass())
			{
				m_syncMethod->Discard();
				m_syncMethod = NULL;

				m_syncMethod = SafeCast<IBehaviorSyncMethod>(templateNode->m_syncMethod->Clone(this));
			}
		}
		else if (m_syncMethod)
		{
			m_syncMethod->Discard();
			m_syncMethod = NULL;
		}
	}*/
}

const CBehaviorGraphStateNode* CBehaviorGraphStateTransitionBlendNode::GetCloserState( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_currentTime ] > m_transitionTime/2.f )
	{
		return m_cachedEndStateNode ? m_cachedEndStateNode : m_cachedStartStateNode;
	}
	else
	{
		return m_cachedStartStateNode ? m_cachedStartStateNode : m_cachedEndStateNode;
	}
}

const IBehaviorSyncMethod* CBehaviorGraphStateTransitionBlendNode::GetSyncMethod() const
{
	return m_syncMethod;
}

void CBehaviorGraphStateTransitionBlendNode::CreateAnimEventDefault( const CName& eventName )
{
	if ( m_transitionCondition )
	{
		m_transitionCondition->Discard();
		m_transitionCondition = NULL;
	}

	m_transitionCondition = CreateObject< CAnimEventTransitionCondition >( this );

	CAnimEventTransitionCondition* trans = Cast< CAnimEventTransitionCondition >( m_transitionCondition );
	if ( trans )
	{
		trans->SetEventName( eventName );
	}

	m_synchronize = true;

	if ( m_syncMethod )
	{
		m_syncMethod->Discard();
		m_syncMethod = NULL;
	}

	m_syncMethod = CreateObject< CBehaviorSyncMethodEventProp >( this );
	
	CBehaviorSyncMethodEventProp* sync = Cast< CBehaviorSyncMethodEventProp >( m_syncMethod );
	if ( sync )
	{
		sync->SetEventName( eventName );
	}
}

void CBehaviorGraphStateTransitionBlendNode::CreateEventDefault( const CName& eventName )
{
	if ( m_transitionCondition )
	{
		m_transitionCondition->Discard();
		m_transitionCondition = NULL;
	}

	m_transitionCondition = CreateObject< CEventStateTransitionCondition >( this );

	CEventStateTransitionCondition* trans = Cast< CEventStateTransitionCondition >( m_transitionCondition );
	if ( trans )
	{
		trans->SetEventName( eventName );
	}

	m_synchronize = true;

	if ( m_syncMethod )
	{
		m_syncMethod->Discard();
		m_syncMethod = NULL;
	}
}

void CBehaviorGraphStateTransitionBlendNode::CreateDefault_FootLeft()
{
	CreateAnimEventDefault( CNAME( FootLeft ) );
}

void CBehaviorGraphStateTransitionBlendNode::CreateDefault_FootRight()
{
	CreateAnimEventDefault( CNAME( FootRight ) );
}

void CBehaviorGraphStateTransitionBlendNode::CreateDefault_AnimEndAUX()
{
	CreateEventDefault( CNAME( AnimEndAUX ) );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
