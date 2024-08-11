/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphComboNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSelfActStateMachine.h"
#include "behaviorGraphStartingStateNode.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionForce.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateMachineNode );

CBehaviorGraphStateMachineNode::CBehaviorGraphStateMachineNode()
	: m_defaultNode( NULL )
	, m_resetStateOnExit( false )
	, m_applySyncTags( true )
{
}

void CBehaviorGraphStateMachineNode::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	file << m_states;
	file << m_transitions;

	// PTom: Why?
	CBehaviorGraphNode* fakeNode = NULL;
	file << fakeNode;
	// ----------

	file << m_defaultNode;
}

void CBehaviorGraphStateMachineNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	for ( Int32 i=(Int32)m_nodes.Size()-1; i>=0; --i )
	{
		if ( m_nodes[i] == NULL )
		{
			m_nodes.Erase( m_nodes.Begin() + i );
		}
	}

	for ( Int32 i=(Int32)m_globalTransitions.Size()-1; i>=0; --i )
	{
		if ( m_globalTransitions[i] == NULL )
		{
			m_globalTransitions.Erase( m_globalTransitions.Begin() + i );
		}
	}

	if ( !m_defaultNode && m_states.Size() > 0 )
	{
		m_defaultNode = m_states[0];
	}

	AddMissingNodes();
}

void CBehaviorGraphStateMachineNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_activeForceTransitions;
	compiler << i_currentNode;
	compiler << i_outputPose;
}

void CBehaviorGraphStateMachineNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_activeForceTransitions ].Clear();
	
	SwitchToState( NULL, instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStateMachineNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	TBaseClass::OnRebuildSockets();
}

String CBehaviorGraphStateMachineNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("State machine [ %s ]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("State machine") );
	}
}

#endif

void CBehaviorGraphStateMachineNode::UpdateCurrentState( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// Update current node
	CBehaviorGraphNode* state = instance[ i_currentNode ];
	if ( state )
	{
		ASSERT( state->IsActive( instance ) );
		state->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphStateMachineNode::UpdateTransitions( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// Update global transitions
	GlobalTransitionsStartUpdate( context, instance, timeDelta );

	// Process all transitions. It must be BEFORE update current node function!
	ProcessTransitions( instance );
}

void CBehaviorGraphStateMachineNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( StateMachine );

	UpdateTransitions( context, instance, timeDelta );
	UpdateCurrentState( context, instance, timeDelta );
}

Bool CBehaviorGraphStateMachineNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( instance[ i_currentNode ] && instance[ i_currentNode ]->IsA< CBehaviorGraphStateNode >() )
	{		
		retVal = instance[ i_currentNode ]->ProcessEvent( instance, event );

		CBehaviorGraphStateNode *state = SafeCast< CBehaviorGraphStateNode >( instance[ i_currentNode ] );
		if ( !retVal )
		{
			Uint32 numConnectedNodes = state->GetNumConnectedTransitions();
			for( Uint32 i=0; i<numConnectedNodes; ++i )
			{
				CBehaviorGraphStateTransitionNode *transition = state->GetConnectedTransition( i );

				if ( transition->IsA< CBehaviorGraphStateTransitionGlobalBlendNode >() )
				{
					continue;
				}

				retVal = transition->ProcessEvent( instance, event ) || retVal;

				// DM: all transitions should process event, the correct one will be chosen during ProcessTransitions
				//if ( retVal )
				//	break;
			}
		}
	}
	else if ( instance[ i_currentNode ] && instance[ i_currentNode ]->IsA< CBehaviorGraphStateTransitionNode >() )
	{
		retVal = instance[ i_currentNode ]->ProcessEvent( instance, event );
	}

	if ( retVal )
	{
		return retVal;
	}
	else
	{
		// Check force transition nodes
		for ( Uint32 i=0; i<m_globalTransitions.Size(); i++ )
		{
			CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];

			retVal |= globalTransition->ProcessEvent( instance, event );
		}
	}

	return retVal;
}

Bool CBehaviorGraphStateMachineNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	// Check force transition nodes
	TDynArray< CBehaviorGraphStateTransitionGlobalBlendNode* > transitions;

	for ( Uint32 i=0; i<m_globalTransitions.Size(); i++ )
	{
		CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];

		Bool ret = globalTransition->ProcessForceEvent( instance, event );
		if ( ret )
		{
			retVal = true;
			instance[ i_activeForceTransitions ].PushBack( globalTransition );
		}
	}

	if ( !retVal )
	{
		// Pass to states
		for ( Uint32 i=0; i<m_states.Size(); i++ )
		{
			retVal |= m_states[i]->ProcessForceEvent( instance, event );
		}
	}

	return retVal;
}

