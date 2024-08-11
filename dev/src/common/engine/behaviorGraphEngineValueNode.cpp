/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behaviorGraphEngineValueNode.h"
#include "behaviorGraphInstance.h"
#include "../engine/game.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/world.h"
#include "../engine/environmentManager.h"
#include "animatedComponent.h"
#include "actorInterface.h"
#include "camera.h"
#include "entity.h"
#include "behaviorProfiler.h"
#include "baseEngine.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphEngineValueNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphEngineVectorValueNode );
IMPLEMENT_RTTI_ENUM( EBehaviorEngineValueType );
IMPLEMENT_RTTI_ENUM( EBehaviorEngineVectorValueType );

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphEngineValueNode::CBehaviorGraphEngineValueNode()
	: m_manualControl( false )
	, m_engineValueType( BEVT_TimeDelta )
{
}

String CBehaviorGraphEngineValueNode::GetType() const
{
	switch ( m_engineValueType )
	{
	case BEVT_TimeDelta :
		return TXT("Time Delta");
	case BEVT_ActorToCameraAngle :
		return TXT("Actor To Camera Angle");
	case BEVT_ActorSpeed :
		return TXT("Actor Speed");
	case BEVT_ActorMoveDirection :
		return TXT("Actor Move Direction");
	case BEVT_ActorHeading :
		return TXT("Actor Heading");
	case BEVT_ActorRotationSpeed :					// PAKSAS TODO: do wywalenia
		return TXT("Actor Rotation Speed");
	case BEVT_ActorRelativeDirection :				// PAKSAS TODO: do wywalenia
		return TXT("Actor Relative Direction");
	case BEVT_ActorRotation :
		return TXT("Actor Rotation");
	case BEVT_ActorRawDesiredRotation:
		return TXT("Actor Raw Desired Rotation");
	case BEVT_CameraFollowAngle:
		return TXT("Camera Follow Angle");
	case BEVT_ActorLookAtLevel:
		return TXT("Actor look at level");
	case BEVT_ActorLookAtEnabled:
		return TXT("Actor look at enabled");
	case BEVT_Pad:
		return TXT("Pad");
	case BEVT_ActorAnimState:
		return TXT("Actor Anim State");
	case BEVT_ActorMoveDirToFacingDiff:
		return TXT("Facing Relative Move Dir");
	case BEVT_AnimationMultiplier:
		return TXT("Animation multiplier");
	case BEVT_IsActorInScene:
		return TXT("Is Actor in Scene");
	case BEVT_CurrentBehaviorGraphInstanceTimeActive:
		return TXT("Current behavior graph instance's time active");
	}

	ASSERT( 0 );

	return TXT("Unknown");
}

String CBehaviorGraphEngineValueNode::GetCaption() const
{
	return String::Printf( TXT("Engine value [ %s ]"), GetType().AsChar() );
}

void CBehaviorGraphEngineValueNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
	compiler << i_updateID;
}

void CBehaviorGraphEngineValueNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = 0.f;
	instance[ i_updateID ] = 0;
}

void CBehaviorGraphEngineValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_updateID ] = 0;
	instance[ i_value ] = 0.f;
}

void CBehaviorGraphEngineValueNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_updateID ] = GEngine->GetCurrentEngineTick();
	instance[ i_value ] = GetValueInternal( instance, 0.f );
}

void CBehaviorGraphEngineValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( EngineValue );

	instance[ i_updateID ] = GEngine->GetCurrentEngineTick();
	instance[ i_value ] = GetValueInternal( instance, timeDelta );
}

Float CBehaviorGraphEngineValueNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	if ( !m_manualControl )
	{
		Float& value = instance[ i_value ];

		const Uint64 currTick = GEngine->GetCurrentEngineTick();
		const Uint64 updateId = instance[ i_updateID ];
		if ( currTick != updateId )
		{
			instance[ i_updateID ] = currTick;
			value = GetValueInternal( instance, 0.f );
		}

		ASSERT( !Red::Math::NumericalUtils::IsNan( value ) );

		return value;
	}
	else
	{
		return TBaseClass::GetValue( instance );
	}
}

