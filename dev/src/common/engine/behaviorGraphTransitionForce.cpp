/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphTransitionForce.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionGlobalBlendNode );

CBehaviorGraphStateTransitionGlobalBlendNode::CBehaviorGraphStateTransitionGlobalBlendNode()
	: m_generateEventForDestState( CName::NONE )
	, m_generateForcedEventForDestState( CName::NONE )
	, m_useProgressiveSampilngForBlending(false)
	, m_cachePoseFromPrevSampling(false)
{
	m_transitionTime = 0.0f;
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_progressiveSamplingAlpha;
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphStateTransitionGlobalBlendNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Global transition [%s]"), m_name.AsChar() );
	}
	else
	{
		return String::EMPTY;
	}
}

EGraphBlockDepthGroup CBehaviorGraphStateTransitionGlobalBlendNode::GetBlockDepthGroup() const
{
	return GBDG_Foreground;
}

Color CBehaviorGraphStateTransitionGlobalBlendNode::GetBorderColor() const
{
	if ( !m_isEnabled )
	{
		return Color::RED;
	}

	if ( ! m_generateEventForDestState.Empty() ||
		 ! m_generateForcedEventForDestState.Empty() )
	{
		return Color( 128, 0, 255 );
	}

	return Color( 255, 128, 0 );
}

Color CBehaviorGraphStateTransitionGlobalBlendNode::GetClientColor() const
{
	if ( !m_isEnabled )
	{
		return Color( 200, 172, 172 );
	}

	if ( ! m_generateEventForDestState.Empty() ||
		 ! m_generateForcedEventForDestState.Empty() )
	{
		return Color( 139, 122, 156 );
	}

	return Color( 156, 139, 122 );
}

EGraphBlockShape CBehaviorGraphStateTransitionGlobalBlendNode::GetBlockShape() const
{
	return GBS_LargeCircle;
}

#endif // NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphStateTransitionGlobalBlendNode::CanConnectWith( CBehaviorGraphInstance& instance, const CBehaviorGraphStateNode* state ) const
{
	// Disallow switching to active state that we're pointing if we're to generate event for it
	/*
	I decided to switch it off, as current implementation is fine but for over a year we've been dealing with improper one that had
		#ifndef NO_EDITOR_GRAPH_SUPPORT
		state->IsActivated() &&
		#endif
	inside and that method was always returning false.
	So not to break functionality (as it was never hit), I decided to comment it out
	if
	(
		state == GetDestState() &&
		state->IsActive( instance ) &&
		(
			! m_generateEventForDestState.Empty() ||
			! m_generateForcedEventForDestState.Empty()
		)
	)
	{
		return false;
	}
	*/

	// Check tag include and exclude list - use operator OR
	const TagList& stateGroup = state->GetGroupList();

	// Includes
	Bool includeTest = TagList::MatchAny( m_includeGroup, stateGroup );

	// Excludes
	Bool excludeTest = false;

	if ( m_excludeGroup.Empty() || stateGroup.Empty() )
	{
		excludeTest = true;
	}
	else
	{
		excludeTest = !TagList::MatchAny( m_excludeGroup, stateGroup );
	}

	if ( includeTest && excludeTest )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnStartBlockActivated( instance );
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnStartBlockDeactivated( instance );
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	ASSERT( m_cachedEndStateNode );
	
	//m_cachedEndStateNode->Reset( instance );

	TBaseClass::OnActivated( instance );

	CreateAndCachePose( instance );

	instance[ i_progressiveSamplingAlpha ] = 0.0f;

	// generate events for destination state

	if ( ! m_generateEventForDestState.Empty() && GetDestState() )
	{
		Uint32 eventID = instance.GetEventId( m_generateEventForDestState );

		if ( eventID != CBehaviorEventsList::NO_EVENT )
		{
			CBehaviorEvent event( eventID );
			GetDestState()->ProcessEvent( instance, event );
		}
	}

	if ( ! m_generateForcedEventForDestState.Empty() && GetDestState() )
	{
		Uint32 eventID = instance.GetEventId( m_generateForcedEventForDestState );

		if ( eventID != CBehaviorEventsList::NO_EVENT )
		{
			CBehaviorEvent event( eventID );
			GetDestState()->ProcessForceEvent( instance, event );
		}
	}
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	DestroyPose( instance );

	TBaseClass::OnDeactivated( instance );
}

Bool CBehaviorGraphStateTransitionGlobalBlendNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	// may process event even if active
	if ( m_transitionCondition && m_isEnabled )
	{
		return m_transitionCondition->ProcessEvent( instance, event );
	}

	return false;
}

