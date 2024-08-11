/**
 * Copyright © 20013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSetInternalVariableFloatInOutNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphSocket.h"
#include "behaviorProfiler.h"
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSetInternalVariableFloatInOutNode );

CBehaviorGraphSetInternalVariableFloatInOutNode::CBehaviorGraphSetInternalVariableFloatInOutNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSetInternalVariableFloatInOutNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// keep names to make it work with base node
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Out ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Value ) ) );		
}

String CBehaviorGraphSetInternalVariableFloatInOutNode::GetCaption() const
{
	if ( m_variableName != CName::NONE )
	{
		return String::Printf( TXT("Set internal variable [ %s ]"), m_variableName.AsChar() );
	}

	return TXT("Set internal variable");
}

#endif

void CBehaviorGraphSetInternalVariableFloatInOutNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVar.PushBack( m_variableName );
}

void CBehaviorGraphSetInternalVariableFloatInOutNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SetInternalVariableFloatInOut );

	// update variable 
	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );
		if ( m_variableName )
		{
			instance.SetInternalFloatValue( m_variableName, m_cachedValueNode->GetValue( instance ) );
		}
	}
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		instance[ i_value ] = m_cachedInputNode->GetValue( instance );
	}
}

void CBehaviorGraphSetInternalVariableFloatInOutNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}
}

void CBehaviorGraphSetInternalVariableFloatInOutNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphSetInternalVariableFloatInOutNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSetInternalVariableFloatInOutNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	if (! m_cachedInputNode)
	{
		m_cachedInputNode = CacheValueBlock( TXT("In") );
	}
	m_cachedValueNode = CacheValueBlock( TXT("Value") );
}