Bool CBehaviorGraphStateMachineNode::ProcessForceTransition( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_2( StateMachine_ProcessForceTransition );

	TDynArray< CBehaviorGraphStateTransitionGlobalBlendNode* >& activeForceTransitions = instance[ i_activeForceTransitions ];

	if ( activeForceTransitions.Size() > 0 )
	{
		// Find current state
		CBehaviorGraphStateNode* state = NULL;
		CBehaviorGraphStateTransitionNode* currentTransition = NULL;

		if ( instance[ i_currentNode ]->IsA< CBehaviorGraphStateNode >() )
		{
			state = static_cast< CBehaviorGraphStateNode* >( instance[ i_currentNode ] );
		}
		else if ( instance[ i_currentNode ]->IsA< CBehaviorGraphStateTransitionNode >() )
		{
			currentTransition = static_cast< CBehaviorGraphStateTransitionNode* >( instance[ i_currentNode ] );
			state = currentTransition->GetCloserState();
		}
		else
		{
			ASSERT( 0 );
		}

		ASSERT( state );

		Int32 bestTransitionIndex = -1;

		while( !activeForceTransitions.Empty() )
		{
			// Find best priority
			Float bestPriority = activeForceTransitions[0]->GetPriority();
			for( Uint32 i=1; i<activeForceTransitions.Size(); ++i )
			{
				Float prio = activeForceTransitions[i]->GetPriority();
				if( prio < bestPriority )
				{
					bestPriority = prio;
				}
			}

			// Find latest transition with priority equal to best priority
			for( Int32 i=(Int32)activeForceTransitions.Size()-1; i>=0; i-- )
			{
				CBehaviorGraphStateTransitionGlobalBlendNode* transiton = activeForceTransitions[i];
				if ( transiton->GetPriority() == bestPriority )
				{					
					BEH_PROFILER_LEVEL_3( StateMachine_CheckTransitionCondition );
					if ( transiton->CheckTransitionCondition( instance ) && transiton->CanConnectWith( instance, state ) )
					{
						// Transition found
						bestTransitionIndex = i;
						break;
					}
					else
					{
						// cannot fire, remove it
						activeForceTransitions.Erase( activeForceTransitions.Begin() + i );
					}
				}
			}

			if( bestTransitionIndex >= 0 )
			{
				// Transition found, break loop
				break;
			}
		}		

		if ( activeForceTransitions.Empty() )
		{
			// List is empty
			return false;
		}

		ASSERT( bestTransitionIndex >= 0 );

		CBehaviorGraphStateTransitionGlobalBlendNode *bestTransition = activeForceTransitions[ bestTransitionIndex ];
		CBehaviorGraphStateNode* destState = bestTransition->GetDestState();

		// gather sync tags from current state
		SBehaviorSyncTags currSyncTags;
		if ( instance[ i_currentNode ] && m_applySyncTags )
		{
			instance.GetOutboundSyncTags( currSyncTags, this );
		}

		// we used to deactivate current transition or state ( we use cached pose ) but as sometimes we end up with some states activated maybe we should deactivate all (there is counter so they won't be deactivated twice)
		// yup, that isn't proper fix, because something else is broken here that leaves one state active
		for( Uint32 i=0; i<m_nodes.Size(); ++i )
		{
			if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
			{
				bgNode->Deactivate( instance );
			}
		}

		// inform about state change
		InformAboutCurrentState( instance[ i_currentNode ], bestTransition->GetDestState(), instance );

		// Switch to new state
		SwitchToState( bestTransition, instance );

		// Activate it
		bestTransition->Activate( instance );

		// Clear active force transition list
		activeForceTransitions.Clear();

		// Force reset global transition
		ForceResetGlobalTransitions( instance );

		// apply sync tags
		if ( destState && currSyncTags.DoesContainAnyTags() && m_applySyncTags )
		{
			instance.ApplyInboundSyncTags( currSyncTags, this );
		}

		// Done
		return true;
	}

	return false;
}

void CBehaviorGraphStateMachineNode::ResetInactiveGlobalTransitions( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_2( StateMachine_ResetInactiveGlobalTrans );
	for ( Uint32 i=0; i<m_globalTransitions.Size(); i++ )
	{
		CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];
		if ( instance[ i_currentNode ] != globalTransition )
		{
			globalTransition->Reset( instance );
		}
	}
}