Bool CBehaviorGraphStateTransitionGlobalBlendNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_transitionCondition && m_isEnabled )
	{
		return m_transitionCondition->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphStateTransitionGlobalBlendNode::CreateAndCachePose( CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphStateMachineNode *stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];

	Bool cacheGlobalPose = true;
	if ( ! m_cachePoseFromPrevSampling )
	{
		if ( const SBehaviorGraphOutput* cachedPose = stateMachine->GetCachedOutputPose( instance ) )
		{
			pose.Create( instance );
			pose.Cache( instance, *cachedPose );
			cacheGlobalPose = false;
		}
	}
	if ( m_cachePoseFromPrevSampling && cacheGlobalPose )
	{
		if ( CAnimatedComponent const * ac = instance.GetAnimatedComponent() )
		{
			if ( SBehaviorSampleContext* bsc = ac->GetBehaviorGraphSampleContext() )
			{
				if ( const SBehaviorGraphOutput* cachedPose = &bsc->GetPoseFromPrevSampling())
				{
					pose.Create( instance );
					pose.Cache( instance, *cachedPose );
					cacheGlobalPose = false;
				}
			}
		}
	}
	if ( cacheGlobalPose )
	{
		// fail safe
		// just in any case if we wouldn't have pose from state machine (such case would be blending into state machine into global transition node)
		pose.CreateAndCache( instance );
	}
}

void CBehaviorGraphStateTransitionGlobalBlendNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_pose ].Free( instance );
}

void CBehaviorGraphStateTransitionGlobalBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( TransitionGlobal );

	ASSERT( !m_cachedStartStateNode );

	Float& currentTime = instance[ i_currentTime ];

	currentTime += timeDelta;

	if (m_useProgressiveSampilngForBlending)
	{
		UpdateProgressiveSamplingAlpha(instance, currentTime, timeDelta);
	}
	
	// synchronize children playback
	Synchronize( instance, timeDelta );

	{
		ASSERT( m_cachedEndStateNode->IsActive( instance ) );

		if ( m_cachedEndStateNode && !m_cachedEndStateNode->IsActive( instance ) )
		{
			m_cachedEndStateNode->Activate( instance );
		}
	}

	// update end state
	if ( m_cachedEndStateNode )
	{
		m_cachedEndStateNode->Update( context, instance, timeDelta );
	}

	// transition has finished
	if ( currentTime >= m_transitionTime )
	{
		CBehaviorGraphStateMachineNode *stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( GetParent() );

		Deactivate( instance );

		ASSERT( m_cachedEndStateNode );

		stateMachine->SwitchToState( m_cachedEndStateNode, instance );

		currentTime = 0.0f;
	}
}

void CBehaviorGraphStateTransitionGlobalBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( TransitionGlobal );

	Float alpha = 0.0f;
	if ( !m_useProgressiveSampilngForBlending )
	{
		 alpha = GetAlpha( instance );
	}
	else
	{
		alpha = instance[ i_progressiveSamplingAlpha ];
	}

	CCacheBehaviorGraphOutput tempPose( context );
	SBehaviorGraphOutput* temp = tempPose.GetPose();

	SBehaviorGraphOutput* cachedPose = instance[ i_pose ].GetPose();

	if ( temp && cachedPose )
	{
		// Sample end state
		if ( m_cachedEndStateNode )
		{
			m_cachedEndStateNode->Sample( context, instance, *temp );
		}

		// Interpolate poses
#ifdef DISABLE_SAMPLING_AT_LOD3
		if ( context.GetLodLevel() <= BL_Lod2 )
		{
			output.SetInterpolate( *cachedPose, *temp, alpha );
		}
		else
		{
			output.SetInterpolateME( *cachedPose, *temp, alpha );
		}
#else
		output.SetInterpolate( *cachedPose, *temp, alpha );
#endif

		// Merge events
		output.MergeEventsAndUsedAnims( *temp, alpha );

		// Motion extraction blending
		if ( m_motionBlendType == BTBM_Source )
		{
			output.m_deltaReferenceFrameLocal = cachedPose->m_deltaReferenceFrameLocal;
		}
		else if ( m_motionBlendType == BTBM_Destination )
		{
			output.m_deltaReferenceFrameLocal = temp->m_deltaReferenceFrameLocal;
		}

		if ( m_useProgressiveSampilngForBlending )
		{
			instance[ i_pose ].Cache(instance, output); // Remember the pose from this step.
		}
	}
}

CBehaviorGraph* CBehaviorGraphStateTransitionGlobalBlendNode::GetParentGraph()
{
	CBehaviorGraphNode *node = FindParent< CBehaviorGraphNode >();
	ASSERT( node );
	CBehaviorGraph* graph = node->GetGraph();
	return graph;
}

