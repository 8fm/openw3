/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphDampVectorValueNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDampVectorValueNode );

CBehaviorGraphDampVectorValueNode::CBehaviorGraphDampVectorValueNode()
	: m_increaseSpeed( Vector::ONES )
	, m_decreaseSpeed( Vector::ONES )
{
}

void CBehaviorGraphDampVectorValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_vecValue;
	compiler << i_increaseSpeed;
	compiler << i_decreaseSpeed;
}

void CBehaviorGraphDampVectorValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_vecValue ] = Vector::ZERO_3D_POINT;
	instance[ i_increaseSpeed ] = m_increaseSpeed;
	instance[ i_decreaseSpeed ] = m_decreaseSpeed;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphDampVectorValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Output ) ) );	

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Inc_speed ), false ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Dec_speed ), false ) );
}

#endif

void CBehaviorGraphDampVectorValueNode::DampValue( CBehaviorGraphInstance& instance, Float value, Int32 num, Float timeDelta ) const
{
	Vector& dampedValue = instance[ i_vecValue ];
	Vector& decreaseSpeed = instance[ i_decreaseSpeed ];
	Vector& increaseSpeed = instance[ i_increaseSpeed ];

	Float diff = value - dampedValue.A[num];

	if ( diff == 0 )
		return;

	Float changeSpeed = increaseSpeed.A[num];
	Float dir = 1.0f;
	Float lowerBound = -NumericLimits<Float>::Max();
	Float upperBound = value;
	if ( diff < 0.0f )
	{
		changeSpeed = decreaseSpeed.A[num];
		dir = -1.0f;
		lowerBound = value;
		upperBound = NumericLimits<Float>::Max();
	}

	if (m_absolute)
	{
		const Float absDir = Sgn<Float> (dampedValue.A[num]) * dir; 
		changeSpeed = (absDir < 0.f) ? decreaseSpeed.A[num] : increaseSpeed.A[num];
	}

	if (changeSpeed == 0.f)
	{
		// changeSpeed == Zero means we have to change the value immediately to the currValue.
		// But! We can't do that when m_absolute is true and we're crossing zero,
		// because the User might want to use different speed after crossing 0.
		const Bool doWeChangeTheSign = Sgn< Float >( value ) * Sgn< Float >( dampedValue.A[num] ) < 0.f;
		dampedValue.A[num] = ( m_absolute && doWeChangeTheSign && increaseSpeed.A[num] != decreaseSpeed.A[num] ) ? 0.f : value;
	}
	else
	{
		dampedValue.A[num] += dir * changeSpeed * timeDelta; 
		dampedValue.A[num] = Clamp( dampedValue.A[num], lowerBound, upperBound );
	}
}

void CBehaviorGraphDampVectorValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( DampVectorValue );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Update( context, instance, timeDelta );
		instance[ i_increaseSpeed ] = m_cachedIncSpeedNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_increaseSpeed ] = m_increaseSpeed;
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Update( context, instance, timeDelta );
		instance[ i_decreaseSpeed ] = m_cachedDecSpeedNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_decreaseSpeed ] = m_decreaseSpeed;
	}

	Vector currValue = m_cachedInputNode ? m_cachedInputNode->GetVectorValue( instance ) : Vector::ZEROS;

	DampValue( instance, currValue.A[0], 0, timeDelta );
	DampValue( instance, currValue.A[1], 1, timeDelta );
	DampValue( instance, currValue.A[2], 2, timeDelta );
}

Vector CBehaviorGraphDampVectorValueNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_vecValue ];
}

void CBehaviorGraphDampVectorValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_vecValue ] = m_cachedInputNode ? m_cachedInputNode->GetVectorValue( instance ) : Vector::ZEROS;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Activate( instance );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Activate( instance );
	}
}

void CBehaviorGraphDampVectorValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->Deactivate( instance );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->Deactivate( instance );
	}
}

void CBehaviorGraphDampVectorValueNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheVectorValueBlock( TXT("Input") );
	m_cachedIncSpeedNode = CacheVectorValueBlock( TXT("Inc speed") );
	m_cachedDecSpeedNode = CacheVectorValueBlock( TXT("Dec speed") );
}

void CBehaviorGraphDampVectorValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedIncSpeedNode )
	{
		m_cachedIncSpeedNode->ProcessActivationAlpha( instance, 1.0f );
	}

	if ( m_cachedDecSpeedNode )
	{
		m_cachedDecSpeedNode->ProcessActivationAlpha( instance, 1.0f );
	}
}
