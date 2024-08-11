/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphCameraControllerNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../engine/camera.h"
#include "../engine/game.h"
#include "../engine/renderFrame.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "behaviorIncludes.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCameraControllerNode );

const Float CBehaviorGraphCameraControllerNode::RESET_DURATION = 0.5f;

CBehaviorGraphCameraControllerNode::CBehaviorGraphCameraControllerNode()
	: m_valueScale( 1.0f )
	, m_axis( A_Z )
	, m_clamp( false )
	, m_angleMin( -180.f )
	, m_angleMax( 180.f )
{
}

void CBehaviorGraphCameraControllerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currAngle;
	compiler << i_boneIndex;
	compiler << i_reset;
	compiler << i_weight;
	compiler << i_resetEvent;
	compiler << i_hardResetEvent;
	compiler << i_resetTimer;
	compiler << i_resetAngle;
}

void CBehaviorGraphCameraControllerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currAngle ] = 0.f;
	instance[ i_reset ] = false;
	instance[ i_weight ] = 0.f;

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_resetTimer ] = 0.f;
	instance[ i_resetAngle ] = 0.f;

	if ( m_axis == A_Y )
	{
		instance[ i_resetEvent ] = -1;
		instance[ i_hardResetEvent ] = -1;
	}
	else
	{
		instance[ i_resetEvent ] = instance.GetEventId( m_axis == A_Z ? CNAME( EVT_RESET_ROT_HOR ) : CNAME( EVT_RESET_ROT_VER ) );
		instance[ i_hardResetEvent ] = instance.GetEventId( m_axis == A_Z ? CNAME( EVT_HARD_RESET_ROT_HOR ) : CNAME( EVT_HARD_RESET_ROT_VER ) );
	}
}

void CBehaviorGraphCameraControllerNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_currAngle );
	INST_PROP( i_reset );
	INST_PROP( i_weight );
	INST_PROP( i_boneIndex );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphCameraControllerNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angular_vel ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angle ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

Bool CBehaviorGraphCameraControllerNode::IsWorking( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_weight ] > 0.5f ? true : false;
}

Float CBehaviorGraphCameraControllerNode::GetAngle( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_currAngle ];
}

Float CBehaviorGraphCameraControllerNode::GetResetAngleDuration( CBehaviorGraphInstance& instance ) const
{
	const Float* value = instance.GetFloatValuePtr( CNAME( VAR_RESET_ROT_DURATION ) );

	if ( value && GGame && GGame->IsActive() )
	{
		return *value > 0.01f ? *value : RESET_DURATION;
	}

	return RESET_DURATION;
}

Bool CBehaviorGraphCameraControllerNode::GetDestinationAngle( CBehaviorGraphInstance& instance, Float& angle ) const
{
	const Float* value = instance.GetFloatValuePtr( m_axis == A_Z ? CNAME( RESET_ANGLE_LEFT_RIGHT ) : CNAME( RESET_ANGLE_UP_DOWN ) );

	if ( !value )
	{	
		angle = 0.f;
		return false;
	}

	if ( GGame && GGame->IsActive() )
	{
		angle = *value;

		if ( angle > 720.f && m_axis == A_Z )
		{
			// You can't wirte proper angle in camera because target position will be valid after locomotion tick and update transform
			// This is not teh best solution but i haven't got time for it
			const CCamera* cam = Cast< const CCamera >( instance.GetAnimatedComponent()->GetEntity() );
			if ( cam )
			{
				angle = cam->GetFollowTargetYaw() - instance.GetAnimatedComponent()->GetWorldYaw();				
			}
		}

		return true;
	}

	angle = 0.f;
	return true;
}

Bool CBehaviorGraphCameraControllerNode::GetFollowAngle( CBehaviorGraphInstance& instance, Float& angle ) const
{
	const Float* value = instance.GetFloatValuePtr( CNAME( HOR_FOLLOW_VAR ) );

	if ( value && *value > 0.5f )
	{
		// TODO
		// Tempshit

		if ( m_axis == A_Z  )
		{
			const CCamera* cam = Cast< const CCamera >( instance.GetAnimatedComponent()->GetEntity() );
			if ( cam )
			{
				angle = cam->GetFollowTargetYaw() - instance.GetAnimatedComponent()->GetWorldYaw();				
			}
		}
		else
		{
			ASSERT( m_axis == A_X );
			angle = 0;
		}

		return true;
	}

	return false;
}

void CBehaviorGraphCameraControllerNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CameraController );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedWeightInputNode )
	{
		m_cachedWeightInputNode->Update( context, instance, timeDelta );
		instance[ i_weight ] = m_cachedWeightInputNode->GetValue( instance );
	}

	if ( !IsWorking( instance ) )
	{
		return;
	}

	Float& currInstanceAngle = instance[ i_currAngle ];
	Float& resetTimer = instance[ i_resetTimer ];
	Float destAngle;

	if ( GetFollowAngle( instance, currInstanceAngle ) )
	{
		//...
	}
	else if ( instance[ i_reset ] )
	{
		GetDestinationAngle( instance, currInstanceAngle );
		instance[ i_reset ] = false;
	}
	else if ( resetTimer > 0.f && GetDestinationAngle( instance, destAngle ) )
	{
		resetTimer = Max( resetTimer - timeDelta, 0.f );
		Float currAngle = instance[ i_resetAngle ];

		Float weight = BehaviorUtils::BezierInterpolation( 1.f - Clamp( resetTimer / GetResetAngleDuration( instance ), 0.f, 1.f ) );

		Float dist = EulerAngles::AngleDistance( currAngle, destAngle );

		currInstanceAngle = currAngle + dist * weight;
	}
	else if ( m_cachedControlInputNode )
	{
		currInstanceAngle = currInstanceAngle + m_cachedControlInputNode->GetValue( instance ) * m_valueScale * timeDelta;
	}
	else if ( m_cachedControlAngleInputNode )
	{
		currInstanceAngle = currInstanceAngle + m_cachedControlAngleInputNode->GetValue( instance ) * m_valueScale;
	}

	NormalizeAngle( currInstanceAngle );

	ASSERT( currInstanceAngle >= -180.f && currInstanceAngle <= 180.f );

	if ( m_clamp )
	{
		currInstanceAngle = Clamp( currInstanceAngle, m_angleMin, m_angleMax );
	}
}

void CBehaviorGraphCameraControllerNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( CameraController );

	TBaseClass::Sample( context, instance, output );

	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( boneIndex != -1 && IsWorking( instance ) )
	{
#ifdef USE_HAVOK_ANIMATION
		hkQuaternion rotationQuat( BehaviorUtils::hkVectorFromAxis( m_axis ), DEG2RAD( GetAngle( instance ) ) );
		hkQsTransform rotation( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

		ASSERT( boneIndex < (Int32)output.m_numBones );
		output.m_outputPose[ boneIndex ].setMul( output.m_outputPose[ boneIndex ], rotation );
#else
		RedQuaternion rotationQuat( BehaviorUtils::RedVectorFromAxis( m_axis ), DEG2RAD( GetAngle( instance ) ) );
		RedQsTransform rotation( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

		ASSERT( boneIndex < (Int32)output.m_numBones );
		output.m_outputPose[ boneIndex ].SetMul( output.m_outputPose[ boneIndex ], rotation );
#endif
	}
}

void CBehaviorGraphCameraControllerNode::NormalizeAngle( Float& angle ) const
{
	// Angle [-180, 180]
	while( angle < -180.0f )
		angle += 360.0f;

	while( angle > 180.0f )
		angle -= 360.0f;
}

void CBehaviorGraphCameraControllerNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	instance[ i_reset ] = true; 
	instance[ i_resetTimer ] = 0.f;
	instance[ i_resetAngle ] = 0.f;
}

void CBehaviorGraphCameraControllerNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlInputNode )
	{
		m_cachedControlInputNode->Activate( instance );
	}

	if ( m_cachedWeightInputNode )
	{
		m_cachedWeightInputNode->Activate( instance );
	}

	if ( m_cachedControlAngleInputNode )
	{
		m_cachedControlAngleInputNode->Activate( instance );
	}

	instance[ i_reset ] = true;
}

void CBehaviorGraphCameraControllerNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlInputNode )
	{
		m_cachedControlInputNode->Deactivate( instance );
	}

	if ( m_cachedWeightInputNode )
	{
		m_cachedWeightInputNode->Deactivate( instance );
	}

	if ( m_cachedControlAngleInputNode )
	{
		m_cachedControlAngleInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphCameraControllerNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlInputNode = CacheValueBlock( TXT("Angular vel.") );
	m_cachedWeightInputNode = CacheValueBlock( TXT("Weight") );
	m_cachedControlAngleInputNode = CacheValueBlock( TXT("Angle") );
}

Bool CBehaviorGraphCameraControllerNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( instance[ i_resetEvent ] != -1 && instance[ i_resetEvent ] == (Int32)event.GetEventID() )
	{
		instance[ i_resetTimer ] = GetResetAngleDuration( instance );
		instance[ i_resetAngle ] = instance[ i_currAngle ];

		TBaseClass::ProcessEvent( instance, event );

		return true;
	}
	else if ( instance[ i_hardResetEvent ] != -1 && instance[ i_hardResetEvent ] == (Int32)event.GetEventID() )
	{
		instance[ i_reset ] = true;

		TBaseClass::ProcessEvent( instance, event );

		return true;
	}
	
	return TBaseClass::ProcessEvent( instance, event );
}

void CBehaviorGraphCameraControllerNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlInputNode )
	{
		m_cachedControlInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightInputNode )
	{
		m_cachedWeightInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlAngleInputNode )
	{
		m_cachedControlAngleInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

CBehaviorGraph* CBehaviorGraphCameraControllerNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCameraVerticalDampNode );

CBehaviorGraphCameraVerticalDampNode::CBehaviorGraphCameraVerticalDampNode()
	: m_length( 0.0f )
	, m_dampSpeed( 0.f )
{
}

void CBehaviorGraphCameraVerticalDampNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_currZ;
	compiler << i_destZ;
}

void CBehaviorGraphCameraVerticalDampNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_currZ ] = 0.f;
	instance[ i_destZ ] = 0.f;
}

void CBehaviorGraphCameraVerticalDampNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_currZ );
	INST_PROP( i_destZ );
}

void CBehaviorGraphCameraVerticalDampNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CameraVerticalDamp );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		Vector inputValue = m_cachedInputNode->GetVectorValue( instance );

		if ( IsWorking() )
		{
			// TODO
			//...
		}
	}
}

Bool CBehaviorGraphCameraVerticalDampNode::IsWorking() const
{
	return m_length > 0.f;
}

void CBehaviorGraphCameraVerticalDampNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( !m_generateEditorFragments )
	{
		return;
	}

	const Vector& pos = instance.GetAnimatedComponent()->GetWorldPositionRef();
	
	Vector dampPos = pos;
	dampPos.Z = instance[ i_value ].Z;

	frame->AddDebugSphere( pos, 0.33f, Matrix::IDENTITY, Color::GRAY );
	frame->AddDebugSphere( dampPos, 0.33f, Matrix::IDENTITY, Color::LIGHT_GREEN );
}