void CBehaviorGraphStateMachineNode::ForceResetGlobalTransitions( CBehaviorGraphInstance& instance ) const
{
	for ( Uint32 i=0; i<m_globalTransitions.Size(); i++ )
	{
		CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];
		globalTransition->OnStartBlockDeactivated( instance );
		globalTransition->OnStartBlockActivated( instance );
	}
}

void CBehaviorGraphStateMachineNode::ProcessSampledPoseToTransitions( CBehaviorGraphInstance& instance , const SBehaviorGraphOutput& pose ) const
{
	CBehaviorGraphNode* currNode = instance[ i_currentNode ];

	if ( currNode && currNode->IsA< CBehaviorGraphStateNode >() )
	{
		CBehaviorGraphStateNode *state = SafeCast< CBehaviorGraphStateNode >( currNode );

		// Local transitions
		Uint32 numConnectedNodes = state->GetNumConnectedTransitions();
		for( Uint32 i=0; i<numConnectedNodes; ++i )
		{
			CBehaviorGraphStateTransitionNode *transition = state->GetConnectedTransition( i );
			transition->OnPoseSampled( instance, pose );
		}
	}

	// Global transitions
	Uint32 nuumGlobals = m_globalTransitions.Size();
	for ( Uint32 i=0; i<nuumGlobals; i++ )
	{
		CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];
		globalTransition->OnPoseSampled( instance, pose );
	}
}

