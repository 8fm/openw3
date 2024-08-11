/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintNodeFloorIK.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/traceTool.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "extAnimOnSlopeEvent.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/springDampers.h"
#include "../physics/physicsWorldUtils.h"
#include "../engine/skeleton.h"
#include "../engine/behaviorProfiler.h"
#include "../engine/skeletonUtils.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

#define MIN_TIME_DELTA 0.0001f

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKCommonData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKVerticalBoneData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKMaintainLookBoneData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKLegsData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKCommon );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKVerticalBone );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKMaintainLookBone );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKLegs );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKCachedTrace );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKLeg );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKWeightHandler );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKLegsIKWeightHandler );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKFrontBackWeightHandler );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintNodeFloorIKDebugTrace );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIKBase );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIK );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIKHandsOnly );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIKBipedLong );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIKQuadruped );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintNodeFloorIKSixLegs );

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

// RED_DEFINE_STATIC_NAME( pelvis ); <- declared in name registry
RED_DEFINE_STATIC_NAME( l_thigh );
RED_DEFINE_STATIC_NAME( l_shin );
RED_DEFINE_STATIC_NAME( l_foot );
RED_DEFINE_STATIC_NAME( r_thigh );
RED_DEFINE_STATIC_NAME( r_shin );
RED_DEFINE_STATIC_NAME( r_foot );
RED_DEFINE_STATIC_NAME( l_shoulder );
RED_DEFINE_STATIC_NAME( l_bicep );
RED_DEFINE_STATIC_NAME( l_forearm );
RED_DEFINE_STATIC_NAME( l_hand );
RED_DEFINE_STATIC_NAME( r_shoulder );
RED_DEFINE_STATIC_NAME( r_bicep );
RED_DEFINE_STATIC_NAME( r_forearm );
RED_DEFINE_STATIC_NAME( r_hand );
RED_DEFINE_STATIC_NAME( spine2 );
RED_DEFINE_STATIC_NAME( neck );
RED_DEFINE_STATIC_NAME( neck1 );
RED_DEFINE_STATIC_NAME( neck2 );
// RED_DEFINE_STATIC_NAME( head ); <- declared in name registry

RED_DEFINE_STATIC_NAME( l_ankle2 );
RED_DEFINE_STATIC_NAME( l_foot2 );
RED_DEFINE_STATIC_NAME( l_toe2 );
RED_DEFINE_STATIC_NAME( r_ankle2 );
RED_DEFINE_STATIC_NAME( r_foot2 );
RED_DEFINE_STATIC_NAME( r_toe2 );
RED_DEFINE_STATIC_NAME( l_ankle1 );
RED_DEFINE_STATIC_NAME( l_foot1 );
RED_DEFINE_STATIC_NAME( l_toe1 );
RED_DEFINE_STATIC_NAME( r_ankle1 );
RED_DEFINE_STATIC_NAME( r_foot1 );
RED_DEFINE_STATIC_NAME( r_toe1 );
RED_DEFINE_STATIC_NAME( l_hand_tip );
RED_DEFINE_STATIC_NAME( r_hand_tip );

RED_DEFINE_STATIC_NAME( AlignToGround ); // we should disable ik when aligning to ground
RED_DEFINE_STATIC_NAME( ForceIKOn ); // switch on floor ik at all
RED_DEFINE_STATIC_NAME( FloorIKOff ); // switch off floor ik at all
RED_DEFINE_STATIC_NAME( FrontIKOff ); // switch off front ik only
RED_DEFINE_STATIC_NAME( BackIKOff ); // switch off back ik only
RED_DEFINE_STATIC_NAME( LegsIKOff ); // switch off leg ik (only two bone ik part)
RED_DEFINE_STATIC_NAME( Jumping );
RED_DEFINE_STATIC_NAME( OnSlope );
RED_DEFINE_STATIC_NAME( horse );

RED_DEFINE_STATIC_NAME( testFloorIK );
RED_DEFINE_STATIC_NAME( testFloorSlidingOnSlopeIK );

RED_DEFINE_STATIC_NAME( testLeft );
RED_DEFINE_STATIC_NAME( testLeftNormal );
RED_DEFINE_STATIC_NAME( testRight );
RED_DEFINE_STATIC_NAME( testRightNormal );

RED_DEFINE_STATIC_NAME( testBackLeft );
RED_DEFINE_STATIC_NAME( testBackLeftNormal );
RED_DEFINE_STATIC_NAME( testBackRight );
RED_DEFINE_STATIC_NAME( testBackRightNormal );

RED_DEFINE_STATIC_NAME( testFrontLeft );
RED_DEFINE_STATIC_NAME( testFrontLeftNormal );
RED_DEFINE_STATIC_NAME( testFrontRight );
RED_DEFINE_STATIC_NAME( testFrontRightNormal );

RED_DEFINE_STATIC_NAME( testMiddleLeft );
RED_DEFINE_STATIC_NAME( testMiddleLeftNormal );
RED_DEFINE_STATIC_NAME( testMiddleRight );
RED_DEFINE_STATIC_NAME( testMiddleRightNormal );

RED_DEFINE_STATIC_NAME( testUsePlane );
RED_DEFINE_STATIC_NAME( testPlaneNormal );

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintNodeFloorIKCommon::ReadyForNextFrame()
{
	m_slopeFromAnim = m_slopeFromAnimAccumulated;
	m_slopeFromAnimAccumulated = 0.0f;
	m_jumping = m_jumpingAccumulator;
	m_jumpingAccumulator = false;
}

void SBehaviorConstraintNodeFloorIKCommon::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.m_type == AET_Duration || event.m_type == AET_DurationStartInTheMiddle )
	{
		if ( event.GetEventName() == CNAME( OnSlope ) )
		{
			ASSERT( event.m_extEvent->GetClass()->IsA( CExtAnimOnSlopeEvent::GetStaticClass() ) );
			const CExtAnimOnSlopeEvent * onSlopeEvent = (CExtAnimOnSlopeEvent*)( event.m_extEvent );
			m_slopeFromAnimAccumulated += onSlopeEvent->GetSlopeAngle() * event.m_alpha;
		}
		if ( event.GetEventName() == CNAME( Jumping ) )
		{
			m_jumpingAccumulator = true;
		}
	}
}

void SBehaviorConstraintNodeFloorIKCommon::Setup( SBehaviorConstraintNodeFloorIKCommonData const & data, CBehaviorGraphInstance& instance )
{
	const CCharacterControllerParam* params = instance.GetAnimatedComponent()->GetEntity()->GetEntityTemplate()->FindGameplayParamT<CCharacterControllerParam>();
	if( !params )
	{
		params = CCharacterControllerParam::GetStaticClass()->GetDefaultObject< CCharacterControllerParam > ();
	}
	m_cachedStepOffset = params? params->m_stepOffset : 1.5f;
	m_centreOfGravityCentreAlt = 1.0f;
	if ( instance.GetAnimatedComponent() && instance.GetAnimatedComponent()->GetSkeleton() )
	{
		Int32 gravityCentreBone = instance.GetAnimatedComponent()->GetSkeleton()->FindBoneByName( data.m_gravityCentreBone );
		if ( gravityCentreBone >= 0 )
		{
			m_centreOfGravityCentreAlt = AnimVectorToVector( GetTranslation( instance.GetAnimatedComponent()->GetSkeleton()->GetBoneMS( gravityCentreBone ) ) ).Z;
		}
	}
	m_slopeFromAnim = 0.0f;
	m_slopeFromAnimAccumulated = 0.0f;
	m_slidingOnSlopeWeight = 0.0f;
	m_prevSlidingOnSlopeWeight = 0.0f;
	m_adjustOffsetByRaw = 0.0f;
	m_prevAdjustOffsetByRaw = 0.0f;
	m_abruptAdjustemntWeight = 0.0f;
	m_currentUpWS = Vector::EZ;
	m_requestedUpWS = Vector::EZ;
	m_velocityUpWS = Vector::ZEROS;
	m_feetDistAlongDirMS = Vector::ZEROS;
	m_feetDistCoef = 1.0f;
	m_feetDistOffset = 0.0f;
	m_feetDistAlongDirRequestedMS = m_feetDistAlongDirMS;
	m_feetDistCoefRequested = m_feetDistCoef;
	m_feetDistOffsetRequested = m_feetDistOffset;
	m_jumping = false;
	m_jumpingAccumulator = false;
	m_jumpingOffset = 0.0f;
	m_speedXY = 0.0f;
}

void SBehaviorConstraintNodeFloorIKCommon::Update( SBehaviorConstraintNodeFloorIKCommonData const & data, Float timeDelta, Float prevTimeDelta, CBehaviorGraphInstance& instance, Float weight )
{
	CAnimatedComponent const * animatedComponent = instance.GetAnimatedComponent();
	CMovingAgentComponent const * movingAgentComponent = Cast< CMovingAgentComponent >( animatedComponent );

	// handle jumping offset - to know how much above ground are we
	if ( m_jumpingAccumulator )
	{
		if ( movingAgentComponent )
		{
			m_jumpingOffset = Max( 0.0f, m_jumpingOffset + movingAgentComponent->GetDeltaPositionFromBehaviorGraph().Z );
		}
	}
	else
	{
		m_jumpingOffset = BlendOnOffWithSpeedBasedOnTime( m_jumpingOffset, 0.0f, 0.3f, timeDelta );
	}

	if ( movingAgentComponent )
	{
		m_speedXY = movingAgentComponent->GetVelocityBasedOnRequestedMovement().Mag2();
	}
	else
	{
		m_speedXY = 0.0f;
	}

	DEBUG_ANIM_MATRIX( m_localToWorld );
	DEBUG_ANIM_MATRIX( m_prevLocalToWorld );

	// update local to world
	m_actualVelocityWS = prevTimeDelta > MIN_TIME_DELTA ? ( m_localToWorld.GetTranslation() - m_prevLocalToWorld.GetTranslation() ) / prevTimeDelta : Vector::ZEROS;
	DEBUG_ANIM_VECTOR( m_actualVelocityWS );

	if ( !m_useFixedVersion )
	{
		m_prevLocalToWorld = m_localToWorld;
		m_localToWorld = animatedComponent->GetEntity()->GetLocalToWorld();

		DEBUG_ANIM_MATRIX( m_localToWorld );
		DEBUG_ANIM_MATRIX( m_prevLocalToWorld );
	}
	m_useFullInverted = Abs( m_localToWorld.GetAxisX().SquareMag3() - 1.0f ) > 0.01f; // may we at least assume that scale is uniform, please?
	m_localToWorldInverted = m_useFullInverted? m_localToWorld.FullInverted() : m_localToWorld.Inverted();

	DEBUG_ANIM_MATRIX( m_localToWorld );
	DEBUG_ANIM_MATRIX( m_localToWorldInverted );

	// decide whether it is teleport or not
	m_additionalBlendCoefOnTeleport = ( m_localToWorld.GetTranslation() - m_prevLocalToWorld.GetTranslation() ).SquareMag3() > 50.0f * 50.0f? 1.0f : 0.0f;
	if ( m_additionalBlendCoefOnTeleport > 0.0f )
	{
		m_prevLocalToWorld = m_localToWorld;

		DEBUG_ANIM_MATRIX( m_prevLocalToWorld );
	}

	if ( m_currentUpWS != m_requestedUpWS && timeDelta > MIN_TIME_DELTA ) 
	{
		Float speed = m_actualVelocityWS.Mag2();

		Float speedCoef = Clamp( speed / data.m_speedForFullRootRotationBlend, 0.0f, 1.0f );
		Float rootRotationBlendTime = data.m_rootRotationBlendTime * ( 2.5f - 2.0f * speedCoef );

		TVectorCriticalDampRef dampUpWS( rootRotationBlendTime );
		dampUpWS.Update( m_currentUpWS, m_velocityUpWS, m_requestedUpWS, timeDelta );
	}

	m_useExtendedIKOffset = movingAgentComponent && movingAgentComponent->GetAnimationProxy().UseExtendedIKOffset();

	// update sliding on slope
	{
		m_prevSlidingOnSlopeWeight = m_slidingOnSlopeWeight;
		Float targetSlidingOnSlopeWeight = movingAgentComponent && movingAgentComponent->GetAnimationProxy().GetSlidingOnSlopeIK()? 1.0f : 0.0f;
#ifdef DEBUG_FLOOR_IK
		if ( instance.IsOpenInEditor() )
		{
			targetSlidingOnSlopeWeight = instance.GetInternalFloatValue( m_testVarSlidingOnSlopeID );
		}
#endif
		m_slidingOnSlopeWeight = BlendOnOffWithSpeedBasedOnTime( m_slidingOnSlopeWeight, targetSlidingOnSlopeWeight, data.m_slidingOnSlopeBlendTime, timeDelta );
	}

	Matrix rotateToCurrentMatWS = Matrix::IDENTITY;
	if ( m_currentUpWS != Vector::EZ )
	{
#ifndef USE_HAVOK_ANIMATION
		RedVector4 currentUpWS = reinterpret_cast< const RedVector4& >( m_currentUpWS );
		m_shortestRotationToCurrentWS.SetShortestRotation( RedVector4::EZ, currentUpWS );
		m_shortestRotationToCurrentMS.SetShortestRotation( VectorToAnimVector( m_localToWorldInverted.TransformVector( Vector::EZ ) ), VectorToAnimVector( m_localToWorldInverted.TransformVector( m_currentUpWS ) ) );
		RedMatrix4x4 rotateToCurrentRedMat = BuildFromQuaternion( m_shortestRotationToCurrentWS.Quat );
		rotateToCurrentMatWS = reinterpret_cast< const Matrix& >( rotateToCurrentRedMat );
		Matrix moveByCentreOfGravity = Matrix::IDENTITY;
		moveByCentreOfGravity.SetTranslation( Vector( 0.0f, 0.0f, m_centreOfGravityCentreAlt ) );
		rotateToCurrentMatWS = rotateToCurrentMatWS * moveByCentreOfGravity;
		moveByCentreOfGravity.SetTranslation( Vector( 0.0f, 0.0f, -m_centreOfGravityCentreAlt * currentUpWS.Z ) );
		rotateToCurrentMatWS = moveByCentreOfGravity * rotateToCurrentMatWS;
#endif
	}
	else
	{
#ifndef USE_HAVOK_ANIMATION
		m_shortestRotationToCurrentWS = RedQuaternion::IDENTITY;
		m_shortestRotationToCurrentMS = RedQuaternion::IDENTITY;
#endif
	}

	if ( m_currentUpWS != Vector::EZ )
	{
		Matrix localToWorldNoTranslation = m_localToWorld;
		Matrix localToWorldInvertedNoTranslation = m_localToWorldInverted;
		localToWorldNoTranslation.SetTranslation( Vector::ZEROS );
		localToWorldInvertedNoTranslation.SetTranslation( Vector::ZEROS );
		m_rootSpaceMS = localToWorldNoTranslation * rotateToCurrentMatWS * localToWorldInvertedNoTranslation;
		m_rootSpaceMSInverted = m_useFullInverted? m_rootSpaceMS.FullInverted() : m_rootSpaceMS.Inverted();
		m_rootSpaceWS = m_rootSpaceMS * m_localToWorld;
		m_rootSpaceWSInverted = m_useFullInverted? m_rootSpaceWS.FullInverted() : m_rootSpaceWS.Inverted();
	}
	else
	{
		m_rootSpaceMS = Matrix::IDENTITY;
		m_rootSpaceMSInverted = Matrix::IDENTITY;
		m_rootSpaceWS = m_localToWorld;
		m_rootSpaceWSInverted = m_localToWorldInverted;
	}

	DEBUG_ANIM_MATRIX( m_rootSpaceMSInverted );
	DEBUG_ANIM_MATRIX( m_rootSpaceMS );

	// update upright normal MS or RS
	if ( m_footUprightNormalMSMoreImportant )
	{
		m_footUprightNormalRS = m_rootSpaceMSInverted.TransformVector( m_footUprightNormalMS );
	}
	else
	{
		m_footUprightNormalMS = m_rootSpaceMS.TransformVector( m_footUprightNormalRS );
	}

	DEBUG_ANIM_VECTOR( m_footUprightNormalRS );
	DEBUG_ANIM_VECTOR( m_footUprightNormalMS );

	m_feetDistAlongDirRequestedMS.Z = 1.0f;
	if ( Vector::Dot2( m_feetDistAlongDirRequestedMS, m_feetDistAlongDirMS ) < 0.0f )
	{
		// we want to take shortest path along dir, we don't care about actual direction whether we're in plus or minus
		m_feetDistAlongDirRequestedMS = -m_feetDistAlongDirRequestedMS;
		m_feetDistOffsetRequested = -m_feetDistOffsetRequested;
	}
	m_feetDistCoef = BlendToWithBlendTime( m_feetDistCoef, m_feetDistCoefRequested, 0.2f, timeDelta + m_additionalBlendCoefOnTeleport );
	m_feetDistOffset = BlendToWithBlendTime( m_feetDistOffset, m_feetDistOffsetRequested, 0.2f, timeDelta + m_additionalBlendCoefOnTeleport );
	m_feetDistAlongDirMS = BlendToWithBlendTime( m_feetDistAlongDirMS, m_feetDistAlongDirRequestedMS, 0.2f, timeDelta + m_additionalBlendCoefOnTeleport );
	m_feetDistAlongDirRequestedMS.Normalize2();

	ReadyForNextFrame();
}

void SBehaviorConstraintNodeFloorIKCommon::UpdateAdjustmentOffset( SBehaviorConstraintNodeFloorIKCommonData const & data, Float timeDelta, Float prevTimeDelta, Float weight )
{
	if ( !m_useFixedVersion )
	{
		// TODO handle moving bases (elevators)
		m_prevAdjustOffsetByRaw = m_adjustOffsetByRaw;
		m_adjustOffsetByRaw = Vector::Dot3( m_footUprightNormalRS, ( m_prevLocalToWorld.GetTranslation() - m_localToWorld.GetTranslation() ) ) * weight * ( 1.0f - m_additionalBlendCoefOnTeleport );
		m_abruptAdjustemntWeight = Max( 0.0f, m_abruptAdjustemntWeight - timeDelta / 0.3f );
		if ( Abs( m_prevAdjustOffsetByRaw - m_adjustOffsetByRaw ) > 0.05f )
		{
			m_abruptAdjustemntWeight = 1.0f;
		}
		Float targetAdjustOffsetByVel = timeDelta > MIN_TIME_DELTA && prevTimeDelta > MIN_TIME_DELTA ? -m_adjustOffsetByRaw / prevTimeDelta : 0.0f;
		DEBUG_ANIM_FLOAT( targetAdjustOffsetByVel );

		targetAdjustOffsetByVel = targetAdjustOffsetByVel < 0.0f? targetAdjustOffsetByVel * 0.95f : targetAdjustOffsetByVel * 1.05f;
		// use mix to handle one frame jumps
		Float useTargetAdjustOffsetByVel = ( 0.3f * m_targetAdjustOffsetByVel + 0.7f * targetAdjustOffsetByVel ) * ( 1.0f - m_additionalBlendCoefOnTeleport );

		//if ( FPCHECK_1( m_adjustOffsetByVel ) || (m_adjustOffsetByVel < FLT_EPSILON && m_adjustOffsetByVel > -FLT_EPSILON) )
		//	m_adjustOffsetByVel = 0.0f;

		m_adjustOffsetByVel = BlendToWithBlendTimeLimit( m_adjustOffsetByVel, useTargetAdjustOffsetByVel, Abs( m_adjustOffsetByVel ) > Abs( useTargetAdjustOffsetByVel )? data.m_verticalVelocityOffsetDownBlendTime : data.m_verticalVelocityOffsetUpBlendTime, Min( 0.06f, timeDelta ) + m_additionalBlendCoefOnTeleport, 0.8f + m_additionalBlendCoefOnTeleport ) * ( 1.0f - m_additionalBlendCoefOnTeleport ); // TODO parameter?
		m_targetAdjustOffsetByVel = targetAdjustOffsetByVel;
	}
}

void SBehaviorConstraintNodeFloorIKCommon::SetUprightNormalRS( const Vector& worldUprightNormalWS, const Vector& footUprightNormalRS )
{
	m_worldUprightNormalWS = worldUprightNormalWS.Normalized3();
	m_footUprightNormalRS = footUprightNormalRS.Normalized3();
	m_footUprightNormalMS = m_rootSpaceMS.TransformVector( m_footUprightNormalRS );
	m_footUprightNormalMSMoreImportant = false;

	DEBUG_ANIM_VECTOR( m_footUprightNormalRS );
	DEBUG_ANIM_VECTOR( m_footUprightNormalMS );
}

void SBehaviorConstraintNodeFloorIKCommon::SetUprightNormalMS( const Vector& worldUprightNormalWS, const Vector& footUprightNormalMS )
{
	m_worldUprightNormalWS = worldUprightNormalWS.Normalized3();
	m_footUprightNormalMS = footUprightNormalMS.Normalized3();
	m_footUprightNormalRS = m_rootSpaceMSInverted.TransformVector( m_footUprightNormalMS );
	m_footUprightNormalMSMoreImportant = true;

	DEBUG_ANIM_VECTOR( m_footUprightNormalRS );
	DEBUG_ANIM_VECTOR( m_footUprightNormalMS );
}

void SBehaviorConstraintNodeFloorIKCommon::Reset( CBehaviorGraphInstance& instance )
{
	CAnimatedComponent const * animatedComponent = instance.GetAnimatedComponent();
	CMovingAgentComponent const * movingAgentComponent = Cast< CMovingAgentComponent >( animatedComponent );

	m_slidingOnSlopeWeight = movingAgentComponent && movingAgentComponent->GetAnimationProxy().GetSlidingOnSlopeIK()? 1.0f : 0.0f;	
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() )
	{
		m_slidingOnSlopeWeight = instance.GetInternalFloatValue( m_testVarSlidingOnSlopeID );
	}
