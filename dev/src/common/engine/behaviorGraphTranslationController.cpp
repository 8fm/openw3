/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphTranslationController.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/game.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "extAnimDurationEvent.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPositionControllerBaseNode );

const Float CBehaviorGraphPositionControllerBaseNode::TRANSLATION_THRESHOLD = 0.01f; // 1mm

CBehaviorGraphPositionControllerBaseNode::CBehaviorGraphPositionControllerBaseNode()
	: m_useHeading( false )
{
}

void CBehaviorGraphPositionControllerBaseNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_nodeActive;
	compiler << i_shift;
}

void CBehaviorGraphPositionControllerBaseNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	ResetController( instance );
}

void CBehaviorGraphPositionControllerBaseNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_nodeActive );
	INST_PROP( i_shift );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPositionControllerBaseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Shift ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphPositionControllerBaseNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( PosController );

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

	UpdateController( context, instance, timeDelta );
}

void CBehaviorGraphPositionControllerBaseNode::UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->Update( context, instance, timeDelta );

		instance[ i_shift ] = m_cachedShiftVariableNode->GetVectorValue( instance );
		instance[ i_nodeActive ] = true;
	}
}

void CBehaviorGraphPositionControllerBaseNode::ResetController( CBehaviorGraphInstance& instance ) const
{
	instance[ i_shift ] = Vector::ZERO_3D_POINT;
	instance[ i_nodeActive ] = false;
}

void CBehaviorGraphPositionControllerBaseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_UPDATE( PosController );

	TBaseClass::Sample( context, instance, output );

	SampleController( context, instance, output );
}

void CBehaviorGraphPositionControllerBaseNode::SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( instance[ i_nodeActive ] )
	{
		const Vector& deltaShift = instance[ i_shift ];

		Translate( deltaShift, instance, output );
	}
}

void CBehaviorGraphPositionControllerBaseNode::Translate( const Vector& deltaShift, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Translate
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	Vector shift = deltaShift;

	if ( !m_useHeading && ac )
	{
		Matrix worldToLocal;
		ac->GetWorldToLocal( worldToLocal );
		shift = worldToLocal.TransformVector( deltaShift );
	}
#ifdef USE_HAVOK_ANIMATION
	const hkVector4& vec = TO_CONST_HK_VECTOR_REF( shift );
	hkQsTransform translation( vec , hkQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ), hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( translation.isOk() );

	output.m_deltaReferenceFrameLocal.setMul( output.m_deltaReferenceFrameLocal, translation );

	ASSERT( output.m_deltaReferenceFrameLocal.isOk() );
#else
	const RedVector4& vec = reinterpret_cast< const RedVector4& >( shift );
	RedQsTransform translation( vec , RedQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ), RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( translation.IsOk() );

	output.m_deltaReferenceFrameLocal.SetMul( output.m_deltaReferenceFrameLocal, translation );

	ASSERT( output.m_deltaReferenceFrameLocal.IsOk() );
#endif
}

void CBehaviorGraphPositionControllerBaseNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	ResetController( instance );
}

void CBehaviorGraphPositionControllerBaseNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->Activate( instance );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Activate( instance );
	}

	ResetController( instance );
}

void CBehaviorGraphPositionControllerBaseNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->Deactivate( instance );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphPositionControllerBaseNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedShiftVariableNode = CacheVectorValueBlock( TXT("Shift") );
	m_cachedWeightVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphPositionControllerBaseNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPositionControllerNode );

void CBehaviorGraphPositionControllerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_allowTrans;
	compiler << i_accShift;
	compiler << i_timer;
	compiler << i_transDuration;
	compiler << i_timeDelta;
}

void CBehaviorGraphPositionControllerNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_allowTrans );
	INST_PROP( i_accShift );
	INST_PROP( i_timer );
	INST_PROP( i_transDuration );
	INST_PROP( i_timeDelta );
}

Bool CBehaviorGraphPositionControllerNode::IsEventOccurred( const CName& eventName, const SBehaviorGraphOutput &output, Float& outEventDuration, CBehaviorGraphInstance& instance ) const
{
	for ( Uint32 i=0; i<output.m_numEventsFired; i++ )
	{
		const CAnimationEventFired& animEvent = output.m_eventsFired[i];
		ASSERT( animEvent.m_extEvent != NULL );

		// Only for duration events
		if( IsType< CExtAnimDurationEvent >( animEvent.m_extEvent ) 
			&& animEvent.m_extEvent->GetEventName() == eventName )
		{
			const CExtAnimDurationEvent* durEvent = static_cast< const CExtAnimDurationEvent* >( animEvent.m_extEvent );
			outEventDuration = durEvent->GetDuration();
			return true;
		}
	}

	return false;
}

