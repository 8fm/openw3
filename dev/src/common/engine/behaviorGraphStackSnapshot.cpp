/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphStackSnapshot.h"
#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "animSyncInfo.h"

IMPLEMENT_ENGINE_CLASS( SBehaviorSnapshotDataStateMachine );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInstanceSnapshot );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStackSnapshot );

namespace
{
	void FillSnapshotData( CBehaviorGraphInstance* inst, CBehaviorGraphStateMachineNode* m, CBehaviorGraphStateNode* s, SBehaviorSnapshotDataStateMachine& data )
	{
		CSyncInfo sync;
		s->GetSyncInfo( *inst, sync );

		data.m_stateMachineId = m->GetId();
		data.m_currentStateId = s->GetId();
		data.m_currentStateTime = sync.m_currTime;
	}
}

CBehaviorGraphInstanceSnapshot::CBehaviorGraphInstanceSnapshot()
	:	m_instanceBuffer( NULL )
{
}

CBehaviorGraphInstanceSnapshot::~CBehaviorGraphInstanceSnapshot()
{
	if ( m_instanceBuffer )
	{
		m_instanceBuffer->Release();
		m_instanceBuffer = NULL;
	}
}

void CBehaviorGraphInstanceSnapshot::ReleaseAndDiscard()
{
	if ( m_instanceBuffer )
	{
		m_instanceBuffer->Release();
		m_instanceBuffer = NULL;
	}
	Discard();
}

void CBehaviorGraphInstanceSnapshot::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	file << m_floatVariables << m_vectorVariables;
}

CBehaviorGraphInstanceSnapshot* CBehaviorGraphInstance::CreateSnapshot( CObject* parent, Bool storeBuffer )
{
	CBehaviorGraphInstanceSnapshot* snapshot = CreateObject< CBehaviorGraphInstanceSnapshot >( parent );
	snapshot->m_instanceName = m_instanceName;

	ASSERT( m_active );

	// Gather nodes, they will be used in a second
	const CBehaviorGraph* graph = GetGraph();
	TDynArray< CBehaviorGraphNode* > nodes;
	graph->GetAllNodes( nodes );
	const Uint32 s = nodes.Size();

	// Save buffer data
	snapshot->m_instanceBuffer = storeBuffer? m_data->CreateCopy( parent, TXT("Snapshot")) : NULL;

	// Store variables
	if (! storeBuffer)
	{
		graph->StoreRuntimeFloatVariables( *this, snapshot->m_floatVariables );
		graph->StoreRuntimeVectorVariables( *this, snapshot->m_vectorVariables );
	}

	// Save state machines
	// even though this should be saved with instance buffer, keep in mind that when loading on other machine, pointers may differ and state machine stores raw pointers
	for( Uint32 i=0; i<s; i++ )
	{
		if( nodes[i]->IsA< CBehaviorGraphStateMachineNode >() )
		{
			CBehaviorGraphStateMachineNode* stateMachine = Cast< CBehaviorGraphStateMachineNode >( nodes[i] );
			CBehaviorGraphNode* currentNode = stateMachine->GetCurrentState( *this );

			if( currentNode )
			{
				if( currentNode->IsA< CBehaviorGraphStateNode >() )
				{
					CBehaviorGraphStateNode* currentState = Cast< CBehaviorGraphStateNode >( currentNode );

					Uint32 idx = static_cast< Uint32 >( snapshot->m_stateMachineData.Grow(1) );
					SBehaviorSnapshotDataStateMachine& data = snapshot->m_stateMachineData[idx];

					FillSnapshotData( this, stateMachine, currentState, data );
				}
				else if( currentNode->IsA< CBehaviorGraphStateTransitionNode >() )
				{
					CBehaviorGraphStateTransitionNode* currentTransition = Cast< CBehaviorGraphStateTransitionNode >( currentNode );
					CBehaviorGraphStateNode* destState = currentTransition->GetDestState();
					ASSERT( destState );

					// Destination state saved as current state
					Uint32 idx = static_cast< Uint32 >( snapshot->m_stateMachineData.Grow(1) );
					SBehaviorSnapshotDataStateMachine& data = snapshot->m_stateMachineData[idx];

					FillSnapshotData( this, stateMachine, destState, data );
				}
				else 
				{
					ASSERT( 0 && TXT("Only transition or state supported" ) );
				}
			}
		}
	}

	return snapshot;
}