#endif
	m_prevSlidingOnSlopeWeight = m_slidingOnSlopeWeight;

	m_prevAdjustOffsetByRaw = m_adjustOffsetByRaw = 0.0f;
	m_abruptAdjustemntWeight = 0.0f;
	m_adjustOffsetByVel = 0.0f;
	m_targetAdjustOffsetByVel = 0.0f;	
}

void SBehaviorConstraintNodeFloorIKCommon::OnActivated( CBehaviorGraphInstance& instance )
{
	m_prevLocalToWorld = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();
	m_localToWorld = m_prevLocalToWorld;

	DEBUG_ANIM_MATRIX( m_localToWorld );
	DEBUG_ANIM_MATRIX( m_prevLocalToWorld );

	m_adjustOffsetByVel = 0.0f;
	m_targetAdjustOffsetByVel = 0.0f;
	m_useExtendedIKOffset = false;

	m_currentUpWS = Vector::EZ;
	m_requestedUpWS = Vector::EZ;
	m_velocityUpWS = Vector::ZEROS;
	m_rootSpaceMS = Matrix::IDENTITY;
	m_rootSpaceMSInverted = Matrix::IDENTITY;
	m_HACK_resetFrameCounter = 0;
}

#ifdef DEBUG_FLOOR_IK
void SBehaviorConstraintNodeFloorIKCommon::SetupTestVars( CBehaviorGraphInstance& instance, const CName onOff, const CName slidingOnSlope )
{

	m_testVarID = onOff;
	m_testVarSlidingOnSlopeID = slidingOnSlope;

}
#endif

//////////////////////////////////////////////////////////////////////////

RED_INLINE static void AdjustFootOffset( SBehaviorConstraintNodeFloorIKLeg& leg, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float vbOffset )
{
	// adjust foot offset, first reference point, move it with vertical bone, so we have valid reference point
	{
		const Vector offsetBy = common.m_footUprightNormalRS * vbOffset;
		leg.m_usedFootOffsetRefRS += offsetBy;
		leg.m_usedFootOffsetRS -= offsetBy;
		leg.m_desiredFootOffsetLeftRS -= offsetBy;
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKVerticalBone::SBehaviorConstraintNodeFloorIKVerticalBone()
	: m_offset( 0.0f )
	, m_parentBone( INDEX_NONE )
	, m_bone( INDEX_NONE )
{
}

void SBehaviorConstraintNodeFloorIKVerticalBone::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
#ifdef DEBUG_FLOOR_IK
	frame->AddDebugText( m_debugVerticalBoneLocWS, String::Printf( TXT("p: %.3f -> %.3f"), m_offset, m_debugDesiredOffset ), 0, 0, true, Color(100,200,100), Color(0,0,0,128) );
	frame->AddDebugText( m_debugVerticalBoneLocWS, String::Printf( TXT("L: %.3f"), m_debugLeftAdditionalOffset ), 0, 2, true, Color(100,200,100), Color(0,0,0,128) );
	frame->AddDebugText( m_debugVerticalBoneLocWS, String::Printf( TXT("R: %.3f"), m_debugRightAdditionalOffset ), 0, 4, true, Color(100,200,100), Color(0,0,0,128) );
#endif
}

void SBehaviorConstraintNodeFloorIKVerticalBone::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data )
{
	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	m_bone = skeleton->FindBoneByName( data.m_bone );
	m_parentBone = m_bone != INDEX_NONE? skeleton->GetParentBoneIndex( m_bone ) : INDEX_NONE;
}

void SBehaviorConstraintNodeFloorIKVerticalBone::Reset()
{
	m_offset = 0.0f;
}

Float SBehaviorConstraintNodeFloorIKVerticalBone::GetVerticalAdjustBy( const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float timeDelta, Float rawCoef, Float velocityCoef ) const
{
	return ( common.m_adjustOffsetByRaw * rawCoef + common.m_adjustOffsetByVel * timeDelta * velocityCoef ) * common.m_footUprightNormalRS.Z;
}

void SBehaviorConstraintNodeFloorIKVerticalBone::Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leg, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common )
{
	if ( m_bone == INDEX_NONE )
	{
		return;
	}

	Float notSlidingOnSlopeWeight = 1.0f - common.m_slidingOnSlopeWeight;
	m_offset += GetVerticalAdjustBy( data, common, timeDelta ) * notSlidingOnSlopeWeight * ( 1.0f - common.m_additionalBlendCoefOnTeleport );

	Float newOffset = m_offset;

	Float desiredOffset = Vector::Dot3( common.m_footUprightNormalRS, leg.m_desiredFootOffsetLeftRS ) * data.m_stiffness;
	Float maximalOffset = desiredOffset;

	// blend, clamp to range
	// TODO blend time should depend on horizontal speed
	newOffset = Min( maximalOffset, BlendToWithBlendTimeLimit( newOffset, desiredOffset, data.m_offsetToDesiredBlendTime, Min( timeDelta, 0.03f ) + common.m_additionalBlendCoefOnTeleport, 0.8f + common.m_additionalBlendCoefOnTeleport ) );
	newOffset *= notSlidingOnSlopeWeight;

	m_offset = BlendToWithBlendTimeLimit( m_offset, newOffset, data.m_verticalOffsetBlendTime, Min( timeDelta, 0.03f ) + common.m_additionalBlendCoefOnTeleport, 0.5f + common.m_additionalBlendCoefOnTeleport ); // extra smoothing

	if( common.m_useExtendedIKOffset )
	{
		m_offset = Clamp( m_offset, -1.f, 1.f );
	}
	else
	{
		m_offset = Clamp( m_offset, data.m_offsetRange.X, data.m_offsetRange.Y );
	}


	// adjust final offset
	// limit offset (using m_offset as reference point)
	// final offset can't get lower than offset, as then legs may overstretch
	AdjustFootOffset( leg, data, common, m_offset );

#ifdef DEBUG_FLOOR_IK
	m_debugDesiredOffset = desiredOffset;
#endif
}

void SBehaviorConstraintNodeFloorIKVerticalBone::Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leftLeg, SBehaviorConstraintNodeFloorIKLeg& rightLeg, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common )
{
	if ( m_bone == INDEX_NONE )
	{
		return;
	}

	Float notSlidingOnSlopeWeight = 1.0f - common.m_slidingOnSlopeWeight;
	m_offset += GetVerticalAdjustBy( data, common, timeDelta, 1.0f, 0.25f ) * notSlidingOnSlopeWeight * ( 1.0f - common.m_additionalBlendCoefOnTeleport ); // !@# experimental for smoother movement

	Float newOffset = m_offset;

	Float desiredLeftOffset = Vector::Dot3( common.m_footUprightNormalRS, leftLeg.m_desiredFootOffsetLeftRS );
	Float desiredRightOffset = Vector::Dot3( common.m_footUprightNormalRS, rightLeg.m_desiredFootOffsetLeftRS );

	if ( !common.m_useFixedVersion )
	{
		Float reduce = Clamp ( 1.0f - ( common.m_speedXY - 0.2f ) / 0.6f, 0.4f, 1.0f );
		if ( reduce < 1.0f )
		{
			desiredLeftOffset *= reduce;
			desiredRightOffset *= reduce;
		}
	}

	const Float maximalOffset = Max( desiredLeftOffset, desiredRightOffset ) * data.m_stiffness;
	const Float desiredOffset = Min( desiredLeftOffset, desiredRightOffset ) * data.m_stiffness;

	// blend, clamp to range
	// TODO blend time should depend on horizontal speed
	//newOffset = Min( maximalOffset, BlendToWithBlendTimeLimit( newOffset, desiredOffset, data.m_offsetToDesiredBlendTime, Min( timeDelta, 0.03f ), 0.8f ) );

	newOffset = Min( maximalOffset, desiredOffset ); // !@# experimental for smoother movement
	Float blendTime = ( 1.0f + common.m_abruptAdjustemntWeight * 0.45f ) * data.m_verticalOffsetBlendTime; // to handle steps smoothly
	if ( !common.m_useFixedVersion )
	{
		blendTime *= ( Clamp( 1.0f - ( common.m_speedXY - 0.2f ) / 0.8f, 0.0f, 1.0f ) * 2.0f + 1.0f ); // !@# experimental for smoother movement
	}
	newOffset *= notSlidingOnSlopeWeight;

	m_offset = BlendToWithBlendTimeLimit( m_offset, newOffset, blendTime, Min( timeDelta, 0.03f ) + common.m_additionalBlendCoefOnTeleport, 0.8f + common.m_additionalBlendCoefOnTeleport ); // extra smoothing
	if( common.m_useExtendedIKOffset )
	{
		m_offset = Clamp( m_offset, -1.f, 1.f );
	}
	else
	{
		m_offset = Clamp( m_offset, data.m_offsetRange.X, data.m_offsetRange.Y );
	}
	

	// adjust final offset
	// limit offset (using m_offset as reference point)
	// final offset can't get lower than offset, as then legs may overstretch
	AdjustFootOffset( leftLeg, data, common, m_offset );
	AdjustFootOffset( rightLeg, data, common, m_offset );

#ifdef DEBUG_FLOOR_IK
	m_debugDesiredOffset = desiredOffset;
#endif
}

void SBehaviorConstraintNodeFloorIKVerticalBone::Update( Float timeDelta, SBehaviorConstraintNodeFloorIKLeg& leftFrontLeg, SBehaviorConstraintNodeFloorIKLeg& rightFrontLeg, SBehaviorConstraintNodeFloorIKLeg& leftBackLeg, SBehaviorConstraintNodeFloorIKLeg& rightBackLeg, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKLegsData& legsData, const SBehaviorConstraintNodeFloorIKCommon& common )
{
	if ( m_bone == INDEX_NONE )
	{
		return;
	}

	Float notSlidingOnSlopeWeight = 1.0f - common.m_slidingOnSlopeWeight;
	m_offset += GetVerticalAdjustBy( data, common, timeDelta ) * notSlidingOnSlopeWeight * ( 1.0f - common.m_additionalBlendCoefOnTeleport );

	Float newOffset = m_offset;

	Float desiredLeftFrontOffset = Vector::Dot3( common.m_footUprightNormalRS, leftFrontLeg.m_desiredFootOffsetLeftRS );
	Float desiredRightFrontOffset = Vector::Dot3( common.m_footUprightNormalRS, rightFrontLeg.m_desiredFootOffsetLeftRS );
	Float desiredLeftBackOffset = Vector::Dot3( common.m_footUprightNormalRS, leftBackLeg.m_desiredFootOffsetLeftRS );
	Float desiredRightBackOffset = Vector::Dot3( common.m_footUprightNormalRS, rightBackLeg.m_desiredFootOffsetLeftRS );

	const Float maximalOffset = Max( Max( desiredLeftFrontOffset, desiredRightFrontOffset ), Max( desiredLeftBackOffset, desiredRightBackOffset ) ) * data.m_stiffness;
	const Float desiredOffset = Min( ( desiredLeftFrontOffset + desiredRightFrontOffset ) * 0.5f, Min( desiredLeftBackOffset, desiredRightBackOffset ) ) * data.m_stiffness;

	// blend, clamp to range
	// TODO blend time should depend on horizontal speed
	newOffset = Min( maximalOffset, BlendToWithBlendTimeLimit( newOffset, desiredOffset, data.m_offsetToDesiredBlendTime, Min( timeDelta, 0.03f ) + common.m_additionalBlendCoefOnTeleport, 0.8f + common.m_additionalBlendCoefOnTeleport ) );
	newOffset *= notSlidingOnSlopeWeight;

	m_offset = BlendToWithBlendTimeLimit( m_offset, newOffset, data.m_verticalOffsetBlendTime, Min( timeDelta, 0.03f ) * (1.0f - common.m_additionalBlendCoefOnTeleport) + common.m_additionalBlendCoefOnTeleport * desiredOffset, 0.5f * (1.0f - common.m_additionalBlendCoefOnTeleport) + common.m_additionalBlendCoefOnTeleport * desiredOffset ); // extra smoothing
	
	if( common.m_useExtendedIKOffset )
	{
		m_offset = Clamp( m_offset, -1.f, 1.f );
	}
	else
	{
		m_offset = Clamp( m_offset, data.m_offsetRange.X, data.m_offsetRange.Y );
	}

	// adjust final offset
	// limit offset (using m_offset as reference point)
	// final offset can't get lower than offset, as then legs may overstretch
	AdjustFootOffset( leftFrontLeg, data, common, m_offset );
	AdjustFootOffset( rightFrontLeg, data, common, m_offset );
	AdjustFootOffset( leftBackLeg, data, common, m_offset );
	AdjustFootOffset( rightBackLeg, data, common, m_offset );

#ifdef DEBUG_FLOOR_IK
	m_debugDesiredOffset = desiredOffset;
#endif
}

void SBehaviorConstraintNodeFloorIKVerticalBone::UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorConstraintNodeFloorIKVerticalBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float weight, Bool storeAppliedOffsetInAnimationProxy )
{
	if ( m_bone == INDEX_NONE ||
		 m_parentBone == INDEX_NONE )
	{
		return;
	}

	AnimQsTransform parentBoneTMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_parentBone );
	AnimQsTransform pelvisTMS;
	SetMulTransform( pelvisTMS, parentBoneTMS, output.m_outputPose[ m_bone ] );

	AnimQsTransform outVerticalBoneTMS = pelvisTMS;
	AnimVector4 outVerticalBoneLoc = GetTranslation( outVerticalBoneTMS );
	m_offsetVerticallyMS = common.m_footUprightNormalMS * ( m_offset * weight );
	AddVector( outVerticalBoneLoc, VectorToAnimVector( m_offsetVerticallyMS ) );
	SetTranslation( outVerticalBoneTMS, outVerticalBoneLoc );

	SetMulInverseMulTransform( output.m_outputPose[ m_bone ], parentBoneTMS, outVerticalBoneTMS );

	if ( storeAppliedOffsetInAnimationProxy )
	{
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			mac->AccessAnimationProxy().SetAppliedIKOffsetWS( mac->AccessAnimationProxy().GetAppliedIKOffsetWS() + common.m_localToWorld.TransformVector( m_offsetVerticallyMS ) );
		}
	}

#ifdef DEBUG_FLOOR_IK
	m_debugVerticalBoneLocWS = common.m_localToWorld.TransformVectorAsPoint( AnimVectorToVector( GetTranslation( outVerticalBoneTMS ) ) );
#endif
}

void SBehaviorConstraintNodeFloorIKVerticalBone::StoreTraces( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& output, const SBehaviorConstraintNodeFloorIKCommon& common, SBehaviorConstraintNodeFloorIKDebugTrace& characterTrace, SBehaviorConstraintNodeFloorIKDebugTrace& characterOffsetTrace, SBehaviorConstraintNodeFloorIKDebugTrace& pelvisTrace )
{
	characterTrace.Store( common.m_localToWorld.GetTranslation() );
	characterOffsetTrace.Store( common.m_localToWorld.GetTranslation() + common.m_localToWorld.TransformVector( m_offsetVerticallyMS ) );
	if ( m_bone != INDEX_NONE )
	{
		pelvisTrace.Store( AnimVectorToVector( output.GetBoneWorldTransform( instance.GetAnimatedComponent(), m_bone ).GetTranslation() ) );
	}
}

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintNodeFloorIKMaintainLookBone::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKMaintainLookBoneData& data )
{
	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	m_bone = skeleton->FindBoneByName( data.m_bone );
	m_parent = m_bone != INDEX_NONE? skeleton->GetParentBoneIndex( m_bone ) : INDEX_NONE;
}

void SBehaviorConstraintNodeFloorIKMaintainLookBone::UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorConstraintNodeFloorIKMaintainLookBoneData& data, const SBehaviorConstraintNodeFloorIKCommon& common, Float weight ) const
{
	if ( m_bone != INDEX_NONE &&
		 m_parent != INDEX_NONE &&
		 data.m_amountOfRotation > 0.0f )
	{
#ifndef USE_HAVOK_ANIMATION
		AnimQsTransform parentTMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_parent );
		AnimQsTransform boneTMS;
		SetMulTransform( boneTMS, parentTMS, output.m_outputPose[ m_bone ] );

		RedQuaternion fullRotation;
		fullRotation.SetInverseMul( common.m_shortestRotationToCurrentMS, boneTMS.GetRotation() );
		RedQuaternion finalRotation;
		finalRotation.SetSlerp( boneTMS.GetRotation(), fullRotation, data.m_amountOfRotation * weight );

		AnimQsTransform boneRotatedTMS = boneTMS;
		boneRotatedTMS.SetRotation( finalRotation );

		SetMulInverseMulTransform( output.m_outputPose[ m_bone ], parentTMS, boneRotatedTMS );
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKLegs::SBehaviorConstraintNodeFloorIKLegs()
	: m_maxCosAngleOffUprightNormal( -1.0f )
	, m_maxSinAngleOffUprightNormal( 0.0f )
	, m_maxCosAngleOffUprightNormalToRevert( -1.0f )
{

}

void SBehaviorConstraintNodeFloorIKLegs::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintNodeFloorIKLegsData& data )
{
	m_maxCosAngleOffUprightNormal = cos( DEG2RAD( data.m_maxAngleOffUprightNormal ) );
	m_maxSinAngleOffUprightNormal = sin( DEG2RAD( data.m_maxAngleOffUprightNormal ) );
	m_maxCosAngleOffUprightNormalSide = cos( DEG2RAD( data.m_maxAngleOffUprightNormalSide ) );
	m_maxSinAngleOffUprightNormalSide = sin( DEG2RAD( data.m_maxAngleOffUprightNormalSide ) );
	m_maxCosAngleOffUprightNormalToRevert = cos( DEG2RAD( data.m_maxAngleOffUprightNormalToRevert ) );
}

//////////////////////////////////////////////////////////////////////////

RED_INLINE static Matrix ConstructRefFloorIKMatrix( const Vector& loc, const Vector& normal, const Vector& side )
{
	//RED_ASSERT( normal.IsNormalized3() );
	Matrix ret = Matrix::IDENTITY;
	Vector fwd = Vector::Cross( normal, side ).Normalized3();
	Vector _side = Vector::Cross( fwd, normal );//.Normalized3();
	ret.Set33( _side, fwd, normal );
	ret.SetTranslation( loc );
	return ret;
}

RED_INLINE static void TraceFootStep( CPhysicsWorld* physicsWorld, const Vector& refWS, const Vector& offsetNormalWS, Float startOffsetWS, Float endOffsetWS, Bool& hit, Vector& hitLocWS, Vector& normalWS, Bool& canCacheResult )
{
	PC_SCOPE( FloorIK_TraceFootStep );

	DEBUG_ANIM_VECTOR( refWS );
	DEBUG_ANIM_VECTOR( offsetNormalWS );
	DEBUG_ANIM_FLOAT( startOffsetWS );
	DEBUG_ANIM_FLOAT( endOffsetWS );

	static CPhysicsEngine::CollisionMask notCachableResultsIncludeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );
	static CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | notCachableResultsIncludeMask;
	static CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );

	Vector startWS = refWS;
	startWS += offsetNormalWS * startOffsetWS;
	Vector endWS = refWS;
	endWS += offsetNormalWS * endOffsetWS;
	SPhysicsContactInfo contactInfo;
	if ( physicsWorld && (physicsWorld->RayCastWithSingleResult( startWS, endWS, include, exclude, contactInfo ) == TRV_Hit) ) 
	{
		hit = true;
		hitLocWS = contactInfo.m_position;
		static Bool DO_CORRECTION = true; // alright, this is HACK, because collisions are offset and there was decision made to just pretend that we're lower
		if ( DO_CORRECTION )
		{
			hitLocWS -= contactInfo.m_normal * 0.015f;
		}
		normalWS = contactInfo.m_normal;
		canCacheResult = ! contactInfo.m_userDataA || ! ( contactInfo.m_userDataA->GetCollisionTypesBits() & notCachableResultsIncludeMask );
	}
	else
	{
		hit = false;
		hitLocWS = refWS;
		normalWS = Vector::EZ;
	}

	DEBUG_ANIM_MATRIX( hitLocWS );
	DEBUG_ANIM_VECTOR( normalWS );
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKCachedTrace::SBehaviorConstraintNodeFloorIKCachedTrace()
	: m_locWS( 0.0f, 0.0f, -1000.0f )
{
}

Bool SBehaviorConstraintNodeFloorIKCachedTrace::GetTraceResult( const Vector& locWS, Bool& outHit, Vector& outHitLocWS, Vector& outNormalWS, const SBehaviorConstraintNodeFloorIKLegsData& legs ) const 
{
	// return false; // !@# test
	// TODO return false for highest quality?
	if ( ( m_locWS - locWS ).SquareMag2() < legs.m_maxDistanceForTraceUpdate * legs.m_maxDistanceForTraceUpdate &&
		Abs( m_locWS.Z - locWS.Z ) < 0.5f ) // to avoid jumping between box/table and floor
	{
		outHit = m_hit;
		const Vector diff = locWS - m_locWS;
		outHitLocWS = m_hitLocWS;
		outHitLocWS.X += diff.X;
		outHitLocWS.Y += diff.Y;
		outHitLocWS.Z += diff.X * m_adjustHitLocZ.X + diff.Y * m_adjustHitLocZ.Y;
		outNormalWS = m_normalWS; 
		return true;
	}
	return false;
}

void SBehaviorConstraintNodeFloorIKCachedTrace::Update( const Vector& locWS, Bool hit, const Vector& hitLocWS, const Vector& normalWS ) 
{
	m_locWS = locWS;
	m_hit = hit;
	m_hitLocWS = hitLocWS;
	m_normalWS = normalWS;
	ASSERT( m_normalWS.Z != 0.0f ); // shouldn't be in XY plane, this may happen when horse is casting a ray and it hits wall
	if ( m_normalWS.Z != 0.0f )
	{
		// correct using normal
		m_adjustHitLocZ.X = -m_normalWS.X * ( 1.0f / m_normalWS.Z );
		m_adjustHitLocZ.Y = -m_normalWS.Y * ( 1.0f / m_normalWS.Z );
	}
	else
	{
		m_adjustHitLocZ.X = 0.0f;
		m_adjustHitLocZ.Y = 0.0f;
		m_adjustHitLocZ.Z = 1.0f;
	}
}