void CBehaviorGraphStateMachineNode::ProcessTransitions( CBehaviorGraphInstance& instance ) const
{	
	// Process force transitions
	if ( ProcessForceTransition( instance ) )
	{
		ProcessTransitions( instance );
		return;
	}

	BEH_PROFILER_LEVEL_2( StateMachine_ProcessTransitions );

	if ( CBehaviorGraphNode* currNode = instance[ i_currentNode ] )
	{
		CBehaviorGraphStateNode *state = Cast< CBehaviorGraphStateNode >( currNode );
		// Process states
		if ( state )
		{
			// find all active transitions
			CBehaviorGraphStateTransitionNode* activeTransitions[ MAX_SIMULTANEOUS_ACTIVE_TRANSITIONS ];
			Uint32 numActiveTransitions = 0;

			{
				BEH_PROFILER_LEVEL_2( StateMachine_CheckTransitions );
				Uint32 numConnectedNodes = state->GetNumConnectedTransitions();
				for( Uint32 i=0; i<numConnectedNodes && numActiveTransitions < MAX_SIMULTANEOUS_ACTIVE_TRANSITIONS; ++i )
				{
					CBehaviorGraphStateTransitionNode *transition = state->GetConnectedTransition( i );

					BEH_PROFILER_LEVEL_3( StateMachine_CheckTransitionCondition );
					if ( transition->CheckTransitionCondition( instance ) )
					{
						activeTransitions[ numActiveTransitions ] = transition;
						++numActiveTransitions;
					}
				}
			}

			if ( numActiveTransitions > 0 )
			{
				BEH_PROFILER_LEVEL_2( StateMachine_ProcessActiveTransition );

				// select one with lowest priority
				Uint32 bestTransitionIndex = 0;

				for( Uint32 i=1; i<numActiveTransitions; ++i )
				{
					if ( activeTransitions[i]->GetPriority() < activeTransitions[ bestTransitionIndex ]->GetPriority() )
					{
						bestTransitionIndex = i;
					}
				}

				CBehaviorGraphStateTransitionNode *transition = activeTransitions[ bestTransitionIndex ];
				CBehaviorGraphStateNode* destState = transition->GetDestState();

				// gather sync tags from current state
				SBehaviorSyncTags currSyncTags;
				if ( instance[ i_currentNode ] && m_applySyncTags )
				{
					BEH_PROFILER_LEVEL_3( StateMachine_GetOutboundSyncTags );
					instance.GetOutboundSyncTags( currSyncTags, this );
				}

				// inform about state change
				InformAboutCurrentState( instance[ i_currentNode ], destState, instance );

				// switch to new state
				SwitchToState( transition, instance );

				// activate it
				transition->Activate( instance );

				// Force reset all global transition
				ForceResetGlobalTransitions( instance );

				// apply sync tags
				if ( destState && currSyncTags.DoesContainAnyTags() && m_applySyncTags )
				{
					BEH_PROFILER_LEVEL_3( StateMachine_ApplyInboundSyncTags );
					instance.ApplyInboundSyncTags( currSyncTags, this );
				}

				// do the thing recursively to process transitions from new node, return afterwards (ignoring loose transitions! they were processsed
				// at the bottom of the recursion!)
				ProcessTransitions( instance );

				return;
			}
		}
		else if ( CBehaviorGraphStateTransitionNode *transition = Cast< CBehaviorGraphStateTransitionNode >( currNode ) )
		{
			state = transition->GetDestState();
		}

		{
			BEH_PROFILER_LEVEL_2( CheckGlobalTransitionCondition );

			// process global transitions using "state"
			// Check global transition
			CBehaviorGraphStateTransitionGlobalBlendNode* activeGlobalTransitions[ MAX_SIMULTANEOUS_ACTIVE_TRANSITIONS ];
			Uint32 numActiveGlobalTransitions = 0;

			for ( Uint32 i=0; i<m_globalTransitions.Size(); i++ )
			{
				CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = m_globalTransitions[i];

				// Collect active transition
				BEH_PROFILER_LEVEL_3( StateMachine_CheckTransitionCondition );
				if ( globalTransition->CanConnectWith( instance, state ) &&  globalTransition->CheckTransitionCondition( instance ) )
				{
					ASSERT( numActiveGlobalTransitions < MAX_SIMULTANEOUS_ACTIVE_TRANSITIONS );
					activeGlobalTransitions[numActiveGlobalTransitions] = globalTransition;
					++ numActiveGlobalTransitions;
				}
			}

			if ( numActiveGlobalTransitions )
			{
				// Check priority
				CBehaviorGraphStateTransitionGlobalBlendNode* bestGlobalTransition = activeGlobalTransitions[0];

				CBehaviorGraphStateTransitionGlobalBlendNode** checkTransitionPtr = activeGlobalTransitions + 1;
				for ( Uint32 i=1; i < numActiveGlobalTransitions; ++i, ++checkTransitionPtr )
				{
					if ( (*checkTransitionPtr)->GetPriority() < bestGlobalTransition->GetPriority() )
					{
						bestGlobalTransition = (*checkTransitionPtr);
					}
				}

				// gather sync tags from current state
				SBehaviorSyncTags currSyncTags;
				if ( instance[ i_currentNode ] )
				{
					instance.GetOutboundSyncTags( currSyncTags, this );
				}
				CBehaviorGraphStateNode* destState = bestGlobalTransition->GetDestState();

				// Deactivate current transition or state ( we use cached pose )
				CBehaviorGraphStateTransitionNode* currentTransition = Cast< CBehaviorGraphStateTransitionNode > ( currNode );
				if ( currentTransition )
				{
					currentTransition->Deactivate( instance );
					currentTransition->GetDestState()->Deactivate( instance );
				}
				else
				{
					currNode->Deactivate( instance );
				}

				// inform about state change
				InformAboutCurrentState( instance[ i_currentNode ], destState, instance );

				// Switch to best force transition
				SwitchToState( bestGlobalTransition, instance );

				// Activation
				bestGlobalTransition->Activate( instance );

				// Force reset all global transition
				ForceResetGlobalTransitions( instance );

				// apply sync tags
				if ( destState && currSyncTags.DoesContainAnyTags() )
				{
					instance.ApplyInboundSyncTags( currSyncTags, this );
				}

				ProcessTransitions( instance );

				return;
			}
		}
	}

	// Reset global transition
	ResetInactiveGlobalTransitions( instance );
}

void CBehaviorGraphStateMachineNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( StateMachine );

	ANIM_NODE_PRE_SAMPLE

	CBehaviorGraphNode* currNode = instance[ i_currentNode ];
	if ( currNode ) 
	{
		currNode->Sample( context, instance, output );

		ProcessSampledPoseToTransitions( instance, output );

		CacheOutputPose( instance, output );
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphStateMachineNode::CacheOutputPose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CAllocatedBehaviorGraphOutput& outputPose = instance[ i_outputPose ];
	if (! outputPose.HasPose() )
	{
		outputPose.Create( instance );
	}
	outputPose.Cache( instance, output );
}

void CBehaviorGraphStateMachineNode::DestroyOutputPose( CBehaviorGraphInstance& instance ) const
{
	instance[ i_outputPose ].Free( instance );
}

void CBehaviorGraphStateMachineNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( instance[ i_currentNode ] ) instance[ i_currentNode ]->GetSyncInfo( instance, info );
}

void CBehaviorGraphStateMachineNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( instance[ i_currentNode ] ) instance[ i_currentNode ]->SynchronizeTo( instance, info );
}

void CBehaviorGraphStateMachineNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	// inform about state change
	InformAboutCurrentState( instance[ i_currentNode ], m_defaultNode, instance );

	SwitchToState( m_defaultNode, instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphStateMachineNode::OnChildNodeAdded( CGraphBlock *node )
{
	TBaseClass::OnChildNodeAdded( node );

	if ( node->IsA<CBehaviorGraphStateNode>() )
	{
		CBehaviorGraphStateNode *stateNode = SafeCast< CBehaviorGraphStateNode >( node );
		m_states.PushBack( stateNode );

		if ( m_defaultNode == NULL )
		{
			SetDefaultState( stateNode );
		}
	}
	else if ( node->IsA<CBehaviorGraphStateTransitionGlobalBlendNode>() )
	{
		m_globalTransitions.PushBack( SafeCast< CBehaviorGraphStateTransitionGlobalBlendNode >( node ) );
	}
	else if ( node->IsA<CBehaviorGraphStateTransitionNode>() )
	{
		m_transitions.PushBack( SafeCast< CBehaviorGraphStateTransitionNode >( node ) );
	}
	AddMissingNodes();
}

void CBehaviorGraphStateMachineNode::RemoveChildNode( CGraphBlock *node )
{
	TBaseClass::RemoveChildNode( node );

	if ( node->GetClass()->IsBasedOn( CBehaviorGraphStateNode::GetStaticClass() ) )
	{
		m_states.Remove( SafeCast< CBehaviorGraphStateNode >( node ) );

		if ( m_defaultNode == node && m_states.Size() > 0 )
		{
			m_defaultNode = m_states[0];
		}
	}
	else if ( node->GetClass()->IsBasedOn( CBehaviorGraphStateTransitionGlobalBlendNode::GetStaticClass() ) )
	{
		m_globalTransitions.Remove( SafeCast< CBehaviorGraphStateTransitionGlobalBlendNode >( node ) );
	}
	else if ( node->GetClass()->IsBasedOn( CBehaviorGraphStateTransitionNode::GetStaticClass() ) )
	{
		m_transitions.Remove( SafeCast< CBehaviorGraphStateTransitionNode >( node ) );
	}
}

Bool CBehaviorGraphStateMachineNode::ChildNodeClassSupported( CClass *nodeClass )
{
	if (  ( nodeClass->IsA<CBehaviorGraphStateNode>() || nodeClass->IsA<CBehaviorGraphStateTransitionNode>() ) && 
			nodeClass != CBehaviorGraphComboStateNode::GetStaticClass() &&
			nodeClass != CBehaviorGraphDefaultSelfActStateNode::GetStaticClass() )
	{
		return true;
	}

	return TBaseClass::ChildNodeClassSupported( nodeClass );
}

#endif

void CBehaviorGraphStateMachineNode::AddMissingNodes()
{
	// for some odd reason, not every state was remembered as state
	// that's why this strange code was added
	// I also decided to do this for transitions, although they seem to be fine, but you never know when things are going to break again and finding such bug can be quite nasty
	for ( TDynArray< CGraphBlock* >::iterator iNode = m_nodes.Begin(); iNode != m_nodes.End(); ++ iNode )
	{
		if ( CBehaviorGraphStateNode* state = Cast< CBehaviorGraphStateNode >( *iNode ) )
		{
			if ( ! m_states.Exist( state ) )
			{
				m_states.PushBack( state );
			}
		}
		else if ( CBehaviorGraphStateTransitionGlobalBlendNode* globalTransition = Cast< CBehaviorGraphStateTransitionGlobalBlendNode >( *iNode ) )
		{
			if ( ! m_globalTransitions.Exist( globalTransition ) )
			{
				m_globalTransitions.PushBack( globalTransition );
			}
		}
		else if ( CBehaviorGraphStateTransitionNode* transition = Cast< CBehaviorGraphStateTransitionNode >( *iNode ) )
		{
			if ( ! m_transitions.Exist( transition ) )
			{
				m_transitions.PushBack( transition );
			}
		}
	}
}

void CBehaviorGraphStateMachineNode::SwitchToState( CBehaviorGraphNode* node, CBehaviorGraphInstance& instance ) const
{	
	CBehaviorGraphNode*& currentNode = instance[ i_currentNode ];
	currentNode = node;
	if ( CBehaviorGraphStateNode * state = Cast< CBehaviorGraphStateNode >( currentNode ) )
	{
		state->OnFullyBlendedIn( instance );
	}
}

void CBehaviorGraphStateMachineNode::InformAboutCurrentState( CBehaviorGraphNode* willBeNoLongerCurrent, CBehaviorGraphNode* willBecomeCurrent, CBehaviorGraphInstance& instance ) const
{	
	if ( CBehaviorGraphStateNode * state = Cast< CBehaviorGraphStateNode >( willBeNoLongerCurrent ) )
	{
		state->OnNoLongerCurrentState( instance );
	}
	if ( CBehaviorGraphStateNode * state = Cast< CBehaviorGraphStateNode >( willBecomeCurrent ) )
	{
		state->OnBecomesCurrentState( instance );
	}
}

void CBehaviorGraphStateMachineNode::SetStateActive( CBehaviorGraphNode *node, CBehaviorGraphInstance& instance ) const
{
	// deactivate current state
	CBehaviorGraphNode* curr = GetCurrentState( instance );
	if ( curr )
	{
		curr->Deactivate( instance );
	}

	// inform about state change
	InformAboutCurrentState( instance[ i_currentNode ], node, instance );

	SwitchToState( node, instance ); 

	// activate current state
	curr = GetCurrentState( instance );
	if ( curr )
	{
		curr->Activate( instance );
	}
}

Bool CBehaviorGraphStateMachineNode::HasGlobalTransitionConnectedTo( const CBehaviorGraphStateNode* state ) const
{
	for ( Uint32 i=0; i<m_globalTransitions.Size(); ++i )
	{
		if ( m_globalTransitions[i]->GetDestState() == state )
		{
			return true;
		}
	}

	return false;
}

const SBehaviorGraphOutput* CBehaviorGraphStateMachineNode::GetCachedOutputPose( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_outputPose ].GetPoseUnsafe();
}