Bool CBehaviorGraphInstance::RestoreSnapshot( const CBehaviorGraphInstanceSnapshot* snapshot )
{
	/*
	 *	NOTE
	 *
	 *	This might have problems with pointers to data that is outside of graph or
	 *	to pointers that refer to other nodes (when loading is implemented)
	 *
	 */
	ASSERT( snapshot );
	ASSERT( snapshot->m_instanceName == m_instanceName );

	// Gather nodes, they will be used in a second
	const CBehaviorGraph* graph = GetGraph();
	TDynArray< CBehaviorGraphNode* > nodes;
	graph->GetAllNodes( nodes );
	const Uint32 s = nodes.Size();

	if ( ! snapshot->m_instanceBuffer )
	{
		// Deactivate
		Deactivate();

		// Internal reset
		ResetInternal();

		// Restore variables
		graph->RestoreRuntimeFloatVariables( *this, snapshot->m_floatVariables );
		graph->RestoreRuntimeVectorVariables( *this, snapshot->m_vectorVariables );
	}

	// now this is tricky - we need raw copy as a reference
	InstanceBuffer* rawCopyOfCurrentInstanceBuffer = m_data->CreateRawCopy( this, TXT("SnapshotOfCurrent"));

	// Go through all nodes and inform that snapshot was loaded
	for( Uint32 i=0; i<s; i++ )
	{
		nodes[i]->OnLoadingSnapshot( *this, *m_data );
	}

	// Restore buffer data
	if ( snapshot->m_instanceBuffer && snapshot->m_instanceBuffer )
	{
		m_data->operator =(*snapshot->m_instanceBuffer);
	}

	// Go through all nodes and inform that snapshot was loaded
	for( Uint32 i=0; i<s; i++ )
	{
		nodes[i]->OnLoadedSnapshot( *this, *rawCopyOfCurrentInstanceBuffer );
	}

	rawCopyOfCurrentInstanceBuffer->ReleaseRaw();

	if ( ! snapshot->m_instanceBuffer )
	{
		// Restore state machines
		// (check information above about loading)
		for( Uint32 i=0; i<snapshot->m_stateMachineData.Size(); i++ )
		{
			const SBehaviorSnapshotDataStateMachine& stateMachineData = snapshot->m_stateMachineData[i];

			CBehaviorGraphStateMachineNode* stateMachine = SafeCast< CBehaviorGraphStateMachineNode >( graph->FindNodeById( stateMachineData.m_stateMachineId ) );
			CBehaviorGraphStateNode* currentState = SafeCast< CBehaviorGraphStateNode >( graph->FindNodeById( stateMachineData.m_currentStateId ) );

			if ( stateMachine && currentState )
			{
				stateMachine->SwitchToState( currentState, *this );
			}
		}

		// Activate
		Activate();

		// synchronization is no longer needed as we restored instance data
		// Restore state machine's current state time
		for( Uint32 i=0; i<snapshot->m_stateMachineData.Size(); i++ )
		{
			const SBehaviorSnapshotDataStateMachine& stateMachineData = snapshot->m_stateMachineData[i];

			CBehaviorGraphStateNode* currentState = SafeCast< CBehaviorGraphStateNode >( graph->FindNodeById( stateMachineData.m_currentStateId ) );

			if ( currentState )
			{
				CSyncInfo sync;
				sync.m_currTime = stateMachineData.m_currentStateTime;
				sync.m_prevTime = stateMachineData.m_currentStateTime;

				currentState->SynchronizeTo( *this, sync );
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphStackSnapshot* CBehaviorGraphStack::CreateSnapshot( CObject* parent )
{
	CBehaviorGraphStackSnapshot* snapshot = CreateObject< CBehaviorGraphStackSnapshot >( parent );
	snapshot->m_instanceSnapshots.Grow( m_instances.Size() );

	for( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		CBehaviorGraphInstanceSnapshot* instanceSnapshot = m_instances[i]->CreateSnapshot( this );
		ASSERT( instanceSnapshot );
		snapshot->m_instanceSnapshots[i] = instanceSnapshot;
	}

	return snapshot;
}

Bool CBehaviorGraphStack::RestoreSnapshot( const CBehaviorGraphStackSnapshot* snapshot )
{
	ASSERT( snapshot );

	TDynArray< CName > instanceNames;
	instanceNames.Grow( snapshot->m_instanceSnapshots.Size() );
	for( Uint32 i=0; i<instanceNames.Size(); i++ )
	{
		instanceNames[i] = snapshot->m_instanceSnapshots[i]->m_instanceName;
	}

	ActivateBehaviorInstances( instanceNames );

	// Find suitable snapshot and restore it
	for( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		CBehaviorGraphInstance* instance = m_instances[i];
		for( Uint32 j=0; i<snapshot->m_instanceSnapshots.Size(); i++ )
		{
			CBehaviorGraphInstanceSnapshot* instanceSnapshot = snapshot->m_instanceSnapshots[j];
			if( instanceSnapshot->m_instanceName == instance->GetInstanceName() )
			{
				instance->RestoreSnapshot( instanceSnapshot );
				break;
			}
		}
	}

	return true;
}