void SBehaviorConstraintNodeFloorIKCachedTrace::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_hit )
	{
		frame->AddDebugLineWithArrow( m_locWS, m_hitLocWS, 1.0f, 0.1f, 0.1f, Color( 50, 255, 200 ), true );
		frame->AddDebugLineWithArrow( m_hitLocWS, m_hitLocWS + m_normalWS * 0.3f, 1.0f, 0.1f, 0.1f, Color( 50, 200, 255 ), true );
		frame->AddDebugSphere( m_hitLocWS, 0.05f, Matrix::IDENTITY, Color( 50, 200, 255 ), true );
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKLeg::SBehaviorConstraintNodeFloorIKLeg()
	: m_minAltitudeAboveGround( 0.0f )
{
}

void SBehaviorConstraintNodeFloorIKLeg::Setup( CBehaviorGraphInstance& instance, STwoBonesIKSolver& ikSolver )
{
	AnimQsTransform lowerTMS;
	ikSolver.GetRefLowerTMS( instance, lowerTMS );
	m_minAltitudeAboveGround = AnimVectorToVector( GetTranslation( lowerTMS ) ).Z;
}

#ifdef DEBUG_FLOOR_IK
void SBehaviorConstraintNodeFloorIKLeg::SetupTestVars( CBehaviorGraphInstance& instance, const CName onOff, const CName loc, const CName normal, const CName testPlane, const CName planeNormal )
{
	m_testVarID = onOff;
	m_testVarLocID = loc;
	m_testVarNormalID = normal;
	m_testVarUsePlaneID = testPlane;
	m_testVarPlaneNormalID = planeNormal;
}
#endif

void SBehaviorConstraintNodeFloorIKLeg::Reset( const SBehaviorConstraintNodeFloorIKCommon& common )
{
	m_footOffsetRS = Vector::ZEROS;
	m_hitLocWS = Vector::ZEROS;
	m_hitLocRS = Vector::ZEROS;
	m_finalNormalRS = common.m_footUprightNormalRS;
	m_finalNotClampedNormalRS = common.m_footUprightNormalRS;
}

RED_INLINE static void FixZOnSlope_Old( Vector& loc, Float slopeAngleRad )
{
	loc.Z = loc.Y * sin( slopeAngleRad );
}

RED_INLINE static void FixZOnSlope_New( Vector& loc, Float slopeAngleRad )
{
	loc.Z = loc.Y * MTan( slopeAngleRad );
}

RED_INLINE static void ApplyFeetDistanceAdjustment_Old( Vector& toVector, Vector const & dir, Float coef, Float offset, Float slopeAngleRad )
{
	Vector toVectorBase = toVector;
	FixZOnSlope_Old( toVectorBase, slopeAngleRad );
	Vector toVectorOffset = toVector - toVectorBase;
	Float alongDir = Vector::Dot2( toVectorBase, dir );
	toVectorBase = toVectorBase + dir * ( alongDir * ( coef - 1.0f ) + offset ); // move along dir backwards to zero it and then by coef and move forward by offset
	FixZOnSlope_Old( toVectorBase, slopeAngleRad );
	toVector = toVectorOffset + toVectorBase;
}

RED_INLINE static void ApplyFeetDistanceAdjustment_New( Vector& toVector, Vector const & dir, Float coef, Float offset, Float slopeAngleRad )
{
	Vector toVectorBase = toVector;
	FixZOnSlope_New( toVectorBase, slopeAngleRad );
	Vector toVectorOffset = toVector - toVectorBase;
	Float alongDir = Vector::Dot2( toVectorBase, dir );
	toVectorBase = toVectorBase + dir * ( alongDir * ( coef - 1.0f ) + offset ); // move along dir backwards to zero it and then by coef and move forward by offset
	FixZOnSlope_New( toVectorBase, slopeAngleRad );
	toVector = toVectorOffset + toVectorBase;
}

RED_INLINE static Vector ApplyJumpingOffset( Vector const & loc, Float jumpingOffset )
{
	return loc - Vector(0.0f, 0.0f, jumpingOffset);
}

RED_INLINE static void SetNormalOfSlope( Vector& normal, Float slopeAngleRad )
{
	normal.X = 0.0f;
	normal.Y = -sin( slopeAngleRad );
	normal.Z = cos( slopeAngleRad );
}

RED_INLINE static Float GetAngleDegFromNormal( const Vector& normal )
{
	const Float angleRad = M_PI_HALF - MAsin_safe( normal.Normalized3().Z );
	return RAD2DEG( angleRad );
}

static RED_INLINE Float SquareZ( Float v )
{
	return v * v;
}

#ifdef DEBUG_FLOOR_IK_POSES

static void CachePose( const CAnimatedComponent* ac, const SBehaviorGraphOutput& output, const Matrix& l2w, TDynArray< Matrix >& outPose )
{
	outPose.Resize( output.m_numBones );
	output.GetBonesModelSpace( ac, outPose );
	
	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		Matrix& m = outPose[ i ];
		m = m * l2w;
	}
}

#endif

void SBehaviorConstraintNodeFloorIKLeg::Update( Float timeDelta, SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const STwoBonesIKSolverData& ikSolverData, STwoBonesIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKLegs& legs, const SBehaviorConstraintNodeFloorIKLegsData& data, const SBehaviorConstraintNodeFloorIKCommon& common )
{
	DEBUG_ANIM_POSES( output );

	AnimQsTransform footTMS;
	AnimQsTransform upperTMS;
	ikSolver.GetUpperAndLowerIKTMS( instance, output, upperTMS, footTMS );

	Matrix footMatMS = AnimQsTransformToMatrix( footTMS );

	DEBUG_ANIM_MATRIX( common.m_rootSpaceMSInverted );
	DEBUG_ANIM_MATRIX( footMatMS );

	m_footOffsetRS += common.m_footUprightNormalRS * common.m_adjustOffsetByRaw; // just raw, no prediction

	// get current transform of leg
	Matrix upperMatRS = AnimQsTransformToMatrix( upperTMS ) * common.m_rootSpaceMSInverted;
	m_startMatRS = footMatMS * common.m_rootSpaceMSInverted;
	m_startSideRS = m_startMatRS.TransformVector( ikSolverData.m_sideDirLowerBS ); // XYZ

	DEBUG_ANIM_MATRIX( m_startMatRS );

	// if we're laying on ground, side dir may point up or down messing up our reference frame
	// use side based on forward and normal then
	Float useOriginalSideRS = 1.0f - common.m_slidingOnSlopeWeight * Clamp( ( Abs( m_startSideRS.Z ) - 0.5f ) * 2.0f, 0.0f, 1.0f );
	if ( useOriginalSideRS < 1.0f )
	{
		Vector startFwdRS = m_startMatRS.TransformVector( ikSolverData.m_bendDirLowerBS ); // XYZ
		Vector startSideRS = Vector::Cross( startFwdRS, common.m_footUprightNormalRS ).Normalized3();
		m_startSideRS = m_startSideRS * useOriginalSideRS + startSideRS * ( 1.0f - useOriginalSideRS );
	}

	Float const slopeAngleFromAnimRad = DEG2RAD( common.m_slopeFromAnim );

	// get location in RS of current location of foot
	m_startLocRS = m_startMatRS.GetTranslation();
	m_usedFootOffsetRefRS = m_startLocRS;
	// apply feet distance adjustment
	if ( common.m_useFixedVersion )
	{
		ApplyFeetDistanceAdjustment_New( m_usedFootOffsetRefRS, common.m_feetDistAlongDirMS, common.m_feetDistCoef, common.m_feetDistOffset, slopeAngleFromAnimRad );
	}
	else
	{
		ApplyFeetDistanceAdjustment_Old( m_usedFootOffsetRefRS, common.m_feetDistAlongDirMS, common.m_feetDistCoef, common.m_feetDistOffset, slopeAngleFromAnimRad );
	}
	
	// create matrix to maintaining upright normal for leg
	Matrix maintainUprightNormalRS = Matrix::IDENTITY;
	if ( common.m_footUprightNormalRS != Vector::EZ )
	{
#ifndef USE_HAVOK_ANIMATION
		RedVector4 currentUpNormalRS = reinterpret_cast< const RedVector4& >( common.m_footUprightNormalRS );
		RedQuaternion shortestRot;
		shortestRot.SetShortestRotation( RedVector4::EZ, currentUpNormalRS );
		RedMatrix4x4 rotateToCurrentRedMat = BuildFromQuaternion( shortestRot.Quat );
		maintainUprightNormalRS = reinterpret_cast< const Matrix& >( rotateToCurrentRedMat );

		Matrix moveToUpperRS = Matrix::IDENTITY;
		moveToUpperRS.SetTranslation( -upperMatRS.GetTranslation() );

		Matrix moveByUpperRS = Matrix::IDENTITY;
		moveByUpperRS.SetTranslation( upperMatRS.GetTranslation() );

		maintainUprightNormalRS = moveToUpperRS * maintainUprightNormalRS * moveByUpperRS;
#endif
	}

	// get location in RS of point where hit check should start
	m_inputStartLocRS = m_startLocRS;
	if ( common.m_useFixedVersion )
	{
		FixZOnSlope_New( m_inputStartLocRS, slopeAngleFromAnimRad ); // for start loc we don't want to apply jumping offset
		SetNormalOfSlope( m_inputStartNormalRS, slopeAngleFromAnimRad );
	}
	else
	{
		FixZOnSlope_Old( m_inputStartLocRS, slopeAngleFromAnimRad ); // for start loc we don't want to apply jumping offset
		SetNormalOfSlope( m_inputStartNormalRS, slopeAngleFromAnimRad * 0.5f ); // that's the worst kind of comment/coding: I have no idea why it works much better like that - but it works :|
	}
	SetNormalOfSlope( m_inputStartNormalRefForClampingRS, slopeAngleFromAnimRad );

	{	// how high above ground is leg TODO use hand picked values?
		Float aboveGroundTemp = ( ( m_startLocRS.Z - m_inputStartLocRS.Z ) - m_minAltitudeAboveGround * 1.4f ) / ( m_minAltitudeAboveGround * 2.5f );
		m_aboveGround = Clamp( aboveGroundTemp, 0.0f, 1.0f );
	}

	Vector usedFootOffsetRefRSFortHitZ = m_usedFootOffsetRefRS;
	if ( common.m_useFixedVersion )
	{
		FixZOnSlope_New( usedFootOffsetRefRSFortHitZ, slopeAngleFromAnimRad );
	}
	else
	{
		FixZOnSlope_Old( usedFootOffsetRefRSFortHitZ, slopeAngleFromAnimRad );
	}

	// maintain upright
	m_startLocUprightMaintainedRS = maintainUprightNormalRS.TransformPoint( m_startLocRS );
	Vector footStartHitMaintainedRS = maintainUprightNormalRS.TransformPoint( usedFootOffsetRefRSFortHitZ );
	
	// drop onto floor - get location of foot on floor
	CWorld* world = instance.GetAnimatedComponent()->GetWorld();

	CPhysicsWorld* physicsWorld = nullptr;
	world->GetPhysicsWorld( physicsWorld );
	Bool footHit = false;
	// hit location calculations in world space are done with applied jumping offset
	const Vector plainFootHitRefWS = common.m_rootSpaceWS.TransformPoint( footStartHitMaintainedRS );
	const Vector footHitRefWS = ApplyJumpingOffset( plainFootHitRefWS, common.m_jumpingOffset );

	const Bool wasTeleported = (footHitRefWS - m_hitLocWS).SquareMag3() > 10.0f * 10.0f;
	if ( wasTeleported )
	{
		//LOG_ENGINE( TXT( "leg teleported: %s %s" ), ToString( m_hitLocWS ).AsChar(), ToString( footHitRefWS ).AsChar() );
		m_hitLocWS = footHitRefWS;
	}

	Vector prevHitLocWS = m_hitLocWS;
	m_hitLocWS = footHitRefWS;
	Vector footNormalWS = common.m_worldUprightNormalWS;
	// used cache value if possible
	Vector newHitLocWS = m_hitLocWS;

	DEBUG_ANIM_VECTOR( common.m_footUprightNormalMS );
	DEBUG_ANIM_VECTOR( common.m_footUprightNormalRS );

	if ( !m_cachedTrace.GetTraceResult( footHitRefWS, footHit, newHitLocWS, footNormalWS, data ) )
	{
		Bool cacheResult = true;
		// TODO right now it just extends range a little bit, maybe it will be enough
		Vector normalWS = common.m_localToWorld.TransformVector( common.m_footUprightNormalMS );

		// do raycast at foot pos
		TraceFootStep( physicsWorld, footHitRefWS, normalWS,
			data.m_traceOffsetRange.Y * 1.3f + Max( 0.0f, common.m_adjustOffsetByVel * 0.3f ),
			data.m_traceOffsetRange.X * 1.5f + Min( 0.0f, common.m_adjustOffsetByVel * 0.3f ), footHit, newHitLocWS, footNormalWS, cacheResult );

		if ( Vector::Dot3( footNormalWS, normalWS ) < 0.3f )
		{
			// if almost perpendicular - ignore hit
			footHit = false;
		}
		if ( cacheResult )
		{
			m_cachedTrace.Update( footHitRefWS, footHit, newHitLocWS, footNormalWS );
		}
		else
		{
			m_cachedTrace.Invalidate();
		}
	}

	// test code to replace drop
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( m_testVarID ) > 0.0f )
	{
		footHit = true;
		Vector testFoot = instance.GetInternalVectorValue( m_testVarLocID );
		testFoot = common.m_localToWorldInverted.TransformPoint( testFoot );
		Vector footUprightWS = common.m_rootSpaceWS.TransformVector( common.m_footUprightNormalRS );
		newHitLocWS += footUprightWS * Vector::Dot3( footUprightWS, testFoot - newHitLocWS ); // do it along normal
		newHitLocWS.Z *= instance.GetInternalFloatValue( m_testVarID );
		Vector testFootNormal = instance.GetInternalVectorValue( m_testVarNormalID ).Normalized3();
		testFootNormal = common.m_localToWorldInverted.TransformVector( testFootNormal );
		footNormalWS = testFootNormal;
		footNormalWS.X *= instance.GetInternalFloatValue( m_testVarID );
		footNormalWS.Y *= instance.GetInternalFloatValue( m_testVarID );
		Float usePlane = instance.GetInternalFloatValue( m_testVarUsePlaneID );
		if ( usePlane > 0.0f )
		{
			Vector planeNormal = instance.GetInternalVectorValue( m_testVarPlaneNormalID );
			if (planeNormal.Mag3() < 0.01)
			{
				planeNormal = Vector::EZ;
			}
			planeNormal.Normalize3();
			Float abovePlane = Vector::Dot3( planeNormal, footHitRefWS );
			Float uprightToPlane = Vector::Dot3( planeNormal, footUprightWS );
			Vector fromPlaneHitLocWS = footHitRefWS - footUprightWS * ( uprightToPlane != 0.0f? abovePlane / uprightToPlane : 0.0f );
			Vector newNormalWS = planeNormal;
			newHitLocWS = newHitLocWS * ( 1.0f - usePlane ) + fromPlaneHitLocWS * usePlane;

			if ( common.m_useFixedVersion )
			{
				FixZOnSlope_New( newHitLocWS, slopeAngleFromAnimRad );
			}

			footNormalWS = footNormalWS * ( 1.0f - usePlane ) + newNormalWS * usePlane;
			footNormalWS.Normalize3();
		}
	}
#endif

	// get new hit location
	// check if there is discontinuity along foot upright normal (this happens when displacement along normal is greater than displacement perpendicular to normal)
	// new hit location should be in place that goes through new hit location along that normal and offset by blended value along that normal
	//	newHitLoc = detectedHitLoc/footRefLoc + footUprightNormal * blendedDisplacement
	//		where blendedDisplacement is value blended from (detectedHitLoc/footRefLoc - previousHitLoc) along normal
	// or just requested
	//	newHitLoc = detectedHitLoc/footRefLoc
	if ( common.m_jumping || common.m_HACK_resetFrameCounter > 0 )
	{
		// jump - during jump try to behave like you would hit foot ref location from previous location in root space (because we're jumping!)
		newHitLocWS = footHitRefWS;
		prevHitLocWS = common.m_rootSpaceWS.TransformPoint( m_hitLocRS );
		if ( m_hitLocRS.SquareMag3() < 0.01f || common.m_additionalBlendCoefOnTeleport > 0.0f )
		{
			prevHitLocWS = newHitLocWS;
		}
		Vector alongDirWS = common.m_localToWorld.TransformVector( common.m_footUprightNormalMS );
		Float alongDisplacement = Vector::Dot3( newHitLocWS - prevHitLocWS, alongDirWS );
		alongDisplacement = BlendOnOffWithSpeedBasedOnTime( alongDisplacement, 0.0f, 0.1f, timeDelta );
		m_hitLocWS = newHitLocWS - alongDirWS * alongDisplacement;
	}
	else
	{
		if ( prevHitLocWS.SquareMag3() < 0.01f || common.m_additionalBlendCoefOnTeleport > 0.0f )
		{
			// in case we just reset
			prevHitLocWS = newHitLocWS;
		}
		if (footHit)
		{
			Vector alongDirWS = common.m_localToWorld.TransformVector( common.m_footUprightNormalMS );
			Float alongDisplacement = Vector::Dot3( newHitLocWS - prevHitLocWS, alongDirWS );
			Float alongDisplacementAbs = Abs( alongDisplacement );
			Float perpendicularDisplacement = ( ( newHitLocWS - alongDirWS * alongDisplacement ) - prevHitLocWS ).Mag3();
			Float discontinuity = perpendicularDisplacement != 0.0f ? alongDisplacementAbs / perpendicularDisplacement : ( alongDisplacementAbs < 0.1f? 0.0f : 1.0f );
			if ( discontinuity >= 1.0f )
			{
				Float discontinuityBlendTime = Min( 0.2f, 0.05f + discontinuity * 0.2f );
				if ( Abs( footHitRefWS.Z - newHitLocWS.Z ) < 0.1f )
				{
					//discontinuityBlendTime = Min( 0.1f, discontinuityBlendTime );
				}
				// when requested movement speed is very different to actual speed
				if ( const CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
				{
					if ( mac->GetVelocityBasedOnRequestedMovement().SquareMag2() > common.m_actualVelocityWS.SquareMag2() * 1.5f * 1.5f )
					{
						discontinuityBlendTime = 0.6f;
					}
				}
				// move along dir WS
				alongDisplacement = BlendOnOffWithSpeedBasedOnTime( alongDisplacement, 0.0f, discontinuityBlendTime, timeDelta );
				m_hitLocWS = newHitLocWS - alongDirWS * alongDisplacement;
			}
			else
			{
				m_hitLocWS = newHitLocWS;
			}
		}
		else
		{
			CAnimatedComponent const * animatedComponent = instance.GetAnimatedComponent();
			CMovingAgentComponent const * movingAgentComponent = Cast< CMovingAgentComponent >( animatedComponent );

			if ( movingAgentComponent->GetEntity()->HasTag( CNAME( horse ) ) )
			{
				newHitLocWS = prevHitLocWS;
				m_hitLocWS = newHitLocWS;
			}
			else
			{
				// blend back to foot ref location as it would be in model space - ignoring root rotation!
				const Vector plainFootHitRefIgnoringRootWS = common.m_localToWorld.TransformPoint( footStartHitMaintainedRS );
				newHitLocWS = ApplyJumpingOffset( plainFootHitRefIgnoringRootWS, common.m_jumpingOffset );
				Vector alongDirWS = common.m_localToWorld.TransformVector( common.m_footUprightNormalMS );
				Float alongDisplacement = Vector::Dot3( newHitLocWS - prevHitLocWS, alongDirWS );
				alongDisplacement = BlendOnOffWithSpeedBasedOnTime( alongDisplacement, 0.0f, 0.6f, timeDelta ); // slow blend
				m_hitLocWS = newHitLocWS - alongDirWS * alongDisplacement;
			}
		}
	}

#ifdef DEBUG_FLOOR_IK
	const Float footNormalDeg = GetAngleDegFromNormal( footNormalWS );
	const Float slopeAngleDiff = EulerAngles::AngleDistance( footNormalDeg, common.m_slopeFromAnim );
	m_debugAngleAnim = common.m_slopeFromAnim;
	m_debugAngleSlope = footNormalDeg;
#endif

	m_hitLocRS = common.m_rootSpaceWSInverted.TransformPoint( m_hitLocWS );

	Vector footNormalMS = common.m_localToWorldInverted.TransformVector( footNormalWS );

	// keep normals sane
	// make sure normal is in some fine range
	// and keep up-right if off ground
	if ( footNormalMS.Mag3() < 0.1f )
	{
		footNormalMS = Vector( 0.0f, 0.0f, 1.0f );
	}

	Vector rawFootNormalMS = footNormalMS;

	Vector footUprightNormalRefMS = data.m_useInputNormalForNormalClamping? m_inputStartNormalRefForClampingRS : common.m_footUprightNormalMS;

	Float footNormalToUpright = Vector::Dot3( footNormalMS, footUprightNormalRefMS );
	// check and correct normal
	Vector footNormalOriginalMS = footNormalMS;

	if ( footNormalToUpright < legs.m_maxCosAngleOffUprightNormalToRevert )
	{
		// in case we hit something strange - wall etc. TODO handle it in better way?
		footNormalMS = Vector::EZ;
		rawFootNormalMS = footNormalMS;
		m_hitLocWS = footHitRefWS;
	}
	else
	{
		if ( footNormalToUpright < legs.m_maxCosAngleOffUprightNormal )
		{
			// if normal is too steep, make normal more acceptable and move feet up
			Vector perpendicularComponentNormalized = ( footNormalMS - footUprightNormalRefMS * footNormalToUpright ).Normalized3();
			footNormalMS = footUprightNormalRefMS * legs.m_maxCosAngleOffUprightNormal + perpendicularComponentNormalized * legs.m_maxSinAngleOffUprightNormal;
			ASSERT( footNormalMS.IsNormalized3() );
		}
		if ( legs.m_maxCosAngleOffUprightNormalSide > legs.m_maxCosAngleOffUprightNormal )
		{
			// as above but just against side
			Vector footForwardMS = Vector::Cross( footUprightNormalRefMS, footMatMS.TransformVector( ikSolverData.m_sideDirLowerBS) ).Normalized3();
			Vector alongForward = footForwardMS * Vector::Dot3( footForwardMS, footNormalMS );
			Vector footNormalMSSideOnly = footNormalMS - alongForward;
			Float footNormalMSSideOnlyLength = footNormalMSSideOnly.Mag3();
			Float footNormalSideOnlyToUpright = Vector::Dot3( footNormalMSSideOnly, footUprightNormalRefMS );
			if ( footNormalSideOnlyToUpright < legs.m_maxCosAngleOffUprightNormalSide * footNormalMSSideOnlyLength )
			{
				Vector perpendicularComponentNormalized = ( footNormalMSSideOnly - footUprightNormalRefMS * footNormalSideOnlyToUpright ).Normalized3();
				footNormalMS = alongForward + ( footUprightNormalRefMS * legs.m_maxCosAngleOffUprightNormalSide + perpendicularComponentNormalized * legs.m_maxSinAngleOffUprightNormalSide ) * footNormalMSSideOnlyLength;
				footNormalMS.Normalize3();
			}
		}
	}

	if ( !common.m_useFixedVersion )
	{
		// when leg is up, it should not be parallel to ground but more in line with body
		footNormalMS += common.m_footUprightNormalMS * m_aboveGround * 0.7f;
	}

	// when sliding on slope, don't modify normal - leave it as it was
	footNormalMS = footNormalOriginalMS * common.m_slidingOnSlopeWeight + footNormalMS * ( 1.0f - common.m_slidingOnSlopeWeight );

	// normalize after recent operations
	footNormalMS.Normalize3();

	// calculate normals in model space
	Vector newFinalNormalRS = common.m_rootSpaceMSInverted.TransformVector( footNormalMS );
	Vector newFinalNotClampedNormalRS = common.m_rootSpaceMSInverted.TransformVector( rawFootNormalMS );

	// and blend
	m_finalNormalRS = BlendToWithBlendTime( m_finalNormalRS, newFinalNormalRS, 0.06f, timeDelta + common.m_additionalBlendCoefOnTeleport );
	m_finalNormalRS.Normalize3();
	m_finalNotClampedNormalRS = BlendToWithBlendTime( m_finalNotClampedNormalRS, newFinalNotClampedNormalRS, 0.06f, timeDelta + common.m_additionalBlendCoefOnTeleport );
	m_finalNotClampedNormalRS.Normalize3();
	
	// calculate final location of each leg
	const Vector footHitLocJORS = common.m_rootSpaceWSInverted.TransformPoint( ApplyJumpingOffset( m_hitLocWS, -common.m_jumpingOffset ) );
	m_usedFootOffsetRefRS = footStartHitMaintainedRS; // maintained
	Vector desiredFootOffsetRS = footHitLocJORS - m_usedFootOffsetRefRS;
#ifdef DEBUG_FLOOR_IK
	m_debugStartFinalOffsetRefBaseRS = m_inputStartLocRS;
	m_debugStartFinalOffsetRefRS = m_usedFootOffsetRefRS;
	m_debugNewBaseVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS );
