/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphCurveDampValueNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "../engine/curve.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCurveDampValueNode );

CBehaviorGraphCurveDampValueNode::CBehaviorGraphCurveDampValueNode()
	: m_curve(NULL)
	, m_abscissaAxisScale(1.0f)
{
}

void CBehaviorGraphCurveDampValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_targetValue;
	compiler << i_prevTarget;
	compiler << i_curveTimer;
}

void CBehaviorGraphCurveDampValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_targetValue ] = 0.f;
	instance[ i_prevTarget ] = 0.f;
	instance[ i_curveTimer ] = 0.f;
}

void CBehaviorGraphCurveDampValueNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_targetValue );
	INST_PROP( i_prevTarget );
	INST_PROP( i_curveTimer );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphCurveDampValueNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_curve = CreateObject< CCurve >( this );
}

void CBehaviorGraphCurveDampValueNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Duration ), false ) );
}

#endif

void CBehaviorGraphCurveDampValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CurveDampValue );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float abscissaAxisScale;

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->Update( context, instance, timeDelta );
		abscissaAxisScale = m_cachedDurationValueNode->GetValue( instance );
	}
	else
	{
		abscissaAxisScale = m_abscissaAxisScale;
	}

	Float& dampedValue = instance[ i_value ];
	Float& targetValue = instance[ i_targetValue ];
	Float& prevTarget = instance[ i_prevTarget ];
	Float& curveTimer = instance[ i_curveTimer ];

	Float currTargetValue = m_cachedInputNode->GetValue( instance );
	Float diff = currTargetValue - targetValue;

	if ( MAbs(diff) > 0.01f )
	{
		// Reset
		prevTarget = targetValue;
		targetValue = currTargetValue;
		curveTimer = 0.0f;
	}

	if ( curveTimer < abscissaAxisScale )
	{
		// Calc new value
		dampedValue = ( targetValue - prevTarget ) * m_curve->GetFloatValue( curveTimer / abscissaAxisScale ) + prevTarget;

		// Inc timer
		curveTimer += timeDelta;
	}
}

void CBehaviorGraphCurveDampValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->Activate( instance );
	}

	// Reset timer
	instance[ i_curveTimer ] = 1.0f;
	instance[ i_value ] = m_curve ? instance[ i_value ] * m_curve->GetFloatValue(1.0f) : instance[ i_value ];
}

void CBehaviorGraphCurveDampValueNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphCurveDampValueNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->ProcessActivationAlpha( instance, 1.0f );
	}
}

void CBehaviorGraphCurveDampValueNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedDurationValueNode = CacheValueBlock( TXT("Duration") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCurveMapValueNode );

CBehaviorGraphCurveMapValueNode::CBehaviorGraphCurveMapValueNode()
	: m_curve( NULL )
	, m_axisXScale( 1.0f )
	, m_valueScale( 1.f )
	, m_valueOffet( 0.f )
	, m_mirrorY( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphCurveMapValueNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_curve = CreateObject< CCurve >( this );
}

#endif

void CBehaviorGraphCurveMapValueNode::CalcValue( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		const Float inputValue = ( m_cachedInputNode->GetValue( instance ) - m_valueOffet ) / m_axisXScale;

		Float& value = instance[ i_value ];
		RED_FATAL_ASSERT( m_curve, "" );

		if ( m_mirrorY )
		{
			value = inputValue >= 0.f ? 
				m_valueScale * m_curve->GetFloatValue( inputValue ) :
				-m_valueScale * m_curve->GetFloatValue( -inputValue );
		}
		else
		{
			value = m_valueScale * m_curve->GetFloatValue( inputValue );
		}
	}
}

void CBehaviorGraphCurveMapValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CurveMapValue );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	CalcValue( instance );
}

void CBehaviorGraphCurveMapValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	CalcValue( instance );
}