Float CBehaviorGraphEngineValueNode::GetValueInternal( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	RED_ASSERT( IsActive( instance ) );
	RED_ASSERT( instance[ i_updateID ] == GEngine->GetCurrentEngineTick() );

	switch ( m_engineValueType )
	{
	case BEVT_TimeDelta :
		return timeDelta;
	case BEVT_ActorToCameraAngle :
		return GetActorToCameraAngle( instance );
	case BEVT_ActorSpeed :
		return GetRelativeMoveSpeed( instance );
	case BEVT_ActorMoveDirection:
		return GetActorMoveDirection( instance );
	case BEVT_ActorHeading :
		return GetActorHeading( instance );
	case BEVT_ActorRotationSpeed :
		return GetActorRotationSpeed( instance );
	case BEVT_ActorRelativeDirection :
		return GetRelativeDirection( instance );
	case BEVT_ActorRotation:
		return GetActorRotation( instance );
	case BEVT_ActorRawDesiredRotation:
		return GetActorRawDesiredRotation( instance );
	case BEVT_CameraFollowAngle:
		return GetCameraFollowAngle( instance );
	case BEVT_ActorLookAtLevel:
		return GetActorLookAtLevel( instance );
	case BEVT_ActorLookAtEnabled:
		return GetActorLookAtEnabled( instance );
	case BEVT_Pad:
		return GetPad( instance );
	case BEVT_ActorAnimState:
		return GetActorAnimState( instance );
	case BEVT_ActorMoveDirToFacingDiff:
		return GetActorMoveDirToFacingDiff( instance );
	case BEVT_AnimationMultiplier:
		return GetAnimationMultiplier( instance );
	case BEVT_IsActorInScene:
		return IsActorInScene( instance );
	case BEVT_CurrentBehaviorGraphInstanceTimeActive:
		return GetCurrentBehaviorGraphInstanceTimeActive( instance );
	default :
		ASSERT( 0 );
		return 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorToCameraAngle( CBehaviorGraphInstance& instance ) const
{
	const CEntity *entity = instance.GetAnimatedComponent()->GetEntity();
	if ( !entity )
	{
		return 0.0f;
	}

	return GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetCameraDirector()->GetNodeAngleInCameraSpace( entity ) : 0.f;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetRelativeMoveSpeed( CBehaviorGraphInstance& instance ) const
{
	return instance.GetAnimatedComponent()->GetRelativeMoveSpeed();
}

//////////////////////////////////////////////////////////////////////////
Float CBehaviorGraphEngineValueNode::GetActorMoveDirection( CBehaviorGraphInstance& instance ) const
{
	return ToBehaviorAngle( instance.GetAnimatedComponent()->GetMoveDirectionModelSpace() );
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorRotation( CBehaviorGraphInstance& instance ) const
{
	// PAKSAS TODO: tutaj wyjatkowo potrzebujemy world-space'owego kata i to nie przekonwertowanego na hrydek space !!! 
	// w odroznieniu do heading'u i direction'a
	return instance.GetAnimatedComponent()->GetMoveRotationWorldSpace();
}

Float CBehaviorGraphEngineValueNode::GetActorRawDesiredRotation( CBehaviorGraphInstance& instance ) const
{
	return instance.GetAnimatedComponent()->GetMoveRawDesiredRotationWorldSpace();
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorRotationSpeed( CBehaviorGraphInstance& instance ) const
{
	return instance.GetAnimatedComponent()->GetRotationSpeed();
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorHeading( CBehaviorGraphInstance& instance ) const
{
	return ToBehaviorAngle( instance.GetAnimatedComponent()->GetMoveHeadingModelSpace() );
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetRelativeDirection( CBehaviorGraphInstance& instance ) const
{
	// Actor to camera angle
	Float actorToCameraAngle = GetActorToCameraAngle( instance );
	ASSERT( !Red::Math::NumericalUtils::IsNan( actorToCameraAngle ) );

	Float rotationSpeed = instance.GetAnimatedComponent()->GetRotationSpeed();
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotationSpeed ) );

	// Angle diff
	Float angleDiff = actorToCameraAngle - rotationSpeed;
	
	// 'Normalize'
	while( angleDiff < -1.0f ) angleDiff += 2.0f;
	while( angleDiff > 1.0f ) angleDiff -= 2.0f;

	return angleDiff;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorMoveDirToFacingDiff( CBehaviorGraphInstance& instance ) const
{
	// Actor to camera angle
	Float orientation = instance.GetAnimatedComponent()->GetWorldYaw();
	ASSERT( !Red::Math::NumericalUtils::IsNan( orientation ) );

	Float moveDir = instance.GetAnimatedComponent()->GetDesiredMoveDirectionWorldSpace();
	ASSERT( !Red::Math::NumericalUtils::IsNan( moveDir ) );

	// Angle diff
	Float angleDiff = EulerAngles::AngleDistance( orientation, moveDir );

	return angleDiff;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetCameraFollowAngle( CBehaviorGraphInstance& instance ) const
{
	const CCamera* camera = Cast< const CCamera >( instance.GetAnimatedComponent()->GetEntity() );
	if ( !camera || !camera->IsFollowing() )
	{
		return 0.0f;
	}

	ASSERT( camera->IsFollowing() );

	Int32 orbitBoneIdx = FindBoneIndex( TXT("Camera_OrbitNode"), instance );
	if ( orbitBoneIdx == -1 )
	{
		return 0.f;
	}

	Float orbitBoneYawWS = instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( orbitBoneIdx ).ToEulerAngles().Yaw;
	Float targetYawWS = camera->GetFollowTargetYaw();

	Float diff = DistanceBetweenAngles( orbitBoneYawWS, targetYawWS );
	return diff;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorLookAtLevel( CBehaviorGraphInstance& instance ) const
{
 	const CEntity* entity = instance.GetAnimatedComponent()->GetEntity();
 	const IActorInterface* actor = entity->QueryActorInterface();
 	if ( actor )
 	{
 		return (Float)actor->GetLookAtLevel();
 	}

	return LL_Body;
}

Float CBehaviorGraphEngineValueNode::GetActorLookAtEnabled( CBehaviorGraphInstance& instance ) const
{
 	const CEntity* entity = instance.GetAnimatedComponent()->GetEntity();
 	const IActorInterface* actor = entity->QueryActorInterface();
 	if ( actor )
 	{
 		return actor->IsLookAtEnabled() ? 1.f : 0.f;
 	}

	return 0.f;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetPad( CBehaviorGraphInstance& instance ) const
{
	return GGame && GGame->IsUsingPad() ? 1.f : 0.f;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetActorAnimState( CBehaviorGraphInstance& instance ) const
{
	const CEntity* entity = instance.GetAnimatedComponent()->GetEntity();
	const IActorInterface* actor = entity->QueryActorInterface();
	if ( actor )
	{
		return (Float)actor->GetActorAnimState();
	}

	return 0.f;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetAnimationMultiplier( CBehaviorGraphInstance& instance ) const
{
	Float multiplier = 1.0f;
	if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager() )
	{
		multiplier *= GGame->GetActiveWorld()->GetEnvironmentManager()->GetCurrentAreaEnvironmentParams().m_gameplayEffects.m_behaviorAnimationMultiplier.GetCachedPoint().GetScalar();
	}
	multiplier *= instance.GetAnimatedComponent()->GetAnimationMultiplier();

	return multiplier;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::IsActorInScene( CBehaviorGraphInstance& instance ) const
{
	const IActorInterface* actor = instance.GetAnimatedComponent()->GetEntity()->QueryActorInterface();
	if ( actor )
	{
		return actor->IsInNonGameplayScene() ? 1.f : 0.f;
	}
	return 0.f;
}

//////////////////////////////////////////////////////////////////////////

Float CBehaviorGraphEngineValueNode::GetCurrentBehaviorGraphInstanceTimeActive( CBehaviorGraphInstance& instance ) const
{
	return instance.GetTimeActive();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehaviorGraphEngineVectorValueNode::CBehaviorGraphEngineVectorValueNode()
	: m_manualControl( false )
	, m_engineValueType( BEVVT_ActorLookAtTarget )
{
}

String CBehaviorGraphEngineVectorValueNode::GetCaption() const
{
	switch ( m_engineValueType )
	{
	case BEVVT_ActorLookAtTarget :
		return TXT("Engine vector value [ Actor look at target ]");
	case BEVVT_ActorLookAtCompressedData:
		return TXT("Engine vector value [ Actor look at compressed data ]");
	case BEVVT_ActorLookAtBodyPartWeights:
		return TXT("Engine vector value [ Actor look at body part weights ]");
	case BEVVT_ActorEyesLookAtData:
		return TXT("Engine vector value [ Actor eyes look at data ]");
	}

	return TXT("Engine vector value [ Unknown ]");
}

void CBehaviorGraphEngineVectorValueNode::OnReset( CBehaviorGraphInstance& instance ) const
{

}

void CBehaviorGraphEngineVectorValueNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	
}

Vector CBehaviorGraphEngineVectorValueNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	if ( !m_manualControl )
	{
		return GetValueInternal( instance );
	}
	else
	{
		return TBaseClass::GetVectorValue( instance );
	}
}

Vector CBehaviorGraphEngineVectorValueNode::GetValueInternal( CBehaviorGraphInstance& instance ) const
{
	switch ( m_engineValueType )
	{
	case BEVVT_ActorLookAtTarget :
		return GetLookAtTarget( instance );
	case BEVVT_ActorLookAtCompressedData:
		return GetLookAtCompressedData( instance );
	case BEVVT_ActorLookAtBodyPartWeights:
		return GetLookAtBodyPartWeights( instance );
	case BEVVT_ActorEyesLookAtData:
		return GetEyesLookAtData( instance );
	default: 
		return Vector::ZEROS;
	}
}

//////////////////////////////////////////////////////////////////////////

Vector CBehaviorGraphEngineVectorValueNode::GetLookAtTarget( CBehaviorGraphInstance& instance ) const
{
 	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
 	const CEntity* entity = ac->GetEntity();
 	const IActorInterface* actor = entity->QueryActorInterface();
 	if ( actor && actor->IsLookAtEnabled() )
 	{
 		// Get target in world space
 		Vector targetWS = actor->GetLookAtTarget();
 
 		 // Convert to animated component space
 		const Matrix& mat = ac->GetThisFrameTempLocalToWorld();
 		Vector targetMS = mat.FullInverted().TransformPoint( targetWS );
 
 		return targetMS;
 	}

	return Vector::ZERO_3D_POINT;
}

//////////////////////////////////////////////////////////////////////////

Vector CBehaviorGraphEngineVectorValueNode::GetLookAtBodyPartWeights( CBehaviorGraphInstance& instance ) const
{
 	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
 	const CEntity* entity = ac->GetEntity();
 	const IActorInterface* actor = entity->QueryActorInterface();
 	if ( actor && actor->IsLookAtEnabled() )
 	{
 		return actor->GetLookAtBodyPartsWeights();
 	}

	return Vector::ZERO_3D_POINT;
}

//////////////////////////////////////////////////////////////////////////

Vector CBehaviorGraphEngineVectorValueNode::GetLookAtCompressedData( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CEntity* entity = ac->GetEntity();
	const IActorInterface* actor = entity->QueryActorInterface();
	if ( actor && actor->IsLookAtEnabled() )
	{
		return actor->GetLookAtCompressedData();
	}

	return Vector::ZERO_3D_POINT;
}

//////////////////////////////////////////////////////////////////////////

Vector CBehaviorGraphEngineVectorValueNode::GetEyesLookAtData( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CEntity* entity = ac->GetEntity();
	const IActorInterface* actor = entity->QueryActorInterface();
	if ( actor && actor->IsLookAtEnabled() )
	{
		return actor->GetEyesLookAtCompressedData();
	}

	return Vector::ZEROS;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