void CBehaviorGraphPositionControllerNode::UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_useAnimEvent && m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->Update( context, instance, timeDelta );
		Vector newShift = m_cachedShiftVariableNode->GetVectorValue( instance );

		if ( m_continueUpdate )
		{
			ResetController( instance );

			if ( instance[ i_shift ].Mag3() > TRANSLATION_THRESHOLD || newShift.Mag3() > TRANSLATION_THRESHOLD )
			{
				instance[ i_nodeActive ] = true;
				instance[ i_shift ] = newShift;
				instance[ i_timer ] = 0.f;
			}
		}
		else if ( ( newShift - instance[ i_shift ] ).Mag3() > TRANSLATION_THRESHOLD )
		{
			ResetController( instance );

			if ( newShift.Mag3() > TRANSLATION_THRESHOLD )
			{
				instance[ i_nodeActive ] = true;
				instance[ i_shift ] = newShift;
				instance[ i_timer ] = 0.f;
			}
		}
	}
	else
	{
		instance[ i_shift ] = Vector::ZERO_3D_POINT;

		if ( m_cachedShiftVariableNode )
		{
			m_cachedShiftVariableNode->Update( context, instance, timeDelta );

			instance[ i_shift ] = m_cachedShiftVariableNode->GetVectorValue( instance );
			instance[ i_nodeActive ] = true;
		}
	}

	instance[ i_timeDelta ] += timeDelta;
}

void CBehaviorGraphPositionControllerNode::ResetController( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::ResetController( instance );

	instance[ i_accShift ] = Vector::ZERO_3D_POINT;
	instance[ i_timer ] = 0.f;
	instance[ i_allowTrans ] = false;
}

void CBehaviorGraphPositionControllerNode::SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( instance[ i_nodeActive ] )
	{
		Vector deltaShift = instance[ i_shift ];

		if ( m_useAnimEvent )
		{
			if ( !instance[ i_allowTrans ] )
			{
				Float eventDuration;

				// Listen output and wait for proper event
				if ( !IsEventOccurred( m_eventAllowTrans, output, eventDuration, instance ) )
				{
					return;
				}

				instance[ i_allowTrans ] = true;
				instance[ i_transDuration ] = eventDuration;
				instance[ i_timer ] = 0.f;
			}

			instance[ i_timer ] += instance[ i_timeDelta ];
			instance[ i_timeDelta ] = 0.f;

			// Calc current delta rotation
			ASSERT( instance[ i_transDuration ] > 0.f );

			Float progress = Clamp( instance[ i_timer ]/instance[ i_transDuration ], 0.f, 1.f ); // [0-1]

			Vector currShift = instance[ i_shift ] * progress;

			deltaShift = currShift - instance[ i_accShift ];

			// HACK that restricts the deltaShift not to be longer than 1 (teleport fix)
			if( deltaShift.Mag3() > 1.0f )
			{
				deltaShift.Normalize3();
			}

			instance[ i_accShift ] += deltaShift;

			if ( progress == 1.f )
			{
				//ASSERT( ( instance[ i_accShift ] - instance[ i_shift ] ).Mag3() <  NumericLimits<Float>::Epsilon() );
				ResetController( instance );
			}
		}

		Translate( deltaShift, instance, output );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPositionControllerWithDampNode );

void CBehaviorGraphPositionControllerWithDampNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
}

void CBehaviorGraphPositionControllerWithDampNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );
}

void CBehaviorGraphPositionControllerWithDampNode::UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedShiftVariableNode )
	{
		m_cachedShiftVariableNode->Update( context, instance, timeDelta );

		instance[ i_nodeActive ] = true;

		Vector& shift = instance[ i_shift ];

		shift = m_cachedShiftVariableNode->GetVectorValue( instance );
		
		if ( GGame->GetGameplayConfig().m_cameraPositionDamp )
		{
			Vector destShift = shift;
			Float weight = 1.f;

			Float dist = destShift.Normalize3();

			Float length = GGame->GetGameplayConfig().m_cameraPositionDampLength;

			Float diff = dist - length;

			if ( diff > GGame->GetGameplayConfig().m_cameraPositionDampLengthOffset )
			{
				weight = Max( 0.f, diff );
				//BEH_LOG( TXT("II %f - diff %f"), weight, diff );	
			}
			else
			{
				weight = Clamp( dist * GGame->GetGameplayConfig().m_cameraPositionDampSpeed * timeDelta, 0.f, dist );
				//BEH_LOG( TXT("I  %f - diff %f"), weight, diff );
			}

			shift = destShift * weight;
		}
	}
}

void CBehaviorGraphPositionControllerWithDampNode::ResetController( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::ResetController( instance );
}

void CBehaviorGraphPositionControllerWithDampNode::SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( instance[ i_nodeActive ] )
	{
		const Vector& currShift = instance[ i_shift ];
		Translate( currShift, instance, output );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMotionExFilterNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMotionExFilterNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphMotionExFilterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_control;
}

void CBehaviorGraphMotionExFilterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_control ] = false;
}

void CBehaviorGraphMotionExFilterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedControlVariableNode )
	{
		instance[ i_control ] = m_cachedControlVariableNode->GetValue( instance ) > 0.5f;
	}
}

void CBehaviorGraphMotionExFilterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( instance[ i_control ] )
	{
#ifdef USE_HAVOK_ANIMATION
		output.m_deltaReferenceFrameLocal.setIdentity();
#else
		output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
	}
}

void CBehaviorGraphMotionExFilterNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphMotionExFilterNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_control ] = false;
}

void CBehaviorGraphMotionExFilterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMotionExFilterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMotionExFilterNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}
