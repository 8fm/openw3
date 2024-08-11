/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphRotationController.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "extAnimDurationEvent.h"
#include "behaviorProfiler.h"

const Float CBehaviorGraphRotationControllerNode::ROTATION_THRESHOLD = 0.001f;

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRotationControllerNode );

CBehaviorGraphRotationControllerNode::CBehaviorGraphRotationControllerNode()
{
}

void CBehaviorGraphRotationControllerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_nodeActive;
	compiler << i_allowRot;
	compiler << i_destAngle;
	compiler << i_accRot;
	compiler << i_timer;
	compiler << i_timeDelta;
	compiler << i_rotDuration;
}

void CBehaviorGraphRotationControllerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	ResetController( instance );
}

void CBehaviorGraphRotationControllerNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_nodeActive );
	INST_PROP( i_allowRot );
	INST_PROP( i_destAngle );
	INST_PROP( i_accRot );
	INST_PROP( i_timer );
	INST_PROP( i_timeDelta );
	INST_PROP( i_rotDuration );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRotationControllerNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angle ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphRotationControllerNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( RotController );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Update( context, instance, timeDelta );
		
		if ( m_cachedWeightVariableNode->GetValue( instance ) < 0.5f )
		{
			instance[ i_nodeActive ] = false;
			return;
		}
	}

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->Update( context, instance, timeDelta );
		Float angle = m_cachedAngleVariableNode->GetValue( instance );

		if ( m_continueUpdate )
		{
			ResetController( instance );

			if ( MAbs( instance[ i_destAngle ] ) > ROTATION_THRESHOLD || MAbs( angle ) > ROTATION_THRESHOLD )
			{
				instance[ i_nodeActive ] = true;
				instance[ i_destAngle ] = angle;
				instance[ i_timer ] = 0.f;
			}
		}
		else if ( MAbs( angle - instance[ i_destAngle ] ) > ROTATION_THRESHOLD )
		{
			ResetController( instance );

			if ( MAbs( angle ) > ROTATION_THRESHOLD )
			{
				instance[ i_nodeActive ] = true;
				instance[ i_destAngle ] = angle;
				instance[ i_timer ] = 0.f;
			}
		}
	}

	instance[ i_timeDelta ] += timeDelta;
}

void CBehaviorGraphRotationControllerNode::ResetController( CBehaviorGraphInstance& instance ) const
{
	instance[ i_destAngle ] = 0.f;
	instance[ i_accRot ] = 0.f;
	instance[ i_timer ] = 0.f;
	instance[ i_allowRot ] = false;
	instance[ i_nodeActive ] = false;
}

void CBehaviorGraphRotationControllerNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( RotController );

	TBaseClass::Sample( context, instance, output );

	if ( instance[ i_nodeActive ] )
	{
		if ( !instance[ i_allowRot ] )
		{
			Float eventDuration;

			// Listen output and wait for proper event
			if ( !IsEventOccurred( m_eventAllowRot, output, eventDuration, instance ) )
			{
				return;
			}

			instance[ i_allowRot ] = true;
			instance[ i_rotDuration ] = eventDuration;
			instance[ i_timer ] = 0.f;
		}

		instance[ i_timer ] += instance[ i_timeDelta ];
		instance[ i_timeDelta ] = 0.f;

		// Calc current delta rotation
		ASSERT( instance[ i_rotDuration ] > 0.f );

		Float progress = Clamp( instance[ i_timer ]/ instance[ i_rotDuration ], 0.f, 1.f ); // [0-1]

		Float currRotDest = instance[ i_destAngle ] * progress;

		Float deltaRot = currRotDest - instance[ i_accRot ];

		instance[ i_accRot ] += deltaRot;

		Rotate( DEG2RAD( deltaRot ), output );

		if ( progress == 1.f )
		{
			ASSERT( MAbs( instance[ i_accRot ] - instance[ i_destAngle ] ) <  NumericLimits<Float>::Epsilon() );
			ResetController( instance );
		}
	}
}

Bool CBehaviorGraphRotationControllerNode::IsEventOccurred( const CName& eventName, const SBehaviorGraphOutput &output, Float& outEventDuration, CBehaviorGraphInstance& instance ) const
{
	for ( Uint32 i=0; i<output.m_numEventsFired; i++ )
	{
		const CAnimationEventFired& animEvent = output.m_eventsFired[i];
		ASSERT( animEvent.m_extEvent != NULL );

		// Only for duration events
		if( IsType< CExtAnimDurationEvent >( animEvent.m_extEvent  ) 
			&& animEvent.m_extEvent->GetEventName() == eventName )
		{
			const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( animEvent.m_extEvent );
			outEventDuration = durEvent->GetDuration();
			return true;
		}
	}

	return false;
}

void CBehaviorGraphRotationControllerNode::Rotate( Float angle, SBehaviorGraphOutput &output ) const
{
#ifdef USE_HAVOK_ANIMATION
	// apply rotation
	hkQuaternion rotationQuat( hkVector4( 0.f, 0.f, 1.f, 1.f ), angle );
	hkQsTransform rotation( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( rotation.isOk() );

	output.m_deltaReferenceFrameLocal.setMul( output.m_deltaReferenceFrameLocal, rotation );

	ASSERT( output.m_deltaReferenceFrameLocal.isOk() );
#else
	// apply rotation
	RedQuaternion rotationQuat( RedVector4( 0.0f, 0.0f, 1.0f, 1.0f ), angle );
	RedQsTransform rotation( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( rotation.IsOk() );

	output.m_deltaReferenceFrameLocal.SetMul( output.m_deltaReferenceFrameLocal, rotation );

	ASSERT( output.m_deltaReferenceFrameLocal.IsOk() );
#endif
}

void CBehaviorGraphRotationControllerNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	ResetController( instance );
}

void CBehaviorGraphRotationControllerNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->Activate( instance );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Activate( instance );
	}

	ResetController( instance );
}

void CBehaviorGraphRotationControllerNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->Deactivate( instance );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphRotationControllerNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedAngleVariableNode = CacheValueBlock( TXT("Angle") );
	m_cachedWeightVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphRotationControllerNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}
