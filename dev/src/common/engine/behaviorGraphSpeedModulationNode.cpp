#include "build.h"
#include "behaviorGraphSpeedModulationNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSpeedModulationNode );
RED_DEFINE_STATIC_NAME( Direction );
RED_DEFINE_STATIC_NAME( SpeedThreshold );

CBehaviorGraphSpeedModulationNode::CBehaviorGraphSpeedModulationNode()
: m_speedThreshold( 1.01f )
, m_halfAngle( 0.4f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSpeedModulationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Direction ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( SpeedThreshold ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

#endif

Float CBehaviorGraphSpeedModulationNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	Float speed = 0.0f;
	Float direction = 0.0f;
	Float speedThreshold = m_speedThreshold;

	if ( m_cachedSpeedNode )
	{
		speed = m_cachedSpeedNode->GetValue( instance );
	}

	if ( m_cachedDirectionNode )
	{
		direction = m_cachedDirectionNode->GetValue( instance );
	}

	if ( m_cachedThresholdNode )
	{
		speedThreshold = m_cachedThresholdNode->GetValue( instance );
	}

	if ( speed <= speedThreshold || MAbs( direction ) <= m_halfAngle )
	{
		return speed;
	}
	else
	{
		return speedThreshold;
	}
}

void CBehaviorGraphSpeedModulationNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedSpeedNode )
	{
		m_cachedSpeedNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedDirectionNode )
	{
		m_cachedDirectionNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedThresholdNode )
	{
		m_cachedThresholdNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphSpeedModulationNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedSpeedNode )
	{
		m_cachedSpeedNode->Activate( instance );
	}

	if ( m_cachedDirectionNode )
	{
		m_cachedDirectionNode->Activate( instance );
	}

	if ( m_cachedThresholdNode )
	{
		m_cachedThresholdNode->Activate( instance );
	}
}

void CBehaviorGraphSpeedModulationNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedSpeedNode )
	{
		m_cachedSpeedNode->Deactivate( instance );
	}

	if ( m_cachedDirectionNode )
	{
		m_cachedDirectionNode->Deactivate( instance );
	}

	if ( m_cachedThresholdNode )
	{
		m_cachedThresholdNode->Deactivate( instance );
	}
}

void CBehaviorGraphSpeedModulationNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedSpeedNode )
	{
		m_cachedSpeedNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedDirectionNode )
	{
		m_cachedDirectionNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedThresholdNode )
	{
		m_cachedThresholdNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSpeedModulationNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedSpeedNode = CacheValueBlock( TXT("Speed") );
	m_cachedDirectionNode = CacheValueBlock( TXT("Direction") );
	m_cachedThresholdNode = CacheValueBlock( TXT("SpeedThreshold") );
}
