/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphRandomSelectNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRandomSelectNode );

CBehaviorGraphRandomSelectNode::CBehaviorGraphRandomSelectNode()
: m_inputNum( 2 )
, m_avoidSelectingPrevious( true )
{
}

void CBehaviorGraphRandomSelectNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_selectInput;
	compiler << i_prevSelectInput;
}

void CBehaviorGraphRandomSelectNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

void CBehaviorGraphRandomSelectNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_selectInput );
	INST_PROP( i_prevSelectInput );
}

CBehaviorGraphNode* CBehaviorGraphRandomSelectNode::GetSelectInput( CBehaviorGraphInstance& instance ) const
{
	Int32 selected = instance[i_selectInput];
	return selected >= 0 && selected < m_cachedInputNodes.SizeInt()? m_cachedInputNodes[ selected ] : nullptr;
}

void CBehaviorGraphRandomSelectNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphRandomSelectNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->Sample( context, instance, output );
	}
}

Bool CBehaviorGraphRandomSelectNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		return node->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphRandomSelectNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphRandomSelectNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_selectInput ] = -1;
	instance[ i_prevSelectInput ] = -1;
}

void CBehaviorGraphRandomSelectNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNodes.Size() == 1 )
	{
		instance[ i_selectInput ] = 1;
	}
	else if ( m_avoidSelectingPrevious )
	{
		Int32& selectInput = instance[ i_selectInput ];
		Int32& prevSelectInput = instance[ i_prevSelectInput ];
		while ( selectInput == prevSelectInput )
		{
			selectInput = GEngine->GetRandomNumberGenerator().Get< Int32 >( m_cachedInputNodes.SizeInt() );
		}
	}
	else
	{
		instance[ i_selectInput ] = GEngine->GetRandomNumberGenerator().Get< Int32 >( m_cachedInputNodes.SizeInt() );
	}

	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		return node->Activate( instance );
	}
}

void CBehaviorGraphRandomSelectNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->Deactivate( instance );
	}

	instance[ i_prevSelectInput ] = instance[ i_selectInput ];
}

void CBehaviorGraphRandomSelectNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphRandomSelectNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphRandomSelectNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( CBehaviorGraphNode* node = GetSelectInput( instance ) )
	{
		node->SynchronizeTo( instance, info );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRandomSelectNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );

	// Input sockets
	const Uint32 numInputs = GetNumInputs();
	for ( Uint32 i=0; i<numInputs; ++i )
	{
		CName socketName( String::Printf( TXT("Input%d"), i ) );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( socketName ) );
	}
}

#endif

void CBehaviorGraphRandomSelectNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input nodes
	m_cachedInputNodes.Clear();
	m_inputNum = 0;
	while ( true )
	{
		const String socketName = String::Printf( TXT("Input%d"), m_inputNum );
		if ( FindSocket( socketName ) )
		{
			if ( CBehaviorGraphNode* node = CacheBlock( socketName ) )
			{
				m_cachedInputNodes.PushBack( node );
			}
			++ m_inputNum;
		}
		else
		{
			break;
		}
	}

	//ASSERT( m_cachedInputNodes.SizeInt() == m_inputNum );
}

void CBehaviorGraphRandomSelectNode::AddInput()
{
	++ m_inputNum;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}

void CBehaviorGraphRandomSelectNode::RemoveInput( Uint32 index )
{
	if ( index < (Uint32)m_inputNum && m_inputNum > 1)
	{
		-- m_inputNum;
	}
#ifndef NO_EDITOR_GRAPH_SUPPORT
	OnRebuildSockets();
#endif
}