#endif
	Float minVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, footHitLocJORS ) - 0.7f * ( Vector::Dot3( common.m_footUprightNormalRS, m_startLocUprightMaintainedRS ) - m_minAltitudeAboveGround ); // 0.7 to allow when toes are pointing toward ground
#ifdef DEBUG_FLOOR_IK
	m_debugHitMinVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS ) < minVerticalOffset;
#endif
	/* !@# TEST
	{	// limit desired final offset to minimal value
		Float currentFinalOffset = Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS );
		Float newFinalOffset = Max( currentFinalOffset, minVerticalOffset );
		desiredFootOffsetRS += common.m_footUprightNormalRS * ( newFinalOffset - currentFinalOffset );
	}
	*/
	m_desiredFootOffsetLeftRS = desiredFootOffsetRS * m_inputStartNormalRefForClampingRS.Z; // because that (being on slope) partially should cover offset

	// blend to desired and don't limit final offset yet
	Float verticalOffsetBlendCoef = Vector::Dot3( common.m_footUprightNormalRS, m_footOffsetRS ) < Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS )? data.m_verticalOffsetBlendUpTime : data.m_verticalOffsetBlendDownTime;
	verticalOffsetBlendCoef *= Vector::Dot3( common.m_footUprightNormalRS, m_usedFootOffsetRS ) < minVerticalOffset? 0.5f : 0.8f;// speed up blending to avoid getting below floor
	m_footOffsetRS = BlendToWithBlendTime( m_footOffsetRS, desiredFootOffsetRS, verticalOffsetBlendCoef, timeDelta + common.m_additionalBlendCoefOnTeleport );
	m_usedFootOffsetRS = m_footOffsetRS;
#ifdef DEBUG_FLOOR_IK
	/*
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( m_testVarID ) > 0.0f )
	{
		m_footOffsetRS = footHitLocJORS - m_finalOffsetRefRS;
	}
	*/
#endif

#ifdef DEBUG_FLOOR_IK
	m_debugUsedFootOffsetToStartRS = m_footOffsetRS;
	m_debugNewFinalAdjustedVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS );
	m_debugFinalVerticalOffsetPrePelvis = Vector::Dot3( common.m_footUprightNormalRS, m_usedFootOffsetRS );
	m_debugNewFinalVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, desiredFootOffsetRS );
	m_debugNewFinalVerticalOffsetLeft = Vector::Dot3( common.m_footUprightNormalRS, m_desiredFootOffsetLeftRS );
	m_debugMinVerticalOffset = minVerticalOffset;
	m_debugMinDefaultVerticalOffset = Vector::Dot3( common.m_footUprightNormalRS, m_startLocRS - m_usedFootOffsetRefRS );
	m_debugHit = footHit;
	m_debugHitLocWS = m_hitLocWS;
	m_debugRawNormalWS = footNormalWS;
	m_debugNormalWS = common.m_localToWorld.TransformVector( m_finalNormalRS );
	m_debugAboveGround = m_aboveGround;
	m_debugFinalBaseWS = common.m_rootSpaceWS.TransformVectorAsPoint( m_desiredFootOffsetLeftRS );
	m_debugUsedFootOffsetRefWS = common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRefRS );
	m_debugLocalToWorldPrev = m_debugLocalToWorldCurr;
	m_debugLocalToWorldCurr = common.m_localToWorld;
#endif
}

void SBehaviorConstraintNodeFloorIKLeg::SetupIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, STwoBonesIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKLegsData& data, const SBehaviorConstraintNodeFloorIKCommon& common ) const
{
	DEBUG_ANIM_POSES( output );

	// calculate final final loc

	Vector finalOffsetRS = m_usedFootOffsetRS;
	{	// limit final offset
		Float currentFinalOffset = Vector::Dot3( common.m_footUprightNormalRS, finalOffsetRS );
		Float newFinalOffset = currentFinalOffset * common.m_slidingOnSlopeWeight + Clamp( currentFinalOffset, data.m_offsetRange.X, data.m_offsetRange.Y ) * ( 1.0f - common.m_slidingOnSlopeWeight ); // don't clamp for slope - although it could be better to use separate range
		finalOffsetRS += common.m_footUprightNormalRS * ( newFinalOffset - currentFinalOffset );
	}

	const Vector finalLocRS = finalOffsetRS + m_usedFootOffsetRefRS; // maintain offset ("loc or hit"?)

	// calculate matrices as final point
	const Matrix footFinalMatRS = ConstructRefFloorIKMatrix( finalLocRS, m_finalNormalRS, m_startSideRS );

	// get into model space
	const Matrix footFinalMatMS = footFinalMatRS * common.m_rootSpaceMS;

	// calculate matrices as reference at start point
	// use root space but pretend we're in original model space
	const Matrix footStartHitNotMaintainedRS = ConstructRefFloorIKMatrix( m_inputStartLocRS, m_inputStartNormalRS, m_startSideRS );

	Matrix footIKMatMS( Matrix::IDENTITY );

	// transform foot matrices
	if ( common.m_useFixedVersion )
	{
		const Matrix footStartHitNotMaintainedRSInv = common.m_useFullInverted ? footStartHitNotMaintainedRS.FullInverted() : footStartHitNotMaintainedRS.Inverted();
		footIKMatMS = m_startMatRS * footStartHitNotMaintainedRSInv * footFinalMatMS;

		const Vector a = m_startMatRS.GetTranslation();
		const Vector b = footStartHitNotMaintainedRS.GetTranslation();
		const Vector c = footFinalMatMS.GetTranslation();
		const Vector t = a - b + c;
		footIKMatMS.SetTranslation( t );
	}
	else
	{
		footIKMatMS = m_startMatRS * ( common.m_useFullInverted? footStartHitNotMaintainedRS.FullInverted() : footStartHitNotMaintainedRS.Inverted() ) * footFinalMatMS;
	}

	// move legs
	DEBUG_ANIM_TRANSFORM( MatrixToAnimQsTransform( footIKMatMS ) );
	ikSolver.SetTargetLowerTMS( MatrixToAnimQsTransform( footIKMatMS ) );

#ifdef DEBUG_FLOOR_IK
	m_debugStartMaintainedMatWS = ConstructRefFloorIKMatrix( m_startLocUprightMaintainedRS, common.m_footUprightNormalRS, m_startSideRS ) * common.m_rootSpaceWS;
	m_debugFinalOffsetRefMatWS = ConstructRefFloorIKMatrix( m_usedFootOffsetRefRS, common.m_footUprightNormalRS, m_startSideRS ) * common.m_rootSpaceWS;
	m_debugStartMatWS = footStartHitNotMaintainedRS * common.m_rootSpaceWS;
	m_debugFinalMatWS = footFinalMatMS * common.m_localToWorld;
	m_debugIKMatWS = footIKMatMS * common.m_localToWorld;
	m_debugOffset = finalOffsetRS;
#endif
}

void SBehaviorConstraintNodeFloorIKLeg::StoreTraces( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& output, const SBehaviorConstraintNodeFloorIKCommon& common, const STwoBonesIKSolver& ikSolver, SBehaviorConstraintNodeFloorIKDebugTrace& legTrace, SBehaviorConstraintNodeFloorIKDebugTrace& legBaseTrace, SBehaviorConstraintNodeFloorIKDebugTrace& legHitTrace )
{
	if ( ikSolver.m_lowerBone != INDEX_NONE )
	{
		legTrace.Store( AnimVectorToVector( output.GetBoneWorldTransform( instance.GetAnimatedComponent(), ikSolver.m_lowerBone ).GetTranslation() ) );
	}
#ifdef DEBUG_FLOOR_IK
	legBaseTrace.Store( m_debugFinalBaseWS );
#endif
	legHitTrace.Store( m_hitLocWS );
}

void SBehaviorConstraintNodeFloorIKLeg::SetupRotIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, SApplyRotationIKSolver& ikSolver, const SBehaviorConstraintNodeFloorIKCommon& common ) const
{
	ikSolver.StorePreIK( instance, output );
	ikSolver.SetAdjustment( VectorToAnimVector( common.m_rootSpaceMS.TransformVector( m_finalNormalRS ) ),
							VectorToAnimVector( common.m_rootSpaceMS.TransformVector( m_finalNotClampedNormalRS ) ) );
}