void CBehaviorGraphStateMachineNode::InternalActivate( CBehaviorGraphInstance& instance ) const
{
	// deactivate everything, just in case something could be still activated - happens sometimes, is cause by something else (so it isn't proper fix :( )
	// this deactivation is treated as failsafe (similar to deactivating everything on global transition)
	for( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->Deactivate( instance );
		}
	}


	if ( !instance[ i_currentNode ] && m_defaultNode )
	{
		SwitchToState( m_defaultNode, instance );
		ASSERT( instance[ i_currentNode ] );
	}

	// inform about state change
	InformAboutCurrentState( NULL, instance[ i_currentNode ], instance );

	if ( instance[ i_currentNode ] ) 
	{
		instance[ i_currentNode ]->Activate( instance );
	}

	// Pass to force transitions
	GlobalTransitionsStartBlockActivated( instance );
}

void CBehaviorGraphStateMachineNode::InternalDeactivate( CBehaviorGraphInstance& instance ) const
{
	// Pass to transitions
	GlobalTransitionsStartBlockDeactivated( instance );

	// deactivate all nodes
	for( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->Deactivate( instance );
		}
	}

	// inform about state change
	InformAboutCurrentState( instance[ i_currentNode ], NULL, instance );

	if ( m_resetStateOnExit && m_defaultNode )
	{
		SwitchToState( m_defaultNode, instance );
	}

	DestroyOutputPose( instance );
}

void CBehaviorGraphStateMachineNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	InternalActivate( instance );
}

void CBehaviorGraphStateMachineNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	InternalDeactivate( instance );
}

void CBehaviorGraphStateMachineNode::SetDefaultState( CBehaviorGraphStateNode *node )
{
	ASSERT( m_nodes.Exist( node ) );
	
	m_defaultNode = node;
}

CBehaviorGraphNode* CBehaviorGraphStateMachineNode::GetDefaultState() const
{
	return m_defaultNode;
}

Bool CBehaviorGraphStateMachineNode::GetSyncInfoForInstance( CBehaviorSyncInfo& info, CBehaviorGraphInstance& instance ) const
{
	if ( IsActive( instance ) == false )
	{
		return false;
	}

	// Local transitions
	CBehaviorGraphNode* currNode = instance[ i_currentNode ];
	if ( currNode && currNode->IsA< CBehaviorGraphStateNode >() )
	{
		CBehaviorGraphStateNode* state = Cast< CBehaviorGraphStateNode >( currNode );
		return state->GetSyncInfoForInstance( instance, info );
	}

	return false;
}

