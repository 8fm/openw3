/**
 * Copyright © 20013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "behaviorGraphSetInternalVariableNode.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSetInternalVariableNode );

CBehaviorGraphSetInternalVariableNode::CBehaviorGraphSetInternalVariableNode()
	: m_setValueOnActivationAsWell( false )
	, m_setValueBeforeInputIsUpdated( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSetInternalVariableNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Value ) ) );
}

String CBehaviorGraphSetInternalVariableNode::GetCaption() const
{
	if ( m_variableName != CName::NONE )
	{
		if ( m_setValueBeforeInputIsUpdated )
		{
			return String::Printf( TXT("Set internal variable [ %s ] {before}"), m_variableName.AsChar() );
		}
		else
		{
			return String::Printf( TXT("Set internal variable [ %s ] {after}"), m_variableName.AsChar() );
		}
	}

	return TXT("Set internal variable");
}

#endif

void CBehaviorGraphSetInternalVariableNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVar.PushBack( m_variableName );
}

void CBehaviorGraphSetInternalVariableNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( SetInternalVariable );

	if ( m_setValueBeforeInputIsUpdated && m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );
		instance.SetInternalFloatValue( m_variableName, m_cachedValueNode->GetValue( instance ) );
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( !m_setValueBeforeInputIsUpdated && m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );
		instance.SetInternalFloatValue( m_variableName, m_cachedValueNode->GetValue( instance ) );
	}
}

void CBehaviorGraphSetInternalVariableNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedValueNode && m_setValueOnActivationAsWell )
	{
		instance.SetInternalFloatValue( m_variableName, m_cachedValueNode->GetValue( instance ) );
	}

	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}
}

void CBehaviorGraphSetInternalVariableNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphSetInternalVariableNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSetInternalVariableNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("In") );
	m_cachedValueNode = CacheValueBlock( TXT("Value") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInternalVariableCounterNode );

CBehaviorGraphInternalVariableCounterNode::CBehaviorGraphInternalVariableCounterNode()
	: m_countOnActivation( true )
	, m_countOnDeactivation( false )
	, m_stepValue( 1.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphInternalVariableCounterNode::GetCaption() const
{
	if ( m_variableName )
	{
		if ( m_countOnActivation )
		{
			return String::Printf( TXT("Internal variable counter [ %s ] {act}"), m_variableName.AsChar() );
		}
		else if ( m_countOnDeactivation )
		{
			return String::Printf( TXT("Internal variable counter [ %s ] {deact}"), m_variableName.AsChar() );
		}
	}

	return TXT("Internal variable counter");
}

#endif

void CBehaviorGraphInternalVariableCounterNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	intVar.PushBack( m_variableName );
}

void CBehaviorGraphInternalVariableCounterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( InternalVariableCounter );

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphInternalVariableCounterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_variableName && m_countOnActivation )
	{
		const Float val = instance.GetInternalFloatValue( m_variableName );
		instance.SetInternalFloatValue( m_variableName, val + m_stepValue );
	}

	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphInternalVariableCounterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_variableName && m_countOnDeactivation )
	{
		const Float val = instance.GetInternalFloatValue( m_variableName );
		instance.SetInternalFloatValue( m_variableName, val + m_stepValue );
	}

	TBaseClass::OnDeactivated( instance );
}