void SBehaviorConstraintNodeFloorIKLeg::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame, const SBehaviorConstraintNodeFloorIKCommon& common ) const
{
#ifdef DEBUG_FLOOR_IK

	{
		Vector normalMS = common.m_rootSpaceMS.TransformVector( m_finalNotClampedNormalRS );
		frame->AddDebugLineWithArrow( m_debugFinalMatWS.GetTranslation(), m_debugFinalMatWS.GetTranslation() + common.m_localToWorld.TransformVector( common.m_rootSpaceMS.TransformVector( m_finalNormalRS ) ), 1.0f, 0.075f, 0.075f, Color( 0, 255, 0, 128 ), true );
		frame->AddDebugLineWithArrow( m_debugFinalMatWS.GetTranslation(), m_debugFinalMatWS.GetTranslation() + common.m_localToWorld.TransformVector( common.m_rootSpaceMS.TransformVector( m_finalNotClampedNormalRS ) ), 1.0f, 0.075f, 0.075f, Color( 255, 0, 255, 128 ), true );
		Vector inputBasedNormalMS = common.m_rootSpaceMS.TransformVector( m_inputStartNormalRefForClampingRS );
		frame->AddDebugLineWithArrow( m_debugFinalMatWS.GetTranslation(), m_debugFinalMatWS.GetTranslation() + common.m_localToWorld.TransformVector( inputBasedNormalMS ), 1.0f, 0.075f, 0.075f, Color( 255, 255, 0, 128 ), true );
	}

	if ( m_debugHit )
	{
		frame->AddDebugLineWithArrow( m_debugHitLocWS, m_debugHitLocWS + m_debugRawNormalWS * 0.3f, 1.0f, 0.01f, 0.01f, Color( 255, 255, 0, 128 ), true );
		frame->AddDebugLineWithArrow( m_debugHitLocWS, m_debugHitLocWS + m_debugNormalWS * 0.3f, 1.0f, 0.01f, 0.01f, Color( 0, 255, 0 ), true );
		frame->AddDebugSphere( m_debugHitLocWS, 0.002f, Matrix::IDENTITY, Color( 0, 255, 0 ), true );
	}
	//!@#m_cachedTrace.OnGenerateFragments( instance, frame );
	//DrawDebugMatrix( m_debugStartMaintainedMatWS, 0.1f, 180, frame, Color( 0, 255, 255, 150 ) );
	//DrawDebugMatrix( m_debugFinalOffsetRefMatWS, 0.1f, 180, frame, Color( 255, 0, 0, 150 ) );
	/*
	frame->AddDebugLineWithArrow( common.m_rootSpaceWS.TransformPoint( m_debugStartFinalOffsetRefBaseRS ), common.m_rootSpaceWS.TransformPoint( m_debugStartFinalOffsetRefRS ), 1.0f, 0.05f, 0.05f, Color( 255, 0, 0, 90 ), true );
	frame->AddDebugLineWithArrow( common.m_rootSpaceWS.TransformPoint( m_debugStartFinalOffsetRefRS ), common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRefRS ), 1.0f, 0.05f, 0.05f, Color( 255, 128, 0, 128 ), true );
	frame->AddDebugLineWithArrow( common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRefRS ), common.m_rootSpaceWS.TransformPoint( m_debugUsedFootOffsetToStartRS + m_debugStartFinalOffsetRefRS ), 1.0f, 0.05f, 0.05f, Color( 128, 255, 0, 128 ), true );
	frame->AddDebugLineWithArrow( common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRefRS ), common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRS + m_usedFootOffsetRefRS ), 1.0f, 0.075f, 0.075f, Color( 0, 255, 0, 128 ), true );
	frame->AddDebugLineWithArrow( common.m_rootSpaceWS.TransformPoint( m_usedFootOffsetRS + m_usedFootOffsetRefRS ), m_debugFinalMatWS.GetTranslation(), 1.0f, 0.075f, 0.075f, Color( 0, 128, 255, 128 ), true );
	*/
	DrawDebugMatrix( m_debugStartMatWS, 0.1f, 180, frame, Color( 0, 255, 0, 150 ) );
	DrawDebugMatrix( m_debugFinalMatWS, 0.05f, 180, frame, Color( 255, 0, 255, 150 ) );
	//DrawDebugMatrix( m_debugIKMatWS, 0.1f, 180, frame, Color( 255, 255, 0, 150 ) );
	/*
	Vector textLoc = m_debugHitLocWS + Vector( 0.0f, 0.0f, 0.8f );
	frame->AddDebugText( textLoc, String::Printf( TXT("%.3f"), m_debugAboveGround ), 0, 0, true,
		Color( (Uint8)( 255.0f * m_debugAboveGround ), (Uint8)( 255.0f * (1.0f - m_debugAboveGround ) ), 100),
		Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("fvo:%.3f"), Vector::Dot3( common.m_footUprightNormalRS, m_usedFinalOffsetRS ) ), 0, 2, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("fvo*:%.3f"), m_debugFinalVerticalOffsetPrePelvis ), 0, 4, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("btvo:%.3f"), m_debugNewBaseVerticalOffset ), 0, 6, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("ntvo:%.3f"), m_debugNewFinalVerticalOffset ), 0, 8, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("atvo:%.3f"), m_debugNewFinalAdjustedVerticalOffset ), 0, 10, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("bvol:%.3f"), m_debugNewFinalVerticalOffsetLeft ), 0, 12, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("evol:%.3f"), Vector::Dot3( common.m_footUprightNormalRS, m_desiredFootOffsetLeftRS ) ), 0, 14, true, Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("mvo:%.3f"), m_debugMinVerticalOffset ), 0, 16, true, m_debugHitMinVerticalOffset? Color(255, 100, 100) : Color(200, 200, 200), Color(0,0,0,128) );
	frame->AddDebugText( textLoc, String::Printf( TXT("mdvo:%.3f"), m_debugMinDefaultVerticalOffset ), 0, 18, true, Color(200, 200, 200), Color(0,0,0,128) );
	*/
#endif
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKWeightHandler::SBehaviorConstraintNodeFloorIKWeightHandler()
{
	ReadyForNextFrame();
}

void SBehaviorConstraintNodeFloorIKWeightHandler::LimitDesiredWeight( Float limit )
{
	m_desiredWeight = Min( m_desiredWeight, limit );
}

void SBehaviorConstraintNodeFloorIKWeightHandler::UpdateWeight( Float timeDelta, Float blendTime, const SBehaviorConstraintNodeFloorIKCommon& common )
{
	Float currentlyDesiredWeight = Max( m_eventForcedWeight, 
										Min( m_blockEventDesiredWeight, Max( m_requiredEventDesiredWeight, Min( m_eventDesiredWeight, m_desiredWeight ) ) ) );
	if ( m_immediateBlendRequired )
	{
		m_currentWeight = currentlyDesiredWeight;
	}
	else
	{
		if ( m_requiresSlowBlendDueToEvent && m_desiredWeight > 0.0f ) // if event is only one wanting to blend out (desired weight is > 0.0f), allow slower blend time
		{
			blendTime = Max( blendTime, 0.5f );
		}
		if ( m_requiresSlowBlendDueToMovement ) // if event is only one wanting to blend out (desired weight is > 0.0f), allow slower blend time
		{
			blendTime = Max( blendTime, 0.4f );
		}
		m_currentWeight = BlendOnOffWithSpeedBasedOnTime( m_currentWeight, currentlyDesiredWeight, blendTime, timeDelta );
	}
	if ( Abs( m_currentWeight - currentlyDesiredWeight ) <= 0.01f )
	{
		// we've finished blending
		m_requiresSlowBlendDueToEvent = false;
		m_requiresSlowBlendDueToMovement = false;
	}
}

void SBehaviorConstraintNodeFloorIKWeightHandler::ReadyForNextFrame()
{
	m_requiresSlowBlendDueToEvent = m_requiresSlowBlendDueToEvent || m_eventDesiredWeight < 1.0f || m_eventForcedWeight > 0.0f || m_requiredEventDesiredWeight > 0.0f || m_blockEventDesiredWeight < 1.0f; // to keep it if animation wants to be blended
	m_requiresSlowBlendDueToMovement = m_requiresSlowBlendDueToMovement || m_desiredWeight < 1.0f; // as above
	m_immediateBlendRequired = false;
	m_desiredWeight = 1.0f;
	m_eventDesiredWeight = 1.0f;
	m_eventForcedWeight = 0.0f;
	m_requiredEventDesiredWeight = 0.0f;
	m_blockEventDesiredWeight = 1.0f;
}

void SBehaviorConstraintNodeFloorIKWeightHandler::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.m_type == AET_Duration || event.m_type == AET_DurationStartInTheMiddle )
	{
		if ( event.GetEventName() == CNAME( ForceIKOn ) )
		{
			m_eventForcedWeight = Max( 0.0f, event.m_alpha );
			m_requiresSlowBlendDueToEvent = true;
		}
		if ( event.GetEventName() == CNAME( FloorIKOff ) )
		{
			m_eventDesiredWeight = Max( 0.0f, m_eventDesiredWeight - event.m_alpha );
			m_requiresSlowBlendDueToEvent = true;
		}
		if ( ! m_requiredAnimEvent.Empty() &&
			event.GetEventName() == m_requiredAnimEvent )
		{
			m_requiredEventDesiredWeight = Max( 0.0f, event.m_alpha );
			m_requiresSlowBlendDueToEvent = true;
		}
		if ( ! m_blockAnimEvent.Empty() &&
			event.GetEventName() == m_blockAnimEvent )
		{
			m_blockEventDesiredWeight = Max( 0.0f, m_blockEventDesiredWeight - event.m_alpha );
			m_requiresSlowBlendDueToEvent = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKLegsIKWeightHandler::SBehaviorConstraintNodeFloorIKLegsIKWeightHandler()
	: m_eventDesiredWeight(1.0f)
	, m_currentWeight(1.0f)
	, m_blendTime(0.25f)
{
}

void SBehaviorConstraintNodeFloorIKLegsIKWeightHandler::Update( Float timeDelta )
{
	m_currentWeight = BlendOnOffWithSpeedBasedOnTime( m_currentWeight, m_eventDesiredWeight, m_blendTime, timeDelta );
	ReadyForNextFrame();
}

void SBehaviorConstraintNodeFloorIKLegsIKWeightHandler::ReadyForNextFrame()
{
	m_eventDesiredWeight = 1.0f;
}

void SBehaviorConstraintNodeFloorIKLegsIKWeightHandler::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.m_type == AET_Duration || event.m_type == AET_DurationStartInTheMiddle )
	{
		if ( event.GetEventName() == CNAME( LegsIKOff ) )
		{
			m_eventDesiredWeight = 0.0f;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintNodeFloorIKFrontBackWeightHandler::PartWeightHandler::Update( Float timeDelta )
{
	m_currentWeight = BlendOnOffWithSpeedBasedOnTime( m_currentWeight, m_eventDesiredWeight, m_blendTime, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKFrontBackWeightHandler::SBehaviorConstraintNodeFloorIKFrontBackWeightHandler()
{
	ReadyForNextFrame();
}

void SBehaviorConstraintNodeFloorIKFrontBackWeightHandler::Update( Float timeDelta )
{
	m_front.Update( timeDelta );
	m_back.Update( timeDelta );
	ReadyForNextFrame();
}

void SBehaviorConstraintNodeFloorIKFrontBackWeightHandler::ReadyForNextFrame()
{
	m_front.m_eventDesiredWeight = 1.0f;
	m_back.m_eventDesiredWeight = 1.0f;
}

void SBehaviorConstraintNodeFloorIKFrontBackWeightHandler::HandleEvent( const CAnimationEventFired &event )
{
	if ( event.m_type == AET_Duration || event.m_type == AET_DurationStartInTheMiddle )
	{
		if ( event.GetEventName() == CNAME( FrontIKOff ) )
		{
			m_front.m_eventDesiredWeight = 0.0f;
		}
		if ( event.GetEventName() == CNAME( BackIKOff ) )
		{
			m_back.m_eventDesiredWeight = 0.0f;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIKBase::CBehaviorConstraintNodeFloorIKBase()
	: m_blockAnimEvent( CNAME( AlignToGround ) )
	, m_useFixedVersion( false )
	, m_slopeAngleDamp( 0.2f )
#ifndef NO_EDITOR_GRAPH_SUPPORT
	, m_forceSetup( false )
	, m_generateEditorFragmentsForLegIndex( -1 )
	, m_canBeDisabledDueToFrameRate( false )
#endif
{
}

void CBehaviorConstraintNodeFloorIKBase::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( ! m_generateEditorFragments )
	{
		return;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_forceSetup )
	{
		Setup( instance );
		m_forceSetup = false;
	}
#endif

#ifdef DEBUG_FLOOR_IK_POSES
	{
		//TDynArray< Matrix >& poseAnim = instance[ i_debugPoseAnim ];
		TDynArray< Matrix >& posePre = instance[ i_debugPosePre ];
		TDynArray< Matrix >& posePost = instance[ i_debugPosePost ];
		
		const CSkeleton* sk = instance.GetAnimatedComponent()->GetSkeleton();
		//SkeletonRenderingUtils::DrawSkeleton( poseAnim, sk, Color( 0, 0, 255 ), frame, true, false, false, true );
		SkeletonRenderingUtils::DrawSkeleton( posePre, sk, Color( 0, 255, 0 ), frame, true, false, false, true );
		SkeletonRenderingUtils::DrawSkeleton( posePost, sk, Color( 255, 0, 0 ), frame, true, false, false, true );
	}
#endif

	GenerateFragments( instance, frame );
}

void CBehaviorConstraintNodeFloorIKBase::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_prevTimeDelta;
	compiler << i_common;
	compiler << i_weightHandler;
	compiler << i_inCutscene;
	compiler << i_slopeDampedValue;

#ifdef DEBUG_FLOOR_IK_POSES
	compiler << i_debugPoseAnim;
	compiler << i_debugPosePre;
	compiler << i_debugPosePost;
#endif
}

void CBehaviorConstraintNodeFloorIKBase::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	Setup( instance );

	instance[ i_common ].SetUprightNormalMS( Vector::EZ, Vector::EZ );
	instance[ i_common ].m_useFixedVersion = m_useFixedVersion;
}

void CBehaviorConstraintNodeFloorIKBase::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( ! m_requiredAnimEvent.Empty() )
	{
		instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_requiredAnimEvent )->RegisterHandler( &instance[ i_weightHandler ] );
	}
	if ( ! m_blockAnimEvent.Empty() )
	{
		instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_blockAnimEvent )->RegisterHandler( &instance[ i_weightHandler ] );
	}
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( ForceIKOn ) )->RegisterHandler( &instance[ i_weightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( FloorIKOff ) )->RegisterHandler( &instance[ i_weightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( OnSlope ) )->RegisterHandler( &instance[ i_common ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( Jumping ) )->RegisterHandler( &instance[ i_common ] );
	instance[ i_common ].OnActivated( instance );
	instance[ i_timeDelta ] = MIN_TIME_DELTA; // Just to be sure we do not divide by zero
	instance[ i_prevTimeDelta ] = MIN_TIME_DELTA; // Just to be sure we do not divide by zero
	instance[ i_slopeDampedValue ] = 0.f;
}

void CBehaviorConstraintNodeFloorIKBase::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( ! m_requiredAnimEvent.Empty() )
	{
		instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_requiredAnimEvent )->UnregisterHandler( &instance[ i_weightHandler ] );
	}
	if ( ! m_blockAnimEvent.Empty() )
	{
		instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_blockAnimEvent )->UnregisterHandler( &instance[ i_weightHandler ] );
	}
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( ForceIKOn ) )->UnregisterHandler( &instance[ i_weightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( FloorIKOff ) )->UnregisterHandler( &instance[ i_weightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( OnSlope ) )->UnregisterHandler( &instance[ i_common ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( Jumping ) )->UnregisterHandler( &instance[ i_common ] );
}

void CBehaviorConstraintNodeFloorIKBase::Setup( CBehaviorGraphInstance& instance ) const
{
	instance[ i_weightHandler ].SetAnimEvents( m_requiredAnimEvent, m_blockAnimEvent );
	instance[ i_common ].Setup( m_common, instance );
#ifdef DEBUG_FLOOR_IK
	instance[ i_common ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testFloorSlidingOnSlopeIK ) );
#endif
}

void CBehaviorConstraintNodeFloorIKBase::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	if ( m_forceSetup )
	{
		Setup( instance );
		m_forceSetup = false;
	}
#endif

	TBaseClass::OnUpdate( context, instance, timeDelta );
	Float & iPrevTimeDelta = instance[ i_prevTimeDelta ];
	Float & iTimeDelta = instance[ i_timeDelta ];
	iPrevTimeDelta = iTimeDelta > MIN_TIME_DELTA ? iTimeDelta : timeDelta;
	iTimeDelta = timeDelta;
}

void CBehaviorConstraintNodeFloorIKBase::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_PROFILER_LEVEL_1( Constraint_FloorIK );

	TBaseClass::Sample( context, instance, output );

	ANIM_NODE_PRE_SAMPLE

	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();
	const CMovingAgentComponent* movingAgentComponent = Cast< CMovingAgentComponent >( animatedComponent );
	ASSERT( movingAgentComponent );

	const Float timeDelta = instance[ i_timeDelta ];
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	common.m_prevLocalToWorld = common.m_localToWorld;
	common.m_localToWorld = animatedComponent->GetEntity()->GetLocalToWorld();

	DEBUG_ANIM_MATRIX( common.m_prevLocalToWorld );
	DEBUG_ANIM_MATRIX( common.m_localToWorld );

	const Vector currentPosition = common.m_localToWorld.GetTranslation();
	AnimVector4 offsetWS( 0.f, 0.f, 0.f );

#ifdef DEBUG_FLOOR_IK_POSES
	{
		TDynArray< Matrix >& pose = instance[ i_debugPoseAnim ];
		CachePose( animatedComponent, output, common.m_localToWorld, pose );
	}
#endif

	Float slopeWeight( 0.f );
	if ( m_useFixedVersion )
	{
		if ( output.m_numBones > 9 )
		{
			// Damp angle instead of weight because input data (angle from animation event) can be corrupted
			Float& currAngleValue = instance[ i_slopeDampedValue ];
			currAngleValue = BlendToWithBlendTime( currAngleValue, common.m_slopeFromAnim, m_slopeAngleDamp, timeDelta );
			common.m_slopeFromAnim = currAngleValue;

			const Float destAngle = common.m_slopeFromAnim;

			slopeWeight = MAbs( destAngle ) / 45.f;
			RED_ASSERT( slopeWeight >= 0.f && slopeWeight <= 1.f );

			Float currValue = slopeWeight;
			//Float& currValue = instance[ i_prePoseCorrDampValue ];
			//currValue = BlendToWithBlendTime( currValue, slopeWeight, m_prePoseCorrDamp, timeDelta );

			offsetWS.Z = -0.17f * currValue; // 0.17 is not a magic value - it depends on slope max angle plus character capsule size
			offsetWS.X = 0.f;
			offsetWS.Y = 0.f;
			offsetWS.W = 1.0;

			common.m_localToWorld.SetTranslation( currentPosition + AnimVectorToVector( offsetWS ) );

			DEBUG_ANIM_MATRIX( common.m_localToWorld );
		}
	}

#ifdef DEBUG_FLOOR_IK_POSES
	{
		TDynArray< Matrix >& pose = instance[ i_debugPosePre ];
		CachePose( animatedComponent, output, common.m_localToWorld, pose );
	}
#endif

	Float prevTimeDelta = instance[ i_prevTimeDelta ];
	SBehaviorConstraintNodeFloorIKWeightHandler& weightHandler = instance [ i_weightHandler ];
	Bool& inCutscene = instance[ i_inCutscene ];

	ECharacterPhysicsState currentPhysicsState = animatedComponent->GetCurrentPhysicsState();
	CWorld* world = instance.GetAnimatedComponent()->GetWorld();

	Bool ragdolled = currentPhysicsState == CPS_Ragdoll
				  || animatedComponent->IsRagdolled()
#ifdef USE_PHYSX
				  || ( movingAgentComponent && movingAgentComponent->GetAnimationProxy().GetInAnimatedRagdoll() )
#endif
					;

	weightHandler.UseLongerBlendDueToMovement( currentPhysicsState == CPS_Animated ||
											   currentPhysicsState == CPS_Swimming ||
											   ragdolled );

	Bool nowInCutScene = animatedComponent->IsInCinematic();
	if ( const IActorInterface* actor = animatedComponent->GetEntity()->QueryActorInterface() ) // TODO - cache it?
	{
#ifdef RED_ASSERTS_ENABLED
		if ( nowInCutScene )
		{
			RED_ASSERT( actor->IsInNonGameplayScene() );
		}
#endif
		nowInCutScene = nowInCutScene || actor->IsInNonGameplayScene();
	}
											
	Bool disableDueToFrameRateAsAsked = m_canBeDisabledDueToFrameRate && animatedComponent->ShouldConstraintsBeDisabledIfPossible();
	Float prevWeight = /* GetWeight( instance ) * */ weightHandler.GetCurrentWeight();
	if ( m_cachedControlValueNode )
	{
		weightHandler.LimitDesiredWeight( Clamp( m_cachedControlValueNode->GetValue( instance ), 0.0f, 1.0f ) );
	}

	// set limit by weight from anim
	weightHandler.LimitDesiredWeight( movingAgentComponent? movingAgentComponent->GetAnimationProxy().GetUseIKWeight() : 1.0f );	//  limit to blended use ik weight
	
	// disable IK in some situations
	if ( nowInCutScene && movingAgentComponent->GetEntity()->HasTag( CNAME( horse ) ) )
	{
		weightHandler.LimitDesiredWeight( 0.0f );
	}
	else
	{
		weightHandler.LimitDesiredWeight( (disableDueToFrameRateAsAsked																	//  requested to be disabled
								   || animatedComponent->GetEntity()->GetTransformParent() != NULL		//	hard attached to something
								   || nowInCutScene														//	in a cutscene
								   || (movingAgentComponent->IsPhysicalRepresentationEnabled() && currentPhysicsState != CPS_Simulated) //  physically not simulated (animated, swimming, falling, ragdoll)
								   || world->GetPreviewWorldFlag()										//	is preview world
								   || ragdolled	)                                                       //	is in ragdoll
									? 0.0f : 1.0f );
	}

	// immediate blending on cutscenes
	if ( nowInCutScene || inCutscene )
	{
		weightHandler.ImmediateBlendRequired();
	}
	inCutscene = nowInCutScene;
	weightHandler.UpdateWeight( timeDelta, currentPhysicsState == CPS_Falling? 0.1f : 0.12f, common ); // TODO parameter?
	Float weight = /* GetWeight( instance ) * */ weightHandler.GetCurrentWeight();
	weightHandler.ReadyForNextFrame();

#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		prevWeight = weight = instance.GetInternalFloatValue( common.m_testVarID );
	}
#endif

	// update common
	common.Update( m_common, timeDelta, prevTimeDelta, instance, weight );

	if ( common.m_additionalBlendCoefOnTeleport > 0.0f )
	{
		// will enforce reset
		prevWeight = 0.0f;
	}

	// here is good place to update upright normal - for quadrupeds we will have to do it here
	UpdateUprightWS( context, instance, output );

	// update adjustment offset
	common.UpdateAdjustmentOffset( m_common, timeDelta, prevTimeDelta, weight );

	// apply rotation
	if ( weight > 0.001f )
	{
		AnimQsTransform& outputRoot = output.m_outputPose[ 0 ];
		AnimQsTransform rotateToCurrentUpMS = MatrixToAnimQsTransform( common.m_rootSpaceMS );
		AnimQsTransform newOutputRoot = output.m_outputPose[ 0 ];
		SetMulTransform( newOutputRoot, rotateToCurrentUpMS, outputRoot );
#ifdef USE_HAVOK_ANIMATION
		outputRoot = weight > 0.0f? newOutputRoot : outputRoot;
		ASSERT(false, TXT("Do we actually need it? It should be lerped"));
		output.m_outputPose[ 0 ].m_rotation.normalize();
#else
		outputRoot.Lerp( outputRoot, newOutputRoot, weight );
		output.m_outputPose[ 0 ].Rotation.Normalize();
#endif
	}

	UpdateAndSampleIK( context, instance, output, weight, prevWeight );

	if ( m_useFixedVersion && output.m_numBones > 9 )
	{
		//AnimQsTransform& root = output.m_outputPose[ 0 ];
		AnimQsTransform& pelvisLS = output.m_outputPose[ 9 ];
		//AnimQsTransform pelvisMS;
		//pelvisMS.SetMul( root, pelvisLS );

		//Matrix reall2w( common.m_localToWorld );

		AnimVector4 finalOffsetWS( offsetWS );

		//reall2w.SetTranslation( currentPosition );
		//const AnimQsTransform l2wTrans = MatrixToAnimQsTransform( reall2w );
		//AnimQsTransform pelvisWS;
		//AnimQsTransform rootWS;

		//pelvisWS.SetMul( l2wTrans, pelvisMS );
		//rootWS.SetMul( l2wTrans, root );

		//AnimQsTransform newPelvisWS( pelvisWS );
		//newPelvisWS.Translation += finalOffsetWS;

		//SetMulInverseMulTransform( pelvisLS, rootWS, newPelvisWS );

		SetAdd( pelvisLS.Translation, finalOffsetWS );
	}

#ifdef DEBUG_FLOOR_IK_POSES
	{
		Matrix reall2w( common.m_localToWorld );
		reall2w.SetTranslation( currentPosition );

		TDynArray< Matrix >& pose = instance[ i_debugPosePost ];
		CachePose( animatedComponent, output, reall2w, pose );
	}

#endif

	ANIM_NODE_POST_SAMPLE
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIK::CBehaviorConstraintNodeFloorIK()
{
	m_canBeDisabledDueToFrameRate = true;
	m_common.m_gravityCentreBone = CNAME( pelvis );
	m_common.m_verticalVelocityOffsetDownBlendTime = 0.08f;
	m_common.m_verticalVelocityOffsetUpBlendTime = 0.10f;
	m_pelvis.m_bone = CNAME( pelvis );
	m_pelvis.m_verticalOffsetBlendTime = 0.05f;
	m_legs.m_verticalOffsetBlendUpTime = 0.08f;
	m_legs.m_verticalOffsetBlendDownTime = 0.03f;
	m_legs.m_traceOffsetRange.X = -1.5f;
	m_legs.m_traceOffsetRange.Y = 1.5f;
	m_legs.m_maxAngleOffUprightNormalSide = 15.0f;
	m_legs.m_useInputNormalForNormalClamping = true;
	m_leftLegIK.m_upperBone.m_name = CNAME( l_thigh );
	m_leftLegIK.m_jointBone.m_name = CNAME( l_shin );
	m_leftLegIK.m_lowerBone.m_name = CNAME( l_foot );
	m_leftLegIK.m_limitToLengthPercentage = 0.99f;
	m_rightLegIK.m_upperBone.m_name = CNAME( r_thigh );
	m_rightLegIK.m_jointBone.m_name = CNAME( r_shin );
	m_rightLegIK.m_lowerBone.m_name = CNAME( r_foot );
	m_rightLegIK.m_limitToLengthPercentage = 0.99f;
}

void CBehaviorConstraintNodeFloorIK::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT( "pelvis" ) ||
		property->GetName() == TXT( "leftLegIK" ) ||
		property->GetName() == TXT( "rightLegIK" ) )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		m_forceSetup = true;
#endif
	}
}

void CBehaviorConstraintNodeFloorIK::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftLeg;
	compiler << i_rightLeg;
	compiler << i_legs;
	compiler << i_pelvis;
	compiler << i_leftLegIK;
	compiler << i_rightLegIK;
#ifdef DEBUG_FLOOR_IK_TRACES
	compiler << i_characterTrace;
	compiler << i_characterOffsetTrace;
	compiler << i_pelvisTrace;
	compiler << i_leftLegTrace;
	compiler << i_leftLegBaseTrace;
	compiler << i_leftLegHitTrace;
	compiler << i_rightLegTrace;
	compiler << i_rightLegBaseTrace;
	compiler << i_rightLegHitTrace;
#endif
}

void CBehaviorConstraintNodeFloorIK::Setup( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::Setup( instance );
	instance[ i_legs ].Setup( instance, m_legs );
	instance[ i_pelvis ].Setup( instance, m_pelvis );
	instance[ i_leftLegIK ].Setup( instance, m_leftLegIK );
	instance[ i_rightLegIK ].Setup( instance, m_rightLegIK );
	instance[ i_leftLeg ].Setup( instance, instance[ i_leftLegIK ] );
	instance[ i_rightLeg ].Setup( instance, instance[ i_rightLegIK ] );
#ifdef DEBUG_FLOOR_IK
	instance[ i_leftLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testLeft ), CNAME( testLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testRight ), CNAME( testRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
#endif
	if ( CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponentUnsafe() ) )
	{
		mac->AccessAnimationProxy().SetPelvisBoneIndex( instance[ i_pelvis ].m_bone );
	}
}

void CBehaviorConstraintNodeFloorIK::UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	SBehaviorConstraintNodeFloorIKLegs& legs = instance[ i_legs ];
	SBehaviorConstraintNodeFloorIKVerticalBone& pelvis = instance[ i_pelvis ];
	SBehaviorConstraintNodeFloorIKLeg& leftLeg = instance[ i_leftLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightLeg = instance[ i_rightLeg ];
	STwoBonesIKSolver& leftLegIK = instance[ i_leftLegIK ];
	STwoBonesIKSolver& rightLegIK = instance[ i_rightLegIK ];

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	leftLegIK.GatherDebugData( instance, output, m_leftLegIK );
	rightLegIK.GatherDebugData( instance, output, m_rightLegIK );
#endif

	// check how much resulting velocity is part of requested one
	// this should help avoiding foot sliding when going on slopes
	Vector feetDistanceDirMS = Vector::ZEROS;
	Float feetDistanceCoef = 1.0f;
	Float feetDistanceOffset = 0.0f;
	Float const notSlidingOnSlope = common.m_slidingOnSlopeWeight;
	if ( notSlidingOnSlope > 0.0f )
	{
		if ( CMovingAgentComponent const * mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponent() ) )
		{
			feetDistanceDirMS = (-mac->GetWorldRotation()).TransformVector( mac->GetWorldForward() );
			Vector behaviorGraphVelocity = mac->GetVelocityBasedOnRequestedMovement();
			Float behaviorGraphSpeed = behaviorGraphVelocity.Mag2();
			if ( behaviorGraphSpeed > 0.1f )
			{
				Vector velocity = mac->GetVelocity();
				Vector behaviorGraphVelocityNormal = behaviorGraphVelocity.Normalized2();
				Float speedCoef = Vector::Dot2( behaviorGraphVelocityNormal, velocity ) / behaviorGraphSpeed;
				feetDistanceDirMS = (-mac->GetWorldRotation()).TransformVector( behaviorGraphVelocityNormal );
				feetDistanceCoef = Clamp( speedCoef, 0.5f, 1.0f );
			}
		}
		{
			Vector deltaTranslation = common.m_localToWorld.GetTranslation() - common.m_prevLocalToWorld.GetTranslation();
			Float limitOffset = 0.5f * common.m_centreOfGravityCentreAlt;
			feetDistanceOffset = Clamp( deltaTranslation.Z * 2.0f, -limitOffset, limitOffset ); // move feet forward when climbing up or backward when going down, note: this is for now and might be gone when we will have proper animations for going up/down slope
		}
	}
	common.RequestFeetDistanceAdjustment( feetDistanceDirMS, ( feetDistanceCoef - 1.0f ) * notSlidingOnSlope + 1.0f, feetDistanceOffset * notSlidingOnSlope );

	CMovingAgentComponent* mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponentUnsafe() );
	if ( mac && mac->GetAnimationProxy().HACK_GetJustTeleportedFlag() )
	{
		common.m_HACK_resetFrameCounter = 2; // change in one frame does not work, cuz it cums when entity is in old position and ik stores state from previous frames.
		mac->AccessAnimationProxy().HACK_SetJustTeleportedFlag( false ); // Unmark flag, cuz we processed it.
	}

	// update ik only if weight is non zero, otherwise ignore it
	if ( weight > 0.001f )
	{
		if ( prevWeight <= 0.001f )
		{
			common.Reset( instance );

			leftLeg.Reset( common );
			rightLeg.Reset( common );

			pelvis.Reset();
		}

		leftLeg.Update( timeDelta, context, instance, output, m_leftLegIK, leftLegIK, legs, m_legs, common );
		rightLeg.Update( timeDelta, context, instance, output, m_rightLegIK, rightLegIK, legs, m_legs, common );

		// calculate offset for pelvis
		pelvis.Update( timeDelta, leftLeg, rightLeg, m_pelvis, m_legs, common );

		leftLeg.SetupIK( instance, output, leftLegIK, m_legs, common );
		rightLeg.SetupIK( instance, output, rightLegIK, m_legs, common );

		pelvis.UpdatePose( instance, output, m_pelvis, common, weight, true );
		leftLegIK.UpdatePose( instance, output, m_leftLegIK, weight, timeDelta );
		rightLegIK.UpdatePose( instance, output, m_rightLegIK, weight, timeDelta );
	}

	if (common.m_HACK_resetFrameCounter > 0)
	{
		--common.m_HACK_resetFrameCounter;
	}

	/*
	static Float timePassed = 0.0f;
	timePassed += timeDelta;
	common.SetRequestedUp( common.m_rootSpaceWS.TransformVector( leftLeg.m_finalNormalRS ) + common.m_rootSpaceWS.TransformVector( rightLeg.m_finalNormalRS ) );
	common.SetRequestedUp( Vector(sin(timePassed), 0.0f, 1.0f).Normalized3() );
	*/