Bool CBehaviorGraphStateMachineNode::SynchronizeInstanceTo( const CBehaviorSyncInfo& info, CBehaviorGraphInstance& instance ) const
{
	if ( IsActive( instance ) == false )
	{
		ASSERT( IsActive( instance ) );
		return false;
	}

	CBehaviorGraphFlowConnectionNode* syncNode = NULL;

	Uint32 statesNum = m_states.Size();
	for ( Uint32 i=0; i<statesNum; ++i )
	{
		if ( m_states[ i ] && m_states[ i ]->IsA< CBehaviorGraphFlowConnectionNode >() )
		{
			CBehaviorGraphFlowConnectionNode* flowState = Cast< CBehaviorGraphFlowConnectionNode >( m_states[ i ] );
			if ( flowState->CanSynchronizeIntanceTo( instance, info ) )
			{
				syncNode = flowState;
				break;
			}
		}
	}

	// Activate new node
	if ( syncNode )
	{
		CBehaviorGraphNode* currNode = GetCurrentState( instance );
		if ( currNode )
		{
			currNode->Deactivate( instance );
		}

		// inform about state change
		InformAboutCurrentState( instance[ i_currentNode ], syncNode, instance );

		SwitchToState( syncNode, instance );

		currNode = GetCurrentState( instance );

		if ( currNode )
		{
			ASSERT( currNode == syncNode );

			currNode->Activate( instance );

			VERIFY( syncNode->SynchronizeInstanceTo( instance, info ) );

			return true;
		}
		else
		{
			ASSERT( currNode );
		}
	}

	return false;
}

Bool CBehaviorGraphStateMachineNode::IsSynchronizing( CBehaviorGraphInstance& instance ) const
{
	if ( IsActive( instance ) == false )
	{
		return false;
	}

	CBehaviorGraphNode* currNode = GetCurrentState( instance );
	if ( currNode )
	{
		return currNode->IsA< CBehaviorGraphFlowTransitionNode >() || currNode->IsA< CBehaviorGraphFlowConnectionNode >();
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
//
//	Synchronization using sync tags
//
//	When switching between behavior graphs, outbound tags are gathered from 
//	all active states (of course from all active state machines only).
//	This info is used when initiating other behavior graph in which all active
//	state machines are queried with sync tags array to choose best state among
//	contained to start.
//

Bool CBehaviorGraphStateMachineNode::GetOutboundSyncTags( SBehaviorSyncTags& tags, CBehaviorGraphInstance& instance ) const
{
	if ( IsActive( instance ) == false )
	{
		return false;
	}

	CBehaviorGraphNode* currNode = GetCurrentState( instance );
	CBehaviorGraphStateNode* currState = Cast< CBehaviorGraphStateNode >( currNode );
	if ( CBehaviorGraphStateTransitionNode* currTransition = Cast< CBehaviorGraphStateTransitionNode >( currNode ) )
	{
		// Get destination node
		currState = currTransition->GetDestState();
	}

	if ( currState )
	{
		return currState->GetOutboundSyncTags( tags );
	}

	return false;
}

Bool CBehaviorGraphStateMachineNode::ApplyInboundSyncTags( SBehaviorSyncTags& tags, CBehaviorGraphInstance& instance ) const
{
	CBehaviorGraphStateNode* bestState = NULL;
	Int32 bestPriority = 0;
	Int32 bestTagCount = 0;
	// find state that matches tags (and has highest priority)
	for ( TDynArray< CBehaviorGraphStateNode* >::const_iterator iState = m_states.Begin(); iState != m_states.End(); ++ iState )
	{
		if ( CBehaviorGraphStateNode* state = *iState )
		{
			const SBehaviorGraphStateBehaviorGraphSyncInfo& stateSyncInfo = state->GetBehaviorGraphSyncInfo();
			if ( bestState == NULL ) // no other? take this!
			{
				Int32 tagCount = stateSyncInfo.GetMatchingInboundTagCount( tags );
				if ( tagCount > 0 ) // matches at least one tag
				{
					bestState = state;
					bestPriority = stateSyncInfo.m_inSyncPriority;
					bestTagCount = stateSyncInfo.GetMatchingInboundTagCount( tags );
				}
			}
			else if ( stateSyncInfo.m_inSyncPriority >= bestPriority ) // only if priority is higher than our best so far
			{
				Int32 tagCount = stateSyncInfo.GetMatchingInboundTagCount( tags );
				if ( tagCount > 0 && // matches at least one tag
					 ( tagCount > bestTagCount || // only if matching tag count is better then our best so far
					   stateSyncInfo.m_inSyncPriority > bestPriority ) ) // or priority is higher
				{
					bestState = state;
					bestPriority = stateSyncInfo.m_inSyncPriority;
					bestTagCount = tagCount;
				}
			}
		}
	}
	// switch only if it is needed
	if ( bestState && bestState != GetCurrentState( instance ) )
	{
		// deactivate all nodes
		for( Uint32 i=0; i<m_nodes.Size(); ++i )
		{
			if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
			{
				if ( bgNode != bestState )
				{
					bgNode->Deactivate( instance );
				}
			}
		}

		// inform about state change
		InformAboutCurrentState( instance[ i_currentNode ], bestState, instance );
		// activate best state
		SwitchToState( bestState, instance );
		bestState->Activate( instance );
		return true;
	}
	return false;
}

CBehaviorGraphNode* CBehaviorGraphStateMachineNode::GetCurrentState( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_currentNode ];
}

String CBehaviorGraphStateMachineNode::GetCurrentStateName( const CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_currentNode ] )
	{
		if ( instance[ i_currentNode ]->IsA< CBehaviorGraphStateNode >() )
		{
			return instance[ i_currentNode ]->GetName();
		}
		else if ( instance[ i_currentNode ]->IsA< CBehaviorGraphStateTransitionNode >() )
		{
			CBehaviorGraphStateTransitionNode* transition = SafeCast< CBehaviorGraphStateTransitionNode >( instance[ i_currentNode ] );

			// Get destination node
			CBehaviorGraphStateNode* state = transition->GetDestState();

			if ( state )
			{
				return state->GetName();
			}
		}
		else
		{
			ASSERT( 0 );
		}
	}

	return String::EMPTY;
}

void CBehaviorGraphStateMachineNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( instance[ i_currentNode ] )
	{
		instance[ i_currentNode ]->ProcessActivationAlpha( instance, alpha );
	}

	// Pass to all force transitions
	for( Uint32 i=0; i<m_globalTransitions.Size(); ++i )
	{
		m_globalTransitions[i]->StartBlockProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphStateMachineNode::GlobalTransitionsStartUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// Pass to all global transitions
	for( Uint32 i=0; i<m_globalTransitions.Size(); ++i )
	{
		m_globalTransitions[i]->OnStartBlockUpdate( context, instance, timeDelta );
	}	
}

void CBehaviorGraphStateMachineNode::GlobalTransitionsStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	// Pass to all global transitions
	for( Uint32 i=0; i<m_globalTransitions.Size(); ++i )
	{
		m_globalTransitions[i]->OnStartBlockActivated( instance );
	}
}

void CBehaviorGraphStateMachineNode::GlobalTransitionsStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Pass to all global transitions
	for( Uint32 i=0; i<m_globalTransitions.Size(); ++i )
	{
		m_globalTransitions[i]->OnStartBlockDeactivated( instance );
	}
}

void CBehaviorGraphStateMachineNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	Bool currentNodeExists = true;
	if ( CBehaviorGraphStateNode *asState = Cast< CBehaviorGraphStateNode >( instance[ i_currentNode ] ) )
	{
		if ( ! m_states.Exist( asState ) )
		{
			currentNodeExists = false;
		}
	}
	if ( CBehaviorGraphStateTransitionGlobalBlendNode *asGlobalTransition = Cast< CBehaviorGraphStateTransitionGlobalBlendNode >( instance[ i_currentNode ] ) )
	{
		if ( ! m_globalTransitions.Exist( asGlobalTransition ) )
		{
			currentNodeExists = false;
		}
	}
	else if ( CBehaviorGraphStateTransitionNode *asStateTransition = Cast< CBehaviorGraphStateTransitionNode >( instance[ i_currentNode ] ) )
	{
		if ( ! m_transitions.Exist( asStateTransition ) )
		{
			currentNodeExists = false;
		}
	}

	if (! currentNodeExists)
	{
		instance[ i_currentNode ] = m_defaultNode;
	}

	// remove all forced transitions that don't exist
	TDynArray< CBehaviorGraphStateTransitionGlobalBlendNode* >& activeForceTransitions = instance[ i_activeForceTransitions ];
	for ( Int32 i = activeForceTransitions.Size() - 1; i >= 0; -- i )
	{
		if ( ! m_globalTransitions.Exist( activeForceTransitions[i] ) )
		{
			activeForceTransitions.RemoveAt( i );
		}
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
