/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphRandomNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animSyncInfo.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRandomNode );

CBehaviorGraphRandomNode::CBehaviorGraphRandomNode()
{
}

void CBehaviorGraphRandomNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currentInput;
	compiler << i_currentCooldowns;
	compiler << i_activedInput;
}

void CBehaviorGraphRandomNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currentInput ] = NULL;
	instance[ i_activedInput ] = false;

	TDynArray< Float >& currentCooldowns = instance[ i_currentCooldowns ];

	currentCooldowns.Resize( m_cooldowns.Size() );

	// Fill cooldowns current value
	for ( Uint32 i=0; i<currentCooldowns.Size(); ++i )
	{
		currentCooldowns[i] = m_cooldowns[i];
	}
}

void CBehaviorGraphRandomNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_activedInput );
	INST_PROP( i_currentInput );
	INST_PROP( i_currentCooldowns );
}

void CBehaviorGraphRandomNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	OnInputListChange();
}

void CBehaviorGraphRandomNode::OnPropertyPostChange( IProperty *prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	// Input list has changed
	if ( prop->GetName() == TXT("weights") ||
		 prop->GetName() == TXT("cooldowns") ||
		 prop->GetName() == TXT("maxStartAnimTime") )
	{
		OnInputListChange();
	}
}

void CBehaviorGraphRandomNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Random );

	TDynArray< Float >& currentCooldowns = instance[ i_currentCooldowns ];

	// Update cooldowns
	for( Uint32 i=0; i<currentCooldowns.Size(); ++i )
	{
		currentCooldowns[i] -= timeDelta;
		if ( currentCooldowns[i] < 0.0f )
		{
			currentCooldowns[i] = 0.0f;
		}
	}

	// Select new random input
	if ( !instance[ i_currentInput ] )
	{
		SelectRandomInput( instance );

		// No input was selected ( empty node )
		if ( !instance[ i_currentInput ] )
		{
			return;
		}
	}

	// Get sync info from current playing node
	CSyncInfo syncInfo;
	syncInfo.m_wantSyncEvents = false;
	instance[ i_currentInput ]->GetSyncInfo( instance, syncInfo );

	// Calculate update time
	Float timeToUpdate = timeDelta;
	if ( syncInfo.m_currTime + instance[ i_currentInput ]->GetSyncData( instance ).m_timeMultiplier * timeDelta >= syncInfo.m_totalTime )
	{
		timeToUpdate = ( syncInfo.m_totalTime - syncInfo.m_currTime ) / instance[ i_currentInput ]->GetSyncData( instance ).m_timeMultiplier;		
	}

	// Apply time update
	instance[ i_currentInput ]->Update( context, instance, timeToUpdate );
	Float timeDeltaLeft = timeDelta - timeToUpdate;

	// Select next random input
	if ( timeDeltaLeft > 0.0f )
	{	
		SelectRandomInput( instance );

		// Update random input node
		if ( instance[ i_currentInput ] )
		{
			instance[ i_currentInput ]->Update( context, instance, timeDeltaLeft );
		}
	}
}

void CBehaviorGraphRandomNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( instance[ i_currentInput ] )
	{
		instance[ i_currentInput ]->Sample( context, instance, output );
	}
}

void CBehaviorGraphRandomNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( instance[ i_currentInput ] )
	{
		instance[ i_currentInput ]->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphRandomNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( instance[ i_currentInput ] )
	{
		instance[ i_currentInput ]->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphRandomNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( instance[ i_currentInput ] )
	{
		return instance[ i_currentInput ]->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphRandomNode::SelectRandomInput( CBehaviorGraphInstance& instance ) const
{
	Bool& activedInput = instance[ i_activedInput ];
	TDynArray< Float >& currentCooldowns = instance[ i_currentCooldowns ];

	// Make sure there are some random inputs
	Uint32 numInputs = GetNumInputs();
	if (  numInputs == 0  )
	{
		instance[ i_currentInput ] = NULL;
		return;
	}

	// Deactivate current random input
	if ( instance[ i_currentInput ] )
	{
		ASSERT( activedInput );
		instance[ i_currentInput ]->Deactivate( instance );
		activedInput = false;
	}

	// Calculate the weight sum
	Float weightSum = 0.0f;
	for( Uint32 i=0; i<m_weights.Size(); ++i )
	{
		// Ignore animations with non-zero cooldowns
		if ( currentCooldowns[i] <= 0.0f && m_cachedInputNodes[i] )
		{
			weightSum += m_weights[i];
		}
	}

	// Select random element
	Float randVal = GEngine->GetRandomNumberGenerator().Get< Float >() * weightSum;

	// Get the element
	Uint32 randInput = 0;
	while( randInput < numInputs )
	{
		if ( currentCooldowns[ randInput ] > 0.0f || !m_cachedInputNodes[randInput])
		{
			++randInput;
			continue;
		}

		if ( randVal <= m_weights[ randInput ] )
		{
			break;
		}

		randVal -= m_weights[ randInput ];
		++randInput;
	}

	randInput = Min( randInput, numInputs-1 );

	// Not enough inputs
	ASSERT( randInput < numInputs );

	// Get the input
	instance[ i_currentInput ] = m_cachedInputNodes[ randInput ];
	ASSERT(instance[ i_currentInput ]);

	// Reset cooldown
	currentCooldowns[ randInput ] = m_cooldowns[ randInput ];

	// Activate new input node
	if ( instance[ i_currentInput ] )
	{
		ASSERT( !activedInput );
		instance[ i_currentInput ]->Activate( instance );
		activedInput = true;
	}
}

void CBehaviorGraphRandomNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	// Reset cooldowns
	for( Uint32 i=0; i<m_cooldowns.Size(); ++i )
	{
		instance[ i_currentCooldowns ][i] = m_cooldowns[i];
	}

	// Reset input node
	ASSERT( !instance[ i_activedInput ] );
	instance[ i_currentInput ] = NULL;
}

void CBehaviorGraphRandomNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{	
	// Deactivate input
	if ( instance[ i_activedInput ] )
	{
		ASSERT( instance[ i_currentInput ] );
		instance[ i_currentInput ]->Deactivate( instance );
		instance[ i_activedInput ] = false;
	}
}

void CBehaviorGraphRandomNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	// Pass to active node only
	if ( instance[ i_currentInput ] )
	{
		instance[ i_currentInput ]->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphRandomNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	// Reset active node
	if ( instance[ i_activedInput ] )
	{
		ASSERT( instance[ i_currentInput ] );
		instance[ i_currentInput ]->Deactivate( instance );
		instance[ i_activedInput ] = false;
	}

	// No active node from now on
	instance[ i_currentInput ] = NULL;

	// Set starting times
	ASSERT( m_maxStartAnimTime.Size() >= m_cachedInputNodes.Size() );

	for ( Uint32 i=0; i<m_cachedInputNodes.Size(); i++ )
	{
		if ( m_cachedInputNodes[i] && m_cachedInputNodes[i]->IsA< CBehaviorGraphAnimationNode >() )
		{
			Float maxTime = m_maxStartAnimTime[i];

			if ( maxTime < 0.01f ) 
			{
				continue;
			}

			CBehaviorGraphAnimationNode* node = Cast< CBehaviorGraphAnimationNode >( m_cachedInputNodes[i] );
			Float startTime = GEngine->GetRandomNumberGenerator().Get< Float >( maxTime );
			node->SetAnimTime( instance, startTime );
		}
	}
}

void CBehaviorGraphRandomNode::AddInput()
{
	// Add input
	m_weights.PushBack( 1.0f );
	m_cooldowns.PushBack( 0.0f );

	// List has changed
	OnInputListChange();
}

void CBehaviorGraphRandomNode::RemoveInput( Uint32 index )
{
	ASSERT( index < m_weights.Size() );

	// Remove input
	m_weights.Erase( m_weights.Begin() + index );
	m_cooldowns.Erase( m_cooldowns.Begin() + index );

	// List has changed
	OnInputListChange();
}

Uint32 CBehaviorGraphRandomNode::GetNumInputs() const
{
	return m_weights.Size();
}

void CBehaviorGraphRandomNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Clear list
	m_cachedInputNodes.Clear();

	// Get connected inputs
	const Uint32 numInputs = GetNumInputs();
	for ( Uint32 i=0; i<numInputs; i++ )
	{
		String socketName = String::Printf( TXT("Input%d"), i );
		CBehaviorGraphNode* node = CacheInputBlock( socketName );
		m_cachedInputNodes.PushBack( node );
	}
}

CBehaviorGraphNode* CBehaviorGraphRandomNode::CacheInputBlock( const String& socketName )
{
	return CacheBlock( socketName );
}

void CBehaviorGraphRandomNode::OnInputListChange()
{
	Uint32 newNumInputs = Max( m_weights.Size(), m_cooldowns.Size() );

	// Fill weights
	while ( m_weights.Size() < newNumInputs )
	{
		m_weights.PushBack( 1.0f );
	}
	
	// Fill cooldowns
	while( m_cooldowns.Size() < newNumInputs )
	{
		m_cooldowns.PushBack( 0.0f );
	}

	// Fill starting times value
	while( m_maxStartAnimTime.Size() < newNumInputs )
	{
		m_maxStartAnimTime.PushBack( 0.0f );
	}

	// Rebuild block sockets	
#ifndef NO_EDITOR_GRAPH_SUPPORT

	OnRebuildSockets();

#endif
}


#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRandomNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Output socket
	CreateOutputSocket();

	// Input sockets
	const Uint32 numInputs = GetNumInputs();
	for ( Uint32 i=0; i<numInputs; ++i )
	{
		CName socketName( String::Printf( TXT("Input%d"), i ) );
		CreateInputSocket( socketName );
	}
}

void CBehaviorGraphRandomNode::CreateOutputSocket()
{
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

void CBehaviorGraphRandomNode::CreateInputSocket( const CName& socketName )
{
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( socketName ) );
}

String CBehaviorGraphRandomNode::GetCaption() const
{
	return TXT("Random");
}

#endif // NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRandomNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );

	if (! m_cachedInputNodes.Exist(instance[ i_currentInput ]))
	{
		// loaded - we have no idea what it was, just null it
		// don't worry about deactivation, as all are deactivated when loading snapshot
		instance[ i_currentInput ] = NULL;
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