#ifdef DEBUG_FLOOR_IK_TRACES
	if ( weight > 0.001f )
	{
		pelvis.StoreTraces( instance, output, common, instance[ i_characterTrace ], instance[ i_characterOffsetTrace ], instance[ i_pelvisTrace ] );
		leftLeg.StoreTraces( instance, output, common, leftLegIK, instance[ i_leftLegTrace ], instance[ i_leftLegBaseTrace ], instance[ i_leftLegHitTrace ] );
		rightLeg.StoreTraces( instance, output, common, rightLegIK, instance[ i_rightLegTrace ], instance[ i_rightLegBaseTrace ], instance[ i_rightLegHitTrace ] );
	}
	else
	{
		instance[ i_characterTrace ].StoreEmpty();
		instance[ i_characterOffsetTrace ].StoreEmpty();
		instance[ i_pelvisTrace ].StoreEmpty();
		instance[ i_leftLegTrace ].StoreEmpty();
		instance[ i_leftLegBaseTrace ].StoreEmpty();
		instance[ i_leftLegHitTrace ].StoreEmpty();
		instance[ i_rightLegTrace ].StoreEmpty();
		instance[ i_rightLegBaseTrace ].StoreEmpty();
		instance[ i_rightLegHitTrace ].StoreEmpty();
	}
#endif
}

void CBehaviorConstraintNodeFloorIK::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
#ifdef DEBUG_FLOOR_IK_TRACES
	instance[ i_characterTrace ].GenerateFragments( frame, Color(0,0,200,128), 0.04f );
	instance[ i_characterOffsetTrace ].GenerateFragments( frame, Color(0,255,255,255) );
	instance[ i_pelvisTrace ].GenerateFragments( frame, Color(255,0,255,255), 0.02f );
	instance[ i_leftLegTrace ].GenerateFragments( frame, Color(100,255,0,255) );
	instance[ i_leftLegBaseTrace ].GenerateFragments( frame, Color(100,255,0,128), 0.01f );
	instance[ i_leftLegHitTrace ].GenerateFragments( frame, Color(0,128,0,128), 0.02f );
	instance[ i_rightLegTrace ].GenerateFragments( frame, Color(255,100,0,255) );
	instance[ i_rightLegBaseTrace ].GenerateFragments( frame, Color(255,100,0,128), 0.01f );
	instance[ i_rightLegHitTrace ].GenerateFragments( frame, Color(128,0,0,128), 0.02f );
#endif
	Float weight = instance[ i_weightHandler ].GetCurrentWeight();
	const SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		weight = instance.GetInternalFloatValue( common.m_testVarID );
	}

	Vector debugLoc = instance[ i_pelvis ].m_debugVerticalBoneLocWS + Vector(0.0f, 0.0f, 0.4f);
	frame->AddDebugText( debugLoc, String::Printf( TXT("IK: %.3f (t:%.0f)"), weight, instance[ i_common ].m_additionalBlendCoefOnTeleport ), 0, 0, true,
		Color( (Uint8)( 255.0f * (1.0f - weight) ), (Uint8)( 255.0f * weight ), 100),
		Color(0,0,0,128) );
#endif

	if ( weight > 0.0f )
	{
		Vector loc = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().GetTranslation() + instance[ i_pelvis ].m_offsetVerticallyMS;
		Vector locNoOffset = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().GetTranslation();
		Vector forward = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().GetAxisY();
		Vector side = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().GetAxisX();
		Float const slopeAngleFromAnimRad = DEG2RAD( instance[ i_common ].m_slopeFromAnim );
		for (Float a = -1.0f; a < 1.0f; a += 0.1f)
		{
			Vector aloc( Vector::ZERO_3D_POINT );
			Vector alocNoOffset( Vector::ZERO_3D_POINT );

			if ( common.m_useFixedVersion )
			{
				aloc = loc + forward * a + Vector::EZ * a * MTan( slopeAngleFromAnimRad );
				alocNoOffset = locNoOffset + forward * a + Vector::EZ * a * MTan( slopeAngleFromAnimRad );
			}
			else
			{
				aloc = loc + forward * a + Vector::EZ * a * sin( slopeAngleFromAnimRad );
				alocNoOffset = locNoOffset + forward * a + Vector::EZ * a * sin( slopeAngleFromAnimRad );
			}

			frame->AddDebugLine( aloc - side, aloc + side, Color( 155, 0, 0, 100 ), true, true );
			frame->AddDebugLine( aloc - side, aloc + side, Color( 0, 255, 0, 150 ), false, false );
			frame->AddDebugLine( alocNoOffset - side, alocNoOffset + side, Color( 0, 155, 200, 100 ), true, true );
		}

#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( m_generateEditorFragmentsForIKSolvers )
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightLegIK ].OnGenerateFragments( instance, frame );
			}

#ifdef DEBUG_FLOOR_IK
			if ( common.m_useFixedVersion )
			{
				frame->AddDebugSphere( instance[ i_leftLeg ].m_debugHitLocWS, 0.035, Matrix::IDENTITY, Color( 255, 0, 0 ), false, true );
				frame->AddDebugSphere( instance[ i_rightLeg ].m_debugHitLocWS, 0.035, Matrix::IDENTITY, Color( 255, 0, 0 ), false, true );
				frame->AddDebugSphere( instance[ i_leftLeg ].m_debugUsedFootOffsetRefWS, 0.02, Matrix::IDENTITY, Color( 255, 255, 0 ), true, true );
				frame->AddDebugSphere( instance[ i_rightLeg ].m_debugUsedFootOffsetRefWS, 0.02, Matrix::IDENTITY, Color( 255, 255, 0 ), true, true );
				
				Float slopeAngleRaw( 0.f );
				{
					const Matrix& l2w_p = instance[ i_leftLeg ].m_debugLocalToWorldPrev;
					const Matrix& l2w_c = instance[ i_leftLeg ].m_debugLocalToWorldCurr;
					const Vector l2w_diff = l2w_c.GetTranslation() - l2w_p.GetTranslation();
					const Float mag3 = l2w_diff.Mag3();
					slopeAngleRaw = mag3 > 0.001f ? RAD2DEG( MAsin_safe( l2w_diff.Z / mag3 ) ) : 0.f;
				}

				Float errL = instance[ i_leftLeg ].m_debugOffset.Z;
				Float errR = instance[ i_rightLeg ].m_debugOffset.Z;

				const Vector startPoint = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld().GetTranslation();
				String txtA = String::Printf( TXT("err: %.3f, Raw: %2.1f, Anim: %2.1f, Slope: %2.1f, D: %2.1f"), errL, slopeAngleRaw, instance[ i_leftLeg ].m_debugAngleAnim, instance[ i_leftLeg ].m_debugAngleSlope, instance[ i_leftLeg ].m_debugAngleAnim-instance[ i_leftLeg ].m_debugAngleSlope );
				frame->AddDebugText( startPoint + Vector( 0.f, 0.f, 2.f ), txtA, 0, 0, true );
				String txtB = String::Printf( TXT("err: %.3f, Raw: %2.1f, Anim: %2.1f, Slope: %2.1f, D: %2.1f"), errR, slopeAngleRaw, instance[ i_rightLeg ].m_debugAngleAnim, instance[ i_rightLeg ].m_debugAngleSlope, instance[ i_rightLeg ].m_debugAngleAnim-instance[ i_rightLeg ].m_debugAngleSlope );
				frame->AddDebugText( startPoint + Vector( 0.f, 0.f, 2.f ), txtB, 0, 2, true );

				frame->AddDebugLine( instance[ i_leftLeg ].m_debugHitLocWS, instance[ i_rightLeg ].m_debugHitLocWS, Color( 255, 255, 255, 0 ), false, true );
				frame->AddDebugLine( instance[ i_rightLeg ].m_debugLocalToWorldCurr.GetTranslation(), instance[ i_rightLeg ].m_debugHitLocWS, Color( 255, 255, 255, 0 ), false, true );
				frame->AddDebugLine( instance[ i_leftLeg ].m_debugHitLocWS, instance[ i_rightLeg ].m_debugLocalToWorldCurr.GetTranslation(), Color( 255, 255, 255, 0 ), false, true );
				frame->AddDebugSphere( instance[ i_rightLeg ].m_debugLocalToWorldCurr.GetTranslation(), 0.04, Matrix::IDENTITY, Color( 255, 255, 255 ), false, true );
			}
#endif
		}
#ifdef DEBUG_FLOOR_IK
		else
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 )
			{
				frame->AddDebugText( debugLoc, String::Printf( TXT("aor: %.6f"), common.m_adjustOffsetByRaw ), 0, 2, true, Color(100,100,255), Color(0,0,0,128) );
				frame->AddDebugText( debugLoc, String::Printf( TXT("aov: %.6f"), common.m_adjustOffsetByVel ), 0, 4, true, Color(100,100,255), Color(0,0,0,128) );
				instance[ i_pelvis ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightLeg ].OnGenerateFragments( instance, frame, common );
			}
		}
#endif
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIKHandsOnly::CBehaviorConstraintNodeFloorIKHandsOnly()
{
	m_common.m_gravityCentreBone = CNAME( pelvis );
	m_common.m_verticalVelocityOffsetDownBlendTime = 0.03f;
	m_common.m_verticalVelocityOffsetUpBlendTime = 0.10f;
	m_hands.m_verticalOffsetBlendUpTime = 0.08f;
	m_hands.m_verticalOffsetBlendDownTime = 0.03f;
	m_hands.m_traceOffsetRange.X = -1.5f;
	m_hands.m_traceOffsetRange.Y = 1.5f;
	m_hands.m_maxAngleOffUprightNormalSide = 15.0f;
	m_leftHandIK.m_upperBone.m_name = CNAME( l_bicep );
	m_leftHandIK.m_jointBone.m_name = CNAME( l_forearm );
	m_leftHandIK.m_lowerBone.m_name = CNAME( l_hand );
	m_rightHandIK.m_upperBone.m_name = CNAME( r_bicep );
	m_rightHandIK.m_jointBone.m_name = CNAME( r_forearm );
	m_rightHandIK.m_lowerBone.m_name = CNAME( r_hand );
}

void CBehaviorConstraintNodeFloorIKHandsOnly::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT( "leftHandIK" ) ||
		 property->GetName() == TXT( "rightHandIK" ) )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		m_forceSetup = true;
#endif
	}
}

void CBehaviorConstraintNodeFloorIKHandsOnly::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftHand;
	compiler << i_rightHand;
	compiler << i_hands;
	compiler << i_leftHandIK;
	compiler << i_rightHandIK;
}

void CBehaviorConstraintNodeFloorIKHandsOnly::Setup( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::Setup( instance );
	instance[ i_hands ].Setup( instance, m_hands );
	instance[ i_leftHandIK ].Setup( instance, m_leftHandIK );
	instance[ i_rightHandIK ].Setup( instance, m_rightHandIK );
	instance[ i_leftHand ].Setup( instance, instance[ i_leftHandIK ] );
	instance[ i_rightHand ].Setup( instance, instance[ i_rightHandIK ] );
#ifdef DEBUG_FLOOR_IK
	instance[ i_leftHand ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testLeft ), CNAME( testLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightHand ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testRight ), CNAME( testRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
#endif
}

void CBehaviorConstraintNodeFloorIKHandsOnly::UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	SBehaviorConstraintNodeFloorIKLegs& hands = instance[ i_hands ];
	SBehaviorConstraintNodeFloorIKLeg& leftHand = instance[ i_leftHand ];
	SBehaviorConstraintNodeFloorIKLeg& rightHand = instance[ i_rightHand ];
	STwoBonesIKSolver& leftHandIK = instance[ i_leftHandIK ];
	STwoBonesIKSolver& rightHandIK = instance[ i_rightHandIK ];

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	leftHandIK.GatherDebugData( instance, output, m_leftHandIK );
	rightHandIK.GatherDebugData( instance, output, m_rightHandIK );
#endif

	// we want only sliding on slope here!
	weight *= common.m_slidingOnSlopeWeight;
	prevWeight *= common.m_prevSlidingOnSlopeWeight;

	// update ik only if weight is non zero, otherwise ignore it
	if ( weight > 0.001f )
	{
		if ( prevWeight <= 0.001f )
		{
			common.Reset( instance );

			leftHand.Reset( common );
			rightHand.Reset( common );
		}

		leftHand.Update( timeDelta, context, instance, output, m_leftHandIK, leftHandIK, hands, m_hands, common );
		rightHand.Update( timeDelta, context, instance, output, m_rightHandIK, rightHandIK, hands, m_hands, common );

		leftHand.SetupIK( instance, output, leftHandIK, m_hands, common );
		rightHand.SetupIK( instance, output, rightHandIK, m_hands, common );

		leftHandIK.UpdatePose( instance, output, m_leftHandIK, weight, timeDelta );
		rightHandIK.UpdatePose( instance, output, m_rightHandIK, weight, timeDelta );
	}

	/*
	static Float timePassed = 0.0f;
	timePassed += timeDelta;
	common.SetRequestedUp( common.m_rootSpaceWS.TransformVector( leftHand.m_finalNormalRS ) + common.m_rootSpaceWS.TransformVector( rightHand.m_finalNormalRS ) );
	common.SetRequestedUp( Vector(sin(timePassed), 0.0f, 1.0f).Normalized3() );
	*/
}

void CBehaviorConstraintNodeFloorIKHandsOnly::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	Float weight = instance[ i_weightHandler ].GetCurrentWeight();
	const SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		weight = instance.GetInternalFloatValue( common.m_testVarID );
	}
#endif
	Vector debugLoc = instance.GetAnimatedComponent()->GetWorldPosition() + Vector(0.0f, 0.0f, 2.0f);
	frame->AddDebugText( debugLoc, String::Printf( TXT("IK: %.3f"), weight ), 0, 0, true,
		Color( (Uint8)( 255.0f * (1.0f - weight) ), (Uint8)( 255.0f * weight ), 100),
		Color(0,0,0,128) );
	if ( weight > 0.0f )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( m_generateEditorFragmentsForIKSolvers )
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftHandIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightHandIK ].OnGenerateFragments( instance, frame );
			}
		}
		else
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 )
			{
				frame->AddDebugText( debugLoc, String::Printf( TXT("aor: %.6f"), common.m_adjustOffsetByRaw ), 0, 2, true, Color(100,100,255), Color(0,0,0,128) );
				frame->AddDebugText( debugLoc, String::Printf( TXT("aov: %.6f"), common.m_adjustOffsetByVel ), 0, 4, true, Color(100,100,255), Color(0,0,0,128) );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftHand ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightHand ].OnGenerateFragments( instance, frame, common );
			}
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIKBipedLong::CBehaviorConstraintNodeFloorIKBipedLong()
{
	m_speedForFullyPerpendicularLegs = 2.5f; // for dog
	m_upDirAdditionalWS = Vector::ZEROS;
	// default setup for dog/cat
	m_common.m_gravityCentreBone = CNAME( spine2 );
	m_legs.m_maxAngleOffUprightNormal = 25.0f;
	m_legs.m_maxAngleOffUprightNormalSide = 15.0f;
	m_legs.m_offsetRange = Vector2( -0.1f, 0.5f );
	m_pelvis.m_bone = CNAME( pelvis );
	m_pelvis.m_stiffness = 0.95f;
	m_pelvis.m_offsetRange = Vector2( -0.5f, 0.5f );
	m_leftLegIK.m_upperBone.m_name = CNAME( l_thigh );
	m_leftLegIK.m_jointBone.m_name = CNAME( l_shin );
	m_leftLegIK.m_lowerBone.m_name = CNAME( l_foot );
	m_rightLegIK.m_upperBone.m_name = CNAME( r_thigh );
	m_rightLegIK.m_jointBone.m_name = CNAME( r_shin );
	m_rightLegIK.m_lowerBone.m_name = CNAME( r_foot );
	m_leftShoulder.m_bone = CNAME( l_thigh );
	m_leftShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_leftShoulder.m_stiffness = 0.5f;
	m_rightShoulder.m_bone = CNAME( r_thigh );
	m_rightShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_rightShoulder.m_stiffness = 0.5f;
	m_neck1MaintainLook.m_bone = CNAME( neck );
	m_neck1MaintainLook.m_amountOfRotation = 0.3f;
	m_neck2MaintainLook.m_bone = CNAME( neck2 );
	m_neck2MaintainLook.m_amountOfRotation = 0.35f;
	m_headMaintainLook.m_bone = CNAME( head );
	m_headMaintainLook.m_amountOfRotation = 0.2f;
}

void CBehaviorConstraintNodeFloorIKBipedLong::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT( "pelvis" ) ||
		 property->GetName() == TXT( "leftLegIK" ) ||
		 property->GetName() == TXT( "rightLegIK" ) )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		m_forceSetup = true;
#endif
	}
}

void CBehaviorConstraintNodeFloorIKBipedLong::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftLeg;
	compiler << i_rightLeg;
	compiler << i_legs;
	compiler << i_pelvis;
	compiler << i_leftShoulder;
	compiler << i_rightShoulder;
	compiler << i_neck1MaintainLook;
	compiler << i_neck2MaintainLook;
	compiler << i_headMaintainLook;
	compiler << i_leftLegIK;
	compiler << i_rightLegIK;
	compiler << i_usePerpendicularUprightWS;
}

void CBehaviorConstraintNodeFloorIKBipedLong::Setup( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::Setup( instance );
	instance[ i_common ].m_cachedStepOffset = 5.0f; // almost always try to match terrain
	instance[ i_legs ].Setup( instance, m_legs );
	instance[ i_pelvis ].Setup( instance, m_pelvis );
	instance[ i_leftShoulder ].Setup( instance,   m_leftShoulder );
	instance[ i_rightShoulder ].Setup( instance,  m_rightShoulder );
	instance[ i_neck1MaintainLook].Setup( instance, m_neck1MaintainLook );
	instance[ i_neck2MaintainLook].Setup( instance, m_neck2MaintainLook );
	instance[ i_headMaintainLook].Setup( instance, m_headMaintainLook );
	instance[ i_leftLegIK ].Setup( instance, m_leftLegIK );
	instance[ i_rightLegIK ].Setup( instance, m_rightLegIK );
	instance[ i_leftLeg ].Setup( instance, instance[ i_leftLegIK ] );
	instance[ i_rightLeg ].Setup( instance, instance[ i_rightLegIK ] );
	instance[ i_usePerpendicularUprightWS ] = 0.0f;
#ifdef DEBUG_FLOOR_IK
	instance[ i_leftLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testLeft ), CNAME( testLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testRight ), CNAME( testRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
#endif
	if ( CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponentUnsafe() ) )
	{
		mac->AccessAnimationProxy().SetPelvisBoneIndex( instance[ i_pelvis ].m_bone );
	}
}

void CBehaviorConstraintNodeFloorIKBipedLong::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
}

void CBehaviorConstraintNodeFloorIKBipedLong::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );
}

void CBehaviorConstraintNodeFloorIKBipedLong::UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	Float prevTimeDelta = instance[ i_prevTimeDelta ];
	Float & usePerpendicularUprightWS = instance[ i_usePerpendicularUprightWS ];
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];

	Vector deltaTranslation = common.m_localToWorld.GetTranslation() - common.m_prevLocalToWorld.GetTranslation();
	Float speed = prevTimeDelta > MIN_TIME_DELTA ? deltaTranslation.Mag2() / prevTimeDelta : 0.f;
	DEBUG_ANIM_FLOAT( speed );

	Float newUsePerpendicularUprightWS = Clamp( speed / Max(0.001f, m_speedForFullyPerpendicularLegs ), 0.0f, 1.0f );

	usePerpendicularUprightWS = BlendToWithBlendTime( usePerpendicularUprightWS, newUsePerpendicularUprightWS, 0.2f, timeDelta + common.m_additionalBlendCoefOnTeleport );

	common.SetUprightNormalMS( Vector::EZ, common.m_localToWorldInverted.TransformVector( common.m_requestedUpWS ) * usePerpendicularUprightWS + Vector::EZ * ( 1.0f - usePerpendicularUprightWS ) );
	//common.SetUprightNormalRS( Vector::EZ, Vector::EZ * usePerpendicularUprightWS + common.m_rootSpaceWSInverted.TransformVector( Vector::EZ ) * ( 1.0f - usePerpendicularUprightWS ) );
}