void CBehaviorGraphStateTransitionGlobalBlendNode::UpdateProgressiveSamplingAlpha(CBehaviorGraphInstance& instance, Float currentTime, Float timeDelta ) const
{
	// [Notes]: remember that progressive sampling have different properties than NLERP interpolation.
	// Interpolation ensures that given pose is always 'between' interpolated poses, while progressive 
	// sampling simply 'chases' the wanted pose. 
	// Advantage of progressive sampling is that is preserves C0 continuity, while NLERP ( or SLERP) interpolation does not (always).
	// TODO: think about sth that preserves C0 continuity and is 'between'.

	Float diff = m_transitionTime - currentTime;

	Float alpha = 0.0f;
	if ( diff > timeDelta ) // to avoid dividing by 0
	{
		alpha = timeDelta / ( m_transitionTime - currentTime );
	}
	else
	{
		alpha = 1.0f;
	}

	instance[ i_progressiveSamplingAlpha ] = alpha;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionGlobalBlendStreamingNode );

CBehaviorGraphStateTransitionGlobalBlendStreamingNode::CBehaviorGraphStateTransitionGlobalBlendStreamingNode()
	: m_defaultStateName( TXT("Idle") )
{

}

//#define NO_USE_GLOBALS_WITH_WAITING

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphStateTransitionGlobalBlendStreamingNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Global transition with streaming[%s]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Global transition with streaming") );
	}
}

Color CBehaviorGraphStateTransitionGlobalBlendStreamingNode::GetBorderColor() const
{
	if ( !m_isEnabled )
	{
		return Color::RED;
	}

	return Color( 255, 255, 255 );
}

#endif

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_waiting;
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( TransitionGlobalBlendStreaming );
#ifndef NO_USE_GLOBALS_WITH_WAITING
	const Float currentTime = instance[ i_currentTime ];

	const CBehaviorGraphStateNode* defNode = GetDefaultNode( instance );

	if ( defNode && currentTime == 0.f )
	{
		Bool& waiting = instance[ i_waiting ];

		Bool isLoaded = PreloadAnimations( instance );
		if ( isLoaded )
		{
			if ( waiting )
			{
				// Cache current pose, prev pose is deprecated
				instance[ i_pose ].Cache( instance );

				waiting = false;
			}
			//else
			//{
				// The fastest way
			//}
		}
		else
		{
			if ( waiting )
			{
				// Still waiting
			}
			else
			{
				waiting = true;
			}

			if ( defNode )
			{
				defNode->Update( context, instance, timeDelta );
			}

			return;
		}
	}
#endif

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
#ifndef NO_USE_GLOBALS_WITH_WAITING
	Bool waiting = instance[ i_waiting ];
	if ( waiting )
	{
		const CBehaviorGraphStateNode* defNode = GetDefaultNode( instance );
		if ( defNode  )
		{
			defNode->Sample( context, instance, output );
		}
	}
	else
	{
		TBaseClass::Sample( context, instance, output );
	}
#else
	TBaseClass::Sample( context, instance, output );
#endif
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

#ifndef NO_USE_GLOBALS_WITH_WAITING
	const CBehaviorGraphStateNode* defNode = GetDefaultNode( instance );
	if ( defNode  )
	{
		defNode->Activate( instance );
	}
#endif
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

#ifndef NO_USE_GLOBALS_WITH_WAITING
	InternalReset( instance );

	const CBehaviorGraphStateNode* defNode = GetDefaultNode( instance );
	if ( defNode  )
	{
		defNode->Deactivate( instance );
	}
#endif
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

#ifndef NO_USE_GLOBALS_WITH_WAITING
	InternalReset( instance );
#endif
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_waiting ] = false;
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	CacheDefaultNode();
}

void CBehaviorGraphStateTransitionGlobalBlendStreamingNode::CacheDefaultNode()
{
	CBehaviorGraphStateMachineNode* m = SafeCast< CBehaviorGraphStateMachineNode >( GetParentNode() );
	if ( m )
	{
		const TDynArray< CGraphBlock* >& children = m->GetConnectedChildren();
		
		const Uint32 size = children.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			CBehaviorGraphNode* node = Cast< CBehaviorGraphNode >( children[ i ] );

			if ( node && node->GetName() == m_defaultStateName )
			{
				m_defaultState = SafeCast< CBehaviorGraphStateNode >( node );
				return;
			}
		}

		m_defaultState = SafeCast< CBehaviorGraphStateNode >( m->GetDefaultState() );
	}
	else
	{
		m_defaultState = NULL;
	}
}

const CBehaviorGraphStateNode* CBehaviorGraphStateTransitionGlobalBlendStreamingNode::GetDefaultNode( CBehaviorGraphInstance& instance ) const
{
	return m_defaultState;
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