void CBehaviorConstraintNodeFloorIKBipedLong::UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	SBehaviorConstraintNodeFloorIKLegs& legs = instance[ i_legs ];
	SBehaviorConstraintNodeFloorIKVerticalBone& pelvis = instance[ i_pelvis ];
	SBehaviorConstraintNodeFloorIKVerticalBone& leftShoulder = instance[ i_leftShoulder ];
	SBehaviorConstraintNodeFloorIKVerticalBone& rightShoulder = instance[ i_rightShoulder ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& neck1MaintainLook = instance[ i_neck1MaintainLook ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& neck2MaintainLook = instance[ i_neck2MaintainLook ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& headMaintainLook = instance[ i_headMaintainLook ];
	SBehaviorConstraintNodeFloorIKLeg& leftLeg = instance[ i_leftLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightLeg = instance[ i_rightLeg ];
	STwoBonesIKSolver& leftLegIK = instance[ i_leftLegIK ];
	STwoBonesIKSolver& rightLegIK = instance[ i_rightLegIK ];
#ifdef DEBUG_TWO_BONES_IK_SOLVER
	leftLegIK.GatherDebugData( instance, output, m_leftLegIK );
	rightLegIK.GatherDebugData( instance, output, m_rightLegIK );
#endif

	// update ik only if weight is non zero, otherwise ignore it
	if ( weight > 0.001f )
	{
		if ( prevWeight <= 0.001f )
		{
			common.Reset( instance );

			leftLeg.Reset( common );
			rightLeg.Reset( common );

			leftShoulder.Reset();
			rightShoulder.Reset();

			pelvis.Reset();
		}

		leftLeg.Update( timeDelta, context, instance, output, m_leftLegIK, leftLegIK, legs, m_legs, common );
		rightLeg.Update( timeDelta, context, instance, output, m_rightLegIK, rightLegIK, legs, m_legs, common );

		pelvis.Update( timeDelta, leftLeg, rightLeg, m_pelvis, m_legs, common );
		leftShoulder.Update( timeDelta, leftLeg, m_leftShoulder, m_legs, common );
		rightShoulder.Update( timeDelta, rightLeg, m_rightShoulder, m_legs, common );

		leftLeg.SetupIK( instance, output, leftLegIK, m_legs, common );
		rightLeg.SetupIK( instance, output, rightLegIK, m_legs, common );

		pelvis.UpdatePose( instance, output, m_pelvis, common, weight, true );
		leftShoulder.UpdatePose( instance, output, m_leftShoulder, common, weight );
		rightShoulder.UpdatePose( instance, output, m_rightShoulder, common, weight );

		leftLegIK.UpdatePose( instance, output, m_leftLegIK, weight, timeDelta );
		rightLegIK.UpdatePose( instance, output, m_rightLegIK, weight, timeDelta );

		neck1MaintainLook.UpdatePose( instance, output, m_neck1MaintainLook, common, weight );
		neck2MaintainLook.UpdatePose( instance, output, m_neck2MaintainLook, common, weight );
		headMaintainLook.UpdatePose( instance, output, m_headMaintainLook, common, weight );
	}

	Vector upWS = Vector::EZ;

	if ( headMaintainLook.m_bone != INDEX_NONE )
	{
		CWorld* world = instance.GetAnimatedComponent()->GetWorld();

		CPhysicsWorld* physicsWorld = nullptr;
		if ( world->GetPhysicsWorld( physicsWorld ) )
		{
			Vector leftHit = leftLeg.m_hitLocWS;
			Vector rightHit = rightLeg.m_hitLocWS;
			Vector headHit;
			Bool hit;
			Vector normalWS;
			Bool canCacheResult;
			TraceFootStep( physicsWorld, AnimVectorToVector( output.GetBoneWorldTransform( instance.GetAnimatedComponent(), headMaintainLook.m_bone ).GetTranslation() ), Vector::EZ, 1.0f, -5.0f, hit, headHit, normalWS, canCacheResult );
			if ( hit )
			{
				Vector back = ( leftHit + rightHit ) * 0.5f;
				Vector back2front = headHit - back;
				upWS = Vector::Cross( common.m_localToWorld.GetAxisX(), back2front );
				if ( upWS.Z < 0.0f )
				{
					// in case we would flip
					upWS = -upWS;
				}
				upWS.Z = Max( 0.3f, upWS.Z ); // block flipping
				upWS.Normalize3();
			}

#ifdef DEBUG_FLOOR_IK
			if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( leftLeg.m_testVarID ) > 0.0f )
			{
				Float usePlane = instance.GetInternalFloatValue( leftLeg.m_testVarUsePlaneID );
				if ( usePlane > 0.0f )
				{
					Vector planeNormal = instance.GetInternalVectorValue( leftLeg.m_testVarPlaneNormalID );
					if (planeNormal.Mag3() < 0.01)
					{
						planeNormal = Vector::EZ;
					}
					upWS = planeNormal;
				}
			}
#endif
		}
	}

	common.SetRequestedUp( upWS );
}

void CBehaviorConstraintNodeFloorIKBipedLong::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	Float weight = instance[ i_weightHandler ].GetCurrentWeight();
	const SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		weight = instance.GetInternalFloatValue( common.m_testVarID );
	}

	Vector debugLoc = instance[ i_pelvis ].m_debugVerticalBoneLocWS + Vector(0.0f, 0.0f, 0.4f);
	frame->AddDebugText( debugLoc, String::Printf( TXT("IK: %.3f (t:%.0f)"), weight, instance[ i_common ].m_additionalBlendCoefOnTeleport ), 0, 0, true,
		Color( (Uint8)( 255.0f * (1.0f - weight) ), (Uint8)( 255.0f * weight ), 100),
		Color(0,0,0,128) );
#endif
	if ( weight > 0.0f )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( m_generateEditorFragmentsForIKSolvers )
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightLegIK ].OnGenerateFragments( instance, frame );
			}
		}
#ifdef DEBUG_FLOOR_IK
		else
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 )
			{
				frame->AddDebugText( debugLoc, String::Printf( TXT("aor: %.6f"), common.m_adjustOffsetByRaw ), 0, 2, true, Color(100,100,255), Color(0,0,0,128) );
				frame->AddDebugText( debugLoc, String::Printf( TXT("aov: %.6f"), common.m_adjustOffsetByVel ), 0, 4, true, Color(100,100,255), Color(0,0,0,128) );
				instance[ i_pelvis ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightLeg ].OnGenerateFragments( instance, frame, common );
			}
		}
#endif
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIKQuadruped::CBehaviorConstraintNodeFloorIKQuadruped()
{
	m_speedForFullyPerpendicularLegs = 2.5f; // for dog
	m_upDirFromFrontAndBackLegsDiffCoef = 1.0f;
	m_upDirUseFrontAndBackLegsDiff = 0.7f;
	m_upDirAdditionalWS = Vector::ZEROS;
	// default setup for dog/cat
	m_common.m_gravityCentreBone = CNAME( spine2 );
	m_legs.m_maxAngleOffUprightNormal = 25.0f;
	m_legs.m_maxAngleOffUprightNormalSide = 15.0f;
	m_legs.m_offsetRange = Vector2( -0.1f, 0.5f );
	m_pelvis.m_bone = CNAME( pelvis );
	m_pelvis.m_stiffness = 0.95f;
	m_pelvis.m_offsetRange = Vector2( -0.5f, 0.5f );
	m_leftBackLegIK.m_upperBone.m_name = CNAME( l_thigh );
	m_leftBackLegIK.m_jointBone.m_name = CNAME( l_shin );
	m_leftBackLegIK.m_lowerBone.m_name = CNAME( l_foot );
	m_rightBackLegIK.m_upperBone.m_name = CNAME( r_thigh );
	m_rightBackLegIK.m_jointBone.m_name = CNAME( r_shin );
	m_rightBackLegIK.m_lowerBone.m_name = CNAME( r_foot );
	m_leftFrontLegIK.m_upperBone.m_name = CNAME( l_bicep );
	m_leftFrontLegIK.m_jointBone.m_name = CNAME( l_forearm );
	m_leftFrontLegIK.m_lowerBone.m_name = CNAME( l_hand );
	m_rightFrontLegIK.m_upperBone.m_name = CNAME( r_bicep );
	m_rightFrontLegIK.m_jointBone.m_name = CNAME( r_forearm );
	m_rightFrontLegIK.m_lowerBone.m_name = CNAME( r_hand );
	m_leftFrontShoulder.m_bone = CNAME( l_shoulder );
	m_leftFrontShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_leftFrontShoulder.m_stiffness = 0.90f;
	m_rightFrontShoulder.m_bone = CNAME( r_shoulder );
	m_rightFrontShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_rightFrontShoulder.m_stiffness = 0.90f;
	m_leftBackShoulder.m_bone = CNAME( l_thigh );
	m_leftBackShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_leftBackShoulder.m_stiffness = 0.5f;
	m_rightBackShoulder.m_bone = CNAME( r_thigh );
	m_rightBackShoulder.m_offsetRange = Vector2( -0.1f, 0.1f );
	m_rightBackShoulder.m_stiffness = 0.5f;
	m_neck1MaintainLook.m_bone = CNAME( neck );
	m_neck1MaintainLook.m_amountOfRotation = 0.3f;
	m_neck2MaintainLook.m_bone = CNAME( neck2 );
	m_neck2MaintainLook.m_amountOfRotation = 0.35f;
	m_headMaintainLook.m_bone = CNAME( head );
	m_headMaintainLook.m_amountOfRotation = 0.2f;
}

void CBehaviorConstraintNodeFloorIKQuadruped::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT( "pelvis" ) ||
		 property->GetName() == TXT( "leftBackLegIK" ) ||
		 property->GetName() == TXT( "rightBackLegIK" ) ||
		 property->GetName() == TXT( "leftFrontLegIK" ) ||
		 property->GetName() == TXT( "rightFrontLegIK" ) ||
		 property->GetName() == TXT( "leftFrontLegRotIK" ) ||
		 property->GetName() == TXT( "rightFrontLegRotIK" ) )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		m_forceSetup = true;
#endif
	}
}

void CBehaviorConstraintNodeFloorIKQuadruped::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftFrontLeg;
	compiler << i_rightFrontLeg;
	compiler << i_leftBackLeg;
	compiler << i_rightBackLeg;
	compiler << i_legs;
	compiler << i_pelvis;
	compiler << i_leftFrontShoulder;
	compiler << i_rightFrontShoulder;
	compiler << i_leftBackShoulder;
	compiler << i_rightBackShoulder;
	compiler << i_neck1MaintainLook;
	compiler << i_neck2MaintainLook;
	compiler << i_headMaintainLook;
	compiler << i_leftFrontLegIK;
	compiler << i_rightFrontLegIK;
	compiler << i_leftBackLegIK;
	compiler << i_rightBackLegIK;
	compiler << i_leftFrontLegRotIK;
	compiler << i_rightFrontLegRotIK;
	compiler << i_usePerpendicularUprightWS;
	compiler << i_frontBackWeightHandler;
	compiler << i_legsIKWeightHandler;
}

void CBehaviorConstraintNodeFloorIKQuadruped::Setup( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::Setup( instance );
	instance[ i_common ].m_cachedStepOffset = 5.0f; // almost always try to match terrain
	instance[ i_legs ].Setup( instance, m_legs );
	instance[ i_pelvis ].Setup( instance, m_pelvis );
	instance[ i_leftFrontShoulder ].Setup( instance,  m_leftFrontShoulder );
	instance[ i_rightFrontShoulder ].Setup( instance, m_rightFrontShoulder );
	instance[ i_leftBackShoulder ].Setup( instance,   m_leftBackShoulder );
	instance[ i_rightBackShoulder ].Setup( instance,  m_rightBackShoulder );
	instance[ i_neck1MaintainLook].Setup( instance, m_neck1MaintainLook );
	instance[ i_neck2MaintainLook].Setup( instance, m_neck2MaintainLook );
	instance[ i_headMaintainLook].Setup( instance, m_headMaintainLook );
	instance[ i_leftBackLegIK ].Setup( instance, m_leftBackLegIK );
	instance[ i_rightBackLegIK ].Setup( instance, m_rightBackLegIK );
	instance[ i_leftFrontLegIK ].Setup( instance, m_leftFrontLegIK );
	instance[ i_rightFrontLegIK ].Setup( instance, m_rightFrontLegIK );
	instance[ i_leftFrontLegRotIK ].Setup( instance, m_leftFrontLegRotIK );
	instance[ i_rightFrontLegRotIK ].Setup( instance, m_rightFrontLegRotIK );
	instance[ i_leftBackLeg ].Setup( instance, instance[ i_leftBackLegIK ] );
	instance[ i_rightBackLeg ].Setup( instance, instance[ i_rightBackLegIK ] );
	instance[ i_leftFrontLeg ].Setup( instance, instance[ i_leftFrontLegIK ] );
	instance[ i_rightFrontLeg ].Setup( instance, instance[ i_rightFrontLegIK ] );
	instance[ i_usePerpendicularUprightWS ] = 0.0f;
#ifdef DEBUG_FLOOR_IK
	instance[ i_leftBackLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testBackLeft ), CNAME( testBackLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightBackLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testBackRight ), CNAME( testBackRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_leftFrontLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testFrontLeft ), CNAME( testFrontLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightFrontLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testFrontRight ), CNAME( testFrontRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
#endif
	if ( CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponentUnsafe() ) )
	{
		mac->AccessAnimationProxy().SetPelvisBoneIndex( instance[ i_pelvis ].m_bone );
	}
}

void CBehaviorConstraintNodeFloorIKQuadruped::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( LegsIKOff ) )->RegisterHandler( &instance[ i_legsIKWeightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( FrontIKOff ) )->RegisterHandler( &instance[ i_frontBackWeightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( BackIKOff ) )->RegisterHandler( &instance[ i_frontBackWeightHandler ] );
}

void CBehaviorConstraintNodeFloorIKQuadruped::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( LegsIKOff ) )->UnregisterHandler( &instance[ i_legsIKWeightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( FrontIKOff ) )->UnregisterHandler( &instance[ i_frontBackWeightHandler ] );
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( CNAME( BackIKOff ) )->UnregisterHandler( &instance[ i_frontBackWeightHandler ] );
}

void CBehaviorConstraintNodeFloorIKQuadruped::UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	Float prevTimeDelta = instance[ i_prevTimeDelta ];
	Float & usePerpendicularUprightWS = instance[ i_usePerpendicularUprightWS ];

	DEBUG_ANIM_FLOAT( timeDelta );
	DEBUG_ANIM_FLOAT( prevTimeDelta );
	DEBUG_ANIM_FLOAT( usePerpendicularUprightWS );

	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];

	Vector deltaTranslation = common.m_localToWorld.GetTranslation() - common.m_prevLocalToWorld.GetTranslation();
	DEBUG_ANIM_VECTOR( deltaTranslation );

	Float speed = prevTimeDelta > MIN_TIME_DELTA ? deltaTranslation.Mag2() / prevTimeDelta : 0.f;
	
	DEBUG_ANIM_FLOAT( speed );
	DEBUG_ANIM_FLOAT( m_speedForFullyPerpendicularLegs );

	Float newUsePerpendicularUprightWS = Clamp( speed / Max(0.001f, m_speedForFullyPerpendicularLegs ), 0.0f, 1.0f );
	DEBUG_ANIM_FLOAT( newUsePerpendicularUprightWS );

	usePerpendicularUprightWS = BlendToWithBlendTime( usePerpendicularUprightWS, newUsePerpendicularUprightWS, 0.2f, timeDelta + common.m_additionalBlendCoefOnTeleport );
	DEBUG_ANIM_FLOAT( usePerpendicularUprightWS );

	DEBUG_ANIM_VECTOR( common.m_localToWorldInverted );
	DEBUG_ANIM_VECTOR( common.m_requestedUpWS );

	common.SetUprightNormalMS( Vector::EZ, common.m_localToWorldInverted.TransformVector( common.m_requestedUpWS ) * usePerpendicularUprightWS + Vector::EZ * ( 1.0f - usePerpendicularUprightWS ) );
	//common.SetUprightNormalRS( Vector::EZ, Vector::EZ * usePerpendicularUprightWS + common.m_rootSpaceWSInverted.TransformVector( Vector::EZ ) * ( 1.0f - usePerpendicularUprightWS ) );
}

void CBehaviorConstraintNodeFloorIKQuadruped::UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const
{
	DEBUG_ANIM_POSES( output );

	Float timeDelta = instance[ i_timeDelta ];
	
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	SBehaviorConstraintNodeFloorIKLegs& legs = instance[ i_legs ];
	SBehaviorConstraintNodeFloorIKVerticalBone& pelvis = instance[ i_pelvis ];
	SBehaviorConstraintNodeFloorIKVerticalBone& leftFrontShoulder = instance[ i_leftFrontShoulder ];
	SBehaviorConstraintNodeFloorIKVerticalBone& rightFrontShoulder = instance[ i_rightFrontShoulder ];
	SBehaviorConstraintNodeFloorIKVerticalBone& leftBackShoulder = instance[ i_leftBackShoulder ];
	SBehaviorConstraintNodeFloorIKVerticalBone& rightBackShoulder = instance[ i_rightBackShoulder ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& neck1MaintainLook = instance[ i_neck1MaintainLook ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& neck2MaintainLook = instance[ i_neck2MaintainLook ];
	SBehaviorConstraintNodeFloorIKMaintainLookBone& headMaintainLook = instance[ i_headMaintainLook ];
	SBehaviorConstraintNodeFloorIKLeg& leftBackLeg = instance[ i_leftBackLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightBackLeg = instance[ i_rightBackLeg ];
	SBehaviorConstraintNodeFloorIKLeg& leftFrontLeg = instance[ i_leftFrontLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightFrontLeg = instance[ i_rightFrontLeg ];
	STwoBonesIKSolver& leftBackLegIK = instance[ i_leftBackLegIK ];
	STwoBonesIKSolver& rightBackLegIK = instance[ i_rightBackLegIK ];
	STwoBonesIKSolver& leftFrontLegIK = instance[ i_leftFrontLegIK ];
	STwoBonesIKSolver& rightFrontLegIK = instance[ i_rightFrontLegIK ];
	SApplyRotationIKSolver& leftFrontLegRotIK = instance[ i_leftFrontLegRotIK ];
	SApplyRotationIKSolver& rightFrontLegRotIK = instance[ i_rightFrontLegRotIK ];
	SBehaviorConstraintNodeFloorIKFrontBackWeightHandler& frontBackWeightHandler = instance[ i_frontBackWeightHandler ];
	SBehaviorConstraintNodeFloorIKLegsIKWeightHandler& legsIKWeightHandler = instance[ i_legsIKWeightHandler ];
#ifdef DEBUG_TWO_BONES_IK_SOLVER
	leftBackLegIK.GatherDebugData( instance, output, m_leftBackLegIK );
	rightBackLegIK.GatherDebugData( instance, output, m_rightBackLegIK );
	leftFrontLegIK.GatherDebugData( instance, output, m_leftFrontLegIK );
	rightFrontLegIK.GatherDebugData( instance, output, m_rightFrontLegIK );
#endif

	frontBackWeightHandler.Update( timeDelta );
	legsIKWeightHandler.Update( timeDelta );

	// update ik only if weight is non zero, otherwise ignore it
	if ( weight > 0.001f )
	{
		if ( prevWeight <= 0.001f )
		{
			common.Reset( instance );

			leftBackLeg.Reset( common );
			rightBackLeg.Reset( common );
			leftFrontLeg.Reset( common );
			rightFrontLeg.Reset( common );

			leftFrontShoulder.Reset();
			rightFrontShoulder.Reset();
			leftBackShoulder.Reset();
			rightBackShoulder.Reset();

			pelvis.Reset();
		}

		leftBackLeg.Update( timeDelta, context, instance, output, m_leftBackLegIK, leftBackLegIK, legs, m_legs, common );
		rightBackLeg.Update( timeDelta, context, instance, output, m_rightBackLegIK, rightBackLegIK, legs, m_legs, common );
		leftFrontLeg.Update( timeDelta, context, instance, output, m_leftFrontLegIK, leftFrontLegIK, legs, m_legs, common );
		rightFrontLeg.Update( timeDelta, context, instance, output, m_rightFrontLegIK, rightFrontLegIK, legs, m_legs, common );

		//pelvis.Update( timeDelta, leftFrontLeg, rightFrontLeg, leftBackLeg, rightBackLeg, m_pelvis, m_legs, common );
		leftFrontShoulder.Update( timeDelta, leftFrontLeg, m_leftFrontShoulder, m_legs, common );
		rightFrontShoulder.Update( timeDelta, rightFrontLeg, m_rightFrontShoulder, m_legs, common );
		leftBackShoulder.Update( timeDelta, leftBackLeg, m_leftBackShoulder, m_legs, common );
		rightBackShoulder.Update( timeDelta, rightBackLeg, m_rightBackShoulder, m_legs, common );

		leftBackLeg.SetupIK( instance, output, leftBackLegIK, m_legs, common );
		rightBackLeg.SetupIK( instance, output, rightBackLegIK, m_legs, common );
		leftFrontLeg.SetupIK( instance, output, leftFrontLegIK, m_legs, common );
		rightFrontLeg.SetupIK( instance, output, rightFrontLegIK, m_legs, common );

		leftFrontLeg.SetupRotIK( instance, output, leftFrontLegRotIK, common );
		rightFrontLeg.SetupRotIK( instance, output, rightFrontLegRotIK, common );

		//pelvis.UpdatePose( instance, output, m_pelvis, common, weight, true );
		const Float frontIkWeight = frontBackWeightHandler.GetFrontWeight() * legsIKWeightHandler.GetWeight();
		const Float backIkWeight = frontBackWeightHandler.GetBackWeight() * legsIKWeightHandler.GetWeight();

		leftFrontShoulder.UpdatePose( instance, output, m_leftFrontShoulder, common, weight * frontIkWeight );
		rightFrontShoulder.UpdatePose( instance, output, m_rightFrontShoulder, common, weight * frontIkWeight );
		leftBackShoulder.UpdatePose( instance, output, m_leftBackShoulder, common, weight * backIkWeight );
		rightBackShoulder.UpdatePose( instance, output, m_rightBackShoulder, common, weight * backIkWeight );

		leftBackLegIK.UpdatePose( instance, output, m_leftBackLegIK, weight * backIkWeight, timeDelta );
		rightBackLegIK.UpdatePose( instance, output, m_rightBackLegIK, weight * backIkWeight, timeDelta );
		leftFrontLegIK.UpdatePose( instance, output, m_leftFrontLegIK, weight * frontIkWeight, timeDelta );
		rightFrontLegIK.UpdatePose( instance, output, m_rightFrontLegIK, weight * frontIkWeight, timeDelta );

		leftFrontLegRotIK.UpdatePose( instance, output, m_leftFrontLegRotIK, weight * frontIkWeight );
		rightFrontLegRotIK.UpdatePose( instance, output, m_rightFrontLegRotIK, weight * frontIkWeight );

		neck1MaintainLook.UpdatePose( instance, output, m_neck1MaintainLook, common, weight );
		neck2MaintainLook.UpdatePose( instance, output, m_neck2MaintainLook, common, weight );
		headMaintainLook.UpdatePose( instance, output, m_headMaintainLook, common, weight );
	}

	Vector upWSFaBD = Vector::EZ; // front and back legs diff
	if ( m_upDirUseFrontAndBackLegsDiff > 0.0f )
	{
		Vector backLegs = ( leftBackLeg.m_hitLocWS + rightBackLeg.m_hitLocWS ) * 0.5f;
		backLegs.Z = Min( leftBackLeg.m_hitLocWS.Z, rightBackLeg.m_hitLocWS.Z ) * 0.3f + backLegs.Z * 0.7f;
		Vector frontLegs = ( leftFrontLeg.m_hitLocWS + rightFrontLeg.m_hitLocWS ) * 0.5f;
		frontLegs.Z = Min( leftFrontLeg.m_hitLocWS.Z, rightFrontLeg.m_hitLocWS.Z ) * 0.8f + frontLegs.Z * 0.2f;
		Vector back2front = frontLegs - backLegs;
		back2front.Z *= m_upDirFromFrontAndBackLegsDiffCoef;

		upWSFaBD = Vector::Cross( common.m_localToWorld.GetAxisX(), back2front );
		upWSFaBD.Z = Max( 0.3f, upWSFaBD.Z ); // block flipping
		upWSFaBD.Normalize3();
	}

	// blend both
	common.SetRequestedUp( upWSFaBD * m_upDirUseFrontAndBackLegsDiff + m_upDirAdditionalWS );
}

void CBehaviorConstraintNodeFloorIKQuadruped::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	Float weight = instance[ i_weightHandler ].GetCurrentWeight();
	const SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		weight = instance.GetInternalFloatValue( common.m_testVarID );
	}

	Vector debugLoc = instance[ i_pelvis ].m_debugVerticalBoneLocWS + Vector(0.0f, 0.0f, 0.4f);
	frame->AddDebugText( debugLoc, String::Printf( TXT("IK: %.3f (t:%.0f)"), weight, instance[ i_common ].m_additionalBlendCoefOnTeleport ), 0, 0, true,
		Color( (Uint8)( 255.0f * (1.0f - weight) ), (Uint8)( 255.0f * weight ), 100),
		Color(0,0,0,128) );
	#endif
	if ( weight > 0.0f )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( m_generateEditorFragmentsForIKSolvers )
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftBackLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightBackLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 2 )
			{
				instance[ i_leftFrontLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 3 )
			{
				instance[ i_rightFrontLegIK ].OnGenerateFragments( instance, frame );
			}
		}
#ifdef DEBUG_FLOOR_IK
		else
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 )
			{
				frame->AddDebugText( debugLoc, String::Printf( TXT("aor: %.6f"), common.m_adjustOffsetByRaw ), 0, 2, true, Color(100,100,255), Color(0,0,0,128) );
				frame->AddDebugText( debugLoc, String::Printf( TXT("aov: %.6f"), common.m_adjustOffsetByVel ), 0, 4, true, Color(100,100,255), Color(0,0,0,128) );
				instance[ i_pelvis ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftBackLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightBackLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 2 )
			{
				instance[ i_leftFrontLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 3 )
			{
				instance[ i_rightFrontLeg ].OnGenerateFragments( instance, frame, common );
			}
		}
#endif
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintNodeFloorIKSixLegs::CBehaviorConstraintNodeFloorIKSixLegs()
{
	m_usePerpendicularUprightWS = 0.9f; // for dog
	m_upDirAdditionalWS = Vector::ZEROS;
	m_upDirUseFromLegsHitLocs = 1.0f;
	// default setup for dog/cat
	m_common.m_gravityCentreBone = CNAME( spine2 );
	m_legs.m_maxAngleOffUprightNormal = 25.0f;
	m_legs.m_maxAngleOffUprightNormalSide = 25.0f;
	m_legs.m_offsetRange = Vector2( -0.5f, 1.0f );
	m_pelvis.m_bone = CNAME( pelvis );
	m_pelvis.m_stiffness = 0.95f;
	m_pelvis.m_offsetRange = Vector2( -0.5f, 0.5f );
	m_leftBackLegIK.m_upperBone.m_name = CNAME( l_ankle2 );
	m_leftBackLegIK.m_jointBone.m_name = CNAME( l_foot2 );
	m_leftBackLegIK.m_lowerBone.m_name = CNAME( l_toe2 );
	m_rightBackLegIK.m_upperBone.m_name = CNAME( r_ankle2 );
	m_rightBackLegIK.m_jointBone.m_name = CNAME( r_foot2 );
	m_rightBackLegIK.m_lowerBone.m_name = CNAME( r_toe2 );
	m_leftMiddleLegIK.m_upperBone.m_name = CNAME( l_ankle1 );
	m_leftMiddleLegIK.m_jointBone.m_name = CNAME( l_foot1 );
	m_leftMiddleLegIK.m_lowerBone.m_name = CNAME( l_toe1 );
	m_rightMiddleLegIK.m_upperBone.m_name = CNAME( r_ankle1 );
	m_rightMiddleLegIK.m_jointBone.m_name = CNAME( r_foot1 );
	m_rightMiddleLegIK.m_lowerBone.m_name = CNAME( r_toe1 );
	m_leftFrontLegIK.m_upperBone.m_name = CNAME( l_forearm );
	m_leftFrontLegIK.m_jointBone.m_name = CNAME( l_hand );
	m_leftFrontLegIK.m_lowerBone.m_name = CNAME( l_hand_tip );
	m_rightFrontLegIK.m_upperBone.m_name = CNAME( r_forearm );
	m_rightFrontLegIK.m_jointBone.m_name = CNAME( r_hand );
	m_rightFrontLegIK.m_lowerBone.m_name = CNAME( r_hand_tip );
}

void CBehaviorConstraintNodeFloorIKSixLegs::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT( "pelvis" ) ||
		 property->GetName() == TXT( "leftBackLegIK" ) ||
		 property->GetName() == TXT( "rightBackLegIK" ) ||
		 property->GetName() == TXT( "leftMiddleLegIK" ) ||
		 property->GetName() == TXT( "rightMiddleLegIK" ) ||
		 property->GetName() == TXT( "leftFrontLegIK" ) ||
		 property->GetName() == TXT( "rightFrontLegIK" ) )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		m_forceSetup = true;
#endif
	}
}

void CBehaviorConstraintNodeFloorIKSixLegs::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftFrontLeg;
	compiler << i_rightFrontLeg;
	compiler << i_leftMiddleLeg;
	compiler << i_rightMiddleLeg;
	compiler << i_leftBackLeg;
	compiler << i_rightBackLeg;
	compiler << i_legs;
	compiler << i_pelvis;
	compiler << i_leftFrontLegIK;
	compiler << i_rightFrontLegIK;
	compiler << i_leftMiddleLegIK;
	compiler << i_rightMiddleLegIK;
	compiler << i_leftBackLegIK;
	compiler << i_rightBackLegIK;
	compiler << i_usePerpendicularUprightWS;
}

void CBehaviorConstraintNodeFloorIKSixLegs::Setup( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::Setup( instance );
	instance[ i_common ].m_cachedStepOffset = 5.0f; // almost always try to match terrain
	instance[ i_legs ].Setup( instance, m_legs );
	instance[ i_pelvis ].Setup( instance, m_pelvis );
	instance[ i_leftBackLegIK ].Setup( instance, m_leftBackLegIK );
	instance[ i_rightBackLegIK ].Setup( instance, m_rightBackLegIK );
	instance[ i_leftMiddleLegIK ].Setup( instance, m_leftMiddleLegIK );
	instance[ i_rightMiddleLegIK ].Setup( instance, m_rightMiddleLegIK );
	instance[ i_leftFrontLegIK ].Setup( instance, m_leftFrontLegIK );
	instance[ i_rightFrontLegIK ].Setup( instance, m_rightFrontLegIK );
	instance[ i_leftBackLeg ].Setup( instance, instance[ i_leftBackLegIK ] );
	instance[ i_rightBackLeg ].Setup( instance, instance[ i_rightBackLegIK ] );
	instance[ i_leftMiddleLeg ].Setup( instance, instance[ i_leftMiddleLegIK ] );
	instance[ i_rightMiddleLeg ].Setup( instance, instance[ i_rightMiddleLegIK ] );
	instance[ i_leftFrontLeg ].Setup( instance, instance[ i_leftFrontLegIK ] );
	instance[ i_rightFrontLeg ].Setup( instance, instance[ i_rightFrontLegIK ] );
	instance[ i_usePerpendicularUprightWS ] = 0.0f;
#ifdef DEBUG_FLOOR_IK
	instance[ i_leftBackLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testBackLeft ), CNAME( testBackLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightBackLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testBackRight ), CNAME( testBackRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_leftMiddleLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testMiddleLeft ), CNAME( testMiddleLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightMiddleLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testMiddleRight ), CNAME( testMiddleRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_leftFrontLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testFrontLeft ), CNAME( testFrontLeftNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
	instance[ i_rightFrontLeg ].SetupTestVars( instance, CNAME( testFloorIK ), CNAME( testFrontRight ), CNAME( testFrontRightNormal ), CNAME( testUsePlane ), CNAME( testPlaneNormal ) );
#endif
	if ( CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( instance.GetAnimatedComponentUnsafe() ) )
	{
		mac->AccessAnimationProxy().SetPelvisBoneIndex( instance[ i_pelvis ].m_bone );
	}
}

void CBehaviorConstraintNodeFloorIKSixLegs::UpdateUprightWS( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	Float & usePerpendicularUprightWS = instance[ i_usePerpendicularUprightWS ];
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];

	usePerpendicularUprightWS = BlendToWithBlendTime( usePerpendicularUprightWS, m_usePerpendicularUprightWS, 0.2f, timeDelta + common.m_additionalBlendCoefOnTeleport );

	common.SetUprightNormalMS( Vector::EZ, common.m_localToWorldInverted.TransformVector( common.m_requestedUpWS ) * usePerpendicularUprightWS + Vector::EZ * ( 1.0f - usePerpendicularUprightWS ) );
	//common.SetUprightNormalRS( Vector::EZ, Vector::EZ * usePerpendicularUprightWS + common.m_rootSpaceWSInverted.TransformVector( Vector::EZ ) * ( 1.0f - usePerpendicularUprightWS ) );
}

void CBehaviorConstraintNodeFloorIKSixLegs::UpdateAndSampleIK( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float weight, Float prevWeight ) const
{
	Float timeDelta = instance[ i_timeDelta ];
	
	SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
	SBehaviorConstraintNodeFloorIKLegs& legs = instance[ i_legs ];
	SBehaviorConstraintNodeFloorIKVerticalBone& pelvis = instance[ i_pelvis ];
	SBehaviorConstraintNodeFloorIKLeg& leftBackLeg = instance[ i_leftBackLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightBackLeg = instance[ i_rightBackLeg ];
	SBehaviorConstraintNodeFloorIKLeg& leftMiddleLeg = instance[ i_leftMiddleLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightMiddleLeg = instance[ i_rightMiddleLeg ];
	SBehaviorConstraintNodeFloorIKLeg& leftFrontLeg = instance[ i_leftFrontLeg ];
	SBehaviorConstraintNodeFloorIKLeg& rightFrontLeg = instance[ i_rightFrontLeg ];
	STwoBonesIKSolver& leftBackLegIK = instance[ i_leftBackLegIK ];
	STwoBonesIKSolver& rightBackLegIK = instance[ i_rightBackLegIK ];
	STwoBonesIKSolver& leftMiddleLegIK = instance[ i_leftMiddleLegIK ];
	STwoBonesIKSolver& rightMiddleLegIK = instance[ i_rightMiddleLegIK ];
	STwoBonesIKSolver& leftFrontLegIK = instance[ i_leftFrontLegIK ];
	STwoBonesIKSolver& rightFrontLegIK = instance[ i_rightFrontLegIK ];

#ifdef DEBUG_TWO_BONES_IK_SOLVER
	leftBackLegIK.GatherDebugData( instance, output, m_leftBackLegIK );
	rightBackLegIK.GatherDebugData( instance, output, m_rightBackLegIK );
	leftMiddleLegIK.GatherDebugData( instance, output, m_leftMiddleLegIK );
	rightMiddleLegIK.GatherDebugData( instance, output, m_rightMiddleLegIK );
	leftFrontLegIK.GatherDebugData( instance, output, m_leftFrontLegIK );
	rightFrontLegIK.GatherDebugData( instance, output, m_rightFrontLegIK );
#endif

	// update ik only if weight is non zero, otherwise ignore it
	if ( weight > 0.001f )
	{
		if ( prevWeight <= 0.001f )
		{
			common.Reset( instance );

			leftBackLeg.Reset( common );
			rightBackLeg.Reset( common );
			leftMiddleLeg.Reset( common );
			rightMiddleLeg.Reset( common );
			leftFrontLeg.Reset( common );
			rightFrontLeg.Reset( common );

			pelvis.Reset();
		}

		leftBackLeg.Update( timeDelta, context, instance, output, m_leftBackLegIK, leftBackLegIK, legs, m_legs, common );
		rightBackLeg.Update( timeDelta, context, instance, output, m_rightBackLegIK, rightBackLegIK, legs, m_legs, common );
		leftMiddleLeg.Update( timeDelta, context, instance, output, m_leftMiddleLegIK, leftMiddleLegIK, legs, m_legs, common );
		rightMiddleLeg.Update( timeDelta, context, instance, output, m_rightMiddleLegIK, rightMiddleLegIK, legs, m_legs, common );
		leftFrontLeg.Update( timeDelta, context, instance, output, m_leftFrontLegIK, leftFrontLegIK, legs, m_legs, common );
		rightFrontLeg.Update( timeDelta, context, instance, output, m_rightFrontLegIK, rightFrontLegIK, legs, m_legs, common );

		pelvis.Update( timeDelta, leftFrontLeg, rightFrontLeg, leftBackLeg, rightBackLeg, m_pelvis, m_legs, common );

		leftBackLeg.SetupIK( instance, output, leftBackLegIK, m_legs, common );
		rightBackLeg.SetupIK( instance, output, rightBackLegIK, m_legs, common );
		leftMiddleLeg.SetupIK( instance, output, leftMiddleLegIK, m_legs, common );
		rightMiddleLeg.SetupIK( instance, output, rightMiddleLegIK, m_legs, common );
		leftFrontLeg.SetupIK( instance, output, leftFrontLegIK, m_legs, common );
		rightFrontLeg.SetupIK( instance, output, rightFrontLegIK, m_legs, common );

		pelvis.UpdatePose( instance, output, m_pelvis, common, weight, true );
		leftBackLegIK.UpdatePose( instance, output, m_leftBackLegIK, weight, timeDelta );
		rightBackLegIK.UpdatePose( instance, output, m_rightBackLegIK, weight, timeDelta );
		leftMiddleLegIK.UpdatePose( instance, output, m_leftMiddleLegIK, weight, timeDelta );
		rightMiddleLegIK.UpdatePose( instance, output, m_rightMiddleLegIK, weight, timeDelta );
		leftFrontLegIK.UpdatePose( instance, output, m_leftFrontLegIK, weight, timeDelta );
		rightFrontLegIK.UpdatePose( instance, output, m_rightFrontLegIK, weight, timeDelta );
	}

	Vector upWStt = Vector::EZ; // two triangles
	{
		Vector lb2lf = leftFrontLeg.m_hitLocWS - leftBackLeg.m_hitLocWS;
		Vector lb2rf = rightFrontLeg.m_hitLocWS - leftBackLeg.m_hitLocWS;
		Vector rb2lf = leftFrontLeg.m_hitLocWS - rightBackLeg.m_hitLocWS;
		Vector rb2rf = rightFrontLeg.m_hitLocWS - rightBackLeg.m_hitLocWS;
		Vector lup = Vector::Cross( lb2rf, lb2lf );
		Vector rup = Vector::Cross( rb2rf, rb2lf );
		// reverse if they point downwards (it's quite big assumption but we shouldn't have such situations as we don't have wall climbing spiders)
		if ( lup.Z < 0.0f )
		{
 			lup = -lup;
		}
		if ( rup.Z < 0.0f )
		{
			rup = -rup;
		}
		upWStt = ( lup + rup ).Normalized3();
	}

	// blend both
	common.SetRequestedUp( upWStt * m_upDirUseFromLegsHitLocs + Vector::EZ * ( 1.0f - m_upDirUseFromLegsHitLocs ) + m_upDirAdditionalWS );
}

void CBehaviorConstraintNodeFloorIKSixLegs::GenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	Float weight = instance[ i_weightHandler ].GetCurrentWeight();
	const SBehaviorConstraintNodeFloorIKCommon& common = instance[ i_common ];
#ifdef DEBUG_FLOOR_IK
	if ( instance.IsOpenInEditor() && instance.GetInternalFloatValue( common.m_testVarID ) > 0.0f )
	{
		weight = instance.GetInternalFloatValue( common.m_testVarID );
	}
	Vector debugLoc = instance[ i_pelvis ].m_debugVerticalBoneLocWS + Vector(0.0f, 0.0f, 0.4f);
	frame->AddDebugText( debugLoc, String::Printf( TXT("IK: %.3f (t:%.0f)"), weight, instance[ i_common ].m_additionalBlendCoefOnTeleport ), 0, 0, true,
		Color( (Uint8)( 255.0f * (1.0f - weight) ), (Uint8)( 255.0f * weight ), 100),
		Color(0,0,0,128) );
#endif
	if ( weight > 0.0f )
	{
#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( m_generateEditorFragmentsForIKSolvers )
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftBackLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightBackLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 2 )
			{
				instance[ i_leftMiddleLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 3 )
			{
				instance[ i_rightMiddleLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 4 )
			{
				instance[ i_leftFrontLegIK ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 5 )
			{
				instance[ i_rightFrontLegIK ].OnGenerateFragments( instance, frame );
			}
		}
#ifdef DEBUG_FLOOR_IK
		else
		{
			if ( m_generateEditorFragmentsForLegIndex < 0 )
			{
				frame->AddDebugText( debugLoc, String::Printf( TXT("aor: %.6f"), common.m_adjustOffsetByRaw ), 0, 2, true, Color(100,100,255), Color(0,0,0,128) );
				frame->AddDebugText( debugLoc, String::Printf( TXT("aov: %.6f"), common.m_adjustOffsetByVel ), 0, 4, true, Color(100,100,255), Color(0,0,0,128) );
				instance[ i_pelvis ].OnGenerateFragments( instance, frame );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 0 )
			{
				instance[ i_leftBackLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 1 )
			{
				instance[ i_rightBackLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 2 )
			{
				instance[ i_leftMiddleLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 3 )
			{
				instance[ i_rightMiddleLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 4 )
			{
				instance[ i_leftFrontLeg ].OnGenerateFragments( instance, frame, common );
			}
			if ( m_generateEditorFragmentsForLegIndex < 0 || m_generateEditorFragmentsForLegIndex == 5 )
			{
				instance[ i_rightFrontLeg ].OnGenerateFragments( instance, frame, common );
			}
		}
#endif
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintNodeFloorIKDebugTrace::SBehaviorConstraintNodeFloorIKDebugTrace()
: m_traceIdx( 0 )
{
	for( Vector *i = m_traceTableWS, *end = &m_traceTableWS[c_traceSize]; i != end; ++ i )
	{
		*i = Vector::ZEROS;
	}
}

void SBehaviorConstraintNodeFloorIKDebugTrace::Store( Vector const & pointWS )
{
	m_traceTableWS[ m_traceIdx ] = pointWS;
	m_traceIdx = ( m_traceIdx + 1 ) % c_traceSize;
}

void SBehaviorConstraintNodeFloorIKDebugTrace::GenerateFragments( CRenderFrame* frame, Color const & color, Float width ) const
{
	Vector prev = Vector::ZEROS;
	Int32 traceIdx = ( m_traceIdx + 1 ) % c_traceSize;
	Float alpha = 0.0f;
	Float alphaStep = 1.0f / ( (Float) c_traceSize );
	while ( traceIdx != m_traceIdx )
	{
		Color useColor = color;
		useColor.A = (Uint8) ( Clamp( alpha * (Float)(color.A), 0.0f, 255.0f) );
		Vector cur = m_traceTableWS[ traceIdx ];
		if ( cur.SquareMag3() > 0.01f && prev.SquareMag3() > 0.01f )
		{
			if ( width > 0.0f )
			{
				frame->AddDebugFatLine( prev, cur, useColor, width, true, true );
			}
			else
			{
				frame->AddDebugLine( prev, cur, useColor, true, true );
			}
		}
		prev = cur;
		traceIdx = ( traceIdx + 1 ) % c_traceSize;

		alpha += alphaStep;
	}
}

#undef INDEX_NONE
#undef MIN_TIME_DELTA

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
